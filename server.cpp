#include <vector>
#include <string>
#include <map>

#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <unistd.h>

#include "hashtable.h"

using namespace std;
const int PORT_NO = 6969;
const size_t k_max_msg = 32 << 20; // likely larger than the kernel buffer
const size_t max_nsts = 3;

#define container_of(ptr, T, member) \
    ((T *)((char *)ptr - offsetof(T, member)))

struct Buffer
{
    vector<uint8_t> arr;
    size_t data_begin = 0;
    size_t data_end = 0;
    bool empty() { return (data_begin == data_end); }
    size_t size() { return (data_end - data_begin); }
};

struct Conn
{
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    Buffer *incoming_buffer = new Buffer();

    Buffer *outgoing_buffer = new Buffer();

    ~Conn() // prevent memory leaks
    {
        delete incoming_buffer;
        delete outgoing_buffer;
    }
};

struct Response
{
    uint32_t status = 0;
    std::vector<uint8_t> data;
};

// response flags
enum
{
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2
};

// global states ie. might store more struct obj in future

static void die(string message)
{
    fprintf(stderr, "[%d] %s", errno, message.c_str());
    abort();
}

static void msg_err(string message)
{
    fprintf(stderr, "[%d] %s", errno, message.c_str());
}

static void fd_set_nb(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

static void buf_consume(Buffer *buffer, size_t sz)
{
    buffer->data_begin += sz;
    if (buffer->data_begin >= buffer->data_end)
    {
        buffer->data_begin = 0;
        buffer->data_end = 0;
        buffer->arr.clear();
    }
}

static void buf_append(Buffer *buffer, const uint8_t *to_append, size_t sz)
{
    buffer->arr.insert(buffer->arr.end(), to_append, to_append + sz);
    buffer->data_end += sz;
}

static Conn *handle_accept(int fd)
{
    struct sockaddr_in client = {};
    socklen_t sz = sizeof(client);
    int conn_fd = accept(fd, (struct sockaddr *)&client, &sz);

    if (conn_fd < 0)
        return NULL;

    uint32_t ip = client.sin_addr.s_addr;
    fprintf(stderr, "new client from %u.%u.%u.%u: %u\n",
            ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
            ntohs(client.sin_port));

    fd_set_nb(conn_fd);
    Conn *conn = new Conn();

    conn->fd = conn_fd;
    conn->want_read = true;

    return conn;
}

static void parse_req(const Conn *conn, vector<string> &cmd)
{
    Buffer *buffer = conn->incoming_buffer;

    int32_t total_len;
    uint8_t *curr = buffer->arr.data() + buffer->data_begin;
    memcpy(&total_len, curr, 4);

    uint8_t *en = curr + total_len;
    curr += 4;

    while (curr != en)
    {
        uint32_t len;
        memcpy(&len, curr, 4);

        curr += 4;
        string str(reinterpret_cast<const char *>(curr),
                   reinterpret_cast<const char *>(curr + len));

        cmd.push_back(str);
        curr += len;
    }
}

static void send_res(Buffer *buffer, const Response *res)
{
    uint32_t len = 4 + res->data.size();
    buf_append(buffer, (uint8_t *)&len, 4);
    buf_append(buffer, (uint8_t *)&(res->status), 4);
    buf_append(buffer, (uint8_t *)res->data.data(), res->data.size());
}

static struct
{
    HMap db; // top-level hashtable
} g_data;

struct Entry
{
    struct HNode node; // hashtable node
    string key;
    string val;
};

// FNV hash
static uint64_t str_hash(const uint8_t *data, size_t len)
{
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++)
    {
        h = (h + data[i]) * 0x01000193; // it is fnv prime
    }
    return h;
}

// to check whether two node with same hash are actually same
bool eq(HNode *n1, HNode *n2)
{
    Entry *e1 = container_of(n1, Entry, node);
    Entry *e2 = container_of(n2, Entry, node);
    return (e1->key == e2->key);
}

void handle_get(string _k, Response *res)
{
    Entry e = {{}, _k};
    e.node = {NULL, str_hash((uint8_t *)_k.c_str(), _k.length())};

    HNode *hnode = hm_lookup(&(g_data.db), &(e.node), &eq);
    res->status |= (hnode ? RES_OK : RES_NX);
    if (hnode)
    {
        Entry *e = container_of(hnode, Entry, node);
        res->data.assign(e->val.begin(), e->val.end());
    }
}

void handle_set(string _k, string _v, Response *res)
{
    Entry e = {{}, _k, _v};
    e.node = {NULL, str_hash((uint8_t *)_k.c_str(), _k.length())};
    HNode *hnode = hm_lookup(&(g_data.db), &(e.node), &eq);

    // heap allocation req. instead of stack
    Entry *entry = new Entry();
    entry->node = {NULL, str_hash((uint8_t *)_k.c_str(), _k.length())};
    if (hnode)
    {
        container_of(hnode, Entry, node)->val = _v;
    }
    else
    {
        entry->key = _k;
        entry->val = _v;
        hm_insert(&(g_data.db), &(entry->node));
    }

    res->data.assign(_v.begin(), _v.end());
    res->status |= RES_OK;
}

void handle_del(string _k, Response *res)
{
    Entry e = {{}, _k};
    e.node = {NULL, str_hash((uint8_t *)_k.c_str(), _k.length())};
    HNode *to_del = hm_delete(&(g_data.db), &(e.node), &eq);
    if (to_del)
    {
        to_del->next = NULL;
        delete container_of(to_del, Entry, node);
    }
    res->status |= RES_OK;
}

static void handle_req(const vector<string> &cmd, Response *res)
{
    if (cmd[0] == "GET" && cmd.size() == 2)
    {
        handle_get(cmd[1], res);
    }
    else if (cmd[0] == "SET" && cmd.size() == 3)
    {
        handle_set(cmd[1], cmd[2], res);
    }
    else if (cmd[0] == "DEL" && cmd.size() == 2)
    {
        handle_del(cmd[1], res);
    }
    else
        res->status |= RES_ERR;
}

static bool try_one_request(Conn *conn)
{

    Buffer *buffer = conn->incoming_buffer;
    if (buffer->size() < 4)
        return false;

    uint8_t *curr = buffer->arr.data() + buffer->data_begin;

    uint32_t len; // total length of req
    memcpy(&len, curr, 4);
    if (len > k_max_msg)
    {
        fprintf(stderr, "msg too long !! \n");
        conn->want_close = true;
        return false;
    }

    curr += 4;

    if (buffer->size() < len)
        return false;

    vector<string> cmd;
    parse_req(conn, cmd);

    Response *res = new Response();
    handle_req(cmd, res);

    send_res(conn->outgoing_buffer, res);
    buf_consume(buffer, size_t(len));

    return true;
}

static void handle_write(Conn *conn)
{
    Buffer *buffer = conn->outgoing_buffer;
    assert(!buffer->empty());
    ssize_t rv = write(conn->fd, buffer->arr.data() + buffer->data_begin, buffer->size());
    if (rv == -1)
    {
        if (errno != EAGAIN)
        { // write buffer is not full still error
            conn->want_close = true;
            msg_err("write() error");
        }
        return;
    }

    buf_consume(buffer, (size_t)rv);

    if (buffer->empty())
    {
        conn->want_write = false;
        conn->want_read = true;
    }
}

static void handle_read(Conn *conn)
{
    uint8_t buf[64 * 1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));

    if (rv == -1)
    {
        if (errno != EAGAIN)
        { // read buffer is not empty still error
            conn->want_close = true;
            msg_err("read() error");
        }
        return;
    }

    Buffer *buffer = conn->incoming_buffer;
    if (rv == 0)
    {
        if (buffer->empty())
        {
            fprintf(stderr, "client closed \n");
        }
        else
        {
            fprintf(stderr, "unexpected EOF");
        }
        conn->want_close = true;
        return; // want close
    }

    buf_append(buffer, buf, (size_t)rv);
    while (try_one_request(conn))
    {
    };

    if (!conn->outgoing_buffer->empty())
    {
        conn->want_write = true;
        conn->want_read = false;
        return handle_write(conn);
    }
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // to tell it is fine to use the previous instance of closed immediately

    struct sockaddr_in addr = {};

    addr.sin_addr.s_addr = htonl(0); // wildcard ip address ie. will be for all my ip like private, public etc.
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_NO);

    int res_b = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (res_b)
    {
        die("bind()");
    }

    int res_l = listen(fd, SOMAXCONN); // 4096 in linux
    if (res_l)
    {
        die("listen()");
    }

    vector<Conn *> fd2conn;   // map fd to Conn object
    vector<pollfd> poll_args; // arg to poll()

    int cnt = 0;
    while (1)
    {
        poll_args.clear();

        poll_args.push_back({fd, POLLIN, 0}); // new connection treated as "read"
        for (Conn *conn : fd2conn)
        {
            if (!conn)
                continue;
            pollfd poll_arg = {conn->fd, POLLERR, 0}; // extra caution to look for POLL_ERR
            if (conn->want_read)
            {
                poll_arg.events |= POLLIN;
            }
            if (conn->want_close)
            {
                poll_arg.events |= POLLOUT;
            }
            poll_args.push_back(poll_arg);
        }

        int rv = poll(poll_args.data(), nfds_t(poll_args.size()), -1); // data() converts into c-style address
        if (rv < 0 && errno == EINTR)
        { // background system interrupt while poll sleeps
            continue;
        }
        if (rv < 0)
        {
            die("poll()");
        }

        if (poll_args[0].revents)
        {
            if (Conn *conn = handle_accept(fd))
            {
                if (fd2conn.size() <= (size_t)conn->fd)
                {
                    fd2conn.resize(conn->fd + 1);
                }
                assert(!fd2conn[conn->fd]);
                fd2conn[conn->fd] = conn;
            }
        }

        for (int i = 1; i < poll_args.size(); i++)
        {
            pollfd pfd = poll_args[i];
            if (pfd.revents & POLLIN)
            {
                handle_read(fd2conn[pfd.fd]);
            }
            if (pfd.revents & POLLOUT)
            {
                handle_write(fd2conn[pfd.fd]);
            }

            Conn *conn = fd2conn[pfd.fd];
            if ((pfd.revents & POLLERR) || conn->want_close) // POLLERR -> HARDWARE/ PROTOCOL error
            {
                close(conn->fd);
                fd2conn[conn->fd] = NULL;
                delete (conn); // prevent memory leak
            }
        }
    }
    return 0;
}
#include <bits/stdc++.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/poll.h>

using namespace std;
const int PORT_NO = 6969;
const size_t k_max_msg = 32 << 20; 

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

    ~Conn() // prevent memory leaks by deleting pointers
    {
        delete incoming_buffer;
        delete outgoing_buffer;
    }
};

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

static bool try_one_request(Conn *conn)
{
    Buffer *buffer = conn->incoming_buffer;
    if (buffer->size() < 4)
    {
        return false;
    }
    uint32_t len;
    memcpy(&len, buffer->arr.data() + buffer->data_begin, 4);

    if (len + 4 > k_max_msg)
    {
        fprintf(stderr, "Msg too large");
        conn->want_close = true;
        return false;
    }

    if (buffer->size() < len + 4)
        return false;

    printf("client says: %.*s\n", len < 100 ? len : 100, buffer->arr.data() + buffer->data_begin + 4);

    buf_append(conn->outgoing_buffer, (const uint8_t *)&len, 4);
    buf_append(conn->outgoing_buffer, buffer->arr.data() + buffer->data_begin + 4, len);
    buf_consume(conn->incoming_buffer, (size_t)len + 4);

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

    buf_consume(conn->outgoing_buffer, (size_t)rv);

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
    // cout << "rv is " << rv << endl;
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
        // cout << "event loop: " << (cnt++) << endl;
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
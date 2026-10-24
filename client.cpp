#include <string>
#include <vector>

#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

using namespace std;
const int PORT_NO = 6969;

static void die(string message)
{
    fprintf(stderr, "[%d] %s \n", errno, message.c_str());
    abort();
}

// 0 for success, -1 failure
static int32_t read_all(int fd, char *r_buf, size_t sz)
{
    while (sz > 0)
    {
        ssize_t curr_sz = read(fd, r_buf, sz);
        if (curr_sz <= 0)
        {
            return -1;
        }
        assert(sz >= (size_t)curr_sz); // extra check ie. defensive proramming
        r_buf += curr_sz;
        sz -= (size_t)curr_sz;
    }
    return 0;
}

// 0 for success, -1 for failure
static int32_t write_all(int fd, const char *w_buf, size_t sz)
{
    while (sz > 0)
    {
        ssize_t curr_sz = write(fd, w_buf, sz);
        if (curr_sz <= 0)
        {
            return -1;
        }
        assert(sz >= (size_t)curr_sz);
        w_buf += curr_sz;
        sz -= (size_t)curr_sz;
    }

    return 0;
}

static uint32_t max_len = 4096;

static int32_t send_req(int fd, vector<string> &cmd)
{
    uint32_t total_len = 4;

    for (string str : cmd)
        total_len += (4 + (uint32_t)str.length());
    if (total_len > max_len)
        return -1;

    uint8_t w_buf[total_len] = {};
    uint32_t nsts = (uint32_t)cmd.size();

    uint8_t *curr = w_buf;
    memcpy(curr, &total_len, 4);
    curr += 4;

    for (size_t i = 0; i < nsts; i++)
    {
        uint32_t len = (uint32_t)cmd[i].length();
        memcpy(curr, &len, 4);
        memcpy(curr + 4, cmd[i].data(), len);
        curr += (4 + len);
    }

    int32_t rv = write_all(fd, (char *)w_buf, total_len);
    if (rv)
    {
        die("write_all()");
    }

    return 0;
}

static void read_res(int fd)
{
    char r_buf[max_len] = {};

    int32_t rv = read_all(fd, r_buf, 4);
    if (rv < 0)
    {
        die("read_all()");
    }

    uint32_t len;
    memcpy(&len, r_buf, 4);

    rv = read_all(fd, r_buf + 4, len);
    if (rv < 0)
    {
        die("read_all()");
    }

    uint32_t status = 0;
    memcpy(&status, &r_buf[4], 4);

    string data;
    if (len > 4)
        data.assign(r_buf + 8, r_buf + 4 + len);

    if (status & 1)
    {
        fprintf(stderr, "ERROR \n");
    }
    else if (status & 2)
    {
        fprintf(stderr, "KEY NOT FOUND \n");
    }
    else
    {
        fprintf(stderr, "SUCCESS: %s \n", data.c_str());
    }
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_NO);

    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv)
    {
        die("connect()");
    }

    vector<vector<string>> cmd = {
        {"SET", "A", "sdf"},
        {"SET", "B", "BOMBAY"},
        {"SET", "C", "CHENNAI"},
          {"GET", "A"},
          {"GET", "B"},
        //   {"GET", "C"},
          {"DEL", "A"},
          {"GET", "A"}
    };
    
    for (int i = 0; i < cmd.size(); i++)
    {
        rv = send_req(fd, cmd[i]);
        if (rv < 0)
            die("send_req()");
        read_res(fd);
    }

    close(fd);
    return 0;
}
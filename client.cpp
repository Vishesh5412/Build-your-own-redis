#include <string>
#include <vector>

#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

// #include <iostream>
// #include <ostream>

using namespace std;
const int PORT_NO = 6969;
enum
{
    TAG_NIL = 0, // nil
    TAG_ERR = 1, // error code + msg
    TAG_STR = 2, // string
    TAG_INT = 3, // int64
    TAG_DBL = 4, // double
    TAG_ARR = 5, // array
};

static void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

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
    uint32_t total_len = 0;

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

    int32_t rv = write_all(fd, (char *)w_buf, 4 + total_len);
    if (rv)
    {
        die("write_all()");
    }

    return 0;
}

static size_t read_res(int fd)
{
    uint32_t sz;
    int32_t rv = read_all(fd, (char *)&sz, 4); // read 4 bytes for total_length

    if (rv)
    {
        msg("read() errror \n");
        return 0;
    }

    char r_buf[max_len] = {};
    rv = read_all(fd, r_buf, (size_t)sz);
    if (rv)
    {
        msg("read() errror \n");
        return 0;
    }

    uint8_t TAG;
    memcpy(&TAG, r_buf, 1);

    switch (TAG)
    {
    case TAG_NIL:
        msg("(nil)");
        return 1;

    case TAG_ERR:
        if (sz < 1 + 4)
        {
            msg("bad response()");
            return 0;
        }

        uint8_t ERR_CODE;
        memcpy(&ERR_CODE, &r_buf[1], 1);

        uint32_t msg_len;
        memcpy(&msg_len, &r_buf[1 + 1], 4);

        if (sz < 1 + 1 + 4 + msg_len)
        {
            msg("bad response()");
            return 0;
        }
        printf("(err) [%d] %.*s", ERR_CODE, msg_len, &r_buf[1 + 1 + 4]);
        return 1;

    case TAG_STR:
    {
        if (sz < 1 + 4)
        {
            msg("bad response()");
            return 0;
        }

        uint32_t msg_len;
        memcpy(&msg_len, &r_buf[1], 4);
        if (sz < 1 + 4 + msg_len)
        {
            msg("bad response()");
            return 0;
        }

        printf("(str) %.*s \n", msg_len, &r_buf[1 + 4]);
        return 1;
    }

    case TAG_INT:
    {
        if (sz < 1 + 8)
        {
            msg("bad response()");
            return 0;
        }

        uint64_t val;
        memcpy(&val, &r_buf[1], 8);
        printf("(int) %ld \n", val);
    }

    default:
        printf("bad response() \n");
    }

    return 1;
}

int main(int argc, char **argv)
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

    vector<string> cmd;
    for (int i = 1; i < argc; i++)
    {
        cmd.push_back(argv[i]);
    }

    rv = send_req(fd, cmd);
    if (rv < 0)
        die("send_req()");
    read_res(fd);

    close(fd);
    return 0;
}
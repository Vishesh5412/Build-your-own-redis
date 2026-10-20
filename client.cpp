#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>

using namespace std;
const size_t MAX_MESSAGE_SIZE = 32 << 20;
const int PORT_NO = 6969;

static void die(string message)
{
    fprintf(stderr, "[%d] %s", errno, message.c_str());
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
            // cout << "sz is " << curr_sz << endl;
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

static int32_t handle_req(int fd, string message)
{
    size_t len = message.length();

    char w_buf[4 + MAX_MESSAGE_SIZE] = {};
    memcpy(w_buf, &len, 4);

    int32_t w_res = write_all(fd, w_buf, 4);
    if (w_res)
    {
        die("write_all()");
    }

    memcpy(w_buf + 4, message.c_str(), len);
    w_res = write_all(fd, w_buf + 4, len);
    if (w_res)
    {
        die("write_all()");
    }

    char r_buf[4 + MAX_MESSAGE_SIZE] = {};

    int32_t r_res = read_all(fd, r_buf, 4);
    if (r_res)
    {
        // cout << "error" << endl;
        die("read_all()");
    }

    memcpy(&len, r_buf, 4);
    if (len > MAX_MESSAGE_SIZE)
    {
        fprintf(stderr, "Too large size");
        return -1;
    }

    r_res = read_all(fd, r_buf + 4, len);
    r_buf[4 + len] = '\0';

    printf("server says: %s \n", r_buf + 4);
    return 0;
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
   
    // int pid = fork();
    // if (pid == 0) {
    //   handle_req(fd, "hello_1");
    // } else {
    //   handle_req(fd, "hello_2");
    // }
    handle_req(fd, "hello_1");
    handle_req(fd, "hello_2");
    handle_req(fd, "hello_3");
    close(fd);
    return 0;
}
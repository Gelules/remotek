#ifndef PARAMS_H
#define PARAMS_H

struct globals
{
    char *ip;
    int port;
    struct socket *sock;
    struct task_struct *thread;
};

#endif /* !PARAMS_H */

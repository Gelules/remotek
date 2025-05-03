#include <err.h>
#include <netdb.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
        return 1;

    int ret = 0;
    int sockfd = 0;
    int clientfd = 0;
    char buff[4096] = { 0 };
    ssize_t len = 0;
    struct addrinfo hints = {0};
    struct addrinfo *res = NULL;
    struct addrinfo *r = NULL;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((ret = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0)
        errx(EXIT_FAILURE, "getaddrinfo: %s", gai_strerror(ret));

    for (r = res; r; r = r->ai_next)
    {
        if ((sockfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1)
            continue;

        if (bind(sockfd, r->ai_addr, r->ai_addrlen) == 0)
            break;

        close(sockfd);
    }

    if (r == NULL)
        errx(EXIT_FAILURE, "Cannot bind the service.");

    freeaddrinfo(res);

    listen(sockfd, SOMAXCONN);

    clientfd = accept(sockfd, NULL, NULL);

    len = recv(clientfd, buff, sizeof (buff) - 1, 0);
    buff[len] = '\0';
    if (strncmp(buff, "remotek", 7) != 0)
    {
        close(sockfd);
        close(clientfd);
        return 2;
    }

    puts("Remotek connected");

    while (true)
    {
        memset(buff, 0, sizeof (buff));
        printf("$ ");
        fflush(stdout);
        len = read(0, buff, sizeof (buff) - 1);
        buff[len] = '\0';
        send(clientfd, buff, len, 0);
        memset(buff, 0, sizeof (buff));

        len = recv(clientfd, buff, sizeof (buff) - 1, 0);
        if (len == 0)
            break;

        buff[len] = '\0';
        printf("%s", buff);
    }

    close(clientfd);
    close(sockfd);

    return 0;
}

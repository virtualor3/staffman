#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "staffman.h"
#include "ui.h"

char buf[BUF_MAX_SIZE];
struct userhandle userhd = { .msg = (struct msg*)buf };
int sfd;

// void sigHander(int sig);

int main(int argc, char const* argv[])
{
    const char* ipaddr = SERVERIP;
    uint16_t port = PORT;
    //获取命令行参数
    switch (argc) {
        case 3:     sscanf(argv[2], "%hd", &port);
        case 2:     ipaddr = argv[1]; break;
        case 1:     break;
        default:    printf("need only 2 argments, but get %d!\n", argc - 1);
    }
    printf("server ip[%s:%d]\n", ipaddr, port);
    //初始化tcp套接字
    struct sockaddr_in sin = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    inet_aton(ipaddr, &sin.sin_addr);
    sfd = socket(AF_INET, SOCK_STREAM, 0);

    // signal(SIGINT, sigHander);
    CHEAK(connect(sfd, (struct sockaddr*)&sin, sizeof(sin)), -1);

    while (True) {
        do_login(sfd, &userhd);
    }
    close(sfd);
    return 0;
}

ssize_t recv_msg(int fd, struct msg* msg)
{
    int ret;
    if ((ret = recv(fd, (void*)msg, sizeof(struct msg), 0)) != sizeof(struct msg)) {
        return ret;
    }
    if (msg->size - sizeof(struct msg) == 0) {
        return sizeof(struct msg);
    }
    if ((ret = recv(fd, (void*)msg + sizeof(struct msg), msg->size - sizeof(struct msg), 0)) == -1) {
        return sizeof(struct msg);
    }
    return msg->size;
}
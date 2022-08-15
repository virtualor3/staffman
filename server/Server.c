#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#include "Server.h"
#include "staffman.h"

int tcp_init(const char *ipaddr, uint16_t port)
{
    struct sockaddr_in sin = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    inet_aton(ipaddr, &sin.sin_addr);
    int sfd = CHEAK(socket(AF_INET, SOCK_STREAM, 0), -1);
    CHEAK(bind(sfd, (struct sockaddr*)&sin, sizeof(sin)), -1);
    listen(sfd, 10);
    return sfd;
} 

char* get_local_ip(void)
{
    struct ifaddrs* ifAddrStruct = NULL;
    struct ifaddrs* tmpAddrPtr = NULL;
    struct sockaddr* sina;

    CHEAK(getifaddrs(&ifAddrStruct), -1);

    tmpAddrPtr = ifAddrStruct;

    while (tmpAddrPtr != NULL) {
        if ((sina = tmpAddrPtr->ifa_addr) != NULL) {
            if (sina->sa_family == AF_INET) {
                if (((struct sockaddr_in*)sina)->sin_addr.s_addr != inet_addr("127.0.0.1")) break;
            }
        }
        tmpAddrPtr = tmpAddrPtr->ifa_next;
    }
    freeifaddrs(ifAddrStruct);
    return inet_ntoa(((struct sockaddr_in*)sina)->sin_addr);
}

int do_tcp_accept(int sfd)
{
    struct sockaddr_in sin;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int ret = CHEAK(accept(sfd, (struct sockaddr*)&sin, &addrlen), -1);
    log("[%s:%d]: fd = %d -> Client successfully connected!\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), sfd);
    return ret;
}

ssize_t recv_msg(int fd, struct msg* msg, size_t size)
{
    int ret;
    (void)size;
    struct sockaddr_in sin;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if ((ret = recvfrom(fd, (void*)msg, sizeof(struct msg), 0, (struct sockaddr*)&sin, &addrlen)) != sizeof(struct msg)) {
        if(ret == 0){
            log("[%s:%d]: fd = %d -> Client disconnected!\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), fd);
        }
        return ret;
    }
    if (msg->size - sizeof(struct msg) == 0) {
        log("[%s:%d]: fd = %d -> %d bytes recvice.\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), fd, (uint32_t)sizeof(struct msg));
        return sizeof(struct msg);
    }
    if ((ret = recv(fd, (void*)msg + sizeof(struct msg), msg->size - sizeof(struct msg), 0)) == -1) {
        log("[%s:%d]: fd = %d -> %d bytes recvice.\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), fd, (uint32_t)sizeof(struct msg));
        return sizeof(struct msg);
    }
    log("[%s:%d]: fd = %d -> %d bytes recvice.\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), fd, msg->size);
    return msg->size;
}

ssize_t send_msg(int fd, struct msg* msg)
{
    log("[%s:%d]: fd = %d -> %d bytes send.\n", SERVERIP, PORT, fd, msg->size);
    return CHEAK(send(fd, (void*)msg, msg->size, 0), -1);
}
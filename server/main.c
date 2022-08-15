#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "staffman.h"
#include "Server.h"
#include "sql.h"

#define EPOLL_NUM	10

int sfd;
char buf[512];

static void epoll_add(int epfd, int fd);
static void epoll_del(int epfd, int fd);

int do_login(int fd, struct msg_login* msg);
void do_change(int fd, struct msg_change* msg);
void do_insert(int fd, struct msg_insert* msg);
void do_delete(int fd, struct msg_delete* msg);
void do_search_own(int fd, struct msg_search* msg);
void do_search_all(int fd, struct msg_search* msg);
void do_search_name(int fd, struct msg_search* msg);
void do_search_history(int fd, struct msg_search* msg);
void sigHander(int sig);

int main(int argc, char const* argv[])
{
    const char* ipaddr = get_local_ip();
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
    sfd = tcp_init(ipaddr, port);

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    epoll_add(epfd, sfd);

    sql_init();
    signal(SIGINT, sigHander);

    uint32_t num;
    struct epoll_event events[EPOLL_NUM];
    struct msg* msg = (struct msg*)buf;

    while (True) {
        num = epoll_wait(epfd, events, ARRAY_SIZE(events), -1);
        for (uint32_t i = 0; i < num; i++) {
            int tmpfd = events[i].data.fd;
            //tcp套接字连接
            if (tmpfd == sfd) {
                epoll_add(epfd, do_tcp_accept(sfd));
            } else {
                if (recv_msg(tmpfd, msg, sizeof(buf)) == 0) {
                    epoll_del(epfd, tmpfd);
                    continue;
                }
                switch (msg->cmdcode) {
                    case CLI_LOGIN:             do_login(tmpfd, (struct msg_login*)msg); break;
                    case CLI_LOGOUT:            log("员工%d退出...\n", ((struct msg_logout*)msg)->id); break;
                    case CLI_CHANGE:            do_change(tmpfd, (struct msg_change*)msg); break;
                    case CLI_INSERT:            do_insert(tmpfd, (struct msg_insert*)msg); break;
                    case CLI_DELETE:            do_delete(tmpfd, (struct msg_delete*)msg); break;
                    case CLI_SEARCH_OWN:        do_search_own(tmpfd, (struct msg_search*)msg); break;
                    case CLI_SEARCH_ALL:        do_search_all(tmpfd, (struct msg_search*)msg); break;
                    case CLI_SEARCH_NAME:       do_search_name(tmpfd, (struct msg_search*)msg); break;
                    case CLI_SEARCH_HISTORY:    do_search_history(tmpfd, (struct msg_search*)msg); break;
                }
            }
        }
    }
    close(sfd);
    sql_close();
    return 0;
}

static void epoll_add(int epfd, int fd)
{
    struct epoll_event epevent;
    epevent.events = EPOLLIN;
    epevent.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epevent);
}

static void epoll_del(int epfd, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void sigHander(int sig)
{
    close(sfd);
    exit(EXIT_SUCCESS);
}

int do_login(int fd, struct msg_login* msg)
{
    struct msg_ack ack;
    struct userinfo tmp;
    struct sqlhandle hd = { NULL };
    if (User_search_loginfo_with_id(&hd, msg->id, &tmp) == NULL) {
        ack.msg.cmdcode = SER_ERR;
        ack.err = ERR_NONUSER;
        ack.msg.size = sizeof(struct msg_ack);
        send_msg(fd, (struct msg*)&ack);
        return -1;
    }
    if (strcmp(tmp.password, msg->password) != 0) {
        ack.msg.cmdcode = SER_ERR;
        ack.err = ERR_PASSWD;
        ack.msg.size = sizeof(struct msg_ack);
        send_msg(fd, (struct msg*)&ack);
        return -1;
    }
    if (msg->usertype == ADMINISTRATOR && tmp.usertype != ADMINISTRATOR) {
        ack.msg.cmdcode = SER_ERR;
        ack.err = ERR_NOADMIN;
        ack.msg.size = sizeof(struct msg_ack);
        send_msg(fd, (struct msg*)&ack);
        return -1;
    }
    ack.msg.cmdcode = SER_SUCCESS;
    ack.msg.size = sizeof(struct msg);
    send_msg(fd, (struct msg*)&ack);
    log("%s%d登录成功!\n", tmp.usertype == ADMINISTRATOR ? "管理员" : "用户", msg->id);
    return 0;
}

void do_change(int fd, struct msg_change* msg)
{
    struct msg_ack ack;
    struct historylog logs;
    if(User_exist(msg->chid) != True) {
        ack.msg.size = sizeof(struct msg_ack);
        ack.msg.cmdcode = SER_ERR;
        ack.err = ERR_NONUSER;
        send_msg(fd, (struct msg*)&ack);
        log("%s%d修改失败!\n", msg->usertype == ADMINISTRATOR ? "管理员" : "用户", msg->id);
        return;
    }
    if (User_update(msg->chid, msg->key, msg->value) != 0) {
        ack.msg.size = sizeof(struct msg_ack);
        ack.msg.cmdcode = SER_ERR;
        send_msg(fd, (struct msg*)&ack);
        log("%s%d修改失败!\n", msg->usertype == ADMINISTRATOR ? "管理员" : "用户", msg->id);
        return;
    }
    ack.msg.size = sizeof(struct msg);
    ack.msg.cmdcode = SER_SUCCESS;
    send_msg(fd, (struct msg*)&ack);

    logs.id = msg->id;
    logs.time = time(NULL);
    if (msg->usertype == ADMINISTRATOR) {
        sprintf(logs.event, "管理员%d修改工号%d的%s为%s", msg->id, msg->chid, memberlst[msg->key - STAFF_BASE], msg->value);
    } else {
        sprintf(logs.event, "用户%d修改工号%d的%s为%s", msg->id, msg->chid, memberlst[msg->key - STAFF_BASE], msg->value);
    }
    if (history_insert(&logs) == 0) log("[%s]history record import success!\n", logs.event);
}

void do_insert(int fd, struct msg_insert* msg)
{
    struct msg_ack ack;
    struct historylog logs;
    if (User_insert(&msg->userinfo) != 0) {
        ack.msg.size = sizeof(struct msg_ack);
        ack.msg.cmdcode = SER_ERR;
        send_msg(fd, (struct msg*)&ack);
        log("员工%d添加失败!\n", msg->userinfo.id);
        return;
    }
    ack.msg.size = sizeof(struct msg);
    ack.msg.cmdcode = SER_SUCCESS;
    send_msg(fd, (struct msg*)&ack);
    log("员工%d添加成功!\n", msg->userinfo.id);

    logs.id = msg->id;
    logs.time = time(NULL);
    if (msg->usertype == ADMINISTRATOR) {
        sprintf(logs.event, "管理员%d添加工号%d的员工", msg->id, msg->userinfo.id);
    }
    if (history_insert(&logs) == 0) log("[%s]history record import success!\n", logs.event);
}

void do_delete(int fd, struct msg_delete* msg)
{
    struct msg_ack ack;
    struct historylog logs;
    if (User_delete(msg->chid) != 0) {
        ack.msg.size = sizeof(struct msg_ack);
        ack.msg.cmdcode = SER_ERR;
        send_msg(fd, (struct msg*)&ack);
        log("员工%d删除失败!\n", msg->chid);
        return;
    }
    ack.msg.size = sizeof(struct msg);
    ack.msg.cmdcode = SER_SUCCESS;
    send_msg(fd, (struct msg*)&ack);
    log("员工%d删除成功!\n", msg->chid);

    logs.id = msg->id;
    logs.time = time(NULL);
    if (msg->usertype == ADMINISTRATOR) {
        sprintf(logs.event, "管理员%d删除工号%d的员工", msg->id, msg->chid);
    }
    if (history_insert(&logs) == 0) log("[%s]history record import success!\n", logs.event);
}

void do_search_own(int fd, struct msg_search* msg)
{
    struct msg_userinfo tmsg;
    struct sqlhandle hd = { NULL };
    tmsg.msg.cmdcode = SER_ACK;
    tmsg.msg.size = sizeof(struct msg_userinfo);
    while (User_search_all_with_id(&hd, msg->id, &tmsg.userinfo) != NULL) {
        send_msg(fd, (struct msg*)&tmsg);
    }
    tmsg.msg.cmdcode = SER_ACKEND;
    tmsg.msg.size = sizeof(struct msg);
    send_msg(fd, (struct msg*)&tmsg);
    log("员工%d信息查找成功!\n", msg->id);
}

void do_search_all(int fd, struct msg_search* msg)
{
    struct msg_userinfo tmsg;
    struct sqlhandle hd = { NULL };
    tmsg.msg.cmdcode = SER_ACK;
    tmsg.msg.size = sizeof(struct msg_userinfo);
    while (User_search_all(&hd, &tmsg.userinfo) != NULL) {
        send_msg(fd, (struct msg*)&tmsg);
    }
    tmsg.msg.cmdcode = SER_ACKEND;
    tmsg.msg.size = sizeof(struct msg);
    send_msg(fd, (struct msg*)&tmsg);
    log("员工信息查找成功!\n");
}

void do_search_name(int fd, struct msg_search* msg)
{
    struct msg_userinfo tmsg;
    struct sqlhandle hd = { NULL };
    tmsg.msg.cmdcode = SER_ACK;
    tmsg.msg.size = sizeof(struct msg_userinfo);
    while (User_search_all_with_name(&hd, msg->name, &tmsg.userinfo) != NULL) {
        send_msg(fd, (struct msg*)&tmsg);
    }
    tmsg.msg.cmdcode = SER_ACKEND;
    tmsg.msg.size = sizeof(struct msg);
    send_msg(fd, (struct msg*)&tmsg);
    log("员工%s信息查找成功!\n", msg->name);
}

void do_search_history(int fd, struct msg_search* msg)
{
    struct msg_historylog tmsg;
    struct sqlhandle hd = { NULL };
    tmsg.msg.cmdcode = SER_ACK;
    tmsg.msg.size = sizeof(struct msg_historylog);
    while (history_search(&hd, &tmsg.log) != NULL) {
        send_msg(fd, (struct msg*)&tmsg);
    }
    tmsg.msg.cmdcode = SER_ACKEND;
    tmsg.msg.size = sizeof(struct msg);
    send_msg(fd, (struct msg*)&tmsg);
    log("历史记录查找成功!\n");
}
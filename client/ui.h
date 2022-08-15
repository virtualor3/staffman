#ifndef __UI_H__
#define __UI_H__

#include "protocol.h"

struct userhandle {
    uint32_t id;
    enum usertype usertype;
    struct msg* msg;
};

void do_login(int fd, struct userhandle* userhd);
void do_logout(int fd, struct userhandle* userhd);

void do_admin_oper(int fd, struct userhandle* userhd);
void do_admin_query(int fd, struct userhandle* userhd);
void do_admin_change(int fd, struct userhandle* userhd);
void do_admin_adduser(int fd, struct userhandle* userhd);
void do_admin_deluser(int fd, struct userhandle* userhd);
void do_admin_history(int fd, struct userhandle* userhd);

void do_normal_oper(int fd, struct userhandle* userhd);
void do_normal_query(int fd, struct userhandle* userhd);
void do_normal_change(int fd, struct userhandle* userhd);

#endif
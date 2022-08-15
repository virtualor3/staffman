#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdint.h>

#include "protocol.h"

extern char* get_local_ip(void);
extern int tcp_init(const char* ipaddr, uint16_t port);
extern int do_tcp_accept(int sfd);
ssize_t recv_msg(int sfd, struct msg* msg, size_t size);
ssize_t send_msg(int sfd, struct msg* msg);

#endif
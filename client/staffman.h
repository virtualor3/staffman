#ifndef __STAFFMAN_H__
#define __STAFFMAN_H__

#include "protocol.h"

#define True		1
#define False		0

#define BUF_MAX_SIZE    512

#define PORT    	2233
#define SERVERIP	"192.168.238.128"

#define ADMINISTRATOR 		ADMINISTRATOR
#define NORMAL 				NORMAL

#define CMD_NULL 			CMD_NULL
#define CLI_LOGIN 			CLI_LOGIN
#define CLI_LOGOUT 			CLI_LOGOUT
#define CLI_CHANGE 			CLI_CHANGE
#define CLI_SEARCH_ALL 		CLI_SEARCH_ALL
#define CLI_SEARCH_NAME 	CLI_SEARCH_NAME
#define CLI_SEARCH_HISTORY 	CLI_SEARCH_HISTORY
#define SER_ACK 			SER_ACK
#define SER_ERR 			SER_ERR

#include <errno.h>
#include <stdlib.h>

#define offsetof(member, type)	&(((type*)0)->member)
#define contain_of(ptr, member, type)	(type*)((ssize_t)ptr - offsetof(member, type))

/**
ARRAY_SIZE - get the number of elements in array @arr
@arr: array to be sized
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define _CHEAK(func, cmp) {typeof(func) ret;\
	if((ret = (func))  == cmp){\
		fprintf(stderr, "%s:%d:error: %s\n", __FILE__, __LINE__, #func);\
		if(errno) perror("error");\
		exit(EXIT_FAILURE);\
	}ret;\
}

extern ssize_t recv_msg(int fd, struct msg* msg);

#ifndef NDEBUG
 /**
 CHEAK - cheak the return of @func compare with @cmp.
		 when equal, print error massage and exit.
 @func: expr or function
 @cmp:
 */
#define CHEAK(func, cmp) (_CHEAK(func, cmp))
#define _Dbug(format, ...)	printf(format"\n", __func__, __LINE__, ##__VA_ARGS__)
#define Dbug(...)  _Dbug("%s:%d:"__VA_ARGS__)
#else
#define CHEAK(func, cmp) (func)
#define Dbug(...)
#endif

#endif
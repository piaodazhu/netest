#ifndef NETEST_H
#define NETEST_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>

#include <arpa/inet.h>

#include <sys/epoll.h>
#include "pkt_impl.h"

#define	MAX_PKT_SIZE	1024
#define	MAX_EVENTS	10000
#define SERVER_PORT	5000
#define SERVER_IP	"127.0.0.1"

#define CUR_TIME(tv, start_sec)		(((tv.tv_sec) - (start_sec)) * 1000000 + (tv.tv_usec))
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define MIN(a, b)	((a) < (b) ? (a) : (b))

enum support_proto {
	TCP,
	UDP
};

struct netest_st {
	int		fd;
	uint64_t	sendtime;
	int		msgcnt;

	int		sendlen;
	u_char		sendbuf[MAX_PKT_SIZE];
	int		recvlen;
	u_char		recvbuf[MAX_PKT_SIZE];
};


#endif

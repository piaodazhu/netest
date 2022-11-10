// this file is a stupid example
// packet format implementation can be customized by users.
// protobuf is recommanded.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "pkt_impl.h"

struct requestmsg {
	unsigned	id : 32;
	unsigned	cmd : 16;
	unsigned	opt : 16;
};

struct replymsg {
	unsigned	id : 32;
	unsigned	rcode : 16;
	unsigned	info : 16;
};

int id_gen;
int total_counter;
int error_counter;

int packets_init()
{
	id_gen = 0;
	total_counter = 0;
	error_counter = 0;
	return 0;
}

int packets_report()
{
	printf("success: [%d/%d]\n", total_counter - error_counter, total_counter);
	return 0;
}


int request_mkbuf(char *emptybuf, int buflen)
{
	if (buflen < sizeof(struct requestmsg)) {
		return -1;
	}
	struct requestmsg req;
	req.id = id_gen++;
	req.cmd = 1;
	req.opt = 15;

	char *ptr = emptybuf;
	unsigned int n_id = htonl(req.id);
	unsigned short n_cmd = htons(req.cmd);
	unsigned short n_opt = htons(req.opt);
	memcpy(ptr, &n_id, 4);
	ptr += 4;
	memcpy(ptr, &n_cmd, 2);
	ptr += 2;
	memcpy(ptr, &n_opt, 2);
	ptr += 2;
	return ptr - emptybuf;
}

int request_updatebuf(char *oldbuf, int buflen)
{
	if (buflen < sizeof(struct requestmsg)) {
		return -1;
	}
	unsigned int *id = (unsigned int *)(oldbuf + 0);
	*id = htonl(id_gen++);
	return sizeof(struct requestmsg);
}

int reply_parse(const char *buf, int buflen)
{
	if (buflen != sizeof(struct replymsg)) {
		return -1;
	}
	struct replymsg reply;
	memcpy(&reply, buf, buflen);
	reply.id = ntohl(reply.id);
	reply.rcode = ntohs(reply.rcode);
	reply.info = ntohs(reply.info);
	if (reply.rcode != 1)
		++error_counter;
	++total_counter;
	return 0;
}
// this file is a stupid example
// packet format implementation can be customized by users.
// protobuf is recommanded.

#ifndef PKT_IMPL_H
#define PKT_IMPL_H

int packets_init();

int request_mkbuf(char *emptybuf, int buflen);

int request_updatebuf(char *oldbuf, int buflen);

int reply_parse(const char *buf, int buflen);

int packets_report();

#endif
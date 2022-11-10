#include <stdio.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/epoll.h>

#define SERV_PORT 5000
#define MAXLINE 8192
#define OPEN_MAX 5000

int main(int argc, char *argv[])
{
	int i, listenfd, connfd, sockfd;
	int n, num = 0;
	ssize_t nready, efd, res;
	char buf[MAXLINE], str[INET_ADDRSTRLEN];
	socklen_t clilen = sizeof(struct sockaddr_in);

	struct sockaddr_in cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_DGRAM, 0);
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 端口复用
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	efd = epoll_create(OPEN_MAX);

	struct epoll_event tep, ep[OPEN_MAX];

	tep.events = EPOLLIN;
	tep.data.fd = listenfd;

	res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);

	for (;;)
	{
		nready = epoll_wait(efd, ep, OPEN_MAX, -1);

		for (i = 0; i < nready; i++)
		{
			if (!(ep[i].events & EPOLLIN))
				continue;

			sockfd = ep[i].data.fd;
			n = recvfrom(sockfd, buf, MAXLINE, 0, (struct sockaddr*)&cliaddr, &clilen);

			if (n == 0)
			{							
				res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
				close(sockfd);
				// printf("client[%d] closed connection\n", sockfd);
			}
			else if (n < 0)
			{
				// perror("read n < 0 error: ");
				res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL); // 摘除节点
				close(sockfd);
			}
			else
			{
				// for (i = 0; i < n; i++)
				// 	buf[i] = toupper(buf[i]);
				int ret = sendto(sockfd, buf, n, 0, (struct sockaddr*)&cliaddr, clilen);
				// printf("ret=%d\n", ret);
			}
			
		}
	}
	close(listenfd);
	close(efd);

	return 0;
}

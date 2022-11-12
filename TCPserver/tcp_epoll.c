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
	socklen_t clilen;

	struct sockaddr_in cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(listenfd, 1024);

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

			if (ep[i].data.fd == listenfd)
			{ // 判断满足事件的fd是不是lfd
				clilen = sizeof(cliaddr);
				connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

				// printf("received from %s at PORT %d\n",
				//        inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
				//        ntohs(cliaddr.sin_port));
				// printf("cfd %d---client %d\n", connfd, ++num);
				int ret = fcntl(connfd, F_SETFL, O_NONBLOCK);

				tep.events = EPOLLIN;
				tep.data.fd = connfd;
				res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
			}
			else
			{
				sockfd = ep[i].data.fd;
				n = recv(sockfd, buf, MAXLINE, 0);

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

					int ret = send(sockfd, buf, n, 0);
					// printf("ret=%d\n", ret);
				}
			}
		}
	}
	close(listenfd);
	close(efd);

	return 0;
}

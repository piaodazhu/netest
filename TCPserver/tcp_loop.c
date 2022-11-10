#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERV_PORT 5000
#define MAXLINE 8192

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
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 端口复用
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(listenfd, 100);

	while (1) {
		connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
		if (connfd <= 0) break;
		// printf("received from %s at PORT %d\n",
		// 		       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
		// 		       ntohs(cliaddr.sin_port));
		while (1) {
			n = recv(connfd, buf, MAXLINE, 0);
			// printf("n=%d\n", n);
			if (n <= 0)
				break;
			int ret = send(connfd, buf, n, 0);
			// printf("ret=%d\n", ret);
		}
		close(connfd);
	}
	return 0;
}
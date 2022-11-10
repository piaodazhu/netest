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
	int i, listenfd;
	int n, num = 0;
	ssize_t nready, efd, res;
	char buf[MAXLINE], str[INET_ADDRSTRLEN];
	socklen_t clilen = sizeof(struct sockaddr_in);

	struct sockaddr_in cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_DGRAM, 0);
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	while (1) {
		n = recvfrom(listenfd, buf, MAXLINE, 0, (struct sockaddr*)&cliaddr, &clilen);
		// printf("recv %d bytes\n", n);
		if (n <= 0) break;

		// printf("client: IPAddr = %s, Port = %d, buf = %s\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), buf);	

		n = sendto(listenfd, buf, n, 0, (struct sockaddr*)&cliaddr, clilen);
		// printf("send %d bytes\n", n);
		// perror("sendto");
		if (n <= 0) break;
	}
	close(listenfd);
	return 0;
	
}
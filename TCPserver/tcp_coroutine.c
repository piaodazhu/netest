#include <dyco/dyco_coroutine.h>

#include <arpa/inet.h>
#include <assert.h>

#define SERV_PORT 5000
#define SHARED_STACK

void server_reader(void *arg) {
	int fd = *(int *)arg;
	free(arg);
	int ret = 0;

	// printf("server_reader start\n");
	while (1) {
		char buf[1024] = {0};
		ret = dyco_recv(fd, buf, 1024, 0);
		if (ret > 0) {
			// printf("read from client: %.*s\n", ret, buf);

			ret = dyco_send(fd, buf, ret, 0);
			if (ret == -1) {
				dyco_close(fd);
				printf("server_reader send failed: fd=%d\n",fd);
				break;
			}
		} else if (ret == 0) {	
			dyco_close(fd);
			// printf("server_reader close: fd=%d\n",fd);
			break;
		} else {
			printf("server_reader recv failed: fd=%d\n",fd);
		}
	}
	dyco_close(fd);
	return;
}

void server(void *arg) {

	unsigned short port = *(unsigned short *)arg;
	free(arg);

	int fd = dyco_socket(AF_INET, SOCK_STREAM, 0);
	assert(fd > 0);

	struct sockaddr_in local, remote;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = INADDR_ANY;
	bind(fd, (struct sockaddr*)&local, sizeof(struct sockaddr_in));

	listen(fd, 1024);
	printf("listen port : %d\n", port);
	// int cnt = 0;
	while (1) {
		socklen_t len = sizeof(struct sockaddr_in);
		int cli_fd = dyco_accept(fd, (struct sockaddr*)&remote, &len);
		// int cli_fd = accept_f(fd, (struct sockaddr*)&remote, &len);
		// if (cli_fd < 0) cli_fd = dyco_accept(fd, (struct sockaddr*)&remote, &len);
		// ++cnt;
		int *client = (int*)malloc(sizeof(int));
		*client = cli_fd;
		int cid = dyco_coroutine_create(server_reader, client);
#ifndef SHARED_STACK
		dyco_coroutine_setStack(cid, NULL, DYCO_DEFAULT_STACKSIZE);
#endif		
		// if (cnt % 100 == 0) printf("cnt = %d\n", cnt);
	}
	dyco_close(fd);
	return;
}

int main() 
{
	int i = 0;
	unsigned short *port = (unsigned short*)malloc(sizeof(short));
	*port = SERV_PORT;
	int cid = dyco_coroutine_create(server, port);
#ifndef SHARED_STACK
	dyco_coroutine_setStack(cid, NULL, 4096);
#endif

	printf("[+] scheduler start running.\n");
	dyco_schedule_run();
	printf("[-] scheduler stopped.\n");

	return 0;
}

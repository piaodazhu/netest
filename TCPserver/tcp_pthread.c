#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#define SERV_PORT 5000
#define MAXLINE 8192

void *thread_function(void *arg);

int main()
{
    int listenfd, *connfdp;
    socklen_t server_len, client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    pthread_t th;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERV_PORT);
    server_len = sizeof(server_address);
    bind(listenfd, (struct sockaddr *)&server_address, server_len);

    listen(listenfd, 1024);

    while (1)
    {
        // printf("server waiting\n");
        connfdp = malloc(sizeof(int));
        client_len = sizeof(client_address);
        *connfdp = accept(listenfd, (struct sockaddr *)&client_address, &client_len);
        pthread_create(&th, NULL, thread_function, connfdp);
    }
}

void *thread_function(void *arg)
{
    int connfd = *((int *)arg);
    //     printf("Thread_function is running. Argument was %d\n", connfd);
    pthread_detach(pthread_self());
    free(arg);

    int ret;
    char buf[MAXLINE];
    while (1)
    {
        ret = recv(connfd, buf, MAXLINE, 0);
        if (ret <= 0)
            break;

        ret = send(connfd, buf, ret, 0);
        if (ret <= 0)
            break;
    }

    close(connfd);
    return NULL;
}
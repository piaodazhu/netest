#include "netest.h"
#include "cJSON.h"

enum support_proto protocol;

#define DEFAULT_CONCURRENT_NUM	10
int concurrent_num;

#define DEFAULT_TOTAL_PACKETS	100
int total_packets;

#define DEFAULT_MSGNUMPERCONN	3
int message_num_per_conn;

char *server_ip;
unsigned short server_port;

int send_pkt, recv_pkt;

int init()
{
	char fbuf[4 * 4096];
	FILE *fp = fopen("config.json", "r");
	if (fp == NULL)
		return -1;
	size_t flen = fread(fbuf, 1, 4 * 4096, fp);
	if (flen <= 0)
		return -1;
	fbuf[flen] = '\0';
	fclose(fp);
	cJSON *conf = cJSON_Parse(fbuf);
	if (conf == NULL)
		return -1;
	
	cJSON *_protocol = cJSON_GetObjectItemCaseSensitive(conf, "protocol");
	if (_protocol == NULL || !cJSON_IsString(_protocol)) {
		return -1;
	}

	int ret;

	if (strncasecmp(_protocol->valuestring, "TCP", 3) == 0) {
		protocol = TCP;
	}
	else if (strncasecmp(_protocol->valuestring, "UDP", 3) == 0) {
		protocol = UDP;
	} else {
		ret = -1;
		goto out;
	}

	cJSON *_totalPackets = cJSON_GetObjectItemCaseSensitive(conf, "totalPackets");
	if (_totalPackets == NULL || !cJSON_IsNumber(_totalPackets)) {
		total_packets = DEFAULT_TOTAL_PACKETS;
	}
	else {
		total_packets = _totalPackets->valueint;
	}

	cJSON *_concurrentNum = cJSON_GetObjectItemCaseSensitive(conf, "concurrentNum");
	if (_concurrentNum == NULL || !cJSON_IsNumber(_concurrentNum)) {
		concurrent_num = DEFAULT_CONCURRENT_NUM;
	}
	else {
		concurrent_num = _concurrentNum->valueint;
	}

	cJSON *_messegeNumPerConn = cJSON_GetObjectItemCaseSensitive(conf, "messegeNumPerConn");
	if (_messegeNumPerConn == NULL || !cJSON_IsNumber(_messegeNumPerConn)) {
		message_num_per_conn = DEFAULT_MSGNUMPERCONN;
	}
	else {
		message_num_per_conn = _messegeNumPerConn->valueint;
	}

	cJSON *_serverIP = cJSON_GetObjectItemCaseSensitive(conf, "serverIP");
	if (_serverIP == NULL || !cJSON_IsString(_serverIP)) {
		ret = -1;
		goto out;
	}

	int iplen = strlen(_serverIP->valuestring);
	server_ip = malloc(iplen + 1);
	memcpy(server_ip, _serverIP->valuestring, iplen);
	server_ip[iplen] = '\0';

	cJSON *_serverPort = cJSON_GetObjectItemCaseSensitive(conf, "serverPort");
	if (_serverPort == NULL || !cJSON_IsNumber(_serverPort)) {
		ret = -1;
		goto out;
	}
	server_port = _serverPort->valueint;
	ret = 0;
out:
	cJSON_Delete(conf);
	return ret;
}

void printconf()
{
	printf("=== configuration ===\n");
	switch(protocol) {
		case TCP: printf("protocol: TCP\n"); break;
		case UDP: printf("protocol: UDP\n"); break;
		default:
			UDP: printf("protocol: Unknown\n");
	}
	printf("server: %s:%u\n", server_ip, server_port);
	printf("total packets: %d\n", total_packets);
	printf("concurrent number: %d\n", concurrent_num);
	printf("message number per connection: %d\n", message_num_per_conn);
	printf("=====================\n");
}

int newsockfd() 
{
	int fd;
	if (protocol == TCP)
		fd = socket(AF_INET, SOCK_STREAM, 0);
	else if (protocol == UDP)
		fd = socket(AF_INET, SOCK_DGRAM, 0);
	else {
		printf("unsupported protocol!\n");
		return -1;
	}

	int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (ret == -1)
	{
		close(fd);
		return -1;
	}
	return fd;
}

int main()
{
	int ret = init();
	if (ret != 0) {
		printf("[x] invalid configuration.\n");
		return 0;
	}
	printconf();
	packets_init();

	int i;
	struct netest_st *packets = (struct netest_st*) malloc(concurrent_num * sizeof(struct netest_st));
	time_t *latency = (time_t*) malloc(total_packets * sizeof(time_t));
	time_t *timepoint = (time_t*) malloc((total_packets / concurrent_num + 1) * sizeof(time_t));

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(server_port);
	server.sin_addr.s_addr = inet_addr(server_ip);

	int epfd = epoll_create(1);
	struct epoll_event ev;

	for (i = 0; i < concurrent_num; ++i) {
		int fd = newsockfd();
		// packets[i].sendlen = snprintf(packets[i].sendbuf, MAX_PKT_SIZE, "it is a test buffer");
		// packets[i].recvlen = MAX_PKT_SIZE;
		// packets[i].sendlen = 512;
		// memset(packets[i].sendbuf, 'x', 512);
		packets[i].sendlen = request_mkbuf(packets[i].sendbuf, MAX_PKT_SIZE);

		packets[i].recvlen = MAX_PKT_SIZE;
		packets[i].msgcnt = 0;
		packets[i].fd = fd;
		ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
		ev.data.ptr = &packets[i];
		epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t start_sec = tv.tv_sec;
	for (i = 0; i < concurrent_num; ++i) {
		gettimeofday(&tv, NULL);
		packets[i].sendtime = CUR_TIME(tv, start_sec);
		ret = connect(packets[i].fd, (struct sockaddr*)&server, sizeof(server));
		++send_pkt;
		if(ret < 0) {
			if(errno != EINPROGRESS) {
				perror("connect");
				exit(EXIT_FAILURE);
			}
		}
	}

	int nevents;
	struct epoll_event events[MAX_EVENTS];
	struct netest_st *pkt;
	int done = 0;
	int startpkt = concurrent_num;
	int endpkt = 0;
	while (1) {
		nevents = epoll_wait(epfd, events, MAX_EVENTS, 3000);
		if (nevents < 0)
			assert(0);
		else if (nevents == 0)
			goto finish;

		for (i = 0; i < nevents; ++i) {
			pkt = events[i].data.ptr;

			if (events[i].events & EPOLLERR) {
				assert(0);
			}
			else if (events[i].events & EPOLLOUT) {
				request_updatebuf(pkt->sendbuf, pkt->sendlen);
				ret = send(pkt->fd, pkt->sendbuf, pkt->sendlen, 0);
				assert(ret > 0);

				events[i].events = EPOLLIN | EPOLLERR | EPOLLHUP;
				epoll_ctl(epfd, EPOLL_CTL_MOD, pkt->fd, &events[i]);
			}
			else if (events[i].events & EPOLLIN) {
				ret = recv(pkt->fd, pkt->recvbuf, pkt->recvlen, 0);
				assert(ret > 0);
				ret = reply_parse(pkt->recvbuf, ret);
				++pkt->msgcnt;
				if (pkt->msgcnt < message_num_per_conn) {
					request_updatebuf(pkt->sendbuf, pkt->sendlen);
					ret = send(pkt->fd, pkt->sendbuf, pkt->sendlen, 0);
					assert(ret > 0);
					continue;
				}
					
				// printf("%.*s\n", ret, pkt->recvbuf);
				gettimeofday(&tv, NULL);
				latency[recv_pkt++] = CUR_TIME(tv, start_sec) - pkt->sendtime;

				epoll_ctl(epfd, EPOLL_CTL_DEL, pkt->fd, NULL);
				close(pkt->fd);
				pkt->fd = newsockfd();
				pkt->msgcnt = 0;

				if (recv_pkt % concurrent_num == 1)
					timepoint[recv_pkt / concurrent_num] = CUR_TIME(tv, start_sec);
					endpkt = recv_pkt;
				
				if (done) {
					if (recv_pkt == send_pkt)
						goto finish;
					else 
						continue;
				}
				
				gettimeofday(&tv, NULL);
				pkt->sendtime = CUR_TIME(tv, start_sec);
				connect(pkt->fd, (struct sockaddr*)&server, sizeof(server));
				
				++send_pkt;
				if (send_pkt == total_packets) {
					done = 1;
				}

				events[i].events = EPOLLOUT | EPOLLERR | EPOLLHUP;
				epoll_ctl(epfd, EPOLL_CTL_ADD, pkt->fd, &events[i]);
			} else {
				assert(0);
			}
		}
	}

finish:
	double sum = 0;
	time_t max_latency, min_latency;
	for (i = startpkt; i < endpkt; i++) {
		max_latency = MAX(max_latency, latency[i]);
		min_latency = MIN(min_latency, latency[i]);
		sum += latency[i];
	}
	double avr_latency = sum * 1.0 / (endpkt - startpkt);


	double lostrate = (send_pkt - recv_pkt) * 1.0 / send_pkt;
	
	printf("send : %d\n", send_pkt);
	printf("recv : %d\n", recv_pkt);
	printf("pkt loss : %f\n", lostrate);
	printf("min_latency: %lu\n", min_latency);
	printf("max_latency: %lu\n", max_latency);
	printf("avr_latency: %f\n", avr_latency);

	time_t tspace;
	double pps = 0;
	sum = 0;
	for (i = 1; i < endpkt / concurrent_num; i++) {
		tspace = timepoint[i] - timepoint[i - 1];
		pps = concurrent_num * 1000000.0 / tspace;
		printf("pps: %f\n", pps);
		sum += pps;
	}

	double avr_pps = sum * 1.0  / (endpkt / concurrent_num - 1);
	printf("\navr_pps: %f\n", avr_pps);

	packets_report();

	for (i = 0; i < concurrent_num; ++i) {
		close(packets[i].fd);
	}
	free(packets);
	free(latency);
	free(timepoint);

	return 0;
}


#include "netest.h"
#include "cJSON.h"

enum support_proto protocol;

#define DEFAULT_CONCURRENT_NUM	10
int concurrent_num;

#define DEFAULT_TOTAL_SESSIONS	100
int total_sessions;

#define DEFAULT_MSGNUMPERCONN	3
int message_num_per_conn;

#define DEFAULT_RECORDUNIT	1000
int record_unit;

char *server_ip;
unsigned short server_port;

int started_ssn, finished_ssn;

typedef struct latency_st {
	time_t endpoint;
	time_t latency;
	int id;
	int fd;
	time_t tpoint[8];
}latency_st;

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

	cJSON *_totalSessions = cJSON_GetObjectItemCaseSensitive(conf, "totalSessions");
	if (_totalSessions == NULL || !cJSON_IsNumber(_totalSessions)) {
		total_sessions = DEFAULT_TOTAL_SESSIONS;
	}
	else {
		total_sessions = _totalSessions->valueint;
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

	cJSON *_recordUnit = cJSON_GetObjectItemCaseSensitive(conf, "recordUnit");
	if (_recordUnit == NULL || !cJSON_IsNumber(_recordUnit)) {
		record_unit = DEFAULT_RECORDUNIT;
	}
	else {
		record_unit = _recordUnit->valueint;
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
	printf("total sessions: %d\n", total_sessions);
	printf("concurrent number: %d\n", concurrent_num);
	printf("record unit: %d\n", record_unit);
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
	int reuse = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
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

	int done = 0;
	int i;
	struct netest_st *sessions = (struct netest_st*) malloc(concurrent_num * sizeof(struct netest_st));
	latency_st *latency = (latency_st*) malloc(total_sessions * sizeof(latency_st));
	time_t *timepoint = (time_t*) malloc((total_sessions / record_unit + 1) * sizeof(time_t));

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(server_port);
	server.sin_addr.s_addr = inet_addr(server_ip);

	int epfd = epoll_create(1);
	struct epoll_event ev;

	for (i = 0; i < concurrent_num; ++i) {
		int fd = newsockfd();
		sessions[i].sendlen = request_mkbuf(sessions[i].sendbuf, MAX_PKT_SIZE);

		sessions[i].recvlen = MAX_PKT_SIZE;
		sessions[i].msgcnt = 0;
		sessions[i].fd = fd;
		ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
		ev.data.ptr = &sessions[i];
		epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t start_sec = tv.tv_sec;
	for (i = 0; i < concurrent_num; ++i) {
		gettimeofday(&tv, NULL);
		sessions[i].sendtime = CUR_TIME(tv, start_sec);
		sessions[i].id = started_ssn;
		ret = connect(sessions[i].fd, (struct sockaddr*)&server, sizeof(server));
		++started_ssn;
		if (started_ssn == total_sessions) {
			done = 1;
			printf("xx: %d  (%d/%d)\n", finished_ssn / record_unit, finished_ssn, started_ssn);
		}
		if(ret < 0) {
			if(errno != EINPROGRESS) {
				perror("connect");
				exit(EXIT_FAILURE);
			}
		}
	}

	int nevents;
	struct epoll_event events[MAX_EVENTS];
	struct netest_st *session;
	
	int startpkt = concurrent_num;
	int end_ssn = 0;
	while (1) {
		nevents = epoll_wait(epfd, events, MAX_EVENTS, 6000);
		if (nevents < 0)
			assert(0);
		else if (nevents == 0)
			goto finish;

		for (i = 0; i < nevents; ++i) {
			session = events[i].data.ptr;

			if (events[i].events & EPOLLERR) {
				perror("epoll");
				assert(0);
			}
			else if (events[i].events & EPOLLOUT) {
				request_updatebuf(session->sendbuf, session->sendlen);
				ret = send(session->fd, session->sendbuf, session->sendlen, 0);
				if (ret <= 0)
				{
					printf("send ret %d, (%d/%d)\n", ret, finished_ssn, started_ssn);
				}
				if (session->msgcnt == 0) {
					gettimeofday(&tv, NULL);
					session->tpoint[0] = CUR_TIME(tv, start_sec) - session->sendtime;
				}

				events[i].events = EPOLLIN | EPOLLERR | EPOLLHUP;
				epoll_ctl(epfd, EPOLL_CTL_MOD, session->fd, &events[i]);
			}
			else if (events[i].events & EPOLLIN) {
				ret = recv(session->fd, session->recvbuf, session->recvlen, 0);
				if (ret <= 0)
				{
					printf("recv ret %d, (%d/%d)\n", ret, finished_ssn, started_ssn);
				}
				assert(ret == 8);
				ret = reply_parse(session->recvbuf, ret);
				++session->msgcnt;
				gettimeofday(&tv, NULL);
				session->tpoint[session->msgcnt] = CUR_TIME(tv, start_sec) - session->sendtime;
				if (session->msgcnt < message_num_per_conn) {
					request_updatebuf(session->sendbuf, session->sendlen);
					ret = send(session->fd, session->sendbuf, session->sendlen, 0);
					assert(ret > 0);
					continue;
				}
					
				// printf("%.*s\n", ret, pkt->recvbuf);
				gettimeofday(&tv, NULL);
				latency[finished_ssn].endpoint = CUR_TIME(tv, start_sec);
				latency[finished_ssn].latency = CUR_TIME(tv, start_sec) - session->sendtime;
				latency[finished_ssn].fd = session->fd;
				latency[finished_ssn].id = session->id;
				int j = 0;
				while (j <= session->msgcnt) {
					latency[finished_ssn].tpoint[j] = session->tpoint[j];
					++j;
				}

				++finished_ssn;
				epoll_ctl(epfd, EPOLL_CTL_DEL, session->fd, NULL);
				close(session->fd);
				session->fd = newsockfd();
				session->msgcnt = 0;

				if (finished_ssn % record_unit == 1) {
					timepoint[finished_ssn / record_unit] = CUR_TIME(tv, start_sec);
					end_ssn = finished_ssn;
				}
					
				
				if (done) {
					if (finished_ssn == started_ssn)
						goto finish;
					else 
						continue;
				}
				
				gettimeofday(&tv, NULL);
				session->sendtime = CUR_TIME(tv, start_sec);
				session->id = started_ssn;
				ret = connect(session->fd, (struct sockaddr*)&server, sizeof(server));
				if(ret < 0) {
					if(errno != EINPROGRESS) {
						perror("connect");
						exit(EXIT_FAILURE);
					}
				}
				
				++started_ssn;
				if (started_ssn == total_sessions) {
					done = 1;
				}

				events[i].events = EPOLLOUT | EPOLLERR | EPOLLHUP;
				epoll_ctl(epfd, EPOLL_CTL_ADD, session->fd, &events[i]);
			} else {
				assert(0);
			}
		}
	}

finish:
	double sum = 0;
	time_t max_latency, min_latency;
	min_latency = 1000000;
	max_latency = 0;
	for (i = startpkt; i < end_ssn; i++) {
		max_latency = MAX(max_latency, latency[i].latency);
		min_latency = MIN(min_latency, latency[i].latency);
		if (i % record_unit == 0) {
			printf("<index=%d, finishsecond=%ld, latency=%ld, fd=%d, timepoints=[", latency[i].id, latency[i].endpoint / 1000000, latency[i].latency, latency[i].fd);
			int j = 0;
			while (j <= message_num_per_conn) {
				printf("%ld, ", latency[i].tpoint[j]);
				++j;
			}
			printf("]>\n");
		}
			
		sum += latency[i].latency;
	}
	double avr_latency = sum * 1.0 / (end_ssn - startpkt);


	double failurerate = (started_ssn - finished_ssn) * 1.0 / started_ssn;
	
	printf("send : %d\n", started_ssn);
	printf("recv : %d\n", finished_ssn);
	printf("session failure rate: %f\n", failurerate);
	printf("min_latency: %lu\n", min_latency);
	printf("max_latency: %lu\n", max_latency);
	printf("avr_latency: %f\n", avr_latency);

	time_t tspace;
	double sps = 0;
	sum = 0;
	// printf("sps: \n");
	for (i = 1; i < end_ssn / record_unit; i++) {
		tspace = timepoint[i] - timepoint[i - 1];
		sps = record_unit * 1000000.0 / tspace;
		// printf("\t%d %f\n", i, sps);
		sum += sps;
	}

	double avr_sps = sum * 1.0  / (end_ssn / record_unit - 1);
	printf("avr_sps: %f\n", avr_sps);
	printf("avr_pps: %f\n", avr_sps * message_num_per_conn);

	packets_report();

	for (i = 0; i < concurrent_num; ++i) {
		close(sessions[i].fd);
	}
	free(sessions);
	free(latency);
	free(timepoint);

	return 0;
}


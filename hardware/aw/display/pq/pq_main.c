/*
* Copyright (c) 2020-2030 allwinnertech.com JetCui<cuiyuntao@allwinnertech.com>
*/
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <pthread.h>

#include "trans_header.h"

#ifndef PQ_TCP_PORT
#define PQ_TCP_PORT 5005
#endif
#define MAX_CONNECT_COUNT 4
#define MAX_SOCKET_CONNECT_COUNT 3

#define EVENT_MSG_LEN 4096
#define HEART_INTERVAL_MS 3000

int epoll_fd;
int alive_connect_thread = 0;
int con_fd[MAX_SOCKET_CONNECT_COUNT] = {-1,};
int write_fd;
int read_fd;
int can_close = 0;

#if PQ_DEBUG_LEVEL
static int debug_cout = 0;
#endif

extern void send_to_translate(struct trans_data* data);
extern int PQ_translate_init(void);

void* connect_thread(void *arg)
{
	char *recv_buf = NULL;
	int client_fd, rev_cnt, tail;
	struct PQ_package *tmp_head, tempackage;
	struct trans_data *trans_da = NULL;
	int eventCount, recvlen, i, ret;
	struct epoll_event eventItems[MAX_SOCKET_CONNECT_COUNT];
	uint32_t epollEvents;
	unsigned int timewaitems = HEART_INTERVAL_MS * 2;
	recv_buf = (char *)arg;

	PQ_Printf("creat a connect thread to deal the socket.");

	recv_buf = (char *)malloc(EVENT_MSG_LEN);
	if (recv_buf == NULL) {
		PQ_Printf("alloc err for connect_thread...\n");
		exit(-1);
	}
	memset(&tempackage, 0, sizeof(tempackage));
	memset(recv_buf, 0, EVENT_MSG_LEN);
	tmp_head = NULL;

wait_event:
	eventCount = epoll_wait(epoll_fd, eventItems, MAX_SOCKET_CONNECT_COUNT, timewaitems);
	if (eventCount == 0) {
		if (!can_close) {
			timewaitems = HEART_INTERVAL_MS * 3;
			goto wait_event;
		}
		timewaitems = -1;
		PQ_Printf("has no clients connectted, close the socket.");
		i = 0;
		while (i < MAX_SOCKET_CONNECT_COUNT) {
			if (con_fd[i] > 0) {
				ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, con_fd[i], NULL);
				if(ret) {
					PQ_Printf("%d epoll_ctl del err:%d", client_fd, ret);
				}
			}
			con_fd[i] = -1;
			i++;
		}
		alive_connect_thread = 0;
	}
	for (i = 0; i < eventCount; i++) {
		timewaitems = HEART_INTERVAL_MS * 3;
		client_fd = eventItems[i].data.fd;
        	epollEvents = eventItems[i].events;
		if (read_fd == client_fd) {
			int R[2];
			while (recv(read_fd, R, sizeof(int), MSG_DONTWAIT) >= 4) {
				if (alive_connect_thread < MAX_SOCKET_CONNECT_COUNT) {
					int t = 0;
					while((con_fd[t] > 0) && (t < MAX_SOCKET_CONNECT_COUNT)){
						t++;
					}
					if (t < MAX_SOCKET_CONNECT_COUNT && R[0] > 0) {
						con_fd[t] = R[0];
						alive_connect_thread++;
						PQ_Printf("translate a new %d connect, now:%d.",
							con_fd[t], alive_connect_thread);
					} else {
						PQ_Printf("something wronge?%d", t);
					}
				}else {
					PQ_Printf("something wronge? connecteted:%d.",
							alive_connect_thread);
				}
			}
			continue;
		}

		tmp_head = NULL;
		tail = 0;
		if (epollEvents & EPOLLIN) {
receiv:
			rev_cnt = 0;
			recvlen = recv(client_fd, recv_buf, EVENT_MSG_LEN, MSG_DONTWAIT | MSG_NOSIGNAL);
			if (recvlen == EINTR || recvlen == EWOULDBLOCK || recvlen == EAGAIN)
				goto receiv;
			if (recvlen < 0) {
				PQ_Printf("receive a err:%d form %d", recvlen, client_fd);
				continue;
			}
			if (recvlen == 0) {
				int t = 0;
				PQ_Printf("a socket %d disconnect...", client_fd);
				if (!can_close) {
					continue;
				}
				ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
				if(ret) {
					PQ_Printf("%d epoll_ctl del err:%d", client_fd, ret);
				}
				close(client_fd);
				while (t < MAX_SOCKET_CONNECT_COUNT) {
					if (con_fd[i] == client_fd) {
						alive_connect_thread--;
						con_fd[t] = -1;
					}
					t++;
				}
				continue;
			}
#if PQ_DEBUG_LEVEL

			{
				struct PQ_package *ttmp_head = NULL;
				if (recvlen >= sizeof(struct PQ_package)) {
					ttmp_head = (struct PQ_package *)(recv_buf);
					PQ_Printf("recive:%c%c%c%c cmd:%d type:%d size:%d times:%dth",
						ttmp_head->head[0], ttmp_head->head[1], ttmp_head->head[2],
						ttmp_head->head[3], ttmp_head->cmd, ttmp_head->type, ttmp_head->size,
						    debug_cout);
					if (recvlen >= sizeof(struct PQ_package) + sizeof(int)) {
						PQ_Printf("id:%d", *(int*)(recv_buf + sizeof(struct PQ_package)));
					}
				}
#if (PQ_DEBUG_LEVEL >= 2)
				int t = 0;
				printf("\npackage info[%d]:\n", debug_cout);
				while (t < recvlen) {
					printf("%02x ", *((char*)((recv_buf + rev_cnt)+t)));
					if (++t%30 == 0) {
						printf("\n");
					}
				}
				printf("\n################end\n");
#endif
				PQ_Printf("deal the %dth of %d,and buffer size:%d", i, eventCount, recvlen);
			}
#endif

many_head:
			if (tmp_head == NULL) {
				/* little-end */
				tmp_head = (struct PQ_package *)(recv_buf + rev_cnt);
				if (recvlen >= sizeof(struct PQ_package)) {
					if (tmp_head->head[0] != 'A'
					    || tmp_head->head[1] != 'W'
					    || tmp_head->head[2] != 'P'
					    || tmp_head->head[3] != 'Q') {
						PQ_Printf("not a head str:%c%c%c%c,something wrong? receive %d?:",
							tmp_head->head[0],tmp_head->head[1],
							tmp_head->head[2],tmp_head->head[3], recvlen);
						int t = 0;
						printf("\nBad package...\n###################\n");
						while (t < recvlen) {
							printf("%02x ", *((char*)((recv_buf + rev_cnt)+t)));
							if (++t%30 == 0) {
								printf("\n");
							}
						}
						printf("\n###################\n");
						trans_da = (struct trans_data*)malloc(sizeof(struct PQ_package)
								+ sizeof(struct trans_reply) + sizeof(struct trans_data));
						tmp_head = (struct PQ_package *)&trans_da->data[0];
						tmp_head->head[0] = 'A';
						tmp_head->head[1] = 'W';
						tmp_head->head[2] = 'P';
						tmp_head->head[3] = 'Q';
						tmp_head->cmd = 255;
						tmp_head->size = sizeof(struct PQ_package)
								+ sizeof(struct trans_reply);
						tmp_head = NULL;

						recvlen = 0;
						goto send_trans;
					}

					trans_da = (struct trans_data*)malloc(tmp_head->size + sizeof(struct trans_data));
					if (trans_da == NULL) {
						PQ_Printf("malloc err...");
						continue;
					}
					if (recvlen >= tmp_head->size) {
						memcpy((char *)&trans_da->data[0], recv_buf + rev_cnt, tmp_head->size);
						tmp_head = (struct PQ_package *)&trans_da->data[0];
						recvlen -= tmp_head->size;
						rev_cnt += tmp_head->size;
						goto deal_data;
					} else {
						tmp_head = (struct PQ_package *)&trans_da->data[0];
						memcpy((char *)tmp_head, recv_buf + rev_cnt, recvlen);
						tail = recvlen;
						recvlen = 0;
						rev_cnt += recvlen;
						goto receiv;
					}
				}else{
					PQ_Printf("Not a head size %d, something wrong?", recvlen);
					goto deal_data;
				}
			}

			if (tmp_head == &tempackage) {
				memcpy(((char *)(&tempackage)) + tail, recv_buf, sizeof(struct PQ_package)- tail);
				trans_da = (struct trans_data*)malloc(tmp_head->size + sizeof(struct trans_data));
				tmp_head = (struct PQ_package *)&trans_da->data[0];
				memcpy(tmp_head, &tempackage, sizeof(struct PQ_package));
				rev_cnt += sizeof(struct PQ_package)- tail;
				recvlen -= (sizeof(struct PQ_package)- tail);
				if (recvlen >= (tmp_head->size - sizeof(struct PQ_package))) {
					memcpy(tmp_head->data, recv_buf + rev_cnt,
						tmp_head->size - sizeof(struct PQ_package));
					recvlen -= (tmp_head->size - sizeof(struct PQ_package));
					rev_cnt += (tmp_head->size - sizeof(struct PQ_package));
				} else {
					memcpy(tmp_head->data, recv_buf + rev_cnt, recvlen);
					rev_cnt += recvlen;
					recvlen = 0;
				}
			} else if (tmp_head != NULL) {
				int cpy_size = 0, half = 0;
				if (recvlen + tail >= tmp_head->size) {
					cpy_size = tmp_head->size - tail;
					recvlen -= cpy_size;
				} else {
					cpy_size = rev_cnt;
					recvlen = 0;
					half = 1;
				}
				memcpy(tmp_head->data + tail, recv_buf + rev_cnt, cpy_size);
				rev_cnt += cpy_size;
				if (half) {
					goto receiv;
				}
			}
deal_data:
			if (recvlen > 0 && recvlen < sizeof(struct PQ_package)) {
				memcpy(&tempackage, tmp_head, recvlen);
				tmp_head = &tempackage;
				tail = recvlen;
				goto receiv;
			}
send_trans:
#if PQ_DEBUG_LEVEL
			trans_da->count = debug_cout++;
#endif
			trans_da->client_fd = client_fd;
			send_to_translate(trans_da);
			if (recvlen > 0) {
				trans_da = NULL;
				tmp_head = NULL;
				PQ_Printf("a buffer has seval head? or give a big buffer size");
				goto many_head;
			}
		}
	}

	goto wait_event;
}


int main(int argc, char *argv[])
{
	pthread_t pthread_handle;
	int i = 0, result = 0, pq_port = PQ_TCP_PORT;
	int sockfd;
	int client_fd;
	struct epoll_event eventItem;
	struct sockaddr_in clnt_addr;
	struct sockaddr_in serv_addr;
    	socklen_t clnt_addr_size = sizeof(clnt_addr);

	if (argc > 1) {
		pq_port = atoi(argv[1]);
	}
	memset(con_fd, -1, sizeof(int)* MAX_SOCKET_CONNECT_COUNT);
	result = PQ_translate_init();
	if (result) {
			PQ_Printf("sunxi PQ PQ_translate err...");
			return -1;
	}
	/* todo load the default config for  de */
	/* load dexxx.so  */
	epoll_fd = epoll_create(MAX_CONNECT_COUNT);
	if (epoll_fd < 0) {
		PQ_Printf("sunxi PQ creat epoll err:%d", epoll_fd);
		goto err;
	}
	int sockets[2];
	if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sockets) == 0) {
	    fcntl(sockets[0], F_SETFL, O_NONBLOCK);
	    fcntl(sockets[1], F_SETFL, O_NONBLOCK);
	} else {
	    PQ_Printf("sunxi PQ pipe creation failed (%s)", strerror(errno));
	    return -1;
	}

	write_fd = sockets[1];
	read_fd = sockets[0];
	memset(&eventItem, 0, sizeof(eventItem));
	eventItem.events = EPOLLIN;
	eventItem.data.fd = read_fd;
	result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, read_fd, &eventItem);
	if (result != 0) {
		PQ_Printf("sunxi PQ add read fd err:%d...", result);
	}

	pthread_create(&pthread_handle, NULL, connect_thread, NULL);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		PQ_Printf("sunxi PQ create socket error:%s(error:%d)",strerror(errno),errno);
		goto err;;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;;
	serv_addr.sin_addr.s_addr = htonl(0x7F000001);//inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(pq_port);
	bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

waite_connect:
	PQ_Printf("sunxi PQ wait for client connect from tcp port:%d...", pq_port);
	if (listen(sockfd, MAX_SOCKET_CONNECT_COUNT) < 0) {
		PQ_Printf("sunxi PQ connect error:%s(errno:%d)",strerror(errno),errno);
		goto err;
	}

	client_fd =  accept(sockfd,(struct sockaddr*)&clnt_addr, &clnt_addr_size);

	memset(&eventItem, 0, sizeof(eventItem));
	eventItem.events = EPOLLIN;
	eventItem.data.fd = client_fd;
	result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &eventItem);
	if (result != 0) {
		PQ_Printf("sunxi PQ epoll_ctl err:%d...", result);
	}
	send(write_fd, &client_fd, sizeof(int), MSG_DONTWAIT | MSG_NOSIGNAL);
	i++;

	PQ_Printf("sunxi PQ has connectted for %d times and new:%d...", i, client_fd);
	goto waite_connect;
err:
	return -1;
}

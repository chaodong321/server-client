#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define SERVER_PORT 8888
#define EVENT_MAX 100

void sig_pipe(int signo) {
	printf("recv signal pipe\n");
}

int handle_data(int cfd) {
	char buf[256];
	ssize_t ret;
	memset(buf, 0, sizeof(buf));
	ret = recv(cfd, buf, sizeof(buf), 0);
	if (ret <= 0) {
		if (ret < 0) {
			printf("recv fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		}
		return ret;
	}
	printf("recv len: %lu, buffer: %s\n", strlen(buf), buf);
	ret = send(cfd, buf, strlen(buf), 0);
	if (ret <= 0) {
		if (ret < 0) {
			printf("send fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		}
	}
	return ret;
}

int main(int argc, char **argv) {
	signal(SIGPIPE, sig_pipe);
	int ret;
	int	sfd, cfd, efd, nfds;
	struct sockaddr_in server_addr, client_addr;
	struct epoll_event ev, events[EVENT_MAX];

	sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sfd < 0) {
		printf("create socket fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		printf("bind fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		goto ERR;
	}

	ret = listen(sfd, 128);
	if (ret < 0) {
		printf("listen fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		goto ERR;
	}

	efd = epoll_create(1);
	if (efd < 0) {
		printf("epoll create fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		goto ERR;
	}

	ev.events = EPOLLIN;
	ev.data.fd = sfd;
	ret = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev);
	if (ret < 0) {
		printf("epoll ctl add listen fd fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		goto EPOLL_ERROR;
	}

	while(1) {
		nfds = epoll_wait(efd, events, EVENT_MAX, -1);
		if (nfds < 0) {
			printf("epoll wait fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
			goto EPOLL_ERROR;
		}

		for (int i = 0; i < nfds; i++) {
			if (events[i].data.fd == sfd) {
				socklen_t client_addr_len;
				cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);

				if (cfd < 0) {
					printf("accept fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
					goto ERR;
				}
				printf("client address: %s, port: %d\n", inet_ntoa(client_addr.sin_addr), 
						ntohs(client_addr.sin_port));

				//设置为非阻塞
				fcntl(cfd, F_SETFL, O_NONBLOCK);
				
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = cfd;
				ret = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev);
				if (ret < 0) {
					printf("epoll ctl add conn fd fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
					goto EPOLL_ERROR;
				}
			} else {
				ret = handle_data(events[i].data.fd);
				if (ret <= 0) {
					ret = epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
					if (ret < 0 ) {
						goto EPOLL_ERROR;
					}
					printf("close client fd: %d\n", events[i].data.fd);
					close(events[i].data.fd);
				}
			}
		}
	}

EPOLL_ERROR:
	close(efd);

ERR:
	close(sfd);
	return 0;
}

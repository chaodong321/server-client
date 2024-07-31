#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888

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
	int	sfd, cfd;
	struct sockaddr_in server_addr;
	
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

	fd_set rfds, rsets;
	FD_ZERO(&rfds);
	FD_SET(sfd, &rfds);

	int max_fd = sfd;
	
	int nready;
	while(1) {
		printf("max fd: %d, select...\n", max_fd);
		
		rsets = rfds;
	
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		
		nready = select(max_fd+1, &rsets, NULL, NULL, &tv);
		if (nready < 0) {
			printf("select fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
			goto ERR;
		}

		if (nready == 0) {
			//select超时
			continue;
		}

		if FD_ISSET(sfd, &rsets) {
			struct sockaddr_in client_addr;
			socklen_t client_addr_len = sizeof(client_addr);
			cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);

			if (cfd < 0) {
				printf("accept fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
				goto ERR;
			}
			printf("client address: %s, port: %d\n", inet_ntoa(client_addr.sin_addr), 
				ntohs(client_addr.sin_port));

			FD_SET(cfd, &rfds);
			if (cfd > max_fd) {
				max_fd = cfd;
			}

			if (--nready == 0) {
				continue;
			}
		}

		for(int i=sfd+1; i <=max_fd; i++) {
			if (FD_ISSET(i, &rsets)) {
				//接收
				ret = handle_data(i);
				if (ret <= 0) {
					printf("client fd %d close\n", i);
					FD_CLR(i, &rfds);
					close(cfd);
				}

				if (--nready == 0) {
					break;
				}
			}
		}
	}

ERR:
	close(sfd);
	return 0;
}


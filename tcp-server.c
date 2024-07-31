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

int main(int argc, char **argv) {
	signal(SIGPIPE, sig_pipe);
	int ret;
	int	sfd, cfd;
	struct sockaddr_in server_addr, client_addr;
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

	socklen_t client_addr_len;
	while(1) {
		cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);

		if (cfd < 0) {
			printf("accept fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
			goto ERR;
		}
		printf("client address: %s, port: %d\n", inet_ntoa(client_addr.sin_addr), 
				ntohs(client_addr.sin_port));

		char buf[256];
		ssize_t recv_bytes, send_bytes;
		while(1) {
			memset(buf, 0, sizeof(buf));
			recv_bytes = recv(cfd, buf, sizeof(buf), 0);
			if (recv_bytes <= 0) {
				if (recv_bytes < 0) {
					printf("recv fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
				}
				goto CLIENT_CLOSE;
			}

			printf("recv len: %lu, buffer: %s\n", strlen(buf), buf);
			send_bytes = send(cfd, buf, strlen(buf), 0);
			if (send_bytes <= 0) {
				if (recv_bytes < 0) {
					printf("send fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
				}
				goto CLIENT_CLOSE;
			}
		}
CLIENT_CLOSE:
		close(cfd);
	}
ERR:
	close(sfd);
	return 0;
}


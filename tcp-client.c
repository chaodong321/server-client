#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_ADDRESS "10.30.12.21"
#define SERVER_PORT 8888

int main(int argc, char **argv) {
	int ret;
	int cfd;
	struct sockaddr_in server_addr;
	cfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (cfd < 0) {
		printf("create socket fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

	//建立有连接udp
	ret = connect(cfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		printf("connect fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		goto ERR;
	}

	char buf[256];
	ssize_t send_bytes, recv_bytes;

	while(1) {
		memset(buf, 0, sizeof(buf));
		printf("send buffer: ");
		scanf("%s", buf);
		send_bytes = send(cfd, buf, strlen(buf), 0);
		if (send_bytes <= 0) {
			if (send_bytes < 0) {
				printf("send fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
			}
			goto ERR;
		}

		memset(buf, 0, sizeof(buf));
		recv_bytes = recv(cfd, buf, sizeof(buf), 0);
		if (recv_bytes <= 0) {
			if (recv_bytes < 0) {
				printf("recv fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
			}
			goto ERR;
		}
		printf("recv buffer: %s\n", buf);
	}
ERR:
	close(cfd);
	return 0;
}

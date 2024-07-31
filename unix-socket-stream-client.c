#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/test_stream.sock"

#define handle_error(msg) \
	do {perror(msg); exit(EXIT_FAILURE);} while(0)

int main(int argc, char **argv) {
	int cfd;
	struct sockaddr_un  server_addr;

	cfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd == -1) {
		handle_error("socket");
	}

	memset(&server_addr, 0, sizeof(struct sockaddr_un));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, SOCK_PATH, sizeof(server_addr.sun_path) - 1);

	char buf[256];
	ssize_t send_size, recv_size;
	socklen_t server_addr_len = sizeof(server_addr);
	
	if(connect(cfd, (struct sockaddr *)&server_addr, server_addr_len) == -1) {
		handle_error("connect");
	}

	while(1) {
		memset(buf, 0, sizeof(buf));
		printf("send buffer: ");
		scanf("%s", buf);
		send_size = send(cfd, buf, strlen(buf), 0);
		//send_size = sendto(cfd, buf, strlen(buf), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un));
		if (send_size < 0) {
			handle_error("sendto");
		}
		if (send_size == 0) {
			goto EXIT;
		}

		recv_size = recv(cfd, buf, sizeof(buf), 0);
		//recv_size = recvfrom(cfd, buf, sizeof(buf), 0, (struct sockaddr *)&server_addr, &server_addr_len);
		if (recv_size < 0) {
			handle_error("recvfrom");
		}
		if (recv_size == 0) {
			goto EXIT;
		}
		printf("recv buff: %s\n", buf);
	}
EXIT:
	close(cfd);
	return 0;
}



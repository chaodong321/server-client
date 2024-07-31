#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/test_dgram.sock"

#define handle_error(msg) \
	do {perror(msg); exit(EXIT_FAILURE);} while(0)

int main(int argc, char **argv) {
	int sfd;
	struct sockaddr_un  server_addr, client_addr;

	sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sfd == -1) {
		handle_error("socket");
	}

	if (remove(SOCK_PATH) == -1 && errno != ENOENT){
        handle_error("remove");
    }

	memset(&server_addr, 0, sizeof(struct sockaddr_un));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, SOCK_PATH, sizeof(server_addr.sun_path) - 1);
	if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1) {
		handle_error("bind");
	}

	char buf[256];
	ssize_t send_size, recv_size;
	socklen_t client_addr_len = sizeof(client_addr);
	while(1) {
		memset(buf, 0, sizeof(buf));
		memset(&client_addr, 0, sizeof(struct sockaddr_un));
		recv_size = recvfrom(sfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &client_addr_len);
		if (recv_size < 0) {
			handle_error("recvfrom");
		}
		if (recv_size == 0) {
			goto EXIT;
		}
		printf("recv buff: %s\n", buf);
		
		send_size = sendto(sfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, client_addr_len);
		if (send_size < 0) {
			handle_error("sendto");
		}
		if (send_size == 0) {
			goto EXIT;
		}

	}
EXIT:
	close(sfd);
	return 0;
}


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/test_stream.sock"

#define handle_error(msg) \
	do {perror(msg); exit(EXIT_FAILURE);} while(0)

int main(int argc, void **argv) {
	int sfd;
	struct sockaddr_un server_addr, client_addr;
	
	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
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

	if (listen(sfd, 128) == -1) {
		handle_error("listen");
	}

	while(1) {
		int cfd;
		struct sockaddr_un client_addr;
		socklen_t client_addr_len;
		client_addr_len = sizeof(struct sockaddr_un);
		cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);
	    //cfd = accept(sfd, NULL, NULL);
		if (cfd == -1) {
			perror("accept");
			break;
		}

		/*
		client_addr_len = sizeof(struct sockaddr_un);
		if (getpeername(cfd, (struct sockaddr *)&client_addr, &client_addr_len) == -1) {
			perror("getpeername");
			close(cfd);
			break;
		}
		printf("client socket filepath: %s\n", client_addr.sun_path);
		*/

		pid_t pid;
		pid = fork();
		if (pid > 0) {
			close(cfd);
			continue;
		} else if (pid < 0) {
			perror("fork");
			close(cfd);
			break;
		} else {
			close(sfd);
			char buf[256];
			ssize_t recv_bytes, send_bytes;
			while(1) {
				memset(buf, 0, sizeof(buf));
				recv_bytes = recv(cfd, buf, sizeof(buf), 0);
				if (recv_bytes <= 0) {
					if (recv_bytes < 0) {
						printf("recv fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
					}
					break;
				}

				printf("recv len: %lu, buffer: %s\n", strlen(buf), buf);
				send_bytes = send(cfd, buf, strlen(buf), 0);
				if (send_bytes <= 0) {
					if (recv_bytes < 0) {
						printf("send fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
					}
					break;
				}
			}
			printf("client close, cfd: %d\n", cfd);
			close(cfd);
		}
	}
	close(sfd);
	return 0;
}

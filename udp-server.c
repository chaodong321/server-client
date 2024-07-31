#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888

int main(int argc, char **argv) {
	struct sockaddr_in server_addr, client_addr;
	int ret;
    int fd;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        printf("create socket fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
        return -1;
    }

    server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen_t client_addr_len = sizeof(client_addr);

    ret = bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
		printf("bind fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
    }

    char buf[256];
    ssize_t recv_bytes;
	while(1) {
		memset(buf, 0, sizeof(buf));
	    recv_bytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &client_addr_len);
	    if (recv_bytes <= 0) {
	        printf("recvfrom fail\n");
	        goto ERR;
	    }
	    printf("client address: %s, port: %d, recv buffer: %s\n", 
	        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buf);

	    ssize_t send_bytes = sendto(fd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, 
	        sizeof(client_addr));
	    if (send_bytes <= 0) {
	        printf("sendto fail\n");
	        goto ERR;
	    }
	}
ERR:
    close(fd);
    return 0;
}

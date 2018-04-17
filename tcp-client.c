#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8888

void process_conn_client(int sc)
{
	ssize_t size = 0;
	char buffer[1024];

	while(1){
		memset(buffer, 0, sizeof(buffer));
		size = read(0, buffer, sizeof(buffer));
		if(size > 0){
			write(sc, buffer, size);
			size = read(sc, buffer, sizeof(buffer));
			write(1, buffer, size);
		}
	}
}

int main(int argc, char **argv)
{
	int sc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sc < 0){
		printf("socket error\n");
		return -1;
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

	connect(sc, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));

	process_conn_client(sc);

	return 0;
}


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888
#define BACKLOG 5

void process_conn_server(int sc, int connect_num)
{
	ssize_t size = 0;
	char buffer[256];

	while(1){
		memset(buffer, 0, sizeof(buffer));
		size = read(sc, buffer, sizeof(buffer));

		printf("%d -- recv: %s", connect_num, buffer);

		if(size == 0)
			return;

		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%ld bytes altogether\n", size);
		write(sc, buffer, strlen(buffer)+1);
	}
}

int main(int argc, char **argv)
{
	int ss = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(ss < 0){
		printf("socket error\n");
		return -1;
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVER_PORT);
	int bind_err = bind(ss, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(bind_err < 0){
		printf("bind error\n");
		return -1;
	}

	int listen_err = listen(ss, BACKLOG);
	if(listen_err < 0){
		printf("listen error\n");
		return -1;
	}

	int connect_num = 0;
	int sc;
	struct sockaddr_in client_addr;
	while(1){

		socklen_t addrlen = sizeof(struct sockaddr);
		sc = accept(ss, (struct sockaddr*)&client_addr, &addrlen);

		if(sc < 0){
			printf("accept error\n");
			continue;
		}

		printf("connect number:%d\n", ++connect_num);
		pid_t pid = fork();
		if(pid == 0){
			close(ss);
			process_conn_server(sc, connect_num);
		}
		else{
			close(sc);
		}
	}
	
	return 0;
}

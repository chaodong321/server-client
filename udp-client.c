#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888

int main() {
	int fd;
	struct sockaddr_in server_addr;	

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		printf("create socket fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

	//建立有连接udp
	int ret = connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
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
		/*
		//有连接udp后两个参数必须为空
		send_bytes = sendto(fd, buf, sizeof(buf), 0, NULL, 0);
		//无连接udp
		send_bytes = sendto(fd, buf, sizeof(buf), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
		//有连接udp
		send_bytes = send(fd, buf, sizeof(buf), 0);
		*/
		send_bytes = write(fd, buf, sizeof(buf));
		/*
		struct iovec vec_send[1];
		vec_send[0].iov_base = &buf;
		vec_send[0].iov_len = sizeof(buf);

		struct msghdr send_msg;
		bzero(&send_msg, sizeof(send_msg));
		send_msg.msg_iov = vec_send;
		send_msg.msg_iovlen = sizeof(vec_send);

		send_bytes = sendmsg(fd, &send_msg, 0);
		*/
		if (send_bytes <= 0) {
			printf("send fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
			goto ERR;
		}

		memset(buf, 0, sizeof(buf));
		/*
		//recv、recvfrom、recvmsg都可以接收有连接和无连接udp,只看需不需要使用到函数的参数
		recv_bytes = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
		recv_bytes = recv(fd, buf, sizeof(buf), 0);
		*/
		recv_bytes = read(fd, buf, sizeof(buf));
		/*
		struct iovec vec_recv[1];
		vec_recv[0].iov_base = &buf;
		vec_recv[0].iov_len = sizeof(buf);

		struct msghdr recv_msg;
		//bzero(&recv_msg, sizeof(recv_msg));
		recv_msg.msg_name = NULL;
		recv_msg.msg_namelen = 0;
		recv_msg.msg_iov = vec_recv;
		recv_msg.msg_iovlen = sizeof(vec_recv);
		recv_msg.msg_control = NULL;
		recv_msg.msg_controllen = 0;
		recv_msg.msg_flags = 0;

		recv_bytes = recvmsg(fd, &recv_msg, 0);
		*/
		if (recv_bytes <= 0) {
			printf("recv fail, errno: %d, errmsg: %s\n", errno, (char*)strerror(errno));
			goto ERR;
		}
		printf("recv buffer: %s\n", buf);
	}
ERR:
	close(fd);
	return 0;
}

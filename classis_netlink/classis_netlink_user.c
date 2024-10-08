#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <errno.h>
 
#define NETLINK_TEST    (25)
#define MAX_PAYLOAD     (1024)
#define TEST_PID        (100)
 
int netlink_create_socket(void)
{
        //create a socket
        return socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
}
 
int netlink_bind(int sock_fd)
{
        struct sockaddr_nl addr;
 
        memset(&addr, 0, sizeof(struct sockaddr_nl));
        addr.nl_family = AF_NETLINK;
        addr.nl_pid = TEST_PID;
        addr.nl_groups = 0;
 
        return bind(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_nl));
}
 
int
netlink_send_message(int sock_fd, const unsigned char *message, int len,
                                        unsigned int pid, unsigned int group)
{
        struct nlmsghdr *nlh = NULL;
        struct sockaddr_nl dest_addr;
        struct iovec iov;
        struct msghdr msg;
 
        if( !message ) {
                return -1;
        }
 
        //create message
        nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(len));
        if( !nlh ) {
                perror("malloc");
                return -2;
        }
        nlh->nlmsg_len = NLMSG_SPACE(len);
        nlh->nlmsg_pid = TEST_PID;
        nlh->nlmsg_flags = 0;
        memcpy(NLMSG_DATA(nlh), message, len);
 
        iov.iov_base = (void *)nlh;
        iov.iov_len = nlh->nlmsg_len;
        memset(&dest_addr, 0, sizeof(struct sockaddr_nl));
        dest_addr.nl_family = AF_NETLINK;
        dest_addr.nl_pid = pid;
        dest_addr.nl_groups = group;
 
        memset(&msg, 0, sizeof(struct msghdr));
        msg.msg_name = (void *)&dest_addr;
        msg.msg_namelen = sizeof(struct sockaddr_nl);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
 
        //send message
        if( sendmsg(sock_fd, &msg, 0) < 0 )
        {
                printf("send error!\n");
                free(nlh);
                return -3;
        }
 
        free(nlh);
        return 0;
}
 
int
netlink_recv_message(int sock_fd, unsigned char *message, int *len)
{
        struct nlmsghdr *nlh = NULL;
        struct sockaddr_nl source_addr;
        struct iovec iov;
        struct msghdr msg;
 
        if( !message || !len ) {
                return -1;
        }
 
        //create message
        nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
        if( !nlh ) {
                perror("malloc");
                return -2;
        }
        iov.iov_base = (void *)nlh;
        iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);
        memset(&source_addr, 0, sizeof(struct sockaddr_nl));
        memset(&msg, 0, sizeof(struct msghdr));
        msg.msg_name = (void *)&source_addr;
        msg.msg_namelen = sizeof(struct sockaddr_nl);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
 
        if ( recvmsg(sock_fd, &msg, 0) < 0 ) {
                printf("recvmsg error!\n");
                return -3;
        }
        *len = nlh->nlmsg_len - NLMSG_SPACE(0);
        memcpy(message, (unsigned char *)NLMSG_DATA(nlh), *len);
 
        free(nlh);
        return 0;
}
 
int
main(int argc, char **argv)
{
        int sock_fd;
        char buf[MAX_PAYLOAD];
        int len;
 
        if( argc < 2) {
                printf("enter message!\n");
                exit(EXIT_FAILURE);
        }
 
        sock_fd = netlink_create_socket();
        if(sock_fd == -1) {
                printf("socket error!\n");
                return -1;
        }
 
        if( netlink_bind(sock_fd) < 0 ) {
                perror("bind");
                close(sock_fd);
                exit(EXIT_FAILURE);
        }
 
        netlink_send_message(sock_fd, argv[1], strlen(argv[1]) + 1, 0, 0);
        if( netlink_recv_message(sock_fd, buf, &len) == 0 ) {
                printf("recv:%s len:%d\n", buf, len);
        }
 
        close(sock_fd);
        return 0;
}
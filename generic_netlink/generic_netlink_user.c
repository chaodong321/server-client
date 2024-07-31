#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <linux/genetlink.h>

#define MAX_MSG_SIZE 256
#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define NLA_DATA(na) ((void *)((char *)(na) + NLA_HDRLEN))
/* attribute type */
  enum {
		EXMPL_A_UNSPEC,
		EXMPL_A_MSG,
		__EXMPL_A_MAX,
  };
  #define EXMPL_A_MAX (__EXMPL_A_MAX - 1)
  /* commands */
  enum {
		EXMPL_C_UNSPEC,
		EXMPL_C_ECHO,
		__EXMPL_C_ECHO,
  };
  #define EXMPL_C_MAX (__EXMPL_C_MAX - 1)

int genl_send_msg(int sock_fd, u_int16_t family_id, u_int32_t nlmsg_pid,
		u_int8_t genl_cmd, u_int8_t genl_version, u_int16_t nla_type,
		void *nla_data, int nla_len);
static int genl_get_family_id(int sock_fd, char *family_name);
void genl_rcv_msg(int family_id, int sock_fd, char *buf);

typedef struct msgtemplate {
	struct nlmsghdr nlh;
	struct genlmsghdr gnlh;
	char data[MAX_MSG_SIZE];
} msgtemplate_t;

static int genl_get_family_id(int sock_fd, char *family_name)
{
	msgtemplate_t ans;
	int id, rc;
	struct nlattr *na;
	int rep_len;
	rc = genl_send_msg(sock_fd, GENL_ID_CTRL, 0, CTRL_CMD_GETFAMILY, 1,
					CTRL_ATTR_FAMILY_NAME, (void *)family_name,
					strlen(family_name)+1);
	rep_len = recv(sock_fd, &ans, sizeof(ans), 0);
	if (rep_len < 0) {
		return 1;
	}
	if (ans.nlh.nlmsg_type == NLMSG_ERROR || !NLMSG_OK((&ans.nlh), rep_len))
	{
		return 1;
	}
	na = (struct nlattr *) GENLMSG_DATA(&ans);
	na = (struct nlattr *) ((char *) na + NLA_ALIGN(na->nla_len));
	if (na->nla_type == CTRL_ATTR_FAMILY_ID) {
		id = *(__u16 *) NLA_DATA(na);
	} else {
		id = 0;
	}
	return id;
}

/**
* genl_send_msg - 通过generic netlink给内核发送数据
*
* @sock_fd: 客户端socket
* @family_id: family id
* @nlmsg_pid: 客户端pid
* @genl_cmd: 命令类型
* @genl_version: genl版本号
* @nla_type: netlink attr类型
* @nla_data: 发送的数据
* @nla_len: 发送数据长度
*
* return:
* 0: 成功
* -1: 失败
*/
int genl_send_msg(int sock_fd, u_int16_t family_id, u_int32_t nlmsg_pid,
		u_int8_t genl_cmd, u_int8_t genl_version, u_int16_t nla_type,
		void *nla_data, int nla_len)
{
	struct nlattr *na;
	struct sockaddr_nl dst_addr;
	int r, buflen;
	char *buf;
	msgtemplate_t msg;
	if (family_id == 0) {
		return 0;
	}
	msg.nlh.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	msg.nlh.nlmsg_type = family_id;
	msg.nlh.nlmsg_flags = NLM_F_REQUEST;
	msg.nlh.nlmsg_seq = 0;
	msg.nlh.nlmsg_pid = nlmsg_pid;
	msg.gnlh.cmd = genl_cmd;
	msg.gnlh.version = genl_version;
	na = (struct nlattr *) GENLMSG_DATA(&msg);
	na->nla_type = nla_type;
	na->nla_len = nla_len + 1 + NLA_HDRLEN;
	memcpy(NLA_DATA(na), nla_data, nla_len);
	msg.nlh.nlmsg_len += NLMSG_ALIGN(na->nla_len);
	buf = (char *) &msg;
	buflen = msg.nlh.nlmsg_len ;
	memset(&dst_addr, 0, sizeof(dst_addr));
	dst_addr.nl_family = AF_NETLINK;
	dst_addr.nl_pid = 0;
	dst_addr.nl_groups = 0;
	while ((r = sendto(sock_fd, buf, buflen, 0, (struct sockaddr *) &dst_addr
			, sizeof(dst_addr))) < buflen) {
		if (r > 0) {
			buf += r;
			buflen -= r;
		} else if (errno != EAGAIN) {
			return -1;
		}
	}
	return 0;
}

void genl_rcv_msg(int family_id, int sock_fd, char *buf)
{
	int ret;
	struct msgtemplate msg;
	struct nlattr *na;
	ret = recv(sock_fd, &msg, sizeof(msg), 0);
	if (ret < 0) {
		return;
	}
	printf("received length %d\n", ret);
	if (msg.nlh.nlmsg_type == NLMSG_ERROR || !NLMSG_OK((&msg.nlh), ret)) {
		return ;
	}
	if (msg.nlh.nlmsg_type == family_id && family_id != 0) {
		na = (struct nlattr *) GENLMSG_DATA(&msg);
		strcpy(buf,(char *)NLA_DATA(na));
	}
}

int main(int argc, char* argv[])
{
	struct sockaddr_nl src_addr, dest_addr;
	struct nlmsghdr *nlh = NULL;
	int sock_fd, retval;
	int family_id = 0;
	char *data;
	// Create a socket
	sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	if(sock_fd == -1){
		printf("error getting socket: %s", strerror(errno));
		return -1;
	}
	// To prepare binding
	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = 1234;
	src_addr.nl_groups = 0;
	//Bind
	retval = bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
	if(retval < 0){
		printf("bind failed: %s", strerror(errno));
		close(sock_fd);
		return -1;
	}
	family_id = genl_get_family_id(sock_fd ,"myfamily");
	printf("family_id:%d\n",family_id);
	data =(char*)malloc(256);
	if(!data)
	{
		perror("malloc error!");
		exit(1);
	}
	memset(data,0,256);
	strcpy(data,"Hello you!");
	retval = genl_send_msg(sock_fd, family_id, 1234,EXMPL_C_ECHO, 1, EXMPL_A_MSG,(void *)data, strlen(data)+1);
	printf("send message %d\n",retval);
	if(retval<0)
	{
		perror("genl_send_msg error");
		exit(1);
	}
	memset(data,0,256);
	genl_rcv_msg(family_id,sock_fd,data);
	printf("receive:%s",data);
	close(sock_fd);
	return 0;
}

#include <linux/module.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <net/sock.h>
#include <net/genetlink.h>
#define MAXLEN 256
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

int stringlength(char *s);
int genl_recv_doit(struct sk_buff *skb, struct genl_info *info);
static inline int genl_msg_prepare_usr_msg(u8 cmd, size_t size, pid_t pid, struct sk_buff **skbp);
static inline int genl_msg_mk_usr_msg(struct sk_buff *skb, int type, void *data, int len);
int genl_msg_send_to_user(void *data, int len, pid_t pid);

 static struct genl_family exmpl_gnl_family = {
	   .id = GENL_ID_GENERATE,
	   .hdrsize = 0,
	   .name = "myfamily",
	   .version = 1,
	   .maxattr = EXMPL_A_MAX,
  };
  /* attribute policy */
  static struct nla_policy exmpl_genl_policy[EXMPL_A_MAX + 1] = {
		[EXMPL_A_MSG] = { .type = NLA_NUL_STRING },
  };
  /* operation definition */
  struct genl_ops exmpl_gnl_ops_echo = {
		.cmd = EXMPL_C_ECHO,
		.flags = 0,
		.policy = exmpl_genl_policy,
		.doit = genl_recv_doit,
		.dumpit = NULL,
  };

int stringlength(char *s)
{
	int slen = 0;
	for(; *s; s++)
	{
		slen++;
	}
	return slen;
}

/*
* genl_msg_prepare_usr_msg : 构建netlink及gennetlink首部
* @cmd : genl_ops的cmd
* @size : gen_netlink用户数据的长度（包括用户定义的首部）
*/
static inline int genl_msg_prepare_usr_msg(u8 cmd, size_t size, pid_t pid, struct sk_buff **skbp)
{
	struct sk_buff *skb;
	/* create a new netlink msg */
	skb = genlmsg_new(size, GFP_KERNEL);
	if (skb == NULL) {
		return -ENOMEM;
	}
	/* Add a new netlink message to an skb */
	genlmsg_put(skb, pid, 0, &exmpl_gnl_family, 0, cmd);
	*skbp = skb;
	return 0;
}

/*
* 添加用户数据，及添加一个netlink addribute
*@type : nlattr的type
*@len : nlattr中的len
*@data : 用户数据
*/
static inline int genl_msg_mk_usr_msg(struct sk_buff *skb, int type, void *data, int len)
{
	int rc;
	/* add a netlink attribute to a socket buffer */
	if ((rc = nla_put(skb, type, len, data)) != 0) {
		return rc;
	}
	return 0;
}

/**
* genl_msg_send_to_user - 通过generic netlink发送数据到netlink
*
* @data: 发送数据缓存
* @len: 数据长度 单位：byte
* @pid: 发送到的客户端pid
*/
int genl_msg_send_to_user(void *data, int len, pid_t pid)
{
	struct sk_buff *skb;
	size_t size;
	void *head;
	int rc;
	printk("begin send to user\n");
	size = nla_total_size(len); /* total length of attribute including padding */
	rc = genl_msg_prepare_usr_msg(EXMPL_C_ECHO, size, pid, &skb);
	printk("genl_msg_prepare_usr_msg\n");
	if (rc) {
		return rc;
	}
	rc = genl_msg_mk_usr_msg(skb, EXMPL_A_MSG, data, len);
	printk("genl_msg_mk_usr_msg\n");
	if (rc) {
		kfree_skb(skb);
		return rc;
   }
	printk("pid :%d",pid);
	rc = genlmsg_unicast(&init_net, skb, pid);
	printk("send end....\n");
	if (rc < 0) {
		return rc;
	}
	return 0;
}

int genl_recv_doit(struct sk_buff *skb, struct genl_info *info)
{
	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nla;
	int len;
	char* str;
	char* data = "I am from kernel!";
	int state = 0;
	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nla = genlmsg_data(genlhdr);
	printk ("received\n");
	str = (char *)NLA_DATA(nla);
	printk("%s\n",str);
	len = stringlength(data);
	if ( (state = genl_msg_send_to_user(data, len, nlhdr->nlmsg_pid)) <0 )
	{
		printk(KERN_ERR "genl_msg_send_to_user error!");
		return 1;
	}
	return 0;
}

int genetlink_init(void)
{
	int state=0;
	state = genl_register_family(&exmpl_gnl_family);
	if(state)
	{
	   printk(KERN_ERR "genl_register_family error!!!\n");
	   return 1;
	}
	state = genl_register_ops(&exmpl_gnl_family, &exmpl_gnl_ops_echo);
	if(state)
	{
		printk(KERN_ERR "genl_register_ops error!!!");
		return 1;
	}
	printk(KERN_ERR "gennetlink register success!!!\n");
	return 0;
}
void genetlink_exit(void)
{
	genl_unregister_family(&exmpl_gnl_family);
	printk(KERN_ERR "gennetlink unregister.....\n");
}
module_init(genetlink_init);
module_exit(genetlink_exit);
MODULE_AUTHOR("chaodong");
MODULE_LICENSE("GPL");
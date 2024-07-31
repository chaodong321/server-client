#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <linux/skbuff.h>
#include <net/genetlink.h>

#define MY_GENL_FAMILY_NAME "my_genl_family"
#define MY_GENL_VERSION 1

enum {
    MY_GENL_CMD_UNSPEC,
    MY_GENL_CMD_SET_PARAM,
    MY_GENL_CMD_GET_PARAMS,
    __MY_GENL_CMD_MAX,
};
#define MY_GENL_CMD_MAX (__MY_GENL_CMD_MAX - 1)

enum {
    MY_GENL_ATTR_UNSPEC,
	MY_GENL_ATTR_KEY,
    MY_GENL_ATTR_PARAM,
    __MY_GENL_ATTR_MAX,
};
#define MY_GENL_ATTR_MAX (__MY_GENL_ATTR_MAX - 1)

static struct nla_policy my_genl_policy[MY_GENL_ATTR_MAX + 1] = {
	[MY_GENL_ATTR_KEY] = { .type = NLA_STRING },
    [MY_GENL_ATTR_PARAM] = { .type = NLA_STRING },
};

static LIST_HEAD(param_list);
static struct genl_family my_genl_family;

struct param_item {
    char param[100];
    struct list_head list;
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

static int my_genl_param_add(struct sk_buff *skb, struct genl_info *info) {
    struct param_item *item;
    if (!info->attrs[MY_GENL_ATTR_PARAM])
        return -EINVAL;

    item = kmalloc(sizeof(*item), GFP_KERNEL);
    if (!item)
        return -ENOMEM;

    strncpy(item->param, nla_data(info->attrs[MY_GENL_ATTR_PARAM]), sizeof(item->param));
    list_add(&item->list, &param_list);
    printk(KERN_INFO "Parameter set: %s\n", item->param);

    return 0;
}

static inline int genl_prepare_usr_msg(u8 cmd, size_t size, u32 pid, u32 seq, struct sk_buff **skbp)
{
	struct sk_buff *skb;
	/* create a new netlink msg */
	skb = genlmsg_new(size, GFP_KERNEL);
	if (skb == NULL) {
		return -ENOMEM;
	}
	/* Add a new netlink message to an skb */
	genlmsg_put(skb, pid, seq, &my_genl_family, 0, cmd);
	*skbp = skb;
	return 0;
}

static inline int genl_mk_usr_msg(struct sk_buff *skb, int type, void *data, int len)
{
	int rc;
	/* add a netlink attribute to a socket buffer */
	if ((rc = nla_put(skb, type, len, data)) != 0) {
		return rc;
	}
	return 0;
}

static int my_genl_param_doit(struct sk_buff *skb, struct genl_info *info) {	
	struct sk_buff *reply;
	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nla;
	int rc;
	size_t size;
	int len;
	void *hdr;
	char* data = "I am from kernel, do it!";

	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nla = genlmsg_data(genlhdr);

	printk(KERN_INFO "received pid :%d\n",nlhdr->nlmsg_pid);

	//todo：读取数据
	len = stringlength(data);
	size = nla_total_size(len);

	//构造消息
	/* create a new netlink msg */
	reply = genlmsg_new(size, GFP_KERNEL);
	if (reply == NULL) {
		return -ENOMEM;
	}
	/* Add a new netlink message to an skb */
	hdr = genlmsg_put(reply, nlhdr->nlmsg_pid, info->snd_seq, &my_genl_family, 0, MY_GENL_CMD_GET_PARAMS);

	/* add a netlink attribute to a socket buffer */
	printk(KERN_INFO "date: %s, len: %d", data, len);
	if ((rc = nla_put(reply, MY_GENL_ATTR_PARAM, len, data)) != 0) {
		printk(KERN_ERR "nla_put fail\n");
		kfree_skb(reply);
		return rc;
	}
	genlmsg_end(reply, hdr);

	//响应消息
	rc = genlmsg_unicast(&init_net, reply, nlhdr->nlmsg_pid);
	if (rc < 0) {
		printk(KERN_ERR "send message to user fail");
		return rc;
	}
	printk(KERN_INFO "finish...\n");
	return 0;
}

static int my_genl_param_doit2(struct sk_buff *skb, struct genl_info *info) {
	struct param_item *item;

	struct sk_buff *reply;
	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nla;
	int rc;
	size_t size;
	int len;
	void *hdr;
	char* data;

	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nla = genlmsg_data(genlhdr);

	printk(KERN_INFO "received pid :%d\n",nlhdr->nlmsg_pid);


	list_for_each_entry(item, &param_list, list) {
		data = item->param;
		len = stringlength(data);
		size = nla_total_size(len);

		/* create a new netlink msg */
		reply = genlmsg_new(size, GFP_KERNEL);
		if (reply == NULL) {
			return -ENOMEM;
		}
		/* Add a new netlink message to an skb */
		hdr = genlmsg_put(reply, nlhdr->nlmsg_pid, info->snd_seq, &my_genl_family, 0, MY_GENL_CMD_GET_PARAMS);

		/* add a netlink attribute to a socket buffer */
		printk(KERN_INFO "date: %s, len: %d", data, len);
		if ((rc = nla_put(reply, MY_GENL_ATTR_PARAM, len, data)) != 0) {
			printk(KERN_ERR "nla_put fail\n");
			kfree_skb(reply);
			return rc;
		}
		genlmsg_end(reply, hdr);

		rc = genlmsg_unicast(&init_net, reply, nlhdr->nlmsg_pid);
		if (rc < 0) {
			printk(KERN_ERR "send message to user fail");
			return rc;
		}
	}

	printk(KERN_INFO "finish...\n");
	return 0;
}

static int my_genl_param_doit3(struct sk_buff *skb, struct genl_info *info) {
	struct param_item *item;
	struct sk_buff *reply;
	int rc;
	size_t size;
	int len;
	void *hdr;
	char* data;
	struct nlattr **attrs = info->attrs;
	char* key = NULL;	

	printk(KERN_INFO "received pid :%d\n",info->nlhdr->nlmsg_pid);
	if (attrs[MY_GENL_ATTR_KEY]) {
		key = nla_data(attrs[MY_GENL_ATTR_KEY]);
		printk(KERN_INFO "key: %s\n", key);
	} else {
		printk(KERN_INFO "no key");
	}
	
	list_for_each_entry(item, &param_list, list) {
		data = item->param;
		len = stringlength(data);
		size = nla_total_size(len);

		/* create a new netlink msg */
		reply = genlmsg_new(size, GFP_KERNEL);
		if (reply == NULL) {
			return -ENOMEM;
		}
		/* Add a new netlink message to an skb */
		hdr = genlmsg_put(reply, info->nlhdr->nlmsg_pid, info->nlhdr->nlmsg_seq, &my_genl_family, 0, MY_GENL_CMD_GET_PARAMS);

		/* add a netlink attribute to a socket buffer */
		printk(KERN_INFO "date: %s, len: %d", data, len);
		if ((rc = nla_put(reply, MY_GENL_ATTR_PARAM, len, data)) != 0) {
			printk(KERN_ERR "nla_put fail\n");
			kfree_skb(reply);
			return rc;
		}
		genlmsg_end(reply, hdr);

		rc = genlmsg_unicast(&init_net, reply, info->nlhdr->nlmsg_pid);
		if (rc < 0) {
			printk(KERN_ERR "send message to user fail");
			return rc;
		}
	}

	printk(KERN_INFO "finish...\n");
	return 0;
}

static int my_genl_params_dumpit(struct sk_buff *skb, struct netlink_callback *cb) {
	int rc;
    void *hdr;
	int len;
	size_t size;
	char* data = "I am from kernel, dump it!";

	len = stringlength(data);
	size = nla_total_size(len);

	/* Add a new netlink message to an skb */
	hdr = genlmsg_put(skb, 0, cb->nlh->nlmsg_seq, &my_genl_family, 0, MY_GENL_CMD_GET_PARAMS);

	/* add a netlink attribute to a socket buffer */
	printk(KERN_INFO "date: %s, len: %d", data, len);
	if ((rc = nla_put(skb, MY_GENL_ATTR_PARAM, len, data)) != 0) {
		printk(KERN_ERR "nla_put fail\n");
		kfree_skb(skb);
		return rc;
	}
	genlmsg_end(skb, hdr);
    return 0;
}

static int my_genl_params_dumpit2(struct sk_buff *skb, struct netlink_callback *cb) {
    struct param_item *item;
    void *hdr;
    int start = cb->args[0];
    int i = 0;
    
    list_for_each_entry(item, &param_list, list) {
        if (i++ < start)
            continue;

        hdr = genlmsg_put(skb, 0, cb->nlh->nlmsg_seq, &my_genl_family, NLM_F_MULTI, MY_GENL_CMD_GET_PARAMS);
        if (!hdr) {
            return -ENOMEM;
        }

        if (nla_put_string(skb, MY_GENL_ATTR_PARAM, item->param)) {
            return -ENOMEM;
        }

        genlmsg_end(skb, hdr);
        cb->args[0] = i;

        if (i >= start + 2) {
            return skb->len;
        }
    }
    return 0;
}

static int my_genl_params_dumpit3(struct sk_buff *skb, struct netlink_callback *cb) {
	struct nlattr *a[__MY_GENL_ATTR_MAX];
    struct param_item *item;
    void *hdr;
	int err;
    int start = cb->args[0];
    int i = 0;
	char *key = NULL;

	err = genlmsg_parse(cb->nlh, &my_genl_family, a,
				       MY_GENL_ATTR_MAX, my_genl_policy, NULL);
	if (err < 0) {
		printk(KERN_ERR "genlmsg_parse_deprecated fail");
		return err;
	}

	if (a[MY_GENL_ATTR_KEY]) {
		key = nla_data(a[MY_GENL_ATTR_KEY]);
		printk(KERN_INFO "key: %s\n", key);
	} else {
		printk(KERN_INFO "no key");
	}

    list_for_each_entry(item, &param_list, list) {
        if (i++ < start)
            continue;

		hdr = genlmsg_put(skb, cb->nlh->nlmsg_pid, cb->nlh->nlmsg_seq, &my_genl_family, NLM_F_MULTI, MY_GENL_CMD_GET_PARAMS);
        if (!hdr) {
            return -ENOMEM;
        }

        if (nla_put_string(skb, MY_GENL_ATTR_PARAM, item->param)) {
            return -ENOMEM;
        }

        genlmsg_end(skb, hdr);
        cb->args[0] = i;

        if (i >= start + 2) {
            return skb->len;
        }
    }
    return 0;
}


static const struct genl_ops my_genl_ops[] = {
    {
        .cmd = MY_GENL_CMD_SET_PARAM,
        .flags = 0,
        .policy = my_genl_policy,
        .doit = my_genl_param_add,
    },
    {
        .cmd = MY_GENL_CMD_GET_PARAMS,
        .flags = 0,
        .policy = my_genl_policy,
        //.doit = my_genl_param_doit,
        //.dumpit = my_genl_params_dumpit,
        .doit = my_genl_param_doit3,
        .dumpit = my_genl_params_dumpit3,
    },
};

static struct genl_family my_genl_family = {
    .hdrsize = 0,
    .name = MY_GENL_FAMILY_NAME,
    .version = MY_GENL_VERSION,
    .maxattr = MY_GENL_ATTR_MAX,
    .ops = my_genl_ops,
    .n_ops = ARRAY_SIZE(my_genl_ops),
};

static int __init my_genl_init(void) {
    int ret;
	printk(KERN_INFO "generic netlink test module init\n");
    ret = genl_register_family(&my_genl_family);
    if (ret)
        printk(KERN_ERR "Failed to register genl family\n");
    return ret;
}

static void __exit my_genl_exit(void) {
    struct param_item *item, *tmp;
    list_for_each_entry_safe(item, tmp, &param_list, list) {
        list_del(&item->list);
        kfree(item);
    }
    genl_unregister_family(&my_genl_family);
	printk(KERN_INFO "generic netlink test module exit\n");
}

module_init(my_genl_init);
module_exit(my_genl_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Example Generic Netlink kernel module");


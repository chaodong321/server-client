#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <linux/genetlink.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#define MY_GENL_FAMILY_NAME "my_genl_family"
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


int send_set_param(struct nl_sock *sock, int family_id, const char *param) {
    struct nl_msg *msg;
    void *hdr;
    int ret;

    msg = nlmsg_alloc();
    if (!msg)
        return -ENOMEM;

    hdr = genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family_id, 0, 0, MY_GENL_CMD_SET_PARAM, 1);
    if (!hdr) {
        nlmsg_free(msg);
        return -ENOMEM;
    }

    ret = nla_put_string(msg, MY_GENL_ATTR_PARAM, param);
    if (ret) {
        nlmsg_free(msg);
        return ret;
    }

    ret = nl_send_auto_complete(sock, msg);
    nlmsg_free(msg);
    if (ret < 0)
        return ret;

    return 0;
}

static int print_reply(struct nl_msg * nlmsg, void * arg)
{
	struct nlmsghdr *nlhdr;
	struct nlattr *nlattrs[MY_GENL_ATTR_MAX + 1];
	struct nlattr *nlattr;

	nlhdr = nlmsg_hdr(nlmsg);

	if (genlmsg_parse(nlhdr, 0, nlattrs, MY_GENL_ATTR_MAX, NULL) < 0) {
		fprintf(stderr, "error parsing msg\n");
		return -NLE_PARSE_ERR;
	}

	 if (nlattrs[MY_GENL_ATTR_PARAM]) {
        printf("value: %s\n", nla_data(nlattrs[MY_GENL_ATTR_PARAM]));
    } else {
        fprintf(stderr, "unknown attribute format received\n");
        return -NLE_MISSING_ATTR;
    }
	return 0;
}

static int my_msg_handler(struct nl_msg *msg, void *arg) {
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct genlmsghdr *genlh = nlmsg_data(nlh);
    struct nlattr *attrs[MY_GENL_ATTR_MAX+1];
	int ret;

    printf("Received message: cmd=%d, version=%d\n", genlh->cmd, genlh->version);

    if (genlh->cmd == MY_GENL_CMD_GET_PARAMS) {
		struct nlattr *head = genlmsg_attrdata(genlh, 0);
		int len = genlmsg_attrlen(genlh, 0);
		printf("attribute head: %p, len: %d\n", head, len);
        ret = nla_parse(attrs, __MY_GENL_ATTR_MAX, head, len, NULL);
		if (ret < 0) {
			printf("nla_parse fail, ret code: %d\n", ret);
			return ret;
		}

        if (attrs[MY_GENL_ATTR_PARAM]) {
            printf("Param: %s\n", nla_get_string(attrs[MY_GENL_ATTR_PARAM]));
        }
    }
    return NL_OK;
}

int recv_get_params(struct nl_sock *sock, int family_id) {
    struct nl_msg *msg;
    void *hdr;
    int ret;

    msg = nlmsg_alloc();
    if (!msg)
        return -ENOMEM;

	//.dumpit
    hdr = genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family_id, 0, NLM_F_DUMP, MY_GENL_CMD_GET_PARAMS, 1);
    //.doit
	//hdr = genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family_id, 0, 0, MY_GENL_CMD_GET_PARAMS, 1);
    if (!hdr) {
		printf("genlmsg_put fail\n");
        nlmsg_free(msg);
        return -ENOMEM;
    }

	if ((ret = nla_put_string(msg, MY_GENL_ATTR_KEY, "test")) < 0) {
		printf("unable to add attribue: %s\n", nl_geterror(ret));
        nlmsg_free(msg);
        return ret;
    }

    ret = nl_send_auto_complete(sock, msg);
    if (ret < 0) {
		printf("nl_send_auto_complete fail: %s\n", nl_geterror(ret));
        return ret;
	}
	nlmsg_free(msg);

	ret = nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, my_msg_handler, NULL);
	if (ret < 0) {
		printf("nl_socket_modify_cb fail: %s\n", nl_geterror(ret));
        return ret;
	}
	
    ret = nl_recvmsgs_default(sock);
	if (ret != 0) {
		printf("nl_recvmsgs_default fail: %s\n", nl_geterror(ret));
        return ret;
	}
    return 0;
}

int main(int argc, char *argv[]) {
    struct nl_sock *sock;
    int family_id, ret;

    sock = nl_socket_alloc();
    if (!sock) {
        printf("Unable to allocate netlink socket\n");
        return -ENOMEM;
    }

    ret = genl_connect(sock);
    if (ret < 0) {
        printf("genl connect failed\n");
        nl_socket_free(sock);
        return ret;
    }

    family_id = genl_ctrl_resolve(sock, MY_GENL_FAMILY_NAME);
    if (family_id < 0) {
        printf("Resolving of \"%s\" failed\n", MY_GENL_FAMILY_NAME);
        nl_socket_free(sock);
        return family_id;
    }

    if (argc > 1) {
        ret = send_set_param(sock, family_id, argv[1]);
        if (ret < 0) {
            printf("send param fail\n");
            nl_socket_free(sock);
            return ret;
        }
    } else {
        ret = recv_get_params(sock, family_id);
        if (ret < 0) {
            printf("recv param fail\n");
            nl_socket_free(sock);
            return ret;
        }
    }

    nl_socket_free(sock);
    return 0;
}


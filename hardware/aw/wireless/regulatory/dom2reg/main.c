#define LOG_TAG "dom2reg"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <linux/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <cutils/properties.h>
#include <log/log.h>
#include <unistd.h>

#include "nl80211.h"

/* support for extack if compilation headers are too old */
#ifndef NETLINK_EXT_ACK
#define NETLINK_EXT_ACK 11
enum nlmsgerr_attrs {
	NLMSGERR_ATTR_UNUSED,
	NLMSGERR_ATTR_MSG,
	NLMSGERR_ATTR_OFFS,
	NLMSGERR_ATTR_COOKIE,
	__NLMSGERR_ATTR_MAX,
	NLMSGERR_ATTR_MAX = __NLMSGERR_ATTR_MAX - 1
};
#endif
#ifndef NLM_F_CAPPED
#define NLM_F_CAPPED 0x100
#endif
#ifndef NLM_F_ACK_TLVS
#define NLM_F_ACK_TLVS 0x200
#endif
#ifndef SOL_NETLINK
#define SOL_NETLINK 270
#endif

struct nl80211_state {
	struct nl_sock *nl_sock;
	int nl80211_id;
};

static int reg_debug = 0;

/* libnl 1.x compatibility code */
#if !defined(CONFIG_LIBNL20) && !defined(CONFIG_LIBNL30)
#define nl_sock nl_handle
static inline struct nl_handle *nl_socket_alloc(void)
{
	return nl_handle_alloc();
}

static inline void nl_socket_free(struct nl_sock *h)
{
	nl_handle_destroy(h);
}

static inline int nl_socket_set_buffer_size(struct nl_sock *sk,
					    int rxbuf, int txbuf)
{
	return nl_set_buffer_size(sk, rxbuf, txbuf);
}
#endif /* CONFIG_LIBNL20 && CONFIG_LIBNL30 */

static int nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nl_sock = nl_socket_alloc();
	if (!state->nl_sock) {
		ALOGE("Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_sock)) {
		ALOGE("Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

	/* try to set NETLINK_EXT_ACK to 1, ignoring errors */
	err = 1;
	setsockopt(nl_socket_get_fd(state->nl_sock), SOL_NETLINK,
		   NETLINK_EXT_ACK, &err, sizeof(err));

	state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
	if (state->nl80211_id < 0) {
		ALOGE("nl80211 not found.\n");
		err = -ENOENT;
		goto out_handle_destroy;
	}

	return 0;

 out_handle_destroy:
	nl_socket_free(state->nl_sock);
	return err;
}

static void nl80211_cleanup(struct nl80211_state *state)
{
	nl_socket_free(state->nl_sock);
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	(void)nla;
	struct nlmsghdr *nlh = (struct nlmsghdr *)err - 1;
	int len = nlh->nlmsg_len;
	struct nlattr *attrs;
	struct nlattr *tb[NLMSGERR_ATTR_MAX + 1];
	int *ret = arg;
	int ack_len = sizeof(*nlh) + sizeof(int) + sizeof(*nlh);

	*ret = err->error;

	if (!(nlh->nlmsg_flags & NLM_F_ACK_TLVS))
		return NL_STOP;

	if (!(nlh->nlmsg_flags & NLM_F_CAPPED))
		ack_len += err->msg.nlmsg_len - sizeof(*nlh);

	if (len <= ack_len)
		return NL_STOP;

	attrs = (void *)((unsigned char *)nlh + ack_len);
	len -= ack_len;

	nla_parse(tb, NLMSGERR_ATTR_MAX, attrs, len, NULL);
	if (tb[NLMSGERR_ATTR_MSG]) {
		len = strnlen((char *)nla_data(tb[NLMSGERR_ATTR_MSG]),
			      nla_len(tb[NLMSGERR_ATTR_MSG]));
		ALOGD("kernel reports: %*s", len,
			(char *)nla_data(tb[NLMSGERR_ATTR_MSG]));
	}

	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	(void)msg;
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	(void)msg;
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static bool is_regdom_valid(char *alpha2)
{
	/* ASCII 0 */
	if (alpha2[0] == 48 && alpha2[1] == 48)
		return true;

	if (*alpha2 < 65 || *alpha2 > 90 ||
		*(alpha2 + 1) < 65 || *(alpha2 + 1) > 90)
		return false;

	return true;
}

static int set_reg(char *alpha2)
{
	struct nl80211_state nlstate;
	struct nl_msg *msg;
	struct nl_cb *cb, *s_cb;
	int err;
	nlstate.nl80211_id = 0;
	err = nl80211_init(&nlstate);
	if (err)
		return 1;

	msg = nlmsg_alloc();
	if (!msg) {
		ALOGE("failed to allocate netlink message");
		return 2;
	}

	cb = nl_cb_alloc(reg_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	s_cb = nl_cb_alloc(reg_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	if (!cb || !s_cb) {
		ALOGE("failed to allocate netlink callbacks");
		err = 2;
		goto out;
	}

	genlmsg_put(msg, 0, 0, nlstate.nl80211_id, 0, 0, NL80211_CMD_REQ_SET_REG, 0);

	if (!is_regdom_valid(alpha2)) {
		ALOGE("not a valid ISO/IEC 3166-1 alpha2");
		ALOGE("Special non-alpha2 usable entries:");
		ALOGE("\t00\tWorld Regulatory domain");
		return 2;
	}

	NLA_PUT_STRING(msg, NL80211_ATTR_REG_ALPHA2, alpha2);

	nl_socket_set_cb(nlstate.nl_sock, s_cb);

	err = nl_send_auto_complete(nlstate.nl_sock, msg);
	if (err < 0)
		goto out;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

	while (err > 0)
		nl_recvmsgs(nlstate.nl_sock, cb);

out:
	nl_cb_put(cb);
	nl_cb_put(s_cb);
	nlmsg_free(msg);
	goto done;

nla_put_failure:
	ALOGD("building message failed");
	err = -ENOBUFS;

done:
	if (err < 0)
		ALOGD("command failed: %s (%d)", strerror(-err), err);

	nl80211_cleanup(&nlstate);

	return err;
}

struct country_tz_t {
	const char *country;
	const char *timezone;
};

#include "country_tz.h"

#define TZ_PROP_NAME  "persist.sys.timezone"

enum handle_state {
	DETECT,
	PARSE,
	SET,
	STORE,
};

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	char alpha2[3];
	int i, ret, reg_set_retry = 0;
	char tz_cur[PROPERTY_VALUE_MAX] = {0};
	char tz_pre[PROPERTY_VALUE_MAX] = {0};
	enum handle_state state = DETECT;

	memset(alpha2, 0, sizeof(alpha2));

	while (true) {
		switch (state) {
			case DETECT:
				if ((property_get(TZ_PROP_NAME, tz_cur, NULL) > 0) &&
					((strlen(tz_cur) != strlen(tz_pre)) ||
					(strcmp(tz_cur, tz_pre) != 0))) {
					ALOGD("timezone change, new: %s, pre: %s", tz_cur, tz_pre);
					state = PARSE;
				} else {
					usleep(2000000);
				}
				break;
			case PARSE:
				for (i = 0; i < sizeof(search_tab) / sizeof(search_tab[0]); i++) {
					if ((strlen(tz_cur) == strlen(search_tab[i].timezone)) &&
						(strcmp(tz_cur, search_tab[i].timezone) == 0)) {
						memcpy(alpha2, search_tab[i].country, 2);
						ALOGD("convert timezone to alpha2 country code: %s", alpha2);
						state = SET;
						break;
					}
				}
				if (state == PARSE) {
					ALOGW("timezone [%s] not in default tables, please check!", tz_cur);
					usleep(2000000);
					state = DETECT;
				}
				break;
			case SET:
				ret = set_reg(alpha2);
				ALOGD("regulatory set to [%s] %s(%d)", alpha2, ret ? "fail" : "success", ret);
				if (ret == 0)
					state = STORE;
				else {
					if (reg_set_retry >= 10) {
						ALOGE("to manany fail to set regulatory, skip");
						state = STORE;
					} else {
						usleep(2000000);
					}
				}
				break;
			case STORE:
				reg_set_retry = 0;
				strcpy(&tz_pre[0], tz_cur);
				state = DETECT;
				break;
			default:
				state = DETECT;
				break;
		}
	}
	return 0;
}

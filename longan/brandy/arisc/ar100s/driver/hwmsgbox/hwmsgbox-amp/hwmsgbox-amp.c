/**
 * driver\hwmsgbox\hwmsgbox.c
 *
 * Descript: general hardware message-box driver (except PSCI).
 * Copyright (C) 2012-2016 AllWinnertech Ltd.
 * Author: AWA1442 <huangyongxing@allwinnertech.com>
 *
 */

#include "hwmsgbox-amp.h"

static struct notifier *msgbox_rec_list;

/*
 * It is an example tells how to send and recieve data to/from
 * other remote core. More callback function can be add into
 * msgbox notifier list by means of 'amp_msgbox_register_service'
 * so that these callback function can be excute when recieve
 * message from other remote core.
 *
static struct amp_msg msg_test ={
	.local_amp = 1,
	.remote_amp = 0,
	.write_ch = 1,
	.read_ch = 1
};

s32 risc_receive_from_arm(u32 remote, u32 channel)
{
	u32 data;
	u8 data1[4];
	struct amp_msg *msg = &msg_test;

	while (readl(AMP_MSGBOX_MSG_STA_REG(AMP_LOCAL, remote, channel))) {
		data = readl(AMP_MSGBOX_MSG_REG(AMP_LOCAL, calculte_n(AMP_LOCAL, remote), channel));
		LOG("Read from box%d channel%d success! data =%d!\n", remote, channel, data);

		data1[0] = ((data >> 0) & 0xff);
		data1[1] = ((data >> 8) & 0xff);
		data1[2] = ((data >> 16) & 0xff);
		data1[3] = ((data >> 24) & 0xff);

		amp_msgbox_send_message(msg, data1, 4, 1000);
		LOG("Write to box%d channel%d success! data = %d!\n", msg->remote_amp, msg->write_ch, data);
		LOG(" \n");
	}

	return OK;
}
 */


int calculte_n(int local, int remote)
{
	if (remote < local)
		return remote;
	else
		return remote - 1;
}


s32 amp_msgbox_register_service(__pNotifier_t pcb)
{
	return notifier_insert(&msgbox_rec_list, pcb);
}

s32 amp_msgbox_init(void)
{
//	amp_msgbox_register_service(risc_receive_from_arm);

	return OK;
}

s32 amp_msgbox_query_message(void)
{
	u32 remote, ch;

	for (remote = 0; remote < MSGBOX_NUM; remote++) {
		if (remote == AMP_LOCAL)
			continue;

		for (ch = 0; ch < CHANNEL_MAX; ch++) {
			if ((remote == AMP_PSCI) && (ch == CHANNEL_PSCI))
				continue;

			if (!!readl(MSGBOX_MSG_STA_REG(AMP_LOCAL, calculte_n(AMP_LOCAL, remote), ch)))
				notifier_notify(&msgbox_rec_list, remote, ch);
		}
	}

	return OK;
}


s32 amp_msgbox_channel_send_data(struct amp_msg *msg, u32 data, u32 timeout)
{
	while (readl(AMP_MSGBOX_MSG_STA_REG(msg->remote_amp, calculte_n(msg->remote_amp,
		msg->local_amp), msg->write_ch)) == MSGBOX_MAX_QUEUE) {
		if (timeout == 0) {
			ERR("amp_msgbox_channel_send_data timeout!\n");
			return -ETIMEOUT;
		}
		time_mdelay(1);
		timeout--;
	};

	writel(data, AMP_MSGBOX_MSG_REG(msg->remote_amp,
		calculte_n(msg->remote_amp, msg->local_amp), msg->write_ch));

	return OK;
}


s32 amp_msgbox_send_message(struct amp_msg *msg, u8 *buff, int len, u32 timeout)
{
	u32 data = 0;
	u32 i;
	s32 ret;

	for (i = 0; i < len; i++) {
		if (!(i % 4))
			data = 0;

		data |= *buff++ << ((i % 4) << 3);

		if ((i % 4) == 3 || i == len - 1) {
			ret = amp_msgbox_channel_send_data(msg, data, timeout);
			if (ret != OK)
				return ret;
		}
	}

	return 0;
}

u32 amp_msgbox_read_message(u32 remote, u32 channel)
{

	u32 data;

	data = readl(AMP_MSGBOX_MSG_REG(AMP_LOCAL, calculte_n(AMP_LOCAL, remote), channel));

	return data;
}

u32 amp_msgbox_remote_fifo_is_empty(u32 remote, u32 channel)
{
	u32 data;

	data = readl(AMP_MSGBOX_MSG_STA_REG(AMP_LOCAL, calculte_n(AMP_LOCAL, remote), channel));

	return (data == 0) ? 1 : 0;
}

u32 amp_msgbox_remote_fifo_is_full(u32 remote, u32 channel)
{
	u32 data;

	data = readl(AMP_MSGBOX_MSG_STA_REG(AMP_LOCAL, calculte_n(AMP_LOCAL, remote), channel));

	return (data == MSGBOX_MAX_QUEUE) ? 1 : 0;
}

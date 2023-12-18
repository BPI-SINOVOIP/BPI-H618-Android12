#include <linux/kernel.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dlfcn.h>


#include "hciattach.h"
#include "hciattach_sprd.h"
#include "bt_vendor_lib.h"
#include "bt_hci_bdroid.h"
#include "bt_types.h"

#define LOG_TAG "hciattach_sprd"

#include <utils/Log.h>

#define UNUSED(expr) do { (void)(expr); } while (0)
#define UNUSED_ATTR __attribute__((unused))

#define HCI_PSKEY 0xFCA0
#define HCI_VSC_ENABLE_COMMMAND 0xFCA1
#define HCI_RF_PARA 0xFCA2
#define RESPONSE_LENGTH 100
#define ARRAY_SIZE(arr)         (sizeof(arr) / sizeof((arr)[0]))


static const char *VENDOR_LIBRARY_NAME[] = {
	"libbt-sprd.so",
	"libbt-vendor.so"};

static const char *VENDOR_LIBRARY_SYMBOL_NAME = "BLUETOOTH_VENDOR_LIB_INTERFACE";

static uint8_t local_bdaddr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static int s_bt_fd = -1;

static void *lib_handle;
static bt_vendor_interface_t *lib_interface;

static void dump_data(uint8_t *p_buf, int data_len) {
	int i, buf_printf_len = data_len * 3 + 1;
	char *p_buf_pskey_printf = malloc(buf_printf_len);
	char *p = p_buf_pskey_printf;

	memset(p_buf_pskey_printf, 0, buf_printf_len);
	ALOGD("start to dump_data");
	for (i = 0; i < data_len; i++)
		p += snprintf(p, buf_printf_len, "%02x ", p_buf[i]);
	ALOGD("%s", p_buf_pskey_printf);
	free(p_buf_pskey_printf);
	p_buf_pskey_printf = NULL;
}

static int recv_event(tINT_CMD_CBACK callback) {
	int read_data_len = 0;
	uint8_t new_state = BT_VND_LPM_WAKE_ASSERT;
	char *event_resp = malloc(RESPONSE_LENGTH + sizeof(HC_BT_HDR) - 1);
	char *p_recv = event_resp + sizeof(HC_BT_HDR) - 1;

	memset(event_resp, 0, RESPONSE_LENGTH);

	while (1) {
		usleep(100 * 1000);
		lib_interface->op(BT_VND_OP_LPM_WAKE_SET_STATE, &new_state);
		printf("wait for rx data +++\n");
		read_data_len += read(s_bt_fd, p_recv + read_data_len, RESPONSE_LENGTH);
		printf("wait for rx data %d---\n", read_data_len);
		if (read_data_len < 0) {
			ALOGI("Failed to read ack, read_data_len=%d(%s)", read_data_len , strerror(errno));
			free(event_resp);
			return -1;
		} else if (read_data_len < 3) {
			continue;
		}

		dump_data((uint8_t*)p_recv, read_data_len);

		if (p_recv[0] == 0x04 && read_data_len >= (p_recv[2] + 2)) {
			ALOGI("read  ACK(0x04) ok \n");
			break;
		} else if (p_recv[0] == 0x04) {
			continue;
		} else {
			ALOGE("read ACK(0x%x)is not expect,retry\n,", p_recv[0]);
		}
	}
	callback(event_resp);
	return read_data_len;
}


/* send hci command */
static int send_cmd(uint8_t *p_buf, int data_len) {
	ssize_t ret = 0;
	uint16_t total = 0;

	dump_data(p_buf, data_len);

	/* Send command via HC's xmit_cb API */
	ALOGD("wirte command size=%d", data_len);
	while (data_len) {
		ret = write(s_bt_fd, p_buf + total, data_len);
		ALOGD("wrote data_len= %d, ret size=%d", data_len, ret);
		switch (ret) {
			case -1:
				ALOGD("%s error writing to serial port: %s", __func__, strerror(errno));
				return total;
			case 0:  // don't loop forever in case write returns 0.
				return total;
			default:
				total += ret;
				data_len -= ret;
				break;
		}
	}
	return 0;
}

static void firmware_config_cb(bt_vendor_op_result_t result) {
	UNUSED(result);
}

static void sco_config_cb(bt_vendor_op_result_t result) {
  	UNUSED(result);
}

static void low_power_mode_cb(bt_vendor_op_result_t result) {
	UNUSED(result);
}


static void *buffer_alloc_cb(int size) {
	char *puf = malloc(size);
	memset(puf, 0, size);
	return puf;
}

static void buffer_free_cb(void *buffer) {
	free(buffer);
}

static uint8_t transmit_cb(UNUSED_ATTR uint16_t opcode, void *buffer, tINT_CMD_CBACK callback) {
	HC_BT_HDR *p_buf = (HC_BT_HDR *)buffer;
	int len = p_buf->len+1;
	uint8_t *p = (uint8_t *)buffer;

	UNUSED(callback);

	if ((opcode == HCI_PSKEY) || (opcode == HCI_VSC_ENABLE_COMMMAND)
        || (opcode == HCI_RF_PARA)) {
		p  = p  + sizeof(HC_BT_HDR) - 1;
		*p  = 0x01;	// hci command
		send_cmd(p, len);
		free(buffer);
		recv_event(callback);
	}
  return true;
}

static void epilog_cb(bt_vendor_op_result_t result) {
	ALOGD("%s, result=%d", __func__, result);
}

static void a2dp_offload_cb(bt_vendor_op_result_t result, bt_vendor_opcode_t op, uint8_t bta_av_handle) {
  ALOGD("result=%d, op=%d, bta_av_handle=%d", result, op, bta_av_handle);
}


static bt_vendor_callbacks_t lib_callbacks = {
	sizeof(lib_callbacks),
	firmware_config_cb,
	sco_config_cb,
	low_power_mode_cb,
    NULL,
	buffer_alloc_cb,
	buffer_free_cb,
	transmit_cb,
	epilog_cb,
    NULL,
};


int sprd_vendor_init(void)
{
	bt_vendor_callbacks_t * lib_cb = &lib_callbacks;
	int status = -1, i;

	for (i = 0; i < ARRAY_SIZE(VENDOR_LIBRARY_NAME); i++) {
		lib_handle = dlopen(VENDOR_LIBRARY_NAME[i], RTLD_NOW);
		if (!lib_handle) {
			ALOGW("%s unable to open %s: %s, continue", __func__, VENDOR_LIBRARY_NAME[i], dlerror());
			continue;
		}
		status = 0;
		break;
	}

	if (status) {
		ALOGE("%s unable to find suit libbt-<xx>.so, break handle!", __func__);
		goto error;
	}

	lib_interface = (bt_vendor_interface_t *)dlsym(lib_handle, VENDOR_LIBRARY_SYMBOL_NAME);
	if (!lib_interface) {
		ALOGD("%s unable to find symbol %s in %s: %s", __func__, VENDOR_LIBRARY_SYMBOL_NAME, VENDOR_LIBRARY_NAME[i], dlerror());
		goto error;
	}

	status = lib_interface->init(lib_cb, (unsigned char *)local_bdaddr);
	if (status) {
		ALOGD("%s unable to initialize vendor library: %d", __func__, status);
		goto error;
	}

	return 0;

error:
	lib_interface = NULL;
	if (lib_handle)
		dlclose(lib_handle);

	lib_handle = NULL;
	return -1;
}

int sprd_vendor_power_on(void)
{
	int power_state = BT_VND_PWR_OFF;

	lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
	power_state = BT_VND_PWR_ON;
	lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
	return 0;
}

int sprd_vendor_power_off(void)
{
	int power_state = BT_VND_PWR_OFF;

	if (lib_interface) {
	lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
	} else {
		ALOGE("%s failed", __func__);
		return -1;
	}
	return 0;
}

int sprd_vendor_config_init(int fd, char *bdaddr, struct termios *ti)
{
    UNUSED(ti);
    UNUSED(bdaddr);
	s_bt_fd = fd;

	sprd_vendor_init();

	sprd_vendor_power_on();

	if (lib_interface) {
		lib_interface->op(BT_VND_OP_FW_CFG, NULL);
	} else {
		ALOGE("%s failed", __func__);
		return -1;
	}
	return 0;
}

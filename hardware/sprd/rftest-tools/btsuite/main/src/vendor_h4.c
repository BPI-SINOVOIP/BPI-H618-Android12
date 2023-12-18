/******************************************************************************
 *
 *  Copyright (C) 2017 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <dlfcn.h>
#include <log/log.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "vendor_h4.h"

#include "bt_types.h"
#include "bt_vendor_lib.h"
#include "bt_hci_bdroid.h"

#include "osi/include/eager_reader.h"
#include "osi/include/osi.h"
#include "osi/include/log.h"
#include "osi/include/reactor.h"
#include "osi/include/semaphore.h"

#define HCI_HAL_SERIAL_BUFFER_SIZE 1026

static eager_reader_t *uart_stream;
static uint8_t stream_has_interpretation;
static const hci_h4_callbacks_t *callbacks;
static serial_data_type_t current_data_type;
static thread_t *thread; // Not owned by us



#define HCI_PSKEY   0xFCA0
#define HCI_VSC_ENABLE_COMMMAND 0xFCA1
#define RDWR_FD_FAIL (-2)
#define GENERIC_FAIL (-1)

#define PSKEY_CMD 0xFCA0
#define VENDOR_CMD_NONSIGNAL_START 0xFCD1
#define VENDOR_CMD_NONSIGNAL_STOP 0xFCD2

#define RESPONSE_LENGTH 100


static const char *VENDOR_LIBRARY_NAME[] = {
    "libbt-sprd.so",
    "libbt-vendor.so"};
static const char *VENDOR_LIBRARY_SYMBOL_NAME = "BLUETOOTH_VENDOR_LIB_INTERFACE";

static void *lib_handle;
static bt_vendor_interface_t *lib_interface;

static int uart_fd;

static uint8_t local_bdaddr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint32_t idle_timeout_ms;

extern const allocator_t *buffer_allocator;

static semaphore_t *firmware_cfg_sem;
static semaphore_t *epilog_sem;
static semaphore_t *lpm_sem;


extern void transmit_command(
    BT_HDR *command,
    command_complete_cb complete_callback,
    command_status_cb status_callback,
    void *context);

#if 0
/* dump received event or sent command */
static void dump_data(uint8_t *p_buf, int data_len) {
    int i = 0;
    int buf_printf_len = data_len * 3 + 1;
    char *p_buf_pskey_printf = (char*)malloc(buf_printf_len);
    memset(p_buf_pskey_printf, 0, buf_printf_len);
    char *p = p_buf_pskey_printf;
    BTD("start to dump_data");
    for (i = 0; i < data_len; i++)
        p += snprintf(p, buf_printf_len, "%02x ", p_buf[i]);
    BTD("%s", p_buf_pskey_printf);
    free(p_buf_pskey_printf);
    p_buf_pskey_printf = NULL;
}

/* send hci command */
static int send_cmd(uint8_t *p_buf, int data_len) {
    int ret = 0;

    dump_data(p_buf, data_len);

    /* Send command via HC's xmit_cb API */
    BTD("wirte command size=%d", data_len);
    uint16_t total = 0;
    while (data_len) {
        ret = write(uart_fd, p_buf + total, data_len);
        BTD("wrote data_len: %d, ret size: %d", data_len, ret);
        switch (ret) {
            case -1:
                ALOGE("%s error writing to serial port: %s", __func__, strerror(errno));
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

/* receive event in while loop */
static int recv_event(tINT_CMD_CBACK callback) {
    int read_data_len = 0;
    uint8_t new_state = BT_VND_LPM_WAKE_ASSERT;
    char *event_resp = malloc(RESPONSE_LENGTH + sizeof(HC_BT_HDR) - 1);
    memset(event_resp, 0, RESPONSE_LENGTH);
    char *p_recv = event_resp + sizeof(HC_BT_HDR) - 1;

    while (1) {
        usleep(100 * 1000);
        lib_interface->op(BT_VND_OP_LPM_WAKE_SET_STATE, &new_state);
        read_data_len += read(uart_fd, p_recv + read_data_len, RESPONSE_LENGTH);
        if (read_data_len < 0) {
            BTD("Failed to read ack, read_data_len=%d(%s)", read_data_len , strerror(errno));
            free(event_resp);
            return -1;
        } else if (read_data_len < 3) {
            continue;
        }

        dump_data((uint8_t*)p_recv, read_data_len);

        if (p_recv[0] == 0x04 && read_data_len >= (p_recv[2] + 2)) {
            BTD("read  ACK(0x04) ok \n");
            break;
        } else if (p_recv[0] == 0x04) {
            continue;
        } else {
            BTE("read ACK(0x%x)is not expect,retry\n,", p_recv[0]);
        }
    }
    callback(event_resp);
    return read_data_len;
}
#endif

uint16_t transmit_data(serial_data_type_t type, uint8_t *data, uint16_t length) {
  assert(data != NULL);
  assert(length > 0);

  if (type < DATA_TYPE_COMMAND || type > DATA_TYPE_SCO) {
    LOG_ERROR("%s invalid data type: %d", __func__, type);
    return 0;
  }

  // Write the signal byte right before the data
  --data;
  uint8_t previous_byte = *data;
  *(data) = type;
  ++length;

  uint16_t transmitted_length = 0;
  while (length > 0) {
    ssize_t ret = write(uart_fd, data + transmitted_length, length);
    switch (ret) {
      case -1:
        LOG_ERROR("In %s, error writing to the uart serial port: %s", __func__, strerror(errno));
        goto done;
      case 0:
        // If we wrote nothing, don't loop more because we
        // can't go to infinity or beyond
        goto done;
      default:
        transmitted_length += ret;
        length -= ret;
        break;
    }
  }

done:;
  // Be nice and restore the old value of that byte
  *(data) = previous_byte;

  // Remove the signal byte from our transmitted length, if it was actually written
  if (transmitted_length > 0)
    --transmitted_length;

  return transmitted_length;
}

static void event_uart_has_bytes(eager_reader_t *reader, UNUSED_ATTR void *context) {
  if (stream_has_interpretation) {
    callbacks->data_ready(current_data_type);
  } else {
    uint8_t type_byte;
    if (eager_reader_read(reader, &type_byte, 1, true) == 0) {
      BTE("%s could not read HCI message type", __func__);
      return;
    }
    if (type_byte < DATA_TYPE_ACL || type_byte > DATA_TYPE_EVENT) {
      BTE("%s Unknown HCI message type. Dropping this byte 0x%x, min %x, max %x", __func__, type_byte, DATA_TYPE_ACL, DATA_TYPE_EVENT);
      return;
    }

    stream_has_interpretation = true;
    current_data_type = type_byte;
  }
}

static void firmware_config_cb(bt_vendor_op_result_t result) {
    BTD("%s, result: %d", __func__, result);
    semaphore_post(firmware_cfg_sem);
}

static void sco_config_cb(bt_vendor_op_result_t result) {
  UNUSED(result);
}

static void low_power_mode_cb(bt_vendor_op_result_t result) {
    BTD("%s, result: %d", __func__, result);
    semaphore_post(lpm_sem);
}

static void sco_audiostate_cb(bt_vendor_op_result_t result) {
  UNUSED(result);
}

static void *buffer_alloc_cb(int size) {
  return buffer_allocator->alloc(size);
}

static void buffer_free_cb(void *buffer) {
  buffer_allocator->free(buffer);
}

static void transmit_completed_callback(BT_HDR *response, void *context) {
  // Call back to the vendor library if it provided a callback to call.
  if (context)
    ((tINT_CMD_CBACK)context)(response);
}

static uint8_t transmit_cb(UNUSED_ATTR uint16_t opcode, void *buffer, tINT_CMD_CBACK callback) {
  transmit_command((BT_HDR *)buffer, transmit_completed_callback, NULL, callback);
  return true;
}
static void epilog_cb(bt_vendor_op_result_t result) {
    BTD("%s, result: %d", __func__, result);
    semaphore_post(epilog_sem);
}

#if (defined(SPRD_FEATURE_A2DPOFFLOAD) && SPRD_FEATURE_A2DPOFFLOAD == TRUE)
static void a2dp_offload_cb(bt_vendor_op_result_t result, bt_vendor_opcode_t op, uint8_t bta_av_handle) {
  BTD("result: %d, op: %d, bta_av_handle: %d", result, op, bta_av_handle);
}
#endif

static bt_vendor_callbacks_t lib_callbacks = {
    sizeof(lib_callbacks),
    firmware_config_cb,
    sco_config_cb,
    low_power_mode_cb,
    sco_audiostate_cb,
    buffer_alloc_cb,
    buffer_free_cb,
    transmit_cb,
    epilog_cb,
#if (defined(SPRD_FEATURE_A2DPOFFLOAD) && SPRD_FEATURE_A2DPOFFLOAD == TRUE)
    a2dp_offload_cb,
#endif
};

static int vendor_open(const uint8_t *local_bdaddr) {
    int status = -1, i;
    BTD("%s", __func__);

    if (lib_handle != NULL) {
        BTW("libbt-vendor handle should be null");
    }

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

    if (!lib_handle) {
        BTE("%s unable to open %s: %s", __func__, VENDOR_LIBRARY_NAME[i], dlerror());
        goto error;
    }

    lib_interface = (bt_vendor_interface_t *)dlsym(lib_handle, VENDOR_LIBRARY_SYMBOL_NAME);
    if (!lib_interface) {
        BTE("%s unable to find symbol %s in %s: %s", __func__, VENDOR_LIBRARY_SYMBOL_NAME, VENDOR_LIBRARY_NAME[i], dlerror());
        goto error;
    }

    if (lib_interface->init(&lib_callbacks, (unsigned char *)local_bdaddr) != 0) {
        BTE("%s unable to initialize vendor library", __func__);
        goto error;
    }

  return 0;

error:;

  if (lib_handle)
    dlclose(lib_handle);
  lib_handle = NULL;
  lib_interface = NULL;

  return -1;
}

static void vendor_close(void) {
    BTD("%s", __func__);

    if (lib_handle)
        dlclose(lib_handle);
    lib_interface = NULL;
    lib_handle = NULL;
}


static void lmp_enable(void) {
    uint8_t command = BT_VND_LPM_ENABLE;
    lib_interface->op(BT_VND_OP_GET_LPM_IDLE_TIMEOUT, &idle_timeout_ms);
    lib_interface->op(BT_VND_OP_LPM_SET_MODE, &command);
}

static void lmp_disable(void) {
    uint8_t command = BT_VND_LPM_DISABLE;
    lib_interface->op(BT_VND_OP_LPM_SET_MODE, &command);
}

void lmp_assert(void) {
    uint8_t command = BT_VND_LPM_WAKE_ASSERT;
    lib_interface->op(BT_VND_OP_LPM_WAKE_SET_STATE, &command);
}

void lmp_deassert(void) {
    uint8_t command = BT_VND_LPM_WAKE_DEASSERT;
    lib_interface->op(BT_VND_OP_LPM_WAKE_SET_STATE, &command);
}

size_t read_data(serial_data_type_t type, uint8_t *buffer, size_t max_size, bool block) {
  if (type < DATA_TYPE_ACL || type > DATA_TYPE_EVENT) {
    LOG_ERROR("%s invalid data type: %d", __func__, type);
    return 0;
  } else if (!stream_has_interpretation) {
    LOG_ERROR("%s with no valid stream intepretation.", __func__);
    return 0;
  } else if (current_data_type != type) {
    LOG_ERROR("%s with different type than existing interpretation.", __func__);
    return 0;
  }

  return eager_reader_read(uart_stream, buffer, max_size, block);
}

void packet_finished(serial_data_type_t type) {
  if (!stream_has_interpretation)
    LOG_ERROR("%s with no existing stream interpretation.", __func__);
  else if (current_data_type != type)
    LOG_ERROR("%s with different type than existing interpretation.", __func__);

  stream_has_interpretation = false;
}

int h4_start_up(const hci_h4_callbacks_t *upper_callbacks, thread_t *upper_thread) {
    int fd_array[CH_MAX];
    int power_state = BT_VND_PWR_OFF;

    BTD("%s", __func__);

    firmware_cfg_sem = semaphore_new(0);
    epilog_sem = semaphore_new(0);
    lpm_sem = semaphore_new(0);

    vendor_open(local_bdaddr);

    callbacks = upper_callbacks;
    thread = upper_thread;

    lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
    power_state = BT_VND_PWR_ON;
    lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
    lib_interface->op(BT_VND_OP_USERIAL_OPEN, &fd_array);
    uart_fd = fd_array[0];


    uart_stream = eager_reader_new(uart_fd, &allocator_malloc, HCI_HAL_SERIAL_BUFFER_SIZE, SIZE_MAX, "hci_single_channel");
    if (!uart_stream) {
        BTE("%s unable to create eager reader for the uart serial port.", __func__);
        goto error;
    }


    stream_has_interpretation = FALSE;
    eager_reader_register(uart_stream, thread_get_reactor(thread), event_uart_has_bytes, NULL);



    lmp_enable();
    BTE("wait lpm_sem ++");
    semaphore_wait(lpm_sem);
    BTE("wait lpm_sem --");

    lmp_assert();

    lib_interface->op(BT_VND_OP_FW_CFG, NULL);

    BTE("wait firmware_cfg_sem ++");
    semaphore_wait(firmware_cfg_sem);
    BTE("wait firmware_cfg_sem --");

    return 0;

error:;


    return -1;
}

void h4_shut_down(void) {
    int power_state = BT_VND_PWR_OFF;

    BTD("%s+++", __func__);

    lib_interface->op(BT_VND_OP_EPILOG, NULL);
    BTE("wait epilog_sem ++");
    semaphore_wait(epilog_sem);
    BTE("wait epilog_sem --");

    eager_reader_free(uart_stream);

    lib_interface->op(BT_VND_OP_USERIAL_CLOSE, NULL);

    lmp_deassert();
    lmp_disable();
    BTE("wait lpm_sem ++");
    semaphore_wait(lpm_sem);
    BTE("wait lpm_sem --");

    lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
    lib_interface->cleanup();
    uart_fd = INVALID_FD;
    vendor_close();

    semaphore_free(firmware_cfg_sem);
    semaphore_free(epilog_sem);
    semaphore_free(lpm_sem);

    BTD("%s---", __func__);
}

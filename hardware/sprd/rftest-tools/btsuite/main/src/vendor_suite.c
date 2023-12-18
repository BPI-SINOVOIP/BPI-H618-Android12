/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
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

#include <assert.h>
#include <cutils/properties.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "bt_types.h"

#include "vendor_h4.h"
#include "vendor_hci.h"
#include "vendor_hcif.h"
#include "vendor_utils.h"
#include "vendor_hcicmds.h"
#include "vendor_test.h"
#include "vendor_suite.h"
#include "hcidefs.h"

#include "buffer_allocator.h"


#include "osi/include/eager_reader.h"
#include "osi/include/osi.h"
#include "osi/include/log.h"
#include "osi/include/reactor.h"
#include "osi/include/thread.h"
#include "osi/include/future.h"
#include "osi/include/list.h"
#include "osi/include/fixed_queue.h"
#include "osi/include/semaphore.h"
#define DISCOVERY_TIMEOUT 1000

static semaphore_t *address_sem;
static semaphore_t *device_sem;
static bool timer_created = false;
static timer_t discovery_timer_t;
bool device_sem_post;
bdremote_t *bdrmt_suite_dev = NULL;
static BD_ADDR local_addr;

static int enable(void)
{
    BTD("%s", __func__);
    address_sem = semaphore_new(0);
    device_sem = semaphore_new(0);
    hcif_start_up();
    hci_start_up();

    return 0;
}

static void disable(void)
{
    BTD("%s", __func__);

    hci_shut_down();
    hcif_shut_down();

    if (timer_created == true)
    {
        timer_delete(discovery_timer_t);
        timer_created = false;
    }

    semaphore_free(address_sem);
    semaphore_free(device_sem);
}

static void hci_cmd_send(uint16_t opcode, size_t len, uint8_t* buf, hci_cmd_complete_cb *p)
{
    BTD("%s", __func__);
    BTM_SendHciCommand(opcode, len & 0xFF, buf, p);
}

static int dut_mode_enable(void)
{
    return bt_test_mode_enable();
}

static void dut_mode_disable(void)
{
    bt_test_mode_disable();
}

void read_local_address_cb(BD_ADDR address) {
    BTD("%s", __func__);
    memcpy(local_addr, address, sizeof(BD_ADDR));
    semaphore_post(address_sem);
}

static int read_local_address(BD_ADDR address) {
    btsnd_hcic_read_bd_addr();
    semaphore_wait(address_sem);
    memcpy(address, local_addr, sizeof(BD_ADDR));
    return 0;
}

void read_discovery_device_cb(void) {
 //   BTD("%s", __func__);
    if (timer_created == true)
    {
        timer_delete(discovery_timer_t);
        timer_created = false;
    }
    semaphore_post(device_sem);
}

/*void read_discovery_device_timeout() {
    BTD("%s %d", __func__,sig_time->sival_int);
    if (timer_created == true)
    {
        BTD("%s timer_created = true", __func__);
        timer_delete(discovery_timer_t);
        timer_created = false;
    }
    semaphore_post(device_sem);
}*/

/*static int read_discovery_device(bdremote_t* bdrmt_dev) {
    int status;
    struct itimerspec ts;
    struct sigevent se;
    memset(&ts, 0, sizeof(struct itimerspec));
    memset(&se, 0, sizeof(struct sigevent));

    if (timer_created == false)
    {
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &discovery_timer_t;
        se.sigev_notify_function = read_discovery_device_timeout;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &discovery_timer_t);

        if (status == 0)
            timer_created = true;
        else
            BTD("Failed to creat discovery_timer");
    }

    if (timer_created == true)
    {
        ts.it_value.tv_sec = DISCOVERY_TIMEOUT / 1000;
        ts.it_value.tv_nsec = 1000000 * (DISCOVERY_TIMEOUT % 1000);
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(discovery_timer_t, 0, &ts, 0);
        if (status == -1)
            BTD("Failed to set discovery_timer");
    }

    bdrmt_suite_dev = bdrmt_dev;
    device_sem_post = false;
    semaphore_wait(device_sem);
    if(!device_sem_post)
      return -1;
    return 0;
}*/

const bt_vendor_suite_interface_t BT_VENDOR_SUITE_INTERFACE = {
    sizeof(bt_vendor_suite_interface_t),
    enable,
    disable,
    hci_cmd_send,
    dut_mode_enable,
    dut_mode_disable,
    read_local_address,
    //read_discovery_device,
};

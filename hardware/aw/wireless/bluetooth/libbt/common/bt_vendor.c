/******************************************************************************
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

/******************************************************************************
 *
 *  Filename:      bt_vendor.c
 *
 ******************************************************************************/

#define LOG_TAG "libbt-auto-detect"

#include <utils/Log.h>
#include <string.h>
#include <dlfcn.h>
#include "libhwinfo.h"
#include "bt_vendor_lib.h"

#define VENDOR_LIBRARY_SYMBOL_NAME "BLUETOOTH_VENDOR_LIB_INTERFACE"

static bt_vendor_interface_t *lib_interface;
static void *lib_handle;

static int init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    const char *libbt_name = get_bluetooth_libbt_name();
    ALOGD("%s, get libbt name: %s.", __func__, libbt_name);

    lib_handle = dlopen(libbt_name, RTLD_NOW);
    if (lib_handle == NULL) {
        ALOGE("dlopen %s fail", libbt_name);
        return -1;
    }

    lib_interface = (bt_vendor_interface_t *)dlsym(lib_handle, VENDOR_LIBRARY_SYMBOL_NAME);
    if (lib_interface == NULL) {
        dlclose(lib_handle);
        ALOGE("dlsym get interface %s fail", VENDOR_LIBRARY_SYMBOL_NAME);
        return -1;
    }

    return lib_interface->init(p_cb, local_bdaddr);
}

/** Requested operations */
static int op(bt_vendor_opcode_t opcode, void *param)
{
    return lib_interface->op(opcode, param);
}

/** Closes the interface */
static void cleanup(void)
{
    lib_interface->cleanup();
    if (lib_handle)
        dlclose(lib_handle);
}

// Entry point of DLib
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    init,
    op,
    cleanup
};

/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "audio_plugin"
//#define LOG_NDEBUG 0

#include <cutils/log.h>
#include "audio_plugin.h"

/*if PRINT_RW_LOG is true, print log on plugin_process_read_write */
#define PRINT_RW_LOG 0

#define UNUSED(x) (void)(x)

int plugin_process(struct audio_plugin *plugin, int flag)
{
    ALOGV("plugin_process(flag:%d): plugin_name:%s", flag, plugin->name);
    int ret = 0;

    switch(flag) {
        case ON_ADEV_OPEN:
            if (plugin->on_adev_open) plugin->on_adev_open();
            break;
        case ON_ADEV_CLOSE:
            if (plugin->on_adev_close) plugin->on_adev_close();
            break;
        case ON_SELECT_DEVICES:
            ALOGW("%s,Do not support flag %d", __func__, flag);
            break;

        case ON_OPEN_OUTPUT_STREAM:
            if (plugin->on_open_output_stream) plugin->on_open_output_stream();
            break;
        case ON_CLOSE_OUTPUT_STREAM:
            if (plugin->on_close_output_stream) plugin->on_close_output_stream();
            break;
        case ON_START_OUTPUT_STREAM:
            ALOGW("%s,Do not support flag %d", __func__, flag);
            break;
        case ON_OUT_STANDBY:
            if (plugin->on_out_standby) plugin->on_out_standby();
            break;
        case ON_OUT_WRITE:
            ALOGW("%s,Do not support flag %d", __func__, flag);
            break;

        case ON_OPEN_INPUT_STREAM:
            if (plugin->on_open_input_stream) plugin->on_open_input_stream();
            break;
        case ON_CLOSE_INPUT_STREAM:
            if (plugin->on_close_input_stream) plugin->on_close_input_stream();
            break;
        case ON_START_INPUT_STREAM:
            ALOGW("%s,Do not support flag %d", __func__, flag);
            break;
        case ON_IN_STANDBY:
            if (plugin->on_in_standby) plugin->on_in_standby();
            break;
        case ON_IN_READ:
            ALOGW("%s,Do not support flag %d", __func__, flag);
            break;
    }

    return ret;
}

int plugin_process_start_stream(struct audio_plugin *plugin, int flag,
                                struct pcm_config config)
{
    ALOGV("plugin_process_start_stream(flag:%d): plugin_name:%s",
          flag, plugin->name);
    switch(flag) {

        case ON_START_OUTPUT_STREAM:
            if (plugin->on_start_output_stream)
                plugin->on_start_output_stream(config);
            break;
        case ON_START_INPUT_STREAM:
            if(plugin->on_start_input_stream)
                plugin->on_start_input_stream(config);
            break;
    }

    return 0;
}

int plugin_process_select_devices(struct audio_plugin *plugin, int flag,
                                  int mode, int out_devices, int in_devices)
{
    ALOGV("plugin_process_select_devices(flag:%d): plugin_name:%s",
          flag, plugin->name);
    int ret = 0;

    switch(flag) {
        case ON_SELECT_DEVICES:
            if (plugin->on_select_devices)
                plugin->on_select_devices(mode, out_devices, in_devices);
            break;
    }

    return ret;
}

int plugin_process_read_write(struct audio_plugin *plugin, int flag,
                              struct pcm_config config, const void *buffer,
                              size_t bytes)
{
    ALOGV_IF(PRINT_RW_LOG, "plugin_process_read_write(flag:%d): plugin_name:%s",
             flag, plugin->name);
    int ret = 0;

    switch(flag) {
        case ON_OUT_WRITE:
            if (plugin->on_out_write)
                plugin->on_out_write(config, buffer, bytes);
            break;
        case ON_IN_READ:
            if (plugin->on_in_read)
                plugin->on_in_read(config, buffer, bytes);
            break;
    }

    return ret;
}

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

#ifndef _AUDIO_PLUGIN_H_
#define _AUDIO_PLUGIN_H_

#include "tinyalsa/asoundlib.h"

enum plugin_process_flag {
    ON_ADEV_OPEN,
    ON_ADEV_CLOSE,
    ON_SELECT_DEVICES,

    ON_OPEN_OUTPUT_STREAM,
    ON_CLOSE_OUTPUT_STREAM,
    ON_START_OUTPUT_STREAM,
    ON_OUT_STANDBY,
    ON_OUT_WRITE,

    ON_OPEN_INPUT_STREAM,
    ON_CLOSE_INPUT_STREAM,
    ON_START_INPUT_STREAM,
    ON_IN_STANDBY,
    ON_IN_READ,
};

struct audio_plugin {
    char name[50];
    void *handle;

    int (*on_adev_open)();
    int (*on_adev_close)();

    int (*on_select_devices)(int mode, int out_devices, int in_devices);

    int (*on_open_output_stream)();
    int (*on_close_output_stream)();
    int (*on_start_output_stream)(struct pcm_config config);
    int (*on_out_standby)();
    int (*on_out_write)(struct pcm_config config, const void *buffer,
                        size_t bytes);

    int (*on_open_input_stream)();
    int (*on_close_input_stream)();
    int (*on_start_input_stream)(struct pcm_config config);
    int (*on_in_standby)();
    int (*on_in_read)(struct pcm_config config, const void *buffer,
                      size_t bytes);
};

int plugin_process(struct audio_plugin *plugin, int flag);
int plugin_process_start_stream(struct audio_plugin *plugin, int flag,
                                struct pcm_config config);
int plugin_process_select_devices(struct audio_plugin *plugin, int flag,
                                  int mode, int out_devices, int in_devices);

int plugin_process_read_write(struct audio_plugin *plugin, int flag,
                              struct pcm_config config, const void *buffer,
                              size_t bytes);
#endif

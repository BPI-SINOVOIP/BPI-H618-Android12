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

#define LOG_TAG "audio_3d_surround"
//#define LOG_NDEBUG 0

#include <dlfcn.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <stdbool.h>
#include <stdlib.h>
#include <system/audio.h>
#include "tinyalsa/asoundlib.h"

#include "audio_plugin.h"
#include "audio_3d_surround.h"

#define UNUSED(x) (void)(x)

/* debug 3d surround parameters*/
//#define DEBUG_3D_SURROUND

#define SUR_LIB_PATH "libAwSurround.so"

#define SPACE_GAIN      (0.50)
#define BASS_GAIN       (0.33)
#define DEFINTION_GAIN  (0.80)

static int sur_load_lib(struct audio_3d_surround *sur)
{
    memset(sur, 0, sizeof(*sur));

    /* open lib */
    sur->lib = dlopen(SUR_LIB_PATH, RTLD_LAZY);
    if (NULL == sur->lib) {
        ALOGW("%s,line:%d, can't open surround lib.", __func__, __LINE__);
        return -1;
    }

    /* get 3d srround function */
    sur->process_init = dlsym(sur->lib, "process_init");
    sur->process_exit = dlsym(sur->lib, "process_exit");
    sur->surround_pro_in_out = dlsym(sur->lib, "surround_pro_in_out");
    sur->set_bass = dlsym(sur->lib, "set_bass");
    sur->set_defintion = dlsym(sur->lib, "set_defintion");
    sur->set_space = dlsym(sur->lib, "set_space");

    return 0;
}

static int sur_init_parameter(struct audio_3d_surround *sur, int samp_rate,
                              int chn, int num_frame, int headp_use)
{
    ALOGV("%s: rate:%d, ch:%d, num_frame:%d, headp_use:%d",
          __func__, samp_rate, chn, num_frame, headp_use);
    sur->headp_use = headp_use;
    sur->sur_handle = sur->process_init(sur->sur_handle, samp_rate, chn,
                                        num_frame, headp_use);
    sur->set_bass(sur->sur_handle, BASS_GAIN);
    sur->set_defintion(sur->sur_handle, DEFINTION_GAIN);
    sur->set_space(sur->sur_handle, SPACE_GAIN);

    return 0;
}

int sur_init(struct audio_3d_surround *sur,
             int samp_rate, int chn, int num_frame)
{
    sur_load_lib(sur);
    sur_init_parameter(sur, samp_rate, chn, num_frame, 1);

    return 0;
}

bool sur_enable(struct audio_3d_surround *sur)
{
    int use;
    char value[PROPERTY_VALUE_MAX];

    if ( (NULL == sur->lib) || (NULL == sur->sur_handle) )
        return false;

    /* get the current switch state. Default value is close. */
    property_get("persist.sys.audio_3d_surround", value, "0");
    use = atoi(value);

    return use;
}

static int sur_headp_use(int out_device, int dul_spk_use)
{
    int headp_dev;
    int spk_dev;

    headp_dev = AUDIO_DEVICE_OUT_WIRED_HEADSET |
                AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
                AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES;

    spk_dev = (dul_spk_use ? AUDIO_DEVICE_OUT_SPEAKER : 0) |
              AUDIO_DEVICE_OUT_BLUETOOTH_A2DP |
              AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER;

    if (out_device & headp_dev) {
        return 1;
    } else if (out_device & spk_dev) {
        return 0;
    }

    return -1;
}

bool sur_prepare(struct audio_3d_surround *sur, int out_device, int dul_spk_use,
                 int samp_rate, int chn, int num_frame)
{
    int headp_use;

    headp_use = sur_headp_use(out_device, dul_spk_use);
    if (headp_use < 0)
        return false;

    /* reinitialization */
    if (headp_use != sur->headp_use) {
        sur->process_exit(sur->sur_handle);
        sur_init_parameter(sur, samp_rate, chn, num_frame, headp_use);
    }

    return true;
}

int sur_process(struct audio_3d_surround *sur, short *buf, int frames, int channels)
{

#ifdef DEBUG_3D_SURROUND
    int bass, defintion, space;
    char value[PROPERTY_VALUE_MAX];

    property_get("persist.sys.gain1", value, "33");
    bass = atoi(value);

    property_get("persist.sys.gain2", value, "80");
    defintion = atoi(value);

    property_get("persist.sys.gain3", value, "50");
    space = atoi(value);

    /*set parameter for debug */
    sur->set_bass(sur->sur_handle, bass/100.0);
    sur->set_defintion(sur->sur_handle, defintion/100.0);
    sur->set_space(sur->sur_handle, space/100.0);
#endif

    sur->surround_pro_in_out(sur->sur_handle, buf, buf, frames * channels);

    return 0;
}

void sur_exit(struct audio_3d_surround *sur)
{
    if (sur->sur_handle != NULL) {
        sur->process_exit(sur->sur_handle);
        sur->sur_handle = NULL;
    }

    if (sur->lib != NULL) {
        dlclose(sur->lib);
        sur->lib = NULL;
    }
}

struct audio_3d_surround sur;
int dul_spk_use;
int now_out_device;
int sur_module_init()
{
    char val[PROPERTY_VALUE_MAX];

    sur_load_lib(&sur);

    property_get("ro.spk_dul.used", val, "0");
    if (!strcmp(val, "0")) {
        dul_spk_use = 0;
    } else {
        dul_spk_use = 1;
    }

    return 0;
}

int sur_module_start(struct pcm_config config)
{
    char val[PROPERTY_VALUE_MAX];

    /* reinitialization */
    if (sur.sur_handle)
        sur.process_exit(sur.sur_handle);

    sur_init_parameter(&sur, config.rate, config.channels,
                       config.period_count, 0);


    return 0;
}

int sur_module_exit()
{
    sur_exit(&sur);

    return 0;
}

int sur_module_prepare(int mode, int out_devices, int in_devices)
{
    UNUSED(mode);
    UNUSED(in_devices);

    now_out_device = out_devices;

    return 0;
}

int sur_module_process(struct pcm_config config, const void *buffer,
                       size_t bytes)
{
    int ret;
    int frames = bytes/(config.channels*sizeof(short));

    if (!sur_enable(&sur))
        return 0;

    ret = sur_prepare(&sur, now_out_device, dul_spk_use, config.rate,
                      config.channels, frames);
    if (ret) {
        sur_process(&sur, (short*)buffer, frames, config.channels);
    }

    return 0;
}

struct audio_plugin audio_3d = {
    .name = "audio_3d_surround",
    .handle = &sur,
    .on_adev_open = sur_module_init,
    .on_start_output_stream = sur_module_start,
    .on_adev_close = sur_module_exit,
    .on_select_devices = sur_module_prepare,
    .on_out_write = sur_module_process,
};

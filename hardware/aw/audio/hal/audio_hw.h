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

#ifndef _AUDIO_HW_H_
#define _AUDIO_HW_H_

#include <system/audio.h>
#include <hardware/audio.h>
#include <pthread.h>
#include <audio_utils/echo_reference.h>
#include <hardware/audio_effect.h>
#include "tinyalsa/asoundlib.h"
#include "audio_data_dump.h"
#include "eq.h"

/* sample rate */
#define RATE_8K     8000
#define RATE_11K    11025
#define RATE_12K    12000
#define RATE_16K    16000
#define RATE_22K    22050
#define RATE_24K    24000
#define RATE_32K    32000
#define RATE_44K    44100
#define RATE_48K    48000
#define RATE_96K    96000
#define RATE_192K   192000

#define PERIOD_SIZE 960
#define DEFAULT_CHANNEL_COUNT 2
#define DEFAULT_OUTPUT_SAMPLING_RATE RATE_48K
#define DEFAULT_OUTPUT_PERIOD_SIZE 480
#define DEFAULT_OUTPUT_PERIOD_COUNT 2

#define DEFAULT_INPUT_SAMPLING_RATE RATE_48K
#define DEFAULT_INPUT_PERIOD_SIZE PERIOD_SIZE
#define DEFAULT_INPUT_PERIOD_COUNT 2

#define DEFAULT_INPUT_VOICE_SAMPLING_RATE RATE_16K
#define DEFAULT_INPUT_RECOGNITION_SAMPLING_RATE RATE_48K
#define DEFAULT_OUTPUT_VOICE_SAMPLING_RATE DEFAULT_INPUT_VOICE_SAMPLING_RATE

#define MAX_SUPPORTED_CHANNEL_MASKS (2 * FCC_2) /* support positional and index masks to 8ch */
#define MAX_SUPPORTED_FORMATS 3
#define MAX_SUPPORTED_SAMPLE_RATES 9

#define PROPERTY_AUDIO_EQ_ENABLE "ro.vendor.audio.eq"
#define EQ_PATH "/vendor/etc/awequal.conf"
#define MAX_LINE_LENGTH 2048

//#define USE_RESAMPLER 1
#define AUDIO_DEVICE_API_VERSION_6_0 HARDWARE_DEVICE_API_VERSION(6, 0)
#define AUDIO_DEVICE_API_VERSION_7_0 HARDWARE_DEVICE_API_VERSION(7, 0)

#define MAX_PREPROCESSORS 3

struct sunxi_audio_device {
    struct audio_hw_device device;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */

    struct sunxi_stream_in *active_input;
    struct sunxi_stream_out *active_output;
    int mode;
    int out_devices;
    int in_devices;
    bool mic_muted;
    struct listnode out_streams;
    struct listnode in_streams;
    audio_patch_handle_t next_patch_handle;
    struct echo_reference_itfe *echo_reference;

    struct platform *platform;
};

struct sunxi_stream_out {
    struct audio_stream_out stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */

    int standby;
    uint32_t sample_rate;
    audio_channel_mask_t channel_mask;
    audio_channel_mask_t channel_mask_vts; /* add for vts */
    audio_format_t format;
    /* Array of supported channel mask configurations. +1 so that the last entry is always 0 */
    audio_channel_mask_t supported_channel_masks[MAX_SUPPORTED_CHANNEL_MASKS + 1];
    audio_format_t supported_formats[MAX_SUPPORTED_FORMATS + 1];
    uint32_t supported_sample_rates[MAX_SUPPORTED_SAMPLE_RATES + 1];

    audio_devices_t devices;
    audio_devices_t deviceArray[AUDIO_PATCH_PORTS_MAX];
    audio_output_flags_t flags;
    bool muted;

    /* total frames written, not cleared when entering standby */
    uint64_t written;
    void* equalizer;
    eq_prms_t prms;

    int card;
    int port;
    int card_hdmi;
    int port_hdmi;

    struct pcm_config config;
    struct pcm *pcm;
    struct pcm *pcm_hdmi;
    uint32_t num_devices;
    struct audio_data_dump dd_write_out;
    audio_io_handle_t handle;
    audio_patch_handle_t patch_handle;
    struct listnode stream_node;
    struct echo_reference_itfe *echo_reference;

    struct sunxi_audio_device *dev;
};

struct sunxi_stream_in {
    struct audio_stream_in stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */

    int standby;
    uint32_t sample_rate;
    audio_channel_mask_t channel_mask;
    audio_format_t format;

    /* Array of supported channel mask configurations.
       +1 so that the last entry is always 0 */
    audio_channel_mask_t supported_channel_masks[MAX_SUPPORTED_CHANNEL_MASKS + 1];
    audio_format_t supported_formats[MAX_SUPPORTED_FORMATS + 1];
    uint32_t supported_sample_rates[MAX_SUPPORTED_SAMPLE_RATES + 1];

    audio_devices_t devices;
    audio_output_flags_t flags;
    bool muted;
    int64_t frames_read; /* total frames read, not cleared when entering standby */

    int card;
    int port;
    struct pcm_config config;
    struct pcm *pcm;
    struct audio_data_dump dd_read_in;
    audio_io_handle_t handle;
    audio_patch_handle_t patch_handle;
    struct listnode stream_node;

    struct echo_reference_itfe *echo_reference;
    bool need_echo_reference;
    effect_handle_t preprocessors[MAX_PREPROCESSORS];
    int num_preprocessors;
    int16_t *proc_buf;
    size_t proc_buf_size;
    size_t proc_frames_in;
    int16_t *ref_buf;
    size_t ref_buf_size;
    size_t ref_frames_in;

    struct sunxi_audio_device *dev;
};

#endif

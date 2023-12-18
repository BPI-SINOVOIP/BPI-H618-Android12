/*
 * Copyright (C) 2011 The Android Open Source Project
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
#ifndef __AUDIO_HW__
#define __AUDIO_HW__

#define F_LOG ALOGV("%s, line: %d", __FUNCTION__, __LINE__);

#define DEFAULT_SAMPLING_RATE RATE_48K
#define OUTPUT_ACTIVE_CARDS AUDIO_CARD_CODEC | AUDIO_CARD_SPDIF
#ifndef TD100
#define INPUT_ACTIVE_CARDS -1 /*AUDIO_CARD_AC107*/ //// only one capture device can be active
#else
#define INPUT_ACTIVE_CARDS AUDIO_CARD_TD100 //// only one capture device can be active
#endif

#define EXTERNAL_OUTPUT_ACTIVE_CARD AUDIO_CARD_SPDIF
#define DSP_OUTPUT_ACTIVE_CARD AUDIO_CARD_I2S2
#define DSP_INPUT_ACTIVE_CARD AUDIO_CARD_I2S2

#define ENABLE_SUB_PCM_DEVICE 0
#define USE_INPUT_RESAMPLER 0

#if ENABLE_SUB_PCM_DEVICE
#define SUB_PCM_DEVICE AUDIO_NAME_I2S2
#endif

#define AUDIO_CARD_AUTO_DEC 0x01
#define AUDIO_CARD_CODEC 0x02
#define AUDIO_CARD_HDMI 0x04
#define AUDIO_CARD_SPDIF 0x08
#define AUDIO_CARD_I2S2 0x10
#define AUDIO_CARD_I2S3 0x20
#define AUDIO_CARD_AC107 0x40
#define AUDIO_CARD_AHUB 0x80
#define AUDIO_CARD_TD100 0x100
#define AUDIO_CARD_NONE 0x1000

#define AUDIO_NONE 0x00
#define AUDIO_IN 0x01
#define AUDIO_OUT 0x02

#define PORT_AC107 0
#define PORT_TD100 0
#define PORT_HDMI 0
#define PORT_I2S2 2
#define PORT_I2S3 1

#define AUDIO_NAME_CODEC "AUDIO_CODEC"
#define AUDIO_NAME_AHUB "AUDIO_AHUB"
#define AUDIO_NAME_HDMI "AUDIO_HDMI"
#define AUDIO_NAME_SPDIF "AUDIO_SPDIF"
#define AUDIO_NAME_AC107 "AUDIO_AC107"
#define AUDIO_NAME_TD100 "AUDIO_TD100"
#define AUDIO_NAME_I2S2 "AUDIO_I2S2"
#define AUDIO_NAME_I2S3 "AUDIO_I2S3"

#define RATE_8K 8000
#define RATE_11K 11025
#define RATE_12K 12000
#define RATE_16K 16000
#define RATE_22K 22050
#define RATE_24K 24000
#define RATE_32K 32000
#define RATE_44K 44100
#define RATE_48K 48000
#define RATE_96K 96000
#define RATE_192K 192000

#define DEFAULT_CHANNEL_COUNT 2
#define DEFAULT_OUTPUT_PERIOD_SIZE (1360 * 2)
#define DEFAULT_OUTPUT_PERIOD_COUNT 2

#define DEFAULT_INPUT_PERIOD_SIZE 1024
#define DEFAULT_INPUT_PERIOD_COUNT 2

#define RESAMPLER_BUFFER_FRAMES (DEFAULT_OUTPUT_PERIOD_SIZE * 2)
#define RESAMPLER_BUFFER_SIZE (4 * RESAMPLER_BUFFER_FRAMES)

#define MAX_AUDIO_DEVICES 16

#define AUDIO_HAL_PARAM_OUTPUT_DEVICES "output devices"
#define AUDIO_HAL_PARAM_OUTPUT_DEVICES_IS_ACTIVE "is active"
#define AUDIO_HAL_PARAM_OUTPUT_DEVICES_AUDIO_CODEC "codec"
#define AUDIO_HAL_PARAM_OUTPUT_DEVICES_HDMI "hdmi"
#define AUDIO_HAL_PARAM_OUTPUT_DEVICES_SPDIF "spdif"

#define AUDIO_HAL_PARAM_ROUTE "audio-route"
#define AUDIO_HAL_PARAM_ROUTE_VALUE "value"
#define AUDIO_HAL_PARAM_ROUTE_LINEOUT "lineout"
#define AUDIO_HAL_PARAM_ROUTE_LINEIN_LINEOUT "linein-lineout"
#define AUDIO_HAL_PARAM_ROUTE_FMIN_LINEOUT "fmin-lineout"
#define AUDIO_HAL_PARAM_ROUTE_HDMI_OUT "hdmi out"
#define AUDIO_HAL_PARAM_ROUTE_AC107_IN "ac107 in"
#define AUDIO_HAL_PARAM_ROUTE_I2S2_OUT "i2s2 out"
#define AUDIO_HAL_PARAM_ROUTE_I2S2_IN "i2s2 in"
#define AUDIO_HAL_PARAM_ROUTE_I2S3_OUT "i2s3 out"
#define AUDIO_HAL_PARAM_ROUTE_I2S3_IN "i2s3 out"
#define AUDIO_HAL_PARAM_ROUTE_SPDIF "spdif"

#define AUDIO_HAL_PARAM_VOL_LINEOUT "lineout vol"
#define AUDIO_HAL_PARAM_VOL_LINEIN "linein vol"
#define AUDIO_HAL_PARAM_VOL_FMIN "fmin vol"
#define AUDIO_HAL_PARAM_VOL_AC107_C1_PGA "ac107 c1 pga vol"
#define AUDIO_HAL_PARAM_VOL_AC107_C2_PGA "ac107 c2 pga vol"
#define AUDIO_HAL_PARAM_VOL_AC107_C1_DIGITAL "ac107 c1 digital vol"
#define AUDIO_HAL_PARAM_VOL_AC107_C2_DIGITAL "ac107 c2 digital vol"

#define AUDIO_HAL_PARAM_VOL_TD100_MIC1 "td100 mic1 vol"
#define AUDIO_HAL_PARAM_VOL_TD100_MIC2 "td100 mic2 vol"
#define AUDIO_HAL_PARAM_VOL_TD100_MIC3 "td100 mic3 vol"
#define AUDIO_HAL_PARAM_VOL_TD100_LINEIN "td100 linein vol"
#define AUDIO_HAL_PARAM_DVOL_TD100_ADC1 "td100 adc1 dvol"
#define AUDIO_HAL_PARAM_DVOL_TD100_ADC2 "td100 adc2 dvol"
#define AUDIO_HAL_PARAM_CH1_TD100_SELECT "td100 ch1 select"
#define AUDIO_HAL_PARAM_CH2_TD100_SELECT "td100 ch2 select"

/* Power state */
#define AUDIO_HAL_PARAM_KEY_POWER_STATE "power_state"

#define PROP_RAWDATA_KEY "vendor.mediasw.sft.rawdata"
#define PROP_RAWDATA_MODE "rawdata-mode"
#define PROP_RAWDATA_MODE_PCM "PCM"
#define PROP_RAWDATA_MODE_HDMI_RAW "HDMI_RAW"
#define PROP_RAWDATA_MODE_SPDIF_RAW "SPDIF_RAW"
#define PROP_RAWDATA_DEFAULT_VALUE PROP_RAWDATA_MODE_PCM
#define PROP_RAWDATA_PORT "rawdata-port"
#define PROP_RAWDATA_PORT_HDMI "hdmi"
#define PROP_RAWDATA_PORT_SPDIF "spdif"

#define AUX_DIGITAL_MULTI_PERIOD_SIZE 2048
#define AUX_DIGITAL_MULTI_PERIOD_COUNT 4
#define AUX_DIGITAL_MULTI_DEFAULT_CHANNEL_COUNT 2
#define AUX_DIGITAL_MULTI_PERIOD_BYTES (AUX_DIGITAL_MULTI_PERIOD_SIZE * AUX_DIGITAL_MULTI_DEFAULT_CHANNEL_COUNT * 2)

#define PROP_RAWDATA_OUTPUT "vendor.rawdata.output"
#define PROP_RAWDATA_OUTPUT_ON "1"
#define PROP_RAWDATA_OUTPUT_OFF "0"

typedef struct sunxi_audio_device_manager
{
    char name[32];
    char card_id[32];
    int card;
    int port;
    int flag_in;        //
    int flag_in_active; // 0: not used, 1: used to caputre
    int flag_out;
    int flag_out_active; // 0: not used, 1: used to playback
    bool flag_exist;     // for hot-plugging
    bool ahub_device;
} sunxi_audio_device_manager;

struct sunxi_stream_in
{
    struct audio_stream_in stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm *pcm;
    struct pcm *sub_pcm;
    struct pcm *ext_pcm;
    struct pcm *ext_sub_pcm;
    struct pcm_config ext_config;
#if USE_INPUT_RESAMPLER
    struct resampler_itfe *resampler;
    struct resampler_buffer_provider buf_provider;
#endif
    int16_t *buffer;
    size_t frames_in;
    unsigned int requested_rate;
    int standby;
    int source;
    int read_status;
    struct sunxi_audio_device *dev;
    FILE *dump_pcm;
};

struct sunxi_stream_out
{
    struct audio_stream_out stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm_config multi_config[MAX_AUDIO_DEVICES];
    struct pcm *multi_pcm[MAX_AUDIO_DEVICES];
    struct pcm *sub_pcm[MAX_AUDIO_DEVICES];
    struct resampler_itfe *multi_resampler[MAX_AUDIO_DEVICES];
    struct pcm_config ext_config;
    struct pcm *ext_pcm;
    struct pcm *ext_sub_pcm;
    struct resampler_itfe *ext_resampler;
    char *buffer;
    int standby;
    audio_format_t format;
    audio_output_flags_t flags;
    struct sunxi_audio_device *dev;
    FILE *dump_pcm;
};

struct sunxi_audio_device
{
    struct audio_hw_device hw_device;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct mixer *mixer_codec;
    struct mixer *mixer_ahub;
    struct mixer *mixer_ac107;
    struct mixer *mixer_td100;
    int mode;
    int in_call;
    struct sunxi_stream_in *active_input;
    struct sunxi_stream_out *active_output;
    bool mic_mute;
    struct sunxi_audio_device_manager dev_manager[MAX_AUDIO_DEVICES];
    struct audio_route *ar_codec;
    struct audio_route *ar_ahub;
    bool stanby;
    int card_codec;
    int card_ahub;
    int card_ac107;
    int card_td100;
    int card_hdmi;
    int card_spdif;
    int output_active_cards;
    int input_active_cards;
    bool raw_flag; // flag for raw data
    bool raw_enable;
    char in_devices[128], out_devices[128];

    //td100 ctrl val
    int vol_td100_mic1_val;
    int vol_td100_mic2_val;
    int vol_td100_mic3_val;
    int vol_td100_linein_val;
    int dvol_td100_adc1_val;
    int dvol_td100_adc2_val;
    int ch1_td100_select_val;
    int ch2_td100_select_val;
};

#endif

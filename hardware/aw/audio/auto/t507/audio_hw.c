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

#define LOG_TAG "audio_hw_primary"
#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>

#include <log/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <audio_route/audio_route.h>

#include <tinyalsa/asoundlib.h>
#include <audio_utils/resampler.h>
#include <hardware/audio_effect.h>
#include <audio_effects/effect_aec.h>
#include <fcntl.h>
#include <unistd.h>

#include <cutils/properties.h>

#include "audio_hw.h"

#define UNUSED(x) (void)(x)

#define AUDIO_CODEC_XML_PATH "/vendor/etc/auto_codec_paths.xml"
#define AUDIO_AHUB_XML_PATH "/vendor/etc/auto_ahub_paths.xml"

typedef struct route_map_t
{
    int id;
    char open[128];
    char close[128];
} route_map;

enum
{
    OUT_DEVICE_LINEOUT,
    OUT_DEVICE_HDMI,
    OUT_DEVICE_I2S2,
    OUT_DEVICE_I2S3,

    IN_DEVICE_AC107,
    IN_DEVICE_TD100,
    IN_DEVICE_I2S2,
    IN_DEVICE_I2S3,

    AA_LINEIN_LINEOUT,
    AA_FMIN_LINEOUT,

    VOL_LINEOUT,
    VOL_LINEIN_OMIX,
    VOL_FMIN_OMIX,
    VOL_AC107_C1_PGA,
    VOL_AC107_C2_PGA,
    VOL_AC107_C1_DIGITAL,
    VOL_AC107_C2_DIGITAL,

    VOL_TD100_MIC1,
    VOL_TD100_MIC2,
    VOL_TD100_MIC3,
    VOL_TD100_LINEIN,
    DVOL_TD100_ADC1,
    DVOL_TD100_ADC2,
    CH1_TD100_SELECT,
    CH2_TD100_SELECT,

    DEVICE_TAB_SIZE,
};

static route_map route_configs[DEVICE_TAB_SIZE] = {
    {OUT_DEVICE_LINEOUT, "codec-lineout", "codec-lineout-close"},
    {OUT_DEVICE_HDMI, "ahub-daudio1-output", "ahub-daudio1-output-close"},
    {OUT_DEVICE_I2S2, "ahub-daudio2-output", "ahub-daudio2-output-close"},
    {OUT_DEVICE_I2S3, "ahub-daudio3-output", "ahub-daudio3-output-close"},

    {IN_DEVICE_AC107, "ahub-daudio0-input", "ahub-daudio0-input-close"},
    {IN_DEVICE_TD100, "ahub-daudio0-input", "ahub-daudio0-input-close"},
    {IN_DEVICE_I2S2, "ahub-daudio2-input", "ahub-daudio2-input-close"},
    {IN_DEVICE_I2S3, "ahub-daudio3-input", "ahub-daudio3-input-close"},

    {AA_LINEIN_LINEOUT, "codec-linein-lineout", "codec-linein-lineout-close"},
    {AA_FMIN_LINEOUT, "codec-fmin-lineout", "codec-fmin-lineout-close"},

    {VOL_LINEOUT, "LINEOUT volume", ""},
    {VOL_LINEIN_OMIX, "LINEIN to output mixer gain control", ""},
    {VOL_FMIN_OMIX, "FMIN to output mixer gain control", ""},
    {VOL_AC107_C1_PGA, "Channel 1 PGA Gain", ""},
    {VOL_AC107_C2_PGA, "Channel 2 PGA Gain", ""},
    {VOL_AC107_C1_DIGITAL, "Channel 1 Digital Volume", ""},
    {VOL_AC107_C2_DIGITAL, "Channel 2 Digital Volume", ""},

    {VOL_TD100_MIC1, "MIC1 Gain", ""},
    {VOL_TD100_MIC2, "MIC2 Gain", ""},
    {VOL_TD100_MIC3, "MIC3 Gain", ""},
    {VOL_TD100_LINEIN, "LINEIN Gain", ""},
    {DVOL_TD100_ADC1, "ADC1 Digital Volume", ""},
    {DVOL_TD100_ADC2, "ADC2 Digital Volume", ""},
    {CH1_TD100_SELECT, "I2S TX CH1 MUX", ""},
    {CH2_TD100_SELECT, "I2S TX CH2 MUX", ""},
};

struct pcm_config pcm_config_out = {
    .channels = DEFAULT_CHANNEL_COUNT,
    .rate = DEFAULT_SAMPLING_RATE,
    .period_size = DEFAULT_OUTPUT_PERIOD_SIZE,
    .period_count = DEFAULT_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_in = {
    .channels = DEFAULT_CHANNEL_COUNT,
    .rate = DEFAULT_SAMPLING_RATE,
    .period_size = DEFAULT_INPUT_PERIOD_SIZE,
    .period_count = DEFAULT_INPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

typedef struct name_map_t
{
    char name_linux[32];
    char name_android[32];
} name_map;

static name_map audio_name_map[MAX_AUDIO_DEVICES] = {
    {"audiocodec", AUDIO_NAME_CODEC},     //inside codec
    {"ahubi2s0", AUDIO_NAME_AC107},       //ac107
    {"ahubi2s2", AUDIO_NAME_I2S2},        //daudio2
    {"ahubi2s3", AUDIO_NAME_I2S3},        //daudio3
    {"ahubdam", AUDIO_NAME_AHUB},         //ahub
    {"sndtd100codec0", AUDIO_NAME_TD100}, //td100
    {"ahubhdmi", AUDIO_NAME_HDMI},
    {"sndspdif", AUDIO_NAME_SPDIF},
};

static void set_audio_path(struct sunxi_audio_device *adev, int device_path, int status);
static int do_input_standby(struct sunxi_stream_in *in);
static int do_output_standby(struct sunxi_stream_out *out);
static int set_audio_devices_active(struct sunxi_audio_device *adev, int in_out);

static int set_mixer_value(struct mixer *mixer, const char *name, int value)
{
    struct mixer_ctl *ctl = NULL;
    ctl = mixer_get_ctl_by_name(mixer, name);
    if (ctl == NULL)
    {
        ALOGE("Control '%s' doesn't exist ", name);
        return -1;
    }
    return mixer_ctl_set_value(ctl, 0, value);
}

static int find_name_map(struct sunxi_audio_device *adev, char *in, char *out)
{
    UNUSED(adev);

    int index = 0;

    if (in == 0 || out == 0)
    {
        ALOGE("error params");
        return -1;
    }

    for (; index < MAX_AUDIO_DEVICES; index++)
    {
        if (strlen(audio_name_map[index].name_linux) == 0)
        {

            //sprintf(out, "AUDIO_USB%d", adev->usb_audio_cnt++);
            sprintf(out, "AUDIO_USB_%s", in);
            strcpy(audio_name_map[index].name_linux, in);
            strcpy(audio_name_map[index].name_android, out);
            ALOGD("linux name = %s, android name = %s",
                  audio_name_map[index].name_linux,
                  audio_name_map[index].name_android);
            return 0;
        }

        if (!strcmp(in, audio_name_map[index].name_linux))
        {
            strcpy(out, audio_name_map[index].name_android);
            ALOGD("linux name = %s, android name = %s",
                  audio_name_map[index].name_linux,
                  audio_name_map[index].name_android);
            return 0;
        }
    }

    return 0;
}

static int do_init_audio_card(struct sunxi_audio_device *adev, int card)
{
    int ret = -1;
    int fd = 0;
    char *snd_path = "/sys/class/sound";
    char snd_card[128], snd_node[128];
    char snd_id[32], snd_name[32];

    memset(snd_card, 0, sizeof(snd_card));
    memset(snd_node, 0, sizeof(snd_node));
    memset(snd_id, 0, sizeof(snd_id));
    memset(snd_name, 0, sizeof(snd_name));

    sprintf(snd_card, "%s/card%d", snd_path, card);
    ret = access(snd_card, F_OK);
    if (ret == 0)
    {
        // id / name
        sprintf(snd_node, "%s/card%d/id", snd_path, card);
        ALOGD("read card %s/card%d/id", snd_path, card);

        fd = open(snd_node, O_RDONLY);
        if (fd > 0)
        {
            ret = read(fd, snd_id, sizeof(snd_id));
            if (ret > 0)
            {
                snd_id[ret - 1] = 0;
                ALOGD("%s, %s, len: %d", snd_node, snd_id, ret);
            }
            close(fd);
        }
        else
        {
            return -1;
        }

        strcpy(adev->dev_manager[card].card_id, snd_id);
        find_name_map(adev, snd_id, snd_name);
        strcpy(adev->dev_manager[card].name, snd_name);
        ALOGD("find name map, card_id = %s, card_name = %s ", adev->dev_manager[card].card_id, adev->dev_manager[card].name);

        adev->dev_manager[card].card = card;
        adev->dev_manager[card].port = 0;
        adev->dev_manager[card].flag_exist = true;
        adev->dev_manager[card].flag_out = AUDIO_NONE;
        adev->dev_manager[card].flag_in = AUDIO_NONE;
        adev->dev_manager[card].flag_out_active = 0;
        adev->dev_manager[card].flag_in_active = 0;

        if (!strcmp(adev->dev_manager[card].name, AUDIO_NAME_CODEC))
        {
            adev->dev_manager[card].port = 0;
            adev->dev_manager[card].ahub_device = 0;
            adev->card_codec = card;
            adev->ar_codec = audio_route_init(adev->card_codec, AUDIO_CODEC_XML_PATH);
        }
        else if (!strcmp(adev->dev_manager[card].name, AUDIO_NAME_AHUB))
        {
            adev->card_ahub = card;
            adev->ar_ahub = audio_route_init(adev->card_ahub, AUDIO_AHUB_XML_PATH);
            return 0;
        }
        else if (!strcmp(adev->dev_manager[card].name, AUDIO_NAME_AC107))
        {
            adev->dev_manager[card].port = PORT_AC107;
            adev->dev_manager[card].ahub_device = 1;
            adev->card_ac107 = card;
            adev->mixer_ac107 = mixer_open(adev->card_ac107);
        }
        else if (!strcmp(adev->dev_manager[card].name, AUDIO_NAME_TD100))
        {
            adev->dev_manager[card].port = PORT_TD100;
            adev->dev_manager[card].ahub_device = 1;
            adev->card_td100 = card;
            adev->mixer_td100 = mixer_open(adev->card_td100);
        }
        else if (!strcmp(adev->dev_manager[card].name, AUDIO_NAME_HDMI))
        {
            adev->dev_manager[card].port = PORT_HDMI;
            adev->dev_manager[card].ahub_device = 1;
        }
        else if (!strcmp(adev->dev_manager[card].name, AUDIO_NAME_I2S2))
        {
            adev->dev_manager[card].port = PORT_I2S2;
            adev->dev_manager[card].ahub_device = 1;
        }
        else if (!strcmp(adev->dev_manager[card].name, AUDIO_NAME_I2S3))
        {
            adev->dev_manager[card].port = PORT_I2S3;
            adev->dev_manager[card].ahub_device = 1;
        }
        else
        {
            adev->dev_manager[card].ahub_device = 0;
            adev->dev_manager[card].port = 0;
        }

        if (strcmp(adev->dev_manager[card].name, AUDIO_NAME_AC107) || strcmp(adev->dev_manager[card].name, AUDIO_NAME_TD100))
        {
            // playback device
            sprintf(snd_node, "%s/card%d/pcmC%dD0p", snd_path, card, card);
            ret = access(snd_node, F_OK);
            if (ret == 0)
            {
                // there is a playback device
                adev->dev_manager[card].flag_out = AUDIO_OUT;
                adev->dev_manager[card].flag_out_active = 0;
            }
        }
        if (strcmp(adev->dev_manager[card].name, AUDIO_NAME_HDMI) || strcmp(adev->dev_manager[card].name, AUDIO_NAME_CODEC))
        {
            // capture device
            sprintf(snd_node, "%s/card%d/pcmC%dD0c", snd_path, card, card);
            ret = access(snd_node, F_OK);
            if (ret == 0)
            {
                // there is a capture device
                adev->dev_manager[card].flag_in = AUDIO_IN;
                adev->dev_manager[card].flag_in_active = 0;
            }
        }
    }
    else
    {
        return -1;
    }

    return 0;
}
static void init_td100_devices(struct sunxi_audio_device *adev)
{
    if (!adev->mixer_td100)
        return;
    set_mixer_value(adev->mixer_td100, route_configs[VOL_TD100_MIC1].open, adev->vol_td100_mic1_val);
    set_mixer_value(adev->mixer_td100, route_configs[VOL_TD100_MIC2].open, adev->vol_td100_mic2_val);
    set_mixer_value(adev->mixer_td100, route_configs[VOL_TD100_MIC3].open, adev->vol_td100_mic3_val);
    set_mixer_value(adev->mixer_td100, route_configs[VOL_TD100_LINEIN].open, adev->vol_td100_linein_val);
    set_mixer_value(adev->mixer_td100, route_configs[DVOL_TD100_ADC1].open, adev->dvol_td100_adc1_val);
    set_mixer_value(adev->mixer_td100, route_configs[DVOL_TD100_ADC2].open, adev->dvol_td100_adc2_val);
    set_mixer_value(adev->mixer_td100, route_configs[CH1_TD100_SELECT].open, adev->ch1_td100_select_val);
    set_mixer_value(adev->mixer_td100, route_configs[CH2_TD100_SELECT].open, adev->ch2_td100_select_val);
}
static void init_ac107_devices(struct sunxi_audio_device *adev)
{
    if (!adev->mixer_ac107)
        return;
    set_mixer_value(adev->mixer_ac107, route_configs[VOL_AC107_C2_PGA].open, 12);
    set_mixer_value(adev->mixer_ac107, route_configs[VOL_AC107_C1_PGA].open, 29);
}

static void init_audio_devices(struct sunxi_audio_device *adev)
{
    int card = 0;
    F_LOG;

    memset(adev->dev_manager, 0, sizeof(adev->dev_manager));

    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (do_init_audio_card(adev, card) == 0)
        {
            // break;
            ALOGV("card: %d, name: %s, capture: %d, playback: %d",
                  card, adev->dev_manager[card].name,
                  adev->dev_manager[card].flag_in == AUDIO_IN,
                  adev->dev_manager[card].flag_out == AUDIO_OUT);
        }
    }

    if (adev->ar_codec)
    {
        audio_route_reset(adev->ar_codec);
    }
    if (adev->ar_ahub)
    {
        audio_route_reset(adev->ar_ahub);
    }
}

static void init_audio_devices_active(struct sunxi_audio_device *adev)
{
    int card = 0;
    int flag_active = 0;

    F_LOG;

    if (set_audio_devices_active(adev, AUDIO_IN) == 0)
    {
        flag_active |= AUDIO_IN;
    }

    if (set_audio_devices_active(adev, AUDIO_OUT) == 0)
    {
        flag_active |= AUDIO_OUT;
    }

    if ((flag_active & AUDIO_IN) && (flag_active & AUDIO_OUT))
    {
        return;
    }

    ALOGV("midle priority, use codec & ac107");
    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        // default use auido codec out, ac107 in.
        if ((!strcmp(adev->dev_manager[card].name, AUDIO_NAME_AC107)) &&
            (adev->dev_manager[card].flag_in == AUDIO_IN))
        {
            ALOGV("OK, default use %s capture", adev->dev_manager[card].name);
            adev->dev_manager[card].flag_in_active = 1;
            flag_active |= AUDIO_IN;
        }
        if ((!strcmp(adev->dev_manager[card].name, AUDIO_NAME_TD100)) &&
            (adev->dev_manager[card].flag_in == AUDIO_IN))
        {
            ALOGV("OK, default use %s capture", adev->dev_manager[card].name);
            adev->dev_manager[card].flag_in_active = 1;
            flag_active |= AUDIO_IN;
        }

        if ((!strcmp(adev->dev_manager[card].name, AUDIO_NAME_CODEC)) &&
            (adev->dev_manager[card].flag_out == AUDIO_OUT))
        {
            ALOGV("OK, default use %s playback", adev->dev_manager[card].name);
            adev->dev_manager[card].flag_out_active = 1;
            flag_active |= AUDIO_OUT;
        }
        if ((flag_active & AUDIO_IN) && (flag_active & AUDIO_OUT))
        {
            return;
        }
    }

    ALOGV("low priority, chose any device");
    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (!adev->dev_manager[card].flag_exist)
        {
            break;
        }

        // there is no auido codec in
        if (!(flag_active & AUDIO_IN))
        {
            if (adev->dev_manager[card].flag_in == AUDIO_IN)
            {
                ALOGV("OK, default use %s capture", adev->dev_manager[card].name);
                adev->dev_manager[card].flag_in_active = 1;
                flag_active |= AUDIO_IN;
            }
        }

        // there is no auido codec out
        if (!(flag_active & AUDIO_OUT))
        {
            if (adev->dev_manager[card].flag_out == AUDIO_OUT)
            {
                ALOGV("OK, default use %s playback", adev->dev_manager[card].name);
                adev->dev_manager[card].flag_out_active = 1;
                flag_active |= AUDIO_OUT;
            }
        }
    }
    return;
}

static int update_audio_devices(struct sunxi_audio_device *adev)
{
    int card = 0;
    int ret = -1;
    char *snd_path = "/sys/class/sound";
    char snd_card[128];

    memset(snd_card, 0, sizeof(snd_card));

    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        sprintf(snd_card, "%s/card%d", snd_path, card);
        ret = access(snd_card, F_OK);
        if (ret == 0)
        {
            if (adev->dev_manager[card].flag_exist == true)
            {
                continue; // no changes
            }
            else
            {
                // plug-in
                ALOGD("do init audio card");
                do_init_audio_card(adev, card);
            }
        }
        else
        {
            if (adev->dev_manager[card].flag_exist == false)
            {
                continue; // no changes
            }
            else
            {
                // plug-out
                adev->dev_manager[card].flag_exist = false;
                adev->dev_manager[card].flag_in = 0;
                adev->dev_manager[card].flag_out = 0;
            }
        }
    }

    return 0;
}

static char *get_audio_devices(struct sunxi_audio_device *adev, int in_out)
{
    char *in_devices = adev->in_devices;
    char *out_devices = adev->out_devices;

    update_audio_devices(adev);

    memset(in_devices, 0, 128);
    memset(out_devices, 0, 128);

    ALOGD("getAudioDevices()");
    int card = 0;
    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (adev->dev_manager[card].flag_exist == true)
        {
            // device in
            if (adev->dev_manager[card].flag_in == AUDIO_IN)
            {
                strcat(in_devices, adev->dev_manager[card].name);
                strcat(in_devices, ",");
                ALOGD("in dev:%s", adev->dev_manager[card].name);
            }
            // device out
            if (adev->dev_manager[card].flag_out == AUDIO_OUT)
            {
                strcat(out_devices, adev->dev_manager[card].name);
                strcat(out_devices, ",");
                ALOGD("out dev:%s", adev->dev_manager[card].name);
            }
        }
    }

    in_devices[strlen(in_devices) - 1] = 0;
    out_devices[strlen(out_devices) - 1] = 0;

    //
    if (in_out & AUDIO_IN)
    {
        ALOGD("in capture: %s", in_devices);
        return in_devices;
    }
    else if (in_out & AUDIO_OUT)
    {
        ALOGD("out playback: %s", out_devices);
        return out_devices;
    }
    else
    {
        ALOGE("unknown in/out flag");
        return 0;
    }
}

#if USE_INPUT_RESAMPLER
static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer *buffer)
{
    struct sunxi_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
        return -EINVAL;

    in = (struct sunxi_stream_in *)((char *)buffer_provider -
                                    offsetof(struct sunxi_stream_in, buf_provider));

    if (in->pcm == NULL)
    {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        in->read_status = -ENODEV;
        return -ENODEV;
    }

    if (in->frames_in == 0)
    {
        in->read_status = pcm_read(in->pcm,
                                   (void *)in->buffer,
                                   in->config.period_size *
                                       audio_stream_in_frame_size(&in->stream));
        if (in->read_status != 0)
        {
            ALOGE("get_next_buffer() pcm_read error %d, %s", in->read_status, strerror(errno));
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return in->read_status;
        }
        in->frames_in = in->config.period_size;
    }

    buffer->frame_count = (buffer->frame_count > in->frames_in) ? in->frames_in : buffer->frame_count;
    buffer->i16 = in->buffer + (in->config.period_size - in->frames_in) *
                                   in->config.channels;

    return in->read_status;
}

static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer *buffer)
{
    struct sunxi_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
        return;

    in = (struct sunxi_stream_in *)((char *)buffer_provider -
                                    offsetof(struct sunxi_stream_in, buf_provider));

    in->frames_in -= buffer->frame_count;
}
#endif

static int get_hdmi_status()
{
    int fd = 0;
    char hdmi_status[32] = {0};
    int ret;
    int hdmi_state = 0;

    fd = open("/sys/class/switch/hdmi/state", O_RDONLY);
    if (fd > 0)
    {
        ret = read(fd, hdmi_status, sizeof(hdmi_status));
        if (ret > 0)
        {
            hdmi_status[ret - 1] = 0;
            ALOGD("hdmi_status : %s", hdmi_status);
        }
        sscanf(hdmi_status, "HDMI=%d", &hdmi_state);
        close(fd);
    }
    else
    {
        return -1;
    }
    return hdmi_state;
}

static void select_output_cards(struct sunxi_audio_device *adev)
{
    if (adev->output_active_cards & EXTERNAL_OUTPUT_ACTIVE_CARD)
    {
        adev->output_active_cards &= (~EXTERNAL_OUTPUT_ACTIVE_CARD);
        ALOGW("card %#x used by external output, ignore it in primary output.", EXTERNAL_OUTPUT_ACTIVE_CARD);
    }
    if (adev->output_active_cards & DSP_OUTPUT_ACTIVE_CARD)
    {
        adev->output_active_cards &= (~DSP_OUTPUT_ACTIVE_CARD);
        ALOGW("card %#x used by dsp output, ignore it in primary output.", DSP_OUTPUT_ACTIVE_CARD);
    }
    if (adev->input_active_cards & DSP_INPUT_ACTIVE_CARD)
    {
        adev->input_active_cards &= (~DSP_INPUT_ACTIVE_CARD);
        ALOGW("card %#x used by dsp input, ignore it in primary input.", DSP_INPUT_ACTIVE_CARD);
    }
}

static int set_audio_devices_active(struct sunxi_audio_device *adev, int in_out)
{
    int card = 0, i = 0;
    char name[8][32];
    int cnt = 0;

    select_output_cards(adev);

    if (in_out & AUDIO_OUT)
        ALOGV("output active = %#x", adev->output_active_cards);
    if (in_out & AUDIO_IN)
        ALOGV("input active = %#x", adev->input_active_cards);

    if (in_out & AUDIO_OUT)
    {
        if (adev->output_active_cards & AUDIO_CARD_CODEC)
        {
            strcpy(name[cnt++], AUDIO_NAME_CODEC);
            set_audio_path(adev, OUT_DEVICE_LINEOUT, 1);
        }
        if (adev->output_active_cards & AUDIO_CARD_HDMI)
        {
            set_audio_path(adev, OUT_DEVICE_HDMI, 1);
            strcpy(name[cnt++], AUDIO_NAME_HDMI);
        }
        if (adev->output_active_cards & AUDIO_CARD_I2S2)
        {
            strcpy(name[cnt++], AUDIO_NAME_I2S2);
            set_audio_path(adev, OUT_DEVICE_I2S2, 1);
        }
        if (adev->output_active_cards & AUDIO_CARD_I2S3)
        {
            strcpy(name[cnt++], AUDIO_NAME_I2S3);
            set_audio_path(adev, OUT_DEVICE_I2S3, 1);
        }
        if (adev->output_active_cards & AUDIO_CARD_SPDIF)
        {
            strcpy(name[cnt++], AUDIO_NAME_SPDIF);
        }
    }

    if (in_out & AUDIO_IN)
    {
        if (adev->input_active_cards & AUDIO_CARD_AC107)
        {
            strcpy(name[cnt++], AUDIO_NAME_AC107);
            set_audio_path(adev, IN_DEVICE_AC107, 1);
        }
        else if (adev->input_active_cards & AUDIO_CARD_TD100)
        {
            strcpy(name[cnt++], AUDIO_NAME_TD100);
            set_audio_path(adev, IN_DEVICE_TD100, 1);
        }
        else if (adev->input_active_cards & AUDIO_CARD_I2S2)
        {
            strcpy(name[cnt++], AUDIO_NAME_I2S2);
            set_audio_path(adev, IN_DEVICE_I2S2, 1);
        }
        else if (adev->input_active_cards & AUDIO_CARD_I2S3)
        {
            strcpy(name[cnt++], AUDIO_NAME_I2S3);
            set_audio_path(adev, IN_DEVICE_I2S3, 1);
        }
    }

    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (in_out & AUDIO_IN)
        {
            adev->dev_manager[card].flag_in_active = 0;
        }
        else
        {
            adev->dev_manager[card].flag_out_active = 0;
        }
    }

    for (i = 0; i < cnt; i++)
    {
        for (card = 0; card < MAX_AUDIO_DEVICES; card++)
        {
            if (in_out & AUDIO_IN)
            {
                if ((adev->dev_manager[card].flag_in == in_out) && (strcmp(adev->dev_manager[card].name, name[i]) == 0))
                {
                    ALOGV("%s %s device will be active", name[i], "input");
                    adev->dev_manager[card].flag_in_active = 1;
                    // only one capture device can be active
                    return 0;
                }
            }
            else if ((adev->dev_manager[card].flag_out == in_out) && (strcmp(adev->dev_manager[card].name, name[i]) == 0))
            {
                ALOGV("%s %s card %d device will be active", name[i], "output", card);
                adev->dev_manager[card].flag_out_active = 1;
                break;
            }
        }

        if (card == MAX_AUDIO_DEVICES && i > cnt)
        {
            if (in_out & AUDIO_IN)
            {
                ALOGE("can not set %s active device", (in_out & AUDIO_IN) ? "input" : "ouput");
                adev->dev_manager[adev->card_codec].flag_in_active = 1;
                ALOGE("but %s %s will be active", adev->dev_manager[adev->card_codec].name, (in_out & AUDIO_IN) ? "input" : "ouput");
                return 0;
            }
            else
            {
                ALOGE("can not set %s active device", (in_out & AUDIO_IN) ? "input" : "ouput");
                adev->dev_manager[adev->card_ac107].flag_out_active = 1;
                ALOGE("but %s %s will be active", adev->dev_manager[adev->card_ac107].name, (in_out & AUDIO_IN) ? "input" : "ouput");
                return -1;
            }
            return -1;
        }
    }

    return 0;
}

static int get_audio_devices_active(struct sunxi_audio_device *adev, int in_out, char *devices)
{
    ALOGD("get_audio_devices_active: %s", devices);
    int card = 0;

    if (devices == 0)
        return -1;

    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (in_out & AUDIO_IN)
        {
            if ((adev->dev_manager[card].flag_in == in_out) && (adev->dev_manager[card].flag_in_active == 1))
            {
                strcat(devices, adev->dev_manager[card].name);
                strcat(devices, ",");
            }
        }
        else
        {
            if ((adev->dev_manager[card].flag_out == in_out) && (adev->dev_manager[card].flag_out_active == 1))
            {
                strcat(devices, adev->dev_manager[card].name);
                strcat(devices, ",");
            }
        }
    }

    devices[strlen(devices) - 1] = 0;

    ALOGD("get_audio_devices_active: %s", devices);

    return 0;
}

static void force_all_standby(struct sunxi_audio_device *adev)
{
    struct sunxi_stream_in *in;
    struct sunxi_stream_out *out;

    if (adev->active_output)
    {
        out = adev->active_output;
        pthread_mutex_lock(&out->lock);
        do_output_standby(out);
        pthread_mutex_unlock(&out->lock);
    }
    if (adev->active_input)
    {
        in = adev->active_input;
        pthread_mutex_lock(&in->lock);
        do_input_standby(in);
        pthread_mutex_unlock(&in->lock);
    }
}

static void select_mode(struct sunxi_audio_device *adev)
{
    if (adev->mode == AUDIO_MODE_IN_CALL)
    {
        ALOGV("Entering IN_CALL state, in_call=%d", adev->in_call);
        if (!adev->in_call)
        {
            force_all_standby(adev);
            adev->in_call = 1;
        }
    }
    else
    {
        ALOGV("Leaving IN_CALL state, in_call=%d, mode=%d",
              adev->in_call, adev->mode);
        if (adev->in_call)
        {
            adev->in_call = 0;
            force_all_standby(adev);
        }
    }
}

static void set_audio_path(struct sunxi_audio_device *adev, int device_path, int status)
{
    const char *name = NULL;
    struct audio_route *ar = NULL;

    if (device_path == OUT_DEVICE_LINEOUT || device_path == AA_LINEIN_LINEOUT || device_path == AA_FMIN_LINEOUT)
    {
        ar = adev->ar_codec;
    }
    else
    {
        ar = adev->ar_ahub;
    }

    if (!ar)
    {
        ALOGE("FUNC: %s, LINE: %d, audio route is not init", __FUNCTION__, __LINE__);
        return;
    }

    //audio_route_reset(ar);
    if (status)
    {
        name = route_configs[device_path].open;
    }
    else
    {
        name = route_configs[device_path].close;
    }

    if (name)
    {
        audio_route_apply_path(ar, name);
        audio_route_update_mixer(ar);
    }
    else
    {
        ALOGE("FUNC: %s, LINE: %d, cannot find path for %d", __FUNCTION__, __LINE__, device_path);
    }
}

static void reset_audio_path(struct sunxi_audio_device *adev)
{
    if (adev->stanby)
    {
        if (adev->ar_ahub)
        {
            audio_route_free(adev->ar_ahub);
            adev->ar_ahub = NULL;
        }
        if (adev->ar_codec)
        {
            audio_route_free(adev->ar_codec);
            adev->ar_codec = NULL;
        }
        if (adev->ar_ahub == NULL)
        {
            ALOGD("reset_audio_path card %d, init ahub route ......", adev->card_ahub);
            adev->ar_ahub = audio_route_init(adev->card_ahub, AUDIO_AHUB_XML_PATH);
        }
        if (adev->ar_codec == NULL)
        {
            ALOGD("reset_audio_path card %d, init codec route ......", adev->card_codec);
            adev->ar_codec = audio_route_init(adev->card_codec, AUDIO_CODEC_XML_PATH);
        }
        adev->stanby = false;
    }
    if (adev->output_active_cards & AUDIO_CARD_CODEC)
    {
        set_audio_path(adev, OUT_DEVICE_LINEOUT, 1);
    }
    if (adev->output_active_cards & AUDIO_CARD_I2S2)
    {
        set_audio_path(adev, OUT_DEVICE_I2S2, 1);
    }
    if (adev->output_active_cards & AUDIO_CARD_I2S3)
    {
        set_audio_path(adev, OUT_DEVICE_I2S3, 1);
    }

    if (adev->input_active_cards & AUDIO_CARD_AC107)
    {
        set_audio_path(adev, IN_DEVICE_AC107, 1);
        init_ac107_devices(adev);
    }
    else if (adev->input_active_cards & AUDIO_CARD_TD100)
    {
        set_audio_path(adev, IN_DEVICE_TD100, 1);
        init_td100_devices(adev);
    }
    else if (adev->input_active_cards & AUDIO_CARD_I2S2)
    {
        set_audio_path(adev, IN_DEVICE_I2S2, 1);
    }
    else if (adev->input_active_cards & AUDIO_CARD_I2S3)
    {
        set_audio_path(adev, IN_DEVICE_I2S3, 1);
    }
}

static void select_output_device(struct sunxi_audio_device *adev)
{
    ALOGD("line:%d,%s,adev->mode:%d", __LINE__, __FUNCTION__, adev->mode);
}

static void select_input_device(struct sunxi_audio_device *adev)
{
    ALOGD("line:%d,%s,adev->mode:%d", __LINE__, __FUNCTION__, adev->mode);
}

/* must be called with hw device and output stream mutexes locked */

static int start_output_stream(struct sunxi_stream_out *out)
{
    struct sunxi_audio_device *adev = out->dev;
    unsigned int card = 0;
    unsigned int port = 0;
    unsigned int index = 0;
    int ret;

    if (adev->mode == AUDIO_MODE_IN_CALL)
    {
        ALOGW("mode in call, do not start stream");
        return 0;
    }

    adev->active_output = out;

    reset_audio_path(adev);

    for (index = 0; index < MAX_AUDIO_DEVICES; index++)
    {
        if (adev->dev_manager[index].flag_exist && (adev->dev_manager[index].flag_out == AUDIO_OUT) && adev->dev_manager[index].flag_out_active)
        {
            card = index;
            port = 0;

            ALOGV("use %s to playback audio", adev->dev_manager[index].name);
            out->multi_config[index] = pcm_config_out;
            out->multi_config[index].start_threshold = DEFAULT_OUTPUT_PERIOD_SIZE * 2;

            if (adev->dev_manager[index].ahub_device)
            {
#if ENABLE_SUB_PCM_DEVICE
                if (strcmp(adev->dev_manager[index].name, SUB_PCM_DEVICE))
                {
                    out->sub_pcm[index] = pcm_open(index, port, PCM_OUT, &out->multi_config[index]);
                }
#else
                out->sub_pcm[index] = pcm_open(index, port, PCM_OUT, &out->multi_config[index]);
#endif
                if (!pcm_is_ready(out->sub_pcm[index]))
                {
                    ALOGE("cannot open pcm driver: %s", pcm_get_error(out->sub_pcm[index]));
                    pcm_close(out->sub_pcm[index]);
                    out->sub_pcm[index] = NULL;
                    adev->active_output = NULL;
                    return -ENOMEM;
                }
                card = adev->card_ahub;
                port = adev->dev_manager[index].port;
                pcm_start(out->sub_pcm[index]);
            }

            out->multi_pcm[index] = pcm_open(card, port, PCM_OUT, &out->multi_config[index]);
            if (!pcm_is_ready(out->multi_pcm[index]))
            {
                ALOGE("cannot open pcm driver: %s", pcm_get_error(out->multi_pcm[index]));
                pcm_close(out->multi_pcm[index]);
                out->multi_pcm[index] = NULL;
                adev->active_output = NULL;
                return -ENOMEM;
            }

            if (DEFAULT_SAMPLING_RATE != out->multi_config[index].rate)
            {
                ret = create_resampler(DEFAULT_SAMPLING_RATE,
                                       out->multi_config[index].rate,
                                       2,
                                       RESAMPLER_QUALITY_DEFAULT,
                                       NULL,
                                       &out->multi_resampler[index]);
                if (ret != 0)
                {
                    ALOGE("create out resampler failed, %d -> %d", DEFAULT_SAMPLING_RATE, out->multi_config[index].rate);
                    return ret;
                }

                ALOGV("create out resampler OK, %d -> %d", DEFAULT_SAMPLING_RATE, out->multi_config[index].rate);
            }
            else
                ALOGV("play audio with %d Hz serial sample rate.", DEFAULT_SAMPLING_RATE);

            if (out->multi_resampler[index])
            {
                out->multi_resampler[index]->reset(out->multi_resampler[index]);
            }
        }
    }

    return 0;
}

static int check_input_parameters(uint32_t sample_rate, int format, int channel_count)
{
    if (format != AUDIO_FORMAT_PCM_16_BIT)
        return -EINVAL;

    if ((channel_count < 1) || (channel_count > 2))
        return -EINVAL;

    switch (sample_rate)
    {
    case 8000:
    case 11025:
    case 12000:
    case 16000:
    case 22050:
    case 24000:
    case 32000:
    case 44100:
    case 48000:
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static size_t get_input_buffer_size(uint32_t sample_rate, int format, int channel_count)
{
    size_t size;

    if (check_input_parameters(sample_rate, format, channel_count) != 0)
        return 0;

    /* take resampling into account and return the closest majoring
    multiple of 16 frames, as audioflinger expects audio buffers to
    be a multiple of 16 frames */
    size = (pcm_config_in.period_size * sample_rate) / pcm_config_in.rate;
    size = ((size + 15) / 16) * 16;

    return size * channel_count * sizeof(short);
}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    UNUSED(stream);
    return DEFAULT_SAMPLING_RATE;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    UNUSED(stream);
    UNUSED(rate);
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;

    /* take resampling into account and return the closest majoring
    multiple of 16 frames, as audioflinger expects audio buffers to
    be a multiple of 16 frames */
    size_t size = ((DEFAULT_OUTPUT_PERIOD_SIZE + 15) / 16) * 16;
    return size * audio_stream_out_frame_size(&out->stream);
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    UNUSED(stream);
    return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    UNUSED(stream);
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    audio_format_t outformat = AUDIO_FORMAT_PCM_16_BIT;
    if (out->format != AUDIO_FORMAT_DEFAULT)
        outformat = out->format;
    return outformat;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    UNUSED(stream);
    UNUSED(format);
    return 0;
}

/* must be called with hw device and output stream mutexes locked */
static int do_output_standby(struct sunxi_stream_out *out)
{
    int index = 0;

#if ENABLE_SUB_PCM_DEVICE
    struct sunxi_audio_device *adev = out->dev;
#endif

    if (!out->standby)
    {
        for (index = 0; index < MAX_AUDIO_DEVICES; index++)
        {
            if (out->multi_pcm[index])
            {
                pcm_close(out->multi_pcm[index]);
                out->multi_pcm[index] = NULL;
            }

            if (out->sub_pcm[index])
            {
#if ENABLE_SUB_PCM_DEVICE
                if (strcmp(adev->dev_manager[index].name, SUB_PCM_DEVICE))
                {
                    pcm_close(out->sub_pcm[index]);
                    out->sub_pcm[index] = NULL;
                }
#else
                pcm_close(out->sub_pcm[index]);
                out->sub_pcm[index] = NULL;
#endif
            }

            if (out->multi_resampler[index])
            {
                release_resampler(out->multi_resampler[index]);
                out->multi_resampler[index] = NULL;
            }
        }

        //adev->active_output = 0;
        out->standby = 1;
    }
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    int status;

    ALOGD("out_standby");
    pthread_mutex_lock(&out->lock);
    status = do_output_standby(out);
    pthread_mutex_unlock(&out->lock);
    return status;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    UNUSED(stream);
    UNUSED(fd);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    UNUSED(stream);
    struct str_parms *parms;
    char value[128];
    int ret, val = 0;

    parms = str_parms_create_str(kvpairs);

    ALOGV("out_set_parameters: %s", kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0)
    {
        val = atoi(value);
    }

    str_parms_destroy(parms);
    return ret;
}

static char *out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    UNUSED(stream);
    UNUSED(keys);
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    UNUSED(stream);

    return (DEFAULT_OUTPUT_PERIOD_SIZE * DEFAULT_OUTPUT_PERIOD_COUNT * 1000) / DEFAULT_SAMPLING_RATE;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    UNUSED(stream);
    UNUSED(left);
    UNUSED(right);
    return -ENOSYS;
}

static ssize_t out_write(struct audio_stream_out *stream, const void *buffer,
                         size_t bytes)
{
    int ret;
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    size_t frame_size = audio_stream_out_frame_size(&out->stream);
    size_t in_frames = bytes / frame_size;
    size_t out_frames = RESAMPLER_BUFFER_SIZE / frame_size;
    void *buf;
    int index;
    int card;
    char prop_value[128];

    if (adev->mode == AUDIO_MODE_IN_CALL)
    {
        ALOGD("mode in call, do not out_write");
        int time = bytes * 1000 * 1000 / 4 / DEFAULT_SAMPLING_RATE;
        usleep(time);
        return bytes;
    }

    ret = property_get(PROP_RAWDATA_OUTPUT, prop_value, "");
    if (ret > 0)
    {
        adev->raw_enable = atoi(prop_value);
    }
    else
    {
        adev->raw_enable = false;
    }

    if (adev->raw_enable)
    {
        ALOGW("rawdata output mode, do not write pcm data");
        if (!out->standby)
        {
            pthread_mutex_lock(&out->lock);
            do_output_standby(out);
            pthread_mutex_unlock(&out->lock);
        }
        int time = bytes * 1000 * 1000 / 4 / DEFAULT_SAMPLING_RATE;
        usleep(time);
        return bytes;
    }

    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the output stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&out->lock);

    if (out->standby)
    {
        ret = start_output_stream(out);
        if (ret != 0)
        {
            pthread_mutex_unlock(&adev->lock);
            goto exit;
        }
        out->standby = 0;
    }
    pthread_mutex_unlock(&adev->lock);

    for (index = MAX_AUDIO_DEVICES; index >= 0; index--)
    {
        if (adev->dev_manager[index].flag_exist && (adev->dev_manager[index].flag_out == AUDIO_OUT) && adev->dev_manager[index].flag_out_active)
        {
            card = index;

            if (out->multi_resampler[card])
            {
                out->multi_resampler[card]->resample_from_input(out->multi_resampler[card],
                                                                (int16_t *)buffer,
                                                                &in_frames,
                                                                (int16_t *)out->buffer,
                                                                &out_frames);
                buf = out->buffer;
            }
            else
            {
                out_frames = in_frames;
                buf = (void *)buffer;
            }

            if (out->multi_config[card].channels == 2)
            {
                ret = pcm_write(out->multi_pcm[card], (void *)buf, out_frames * frame_size);
            }
            else
            {
                size_t i;
                char *pcm_buf = (char *)buf;
                for (i = 0; i < out_frames; i++)
                {
                    pcm_buf[2 * i + 2] = pcm_buf[4 * i + 4];
                    pcm_buf[2 * i + 3] = pcm_buf[4 * i + 5];
                }
                ret = pcm_write(out->multi_pcm[card], (void *)buf, out_frames * frame_size / 2);
            }

            if (ret != 0)
            {
                ALOGE("##############out_write()  Warning:write fail, reopen it ret = %d #######################", ret);
                do_output_standby(out);
                usleep(30000);
                break;
            }
        }
    }
exit:
    pthread_mutex_unlock(&out->lock);
    if (ret != 0)
    {
        usleep(bytes * 1000000 / audio_stream_out_frame_size(stream) /
               out_get_sample_rate(&stream->common));
    }

    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    UNUSED(stream);
    UNUSED(dsp_frames);
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    UNUSED(stream);
    UNUSED(effect);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    UNUSED(stream);
    UNUSED(effect);
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    UNUSED(stream);
    UNUSED(timestamp);
    return -EINVAL;
}

/* must be called with hw device and input stream mutexes locked */
static int start_input_stream(struct sunxi_stream_in *in)
{
    int index = 0;
    int card = 0;
    int port = 0;
    struct sunxi_audio_device *adev = in->dev;

    adev->active_input = in;

    if (adev->mode == AUDIO_MODE_IN_CALL)
    {
        ALOGD("in call mode , start_input_stream, return");
        return 0;
    }

    reset_audio_path(adev);

    ALOGV("catpure audio with %d Hz serial sample rate.", DEFAULT_SAMPLING_RATE);

    for (index = 0; index < MAX_AUDIO_DEVICES; index++)
    {
        if (adev->dev_manager[index].flag_exist && (adev->dev_manager[index].flag_in == AUDIO_IN) && adev->dev_manager[index].flag_in_active)
        {
            ALOGD("use %s to capture audio", adev->dev_manager[index].name);
            break;
        }
    }

    card = index;
    port = 0;
    if (adev->dev_manager[index].ahub_device)
    {
        in->sub_pcm = pcm_open(index, port, PCM_IN, &in->config);
        if (!pcm_is_ready(in->sub_pcm))
        {
            ALOGE("cannot open pcm_in driver: %s", pcm_get_error(in->sub_pcm));
            pcm_close(in->sub_pcm);
            adev->active_input = NULL;
            return -ENOMEM;
        }
        card = adev->card_ahub;
        port = adev->dev_manager[index].port;
        ALOGD("ahub card port : %d", port);
        pcm_start(in->sub_pcm);
    }

    in->pcm = pcm_open(card, port, PCM_IN, &in->config);
    if (!pcm_is_ready(in->pcm))
    {
        ALOGE("cannot open pcm_in driver: %s", pcm_get_error(in->pcm));
        pcm_close(in->pcm);
        adev->active_input = NULL;
        return -ENOMEM;
    }

#if USE_INPUT_RESAMPLER
    int ret = 0;
    if (in->requested_rate != in->config.rate)
    {
        in->buf_provider.get_next_buffer = get_next_buffer;
        in->buf_provider.release_buffer = release_buffer;

        ret = create_resampler(in->config.rate,
                               in->requested_rate,
                               in->config.channels,
                               RESAMPLER_QUALITY_DEFAULT,
                               &in->buf_provider,
                               &in->resampler);
        if (ret != 0)
        {
            ALOGE("create in resampler failed, %d -> %d", in->config.rate, in->requested_rate);
            ret = -EINVAL;
            goto err;
        }

        ALOGV("create in resampler OK, %d -> %d", in->config.rate, in->requested_rate);
    }
    else
        ALOGV("do not use in resampler");

    /* if no supported sample rate is available, use the resampler */
    if (in->resampler)
    {
        in->resampler->reset(in->resampler);
        in->frames_in = 0;
    }

    return 0;

err:
    if (in->resampler)
        release_resampler(in->resampler);

    return -1;
#else
    return 0;
#endif
}

static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;

    return in->requested_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    UNUSED(stream);
    UNUSED(rate);
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;

    return get_input_buffer_size(in->requested_rate,
                                 AUDIO_FORMAT_PCM_16_BIT,
                                 in->config.channels);
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;

    if (in->config.channels == 1)
    {
        return AUDIO_CHANNEL_IN_MONO;
    }
    else
    {
        return AUDIO_CHANNEL_IN_STEREO;
    }
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    UNUSED(stream);
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    UNUSED(stream);
    UNUSED(format);
    return 0;
}

/* must be called with hw device and input stream mutexes locked */
static int do_input_standby(struct sunxi_stream_in *in)
{
    struct sunxi_audio_device *adev = in->dev;

    if (!in->standby)
    {
        if (in->pcm)
        {
            pcm_close(in->pcm);
            in->pcm = NULL;
        }

        if (in->sub_pcm)
        {
            pcm_close(in->sub_pcm);
            in->sub_pcm = NULL;
        }

        adev->active_input = 0;

#if USE_INPUT_RESAMPLER
        if (in->resampler)
        {
            release_resampler(in->resampler);
            in->resampler = NULL;
        }
#endif

        in->standby = 1;
    }
    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    int status;

    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    status = do_input_standby(in);
    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);
    return status;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    UNUSED(stream);
    UNUSED(fd);
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    UNUSED(stream);
    ALOGV("in_set_parameters: %s", kvpairs);
    return 0;
}

static char *in_get_parameters(const struct audio_stream *stream,
                               const char *keys)
{
    UNUSED(stream);
    UNUSED(keys);
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    UNUSED(stream);
    UNUSED(gain);
    return 0;
}

#if USE_INPUT_RESAMPLER
/* read_frames() reads frames from kernel driver, down samples to capture rate
 * if necessary and output the number of frames requested to the buffer specified */
static ssize_t read_frames(struct sunxi_stream_in *in, void *buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;

    while (frames_wr < frames)
    {
        size_t frames_rd = frames - frames_wr;
        if (in->resampler != NULL)
        {
            in->resampler->resample_from_provider(in->resampler,
                                                  (int16_t *)((char *)buffer +
                                                              frames_wr * audio_stream_in_frame_size(&in->stream)),
                                                  &frames_rd);
        }
        else
        {
            struct resampler_buffer buf = {
                {
                    .raw = NULL,
                },
                .frame_count = frames_rd,
            };
            get_next_buffer(&in->buf_provider, &buf);
            if (buf.raw != NULL)
            {
                memcpy((char *)buffer +
                           frames_wr * audio_stream_in_frame_size(&in->stream),
                       buf.raw,
                       buf.frame_count * audio_stream_in_frame_size(&in->stream));
                frames_rd = buf.frame_count;
            }
            release_buffer(&in->buf_provider, &buf);
        }
        /* in->read_status is updated by getNextBuffer() also called by
         * in->resampler->resample_from_provider() */
        if (in->read_status != 0)
            return in->read_status;

        frames_wr += frames_rd;
    }
    return frames_wr;
}
#endif

static ssize_t in_read(struct audio_stream_in *stream, void *buffer,
                       size_t bytes)
{
    int ret = 0;
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    struct sunxi_audio_device *adev = in->dev;
    size_t frames_rq = 0;

    if (adev->mode == AUDIO_MODE_IN_CALL)
    {
        //ALOGD("in call mode, in_read, return ;");
        usleep(10000);
        return 1;
    }

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);
    if (in->standby)
    {
        ret = start_input_stream(in);
        if (ret == 0)
            in->standby = 0;
    }
    pthread_mutex_unlock(&adev->lock);

    if (ret < 0)
        goto exit;

    /* place after start_input_stream, because start_input_stream() change frame size */
    frames_rq = bytes / audio_stream_in_frame_size(stream);

#if USE_INPUT_RESAMPLER
    if (in->resampler != NULL)
    {
        ret = read_frames(in, buffer, frames_rq);
    }
    else
#endif
    {
        ret = pcm_read(in->pcm, buffer, bytes);
    }

    if (ret > 0)
        ret = 0;

    if (ret == 0 && adev->mic_mute)
        memset(buffer, 0, bytes);

exit:
    if (ret < 0)
        usleep(bytes * 1000000 / audio_stream_in_frame_size(stream) /
               in_get_sample_rate(&stream->common));

    pthread_mutex_unlock(&in->lock);
    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    UNUSED(stream);
    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream,
                               effect_handle_t effect)
{
    UNUSED(stream);
    UNUSED(effect);

    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream,
                                  effect_handle_t effect)
{
    UNUSED(stream);
    UNUSED(effect);

    return 0;
}

#if ENABLE_SUB_PCM_DEVICE
static int set_sub_pcm_output_status(struct sunxi_stream_out *out, bool open)
{
    if (!out)
    {
        ALOGE("output device is not inited");
        return -ENOMEM;
    }
    int card;
    struct sunxi_audio_device *adev = out->dev;
    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (!strcmp(adev->dev_manager[card].name, SUB_PCM_DEVICE))
        {
            if (open)
            {
                out->multi_config[card] = pcm_config_out;
                out->multi_config[card].start_threshold = DEFAULT_OUTPUT_PERIOD_SIZE * 2;
                out->sub_pcm[card] = pcm_open(card, 0, PCM_OUT, &out->multi_config[card]);
                if (!pcm_is_ready(out->sub_pcm[card]))
                {
                    ALOGE("cannot open pcm driver: %s", pcm_get_error(out->sub_pcm[card]));
                    pcm_close(out->sub_pcm[card]);
                    out->sub_pcm[card] = NULL;
                    return -ENOMEM;
                }
                pcm_start(out->sub_pcm[card]);
            }
            else
            {
                if (out->sub_pcm[card])
                {
                    ALOGD("close sub pcm %s", SUB_PCM_DEVICE);
                    pcm_close(out->sub_pcm[card]);
                    out->sub_pcm[card] = NULL;
                }
            }
            break;
        }
    }
    return 0;
}
#endif

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address)
{
    struct sunxi_audio_device *ladev = (struct sunxi_audio_device *)dev;
    struct sunxi_stream_out *out;
    UNUSED(handle);
    UNUSED(devices);
    UNUSED(address);

    ALOGV("adev_open_output_stream, flags: %x", flags);

    out = (struct sunxi_stream_out *)calloc(1, sizeof(struct sunxi_stream_out));
    if (!out)
        return -ENOMEM;

    out->buffer = malloc(RESAMPLER_BUFFER_SIZE); /* todo: allow for reallocing */

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
    out->dev = ladev;
    out->standby = 1;

    ALOGV("+++++++++++++++ adev_open_output_stream: req_sample_rate: %d, fmt: %x, channel_count: %d",
          config->sample_rate, config->format, config->channel_mask);

    out->config = pcm_config_out;

    config->format = out_get_format(&out->stream.common);
    config->channel_mask = out_get_channels(&out->stream.common);
    config->sample_rate = out_get_sample_rate(&out->stream.common);

    out->flags = flags;
    *stream_out = &out->stream;
    ladev->active_output = out;

#if ENABLE_SUB_PCM_DEVICE
    set_sub_pcm_output_status(out, true);
#endif

    select_output_device(ladev);
    return 0;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    UNUSED(dev);

    if (adev->mode == AUDIO_MODE_IN_CALL)
    {
        ALOGW("mode in call, do not adev_close_output_stream");
        return;
    }

    out_standby(&stream->common);

    if (out->buffer)
        free(out->buffer);

    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;
    struct str_parms *parms;
    char value[32];
    int ret;
    char value2[32];
    int val = 0;

    ALOGV("adev_set_parameters, %s", kvpairs);

    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_HAL_PARAM_OUTPUT_DEVICES, value, sizeof(value));
    if (ret >= 0)
    {
        ret = str_parms_get_str(parms, AUDIO_HAL_PARAM_OUTPUT_DEVICES_IS_ACTIVE, value2, sizeof(value2));
        if (ret >= 0)
        {
            val = atoi(value2);
            if (val)
            {
                if (strcmp(value, AUDIO_HAL_PARAM_OUTPUT_DEVICES_AUDIO_CODEC) == 0)
                {
                    adev->output_active_cards |= AUDIO_CARD_CODEC;
                }
                else if (strcmp(value, AUDIO_HAL_PARAM_OUTPUT_DEVICES_HDMI) == 0)
                {
                    adev->output_active_cards |= AUDIO_CARD_HDMI;
                }
                else if (strcmp(value, AUDIO_HAL_PARAM_OUTPUT_DEVICES_SPDIF) == 0)
                {
                    adev->output_active_cards |= AUDIO_CARD_SPDIF;
                }
                else
                {
                    ALOGE("known output device %s", value);
                }
            }
            else
            {
                if (strcmp(value, AUDIO_HAL_PARAM_OUTPUT_DEVICES_AUDIO_CODEC) == 0)
                {
                    adev->output_active_cards &= ~AUDIO_CARD_CODEC;
                }
                else if (strcmp(value, AUDIO_HAL_PARAM_OUTPUT_DEVICES_HDMI) == 0)
                {
                    adev->output_active_cards &= ~AUDIO_CARD_HDMI;
                }
                else if (strcmp(value, AUDIO_HAL_PARAM_OUTPUT_DEVICES_SPDIF) == 0)
                {
                    adev->output_active_cards &= ~AUDIO_CARD_SPDIF;
                }
                else
                {
                    ALOGE("known output device %s", value);
                }
            }
            if (adev->output_active_cards == 0)
            {
                adev->output_active_cards |= AUDIO_CARD_CODEC;
            }
        }

        pthread_mutex_lock(&adev->lock);
        set_audio_devices_active(adev, AUDIO_OUT);
        if (adev->active_output)
            do_output_standby(adev->active_output);
        pthread_mutex_unlock(&adev->lock);
    }

    ret = str_parms_get_str(parms, AUDIO_HAL_PARAM_ROUTE, value, sizeof(value));
    if (ret >= 0)
    {
        ret = str_parms_get_str(parms, AUDIO_HAL_PARAM_ROUTE_VALUE, value2, sizeof(value2));
        if (ret >= 0)
        {
            val = atoi(value2);
            ALOGD("route value = %d", val);
        }
        if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_LINEOUT) == 0)
        {
            set_audio_path(adev, OUT_DEVICE_LINEOUT, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_LINEIN_LINEOUT) == 0)
        {
            set_audio_path(adev, AA_LINEIN_LINEOUT, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_FMIN_LINEOUT) == 0)
        {
            set_audio_path(adev, AA_FMIN_LINEOUT, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_HDMI_OUT) == 0)
        {
            set_audio_path(adev, OUT_DEVICE_HDMI, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_AC107_IN) == 0)
        {
            set_audio_path(adev, IN_DEVICE_AC107, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_I2S2_OUT) == 0)
        {
            set_audio_path(adev, OUT_DEVICE_I2S2, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_I2S2_IN) == 0)
        {
            set_audio_path(adev, IN_DEVICE_I2S2, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_I2S3_OUT) == 0)
        {
            set_audio_path(adev, OUT_DEVICE_I2S3, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_I2S3_IN) == 0)
        {
            set_audio_path(adev, IN_DEVICE_I2S3, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_ROUTE_SPDIF) == 0)
        {
            set_audio_path(adev, VOL_LINEOUT, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_LINEOUT) == 0)
        {
            set_mixer_value(adev->mixer_ac107, route_configs[VOL_LINEOUT].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_LINEIN) == 0)
        {
            set_mixer_value(adev->mixer_ac107, route_configs[VOL_LINEIN_OMIX].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_FMIN) == 0)
        {
            set_mixer_value(adev->mixer_ac107, route_configs[VOL_FMIN_OMIX].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_AC107_C1_PGA) == 0)
        {
            set_mixer_value(adev->mixer_ac107, route_configs[VOL_AC107_C1_PGA].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_AC107_C2_PGA) == 0)
        {
            set_mixer_value(adev->mixer_ac107, route_configs[VOL_AC107_C2_PGA].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_AC107_C1_DIGITAL) == 0)
        {
            set_mixer_value(adev->mixer_ac107, route_configs[VOL_AC107_C1_DIGITAL].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_AC107_C2_DIGITAL) == 0)
        {
            set_mixer_value(adev->mixer_ac107, route_configs[VOL_AC107_C2_DIGITAL].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_TD100_MIC1) == 0)
        {
            adev->vol_td100_mic1_val = val;
            set_mixer_value(adev->mixer_td100, route_configs[VOL_TD100_MIC1].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_TD100_MIC2) == 0)
        {
            adev->vol_td100_mic2_val = val;
            set_mixer_value(adev->mixer_td100, route_configs[VOL_TD100_MIC2].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_TD100_MIC3) == 0)
        {
            adev->vol_td100_mic3_val = val;
            set_mixer_value(adev->mixer_td100, route_configs[VOL_TD100_MIC3].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_VOL_TD100_LINEIN) == 0)
        {
            adev->vol_td100_linein_val = val;
            set_mixer_value(adev->mixer_td100, route_configs[VOL_TD100_LINEIN].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_DVOL_TD100_ADC1) == 0)
        {
            adev->dvol_td100_adc1_val = val;
            set_mixer_value(adev->mixer_td100, route_configs[DVOL_TD100_ADC1].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_DVOL_TD100_ADC2) == 0)
        {
            adev->dvol_td100_adc2_val = val;
            set_mixer_value(adev->mixer_td100, route_configs[DVOL_TD100_ADC2].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_CH1_TD100_SELECT) == 0)
        {
            adev->ch1_td100_select_val = val;
            set_mixer_value(adev->mixer_td100, route_configs[CH1_TD100_SELECT].open, val);
        }
        else if (strcmp(value, AUDIO_HAL_PARAM_CH2_TD100_SELECT) == 0)
        {
            adev->ch2_td100_select_val = val;
            set_mixer_value(adev->mixer_td100, route_configs[CH2_TD100_SELECT].open, val);
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SCREEN_STATE, value, sizeof(value));
    if (ret >= 0)
    {
        pthread_mutex_lock(&adev->lock);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
        {
            adev->stanby = true;
#if ENABLE_SUB_PCM_DEVICE
            set_sub_pcm_output_status(adev->active_output, true);
#endif
        }
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_OFF) == 0)
        {
            adev->stanby = false;
#if ENABLE_SUB_PCM_DEVICE
            set_sub_pcm_output_status(adev->active_output, false);
#endif
        }
        pthread_mutex_unlock(&adev->lock);
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICE_CONNECT, value, sizeof(value));
    if (ret >= 0)
    {
        int val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        if (adev->output_active_cards & AUDIO_CARD_AUTO_DEC)
        {
            if (val == AUDIO_DEVICE_OUT_AUX_DIGITAL)
            {
                adev->output_active_cards &= ~AUDIO_CARD_CODEC;
                adev->output_active_cards |= AUDIO_CARD_HDMI;
            }
            else if (val == AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET)
            {
                adev->output_active_cards |= AUDIO_CARD_SPDIF;
            }
        }

        set_audio_devices_active(adev, AUDIO_OUT);
        pthread_mutex_unlock(&adev->lock);
    }
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICE_DISCONNECT, value, sizeof(value));
    if (ret >= 0)
    {
        int val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        if (adev->output_active_cards & AUDIO_CARD_AUTO_DEC)
        {
            if (val == AUDIO_DEVICE_OUT_AUX_DIGITAL)
            {
                adev->output_active_cards &= ~AUDIO_CARD_HDMI;
                adev->output_active_cards |= AUDIO_CARD_CODEC;
            }
            else if (val == AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET)
            {
                adev->output_active_cards &= ~AUDIO_CARD_SPDIF;
            }
        }

        set_audio_devices_active(adev, AUDIO_OUT);
        pthread_mutex_unlock(&adev->lock);
    }
    ret = str_parms_get_str(parms, "mediasw.sft.rawdata", value, sizeof(value));
    if (ret >= 0)
    {
        property_set(PROP_RAWDATA_KEY, value);
    }
    str_parms_destroy(parms);
    return ret;
}

static char *adev_get_parameters(const struct audio_hw_device *dev,
                                 const char *keys)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;
    ALOGV("adev_get_parameters, %s", keys);

    char devices[128];
    memset(devices, 0, sizeof(devices));

    if (!strcmp(keys, AUDIO_HAL_PARAM_OUTPUT_DEVICES))
    {
        strcpy(devices, AUDIO_HAL_PARAM_OUTPUT_DEVICES);
        strcat(devices, " :");
        if (adev->output_active_cards & AUDIO_CARD_CODEC)
        {
            strcat(devices, " ");
            strcat(devices, AUDIO_HAL_PARAM_OUTPUT_DEVICES_AUDIO_CODEC);
        }
        if (adev->output_active_cards & AUDIO_CARD_HDMI)
        {
            strcat(devices, " ");
            strcat(devices, AUDIO_HAL_PARAM_OUTPUT_DEVICES_HDMI);
        }
        if (adev->output_active_cards & AUDIO_CARD_SPDIF)
        {
            strcat(devices, " ");
            strcat(devices, AUDIO_HAL_PARAM_OUTPUT_DEVICES_SPDIF);
        }
        ALOGD("%s", devices);
        return strdup(devices);
    }
    if (!strcmp(keys, AUDIO_PARAMETER_STREAM_ROUTING))
    {
        char prop_value[512];
        int ret = property_get("audio.routing", prop_value, "");
        if (ret > 0)
        {
            return strdup(prop_value);
        }
    }

    if (!strcmp(keys, AUDIO_PARAMETER_DEVICES_IN))
        return strdup(get_audio_devices(adev, AUDIO_IN));

    if (!strcmp(keys, AUDIO_PARAMETER_DEVICES_OUT))
        return strdup(get_audio_devices(adev, AUDIO_OUT));

    if (!strcmp(keys, AUDIO_PARAMETER_DEVICES_IN_ACTIVE))
        if (!get_audio_devices_active(adev, AUDIO_IN, devices))
            return strdup(devices);

    if (!strcmp(keys, AUDIO_PARAMETER_DEVICES_OUT_ACTIVE))
        if (!get_audio_devices_active(adev, AUDIO_OUT, devices))
            return strdup(devices);

    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    UNUSED(dev);
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    UNUSED(dev);

    ALOGV("adev_set_voice_volume, volume: %f", volume);

    return 0;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    UNUSED(dev);
    UNUSED(volume);
    F_LOG;
    return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    UNUSED(dev);
    UNUSED(volume);
    F_LOG;
    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;

    pthread_mutex_lock(&adev->lock);
    if (adev->mode != mode)
    {
        adev->mode = mode;
        select_mode(adev);
    }
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;

    adev->mic_mute = state;

    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;

    *state = adev->mic_mute;

    return 0;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    UNUSED(dev);
    int channel_count = popcount(config->channel_mask);
    if (check_input_parameters(config->sample_rate, config->format, channel_count) != 0)
        return 0;

    return get_input_buffer_size(config->sample_rate, config->format, channel_count);
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags,
                                  const char *address,
                                  audio_source_t source)
{
    UNUSED(handle);
    UNUSED(flags);
    UNUSED(address);
    UNUSED(source);
    UNUSED(devices);
    struct sunxi_audio_device *ladev = (struct sunxi_audio_device *)dev;
    struct sunxi_stream_in *in;
    int ret;
    int channel_count = popcount(config->channel_mask);

    *stream_in = NULL;

    if (check_input_parameters(config->sample_rate, config->format, channel_count) != 0)
        return -EINVAL;

    in = (struct sunxi_stream_in *)calloc(1, sizeof(struct sunxi_stream_in));
    if (!in)
        return -ENOMEM;

    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    // default config
    memcpy(&in->config, &pcm_config_in, sizeof(pcm_config_in));

#if USE_INPUT_RESAMPLER
    in->requested_rate = config->sample_rate;
    in->config.channels = channel_count;
#else
    in->requested_rate = DEFAULT_SAMPLING_RATE;
    in->config.channels = DEFAULT_CHANNEL_COUNT;
#endif

    ALOGV("to malloc in-buffer: period_size: %d, frame_size: %zu",
          in->config.period_size, audio_stream_in_frame_size(&in->stream));
    in->buffer = malloc(in->config.period_size *
                        audio_stream_in_frame_size(&in->stream) * 8);

    if (!in->buffer)
    {
        ret = -ENOMEM;
        goto err;
    }

    in->dev = ladev;
    in->standby = 1;

    *stream_in = &in->stream;
    select_input_device(ladev);
    return 0;

err:
#if USE_INPUT_RESAMPLER
    if (in->resampler)
        release_resampler(in->resampler);
#endif

    free(in);
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                    struct audio_stream_in *stream)
{
    UNUSED(dev);
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;

    in_standby(&stream->common);

    if (in->buffer)
    {
        free(in->buffer);
        in->buffer = 0;
    }

#if USE_INPUT_RESAMPLER
    if (in->resampler)
    {
        release_resampler(in->resampler);
    }
#endif

    free(stream);

    ALOGD("adev_close_input_stream set voice record status");
    return;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    UNUSED(device);
    UNUSED(fd);
    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)device;
    // free audio route
    if (adev->ar_ahub)
    {
        audio_route_free(adev->ar_ahub);
        adev->ar_ahub = NULL;
    }
    if (adev->ar_codec)
    {
        audio_route_free(adev->ar_codec);
        adev->ar_codec = NULL;
    }
    if (adev->mixer_ac107)
    {
        mixer_close(adev->mixer_ac107);
        adev->mixer_ac107 = NULL;
    }
    if (adev->mixer_td100)
    {
        mixer_close(adev->mixer_td100);
        adev->mixer_td100 = NULL;
    }
    free(device);

    return 0;
}

static int adev_open(const hw_module_t *module, const char *name,
                     hw_device_t **device)
{
    struct sunxi_audio_device *adev;
    int hdmi_status;

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = calloc(1, sizeof(struct sunxi_audio_device));
    if (!adev)
        return -ENOMEM;

    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->hw_device.common.module = (struct hw_module_t *)module;
    adev->hw_device.common.close = adev_close;

    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.get_master_volume = adev_get_master_volume;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.dump = adev_dump;
    adev->stanby = false;
    adev->output_active_cards = OUTPUT_ACTIVE_CARDS;
    adev->input_active_cards = INPUT_ACTIVE_CARDS;

    adev->card_ac107 = adev->card_td100 = adev->card_ahub = adev->card_codec = -1;

    hdmi_status = get_hdmi_status();
    if (hdmi_status < 0)
    {
        ALOGE("get hdmi status error");
    }
    else if (hdmi_status == 1)
    {
        adev->output_active_cards |= AUDIO_CARD_HDMI;
    }

    init_audio_devices(adev);
    init_audio_devices_active(adev);

    if (adev->input_active_cards == AUDIO_CARD_AC107)
    {
        if (!adev->mixer_ac107)
        {
            free(adev);
            ALOGE("Unable to open ac107 mixer, aborting.");
            return -EINVAL;
        }
        else
        {
            init_ac107_devices(adev);
        }
    }

    if (adev->input_active_cards == AUDIO_CARD_TD100)
    {
        if (!adev->mixer_td100)
        {
            free(adev);
            ALOGE("Unable to open td100 mixer, aborting.");
            return -EINVAL;
        }
        else
        {
            adev->vol_td100_mic1_val = 5;
            adev->vol_td100_mic2_val = 5;
            adev->vol_td100_mic3_val = 5;
            adev->vol_td100_linein_val = 4;
            adev->dvol_td100_adc1_val = 170;
            adev->dvol_td100_adc2_val = 170;
            adev->ch1_td100_select_val = 0;
            adev->ch2_td100_select_val = 1;
            init_td100_devices(adev);
        }
    }

    /* Set the default route before the PCM stream is opened */
    pthread_mutex_lock(&adev->lock);
    adev->mode = AUDIO_MODE_NORMAL;

    // init T501 audio input and output path
    // set_audio_path(adev, AA_LINEIN_LINEOUT);      //default open linein to lineout path
    //set_mixer_value(adev->mixer_ac107, route_configs[VOL_AC107_C2_PGA].open, 12);

    pthread_mutex_unlock(&adev->lock);

    *device = &adev->hw_device.common;

    ALOGD("adev_open success ,LINE:%d,FUNC:%s", __LINE__, __FUNCTION__);
    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "sunxi audio HW HAL",
        .author = "author",
        .methods = &hal_module_methods,
    },
};

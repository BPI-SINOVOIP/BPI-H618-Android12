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

#define LOG_TAG "audio_hw_rawdata"
#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <log/log.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#include <audio_utils/resampler.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>

#include "audio_hw.h"

#define UNUSED(x) (void)(x)

enum SND_AUIDO_RAW_DATA_TYPE
{
    SND_AUDIO_RAW_DATA_UNKOWN = 0,
    SND_AUDIO_RAW_DATA_PCM = 1,
    SND_AUDIO_RAW_DATA_AC3 = 2,
    SND_AUDIO_RAW_DATA_MPEG1 = 3,
    SND_AUDIO_RAW_DATA_MP3 = 4,
    SND_AUDIO_RAW_DATA_MPEG2 = 5,
    SND_AUDIO_RAW_DATA_AAC = 6,
    SND_AUDIO_RAW_DATA_DTS = 7,
    SND_AUDIO_RAW_DATA_ATRAC = 8,
    SND_AUDIO_RAW_DATA_ONE_BIT_AUDIO = 9,
    SND_AUDIO_RAW_DATA_DOLBY_DIGITAL_PLUS = 10,
    SND_AUDIO_RAW_DATA_DTS_HD = 11,
    SND_AUDIO_RAW_DATA_MAT = 12,
    SND_AUDIO_RAW_DATA_DST = 13,
    SND_AUDIO_RAW_DATA_WMAPRO = 14
};

struct pcm_config pcm_config_rawdata = {
    .channels = AUX_DIGITAL_MULTI_DEFAULT_CHANNEL_COUNT, /* changed when the stream is opened */
    .rate = DEFAULT_SAMPLING_RATE,                       /* changed when the stream is opened */
    .period_size = AUX_DIGITAL_MULTI_PERIOD_SIZE,
    .period_count = AUX_DIGITAL_MULTI_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 0,
    .stop_threshold = INT_MAX,
    .avail_min = 0,
};

struct pcm_config pcm_config_out = {
    .channels = DEFAULT_CHANNEL_COUNT,
    .rate = DEFAULT_SAMPLING_RATE,
    .period_size = DEFAULT_OUTPUT_PERIOD_SIZE,
    .period_count = DEFAULT_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

#if 0
static int set_raw_flag(struct sunxi_audio_device *adev, int card, int raw_flag)
{
    ALOGD("set_raw_flag(card=%d, raw_flag=%d)", card, raw_flag);
    if (card == adev->cardCODEC) /* codec support pcm only */
        return -1;
    struct mixer *mixer = mixer_open(card);
    if (!mixer) {
        ALOGE("Unable to open the mixer, aborting.");
        return -1;
    }

    const char *control_name = (card == adev->cardHDMI) ? "hdmi audio format Function" :
                               (card == adev->cardAHUB)  ? "ahub audio format Function" : "spdif audio format Function";
    ALOGD("set_raw_flag control_name = %s)", control_name);
    const char *control_value = (raw_flag==SND_AUDIO_RAW_DATA_AC3) ? "AC3" :
                                (raw_flag==SND_AUDIO_RAW_DATA_DOLBY_DIGITAL_PLUS) ? "DOLBY_DIGITAL_PLUS":
                                (raw_flag==SND_AUDIO_RAW_DATA_MAT) ? "MAT":
                                (raw_flag==SND_AUDIO_RAW_DATA_DTS_HD) ? "DTS_HD":
                                (raw_flag==SND_AUDIO_RAW_DATA_DTS) ? "DTS" : "pcm";

    struct mixer_ctl *audio_format = mixer_get_ctl_by_name(mixer, control_name);
    if (audio_format)
        mixer_ctl_set_enum_by_string(audio_format, control_value);
    mixer_close(mixer);
    return 0;

}
#endif

static int find_card_id_by_name(const char *card_name)
{
    int ret;
    int fd;
    int i;
    char path[128];
    char name[64];

    for (i = 0; i < MAX_AUDIO_DEVICES; i++)
    {
        sprintf(path, "/sys/class/sound/card%d/id", i);
        ret = access(path, F_OK);
        if (ret)
        {
            ALOGW("can't find node %s", path);
            return -1;
        }

        fd = open(path, O_RDONLY);
        if (fd <= 0)
        {
            ALOGE("can't open %s", path);
            return -1;
        }

        ret = read(fd, name, sizeof(name));
        close(fd);
        if (ret > 0)
        {
            name[ret - 1] = '\0';
            if (!strcmp(name, card_name))
                return i;
        }
    }

    ALOGW("can't find card:%s", card_name);
    return -1;
}

/* must be called with hw device and output stream mutexes locked */
static int start_output_stream(struct sunxi_stream_out *out)
{
    int card = -1;
    int port = -1;
    int index = 0;
    char prop_value[PROPERTY_VALUE_MAX] = {0};
    struct sunxi_audio_device *adev = out->dev;

    if (adev->mode == AUDIO_MODE_IN_CALL)
    {
        ALOGW("call mode, do not start stream");
        return 0;
    }

    property_set(PROP_RAWDATA_OUTPUT, PROP_RAWDATA_OUTPUT_ON);
    adev->raw_enable = true;
    //out->ext_config = pcm_config_rawdata;

    property_get(PROP_RAWDATA_KEY, prop_value, PROP_RAWDATA_DEFAULT_VALUE);

    if (!strcmp(prop_value, PROP_RAWDATA_MODE_HDMI_RAW))
    {
        index = AUDIO_CARD_HDMI;
        card = adev->card_ahub;
        port = PORT_HDMI;
    }
    else if (!strcmp(prop_value, PROP_RAWDATA_MODE_SPDIF_RAW))
    {
        index = AUDIO_CARD_SPDIF;
        card = adev->card_spdif;
        port = 0;
    }
    else
    {
        ALOGE("rawdata output mode is invalid.");
        index = AUDIO_CARD_SPDIF;
        card = adev->card_spdif;
        port = 0;
        ALOGD("default use spdif");
    }

    if (card < 0)
    {
        ALOGE("cannot find raw data output card.");
        return -ENOMEM;
    }

    if (index == AUDIO_CARD_HDMI)
    {
        out->ext_sub_pcm = pcm_open(adev->card_hdmi, 0, PCM_OUT, &out->ext_config);
        if (!pcm_is_ready(out->ext_sub_pcm))
        {
            ALOGE("cannot open pcm driver: %s", pcm_get_error(out->ext_sub_pcm));
            pcm_close(out->ext_sub_pcm);
            out->ext_sub_pcm = NULL;
            adev->active_output = NULL;
            return -ENOMEM;
        }
        pcm_start(out->ext_sub_pcm);
    }

    out->ext_pcm = pcm_open(card, port, PCM_OUT, &out->ext_config);
    if (!pcm_is_ready(out->ext_pcm))
    {
        ALOGE("cannot open pcm driver: %s", pcm_get_error(out->ext_pcm));
        pcm_close(out->ext_pcm);
        out->ext_pcm = NULL;
        adev->active_output = NULL;
        return -ENOMEM;
    }
    ALOGD("start_rawdata_output_stream");
    return 0;
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
    struct sunxi_audio_device *adev = out->dev;

    if (!out->standby)
    {
        if (adev->raw_enable)
        {
            property_set(PROP_RAWDATA_OUTPUT, PROP_RAWDATA_OUTPUT_OFF);
            adev->raw_enable = false;
        }
        if (out->ext_pcm)
        {
            pcm_close(out->ext_pcm);
            out->ext_pcm = NULL;
        }

        if (out->ext_sub_pcm)
        {
            pcm_close(out->ext_sub_pcm);
            out->ext_sub_pcm = NULL;
        }

        if (out->ext_resampler)
        {
            release_resampler(out->ext_resampler);
            out->ext_resampler = NULL;
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
    ALOGV("out_set_parameters: %s", kvpairs);
    return 0;
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
typedef enum IEC61937DataType
{
    IEC61937_AC3 = 0x01,                       ///< AC-3 data
    IEC61937_MPEG1_LAYER1 = 0x04,              ///< MPEG-1 layer 1
    IEC61937_MPEG1_LAYER23 = 0x05,             ///< MPEG-1 layer 2 or 3 data or MPEG-2 without extension
    IEC61937_MPEG2_EXT = 0x06,                 ///< MPEG-2 data with extension
    IEC61937_MPEG2_AAC = 0x07,                 ///< MPEG-2 AAC ADTS
    IEC61937_MPEG2_LAYER1_LSF = 0x08,          ///< MPEG-2, layer-1 low sampling frequency
    IEC61937_MPEG2_LAYER2_LSF = 0x09,          ///< MPEG-2, layer-2 low sampling frequency
    IEC61937_MPEG2_LAYER3_LSF = 0x0A,          ///< MPEG-2, layer-3 low sampling frequency
    IEC61937_DTS1 = 0x0B,                      ///< DTS type I   (512 samples)
    IEC61937_DTS2 = 0x0C,                      ///< DTS type II  (1024 samples)
    IEC61937_DTS3 = 0x0D,                      ///< DTS type III (2048 samples)
    IEC61937_ATRAC = 0x0E,                     ///< ATRAC data
    IEC61937_ATRAC3 = 0x0F,                    ///< ATRAC3 data
    IEC61937_ATRACX = 0x10,                    ///< ATRAC3+ data
    IEC61937_DTSHD = 0x11,                     ///< DTS HD data
    IEC61937_WMAPRO = 0x12,                    ///< WMA 9 Professional data
    IEC61937_MPEG2_AAC_LSF_2048 = 0x13,        ///< MPEG-2 AAC ADTS half-rate low sampling frequency
    IEC61937_MPEG2_AAC_LSF_4096 = 0x13 | 0x20, ///< MPEG-2 AAC ADTS quarter-rate low sampling frequency
    IEC61937_EAC3 = 0x15,                      ///< E-AC-3 data
    IEC61937_TRUEHD = 0x16,                    ///< TrueHD data
} IEC61937DataType;

static bool detectIEC61937(struct sunxi_stream_out *out, const void *audio_buf, size_t bytes)
{
    unsigned int tansfer_chanels = 2;
    unsigned int sample_rate = 48000;
    audio_format_t format = AUDIO_FORMAT_PCM;
    if (bytes < 6)
    {
        return false;
    }
    char *buf = (char *)audio_buf;
    unsigned int i = 0;
    while (i < (bytes - 3))
    {
        if (buf[i] == 0x72 && buf[i + 1] == 0xF8 && buf[i + 2] == 0x1F && buf[i + 3] == 0x4E)
        {
            ALOGV("Detect a iec header!!, byte : %zu", bytes);
            break;
        }
        i++;
    }
    if (i == (bytes - 3))
        return false;

    char type = buf[i + 4] & 0xff;
    switch (type)
    {
    case IEC61937_AC3:
        sample_rate = 48000;
        format = AUDIO_FORMAT_AC3;
        break;
    case IEC61937_DTS1:
    case IEC61937_DTS2:
    case IEC61937_DTS3:
        sample_rate = 48000;
        format = AUDIO_FORMAT_DTS;
        break;
    case IEC61937_EAC3:
        sample_rate = 192000;
        out->format = AUDIO_FORMAT_E_AC3;
        break;
    case IEC61937_DTSHD:
        sample_rate = 192000;
        tansfer_chanels = 8;
        format = AUDIO_FORMAT_DTS_HD;
        break;
    case IEC61937_TRUEHD:
        sample_rate = 192000;
        tansfer_chanels = 8;
        format = AUDIO_FORMAT_DTS_HD;
        break;
    default:
        break;
    }
    //out->config.rate = sample_rate;
    //out->config.channels = tansfer_chanels;
    ALOGV("rate : %d, channels : %d", sample_rate, tansfer_chanels);
    return true;
}

static ssize_t out_write(struct audio_stream_out *stream, const void *buffer,
                         size_t bytes)
{
    int ret = 0;
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    size_t frame_size = audio_stream_out_frame_size(&out->stream);
    size_t out_frames = bytes / frame_size;
    void *buf;
    char prop_value_card[PROPERTY_VALUE_MAX] = {0};

    if (adev->mode == AUDIO_MODE_IN_CALL)
    {
        ALOGD("mode in call, do not out_write");
        int time = bytes * 1000 * 1000 / 4 / DEFAULT_SAMPLING_RATE;
        usleep(time);
        return bytes;
    }

    if (out->standby)
    {
        bool bok = detectIEC61937(out, buffer, bytes);
        if (bok)
        {
            adev->raw_flag = true;
            ret = start_output_stream(out);
            if (ret != 0)
            {
                goto exit;
            }
            out->standby = 0;
        }
    }

    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the output stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    pthread_mutex_lock(&out->lock);

    pcm_set_avail_min(out->ext_pcm, out->ext_config.avail_min);

    buf = (void *)buffer;

    property_get(PROP_RAWDATA_KEY, prop_value_card, PROP_RAWDATA_DEFAULT_VALUE);

    if (!strcmp(prop_value_card, PROP_RAWDATA_MODE_HDMI_RAW))
    {
        //add by khan : for 24bit pcm, we should extend the highest 8 bit with signed
        if (out->format == AUDIO_FORMAT_PCM_24_BIT_PACKED)
        {
            int idx = 0, jdx = 0;
            char *dst = NULL;
            char *src = NULL;
            char *buf_conv = NULL;
            int ch = out->ext_config.channels;
            int write_len = out_frames * frame_size;
            int buf_conv_len = 0;
            int samples = write_len / audio_bytes_per_sample(out->format) / ch;
            buf_conv_len = write_len * 4 / 3;
            buf_conv = (char *)malloc(buf_conv_len);
            if (buf_conv == NULL)
            {
                ALOGE("No memory while malloc conventer buffer...");
                pthread_mutex_unlock(&out->lock);
                return 0;
            }

            dst = (char *)buf_conv;
            for (idx = 0; idx < ch; idx++)
            {
                dst = ((char *)buf_conv) + 4 * idx;
                src = ((char *)buf) + 3 * idx;
                for (jdx = 0; jdx < samples; jdx++)
                {
                    dst[0] = src[0] & 0xff;
                    dst[1] = src[1] & 0xff;
                    dst[2] = src[2] & 0xff;
                    dst[3] = (dst[2] & 0x80) ? 0xff : 0x00;
                    dst += 4 * ch;
                    src += 3 * ch;
                }
            }
            ret = pcm_write(out->ext_pcm, (void *)buf_conv, buf_conv_len);
            if (buf_conv != NULL)
            {
                free(buf_conv);
                buf_conv = NULL;
            }
        }
        else
        {
            ret = pcm_write(out->ext_pcm, (void *)buf, out_frames * frame_size);
        }
    }
    else if (!strcmp(prop_value_card, PROP_RAWDATA_MODE_SPDIF_RAW))
    {
        if (out->ext_config.channels == 2)
        {
            if (out->format == AUDIO_FORMAT_PCM_24_BIT_PACKED)
            {
                int idx = 0, jdx = 0;
                char *dst = NULL;
                char *src = NULL;
                char *buf_conv = NULL;
                int ch = out->ext_config.channels;
                int write_len = out_frames * frame_size;
                int buf_conv_len = 0;
                int samples = write_len / audio_bytes_per_sample(out->format) / ch;
                buf_conv_len = write_len * 4 / 3;

                buf_conv = (char *)malloc(buf_conv_len);
                if (buf_conv == NULL)
                {
                    ALOGE("No memory while malloc conventer buffer...");
                    pthread_mutex_unlock(&out->lock);
                    return 0;
                }

                dst = (char *)buf_conv;
                for (idx = 0; idx < ch; idx++)
                {
                    dst = ((char *)buf_conv) + 4 * idx;
                    src = ((char *)buf) + 3 * idx;
                    for (jdx = 0; jdx < samples; jdx++)
                    {
                        dst[0] = src[0] & 0xff;
                        dst[1] = src[1] & 0xff;
                        dst[2] = src[2] & 0xff;
                        dst[3] = (dst[2] & 0x80) ? 0xff : 0x00;
                        dst += 4 * ch;
                        src += 3 * ch;
                    }
                }
                ret = pcm_write(out->ext_pcm, (void *)buf_conv, buf_conv_len);
                if (buf_conv != NULL)
                {
                    free(buf_conv);
                    buf_conv = NULL;
                }
            }
            else
            {
                ret = pcm_write(out->ext_pcm, (void *)buf, out_frames * frame_size);
            }
        }
    }
    if (ret != 0)
    {
        ALOGE("##############out_write()  Warning:write fail, reopen it ret = %d #######################", ret);
        do_output_standby(out);
        usleep(30000);
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

    if (flags & AUDIO_OUTPUT_FLAG_DIRECT)
    {
        if (config->sample_rate == 0)
            config->sample_rate = DEFAULT_SAMPLING_RATE;
        if (config->channel_mask == 0)
            config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;

        out->format = config->format;
        out->ext_config = pcm_config_rawdata;
        out->ext_config.rate = config->sample_rate;
        out->ext_config.channels = popcount(config->channel_mask);
        out->ext_config.period_size = AUX_DIGITAL_MULTI_PERIOD_BYTES / (out->ext_config.channels * 2);
    }
    else
    {
        out->ext_config = pcm_config_out;
    }

    out->flags = flags;
    config->format = out_get_format(&out->stream.common);
    config->channel_mask = out_get_channels(&out->stream.common);
    config->sample_rate = out_get_sample_rate(&out->stream.common);

    *stream_out = &out->stream;
    ladev->active_output = out;

    return 0;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    UNUSED(dev);

    out_standby(&stream->common);

    if (out->buffer)
        free(out->buffer);

    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    UNUSED(dev);
    UNUSED(kvpairs);
    return 0;
}

static char *adev_get_parameters(const struct audio_hw_device *dev,
                                 const char *keys)
{
    UNUSED(dev);
    ALOGV("adev_get_parameters, %s", keys);

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
    F_LOG;
    UNUSED(dev);
    UNUSED(mode);
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
    UNUSED(config);
    return 0;
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
    UNUSED(dev);
    UNUSED(handle);
    UNUSED(devices);
    UNUSED(flags);
    UNUSED(config);
    UNUSED(stream_in);
    UNUSED(address);
    UNUSED(source);
    return 0;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                    struct audio_stream_in *stream)
{
    UNUSED(dev);
    UNUSED(stream);
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
    free(device);

    return 0;
}

static int adev_open(const hw_module_t *module, const char *name,
                     hw_device_t **device)
{
    struct sunxi_audio_device *adev;

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

    /* Set the default route before the PCM stream is opened */
    pthread_mutex_lock(&adev->lock);
    adev->mode = AUDIO_MODE_NORMAL;
    adev->card_hdmi = find_card_id_by_name("sndhdmi");
    adev->card_spdif = find_card_id_by_name("sndspdif");
    adev->card_ahub = find_card_id_by_name("sndahub");

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

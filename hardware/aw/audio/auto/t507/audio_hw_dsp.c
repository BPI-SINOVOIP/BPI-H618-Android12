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

#define LOG_TAG "audio_hw_dsp"
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
#undef DSP_DUMP_PCM_DATA_ENABLE
#undef TEST_I2S2

typedef struct extrnal_map_t
{
    char name[32];
    int index;
    int card_id;
    int card_port;
} extrnal_map;

enum EXTERNAL_AUDIO_CARDS
{
    CARD_NULL = -1,
    CARD_I2S1 = 0,
    CARD_I2S2,
    CARD_I2S3,
    CARD_AHUB,
    CARD_HDMI,
    CARD_SPDIF,
    CARD_CNT,
};

//the order must be kept in sync with EXTERNAL_AUDIO_CARDS
static extrnal_map extrnal_audio_map[CARD_CNT] = {
    {"snddaudio2", AUDIO_CARD_I2S2, -1, PORT_I2S2},
    {"snddaudio3", AUDIO_CARD_I2S3, -1, PORT_I2S3},
    {"sndahub", AUDIO_CARD_AHUB, -1, 0},
    {"sndhdmi", AUDIO_CARD_HDMI, -1, PORT_HDMI},
    {"sndspdif", AUDIO_CARD_SPDIF, -1, 0},
};

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

struct pcm_config ext_pcm_config_out = {
    .channels = DEFAULT_CHANNEL_COUNT,
    .rate = DEFAULT_SAMPLING_RATE,
    .period_size = DEFAULT_OUTPUT_PERIOD_SIZE,
    .period_count = DEFAULT_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

static int find_dsp_audio_map(char *name, int id)
{
    int i = 0;
    for (i = 0; i < CARD_CNT; i++)
    {
        if (!strcmp(name, extrnal_audio_map[i].name))
        {
            extrnal_audio_map[i].card_id = id;
            return 0;
        }
    }
    return -1;
}

static int init_dsp_audio_devices()
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
            if (find_dsp_audio_map(name, i) < 0)
            {
                ALOGW("dsp card %s is not supported.", name);
            }
        }
    }
    return 0;
}

static int get_card_port_id(int index, int *id, int *port)
{
    int i = 0;

    for (i = 0; i < CARD_CNT; i++)
    {
        if (extrnal_audio_map[i].index == index)
        {
            *id = extrnal_audio_map[i].card_id;
            *port = extrnal_audio_map[i].card_port;
            return 0;
        }
    }

    ALOGE("can not find card id & port for card index %d", index);
    return -1;
}

static int is_ahub_device(int card)
{
    if ((card & AUDIO_CARD_I2S2) || (card & AUDIO_CARD_I2S3) || (card & AUDIO_CARD_HDMI))
        return 1;

    return 0;
}

/* must be called with hw device and output stream mutexes locked */
static int start_output_stream(struct sunxi_stream_out *out)
{
    struct sunxi_audio_device *adev = out->dev;
    int card = 0;
    int port = 0;
    int ret;

    adev->active_output = out;

    if (get_card_port_id(DSP_OUTPUT_ACTIVE_CARD, &card, &port) < 0)
    {
        ALOGE("can not find dsp card %d", DSP_OUTPUT_ACTIVE_CARD);
        return -1;
    }
    else
    {
        ALOGD("get dsp card id : %d", card);
    }

    //ALOGV("use %s to playback audio", adev->dev_manager[index].name);
    out->ext_config = ext_pcm_config_out;
    out->ext_config.start_threshold = DEFAULT_OUTPUT_PERIOD_SIZE * 2;

    if (is_ahub_device(DSP_OUTPUT_ACTIVE_CARD))
    {
        out->ext_sub_pcm = pcm_open(card, 0, PCM_OUT, &out->ext_config);
        if (!pcm_is_ready(out->ext_sub_pcm))
        {
            ALOGE("cannot open pcm driver: %s", pcm_get_error(out->ext_sub_pcm));
            pcm_close(out->ext_sub_pcm);
            out->ext_sub_pcm = NULL;
            adev->active_output = NULL;
            return -ENOMEM;
        }
        pcm_prepare(out->ext_sub_pcm);
        int temp = 0;
        get_card_port_id(AUDIO_CARD_AHUB, &card, &temp);
    }

#ifdef DSP_DUMP_PCM_DATA_ENABLE
    out->dump_pcm = fopen("/data/vendor/hardware/audio_d/audio_hw_dsp_write.pcm", "wb");
    if (!out->dump_pcm)
    {
        ALOGE("cannot fopen dump.");
    }
    else
    {
        ALOGE("dump_pcm:%p", out->dump_pcm);
    }
#endif
    ALOGD("start open card[%d:%d]", card, port);
    out->ext_pcm = pcm_open(card, port, PCM_OUT, &out->ext_config);
    if (!pcm_is_ready(out->ext_pcm))
    {
        ALOGE("cannot open pcm driver: %s", pcm_get_error(out->ext_pcm));
        pcm_close(out->ext_pcm);
        out->ext_pcm = NULL;
        adev->active_output = NULL;
        return -ENOMEM;
    }

    if (DEFAULT_SAMPLING_RATE != out->ext_config.rate)
    {
        ret = create_resampler(DEFAULT_SAMPLING_RATE,
                               out->ext_config.rate,
                               2,
                               RESAMPLER_QUALITY_DEFAULT,
                               NULL,
                               &out->ext_resampler);
        if (ret != 0)
        {
            ALOGE("create out resampler failed, %d -> %d", DEFAULT_SAMPLING_RATE, out->ext_config.rate);
            return ret;
        }

        ALOGV("create out resampler OK, %d -> %d", DEFAULT_SAMPLING_RATE, out->ext_config.rate);
    }
    else
        ALOGV("play audio with %d Hz serial sample rate.", DEFAULT_SAMPLING_RATE);

    if (out->ext_resampler)
    {
        out->ext_resampler->reset(out->ext_resampler);
    }

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
    if (!out->standby)
    {
        if (out->ext_pcm)
        {
            pcm_close(out->ext_pcm);
            out->ext_pcm = NULL;
        }

#ifdef DSP_DUMP_PCM_DATA_ENABLE
        if (out->dump_pcm)
        {
            fclose(out->dump_pcm);
            out->dump_pcm = NULL;
        }
#endif

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

static ssize_t out_write(struct audio_stream_out *stream, const void *buffer,
                         size_t bytes)
{
    int ret = 0;
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    size_t frame_size = audio_stream_out_frame_size(&out->stream);
    size_t in_frames = bytes / frame_size;
    size_t out_frames = RESAMPLER_BUFFER_SIZE / frame_size;
    void *buf;
    char prop_value[PROPERTY_VALUE_MAX] = {0};

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

    if (out->ext_resampler)
    {
        out->ext_resampler->resample_from_input(out->ext_resampler,
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

    if (out->ext_config.channels == 2)
    {
        ret = pcm_write(out->ext_pcm, (void *)buf, out_frames * frame_size);
#ifdef DSP_DUMP_PCM_DATA_ENABLE
        if (out->dump_pcm)
            fwrite(buf, 1, out_frames * frame_size, out->dump_pcm);
#endif
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
        ret = pcm_write(out->ext_pcm, (void *)buf, out_frames * frame_size / 2);
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
    out->ext_config = ext_pcm_config_out;
    out->flags = flags;
    config->format = out_get_format(&out->stream.common);
    config->channel_mask = out_get_channels(&out->stream.common);
    config->sample_rate = out_get_sample_rate(&out->stream.common);

    *stream_out = &out->stream;
    ladev->active_output = out;

    ALOGV("+++++++++++++++ adev_open_output_stream: req_sample_rate: %d, fmt: %x, channel_count: %d",
          config->sample_rate, config->format, config->channel_mask);

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

struct pcm_config pcm_config_in = {
    .channels = DEFAULT_CHANNEL_COUNT,
    .rate = DEFAULT_SAMPLING_RATE,
    .period_size = DEFAULT_INPUT_PERIOD_SIZE,
    .period_count = DEFAULT_INPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

/* must be called with hw device and input stream mutexes locked */
static int start_input_stream(struct sunxi_stream_in *in)
{
    struct sunxi_audio_device *adev = in->dev;
    int card = 0;
    int port = 0;

    adev->active_input = in;

    if (get_card_port_id(DSP_INPUT_ACTIVE_CARD, &card, &port) < 0)
    {
        ALOGE("can not find dsp card %d", DSP_INPUT_ACTIVE_CARD);
        return -1;
    }
    else
    {
        ALOGD("get dsp card id : %d", card);
    }

    //ALOGV("use %s to playback audio", adev->dev_manager[index].name);
    in->ext_config = ext_pcm_config_out;
    in->ext_config.start_threshold = DEFAULT_OUTPUT_PERIOD_SIZE * 2;

    if (is_ahub_device(DSP_INPUT_ACTIVE_CARD))
    {
        ALOGE("start open card[%d:0]", card);
        in->ext_sub_pcm = pcm_open(card, 0, PCM_IN, &in->ext_config);
        if (!pcm_is_ready(in->ext_sub_pcm))
        {
            ALOGE("cannot open pcm driver: %s", pcm_get_error(in->ext_sub_pcm));
            pcm_close(in->ext_sub_pcm);
            in->ext_sub_pcm = NULL;
            adev->active_input = NULL;
            return -ENOMEM;
        }
        pcm_prepare(in->ext_sub_pcm);
        int temp = 0;
        get_card_port_id(AUDIO_CARD_AHUB, &card, &temp);
    }

#ifdef DSP_DUMP_PCM_DATA_ENABLE
    in->dump_pcm = fopen("/data/vendor/hardware/audio_d/audio_hw_dsp_read.pcm", "wb");
    if (!in->dump_pcm)
    {
        ALOGE("cannot fopen dump.");
    }
    else
    {
        ALOGE("dump_pcm:%p", in->dump_pcm);
    }
#endif
    ALOGE("start open card[%d:%d]", card, port);
    in->ext_pcm = pcm_open(card, port, PCM_IN, &in->ext_config);
    if (!pcm_is_ready(in->ext_pcm))
    {
        ALOGE("cannot open pcm driver: %s", pcm_get_error(in->ext_pcm));
        pcm_close(in->ext_pcm);
        in->ext_pcm = NULL;
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

static void select_input_device(struct sunxi_audio_device *adev)
{
    ALOGD("line:%d,%s,adev->mode:%d", __LINE__, __FUNCTION__, adev->mode);
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
        if (in->ext_pcm)
        {
            pcm_close(in->ext_pcm);
            in->ext_pcm = NULL;
        }

#ifdef DSP_DUMP_PCM_DATA_ENABLE
        if (in->dump_pcm)
        {
            fclose(in->dump_pcm);
            in->dump_pcm = NULL;
        }
#endif

        if (in->ext_sub_pcm)
        {
            pcm_close(in->ext_sub_pcm);
            in->ext_sub_pcm = NULL;
        }

#if USE_INPUT_RESAMPLER
        if (in->resampler)
        {
            release_resampler(in->resampler);
            in->resampler = NULL;
        }
#endif
        adev->active_input = 0;

        in->standby = 1;
    }
    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    int status;

    pthread_mutex_lock(&in->lock);
    status = do_input_standby(in);
    pthread_mutex_unlock(&in->lock);
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
        ret = pcm_read(in->ext_pcm, buffer, bytes);
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

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    UNUSED(dev);
    int channel_count = popcount(config->channel_mask);
    if (check_input_parameters(config->sample_rate, config->format, channel_count) != 0)
        return 0;

    return get_input_buffer_size(config->sample_rate, config->format, channel_count);

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

    ALOGE("dsp adev_open_input_stream, flags: %x", flags);

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

    ALOGE("dsp to malloc in-buffer: period_size: %d, frame_size: %zu",
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
    return 0;
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

    ALOGD("dsp adev_close_input_stream set voice record status");
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

#ifdef TEST_I2S2
void enable_ahub_daudio2(struct mixer *mixer)
{
    struct mixer_ctl *ctl;
    //ctl = mixer_get_ctl_by_name(mixer, "I2S2 SD0 to SDI0 Loopback Debug");
    //mixer_ctl_set_value(ctl, 0, 1);
    ctl = mixer_get_ctl_by_name(mixer, "I2S2IN Switch");
    mixer_ctl_set_value(ctl, 0, 1);
    ctl = mixer_get_ctl_by_name(mixer, "I2S2OUT Switch");
    mixer_ctl_set_value(ctl, 0, 1);
    ctl = mixer_get_ctl_by_name(mixer, "APBIF2 Src Select");
    mixer_ctl_set_value(ctl, 0, 6);
    ctl = mixer_get_ctl_by_name(mixer, "I2S2 Src Select");
    mixer_ctl_set_value(ctl, 0, 3);
}
#endif

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
    init_dsp_audio_devices();

    int card = -1;
    int port = -1;
    struct mixer *mixer;
    get_card_port_id(AUDIO_CARD_AHUB, &card, &port);
    mixer = mixer_open(card);
#ifdef TEST_I2S2
    enable_ahub_daudio2(mixer);
#endif
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

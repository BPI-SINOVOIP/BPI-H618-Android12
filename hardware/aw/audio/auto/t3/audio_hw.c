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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/resource.h>
#include <sys/prctl.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <cutils/atomic.h>
#include <cutils/sched_policy.h>
#include <cutils/list.h>
#include <unistd.h>
#include <hardware/audio.h>
#include <audio_route/audio_route.h>
#include <audio_utils/resampler.h>
#include "tinyalsa/asoundlib.h"
#include "audio_hw.h"
#include "platform.h"
#define UNUSED(x) (void)(x)

#define USE_RESAMPLER 1

/* debug flag */
int AUDIO_HAL_DEBUG = 0;
inline int update_debug_flag()
{
    char val[PROPERTY_VALUE_MAX];
    property_get("vendor.debug.audio.hal_debug", val, "0");
    if (!strcmp(val, "0")) {
        AUDIO_HAL_DEBUG = 0;
    } else {
        AUDIO_HAL_DEBUG = 1;
    }

    return AUDIO_HAL_DEBUG;
}

/* audio debug log */
#ifndef ADLOG
#define ADLOG(...) \
    ALOGD_IF(AUDIO_HAL_DEBUG, __VA_ARGS__)
#endif

/* changed when the stream is opened */
struct pcm_config out_pcm_config = {
    .channels = DEFAULT_CHANNEL_COUNT,
    .rate = DEFAULT_OUTPUT_SAMPLING_RATE,
    .period_size = DEFAULT_OUTPUT_PERIOD_SIZE,
    .period_count = DEFAULT_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

/* changed when the stream is opened */
struct pcm_config in_pcm_config = {
    .channels = DEFAULT_CHANNEL_COUNT,
    .rate = DEFAULT_INPUT_SAMPLING_RATE,
    .period_size = DEFAULT_INPUT_PERIOD_SIZE,
    .period_count = DEFAULT_INPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

#define STRING_TO_ENUM(string) { #string, string }

struct string_to_enum {
    const char *name;
    uint32_t value;
};

static const struct string_to_enum channels_name_to_enum_table[] = {
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_STEREO),
    STRING_TO_ENUM(AUDIO_CHANNEL_IN_MONO),
    STRING_TO_ENUM(AUDIO_CHANNEL_IN_STEREO),
    STRING_TO_ENUM(AUDIO_CHANNEL_IN_FRONT_BACK),
};

static int do_out_standby(struct sunxi_stream_out *out);
static int do_in_standby(struct sunxi_stream_in *in);

#ifdef USE_RESAMPLER
struct resampler_itfe *in_resampler;
struct resampler_itfe *out_resampler;
struct resampler_buffer_provider in_buf_provider;
void *out_buffer;
void *in_buffer;
struct sunxi_stream_in *resamp_stream_in;
size_t in_frames_in;

static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer* buffer)
{

    struct sunxi_stream_in *in = resamp_stream_in;
    size_t frame_size = audio_stream_in_frame_size(&in->stream);
    int read_status = 0;

    if (buffer_provider == NULL || buffer == NULL)
        return -EINVAL;

    if (in->pcm == NULL) {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        return -ENODEV;
    }

    if (in_frames_in == 0) {
        read_status = pcm_read(in->pcm, in_buffer,
                                   in->config.period_size * frame_size);
        if (read_status) {
            ALOGE("get_next_buffer() pcm_read error %d, %s",
                  read_status, strerror(errno));
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return read_status;
        }
        in_frames_in = in->config.period_size;
    }
    buffer->frame_count = (buffer->frame_count > in_frames_in) ?
                          in_frames_in : buffer->frame_count;
    buffer->i16 = (short*)in_buffer +
                  (in->config.period_size - in_frames_in) *
                  in->config.channels;

    return read_status;
}

static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer* buffer)
{
    UNUSED(buffer_provider);
    if (buffer == NULL)
        return;

    in_frames_in -= buffer->frame_count;
}

/* read_frames() reads frames from kernel driver, down samples to capture rate
 * if necessary and output the number of frames requested to the buffer specified
 */
static ssize_t read_frames(struct sunxi_stream_in *in, void *buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;
    size_t frame_size = audio_stream_in_frame_size(&in->stream);
    int read_status = 0;

    resamp_stream_in = in;

    while (frames_wr < frames) {
        size_t frames_rd = frames - frames_wr;
        if (in_resampler) {
            in_resampler->resample_from_provider(in_resampler,
                                                 (int16_t *)((char *)buffer +
                                                 frames_wr * frame_size),
                                                 &frames_rd);
        } else {
            struct resampler_buffer buf = {
                { .raw = NULL, },
                .frame_count = frames_rd,
            };
            read_status = get_next_buffer(&in_buf_provider, &buf);
            if (buf.raw != NULL) {
                memcpy((char *)buffer + frames_wr * frame_size, buf.raw,
                       buf.frame_count * frame_size);
                frames_rd = buf.frame_count;
            }
            release_buffer(&in_buf_provider, &buf);
        }

        /* in->read_status is updated by getNextBuffer() also called by
         * in->resampler->resample_from_provider()
         */
        if (read_status)
            return read_status;
        frames_wr += frames_rd;
    }
    return frames_wr;
}

#endif //USE_RESAMPLER

static inline void print_sunxi_audio_device(const struct sunxi_audio_device *adev)
{
    if (!adev) {
        ADLOG("can't print sunxi_audio_device:adev == NULL");
        return;
    }

    ADLOG("print sunxi_audio_device:\n"
          "active_input:%p\n"
          "active_output:%p\n"
          "out_devices:%#x\n"
          "in_devices:%#x\n"
          "platform:%p\n"
          "mode:%#x\n"
          "mic_muted:%d\n",
          adev->active_input,
          adev->active_output,
          adev->out_devices,
          adev->in_devices,
          adev->platform,
          adev->mode,
          adev->mic_muted);
}

static inline void print_sunxi_stream_out(const struct sunxi_stream_out *out)
{
    if (!out) {
        ADLOG("can't print sunxi_stream_out:out == NULL");
        return;
    }

    ADLOG("print sunxi_stream_out:\n"
          "standby:%d\n"
          "sample_rate:%d\n"
          "channel_mask:%#x\n"
          "format:%#x\n"
          "devices:%#x\n"
          "flags:%#x\n"
          "muted:%d\n"
          "card:%d\n"
          "port:%d\n"
          "pcm:%p\n"
          "dev:%p\n",
          out->standby,
          out->sample_rate,
          out->channel_mask,
          out->format,
          out->devices,
          out->flags,
          out->muted,
          out->card,
          out->port,
          out->pcm,
          out->dev);

    ADLOG("print out config:\n"
          "channels:%d\n"
          "rate:%d\n"
          "period_size:%d\n"
          "period_count:%d\n"
          "format:%#x\n",
          (int)out->config.channels,
          (int)out->config.rate,
          (int)out->config.period_size,
          (int)out->config.period_count,
          out->config.format);
}

static inline void print_sunxi_stream_in(const struct sunxi_stream_in *in)
{
    if (!in) {
        ADLOG("can't print sunxi_stream_in:in == NULL");
        return;
    }

    ADLOG("print sunxi_stream_in:\n"
          "standby:%d\n"
          "sample_rate:%d\n"
          "channel_mask:%#x\n"
          "format:%#x\n"
          "devices:%#x\n"
          "flags:%#x\n"
          "muted:%d\n"
          "card:%d\n"
          "port:%d\n"
          "pcm:%p\n"
          "dev:%p\n",
          in->standby,
          in->sample_rate,
          in->channel_mask,
          in->format,
          in->devices,
          in->flags,
          in->muted,
          in->card,
          in->port,
          in->pcm,
          in->dev);

    ADLOG("print in config:\n"
          "channels:%d\n"
          "rate:%d\n"
          "period_size:%d\n"
          "period_count:%d\n"
          "format:%#x\n",
          (int)in->config.channels,
          (int)in->config.rate,
          (int)in->config.period_size,
          (int)in->config.period_count,
          in->config.format);
}


/** Add a value in a list if not already present.
 * @return true if value was successfully inserted or already present,
 *         false if the list is full and does not contain the value.
 */
static bool register_uint(uint32_t value, uint32_t* list, size_t list_length) {
    for (size_t i = 0; i < list_length; i++) {
        if (list[i] == value) return true; // value is already present
        if (list[i] == 0) { // no values in this slot
            list[i] = value;
            return true; // value inserted
        }
    }
    return false; // could not insert value
}

/** Add channel_mask in supported_channel_masks if not already present.
 * @return true if channel_mask was successfully inserted or already present,
 *         false if supported_channel_masks is full and does not contain channel_mask.
 */
static void register_channel_mask(audio_channel_mask_t channel_mask,
            audio_channel_mask_t supported_channel_masks[static MAX_SUPPORTED_CHANNEL_MASKS]) {
    ALOGE_IF(!register_uint(channel_mask, supported_channel_masks, MAX_SUPPORTED_CHANNEL_MASKS),
        "%s: stream can not declare supporting its channel_mask %x", __func__, channel_mask);
}

/** Add format in supported_formats if not already present.
 * @return true if format was successfully inserted or already present,
 *         false if supported_formats is full and does not contain format.
 */
static void register_format(audio_format_t format,
            audio_format_t supported_formats[static MAX_SUPPORTED_FORMATS]) {
    ALOGE_IF(!register_uint(format, supported_formats, MAX_SUPPORTED_FORMATS),
             "%s: stream can not declare supporting its format %x", __func__, format);
}
/** Add sample_rate in supported_sample_rates if not already present.
 * @return true if sample_rate was successfully inserted or already present,
 *         false if supported_sample_rates is full and does not contain sample_rate.
 */
static void register_sample_rate(uint32_t sample_rate,
            uint32_t supported_sample_rates[static MAX_SUPPORTED_SAMPLE_RATES]) {
    ALOGE_IF(!register_uint(sample_rate, supported_sample_rates, MAX_SUPPORTED_SAMPLE_RATES),
             "%s: stream can not declare supporting its sample rate %x", __func__, sample_rate);
}

static bool stream_get_parameter_channels(struct str_parms *query,
                                          struct str_parms *reply,
                                          audio_channel_mask_t *supported_channel_masks) {
    int ret = -1;
    char value[256];
    bool first = true;
    size_t i, j;

    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
        ret = 0;
        value[0] = '\0';
        i = 0;
        while (supported_channel_masks[i] != 0) {
            for (j = 0; j < ARRAY_SIZE(channels_name_to_enum_table); j++) {
                if (channels_name_to_enum_table[j].value == supported_channel_masks[i]) {
                    if (!first) {
                        strcat(value, "|");
                    }
                    strcat(value, channels_name_to_enum_table[j].name);
                    first = false;
                    break;
                }
            }
            i++;
        }
        str_parms_add_str(reply, AUDIO_PARAMETER_STREAM_SUP_CHANNELS, value);
    }
    return ret >= 0;
}

static bool stream_get_parameter_formats(struct str_parms *query,
                                         struct str_parms *reply,
                                         audio_format_t *supported_formats) {
    int ret = -1;
    char value[256];
    int i;

    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
        ret = 0;
        value[0] = '\0';
        switch (supported_formats[0]) {
            case AUDIO_FORMAT_PCM_16_BIT:
                strcat(value, "AUDIO_FORMAT_PCM_16_BIT");
                break;
            default:
                ALOGE("%s: unsupported format %#x", __func__,
                      supported_formats[0]);
                break;
        }
        str_parms_add_str(reply, AUDIO_PARAMETER_STREAM_SUP_FORMATS, value);
    }
    return ret >= 0;
}

static bool stream_get_parameter_rates(struct str_parms *query,
                                       struct str_parms *reply,
                                       uint32_t *supported_sample_rates) {

    int i;
    char value[256];
    int ret = -1;
    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
        ret = 0;
        value[0] = '\0';
        i=0;
        int cursor = 0;
        while (supported_sample_rates[i]) {
            int avail = sizeof(value) - cursor;
            ret = snprintf(value + cursor, avail, "%s%d",
                           cursor > 0 ? "|" : "",
                           supported_sample_rates[i]);
            if (ret < 0 || ret >= avail) {
                // if cursor is at the last element of the array
                //    overwrite with \0 is duplicate work as
                //    snprintf already put a \0 in place.
                // else
                //    we had space to write the '|' at value[cursor]
                //    (which will be overwritten) or no space to fill
                //    the first element (=> cursor == 0)
                value[cursor] = '\0';
                break;
            }
            cursor += ret;
            ++i;
        }
        str_parms_add_str(reply, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES,
                          value);
    }
    return ret >= 0;
}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    ALOGV("out_set_sample_rate: %d", out->sample_rate);

    return out->sample_rate;
}

static void out_update_voice_sample_rate(const struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)out->dev;

    unsigned int old_rate = out->config.rate;
    out->config.rate = DEFAULT_OUTPUT_VOICE_SAMPLING_RATE;
    update_platform_snd_card_config(out->config, OUT_SPK, adev->platform);
    ALOGD("out_update_voice_sample_rate: %d -> %d", old_rate, out->config.rate);
    return;
}

static void out_reset_voice_sample_rate(const struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)out->dev;

    unsigned int old_rate = out->config.rate;
    out->config.rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    update_platform_snd_card_config(out->config, OUT_SPK, adev->platform);
    ALOGD("out_reset_voice_sample_rate: %d -> %d", old_rate, out->config.rate);
    return;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    UNUSED(stream); UNUSED(rate);
    ALOGV("out_set_sample_rate: %d", 0);

    return -ENOSYS;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;

    return out->config.period_size *
        audio_stream_out_frame_size((const struct audio_stream_out *)stream);
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    ALOGV("out_get_channels: %#x", out->channel_mask);
    return out->channel_mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    ALOGV("out_get_format: %#x", out->format);

    return out->format;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    UNUSED(stream);
    ALOGV("out_set_format: %#x",format);

    return -ENOSYS;
}

/* must be called with hw device and output stream mutexes locked */
static int do_out_standby(struct sunxi_stream_out *out)
{
    if (!out->standby) {
        if (out->pcm) {
            pcm_close(out->pcm);
            out->pcm = NULL;
        }
        /* audio dump data close*/
        close_dump_flags(&out->dd_write_out);

        out->standby = true;
        out->dev->active_output = NULL;
    }

    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    ALOGD("out_standby");
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;

    pthread_mutex_lock(&out->lock);
    pthread_mutex_lock(&adev->lock);
    do_out_standby(out);
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&out->lock);

    platform_plugins_process(adev->platform, ON_OUT_STANDBY);

    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    ALOGV("out_dump");

    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;

    dprintf(fd, "\tout_dump:\n"
                "\t\tstandby:%d\n"
                "\t\tdevices:%#x\n"
                "\t\tdev:%p\n"
                "\t\tflags:%#x\n"
                "\t\tmuted:%d\n"
                "\t\twritten:%ld\n"
                "\t\tformat:%#x\n"
                "\t\tchannel_mask:%#x\n"
                "\t\tsample_rate:%d\n"
                "\t\tcard:%d\n"
                "\t\tport:%d\n"
                "\t\tpcm:%p\n",
                out->standby,
                out->devices,
                out->dev,
                out->flags,
                out->muted,
                (long int)out->written,
                out->format,
                out->channel_mask,
                out->sample_rate,
                out->card,
                out->port,
                out->pcm);

    /* dump stream_out pcm_config */
    dprintf(fd, "\t\tstream_out pcm_config dump:\n"
                "\t\t\tavail_min:%d\n"
                "\t\t\tchannels:%d\n"
                "\t\t\tformat:%#x\n"
                "\t\t\tperiod_count:%d\n"
                "\t\t\tperiod_size:%d\n"
                "\t\t\trate:%d\n"
                "\t\t\tsilence_threshold:%d\n"
                "\t\t\tstart_threshold:%d\n"
                "\t\t\tstop_threshold:%d\n",
                out->config.avail_min,
                out->config.channels,
                out->config.format,
                out->config.period_count,
                out->config.period_size,
                out->config.rate,
                out->config.silence_threshold,
                out->config.start_threshold,
                out->config.stop_threshold);
    return 0;
}

static void select_devices(struct sunxi_audio_device *adev, bool out_device, bool in_device)
{
    ALOGV("select_devices");

    char phone_path[50] = "null";
    char out_path[50] = "null";
    char in_path[50] = "null";
    char pdev_name[50] ="";
    int phone_pdev = 0;
    int out_pdev = 0;
    int in_pdev = 0;

    ADLOG("select_devices:mode(%#x),out_devices(%#x),in_devices(%#x),"
          "active_output(%p),active_input(%p).",
          adev->mode, adev->out_devices, adev->in_devices,
          adev->active_output, adev->active_input);

    /* select phone device */
    if (adev->mode == AUDIO_MODE_IN_CALL) {
        disable_platform_backend_pcm(adev->platform, PCM_OUT);
        disable_platform_backend_pcm(adev->platform, PCM_IN);
        reset_platform_path(adev->platform);
        phone_pdev = get_platform_phone_device(adev->out_devices,
                                               adev->platform);
        get_platform_path(phone_path, adev->platform, phone_pdev);
        apply_platform_path(adev->platform, phone_path);
        ALOGD("select device(phone):pdev:%s, path:%s",
              pdev2str(pdev_name, phone_pdev), phone_path);
        update_platform_path(adev->platform);
        enable_platform_backend_pcm(phone_pdev, adev->platform, PCM_OUT);
        enable_platform_backend_pcm(phone_pdev, adev->platform, PCM_IN);
    }

    /* select output device */
    if (adev->active_output && out_device) {
        disable_platform_backend_pcm(adev->platform, PCM_OUT);
        reset_platform_out_path(adev->platform);
        out_pdev = get_platform_device(adev->mode, adev->out_devices,
                                           adev->platform);
        get_platform_path(out_path, adev->platform, out_pdev);
        apply_platform_path(adev->platform, out_path);
        ALOGD("select device(out):pdev:%s, path:%s",
              pdev2str(pdev_name, out_pdev), out_path);
        update_platform_path(adev->platform);
        enable_platform_backend_pcm(out_pdev, adev->platform, PCM_OUT);
    }

    /* select input device */
    if (adev->active_input && in_device) {
        disable_platform_backend_pcm(adev->platform, PCM_IN);
		reset_platform_in_path(adev->platform);
        in_pdev = get_platform_device(adev->mode, adev->in_devices,
                                          adev->platform);
        get_platform_path(in_path, adev->platform, in_pdev);
        apply_platform_path(adev->platform, in_path);
        ALOGD("select device(in):pdev:%s, path:%s",
              pdev2str(pdev_name, in_pdev), in_path);
        update_platform_path(adev->platform);
        enable_platform_backend_pcm(in_pdev, adev->platform, PCM_IN);
    }
    platform_plugins_process_select_devices(adev->platform, ON_SELECT_DEVICES,
                                            adev->mode, adev->out_devices,
                                            adev->in_devices);
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    struct str_parms *parms;
    char value[32];
    int val;
    int ret = 0;

    ALOGD("%s:kvpairs: %s", __func__, kvpairs);
    parms = str_parms_create_str(kvpairs);

    /* stream out routing */
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING,
            value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);
        if (adev->out_devices != val && val) {
            /* force standby if moving to/from HDMI, BT SCO */
            int dev_mask = AUDIO_DEVICE_OUT_AUX_DIGITAL |
                AUDIO_DEVICE_OUT_ALL_SCO;
            if ((adev->out_devices & dev_mask) || (val & dev_mask)) {
                do_out_standby(out);
            }

            adev->out_devices = val;
            select_devices(adev, true, false);
        }
        pthread_mutex_unlock(&out->lock);
        pthread_mutex_unlock(&adev->lock);
    }

    /* TODO: process other parameters settings */
    str_parms_destroy(parms);
    return 0;
}

static char * out_get_parameters(const struct audio_stream *stream,
                                 const char *keys)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct str_parms *query = str_parms_create_str(keys);
    char *str;
    struct str_parms *reply = str_parms_create();
    bool replied = false;
    ALOGV("%s: enter: keys - %s", __func__, keys);

    replied |= stream_get_parameter_channels(query, reply,
                                             &out->supported_channel_masks[0]);
    replied |= stream_get_parameter_formats(query, reply,
                                            &out->supported_formats[0]);
    replied |= stream_get_parameter_rates(query, reply,
                                          &out->supported_sample_rates[0]);
    if (replied) {
        str = str_parms_to_str(reply);
    } else {
        str = strdup("");
    }
    str_parms_destroy(query);
    str_parms_destroy(reply);
    ALOGV("%s: exit: returns - %s", __func__, str);

    return str;
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    ALOGV("out_get_latency");
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;

    return (out->config.period_count * out->config.period_size * 1000) /
           (out->config.rate);
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    UNUSED(stream);
    ALOGV("out_set_volume: Left:%f Right:%f", left, right);

    return 0;
}

int start_output_stream(struct sunxi_stream_out *out)
{
    ALOGD("start_output_stream");
    int ret = 0;
    struct sunxi_audio_device *adev = out->dev;
    int platform_device;

    /* audio dump data init */
    init_dump_flags(true, &out->dd_write_out);

    adev->active_output = out;
    select_devices(adev, true, false);

    platform_device = get_platform_device(adev->mode, adev->out_devices,
                                          adev->platform);
    get_platform_snd_card_config(&out->card, &out->port, &out->config,
                                 platform_device, adev->platform);

    update_debug_flag();
    print_sunxi_stream_out(out);

#ifdef USE_RESAMPLER
    if (out->sample_rate != out->config.rate) {
        if (out_resampler) {
            release_resampler(out_resampler);
            out_resampler = NULL;
        }
        ret = create_resampler(out->sample_rate,
                               out->config.rate, 2,
                               RESAMPLER_QUALITY_DEFAULT, NULL,
                               &out_resampler);
        if (ret != 0) {
            ALOGE("create out resampler(%d->%d) failed.", out->sample_rate,
                  out->config.rate);
            return ret;
        } else {
            ALOGD("create out resampler(%d->%d) ok.", out->sample_rate,
                  out->config.rate);
        }

        if (out_resampler)
            out_resampler->reset(out_resampler);

        if (out_buffer)
            free(out_buffer);
        out_buffer = calloc(1, out->config.period_size * 8 * 4);
        if (!out_buffer) {
            ALOGE("can't calloc out_buffer");
            return -1;
        }
    }
#endif

    platform_plugins_process_start_stream(adev->platform,
                                          ON_START_OUTPUT_STREAM,
                                          out->config);
    ALOGD("+++++++++++++++ start_output_stream: pcm sample_rate: %d,pcm fmt: 0x%08x,pcm channels: %d",
            out->config.rate, out->config.format, out->config.channels);

    out->pcm = pcm_open(out->card, out->port, PCM_OUT | PCM_MONOTONIC,
                        &out->config);
    if (!pcm_is_ready(out->pcm)) {
        ALOGE("cannot open pcm_out driver: %s", pcm_get_error(out->pcm));
        pcm_close(out->pcm);
        adev->active_output = NULL;
        return -ENOMEM;
    }

    return ret;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    void *buf = (void*)buffer;
    size_t frame_size = audio_stream_out_frame_size(stream);
    size_t out_frames = bytes / frame_size;
    ssize_t ret = 0;
    ALOGV("out_write");

    pthread_mutex_lock(&out->lock);
    if (out->standby) {
        out->standby = 0;
        pthread_mutex_lock(&adev->lock);
        ret = start_output_stream(out);
        pthread_mutex_unlock(&adev->lock);
    }

#ifdef USE_RESAMPLER
    if (out_resampler) {
        size_t in_frames = bytes / frame_size;
        out_frames = in_frames * 8;
        out_resampler->resample_from_input(out_resampler,(int16_t *)buffer,
                                            &in_frames, (int16_t *)out_buffer,
                                            &out_frames);
        buf = out_buffer;
    }
#endif

    platform_plugins_process_read_write(adev->platform, ON_OUT_WRITE,
                                        out->config, (void*)buffer,
                                        out_frames * frame_size);
    /* audio dump data write */
    debug_dump_data(buf, out_frames * frame_size, &out->dd_write_out);

    /* write audio data to kernel */
    if (out->pcm) {
        if (out->muted)
            memset(buf, 0, bytes);
        ret = pcm_write(out->pcm, buf, out_frames * frame_size);
        if (ret == 0)
            out->written += bytes / (out->config.channels * sizeof(short));
    }

exit:
    pthread_mutex_unlock(&out->lock);

    if (ret != 0) {
        if (out->pcm)
            ALOGE("%s: error %zu - %s", __func__, ret, pcm_get_error(out->pcm));
        out_standby(&out->stream.common);
        usleep(bytes * 1000000 / audio_stream_out_frame_size(stream) /
               out_get_sample_rate(&out->stream.common));
    }
    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    UNUSED(stream);
    *dsp_frames = 0;
    ALOGV("out_get_render_position: dsp_frames: %p", dsp_frames);
    return 0;
}

static int out_add_audio_effect(const struct audio_stream *stream,
                                effect_handle_t effect)
{
    UNUSED(stream);
    ALOGV("out_add_audio_effect: %p", effect);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream,
                                   effect_handle_t effect)
{
    UNUSED(stream);
    ALOGV("out_remove_audio_effect: %p", effect);
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    UNUSED(stream);
    *timestamp = 0;
    //ALOGV("out_get_next_write_timestamp: %ld", (long int)(*timestamp));

    return 0;
}

static int out_get_presentation_position(const struct audio_stream_out *stream,
                                         uint64_t *frames,
                                         struct timespec *timestamp)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    int ret = -1;
    pthread_mutex_lock(&out->lock);
    int i;

    /* There is a question how to implement this correctly when there is more
     * than one PCM stream.
     * We are just interested in the frames pending for playback in the kernel
     * buffer here, not the total played since start.
     * The current behavior should be safe because the cases where both cards
     * are active are marginal.
     */
    if (out->pcm) {
        size_t avail;
        if (pcm_get_htimestamp(out->pcm, &avail, timestamp) == 0) {
            size_t kernel_buffer_size = out->config.period_size *
                                        out->config.period_count;
            /* FIXME This calculation is incorrect if there is buffering after
             * app processor
             */
            int64_t signed_frames = out->written - kernel_buffer_size + avail;
            /* It would be unusual for this value to be negative, but check
             * just in case ...
             */
            if (signed_frames >= 0) {
                *frames = signed_frames;
                ret = 0;
            }
        }
    }
    pthread_mutex_unlock(&out->lock);

    return ret;
}

static void out_update_source_metadata(struct audio_stream_out *stream,
                               const struct source_metadata* source_metadata)
{
    UNUSED(stream);
    UNUSED(source_metadata);
    //ALOGV("update_source_metadata");
}

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    ALOGV("in_get_sample_rate: %d", in->sample_rate);

    return in->sample_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    UNUSED(stream);
    ALOGV("in_set_sample_rate: %d", rate);

    return -ENOSYS;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    ALOGV("in_get_buffer_size");
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    size_t size = 0;

    /* take resampling into account and return the closest majoring
     * multiple of 16 frames, as audioflinger expects audio buffers to
     * be a multiple of 16 frames
     */
    size = (in->config.period_size * in->sample_rate) / in->config.rate;
    size = ((size + 15) / 16) * 16;

    return size * in->config.channels * sizeof(short);
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    ALOGV("in_get_channels");
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;

    return in->channel_mask;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    UNUSED(stream);
    ALOGV("in_get_format");

    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    UNUSED(stream); UNUSED(format);
    ALOGV("in_set_format");

    return -ENOSYS;
}

static int do_in_standby(struct sunxi_stream_in *in)
{
    if (!in->standby) {
        in->standby = true;
        if (in->pcm) {
            pcm_close(in->pcm);
            in->pcm = NULL;
        }
        if (in_resampler) {
            release_resampler(in_resampler);
            in_resampler = NULL;
        }
        /* audio dump data close*/
        close_dump_flags(&in->dd_read_in);
    }

    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    struct sunxi_audio_device *adev = in->dev;
    int status = 0;
    ALOGD("%s: enter", __func__);

    pthread_mutex_lock(&in->lock);
    pthread_mutex_lock(&adev->lock);
    do_in_standby(in);
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&in->lock);

    platform_plugins_process(adev->platform, ON_IN_STANDBY);
    ALOGV("%s: exit:  status(%d)", __func__, status);

    return status;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    ALOGV("in_dump");

    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;

    dprintf(fd, "\tin_dump:\n"
                "\t\tstandby:%d\n"
                "\t\tdevices:%#x\n"
                "\t\tdev:%p\n"
                "\t\tflags:%#x\n"
                "\t\tmuted:%d\n"
                "\t\tformat:%#x\n"
                "\t\tchannel_mask:%#x\n"
                "\t\tsample_rate:%d\n"
                "\t\tframes_read:%ld\n"
                "\t\tcard:%d\n"
                "\t\tport:%d\n"
                "\t\tpcm:%p\n",
                in->standby,
                in->devices,
                in->dev,
                in->flags,
                in->muted,
                in->format,
                in->channel_mask,
                in->sample_rate,
                (long int)in->frames_read,
                in->card,
                in->port,
                in->pcm);

    /* dump stream_in pcm_config */
    dprintf(fd, "\t\tstream_in pcm_config dump:\n"
                "\t\t\tavail_min:%d\n"
                "\t\t\tchannels:%d\n"
                "\t\t\tformat:%#x\n"
                "\t\t\tperiod_count:%d\n"
                "\t\t\tperiod_size:%d\n"
                "\t\t\trate:%d\n"
                "\t\t\tsilence_threshold:%d\n"
                "\t\t\tstart_threshold:%d\n"
                "\t\t\tstop_threshold:%d\n",
                in->config.avail_min,
                in->config.channels,
                in->config.format,
                in->config.period_count,
                in->config.period_size,
                in->config.rate,
                in->config.silence_threshold,
                in->config.start_threshold,
                in->config.stop_threshold
                );

    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    struct sunxi_audio_device *adev = in->dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;
    int status = 0;

    ALOGD("%s: enter: kvpairs=%s", __func__, kvpairs);
    parms = str_parms_create_str(kvpairs);

    /* in stream routing */
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING,
                            value, sizeof(value));
    pthread_mutex_lock(&in->lock);
    pthread_mutex_lock(&adev->lock);
    if (ret >= 0) {
        val = atoi(value);
        if ((adev->in_devices != val) && (val != 0)) {
            adev->in_devices = val;
            select_devices(adev, false, true);
        }
    }
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&in->lock);

    /*TODO: process other parameters setting */
    str_parms_destroy(parms);
    ALOGV("%s: exit: status(%d)", __func__, status);

    return status;
}

static char * in_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    struct str_parms *query = str_parms_create_str(keys);
    char *str;
    struct str_parms *reply = str_parms_create();
    bool replied = false;

    ALOGV("%s: enter: keys - %s", __func__, keys);
    replied |= stream_get_parameter_channels(query, reply,
                                             &in->supported_channel_masks[0]);
    replied |= stream_get_parameter_formats(query, reply,
                                            &in->supported_formats[0]);
    replied |= stream_get_parameter_rates(query, reply,
                                          &in->supported_sample_rates[0]);
    if (replied) {
        str = str_parms_to_str(reply);
    } else {
        str = strdup("");
    }
    str_parms_destroy(query);
    str_parms_destroy(reply);
    ALOGV("%s: exit: returns - %s", __func__, str);

    return str;
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    UNUSED(stream); UNUSED(gain);
    ALOGV("in_set_gain");

    return 0;
}

int start_input_stream(struct sunxi_stream_in *in)
{
    ALOGD("start_input_stream");
    int ret = 0;
    struct sunxi_audio_device *adev = in->dev;
    int platform_device;

    /* audio dump data init */
    init_dump_flags(false, &in->dd_read_in);

    adev->active_input = in;
    adev->in_devices = in->devices;
    select_devices(adev, false, true);

    platform_device = get_platform_device(adev->mode, adev->in_devices,
                                          adev->platform);
    get_platform_snd_card_config(&in->card, &in->port, &in->config,
                                 platform_device, adev->platform);

    update_debug_flag();
    print_sunxi_stream_in(in);
    ALOGD("+++++++++++++++ start_input_stream: pcm sample_rate: %d,pcm fmt: 0x%08x,pcm channels: %d",
        in->config.rate, in->config.format, in->config.channels);

    in->pcm = pcm_open(in->card, in->port, PCM_IN | PCM_MONOTONIC, &in->config);
    if (!pcm_is_ready(in->pcm)) {
        ALOGE("cannot open pcm_in driver: %s", pcm_get_error(in->pcm));
        pcm_close(in->pcm);
        adev->active_input = NULL;
        return -ENOMEM;
    }

#ifdef USE_RESAMPLER
    if (in->sample_rate != in->config.rate) {
        if (in_resampler) {
            release_resampler(in_resampler);
            in_resampler = NULL;
        }

        in_buf_provider.get_next_buffer = get_next_buffer;
        in_buf_provider.release_buffer = release_buffer;
        ret = create_resampler(in->config.rate, in->sample_rate,
                               in->config.channels, RESAMPLER_QUALITY_DEFAULT,
                               &in_buf_provider, &in_resampler);
        if (ret != 0) {
            ALOGE("create in resampler(%d->%d) failed.", in->config.rate,
                  in->sample_rate);
            if (in_resampler) {
                release_resampler(in_resampler);
                in_resampler = NULL;
                return -1;
            }
        } else {
            ALOGD("create in resampler(%d->%d) ok.", in->config.rate,
                  in->sample_rate);
            if (in_resampler) {
                in_resampler->reset(in_resampler);
                in_frames_in = 0;
            }
        }

        if(in_resampler)
            in_resampler->reset(in_resampler);

        if (in_buffer)
            free(in_buffer);
        in_buffer = calloc(1, in->config.period_size *
                           audio_stream_in_frame_size(&in->stream) * 8);
        if (!in_buffer) {
            ALOGE("can't calloc in_buffer");
            return -1;
        }
    }
#endif

    platform_plugins_process_start_stream(adev->platform,
                                          ON_START_INPUT_STREAM,
                                          in->config);

    return ret;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    struct sunxi_audio_device *adev = in->dev;
    size_t frames = bytes / audio_stream_in_frame_size(stream);
    int i;
    int ret = -1;
    ALOGV("in_read");

    pthread_mutex_lock(&in->lock);

    if (in->standby) {
        pthread_mutex_lock(&adev->lock);
        ret = start_input_stream(in);
        pthread_mutex_unlock(&adev->lock);
        if (ret != 0) {
            goto exit;
        }
        in->standby = 0;
    }

#ifdef USE_RESAMPLER
    if (in_resampler && in->pcm) {
        ret = read_frames(in, buffer, frames);
        ret = ret>0 ? 0:ret;
    } else if (!in_resampler && in->pcm) {
        ret = pcm_read(in->pcm, buffer, bytes);
    }
#else
    if (in->pcm) {
        ret = pcm_read(in->pcm, buffer, bytes);
    }
#endif

    platform_plugins_process_read_write(adev->platform, ON_IN_READ, in->config,
                                        buffer, bytes);
    /* audio dump data write */
    debug_dump_data(buffer, bytes, &in->dd_read_in);

    if (ret == 0 && adev->mic_muted)
        memset(buffer, 0, bytes);

exit:
    pthread_mutex_unlock(&in->lock);

    if (ret != 0) {
        in_standby(&in->stream.common);
        ALOGV("%s: read failed - sleeping for buffer duration", __func__);
        usleep(bytes * 1000000 / audio_stream_in_frame_size(stream) /
               in_get_sample_rate(&in->stream.common));
    }

    if (bytes > 0) {
        in->frames_read += bytes / audio_stream_in_frame_size(stream);
    }

    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    UNUSED(stream);
    return 0;
}

static int in_get_capture_position(const struct audio_stream_in *stream,
                                   int64_t *frames, int64_t *time)
{
    if (stream == NULL || frames == NULL || time == NULL) {
        return -EINVAL;
    }

    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    int ret = -ENOSYS;

    pthread_mutex_lock(&in->lock);
    if (in->pcm) {
        struct timespec timestamp;
        unsigned int avail;
        if (pcm_get_htimestamp(in->pcm, &avail, &timestamp) == 0) {
            *frames = in->frames_read + avail;
            *time = timestamp.tv_sec * 1000000000LL + timestamp.tv_nsec;
            ret = 0;
        }
    }
    pthread_mutex_unlock(&in->lock);

    return ret;
}

static int in_add_audio_effect(const struct audio_stream *stream,
                               effect_handle_t effect)
{
    UNUSED(stream); UNUSED(effect);
    ALOGV("in_add_audio_effect");

    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream,
                                  effect_handle_t effect)
{
    UNUSED(stream); UNUSED(effect);
    ALOGV("in_remove_audio_effect");

    return 0;
}

static int in_get_active_microphones(const struct audio_stream_in *stream,
                              struct audio_microphone_characteristic_t *mic_array,
                              size_t *mic_count)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    struct sunxi_audio_device *adev = in->dev;
    ALOGV("%s", __func__);

    pthread_mutex_lock(&in->lock);
    pthread_mutex_lock(&adev->lock);
    int ret = platform_get_active_microphones(adev->platform,
                                              audio_channel_count_from_in_mask(in->channel_mask),
                                              mic_array, mic_count);
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&in->lock);

    return ret;
}

static void in_update_sink_metadata(struct audio_stream_in *stream,
                             const struct sink_metadata* sink_metadata)
{
    UNUSED(stream);
    UNUSED(sink_metadata);
    ALOGV("in_update_sink_metadata");
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused)
{
    UNUSED(handle);
    ALOGD("%s: enter: request format(%#x) sample_rate(%d) channel_mask(%#x) devices(%#x) flags(%#x)",
          __func__, config->format, config->sample_rate, config->channel_mask, devices, flags);

    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;
    struct sunxi_stream_out *out;
    int ret;

    out = (struct sunxi_stream_out *)calloc(1, sizeof(struct sunxi_stream_out));
    if (!out)
        return -ENOMEM;

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
    out->stream.get_presentation_position = NULL;//out_get_presentation_position;
    out->stream.update_source_metadata = out_update_source_metadata;

    *stream_out = &out->stream;

    out->standby = true;
    out->flags = flags;
    out->devices = devices;
    out->dev = adev;
    out->card = 0;
    out->port = 0;
    /* audio data dump */
    out->dd_write_out.file = NULL;
    out->dd_write_out.enable_flags = false;

    if (AUDIO_DEVICE_OUT_AUX_DIGITAL & devices) {
        out->format = AUDIO_FORMAT_PCM_16_BIT;
        get_platform_snd_card_config(&out->card, &out->port, &out->config,
            OUT_HDMI, adev->platform);
        out->sample_rate = out->config.rate;
        if (1 == out->config.channels) {
            out->channel_mask = AUDIO_CHANNEL_OUT_MONO;
        } else {
            out->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
        }

    } else {
        out->format = config->format;
        out->sample_rate = config->sample_rate;
        out->channel_mask = config->channel_mask;
    }
    memcpy(&out->config, &out_pcm_config, sizeof(struct pcm_config));

    /*TODO: update snd_pcm_config in final config when opened*/
    out->config.channels = audio_channel_count_from_out_mask(out->channel_mask);
    //out->config.rate = out->sample_rate;
    update_platform_snd_card_config(out->config, OUT_SPK, adev->platform);

    /* FIXME: when we support multiple output devices, we will want to
    * do the following:
    * adev->out_device = out->device;
    * select_output_device(adev);
    * This is because out_set_parameters() with a route is not
    * guaranteed to be called after an output stream is opened. */

    config->format          = out_get_format(&out->stream.common);
    config->channel_mask    = out_get_channels(&out->stream.common);
    config->sample_rate     = out_get_sample_rate(&out->stream.common);

    register_format(out->format, out->supported_formats);
    register_channel_mask(out->channel_mask, out->supported_channel_masks);
    register_sample_rate(out->sample_rate, out->supported_sample_rates);

    ALOGD("+++++++++++++++ adev_open_output_stream: req_sample_rate: %d, fmt: 0x%08x, channel_mask: 0x%08x",
        config->sample_rate, config->format, config->channel_mask);

    platform_plugins_process(adev->platform, ON_OPEN_OUTPUT_STREAM);

    print_sunxi_stream_out(out);
    return 0;

err_open:
    ALOGE("%s: err_open", __func__);
    free(out);
    *stream_out = NULL;

    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    ALOGV("adev_close_output_stream...");
    struct sunxi_audio_device *adev = (struct sunxi_audio_device*)dev;

    platform_plugins_process(adev->platform, ON_CLOSE_OUTPUT_STREAM);

    adev->active_output = NULL;
    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    ALOGD("adev_set_parameters, %s", kvpairs);
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;
    struct sunxi_stream_out *out = adev->active_output;

    struct str_parms *parms;
    char value[32];
    int val;
    int ret = 0;

    parms = str_parms_create_str(kvpairs);

    /* stream out routing */
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING,
            value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);
        if (adev->out_devices != val && val) {
            /* force standby if moving to/from HDMI, BT SCO */
            int dev_mask = AUDIO_DEVICE_OUT_AUX_DIGITAL |
                AUDIO_DEVICE_OUT_ALL_SCO;
            if ((adev->out_devices & dev_mask) || (val & dev_mask)) {
                do_out_standby(out);
            }

            adev->out_devices = val;
            select_devices(adev, true, true);
        }
        pthread_mutex_unlock(&out->lock);
        pthread_mutex_unlock(&adev->lock);
    }

    /* TODO: process other parameters settings */
    str_parms_destroy(parms);
    return 0;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    UNUSED(dev); UNUSED(keys);
    ALOGV("adev_get_parameters, %s", keys);

    return 0;
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    UNUSED(dev);
    ALOGV("adev_init_check");

    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    UNUSED(dev);
    ALOGV("adev_set_voice_volume: %f", volume);

    return 0;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    UNUSED(dev);
    ALOGV("adev_set_master_volume: %f", volume);

    return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    UNUSED(dev);
    ALOGV("adev_get_master_volume: %f", *volume);

    return -ENOSYS;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool muted)
{
    UNUSED(dev);
    ALOGV("adev_set_master_mute: %d", muted);

    return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *muted)
{
    UNUSED(dev);
    ALOGV("adev_get_master_mute: %d", *muted);

    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;
    struct sunxi_stream_out *out = adev->active_output;
    ADLOG("adev_set_mode: %d[current is %d]", mode, adev->mode);

    pthread_mutex_lock(&adev->lock);
    if (adev->mode != mode) {
        adev->mode = mode;
        select_devices(adev, true, true);
        #if 0
        // FIXME:support different sr from 48k to 16k when communication,and vice versa.
        // start
        if(out != NULL) {
            if(mode == AUDIO_MODE_IN_COMMUNICATION) {
                pthread_mutex_lock(&out->lock);
                do_out_standby(out);
                pthread_mutex_unlock(&out->lock);

                platform_plugins_process(adev->platform, ON_OUT_STANDBY);
                out_update_voice_sample_rate((const struct audio_stream *)out);
            } else if(mode == AUDIO_MODE_NORMAL) {
                pthread_mutex_lock(&out->lock);
                do_out_standby(out);
                pthread_mutex_unlock(&out->lock);

                platform_plugins_process(adev->platform, ON_OUT_STANDBY);
                out_reset_voice_sample_rate((const struct audio_stream *)out);
            }
        }
        // end
        #endif
    }
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    ALOGV("%s: state %d\n", __func__, state);
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;

    pthread_mutex_lock(&adev->lock);
    adev->mic_muted = state;
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    ALOGV("adev_get_mic_mute");
    struct sunxi_audio_device *adev = (struct sunxi_audio_device*)dev;
    *state = adev->mic_muted;
    return 0;
}

static int adev_get_microphones(const struct audio_hw_device *dev,
                   struct audio_microphone_characteristic_t *mic_array,
                   size_t *mic_count)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;
    ALOGV("%s", __func__);

    pthread_mutex_lock(&adev->lock);
    int ret = platform_get_microphones(adev->platform, mic_array, mic_count);
    pthread_mutex_unlock(&adev->lock);

    return ret;
}

static int check_input_parameters(uint32_t sample_rate,
                                  audio_format_t format,
                                  int channel_count)
{
    if (format != AUDIO_FORMAT_PCM_16_BIT) {
        ALOGE("%s: unsupported AUDIO FORMAT (%d) ", __func__, format);
        return -EINVAL;
    }

    if ((channel_count < 1) ||
        (channel_count > 2)) {
        ALOGE("%s: unsupported channel count (%d)", __func__, channel_count);
        return -EINVAL;
    }

    switch (sample_rate) {
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
        ALOGE("%s: unsupported (%d) samplerate", __func__, sample_rate);
        return -EINVAL;
    }

    return 0;
}

static size_t get_input_buffer_size(uint32_t sample_rate,
                                    audio_format_t format,
                                    int channel_count)
{
    size_t size = 0;

    if (check_input_parameters(sample_rate, format, channel_count) != 0)
        return 0;

    /* take resampling into account and return the closest majoring
     * multiple of 16 frames, as audioflinger expects audio buffers to
     * be a multiple of 16 frames
     */
    /* TODO: use platform pcm_config config !!!!!!!! */
    size = (in_pcm_config.period_size * sample_rate) / in_pcm_config.rate;
    size = ((size + 15) / 16) * 16;
    return size * channel_count * sizeof(short);
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    UNUSED(dev);
    ALOGV("adev_get_input_buffer_size");
    size_t size;
    int channel_count = popcount(config->channel_mask);
    if (check_input_parameters(config->sample_rate, config->format, channel_count))
        return 0;

    return get_input_buffer_size(config->sample_rate, config->format, channel_count);
}
static int in_set_microphone_direction(const struct audio_stream_in *stream,
                                           audio_microphone_direction_t dir) {
    UNUSED(stream);
    UNUSED(dir);
    return -ENOSYS;
}

static int in_set_microphone_field_dimension(const struct audio_stream_in *stream, float zoom) {
    UNUSED(zoom);
    return -ENOSYS;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags,
                                  const char *address __unused,
                                  audio_source_t source)
{
    UNUSED(handle);
    ALOGD("%s: enter: request format(%#x) sample_rate(%d) channel_mask(%#x) devices(%#x) flags(%#x) source(%#x)",
          __func__, config->format, config->sample_rate, config->channel_mask, devices, flags, source);

    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;
    struct sunxi_stream_in *in;
    int channel_count = audio_channel_count_from_in_mask(config->channel_mask);
    int ret;

    in = (struct sunxi_stream_in *)calloc(1, sizeof(struct sunxi_stream_in));
    if (!in)
        return -ENOMEM;

    /* 1. init input stream common func */
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
    in->stream.get_capture_position = in_get_capture_position;
    in->stream.get_active_microphones = in_get_active_microphones;
    in->stream.update_sink_metadata = in_update_sink_metadata;
    in->stream.set_microphone_direction = in_set_microphone_direction;
    in->stream.set_microphone_field_dimension = in_set_microphone_field_dimension;

    /* 2. init other parameters */
    in->standby = true;
    in->card = 0;
    in->port = 0;
    in->devices = devices;
    in->channel_mask = config->channel_mask;
    in->sample_rate = config->sample_rate;
    in->format = config->format;
    in->dev = adev;
    /* audio data dump */
    in->dd_read_in.file = NULL;
    in->dd_read_in.enable_flags = false;

    memcpy(&in->config, &in_pcm_config, sizeof(struct pcm_config));
    in->config.channels = channel_count;
    // FIXME:sr select 16k when communication.Due to input isn't the common stream,
    // so it's done here instead of @start_input_stream.
    // start
#if 0
    if(source == AUDIO_SOURCE_VOICE_COMMUNICATION) {
        in->config.rate = DEFAULT_INPUT_VOICE_SAMPLING_RATE;
    } else if(source == AUDIO_SOURCE_VOICE_RECOGNITION) {
        // This means that input would be restricted to 48000 when Recognition
        in->config.rate = DEFAULT_INPUT_RECOGNITION_SAMPLING_RATE;
        ALOGD("adev_open_input_stream...: AUDIO_SOURCE_VOICE_RECOGNITION!!!");
    } else {
        in->config.rate = in->sample_rate;
    }
    if(adev->active_output && source == AUDIO_SOURCE_VOICE_RECOGNITION) {
        ALOGD("adev_open_input_stream...: active output!!!");
        struct sunxi_stream_out *out = adev->active_output;
        // This means that input will be resampled in @start_input_stream
        in->config.rate = out->config.rate;
    } else {
        in->config.rate = in->sample_rate;
    }
#endif
    /*TODO: update snd_pcm_config in final config when opened*/
    update_platform_snd_card_config(in->config, IN_AMIC, adev->platform);

    ALOGD("+++++++++++++++ adev_open_input_stream: req_sample_rate: %d, fmt: 0x%08x, channel_mask: 0x%08x",
        config->sample_rate, config->format, config->channel_mask);

    register_format(in->format, in->supported_formats);
    register_channel_mask(in->channel_mask, in->supported_channel_masks);
    register_sample_rate(in->sample_rate, in->supported_sample_rates);

    *stream_in = &in->stream;

    platform_plugins_process(adev->platform, ON_OPEN_INPUT_STREAM);

    print_sunxi_stream_in(in);

    return 0;

err_open:
    ALOGE("%s:err_open", __func__);
    free(in);
    *stream_in = NULL;

    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *in)
{
    ALOGV("adev_close_input_stream...");
    struct sunxi_audio_device *adev = (struct sunxi_audio_device*)dev;

    adev->active_input = NULL;

    platform_plugins_process(adev->platform, ON_CLOSE_INPUT_STREAM);

    free(in);
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    ALOGV("adev_dump");
    struct sunxi_audio_device *adev = (struct sunxi_audio_device*)device;

    dprintf(fd, "\tadev_dump:\n"
                "\t\tactive_input:%p\n"
                "\t\tactive_output:%p\n"
                "\t\tmode:%#x\n"
                "\t\tin_devices:%#x\n"
                "\t\tout_devices:%#x\n"
                "\t\tmic_muted:%d\n",
                adev->active_input,
                adev->active_output,
                adev->mode,
                adev->in_devices,
                adev->out_devices,
                adev->mic_muted);

    /* dump out stream */
    if (adev->active_output) {
        out_dump((const struct audio_stream *)adev->active_output, fd);
    }

    /* dump in stream */
    if (adev->active_input) {
        in_dump((const struct audio_stream *)adev->active_input, fd);
    }

    /* dump platform */
    if (adev->platform) {
        platform_dump((const struct platform *)adev->platform, fd);
    }

    return 0;
}

static int adev_close(hw_device_t *device)
{
    ALOGV("adev_close");
    struct sunxi_audio_device *adev = (struct sunxi_audio_device*)device;

    platform_plugins_process(adev->platform, ON_ADEV_CLOSE);

    platform_exit(adev->platform);
    free(adev);

    return 0;
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    ALOGD("adev_open: %s", name);

    struct sunxi_audio_device *adev;
    int ret;

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = calloc(1, sizeof(struct sunxi_audio_device));
    if (!adev) {
        ALOGE("can't calloc sunxi_audio_device");
        return -ENOMEM;
    }

    adev->device.common.tag = HARDWARE_DEVICE_TAG;
    adev->device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->device.common.module = (struct hw_module_t *) module;
    adev->device.common.close = adev_close;

    adev->device.init_check = adev_init_check;
    adev->device.set_voice_volume = adev_set_voice_volume;
    adev->device.set_master_volume = adev_set_master_volume;
    adev->device.get_master_volume = adev_get_master_volume;
    adev->device.set_master_mute = adev_set_master_mute;
    adev->device.get_master_mute = adev_get_master_mute;
    adev->device.set_mode = adev_set_mode;
    adev->device.set_mic_mute = adev_set_mic_mute;
    adev->device.get_mic_mute = adev_get_mic_mute;
    adev->device.set_parameters = adev_set_parameters;
    adev->device.get_parameters = adev_get_parameters;
    adev->device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->device.open_output_stream = adev_open_output_stream;
    adev->device.close_output_stream = adev_close_output_stream;
    adev->device.open_input_stream = adev_open_input_stream;
    adev->device.close_input_stream = adev_close_input_stream;
    adev->device.get_microphones = adev_get_microphones;
    adev->device.dump = adev_dump;

    *device = &adev->device.common;

    update_debug_flag();
    ADLOG("%s: hal debug enable", __func__);

    /* load platform config */
    adev->platform = platform_init(adev);

    print_sunxi_audio_device(adev);

    /* plugins process */
    platform_plugins_process(adev->platform, ON_ADEV_OPEN);

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = 2,
        .hal_api_version = 0,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "SUNXI Audio HAL",
        .author = "ywx@Allwinnertech",
        .methods = &hal_module_methods,
    },
};

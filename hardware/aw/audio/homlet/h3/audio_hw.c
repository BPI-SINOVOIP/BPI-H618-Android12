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
#include <pthread.h>
#include <utils/threads.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <audio_utils/resampler.h>
#include <audio_utils/echo_reference.h>
#include <hardware/audio_effect.h>
#include <audio_effects/effect_aec.h>
#include <audio_route/audio_route.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <cutils/properties.h> // for property_get
#include "libtinyalsa_audio/asoundlib.h"

#define F_LOG ALOGV("%s, line: %d", __FUNCTION__, __LINE__);
#define UNUSED(x) ((void)(x))

#define PRO_AUDIO_OUTPUT_ACTIVE     "vendor.audio.output.active"
#define PRO_AUDIO_INPUT_ACTIVE      "vendor.audio.input.active"

#define MIXER_USB_MIC_GAIN          "Mic Capture Volume"

#define USERECORD_44100_SAMPLERATE
#define call_button_voice 0
#define MIXER_CARD 0

/* ALSA cards for A1X */
#define CARD_A1X_CODEC      0
#define CARD_A1X_HDMI       1
#define CARD_A1X_SPDIF      2
#define CARD_DAUDIO1        3
#define CARD_A1X_DEFAULT    0
#define CARD_A1X_MIX        4

/* ALSA ports for A1X */
#define PORT_CODEC      0
#define PORT_HDMI       0
#define PORT_SPDIF      0
#define PORT_DAUDIO1    0

#define SAMPLING_RATE_8K    8000
#define SAMPLING_RATE_11K    11025
#define SAMPLING_RATE_44K    44100
#define SAMPLING_RATE_48K    48000

/* constraint imposed by ABE: all period sizes must be multiples of 24 */
#define ABE_BASE_FRAME_COUNT 24
/* number of base blocks in a short period (low latency) */
//#define SHORT_PERIOD_MULTIPLIER 44  /* 22 ms */
#define SHORT_PERIOD_MULTIPLIER 55 /* 40 ms */
/* number of frames per short period (low latency) */
//#define SHORT_PERIOD_SIZE (ABE_BASE_FRAME_COUNT * SHORT_PERIOD_MULTIPLIER)
#define OUTPUT (960) // must be 16x, 640x for webrtc
#define SHORT_PERIOD_SIZE OUTPUT
/* number of short periods in a long period (low power) */
//#define LONG_PERIOD_MULTIPLIER 14  /* 308 ms */
#define LONG_PERIOD_MULTIPLIER 6  /* 240 ms */
/* number of frames per long period (low power) */
#define LONG_PERIOD_SIZE (SHORT_PERIOD_SIZE * LONG_PERIOD_MULTIPLIER)
/* number of pseudo periods for playback */
#define PLAYBACK_PERIOD_COUNT 2
/* number of periods for capture */
#define CAPTURE_PERIOD_COUNT 2
/* minimum sleep time in out_write() when write threshold is not reached */
#define MIN_WRITE_SLEEP_US 5000

#define RESAMPLER_BUFFER_FRAMES (SHORT_PERIOD_SIZE * 2)
#define RESAMPLER_BUFFER_SIZE (4 * RESAMPLER_BUFFER_FRAMES)

/* android default out sampling rate*/
#define DEFAULT_OUT_SAMPLING_RATE SAMPLING_RATE_44K

/* audio codec default sampling rate*/
#define MM_SAMPLING_RATE SAMPLING_RATE_44K

/*wifi display buffer size*/
#define AF_BUFFER_SIZE 1024 * 80
#define AC100_XML_PATH "/vendor/etc/ac100_paths.xml"
#define AUDIO_USE_AC100 1
#define USB_MIC_KARAOK 1
 /*normal path*/
#define media_speaker    "media-speaker"   /* OUT_DEVICE_SPEAKER */
#define media_headset "media-headphones"/* OUT_DEVICE_HEADSET */
#define media_headphones    "media-headphones"/* OUT_DEVICE_HEADPHONES */
#define media_bluetooth_sco "com-ap-bt"/* OUT_DEVICE_BT_SCO */
#define media_speaker_and_headphones    "media-speaker-headphones"/* OUT_DEVICE_SPEAKER_AND_HEADSET */
#define media_speaker_and_headphones_off "media-speaker-headphones-off"
#define media_earpiece  "null"          /*OUT_DEVICE_EARPIECE*/
#define media_single_speaker    "media-single-speaker"          /*OUT_DEVICE_SINGLE_SPEAKER*/
#define media_mainmic   "media-main-mic"       /* IN_DEVICE_MAINMIC */
#define media_main_dmic "media-digital-mic"       /* IN_DEVICE_MAINMIC */
#define media_headsetmic    "media-headset-mic"           /* IN_DEVICE_HEADSET */
#define media_btmic "com-bt-ap"           /* IN_DEVICE_HEADSET */
#define media_karaoke_ac100_speaker_output "karaoke-ac100-speaker-output" /*OUT_DEVICE_KARAOKE_AC100_SPEAKER*/
#define media_karaoke_ac100_to_hdmi_output "karaoke-ac100-to-hdmi-output" /*OUT_DEVICE_KARAOKE_AC100_TO_HDMI*/

#define media_karaoke_ac100_mic1_input "karaoke-ac100-mic1-input" /*OUT_DEVICE_KARAOKE_AC100_TO_HDMI*/
#define media_ac200_output  "ac200-output"           /* OUT_DEVICE_AC200_OUTPUT */ 

enum tty_modes {
    TTY_MODE_OFF,
    TTY_MODE_VCO,
    TTY_MODE_HCO,
    TTY_MODE_FULL
};
enum usb_direction {
    PLAYBACK,
    CAPTURE
};

enum {
    OUT_DEVICE_SPEAKER,
    OUT_DEVICE_HEADSET,
    OUT_DEVICE_HEADPHONES,
    OUT_DEVICE_BT_SCO,
    OUT_DEVICE_SPEAKER_AND_HEADSET,
    OUT_DEVICE_SPEAKER_AND_HEADSET_OFF,
    OUT_DEVICE_EARPIECE,
    OUT_DEVICE_SINGLE_SPEAKER,
    OUT_DEVICE_KARAOKE_AC100_SPEAKER,
    OUT_DEVICE_KARAOKE_AC100_TO_HDMI,
    OUT_DEVICE_AC200_OUTPUT,
    OUT_DEVICE_TAB_SIZE,           /* number of rows in route_configs[][] */
};

enum {
    IN_SOURCE_MAINMIC,
    IN_SOURCE_HEADSETMIC,
    IN_SOURCE_BTMIC,
    IN_SOURCE_AC100_MIC1,
    IN_SOURCE_TAB_SIZE,            /* number of lines in route_configs[][] */
    IN_SOURCE_NONE,
};

const char * const normal_route_configs[OUT_DEVICE_TAB_SIZE] = {
    media_speaker,             /* OUT_DEVICE_SPEAKER */
    media_headset,             /* OUT_DEVICE_HEADSET */
    media_headphones,          /* OUT_DEVICE_HEADPHONES */
    media_bluetooth_sco,             /* OUT_DEVICE_BT_SCO */
    media_speaker_and_headphones,    /* OUT_DEVICE_SPEAKER_AND_HEADSET */
    media_speaker_and_headphones_off,/* OUT_DEVICE_SPEAKER_AND_HEADSET_OFF */
    media_earpiece,    /* OUT_DEVICE_EARPIECE */
    media_single_speaker,    /* OUT_DEVICE_SINGLE_SPEAKER */
    media_karaoke_ac100_speaker_output,/*OUT_DEVICE_KARAOKE_AC100_SPEAKER*/
    media_karaoke_ac100_to_hdmi_output,/*OUT_DEVICE_KARAOKE_AC100_TO_HDMI*/
    media_ac200_output,    /* OUT_DEVICE_AC200_OUTPUT */
};

const char * const cap_normal_route_configs[IN_SOURCE_TAB_SIZE] = {
    media_mainmic,             /* IN_DEVICE_MAINMIC */
    media_headsetmic,             /* IN_DEVICE_HEADSET */
    media_btmic,             /* IN_DEVICE_bt */
    media_karaoke_ac100_mic1_input,/*IN_SOURCE_AC100_MIC1*/
};

struct pcm_config pcm_config_mm_out = {
    .channels = 2,
    .rate = MM_SAMPLING_RATE,
    .period_size = SHORT_PERIOD_SIZE,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};
struct pcm_config pcm_config_mm_in = {
    .channels = 2,
    .rate = MM_SAMPLING_RATE,
    .period_size = 240,
    .period_count = CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

#define PROP_RAWDATA_KEY               "vendor.mediasw.sft.rawdata"
#define PROP_RAWDATA_MODE_PCM          "PCM"
#define PROP_RAWDATA_MODE_HDMI_RAW     "HDMI_RAW"
#define PROP_RAWDATA_MODE_SPDIF_RAW    "SPDIF_RAW"
#define PROP_RAWDATA_DEFAULT_VALUE     PROP_RAWDATA_MODE_PCM
#define AUX_DIGITAL_MULTI_PERIOD_SIZE  2048
#define AUX_DIGITAL_MULTI_PERIOD_COUNT 2
#define AUX_DIGITAL_MULTI_DEFAULT_CHANNEL_COUNT 2
#define AUX_DIGITAL_MULTI_PERIOD_BYTES (AUX_DIGITAL_MULTI_PERIOD_SIZE * AUX_DIGITAL_MULTI_DEFAULT_CHANNEL_COUNT * 2)

struct pcm_config pcm_config_aux_digital = {
    .channels = AUX_DIGITAL_MULTI_DEFAULT_CHANNEL_COUNT, /* changed when the stream is opened */
    .rate = MM_SAMPLING_RATE, /* changed when the stream is opened */
    .period_size = AUX_DIGITAL_MULTI_PERIOD_SIZE,
    .period_count = AUX_DIGITAL_MULTI_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 0,
    .stop_threshold = INT_MAX,
    .avail_min = 0,
};

struct route_setting
{
    char *ctl_name;
    int intval;
    char *strval;
};

struct mixer_ctls
{
    struct mixer_ctl *master_playback_volume;
    struct mixer_ctl *audio_spk_headset_switch;
    struct mixer_ctl *usb_gain;
};

struct pcm_buf_manager
{
    pthread_mutex_t lock;               /* see note below on mutex acquisition order */
    bool            BufExist;
    unsigned char   *BufStart;
    int             BufTotalLen;
    unsigned char   *BufReadPtr;
    int             DataLen;
    unsigned char   *BufWritPtr;
    int             BufValideLen;
    int             SampleRate;
    int             Channel;
    struct sunxi_audio_device *dev;
};

#define MAX_AUDIO_DEVICES   16

typedef enum e_AUDIO_DEVICE_MANAGEMENT
{
    AUDIO_IN        = 0x01,
    AUDIO_OUT       = 0x02,
}e_AUDIO_DEVICE_MANAGEMENT;

typedef struct _sunxi_buffer_list{
    struct _sunxi_buffer_list *prev;
    struct _sunxi_buffer_list *next;
    void *buffer;
    int32_t size;
}sunxi_buffer_list;
 
typedef struct sunxi_audio_device_manager {
    char        name[32];
    char        card_id[32];
    int         card;
    int         device;
    int         flag_in;            //
    int         flag_in_active;     // 0: not used, 1: used to caputre
    int         flag_out;
    int         flag_out_active;    // 0: not used, 1: used to playback
    bool        flag_exist;         // for hot-plugging
}sunxi_audio_device_manager;
struct sunxi_audio_device {
    struct audio_hw_device hw_device;
    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct mixer *mixer;
    struct mixer_ctls mixer_ctls;
    int mode;
    int out_device;
    int in_device;
    struct pcm *pcm_modem_dl;
    struct pcm *pcm_modem_ul;
    int in_call;
    float voice_volume;
    struct sunxi_stream_in *active_input;
    struct sunxi_stream_out *active_output;
    bool mic_mute;
    int tty_mode;
    struct echo_reference_itfe *echo_reference;
    bool bluetooth_nrec;
    int wb_amr;
    bool raw_flag;      // flag for raw data
    int fm_mode;
    bool in_record;
    bool bluetooth_voice;
    bool af_capture_flag;
    int channelnum;
    bool micstart;
    bool inUsb_mic_mode;
    struct pcm_buf_manager PcmManager;
    // add for audio device management
    struct sunxi_audio_device_manager dev_manager[MAX_AUDIO_DEVICES];
    int usb_audio_cnt;
    char in_devices[128], out_devices[128];
    bool first_set_audio_routing;
    struct audio_route *ar;
    bool direct_mode;
    bool parse_iec61937;
    //card
    int cardCODEC;
    int cardHDMI;
    int cardSPDIF;
    int cardBT;
    int cardMIX;
    int cardIn;
};

struct sunxi_stream_out {
    struct audio_stream_out stream;
    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm_config multi_config[16];
    struct pcm *pcm;
    struct pcm *multi_pcm[16];
    struct resampler_itfe *resampler;
    struct resampler_itfe *multi_resampler[16];
    char *buffer;
    int standby;
    struct echo_reference_itfe *echo_reference;
    struct sunxi_audio_device *dev;
    int write_threshold;
    uint64_t written;
    unsigned int sample_rate;
    audio_format_t format;
    audio_channel_mask_t channel_mask;
    audio_output_flags_t flags;
};

#define MAX_PREPROCESSORS 3 /* maximum one AGC + one NS + one AEC per input stream */

struct sunxi_stream_in {
    struct audio_stream_in stream;
    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm *pcm;
    int device;
    struct resampler_itfe *resampler;
    struct resampler_buffer_provider buf_provider;
    int16_t *buffer;
    size_t frames_in;
    unsigned int requested_rate;
    int standby;
    int source;
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
    int read_status;
    struct sunxi_audio_device *dev;
};

#if !LOG_NDEBUG
// for test
static void tinymix_print_enum(struct mixer_ctl *ctl, int print_all)
{
    unsigned int num_enums;
    unsigned int i;
    const char *string;

    num_enums = mixer_ctl_get_num_enums(ctl);

    for (i = 0; i < num_enums; i++) {
        string = mixer_ctl_get_enum_string(ctl, i);
        if (print_all)
            printf("\t%s%s", mixer_ctl_get_value(ctl, 0) == (int)i ? ">" : "", string);
        else if (mixer_ctl_get_value(ctl, 0) == (int)i)
            printf(" %-s", string);
    }
}

static void tinymix_detail_control(struct mixer *mixer, unsigned int id,
                                   int print_all)
{
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_values;
    unsigned int i;
    int min, max;

    if (id >= mixer_get_num_ctls(mixer)) {
        fprintf(stderr, "Invalid mixer control\n");
        return;
    }

    ctl = mixer_get_ctl(mixer, id);

    type = mixer_ctl_get_type(ctl);
    num_values = mixer_ctl_get_num_values(ctl);

    if (print_all)
        printf("%s:", mixer_ctl_get_name(ctl));

    for (i = 0; i < num_values; i++) {
        switch (type)
        {
        case MIXER_CTL_TYPE_INT:
            printf(" %d", mixer_ctl_get_value(ctl, i));
            break;
        case MIXER_CTL_TYPE_BOOL:
            printf(" %s", mixer_ctl_get_value(ctl, i) ? "On" : "Off");
            break;
        case MIXER_CTL_TYPE_ENUM:
            tinymix_print_enum(ctl, print_all);
            break;
         case MIXER_CTL_TYPE_BYTE:
            printf(" 0x%02x", mixer_ctl_get_value(ctl, i));
            break;
        default:
            printf(" unknown");
            break;
        };
    }

    if (print_all) {
        if (type == MIXER_CTL_TYPE_INT) {
            min = mixer_ctl_get_range_min(ctl);
            max = mixer_ctl_get_range_max(ctl);
            printf(" (range %d->%d)", min, max);
        }
    }
    printf("\n");
}

static void tinymix_list_controls(struct mixer *mixer)
{
    struct mixer_ctl *ctl;
    const char *name, *type;
    unsigned int num_ctls, num_values;
    unsigned int i;

    num_ctls = mixer_get_num_ctls(mixer);

    printf("Number of controls: %u\n", num_ctls);

    printf("ctl\ttype\tnum\t%-40s value\n", "name");
    for (i = 0; i < num_ctls; i++) {
        ctl = mixer_get_ctl(mixer, i);

        name = mixer_ctl_get_name(ctl);
        type = mixer_ctl_get_type_string(ctl);
        num_values = mixer_ctl_get_num_values(ctl);
        printf("%u\t%s\t%u\t%-40s", i, type, num_values, name);
        tinymix_detail_control(mixer, i, 0);
    }
}
#endif

/*wifi display buffer manager*/
static int WritePcmData(void * pInbuf, int inSize, struct pcm_buf_manager *PcmManager)
{
    ALOGV("RequestWriteBuf ++: size: %d", inSize);

    if (PcmManager->BufValideLen< inSize)
    {
        ALOGE("not enough buffer to write");
        return -1;
    }

    pthread_mutex_lock(&PcmManager->lock);

    if ((PcmManager->BufWritPtr + inSize)
        > (PcmManager->BufStart + PcmManager->BufTotalLen))
    {
        int endSize = PcmManager->BufStart + PcmManager->BufTotalLen
            - PcmManager->BufWritPtr;
        memcpy(PcmManager->BufWritPtr, pInbuf, endSize);
        memcpy(PcmManager->BufStart, (void *)((char *)pInbuf + endSize), inSize - endSize);

        PcmManager->BufWritPtr = PcmManager->BufWritPtr
            + inSize - PcmManager->BufTotalLen;
    }
    else
    {
        memcpy(PcmManager->BufWritPtr, pInbuf, inSize);
        PcmManager->BufWritPtr += inSize;
    }

    PcmManager->BufValideLen -= inSize;
    PcmManager->DataLen += inSize;

    ALOGV("after wr: BufTotalLen: %d, DataLen: %d, BufValideLen: %d, BufReadPtr: %p, BufWritPtr: %p",
        PcmManager->BufTotalLen, PcmManager->DataLen, PcmManager->BufValideLen,
        PcmManager->BufReadPtr, PcmManager->BufWritPtr);

    pthread_mutex_unlock(&PcmManager->lock);
    ALOGV("RequestWriteBuf --");
    return 0;
}

static int ReadPcmData(void *pBuf, int uGetLen, struct pcm_buf_manager *PcmManager)
{
    int underflow = 0, fill_dc_size;
    int size_read = uGetLen;
    int timeout = 0, max_wait_count;

    struct sunxi_audio_device *adev = PcmManager->dev;

    max_wait_count = uGetLen * 100 / (PcmManager->SampleRate*PcmManager->Channel * 2) + 1; //normal
    max_wait_count *= 2;//twice

    ALOGV("ReadPcmDataForEnc ++, getLen: %d max_wait_count=%d", uGetLen,max_wait_count);
    if (adev->active_output != NULL)
    {
        while(PcmManager->DataLen < uGetLen)
        {
            ALOGV("pcm is not enough for audio encoder! uGetLen: %d, uDataLen: %d\n",
                uGetLen, PcmManager->DataLen);
            usleep(10 * 1000);
            timeout++;
            if(timeout > max_wait_count) {
                if (PcmManager->DataLen < uGetLen) {
                    underflow = 1;
                    size_read = PcmManager->DataLen;
                    fill_dc_size = uGetLen - PcmManager->DataLen;
                    ALOGV("fill with dc size:%d",uGetLen - PcmManager->DataLen);
                }
            break;
            }
        }
    }
    else
    {
        ALOGV("pcm is not enough for audio encoder! uGetLen: %d, uDataLen: %d\n",
            uGetLen, PcmManager->DataLen);
        if (PcmManager->DataLen < uGetLen) {
            underflow = 1;
            size_read = PcmManager->DataLen;
            fill_dc_size = uGetLen - PcmManager->DataLen;
            usleep(fill_dc_size * 1000000 / 4 / PcmManager->SampleRate);
            ALOGV("fill with dc size:%d",uGetLen - PcmManager->DataLen);
        }
    }

    if((PcmManager->BufReadPtr + size_read)
        > (PcmManager->BufStart + PcmManager->BufTotalLen))
    {
        int len1 = PcmManager->BufStart
            + PcmManager->BufTotalLen - PcmManager->BufReadPtr;
        memcpy((void *)pBuf, (void *)PcmManager->BufReadPtr, len1);
        memcpy((void *)((char *)pBuf + len1), (void *)PcmManager->BufStart, size_read - len1);
    }
    else
    {
        memcpy(pBuf, PcmManager->BufReadPtr, size_read);
    }

    pthread_mutex_lock(&PcmManager->lock);

    PcmManager->BufReadPtr += size_read;

    if(PcmManager->BufReadPtr
        >= PcmManager->BufStart + PcmManager->BufTotalLen)
    {
        PcmManager->BufReadPtr -= PcmManager->BufTotalLen;
    }
    PcmManager->DataLen -= size_read;
    PcmManager->BufValideLen += size_read;

    ALOGV("after rd: BufTotalLen: %d, DataLen: %d, BufValideLen: %d, pBufReadPtr: %p, pBufWritPtr: %p",
        PcmManager->BufTotalLen, PcmManager->DataLen, PcmManager->BufValideLen,
        PcmManager->BufReadPtr, PcmManager->BufWritPtr);

    pthread_mutex_unlock(&PcmManager->lock);
    ALOGV("ReadPcmDataForEnc --");

    if (underflow) {
        char *ptr = (char*)pBuf;
        memset(ptr+size_read, ptr[size_read-1], fill_dc_size);
    }

    return uGetLen;
}

/**
 * NOTE: when multiple mutexes have to be acquired, always respect the following order:
 *        hw device > in stream > out stream
 */
static void init_ac100_input_output_path(struct sunxi_audio_device *adev,int output_device_path ,int input_device_path);
static void select_output_device(struct sunxi_audio_device *adev);
static void select_input_device(struct sunxi_audio_device *adev);
static int adev_set_voice_volume(struct audio_hw_device *dev, float volume);
static int do_input_standby(struct sunxi_stream_in *in);
static int do_output_standby(struct sunxi_stream_out *out); 


typedef struct name_map_t
{
    char name_linux[32];
    char name_android[32];
}name_map;
#define AUDIO_MAP_CNT   16
#define AUDIO_NAME_CODEC    "AUDIO_CODEC"
#define AUDIO_NAME_HDMI     "AUDIO_HDMI"
#define AUDIO_NAME_SPDIF    "AUDIO_SPDIF"
#define AUDIO_NAME_I2S      "AUDIO_I2S"
#define AUDIO_NAME_MIX      "AUDIO_MIX"
#define AUDIO_NAME_BT       "AUDIO_BT"

static name_map audio_name_map[AUDIO_MAP_CNT] =
{
    {"snddaudio",       AUDIO_NAME_CODEC},//daudio
    {"sndac100",        AUDIO_NAME_CODEC},//ac100 codec
    {"sndac200",        AUDIO_NAME_CODEC},//ac200 codec
    {"audiocodec",      AUDIO_NAME_CODEC},//inside codec
    {"sndhdmi",         AUDIO_NAME_HDMI},
    {"sndhdmiraw",      AUDIO_NAME_HDMI},
    {"sndspdif",        AUDIO_NAME_SPDIF},
    {"snddaudio1",      AUDIO_NAME_BT},
    {"sndahub",         AUDIO_NAME_MIX},
};

static int set_audio_devices_active(struct sunxi_audio_device *adev, int in_out, char * devices);

static int find_name_map(struct sunxi_audio_device *adev, char * in, char * out)
{
    int index = 0;

    UNUSED(adev);

    if (in == 0 || out == 0)
    {
        ALOGE("error params");
        return -1;
    }

    for (; index < AUDIO_MAP_CNT; index++)
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
    char * snd_path = "/sys/class/sound";
    char snd_card[128], snd_node[128];
    char snd_id[32], snd_name[32];

    memset(snd_card, 0, sizeof(snd_card));
    memset(snd_node, 0, sizeof(snd_node));
    memset(snd_id, 0, sizeof(snd_id));
    memset(snd_name, 0, sizeof(snd_name));

    sprintf(snd_card, "%s/card%d", snd_path, card);
    ret = access(snd_card, F_OK);
    if(ret == 0)
    {
        // id / name
        sprintf(snd_node, "%s/card%d/id", snd_path, card);
        ALOGD("read card %s/card%d/id",snd_path, card);
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
        ALOGD("find name map, card_id = %s, card_name = %s ",adev->dev_manager[card].card_id,adev->dev_manager[card].name);

        adev->dev_manager[card].card = card;
        adev->dev_manager[card].device = 0;
        adev->dev_manager[card].flag_exist = true;

        // playback device
        sprintf(snd_node, "%s/card%d/pcmC%dD0p", snd_path, card, card);
        ret = access(snd_node, F_OK);
        if(ret == 0)
        {
            // there is a playback device
            adev->dev_manager[card].flag_out = AUDIO_OUT;
            adev->dev_manager[card].flag_out_active = 0;
        }

        // capture device
        sprintf(snd_node, "%s/card%d/pcmC%dD0c", snd_path, card, card);
        ret = access(snd_node, F_OK);
        if(ret == 0)
        {
            // there is a capture device
            adev->dev_manager[card].flag_in = AUDIO_IN;
            adev->dev_manager[card].flag_in_active = 0;
        }
    }
    else
    {
        return -1;
    }

    return 0;
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
}

static void init_audio_devices_active(struct sunxi_audio_device *adev)
{
    int card = 0;
    int flag_active = 0;
    char * active_name_in;
    char * active_name_out;

    F_LOG;

    // high priority, due to the proprety
    char prop_value_in[128];
    int ret = property_get(PRO_AUDIO_INPUT_ACTIVE, prop_value_in, "");
    if (ret > 0)
    {
        ALOGV("init_audio_devices_active: get property %s: %s", PRO_AUDIO_INPUT_ACTIVE, prop_value_in);
        if (set_audio_devices_active(adev, AUDIO_IN, prop_value_in) == 0)
        {
            active_name_in = prop_value_in;
            flag_active |= AUDIO_IN;
        }
    }
    else
    {
        ALOGV("init_audio_devices_active: get property %s failed, %s", PRO_AUDIO_INPUT_ACTIVE, strerror(errno));
    }

    char prop_value_out[128];
    ret = property_get(PRO_AUDIO_OUTPUT_ACTIVE, prop_value_out, "");
    if (ret > 0)
    {
        ALOGV("init_audio_devices_active: get property %s: %s", PRO_AUDIO_OUTPUT_ACTIVE, prop_value_out);
        if (set_audio_devices_active(adev, AUDIO_OUT, prop_value_out) == 0)
        {
            active_name_out = prop_value_out;
            flag_active |= AUDIO_OUT;
        }
    }
    else
    {
        ALOGV("init_audio_devices_active: get property %s failed, %s", PRO_AUDIO_OUTPUT_ACTIVE, strerror(errno));
    }

    if ((flag_active & AUDIO_IN)
        && (flag_active & AUDIO_OUT))
    {
        goto INIT_END;
    }

    // midle priority, use codec
    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        // default use auido codec in/out
        if (adev->dev_manager[card].flag_exist
            && (!strcmp(adev->dev_manager[card].name, AUDIO_NAME_CODEC)))
        {
            if (!(flag_active & AUDIO_IN)
                && (adev->dev_manager[card].flag_in == AUDIO_IN))
            {
                ALOGV("OK, default use %s capture", adev->dev_manager[card].name);
                active_name_in = adev->dev_manager[card].name;
                adev->dev_manager[card].flag_in_active = 1;
                flag_active |= AUDIO_IN;
            }
            if (!(flag_active & AUDIO_OUT)
                && (adev->dev_manager[card].flag_out == AUDIO_OUT))
            {
                ALOGV("OK, default use %s playback", adev->dev_manager[card].name);
                active_name_out = adev->dev_manager[card].name;
                adev->dev_manager[card].flag_out_active = 1;
                flag_active |= AUDIO_OUT;
            }

            break;
        }
    }

    if ((flag_active & AUDIO_IN)
        && (flag_active & AUDIO_OUT))
    {
        goto INIT_END;
    }

    // low priority, chose any device
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
                active_name_in = adev->dev_manager[card].name;
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
                active_name_out = adev->dev_manager[card].name;
                adev->dev_manager[card].flag_out_active = 1;
                flag_active |= AUDIO_OUT;
            }
        }
    }

INIT_END:

    if (flag_active & AUDIO_IN)
    {
        if (active_name_in)
        {
            adev->in_device |= AUDIO_DEVICE_IN_BUILTIN_MIC;
        }
    }
    else
    {
        ALOGW("there is not a audio capture devices");
    }

    if (flag_active & AUDIO_OUT)
    {
        if (active_name_out)
        {
            if (strstr(active_name_out, AUDIO_NAME_CODEC))
            {
                adev->out_device |= AUDIO_DEVICE_OUT_SPEAKER;
            }
            if (strstr(active_name_out, AUDIO_NAME_SPDIF))
            {
                adev->out_device |= AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET;
            }
            if (strstr(active_name_out, AUDIO_NAME_HDMI))
            {
                adev->out_device |= AUDIO_DEVICE_OUT_AUX_DIGITAL;
            }
        }
    }
    else
    {
        ALOGW("there is not a audio playback devices");
    }

    //ALOGV("OK, default adev->devices: %08x", adev->devices);
    //adev->dev_manager[0].flag_out_active = 1;
    //adev->dev_manager[1].flag_out_active = 1;
    ALOGD("XX %d %d %d %d", adev->dev_manager[0].flag_out, adev->dev_manager[0].flag_out_active,
        adev->dev_manager[1].flag_out, adev->dev_manager[1].flag_out_active);
}

static int updata_audio_devices(struct sunxi_audio_device *adev)
{
    int card = 0;
    int ret = -1;
    char * snd_path = "/sys/class/sound";
    char snd_card[128];

    memset(snd_card, 0, sizeof(snd_card));

    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        sprintf(snd_card, "%s/card%d", snd_path, card);
        ret = access(snd_card, F_OK);
        if(ret == 0)
        {
            if (adev->dev_manager[card].flag_exist == true)
            {
                continue;       // no changes
            }
            else                // plug-in
            {
                ALOGD("do init audio card");
                do_init_audio_card(adev, card);
            }
        }
        else
        {
            if (adev->dev_manager[card].flag_exist == false)
            {
                continue;       // no changes
            }
            else                // plug-out
            {
                adev->dev_manager[card].flag_exist = false;
                adev->dev_manager[card].flag_in = 0;
                adev->dev_manager[card].flag_out = 0;
            }
        }
    }

    return 0;
}

static char * get_audio_devices(struct sunxi_audio_device *adev, int in_out)
{
    char * in_devices = adev->in_devices;
    char * out_devices = adev->out_devices;

    updata_audio_devices(adev);

    memset(in_devices, 0, sizeof(adev->in_devices));
    memset(out_devices, 0, sizeof(adev->out_devices));

    ALOGD("getAudioDevices()");
    int card = 0;
    for(card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (adev->dev_manager[card].flag_exist == true
                && (strcmp(adev->dev_manager[card].name, "AUDIO_BT") != 0)
                && (strcmp(adev->dev_manager[card].name, "AUDIO_MIX") != 0))
        {
            if (adev->dev_manager[card].flag_in == AUDIO_IN)
            {
                strcat(in_devices, adev->dev_manager[card].name);
                strcat(in_devices, ",");
                ALOGD("in dev:%s",adev->dev_manager[card].name);
            }

            if (adev->dev_manager[card].flag_out == AUDIO_OUT)
            {
                strcat(out_devices, adev->dev_manager[card].name);
                strcat(out_devices, ",");
                ALOGD("out dev:%s",adev->dev_manager[card].name);
            }
        }
    }

    in_devices[strlen(in_devices) - 1] = 0;
    out_devices[strlen(out_devices) - 1] = 0;

    if (in_out & AUDIO_IN)
    {
        ALOGD("in capture: %s",in_devices);
        return in_devices;
    }
    else if(in_out & AUDIO_OUT)
    {
        ALOGD("out playback: %s",out_devices);
        return out_devices;
    }
    else
    {
        ALOGE("unknown in/out flag");
        return 0;
    }
}

// new add
static void getCardNumbyName(struct sunxi_audio_device *adev, char *name, int *card)
{
    int index;
    for (index = 0; index < MAX_AUDIO_DEVICES; index++) {
        if (!strcmp(adev->dev_manager[index].name, name)) {
            *card = index;
            ALOGD("getCardNumbyName: name = %s , card = %d",name, index);
            return;
        }
    }

    for (index = 0; index < MAX_AUDIO_DEVICES; index++) {
        if (!strncmp(adev->dev_manager[index].name, "AUDIO_USB", 9)
        && (!strncmp(name, "AUDIO_USB", 9))
        && adev->dev_manager[index].flag_exist) {
            *card = index;
            ALOGD("getCardNumbyName: name = %s , card = %d",name, index);
            break;
        }
    }
    if (index == MAX_AUDIO_DEVICES) {
        *card = -1;
        ALOGE("%s card does not exist",name);
    }
}
static bool isUsbDeviceExist(int dir, int card)
{
    int ret = -1;
    char *snd_path = "/sys/class/sound";
    char snd_node[128];

    memset(snd_node, 0, sizeof(snd_node));
    if (card < 0 || card > 8) {
        return false;
    }
    if (dir == PLAYBACK) {
        // playback device
        snprintf(snd_node, 128, "%s/card%d/pcmC%dD0p", snd_path, card, card);
    } else if (dir == CAPTURE) {
        // capture device
        snprintf(snd_node, 128, "%s/card%d/pcmC%dD0c", snd_path, card, card);
    } else {
        return false;
    }
    ret = access(snd_node, F_OK);
    if (ret == 0)
        return true;
    else
        return false;
}
#if 0
static int set_audio_devices_active_internal(struct sunxi_stream_out *stream, int in_out, int value)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    char devices[128];
    int ret = -1;

    F_LOG;

    switch(value & AUDIO_DEVICE_OUT_ALL) {
        case AUDIO_DEVICE_OUT_EARPIECE:
        case AUDIO_DEVICE_OUT_SPEAKER:
        case AUDIO_DEVICE_OUT_WIRED_HEADSET:
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
            // codec
            strcpy(devices, AUDIO_NAME_CODEC);
            break;
        case AUDIO_DEVICE_OUT_AUX_DIGITAL:
            // hdmi
            strcpy(devices, AUDIO_NAME_HDMI);
            break;
        case AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET:
            // spdif
            strcpy(devices, AUDIO_NAME_SPDIF);
            break;
        default:
            //do nothing
            // codec
            //strcpy(devices, AUDIO_NAME_CODEC);
            return 0;
    }

    if (in_out & AUDIO_OUT)
    {
        char prop_value_out[128];
        ret = property_get(PRO_AUDIO_OUTPUT_ACTIVE, prop_value_out, "");
        int odev = value & AUDIO_DEVICE_OUT_ALL;
        if (ret > 0)
        {
            // support multi-audio devices output at the same time
            ALOGV("get property %s: %s", PRO_AUDIO_OUTPUT_ACTIVE, prop_value_out);
            if(odev == AUDIO_DEVICE_OUT_WIRED_HEADPHONE
                || odev == AUDIO_DEVICE_OUT_AUX_DIGITAL)
            {
                //deal with:headphone plugin or HDMI plugin
                ALOGV("just change to this audio device:%d", odev);
            }
            else if (!strstr(prop_value_out, devices))
            {

                strcat(devices, ",");
                strcat(devices, prop_value_out);

            }
            else
            {
                strcpy(devices, prop_value_out);
            }
        }
        if (strstr(prop_value_out, AUDIO_NAME_HDMI))
        {
            do_output_standby(out);
        }
    }
    set_audio_devices_active(adev, in_out, devices);

    return 0;
}
#endif 
static int set_audio_devices_active(struct sunxi_audio_device *adev, int in_out, char * devices)
{
    int card = 0, i = 0;
    char name[8][32];
    int cnt = 0;
    char str[128];
    int ret = -1; 

    strcpy(str, devices);
    char *pval = str;

    if (pval == NULL)
    {
        return -1;
    }

    if (in_out & AUDIO_IN)
    {
        ret = property_set(PRO_AUDIO_INPUT_ACTIVE, devices);
        if (ret < 0)
        {
            ALOGE("set property %s: %s failed", PRO_AUDIO_INPUT_ACTIVE, devices);
        }
        else
        {
            ALOGV("set property %s: %s ok", PRO_AUDIO_INPUT_ACTIVE, devices);
        }
    }

    if (in_out & AUDIO_OUT)
    {
        ret = property_set(PRO_AUDIO_OUTPUT_ACTIVE, devices);
        if (ret < 0)
        {
            ALOGE("set property %s: %s failed", PRO_AUDIO_OUTPUT_ACTIVE, devices);
        }
        else
        {
            ALOGV("set property %s: %s ok", PRO_AUDIO_OUTPUT_ACTIVE, devices);
        }
    }

    char *seps = " ,";
    pval = strtok(pval, seps);
    while (pval != NULL)
    {
        strcpy(name[cnt++], pval);
        pval = strtok(NULL, seps);
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
                if ((adev->dev_manager[card].flag_in == in_out)
                    && (strcmp(adev->dev_manager[card].name, name[i]) == 0))
                {
                    ALOGV("%s %s device will be active", name[i], "input");
                    adev->dev_manager[card].flag_in_active = 1; 
                    // only one capture device can be active
                    if(isUsbDeviceExist(CAPTURE, card)
                           && !strncmp(adev->dev_manager[card].name,"AUDIO_USB",9)) {
                             adev->cardIn = card; 
                    }
                    return 0;
                }
            }
            else
            {
                if ((adev->dev_manager[card].flag_out == in_out)
                    && ((strcmp(adev->dev_manager[card].name, name[i]) == 0)
                        || ((!strncmp(name[i],"AUDIO_USB",9)) && (!strncmp(name[i],adev->dev_manager[card].name,9))))
                    )
                {
                    ALOGV("%s %s device will be active", name[i], "output");
                    adev->dev_manager[card].flag_out_active = 1; 
                    break;
                }
            }
        }

        if (card == MAX_AUDIO_DEVICES)
        {
            if (in_out & AUDIO_IN)
            {
                ALOGE("can not set %s %s active", name[i], (in_out & AUDIO_IN) ? "input" : "ouput");
                adev->dev_manager[0].flag_in_active = 1;
                ALOGE("but device %s %s will be active", adev->dev_manager[0].name, (in_out & AUDIO_IN) ? "input" : "ouput");
                return 0;
            }
            else
            {
                ALOGE("can not set %s %s active", name[i], (in_out & AUDIO_IN) ? "input" : "ouput");
                adev->dev_manager[0].flag_out_active = 1;
                adev->dev_manager[1].flag_out_active = 1;
                ALOGE("but device %s %s %s will be active", adev->dev_manager[0].name, adev->dev_manager[1].name,(in_out & AUDIO_IN) ? "input" : "ouput");
                return -1;
            }
            return -1;
        }
    } 
    return 0;
}

static int get_audio_devices_active(struct sunxi_audio_device *adev, int in_out, char * devices)
{
    ALOGD("Mic get_audio_devices_active: %s", devices);

    int card = 0;

    if (devices == 0)
    {
        return -1;
    }

    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (in_out & AUDIO_IN)
        {
            if ((adev->dev_manager[card].flag_in == in_out)
                && (adev->dev_manager[card].flag_in_active == 1))
            {
                strcat(devices, adev->dev_manager[card].name);
                strcat(devices, ",");
            }
        }
        else
        {
            if ((adev->dev_manager[card].flag_out == in_out)
                && (adev->dev_manager[card].flag_out_active == 1))
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

static int get_output_device_params(unsigned int card,struct sunxi_stream_out *out)
{
    unsigned int min, max;
    struct pcm_params *params = pcm_params_get(card, 0, PCM_OUT);
    if (params == NULL) {
        ALOGD("LINE: %d, FUNC: %s ,Device does not exist.\n",__LINE__,__FUNCTION__);
        pcm_params_free(params);
        return -1;
    }
    min = pcm_params_get_min(params, PCM_PARAM_RATE);
    max = pcm_params_get_max(params, PCM_PARAM_RATE);
    if ((min==44100)||(max==44100)) {
        out->multi_config[card].rate = 44100;
    } else if ((min==48000)||(max==48000)) {
        out->multi_config[card].rate = 48000;
    } else if ((min==32000)||(max==32000)) {
        out->multi_config[card].rate = 32000;
    } else if ((min==24000)||(max==24000)) {
        out->multi_config[card].rate = 24000;
    } else if ((min==22050)||(max==22050)) {
        out->multi_config[card].rate = 22050;
    } else if ((min==16000)||(max==16000)) {
        out->multi_config[card].rate = 16000;
    } else if ((min==12000)||(max==12000)) {
        out->multi_config[card].rate = 12000;
    } else if ((min==11025)||(max==11025)) {
        out->multi_config[card].rate = 11025;
    } else if ((min==8000)||(max==8000)) {
        out->multi_config[card].rate = 8000;
    }
    min = pcm_params_get_min(params, PCM_PARAM_CHANNELS);
    max = pcm_params_get_max(params, PCM_PARAM_CHANNELS);
    if ((min==2)||(max==2)) {
        out->multi_config[card].channels= 2;
    } else {
        out->multi_config[card].channels = 1;
    }
    ALOGD("LINE: %d, FUNC: %s , card = %d , rate = %d , channels = %d",__LINE__,__FUNCTION__,card,out->multi_config[card].rate,out->multi_config[card].channels);
    pcm_params_free(params);
    return 0;
}

#if 0
/* The enable flag when 0 makes the assumption that enums are disabled by
 * "Off" and integers/booleans by 0 */
static int set_route_by_array(struct mixer *mixer, struct route_setting *route,
                              int enable)
{
    struct mixer_ctl *ctl;
    unsigned int i, j;

    /* Go through the route array and set each value */
    i = 0;
    while (route[i].ctl_name) {
        ctl = mixer_get_ctl_by_name(mixer, route[i].ctl_name);
        if (!ctl)
            return -EINVAL;

        if (route[i].strval) {
            if (enable)
                mixer_ctl_set_enum_by_string(ctl, route[i].strval);
            else
                mixer_ctl_set_enum_by_string(ctl, "Off");
        } else {
            /* This ensures multiple (i.e. stereo) values are set jointly */
            for (j = 0; j < mixer_ctl_get_num_values(ctl); j++) {
                if (enable)
                    mixer_ctl_set_value(ctl, j, route[i].intval);
                else
                    mixer_ctl_set_value(ctl, j, 0);
            }
        }
        i++;
    }

    return 0;
}
#endif
static int start_call(struct sunxi_audio_device *adev)
{
    F_LOG;
    UNUSED(adev);
    return 0;
}

static void end_call(struct sunxi_audio_device *adev)
{
    UNUSED(adev);
}

static void force_all_standby(struct sunxi_audio_device *adev)
{
    struct sunxi_stream_in *in;
    struct sunxi_stream_out *out;

    if (adev->active_output) {
        out = adev->active_output;
        pthread_mutex_lock(&out->lock);
        do_output_standby(out);
        pthread_mutex_unlock(&out->lock);
    }
    if (adev->active_input) {
        in = adev->active_input;
        pthread_mutex_lock(&in->lock);
        do_input_standby(in);
        pthread_mutex_unlock(&in->lock);
    }
}

static void select_mode(struct sunxi_audio_device *adev)
{
    if (adev->mode == AUDIO_MODE_IN_CALL || adev->mode == AUDIO_MODE_MODE_FACTORY_TEST) {
        ALOGV("Entering IN_CALL state, in_call=%d", adev->in_call);
        if (!adev->in_call) {
            force_all_standby(adev);
            /* force earpiece route for in call state if speaker is the
            only currently selected route. This prevents having to tear
            down the modem PCMs to change route from speaker to earpiece
            after the ringtone is played, but doesn't cause a route
            change if a headset or bt device is already connected. If
            speaker is not the only thing active, just remove it from
            the route. We'll assume it'll never be used initially during
            a call. This works because we're sure that the audio policy
            manager will update the output device after the audio mode
            change, even if the device selection did not change. */
            if (adev->out_device == AUDIO_DEVICE_OUT_SPEAKER) {
                adev->out_device = AUDIO_DEVICE_OUT_EARPIECE;
                adev->in_device = AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN;
            } else
                adev->out_device &= ~AUDIO_DEVICE_OUT_SPEAKER;

            start_call(adev);
            F_LOG;
            select_output_device(adev);
            F_LOG;
            adev_set_voice_volume(&adev->hw_device, adev->voice_volume);
            adev->in_call = 1;
        }
    } else {
        ALOGV("Leaving IN_CALL state, in_call=%d, mode=%d",
             adev->in_call, adev->mode);
        if (adev->in_call) {
            adev->in_call = 0;
            end_call(adev);
            force_all_standby(adev);
            select_output_device(adev);
            select_input_device(adev);
        }
    }
    

    if (adev->mode == AUDIO_MODE_IN_COMMUNICATION) {
        ALOGV("AUDIO_MODE_IN_COMMUNICATION");
        F_LOG;
        select_output_device(adev);
        F_LOG;
    }

    if (adev->mode == AUDIO_MODE_FM) {
        force_all_standby(adev);
        F_LOG;
        select_output_device(adev);
        F_LOG;
        adev_set_voice_volume(&adev->hw_device, adev->voice_volume);
    }

}

static void init_ac100_input_output_path(struct sunxi_audio_device *adev,int output_device_path ,int input_device_path)
{
    int output_device_id = 0;
    int input_device_id = 0;
    const char *output_route = NULL;
    const char *input_route = NULL;

    if(!adev->ar){
        ALOGD("FUNC: %s, LINE: %d, !adev->ar",__FUNCTION__,__LINE__);
        return;
    }
    audio_route_reset(adev->ar);
    if(!strcmp(adev->dev_manager[0].card_id, "sndac200")){
        output_device_id = OUT_DEVICE_AC200_OUTPUT;
        output_route = normal_route_configs[output_device_id];
        input_device_id = IN_SOURCE_MAINMIC;
        input_route = cap_normal_route_configs[input_device_id];
    }else{
        //config output path
        if(output_device_path){
            output_route = normal_route_configs[output_device_path];
        }
        //config input path
        if(!input_device_path){
            input_device_path = IN_SOURCE_MAINMIC;
        }
        input_route = cap_normal_route_configs[input_device_path];
    }
    if (output_route)
        audio_route_apply_path(adev->ar, output_route);
    if (input_route)
        audio_route_apply_path(adev->ar, input_route);

    audio_route_update_mixer(adev->ar);
}

static void select_output_device(struct sunxi_audio_device *adev)
{
    UNUSED(adev);
}

static void select_input_device(struct sunxi_audio_device *adev)
{
    UNUSED(adev);
}

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
                               (card == adev->cardMIX)  ? "ahub audio format Function" : "spdif audio format Function";
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

/* must be called with hw device and output stream mutexes locked */

static int start_output_stream(struct sunxi_stream_out *out)
{
    struct sunxi_audio_device *adev = out->dev;
    unsigned int card = adev->cardCODEC;
    unsigned int port = PORT_CODEC;
    unsigned int index = 0;

    char prop_value_card[PROPERTY_VALUE_MAX] = {0};
    ALOGD("start_output_stream   out->format : 0x%08x", out->format);
    //standby all none AUDIO_FORMAT_AC3 or AUDIO_FORMAT_E_AC3 or DTS output 
    if(out->flags & AUDIO_OUTPUT_FLAG_DIRECT)
    {
        adev->raw_flag = true;
        if (adev->active_output && ((adev->active_output->flags & AUDIO_OUTPUT_FLAG_DIRECT) == 0))
        {
            ALOGE("adev->active_output(%p) format : %d standby",adev->active_output,adev->active_output->format);
            //Must not be himself(active_output != out)
            struct sunxi_stream_out * active_output = adev->active_output;
            pthread_mutex_lock(&active_output->lock);
            do_output_standby(active_output);
            pthread_mutex_unlock(&active_output->lock);
            active_output = NULL;
        }
    }

    if (adev->mode == AUDIO_MODE_IN_CALL || adev->mode == AUDIO_MODE_MODE_FACTORY_TEST || adev->mode == AUDIO_MODE_FM) // 10-16 modify
    //  if ((adev->mode == AUDIO_MODE_IN_CALL&&adev->out_device==AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET)||adev->mode == AUDIO_MODE_MODE_FACTORY_TEST || adev->mode == AUDIO_MODE_FM)
    {
        #if call_button_voice
            if (adev->mode == AUDIO_MODE_IN_CALL&&adev->out_device==AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET) {
                ALOGV("***********************adev->out_device:%d*************",adev->out_device);
            } else {

                ALOGW("mode in call, do not start stream");
                return 0;
            }
        #else
            ALOGW("mode in call, do not start stream");
            return 0;
        #endif
    }
    if ((adev->raw_flag && ((out->flags & AUDIO_OUTPUT_FLAG_DIRECT) == 0)) || adev->direct_mode)
    {
        return 0;
    }

    int device = adev->out_device;
    char prop_value[512];
    int ret = property_get("audio.routing", prop_value, "");
    if (ret > 0)
    {
        if(atoi(prop_value) == AUDIO_DEVICE_OUT_SPEAKER)
        {
            ALOGD("start_output_stream, AUDIO_DEVICE_OUT_SPEAKER");
            device = AUDIO_DEVICE_OUT_SPEAKER;
        }
        else if(atoi(prop_value) == AUDIO_DEVICE_OUT_AUX_DIGITAL)
        {
            ALOGD("start_output_stream AUDIO_DEVICE_OUT_AUX_DIGITAL");
            device = AUDIO_DEVICE_OUT_AUX_DIGITAL;
        }
        else if(atoi(prop_value) == AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET)
        {
            ALOGD("start_output_stream AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET");
            device = AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET;
        }
        else
        {
            ALOGW("unknown audio.routing : %s", prop_value);
        }
    }
    else
    {
        // ALOGW("get audio.routing failed");
    }

    adev->out_device = device;
    adev->active_output = out;
    if (adev->mode != AUDIO_MODE_IN_CALL) {
        F_LOG;
        /* FIXME: only works if only one output can be active at a time */
        select_output_device(adev);
    } else {
    #if call_button_voice
    #if 1
        if (adev->out_device == AUDIO_DEVICE_OUT_EARPIECE) {
            F_LOG;
            adev->out_device = AUDIO_DEVICE_OUT_SPEAKER;
        }
    #endif
        normal_play_route(adev->out_device);
    #endif
    }
    /* S/PDIF takes priority over HDMI audio. In the case of multiple
     * devices, this will cause use of S/PDIF or HDMI only */
    if(adev->raw_flag || (out->flags & AUDIO_OUTPUT_FLAG_DIRECT))
    {
        //do nothing for the config has been setted
    }
    else
    {
        out->config.rate = MM_SAMPLING_RATE;
        if (adev->out_device & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET) {
            card = adev->cardSPDIF;
            port = PORT_SPDIF;
        }
        else if(adev->out_device & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            card = adev->cardHDMI;
            port = PORT_HDMI;
            out->config.rate = MM_SAMPLING_RATE;
        }
        /* default to low power: will be corrected in out_write if necessary before first write to
                * tinyalsa.
              */
        out->config.start_threshold = SHORT_PERIOD_SIZE * 2;
        out->config.avail_min = SHORT_PERIOD_SIZE;
    }
    out->write_threshold = 0;// SHORT_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
    if(adev->out_device & AUDIO_DEVICE_OUT_ALL_SCO)
    {
        out->multi_config[adev->cardBT] = pcm_config_mm_out;
        out->multi_config[adev->cardBT].channels = 1;
        out->multi_config[adev->cardBT].rate = SAMPLING_RATE_8K;
        out->multi_config[adev->cardBT].start_threshold = 0;
        out->multi_config[adev->cardBT].avail_min = 0;
        out->multi_config[adev->cardBT].stop_threshold = 0;
        out->multi_config[adev->cardBT].silence_threshold = 0;
        adev->dev_manager[adev->cardBT].flag_out = AUDIO_OUT;
        adev->dev_manager[adev->cardBT].flag_out_active = 1;

        out->multi_pcm[adev->cardBT] = pcm_open(adev->cardBT, PORT_DAUDIO1, PCM_OUT, &out->multi_config[adev->cardBT]);
        if(!pcm_is_ready(out->multi_pcm[adev->cardBT]))
        {
            ALOGE("cannot open pcm driver: %s", pcm_get_error(out->multi_pcm[adev->cardBT]));
            pcm_close(out->multi_pcm[adev->cardBT]);
            out->multi_pcm[adev->cardBT] = NULL;
            adev->active_output = NULL;
            return -ENOMEM;
        }
        if(DEFAULT_OUT_SAMPLING_RATE != out->multi_config[adev->cardBT].rate)
        {
            ret = create_resampler(DEFAULT_OUT_SAMPLING_RATE,
                        out->multi_config[adev->cardBT].rate,
                        2,
                        RESAMPLER_QUALITY_DEFAULT,
                        NULL,
                        &out->multi_resampler[adev->cardBT]);
            if(ret != 0)
            {
                ALOGE("create out resampler failed, %d -> %d", DEFAULT_OUT_SAMPLING_RATE, out->multi_config[adev->cardBT].rate);
                return ret;
            }
            ALOGV("create out resampler OK, %d -> %d", DEFAULT_OUT_SAMPLING_RATE, out->multi_config[adev->cardBT].rate);
        }
        else
        {
            ALOGV("do not use out resampler");
        }
        if(out->multi_resampler[adev->cardBT])
        {
            out->multi_resampler[adev->cardBT]->reset(out->multi_resampler[adev->cardBT]);
        }
    }
    else
    {
        for (index = 0; index < MAX_AUDIO_DEVICES; index++)
        {
            if (adev->dev_manager[index].flag_exist
                && (adev->dev_manager[index].flag_out == AUDIO_OUT)
                && adev->dev_manager[index].flag_out_active)
            {
                card = index;
                ALOGV("use %s to playback audio", adev->dev_manager[index].name);
                out->multi_config[card] = pcm_config_mm_out;
                out->multi_config[card].rate = MM_SAMPLING_RATE;
                    out->multi_config[card].start_threshold = 0;//SHORT_PERIOD_SIZE * 2;
                out->multi_config[card].start_threshold = SHORT_PERIOD_SIZE * 2;
                out->multi_config[card].avail_min = LONG_PERIOD_SIZE;
                out->multi_config[card].raw_flag = 1;

                if(adev->raw_flag || (out->flags & AUDIO_OUTPUT_FLAG_DIRECT))
                {
                    property_get(PROP_RAWDATA_KEY, prop_value_card, PROP_RAWDATA_DEFAULT_VALUE);
                    if(card == adev->cardHDMI && !strcmp(prop_value_card, PROP_RAWDATA_MODE_HDMI_RAW))
                    {
                        ALOGD("card[%s] use hdmi pcm config, spr : %d",prop_value_card, out->config.rate);
                        out->multi_config[card] = out->config;
                    }
                    else if(card == adev->cardSPDIF && !strcmp(prop_value_card, PROP_RAWDATA_MODE_SPDIF_RAW))
                    {
                        ALOGD("card[%s] use spdif pcm config",prop_value_card);
                        out->multi_config[card] = out->config;
                    }
                }

                if(!strncmp(adev->dev_manager[index].name, "AUDIO_USB", 9)){
                    get_output_device_params(card, out);
                }
#if AUDIO_USE_AC100
                if(card == adev->cardCODEC){
                    if(adev->ar == NULL)
                        adev->ar = audio_route_init(adev->cardCODEC, AC100_XML_PATH);
                    init_ac100_input_output_path(adev,OUT_DEVICE_SPEAKER_AND_HEADSET,IN_SOURCE_HEADSETMIC);
                }
#endif
                struct pcm_config *config = &out->multi_config[card];
                ALOGD("open card=%d, port=%d", card, port);
                ALOGD("config:%d %d %d %d %d %d %d %d", config->channels, config->rate, config->period_size, config->period_count,
                    config->format, config->avail_min, config->raw_flag, config->in_init_channels);
                set_raw_flag(adev, card, config->raw_flag);
                if(adev->raw_flag || (out->flags & AUDIO_OUTPUT_FLAG_DIRECT))
                {
                    property_get(PROP_RAWDATA_KEY, prop_value_card, PROP_RAWDATA_DEFAULT_VALUE);
                    if(card == adev->cardHDMI && !strcmp(prop_value_card, PROP_RAWDATA_MODE_HDMI_RAW))
                    {
                        ALOGD("ahub use hdmi pcm config, hdmicard : %d, spr : %d",adev->cardHDMI, out->config.rate);
                        out->multi_config[card] = out->config;
                    }
                }

                out->multi_pcm[card] = pcm_open(card, port, PCM_OUT | PCM_MONOTONIC, &out->multi_config[card]);
                if (!pcm_is_ready(out->multi_pcm[card])) {
                    ALOGE("cannot open pcm driver: %s", pcm_get_error(out->multi_pcm[card]));
                    pcm_close(out->multi_pcm[card]);
                    out->multi_pcm[card] = NULL;
                    adev->active_output = NULL;
                    return -ENOMEM;
                }

                if (adev->echo_reference != NULL)
                    out->echo_reference = adev->echo_reference;

                if (DEFAULT_OUT_SAMPLING_RATE != out->multi_config[card].rate
                    && ((out->flags & AUDIO_OUTPUT_FLAG_DIRECT) == 0)
                )
                {
                    ret = create_resampler(DEFAULT_OUT_SAMPLING_RATE,
                                                out->multi_config[card].rate,
                                                2,
                                                RESAMPLER_QUALITY_DEFAULT,
                                                NULL,
                                                &out->multi_resampler[card]);
                    if (ret != 0)
                    {
                        ALOGE("create out resampler failed, %d -> %d", DEFAULT_OUT_SAMPLING_RATE, out->multi_config[card].rate);
                        return ret;
                    }
                    ALOGV("create out resampler OK, %d -> %d", DEFAULT_OUT_SAMPLING_RATE, out->multi_config[card].rate);
                }
                else
                {
                    ALOGV("do not use out resampler");
                }

                if (out->multi_resampler[card])
                {
                    out->multi_resampler[card]->reset(out->multi_resampler[card]);
                }
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

    switch(sample_rate) {
    case 8000:
    case 11025:
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
    ALOGD("sample_rate = %d",sample_rate);
    if (check_input_parameters(sample_rate, format, channel_count) != 0)
        return 0;

    /* take resampling into account and return the closest majoring
    multiple of 16 frames, as audioflinger expects audio buffers to
    be a multiple of 16 frames */
    //size = (pcm_config_mm_in.period_size * sample_rate) / pcm_config_mm_in.rate;
    size = pcm_config_mm_in.period_size;
    size = ((size + 15) / 16) * 16;

    return size * channel_count * sizeof(short);
}

static void add_echo_reference(struct sunxi_stream_out *out,
                               struct echo_reference_itfe *reference)
{
    pthread_mutex_lock(&out->lock);
    out->echo_reference = reference;
    pthread_mutex_unlock(&out->lock);
}

static void remove_echo_reference(struct sunxi_stream_out *out,
                                  struct echo_reference_itfe *reference)
{
    pthread_mutex_lock(&out->lock);
    if (out->echo_reference == reference) {
        /* stop writing to echo reference */
        reference->write(reference, NULL);
        out->echo_reference = NULL;
    }
    pthread_mutex_unlock(&out->lock);
}

static void put_echo_reference(struct sunxi_audio_device *adev,
                          struct echo_reference_itfe *reference)
{
    if (adev->echo_reference != NULL &&
            reference == adev->echo_reference) {
        if (adev->active_output != NULL)
            remove_echo_reference(adev->active_output, reference);
        release_echo_reference(reference);
        adev->echo_reference = NULL;
    }
}

static struct echo_reference_itfe *get_echo_reference(struct sunxi_audio_device *adev,
                                               audio_format_t format,
                                               uint32_t channel_count,
                                               uint32_t sampling_rate)
{
    UNUSED(format);
    put_echo_reference(adev, adev->echo_reference);
    if (adev->active_output != NULL) {
        struct audio_stream *stream = &adev->active_output->stream.common;
        uint32_t wr_channel_count = popcount(stream->get_channels(stream));
        uint32_t wr_sampling_rate = stream->get_sample_rate(stream);

        int status = create_echo_reference(AUDIO_FORMAT_PCM_16_BIT,
                                           channel_count,
                                           sampling_rate,
                                           AUDIO_FORMAT_PCM_16_BIT,
                                           wr_channel_count,
                                           wr_sampling_rate,
                                           &adev->echo_reference);
        if (status == 0)
            add_echo_reference(adev->active_output, adev->echo_reference);
    }
    return adev->echo_reference;
}

static int get_playback_delay(struct sunxi_stream_out *out,
                       size_t frames,
                       struct echo_reference_buffer *buffer)
{
    struct sunxi_audio_device *adev = out->dev;
    unsigned int kernel_frames = 0;
    int status;
    int index;
    int card;

    for (index = 0; index < MAX_AUDIO_DEVICES; index++)
    {
        if (adev->dev_manager[index].flag_exist
            && (adev->dev_manager[index].flag_out == AUDIO_OUT)
            && adev->dev_manager[index].flag_out_active
            && adev->raw_flag == 0)
        {
            card = index;

            status = pcm_get_htimestamp(out->multi_pcm[card], &kernel_frames, &buffer->time_stamp);
            if (status < 0) {
                buffer->time_stamp.tv_sec  = 0;
                buffer->time_stamp.tv_nsec = 0;
                buffer->delay_ns           = 0;
                ALOGV("get_playback_delay(): pcm_get_htimestamp error,"
                        "setting playbackTimestamp to 0");
                return status;
            }

            kernel_frames = pcm_get_buffer_size(out->multi_pcm[card]) - kernel_frames;
            break;
        }

    }

    /* adjust render time stamp with delay added by current driver buffer.
     * Add the duration of current frame as we want the render time of the last
     * sample being written. */
    buffer->delay_ns = (long)(((int64_t)(kernel_frames + frames)* 1000000000)/
                            MM_SAMPLING_RATE);

    return 0;
}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    uint32_t  outrate = DEFAULT_OUT_SAMPLING_RATE;
    if(out->sample_rate != 0)
        outrate = out->sample_rate;
    return outrate;
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
    size_t size = 0;

    /* take resampling into account and return the closest majoring
    multiple of 16 frames, as audioflinger expects audio buffers to
    be a multiple of 16 frames */
    if(out->flags & AUDIO_OUTPUT_FLAG_DIRECT)
        size = out->config.period_size;
    else
        size = (SHORT_PERIOD_SIZE * DEFAULT_OUT_SAMPLING_RATE) / out->config.rate;

    size = ((size + 15) / 16) * 16;
    //return size * audio_stream_frame_size((struct audio_stream *)stream);
    return size * audio_stream_out_frame_size((const struct audio_stream_out *)out);
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    audio_channel_mask_t outchan = AUDIO_CHANNEL_OUT_STEREO;
    if(out->channel_mask != AUDIO_CHANNEL_NONE)
        outchan = out->channel_mask;
    return outchan;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    UNUSED(stream);
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    audio_format_t outformat = AUDIO_FORMAT_PCM_16_BIT;
    if(out->format != AUDIO_FORMAT_DEFAULT)
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
    int index = 0;
    if (!out->standby) {
        if (out->pcm)
        {
            pcm_close(out->pcm);
            out->pcm = NULL;
        }

        if (out->resampler)
        {
            release_resampler(out->resampler);
            out->resampler = NULL;
        }

        for (index = 0; index < MAX_AUDIO_DEVICES; index++)
        {
            if (out->multi_pcm[index])
            {
                pcm_close(out->multi_pcm[index]);
                out->multi_pcm[index] = NULL;
            }

            if (out->multi_resampler[index])
            {
                release_resampler(out->multi_resampler[index]);
                out->multi_resampler[index] = NULL;
            }
        }

        adev->active_output = 0;
        /* stop writing to echo reference */
        if (out->echo_reference != NULL) {
            out->echo_reference->write(out->echo_reference, NULL);
            out->echo_reference = NULL;
        }

        if(out->flags & AUDIO_OUTPUT_FLAG_DIRECT)
        {
            adev->raw_flag = false;
        }

        out->standby = 1;
    }
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    int status;
ALOGD("out_standby");
    //pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);
    status = do_output_standby(out);
    pthread_mutex_unlock(&out->lock);
    //pthread_mutex_unlock(&out->dev->lock);
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
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    struct sunxi_stream_in *in;
    struct str_parms *parms;
    char value[128];
    int ret, val = 0;
    bool force_input_standby = false;

    parms = str_parms_create_str(kvpairs);

    ALOGV("out_set_parameters: %s", kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        if (adev->first_set_audio_routing)
        {
            // we do not use the android default routing
            // init audio routing by fuction init_audio_devices_active()
            adev->first_set_audio_routing= false;
            return ret;
        }
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);
        if ((adev->out_device  != val) && (val != 0)) {
            if (out == adev->active_output) {
                F_LOG;
                /* a change in output device may change the microphone selection */
                if (adev->active_input &&
                        adev->active_input->source == AUDIO_SOURCE_VOICE_COMMUNICATION) {
                    force_input_standby = true;
                }
                /* force standby if moving to/from HDMI */
                if (((val & AUDIO_DEVICE_OUT_AUX_DIGITAL) ^
                        (adev->out_device & AUDIO_DEVICE_OUT_AUX_DIGITAL)) ||
                        (adev->out_device & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET) ||
                        ((val & AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET) ^
                        (adev->out_device & AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET)))
                    do_output_standby(out);
            }
            adev->out_device = val;
            F_LOG;
            select_output_device(adev);
            if (adev->mode == AUDIO_MODE_IN_CALL || adev->mode == AUDIO_MODE_MODE_FACTORY_TEST || adev->mode == AUDIO_MODE_FM){
                adev_set_voice_volume(&adev->hw_device, adev->voice_volume);
            }
        }
        pthread_mutex_unlock(&out->lock);
        if (force_input_standby) {
            in = adev->active_input;
            pthread_mutex_lock(&in->lock);
            do_input_standby(in);
            pthread_mutex_unlock(&in->lock);
        }
        pthread_mutex_unlock(&adev->lock);
    }

    // set audio out device
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICES_OUT_ACTIVE, value, sizeof(value));
    if (ret >= 0)
    {
        ALOGV("out AUDIO_PARAMETER_DEVICES_OUT_ACTIVE: %s", value);

        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);

        if (adev->raw_flag == true)
        {
            ALOGW("in raw mode, should not set other audio out devices");
            pthread_mutex_unlock(&out->lock);
            pthread_mutex_unlock(&adev->lock);
            return -1;
        }

        set_audio_devices_active(adev, AUDIO_OUT, value);
        //strcpy(adev->out_device_active_req, value);

        do_output_standby(out);
        select_output_device(adev);
        pthread_mutex_unlock(&out->lock);
        pthread_mutex_unlock(&adev->lock);
    }
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_RAW_DATA_OUT, value, sizeof(value));
    if (ret >= 0)
    {
        bool bval = (atoi(value) == 1) ? true : false;
        ALOGV("AUDIO_PARAMETER_RAW_DATA_OUT: %d", bval);
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);
        if (adev->raw_flag != bval)
        {
            adev->raw_flag = bval;
            do_output_standby(out);
        }
        pthread_mutex_unlock(&out->lock);
        pthread_mutex_unlock(&adev->lock);
    }

    str_parms_destroy(parms);
    return ret;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    UNUSED(stream);
    UNUSED(keys);
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    uint32_t latency = 0;
    if(out->flags & AUDIO_OUTPUT_FLAG_DIRECT)
        latency = (out->config.period_size * out->config.period_count * 1000) / out->config.rate;
    else
        latency = (SHORT_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT * 1000) / out->config.rate;
    return latency;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    UNUSED(stream);
    UNUSED(left);
    UNUSED(right);
    return -ENOSYS;
}
#if 0
static int64_t __systemTime()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec*1000000000LL + t.tv_nsec;
}
#endif
typedef enum IEC61937DataType {
    IEC61937_AC3                = 0x01,          ///< AC-3 data
    IEC61937_MPEG1_LAYER1       = 0x04,          ///< MPEG-1 layer 1
    IEC61937_MPEG1_LAYER23      = 0x05,          ///< MPEG-1 layer 2 or 3 data or MPEG-2 without extension
    IEC61937_MPEG2_EXT          = 0x06,          ///< MPEG-2 data with extension
    IEC61937_MPEG2_AAC          = 0x07,          ///< MPEG-2 AAC ADTS
    IEC61937_MPEG2_LAYER1_LSF   = 0x08,          ///< MPEG-2, layer-1 low sampling frequency
    IEC61937_MPEG2_LAYER2_LSF   = 0x09,          ///< MPEG-2, layer-2 low sampling frequency
    IEC61937_MPEG2_LAYER3_LSF   = 0x0A,          ///< MPEG-2, layer-3 low sampling frequency
    IEC61937_DTS1               = 0x0B,          ///< DTS type I   (512 samples)
    IEC61937_DTS2               = 0x0C,          ///< DTS type II  (1024 samples)
    IEC61937_DTS3               = 0x0D,          ///< DTS type III (2048 samples)
    IEC61937_ATRAC              = 0x0E,          ///< ATRAC data
    IEC61937_ATRAC3             = 0x0F,          ///< ATRAC3 data
    IEC61937_ATRACX             = 0x10,          ///< ATRAC3+ data
    IEC61937_DTSHD              = 0x11,          ///< DTS HD data
    IEC61937_WMAPRO             = 0x12,          ///< WMA 9 Professional data
    IEC61937_MPEG2_AAC_LSF_2048 = 0x13,          ///< MPEG-2 AAC ADTS half-rate low sampling frequency
    IEC61937_MPEG2_AAC_LSF_4096 = 0x13 | 0x20,   ///< MPEG-2 AAC ADTS quarter-rate low sampling frequency
    IEC61937_EAC3               = 0x15,          ///< E-AC-3 data
    IEC61937_TRUEHD             = 0x16,          ///< TrueHD data
}IEC61937DataType;

static bool detectIEC61937(struct sunxi_stream_out *out, const void * audio_buf, size_t bytes)
{
   unsigned int tansfer_chanels = 2;
   unsigned int sample_rate = 48000;
   audio_format_t format = AUDIO_FORMAT_PCM;
   if(bytes < 6)
   {
     return false;
   }
   char *buf = (char *)audio_buf;
   unsigned int i = 0;
   while(i < (bytes - 3)){
       if(buf[i] == 0x72 && buf[i+1] == 0xF8 && buf[i+2] == 0x1F && buf[i+3] == 0x4E)
       {
           ALOGV("Detect a iec header!!, byte : %d", bytes);
           break;
       }
       i++;
   }
   if(i == (bytes - 3))
       return false;

   char type = buf[i + 4] & 0xff;
   switch(type)
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
   out->config.rate = sample_rate;
   out->config.channels = tansfer_chanels;
   out->config.raw_flag = (format == AUDIO_FORMAT_AC3)?2:
                          (format == AUDIO_FORMAT_E_AC3)?10:
                          (format == AUDIO_FORMAT_DTS_HD)?11:
                          (format == AUDIO_FORMAT_DTS)?7:1;
   ALOGV("rate : %d, channels : %d, raw_flag : %d", out->config.rate, out->config.channels ,out->config.raw_flag);
   return true;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    int ret = 0;
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    //size_t frame_size = audio_stream_frame_size(&out->stream.common);
    size_t frame_size = audio_stream_out_frame_size((const struct audio_stream_out *)out);
    size_t in_frames = bytes / frame_size;
    size_t out_frames = RESAMPLER_BUFFER_SIZE / frame_size;
    bool force_input_standby = false;
    struct sunxi_stream_in *in;
    void *buf;
    int index;
    int card;
    char prop_value_card[PROPERTY_VALUE_MAX] = {0};

    if (adev->mode == AUDIO_MODE_IN_CALL || adev->mode == AUDIO_MODE_MODE_FACTORY_TEST || adev->mode == AUDIO_MODE_FM)   //10-16 modify
    {
        #if call_button_voice
        if ((adev->mode == AUDIO_MODE_IN_CALL)&&(adev->out_device==AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET)) {
        } else {
            ALOGD("mode in call, do not out_write");
            return bytes;
        }
        #else
            ALOGD("mode in call, do not out_write");
            return bytes;
        #endif
    }

if((out->flags & AUDIO_OUTPUT_FLAG_DIRECT) && (out->format == AUDIO_FORMAT_IEC61937) && (!adev->parse_iec61937))
{
    bool bok = detectIEC61937(out,buffer,bytes);
    ALOGV("### detectIEC61937: %d",bok);
    if(bok){
        adev->raw_flag = true;
        if (adev->active_output)
        {
            ALOGV("adev->active_output(%p) format : %d standby",adev->active_output,adev->active_output->format);
            //Must not be himself(active_output != out)
            struct sunxi_stream_out * active_output = adev->active_output;
            pthread_mutex_lock(&active_output->lock);
            do_output_standby(active_output);
            pthread_mutex_unlock(&active_output->lock);
            active_output = NULL;
        }
        adev->parse_iec61937 = true;
    }
}

    if (adev->direct_mode)
    {
        if (!out->standby) {
            out_standby(&(out->stream.common));
        }
        return bytes;
    }
    if (adev->raw_flag && ((out->flags & AUDIO_OUTPUT_FLAG_DIRECT) == 0))
    {
        return 0;//Hold on mixthread audio data for async between mixthread and directthread
    }

    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the output stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&out->lock);
    if (out->standby) {
        ret = start_output_stream(out);
        if (ret != 0) {
            pthread_mutex_unlock(&adev->lock);
            goto exit;
        }
        out->standby = 0;
        /* a change in output device may change the microphone selection */
        if (adev->active_input &&
                adev->active_input->source == AUDIO_SOURCE_VOICE_COMMUNICATION)
            force_input_standby = true;
    }
    pthread_mutex_unlock(&adev->lock);

    out->write_threshold = SHORT_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
    out->config.avail_min = SHORT_PERIOD_SIZE;

    if (adev->af_capture_flag && adev->PcmManager.BufExist) {
        WritePcmData((void *)buffer, out_frames * frame_size, &adev->PcmManager);
        memset((void *)buffer, 0, out_frames * frame_size); //mute
    }
    for (index = MAX_AUDIO_DEVICES; index >= 0; index--)
    {
        if (adev->dev_manager[index].flag_exist
            && (adev->dev_manager[index].flag_out == AUDIO_OUT)
            && adev->dev_manager[index].flag_out_active
            && out->multi_pcm[index] != NULL)
        {
            card = index;
            if(!adev->raw_flag || !(out->flags & AUDIO_OUTPUT_FLAG_DIRECT))
            {
                out->multi_config[card].avail_min = SHORT_PERIOD_SIZE;
            }
            pcm_set_avail_min(out->multi_pcm[card], out->multi_config[card].avail_min);

            if (out->multi_resampler[card]) {
                out->multi_resampler[card]->resample_from_input(out->multi_resampler[card],
                                                                (int16_t *)buffer,
                                                                &in_frames,
                                                                (int16_t *)out->buffer,
                                                                &out_frames);
                buf = out->buffer;
            } else {
                out_frames = in_frames;
                buf = (void *)buffer;
            }

            if (out->echo_reference != NULL) {
                struct echo_reference_buffer b;
                b.raw = (void *)buffer;
                b.frame_count = in_frames;

                get_playback_delay(out, out_frames, &b);
                out->echo_reference->write(out->echo_reference, &b);
            }

            if(adev->raw_flag || (out->flags & AUDIO_OUTPUT_FLAG_DIRECT))//TODO: strong the condition of direct out put  
            {
                property_get(PROP_RAWDATA_KEY, prop_value_card, PROP_RAWDATA_DEFAULT_VALUE);
                if(card == adev->cardHDMI && !strcmp(prop_value_card, PROP_RAWDATA_MODE_HDMI_RAW))
                {
                    //add by khan : for 24bit pcm, we should extend the highest 8 bit with signed
                    if (out->format == AUDIO_FORMAT_PCM_24_BIT_PACKED)
                    {
                        int idx = 0, jdx = 0;
                        char* dst = NULL;
                        char* src = NULL;
                        char* buf_conv = NULL;
                        int   ch  = out->multi_config[card].channels;
                        int   write_len = out_frames * frame_size;
                        int   buf_conv_len = 0;
                        int  samples = write_len / audio_bytes_per_sample(out->format) / ch;
                        buf_conv_len = write_len*4/3;
                        buf_conv = (char*)malloc(buf_conv_len);
                        if(buf_conv == NULL)
                        {
                            ALOGE("No memory while malloc conventer buffer...");
                            return 0;
                        }

                        dst = (char*)buf_conv;
                        for(idx = 0; idx < ch; idx++)
                        {
                            dst = ((char*)buf_conv) + 4*idx;
                            src = ((char*)buf) + 3*idx;
                            for(jdx = 0; jdx < samples; jdx++){
                                dst[0] = src[0]&0xff;
                                dst[1] = src[1]&0xff;
                                dst[2] = src[2]&0xff;
                                dst[3] = (dst[2]&0x80)?0xff:0x00;
                                dst += 4*ch;
                                src += 3*ch;
                            }
                        }
                        ret = pcm_write(out->multi_pcm[card], (void *)buf_conv, buf_conv_len);
                        if(buf_conv != NULL)
                        {
                            free(buf_conv);
                            buf_conv = NULL;
                        }
                    }
                    else
                    {
                        ret = pcm_write(out->multi_pcm[card], (void *)buf, out_frames * frame_size);
                    }
                }
                else if(card == adev->cardSPDIF && !strcmp(prop_value_card, PROP_RAWDATA_MODE_SPDIF_RAW))
                {
                    if (out->multi_config[card].channels == 2)
                    {
                        if (out->format == AUDIO_FORMAT_PCM_24_BIT_PACKED)
                        {
                            int idx = 0, jdx = 0;
                            char* dst = NULL;
                            char* src = NULL;
                            char* buf_conv = NULL;
                            int   ch  = out->multi_config[card].channels;
                            int   write_len = out_frames * frame_size;
                            int   buf_conv_len = 0;
                            int  samples = write_len / audio_bytes_per_sample(out->format) / ch;
                            buf_conv_len = write_len*4/3;

                            buf_conv = (char*)malloc(buf_conv_len);
                            if(buf_conv == NULL)
                            {
                                ALOGE("No memory while malloc conventer buffer...");
                                return 0;
                            }

                            dst = (char*)buf_conv;
                            for(idx = 0; idx < ch; idx++)
                            {
                                dst = ((char*)buf_conv) + 4*idx;
                                src = ((char*)buf) + 3*idx;
                                for(jdx = 0; jdx < samples; jdx++){
                                    dst[0] = src[0]&0xff;
                                    dst[1] = src[1]&0xff;
                                    dst[2] = src[2]&0xff;
                                    dst[3] = (dst[2]&0x80)?0xff:0x00;
                                    dst += 4*ch;
                                    src += 3*ch;
                                }
                            }
                            ret = pcm_write(out->multi_pcm[card], (void *)buf_conv, buf_conv_len);
                            if(buf_conv != NULL)
                            {
                                free(buf_conv);
                                buf_conv = NULL;
                            }
                        }
                        else
                        {
                            ret = pcm_write(out->multi_pcm[card], (void *)buf, out_frames * frame_size);
                        }
                    }
                }
            }
            else
            {
                if (out->multi_config[index].channels == 2)
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
            }
            if(ret!=0){
                ALOGE("##############out_write()  Warning:write fail, reopen it ret = %d #######################", ret);
                do_output_standby(out);
                usleep(30000);
                break;
            }
        }
    }

exit:
    pthread_mutex_unlock(&out->lock);

    if (ret != 0) {
        //usleep(bytes * 1000000 / audio_stream_frame_size(&stream->common) / out_get_sample_rate(&stream->common));
        usleep(bytes * 1000000 / audio_stream_out_frame_size((const struct audio_stream_out *)out) / out_get_sample_rate(&stream->common));
    } else {
        out->written += bytes / (out->config.channels * sizeof(short));
    }

    if (force_input_standby) {
        pthread_mutex_lock(&adev->lock);
        if (adev->active_input) {
            in = adev->active_input;
            pthread_mutex_lock(&in->lock);
            do_input_standby(in);
            pthread_mutex_unlock(&in->lock);
        }
        pthread_mutex_unlock(&adev->lock);
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

#if 0
static int out_get_presentation_position(const struct audio_stream_out *stream,
                                   uint64_t *frames, struct timespec *timestamp)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    unsigned int kernel_frames;
    int status;
    int index;
    int card;
    int ret = -1;

    pthread_mutex_lock(&out->lock);
    for (index = 0; index < MAX_AUDIO_DEVICES; index++)
    {
        if (adev->dev_manager[index].flag_exist
            && (adev->dev_manager[index].flag_out == AUDIO_OUT)
            && adev->dev_manager[index].flag_out_active
            && adev->raw_flag == 0)
        {
            card = index;

            status = pcm_get_htimestamp(out->multi_pcm[card], &kernel_frames, timestamp);
            //ALOGD("time:%d,%d", timestamp->tv_sec, timestamp->tv_nsec);
            if (status == 0) {
                int64_t hw_frames = out->written - pcm_get_buffer_size(out->multi_pcm[card]) + kernel_frames;
                if (hw_frames >= 0) {
                    *frames = hw_frames;
                    ret = 0;
                }
                break;
            }
        }
    }
    pthread_mutex_unlock(&out->lock);
    return ret;
}
#endif
/** audio_stream_in implementation **/

static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                                   struct resampler_buffer* buffer);
static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                                  struct resampler_buffer* buffer);

/* should be in asoundlib.h */
struct pcm *pcm_open_req(unsigned int card, unsigned int device,
                     unsigned int flags, struct pcm_config *config, int requested_rate);
/* must be called with hw device and input stream mutexes locked */
static int start_input_stream(struct sunxi_stream_in *in)
{
    int ret = 0;
    struct sunxi_audio_device *adev = in->dev;

    adev->active_input = in;

    if (adev->mode == AUDIO_MODE_IN_CALL) { // && adev->bluetooth_voice
    ALOGD("in call mode , start_input_stream, return");
    return 0;
    }

    if (adev->mode != AUDIO_MODE_IN_CALL) {
        adev->in_device = in->device;
        select_input_device(adev);
    }

    if (in->need_echo_reference && in->echo_reference == NULL)
        in->echo_reference = get_echo_reference(adev,
                                        AUDIO_FORMAT_PCM_16_BIT,
                                        in->config.channels,
                                        in->requested_rate);

    int in_ajust_rate = in->requested_rate;
    // out/in stream should be both 44.1K serial
    if (!(in->requested_rate % SAMPLING_RATE_11K))
    {
        // OK
        in_ajust_rate = in->requested_rate;
    }
    else
    {
        in_ajust_rate = SAMPLING_RATE_11K * in->requested_rate / SAMPLING_RATE_8K;
        if (in_ajust_rate > SAMPLING_RATE_44K)
        {
            in_ajust_rate = SAMPLING_RATE_44K;
        }
        ALOGV("out/in stream should be both 44.1K serial, force capture rate: %d", in_ajust_rate);
    }

    int card = 0;
    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (adev->dev_manager[card].flag_exist
            && (adev->dev_manager[card].flag_in == AUDIO_IN)
            && adev->dev_manager[card].flag_in_active)
        {
            ALOGV("use %s to capture audio", adev->dev_manager[card].name);
            break;
        }
    }

    int port = PORT_CODEC;
    if(in->device & AUDIO_DEVICE_IN_ALL_SCO)
    {
        card = adev->cardBT;
        port = PORT_DAUDIO1;
        in->config.channels = 1;
        in->config.in_init_channels = 1;
        in_ajust_rate = SAMPLING_RATE_8K;
    }
    ALOGD("audio_devide_in card = %d",card);
    in->pcm = pcm_open_req(card, PORT_CODEC, PCM_IN, &in->config, in_ajust_rate);

    if (!pcm_is_ready(in->pcm)) {
        ALOGE("cannot open pcm_in driver: %s", pcm_get_error(in->pcm));
        pcm_close(in->pcm);
        adev->active_input = NULL;
        return -ENOMEM;
    }
    if (in->requested_rate != in->config.rate) {
        in->buf_provider.get_next_buffer = get_next_buffer;
        in->buf_provider.release_buffer = release_buffer;

        ret = create_resampler(in->config.rate,
                               in->requested_rate,
                               in->config.channels,
                               RESAMPLER_QUALITY_DEFAULT,
                               &in->buf_provider,
                               &in->resampler);
        if (ret != 0) {
            ALOGE("create in resampler failed, %d -> %d", in->config.rate, in->requested_rate);
            ret = -EINVAL;
            goto err;
        }

        ALOGV("create in resampler OK, %d -> %d", in->config.rate, in->requested_rate);
    }
    else
    {
        ALOGV("do not use in resampler");
    }

    /* if no supported sample rate is available, use the resampler */
    if (in->resampler) {
        in->resampler->reset(in->resampler);
        in->frames_in = 0;
    }
    return 0;

err:
    if (in->resampler) {
        release_resampler(in->resampler);
    }

    return -1;
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

    if (in->config.channels == 1) {
        return AUDIO_CHANNEL_IN_MONO;
    } else {
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

    if (!in->standby) {
        pcm_close(in->pcm);
        in->pcm = NULL;

        adev->active_input = 0;
        if (adev->mode != AUDIO_MODE_IN_CALL) {
            adev->in_device = AUDIO_DEVICE_NONE;
            select_input_device(adev);
        }

        if (in->echo_reference != NULL) {
            /* stop reading from echo reference */
            in->echo_reference->read(in->echo_reference, NULL);
            put_echo_reference(adev, in->echo_reference);
            in->echo_reference = NULL;
        }

        if (in->resampler) {
            release_resampler(in->resampler);
            in->resampler = NULL;
        }

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
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    struct sunxi_audio_device *adev = in->dev;
    struct str_parms *parms;
    char value[128];
    int ret, val = 0;
    bool do_standby = false;

    ALOGV("in_set_parameters: %s", kvpairs);

    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_INPUT_SOURCE, value, sizeof(value));

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);
    if (ret >= 0) {
        val = atoi(value);
        /* no audio source uses val == 0 */
        if ((in->source != val) && (val != 0)) {
            in->source = val;
            do_standby = true;
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value) & ~AUDIO_DEVICE_BIT_IN;
        if ((adev->mode != AUDIO_MODE_IN_CALL) && (in->device != val) && (val != 0)) {
            in->device = val;
            do_standby = true;
        } else if((adev->mode == AUDIO_MODE_IN_CALL) && (in->source != val) && (val != 0)) {
            in->device = val;

            select_input_device(adev);
        }
    }
    // set audio in device
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICES_IN_ACTIVE, value, sizeof(value));
    if (ret >= 0)
    {
        ALOGV("in_set_parament AUDIO_PARAMETER_DEVICES_IN_ACTIVE: %s", value);
        //set_audio_devices_active(adev, AUDIO_IN, value);
        do_standby = true;
    }

    if (do_standby)
        do_input_standby(in);
    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&adev->lock);

    str_parms_destroy(parms);
    return ret;
}

static char * in_get_parameters(const struct audio_stream *stream,
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
#if 0
static void get_capture_delay(struct sunxi_stream_in *in,
                       size_t frames,
                       struct echo_reference_buffer *buffer)
{

    /* read frames available in kernel driver buffer */
    unsigned int kernel_frames;
    struct timespec tstamp;
    long buf_delay;
    long rsmp_delay;
    long kernel_delay;
    long delay_ns;

    if (pcm_get_htimestamp(in->pcm, &kernel_frames, &tstamp) < 0) {
        buffer->time_stamp.tv_sec  = 0;
        buffer->time_stamp.tv_nsec = 0;
        buffer->delay_ns           = 0;
        ALOGW("read get_capture_delay(): pcm_htimestamp error");
        return;
    }

    /* read frames available in audio HAL input buffer
     * add number of frames being read as we want the capture time of first sample
     * in current buffer */
    buf_delay = (long)(((int64_t)(in->frames_in + in->proc_frames_in) * 1000000000)
                                    / in->config.rate);
    /* add delay introduced by resampler */
    rsmp_delay = 0;
    if (in->resampler) {
        rsmp_delay = in->resampler->delay_ns(in->resampler);
    }

    kernel_delay = (long)(((int64_t)kernel_frames * 1000000000) / in->config.rate);

    delay_ns = kernel_delay + buf_delay + rsmp_delay;

    buffer->time_stamp = tstamp;
    buffer->delay_ns   = delay_ns;
    ALOGV("get_capture_delay time_stamp = [%ld].[%ld], delay_ns: [%d],"
         " kernel_delay:[%ld], buf_delay:[%ld], rsmp_delay:[%ld], kernel_frames:[%d], "
         "in->frames_in:[%zu], in->proc_frames_in:[%zu], frames:[%zu]",
         buffer->time_stamp.tv_sec , buffer->time_stamp.tv_nsec, buffer->delay_ns,
         kernel_delay, buf_delay, rsmp_delay, kernel_frames,
         in->frames_in, in->proc_frames_in, frames);
}

static int32_t update_echo_reference(struct sunxi_stream_in *in, size_t frames)
{
    struct echo_reference_buffer b;
    b.delay_ns = 0;

    ALOGV("update_echo_reference, frames = [%zu], in->ref_frames_in = [%zu],  "
          "b.frame_count = [%zu]",
         frames, in->ref_frames_in, frames - in->ref_frames_in);
    if (in->ref_frames_in < frames) {
        if (in->ref_buf_size < frames) {
            in->ref_buf_size = frames;
            in->ref_buf = (int16_t *)realloc(in->ref_buf,
                                             in->ref_buf_size *
                                                 in->config.channels * sizeof(int16_t));
        }

        b.frame_count = frames - in->ref_frames_in;
        b.raw = (void *)(in->ref_buf + in->ref_frames_in * in->config.channels);

        get_capture_delay(in, frames, &b);

        if (in->echo_reference->read(in->echo_reference, &b) == 0)
        {
            in->ref_frames_in += b.frame_count;
            ALOGV("update_echo_reference: in->ref_frames_in:[%zu], "
                    "in->ref_buf_size:[%zu], frames:[%zu], b.frame_count:[%zu]",
                 in->ref_frames_in, in->ref_buf_size, frames, b.frame_count);
        }
    } else
        ALOGW("update_echo_reference: NOT enough frames to read ref buffer");
    return b.delay_ns;
}

static int set_preprocessor_param(effect_handle_t handle,
                           effect_param_t *param)
{
    uint32_t size = sizeof(int);
    uint32_t psize = ((param->psize - 1) / sizeof(int) + 1) * sizeof(int) +
                        param->vsize;

    int status = (*handle)->command(handle,
                                   EFFECT_CMD_SET_PARAM,
                                   sizeof (effect_param_t) + psize,
                                   param,
                                   &size,
                                   &param->status);
    if (status == 0)
        status = param->status;

    return status;
}

static int set_preprocessor_echo_delay(effect_handle_t handle,
                                     int32_t delay_us)
{
    uint32_t buf[sizeof(effect_param_t) / sizeof(uint32_t) + 2];
    effect_param_t *param = (effect_param_t *)buf;

    param->psize = sizeof(uint32_t);
    param->vsize = sizeof(uint32_t);
    *(uint32_t *)param->data = AEC_PARAM_ECHO_DELAY;
    *((int32_t *)param->data + 1) = delay_us;

    return set_preprocessor_param(handle, param);
}

static void push_echo_reference(struct sunxi_stream_in *in, size_t frames)
{
    /* read frames from echo reference buffer and update echo delay
     * in->ref_frames_in is updated with frames available in in->ref_buf */
    int32_t delay_us = update_echo_reference(in, frames)/1000;
    int i;
    audio_buffer_t buf;

    if (in->ref_frames_in < frames)
        frames = in->ref_frames_in;

    buf.frameCount = frames;
    buf.raw = in->ref_buf;

    for (i = 0; i < in->num_preprocessors; i++) {
        if ((*in->preprocessors[i])->process_reverse == NULL)
            continue;

        (*in->preprocessors[i])->process_reverse(in->preprocessors[i],
                                               &buf,
                                               NULL);
        set_preprocessor_echo_delay(in->preprocessors[i], delay_us);
    }

    in->ref_frames_in -= buf.frameCount;
    if (in->ref_frames_in) {
        memcpy(in->ref_buf,
               in->ref_buf + buf.frameCount * in->config.channels,
               in->ref_frames_in * in->config.channels * sizeof(int16_t));
    }
}
#endif
static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                                   struct resampler_buffer* buffer)
{
    struct sunxi_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
        return -EINVAL;

    in = (struct sunxi_stream_in *)((char *)buffer_provider -
                                   offsetof(struct sunxi_stream_in, buf_provider));

    if (in->pcm == NULL) {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        in->read_status = -ENODEV;
        return -ENODEV;
    }

//  ALOGV("get_next_buffer: in->config.period_size: %d, audio_stream_frame_size: %d",
//      in->config.period_size, audio_stream_frame_size(&in->stream.common));
    if (in->frames_in == 0) {
        in->read_status = pcm_read_ex(in->pcm,
                                   (void*)in->buffer,
                                   in->config.period_size *
                                       audio_stream_in_frame_size((const struct audio_stream_in *)in));
        if (in->read_status != 0) {
            ALOGE("get_next_buffer() pcm_read error %d, %s", in->read_status, strerror(errno));
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return in->read_status;
        }
        in->frames_in = in->config.period_size;
    }

    buffer->frame_count = (buffer->frame_count > in->frames_in) ?
                                in->frames_in : buffer->frame_count;
    buffer->i16 = in->buffer + (in->config.period_size - in->frames_in) *
                                                in->config.channels;

    return in->read_status;

}

static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                                  struct resampler_buffer* buffer)
{
    struct sunxi_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
        return;

    in = (struct sunxi_stream_in *)((char *)buffer_provider -
                                   offsetof(struct sunxi_stream_in, buf_provider));

    in->frames_in -= buffer->frame_count;
}

/* read_frames() reads frames from kernel driver, down samples to capture rate
 * if necessary and output the number of frames requested to the buffer specified */
static ssize_t read_frames(struct sunxi_stream_in *in, void *buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;

    while (frames_wr < frames) {
        size_t frames_rd = frames - frames_wr;
        if (in->resampler != NULL) {
            in->resampler->resample_from_provider(in->resampler,
                    (int16_t *)((char *)buffer +
                            frames_wr * audio_stream_in_frame_size((const struct audio_stream_in *)in)),
                    &frames_rd);
        } else {
            struct resampler_buffer buf = {
                    { .raw = NULL, },
                    .frame_count = frames_rd,
            };
            get_next_buffer(&in->buf_provider, &buf);
            if (buf.raw != NULL) {
                memcpy((char *)buffer +
                           frames_wr * audio_stream_in_frame_size((const struct audio_stream_in *)in),
                        buf.raw,
                        buf.frame_count * audio_stream_in_frame_size((const struct audio_stream_in *)in));
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

#if 0
/* process_frames() reads frames from kernel driver (via read_frames()),
 * calls the active audio pre processings and output the number of frames requested
 * to the buffer specified */
static ssize_t process_frames(struct sunxi_stream_in *in, void* buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;
    audio_buffer_t in_buf;
    audio_buffer_t out_buf;
    int i;

    while (frames_wr < frames) {
        /* first reload enough frames at the end of process input buffer */
        if (in->proc_frames_in < (size_t)frames) {
            ssize_t frames_rd;

            if (in->proc_buf_size < (size_t)frames) {
                in->proc_buf_size = (size_t)frames;
                in->proc_buf = (int16_t *)realloc(in->proc_buf,
                                         in->proc_buf_size *
                                             in->config.channels * sizeof(int16_t));
                ALOGV("process_frames(): in->proc_buf %p size extended to %zu frames",
                     in->proc_buf, in->proc_buf_size);
            }
            frames_rd = read_frames(in,
                                    in->proc_buf +
                                        in->proc_frames_in * in->config.channels,
                                    frames - in->proc_frames_in);
            if (frames_rd < 0) {
                frames_wr = frames_rd;
                break;
            }
            in->proc_frames_in += frames_rd;
        }

        if (in->echo_reference != NULL)
            push_echo_reference(in, in->proc_frames_in);

         /* in_buf.frameCount and out_buf.frameCount indicate respectively
          * the maximum number of frames to be consumed and produced by process() */
        in_buf.frameCount   = in->proc_frames_in;
        in_buf.s16          = in->proc_buf;
        out_buf.frameCount  = frames - frames_wr;
        out_buf.s16 = (int16_t *)buffer + frames_wr * in->config.channels;

        for (i = 0; i < in->num_preprocessors; i++)
            (*in->preprocessors[i])->process(in->preprocessors[i],
                                               &in_buf,
                                               &out_buf);

        /* process() has updated the number of frames consumed and produced in
         * in_buf.frameCount and out_buf.frameCount respectively
         * move remaining frames to the beginning of in->proc_buf */
        in->proc_frames_in -= in_buf.frameCount;
        if (in->proc_frames_in) {
            memcpy(in->proc_buf,
                   in->proc_buf + in_buf.frameCount * in->config.channels,
                   in->proc_frames_in * in->config.channels * sizeof(int16_t));
        }

        /* if not enough frames were passed to process(), read more and retry. */
        if (out_buf.frameCount == 0)
            continue;

        frames_wr += out_buf.frameCount;
    }
    return frames_wr;
}
#endif
static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    int ret = 0;
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    struct sunxi_audio_device *adev = in->dev;
    size_t frames_rq                = 0;

    if (adev->mode == AUDIO_MODE_IN_CALL) {
    //ALOGD("in call mode, in_read, return ;");
    usleep(10000);
    return 1;
    }

    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the input stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    if (adev->af_capture_flag && adev->PcmManager.BufExist) {
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&in->lock);
        if (in->standby) {
            in->standby = 0;
        }
        pthread_mutex_unlock(&adev->lock);

        if (ret < 0)
            goto exit;

        //if (bytes > adev->PcmManager.DataLen)
            //usleep(10000);

        ret = ReadPcmData(buffer, bytes, &adev->PcmManager);

        if (ret > 0)
            ret = 0;

        if (ret == 0 && adev->mic_mute)
            memset(buffer, 0, bytes);

        pthread_mutex_unlock(&in->lock);
        return bytes;
    }

    pthread_mutex_lock(&adev->lock);
    bool kara_flag = adev->micstart;
    pthread_mutex_unlock(&adev->lock);
    if (kara_flag) {
       memset(buffer,0,bytes);
       int time = bytes*1000*1000/4/44100;
       usleep(time);
       return bytes;
    }
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);
    if (in->standby) {
        ret = start_input_stream(in);
        if (ret == 0)
            in->standby = 0;
    }
    pthread_mutex_unlock(&adev->lock);

    if (ret < 0)
        goto exit;

    /* place after start_input_stream, because start_input_stream() change frame size */
    frames_rq = bytes / audio_stream_in_frame_size((const struct audio_stream_in *)in);

    if (in->num_preprocessors != 0) {
        ret = read_frames(in, buffer, frames_rq);//ret = process_frames(in, buffer, frames_rq);
    } else if (in->resampler != NULL) {
        ret = read_frames(in, buffer, frames_rq);
    } else {
        ret = pcm_read_ex(in->pcm, buffer, bytes);
    }

    if (ret > 0)
        ret = 0;

    if (ret == 0 && adev->mic_mute)
        memset(buffer, 0, bytes);

exit:
    if (ret < 0)
        usleep(bytes * 1000000 / audio_stream_in_frame_size((const struct audio_stream_in *)in) /
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
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    int status;
    effect_descriptor_t desc;

    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    if (in->num_preprocessors >= MAX_PREPROCESSORS) {
        status = -ENOSYS;
        goto exit;
    }

    status = (*effect)->get_descriptor(effect, &desc);
    if (status != 0)
        goto exit;

    in->preprocessors[in->num_preprocessors++] = effect;

    if (memcmp(&desc.type, FX_IID_AEC, sizeof(effect_uuid_t)) == 0) {
        in->need_echo_reference = true;
        do_input_standby(in);
    }

exit:

    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);
    return status;
}

static int in_remove_audio_effect(const struct audio_stream *stream,
                                  effect_handle_t effect)
{
    struct sunxi_stream_in *in = (struct sunxi_stream_in *)stream;
    int i;
    int status = -EINVAL;
    bool found = false;
    effect_descriptor_t desc;

    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    if (in->num_preprocessors <= 0) {
        status = -ENOSYS;
        goto exit;
    }

    for (i = 0; i < in->num_preprocessors; i++) {
        if (found) {
            in->preprocessors[i - 1] = in->preprocessors[i];
            continue;
        }
        if (in->preprocessors[i] == effect) {
            in->preprocessors[i] = NULL;
            status = 0;
            found = true;
        }
    }

    if (status != 0)
        goto exit;

    in->num_preprocessors--;

    status = (*effect)->get_descriptor(effect, &desc);
    if (status != 0)
        goto exit;
    if (memcmp(&desc.type, FX_IID_AEC, sizeof(effect_uuid_t)) == 0) {
        in->need_echo_reference = false;
        do_input_standby(in);
    }

exit:

    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);
    return status;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused)
{
    struct sunxi_audio_device *ladev = (struct sunxi_audio_device *)dev;
    struct sunxi_stream_out *out;
    UNUSED(handle);
    UNUSED(devices);
    ALOGD("adev_open_output_stream");
    ALOGV("adev_open_output_stream, flags: %x", flags);

    out = (struct sunxi_stream_out *)calloc(1, sizeof(struct sunxi_stream_out));
    if (!out)
        return -ENOMEM;

    out->buffer = malloc(RESAMPLER_BUFFER_SIZE); /* todo: allow for reallocing */

    out->stream.common.get_sample_rate  = out_get_sample_rate;
    out->stream.common.set_sample_rate  = out_set_sample_rate;
    out->stream.common.get_buffer_size  = out_get_buffer_size;
    out->stream.common.get_channels     = out_get_channels;
    out->stream.common.get_format       = out_get_format;
    out->stream.common.set_format       = out_set_format;
    out->stream.common.standby          = out_standby;
    out->stream.common.dump             = out_dump;
    out->stream.common.set_parameters   = out_set_parameters;
    out->stream.common.get_parameters   = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency             = out_get_latency;
    out->stream.set_volume              = out_set_volume;
    out->stream.write                   = out_write;
    out->stream.get_render_position     = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
    //out->stream.get_presentation_position = out_get_presentation_position;

    if (flags & AUDIO_OUTPUT_FLAG_DIRECT) {
        if (config->sample_rate == 0)
            config->sample_rate = DEFAULT_OUT_SAMPLING_RATE;
        if (config->channel_mask == 0)
            config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;

        out->format = config->format;
        out->sample_rate = config->sample_rate;
        out->channel_mask = config->channel_mask;
        out->config = pcm_config_aux_digital;
        out->config.rate = config->sample_rate;
        out->config.channels = popcount(config->channel_mask);
        out->config.period_size = AUX_DIGITAL_MULTI_PERIOD_BYTES / (out->config.channels * 2);
        out->config.raw_flag = (config->format == AUDIO_FORMAT_AC3)?2:(config->format == AUDIO_FORMAT_E_AC3)?10
                                :(config->format == AUDIO_FORMAT_DTS)?7:1;
        if(out->config.raw_flag == 1)
        {
            if(config->format == AUDIO_FORMAT_PCM_32_BIT)
            {
                out->config.format = PCM_FORMAT_S32_LE;
            }
        }
    }else{
        out->config = pcm_config_mm_out;
    }

    out->flags      = flags;
    out->dev        = ladev;
    out->standby    = 1;

    /* FIXME: when we support multiple output devices, we will want to
     * do the following:
     * adev->out_device = out->device;
     * select_output_device(adev);
     * This is because out_set_parameters() with a route is not
     * guaranteed to be called after an output stream is opened. */
    config->format          = out_get_format(&out->stream.common);
    config->channel_mask    = out_get_channels(&out->stream.common);
    config->sample_rate     = out_get_sample_rate(&out->stream.common);

    ALOGV("+++++++++++++++ adev_open_output_stream: req_sample_rate: %d, fmt: %x, channel_count: %d",
        config->sample_rate, config->format, config->channel_mask);

    *stream_out = &out->stream;
    return 0;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct sunxi_stream_out *out = (struct sunxi_stream_out *)stream;
    struct sunxi_audio_device *adev = out->dev;
    unsigned int index;
    UNUSED(dev);

    if (adev->mode == AUDIO_MODE_IN_CALL || adev->mode == AUDIO_MODE_MODE_FACTORY_TEST  || adev->mode == AUDIO_MODE_FM)
    {
        ALOGW("mode in call, do not adev_close_output_stream");
        return ;
    }
    out_standby(&stream->common);

    if (out->buffer)
        free(out->buffer);
    if (out->resampler)
        release_resampler(out->resampler);
    for (index = 0; index < MAX_AUDIO_DEVICES; index++)
    {
        if (out->multi_resampler[index])
            release_resampler(out->multi_resampler[index]);
    }
    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;
    struct str_parms *parms;
    char value[64];
    int ret;

    ALOGV("adev_set_parameters, %s", kvpairs);

    parms   = str_parms_create_str(kvpairs);
    ret     = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_TTY_MODE, value, sizeof(value));
    if (ret >= 0) {
        int tty_mode;

        if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_OFF) == 0)
            tty_mode = TTY_MODE_OFF;
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_VCO) == 0)
            tty_mode = TTY_MODE_VCO;
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_HCO) == 0)
            tty_mode = TTY_MODE_HCO;
        else if (strcmp(value, AUDIO_PARAMETER_VALUE_TTY_FULL) == 0)
            tty_mode = TTY_MODE_FULL;
        else
            return -EINVAL;

        pthread_mutex_lock(&adev->lock);
        if (tty_mode != adev->tty_mode) {
            adev->tty_mode = tty_mode;
            if (adev->mode == AUDIO_MODE_IN_CALL || adev->mode == AUDIO_MODE_MODE_FACTORY_TEST || adev->mode == AUDIO_MODE_FM) {
                select_output_device(adev);
                adev_set_voice_volume(&adev->hw_device, adev->voice_volume);
            }
        }
        pthread_mutex_unlock(&adev->lock);
    }

    // set audio out device
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICES_OUT_ACTIVE, value, sizeof(value));
    if (ret >= 0)
    {
        ALOGV("out AUDIO_PARAMETER_DEVICES_OUT_ACTIVE: %s", value);
        pthread_mutex_lock(&adev->lock);
        if (adev->raw_flag == true)
        {
            ALOGW("in raw mode, should not set other audio out devices");
            pthread_mutex_unlock(&adev->lock);
            return -1;
        }
        set_audio_devices_active(adev, AUDIO_OUT, value);
        force_all_standby(adev);
        select_output_device(adev);
        pthread_mutex_unlock(&adev->lock);
    }
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_NREC, value, sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            adev->bluetooth_nrec = true;
        else
            adev->bluetooth_nrec = false;
    }

    ret = str_parms_get_str(parms,"direct-mode", value, sizeof(value));
    if (ret >= 0) {
        if (atoi(value) == 1){
            adev->direct_mode = true;
        }else{
            adev->direct_mode = false;
        }
        ALOGD("adev->direct_mode = %d",adev->direct_mode);
    }

    // set audio in device
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICES_IN_ACTIVE, value, sizeof(value));
    if (ret >= 0)
    {
        ALOGV("in AUDIO_PARAMETER_DEVICES_IN_ACTIVE: %s", value);
        set_audio_devices_active(adev, AUDIO_IN, value);
        force_all_standby(adev);
    }
    ret = str_parms_get_str(parms, "mediasw.sft.rawdata", value, sizeof(value));
    if (ret >= 0)
    {
        pthread_mutex_lock(&adev->lock);
        property_set(PROP_RAWDATA_KEY, value);
        pthread_mutex_unlock(&adev->lock);
        ret = 0;
    }
    ret = str_parms_get_str(parms, "screen_state", value, sizeof(value));
    if (ret >= 0)
    {
#define ENABLE_STANDBY_MUSIC 0
#if ENABLE_STANDBY_MUSIC
        if (strcmp(value, "on") == 0) {
            if(adev->ar != NULL){
                audio_route_free(adev->ar);
            }
            adev->ar = audio_route_init(adev->cardCODEC, AC100_XML_PATH);
            init_ac100_input_output_path(adev,OUT_DEVICE_SPEAKER_AND_HEADSET,IN_SOURCE_HEADSETMIC);
        }
#else
        ALOGV("in screen_state: %s", value);
        if (strcmp(value, "off") == 0) {
            /* we do revert jobs when suspend,
             * because codec_register will be lost when suspend while audio_route will be hold.
             * we have to make codec_register and audio_route synchronized. */
            if(adev->ar == NULL){
                adev->ar = audio_route_init(adev->cardCODEC, AC100_XML_PATH);
            }
            init_ac100_input_output_path(adev,OUT_DEVICE_SPEAKER_AND_HEADSET_OFF,IN_SOURCE_HEADSETMIC);
        } else if (strcmp(value, "on") == 0) {
            /* route for quick (<3s) hibernate. */
            if(adev->ar == NULL){
                adev->ar = audio_route_init(adev->cardCODEC, AC100_XML_PATH);
            }
            init_ac100_input_output_path(adev,OUT_DEVICE_SPEAKER_AND_HEADSET,IN_SOURCE_HEADSETMIC);
        }
#endif
    }  
    str_parms_destroy(parms);
    return ret;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)dev;

    char devices[128];
    memset(devices, 0, sizeof(devices));

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
    {
        return strdup(get_audio_devices(adev, AUDIO_IN));
    }

    if (!strcmp(keys, AUDIO_PARAMETER_DEVICES_OUT))
    {
        return strdup(get_audio_devices(adev, AUDIO_OUT));
    }

    if (!strcmp(keys, AUDIO_PARAMETER_DEVICES_IN_ACTIVE))
    {
        if (!get_audio_devices_active(adev, AUDIO_IN, devices))
        {
            return strdup(devices);
        }
    }

    if (!strcmp(keys, AUDIO_PARAMETER_DEVICES_OUT_ACTIVE))
    {
        if (!get_audio_devices_active(adev, AUDIO_OUT, devices))
        {
            return strdup(devices);
        }
    }

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
    UNUSED(volume);
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
    if (adev->mode != mode) {
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
                                  audio_input_flags_t flags __unused,
                                  const char *address __unused,
                                  audio_source_t source __unused)
{
    struct sunxi_audio_device *ladev = (struct sunxi_audio_device *)dev;
    struct sunxi_stream_in *in;
    int ret;
    int channel_count = popcount(config->channel_mask);

    UNUSED(handle);

    *stream_in = NULL;

    if (check_input_parameters(config->sample_rate, config->format, channel_count) != 0)
        return -EINVAL;

    in = (struct sunxi_stream_in *)calloc(1, sizeof(struct sunxi_stream_in));
    if (!in)
        return -ENOMEM;

    in->stream.common.get_sample_rate   = in_get_sample_rate;
    in->stream.common.set_sample_rate   = in_set_sample_rate;
    in->stream.common.get_buffer_size   = in_get_buffer_size;
    in->stream.common.get_channels      = in_get_channels;
    in->stream.common.get_format        = in_get_format;
    in->stream.common.set_format        = in_set_format;
    in->stream.common.standby           = in_standby;
    in->stream.common.dump              = in_dump;
    in->stream.common.set_parameters    = in_set_parameters;
    in->stream.common.get_parameters    = in_get_parameters;
    in->stream.common.add_audio_effect  = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read     = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    in->requested_rate  = config->sample_rate;

    // default config
    memcpy(&in->config, &pcm_config_mm_in, sizeof(pcm_config_mm_in));
    in->config.channels = channel_count;
    in->config.in_init_channels = channel_count;

    ALOGV("to malloc in-buffer: period_size: %d, frame_size: %zu",
        in->config.period_size, audio_stream_in_frame_size((const struct audio_stream_in *)in));
    in->buffer = malloc(in->config.period_size *
                        audio_stream_in_frame_size((const struct audio_stream_in *)in) * 8);

    if (!in->buffer) {
        ret = -ENOMEM;
        goto err;
    }
//  memset(in->buffer, 0, in->config.period_size *
//                        audio_stream_frame_size(&in->stream.common) * 8); //mute

    ladev->af_capture_flag = false;
    //devices = AUDIO_DEVICE_IN_WIFI_DISPLAY;//for test

    if (devices == AUDIO_DEVICE_IN_AF) {
        ALOGV("to malloc PcmManagerBuffer: Buffer_size: %d", AF_BUFFER_SIZE);
        ladev->PcmManager.BufStart= (unsigned char *)malloc(AF_BUFFER_SIZE);

        if(!ladev->PcmManager.BufStart) {
            ret = -ENOMEM;
            goto err;
        }

        ladev->PcmManager.BufExist      = true;
        ladev->PcmManager.BufTotalLen   = AF_BUFFER_SIZE;
        ladev->PcmManager.BufWritPtr    = ladev->PcmManager.BufStart;
        ladev->PcmManager.BufReadPtr    = ladev->PcmManager.BufStart;
        ladev->PcmManager.BufValideLen  = ladev->PcmManager.BufTotalLen;
        ladev->PcmManager.DataLen       = 0;
        ladev->PcmManager.SampleRate    = config->sample_rate;
        ladev->PcmManager.Channel       = 2;
        ladev->af_capture_flag          = true;

        ladev->PcmManager.dev           = (struct sunxi_audio_device *)ladev;
    }

    in->dev     = ladev;
    in->standby = 1;
    in->device  = devices & ~AUDIO_DEVICE_BIT_IN;

    *stream_in  = &in->stream;
    return 0;

err:
    if (in->resampler)
        release_resampler(in->resampler);

    free(in);
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *stream)
{
    struct sunxi_stream_in *in          = (struct sunxi_stream_in *)stream;
    struct sunxi_audio_device *ladev    = (struct sunxi_audio_device *)dev;

    in_standby(&stream->common);

    if (in->buffer) {
        free(in->buffer);
        in->buffer = 0;
    }
    if (in->resampler) {
        release_resampler(in->resampler);
    }
    if (ladev->af_capture_flag) {
        ladev->af_capture_flag = false;
    }
    if (ladev->PcmManager.BufStart) {
        ladev->PcmManager.BufExist = false;
        free(ladev->PcmManager.BufStart);
        ladev->PcmManager.BufStart = 0;
    }
    free(stream);
    ALOGD("adev_close_input_stream set voice record status");
    return;
}

#define IN_SAMPLERATE (44100)  

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    UNUSED(device);
    UNUSED(fd);
    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct sunxi_audio_device *adev = (struct sunxi_audio_device *)device;
#if AUDIO_USE_AC100
    if(adev->ar){
        audio_route_free(adev->ar);
        adev->ar = NULL;
    }
#endif 
    mixer_close(adev->mixer); 
    free(device);

    return 0;
}



static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct sunxi_audio_device *adev; 

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = calloc(1, sizeof(struct sunxi_audio_device));
    if (!adev)
        return -ENOMEM; 
    adev->hw_device.common.tag      = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version  = AUDIO_DEVICE_API_VERSION_2_0;
    adev->hw_device.common.module   = (struct hw_module_t *) module;
    adev->hw_device.common.close    = adev_close;

    adev->hw_device.init_check              = adev_init_check;
    adev->hw_device.set_voice_volume        = adev_set_voice_volume;
    adev->hw_device.set_master_volume       = adev_set_master_volume;
    adev->hw_device.get_master_volume       = adev_get_master_volume;
    adev->hw_device.set_mode                = adev_set_mode;
    adev->hw_device.set_mic_mute            = adev_set_mic_mute;
    adev->hw_device.get_mic_mute            = adev_get_mic_mute;
    adev->hw_device.set_parameters          = adev_set_parameters;
    adev->hw_device.get_parameters          = adev_get_parameters;
    adev->hw_device.get_input_buffer_size   = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream      = adev_open_output_stream;
    adev->hw_device.close_output_stream     = adev_close_output_stream;
    adev->hw_device.open_input_stream       = adev_open_input_stream;
    adev->hw_device.close_input_stream      = adev_close_input_stream;
    adev->hw_device.dump                    = adev_dump;
    adev->raw_flag                          = false;
    adev->af_capture_flag = false;
    adev->first_set_audio_routing = true;
    adev->channelnum                        = 2;
    adev->micstart                         = false;
    adev->direct_mode = false;
    adev->inUsb_mic_mode                        = false; 
    init_audio_devices(adev);
    init_audio_devices_active(adev);
    //card
    adev->cardCODEC = -1;
    adev->cardHDMI  = -1;
    adev->cardSPDIF = -1;
    adev->cardBT    = -1;
    adev->cardMIX   = -1;
    adev->cardIn    = -1;

    getCardNumbyName(adev, AUDIO_NAME_CODEC, &adev->cardCODEC);
    getCardNumbyName(adev, AUDIO_NAME_HDMI, &adev->cardHDMI);
    getCardNumbyName(adev, AUDIO_NAME_SPDIF, &adev->cardSPDIF);
    getCardNumbyName(adev, AUDIO_NAME_BT, &adev->cardBT);
    getCardNumbyName(adev, AUDIO_NAME_MIX, &adev->cardMIX);
    int card = 0;
    for (card = 0; card < MAX_AUDIO_DEVICES; card++)
    {
        if (adev->dev_manager[card].flag_exist
            && (adev->dev_manager[card].flag_in == AUDIO_IN)
            && !strcmp(adev->dev_manager[card].name, AUDIO_NAME_CODEC))
        {
            ALOGV("use %s mixer control", adev->dev_manager[card].name);
            break;
        }
    }

    if (card == MAX_AUDIO_DEVICES)
    {
        ALOGE("can not find audio codec mixer control");
    }

    adev->mixer = mixer_open(card);
 
    if (!adev->mixer) {
        free(adev);
        ALOGE("Unable to open the mixer, aborting.");
        return -EINVAL;
    }

#if !LOG_NDEBUG
    // dump list of mixer controls
    tinymix_list_controls(adev->mixer);
#endif

    /* Set the default route before the PCM stream is opened */
    pthread_mutex_lock(&adev->lock);

    adev->mode      = AUDIO_MODE_NORMAL;
    adev->out_device = AUDIO_DEVICE_OUT_SPEAKER;
    adev->in_device = AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN;
#if AUDIO_USE_AC100
    adev->ar = audio_route_init(adev->cardCODEC, AC100_XML_PATH);
    init_ac100_input_output_path(adev,OUT_DEVICE_SPEAKER_AND_HEADSET,IN_SOURCE_HEADSETMIC);
#endif
    select_output_device(adev);
    adev->pcm_modem_dl      = NULL;
    adev->pcm_modem_ul      = NULL;
    adev->voice_volume      = 1.0f;
    adev->tty_mode          = TTY_MODE_OFF;
    adev->bluetooth_nrec    = false;
    adev->wb_amr            = 0;
    adev->fm_mode = 2;

    pthread_mutex_unlock(&adev->lock);

    *device = &adev->hw_device.common;
    return 0; 
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version    = HARDWARE_HAL_API_VERSION,
        .id                 = AUDIO_HARDWARE_MODULE_ID,
        .name               = "sunxi audio HW HAL",
        .author             = "author",
        .methods            = &hal_module_methods,
    },
};
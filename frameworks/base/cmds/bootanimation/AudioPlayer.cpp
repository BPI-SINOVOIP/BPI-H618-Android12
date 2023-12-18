/*
 * Copyright (C) 2014 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "BootAnim_AudioPlayer"

#include "AudioPlayer.h"

#include <androidfw/ZipFileRO.h>
#include <tinyalsa/asoundlib.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

// Maximum line length for audio_conf.txt
// We only accept lines less than this length to avoid overflows using sscanf()
#define MAX_LINE_LENGTH 1024

struct riff_wave_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct chunk_header {
    uint32_t id;
    uint32_t sz;
};

struct chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct name_map
{
    char name_linux[32];
    char name_android[32];
};

struct audio_card {
    bool flag_exist;
    int card_num;
    int device_num;
};
struct audio_manager {
    char        name[32];
    char        card_id[32];
    int         card;
    bool        flag_exist;
};

struct name_map audio_name_map[AUDIO_MAP_CNT] =
{
    {"audiocodec",    AUDIO_NAME_CODEC},//H3
    {"ahubhdmi",       AUDIO_NAME_HDMI}, //H3
    {"sndahub",       AUDIO_NAME_AHUB}, //
};

struct audio_card         mCardCodec;
struct audio_card         mCardHDMI;
struct audio_card         mCardAHUBHDMI;
struct audio_manager      mAudioDeviceManager[AUDIO_MAP_CNT];
struct pcm *pcmCodec       = NULL;
struct pcm *pcmHDMI        = NULL;
struct pcm *pcmAHUBHDMI   = NULL;

namespace android {

AudioPlayer::AudioPlayer()
    :   mCard(-1),
        mDevice(-1),
        mHeadsetPlugged(false),
        mPeriodSize(0),
        mPeriodCount(0),
        mSingleVolumeControl(false),
        mSpeakerHeadphonesSyncOut(false),
        mWavData(NULL),
        mWavLength(0)
{
}

AudioPlayer::~AudioPlayer() {
}
void enable_ahub_codec_out(struct mixer *mixer)
{
    ALOGD("enable_ahub_codec");
    struct mixer_ctl *ctl;
    ctl = mixer_get_ctl_by_name(mixer, "I2S3 Src Select");
    mixer_ctl_set_value(ctl, 0, 2);
    ctl = mixer_get_ctl_by_name(mixer, "I2S3OUT Switch");
    mixer_ctl_set_value(ctl, 0, 1);
}
void enable_ahub_hdmi_out(struct mixer *mixer)
{
    ALOGD("enable_ahub_hdmi");
    struct mixer_ctl *ctl;
    ctl = mixer_get_ctl_by_name(mixer, "I2S1 Src Select");
    mixer_ctl_set_value(ctl, 0, 1);
    ctl = mixer_get_ctl_by_name(mixer, "I2S1OUT Switch");
    mixer_ctl_set_value(ctl, 0, 1);
}

void AudioPlayer::getCardNumbyName(const char *name, int *card, bool *flag_exist)
{
    int index;
    for (index = 0; index < AUDIO_MAP_CNT; index++) {
        if (!strcmp(mAudioDeviceManager[index].name, name)) {
            *card = index;
            *flag_exist = true;
            return;
        }
    }

    if (index == AUDIO_MAP_CNT) {
        *card = -1;
        *flag_exist = false;
        ALOGE("%s card does not exist",name);
    }
}
int AudioPlayer::find_name_map(char * in, char * out)
{
    int index = 0;

    if (in == 0 || out == 0) {
        ALOGE("error params");
        return -1;
    }

    for (; index < AUDIO_MAP_CNT; index++) {
        if (!strcmp(in, audio_name_map[index].name_linux)) {
            strcpy(out, audio_name_map[index].name_android);
            return 0;
        }
    }
    return 0;
}

int AudioPlayer::do_init_audio_card(int card)
{
    int ret = -1;
    int fd = 0;
    char snd_card[128], snd_node[128];
    char snd_id[32], snd_name[32];

    memset(snd_card, 0, sizeof(snd_card));
    memset(snd_node, 0, sizeof(snd_node));
    memset(snd_id, 0, sizeof(snd_id));
    memset(snd_name, 0, sizeof(snd_name));

    sprintf(snd_card, "%s/card%d", "/sys/class/sound", card);
    ret = access(snd_card, F_OK);
    if(ret == 0) {
        sprintf(snd_node, "%s/card%d/id", "/sys/class/sound", card);
        ALOGV("read card %s/card%d/id","/sys/class/sound", card);
        fd = open(snd_node, O_RDONLY);
        if (fd > 0) {
            ret = read(fd, snd_id, sizeof(snd_id));
            if (ret > 0) {
                snd_id[ret - 1] = 0;
                ALOGV("%s, %s, len: %d", snd_node, snd_id, ret);
            }
            close(fd);
        }
        else
            return -1;

        strcpy(mAudioDeviceManager[card].card_id, snd_id);
        find_name_map(snd_id, snd_name);
        strcpy(mAudioDeviceManager[card].name, snd_name);

        mAudioDeviceManager[card].card = card;
        mAudioDeviceManager[card].flag_exist = true;
    }
    else
        return -1;
    return 0;
}

void AudioPlayer::init_audio_devices()
{
    int card = 0;
    memset(mAudioDeviceManager, 0, sizeof(mAudioDeviceManager));

    for (card = 0; card < AUDIO_MAP_CNT; card++)
        do_init_audio_card(card);
    getCardNumbyName(AUDIO_NAME_CODEC, &mCardCodec.card_num, &mCardCodec.flag_exist);
    getCardNumbyName(AUDIO_NAME_HDMI, &mCardHDMI.card_num, &mCardHDMI.flag_exist);
    getCardNumbyName(AUDIO_NAME_AHUB, &mCardAHUBHDMI.card_num, &mCardAHUBHDMI.flag_exist);
    mCardCodec.device_num = 0;
    mCardHDMI.device_num = 0;
    mCardAHUBHDMI.device_num = 0;
}


void AudioPlayer::jack_switch_state_detection(char *path)
{
    int fd;
    int value = 0;

    int read_count;
    if (strcmp(path,"") == 0) {
        ALOGE("jack state path is null\n");
    } else {
        ALOGD("jack state read from(%s)\n", path);
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ALOGE("cannot open %s(%s)\n", path, strerror(errno));
        return;
    }

    errno = 0;
    read_count = read(fd, &value, 1);
    value &= 0xF;
    if (read_count<0){
        ALOGE("jack state read error %d(%s)\n", errno, strerror(errno));
        return;
    }
    ALOGD("jack state :value = %d\n", value);
    switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    /* open */
        mHeadsetPlugged = true;
        ALOGD("jack has been plugged!");
        break;
    case 0:
    /* close */
        mHeadsetPlugged = false;
        ALOGD("jack has not been plugged!");
        break;
    default:
        ALOGE("switch state error!\n");
        break;
    }

    close(fd);
}

static int get_card(const char *card_name)
{
    int ret;
    int fd;
    int i;
    char path[128];
    char name[64];

    for (i = 0; i < 10; i++) {
        sprintf(path, "/sys/class/sound/card%d/id", i);
        ret = access(path, F_OK);
        if (ret) {
            ALOGW("can't find node %s, use card0", path);
            return 0;
        }

        fd = open(path, O_RDONLY);
        if (fd <= 0) {
            ALOGW("can't open %s, use card0", path);
            return 0;
        }

        ret = read(fd, name, sizeof(name));
        close(fd);
        if (ret > 0) {
            name[ret-1] = '\0';
            if (strstr(name, card_name))
                return i;
        }
    }

    ALOGW("can't find card:%s, use card0", card_name);
    return 0;
}


static bool setMixerValue(struct mixer* mixer, const char* name, const char* values)
{
    if (!mixer) {
        ALOGE("no mixer in setMixerValue");
        return false;
    }
    struct mixer_ctl *ctl = mixer_get_ctl_by_name(mixer, name);
    if (!ctl) {
        ALOGE("mixer_get_ctl_by_name failed for %s", name);
        return false;
    }

    enum mixer_ctl_type type = mixer_ctl_get_type(ctl);
    int numValues = mixer_ctl_get_num_values(ctl);
    int intValue;
    char stringValue[MAX_LINE_LENGTH];

    for (int i = 0; i < numValues && values; i++) {
        // strip leading space
        while (*values == ' ') values++;
        if (*values == 0) break;

        switch (type) {
            case MIXER_CTL_TYPE_BOOL:
            case MIXER_CTL_TYPE_INT:
                if (sscanf(values, "%d", &intValue) == 1) {
                    if (mixer_ctl_set_value(ctl, i, intValue) != 0) {
                        ALOGE("mixer_ctl_set_value failed for %s %d", name, intValue);
                    }
                } else {
                    ALOGE("Could not parse %s as int for %s", values, name);
                }
                break;
            case MIXER_CTL_TYPE_ENUM:
                if (sscanf(values, "%s", stringValue) == 1) {
                    if (mixer_ctl_set_enum_by_string(ctl, stringValue) != 0) {
                        ALOGE("mixer_ctl_set_enum_by_string failed for %s %s", name, stringValue);
                    }
                } else {
                    ALOGE("Could not parse %s as enum for %s", values, name);
                }
                break;
            default:
                ALOGE("unsupported mixer type %d for %s", type, name);
                break;
        }

        values = strchr(values, ' ');
    }

    return true;
}


/*
 * Parse the audio configuration file.
 * The file is named audio_conf.txt and must begin with the following header:
 *
 * card=<ALSA card number>
 * device=<ALSA device number>
 * period_size=<period size>
 * period_count=<period count>
 *
 * This header is followed by zero or more mixer settings, each with the format:
 * mixer "<name>" = <value list>
 * Since mixer names can contain spaces, the name must be enclosed in double quotes.
 * The values in the value list can be integers, booleans (represented by 0 or 1)
 * or strings for enum values.
 */
bool AudioPlayer::init(const char* config)
{
    int tempInt;
    struct mixer* mixer = NULL;
    char    name[MAX_LINE_LENGTH];
    init_audio_devices();

    for (;;) {
        const char* endl = strstr(config, "\n");
        if (!endl) break;
        String8 line(config, endl - config);
        if (line.length() >= MAX_LINE_LENGTH) {
            ALOGE("Line too long in audio_conf.txt");
            return false;
        }
        const char* l = line.string();

        if (sscanf(l, "card=%d", &tempInt) == 1) {
            mCard = mCardCodec.card_num;
            mixer = mixer_open(mCard);
            if (!mixer) {
                ALOGE("could not open mixer for card %d", mCard);
                return false;
            }
        } else if (sscanf(line, "card=%s", name) == 1) {
            mCard = get_card(name);
            mixer = mixer_open(mCard);
            if (!mixer) {
                ALOGE("could not open mixer for card %d", mCard);
                return false;
            }
        }else if (sscanf(l, "device=%d", &tempInt) == 1) {
            ALOGD("device=%d", tempInt);
            mDevice = tempInt;
        } else if (sscanf(l, "period_size=%d", &tempInt) == 1) {
            ALOGD("period_size=%d", tempInt);
            mPeriodSize = tempInt;
        } else if (sscanf(l, "period_count=%d", &tempInt) == 1) {
            ALOGD("period_count=%d", tempInt);
            mPeriodCount = tempInt;
        } else if (sscanf(line, "Headset detection=%s", name) == 1) {
            ALOGD("Headset detection=%s", name);
            //jack_state_detection();
            //jack_switch_state_detection();
        } else if (sscanf(line, "switch_path=%s", name) == 1) {
            ALOGD("switch_path=%s", name);
            String8 switch_path((char *)name);
            mSwitchStatePath = switch_path;
            ALOGD("mSwitchStatePath=%s", mSwitchStatePath.string());

            //jack_state_detection();
            jack_switch_state_detection((char *)name);
        } else if (sscanf(l, "single_volume_control=%d", &tempInt) == 1) {
            ALOGD("single_volume_control=%d", tempInt);
            if (tempInt)
                mSingleVolumeControl = true;
            else
                mSingleVolumeControl = false;
        } else if (sscanf(line, "speaker_headphones_out=%d", &tempInt) == 1) {
            ALOGD("speaker_headphones_out=%d", tempInt);
            if (tempInt)
                mSpeakerHeadphonesSyncOut= true;
            else
                mSpeakerHeadphonesSyncOut = false;
        } else {
            if (mHeadsetPlugged && !mSpeakerHeadphonesSyncOut) {
                if (sscanf(l, "headset mixer \"%[0-9a-zA-Z _]s\"", name) == 1) {
                    const char* values = strchr(l, '=');
                    if (values) {
                        values++;   // skip '='
                        ALOGD("name: \"%s\" = %s", name, values);
                        setMixerValue(mixer, name, values);
                    } else {
                        ALOGE("values missing for name: \"%s\"", name);
                    }
                }
            } else if(!mSpeakerHeadphonesSyncOut){
                if (sscanf(l, "mixer \"%[0-9a-zA-Z _]s\"", name) == 1) {
                    const char* values = strchr(l, '=');
                    if (values) {
                        values++;   // skip '='
                        ALOGD("name: \"%s\" = %s", name, values);
                        setMixerValue(mixer, name, values);
                    } else {
                        ALOGE("values missing for name: \"%s\"", name);
                    }
                }
            } else{
                if (sscanf(l, "mixer \"%[0-9a-zA-Z _]s\"", name) == 1 ||
                    sscanf(l, "headset mixer \"%[0-9a-zA-Z _]s\"", name) == 1) {
                    const char* values = strchr(l, '=');
                    if (values) {
                        values++;   // skip '='
                        ALOGD("name: \"%s\" = %s", name, values);
                        setMixerValue(mixer, name, values);
                    } else {
                        ALOGE("values missing for name: \"%s\"", name);
                    }
                }
            }
        }
        config = ++endl;
    }

    mixer_close(mixer);

    if (mCard >= 0 && mDevice >= 0) {
        return true;
    }

    return false;
}

void AudioPlayer::playClip(const uint8_t* buf, int size) {
    // stop any currently playing sound
    requestExitAndWait();

    mWavData = (uint8_t *)buf;
    mWavLength = size;
    run("bootanim audio", PRIORITY_URGENT_AUDIO);
}

bool AudioPlayer::threadLoop()
{
    struct pcm_config config;
    bool moreChunks = true;
    const struct chunk_fmt* chunkFmt = NULL;
    int bufferSize;
    const uint8_t* wavData;
    size_t wavLength;
    const struct riff_wave_header* wavHeader;
    struct mixer* mixer_ahub = NULL;

    mixer_ahub = mixer_open(mCardAHUBHDMI.card_num);

    if (mWavData == NULL) {
        ALOGE("mWavData is NULL");
        return false;
     }

    wavData = (const uint8_t *)mWavData;
    if (!wavData) {
        ALOGE("Could not access WAV file data");
        goto exit;
    }
    wavLength = mWavLength;

    wavHeader = (const struct riff_wave_header *)wavData;
    if (wavLength < sizeof(*wavHeader) || (wavHeader->riff_id != ID_RIFF) ||
        (wavHeader->wave_id != ID_WAVE)) {
        ALOGE("Error: audio file is not a riff/wave file\n");
        goto exit;
    }
    wavData += sizeof(*wavHeader);
    wavLength -= sizeof(*wavHeader);

    do {
        const struct chunk_header* chunkHeader = (const struct chunk_header*)wavData;
        if (wavLength < sizeof(*chunkHeader)) {
            ALOGE("EOF reading chunk headers");
            goto exit;
        }

        wavData += sizeof(*chunkHeader);
        wavLength -=  sizeof(*chunkHeader);

        switch (chunkHeader->id) {
            case ID_FMT:
                chunkFmt = (const struct chunk_fmt *)wavData;
                wavData += chunkHeader->sz;
                wavLength -= chunkHeader->sz;
                break;
            case ID_DATA:
                /* Stop looking for chunks */
                moreChunks = 0;
                break;
            default:
                /* Unknown chunk, skip bytes */
                wavData += chunkHeader->sz;
                wavLength -= chunkHeader->sz;
        }
    } while (moreChunks);

    if (!chunkFmt) {
        ALOGE("format not found in WAV file");
        goto exit;
    }


    memset(&config, 0, sizeof(config));
    config.channels = chunkFmt->num_channels;
    config.rate = chunkFmt->sample_rate;
    config.period_size = mPeriodSize;
    config.period_count = mPeriodCount;
    config.start_threshold = mPeriodSize / 4;
    config.stop_threshold = INT_MAX;
    config.avail_min = config.start_threshold;
    if (chunkFmt->bits_per_sample != 16) {
        ALOGE("only 16 bit WAV files are supported");
        goto exit;
    }
    config.format = PCM_FORMAT_S16_LE;
    if(mCardCodec.flag_exist == 1) {
        pcmCodec = pcm_open(mCardCodec.card_num, mCardCodec.device_num, PCM_OUT, &config);
        if (!pcmCodec || !pcm_is_ready(pcmCodec)) {
            ALOGE("codec Unable to open PCM device (%s)\n", pcm_get_error(pcmCodec));
            goto exit;
        }
        pcm_prepare(pcmCodec);
    }
    if(mCardHDMI.flag_exist == 1) {
        pcmHDMI = pcm_open(mCardHDMI.card_num, mCardHDMI.device_num, PCM_OUT, &config);
        if (!pcmHDMI || !pcm_is_ready(pcmHDMI)) {
            ALOGE("hdmi Unable to open PCM device (%s)\n", pcm_get_error(pcmHDMI));
            goto exit;
        }
        pcm_prepare(pcmHDMI);
    }
    if(mCardAHUBHDMI.flag_exist == 1) {
        pcmAHUBHDMI = pcm_open(mCardAHUBHDMI.card_num, mCardAHUBHDMI.device_num, PCM_OUT, &config);
        if (!pcmAHUBHDMI || !pcm_is_ready(pcmAHUBHDMI)) {
            ALOGE("hdmi Unable to open PCM device (%s)\n", pcm_get_error(pcmAHUBHDMI));
            goto exit;
        }
        enable_ahub_hdmi_out(mixer_ahub);
        pcm_prepare(pcmAHUBHDMI);
    }
    bufferSize = pcm_frames_to_bytes(pcmCodec, pcm_get_buffer_size(pcmCodec));

    while (wavLength > 0) {
        if (exitPending()) goto exit;
        size_t count = bufferSize / 4;

        if (count > wavLength)
            count = wavLength;

        if(mCardCodec.flag_exist == 1 && pcmCodec) {
            if (pcm_write(pcmCodec, wavData, count)) {
                ALOGE("codec pcm_write failed (%s)", pcm_get_error(pcmCodec));
                goto exit;
            }
        }
        if(mCardAHUBHDMI.flag_exist == 0) {
            if(mCardHDMI.flag_exist == 1 && pcmHDMI) {
                if (pcm_write(pcmHDMI, wavData, count)) {
                    ALOGE("hdmi pcm_write failed (%s)", pcm_get_error(pcmHDMI));
                    goto exit;
                }
            }
        }
        else {
            if(mCardHDMI.flag_exist == 1 && pcmAHUBHDMI) {
                if (pcm_write(pcmAHUBHDMI, wavData, count)) {
                    ALOGE("hdmi pcm_write failed (%s)", pcm_get_error(pcmAHUBHDMI));
                    goto exit;
                }
            }
        }

        wavData += count;
        wavLength -= count;
    }

exit:
    if(pcmAHUBHDMI)
        pcm_close(pcmAHUBHDMI);
    if(pcmHDMI)
        pcm_close(pcmHDMI);
    if(pcmCodec)
        pcm_close(pcmCodec);
    return false;
}

} // namespace android

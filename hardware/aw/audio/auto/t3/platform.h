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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <cutils/list.h>
#include <hardware/audio.h>
#include "usecase.h"
#include "audio_plugin.h"
#include "tinyalsa/asoundlib.h"
#include "audio_hw.h"

#define PLATFORM_INFO_XML_PATH "/vendor/etc/audio_platform_info.xml"
#define MIXER_PATHS_XML_PATH "/vendor/etc/audio_mixer_paths.xml"
#define MIXER_CARD 0

#define NUM_PROFILE 16
#define NUM_BACKEND 2

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

#define STR(string) #string
#define ENUM_TO_STR(string) {string, #string}
struct enum_to_str {
    uint32_t value;
    const char *name;
};

#define AUDIO_MAKE_STRING_FROM_ENUM(X)   { #X, X }

/* Sound devices specific to the platform
 * The DEVICE_OUT_* and DEVICE_IN_* should be mapped to these sound
 * devices to enable corresponding mixer paths
 */
enum platform_device {
    /** platform playback device **/
    OUT_NONE = 0,
    OUT_RESET,
    OUT_EAR,                /* earpiece */
    OUT_SPK,                /* speaker */
    OUT_DULSPK,             /* double speaker */
    OUT_HP,                 /* headphone */
    OUT_SPK_HP,             /* speaker and headphone */
    OUT_DULSPK_HP,          /* double speaker and headphone */
    OUT_BTSCO,              /* bt sco */
    OUT_DPH_PLAY,
    OUT_APH_PLAY,
    OUT_HDMI,               /* hdmi */
    OUT_SPDIF,              /* spdif */

    /* codec and hdmi */
    OUT_SPK_HDMI,           /* speaker and hdmi */
    OUT_DULSPK_HDMI,        /* double spk and hdmi */
    OUT_HP_HDMI,            /* headphone and hdmi */

    /* audio hub: codec and spdif */
    OUT_SPK_SPDIF,          /* speaker and spdif */
    OUT_DULSPK_SPDIF,       /* double speaker and spdif */
    OUT_HP_SPDIF,           /* headphone and spdif */

    /* audio hub: codec, hdmi and spdif */
    OUT_HDMI_SPDIF,         /* hdmi and spdif */
    OUT_SPK_HDMI_SPDIF,     /* spk, hdmi and spdif */
    OUT_DULSPK_HDMI_SPDIF,  /* double spk, hdmi and spdif */
    OUT_HP_HDMI_SPDIF,      /* headphone, hdmi and spdif */

    /** platform record device **/
    IN_AMIC,            /* analog mic */
    IN_DMIC,            /* digital mic */
    IN_HPMIC,           /* headphone mic */
    IN_BTSCO,           /* bt sco in */
    IN_DPH_REC,         /* digital phone record */
    IN_APH_REC,         /* analog phone record */

    /** platform analog phone device **/
    APH_SPK,    /* analog phone spk */
    APH_EAR,    /* analog phone earpiece */
    APH_HP,     /* analog phone headphone */
    APH_HS,     /* analog phone headset */
    APH_BTSCO,  /* analog phone bt sco */

    /** platform digital phone device **/
    DPH_SPK,    /* digital phone spk */
    DPH_EAR,    /* digital phone earpiece */
    DPH_HP,     /* digital phone headphone */
    DPH_HS,     /* digital phone headset */
    DPH_BTSCO,  /* digital phone bt sco */

    NUM_PLATFORM_DEVICE,
};

/* platform device to string tab */
static const struct enum_to_str pdev_str_tab[] = {
    /* platform playback device */
    ENUM_TO_STR(OUT_NONE),
    ENUM_TO_STR(OUT_RESET),
    ENUM_TO_STR(OUT_EAR),
    ENUM_TO_STR(OUT_SPK),
    ENUM_TO_STR(OUT_DULSPK),
    ENUM_TO_STR(OUT_HP),
    ENUM_TO_STR(OUT_SPK_HP),
    ENUM_TO_STR(OUT_DULSPK_HP),
    ENUM_TO_STR(OUT_BTSCO),
    ENUM_TO_STR(OUT_DPH_PLAY),
    ENUM_TO_STR(OUT_APH_PLAY),
    ENUM_TO_STR(OUT_HDMI),
    ENUM_TO_STR(OUT_SPDIF),

    ENUM_TO_STR(OUT_SPK_HDMI),
    ENUM_TO_STR(OUT_DULSPK_HDMI),
    ENUM_TO_STR(OUT_HP_HDMI),

    ENUM_TO_STR(OUT_SPK_SPDIF),
    ENUM_TO_STR(OUT_DULSPK_SPDIF),
    ENUM_TO_STR(OUT_HP_SPDIF),

    ENUM_TO_STR(OUT_HDMI_SPDIF),
    ENUM_TO_STR(OUT_SPK_HDMI_SPDIF),
    ENUM_TO_STR(OUT_DULSPK_HDMI_SPDIF),
    ENUM_TO_STR(OUT_HP_HDMI_SPDIF),

    /* platform record device */
    ENUM_TO_STR(IN_AMIC),
    ENUM_TO_STR(IN_DMIC),
    ENUM_TO_STR(IN_HPMIC),
    ENUM_TO_STR(IN_BTSCO),
    ENUM_TO_STR(IN_DPH_REC),
    ENUM_TO_STR(IN_APH_REC),

    /* platform analog phone device */
    ENUM_TO_STR(APH_SPK),
    ENUM_TO_STR(APH_EAR),
    ENUM_TO_STR(APH_HP),
    ENUM_TO_STR(APH_HS),
    ENUM_TO_STR(APH_BTSCO),

    /* platform digital phone device */
    ENUM_TO_STR(DPH_SPK),
    ENUM_TO_STR(DPH_EAR),
    ENUM_TO_STR(DPH_HP),
    ENUM_TO_STR(DPH_HS),
    ENUM_TO_STR(DPH_BTSCO),
};

static inline int str2pdev(const char *str)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(pdev_str_tab); i++) {
        if (!strcmp(pdev_str_tab[i].name, str))
            return pdev_str_tab[i].value;
    }

    ALOGE("unkown platform device:%s", str);
    return OUT_NONE;
}

static inline char* pdev2str(char *str, const int val)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(pdev_str_tab); i++) {
        if (pdev_str_tab[i].value == (unsigned int)val) {
            strcpy(str, pdev_str_tab[i].name);
            return str;
        }
    }

    ALOGE("unkown platform device:%d", val);
    return str;
}

struct card_config {
    int card;
    int port;
    int channels;
    int rate;
    int period_size;
    int period_count;
};

/* platform device profile */
struct pdev_profile {
    char *devices;
    struct card_config frontend;

    unsigned int in_bec;                    /* in backend count */
    struct card_config in_be[NUM_BACKEND];  /* in backend */

    unsigned int out_bec;                       /* out backend count */
    struct card_config out_be[NUM_BACKEND];     /* out backend */
};

enum plugin_enable_flag {
    PLUGIN_NONE = 0x0,
    PLUGIN_DUMP_DATA = 0x1,
    PLUGIN_3D_AUDIO = 0x2,
    PLUGIN_BP = 0x4,
};
typedef uint32_t plugin_enable_flag_t;

struct mic_info {
    char device_id[AUDIO_MICROPHONE_ID_MAX_LEN];
    size_t channel_count;
    audio_microphone_channel_mapping_t channel_mapping[AUDIO_CHANNEL_COUNT_MAX];
};

struct snd_device_to_mic_map {
    struct mic_info microphones[AUDIO_MICROPHONE_MAX_COUNT];
    size_t mic_count;
};

/* platform_info read from audio_platform_info.xml */
struct platform_info {
    plugin_enable_flag_t plugins_config;

    struct card_config mixer_config;
    struct pdev_profile *profiles[NUM_PROFILE];

    char *pdev_path[NUM_PLATFORM_DEVICE];

    uint32_t declared_mic_count;
    struct audio_microphone_characteristic_t microphones[AUDIO_MICROPHONE_MAX_COUNT];
    struct snd_device_to_mic_map mic_map[NUM_PLATFORM_DEVICE];
};

struct platform {
    struct sunxi_audio_device *adev;
    struct audio_route *ar;
    int in_bec;
    int out_bec;
    struct pcm *in_backends[NUM_BACKEND];
    struct pcm *out_backends[NUM_BACKEND];

    struct platform_info *info;

    use_case_t uc;
};

struct platform *platform_init(struct sunxi_audio_device *adev);
void platform_exit(struct platform *platform);

int reset_platform_path(struct platform *platform);
int update_platform_path(struct platform *platform);

int get_platform_phone_device(audio_devices_t devices,
                              const struct platform *platform);
int get_platform_device(audio_mode_t mode, audio_devices_t devices,
                        const struct platform *platform);
int get_platform_path(char *path_name, const struct platform *platform,
                      int platform_device);

int get_platform_snd_card_config(int *card, int *port,
                                 struct pcm_config *pcm_conf,
                                 int platform_device,
                                 const struct platform *platform);

/*TODO: now only the channels is updated*/
int update_platform_snd_card_config(
                                 struct pcm_config new_pcm_conf,
                                 int platform_device,
                                 const struct platform *platform);

int apply_platform_path(struct platform *platform, char *mixer_path);

int apply_and_update_platform_path(struct platform *platform, char *mixer_path);

int enable_platform_backend_pcm(int platform_device, struct platform *platform,
                                int direction);
int disable_platform_backend_pcm(struct platform *platform, int direction);

int platform_dump(const struct platform *platform, int fd);

int platform_plugins_process(const struct platform *platform, int flag);
int platform_plugins_process_select_devices(const struct platform *platform,
                                            int flag,
                                            int mode, int out_devices,
                                            int in_devices);

bool platform_set_microphone_characteristic(struct platform_info *plat_info,
                                            struct audio_microphone_characteristic_t mic);
bool platform_set_microphone_map(struct platform_info *plat_info, int in_snd_device,
                                 const struct mic_info *mic_info);
int platform_get_microphones(struct platform *platform,
                             struct audio_microphone_characteristic_t *mic_array,
                             size_t *mic_count);
int platform_get_active_microphones(struct platform *platform, unsigned int channels,
                                    struct audio_microphone_characteristic_t *mic_array,
                                    size_t *mic_count);

unsigned int platform_get_value_by_name(struct platform *platform, const char *name);

void platform_set_value(struct platform *platform, int id, int value, unsigned int num_values);

int platform_plugins_process_start_stream(const struct platform *platform,
                                          int flag,
                                          struct pcm_config config);

int platform_plugins_process_read_write(const struct platform *platform,
                                        int flag,
                                        struct pcm_config config, void *buffer,
                                        size_t bytes);

#endif

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

#define LOG_TAG "audio_platform"
//#define LOG_NDEBUG 0
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include <system/audio.h>
#include <audio_route/audio_route.h>
#include <cutils/log.h>
#include <expat.h>
#include "platform.h"
#include "tinyalsa/asoundlib.h"

/* log print attr for debug */
#define PRINT_ATTR 0

struct platform_info info;
static int profile_index = -1;

struct platform_plugins {
    struct audio_plugin *plugin;
    plugin_enable_flag_t enable_flag; /* Indicate conditions for enable this plugin */
};

struct audio_route {
    struct mixer *mixer;
    unsigned int num_mixer_ctls;
    struct mixer_state *mixer_state;

    unsigned int mixer_path_size;
    unsigned int num_mixer_paths;
    struct mixer_path *mixer_path;
};

extern struct audio_plugin dump_data;
extern struct audio_plugin audio_3d;
struct platform_plugins queue[] = {
    {&dump_data, PLUGIN_DUMP_DATA},
    /*{&audio_3d, PLUGIN_3D_AUDIO},*/
};

struct audio_string_to_enum {
    const char* name;
    unsigned int value;
};

static int in_snd_device;

static const struct audio_string_to_enum mic_locations[AUDIO_MICROPHONE_LOCATION_CNT] = {
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_LOCATION_UNKNOWN),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_LOCATION_MAINBODY),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_LOCATION_MAINBODY_MOVABLE),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_LOCATION_PERIPHERAL),
};

static const struct audio_string_to_enum mic_directionalities[AUDIO_MICROPHONE_DIRECTIONALITY_CNT] = {
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_DIRECTIONALITY_OMNI),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_DIRECTIONALITY_BI_DIRECTIONAL),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_DIRECTIONALITY_UNKNOWN),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_DIRECTIONALITY_CARDIOID),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_DIRECTIONALITY_HYPER_CARDIOID),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_DIRECTIONALITY_SUPER_CARDIOID),
};

static const struct audio_string_to_enum mic_channel_mapping[AUDIO_MICROPHONE_CHANNEL_MAPPING_CNT] = {
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_CHANNEL_MAPPING_UNUSED),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_CHANNEL_MAPPING_DIRECT),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_MICROPHONE_CHANNEL_MAPPING_PROCESSED),
};

static const struct audio_string_to_enum device_in_types[] = {
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_AMBIENT),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_COMMUNICATION),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_BUILTIN_MIC),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_WIRED_HEADSET),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_AUX_DIGITAL),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_HDMI),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_VOICE_CALL),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_TELEPHONY_RX),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_BACK_MIC),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_REMOTE_SUBMIX),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_ANLG_DOCK_HEADSET),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_DGTL_DOCK_HEADSET),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_USB_ACCESSORY),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_USB_DEVICE),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_FM_TUNER),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_TV_TUNER),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_LINE),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_SPDIF),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_BLUETOOTH_A2DP),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_LOOPBACK),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_IP),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_BUS),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_PROXY),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_USB_HEADSET),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_BLUETOOTH_BLE),
    AUDIO_MAKE_STRING_FROM_ENUM(AUDIO_DEVICE_IN_DEFAULT),
};

enum {
    AUDIO_MICROPHONE_CHARACTERISTIC_NONE = 0u, // 0x0
    AUDIO_MICROPHONE_CHARACTERISTIC_SENSITIVITY = 1u, // 0x1
    AUDIO_MICROPHONE_CHARACTERISTIC_MAX_SPL = 2u, // 0x2
    AUDIO_MICROPHONE_CHARACTERISTIC_MIN_SPL = 4u, // 0x4
    AUDIO_MICROPHONE_CHARACTERISTIC_ORIENTATION = 8u, // 0x8
    AUDIO_MICROPHONE_CHARACTERISTIC_GEOMETRIC_LOCATION = 16u, // 0x10
    AUDIO_MICROPHONE_CHARACTERISTIC_ALL = 31u, /* ((((SENSITIVITY | MAX_SPL) | MIN_SPL)
                                                  | ORIENTATION) | GEOMETRIC_LOCATION) */
};

static bool find_enum_by_string(const struct audio_string_to_enum * table, const char * name,
                                int32_t len, unsigned int *value)
{
    if (table == NULL) {
        ALOGE("%s: table is NULL", __func__);
        return false;
    }

    if (name == NULL) {
        ALOGE("null key");
        return false;
    }

    for (int i = 0; i < len; i++) {
        if (!strcmp(table[i].name, name)) {
            *value = table[i].value;
            return true;
        }
    }
    return false;
}

static void parse_root(const XML_Char **attr)
{
    UNUSED(attr);
    ALOGV("%s", __func__);
}

static void parse_platform_audio_plugins_config(const XML_Char **attr)
{
    ALOGV("%s", __func__);
    UNUSED(attr);
}

static void parse_plugin(const XML_Char **attr)
{
    ALOGV("%s", __func__);

    if (!strcmp(attr[1], "audio_3d_surround") && !strcmp(attr[3], "on")) {
        info.plugins_config |= PLUGIN_3D_AUDIO;
    } else if(!strcmp(attr[1], "dump_data") && !strcmp(attr[3], "on")) {
        info.plugins_config |= PLUGIN_DUMP_DATA;
    } else if(!strcmp(attr[1], "bp") && !strcmp(attr[3], "on")) {
        info.plugins_config |= PLUGIN_BP;
    }

    ALOGD("plugins_config:%#x", info.plugins_config);
}

static void parse_platform_devices_profile(const XML_Char **attr)
{
    UNUSED(attr);
    ALOGV("%s", __func__);
    if (profile_index > NUM_PROFILE -1) {
        ALOGE("profile_index:%d, max_index:%d.", profile_index, NUM_PROFILE-1);
        return;
    }

    profile_index++;

    info.profiles[profile_index] =
        calloc(1, sizeof(struct pdev_profile));
    if (!info.profiles[profile_index]) {
        ALOGE("can't calloc platform_devices_profile");
        return;
    }
}

static void parse_platform_devices(const XML_Char **attr)
{
    UNUSED(attr);
    ALOGV("%s: profile[%d] devices:%s", __func__, profile_index, attr[1]);
    info.profiles[profile_index]->devices = strdup(attr[1]);
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
            ALOGE("can't open %s, use card0", path);
            return 0;
        }

        ret = read(fd, name, sizeof(name));
        close(fd);
        if (ret > 0) {
            name[ret-1] = '\0';
            if (!strcmp(name, card_name))
                return i;
        }
    }

    ALOGW("can't find card:%s, use card0", card_name);
    return 0;
}

static void parse_snd_card_config(const XML_Char **attr)
{
    ALOGV("%s", __func__);
    unsigned int i = 0;
    struct pdev_profile *profile;
    struct card_config card_config;

    if (profile_index >=0 && profile_index < NUM_PROFILE) {
        profile = info.profiles[profile_index];
    } else {
        ALOGE("profile_index:%d, max_index:%d.", profile_index, NUM_PROFILE-1);
        return;
    }

    for (i = 0;  attr[i]; i += 2) {
        ALOGV_IF(PRINT_ATTR, "attr[%d]:%s=%s", i, attr[i], attr[i+1]);

        if (!strcmp(attr[i], "card_name")) {
            card_config.card = get_card(attr[i+1]);
        } else if (!strcmp(attr[i], "device")) {
            card_config.port = atoi(attr[i+1]);
        } else if (!strcmp(attr[i], "channels")) {
            card_config.channels = atoi(attr[i+1]);
        } else if (!strcmp(attr[i], "rate")) {
            card_config.rate = atoi(attr[i+1]);
        } else if (!strcmp(attr[i], "period_size")) {
            card_config.period_size = atoi(attr[i+1]);
        } else if (!strcmp(attr[i], "period_count")) {
            card_config.period_count = atoi(attr[i+1]);
        }
    }

    /* find type attr */
    for (i = 0;  attr[i]; i += 2) {
        if (!strcmp(attr[i], "type")) {
            ALOGV_IF(PRINT_ATTR, "attr[%d]:%s=%s", i, attr[i], attr[i+1]);
            if (!strcmp(attr[i+1], "frontend")) {
                memcpy(&profile->frontend, &card_config,
                       sizeof(struct card_config));
            } else if(!strcmp(attr[i+1], "in_backend")) {
                int count = profile->in_bec;
                memcpy(&profile->in_be[count], &card_config,
                       sizeof(struct card_config));
                profile->in_bec++;
            } else if(!strcmp(attr[i+1], "out_backend")) {
                int count = profile->out_bec;
                memcpy(&profile->out_be[count], &card_config,
                       sizeof(struct card_config));
                profile->out_bec++;
            } else if(!strcmp(attr[i+1], "backend")) {
                int in_count = profile->in_bec;
                int out_count = profile->out_bec;
                memcpy(&profile->in_be[in_count], &card_config,
                       sizeof(struct card_config));
                memcpy(&profile->out_be[out_count], &card_config,
                       sizeof(struct card_config));
                profile->in_bec++;
                profile->out_bec++;
            } else {
                ALOGE("unknow type:%s", attr[i+1]);
                return;
            }
        }
    }
}

static void parse_platform_device_path(const XML_Char **attr)
{
    UNUSED(attr);
    ALOGV("%s", __func__);
}

static void parse_device_path_map(const XML_Char **attr)
{
    unsigned int i = 0;
    int pdev = OUT_NONE;
    char *path = NULL;

    /* get device and path value */
    for (i = 0;  attr[i]; i += 2) {
        if (!strcmp(attr[i], "device")) {
            pdev = str2pdev(attr[i+1]);
        } else if (!strcmp(attr[i], "path")) {
            path = strdup(attr[i+1]);
            break;
        }
    }

    info.pdev_path[pdev] = path;
    ALOGV_IF(PRINT_ATTR, "device:%d,path:%s", pdev, path);
}

bool platform_set_microphone_map(struct platform_info *plat_info, int in_snd_device,
                                 const struct mic_info *mic_info) {
    //struct platform_info *info = platform->info;
    if (in_snd_device < IN_AMIC || in_snd_device >= IN_APH_REC) {
        ALOGE("%s: Sound device not valid", __func__);
        return false;
    }
    size_t m_count = plat_info->mic_map[in_snd_device].mic_count++;
    if (m_count >= AUDIO_MICROPHONE_MAX_COUNT) {
        ALOGE("%s: Microphone count is greater than max allowed value", __func__);
        plat_info->mic_map[in_snd_device].mic_count--;
        return false;
    }
    plat_info->mic_map[in_snd_device].microphones[m_count] = *mic_info;
    return true;
}

static void parse_platform_snd_dev(const XML_Char **attr)
{
    uint32_t curIdx = 0;
    in_snd_device = OUT_NONE;

    if (strcmp(attr[curIdx++], "in_snd_device")) {
        ALOGE("%s: snd_device not found", __func__);
        return;
    }
    in_snd_device = str2pdev((char *)attr[curIdx++]);
    if (in_snd_device < IN_AMIC ||
            in_snd_device > IN_APH_REC) {
        ALOGE("%s: Sound device not valid", __func__);
        in_snd_device = OUT_NONE;
    }

    return;
}

static void parse_platform_mic_info(const XML_Char **attr)
{
    uint32_t curIdx = 0;
    struct mic_info microphone;

    memset(&microphone.channel_mapping, AUDIO_MICROPHONE_CHANNEL_MAPPING_UNUSED,
               sizeof(microphone.channel_mapping));

    if (strcmp(attr[curIdx++], "mic_device_id")) {
        ALOGE("%s: mic_device_id not found", __func__);
        goto on_error;
    }
    strlcpy(microphone.device_id,
                (char *)attr[curIdx++], AUDIO_MICROPHONE_ID_MAX_LEN);

    if (strcmp(attr[curIdx++], "channel_mapping")) {
        ALOGE("%s: channel_mapping not found", __func__);
        goto on_error;
    }
    const char *token = strtok((char *)attr[curIdx++], " ");
    uint32_t idx = 0;
    while (token) {
        if (!find_enum_by_string(mic_channel_mapping, token,
                AUDIO_MICROPHONE_CHANNEL_MAPPING_CNT,
                &microphone.channel_mapping[idx++])) {
            ALOGE("%s: channel_mapping %s in %s not found!",
                      __func__, attr[--curIdx], PLATFORM_INFO_XML_PATH);
            goto on_error;
        }
        token = strtok(NULL, " ");
    }
    microphone.channel_count = idx;

    platform_set_microphone_map(&info, in_snd_device,
                                    &microphone);
    return;
on_error:
    in_snd_device = OUT_NONE;
    return;
}

static void parse_platform_microphone_characteristic(const XML_Char **attr) {
    struct audio_microphone_characteristic_t microphone;
    uint32_t curIdx = 0;

    if (strcmp(attr[curIdx++], "valid_mask")) {
        ALOGE("%s: valid_mask not found", __func__);
        goto done;
    }
    uint32_t valid_mask = atoi(attr[curIdx++]);

    if (strcmp(attr[curIdx++], "device_id")) {
        ALOGE("%s: device_id not found", __func__);
        goto done;
    }
    if (strlen(attr[curIdx]) > AUDIO_MICROPHONE_ID_MAX_LEN) {
        ALOGE("%s: device_id %s is too long", __func__, attr[curIdx]);
        goto done;
    }
    strcpy(microphone.device_id, attr[curIdx++]);

    if (strcmp(attr[curIdx++], "type")) {
        ALOGE("%s: device not found", __func__);
        goto done;
    }
    if (!find_enum_by_string(device_in_types, (char*)attr[curIdx++],
            ARRAY_SIZE(device_in_types), &microphone.device)) {
        ALOGE("%s: type %s in %s not found!",
              __func__, attr[--curIdx], PLATFORM_INFO_XML_PATH);
        goto done;
    }

    if (strcmp(attr[curIdx++], "address")) {
        ALOGE("%s: address not found", __func__);
        goto done;
    }
    if (strlen(attr[curIdx]) > AUDIO_DEVICE_MAX_ADDRESS_LEN) {
        ALOGE("%s, address %s is too long", __func__, attr[curIdx]);
        goto done;
    }
    strcpy(microphone.address, attr[curIdx++]);
    if (strlen(microphone.address) == 0) {
        // If the address is empty, populate the address according to device type.
        if (microphone.device == AUDIO_DEVICE_IN_BUILTIN_MIC) {
            strcpy(microphone.address, AUDIO_BOTTOM_MICROPHONE_ADDRESS);
        } else if (microphone.device == AUDIO_DEVICE_IN_BACK_MIC) {
            strcpy(microphone.address, AUDIO_BACK_MICROPHONE_ADDRESS);
        }
    }

    if (strcmp(attr[curIdx++], "location")) {
        ALOGE("%s: location not found", __func__);
        goto done;
    }
    if (!find_enum_by_string(mic_locations, (char*)attr[curIdx++],
            AUDIO_MICROPHONE_LOCATION_CNT, &microphone.location)) {
        ALOGE("%s: location %s in %s not found!",
              __func__, attr[--curIdx], PLATFORM_INFO_XML_PATH);
        goto done;
    }

    if (strcmp(attr[curIdx++], "group")) {
        ALOGE("%s: group not found", __func__);
        goto done;
    }
    microphone.group = atoi(attr[curIdx++]);

    if (strcmp(attr[curIdx++], "index_in_the_group")) {
        ALOGE("%s: index_in_the_group not found", __func__);
        goto done;
    }
    microphone.index_in_the_group = atoi(attr[curIdx++]);

    if (strcmp(attr[curIdx++], "directionality")) {
        ALOGE("%s: directionality not found", __func__);
        goto done;
    }
    if (!find_enum_by_string(mic_directionalities, (char*)attr[curIdx++],
                AUDIO_MICROPHONE_DIRECTIONALITY_CNT, &microphone.directionality)) {
        ALOGE("%s: directionality %s in %s not found!",
              __func__, attr[--curIdx], PLATFORM_INFO_XML_PATH);
        goto done;
    }

    if (strcmp(attr[curIdx++], "num_frequency_responses")) {
        ALOGE("%s: num_frequency_responses not found", __func__);
        goto done;
    }
    microphone.num_frequency_responses = atoi(attr[curIdx++]);
    if (microphone.num_frequency_responses > AUDIO_MICROPHONE_MAX_FREQUENCY_RESPONSES) {
        ALOGE("%s: num_frequency_responses is too large", __func__);
        goto done;
    }
    if (microphone.num_frequency_responses > 0) {
        if (strcmp(attr[curIdx++], "frequencies")) {
            ALOGE("%s: frequencies not found", __func__);
            goto done;
        }
        char *token = strtok((char *)attr[curIdx++], " ");
        uint32_t num_frequencies = 0;
        while (token) {
            microphone.frequency_responses[0][num_frequencies++] = atof(token);
            if (num_frequencies > AUDIO_MICROPHONE_MAX_FREQUENCY_RESPONSES) {
                ALOGE("%s: num %u of frequency is too large", __func__, num_frequencies);
                goto done;
            }
            token = strtok(NULL, " ");
        }

        if (strcmp(attr[curIdx++], "responses")) {
            ALOGE("%s: responses not found", __func__);
            goto done;
        }
        token = strtok((char *)attr[curIdx++], " ");
        uint32_t num_responses = 0;
        while (token) {
            microphone.frequency_responses[1][num_responses++] = atof(token);
            if (num_responses > AUDIO_MICROPHONE_MAX_FREQUENCY_RESPONSES) {
                ALOGE("%s: num %u of response is too large", __func__, num_responses);
                goto done;
            }
            token = strtok(NULL, " ");
        }

        if (num_frequencies != num_responses
                || num_frequencies != microphone.num_frequency_responses) {
            ALOGE("%s: num of frequency and response not match: %u, %u, %u",
                  __func__, num_frequencies, num_responses, microphone.num_frequency_responses);
            goto done;
        }
    }

    if (valid_mask & AUDIO_MICROPHONE_CHARACTERISTIC_SENSITIVITY) {
        if (strcmp(attr[curIdx++], "sensitivity")) {
            ALOGE("%s: sensitivity not found", __func__);
            goto done;
        }
        microphone.sensitivity = atof(attr[curIdx++]);
    } else {
        microphone.sensitivity = AUDIO_MICROPHONE_SENSITIVITY_UNKNOWN;
    }

    if (valid_mask & AUDIO_MICROPHONE_CHARACTERISTIC_MAX_SPL) {
        if (strcmp(attr[curIdx++], "max_spl")) {
            ALOGE("%s: max_spl not found", __func__);
            goto done;
        }
        microphone.max_spl = atof(attr[curIdx++]);
    } else {
        microphone.max_spl = AUDIO_MICROPHONE_SPL_UNKNOWN;
    }

    if (valid_mask & AUDIO_MICROPHONE_CHARACTERISTIC_MIN_SPL) {
        if (strcmp(attr[curIdx++], "min_spl")) {
            ALOGE("%s: min_spl not found", __func__);
            goto done;
        }
        microphone.min_spl = atof(attr[curIdx++]);
    } else {
        microphone.min_spl = AUDIO_MICROPHONE_SPL_UNKNOWN;
    }

    if (valid_mask & AUDIO_MICROPHONE_CHARACTERISTIC_ORIENTATION) {
        if (strcmp(attr[curIdx++], "orientation")) {
            ALOGE("%s: orientation not found", __func__);
            goto done;
        }
        char *token = strtok((char *)attr[curIdx++], " ");
        float orientation[3];
        uint32_t idx = 0;
        while (token) {
            orientation[idx++] = atof(token);
            if (idx > 3) {
                ALOGE("%s: orientation invalid", __func__);
                goto done;
            }
            token = strtok(NULL, " ");
        }
        if (idx != 3) {
            ALOGE("%s: orientation invalid", __func__);
            goto done;
        }
        microphone.orientation.x = orientation[0];
        microphone.orientation.y = orientation[1];
        microphone.orientation.z = orientation[2];
    } else {
        microphone.orientation.x = 0.0f;
        microphone.orientation.y = 0.0f;
        microphone.orientation.z = 0.0f;
    }

    if (valid_mask & AUDIO_MICROPHONE_CHARACTERISTIC_GEOMETRIC_LOCATION) {
        if (strcmp(attr[curIdx++], "geometric_location")) {
            ALOGE("%s: geometric_location not found", __func__);
            goto done;
        }
        char *token = strtok((char *)attr[curIdx++], " ");
        float geometric_location[3];
        uint32_t idx = 0;
        while (token) {
            geometric_location[idx++] = atof(token);
            if (idx > 3) {
                ALOGE("%s: geometric_location invalid", __func__);
                goto done;
            }
            token = strtok(NULL, " ");
        }
        if (idx != 3) {
            ALOGE("%s: geometric_location invalid", __func__);
            goto done;
        }
        microphone.geometric_location.x = geometric_location[0];
        microphone.geometric_location.y = geometric_location[1];
        microphone.geometric_location.z = geometric_location[2];
    } else {
        microphone.geometric_location.x = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
        microphone.geometric_location.y = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
        microphone.geometric_location.z = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
    }

    platform_set_microphone_characteristic(&info, microphone);
done:
    return;
}

static void start_tag(void *userdata __unused, const XML_Char *tag_name,
                      const XML_Char **attr)
{

    if (!strcmp(tag_name, "snd_card_config")) {
        parse_snd_card_config(attr);
    } else if (!strcmp(tag_name, "platform_devices_profile")) {
        parse_platform_devices_profile(attr);
    } else if (!strcmp(tag_name, "platform_devices")) {
        parse_platform_devices(attr);
    } else if (!strcmp(tag_name, "platform_device_path")) {
        parse_platform_device_path(attr);
    } else if (!strcmp(tag_name, "device_path_map")) {
        parse_device_path_map(attr);
    } else if (!strcmp(tag_name, "platform_audio_plugins_config")) {
        parse_platform_audio_plugins_config(attr);
    } else if (!strcmp(tag_name, "plugin")) {
        parse_plugin(attr);
    } else if (!strcmp(tag_name, "snd_dev")) {
        parse_platform_snd_dev(attr);
    } else if (!strcmp(tag_name, "mic_info")) {
        parse_platform_mic_info(attr);
    }else if (!strcmp(tag_name, "microphone")) {
        parse_platform_microphone_characteristic(attr);
    }
}

static void end_tag(void *userdata __unused, const XML_Char *tag_name)
{
    UNUSED(tag_name);
    //ALOGV("%s, tag_name:%s", __func__, tag_name);
}

static int parse_xml()
{
    XML_Parser parser;
    FILE *file;
    int ret = 0;
    int bytes_read;
    void *buf;
    static const uint32_t kBufSize = 1024;


    file = fopen(PLATFORM_INFO_XML_PATH, "r");
    if (!file) {
        ALOGD("%s: Failed to open %s, using defaults.",
            __func__, PLATFORM_INFO_XML_PATH);
        ret = -ENODEV;
        goto done;
    }
    ALOGV("%s:line:%d:file:%p", __func__, __LINE__, file);

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        ALOGE("%s: Failed to create XML parser!", __func__);
        ret = -ENODEV;
        goto err_close_file;
    }
    ALOGV("%s:line:%d:parser:%p", __func__, __LINE__, parser);

    XML_SetElementHandler(parser, start_tag, end_tag);

    while (1) {
        buf = XML_GetBuffer(parser, kBufSize);
        if (buf == NULL) {
            ALOGE("%s: XML_GetBuffer failed", __func__);
            ret = -ENOMEM;
            goto err_free_parser;
        }

        bytes_read = fread(buf, 1, kBufSize, file);
        if (bytes_read < 0) {
            ALOGE("%s: fread failed, bytes read = %d", __func__, bytes_read);
             ret = bytes_read;
            goto err_free_parser;
        }

        if (XML_ParseBuffer(parser, bytes_read,
                            bytes_read == 0) == XML_STATUS_ERROR) {
            ALOGE("%s: XML_ParseBuffer failed, for %s",
                __func__, PLATFORM_INFO_XML_PATH);
            ret = -EINVAL;
            goto err_free_parser;
        }

        if (bytes_read == 0)
            break;
    }

err_free_parser:
    XML_ParserFree(parser);
err_close_file:
    fclose(file);
done:
    return ret;
}

struct platform *platform_init(struct sunxi_audio_device *adev)
{
    ALOGV("platform_init");
    struct platform *platform = NULL;
    int card;
    int port;
    struct pcm_config pcm_conf;

    platform = calloc(1, sizeof(struct platform));
    if (!platform) {
        ALOGE("can't calloc platform");
        return NULL;
    }

    info.declared_mic_count = 0;
    parse_xml();

    platform->info = &info;

    /* select SPK snd card id as default mixer card id.
     * NOTE: There may be multiple snd card in the future.
     */
    get_platform_snd_card_config(&card, &port, &pcm_conf, OUT_SPK, platform);
    platform->info->mixer_config.card = card;

    /* load mixer config from mixer xml file */
    platform->ar = audio_route_init(platform->info->mixer_config.card,
                                    MIXER_PATHS_XML_PATH);

    platform->uc = get_use_case();
    platform->adev = adev;

    return platform;
}

void platform_exit(struct platform *platform)
{
    ALOGV("platform_exit");
    struct platform_info *info = platform->info;
    unsigned int i;

    /* free platform device profile */
    for (i = 0; i < ARRAY_SIZE(info->profiles); i++) {
        if (!info->profiles[i]) {
            free(info->profiles[i]);
            info->profiles[i] = NULL;
        }
    }

    /* free device path */
    for (i = 0; i < ARRAY_SIZE(info->pdev_path); i++) {
        if (!info->pdev_path[i]) {
            free(info->pdev_path[i]);
            info->pdev_path[i] = NULL;
        }
    }

    if (platform->ar)
        audio_route_free(platform->ar);

    if (platform)
        free(platform);
}

static int snd_card_config_dump(const struct card_config *card, int fd)
{
    dprintf(fd, "\t\tcard_config:\n"
                "\t\t\tcard:%d\n"
                "\t\t\tport:%d\n"
                "\t\t\tperiod_count:%d\n"
                "\t\t\tperiod_size:%d\n"
                "\t\t\tchannels:%d\n"
                "\t\t\trate:%d\n",
                card->card,
                card->port,
                card->period_count,
                card->period_size,
                card->channels,
                card->rate);
    return 0;
}


static int platform_devices_profile_dump(
           const struct pdev_profile *prof, int fd)
{
    unsigned int i;

    dprintf(fd, "\tplatform_devices_profile_dump:\n"
                "\t\tdevices:%s\n",
                prof->devices);

    dprintf(fd, "\t\tfrontend dump:\n");
    snd_card_config_dump(&prof->frontend, fd);

    for (i = 0; i < prof->in_bec; i++) {
        dprintf(fd, "\t\tin_backend[%d] dump:\n", i);
        snd_card_config_dump(&prof->in_be[i], fd);
    }

    for (i = 0; i < prof->out_bec; i++) {
        dprintf(fd, "\t\tout_backend[%d] dump:\n", i);
        snd_card_config_dump(&prof->out_be[i], fd);
    }

    return 0;
}

static int platform_info_dump(const struct platform_info *info, int fd)
{
    int i;
    struct pdev_profile *prof = NULL;

    dprintf(fd, "\tplatform_info_dump:\n"
                "\t\tplugins_config:%#x\n"
                "\t\tmixer card_id:%d\n",
                info->plugins_config,
                info->mixer_config.card);

    for (i = 0; i < NUM_PROFILE; i++) {
        prof = info->profiles[i];
        if (prof) {
            platform_devices_profile_dump(prof, fd);
        }
    }

    dprintf(fd, "\tplatform device path dump:\n");
    for (i = 0; i < NUM_PLATFORM_DEVICE; i++) {
        if (info->pdev_path[i]) {
            dprintf(fd, "\t\tdevice_path[%d]=%s\n",
                    i, info->pdev_path[i]);
        }
    }

    return 0;
}

int platform_dump(const struct platform *platform, int fd)
{
    ALOGV("platform_dump");

    struct platform_info *info = platform->info;

    dprintf(fd, "\tplatform_dump:\n"
                "\t\tar:%p\n"
                "\t\tinfo:%p\n"
                "\t\tin_backends[0]:%p\n"
                "\t\tout_backends[0]:%p\n"
                "\t\tuc:%#x\n",
                platform->ar,
                platform->info,
                platform->in_backends[0],
                platform->out_backends[0],
                platform->uc);

    if (info) {
        platform_info_dump(info, fd);
    }

    return 0;
}

int reset_platform_path(struct platform *platform)
{
    struct platform_info *info = platform->info;

    audio_route_reset(platform->ar);
    audio_route_update_mixer(platform->ar);

    return 0;
}

int reset_platform_in_path(struct platform *platform)
{
    struct platform_info *info = platform->info;

    audio_route_apply_path(platform->ar, "in-reset");
    audio_route_update_mixer(platform->ar);

    return 0;
}

int reset_platform_out_path(struct platform *platform)
{
    struct platform_info *info = platform->info;

    audio_route_apply_path(platform->ar, "out-reset");
    audio_route_update_mixer(platform->ar);

    return 0;
}


int update_platform_path(struct platform *platform)
{
    audio_route_update_mixer(platform->ar);

    return 0;
}

int get_platform_phone_device(audio_devices_t devices,
                              const struct platform *platform)
{
    struct platform_info *info = platform->info;
    int pdev = OUT_NONE;

    if (platform->uc & UC_DPHONE) {
        if (devices & AUDIO_DEVICE_OUT_EARPIECE) {
            pdev = (platform->uc & UC_EAR) ? DPH_EAR : DPH_SPK;
        } else if (devices & AUDIO_DEVICE_OUT_SPEAKER) {
            pdev = DPH_SPK;
        } else if (devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
            pdev = DPH_HP;
        } else if (devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            pdev = DPH_HS;
        } else if (devices & AUDIO_DEVICE_OUT_ALL_SCO) {
            pdev = DPH_BTSCO;
        } else {
            pdev = DPH_SPK;
        }
    } else {
        if (devices & AUDIO_DEVICE_OUT_EARPIECE) {
            pdev = (platform->uc & UC_EAR) ? APH_EAR : APH_SPK;
        } else if (devices & AUDIO_DEVICE_OUT_SPEAKER) {
            pdev = APH_SPK;
        } else if (devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
            pdev = APH_HP;
        } else if (devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            pdev = APH_HS;
        } else if (devices & AUDIO_DEVICE_OUT_ALL_SCO) {
            pdev = APH_BTSCO;
        } else {
            pdev = APH_SPK;
        }
    }

    char str[50] = "null";
    ALOGV("devices(%#x):platform phone device:%s(%#x)",
          devices, pdev2str(str, pdev), pdev);

    return pdev;
}

int get_platform_device(audio_mode_t mode, audio_devices_t devices,
                        const struct platform *platform)
{
    struct platform_info *info = platform->info;
    int pdev = OUT_NONE;

    /* 1. get platform device */
    if (devices & AUDIO_DEVICE_BIT_IN) {
        audio_devices_t indev = devices & (~AUDIO_DEVICE_BIT_IN);
        /* record device */
        if (indev & AUDIO_DEVICE_IN_BUILTIN_MIC) {
            pdev = (platform->uc & UC_DMIC) ? IN_DMIC : IN_AMIC;
        } else if (indev & AUDIO_DEVICE_IN_WIRED_HEADSET) {
            pdev = IN_HPMIC;
        } else if (indev & AUDIO_DEVICE_IN_ALL_SCO) {
            pdev = IN_BTSCO;
        } else {
            pdev = IN_AMIC;
        }
    } else {
        /* playback device */
        int headphone = AUDIO_DEVICE_OUT_WIRED_HEADSET |
                        AUDIO_DEVICE_OUT_WIRED_HEADPHONE;

        if (devices & AUDIO_DEVICE_OUT_EARPIECE) {
            pdev = OUT_EAR;
        } else if (devices & AUDIO_DEVICE_OUT_SPEAKER &&
                   devices & headphone) {
            pdev = (platform->uc & UC_DUAL_SPK) ? OUT_DULSPK_HP : OUT_SPK_HP;
        } else if (devices & AUDIO_DEVICE_OUT_SPEAKER) {
            pdev = (platform->uc & UC_DUAL_SPK) ? OUT_DULSPK : OUT_SPK;
        } else if (devices & headphone) {
            pdev = OUT_HP;
        } else if (devices & AUDIO_DEVICE_OUT_ALL_SCO) {
            pdev = OUT_BTSCO;
        } else if (devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            pdev = OUT_HDMI;
        } else {
            pdev = OUT_SPK;
        }
    }

    /* phone record and playback */
    if (AUDIO_MODE_IN_CALL == mode) {
        if (devices & AUDIO_DEVICE_BIT_IN) {
            pdev = (platform->uc & UC_DPHONE)? IN_DPH_REC : IN_APH_REC;
        } else {
            pdev = (platform->uc & UC_DPHONE)? OUT_DPH_PLAY : OUT_APH_PLAY;
        }
    }

    /* TODO: force use platform device */

    char str[50] = "null";
    ALOGV("mode(%#x),devices(%#x):platform device:%s(%#x)",
          mode, devices, pdev2str(str, pdev), pdev);

    return pdev;
}

int get_platform_path(char *path_name, const struct platform *platform,
                      int pdev)
{
    struct platform_info *info = platform->info;
    strcpy(path_name, info->pdev_path[pdev]);
    return 0;
}

int apply_platform_path(struct platform *platform, char *mixer_path)
{
    return audio_route_apply_path(platform->ar, mixer_path);
}

int apply_and_update_platform_path(struct platform *platform, char *mixer_path)
{
    return audio_route_apply_and_update_path(platform->ar, mixer_path);
}

int enable_platform_backend_pcm(int pdev,
                                struct platform *platform,
                                int direction)
{

    unsigned int count = 0;
    struct card_config *backend = NULL;

    int card = 0;
    int port = 0;
    struct pcm_config pcm_conf;
    struct platform_info *info = platform->info;
    struct pdev_profile *profile = NULL;
    char pdev_name[50];
    unsigned int i;

    pdev2str(pdev_name, pdev);

    /* 1. find platform_device profile */
    for (i = 0; i < ARRAY_SIZE(info->profiles); i++) {
        profile = info->profiles[i];
        if (profile) {
            if (strstr(profile->devices, pdev_name)) {
                break;
            }
        }
    }

    if (!profile) {
        ALOGE("can't get profile.");
        return -1;
    }

    if (direction == PCM_OUT) {
        count = profile->out_bec;
        backend = profile->out_be;
    } else if (direction == PCM_IN) {
        count = profile->in_bec;
        backend = profile->in_be;
    } else {
        ALOGE("unkown direction:%d", direction);
    }

    /* 2. open and start backend pcm */
    for (i = 0; i < count; i++) {
        card = backend[i].card;
        port = backend[i].port;
        pcm_conf.rate = backend[i].rate;
        pcm_conf.channels = backend[i].channels;
        pcm_conf.period_count = backend[i].period_count;
        pcm_conf.period_size = backend[i].period_size;

        ALOGD("enable backend_pcm:card(%d),port(%d),direction(%#x),"
              "rate(%d),ch(%d)"
              "period_count(%d),period_size(%d)",
              card, port, direction,
              pcm_conf.rate, pcm_conf.channels,
              pcm_conf.period_count, pcm_conf.period_size);

        struct pcm *backend_pcm = pcm_open(card, port, direction, &pcm_conf);
        if (!pcm_is_ready(backend_pcm)) {
            ALOGE("cannot open backend_pcm driver: %s",
                  pcm_get_error(backend_pcm));
            pcm_close(backend_pcm);
            return -ENOMEM;
        }

        pcm_start(backend_pcm);

        if (direction == PCM_OUT)
            platform->out_backends[platform->out_bec++] = backend_pcm;
        else if (direction == PCM_IN)
            platform->in_backends[platform->in_bec++] = backend_pcm;
    }

    return 0;
}

int disable_platform_backend_pcm(struct platform *platform, int direction)
{
    unsigned int i;

    ALOGV("disable backend pcm(direction:%s)",
          direction==PCM_OUT ? "PCM_OUT":"PCM_IN");

    if (PCM_OUT == direction)
        goto OUT;
    else if (PCM_IN == direction)
        goto IN;
    else
        return 0;

OUT:
    /* close out backend pcm */
    for (i = 0; i < ARRAY_SIZE(platform->out_backends); i++) {
        if (platform->out_backends[i]) {
            pcm_close(platform->out_backends[i]);
            platform->out_backends[i] = NULL;
        }
    }
    platform->out_bec = 0;
    return 0;

IN:
    /* close in backend pcm */
    for (i = 0; i < ARRAY_SIZE(platform->in_backends); i++) {
        if (platform->in_backends[i]) {
            pcm_close(platform->in_backends[i]);
            platform->in_backends[i] = NULL;
        }
    }
    platform->in_bec = 0;
    return 0;
}

int get_platform_snd_card_config(int *card, int *port,
                                 struct pcm_config *pcm_conf,
                                 int platform_device,
                                 const struct platform *platform)
{
    struct platform_info *info = platform->info;
    struct pdev_profile *profile = NULL;
    char dev_name[50];
    unsigned int i;

    pdev2str(dev_name, platform_device);

    /* find platform_device profile */
    for (i = 0; i < ARRAY_SIZE(info->profiles); i++) {
        profile = info->profiles[i];
        if (profile) {
            if (strstr(profile->devices, dev_name)) {
                break;
            }
        }
    }

    if (!profile) {
        ALOGE("can't get snd_card_config.");
        return -1;
    }

    /* get snd_card_config */
    *card = profile->frontend.card;
    *port = profile->frontend.port;
    pcm_conf->rate = profile->frontend.rate;
    pcm_conf->channels = profile->frontend.channels;
    pcm_conf->period_count = profile->frontend.period_count;
    pcm_conf->period_size = profile->frontend.period_size;

    return 0;
}

int update_platform_snd_card_config(
                                 struct pcm_config new_pcm_conf,
                                 int platform_device,
                                 const struct platform *platform)
{
    struct platform_info *info = platform->info;
    struct pdev_profile *profile = NULL;
    char dev_name[50];
    unsigned int i;

    pdev2str(dev_name, platform_device);

    /* find platform_device profile */
    for (i = 0; i < ARRAY_SIZE(info->profiles); i++) {
        profile = info->profiles[i];
        if (profile) {
            if (strstr(profile->devices, dev_name)) {
                break;
            }
        }
    }

    if (!profile) {
        ALOGE("can't get snd_card_config.");
        return -1;
    }

    /* update snd_card_config */
    profile->frontend.channels = new_pcm_conf.channels;
    profile->frontend.rate = new_pcm_conf.rate;

    return 0;
}

bool platform_set_microphone_characteristic(struct platform_info *plat_info,
                                            struct audio_microphone_characteristic_t mic) {
    //struct platform_info *info = platform->info;
    if (plat_info->declared_mic_count >= AUDIO_MICROPHONE_MAX_COUNT) {
        ALOGE("mic number is more than maximum number");
        return false;
    }
    for (size_t ch = 0; ch < AUDIO_CHANNEL_COUNT_MAX; ch++) {
        mic.channel_mapping[ch] = AUDIO_MICROPHONE_CHANNEL_MAPPING_UNUSED;
    }
    plat_info->microphones[plat_info->declared_mic_count++] = mic;
    return true;
}

int platform_get_microphones(struct platform *platform,
                             struct audio_microphone_characteristic_t *mic_array,
                             size_t *mic_count) {
    struct platform_info *info = platform->info;
    if (mic_count == NULL) {
        return -EINVAL;
    }
    if (mic_array == NULL) {
        return -EINVAL;
    }

    if (*mic_count == 0) {
        *mic_count = info->declared_mic_count;
        return 0;
    }

    size_t max_mic_count = *mic_count;
    size_t actual_mic_count = 0;
    for (size_t i = 0; i < max_mic_count && i < info->declared_mic_count; i++) {
        mic_array[i] = info->microphones[i];
        actual_mic_count++;
    }
    *mic_count = actual_mic_count;
    return 0;
}

int platform_get_active_microphones(struct platform *platform, unsigned int channels,
                                    struct audio_microphone_characteristic_t *mic_array,
                                    size_t *mic_count) {
    struct sunxi_audio_device *adev = platform->adev;
    struct platform_info *info = platform->info;
    if (mic_count == NULL || mic_array == NULL) {// || usecase == NULL) {
        return -EINVAL;
    }
    size_t max_mic_count = info->declared_mic_count;
    size_t actual_mic_count = 0;
    int in_pdev = 0;

    in_pdev = get_platform_device(adev->mode, adev->in_devices, platform);

    //OUT_NONE means zero
    if (in_pdev == OUT_NONE) {
        ALOGI("%s: No active microphones found", __func__);
        goto end;
    }

    size_t  active_mic_count = info->mic_map[in_pdev].mic_count;
    struct mic_info *m_info = info->mic_map[in_pdev].microphones;

    for (size_t i = 0; i < active_mic_count; i++) {
        unsigned int channels_for_active_mic = channels;
        if (channels_for_active_mic > m_info[i].channel_count) {
            channels_for_active_mic = m_info[i].channel_count;
        }
        for (size_t j = 0; j < max_mic_count; j++) {
            if (strcmp(info->microphones[j].device_id,
                       m_info[i].device_id) == 0) {
                mic_array[actual_mic_count] = info->microphones[j];
                for (size_t ch = 0; ch < channels_for_active_mic; ch++) {
                     mic_array[actual_mic_count].channel_mapping[ch] =
                             m_info[i].channel_mapping[ch];
                }
                actual_mic_count++;
                break;
            }
        }
    }
end:
    *mic_count = actual_mic_count;
    return 0;
}

unsigned int platform_get_value_by_name(struct platform *platform, const char *name)
{
    struct audio_route *ar = platform->ar;
    unsigned int val = 0;
    enum mixer_ctl_type type;
    unsigned int num_values;
    struct mixer_ctl *ctl;
    unsigned int i;

    ctl = mixer_get_ctl_by_name(ar->mixer, name);

    if (!ctl) {
        ALOGE("Invalid mixer control");
        return -1;
    }

    type = mixer_ctl_get_type(ctl);

    num_values = mixer_ctl_get_num_values(ctl);

    for (i = 0; i < num_values; i++) {
        switch (type)
        {
            case MIXER_CTL_TYPE_INT:
            val = mixer_ctl_get_value(ctl, i);
            break;
            case MIXER_CTL_TYPE_BOOL:
            val = mixer_ctl_get_value(ctl, i);
            break;

            default:
            printf(" unknown");
            printf(" now we only support int and bool ");
            break;
        };
    }
    return val;
}

void platform_set_value(struct platform *platform, int id, int value, unsigned int num_values)
{
    struct audio_route *ar = platform->ar;
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_ctl_values;
    unsigned int i;

    ctl = mixer_get_ctl(ar->mixer, id);

    if (!ctl) {
        return;
    }

    type = mixer_ctl_get_type(ctl);
    num_ctl_values = mixer_ctl_get_num_values(ctl);

    if (num_values == 1) {
        /* Set all values the same */

        for (i = 0; i < num_ctl_values; i++) {
            if (mixer_ctl_set_value(ctl, i, value)) {
                ALOGE("Error: invalid value");
                return;
            }
        }
    } else {
        /* Set multiple values */
        ALOGE("not support mutiple values yet!!");
    }
}

int platform_plugins_process(const struct platform *platform, int flag)
{
    struct platform_info *info = platform->info;
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(queue); i++) {
        if (info->plugins_config & queue[i].enable_flag) {
            if (queue[i].plugin)
                plugin_process(queue[i].plugin, flag);
        }
    }

    return 0;
}

int platform_plugins_process_select_devices(const struct platform *platform,
                                            int flag,
                                            int mode, int out_devices,
                                            int in_devices)
{
    struct platform_info *info = platform->info;
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(queue); i++) {
        if (info->plugins_config & queue[i].enable_flag) {
            if (queue[i].plugin)
                plugin_process_select_devices(queue[i].plugin, flag, mode,
                                              out_devices, in_devices);
        }
    }

    return 0;
}

int platform_plugins_process_start_stream(const struct platform *platform,
                                          int flag,
                                          struct pcm_config config)
{
    struct platform_info *info = platform->info;
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(queue); i++) {
        if (info->plugins_config & queue[i].enable_flag) {
            if (queue[i].plugin)
                plugin_process_start_stream(queue[i].plugin, flag, config);
        }
    }

    return 0;
}

int platform_plugins_process_read_write(const struct platform *platform,
                                        int flag,
                                        struct pcm_config config, void *buffer,
                                        size_t bytes)
{
    struct platform_info *info = platform->info;
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(queue); i++) {
        if (info->plugins_config & queue[i].enable_flag) {
            if (queue[i].plugin)
                plugin_process_read_write(queue[i].plugin, flag, config,
                                          buffer, bytes);
        }
    }

    return 0;
}

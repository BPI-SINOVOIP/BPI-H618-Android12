/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef SUNXI_COMPOSITION_ENGINE_DEFS_H
#define SUNXI_COMPOSITION_ENGINE_DEFS_H

#define MINIMUM_HARDWARE_LAYER_SIZE (16)

#define eHybridChannelWithAlpha  1
#define eAfbcBufferSupported     2
#define eHdrSupported            4

struct BandwidthLimit {
    int dramFrequency;
    int maxAvaliableBandwidth;
};

struct DEHardwareInfo {
    int videoChannelCount;
    int uiChannelCount;
    int videoHdrCount;
    int uiHdrCount;
    int featureMask;
};

#if defined(_board_dolphin_)
#define CONFIG_MAX_DISPLAY_ENGINE   2
#define CONFIG_MAX_BANDWIDTH_LEVE   3

const struct BandwidthLimit BandwidthConfigs[CONFIG_MAX_BANDWIDTH_LEVE] = {
    {672000, 49152000},
    {552000, 49152000},
    {432000, 30736000},
};

const struct DEHardwareInfo HardwareConfigs[CONFIG_MAX_DISPLAY_ENGINE] = {
    {1, 3, 0, 0, 0x00000000}, // DE0: 1 video channel, 3 ui channel, not any other feature
    {1, 1, 0, 0, 0x00000000}, // DE1: 1 video channel, 1 ui channel, not any other feature
};

#elif defined(_board_petrel_)
#define CONFIG_MAX_DISPLAY_ENGINE   2
#define CONFIG_MAX_BANDWIDTH_LEVE   3

const struct BandwidthLimit BandwidthConfigs[CONFIG_MAX_BANDWIDTH_LEVE] = {
    {672000, 49152000},
    {552000, 49152000},
    {432000, 30736000},
};

const struct DEHardwareInfo HardwareConfigs[CONFIG_MAX_DISPLAY_ENGINE] = {
    /*DE0: 1 video channel, 3 ui channel, not any other feature*/
    {1, 3, 1, 1, eHybridChannelWithAlpha | eAfbcBufferSupported | eHdrSupported},
    /* DE1: 1 video channel, 1 ui channel, not any other feature*/
    {1, 1, 0, 0, eHybridChannelWithAlpha},
};
#define CONFIG_MAX_DISPLAY_ENGINE   2

#elif defined(_board_venus_)
#define CONFIG_MAX_DISPLAY_ENGINE   2
#define CONFIG_MAX_BANDWIDTH_LEVE   3

const struct BandwidthLimit BandwidthConfigs[CONFIG_MAX_BANDWIDTH_LEVE] = {
    {672000, 49152000},
    {552000, 49152000},
    {432000, 30736000},
};

const struct DEHardwareInfo HardwareConfigs[CONFIG_MAX_DISPLAY_ENGINE] = {
    {1, 3, 0, 0, 0x00000000}, // DE0: 1 video channel, 3 ui channel, not any other feature
    {1, 1, 0, 0, 0x00000000}, // DE1: 1 video channel, 1 ui channel, not any other feature
};

#elif defined(_board_ceres_)
#define CONFIG_MAX_DISPLAY_ENGINE   2
#define CONFIG_MAX_BANDWIDTH_LEVE   3

const struct BandwidthLimit BandwidthConfigs[CONFIG_MAX_BANDWIDTH_LEVE] = {
    {672000, 30000000},
    {552000, 30000000},
    {432000, 30000000},
};

#define CONFIG_RECONFIG_BANDWIDTH_FOR_FULL_HD
const struct BandwidthLimit BandwidthConfigs_FullHD[CONFIG_MAX_BANDWIDTH_LEVE] = {
    {672000, 24883200},
    {552000, 24883200},
    {432000, 24883200},
};

const struct DEHardwareInfo HardwareConfigs[CONFIG_MAX_DISPLAY_ENGINE] = {
    {2, 2, 0, 0, eHybridChannelWithAlpha}, // DE0: 2 video channel, 2 ui channel, no other feature
    {1, 2, 0, 0, eHybridChannelWithAlpha}, // DE1: 1 video channel, 2 ui channel, no other feature
};

#elif defined(_board_pluto_)
#define CONFIG_MAX_DISPLAY_ENGINE   2
#define CONFIG_MAX_BANDWIDTH_LEVE   3

const struct BandwidthLimit BandwidthConfigs[CONFIG_MAX_BANDWIDTH_LEVE] = {
    {672000, 30000000},
    {552000, 30000000},
    {432000, 30000000},
};

const struct DEHardwareInfo HardwareConfigs[CONFIG_MAX_DISPLAY_ENGINE] = {
    {2, 2, 0, 0, eHybridChannelWithAlpha}, // DE0: 2 video channel, 2 ui channel, no other feature
    {1, 2, 0, 0, eHybridChannelWithAlpha}, // DE1: 1 video channel, 2 ui channel, no other feature
};
#elif defined(_board_apollo_) || defined(_board_epic_) || defined(_board_mercury_)

#define CONFIG_MAX_DISPLAY_ENGINE   2
#define CONFIG_MAX_BANDWIDTH_LEVE   3

const struct BandwidthLimit BandwidthConfigs[CONFIG_MAX_BANDWIDTH_LEVE] = {
    {720000, 57600000},
    {672000, 49152000},
    {552000, 49152000},
};

const struct DEHardwareInfo HardwareConfigs[CONFIG_MAX_DISPLAY_ENGINE] = {
    /* DE0: 1 video channel, 3 ui channel, not any other feature*/
    {1, 3, 1, 1, eHybridChannelWithAlpha | eAfbcBufferSupported | eHdrSupported},
    /*DE1: 1 video channel, 1 ui channel, not any other feature*/
    {1, 1, 0, 0, eHybridChannelWithAlpha},
};

#else
#error "Please add the corresponding platform definition"
#endif

#endif

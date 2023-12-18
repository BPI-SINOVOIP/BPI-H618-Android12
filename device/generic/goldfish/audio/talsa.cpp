/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <mutex>
#include <log/log.h>
#include "talsa.h"
#include "debug.h"

namespace android {
namespace hardware {
namespace audio {
namespace V6_0 {
namespace implementation {
namespace talsa {

namespace {

struct mixer *gMixer0 = nullptr;
int gMixerRefcounter0 = 0;
std::mutex gMixerMutex;

void mixerSetValueAll(struct mixer_ctl *ctl, int value) {
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    for (unsigned int i = 0; i < n; i++) {
        ::mixer_ctl_set_value(ctl, i, value);
    }
}

void mixerSetPercentAll(struct mixer_ctl *ctl, int percent) {
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    for (unsigned int i = 0; i < n; i++) {
        ::mixer_ctl_set_percent(ctl, i, percent);
    }
}

struct mixer *mixerGetOrOpenImpl(const unsigned card,
                                 struct mixer *&gMixer,
                                 int &refcounter) {
    if (!gMixer) {
        struct mixer *mixer = ::mixer_open(card);
        if (!mixer) {
            return FAILURE(nullptr);
        }

        mixerSetPercentAll(::mixer_get_ctl_by_name(mixer, "Master Playback Volume"), 100);
        mixerSetPercentAll(::mixer_get_ctl_by_name(mixer, "Capture Volume"), 100);

        mixerSetValueAll(::mixer_get_ctl_by_name(mixer, "Master Playback Switch"), 1);
        mixerSetValueAll(::mixer_get_ctl_by_name(mixer, "Capture Switch"), 1);

        gMixer = mixer;
    }

    ++refcounter;
    return gMixer;
}

struct mixer *mixerGetOrOpen(const unsigned card) {
    std::lock_guard<std::mutex> guard(gMixerMutex);

    switch (card) {
    case 0:  return mixerGetOrOpenImpl(card, gMixer0, gMixerRefcounter0);
    default: return FAILURE(nullptr);
    }
}

bool mixerUnrefImpl(struct mixer *mixer, struct mixer *&gMixer, int &refcounter) {
    if (mixer == gMixer) {
        if (0 == --refcounter) {
            ::mixer_close(mixer);
            gMixer = nullptr;
        }
        return true;
    } else {
        return false;
    }
}

bool mixerUnref(struct mixer *mixer) {
    std::lock_guard<std::mutex> guard(gMixerMutex);

    return mixerUnrefImpl(mixer, gMixer0, gMixerRefcounter0);
}

}  // namespace

void PcmDeleter::operator()(pcm_t *x) const {
    LOG_ALWAYS_FATAL_IF(::pcm_close(x) != 0);
};

std::unique_ptr<pcm_t, PcmDeleter> pcmOpen(const unsigned int dev,
                                           const unsigned int card,
                                           const unsigned int nChannels,
                                           const size_t sampleRateHz,
                                           const size_t frameCount,
                                           const bool isOut) {
    struct pcm_config pcm_config;
    memset(&pcm_config, 0, sizeof(pcm_config));

    pcm_config.channels = nChannels;
    pcm_config.rate = sampleRateHz;
    pcm_config.period_size = frameCount;     // Approx frames between interrupts
    pcm_config.period_count = 8;             // Approx interrupts per buffer
    pcm_config.format = PCM_FORMAT_S16_LE;
    pcm_config.start_threshold = 0;
    pcm_config.stop_threshold = isOut ? 0 : INT_MAX;

    PcmPtr pcm =
        PcmPtr(::pcm_open(dev, card,
                          (isOut ? PCM_OUT : PCM_IN) | PCM_MONOTONIC,
                           &pcm_config));
    if (::pcm_is_ready(pcm.get())) {
        return pcm;
    } else {
        ALOGE("%s:%d pcm_open failed for nChannels=%u sampleRateHz=%zu "
              "frameCount=%zu isOut=%d with %s", __func__, __LINE__,
              nChannels, sampleRateHz, frameCount, isOut,
              pcm_get_error(pcm.get()));
        return FAILURE(nullptr);
    }
}

Mixer::Mixer(unsigned card): mMixer(mixerGetOrOpen(card)) {}

Mixer::~Mixer() {
    if (mMixer) {
        LOG_ALWAYS_FATAL_IF(!mixerUnref(mMixer));
    }
}

}  // namespace talsa
}  // namespace implementation
}  // namespace V6_0
}  // namespace audio
}  // namespace hardware
}  // namespace android

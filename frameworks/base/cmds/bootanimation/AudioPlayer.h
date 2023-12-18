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

#ifndef _BOOTANIMATION_AUDIOPLAYER_H
#define _BOOTANIMATION_AUDIOPLAYER_H

#include <utils/Thread.h>
#include <utils/FileMap.h>
#include <utils/String8.h>

// get audio card
#define AUDIO_MAP_CNT        6
#define AUDIO_NAME_CODEC    "AUDIO_CODEC"
#define AUDIO_NAME_HDMI     "AUDIO_HDMI"
#define AUDIO_NAME_AHUB     "AUDIO_HUB"

namespace android {

class AudioPlayer : public Thread
{
public:
                AudioPlayer();
    virtual     ~AudioPlayer();
    bool        init(const char* config);

    void        playFile(FileMap* fileMap);
    void        playClip(const uint8_t* buf, int size);

private:
    virtual bool        threadLoop();
    void                init_audio_devices();
    int                 do_init_audio_card(int card);
    int                 find_name_map(char * in, char * out);
    void                getCardNumbyName(const char *name, int *card, bool *flag_exist);
#if 0
    void                check_event(char *path);
    void                jack_state_detection();
#else
    void                jack_switch_state_detection(char *path);
#endif

private:
    int                 mCard;      // ALSA card to use
    int                 mDevice;    // ALSA device to use
    bool                mHeadsetPlugged;
    String8             mSwitchStatePath;
    int                 mPeriodSize;
    int                 mPeriodCount;
    bool                mSingleVolumeControl; // speaker volume and headphones volume is not controlled respectively
    bool                mSpeakerHeadphonesSyncOut; //speaker and headphones is not out simutaneously
    uint8_t*            mWavData;
    int                 mWavLength;
};

} // namespace android

#endif // _BOOTANIMATION_AUDIOPLAYER_H

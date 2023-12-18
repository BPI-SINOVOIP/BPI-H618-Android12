# Copyright 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

sound_path := $(shell dirname $(lastword $(MAKEFILE_LIST)))

# Ring_Classic_02 : Bell Phone
# Ring_Synth_02 : Chimey Phone
# Ring_Digital_02 : Digital Phone
# Ring_Synth_04 : Flutey Phone
# Alarm_Beep_03 : Beep Beep Beep
PRODUCT_COPY_FILES += \
    $(sound_path)/notifications/ogg/Alya.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/notifications/Alya.ogg \
    $(sound_path)/notifications/ogg/Argon.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/notifications/Argon.ogg \
    $(sound_path)/notifications/Canopus.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/notifications/Canopus.ogg \
    $(sound_path)/notifications/Deneb.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/notifications/Deneb.ogg \
    $(sound_path)/newwavelabs/Highwire.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/notifications/Highwire.ogg \
    $(sound_path)/notifications/ogg/Iridium.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/notifications/Iridium.ogg \
    $(sound_path)/notifications/pixiedust.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/notifications/pixiedust.ogg \
    $(sound_path)/notifications/ogg/Talitha.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/notifications/Talitha.ogg \
    $(sound_path)/Ring_Classic_02.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ringtones/Ring_Classic_02.ogg \
    $(sound_path)/Ring_Synth_02.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ringtones/Ring_Synth_02.ogg \
    $(sound_path)/ringtones/ogg/Cygnus.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ringtones/Cygnus.ogg \
    $(sound_path)/Ring_Digital_02.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ringtones/Ring_Digital_02.ogg \
    $(sound_path)/Ring_Synth_04.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ringtones/Ring_Synth_04.ogg \
    $(sound_path)/ringtones/ogg/Kuma.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ringtones/Kuma.ogg \
    $(sound_path)/ringtones/ogg/Themos.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ringtones/Themos.ogg \
    $(sound_path)/alarms/ogg/Alarm_Classic.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/alarms/Alarm_Classic.ogg \
    $(sound_path)/alarms/ogg/Argon.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/alarms/Argon.ogg \
    $(sound_path)/alarms/ogg/Platinum.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/alarms/Platinum.ogg \
    $(sound_path)/Alarm_Beep_03.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/alarms/Alarm_Beep_03.ogg \
    $(sound_path)/alarms/ogg/Helium.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/alarms/Helium.ogg \
    $(sound_path)/alarms/ogg/Oxygen.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/alarms/Oxygen.ogg \
    $(sound_path)/effects/ogg/Effect_Tick.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/Effect_Tick.ogg \
    $(sound_path)/effects/ogg/KeypressStandard.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/KeypressStandard.ogg \
    $(sound_path)/effects/ogg/KeypressSpacebar.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/KeypressSpacebar.ogg \
    $(sound_path)/effects/ogg/KeypressDelete.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/KeypressDelete.ogg \
    $(sound_path)/effects/ogg/KeypressInvalid.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/KeypressInvalid.ogg \
    $(sound_path)/effects/ogg/KeypressReturn.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/KeypressReturn.ogg \
    $(sound_path)/effects/ogg/camera_click.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/camera_click.ogg \
    $(sound_path)/effects/ogg/Lock.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/Lock.ogg \
    $(sound_path)/effects/ogg/Unlock.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/Unlock.ogg \
    $(sound_path)/effects/ogg/VideoRecord.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/VideoRecord.ogg \
    $(sound_path)/effects/ogg/VideoStop.ogg:$(TARGET_COPY_OUT_PRODUCT)/media/audio/ui/VideoStop.ogg \


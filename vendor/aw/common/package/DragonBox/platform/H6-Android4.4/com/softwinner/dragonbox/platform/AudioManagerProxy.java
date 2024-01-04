package com.softwinner.dragonbox.platform;

import android.content.Context;
import android.media.AudioManager;
import java.util.ArrayList;

public class AudioManagerProxy{
    public static final String AUDIO_NAME_CODEC = AudioManager.AUDIO_NAME_CODEC;
    public static final String AUDIO_NAME_HDMI = AudioManager.AUDIO_NAME_HDMI;
    public static final String AUDIO_NAME_SPDIF = AudioManager.AUDIO_NAME_SPDIF;
    public static final int STREAM_MUSIC = AudioManager.STREAM_MUSIC;
    public static final String AUDIO_OUTPUT_ACTIVE = AudioManager.AUDIO_OUTPUT_ACTIVE;

    public static AudioManager getAudioManager(Context context) {
        return new AudioManager(context);
    }

    public static ArrayList<String> getActiveAudioDevices(Context context ,String devType){
        return new AudioManager(context).getActiveAudioDevices(devType);
    }

    public static void setAudioDeviceActive(Context context, ArrayList<String> dev, String state){
        new AudioManager(context).setAudioDeviceActive(dev,state);
    }
}


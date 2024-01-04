package com.softwinner.dragonbox.utils;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.media.AudioManager;
import android.widget.Toast;
import com.softwinner.dragonbox.platform.AudioManagerProxy;
public class AudioChannelUtil {
    private static ArrayList<String> lastAudioOutputChannels = null;
    public AudioChannelUtil() {

    }
    /**
     * set the output channel, when test output ,it always need set channel
     * @param context
     * @param channels may use AudioManager.AUDIO_NAME_***
     * @param setType  remeber to restore after set
     * @return the ordinary channels.the values before changed.
     */
    public static List<String> setOuputChannels(Context context, boolean setType,String... channels) {
        if(lastAudioOutputChannels == null){
            lastAudioOutputChannels = AudioManagerProxy.getActiveAudioDevices(context,AudioManagerProxy.AUDIO_OUTPUT_ACTIVE);
        }
        if(setType){
            ArrayList<String> newOutPutChannels = new ArrayList<String>();
            for (String channel : channels) {
                newOutPutChannels.add(channel);
            }
            AudioManagerProxy.setAudioDeviceActive(context,newOutPutChannels,AudioManagerProxy.AUDIO_OUTPUT_ACTIVE);
            return lastAudioOutputChannels;
        }else {
            AudioManagerProxy.setAudioDeviceActive(context,lastAudioOutputChannels,AudioManagerProxy.AUDIO_OUTPUT_ACTIVE);
            return lastAudioOutputChannels;
        }
    }
}

/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.server;

import android.app.ActivityManagerNative;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.UEventObserver;
import android.util.Slog;
import android.media.AudioManager;
import android.media.AudioManagerEx;

import android.util.Log;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.SystemClock;

import java.io.FileReader;
import java.io.FileNotFoundException;
import android.media.AudioSystem;
import java.io.File;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.lang.String;
import java.lang.Exception;
import android.content.Intent;
import android.hardware.display.DisplayManager;


/**
 * AudioDeviceManagerObserver monitors for audio devices on the main board.
 */
public class AudioDeviceManagerObserver extends UEventObserver {
    private static final String TAG = AudioDeviceManagerObserver.class.getSimpleName();
    private static final boolean LOG = true;

    private static final int MAX_AUDIO_DEVICES      = 16;
    public static final String AUDIO_TYPE = "audioType";
    public static final int AUDIO_INPUT_TYPE    = 0;
    public static final int AUDIO_OUTPUT_TYPE   = 1;

    public static final String AUDIO_STATE = "audioState";
    public static final int PLUG_IN             = 1;
    public static final int PLUG_OUT                = 0;

    public static final String AUDIO_NAME = "audioName";

    public static final String EXTRA_MNG = "extral_mng";

    private static final String uAudioDevicesPath   = "/sys/class/sound/";
    private static final String uEventSubsystem     = "SUBSYSTEM=sound";
    private static final String uEventDevPath       = "DEVPATH";
    private static final String uEventDevName       = "DEVNAME";
    private static final String uEventAction        = "ACTION";
    private static final String uPcmDev             = "snd/pcm";
    private static final String uAudioInType = "c";
    private static final String uAudioOutType = "p";

    private static final String h2wDevPath = String.format("/devices/virtual/switch/%s", "h2w");
    private static final String state = "state";
    public static final String H2W_DEV = "AUDIO_H2W";

    //private int mUsbAudioCnt = 0;

    private final Context mContext;
    private AudioManager mAudioManager = null;
    private AudioManagerEx mAudioManagerEx = null;

    /* use id,devName,androidName to identify a audio dev, the id is the key(one id to one dev)*/
    /*a device = {id,devNameForIn,devNameForOut,androidName}*/
    private String mAudioNameMap[][] = {
        {"audiocodec",  "unknown",  "unknown",  "AUDIO_CODEC"},
        {"snddaudio0",  "unknown",  "unknown",  "AUDIO_AC107"},
        {"snddaudio",   "unknown",  "unknown",  "AUDIO_CODEC"},
        {"sndac100",    "unknown",  "unknown",  "AUDIO_CODEC"},
        {"sndac200",    "unknown",  "unknown",  "AUDIO_CODEC"},
        {"sndhdmi",     "unknown",  "unknown",  "AUDIO_HDMI"},
        {"sndspdif",    "unknown",  "unknown",  "AUDIO_SPDIF"},
        {"unknown",     "unknown",  "unknown",  "unknown"},
        {"unknown",     "unknown",  "unknown",  "unknown"},
        {"unknown",     "unknown",  "unknown",  "unknown"},
        {"unknown",     "unknown",  "unknown",  "unknown"},
        {"unknown",     "unknown",  "unknown",  "unknown"},
        {"unknown",     "unknown",  "unknown",  "unknown"},
    };

    private AudioDeviceManagerObserver(Context context) {
        mContext = context;
        mAudioManager = (AudioManager)mContext.getSystemService(Context.AUDIO_SERVICE);
        mAudioManagerEx = new AudioManagerEx(context);

        Log.d(TAG, "AudioDeviceManagerObserver construct");

        context.registerReceiver(new BootCompletedReceiver(),
            new IntentFilter(Intent.ACTION_BOOT_COMPLETED), null, null);
    }

    private static AudioDeviceManagerObserver mAudioObserver = null;

    public synchronized static AudioDeviceManagerObserver getInstance(Context context){
        if(mAudioObserver == null){
            mAudioObserver = new AudioDeviceManagerObserver(context);
        }
        return mAudioObserver;
    }

    private final class BootCompletedReceiver extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context context, Intent intent) {
            // At any given time accessories could be inserted
            // one on the board, one on the dock and one on HDMI:
            // observe three UEVENTs
            init();  // set initial status

            //monitor usb
            startObserving(uEventSubsystem);

            //monitor h2w
            startObserving("DEVPATH=" + h2wDevPath);

            //minitor hdmi,and codec
            startObservingTv(context);

            //create a handler to switch the audio devices
            new AudioManagerPolicy(context);
        }
    }

    private synchronized final void init() {
        char[] buffer = new char[1024];

        String name_linux = String.format("%s", "unknown");
        String name_android = String.format("%s", "unknown");

        Log.v(TAG, "AudioDeviceManagerObserver init()");

        for (int card = 0; card < MAX_AUDIO_DEVICES; card++) {
            try {
                String newCard = String.format("/sys/class/sound/card%d/",card);
                String newCardId = newCard + "id";
                Log.d(TAG, "AudioDeviceManagerObserver: newCardId: " + newCardId);

                FileReader file = new FileReader(newCardId);
                int len = file.read(buffer, 0, 1024);
                file.close();

                name_linux = new String(buffer, 0, len).trim();


                if (len > 0)
                {
                    File cardDir = new File(newCard);
                    String[] list = cardDir.list();
                    for(String name:list){
                        if(name.startsWith("pcm")){
                            int length = name.length();
                            String ext = name.substring(length - 1, length);
                            Log.d(TAG,"AudioDeviceManagerObserver: devName: " + name);
                            if(ext.equalsIgnoreCase(uAudioInType)){
                                name_android = findNameMap(name_linux, "snd/" + name, true);
                            }
                            else if(ext.equalsIgnoreCase(uAudioOutType)){
                                name_android = findNameMap(name_linux, "snd/" + name, false);
                            }
                        }
                    }
                    Log.d(TAG, "AudioDeviceManagerObserver: name_linux: " + name_linux + ", name_android: " + name_android);
                }

            } catch (FileNotFoundException e) {
                Log.v(TAG, "This kernel does not have sound card" + card);
                break;
            } catch (Exception e) {
                Log.e(TAG, "" , e);
            }
        }
    }

    /** when the id is not null,it will find mapped androidName by id,
     *if the devName is not null,its device name will be set to devName,else not change the device name.*/
    private String findNameMap(String id,String devName,boolean audioIn) {

        Log.d(TAG, "~~~~~~~~ AudioDeviceManagerObserver: findNameMap: id: " + id);
        String out = null;
        if(id != null){
            if(devName == null){
                devName = "unknown";
            }
            for (int index = 0; index < MAX_AUDIO_DEVICES; index++) {
                if (mAudioNameMap[index][0].equals("unknown")) {
                    mAudioNameMap[index][0] = id;
                    if(audioIn){
                        mAudioNameMap[index][1] = devName;
                    }else{
                        mAudioNameMap[index][2] = devName;
                    }
                    //mAudioNameMap[index][3] = String.format("AUDIO_USB%d", mUsbAudioCnt++);
                    mAudioNameMap[index][3] = String.format("AUDIO_USB_%s", id);

                    out = mAudioNameMap[index][3];

                    // Log.d(TAG, "xxxx index: " + index + " in: " + mAudioNameMap[index][0] + " out:" + mAudioNameMap[index][1]);
                    break;
                }

                if (mAudioNameMap[index][0].equals(id)) {
                    if(audioIn){
                        mAudioNameMap[index][1] = devName;
                    }else{
                        mAudioNameMap[index][2] = devName;
                    }

                    out = mAudioNameMap[index][3];
                    // Log.d(TAG, "qqqq index: " + index + " in: " + mAudioNameMap[index][0] + " out:" + mAudioNameMap[index][1]);
                    break;
                }
            }
        }

        return out;
    }

    /** find mapped androidName by devName,if the reset is true,its devName will reset to 'unknown' when find it */
    private String findNameMap(String devName,boolean audioIn,boolean reset){
        Log.d(TAG,"~~~~~~~~ AudioDeviceManagerObserver: findNameMap: devName: " + devName);
        String out = null;
        if(devName != null && !devName.equals("unknown")){
            for(int index = 0; index < MAX_AUDIO_DEVICES; index++) {
                if(audioIn && mAudioNameMap[index][1].equals(devName)){
                    out = mAudioNameMap[index][3];
                    if(reset){
                        mAudioNameMap[index][1] = "unknown";
                    }
                    break;
                }else if(!audioIn && mAudioNameMap[index][2].equals(devName)){
                    out = mAudioNameMap[index][3];
                    if(reset){
                        mAudioNameMap[index][2] = "unknown";
                    }
                    break;
                }
            }
        }
        return out;
    }


    private void startObservingTv(Context context){
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_HDMI_PLUGGED);
        BroadcastReceiver receiver = new BroadcastReceiver(){
            public void onReceive(Context context, Intent intent){
                final String action = intent.getAction();
                if (action.equals(Intent.ACTION_HDMI_PLUGGED))
                {
                    boolean hdmiplug;
                    hdmiplug = intent.getBooleanExtra("state", false);
                    if(hdmiplug){
                        updateState(AudioManagerEx.AUDIO_NAME_HDMI, AUDIO_OUTPUT_TYPE, PLUG_IN);
                    }
                    else
                    {
                        updateState(AudioManagerEx.AUDIO_NAME_HDMI, AUDIO_OUTPUT_TYPE, PLUG_OUT);
                    }
                }
            }
        };
        context.registerReceiver(receiver, filter);
    }


    @Override
    public void onUEvent(UEventObserver.UEvent event) {
        Log.d(TAG, "Audio device change: " + event.toString());

        try {
            String devPath = event.get(uEventDevPath);
            String devName = event.get(uEventDevName);
            String action = event.get(uEventAction);
            String devCardPath;
            String sndName;
            String sndNameMap = "unknown";
            String audioType = null;

            //deal with headphone
            if((devPath != null) && (devPath.equals(h2wDevPath))){
                String switchName = event.get("SWITCH_NAME");
                int state = Integer.parseInt(event.get("SWITCH_STATE"));
                Log.d(TAG, "device " + switchName + ((state != 0) ? " connected" : " disconnected"));
                updateState(AudioManagerEx.AUDIO_NAME_CODEC, AUDIO_OUTPUT_TYPE, (state != 0) ? PLUG_IN : PLUG_OUT, H2W_DEV);
            }

            //deal with usb audio
            if ((devName != null) && (devName.substring(0, 7).equals(uPcmDev.substring(0, 7)))) {
                Log.d(TAG, "action: " + action + " devName: " + devName + " devPath: " + devPath);
                char[] buffer = new char[64];
                int index = devPath.lastIndexOf("/");
                devCardPath = devPath.substring(0, index);
                devCardPath = devCardPath + "/id";
                int length = devName.length();
                audioType = devName.substring(length - 1, length);
                if(action.equals("add")){
                    int cnt = 10;
                    while((cnt-- != 0)) {
                        try {
                            FileReader file = new FileReader("sys" + devCardPath);
                            int len = file.read(buffer, 0, 64);
                            file.close();
                            if(len == 1)
                            {
                                Thread.sleep(5);
                                continue;
                            }

                            if (len > 0)
                            {
                                sndName = new String(buffer, 0, len).trim();
                                if(audioType.equalsIgnoreCase(uAudioInType)){
                                    sndNameMap = findNameMap(sndName, devName, true);
                                    updateState(sndNameMap, AUDIO_INPUT_TYPE, PLUG_IN);
                                }else if(audioType.equalsIgnoreCase(uAudioOutType)){
                                    sndNameMap = findNameMap(sndName,devName, false);
                                    updateState(sndNameMap, AUDIO_OUTPUT_TYPE, PLUG_IN);
                                }


                                break;
                            }

                        } catch (FileNotFoundException e) {
                            if (cnt == 0) {
                                Slog.e(TAG, "can not read card id");
                                return ;
                            }
                            try {
                                Slog.w(TAG, "read card id, wait for a moment ......");
                                Thread.sleep(10);
                            } catch (Exception e0) {
                                Slog.e(TAG, "" , e0);
                            }
                        } catch (Exception e) {
                            Slog.e(TAG, "" , e);
                        }
                    }
                }
                else if(action.equals("remove")){
                    try{
                        if(audioType.equalsIgnoreCase(uAudioInType)){
                            SystemClock.sleep(5);
                            sndNameMap = findNameMap(devName, true, true);
                            mAudioManagerEx.getAudioDevices(AudioManagerEx.AUDIO_INPUT_TYPE); //update the audio hal
                            updateState(sndNameMap, AUDIO_INPUT_TYPE, PLUG_OUT);
                        }else if(audioType.equalsIgnoreCase(uAudioOutType)){
                            SystemClock.sleep(5);
                            sndNameMap = findNameMap(devName, false, true);
                            mAudioManagerEx.getAudioDevices(AudioManagerEx.AUDIO_OUTPUT_TYPE); //update the audio hal
                            updateState(sndNameMap, AUDIO_OUTPUT_TYPE, PLUG_OUT);
                        }
                    }catch(Exception e){
                        Slog.e(TAG,"",e);
                    }
                }

            }
        } catch (NumberFormatException e) {
            Slog.e(TAG, "Could not parse switch state from event " + event);
        }
    }

    public synchronized final void updateState(String name, int type,int state){
        updateState(name,type,state,null);
    }

    public synchronized final void updateState(String name, int type,int state, String extraMng) {
        Log.d(TAG, "name: " + name + ", state: " + state + ", type: " + type);
        Intent intent;

        intent = new Intent(Intent.ACTION_AUDIO_PLUG_IN_OUT);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY);
        Bundle bundle = new Bundle();
        bundle.putInt(AUDIO_STATE,state);
        bundle.putString(AUDIO_NAME,name);
        bundle.putInt(AUDIO_TYPE,type);
        if(extraMng != null){
            bundle.putString(EXTRA_MNG, extraMng);
        }
        intent.putExtras(bundle);
        ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);
    }

    public int getHeadphoneAvailableState(){
        String h2wState = "/sys" + h2wDevPath + "/" + state;
        if(h2wState == null){
            return AudioSystem.DEVICE_STATE_UNAVAILABLE;
        }
        try{
            int buffer = 32;
            File file = new File(h2wState);
            byte[] data = new byte[buffer];
            String d = null;
            BufferedInputStream i_stream = new BufferedInputStream(new FileInputStream(file));
            int i = 0;
            if((i = i_stream.read(data, 0 ,buffer)) != -1){
                d = new String(data, 0 , 1);
            }
			i_stream.close();
            int state;
            state = Integer.valueOf(d).intValue();
            Log.d(TAG,"headphone state is " + d);
            if(state == 0){
                return AudioSystem.DEVICE_STATE_UNAVAILABLE;
            }else if(state == 2){
                return AudioSystem.DEVICE_STATE_AVAILABLE;
            }else{
                return AudioSystem.DEVICE_STATE_UNAVAILABLE;
            }
        }catch(Exception e) {
            Slog.e(TAG, "Could not parse switch state for " + h2wState + " ,fail because " + e.getMessage());
            return AudioSystem.DEVICE_STATE_UNAVAILABLE;
        }
    }
}

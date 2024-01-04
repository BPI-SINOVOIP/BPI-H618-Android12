package com.android.server;

import android.content.Context;
import java.lang.Exception;
import java.lang.Thread;
import android.util.Log;
import java.util.ArrayList;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.net.Uri;
import android.os.Bundle;
import android.hardware.display.DisplayManager;

import android.widget.Toast;
import com.android.internal.R;
import android.app.NotificationManager;
import android.app.Notification;
import android.os.Handler;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.provider.Settings;
import android.os.Message;
import android.media.AudioManager;
import android.media.AudioManagerEx;

import android.media.AudioSystem;
import java.io.FileReader;
import java.io.IOException;
import android.view.Display;
import java.io.FileNotFoundException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import android.app.Dialog;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.WindowManager;
import android.os.AsyncTask;

import android.os.SystemProperties;
import android.media.AudioPort;
import android.media.AudioDevicePort;
import android.media.AudioFormat;
import android.app.ActivityManagerNative;
import android.os.UserHandle;
import android.app.NotificationChannel;

import vendor.display.DisplayOutputManager;
/**
*@hide
*/
public class AudioManagerPolicy extends BroadcastReceiver {

    private Context mCtx;
    private static final String TAG = "AudioManagerPolicy";
    private AudioManager mAudioManager = null;
    private AudioManagerEx mAudioManagerEx = null;
    private DisplayManager mDisplayManager = null;
    private DisplayOutputManager mDisplayOutputManager = null;
    private static final int POLICY_0 = 0x00;//A80 use it
    private static final int POLICY_1 = 0x01;//Aliyun use it
    private static final int POLICY_2 = 0x02;//H8 usr it
    private static final boolean enable_close_hdmiplug = false;
    private static final int USB_TOAST_TIME = 10;
    private boolean mBooting = false;
    private ExecutorService mThreadExecutor = Executors.newSingleThreadExecutor();
    private AlertDialog alertDialog = null;
    //NotificationChannel
    private NotificationChannel mChannel = null;

    public AudioManagerPolicy(Context context) {
        mCtx = context;
        mAudioManager = (AudioManager)mCtx.getSystemService(Context.AUDIO_SERVICE);
        mAudioManagerEx = new AudioManagerEx(mCtx);
        mDisplayManager = (DisplayManager)mCtx.getSystemService(Context.DISPLAY_SERVICE);
        mDisplayOutputManager = DisplayOutputManager.getInstance();
        initReceriver();
        initAudioOut();
        initAudioIn();

        mCtx.getContentResolver().registerContentObserver(
                Settings.System.getUriFor(Settings.System.ENABLE_PASS_THROUGH),
                true, mContentObserver);
    }

    private void initAudioOut(){
        ArrayList<String> actived = new ArrayList<String>();
        ArrayList<String> mAudioOutputChannels = mAudioManagerEx.getAudioDevices(AudioManagerEx.AUDIO_OUTPUT_TYPE);
        boolean  usbAudioOutputDeviceExist = false;
        int policy = getPolicy();
        int i = 0;
        int format = mDisplayOutputManager.getDisplayOutputType(0);
        mBooting = true;

        //if the headphone is available,use it first
        if(AudioDeviceManagerObserver.getInstance(mCtx).getHeadphoneAvailableState() == AudioSystem.DEVICE_STATE_AVAILABLE){
            Log.d(TAG, "headphone is available");
            actived.add(AudioManagerEx.AUDIO_NAME_CODEC);
            mAudioManagerEx.setAudioDeviceActive(actived,AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
        }

        //otherwise check whether the usb audio output device is available?
        else{

            for(i = 0; i < mAudioOutputChannels.size(); i++){
                String device = mAudioOutputChannels.get(i);
                if(device.contains("USB")){
                    actived.add(device);
                    usbAudioOutputDeviceExist = true;
                    mAudioManagerEx.setAudioDeviceActive(actived, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
                    break;
                }
            }
        }
        if(!usbAudioOutputDeviceExist){
            Log.d(TAG, "no usb audio i=" + i + " size=" + mAudioOutputChannels.size());
            //check whether the hdmi is available
            int curType = mDisplayOutputManager.getDisplayOutputType(0);
            Log.d(TAG, "curType" + curType + " isHdmiPlugin =" + isHdmiPlugin());
            if(curType == DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI
                && isHdmiPlugin()){
                Log.d(TAG, "hdmi is available on init.");
                notifyHdmiAvailable();
            }

            //otherwise, switch the audio device based on current disp mode and the value save in database.
            if(i == mAudioOutputChannels.size()){
                switchAudioDevice(format,false,true);
            }
        }
        if(getPolicy() != POLICY_2){
            //start observe display mode change
            observeDispChange(format);
        }
    }

    private void initAudioIn(){
        //if the usb audio input device is avaiable,switch to it
        ArrayList<String> actived = new ArrayList<String>();
        ArrayList<String> mAudioInputChannels = mAudioManagerEx.getAudioDevices(AudioManagerEx.AUDIO_INPUT_TYPE);
        int i = 0;
        for(i = 0; i < mAudioInputChannels.size(); i++){
            String device = mAudioInputChannels.get(i);
            if(device.contains("USB")){
                actived.add(device);
                break;
            }
        }
        //otherwise use codec by default
        if(i == mAudioInputChannels.size()){
            actived.add(AudioManagerEx.AUDIO_NAME_CODEC);
        }
        mAudioManagerEx.setAudioDeviceActive(actived, AudioManagerEx.AUDIO_INPUT_ACTIVE);
    }

    private void initReceriver(){
        //register receiver for audio device plug event
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_AUDIO_PLUG_IN_OUT);
        mCtx.registerReceiver(this, filter);
   	}

    private boolean isUSBAudioDeviceExist(String audioOutputChannelName){

        if(audioOutputChannelName == null){
            return false;
        }
        ArrayList<String> activedChannels = new ArrayList<String>();
        boolean  usbAudioDeviceExist = false;
        if(audioOutputChannelName.contains("USB")){
            String audioUsb = String.format("%s", "AUDIO_USB_");
            String usbCardId = audioOutputChannelName.substring(audioUsb.length());
            char[] buffer = new char[1024];
            String cardName = null;
            for (int card = 0; card < 8; card++) {
                try {
                    String newCard = String.format("/sys/class/sound/card%d/",card);
                    String newCardId = newCard + "id";

                    FileReader file = new FileReader(newCardId);
                    int len = file.read(buffer, 0, 1024);
                    file.close();

                    cardName = new String(buffer, 0, len).trim();
                    if (usbCardId.contains(cardName))
                    {
                        usbAudioDeviceExist = true;
                        Log.d(TAG,"USB Audio Device  " + cardName + " exist");
                    }
                } catch (FileNotFoundException e) {
                    break;
                } catch (Exception e) {
                }
            }
            if(usbAudioDeviceExist){
                return true;
            }else{
                return false;
            }
        }else{
            return false;
        }
    }

    private void switchAudioDevice(int dispFormat , boolean remove_usb, boolean boot){

        AudioManagerEx mAudioManagerEx =
            new AudioManagerEx(mCtx);
        ArrayList<String> actived = new ArrayList<String>();
        String audioOutputChannelName = Settings.System.getString(mCtx.getContentResolver(),
                Settings.System.AUDIO_OUTPUT_CHANNEL);
        Log.d(TAG, "save audio is " + audioOutputChannelName);
        ArrayList<String> audioOutputChannels = new ArrayList<String>();
        if (audioOutputChannelName != null) {
            String[] audioList = audioOutputChannelName.split(",");
            for (String audio : audioList) {
                if (!"".equals(audio)) {
                    audioOutputChannels.add(audio);
                }
            }
        }
        switch(getPolicy()){
        case POLICY_0:

            if(!isUSBAudioDeviceExist(audioOutputChannelName) || (audioOutputChannelName == null) || remove_usb){
                Log.d(TAG," POLICY_0 Remove USB Audio or No USB Audio Output Device Connected!");
                ArrayList<String> activedChannels = new ArrayList<String>();
                if(audioOutputChannels.contains(AudioManagerEx.AUDIO_NAME_SPDIF)){
                    activedChannels.add(AudioManagerEx.AUDIO_NAME_SPDIF);
                }
                if(audioOutputChannels.contains(AudioManagerEx.AUDIO_NAME_CODEC)){
                    activedChannels.add(AudioManagerEx.AUDIO_NAME_CODEC);
                }
                if(dispFormat == DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI){
                    activedChannels.add(AudioManagerEx.AUDIO_NAME_HDMI);
                }else if(!audioOutputChannels.contains(AudioManagerEx.AUDIO_NAME_CODEC)){
                    activedChannels.add(AudioManagerEx.AUDIO_NAME_CODEC);
                }
                mAudioManagerEx.setAudioDeviceActive(activedChannels,AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
            }

            break;
        case POLICY_1:

            if(!isUSBAudioDeviceExist(audioOutputChannelName) || (audioOutputChannelName == null) || remove_usb){
                Log.d(TAG," POLICY_1 Remove USB Audio or No USB Audio Output Device Connected!");
                ArrayList<String> activedChannels = new ArrayList<String>();
                activedChannels.add(AudioManagerEx.AUDIO_NAME_SPDIF);
                activedChannels.add(AudioManagerEx.AUDIO_NAME_CODEC);
                activedChannels.add(AudioManagerEx.AUDIO_NAME_HDMI);
                mAudioManagerEx.setAudioDeviceActive(activedChannels,AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
                Log.i(TAG," POLICY_1 activedChannels = " + activedChannels);
            }
            /*
            if (!audioOutputChannels.contains(name))
                audioOutputChannels.add(name);
            Log.d(TAG, "audio manual is " + audioOutputChannels);
            mAudioManager.setAudioDeviceActive(audioOutputChannels, AudioManager.AUDIO_OUTPUT_ACTIVE);
            */
            break;
        case POLICY_2:

            if(!isUSBAudioDeviceExist(audioOutputChannelName) || (audioOutputChannelName == null) || remove_usb){
                Log.d(TAG," POLICY_2 Remove USB Audio or No USB Audio Output Device Connected!");
                ArrayList<String> activedChannels = new ArrayList<String>();
                boolean hdmiPlugIn = isHdmiPlugin();
                if(audioOutputChannels.contains(AudioManagerEx.AUDIO_NAME_SPDIF)){
                    activedChannels.add(AudioManagerEx.AUDIO_NAME_SPDIF);
                }
                if(audioOutputChannels.contains(AudioManagerEx.AUDIO_NAME_CODEC)){
                    activedChannels.add(AudioManagerEx.AUDIO_NAME_CODEC);
                }
                if(enable_close_hdmiplug){
                    if(audioOutputChannels.contains(AudioManagerEx.AUDIO_NAME_HDMI)){
                        activedChannels.add(AudioManagerEx.AUDIO_NAME_HDMI);
                    }
                }
                if(!enable_close_hdmiplug){
                    boolean enablePassThrough = Settings.System.getInt(mCtx.getContentResolver(),
                    Settings.System.ENABLE_PASS_THROUGH, 0) == 1;
                    if(dispFormat == DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI && hdmiPlugIn){
                        activedChannels.add(AudioManagerEx.AUDIO_NAME_HDMI);
                    }else if(!audioOutputChannels.contains(AudioManagerEx.AUDIO_NAME_CODEC)){
                        activedChannels.add(AudioManagerEx.AUDIO_NAME_CODEC);
                    }
                    if((!enablePassThrough && !activedChannels.contains(AudioManagerEx.AUDIO_NAME_CODEC) && !boot && !remove_usb) ||
                        ((dispFormat == DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI) && !hdmiPlugIn)){
                        Log.d(TAG," POLICY_2 add codec output!");
                        activedChannels.add(AudioManagerEx.AUDIO_NAME_CODEC);
                        if(enablePassThrough && activedChannels.contains(AudioManagerEx.AUDIO_NAME_HDMI)){
                            Log.d(TAG," POLICY_2 rm hdmi output!");
                            activedChannels.remove(AudioManagerEx.AUDIO_NAME_HDMI);
                        }
                    }
                }
                mAudioManagerEx.setAudioDeviceActive(activedChannels,AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
            }

            break;
        }
    }

    private void observeDispChange(final int defType){
        Thread thread = new Thread(new Runnable(){
            private int curType = defType;
            private int preType = defType;
            @Override
            public void run(){
                while(true){
                    DisplayOutputManager dmg = DisplayOutputManager.getInstance();
                    try{
                        curType = dmg.getDisplayOutputType(0);
                    }catch(Exception e){
                        curType = DisplayOutputManager.DISPLAY_OUTPUT_TYPE_NONE;
                    }
                    if(preType != curType){
                        if(curType == DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI && 
                            isHdmiPlugin()){
                            notifyHdmiAvailable();
                        }
                        notifyDispChange(curType);
                        preType = curType;
                    }
                    try
                    {
                        Thread.currentThread().sleep(200);
                    }
                    catch(Exception e) {};
                }
            }
        });
        thread.start();
    }

    private void notifyDispChange(int curFormat){
        switchAudioDevice(curFormat,false,false);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        Bundle bundle = intent.getExtras();
        Intent intent_hdmi_audio = new Intent();
        if(bundle == null){
            Log.d(TAG, "bundle is null");
            return;
        }
        final int state = bundle.getInt(AudioDeviceManagerObserver.AUDIO_STATE);
        final String name = bundle.getString(AudioDeviceManagerObserver.AUDIO_NAME);
        final int type = bundle.getInt(AudioDeviceManagerObserver.AUDIO_TYPE);
        final String extra = bundle.getString(AudioDeviceManagerObserver.EXTRA_MNG);
        Log.d(TAG, "On Audio device plug in/out receive,name=" + name + " type=" + type + " state=" + state + " extra=" + extra);

        if(name == null){
            Log.d(TAG, "audio name is null");
            return;
        }
        intent_hdmi_audio.setAction(AudioManager.ACTION_HDMI_AUDIO_PLUG);
        if(name.equals(AudioManagerEx.AUDIO_NAME_HDMI)){
            intent_hdmi_audio.putExtra(AudioManager.EXTRA_AUDIO_PLUG_STATE, 1);
            if(state == AudioDeviceManagerObserver.PLUG_IN) {
                notifyHdmiAvailable();
                {
                    Log.v(TAG, "chengkan-------------   hdmi active");
                    //AudioSystem.setDeviceConnectionState(AudioSystem.DEVICE_OUT_AUX_DIGITAL, AudioSystem.DEVICE_STATE_AVAILABLE, "", "");
                    {
                        ArrayList<AudioPort> ports = new ArrayList<AudioPort>();
                        int[] portGeneration = new int[1];
                        int status = AudioSystem.listAudioPorts(ports, portGeneration);
                        Log.d(TAG,"i come");
                        if (status == AudioManager.SUCCESS) {
                            Log.d(TAG,"list audio port success");
                            for (AudioPort port : ports) {
                                if (port instanceof AudioDevicePort) {
                                    final AudioDevicePort devicePort = (AudioDevicePort) port;
                                    if (devicePort.type() == AudioManager.DEVICE_OUT_HDMI ||
                                            devicePort.type() == AudioManager.DEVICE_OUT_HDMI_ARC) {
                                        int[] formats = devicePort.formats();
                                        if (formats.length > 0) {
                                            ArrayList<Integer> encodingList = new ArrayList(1);
                                            for (int format : formats) {
                                                Log.d(TAG,"load format : "+format);
                                                if (format != AudioFormat.ENCODING_INVALID) {
                                                    encodingList.add(format);
                                                }
                                            }
                                            int[] encodingArray = new int[encodingList.size()];
                                            for (int i = 0 ; i < encodingArray.length ; i++) {
                                                encodingArray[i] = encodingList.get(i);
                                            }
                                            intent_hdmi_audio.putExtra(AudioManager.EXTRA_ENCODINGS, encodingArray);
                                        }
                                        int maxChannels = 0;
                                        for (int mask : devicePort.channelMasks()) {
                                            int channelCount = AudioFormat.channelCountFromOutChannelMask(mask);
                                            if (channelCount > maxChannels) {
                                                maxChannels = channelCount;
                                            }
                                        }
                                        intent_hdmi_audio.putExtra(AudioManager.EXTRA_MAX_CHANNEL_COUNT, maxChannels);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                intent_hdmi_audio.putExtra(AudioManager.EXTRA_AUDIO_PLUG_STATE, 1);
                {
                    Log.v(TAG, "chengkan-------------   hdmi inactive");
                    //AudioSystem.setDeviceConnectionState(AudioSystem.DEVICE_OUT_AUX_DIGITAL, AudioSystem.DEVICE_STATE_UNAVAILABLE, "", "");
                    {
                        ArrayList<AudioPort> ports = new ArrayList<AudioPort>();
                        int[] portGeneration = new int[1];
                        int status = AudioSystem.listAudioPorts(ports, portGeneration);
                        if (status == AudioManager.SUCCESS) {
                            for (AudioPort port : ports) {
                                if (port instanceof AudioDevicePort) {
                                    final AudioDevicePort devicePort = (AudioDevicePort) port;
                                    if (devicePort.type() == AudioManager.DEVICE_OUT_HDMI ||
                                            devicePort.type() == AudioManager.DEVICE_OUT_HDMI_ARC) {
                                        int[] formats = devicePort.formats();
                                        if (formats.length > 0) {
                                            ArrayList<Integer> encodingList = new ArrayList(1);
                                            for (int format : formats) {
                                                if (format != AudioFormat.ENCODING_INVALID) {
                                                    encodingList.add(format);
                                                }
                                            }
                                            int[] encodingArray = new int[encodingList.size()];
                                            for (int i = 0 ; i < encodingArray.length ; i++) {
                                                encodingArray[i] = encodingList.get(i);
                                            }
                                            intent_hdmi_audio.putExtra(AudioManager.EXTRA_ENCODINGS, encodingArray);
                                        }
                                        int maxChannels = 0;
                                        for (int mask : devicePort.channelMasks()) {
                                            int channelCount = AudioFormat.channelCountFromOutChannelMask(mask);
                                            if (channelCount > maxChannels) {
                                                maxChannels = channelCount;
                                            }
                                        }
                                        intent_hdmi_audio.putExtra(AudioManager.EXTRA_MAX_CHANNEL_COUNT, maxChannels);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (getPolicy() == POLICY_2){
                if(mBooting){
                    Log.d(TAG,"Policy2 HDMI Plug Out/In Switch! Booting");
                    switchAudioDevice(mDisplayOutputManager.getDisplayOutputType(0),false,true);
                }else{
                    Log.d(TAG,"Policy2 HDMI Plug Out/In Switch!");
                    switchAudioDevice(mDisplayOutputManager.getDisplayOutputType(0),false,false);
                }
            }
        }
        ActivityManagerNative.broadcastStickyIntent(intent_hdmi_audio, null, UserHandle.USER_ALL);
        if(name.equals(AudioManagerEx.AUDIO_NAME_HDMI)
            || (name.equals(AudioManagerEx.AUDIO_NAME_CODEC) && ((extra == null) || (extra != null) && !extra.equals(AudioDeviceManagerObserver.H2W_DEV)))
            || (name.equals(AudioManagerEx.AUDIO_NAME_SPDIF))){

        }else{
            handleExternalDevice(name, type, state, extra);
        }
    }

    private static final int AUDIO_OUT_NOTIFY = 20130815;
    private static final int AUDIO_IN_NOTIFY = 20130816;
    private boolean headPhoneConnected = false;
    private void toastMessage(String message){
        Toast.makeText(mCtx, message, Toast.LENGTH_LONG).show();
    }
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg){
            String message = (String)msg.obj;
            toastMessage(message);
        }
    };

    /* 监听pass through的设置变化 */
    private ContentObserver mContentObserver = new ContentObserver(mHandler){

        @Override
            public void onChange(boolean selfChange, Uri uri) {
                ArrayList<String> list = mAudioManagerEx
                    .getActiveAudioDevices(AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
                mAudioManagerEx.setAudioDeviceActive(list, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
            }

    };

    private void handleToastMessage(String message){
        if(mHandler == null) return;
        Message mng = mHandler.obtainMessage();
        mng.obj = message;
        mHandler.sendMessage(mng);
    }
    private void toastPlugOutNotification(String title,String mng, int id){

        NotificationManager notificationManager = (NotificationManager) mCtx
            .getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(id);
        handleToastMessage(mng);
    }

    private void toastPlugInNotification(String title, int id){

        NotificationManager notificationManager = (NotificationManager) mCtx
            .getSystemService(Context.NOTIFICATION_SERVICE);
        if(null == mChannel){
            mChannel = new NotificationChannel("Audio_PlugIn", "PlugInToast", NotificationManager.IMPORTANCE_DEFAULT);
            notificationManager.createNotificationChannel(mChannel);
        }
        Notification notification = new Notification.Builder(mCtx).setWhen(System.currentTimeMillis())
            .setSmallIcon(com.android.internal.R.drawable.stat_sys_data_usb)
            .setTicker(title)
            .setContentTitle(title)
            .setContentText(title)
            .setChannel("Audio_PlugIn").build();

        notification.defaults &= ~Notification.DEFAULT_SOUND;
        notification.flags = Notification.FLAG_AUTO_CANCEL;

        notificationManager.notify(id, notification);

        handleToastMessage(title);
    }

    private void chooseUsbAudioDeviceDialog(final ArrayList<String> usb_device)
    {
        String str = mCtx.getResources().getString(com.android.internal.R.string.usb_output_message);
        alertDialog = new AlertDialog.Builder(mCtx).
                setTitle(com.android.internal.R.string.usb_output_title).
                setMessage(String.format(str, Integer.toString(USB_TOAST_TIME))).
                setPositiveButton(com.android.internal.R.string.usb_yes, new DialogInterface.OnClickListener() {
                     @Override
                     public void onClick(DialogInterface dialog, int which) {
                           // TODO Auto-generated method stub
                           String message = mCtx.getResources().getString(R.string.usb_audio_plug_in_message);
                           toastPlugInNotification(message, AUDIO_OUT_NOTIFY);
                           mAudioManagerEx.setAudioDeviceActive(usb_device, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
                     }
                }).
                setNegativeButton(com.android.internal.R.string.usb_no, new DialogInterface.OnClickListener() {
                     @Override
                     public void onClick(DialogInterface dialog, int which) {
                           // TODO Auto-generated method stub
                           //Log.v(TAG, "setNegativeButton ");
                           String message = mCtx.getResources().getString(R.string.usb_audio_plug_out_message);
                           toastPlugInNotification(message, AUDIO_OUT_NOTIFY);
                     }
                }).
                create();
        alertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG);
        alertDialog.show();
        new AsyncTask() {
            @Override
            protected Object doInBackground(Object... arg0) {
                int time = USB_TOAST_TIME;
                while(time >= 0 && alertDialog != null && alertDialog.isShowing()){
                    publishProgress(time);
                    try {
                        Thread.sleep(1000);
                    } catch(Exception e) {
                        // do nothing
                    }
                    time--;
                }
                return null;
            }
            @Override
            protected void onPostExecute(Object result) {
                super.onPostExecute(result);
                if (alertDialog != null && alertDialog.isShowing()) {
                    alertDialog.dismiss();
                    mAudioManagerEx.setAudioDeviceActive(usb_device, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
                    alertDialog = null;
                }
            }
            @Override
            protected void onProgressUpdate(Object... values) {
                super.onProgressUpdate(values);
                int time = (Integer)values[0];
                String str = mCtx.getResources().getString(com.android.internal.R.string.usb_output_message);
                if(alertDialog != null)
                    alertDialog.setMessage(String.format(str, Integer.toString(time)));
            }
       }.execute();
    }

    private void handleExternalDevice(final String name, final int type, final int state, final String extra){
        //handle device:headphone(if has), or usb, they have higner privileges then internal device(hdmi,codec,spdif)

        mThreadExecutor.execute(new Runnable() {

            @Override
            public void run() {
                ArrayList<String> mAudioOutputChannels = mAudioManagerEx.getAudioDevices(AudioManagerEx.AUDIO_OUTPUT_TYPE);
                ArrayList<String> mAudioInputChannels = mAudioManagerEx.getAudioDevices(AudioManagerEx.AUDIO_INPUT_TYPE);

                String title = null;
                String message = null;

                switch(state){
                case AudioDeviceManagerObserver.PLUG_IN:
                    switch(type){
                    case AudioDeviceManagerObserver.AUDIO_INPUT_TYPE:
                        //auto change to this audio-in channel
                        Log.d(TAG, "audio input plug in");
                        ArrayList<String> audio_in = new ArrayList<String>();
                        audio_in.add(name);
                        mAudioManagerEx.setAudioDeviceActive(audio_in, AudioManagerEx.AUDIO_INPUT_ACTIVE);

                        title = mCtx.getResources().getString(R.string.usb_audio_in_plug_in_title);
                        //message = mCtx.getResources().getString(R.string.usb_audio_plug_in_message);
                        toastPlugInNotification(title, AUDIO_IN_NOTIFY);
                        break;
                    case AudioDeviceManagerObserver.AUDIO_OUTPUT_TYPE:
                        Log.d(TAG, "audio output plug in");
                        //update devices state
                        if(extra != null && extra.equals(AudioDeviceManagerObserver.H2W_DEV)){
                            headPhoneConnected = true;
                        }

                        //switch audio output
                        final ArrayList<String> audio_out = new ArrayList<String>();
                        if(extra != null && extra.equals(AudioDeviceManagerObserver.H2W_DEV)){
                            audio_out.add(name);
                            mAudioManagerEx.setAudioDeviceActive(audio_out, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
                            title = mCtx.getResources().getString(R.string.headphone_plug_in_title);
                            message = mCtx.getResources().getString(R.string.headphone_plug_in_message);
                        }else if(name.contains("USB")){
                            audio_out.add(name);
                            mHandler.post(new java.lang.Runnable() {
                                @Override
                                public void run() {
                                    // TODO Auto-generated method stub
                                    chooseUsbAudioDeviceDialog(audio_out);
                                }
                            });
                            //title = mCtx.getResources().getString(R.string.usb_audio_out_plug_in_title);
                        }
                        break;
                    }
                    break;
                case AudioDeviceManagerObserver.PLUG_OUT:
                    switch(type){
                    case AudioDeviceManagerObserver.AUDIO_INPUT_TYPE:
                        Log.d(TAG, "audio input plug out");
                        title = mCtx.getResources().getString(R.string.usb_audio_in_plug_out_title);
                        message = mCtx.getResources().getString(R.string.usb_audio_plug_out_message);
                        ArrayList<String> actived = mAudioManagerEx.getActiveAudioDevices(AudioManagerEx.AUDIO_INPUT_ACTIVE);
                        if(actived == null || actived.size() == 0 || actived.contains(name)){
                            ArrayList<String> ilist = new ArrayList<String>();
                            for(String dev:mAudioInputChannels){
                                if(dev.contains("USB")){
                                    ilist.add(dev);
                                    break;
                                }
                            }
                            if(ilist.size() == 0){
                                ilist.add(AudioManagerEx.AUDIO_NAME_CODEC);
                                ilist.add(AudioManagerEx.AUDIO_NAME_AC107);
                            }
                            mAudioManagerEx.setAudioDeviceActive(ilist, AudioManagerEx.AUDIO_INPUT_ACTIVE);
                            toastPlugOutNotification(title, message, AUDIO_IN_NOTIFY);
                        }
                        else if(!actived.contains("USB")){
                            ArrayList<String> ilist = new ArrayList<String>();
                            ilist.add(AudioManagerEx.AUDIO_NAME_CODEC);
                            ilist.add(AudioManagerEx.AUDIO_NAME_AC107);
                            mAudioManagerEx.setAudioDeviceActive(ilist, AudioManagerEx.AUDIO_INPUT_ACTIVE);
                        }
                        break;
                    case AudioDeviceManagerObserver.AUDIO_OUTPUT_TYPE:
                        ArrayList<String> olist = new ArrayList<String>();
                        Log.d(TAG, "audio output plug out");
                        if(extra != null && extra.equals(AudioDeviceManagerObserver.H2W_DEV)){
                            headPhoneConnected = false;
                            for(String dev:mAudioOutputChannels){
                                if(dev.contains("USB")){
                                    olist.add(dev);
                                    mAudioManagerEx.setAudioDeviceActive(olist,AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
                                    break;
                                }
                            }
                            if(olist.size() == 0){
                                switchAudioDevice(mDisplayOutputManager.getDisplayOutputType(0),false,false);
                            }
                            title = mCtx.getResources().getString(R.string.headphone_plug_out_title);
                            message = mCtx.getResources().getString(R.string.headphone_plug_out_message);
                        }else{
                            if(headPhoneConnected){
                                olist.add(AudioManagerEx.AUDIO_NAME_CODEC);
                            }else{
                                if(name.contains("USB")){
                                    Log.d(TAG,"switchAudioDevice, remove USB Audio device!");
                                    if (alertDialog != null && alertDialog.isShowing()) {
                                        alertDialog.dismiss();
                                        alertDialog = null;
                                    }
                                    switchAudioDevice(mDisplayOutputManager.getDisplayOutputType(0),true,false);
                                }else{
                                    Log.d(TAG,"switchAudioDevice, NO USB Audio device is connected!");
                                    switchAudioDevice(mDisplayOutputManager.getDisplayOutputType(0),false,false);
                                }
                            }
                            title = mCtx.getResources().getString(R.string.usb_audio_out_plug_out_title);
                            message = mCtx.getResources().getString(R.string.usb_audio_plug_out_message);
                        }
                        toastPlugOutNotification(title, message, AUDIO_OUT_NOTIFY);
                        break;
                    }
                    break;
                }
            }
        });

    }
    
    private void notifyHdmiAvailable(){
        Log.d(TAG, "notifyHdmiAvailable hdmi is available");
        AudioManagerEx.setHdmiAvailable(true);
        try{
            Thread.currentThread().sleep(100);
        }catch(Exception e){};
        
        ArrayList<String> list = mAudioManagerEx.getActiveAudioDevices(AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
        if(AudioManagerEx.getHdmiExpected()){
            mAudioManagerEx.setAudioDeviceActive(list, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
        }
    }
    
    private boolean isHdmiPlugin() {
        boolean plugged = false;
        final String filename = "/sys/class/extcon/extcon0/state";
        FileReader reader = null;
        try {
            reader = new FileReader(filename);
            char[] buf = new char[15];
            int n = reader.read(buf);
            if (n > 1) {
                plugged = 0 != Integer.parseInt(new String(buf, 5, 1));
            }
        } catch (IOException ex) {
            Log.w(TAG, "Couldn't read hdmi state from " + filename + ": " + ex);
        } catch (NumberFormatException ex) {
            Log.w(TAG, "Couldn't read hdmi state from " + filename + ": " + ex);
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException ex) {
                }
            }
        }
        return plugged;
    }
    
    private int getPolicy(){
        int ret = Settings.System.getInt(mCtx.getContentResolver(), Settings.System.AUDIO_MANAGE_POLICY, POLICY_0);
        Log.d(TAG, "get AUDIO_MANAGE_POLICY is " + ret);
        return ret;
    }
}

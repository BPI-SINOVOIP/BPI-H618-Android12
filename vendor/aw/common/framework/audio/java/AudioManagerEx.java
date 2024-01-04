package android.media;

import android.content.Context;
import android.media.AudioManager;
import android.os.SystemProperties;
import android.util.Log;
import android.provider.Settings;
import vendor.display.DisplayOutputManager;


import java.util.ArrayList;

public class AudioManagerEx extends AudioManager {

   /* define audio device name */
    public static final String AUDIO_NAME_CODEC     = "AUDIO_CODEC";
    public static final String AUDIO_NAME_AC107     = "AUDIO_AC107";
    public static final String AUDIO_NAME_HDMI      = "AUDIO_HDMI";
    public static final String AUDIO_NAME_SPDIF     = "AUDIO_SPDIF";

    /* define type of device */
    public static final String AUDIO_INPUT_TYPE     = "audio_devices_in";
    public static final String AUDIO_OUTPUT_TYPE    = "audio_devices_out";
    public static final String AUDIO_INPUT_ACTIVE   = "audio_devices_in_active";
    public static final String AUDIO_OUTPUT_ACTIVE  = "audio_devices_out_active";

    private static final String hdmiExpected = "audio.hdmi.expected";
    private static final String hdmiAvailable = "audio.hdmi.available";
    private static final String TAG = "AudioManagerEx";

    /* control the raw data mode */
    private static final String PROP_RAWDATA_KEY            =  "mediasw.sft.rawdata";
    private static final String PROP_RAWDATA_MODE_PCM       =  "PCM";
    private static final String PROP_RAWDATA_MODE_HDMI_RAW  =  "HDMI_RAW";
    private static final String PROP_RAWDATA_MODE_SPDIF_RAW =  "SPDIF_RAW";
    private static final String PROP_RAWDATA_DEFAULT_VALUE  =  PROP_RAWDATA_MODE_PCM;
    private final Context mContextEx;
    private DisplayOutputManager mDisplayOutputManager = null;
    private static final boolean enable_close_hdmiplug = false;

   public AudioManagerEx(Context ctx) {
    super(ctx);
    mContextEx = ctx;
    mDisplayOutputManager = DisplayOutputManager.getInstance();

        Log.d(TAG,"AudioManagerEx");
    // TODO Auto-generated constructor stub
    }

    private static void writeFileVal(String filename, int avail){
        try {
            SystemProperties.set(filename, String.valueOf(avail));
        } catch (Throwable e) {
            Log.e(TAG,"Failure set property "  +  filename + ", avail " + ", error " + e);
        }
    }

    private static int readFileVal(String filename){
        return SystemProperties.getInt(filename, 0);
    }

    /**
    * @hide
    */
    public static void setHdmiAvailable(boolean val){
        int avail = ((val == true) ? 1:0);
        writeFileVal(hdmiAvailable, avail);
    }

    public static boolean getHdmiAvailable(){
        return true;
    }
    /**
    * @hide
    */
    public static boolean getHdmiExpected(){
        int ret = readFileVal(hdmiExpected);
        return (ret == 1)?true:false;
    }

    /**
    * @hide
    */
    public static void setHdmiExpected(boolean val){
        int avail = (val == true)?1:0;
        writeFileVal(hdmiExpected, avail);
    }

    /* get audio devices list(the devices which are supported)
    *  @param devType  the audio device type,maybe in/out
    */
    public ArrayList<String> getAudioDevices(String devType){
        if(!devType.equals(AUDIO_INPUT_TYPE) && !devType.equals(AUDIO_OUTPUT_TYPE)){
            return null;
        }
        int length = 0;
        ArrayList<String> audioDevList = new ArrayList<String>();
        String list = getParameters(devType);
        if(list == null)
            return null;
        Log.d(TAG,"type " + devType + "  list " + list);
        String[] audioList = list.split(",");
        for (String audio: audioList) {
            length  = audio.length();
            if (audio.contains("="))
                audio = audio.substring(0, length-1);
            if (!"".equals(audio)) {
                audioDevList.add(audio);
            }
        }
        return audioDevList;
    }

    /* get current active devices */
    /* @param devType  the audio device type
    *  @return the current list of the active device
    */
    public ArrayList<String> getActiveAudioDevices(String devType){
        if(!devType.equals(AUDIO_INPUT_ACTIVE) && !devType.equals(AUDIO_OUTPUT_ACTIVE)){
            return null;
        }
        int length = 0;
        ArrayList<String> audioDevList = new ArrayList<String>();
        String list = getParameters(devType);
        if(list == null)
            return null;
        Log.d(TAG,"type " + devType + "  list " + list);
        String[] audioList = list.split(",");
        for (String audio: audioList) {
            length  = audio.length();
            if (audio.contains("="))
                audio = audio.substring(0, length-1);
            if (!"".equals(audio)) {
                audioDevList.add(audio);
            }
        }
        if(devType.equals(AUDIO_OUTPUT_ACTIVE)){
            if(!audioDevList.contains(AUDIO_NAME_HDMI) && getHdmiExpected()){
                audioDevList.add(AUDIO_NAME_HDMI);
            }
        }
        return audioDevList;
    }
    /* set audio devices active
    * @param devices
    * @param state  must be AUDIO_INPUT_ACTIVE or AUDIO_OUTPUT_ACTIVE
    */
    public void setAudioDeviceActive(ArrayList<String> dev, String state){
        if((!state.equals(AUDIO_INPUT_ACTIVE) && !state.equals(AUDIO_OUTPUT_ACTIVE)) ||
            dev == null)
        return;

        String audio = null;
        ArrayList<String> devices = (ArrayList<String>) dev.clone();
        Log.d(TAG, "state=" + state + ",dev=" + dev);
        boolean enablePassThrough = Settings.System.getInt(mContextEx.getContentResolver(),
                Settings.System.ENABLE_PASS_THROUGH, 0) == 1;
        Log.d(TAG,"enablePassThrough = " + enablePassThrough);
        if (enablePassThrough && state.equals(AUDIO_OUTPUT_ACTIVE)) {
            boolean usbConnected = false;
            for(String dev_name:devices){
                if(dev_name.contains("USB")){
                    usbConnected = true;
                    audio = dev_name;
                    Log.d(TAG,"USB Audio Output Device " + dev_name +" is Connect!");
                    break;
                }
            }
            if(usbConnected){
                SystemProperties.set(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_PCM);
                setParameter(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_PCM);
            }else{
                boolean hdmi = mDisplayOutputManager.getDisplayOutputType(0)
                        == DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI;
                Log.d(TAG, "Hdmi status " + hdmi + "," + devices.contains(AUDIO_NAME_HDMI) + "," +
                        devices.size());
                if(devices.size() > 0){
                    Log.d(TAG, "HDMI " + devices.get(0));
                }
                if(enable_close_hdmiplug){
                    if(devices.contains(AUDIO_NAME_HDMI)){
                        Log.d(TAG,"AUDIO_NAME_HDMI");
                        audio = AUDIO_NAME_HDMI;
                        SystemProperties.set(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_HDMI_RAW);
                        setParameter(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_HDMI_RAW);
                    }else if (devices.contains(AUDIO_NAME_SPDIF)) {
                        Log.d(TAG,"AUDIO_NAME_SPDIF");
                        audio = AUDIO_NAME_SPDIF;
                        SystemProperties.set(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_SPDIF_RAW);
                        setParameter(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_SPDIF_RAW);
                    } else if (devices.contains(AUDIO_NAME_CODEC)) {
                        Log.d(TAG,"AUDIO_NAME_CODEC");
                        audio = AUDIO_NAME_CODEC;
                        SystemProperties.set(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_PCM);
                        setParameter(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_PCM);
                    }
                }
                else{
                    if(devices.contains(AUDIO_NAME_HDMI) && hdmi){
                        Log.d(TAG,"AUDIO_NAME_HDMI");
                        audio = AUDIO_NAME_HDMI;
                        SystemProperties.set(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_HDMI_RAW);
                        setParameter(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_HDMI_RAW);
                    }else if (devices.contains(AUDIO_NAME_SPDIF)) {
                        Log.d(TAG,"AUDIO_NAME_SPDIF");
                        audio = AUDIO_NAME_SPDIF;
                        SystemProperties.set(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_SPDIF_RAW);
                        setParameter(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_SPDIF_RAW);
                    } else if (devices.contains(AUDIO_NAME_CODEC)) {
                        Log.d(TAG,"AUDIO_NAME_CODEC");
                        audio = AUDIO_NAME_CODEC;
                        SystemProperties.set(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_PCM);
                        setParameter(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_PCM);
                    }
                }
            }
        } else {
            if(state.equals(AUDIO_OUTPUT_ACTIVE)){
                Log.d(TAG, "hdmiAvailable=" + getHdmiAvailable()+ "  list=" + devices.toString());
                if(devices.contains(AUDIO_NAME_HDMI) && !getHdmiAvailable()){
                    setHdmiExpected(true);
                    devices.remove(AUDIO_NAME_HDMI);
                }else{
                    setHdmiExpected(false);
                }
            }
            for (String device : devices) {
                if (audio == null) {
                    audio = device;
                } else {
                audio = audio + "," + device;
                }
            }
        }
        Log.d(TAG, "  audio=" + audio);
        if (!enablePassThrough) {
            SystemProperties.set(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_PCM);
            setParameter(PROP_RAWDATA_KEY, PROP_RAWDATA_MODE_PCM);
        }
        setParameter(state,audio);
        //save audio out device
        if(state.equals(AUDIO_OUTPUT_ACTIVE)){
            Settings.System.putString(mContextEx.getContentResolver(), Settings.System.AUDIO_OUTPUT_CHANNEL, audio);
            Log.d(TAG, "Update Settings.System.AUDIO_OUTPUT_CHANNEL, now save audio is " + audio);
        }
    }

}


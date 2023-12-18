package com.allwinner.camera.utils;

import android.content.Context;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.media.SoundPool;
import android.preference.PreferenceManager;

import com.allwinner.camera.R;
import com.allwinner.camera.data.Contants;

import java.util.HashMap;
import java.util.Map;

public class SoundPlayerUtil {
    private static SoundPool soundPool;
    private static Context context;
    private static Map<String,Integer> soundMap; //音效资源id与加载过后的音源id的映射关系表
    private static SharedPreferences mSp;

    public final static String TYPE_VIDEO_SOUND = "type_video_sound";//sound of clicking video ui
    public final static String TYPE_IMAGE_SOUND = "type_image_sound";//sound of clicking image ui,such as square and auto

    /**
     * 初始化方法
     * @param c
     */
    public static void init(Context c){
        context = c;
        mSp = PreferenceManager.getDefaultSharedPreferences(context);
        initSound();
    }

    //初始化音效播放器
    private static void initSound(){
        soundPool = new SoundPool(10, AudioManager.STREAM_MUSIC,0);
        soundMap = new HashMap<>();
        soundMap.put(TYPE_IMAGE_SOUND, soundPool.load(context, R.raw.shutter, 1));
        soundMap.put(TYPE_VIDEO_SOUND, soundPool.load(context, R.raw.video_record, 1));
    }

    public static void playSound(String type){
        if(!mSp.getBoolean(Contants.KEY_SOUND_CAPTURE, false)){
            return;
        }
        Integer soundId = soundMap.get(type);
        if(soundId != null){
            soundPool.play(soundId, 1, 1, 1, 0, 1);
        }
    }

}

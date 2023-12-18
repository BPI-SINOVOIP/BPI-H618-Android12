package com.softwinner.dragonbox.utils;

import java.util.ArrayList;

import android.content.Context;
import android.media.AudioManager;
import android.media.SoundPool;
import android.util.Log;
import android.widget.Toast;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.platform.AudioManagerProxy;

public class MusicerUtil {
    private int mPlayType;
    private final static String TAG = "MusicerUtil";
    private static MusicerUtil instance;
    private int lastStreamVolume;//记录测试前系统状态：音量大小；
    private AudioManager mAM;
    SoundPool mSoundPool;
    Context mContext;
    public static final int PLAY_LEFT = 1;
    public static final int PLAY_RIGHT = 2;
    public static final int PLAY_NORMAL = 3;
    public static final int PLAY_RINGTONE = 4;

    public static synchronized MusicerUtil getInstance(Context context) {
        if (instance == null) {
            instance = new MusicerUtil(context);
        }
        return instance;
    }

    private MusicerUtil(Context context) {
        mContext = context;
        mAM = AudioManagerProxy.getAudioManager(context);
    }

    /**
     * @param playType PLAY_LEFT or PLAY_RIGHT or PLAY_NORMAL or PLAY_RINGTONE
     */
    public boolean playMusic(int playType) {
        lastStreamVolume = mAM.getStreamVolume(AudioManager.STREAM_MUSIC);

        int maxVolume = mAM.getStreamMaxVolume(AudioManager.STREAM_MUSIC);//任何测试开始前都采用最大音量。
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, maxVolume, 0);
        mPlayType = playType;
        if (mSoundPool != null) {
            mSoundPool.autoPause();
            mSoundPool.release();
        }
        mSoundPool = new SoundPool(2, AudioManager.STREAM_MUSIC, 0);
        switch (playType) {
            case PLAY_LEFT:
                final int soundLeftId = mSoundPool.load(mContext, R.raw.left, 1);
                mSoundPool
                    .setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
                        @Override
                        public void onLoadComplete(SoundPool soundPool,int sampleId, int status) {
                            mSoundPool.play(soundLeftId, 1.0f, 0.0f, 1,-1, 1.0f);
                        }
                    });

                break;
            case PLAY_RIGHT:
                final int soundRightId = mSoundPool.load(mContext, R.raw.right, 1);
                mSoundPool
                    .setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
                        @Override
                        public void onLoadComplete(SoundPool soundPool,int sampleId, int status) {
                            mSoundPool.play(soundRightId, 0.0f, 1.0f,1, -1, 1.0f);
                        }
                    });
                break;
            case PLAY_NORMAL:
                final int soundNormalId = mSoundPool.load(mContext, R.raw.hdmi, 1);
                mSoundPool.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
                        @Override
                        public void onLoadComplete(SoundPool soundPool,int sampleId, int status) {
                            mSoundPool.play(soundNormalId, 1.0f, 1.0f, 1,-1, 1.0f);
                        }
                    });
                break;
            case PLAY_RINGTONE:
                final int soundRingtoneId = mSoundPool.load(mContext, R.raw.ringtone, 1);
                if(soundRingtoneId == 0){
                    Toast.makeText(mContext, "load ringtone error!", 0).show();
                    break;
                }
                mSoundPool.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
                        @Override
                        public void onLoadComplete(SoundPool soundPool,int sampleId, int status) {
                            mSoundPool.play(sampleId, 1.0f, 1.0f, 1, -1, 1.0f);
                        }
                    });
                break;
            default:
                break;
        }
        return false;
    }

    public void pause() {
        mSoundPool.autoPause();
    }

    public void resume() {
        mSoundPool.autoResume();
    }

    public void release() {
        mSoundPool.release();
        instance = null;
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, lastStreamVolume , 0);//测试结束，恢复测试前音量
    }
}

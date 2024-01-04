package com.softwinner.dragonbox.manager;

import java.io.IOException;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnPreparedListener;

import com.cmcc.media.Micphone;
import com.cmcc.media.RTSoundEffects;

public class MixPlayManager {
    Context mContext;
    RTSoundEffects mEffects;
    MediaPlayer mMediaplayer;
    Micphone mMic;
    public MixPlayManager(Context context) {
        mContext = context;
        mMic = (Micphone) mContext.getSystemService("Micphone");
    }

    public int getMicNum(){
        return mMic.getMicNumber();
    }

    public void startMixPlay() throws IllegalArgumentException, IllegalStateException, IOException{
        mEffects = (RTSoundEffects) mContext.getSystemService("RTSoundEffects");
        mEffects.setReverbMode(2);/**KTV**/
        if (mMediaplayer == null) {
            mMediaplayer = new MediaPlayer();
        }
        mMediaplayer.reset();
        mMediaplayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        AssetFileDescriptor afd = mContext.getAssets().openFd("case_speaker_sample.mp3");
        mMediaplayer.setDataSource(afd.getFileDescriptor() , afd.getStartOffset(), afd.getLength());
        //mMediaplayer.setOnErrorListener(this);
        //mMediaplayer.setOnBufferingUpdateListener(this);
        mMediaplayer.setOnPreparedListener(new OnPreparedListener(){
            public void onPrepared (MediaPlayer mp){
                mMic.initial();
                mMic.start();
                mMediaplayer.start();
            }
        });
        //mMediaplayer.setOnInfoListener(this);
        //mMediaplayer.setOnCompletionListener(this);
        mMediaplayer.prepareAsync();
    }

    public void stopMixPlay(){
        if (mMediaplayer != null && mMediaplayer.isPlaying()) {
            mMediaplayer.stop();
            mMediaplayer.release();
            mMediaplayer = null;
        }
        if (mMic != null) {
            mMic.stop();
            mMic.release();
        }
    }
}

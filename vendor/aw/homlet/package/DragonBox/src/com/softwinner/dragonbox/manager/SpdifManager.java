package com.softwinner.dragonbox.manager;

import android.content.Context;
import android.os.Handler;

import com.softwinner.dragonbox.utils.MediaPlayerUtil;
import com.softwinner.dragonbox.utils.MusicerUtil;


public class SpdifManager {

    //private MusicerUtil mMusicUtil;
    private MediaPlayerUtil mMediaPlayerUtil;

    private boolean mLeftPlaySuccess = false;
    private boolean mRightPlaySuccess = false;
    private Handler mHandler = new Handler();
    public SpdifManager(Context context) {
        //mMusicUtil = MusicerUtil.getInstance(context);
        mMediaPlayerUtil = MediaPlayerUtil.getInstance(context);
    }

    public void playLeft(){
        //mMusicUtil.playMusic(MusicerUtil.PLAY_LEFT);
        mMediaPlayerUtil.playMusic(MediaPlayerUtil.PLAY_LEFT);
    }

    public void playRight(){
        mMediaPlayerUtil.stop();
        mMediaPlayerUtil.release();
        mMediaPlayerUtil.playMusic(MediaPlayerUtil.PLAY_RIGHT);
    }

    public void stopPlaying(){
        //mMusicUtil.pause();
        mMediaPlayerUtil.release();
    }

    public boolean isLeftPlaySuccess() {
        return mLeftPlaySuccess;
    }

    public void setLeftPlaySuccess(boolean leftPlaySuccess) {
        this.mLeftPlaySuccess = leftPlaySuccess;
    }

    public boolean isRightPlaySuccess() {
        return mRightPlaySuccess;
    }

    public void setRightPlaySuccess(boolean rightPlaySuccess) {
        this.mRightPlaySuccess = rightPlaySuccess;
    }

    public boolean getResult () {
        return isLeftPlaySuccess() && isRightPlaySuccess();
    }

}

package com.softwinner.dragonbox.manager;

import android.content.Context;
import android.media.AudioManager;

import com.softwinner.dragonbox.platform.DisplayManagerPlatform;
import com.softwinner.dragonbox.platform.IDisplayManagerPlatform;
import com.softwinner.dragonbox.utils.AudioChannelUtil;
import com.softwinner.dragonbox.utils.MediaPlayerUtil;
import com.softwinner.dragonbox.utils.MusicerUtil;
import java.util.List;
import android.widget.Toast;
public class HdmiManager {

    private Context mContext;
    private IDisplayManagerPlatform mDisplayManagerPlatform;

    public HdmiManager(Context context) {
        mContext = context;
        mDisplayManagerPlatform = new DisplayManagerPlatform(context);
    }

    public void changeToHDMI() {
        mDisplayManagerPlatform.changeToHDMI();
    }

    public void playMusic(){
        //MusicerUtil.getInstance(mContext).playMusic(MusicerUtil.PLAY_NORMAL);
        MediaPlayerUtil.getInstance(mContext).playMusic(MediaPlayerUtil.PLAY_NORMAL);
    }

    public void playMusic(int random){
        //MusicerUtil.getInstance(mContext).playMusic(MusicerUtil.PLAY_NORMAL);
        MediaPlayerUtil.getInstance(mContext).playMusicRandom(MediaPlayerUtil.PLAY_NORMAL,random);
    }

    public void stopMusic(){
        //MusicerUtil.getInstance(mContext).pause();
        MediaPlayerUtil.getInstance(mContext).release();
    }

    public boolean isHDMIStatusConn(){
        return mDisplayManagerPlatform.getHdmiHotPlugStatus();
    }

}

package com.softwinner.dragonbox.testcase;

import java.io.File;
import org.xmlpull.v1.XmlPullParser;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.widget.TextView;
import android.util.Log;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.utils.Utils;

public class CaseHeadSet extends IBaseCase {
    public static final String TAG = "DragonBox-CaseHeadSet";
    HeadSetPlugReceiver mHeadSetPlugReceiver = null;
    Context mContext;
    AudioManager mAudioManager;
    String mMusicName = "case_speaker_sample.mp3";
    MediaPlayer mMediaplayer;
    TextView mTvMinTestResult;
    TextView mTvMaxInfo;
    boolean mMusicPlaying=false;
    public CaseHeadSet(Context context) {
        super(context, R.string.case_headset_name, R.layout.case_headset_max,
                R.layout.case_headset_min, TYPE_MODE_MANUAL);
        mContext = context;
        mTvMinTestResult = (TextView) mMinView.findViewById(R.id.case_headset_status);
        mTvMaxInfo = (TextView) mMaxView.findViewById(R.id.case_headset_max_info);
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        Utils.writeFileToCacheSecure(mMusicName,mContext);
    }

    public CaseHeadSet(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    public class HeadSetPlugReceiver extends BroadcastReceiver{
        @Override
        public void onReceive(Context context, Intent intent) {
            if(intent.hasExtra("state")) {
                if(intent.getIntExtra("state", 0)==0){
                    Log.w(TAG,"Receive HeadSetPlugReceiver, headset plug out, state is "+intent.getIntExtra("state", 0));
                    if (mMediaplayer != null && mMediaplayer.isPlaying()) {
                        Log.w(TAG,"headset plug out ,stop play music!");
                        mMediaplayer.stop();
                        mMediaplayer.release();
                        mMediaplayer = null;
                        mMusicPlaying = false;
                    }
                    mTvMaxInfo.setText(R.string.case_headset_manual_plug);
                    setDialogPositiveButtonEnable(false);
                }else if(intent.getIntExtra("state", 0)==1){
                    Log.w(TAG,"Receive HeadSetPlugReceiver, headset plug in, state is "+intent.getIntExtra("state", 0));
                    mTvMaxInfo.setText(R.string.case_headset_info);
                    setDialogPositiveButtonEnable(true);
                    synchronized (mContext) {
                        if(!mMusicPlaying) {
                            playMusic();
                        }
                    }
                }else {
                    Log.w(TAG,"headset state is "+intent.getIntExtra("state", 0));
                }
            }
        }
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseHeadSet");
        if(mHeadSetPlugReceiver == null) {
            Log.w(TAG, "mHeadSetPlugReceiver is null ,register mHeadSetPlugReceiver");
            registerHeadSetPlugReceiver();
        }
        if(mAudioManager.isWiredHeadsetOn()) {
            synchronized (mContext) {
                if(!mMusicPlaying) {
                    playMusic();
                }
            }
        }else {
            mTvMaxInfo.setText(R.string.case_headset_manual_plug);
            setDialogPositiveButtonEnable(false);
            //提示插入耳机
        }
        Log.w(TAG, "headset plug state is "+mAudioManager.isWiredHeadsetOn());
    }

    public void playMusic() {
        try {
            Log.w(TAG, "try to play music!");
            mMusicPlaying = true;
            if (mMediaplayer == null) {
                mMediaplayer = new MediaPlayer();
            }

            mMediaplayer.reset();
            mMediaplayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mMediaplayer.setDataSource(mContext.getCacheDir().getPath() + File.separator + mMusicName);
            mMediaplayer.setOnPreparedListener(new OnPreparedListener() {
                public void onPrepared(MediaPlayer mp) {
                    mMediaplayer.setLooping(true);
                    mMediaplayer.start();
                }
            });
            mMediaplayer.prepareAsync();
        } catch (Exception e) {
            // TODO: handle exception
        }

    }

    private void registerHeadSetPlugReceiver() {
        mHeadSetPlugReceiver = new HeadSetPlugReceiver();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction("android.intent.action.HEADSET_PLUG");
        mContext.registerReceiver(mHeadSetPlugReceiver,intentFilter);
    }

    @Override
    public void onStopCase() {
        if (mMediaplayer != null && mMediaplayer.isPlaying()) {
            mMusicPlaying = false;
            mMediaplayer.stop();
            mMediaplayer.release();
            mMediaplayer = null;
            Log.w(TAG,"test over ,release mediaplayer");
        }
        if (mHeadSetPlugReceiver != null) {
            Log.w(TAG,"test over,release mHeadSetPlugReceiver");
            mContext.unregisterReceiver(mHeadSetPlugReceiver);
            mHeadSetPlugReceiver = null;
        }
        Log.w(TAG,"HeadSet testcase end, test result is "+getCaseResult());
        if(getCaseResult()) {
            mTvMinTestResult.setText(mContext.getString(R.string.case_headset_test_result)+"成功");
        }else {
            mTvMinTestResult.setText(mContext.getString(R.string.case_headset_test_result)+"失败");
        }
    }

    @Override
    public void reset() {
        super.reset();
    }

}

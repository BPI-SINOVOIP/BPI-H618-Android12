package com.allwinnertech.socs;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

public class VideoTestActivity extends Activity implements OnCompletionListener, OnErrorListener {

    private static final String TAG = "VideoTestActivity";
    Context mContext;
    private String configFilePath;
    Handler mHandler;
    FullScreenVideoView mVideoView;
    TextView mVideoInfo;
    TestsConfig testsconfig;
    ToastInfo mTostInfo;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_video_test);
        mContext = this;
        configFilePath = mContext.getFilesDir() + "/config.json";
        testsconfig = new TestsConfig();
        testsconfig.loadAllConfigFromFile(configFilePath);
        setMaxVolume(mContext);
        mVideoView = (FullScreenVideoView) this.findViewById(R.id.fullscreenview);
        mVideoInfo = (TextView)this.findViewById(R.id.videoinfo);
        mVideoView.setOnCompletionListener(this);
        mHandler = new Handler();
        mTostInfo = new ToastInfo(mContext);
        mHandler.postDelayed(new Runnable(){
            @Override
            public void run(){
                startTfcardVideoViewTest();
            }
        }, 100);
    }


    private void setMaxVolume(Context context){
	AudioManager am = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
	int maxVolume = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
	am.setStreamVolume(AudioManager.STREAM_MUSIC, maxVolume, AudioManager.FLAG_PLAY_SOUND);
    }

    private void startTfcardVideoViewTest(){
        String videoPath = testsconfig.getTfcardVideoPath();
        File mediaFile = new File(videoPath);
        if(!mediaFile.exists()){
            mTostInfo.showInfo(mContext.getResources().getString(R.string.set_tfcard_path));
            return;
        }
        mVideoView.setVideoPath(videoPath);
        mVideoInfo.setText(videoPath);
        mVideoView.start();
    }

    private void startUsbhostVideoViewTest(){
        String videoPath = testsconfig.getUsbhostVideoPath();
        File mediaFile = new File(videoPath);
        if(!mediaFile.exists()){
            mTostInfo.showInfo(mContext.getResources().getString(R.string.set_usbhost_path));
        }
        mVideoView.setVideoPath(videoPath);
        mVideoInfo.setText(videoPath);
        mVideoView.start();
    }



    @Override
    public void onCompletion(MediaPlayer arg0) {
        Log.d(TAG,"video play finish");
        if(!mVideoView.getCurrentVideoPath().equals(testsconfig.getUsbhostVideoPath())){
            startUsbhostVideoViewTest();
        }else{
            mTostInfo.showInfo(mContext.getResources().getString(R.string.finish_video_test));
            startNextTest();
        }

    }

    public void startNextTest(){
        Intent intent = new Intent();
        intent.putExtra(Utilities.RESULT_KEY, "test pass");
        setResult(RESULT_OK, intent);
        finish();
    }


    @Override
    public boolean onError(MediaPlayer arg0, int arg1, int arg2) {
        mTostInfo.showInfo(mContext.getResources().getString(R.string.video_test_error) + "Code = " + arg1);
        return false;
    }
}

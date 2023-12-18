package com.softwinner.dragonbox.manager;

import java.io.File;
import java.io.IOException;

import com.softwinner.dragonbox.testcase.CaseYSTMicRecord;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaRecorder;
import android.widget.Toast;

public class MicRecordManager {
    Context mContext;

    private MediaPlayer mMediaplayer = null;
    private MediaRecorder mRecorder = null;

    private static final String OUTPUT_FILE = "audiorecordtest.3gp";
    public String mFilePath;
    public MicRecordManager(Context context) {
        mContext = context;
        mFilePath = mContext.getCacheDir().getPath() + File.separator + OUTPUT_FILE;
    }

    public void startRecorder(){
        stopPlay();
        if (mRecorder == null) {
            mRecorder = new MediaRecorder();
        }
        //mRecorder.setAudioSource(MediaRecorder.AudioSource.CMCC_KARAOK_MIC);
        mRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mRecorder.setOutputFormat(MediaRecorder.OutputFormat.DEFAULT);
        File recordFile = new File(mFilePath);
        if(recordFile.exists())
            recordFile.delete();
        mRecorder.setOutputFile(mFilePath);
        mRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.DEFAULT);
        try {
            mRecorder.prepare();
        } catch (IOException e) {
            e.printStackTrace();
        }
        mRecorder.start();
    }

    public void stopRecorder(){
        if (mRecorder != null) {
            try{
                mRecorder.stop();
            }catch(Exception e){
                Toast.makeText(mContext,"获取音频数据失败,请检测MIC设备是否存在",0).show();
                mRecorder.release();
                mRecorder = null;
            }
        }
    }
    public void stopPlay() {
        if (mMediaplayer != null && mMediaplayer.isPlaying())
            mMediaplayer.stop();
    }

    public void playRecorder() throws IOException {
        if (mMediaplayer == null) {
            mMediaplayer = new MediaPlayer();
        }
        mMediaplayer.reset();
        mMediaplayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        mMediaplayer.setDataSource(mFilePath);
        // mMediaplayer.setOnErrorListener(this);
        // mMediaplayer.setOnBufferingUpdateListener(this);
        mMediaplayer.setOnPreparedListener(new OnPreparedListener() {
            public void onPrepared(MediaPlayer mp) {
                mMediaplayer.setLooping(true);
                mMediaplayer.start();
            }
        });
        // mMediaplayer.setOnInfoListener(this);
        // mMediaplayer.setOnCompletionListener(this);
        mMediaplayer.prepareAsync();
    }

    public void playRecorder(int playType) throws IOException {
        if (mMediaplayer == null) {
            mMediaplayer = new MediaPlayer();
        }
        mMediaplayer.reset();
        mMediaplayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        switch (playType) {
            case CaseYSTMicRecord.PLAY_LEFT:
                mMediaplayer.setVolume(1, 0);
                break;
            case CaseYSTMicRecord.PLAY_RIGHT:
                mMediaplayer.setVolume(0, 1);
                break;
            case CaseYSTMicRecord.PLAY_NORMAL:
                mMediaplayer.setVolume(1, 1);
                break;
            default:
                break;
        }
        mMediaplayer.setDataSource(mFilePath);
        // mMediaplayer.setOnErrorListener(this);
        // mMediaplayer.setOnBufferingUpdateListener(this);
        mMediaplayer.setOnPreparedListener(new OnPreparedListener() {
            public void onPrepared(MediaPlayer mp) {
                mMediaplayer.setLooping(true);
                mMediaplayer.start();
            }
        });
        // mMediaplayer.setOnInfoListener(this);
        // mMediaplayer.setOnCompletionListener(this);
        mMediaplayer.prepareAsync();
    }

    public void release(){
        if (mMediaplayer != null && mMediaplayer.isPlaying()) {
            mMediaplayer.stop();
            mMediaplayer.release();
            mMediaplayer = null;
        }
        if (mRecorder != null) {
            mRecorder.release();
            mRecorder = null;
        }
    }


}

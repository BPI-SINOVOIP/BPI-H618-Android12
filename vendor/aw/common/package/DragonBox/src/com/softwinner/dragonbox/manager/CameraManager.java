package com.softwinner.dragonbox.manager;

import java.io.File;
import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

import android.content.Context;
import android.hardware.Camera;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaRecorder;
import android.view.SurfaceView;

public class CameraManager {
    public static final String OUTPUT_FILE = "test.mp4";
    public String mFilePath;
    public CameraManager(Context context) {
        mFilePath = context.getCacheDir().getPath() + File.separator + OUTPUT_FILE;
    }

    private MediaRecorder mediarecorder;
    boolean isRecording;

    public synchronized void startRecording(SurfaceView surfaceView,int iRecordWidth,int iRecordHeight) {
        mediarecorder = new MediaRecorder();// 创建mediarecorder对象
        // 设置录制视频源为Camera(相机)
        // Camera c = Camera.open();
        // mediarecorder.setCamera(c);
        mediarecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
        // 设置录制完成后视频的封装格式THREE_GPP为3gp.MPEG_4为mp4
        mediarecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
        // 设置录制的视频编码h263 h264
        mediarecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
        // 设置视频录制的分辨率。必须放在设置编码和格式的后面，否则报错
        mediarecorder.setVideoSize(iRecordWidth, iRecordHeight);
        mediarecorder.setVideoEncodingBitRate(3000000);
        // 设置录制的视频帧率。必须放在设置编码和格式的后面，否则报错
        mediarecorder.setVideoFrameRate(30);
        mediarecorder.setPreviewDisplay(surfaceView.getHolder().getSurface());
        // 设置视频文件输出的路径
        //lastFileName = newFileName();
        mediarecorder.setOutputFile(mFilePath);
        try {
            // 准备录制
            mediarecorder.prepare();
            // 开始录制
            mediarecorder.start();
        } catch (IllegalStateException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        isRecording = true;
        timeSize = 0;
        timer = new Timer();
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                timeSize++;
            }
        }, 0, 1000);
    }

    Timer timer;
    int timeSize = 0;
    private String lastFileName;

    public String getLastFileName() {
        return lastFileName;
    }

    public synchronized void stopRecording() {
        if (mediarecorder != null) {
            // 停止
            mediarecorder.stop();
            mediarecorder.release();
            mediarecorder = null;
            timer.cancel();
            if (null != lastFileName && !"".equals(lastFileName)) {
                File f = new File(lastFileName);
                String name = f.getName().substring(0,
                        f.getName().lastIndexOf(".3gp"));
                name += "_" + timeSize + "s.3gp";
                String newPath = f.getParentFile().getAbsolutePath() + "/"
                        + name;
                if (f.renameTo(new File(newPath))) {
                    int i = 0;
                    i++;
                }
            }
        }
    }

    public String newFileName() {
        try {
            return File.createTempFile("/mov_", ".3gp").getAbsolutePath();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            return null;
        }
    }

    public synchronized void releaseRecord() {
        if (mediarecorder != null) {
            // 停止
            mediarecorder.stop();
            mediarecorder.release();
            mediarecorder = null;
        }
    }

    private MediaPlayer mPlayer = null;

    public synchronized void startPlayVideo(String fileName, SurfaceView view) throws Exception{
        mPlayer = new MediaPlayer();
        mPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        mPlayer.setDisplay(view.getHolder()); // 定义一个SurfaceView播放它
        mPlayer.setOnCompletionListener(new OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer arg0) {
                releasePlayVideo();
            }
        });
        try {
            mPlayer.setDataSource(fileName);
            mPlayer.prepare();
        } catch (IllegalStateException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        mPlayer.start();
    }

    public synchronized void releasePlayVideo() {
        if (mPlayer != null) {
            if (mPlayer.isPlaying()) {
                mPlayer.stop();
            }
            mPlayer.release();
            mPlayer = null;
        }
    }

}

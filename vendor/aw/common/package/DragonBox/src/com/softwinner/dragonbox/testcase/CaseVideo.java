package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.MediaController;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.VideoView;
import android.media.AudioManager;
import com.softwinner.dragonbox.platform.AudioManagerProxy;

import com.softwinner.dragonbox.utils.AudioChannelUtil;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.VideoManager;

public class CaseVideo extends IBaseCase {
    VideoView videoView;
    ProgressBar progressBar;
    VideoManager videoManager;
    private Context mContext;
    private AudioManager mAM;
    private int lastStreamVolume;
    private TextView mMinVideoResult;
    private TextView tv_case_video_name;
    private ProgressBar pb_case_video_progress_bar;

    public CaseVideo(Context context) {

        super(context, R.string.case_video_name, R.layout.case_video_max,R.layout.case_video_min, TYPE_MODE_MANUAL);
        videoManager = new VideoManager(context);
        videoView = (VideoView) mMaxView.findViewById(R.id.case_video_videoview);
        tv_case_video_name = (TextView) mMaxView.findViewById(R.id.case_video_name);
        pb_case_video_progress_bar = (ProgressBar) mMaxView.findViewById(R.id.case_video_progress_bar);
        if (isTvCaseVideo(R.layout.case_video_max)) {
            tv_case_video_name.setVisibility(View.GONE);
            pb_case_video_progress_bar.setVisibility(View.GONE);
        }
        videoView.setZOrderOnTop(true);
        progressBar = (ProgressBar) mMaxView.findViewById(R.id.case_video_progress_bar);
        mMinVideoResult = (TextView) mMinView.findViewById(R.id.case_video_result);
        videoView.setVideoPath(videoManager.videoFile.getAbsolutePath());
        mContext = context;
        mAM= (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
    }

    public CaseVideo(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    @Override
    public void onStartCase() {
        Log.d("----", "getDuration=" + videoView.getDuration());
        lastStreamVolume = mAM.getStreamVolume(AudioManager.STREAM_MUSIC);
        int maxVolume = mAM.getStreamMaxVolume(AudioManager.STREAM_MUSIC);//set the max Volume before testing
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, maxVolume, 0);
        AudioChannelUtil.setOuputChannels(mContext,true,AudioManagerProxy.AUDIO_NAME_CODEC,AudioManagerProxy.AUDIO_NAME_HDMI);
        videoView.setOnCompletionListener(new OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer player) {
                videoView.setVideoPath(videoManager.videoFile.getAbsolutePath());
                videoView.start();

            }
        });
        videoView.seekTo(0);
        progressBar.setProgress(0);
        videoView.start();
        final Handler handler = new Handler();
        new Thread(new Runnable() {

            @Override
            public void run() {
                while (mMaxViewDialog.isShowing()) {
                    if (videoView.isPlaying()) {
                        handler.post(new Runnable() {

                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                progressBar.setMax(videoView.getDuration());
                                progressBar.setProgress(videoView.getCurrentPosition());
                            }
                        });
                    }
                    try {
                        Thread.sleep(20);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
                Log.d("----", "not showing");
            }
        }).start();
    }

    @Override
    public void onStopCase() {
        videoView.pause();
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, lastStreamVolume , 0);//restore StreamVolume after testing
        AudioChannelUtil.setOuputChannels(mContext,false,AudioManagerProxy.AUDIO_NAME_CODEC,AudioManagerProxy.AUDIO_NAME_HDMI);
        mMinVideoResult.setText(getCaseResult() ? R.string.case_video_result_success
                : R.string.case_video_result_fail);
    }

    @Override
    public void reset() {
        super.reset();
        mMinVideoResult.setText(R.string.case_video_result);
    }
}

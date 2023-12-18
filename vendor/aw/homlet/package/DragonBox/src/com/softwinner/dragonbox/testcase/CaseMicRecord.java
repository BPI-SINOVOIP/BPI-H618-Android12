package com.softwinner.dragonbox.testcase;

import java.io.IOException;
import java.io.File;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.widget.TextView;
import android.media.AudioManager;
import com.softwinner.dragonbox.utils.AudioChannelUtil;
import com.softwinner.dragonbox.platform.AudioManagerProxy;


import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.MicRecordManager;

public class CaseMicRecord extends IBaseCase {
    private static final int WHAT_COUNT_DOWN = 1;
    private static final int WHAT_COUNT_DOWN_FINISH = 2;

    private int mrecordTime;
    MicRecordManager mMicRecordManager;
    TextView mMinMicrecordStatus;
    TextView mMaxMicrecordStatus;
    private AudioManager mAM;
    private int lastStreamVolume;
    private boolean mStopThread = false;

    Handler mHandler = new Handler(Looper.myLooper()) {
        @Override
        public void handleMessage(Message message) {
            switch (message.what) {
                case WHAT_COUNT_DOWN:
                    mMaxMicrecordStatus.setText(mContext.getString(R.string.case_micrecord_info, message.arg1));
                    break;
                case WHAT_COUNT_DOWN_FINISH:
                    setDialogPositiveButtonEnable(true);
                    mMaxMicrecordStatus.setText(R.string.case_micrecord_play_info);
                    mMicRecordManager.stopRecorder();
                    try {
                        mMicRecordManager.playRecorder();
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    break;

            }
        }
    };

    public CaseMicRecord(Context context) {
        super(context, R.string.case_micrecord_name, R.layout.case_micrecord_max,
                R.layout.case_micrecord_min, TYPE_MODE_MANUAL);
        mMicRecordManager = new MicRecordManager(context);
        mMinMicrecordStatus = (TextView) mMinView
                .findViewById(R.id.case_micrecord_status);
        mMaxMicrecordStatus = (TextView) mMaxView
                .findViewById(R.id.case_micrecord_max_status);
        mAM= (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
    }

    public CaseMicRecord(Context context, XmlPullParser xmlParser) {
        this(context);
        String time = xmlParser.getAttributeValue(null, "recordtime");
        try{
            mrecordTime = Integer.parseInt(time);
        } catch (Exception e) {
            e.printStackTrace();
            mrecordTime = 5;
        }
    }

    @Override
    public void onStartCase() {
        mStopThread = false;
        lastStreamVolume = mAM.getStreamVolume(AudioManager.STREAM_MUSIC);
        int maxVolume = mAM.getStreamMaxVolume(AudioManager.STREAM_MUSIC);//set the max Volume before testing
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, maxVolume, 0);
        AudioChannelUtil.setOuputChannels(mContext,true,AudioManagerProxy.AUDIO_NAME_CODEC,AudioManagerProxy.AUDIO_NAME_HDMI);
        setDialogPositiveButtonEnable(false);
        try {
            mMicRecordManager.startRecorder();
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        //Message msg = mHandler.obtainMessage(WHAT_COUNT_DOWN_FINISH);
        // mHandler.handleMessage(msg);
        new Thread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < mrecordTime; i++) {
                    if (mStopThread)
                        return;
                    final int time = mrecordTime - i;
                    // Message msg = mHandler.obtainMessage(WHAT_COUNT_DOWN);
                    Message msg = new Message();
                    msg.what = WHAT_COUNT_DOWN;
                    msg.arg1 = time;
                    mHandler.sendMessage(msg);
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                Message msg = mHandler.obtainMessage(WHAT_COUNT_DOWN_FINISH);
                mHandler.sendMessage(msg);
            }
        }).start();

    }

    @Override
    public void onStopCase() {
        mStopThread = true;
        mMicRecordManager.release();
        mMinMicrecordStatus
                .setText(getCaseResult() ? R.string.case_micrecord_status_success_text
                        : R.string.case_micrecord_status_fail_text);
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, lastStreamVolume , 0);//restore StreamVolume after testing
        AudioChannelUtil.setOuputChannels(mContext,false,AudioManagerProxy.AUDIO_NAME_CODEC,AudioManagerProxy.AUDIO_NAME_HDMI);
        File file = new File(mMicRecordManager.mFilePath);
        if(file.exists()){
            file.delete();
        }
    }

    @Override
    public void reset() {
        super.reset();
        mMinMicrecordStatus.setText(R.string.case_micrecord_status_text);

    }

}

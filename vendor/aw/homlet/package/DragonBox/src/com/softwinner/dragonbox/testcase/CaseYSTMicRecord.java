package com.softwinner.dragonbox.testcase;

import java.io.File;
import java.io.IOException;

import org.xmlpull.v1.XmlPullParser;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.MicRecordManager;
import com.softwinner.dragonbox.manager.SpdifManager;
import com.softwinner.dragonbox.utils.AudioChannelUtil;
import com.softwinner.dragonbox.platform.AudioManagerProxy;

import android.content.Context;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

public class CaseYSTMicRecord extends IBaseCase implements OnClickListener {
    private static final int WHAT_COUNT_DOWN = 1;
    private static final int WHAT_COUNT_DOWN_FINISH = 2;

    public static final int PLAY_LEFT = 1;
    public static final int PLAY_RIGHT = 2;
    public static final int PLAY_NORMAL = 3;

    private int mrecordTime;
    MicRecordManager mMicRecordManager;
    private AudioManager mAM;
    private int lastStreamVolume;
    private boolean mLeftResult = false;
    private boolean mRightResult = false;
    private boolean mBothResult = false;
    private boolean mStopThread = false;

    Button leftButtonPass;
    Button leftButtonFail;
    Button rightButtonPass;
    Button rightButtonFail;
    Button bothButtonPass;
    Button bothButtonFail;
    Button recordButtonFail;
    ViewGroup leftContainer;//switch  view content
    ViewGroup rightContainer;
    ViewGroup bothContainer;
    ViewGroup recordContainer;
    TextView mShowAlertTV;//dialog text
    TextView mMinLeftResultTV;
    TextView mMinRightResultTV;
    TextView mMinBothResultTV;
    Thread timeThread;

    Handler mHandler = new Handler(Looper.myLooper()) {
        @Override
        public void handleMessage(Message message) {
            switch (message.what) {
                case WHAT_COUNT_DOWN:
                    mShowAlertTV.setText(mContext.getString(R.string.case_micrecord_info, message.arg1));
                    break;
                case WHAT_COUNT_DOWN_FINISH:
                    mMicRecordManager.stopRecorder();
                    resetTest(PLAY_LEFT);
                    break;
            }
        }
    };

    public CaseYSTMicRecord(Context context) {
        super(context, R.string.case_micrecord_name,R.layout.case_yst_micrecord_max, R.layout.case_yst_micrecord_min,TYPE_MODE_MANUAL);
        mMicRecordManager = new MicRecordManager(context);

        leftButtonPass = (Button) mMaxView.findViewById(R.id.case_micrecord_left_button_passed);
        leftButtonFail = (Button) mMaxView.findViewById(R.id.case_micrecord_left_button_fail);
        rightButtonPass = (Button) mMaxView.findViewById(R.id.case_micrecord_right_button_passed);
        rightButtonFail = (Button) mMaxView.findViewById(R.id.case_micrecord_right_button_fail);
        bothButtonPass = (Button) mMaxView.findViewById(R.id.case_micrecord_both_button_passed);
        bothButtonFail = (Button) mMaxView.findViewById(R.id.case_micrecord_both_button_fail);
        recordButtonFail = (Button) mMaxView.findViewById(R.id.case_micrecord_button_fail);
        recordContainer = (ViewGroup) mMaxView.findViewById(R.id.case_micrecord_container);
        leftContainer = (ViewGroup) mMaxView.findViewById(R.id.case_micrecord_left_container);
        rightContainer = (ViewGroup) mMaxView.findViewById(R.id.case_micrecord_right_container);
        bothContainer = (ViewGroup) mMaxView.findViewById(R.id.case_micrecord_both_container);
        mShowAlertTV = (TextView) mMaxView.findViewById(R.id.case_micrecord_alert);
        mMinLeftResultTV = (TextView) mMinView.findViewById(R.id.case_micrecord_left_status);
        mMinRightResultTV = (TextView) mMinView.findViewById(R.id.case_micrecord_right_status);
        mMinBothResultTV = (TextView) mMinView.findViewById(R.id.case_micrecord_both_status);

        leftButtonPass.setOnClickListener(this);
        leftButtonFail.setOnClickListener(this);
        rightButtonPass.setOnClickListener(this);
        rightButtonFail.setOnClickListener(this);
        bothButtonPass.setOnClickListener(this);
        bothButtonFail.setOnClickListener(this);
        recordButtonFail.setOnClickListener(this);

        setShowDialogButton(false);
        mAM = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
    }

    public CaseYSTMicRecord(Context context, XmlPullParser xmlParser) {
        this(context);
        String time = xmlParser.getAttributeValue(null, "recordtime");
        try {
            mrecordTime = Integer.parseInt(time);
        } catch (Exception e) {
            e.printStackTrace();
            mrecordTime = 5;
        }
    }

    private void resetTest(int playType) {
        switch (playType) {
            case PLAY_LEFT:
                recordContainer.setVisibility(View.GONE);
                leftContainer.setVisibility(View.VISIBLE);
                rightContainer.setVisibility(View.GONE);
                bothContainer.setVisibility(View.GONE);
                mMaxViewDialog.setTitle(R.string.case_micrecord_left_title);
                mShowAlertTV.setText(R.string.case_micrecord_left_alert);
                try {
                    mMicRecordManager.playRecorder(PLAY_LEFT);
                } catch (IOException e) {
                    e.printStackTrace();
                }
                leftButtonPass.requestFocus();
                break;
            case PLAY_RIGHT:
                recordContainer.setVisibility(View.GONE);
                leftContainer.setVisibility(View.GONE);
                rightContainer.setVisibility(View.VISIBLE);
                bothContainer.setVisibility(View.GONE);
                mMaxViewDialog.setTitle(R.string.case_micrecord_right_title);
                mShowAlertTV.setText(R.string.case_micrecord_right_alert);
                try {
                    mMicRecordManager.playRecorder(PLAY_RIGHT);
                } catch (IOException e) {
                    e.printStackTrace();
                }
                rightButtonPass.requestFocus();
                break;
            case PLAY_NORMAL:
                recordContainer.setVisibility(View.GONE);
                leftContainer.setVisibility(View.GONE);
                rightContainer.setVisibility(View.GONE);
                bothContainer.setVisibility(View.VISIBLE);
                mMaxViewDialog.setTitle(R.string.case_micrecord_both_title);
                mShowAlertTV.setText(R.string.case_micrecord_both_alert);
                try {
                    mMicRecordManager.playRecorder(PLAY_NORMAL);
                } catch (IOException e) {
                    e.printStackTrace();
                }
                if(mLeftResult&&mRightResult) {
                    bothButtonPass.requestFocus();
                }else {
                    bothButtonFail.requestFocus();
                }
                break;
            default:
                break;
        }
    }
    @Override
    public void onStartCase() {
        mStopThread = false;
        lastStreamVolume = mAM.getStreamVolume(AudioManager.STREAM_MUSIC);
        int maxVolume = mAM.getStreamMaxVolume(AudioManager.STREAM_MUSIC);// set the max Volume before testing
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, maxVolume, 0);
        AudioChannelUtil.setOuputChannels(mContext, true,
                AudioManagerProxy.AUDIO_NAME_CODEC,AudioManagerProxy.AUDIO_NAME_HDMI);
        recordContainer.setVisibility(View.VISIBLE);
        leftContainer.setVisibility(View.GONE);
        rightContainer.setVisibility(View.GONE);
        bothContainer.setVisibility(View.GONE);
        recordButtonFail.requestFocus();
        try {
            mMicRecordManager.startRecorder();
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        // Message msg = mHandler.obtainMessage(WHAT_COUNT_DOWN_FINISH);
        // mHandler.handleMessage(msg);
        timeThread = new Thread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < mrecordTime; i++) {
                    if(mStopThread)
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
        });
        timeThread.start();

    }

    @Override
    public void onStopCase() {
        mStopThread = true;
        try {
            timeThread.join();
        } catch (Exception e) {
            e.printStackTrace();
        }
        mMicRecordManager.release();
        setCaseResult(mLeftResult && mRightResult && mBothResult);
        mMinLeftResultTV.setText(mLeftResult ? R.string.case_micrecord_left_success:R.string.case_micrecord_left_fail);
        mMinRightResultTV.setText(mRightResult ? R.string.case_micrecord_right_success:R.string.case_micrecord_right_fail);
        mMinBothResultTV.setText(mBothResult ? R.string.case_micrecord_both_success:R.string.case_micrecord_both_fail);
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, lastStreamVolume, 0);// restore StreamVolume after testing
        AudioChannelUtil.setOuputChannels(mContext, false,AudioManagerProxy.AUDIO_NAME_CODEC,AudioManagerProxy.AUDIO_NAME_HDMI);
        File file = new File(mMicRecordManager.mFilePath);
        if (file.exists()) {
            file.delete();
        }
    }

    @Override
    public void reset() {
        super.reset();
        mMinLeftResultTV.setText(R.string.case_micrecord_left_status);
        mMinRightResultTV.setText(R.string.case_micrecord_right_status);
        mMinBothResultTV.setText(R.string.case_micrecord_both_status);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.case_micrecord_left_button_passed:
                mLeftResult = true;
                resetTest(PLAY_RIGHT);
                break;
            case R.id.case_micrecord_left_button_fail:
                mLeftResult = false;
                resetTest(PLAY_RIGHT);
                break;
            case R.id.case_micrecord_right_button_passed:
                mRightResult = true;
                resetTest(PLAY_NORMAL);
                break;
            case R.id.case_micrecord_right_button_fail:
                mRightResult = false;
                resetTest(PLAY_NORMAL);
                break;
            case R.id.case_micrecord_both_button_passed:
                mBothResult = true;
                mMaxViewDialog.dismiss();
                break;
            case R.id.case_micrecord_both_button_fail:
                mBothResult = false;
                mMaxViewDialog.dismiss();
                break;
            case R.id.case_micrecord_button_fail:
                mStopThread = true;
                mMaxViewDialog.dismiss();
                break;
            default:
                break;
        }
    }

}

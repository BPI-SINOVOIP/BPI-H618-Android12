package com.softwinner.dragonbox.testcase;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;

import org.xmlpull.v1.XmlPullParser;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.MicRecordManager;
import com.softwinner.dragonbox.manager.SpdifManager;
import com.softwinner.dragonbox.utils.AudioChannelUtil;
import com.softwinner.dragonbox.platform.AudioManagerProxy;

import android.R.string;
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

public class CaseYSTMicRecord2 extends IBaseCase implements OnClickListener {
    private static final int WHAT_COUNT_DOWN = 1;
    private static final int WHAT_COUNT_DOWN_FINISH = 2;
    private static final String TAG = "DragonBox-CaseYSTMicRecord2";

    public static final int PLAY_LEFT = 1;
    public static final int PLAY_RIGHT = 2;
    public static final int PLAY_NORMAL = 3;
    public static final String AUDIO_CHANNEL = "/sys/devices/soc.0/1c22c00.codec/audio_reg_debug/audio_reg";
    public static final String ADCMIXSC[] = {"LADCMIXSC 0xc: ","RADCMIXSC 0xd: "};
    public static final String PREFIX_ADCMIXSC[] = {"1,2,0x0c,","1,2,0x0d,"};

    private int mrecordTime;
    MicRecordManager mMicRecordManager;
    private AudioManager mAM;
    private int lastStreamVolume;
    private boolean mLeftResult = false;
    private boolean mRightResult = false;
    private boolean mBothResult = false;
    private boolean mStopThread = false;
    private String[] mAdcMixSc;

    Button leftButtonPass;
    Button leftButtonFail;
    Button rightButtonPass;
    Button rightButtonFail;
    Button bothButtonPass;
    Button bothButtonFail;
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
                    resetTest(message.arg1);
                    break;
            }
        }
    };

    public CaseYSTMicRecord2(Context context) {
        super(context, R.string.case_micrecord_name,R.layout.case_yst_micrecord_max2, R.layout.case_yst_micrecord_min2,TYPE_MODE_MANUAL);
        mMicRecordManager = new MicRecordManager(context);

        leftButtonPass = (Button) mMaxView.findViewById(R.id.case_micrecord_leftmic_button_passed);
        leftButtonFail = (Button) mMaxView.findViewById(R.id.case_micrecord_leftmic_button_fail);
        rightButtonPass = (Button) mMaxView.findViewById(R.id.case_micrecord_rightmic_button_passed);
        rightButtonFail = (Button) mMaxView.findViewById(R.id.case_micrecord_rightmic_button_fail);
        bothButtonPass = (Button) mMaxView.findViewById(R.id.case_micrecord_bothmic_button_passed);
        bothButtonFail = (Button) mMaxView.findViewById(R.id.case_micrecord_bothmic_button_fail);
        recordContainer = (ViewGroup) mMaxView.findViewById(R.id.case_micrecord_container2);
        leftContainer = (ViewGroup) mMaxView.findViewById(R.id.case_micrecord_leftmic_container);
        rightContainer = (ViewGroup) mMaxView.findViewById(R.id.case_micrecord_rightmic_container);
        bothContainer = (ViewGroup) mMaxView.findViewById(R.id.case_micrecord_bothmic_container);
        mShowAlertTV = (TextView) mMaxView.findViewById(R.id.case_micrecord_alert2);
        mMinLeftResultTV = (TextView) mMinView.findViewById(R.id.case_micrecord_leftmic_status);
        mMinRightResultTV = (TextView) mMinView.findViewById(R.id.case_micrecord_rightmic_status);
        mMinBothResultTV = (TextView) mMinView.findViewById(R.id.case_micrecord_bothmic_status);

        leftButtonPass.setOnClickListener(this);
        leftButtonFail.setOnClickListener(this);
        rightButtonPass.setOnClickListener(this);
        rightButtonFail.setOnClickListener(this);
        bothButtonPass.setOnClickListener(this);
        bothButtonFail.setOnClickListener(this);

        setShowDialogButton(false);
        mAM = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        mAdcMixSc = getAudioReg(AUDIO_CHANNEL);
    }

    public CaseYSTMicRecord2(Context context, XmlPullParser xmlParser) {
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
                mMaxViewDialog.setTitle(R.string.case_micrecord_leftmic_title);
                mShowAlertTV.setText(R.string.case_micrecord_leftmic_alert);
                try {
                    mMicRecordManager.playRecorder(PLAY_NORMAL);
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
                mMaxViewDialog.setTitle(R.string.case_micrecord_rightmic_title);
                mShowAlertTV.setText(R.string.case_micrecord_rightmic_alert);
                try {
                    mMicRecordManager.playRecorder(PLAY_NORMAL);
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
                mMaxViewDialog.setTitle(R.string.case_micrecord_bothmic_title);
                mShowAlertTV.setText(R.string.case_micrecord_bothmic_alert);
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

    public String[] getAudioReg(String path) {
        String[] adcMixSc = new String[2];
        try {
            BufferedReader br = new BufferedReader(new InputStreamReader(new FileInputStream(path)));
            String line = null;
            while ((line = br.readLine())!=null) {
                if(line.contains(ADCMIXSC[0])) {
                    adcMixSc[0] = line.replaceAll(ADCMIXSC[0], PREFIX_ADCMIXSC[0]);
                    Log.w(TAG,"Ready to write "+adcMixSc[0]+" to LADCMIXSC");
                }
                if(line.contains(ADCMIXSC[1])) {
                    adcMixSc[1] = line.replaceAll(ADCMIXSC[1], PREFIX_ADCMIXSC[1]);
                    Log.w(TAG,"Ready to write "+adcMixSc[1]+" to RADCMIXSC");
                    break;
                }
            }
            if(br != null) {
                br.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        return adcMixSc;
    }

    public void startMicRecord(final int channel, boolean isStopCase) {
        if (!isStopCase) {
            recordContainer.setVisibility(View.VISIBLE);
            leftContainer.setVisibility(View.GONE);
            rightContainer.setVisibility(View.GONE);
            bothContainer.setVisibility(View.GONE);
        }

        FileWriter audioChannel = null;
        try {
            audioChannel = new FileWriter(AUDIO_CHANNEL);
            String audioWriteChannelC = "0x60";
            String audioWriteChannelD = "0x64";
            switch (channel) {
                case PLAY_LEFT:
                    mMaxViewDialog.setTitle(R.string.case_micrecord_leftmic_title);
                    audioWriteChannelC = "1,2,0x0c,0x20";
                    audioWriteChannelD = "1,2,0x0d,0x20";
                    break;
                case PLAY_RIGHT:
                    mMaxViewDialog.setTitle(R.string.case_micrecord_rightmic_title);
                    audioWriteChannelC = "1,2,0x0c,0x40";
                    audioWriteChannelD = "1,2,0x0d,0x40";
                    break;
                case PLAY_NORMAL:
                    mMaxViewDialog.setTitle(R.string.case_micrecord_bothmic_title);
                    if(mAdcMixSc != null) {
                        audioWriteChannelC = mAdcMixSc[0];
                        audioWriteChannelD = mAdcMixSc[1];
                    }
                    break;
                default:
                    break;
            }
            audioChannel.write(audioWriteChannelC);
            audioChannel.flush();
            audioChannel.write(audioWriteChannelD);
            audioChannel.flush();
        } catch (Exception e) {
            // TODO: handle exception
        } finally {
            if (audioChannel != null) {
                try {
                    audioChannel.close();
                } catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }
        if (!isStopCase) {
            mMicRecordManager.startRecorder();
            // Message msg = mHandler.obtainMessage(WHAT_COUNT_DOWN_FINISH);
            // mHandler.handleMessage(msg);
            timeThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    for (int i = 0; i < mrecordTime; i++) {
                        if (mStopThread)
                            return;
                        final int time = mrecordTime - i;
                        // Message msg =
                        // mHandler.obtainMessage(WHAT_COUNT_DOWN);
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
                    msg.arg1 = channel;
                    mHandler.sendMessage(msg);
                }
            });
            timeThread.start();
        }
    }

    @Override
    public void onStartCase() {
        mStopThread = false;
        lastStreamVolume = mAM.getStreamVolume(AudioManager.STREAM_MUSIC);
        int maxVolume = mAM.getStreamMaxVolume(AudioManager.STREAM_MUSIC);// set the max Volume before testing
        mAM.setStreamVolume(AudioManager.STREAM_MUSIC, maxVolume, 0);
        AudioChannelUtil.setOuputChannels(mContext, true,
                AudioManagerProxy.AUDIO_NAME_CODEC, AudioManagerProxy.AUDIO_NAME_HDMI);
        startMicRecord(PLAY_LEFT,false);
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
        mMinLeftResultTV.setText(mLeftResult ? R.string.case_micrecord_leftmic_success:R.string.case_micrecord_leftmic_fail);
        mMinRightResultTV.setText(mRightResult ? R.string.case_micrecord_rightmic_success:R.string.case_micrecord_rightmic_fail);
        mMinBothResultTV.setText(mBothResult ? R.string.case_micrecord_bothmic_success:R.string.case_micrecord_bothmic_fail);
        mAM.setStreamVolume(AudioManagerProxy.STREAM_MUSIC, lastStreamVolume, 0);// restore StreamVolume after testing
        AudioChannelUtil.setOuputChannels(mContext, false,AudioManagerProxy.AUDIO_NAME_CODEC, AudioManagerProxy.AUDIO_NAME_HDMI);
        File file = new File(mMicRecordManager.mFilePath);
        if (file.exists()) {
            file.delete();
        }
        startMicRecord(PLAY_NORMAL,true);
    }

    @Override
    public void reset() {
        super.reset();
        mMinLeftResultTV.setText(R.string.case_micrecord_leftmic_status);
        mMinRightResultTV.setText(R.string.case_micrecord_rightmic_status);
        mMinBothResultTV.setText(R.string.case_micrecord_bothmic_status);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.case_micrecord_leftmic_button_passed:
                mLeftResult = true;
                startMicRecord(PLAY_RIGHT,false);
                break;
            case R.id.case_micrecord_leftmic_button_fail:
                mLeftResult = false;
                startMicRecord(PLAY_RIGHT,false);
                break;
            case R.id.case_micrecord_rightmic_button_passed:
                mRightResult = true;
                startMicRecord(PLAY_NORMAL,false);
                break;
            case R.id.case_micrecord_rightmic_button_fail:
                mRightResult = false;
                startMicRecord(PLAY_NORMAL,false);
                break;
            case R.id.case_micrecord_bothmic_button_passed:
                mBothResult = true;
                mMaxViewDialog.dismiss();
                break;
            case R.id.case_micrecord_bothmic_button_fail:
                mBothResult = false;
                mMaxViewDialog.dismiss();
                break;
            case R.id.case_micrecord_button_fail2:
                mStopThread = true;
                mMaxViewDialog.dismiss();
                break;
            default:
                break;
        }
    }

}

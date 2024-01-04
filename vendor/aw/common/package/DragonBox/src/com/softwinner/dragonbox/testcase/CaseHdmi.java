package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.manager.HdmiManager;
import com.softwinner.dragonbox.utils.AudioChannelUtil;

import android.media.AudioManager;
import android.app.Dialog;
import android.content.Context;
import android.os.Handler;
import android.widget.TextView;
import android.util.Log;
import com.softwinner.dragonbox.platform.AudioManagerProxy;

public class CaseHdmi extends IBaseCase {

    public static final String TAG = "DragonBox-CaseHdmi";
    private TextView mMaxStatusTV;
    private TextView mMinStatusTV;
    private TextView mMinSoundStatusTV;

    HdmiManager hm;

    private static final int WHAT_CHECK_HDMI_STATUS = 1;
    private Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
                case WHAT_CHECK_HDMI_STATUS:
                    int resId;
                    boolean isConn = hm.isHDMIStatusConn();
                    if (isConn) {
                        resId = R.string.case_hdmi_status_success_text;
                    } else {
                        resId = R.string.case_hdmi_status_fail_text;
                        mHandler.sendEmptyMessageDelayed(WHAT_CHECK_HDMI_STATUS,1000);
                    }
                    Log.d(TAG,"isConn:"+isConn);
                    setDialogPositiveButtonEnable(isConn);
                    mMaxStatusTV.setText(resId);
                    mMinStatusTV.setText(resId);
                    break;
            }
        };
    };

    public CaseHdmi(Context context) {
        super(context, R.string.case_hdmi_name, R.layout.case_hdmi_max,
                R.layout.case_hdmi_min, TYPE_MODE_MANUAL);
        hm = new HdmiManager(mContext);
        mMaxStatusTV = (TextView) mMaxView.findViewById(R.id.case_hdmi_status);
        mMinStatusTV = (TextView) mMinView.findViewById(R.id.case_hdmi_status_text);
        mMinSoundStatusTV = (TextView) mMinView.findViewById(R.id.case_hdmi_sound_status_text);
    }

    public CaseHdmi(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseHdmi");
        hm.changeToHDMI();//when supporting hdmi and cvbs output at the same time ,should delete this row.
        AudioChannelUtil.setOuputChannels(mContext, true ,AudioManagerProxy.AUDIO_NAME_HDMI);
        hm.playMusic();
        mHandler.sendEmptyMessage(WHAT_CHECK_HDMI_STATUS);
    }

    @Override
    public void onStopCase() {
        Log.w(TAG,"CaseHdmi test over,test result is "+getCaseResult());
        hm.stopMusic();
        AudioChannelUtil.setOuputChannels(mContext, false,AudioManagerProxy.AUDIO_NAME_HDMI);
        mMinSoundStatusTV.setText(getCaseResult() ? R.string.case_hdmi_sound_status_success_text
                : R.string.case_hdmi_sound_status_fail_text);
    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[18], getCaseResult()+"");//hdmi
    }

    @Override
    public void reset() {
        super.reset();
        mMinStatusTV.setText(R.string.case_hdmi_status_text);
        mMinSoundStatusTV.setText(R.string.case_hdmi_sound_status_text);
    }
}

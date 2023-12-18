package com.softwinner.dragonbox.testcase;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.xmlpull.v1.XmlPullParser;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.HdmiManager;
import com.softwinner.dragonbox.utils.AudioChannelUtil;

import android.media.AudioManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.util.TypedValue;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.util.Log;
import com.softwinner.dragonbox.platform.AudioManagerProxy;

public class CaseHdmiFool extends IBaseCase implements OnClickListener {

    public static final String TAG = "DragonBox-CaseHdmiFool";
    private static final int BUTTON_NUMBER = 5;
    private static final int FONT_SIZE = 28;

    private TextView mMaxStatusTV;
    private TextView mMinStatusTV;
    private TextView mMinSoundStatusTV;
    ViewGroup buttonContainer;
    Random random = new Random();
    private int musicNum;//播放hdmi音频文件的随机数，用于随机选择hdmi音频
    private List<Button> lstButton = new ArrayList<Button>();

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
                    for(int i=0;i<BUTTON_NUMBER-1;i++) {
                        Button button = lstButton.get(i);
                        button.setEnabled(isConn);
                    }
                    mMaxStatusTV.setText(resId);
                    mMinStatusTV.setText(resId);
                    break;
            }
        };
    };

    public CaseHdmiFool(Context context) {
        super(context, R.string.case_hdmi_name, R.layout.case_hdmi_fool_max,
                R.layout.case_hdmi_min, TYPE_MODE_MANUAL);
        hm = new HdmiManager(mContext);
        mMaxStatusTV = (TextView) mMaxView.findViewById(R.id.case_hdmi_status);
        mMinStatusTV = (TextView) mMinView.findViewById(R.id.case_hdmi_status_text);
        mMinSoundStatusTV = (TextView) mMinView.findViewById(R.id.case_hdmi_sound_status_text);
        setShowDialogButton(false);
        initButtonContainer();
    }

    public CaseHdmiFool(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    public void initButtonContainer() {
        buttonContainer = (ViewGroup) mMaxView
                .findViewById(R.id.case_hdmi_button_container);
        for(int i=0;i<BUTTON_NUMBER-1;i++) {
            Button button = new Button(mContext);
            button.setText(""+(i+1));
            button.setEnabled(false);
            button.setSingleLine(true);
            button.setPadding(1, 1, 1, 1);
            button.setTextSize(TypedValue.COMPLEX_UNIT_PX, FONT_SIZE);
            button.setOnClickListener(this);
            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(0, LayoutParams.MATCH_PARENT);
            layoutParams.weight =1;
            buttonContainer.addView(button, i,layoutParams);
            lstButton.add(button);
        }
        Button button = new Button(mContext);
        button.setText("失败");
        button.setEnabled(true);
        button.setSingleLine(true);
        button.setPadding(1, 1, 1, 1);
        button.setTextSize(TypedValue.COMPLEX_UNIT_PX, FONT_SIZE);
        button.setOnClickListener(this);
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(0, LayoutParams.MATCH_PARENT);
        layoutParams.weight =1;
        buttonContainer.addView(button, BUTTON_NUMBER-1,layoutParams);
        lstButton.add(button);
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseHdmiFool");
        hm.changeToHDMI();//when supporting hdmi and cvbs output at the same time ,should delete this row.
        AudioChannelUtil.setOuputChannels(mContext, true ,AudioManagerProxy.AUDIO_NAME_HDMI);
        musicNum = random.nextInt(BUTTON_NUMBER-1)+1;
        hm.playMusic(musicNum);
        mHandler.sendEmptyMessage(WHAT_CHECK_HDMI_STATUS);
    }

    @Override
    public void onStopCase() {
        hm.stopMusic();
        AudioChannelUtil.setOuputChannels(mContext, false,AudioManagerProxy.AUDIO_NAME_HDMI);
        mMinSoundStatusTV.setText(getCaseResult() ? R.string.case_hdmi_sound_status_success_text
                : R.string.case_hdmi_sound_status_fail_text);
    }

    @Override
    public void reset() {
        super.reset();
        mMinStatusTV.setText(R.string.case_hdmi_status_text);
        mMinSoundStatusTV.setText(R.string.case_hdmi_sound_status_text);
    }

    @Override
    public void onClick(View v) {
        Button button;
        if (v instanceof Button)
            button = (Button) v;
        else
            return;
        final int index = lstButton.indexOf(button);
        if ((index+1) == musicNum) {
            setCaseResult(true);
            mMaxViewDialog.dismiss();
            Log.w(TAG,"CaseHdmiFool test over ,test resutl is success");
        } else {//对测试失败项进行确认
            AlertDialog alertDialog = new AlertDialog.Builder(mContext).setMessage(R.string.confirm_test_fail)
                    .setPositiveButton(R.string.confirm_yes, new DialogInterface.OnClickListener() {

                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            setCaseResult(false);
                            mMaxViewDialog.dismiss();
                            Log.w(TAG,"CaseHdmiFool test over ,test resutl is fail,should choose "+musicNum+",but choose "+(index+1)+"  in fact");
                        }

                    }).setNegativeButton(R.string.confirm_no, new DialogInterface.OnClickListener() {

                        @Override
                        public void onClick(DialogInterface dialog, int which) {

                        }
                    }).setCancelable(false).create();
            alertDialog.show();
            alertDialog.getButton(DialogInterface.BUTTON_NEGATIVE).requestFocus();
        }
    }
}

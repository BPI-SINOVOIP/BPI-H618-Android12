package com.softwinner.dragonbox.testcase;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.lang.Thread;

import org.xmlpull.v1.XmlPullParser;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.util.TypedValue;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.media.AudioManager;

import com.softwinner.dragonbox.utils.AudioChannelUtil;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.CvbsManager;
import com.softwinner.dragonbox.testcase.IBaseCase.onResultChangeListener;
import com.softwinner.dragonbox.platform.AudioManagerProxy;

public class CaseCvbsFool extends IBaseCase implements OnClickListener,
        onResultChangeListener {
    public static final String TAG = "DragonBox-CaseCvbsFool";
    private static final int BUTTON_NUMBER = 5;
    private static final int FONT_SIZE = 28;//按钮字体大小
    private List<Button> lstLeftButton = new ArrayList<Button>();
    private List<Button> lstRightButton = new ArrayList<Button>();
    ViewGroup leftContainer;
    ViewGroup rightContainer;
    TextView mShowAlertTV;

    TextView mMinLeftResultTV;
    TextView mMinRightResultTV;
    TextView mMinStatusTV;

    private TextView mMaxCvbsStatusTV;
    private TextView mMinCvbsStatusTV;

    CvbsManager cvbsManager;
    private int leftMusic;
    private int rightMusic;
    Random random = new Random();

    private static final int WHAT_CHECK_CVBS_STATUS = 1;
    private Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
                case WHAT_CHECK_CVBS_STATUS:
                    int resId;
                    boolean isConn = cvbsManager.isCvbsStatusConn();
                    if (isConn) {
                        resId = R.string.case_cvbs_status_success_text;
                    } else {
                        resId = R.string.case_cvbs_status_fail_text;
                        mHandler.sendEmptyMessageDelayed(WHAT_CHECK_CVBS_STATUS,
                                1000);
                    }
                    for(int i=0;i<BUTTON_NUMBER-1;i++) {
                        Button leftButton = lstLeftButton.get(i);
                        Button rightButton = lstRightButton.get(i);
                        leftButton.setEnabled(isConn);
                        rightButton.setEnabled(isConn);
                    }
                    mMaxCvbsStatusTV.setText(resId);
                    mMinCvbsStatusTV.setText(resId);
                    break;
            }
        };
    };

    public CaseCvbsFool(Context context) {
        super(context, R.string.case_cvbs_name, R.layout.case_cvbs_fool_max,
                R.layout.case_cvbs_min, TYPE_MODE_MANUAL);
        cvbsManager = new CvbsManager(mContext);

        rightContainer = (ViewGroup) mMaxView
                .findViewById(R.id.case_cvbs_right_container);
        initLeftContainer();
        initRightContainer();
        mShowAlertTV = (TextView) mMaxView.findViewById(R.id.case_cvbs_alert);

        mMinLeftResultTV = (TextView) mMinView
                .findViewById(R.id.case_cvbs_left);
        mMinRightResultTV = (TextView) mMinView
                .findViewById(R.id.case_cvbs_right);

        mMaxCvbsStatusTV = (TextView) mMaxView
                .findViewById(R.id.case_cvbs_status);
        mMinCvbsStatusTV = (TextView) mMinView
                .findViewById(R.id.case_cvbs_status_text);

        setShowDialogButton(false);
    }

    public void initLeftContainer() {
        leftContainer = (ViewGroup) mMaxView
                .findViewById(R.id.case_cvbs_left_container);
        for(int i=0;i<BUTTON_NUMBER-1;i++) {
            Button button = new Button(mContext);
            button.setText("L"+(i+1));
            button.setEnabled(false);
            button.setSingleLine(true);
            button.setPadding(1, 1, 1, 1);
            button.setTextSize(TypedValue.COMPLEX_UNIT_PX, FONT_SIZE);
            button.setOnClickListener(this);
            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(0, LayoutParams.MATCH_PARENT);
            layoutParams.weight =1;
            leftContainer.addView(button, i,layoutParams);
            lstLeftButton.add(button);
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
        leftContainer.addView(button, BUTTON_NUMBER-1,layoutParams);
        lstLeftButton.add(button);
    }

    public void initRightContainer() {
        rightContainer = (ViewGroup) mMaxView
                .findViewById(R.id.case_cvbs_right_container);
        for(int i=0;i<BUTTON_NUMBER-1;i++) {
            Button button = new Button(mContext);
            button.setText("R"+(i+1));
            button.setEnabled(false);
            button.setSingleLine(true);
            button.setPadding(1, 1, 1, 1);
            button.setTextSize(TypedValue.COMPLEX_UNIT_PX, FONT_SIZE);
            button.setOnClickListener(this);
            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(0, LayoutParams.MATCH_PARENT);
            layoutParams.weight =1;
            rightContainer.addView(button, i,layoutParams);
            lstRightButton.add(button);
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
        rightContainer.addView(button, BUTTON_NUMBER-1,layoutParams);
        lstRightButton.add(button);
    }

    public CaseCvbsFool(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    /**
     * change show item,left for left sound
     *
     * @param isLeft
     *            ture for show left ,false for show right
     */
    private void resetTest(boolean isLeft) {
        if (isLeft) {
            leftContainer.setVisibility(View.VISIBLE);
            rightContainer.setVisibility(View.GONE);

            mMaxViewDialog.setTitle(R.string.case_cvbs_left_title);
            mShowAlertTV.setText(R.string.case_cvbs_left_alert);
            leftMusic = random.nextInt(BUTTON_NUMBER-1)+1;
            cvbsManager.playLeft(leftMusic);
        } else {
            leftContainer.setVisibility(View.GONE);
            rightContainer.setVisibility(View.VISIBLE);

            mMaxViewDialog.setTitle(R.string.case_cvbs_right_title);
            mShowAlertTV.setText(R.string.case_cvbs_right_alert);
            rightMusic = random.nextInt(BUTTON_NUMBER-1)+1;
            cvbsManager.playRight(rightMusic);
            lstRightButton.get(0).requestFocus();
        }
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseCvbsFool");
        cvbsManager.changeToCvbs();//when supporting hdmi and cvbs output at the same time ,should delete this row.
        AudioChannelUtil.setOuputChannels(mContext, true,AudioManagerProxy.AUDIO_NAME_CODEC);
        mHandler.sendEmptyMessage(WHAT_CHECK_CVBS_STATUS);
        resetTest(true);
    }

    @Override
    public void onStopCase() {
        cvbsManager.changeToHdmi();
        cvbsManager.stopPlaying();
        setCaseResult(cvbsManager.getResult());
        Log.w(TAG,"CaseCvbsFool test over, test result is "+getCaseResult());
        AudioChannelUtil.setOuputChannels(mContext, false,AudioManagerProxy.AUDIO_NAME_CODEC);
        mMinLeftResultTV
                .setText(cvbsManager.isLeftPlaySuccess() ? R.string.case_cvbs_left_success
                        : R.string.case_cvbs_left_fail);
        mMinRightResultTV
                .setText(cvbsManager.isRightPlaySuccess() ? R.string.case_cvbs_right_success
                        : R.string.case_cvbs_right_fail);
    }

    @Override
    public void reset() {
        super.reset();
        mMinLeftResultTV.setText(R.string.case_cvbs_left);
        mMinCvbsStatusTV.setText(R.string.case_cvbs_status_text);
        mMinRightResultTV.setText(R.string.case_cvbs_right);
    }

    @Override
    public void onClick(View view) {
        Button button;
        if (view instanceof Button)
            button = (Button) view;
        else
            return;
        if (leftContainer.isShown()) {
            final int index = lstLeftButton.indexOf(button);
            if ((index+1) == leftMusic) {
                cvbsManager.setLeftPlaySuccess(true);
                Log.w(TAG,"CaseCvbsFool left channel test result success");
                try{
                    Thread.sleep(300);//休眠300ms，防止连击
                }catch(Exception e){
                }
                resetTest(false);
            } else {
                //对测试失败项进行确认
                AlertDialog alertDialog = new AlertDialog.Builder(mContext).setMessage(R.string.confirm_test_fail)
                        .setPositiveButton(R.string.confirm_yes, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                Log.w(TAG,"CaseCvbsFool left channel test result fail,should choose "+leftMusic+",but choose "+(index+1)+" in fact");
                                resetTest(false);
                                cvbsManager.setLeftPlaySuccess(false);
                            }
                        }).setNegativeButton(R.string.confirm_no, new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {

                            }
                        }).setCancelable(false).create();
                alertDialog.show();
                alertDialog.getButton(DialogInterface.BUTTON_NEGATIVE).requestFocus();
            }
        } else {
            final int index = lstRightButton.indexOf(button);
            if ((index+1) == rightMusic) {
                Log.w(TAG,"CaseCvbsFool right channel test result success");
                cvbsManager.setRightPlaySuccess(true);
                mMaxViewDialog.dismiss();
            } else {
                //对测试失败项进行确认
                AlertDialog alertDialog = new AlertDialog.Builder(mContext).setMessage(R.string.confirm_test_fail)
                        .setPositiveButton(R.string.confirm_yes, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                Log.w(TAG,"CaseCvbsFool right channel test result fail,should choose "+rightMusic+",but choose "+(index+1)+" in fact");
                                cvbsManager.setRightPlaySuccess(false);
                                mMaxViewDialog.dismiss();
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

    @Override
    public void onResultChange(IBaseCase baseCase, boolean caseResult) {
        mMinLeftResultTV
                .setText(cvbsManager.isLeftPlaySuccess() ? R.string.case_cvbs_left_success
                        : R.string.case_cvbs_left_fail);
        mMinRightResultTV
                .setText(cvbsManager.isRightPlaySuccess() ? R.string.case_cvbs_right_success
                        : R.string.case_cvbs_right_fail);
    }

}

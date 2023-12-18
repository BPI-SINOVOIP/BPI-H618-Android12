package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.DialogInterface.OnShowListener;
import android.media.AudioManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.SpdifManager;
import com.softwinner.dragonbox.testcase.IBaseCase.onResultChangeListener;
import com.softwinner.dragonbox.utils.AudioChannelUtil;
import com.softwinner.dragonbox.platform.AudioManagerProxy;

public class CaseSpdif extends IBaseCase implements OnClickListener,
        onResultChangeListener {

    Button leftButtonPass;
    Button leftButtonFail;
    Button rightButtonPass;
    Button rightButtonFail;
    ViewGroup leftContainer;
    ViewGroup rightContainer;
    TextView mShowAlertTV;

    TextView mMinLeftResultTV;
    TextView mMinRightResultTV;

    SpdifManager spdifManager;

    public CaseSpdif(Context context) {
        super(context, R.string.case_spdif_name, R.layout.case_spdif_max,
                R.layout.case_spdif_min, TYPE_MODE_MANUAL);
        spdifManager = new SpdifManager(context);

        leftButtonPass = (Button) mMaxView
                .findViewById(R.id.case_spdif_left_button_passed);
        leftButtonFail = (Button) mMaxView
                .findViewById(R.id.case_spdif_left_button_fail);
        rightButtonPass = (Button) mMaxView
                .findViewById(R.id.case_spdif_right_button_passed);
        rightButtonFail = (Button) mMaxView
                .findViewById(R.id.case_spdif_right_button_fail);
        leftContainer = (ViewGroup) mMaxView
                .findViewById(R.id.case_spdif_left_container);
        rightContainer = (ViewGroup) mMaxView
                .findViewById(R.id.case_spdif_right_container);

        leftButtonPass.setOnClickListener(this);
        leftButtonFail.setOnClickListener(this);
        leftButtonPass
                .setNextFocusForwardId(R.id.case_spdif_right_button_passed);
        leftButtonFail
                .setNextFocusForwardId(R.id.case_spdif_right_button_passed);
        rightButtonPass.setOnClickListener(this);
        rightButtonFail.setOnClickListener(this);
        mShowAlertTV = (TextView) mMaxView.findViewById(R.id.case_spdif_alert);

        mMinLeftResultTV = (TextView) mMinView
                .findViewById(R.id.case_spdif_left_status);
        mMinRightResultTV = (TextView) mMinView
                .findViewById(R.id.case_spdif_right_status);
        setShowDialogButton(false);
        addRusultChangeListener(this);
    }

    public CaseSpdif(Context context, XmlPullParser xmlParser) {
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

            mMaxViewDialog.setTitle(R.string.case_spdif_left_title);
            mShowAlertTV.setText(R.string.case_spdif_left_alert);
            spdifManager.playLeft();
        } else {
            leftContainer.setVisibility(View.GONE);
            rightContainer.setVisibility(View.VISIBLE);

            mMaxViewDialog.setTitle(R.string.case_spdif_right_title);
            mShowAlertTV.setText(R.string.case_spdif_right_alert);
            spdifManager.playRight();
            rightButtonPass.requestFocus();
        }
    }

    @Override
    public void onStartCase() {
        AudioChannelUtil.setOuputChannels(mContext,true,AudioManagerProxy.AUDIO_NAME_SPDIF);
        resetTest(true);
    }

    @Override
    public void onStopCase() {
        spdifManager.stopPlaying();
        AudioChannelUtil.setOuputChannels(mContext,false,AudioManagerProxy.AUDIO_NAME_SPDIF);
        setCaseResult(spdifManager.getResult());
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.case_spdif_left_button_passed:
                resetTest(false);
                spdifManager.setLeftPlaySuccess(true);
                break;
            case R.id.case_spdif_left_button_fail:
                resetTest(false);
                spdifManager.setLeftPlaySuccess(false);
                break;
            case R.id.case_spdif_right_button_passed:
                spdifManager.setRightPlaySuccess(true);
                mMaxViewDialog.dismiss();
                break;
            case R.id.case_spdif_right_button_fail:
                spdifManager.setRightPlaySuccess(false);
                mMaxViewDialog.dismiss();
                break;

            default:
                break;
        }

    }

    @Override
    public void onResultChange(IBaseCase baseCase, boolean caseResult) {
        mMinLeftResultTV
                .setText(spdifManager.isLeftPlaySuccess() ? R.string.case_spdif_left_success
                        : R.string.case_spdif_left_fail);
        mMinRightResultTV
                .setText(spdifManager.isRightPlaySuccess() ? R.string.case_spdif_right_success
                        : R.string.case_spdif_right_fail);
    }

    @Override
    public void reset() {
        super.reset();
        mMinLeftResultTV.setText(R.string.case_spdif_left_status);
        mMinRightResultTV.setText(R.string.case_spdif_right_status);
    }

}

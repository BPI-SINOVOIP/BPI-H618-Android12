package com.softwinner.dragonbox.testcase;

import java.io.IOException;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.LedManager;
import com.softwinner.dragonbox.manager.MixPlayManager;

public class CaseMixPlay extends IBaseCase {
    MixPlayManager mMixPlayManager;

    TextView mMinMixPlayStatus;

    public CaseMixPlay(Context context) {
        super(context, R.string.case_mixplay_name, R.layout.case_mixplay_max,
                R.layout.case_mixplay_min, TYPE_MODE_MANUAL);
        mMixPlayManager = new MixPlayManager(context);
        mMinMixPlayStatus = (TextView) mMinView.findViewById(R.id.case_mixplay_status);
    }

    public CaseMixPlay(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    @Override
    public void onStartCase() {

        try {
            mMixPlayManager.startMixPlay();
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IllegalStateException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    @Override
    public void onStopCase() {
        mMixPlayManager.stopMixPlay();
        mMinMixPlayStatus
                .setText(getCaseResult() ? R.string.case_mixplay_status_success_text
                        : R.string.case_mixplay_status_fail_text);
    }

    @Override
    public void reset() {
        super.reset();
        mMinMixPlayStatus.setText(R.string.case_mixplay_status_text);

    }

}

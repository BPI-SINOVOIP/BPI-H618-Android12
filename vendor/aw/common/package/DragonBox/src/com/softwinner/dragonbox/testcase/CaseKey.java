package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;

import android.R.xml;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.widget.TextView;

public class CaseKey extends IBaseCase {
    private final static String TAG="CaseKey";
    private final static int MSG_KEY_TEST_END = 0;
    TextView mTvMaxKeyInfo;
    TextView mTvMinKeyStatus;
    int mTestKeyValue=0;
    int mExitKeyValue=0;
    Context mContext;
    Handler mKeyHandler = new Handler(Looper.myLooper()) {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
                case MSG_KEY_TEST_END:
                    mMaxViewDialog.dismiss();
                    break;

                default:
                    break;
            }
        };
    };
    public CaseKey(Context context) {
        super(context, R.string.case_key_name, R.layout.case_key_max, R.layout.case_key_min, TYPE_MODE_MANUAL);
        mContext = context;
        mTvMaxKeyInfo = (TextView) mMaxView.findViewById(R.id.case_key_info);
        mTvMinKeyStatus = (TextView) mMinView.findViewById(R.id.case_key_status);
        setShowDialogButton(false);
    }

    public CaseKey(Context context, XmlPullParser xmlParser) {
        this(context);
        String testKeyValue = xmlParser.getAttributeValue(null,"testKey");
        String exitKeyValue = xmlParser.getAttributeValue(null,"exitKey");
        mTestKeyValue = KeyEvent.keyCodeFromString(testKeyValue);
        mExitKeyValue = KeyEvent.keyCodeFromString(exitKeyValue);
        Log.d(TAG, "the test key value is "+KeyEvent.keyCodeToString(mTestKeyValue));
        Log.d(TAG, "the exit key value is "+KeyEvent.keyCodeToString(mExitKeyValue));
    }

    @Override
    public void onStartCase() {
        // TODO Auto-generated method stub
        mTvMaxKeyInfo.setText(mContext.getString(R.string.case_key_info, KeyEvent.keyCodeToString(mTestKeyValue),KeyEvent.keyCodeToString(mExitKeyValue)));
        mTvMaxKeyInfo.setFocusable(true);
        mTvMaxKeyInfo.requestFocus();
        setDialogPositiveButtonEnable(false);
        mTvMaxKeyInfo.setOnKeyListener(new OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                Log.d(TAG, "key down code is " + keyCode + ", event action is "+ event.getAction());
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    if (keyCode == mTestKeyValue) {
                        setCaseResult(true);
                        mTvMaxKeyInfo.setText(mContext.getString(R.string.case_key_detect_success,KeyEvent.keyCodeToString(keyCode)));
                        mKeyHandler.sendEmptyMessageDelayed(MSG_KEY_TEST_END,850);
                    } else if (keyCode == mExitKeyValue) {
                        setCaseResult(false);
                        mTvMaxKeyInfo.setText(mContext.getString(R.string.case_key_detect_exit));
                        mKeyHandler.sendEmptyMessageDelayed(MSG_KEY_TEST_END,850);
                    } else {
                        mTvMaxKeyInfo.setText(mContext.getString(R.string.case_key_detect_error, KeyEvent.keyCodeToString(keyCode),
                                KeyEvent.keyCodeToString(mTestKeyValue), KeyEvent.keyCodeToString(mExitKeyValue)));
                    }
                }
                return true;
            }
        });
    }

    @Override
    public void onStopCase() {
        // TODO Auto-generated method stub
        mTvMinKeyStatus
                .setText(getCaseResult() ? R.string.case_key_status_success_text
                        : R.string.case_key_status_fail_text);
    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[17], getCaseResult()+"");//ir
    }

    @Override
    public void reset() {
        super.reset();
    }


}

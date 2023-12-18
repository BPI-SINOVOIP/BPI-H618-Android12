package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.TextView;
import com.softwinner.Gpio;
import android.app.Activity;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.manager.LedManager;

public class CaseResetKey extends IBaseCase {
    public static final String TAG = "DragonBox-CaseResetKey";
    private static final char RESET_KEY_GROUP = 'H';
    private static final int RESET_KEY_NUM = 9;
    private static final int WHAT_HANDLE_READGPIO = 0;
    private static final int READ_GPIO_DELAY = 50;//ms
    private boolean mTerminate = false;
    private int ret = 1;
    private TextView mResultText;
    private Context mContext;
    private int mResetKeyNum = RESET_KEY_NUM;
    private Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
                case WHAT_HANDLE_READGPIO:
                    ret = Gpio.readGpio(RESET_KEY_GROUP, mResetKeyNum);
                    if(ret == 0) {
                        Log.w(TAG,"test over, result is true");
                        setCaseResult(true);
                        mMaxViewDialog.dismiss();
                        return;
                    }
                    Log.w(TAG,"send msg to read gpio agagin!");
                    sendEmptyMessageDelayed(WHAT_HANDLE_READGPIO, READ_GPIO_DELAY);
                    break;
                 default:
                    break;
            }
        };
    };

    public CaseResetKey(Context context) {
        super(context , R.string.case_reset_name,
                R.layout.case_reset_max, R.layout.case_reset_min,
                TYPE_MODE_MANUAL);
        mResultText = (TextView)mMinView.findViewById(R.id.case_reset_status);
        mContext = context;
    }

    public CaseResetKey(Context context, XmlPullParser xmlParser) {
        this(context);
        String sResetKeyNum = xmlParser.getAttributeValue(null, "key");
        if (sResetKeyNum != null) {
            mResetKeyNum = Integer.parseInt(sResetKeyNum);
        }
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseResetKey!");
        mHandler.sendEmptyMessage(WHAT_HANDLE_READGPIO);
        setDialogPositiveButtonEnable(false);
    }

    @Override
    public void onStopCase() {
        Log.w(TAG,"onStopCase, test result is "+getCaseResult());
        if(mHandler.hasMessages(WHAT_HANDLE_READGPIO)) {
            mHandler.removeMessages(WHAT_HANDLE_READGPIO);
            Log.w(TAG,"remove Message WHAT_HANDLE_READGPIO");
        }
        mResultText.setText(getCaseResult()?R.string.case_reset_status_success_text:R.string.case_reset_status_fail_text);
    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[21], getCaseResult()+"");//reset
    }

    @Override
    public void reset() {
        super.reset();
        mResultText.setText(R.string.case_reset_status_text);
    }

}

package com.android.phone;

import static com.android.phone.TimeConsumingPreferenceActivity.EXCEPTION_ERROR;
import static com.android.phone.TimeConsumingPreferenceActivity.RESPONSE_ERROR;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.preference.SwitchPreference;
import android.telephony.TelephonyManager;
import android.util.AttributeSet;
import android.util.Log;

import com.android.internal.telephony.Phone;

import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

public class CallWaitingSwitchPreference extends SwitchPreference {
    private static final String LOG_TAG = "CallWaitingSwitchPreference";
    private final boolean DBG = (PhoneGlobals.DBG_LEVEL >= 2);

    private final MyHandler mHandler = new MyHandler();
    private Phone mPhone;
    private TimeConsumingPreferenceListener mTcpListener;
    private Executor mExecutor;
    private TelephonyManager mTelephonyManager;
    private boolean mIsDuringUpdateProcess = false;
    private int mUpdateStatus = TelephonyManager.CALL_WAITING_STATUS_UNKNOWN_ERROR;
    private int mQueryStatus = TelephonyManager.CALL_WAITING_STATUS_UNKNOWN_ERROR;

    public CallWaitingSwitchPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public CallWaitingSwitchPreference(Context context, AttributeSet attrs) {
        this(context, attrs, com.android.internal.R.attr.switchPreferenceStyle);
    }

    public CallWaitingSwitchPreference(Context context) {
        this(context, null);
    }

    /* package */ void init(
            TimeConsumingPreferenceListener listener, boolean skipReading, Phone phone) {
        mPhone = phone;
        mTcpListener = listener;
        mExecutor = Executors.newSingleThreadExecutor();
        mTelephonyManager = getContext().getSystemService(
                TelephonyManager.class).createForSubscriptionId(phone.getSubId());

        if (!skipReading) {
            Log.d(LOG_TAG, "init getCallWaitingStatus");
            mTelephonyManager.getCallWaitingStatus(mExecutor, this::queryStatusCallBack);
            if (mTcpListener != null) {
                mTcpListener.onStarted(this, true);
            }
        }
    }

    private void queryStatusCallBack(int result) {
        Log.d(LOG_TAG, "queryStatusCallBack: CW state " + result);
        mQueryStatus = result;
        mHandler.sendMessage(mHandler.obtainMessage(MyHandler.MESSAGE_UPDATE_CALL_WAITING));
    }

    private void updateStatusCallBack(int result) {
        Log.d(LOG_TAG, "updateStatusCallBack: CW state " + result + ", and re get");
        mUpdateStatus = result;
        mTelephonyManager.getCallWaitingStatus(mExecutor, this::queryStatusCallBack);
    }

    @Override
    protected void onClick() {
        super.onClick();
        mTelephonyManager.setCallWaitingEnabled(isChecked(), mExecutor, this::updateStatusCallBack);
        if (mTcpListener != null) {
            mIsDuringUpdateProcess = true;
            mTcpListener.onStarted(this, false);
        }
    }

    private class MyHandler extends Handler {
        static final int MESSAGE_UPDATE_CALL_WAITING = 0;

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_UPDATE_CALL_WAITING:
                    updateUi();
                    break;
            }
        }

        private void updateUi() {
            if (mTcpListener != null) {
                if (mIsDuringUpdateProcess) {
                    mTcpListener.onFinished(CallWaitingSwitchPreference.this, false);
                } else {
                    mTcpListener.onFinished(CallWaitingSwitchPreference.this, true);
                }
            }

            if (mIsDuringUpdateProcess && (
                    mUpdateStatus == TelephonyManager.CALL_WAITING_STATUS_NOT_SUPPORTED
                            || mUpdateStatus
                            == TelephonyManager.CALL_WAITING_STATUS_UNKNOWN_ERROR)) {
                Log.d(LOG_TAG, "handleSetCallWaitingResponse: Exception");
                if (mTcpListener != null) {
                    mTcpListener.onError(CallWaitingSwitchPreference.this, EXCEPTION_ERROR);
                }
            } else if (mQueryStatus == TelephonyManager.CALL_WAITING_STATUS_NOT_SUPPORTED
                    || mQueryStatus == TelephonyManager.CALL_WAITING_STATUS_UNKNOWN_ERROR) {
                Log.d(LOG_TAG, "handleGetCallWaitingResponse: Exception");
                if (mTcpListener != null) {
                    mTcpListener.onError(CallWaitingSwitchPreference.this, RESPONSE_ERROR);
                }
            } else {
                if (mQueryStatus == TelephonyManager.CALL_WAITING_STATUS_ENABLED) {
                    setChecked(true);
                } else {
                    setChecked(false);
                }
            }
            mIsDuringUpdateProcess = false;
        }
    }
}

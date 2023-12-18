package com.softwinner.dragonbox.testcase;

import java.io.File;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.os.Build;
import android.widget.TextView;
import android.util.Log;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.jni.ReadPrivateJNI;
import com.softwinner.dragonbox.manager.LedManager;
import com.softwinner.dragonbox.utils.Utils;
import com.softwinner.SystemMix;
import com.softwinner.dragonbox.DragonBoxApplication;

public class CaseTMSN extends IBaseCase {
    public static final String TAG = "DragonBox-CaseTMSN";//ali 天猫 烧号测试
    public static final String ROTPK_PATH = "/sys/class/sunxi_info/key_info";
    public static final String ROTPK_FAIL = "00000000";
    public static final String WIDEVINE_KEY = "widevine";
    public static final String HDCPKEY_KEY = "hdcp22";
    public static final String WIFI_MAC_KEY = "wifi_mac";
    public static final String MAC_KEY = "mac";
    public static final String SN_KEY = "snum";
    private ReadPrivateJNI mReadPrivateJNI = DragonBoxApplication.getReadPrivateJNI();
    TextView mMinSNStatus;
    TextView mMaxSNStatus;
    Context mContext;
    public static String rotpk;
    public static String hdcp;
    public static String widevine;

    public CaseTMSN(Context context) {
        super(context, R.string.case_tmsn_name, R.layout.case_tmsn_max,
            R.layout.case_tmsn_min, TYPE_MODE_AUTO);
        mMinSNStatus = (TextView) mMinView.findViewById(R.id.case_tmsn_min_info);
        mMaxSNStatus = (TextView) mMaxView.findViewById(R.id.case_tmsn_max_info);
    }

    public CaseTMSN(Context context, XmlPullParser xmlParser) {
        this(context);
        mContext = context;
    }

    private String getBurnResult(String test){
        if(test == null || test.equals("") || test.equals("null")) {
            return "F";
        }
        return "P";
    }

    @Override
    public void onStartCase() {
        rotpk = getBurnResult(getRotpk(ROTPK_PATH));
        hdcp = getBurnResult(getValueFromSST(HDCPKEY_KEY));
        widevine = getBurnResult(getValueFromSST(WIDEVINE_KEY));

        boolean widevineKey = getKeyFromSST(WIDEVINE_KEY);
        boolean hdcpkeyKey = getKeyFromSST(HDCPKEY_KEY);
        boolean wifiMacKey = getKeyFromSST(WIFI_MAC_KEY);
        boolean macKey = getKeyFromSST(MAC_KEY);
        String sn = getValueFromSST(SN_KEY);
        boolean snKey;
        if(sn==null) {
            snKey = false;
            DragonBoxMain.getTestResultforSQLInfo().setPrimaryValue("");
        } else {
            snKey = true;
            DragonBoxMain.getTestResultforSQLInfo().setPrimaryValue(sn);
        }
        Log.w(TAG,"onStartCase, rotpk = "+rotpk+"\nwidevineKey exist: "+widevineKey+"\nhdcpkeyKey exist: "
          +hdcpkeyKey+"\nwifimac exist: "+wifiMacKey+"\nmac exist: "+macKey+"\nsnKey exist: "+snKey+" snKey = "+sn);
        Log.w(TAG,"rotpk length is "+rotpk.length());
        boolean testResult=false;
        if(!rotpk.startsWith(ROTPK_FAIL)) {
            if(widevineKey&&hdcpkeyKey/*&&wifiMacKey*/&&snKey&&macKey) {
                testResult = true;
            }
        } else {
            Log.w(TAG,"rotpk exist: false");
        }
        String burned = mContext.getString(R.string.case_tmsn_burned);
        String unburned = mContext.getString(R.string.case_tmsn_unburned);
        rotpk = rotpk.startsWith(ROTPK_FAIL)?"F":"P";
        mMinSNStatus.setText(mContext.getString(R.string.case_tmsn_inof, rotpk.startsWith(ROTPK_FAIL)?unburned:burned,
                widevineKey?burned:unburned,
                hdcpkeyKey?burned:unburned,
                wifiMacKey?burned:unburned,
                macKey?burned:unburned,
                snKey?burned:unburned));
        mMaxSNStatus.setText(mContext.getString(R.string.case_tmsn_inof, rotpk.startsWith(ROTPK_FAIL)?unburned:burned,
                widevineKey?burned:unburned,
                hdcpkeyKey?burned:unburned,
                wifiMacKey?burned:unburned,
                macKey?burned:unburned,
                snKey?burned:unburned));
        stopCase();
        setCaseResult(testResult);
        Log.w(TAG,"CaseTMSN test over ,test result is "+getCaseResult());
    }

    private String getRotpk(String path) {
        int error = SystemMix.writeFile(path,"rotpk");
        if(error==0) {
            Log.w(TAG,"Rotpk write failed, return fail");
            return ROTPK_FAIL;
        }else {
            Log.w(TAG,"Rotpk write success, return bytes: "+error);
            String rotpkSN = SystemMix.readFile(path);
            if(rotpkSN==null||rotpkSN.equals(ROTPK_FAIL)) {
                return ROTPK_FAIL;
            }else {
                return rotpkSN;
            }
        }
    }

    private boolean getKeyFromSST(String key) {
        String value = mReadPrivateJNI.nativeGetParameter(key);
        Log.w(TAG, key+": "+value);
        if(value == null) {
            return false;
        }else {
            return true;
        }
    }

    private String getValueFromSST(String key) {
        String value = mReadPrivateJNI.nativeGetParameter(key);
        Log.w(TAG, key+": "+value);
        return value;
    }

    @Override
    public void onStopCase() {

    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[6], ""+!(getRotpk(ROTPK_PATH).startsWith(ROTPK_FAIL)));//rotpk
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[7], ""+getKeyFromSST(HDCPKEY_KEY));//hdcp
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[8], ""+getKeyFromSST(WIDEVINE_KEY));//widevine
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[9], ""+getValueFromSST(MAC_KEY));//mac
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[10], ""+getValueFromSST(WIFI_MAC_KEY));//wifi_mac
    };

    @Override
    public void reset() {
        super.reset();
        mMinSNStatus.setText(R.string.case_rid_status_text);
    }

}

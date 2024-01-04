package com.softwinner.dragonbox.platform;

import com.softwinner.dragonbox.utils.RemoteDBUtils;
import com.softwinner.dragonbox.utils.Utils;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Handler;
import android.util.Log;
import android.widget.TextView;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.DragonBoxApplication;
import com.softwinner.dragonbox.config.ConfigManager;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.jni.ReadPrivateJNI;
import com.softwinner.dragonbox.testcase.CaseSql;
import com.softwinner.dragonbox.testcase.CaseWifi;

public class PhaseCallBack implements IPhaseCallback {
    private static final String TAG = "DragonBox-H3-AndroidN-PhaseCallBack";
    private static final String ACTION = "com.lhxk.voice_recognition_end";
    private static final String DRAGON_STAGE_SMT = "persist.sys.dragon_stage_smt";
    private static final String DRAGON_STAGE_ASSY = "persist.sys.dragon_stage_assy";
    private static final int WHAT_MSG_UPDATE_SQL_OVER = 0;
    private static final int RESET = 1<<0;
    private static final int VERSION = 1<<1;
    private static final int AGING = 1<<2;
    private static final int CPU_TEMPERATURE = 1<<3;
    private static final int IR = 1<<4;
    private static final int HDMI = 1<<5;
    private static final int CVBS = 1<<6;
    private static final int USB = 1<<7;
    private static final int WIFI = 1<<8;
    private boolean mIsAssy = false;//标志位:SMT和assy
    boolean mUpdateResult = false;
    private Map<String, Integer> mCaseToCodeHashMap = new HashMap<String, Integer>(){
        {
            for(int i=0;i<ConfigManager.UPLOAD_KEY.length;i++) {
                put(ConfigManager.UPLOAD_KEY[i][1], 1<<i);
            }
        }
    };

    public void showAlertDialogToFinish(String msg,int size, Context context) {
        AlertDialog alertDialog = new AlertDialog.Builder(context)
                .setMessage(msg).create();
        alertDialog.setCancelable(false);
        alertDialog.show();
        try {
            Field mAlert = AlertDialog.class.getDeclaredField("mAlert");
            mAlert.setAccessible(true);
            Object mAlertController = mAlert.get(alertDialog);
            //通过反射修改message字体大小和颜色
            Field mMessage = mAlertController.getClass().getDeclaredField("mMessageView");
            mMessage.setAccessible(true);
            TextView mMessageView = (TextView) mMessage.get(mAlertController);
            mMessageView.setTextSize(size);
            //mMessageView.setTextColor(Color.GREEN);
        } catch (IllegalAccessException e1) {
            e1.printStackTrace();
        } catch (NoSuchFieldException e2) {
            e2.printStackTrace();
        }
    }

    private ReadPrivateJNI mReadPrivateJNI = DragonBoxApplication.getReadPrivateJNI();
    public void allResultTestPassCallback(Context context) {
        Log.w(TAG,"trigger allResultTestPassCallback!");
        if(mIsAssy) {
            TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
            testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[22], "testok");//result
            testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[23], com.ut.device.UTDevice.getUtdid(context));//utdid
            RemoteDBUtils remoteDBUtils = new RemoteDBUtils(CaseSql.getSQLConfig());
            mUpdateResult = remoteDBUtils.updateData(DragonBoxMain.getTestResultforSQLInfo());
            Log.w(TAG,"update sql result: "+mUpdateResult);
            if (mUpdateResult) {
                Utils.setPropertySecure("persist.sys.usb.config", "mtp");
                mReadPrivateJNI.nativeSetParameter("assy", "pass");
            }
        }else {//smt over, goto aging next boot
            Utils.setPropertySecure(DragonBoxMain.PROPERTY_DRAGONAGING_NEXTBOOT,"true");
            Utils.setPropertySecure(DragonBoxMain.PROPERTY_DRAGONAGING_TIME,"0");
            Utils.setPropertySecure(DragonBoxMain.PROPERTY_DRAGONAGING_SET_TIME, CaseWifi.agingTime+"");
        }
    }
    public void startAppCallback(Context context) {
        Log.w(TAG,"trigger startAppCallback");
        Log.w(TAG,com.ut.device.UTDevice.getUtdid(context)+"");
    }
    public void allTestOverCallback(Context context,String...params) {
        Log.w(TAG,"trigger allTestOverCallback");
        Utils.setPropertySecure(DragonBoxMain.PROPERTY_SMT_DRAGONBOX_TESTCASE,params[0]);
        Utils.setPropertySecure(DragonBoxMain.PROPERTY_SMT_DRAGONBOX_TESTRESULT,params[1]);

        int testFailFlag = 0;
        String[] testCaseArr = params[0].split(",");
        for(int i=0;i<testCaseArr.length;i++) {
            if(testCaseArr[i].equals("Ag")) {
                mIsAssy = true;
            }
        }
        if(mIsAssy) {
            Log.w(TAG,"this is Assy!");
            Utils.setPropertySecure(DRAGON_STAGE_ASSY, "test_over");
        }else {
            Log.w(TAG,"this is SMT!");
            Utils.setPropertySecure(DRAGON_STAGE_SMT, "test_over");
            return;
        }
        String[] testCaseResultArr = params[1].split(",");
        for(int i=0;i<testCaseResultArr.length;i++) {
            if(testCaseResultArr[i]!=null&&testCaseResultArr[i].startsWith("F")) {
                Integer flag = mCaseToCodeHashMap.get(testCaseArr[i]);
                if(flag!=null) {
                    testFailFlag |= flag;
                }
            }
        }
        if(testFailFlag!=0) {
            Log.w(TAG,"testFailFlag is "+testFailFlag);
            mReadPrivateJNI.nativeSetParameter("assy", testFailFlag+"");
        }
    }
    public void allResultTestPassCallbackAfterResultDialog(Context context) {
        if (mIsAssy) {
            if (mUpdateResult) {
                showAlertDialogToFinish("上传数据库成功", 40, context);
            } else {
                showAlertDialogToFinish("上传数据库     失败   !!", 40, context);
            }
        }
        Log.w(TAG,"trigger allResultTestPassCallbackAfterResultDialog");
    }
}

package com.softwinner.dragonbox.platform;

import android.content.Context;
import com.softwinner.dragonbox.entity.UploadBean;
import com.softwinner.dragonbox.testcase.IBaseCase;
import com.softwinner.dragonbox.testcase.CaseWifi;
import java.util.List;
import android.os.Build;

public interface IPhaseCallback {
    void allResultTestPassCallback(Context context);
    void allResultTestPassCallbackAfterResultDialog(Context context);
    void allTestOverCallback(Context context,String...params);
    void startAppCallback(Context context);

    default UploadDockingSystem getUploadDockingSystem(){
        return new UploadDockingSystem();
    }

    default UploadBean getUploadBean(List<IBaseCase> mAllCases){
        if(mAllCases == null) return null;
        UploadBean ret = new UploadBean();
        String mTestItems = "";
        String mTestValues = "";
        String mTestResult = "PASS";
        for(IBaseCase cases : mAllCases) {//all pass
            boolean result = cases.getCaseResult();
            if(!result) {
                mTestResult = "FAIL";
            }
            mTestItems += cases.mTestName + ",";
            if(cases.mTestName.contains("Wifi")){
                mTestValues += result?"P" + CaseWifi.wifiRssi + "," : "F" + CaseWifi.wifiRssi + ",";
            }else if(cases.mTestName.contains("Ver")){
                String display = Build.DISPLAY;
                String[] split = display.split("\\.");
                String version = split[split.length-1];
                mTestValues += result? "P" + version + "," :"F" + version + ",";
            } else{
                if(result){
                    mTestValues += "P" + ",";
                }else{
                    mTestValues += "F" + ",";
                }
            }
        }
        if(mTestItems.length()>0){
            mTestItems = mTestItems.substring(0, mTestItems.length()-1);
        }
        if(mTestValues.length()>0){
            mTestValues = mTestValues.substring(0,mTestValues.length()-1);
        }
        ret.uploadTestKey = mTestItems;
        ret.uploadTestValue = mTestValues;
        ret.uploadTestResult = mTestResult;
        return ret;
    }
}

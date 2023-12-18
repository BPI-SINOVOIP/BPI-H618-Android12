package com.softwinner.dragonbox.platform;

import com.softwinner.dragonbox.utils.NetUtil;
import com.softwinner.dragonbox.entity.MES_RequestBean;
import com.softwinner.dragonbox.entity.NetReceivedResult;
import com.softwinner.dragonbox.platform.DockingSystemCategory;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;

public class MES_DockingSystem extends UploadDockingSystem {

    private static final String TAG = "DragonBox-MES_DockingSystem";

    public NetReceivedResult CheckSSN_NEW(String url,String sSSN,String sStation) {
        MES_RequestBean requestBean = MES_RequestBean.getQuestBeanFromCaseWifi();
        requestBean.step = "FIRST";
        requestBean.passStationNum = sSSN;
        NetReceivedResult netReceivedResult = NetUtil.getURLContentByGet(url,requestBean);
        return netReceivedResult;
    }

    public NetReceivedResult SfcTestResult_Upload(String url,String sStation,String sSSN,String sTestResult,
            String sTestTime,String sTestItem,String sTestValue,String sPcName) {
        MES_RequestBean requestBean = MES_RequestBean.getQuestBeanFromCaseWifi();
        requestBean.step = "SECOND";
        requestBean.passStationNum = sSSN;
        requestBean.scpContent = getScpContent(sTestItem,sTestValue,sSSN);
        if(sTestResult!=null&&sTestResult.equals("FAIL")){
            requestBean.defectiveProduct = "ERROR0001,,1";
        }
        NetReceivedResult netReceivedResult = NetUtil.getURLContentByGet(url,requestBean);
        return netReceivedResult;
    }

    public NetReceivedResult SaveSSN_NEW(String url,String sSSN,String sStation,String sResult,String sFailCode,String sUser) {
        String[] arrSParams = {sSSN,sStation,sResult,sFailCode,sUser};
        MES_RequestBean requestBean = MES_RequestBean.getQuestBeanFromCaseWifi();
        requestBean.step = "THIRD";
        requestBean.passStationNum = sSSN;
        NetReceivedResult netReceivedResult = NetUtil.getURLContentByPost(url, SAVESSN, arrSParams,requestBean);
        return netReceivedResult;
    }

	public DockingSystemCategory getCurDockingSystemCategory(){
		return DockingSystemCategory.MES;
	}

    private String getScpContent(String sTestItem,String sTestValue,String sSSN){
        if(sTestItem!=null && sTestValue!=null){
            sTestItem = sTestItem.toLowerCase();
            sTestValue = sTestValue.toUpperCase();
            StringBuilder sb = new StringBuilder();
            String[] items = sTestItem.split(",");
            String[] values = sTestValue.split(",");
            if(items.length != values.length){
                Log.e(TAG,"getScpContent format error!");
            }
            Log.d(TAG,"items size:"+items.length+"  ,values size:"+values.length);
            ArrayList<String> itemsList = new ArrayList<String>(Arrays.asList(items));
            itemsList.add("snum");
            ArrayList<String> valuesList = new ArrayList<String>(Arrays.asList(values));
            valuesList.add(sSSN);
            Log.d(TAG,"itemsList size:"+itemsList.size()+"  ,valuesList size:"+valuesList.size());
            int size = Math.min(itemsList.size(),valuesList.size());
            for(int i = 0; i < size; i++){
                if(i != size-1) {
                    sb.append(itemsList.get(i)).append("%0B").append(valuesList.get(i)).append("%09");
                } else {
                    sb.append(itemsList.get(i)).append("%0B").append(valuesList.get(i));
                }
            }
            String ret = sb.toString();
            Log.d(TAG,"--------scpContent:"+ret);
            return ret;
        }
        return null;
    }

}

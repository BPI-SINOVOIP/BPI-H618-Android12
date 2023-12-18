package com.softwinner.dragonbox.platform;

import com.softwinner.dragonbox.entity.NetReceivedResult;
import com.softwinner.dragonbox.utils.NetUtil;
import com.softwinner.dragonbox.platform.DockingSystemCategory;

public class UploadDockingSystem {

    protected static final String CHECKSSN="CheckSSN_NEW";
    protected static final String SAVESSN="SaveSSN_NEW";
    protected static final String SFCTESTRESULT_UPLOAD="SfcTestResult_Upload";

    public NetReceivedResult CheckSSN_NEW(String url,String sSSN,String sStation) {
        String[] arrSParams = {sSSN , sStation};
        NetReceivedResult netReceivedResult = NetUtil.getURLContentByPost(url, CHECKSSN, arrSParams);
        return netReceivedResult;
    }

    public NetReceivedResult SfcTestResult_Upload(String url,String sStation,
                                                  String sSSN,String sTestResult,String sTestTime,
                                                  String sTestItem,String sTestValue,String sPcName) {
        String[] arrSParams = {sStation,sSSN,sTestResult,sTestTime,sTestItem,sTestValue,sPcName};
        NetReceivedResult netReceivedResult = NetUtil.getURLContentByPost(url, SFCTESTRESULT_UPLOAD, arrSParams);
        return netReceivedResult;
    }

    public NetReceivedResult SaveSSN_NEW(String url,String sSSN,String sStation,
                                         String sResult,String sFailCode,String sUser) {
        String[] arrSParams = {sSSN,sStation,sResult,sFailCode,sUser};
        NetReceivedResult netReceivedResult = NetUtil.getURLContentByPost(url, SAVESSN, arrSParams);
        return netReceivedResult;
    }

    public DockingSystemCategory getCurDockingSystemCategory(){
        return DockingSystemCategory.SFC;
    }


}
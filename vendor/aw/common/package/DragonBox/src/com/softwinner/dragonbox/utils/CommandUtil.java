package com.softwinner.dragonbox.utils;

import com.softwinner.dragonbox.entity.NetReceivedResult;;

public class CommandUtil{
    public static final String sUrl = "http://219.133.2.168:81/SFCWEBSERVICETEST1/SFCWebService.asmx";
    public static final String CHECKSSN="CheckSSN_NEW";
    public static final String SAVESSN="SaveSSN_NEW";
    public static final String SFCTESTRESULT_UPLOAD="SfcTestResult_Upload";
    public static NetReceivedResult CheckSSN_NEW(String url,String sSSN,String sStation) {
        String[] arrSParams = {sSSN , sStation};
        NetReceivedResult netReceivedResult = NetUtil.getURLContentByPost(url, CHECKSSN, arrSParams);
        return netReceivedResult;
    }
    public static NetReceivedResult SaveSSN_NEW(String url,String sSSN,String sStation,String sResult,String sFailCode,String sUser) {
        String[] arrSParams = {sSSN,sStation,sResult,sFailCode,sUser};
        NetReceivedResult netReceivedResult = NetUtil.getURLContentByPost(url, SAVESSN, arrSParams);
        return netReceivedResult;
    }
    public static NetReceivedResult SfcTestResult_Upload(String url,String sStation,String sSSN,String sTestResult,String sTestTime,
                                                         String sTestItem,String sTestValue,String sPcName) {
        String[] arrSParams = {sStation,sSSN,sTestResult,sTestTime,sTestItem,sTestValue,sPcName};
        NetReceivedResult netReceivedResult = NetUtil.getURLContentByPost(url, SFCTESTRESULT_UPLOAD, arrSParams);
        return netReceivedResult;
    }
}

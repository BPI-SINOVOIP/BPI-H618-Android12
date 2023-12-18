package com.softwinner.dragonbox.utils;
import android.util.Log;

public class SOAPUtil {
    public static String TAG = "DragonBox-SOAPUtil";
    private static String getXmlHead(){
        String str="<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
        str +="<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">";
        str +="<soap:Body>";
        return str;
    }
    private static String getXmlTail() {
        String str="</soap:Body>";
        str +="</soap:Envelope>";
        return str;
    }
    private static String getXmlBody(String sCommand,String[] arrSParams)throws IllegalArgumentException {
        String str="";
        if(sCommand.equals(CommandUtil.CHECKSSN)){
            if(arrSParams.length != 2)
                throw new IllegalArgumentException("the number of "+sCommand+" params should be 2");
            str +="<CheckSSN_NEW xmlns=\"http://tempuri.org/\">";
            str +="<strSN>"+arrSParams[0]+"</strSN>";
            str +="<station>"+arrSParams[1]+"</station>";
            str +="</CheckSSN_NEW>";
        }else if(sCommand.equals(CommandUtil.SAVESSN)){
            if(arrSParams.length!=5)
                throw new IllegalArgumentException("the number of "+sCommand+" params should be 5");
            str +="<SaveSSN_NEW xmlns=\"http://tempuri.org/\">";
            str +="<strSSN>"+arrSParams[0]+"</strSSN>";
            str +="<strEventPoint>"+arrSParams[1]+"</strEventPoint>";
            str +="<strIspass>"+arrSParams[2]+"</strIspass>";
            str +="<strFailcode>"+arrSParams[3]+"</strFailcode>";
            str +="<strScanner>"+arrSParams[4]+"</strScanner>";
            str +="</SaveSSN_NEW>";
        }else if(sCommand.equals(CommandUtil.SFCTESTRESULT_UPLOAD)){
            if(arrSParams.length!=7)
                throw new IllegalArgumentException("the number of "+sCommand+" params should be 7");
            str +="<SfcTestResult_Upload xmlns=\"http://tempuri.org/\">";
            str +="<strEventPoint>"+arrSParams[0]+"</strEventPoint>";
            str +="<strSSN>"+arrSParams[1]+"</strSSN>";
            str +="<testresult>"+arrSParams[2]+"</testresult>";
            str +="<testtime>"+arrSParams[3]+"</testtime>";
            str +="<testitem>"+arrSParams[4]+"</testitem>";
            str +="<testvalue>"+arrSParams[5]+"</testvalue>";
            str +="<strScanner>"+arrSParams[6]+"</strScanner>";
            str +="</SfcTestResult_Upload>";
        }
        return str;
    }
    public static String getXml(String sCommand,String[] arrSParams) {
        String str="";
        str += getXmlHead();
        str += getXmlBody(sCommand, arrSParams);
        str += getXmlTail();
        Log.e(TAG,"getXml = "+str);
        return str;
    }
}

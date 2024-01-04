package com.softwinner.protocol.net;

public final class ServerUrl
{

    private static final int DEBUG_SERVER = 1001;
    private static final int PUBLIC_SERVER = 1002;

    private static final String DEBUG_URL_ROOT = "http://awszp2.awbase.com:8002/";

    private static final String URL_OTA = "ota";

    private static final String URL_REPORT = "action";

    /**
     * 多域名ota服务器
     */
    private static final String PUBLIC_OTA_URL_A = "http://ota.api.pada.cc";
    private static final String PUBLIC_OTA_URL_B = "http://ota.api.eaglenet.cn"; //备用

    private static final String PUBLIC_REPORT_URL = "http://action.api.pada.cc";

    /**
     * 修改此处，切换正式环境，与测试环境
     */
    private static int CONNECT_TO = PUBLIC_SERVER;

    //    private static int CONNECT_TO = DEBUG_SERVER;

    public static String [] getReportUrl()
    {
	if( CONNECT_TO == PUBLIC_SERVER )
	{
	    return new String [] { PUBLIC_REPORT_URL };
	}
	else
	{
	    return new String [] { DEBUG_URL_ROOT + URL_REPORT };
	}
    }

    public static String [] getOtaUpdateUrl()
    {
	if( CONNECT_TO == PUBLIC_SERVER )
	{
	    return otaUpdateUrlArray( );
	}
	else
	{
	    return new String [] { DEBUG_URL_ROOT + URL_OTA };
	}
    }

    /**
     * PUBLIC_OTA_URL_B , PUBLIC_OTA_URL_A分别为域名，检查跟新
     * @return 域名数组
     */
    private static String [] otaUpdateUrlArray()
    {
	String [] urlArray = new String [] { PUBLIC_OTA_URL_A };
	return urlArray;
    }
}

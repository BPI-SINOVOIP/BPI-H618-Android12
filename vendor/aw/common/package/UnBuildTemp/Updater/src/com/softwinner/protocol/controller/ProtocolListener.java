package com.softwinner.protocol.controller;

public interface ProtocolListener
{
    public static final class ACTION
    {

	// update
	public static String ACTION_UPDATE = "ReqUpdate";
	public static String ACTION_UPDATE_RESPONSE = "RspUpdate";
    }

    public static final class ERROR
    {
	public static int ERROR_ACTION_MISMATCH = 0x1000;
	public static int ERROR_BAD_PACKET = 0x1001;
	public static int ERROR_ACTION_FAIL = 0x1002;
	public static int ERROR_CONNECT_TIME_OUT = 104;
	public static int ERROR_CONNECT_FAIL = 101;
	public static int NO_ERROR = 0x0000;
	public static int PACK_INFORMATION_MODIFYED = 105;
	/**
	 * 可以继续下载PACK   106
	 */
	public static int CONTINUE_DOWNLOAD_PACK = 106;
	/**
	 * 下载地址已经改变     	  107
	 */
	public static int DOWNLOAD_URL_MODIFYED = 107;
	/**
	 * 没有找到可用的包	108
	 */
	public static int NOT_FIND_AVIABLE_PACK = 108;
    }

    public interface AbstractNetListener
    {
	public void onNetError( int errCode , String errorMsg );
    }

    public interface ReqUpdateListener extends AbstractNetListener
    {

	/**
	 * 
	 * @param statusCode
	 *            检查更新错误
	 * @param errorMsg
	 *            错误信息
	 */
	public void onReqFailed( int statusCode , String errorMsg );

	public void onRequpdateSucceed( OTA.ReqUpdate updateInfo );
    }

}

package com.softwinner.protocol.controller;

import java.text.SimpleDateFormat;
import java.util.Date;

import android.text.TextUtils;

import com.softwinner.protocol.controller.Packet.MaskCode;
import com.softwinner.protocol.controller.ProtocolListener.ERROR;
import com.softwinner.protocol.controller.Reported.ReportedInfo;
import com.softwinner.protocol.net.ServerUrl;
import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;
import com.lidroid.xutils.util.LogUtils;

/**
 * OTA上报请求处理
 * @author Nurmuhammad 
 *
 */
public class ReportOta extends AbstractNetController
{
    public static final int REPORT_DOWNLOAD_ACTION = 30001;
    public static final int REPORT_UPDATE_ACTION = 30002;
    public static final int REPORT_STATE_START = 1;
    public static final int REPORT_STATE_ERROR = 2;
    public static final int REPORT_STATE_SUCCESS = 3;
    int mReportAction; //统计操作Id1 更新完毕或下载完毕
    int mReportStateAction; //统计操作Id2 成功或失败
    String mNewVersionName; //新系统版本号
    String mNewVersion; //新系统内部版本号
    String mPacketID; //包编号
    String mPacketType; //包类型（1=整包，2=增量包）
    String mErrorMessage = null; //失败信息。	当mReportStateAction为2时填入失败原因
    private SimpleDateFormat mDateFormat = new SimpleDateFormat( "yyyyMMddHHmmss" );//设置日期格式

    public ReportOta( int action , int action2 , String newVername , String newVersion , String packetID , String packetType , String errorMessage )
    {
	this.mReportAction = action;
	this.mReportStateAction = action2;
	this.mNewVersionName = newVername;
	this.mNewVersion = newVersion;
	this.mPacketID = packetID;
	this.mPacketType = packetType;
	this.mErrorMessage = errorMessage;
    }

    @Override
    protected ByteString getRequestBody()
    {
	Reported.ReportedInfo.Builder builder = Reported.ReportedInfo.newBuilder( );
	builder.setStatActId( mReportAction );
	builder.setStatActId2( mReportStateAction );
	//yyyyMMddHHmmss
	builder.setActionTime( mDateFormat.format( new Date( ) ) );
	builder.setExt1( mNewVersionName );
	builder.setExt2( mNewVersion );
	builder.setExt3( mPacketID );
	builder.setExt4( mPacketType );
	// 如果结果为失败且有错误信息，上报错误信息
	if( mErrorMessage != null && !TextUtils.isEmpty( mErrorMessage ) )
	{
	    builder.setExt5( mErrorMessage );
	}
	ReportedInfo ri = builder.build( );
	LogUtils.e( "ActId=" + ri.getStatActId( ) + "ActId2" + ri.getStatActId2( ) + "Ext1=" + ri.getExt1( ) + "Ext2="
		+ ri.getExt2( ) + "Ext3=" + ri.getExt3( ) + "Ext4=" + ri.getExt4( ) + "Ext5=" + ri.getExt5( ) );
	Reported.ReqReported.Builder reqbuilder = Reported.ReqReported.newBuilder( );
	reqbuilder.addReportedInfo( ri );
	return reqbuilder.build( ).toByteString( );
    }

    @Override
    protected int getRequestMask()
    {
	// TODO Auto-generated method stub
	return MaskCode.DEFAULT_VALUE;
    }

    @Override
    protected String getRequestAction()
    {
	// TODO Auto-generated method stub
	return ACTION.ACTION_REPORT_OTA;
    }

    @Override
    protected String getResponseAction()
    {
	// TODO Auto-generated method stub
	return ACTION.ACTION_REPORT_OTA_RESPONSE;
    }

    @Override
    protected void handleResponseBody( ByteString byteString )
    {
	// TODO Auto-generated method stub
	try
	{
	    Reported.RspReported resBody = Reported.RspReported.parseFrom( byteString );
	    int resCode = resBody.getRescode( );
	    String resMsg = resBody.getResmsg( );
	    LogUtils.e( "resMsg" + resMsg + ",resCode:" + resCode );
	}
	catch ( InvalidProtocolBufferException e )
	{
	    handleResponseError( ERROR.ERROR_BAD_PACKET , e.getMessage( ) );
	    e.printStackTrace( );
	}
    }

    @Override
    protected void handleResponseError( int errCode , String strErr )
    {
	LogUtils.e( "errCode=" + errCode + "errorMessage==" + strErr );

    }

    @Override
    protected String [] getServerUrl()
    {
	return ServerUrl.getReportUrl( );
    }

}

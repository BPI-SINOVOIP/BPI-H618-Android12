package com.softwinner.protocol.controller;

import java.util.concurrent.atomic.AtomicInteger;

import com.softwinner.protocol.controller.Packet.MaskCode;
import com.softwinner.protocol.controller.Packet.RspPacket;
import com.softwinner.protocol.controller.ProtocolListener.ERROR;
import com.softwinner.protocol.net.AsyncHttpPostSession;
import com.softwinner.protocol.net.AsyncHttpSession;
import com.softwinner.protocol.net.HttpSessionCallBack;
import com.softwinner.update.App;
import com.softwinner.update.utils.DeviceUtil;
import com.softwinner.update.utils.Utils;
import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;

/**
 * 网络请求处理
 * @author Spring su
 * @Modifyded Nurmuhammad 
 *
 */

public abstract class AbstractNetController
{

    private final String TAG = "AbstractNetController";

    private AsyncHttpPostSession mSession;
    private static AtomicInteger mRequestSeq = new AtomicInteger( 0 );

    public static final class ACTION
    {
	public static String ACTION_REPORT_OTA = "ReqReported";
	public static String ACTION_REPORT_OTA_RESPONSE = "RspReported";
	public static String ACTION_OTA = "ReqUpdate";
	public static String ACTION_OTA_RESPONSE = "RspUpdate";
    }

    /**
     * 子类实现，返回请求的业务包体
     * 
     * @return ByteString
     */
    protected abstract ByteString getRequestBody();

    /**
     * 子类实现，返回请求的Mask设置
     * 
     * @return
     */
    protected abstract int getRequestMask();

    /**
     * 子类实现，返回请求的action设置
     * 
     * @return
     */
    protected abstract String getRequestAction();

    /**
     * 子类实现，返回回复包的action设置
     * 
     * @return
     */
    protected abstract String getResponseAction();

    /**
     * 子类实现，处理返回的业务数据与业务本身的逻辑错误
     * 
     * @param byteString
     */
    protected abstract void handleResponseBody( ByteString byteString );

    /**
     * 子类实现，处理返回的网络与包数据相关的错误
     * 
     * @param errCode
     * @param strErr
     */
    protected abstract void handleResponseError( int errCode , String strErr );

    /**
     * 
     * @return 对应请求的服务器地址
     */
    protected abstract String [] getServerUrl();

    protected AbstractNetController()
    {}

    public AsyncHttpSession getSession()
    {
	return mSession;
    }

    public void doRequest()
    {
	mSession = new AsyncHttpPostSession( getServerUrl( ) );
	mSession.registerCallBack( mHttpSessionCallBack );
	mSession.doPost( makePacket( ) );
    }

    private byte [] makePacket()
    {
	// 包头
	Packet.ReqPacket.Builder builder = Packet.ReqPacket.newBuilder( );
	builder.setMask( getRequestMask( ) );
	builder.setUdi( getDeviceInfo( ) );
	builder.addAction( getRequestAction( ) );
	builder.addParams( preHandleRequestBody( getRequestBody( ) ) );
	builder.setReqNo( getRequestSequence( ) );
	builder.setClientId( 5 ); // ota 固定5
	builder.setClientVer( Utils.VersionName( App.getAppContext( ) ) );
	return builder.build( ).toByteArray( );
    }

    /*
     * 预处理请求包
     */
    private ByteString preHandleRequestBody( ByteString body )
    {
	ByteString handleBody = body;

	if( ( getRequestMask( ) | 0xFF ) == MaskCode.PARAMS_RSA_VALUE )
	{
	    // TODO encrypt body
	    handleBody = body;
	}

	if( ( getRequestMask( ) | 0xFF ) == MaskCode.PARAMS_GZIP_VALUE )
	{
	    // TODO zip compress
	    handleBody = body;
	}
	return handleBody;
    }

    /*
     * 预处理返回包
     */
    private ByteString preHandleResponseBody( ByteString body , int mask )
    {
	ByteString handleBody = body;

	if( ( mask | 0xFF ) == MaskCode.PARAMS_RSA_VALUE )
	{

	}

	if( ( mask | 0xFF ) == MaskCode.PARAMS_GZIP_VALUE )
	{

	}

	return handleBody;
    }

    private String getDeviceInfo()
    {
	return DeviceUtil.getDeviceUID( App.getAppContext( ) );
    }

    /*
     * private String getServerUrl() { return ServerUrl.SERVER_URL; }
     */
    private int getRequestSequence()
    {
	return mRequestSeq.getAndIncrement( );
    }

    private final HttpSessionCallBack mHttpSessionCallBack = new HttpSessionCallBack( )
    {

	@Override
	public void onSucceed( byte [] rspData )
	{
	    try
	    {
		RspPacket resPacket = RspPacket.parseFrom( rspData );
		int resCode = resPacket.getRescode( );

		if( resCode == 0 )
		{
		    String action = resPacket.getAction( 0 );
		    if( action.equals( getResponseAction( ) ) )
		    {
			handleResponseBody( preHandleResponseBody( resPacket.getParams( 0 ) , resPacket.getMask( ) ) );
		    }
		    else
		    {
			// 包的action不匹配，在调试过程中可能会出现
			handleResponseError( ERROR.ERROR_ACTION_MISMATCH , "Action from server is misMatch, reqAction:"
				+ getRequestAction( ) + ",resAction:" + action );
		    }
		}
		else
		{
		    String msg = resPacket.getResmsg( );
		    handleResponseError( ERROR.ERROR_ACTION_FAIL , msg );
		}

	    }
	    catch ( InvalidProtocolBufferException e )
	    {

		e.printStackTrace( );
		handleResponseError( ERROR.ERROR_BAD_PACKET , e.getMessage( ) );
	    }
	}

	@Override
	public void onError( int errCode , String strErr )
	{
	    handleResponseError( errCode , strErr );
	}
    };

}

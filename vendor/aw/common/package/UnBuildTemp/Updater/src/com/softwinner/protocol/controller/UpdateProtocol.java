package com.softwinner.protocol.controller;

import com.softwinner.protocol.controller.Packet.MaskCode;
import com.softwinner.protocol.controller.ProtocolListener.ERROR;
import com.softwinner.protocol.net.ServerUrl;
import com.softwinner.update.entity.UpdateBean;
import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;
import com.lidroid.xutils.util.LogUtils;

/**
 * OTA更新请求处理
 * @author Nurmuhammad 
 *
 */
public class UpdateProtocol extends AbstractNetController
{
    //Rom 名称
    String mRomName;
    //Rom 类型
    String mRomType;
    //Rom 版本
    String mRomVersion;
    //返回数据
    UpdateBean mUpdateBean;
    HandlResponse mHandlResponse;

    public interface HandlResponse
    {
	public void handleResponseBean( int errcode , Object msg );
    }

    public UpdateProtocol( String RomName , String RomType , String RomVersion , HandlResponse ResponseHandle )
    {
	this.mRomName = RomName;
	this.mRomType = RomType;
	this.mRomVersion = RomVersion;
	this.mHandlResponse = ResponseHandle;

    }

    public String getRomName()
    {
	return mRomName;
    }

    public void setRomName( String mRomName )
    {
	this.mRomName = mRomName;
    }

    public String getRomType()
    {
	return mRomType;
    }

    public void setRomType( String mRomType )
    {
	this.mRomType = mRomType;
    }

    public String getRomVersion()
    {
	return mRomVersion;
    }

    public void setRomVersion( String mRomVersion )
    {
	this.mRomVersion = mRomVersion;
    }

    public UpdateBean getUpdateBean()
    {
	return mUpdateBean;
    }

    public void setUpdateBean( UpdateBean mUpdateBean )
    {
	this.mUpdateBean = mUpdateBean;
    }

    @Override
    protected ByteString getRequestBody()
    {
	OTA.ReqUpdate.Builder builder = OTA.ReqUpdate.newBuilder( );
	builder.setRomName( getRomName( ) );
	builder.setRomType( getRomType( ) );
	builder.setRomVersion( getRomVersion( ) );
	return builder.build( ).toByteString( );
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
	return ACTION.ACTION_OTA;
    }

    @Override
    protected String getResponseAction()
    {
	// TODO Auto-generated method stub
	return ACTION.ACTION_OTA_RESPONSE;
    }

    @Override
    protected void handleResponseBody( ByteString byteString )
    {
	// TODO Auto-generated method stub
	try
	{
	    OTA.RspUpdate resBody = OTA.RspUpdate.parseFrom( byteString );
	    mUpdateBean = new UpdateBean( );
	    mUpdateBean.setRescode( resBody.getRescode( ) );
	    mUpdateBean.setResmsg( resBody.getResmsg( ) );
	    mUpdateBean.setUpdateType( resBody.getUpdateType( ) );
	    mUpdateBean.setNewRomName( resBody.getNewRomName( ) );
	    mUpdateBean.setNewRomType( resBody.getNewRomType( ) );
	    mUpdateBean.setNewRomVersion( resBody.getNewRomVersion( ) );
	    mUpdateBean.setPackId( resBody.getPackId( ) );
	    mUpdateBean.setPackType( resBody.getPackType( ) );
	    mUpdateBean.setPackSize( resBody.getPackSize( ) );
	    mUpdateBean.setPackMD5( resBody.getPackMD5( ) );
	    mUpdateBean.setPackUrl( resBody.getPackUrl( ) );
	    mUpdateBean.setPubTime( resBody.getPubTime( ) );
	    mUpdateBean.setUpdatePrompt( resBody.getUpdatePrompt( ) );
	    mUpdateBean.setUpdateDesc( resBody.getUpdateDesc( ) );
	    mHandlResponse.handleResponseBean( ERROR.NO_ERROR , mUpdateBean );
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
	mHandlResponse.handleResponseBean( errCode , strErr );
    }

    @Override
    protected String [] getServerUrl()
    {
	LogUtils.i( "ServerUrl.getOtaUpdateUrl( ) =" + ServerUrl.getOtaUpdateUrl( ) );
	return ServerUrl.getOtaUpdateUrl( );
    }
}

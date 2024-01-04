package com.softwinner.protocol.net;

import java.io.InterruptedIOException;
import java.net.ConnectException;
import java.net.UnknownHostException;

import org.apache.http.HttpEntity;
import org.apache.http.HttpStatus;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.protocol.BasicHttpContext;
import org.apache.http.protocol.HttpContext;
import org.apache.http.util.EntityUtils;

import com.lidroid.xutils.util.LogUtils;

public class AsyncHttpPostSession extends AsyncHttpSession
{

    private byte [] mRequestData = null;
    private HttpPost mHttpPost = null;

    public AsyncHttpPostSession( String [] url )
    {
	super( url );
    }

    public void doPost( byte [] requestData )
    {
	mRequestData = requestData;
	createPost( );
    }

    private void createPost()
    {
	AsyncHttpPostSessionManager.getInstance( ).submit( this );
    }

    public void release()
    {
	if( mHttpPost != null )
	    mHttpPost.abort( );
	shutDown( );
    }

    @Override
    protected void doRun()
    {
	//	while ( mHasRetry < HttpSessionConstant.MAX_RETRY.POST_DATA )
	while ( mHasRetry < mUrl.length )
	{
	    LogUtils.e( "-------------------mHasRetry = " + mHasRetry );
	    try
	    {
		HttpClient httpClient = getHttpClient( );

		mHttpPost = new HttpPost( mUrl[mHasRetry] );
		mHttpPost.setHeader( "Content-Type" , "multipart/form-data" );
		mHttpPost.setEntity( new ByteArrayEntity( mRequestData ) );

		HttpContext localContext = new BasicHttpContext( );
		mResponse = httpClient.execute( mHttpPost , localContext );

		int statusCode = mResponse.getStatusLine( ).getStatusCode( );
		if( statusCode == HttpStatus.SC_OK )
		{
		    HttpEntity httpEntity = mResponse.getEntity( );
		    if( httpEntity != null )
		    {
			byte [] byteData = EntityUtils.toByteArray( httpEntity );

			if( mCallBack != null )
			    mCallBack.onSucceed( byteData );
		    }

		    release( );
		    return;
		}

		String strStatus = mResponse.getStatusLine( ).toString( );
		release( );
		if( mCallBack != null )
		    mCallBack.onError( statusCode , strStatus );

		continue;
	    }
	    catch ( Exception excp )
	    {
		if( mHasRetry == mUrl.length - 1 ) //只有在最后一次出错在调用输出
		{
		    if( excp instanceof InterruptedIOException )
		    {
			release( );
			if( mCallBack != null )
			    mCallBack.onError( HttpSessionConstant.ERROR_CODE.ERR_CONNECT_TIMEOUT , excp.toString( ) );
			return;
		    }

		    // Unknown host
		    if( excp instanceof UnknownHostException )
		    {
			release( );
			if( mCallBack != null )
			    mCallBack.onError( HttpSessionConstant.ERROR_CODE.ERR_UNKNOWN_HOST , excp.toString( ) );
			return;
		    }

		    // connect refuse
		    if( excp instanceof ConnectException )
		    {
			release( );
			if( mCallBack != null )
			    mCallBack.onError( HttpSessionConstant.ERROR_CODE.ERR_CONNECT_REFUSE , excp.toString( ) );
			return;
		    }

		    // protocol error
		    if( excp instanceof ClientProtocolException )
		    {
			release( );
			if( mCallBack != null )
			    mCallBack.onError( HttpSessionConstant.ERROR_CODE.ERR_PROTOCOL_ERROR , excp.toString( ) );
			return;
		    }
		}

	    }
	    finally
	    {
		release( );
	    }
	    mHasRetry++;
	}
    }
}

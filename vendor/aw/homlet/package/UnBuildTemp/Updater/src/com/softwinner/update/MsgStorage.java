package com.softwinner.update;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import org.apache.http.util.EncodingUtils;

import android.content.SharedPreferences;

import com.softwinner.update.utils.Constants;
import com.lidroid.xutils.util.LogUtils;

public class MsgStorage
{
    public static MsgStorage msgStorage = null;

    private final static String PERFERENCE_FLAG = "flag";

    private final static String SHARED_PERFERENCE_NAME = "msg.conf";
    private SharedPreferences share = null;

    public static MsgStorage getInstance()
    {
	if( msgStorage == null )
	{
	    msgStorage = new MsgStorage( );
	}
	return msgStorage;
    }

    private MsgStorage()
    {
	share = App.getAppContext( ).getSharedPreferences( SHARED_PERFERENCE_NAME , 0 );
    }

    /**
     * 保存是否以及保存
     * @param flag 保存
     * @param version 保存版本号
     */
    public void saveState( boolean flag )
    {
	share.edit( ).putBoolean( PERFERENCE_FLAG , flag );
    }

    /**
     * 判断是否已经保存
     * @return true代表已保存， false代表未保存
     */
    public boolean isSave()
    {
	return share.getBoolean( PERFERENCE_FLAG , true );
    }

    /**
     * 保存OTA升级信息
     * @param msg 需要保存的信息
     * @param saveFile 保存的路径
     */
    public void saveOTAMsg( String msg , String fileName )
    {
	try
	{
	    File file = App.getAppContext( ).getFilesDir();
	    if( !file.exists( ) )
	    {
		file.mkdir( );
	    }
	    else
	    {
		for( int i = 0 ; i < file.list( ).length ; i++ )
		{
		    if( file.list( )[i].equals( Constants.MSG_SAVE_FILE ) )
		    {
			file.listFiles( )[i].delete( );
		    }
		}
	    }
	    file = new File( App.getAppContext( ).getFilesDir().getPath() + File.separator+ Constants.MSG_SAVE_FILE );
	    LogUtils.i( "saveFile = " + file );
	    FileOutputStream fos = new FileOutputStream( file );
	    byte [] buffer = msg.getBytes( );
	    fos.write( buffer );
	    fos.close( );
	}
	catch ( FileNotFoundException e )
	{
	    LogUtils.e( "no this fileName = " + fileName );
	}
	catch ( IOException e )
	{
	    e.printStackTrace( );
	}
    }

    /**
     * 获取OTA升级信息
     * @param saveFile 保存msg的文件
     * @return 返回String类型MSG的内容
     */
    public String getOTAMsg( File saveFile )
    {
	String sBuffer = "";
	try
	{
	    FileInputStream fis = new FileInputStream( saveFile );
	    int length = fis.available( );
	    byte [] buffer = new byte [length];
	    fis.read( buffer );
	    sBuffer = EncodingUtils.getString( buffer , "UTF-8" );
	    fis.close( );
	}
	catch ( FileNotFoundException e )
	{
	    LogUtils.e( "no found saveFile " + saveFile );
	}
	catch ( IOException e )
	{
	    e.printStackTrace( );
	}
	return sBuffer;
    }
}

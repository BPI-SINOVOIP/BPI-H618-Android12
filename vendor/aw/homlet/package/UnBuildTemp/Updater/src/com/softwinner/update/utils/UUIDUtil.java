package com.softwinner.update.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import org.apache.http.util.EncodingUtils;

import android.content.Context;
import android.content.SharedPreferences;
import android.provider.Settings;

import com.softwinner.update.App;
import com.lidroid.xutils.util.LogUtils;

/**
 * 生成唯一标志符
 * @author greatzhang
 */
public class UUIDUtil
{

    private static UUIDUtil uuidUtil = null;

    private static String UUID_SAVE_DATA_PATH = "/data/tmp/";
    private static String UUID_SAVE_DATA_NAME = "ota_uuid.txt";

    private static final String UUID_FLAG = "uuid";

    //保存的sharePreference名字
    private String UUID_SAVE_SYSTEM_NAME = "uuid_preference";

    private SharedPreferences sp = null;
    private SharedPreferences.Editor ed = null;

    public synchronized static UUIDUtil getInstance()
    {
	if( uuidUtil == null )
	{
	    uuidUtil = new UUIDUtil( );
	}
	return uuidUtil;
    }

    /**
     * 获取唯一标识符
     * @return 返回标识符信息
     */
    public String getUUID()
    {
	String dataUUID = "";
	if( isUUIDExist( ) )
	{
	    dataUUID = getDateUUID( new File( UUID_SAVE_DATA_PATH + UUID_SAVE_DATA_NAME ) );
	}
	else
	{
	    dataUUID = creatUUID( );
	    saveUUID( dataUUID );
	}
	return dataUUID;
    }

    private UUIDUtil()
    {
	sp = App.getAppContext( ).getSharedPreferences( UUID_SAVE_SYSTEM_NAME , Context.MODE_PRIVATE );
	ed = App.getAppContext( ).getSharedPreferences( UUID_SAVE_SYSTEM_NAME , Context.MODE_PRIVATE ).edit( );
    }

    /**
     * 判断是否存在UUID
     * @return (先后从系统和anpda目录读取，只要存在则为真)
     */
    private boolean isUUIDExist()
    {
	String systemUUID = getSystemUUID( );
	String dataUUID = getDateUUID( new File( UUID_SAVE_DATA_PATH + UUID_SAVE_DATA_NAME ) );
	LogUtils.i( "systemUUID　＝　" + systemUUID + "  dataUUID = " + dataUUID );
	if( systemUUID.equals( "" ) )
	{
	    if( dataUUID.equals( "" ) )
	    {
		return false;
	    }
	    else
	    {
		ed.putString( UUID_FLAG , dataUUID );
		ed.commit( );
		return true;
	    }
	}
	else
	{
	    if( dataUUID.equals( "" ) )
	    {
		saveDataUUID( systemUUID );
	    }
	    return true;
	}
    }

    //存储UUID
    private void saveUUID( String uuid )
    {
	if( uuid != null && !uuid.equals( "" ) )
	{
	    LogUtils.i( "UUID = " + uuid );
	    saveDataUUID( uuid );
	    saveSystemUUID( uuid );
	}
    }

    /**
     * 生成UUID 即唯一标识符
     * @return 唯一标识符
     */
    private String creatUUID()
    {
	String uuid = "";
	uuid = Settings.Secure.getString( App.getAppContext( ).getContentResolver( ) , Settings.Secure.ANDROID_ID );
	return uuid;
    }

    /**
     * 保存系统分区UUID
     */
    private void saveSystemUUID( String UUID )
    {
	ed.putString( UUID_FLAG , UUID );
	ed.commit( );
    }

    /**
     * 获取系统分区的UUID
     */
    private String getSystemUUID()
    {
	return sp.getString( UUID_FLAG , "" );
    }

    /**
     * 保存OTA升级信息
     * @param msg 需要保存的信息
     * @param saveFile 保存的路径
     */
    private void saveDataUUID( String UUID )
    {
	try
	{
	    File file = new File( UUID_SAVE_DATA_PATH );
	    if( !file.exists( ) )
	    {
		file.mkdir( );
	    }
	    else
	    {
		if(file.list( ) != null){
		    for( int i = 0 ; i < file.list( ).length ; i++ )
		    {
		        if( file.list( )[i].equals( UUID_SAVE_DATA_NAME ) )
		        {
			    file.listFiles( )[i].delete( );
		        }
		    }
		}
	    }
	    file = new File( UUID_SAVE_DATA_PATH + UUID_SAVE_DATA_NAME );
	    LogUtils.i( "saveFile = " + file );
	    FileOutputStream fos = new FileOutputStream( file );
	    byte [] buffer = UUID.getBytes( );
	    fos.write( buffer );
	    fos.close( );
	}
	catch ( FileNotFoundException e )
	{
	    LogUtils.e( "no this fileName = " + UUID_SAVE_DATA_NAME );
	}
	catch ( IOException e )
	{
	    e.printStackTrace( );
	}
    }

    /**
     * 获取DATA分区下的UUID
     * @param saveFile 保存msg的文件
     * @return 返回String类型MSG的内容
     */
    private String getDateUUID( File saveFile )
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

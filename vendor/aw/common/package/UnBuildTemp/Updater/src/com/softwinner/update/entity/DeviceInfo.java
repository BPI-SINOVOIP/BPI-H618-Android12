package com.softwinner.update.entity;

import android.os.SystemProperties;

import com.lidroid.xutils.util.LogUtils;

public class DeviceInfo
{
    public static final String UNKNOWN = "unknown";

    /**
     * 获取信息
     * @param firmWareName
     * @return 返回数组，deviceinfo[1]为RomName,一次分别为RomType,RomVersion
     */
    public static String [] getDeviceInfo()
    {
	//得到版本信息
	String info = getString( "ro.product.firmware" );
	LogUtils.i( "fireware = " + info );
	String [] deviceInfo = info.split( "_" );

	//	for( int i = 0 ; i < deviceInfo.length ; i++ )
	//	{
	//	    LogUtils.i( deviceInfo[i] );
	//	}
	return deviceInfo;
    }

    public static String getAWDeviceVersion()
    {
	//得到版本信息
	String info = getString( "ro.product.firmware" );
	LogUtils.i( "fireware = " + info );

	//	for( int i = 0 ; i < deviceInfo.length ; i++ )
	//	{
	//	    LogUtils.i( deviceInfo[i] );
	//	}
	return info;
    }
    /**
     * 获取Rom 名字
     */
    public static String getRomName()
    {
   	
	String romName = getString( "ro.product.rom.name" );
	if( isEmpty( romName ) )
	{
		 romName = "BoxRom";
	}
	return romName;
    }

    /**
     * 获取Rom类型
     */
    public static String getRomType()
    {
	String romType = getString( "ro.product.rom.type" );
	if( isEmpty( romType ) )
	{
		romType = "NB";	
	}
   
	return romType;
    }

    /**
     * 获取Rom版本
     */
    public static String getRomVersion()
    {
    	
	String romVersion = getString( "ro.product.rom.version" );
	if( isEmpty( romVersion ) )
	{
		String incremental = getString( "ro.build.version.incremental" );
		//String year = incremental.substring(0, 4);
		//String month = incremental.substring(4, 6);
	//	String day = incremental.substring(6, 8);
	//	romVersion = year + "." + month + "." + day;
		romVersion = incremental;
		LogUtils.d("getRomVersion = " + romVersion); 
	}
	return romVersion;
    }

    //获取固件信息
    public static String getDeviceFwVersion()
    {
	return getString( "ro.product.onda.firmware" );
    }

    /**
     * 获取系统版本信息
     */
    private static String getString( String property )
    {
	return SystemProperties.get( property , UNKNOWN );
    }

    /**
     * 判断是否存在该字段
     */
    private static boolean isEmpty( String tmp )
    {
	if( tmp == null || tmp.equals( "" ) || tmp.equals( UNKNOWN ) )
	{
	    return true;
	}
	return false;
    }
}

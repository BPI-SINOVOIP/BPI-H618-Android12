package com.softwinner.update.utils;

import java.util.ArrayList;

import android.content.Context;

import com.softwinner.update.entity.DeviceInfo;
import com.lidroid.xutils.util.LogUtils;

/**
 * 工具类
 * @author Greatzhang
 *
 */
public class Utils
{
    public static String TAG = "Utils";

    public static String VersionName( Context context )
    {
	String packageName = context.getPackageName( );
	String versionName = null;
	try
	{
	    versionName = context.getPackageManager( ).getPackageInfo( packageName , 0 ).versionName;
	}
	catch ( Exception e )
	{
	    if( Constants.DEBUG )
		LogUtils.e( "versionName error ===" + e.toString( ) );
	    e.printStackTrace( );
	}
	return versionName;
    }

    /**
     * 获取模块版本号
     * @param context 内容管理器
     * @return 返回版本号
     */
    public static String getVersionCode( Context context )
    {
	String packageName = context.getPackageName( );
	int versionCode = 0;
	try
	{
	    versionCode = context.getPackageManager( ).getPackageInfo( packageName , 0 ).versionCode;
	}
	catch ( Exception e )
	{
	    if( Constants.DEBUG )
		LogUtils.e( "getVersionCode error ===" + e.toString( ) );
	    e.printStackTrace( );
	}
	return String.valueOf( versionCode );
    }

    /**
     * 将字节表示的文件大小格式化为K/M/G这样的字符串形式
     * @param lSize 文件大小(字节为单位)
     * @return 格式化后的字符串
     */
    public static String fileSize2String( long lSize )
    {
	if( lSize > 1024 * 1024 * 1024 )
	{
	    return String.format( "%.1fGB" , lSize / 1024.0 / 1024.0 / 1024.0 );
	}
	return String.format( "%.1fMB" , lSize / 1024.0 / 1024.0 );
    }

    /**
     * 将字节表示的文件大小格式化为K/M/G这样的字符串形式
     * @param lSize 文件大小(字节为单位)
     * @return 格式化后的字符串
     */
    public static String fileSize2StringWithoutB( long lSize )
    {
	if( lSize > 1024 * 1024 * 1024 )
	{
	    return lSize / 1024 / 1024 / 1024 + "G";
	}
	return ( lSize / 1024 / 1024 == 0 ) ? String.format( "%.1fM" , lSize / 1024.0 / 1024.0 ) : ( lSize / 1024 / 1024 + "M" );
    }

    /**
     * 分割字符串
     * 
     * @param line
     *            原始字符串
     * @param seperator
     *            分隔符
     * @return 分割结果
     */
    public static String [] split( String line , String seperator )
    {
	if( line == null || seperator == null || seperator.length( ) == 0 )
	    return null;
	ArrayList< String > list = new ArrayList< String >( );
	int pos1 = 0;
	int pos2;
	for( ; ; )
	{
	    pos2 = line.indexOf( seperator , pos1 );
	    if( pos2 < 0 )
	    {
		list.add( line.substring( pos1 ) );
		break;
	    }
	    list.add( line.substring( pos1 , pos2 ) );
	    pos1 = pos2 + seperator.length( );
	}
	// 去掉末尾的空串，和String.split行为保持一致
	for( int i = list.size( ) - 1 ; i >= 0 && list.get( i ).length( ) == 0 ; --i )
	{
	    list.remove( i );
	}
	return list.toArray( new String [0] );
    }

    /**
     * 将字节形式的文件大小转换为可读的字符串，比如K、M、G
     */
    public static String fileSizeToString( int size )
    {
	try
	{
	    if( size < 1024 )
	    {
		return new Integer( size ).toString( ) + "B";
	    }
	    if( size < 1024 * 1024 )
	    {
		return new Integer( size / 1024 ).toString( ) + "K";
	    }
	    if( size < 1024 * 1024 * 1024 )
	    {
		return String.format( "%.2fM" , (double)size / ( 1024 * 1024 ) );
	    }
	    // 全部按G算
	    return String.format( "%.2fG" , (double)size / ( 1024 * 1024 * 1024 ) );
	}
	catch ( Exception e )
	{
	    e.printStackTrace( );

	    return "";
	}
    }

    //通过Url获取文件名
    public static String getFileNameFromUrl( String url )
    {
	String name = "";
	int index = url.lastIndexOf( "/" );
	if( index > 0 )
	{
	    name = url.substring( index + 1 );
	    if( name.trim( ).length( ) > 0 )
	    {
		return name;
	    }
	}
	return name;
    }

    public static boolean comareString( String str1 , String str2 )
    {
	return str1.equals( str2 );
    }

    /**
     * 
     * @param version1
     * @param version2
     * @return if version1 > version2, return 1, if equal, return 0, else return -1
     */
    public static int compareVersion( String version1 , String version2 )
    {
    	
	if( version1 == null || version1.length( ) == 0 || version2 == null || version2.length( ) == 0
		|| version1.equals( DeviceInfo.UNKNOWN ) )
	{
		LogUtils.e("version1 = " + version1 + " version2 = " + version2);
	    LogUtils.e( "throw new IllegalArgumentException , Invalid parameter!" );
	    return -1;
	}

	int index1 = 0;
	int index2 = 0;
	while ( index1 < version1.length( ) && index2 < version2.length( ) )
	{
	    int [] number1 = getValue( version1 , index1 );
	    int [] number2 = getValue( version2 , index2 );

	    if( number1[0] < number2[0] )
		return -1;
	    else if( number1[0] > number2[0] )
		return 1;
	    else
	    {
		index1 = number1[1] + 1;
		index2 = number2[1] + 1;
	    }
	}
	if( index1 == ( version1.length( ) + 1 ) && index2 == ( version2.length( ) + 1 ) )
	    return 0;
	if( index1 < version1.length( ) )
	    return 1;
	else
	    return -1;
    }

    /**
     * 
     * @param version
     * @param index the starting point
     * @return the number between two dots, and the index of the dot
     */
    private static int [] getValue( String version , int index )
    {
	int [] value_index = new int [2];
	StringBuilder sb = new StringBuilder( );
	while ( index < version.length( ) && version.charAt( index ) != '.' )
	{
	    sb.append( version.charAt( index ) );
	    index++;
	}
	value_index[0] = Integer.parseInt( sb.toString( ) );
	value_index[1] = index;

	return value_index;
    }

    private static long lastClickTime = 0;

    public static synchronized boolean isFastDouble()
    {
	long time = System.currentTimeMillis( );
	long timeD = time - lastClickTime;
	if( 0 < timeD && timeD < 500 )
	{ // 500ms内不能同事起效
	    return true;
	}
	lastClickTime = time;
	return false;
    }
}

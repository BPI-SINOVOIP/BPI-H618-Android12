package com.softwinner.update.utils;

import java.util.Locale;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;

import com.softwinner.update.entity.DeviceInfo;

/**
 * 设备信息工具类
 * @author Nurmuhammad
 *
 */
public class DeviceUtil
{

    public static final String TAG = "DeviceUtil";
    private static boolean OtherAPIEnable = true;
    public static final String UNKNOWN = "unknown";

    //获取CPU核数
    public static int getAviableCpuCount()
    {
	return Runtime.getRuntime( ).availableProcessors( );
    }

    //获取DeviceBrand信息
    public static String getDeviceBrand()
    {
	return android.os.Build.BRAND;
    }

    public static String getFullRomNameAndVersion()
    {
	return DeviceInfo.getRomName( ).toUpperCase( ) + " " + DeviceInfo.getRomVersion( );
    }

    //获取Device名字
    public static String getDeviceName()
    {
	return android.os.Build.DEVICE;
    }

    //获取Device Board 信息
    public static String getDeviceBoard()
    {
	return android.os.Build.BOARD;
    }

    //获取 Android 版本
    public static String getDeviceAndroidVersion()
    {
	return android.os.Build.VERSION.RELEASE;
    }

    //获取编译时间
    public static long getDeviceBuildTime()
    {
	return android.os.Build.TIME;
    }

    //获取编译者
    public static String getDeviceBuildUser()
    {
	return android.os.Build.USER;
    }

    //获取指纹信息
    public static String getDeviceBuildFinger()
    {
	return android.os.Build.FINGERPRINT;
    }

    //获取Android Build ID
    public static String getDeviceBuildId()
    {
	return android.os.Build.ID;
    }

    //获取设备型号
    public static String getDeviceModel()
    {
	return android.os.Build.MODEL;
    }

    //获取制造商
    public static String getDeviceManufactury()
    {
	return android.os.Build.MANUFACTURER;
    }

    //获取产品名
    public static String getDeviceProduct()
    {
	return android.os.Build.PRODUCT;
    }

    //获取android SDK 版本
    public static int getSdkVersion()
    {
	return android.os.Build.VERSION.SDK_INT;
    }

    //获取设备wifi的 mac地址
    public static String getMacAddr( Context context )
    {
	WifiManager wifi = (WifiManager)context.getSystemService( Context.WIFI_SERVICE );
	return wifi.getConnectionInfo( ).getMacAddress( );
    }

    //获取国家与语言码
    public static String getCountryCode( Context context )
    {
	Locale locale = context.getResources( ).getConfiguration( ).locale;
	return locale.getLanguage( ) + "-" + locale.getCountry( );
    }

    //获取设备ID（手机IMEI，平板CHIPID，取不到则不传） ------ 现在没chipid可取， 取imei
    public static String getDeviceID( Context context )
    {
	TelephonyManager tm = (TelephonyManager)context.getSystemService( Context.TELEPHONY_SERVICE );
	return tm.getDeviceId( );
    }

    //手机卡的IMSI值
    public static String getDeviceImsi( Context context )
    {
	TelephonyManager tm = (TelephonyManager)context.getSystemService( Context.TELEPHONY_SERVICE );
	return tm.getSubscriberId( );
    }

    //获取UIID值
    public static String getSoloUUID( Context context )
    {
	return UUIDUtil.getInstance( ).getUUID( );
	//	return Settings.Secure.getString( context.getContentResolver( ) , Settings.Secure.ANDROID_ID );
    }

    public static int getDeviceDisplayWidth( Context context )
    {
	DisplayMetrics dm = context.getResources( ).getDisplayMetrics( );
	return dm.widthPixels;
    }

    public static int getDeviceDisplayHeight( Context context )
    {
	DisplayMetrics dm = context.getResources( ).getDisplayMetrics( );
	return dm.heightPixels;
    }

    public static String getDeviceUID( Context context )
    {
	StringBuilder sbuilder = new StringBuilder( );
	sbuilder.append( "ei=" + getDeviceID( context ) );
	String ai = getSoloUUID( context );
	if( !TextUtils.isEmpty( ai ) )
	{
	    sbuilder.append( "&ai=" + ai );
	}
	//sbuilder.append("&ui=");
	String wm = getMacAddr( context );
	if( !TextUtils.isEmpty( wm ) )
	{
	    sbuilder.append( "&wm=" + wm );
	}
	sbuilder.append( "&si=" + getDeviceImsi( context ) );
	sbuilder.append( "&mf=" + getDeviceManufactury( ) );
	sbuilder.append( "&bd=" + getDeviceBrand( ) );
	sbuilder.append( "&md=" + getDeviceModel( ) );
	sbuilder.append( "&de=" + getDeviceName( ) );
	sbuilder.append( "&pl=" + getSdkVersion( ) );
	DisplayMetrics dm = context.getResources( ).getDisplayMetrics( );
	sbuilder.append( "&sw=" + dm.widthPixels );
	sbuilder.append( "&sh=" + dm.heightPixels );
	ConnectivityManager cm = (ConnectivityManager)context.getSystemService( Context.CONNECTIVITY_SERVICE );
	NetworkInfo ni = cm.getActiveNetworkInfo( );
	if( ni != null )
	{
	    sbuilder.append( "&nt=" + cm.getActiveNetworkInfo( ).getType( ) );
	}
	sbuilder.append( "&la=" + getCountryCode( context ) );
	if( Constants.DEBUG )
	    Log.d( TAG , "getDeviceUID==" + sbuilder.toString( ) );
	return sbuilder.toString( );
    }

    //    private static String getString( String property )
    //    {
    //	if( OtherAPIEnable )
    //	    return SystemProperties.get( property , UNKNOWN );
    //	else
    //	    return UNKNOWN;
    //    }
}

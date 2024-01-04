package com.softwinner.update.utils;

import java.io.File;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Parcelable;
import android.os.StatFs;
import android.util.Log;
import android.widget.Toast;

/**
 * 文件操作相关工具
 * 
 * 
 * 
 */
public class FileUtils
{

    /**
     * 程序数据保存的根路径
     */
    public static final String BASE_DIR = "/pada/";

    /** APK保存路径 */
    public static final String APK_DIR = BASE_DIR + "apk/";

    /** 日志保存路径 */
    public static final String LOG_DIR = BASE_DIR + "log/";

    /** 缓存数据库保存路径 */
    public static final String CACHE_PATH = BASE_DIR + "Cache/";

    /** 图片保存路径 */
    public final static String ICON_CACHE_PATH = BASE_DIR + "Icon/";

    /** 放大图片的路径 */
    public final static String ICON_BIG_CACHE_PATH = BASE_DIR + "IconCache/";

    /**
     * 图标扩展名
     */
    public static final String ICON_EXT_NAME = ".qqmm";

    private FileUtils()
    {};

    /**
     * 获取当前有效的存储路径, 以"/"结尾
     * 
     * @param path
     * @return
     */
    public static String getStorePath( String path , Context context )
    {
	// 获取SdCard状态
	String state = android.os.Environment.getExternalStorageState( );
	// 判断SdCard是否存在并且是可用的
	if( android.os.Environment.MEDIA_MOUNTED.equals( state ) )
	{
	    if( android.os.Environment.getExternalStorageDirectory( ).canWrite( ) )
	    {
		File file = new File( android.os.Environment.getExternalStorageDirectory( ).getPath( ) + path );
		if( !file.exists( ) )
		{
		    file.mkdirs( );
		}
		String absolutePath = file.getAbsolutePath( );
		if( !absolutePath.endsWith( "/" ) )
		{// 保证以"/"结尾
		    absolutePath += "/";
		}
		return absolutePath;
	    }
	}
	String absolutePath = context.getFilesDir( ).getAbsolutePath( );
	if( !absolutePath.endsWith( "/" ) )
	{// 保证以"/"结尾
	    absolutePath += "/";
	}
	return absolutePath;
    }

    /**
     * 启动指定包名的软件
     * 
     * @param packageName
     */
    public static void openInstallApk( String packageName , Context context , CharSequence strError )
    {
	Intent intentdel = new Intent( );
	intentdel = getStartPackage( packageName , context );
	if( intentdel != null )
	{

	    intentdel.setAction( android.content.Intent.ACTION_MAIN );
	    try
	    {
		context.startActivity( intentdel );
	    }
	    catch ( Exception e )
	    {
		e.printStackTrace( );
	    }
	}
	else
	{
	    Toast.makeText( context , strError , Toast.LENGTH_LONG ).show( );
	}
    }

    /**
     * 找到软件中可以启动的activity
     */
    private static Map< String , String > sMapPackageAndActivity = new HashMap< String , String >( );

    /**
     * 找到该该包名所包含的第一个标记为Intent.ACTION_MAIN属性的Activity并返回该intent
     * 
     * @param packageName
     *            :要启动的包名
     * @return Intent： 返回的Intent可以直接调用startActivity使用
     */
    public static Intent getStartPackage( String packageName , Context context )
    {
	Intent mainIntent = new Intent( Intent.ACTION_MAIN , null );
	mainIntent.addCategory( Intent.CATEGORY_LAUNCHER );

	if( !sMapPackageAndActivity.containsKey( packageName ) )
	{
	    final List< ResolveInfo > apps = context.getPackageManager( ).queryIntentActivities( mainIntent , 0 );

	    for( ResolveInfo info : apps )
	    {
		// Log.v(TAG, " packageName" + info.activityInfo.packageName);
		sMapPackageAndActivity.put( info.activityInfo.applicationInfo.packageName , info.activityInfo.name );
	    }
	}

	String name = sMapPackageAndActivity.get( packageName );
	if( name != null )
	{
	    Intent intent = new Intent( Intent.ACTION_MAIN );
	    intent.addCategory( Intent.CATEGORY_LAUNCHER );
	    intent.setComponent( new ComponentName( packageName , name ) );
	    intent.setFlags( Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED );
	    return intent;
	}
	return null;
    }

    /* 判断文件MimeType的method */
    public static String getMIMEType( File f )
    {
	String type = "";
	String fName = f.getName( );
	/* 取得扩展名 */
	String end = fName.substring( fName.lastIndexOf( "." ) + 1 , fName.length( ) ).toLowerCase( );

	/* 依扩展名的类型决定MimeType */
	if( end.equals( "m4a" ) || end.equals( "mp3" ) || end.equals( "mid" ) || end.equals( "xmf" )
		|| end.equals( "ogg" ) || end.equals( "wav" ) )
	{
	    type = "audio";
	}
	else if( end.equals( "3gp" ) || end.equals( "mp4" ) )
	{
	    type = "video";
	}
	else if( end.equals( "jpg" ) || end.equals( "gif" ) || end.equals( "png" ) || end.equals( "jpeg" )
		|| end.equals( "bmp" ) )
	{
	    type = "image";
	}
	else if( end.equals( "apk" ) )
	{
	    /* android.permission.INSTALL_PACKAGES */
	    type = "application/vnd.android.package-archive";
	}
	else
	{
	    type = "*";
	}
	/* 如果无法直接打开，就跳出软件列表给用户选择 */
	if( end.equals( "apk" ) )
	{}
	else
	{
	    type += "/*";
	}
	return type;
    }

    /**
     * 获取可用的手机SD卡或内存的容量大小
     * 
     * @return
     */
    public static long getAvailableStorageSize( String partition )
    {
	File base = new File( partition );
	StatFs stat = new StatFs( base.getPath( ) );
	long nAvailableCount = stat.getBlockSize( ) * ( (long)stat.getAvailableBlocks( ) - 4 );
	return nAvailableCount;
    }

    /**
     * 删除文件夹中的所有文件
     * 
     * @param path
     */
    public static void clearnSubFiles( String path )
    {
	File f = new File( path );
	if( f.exists( ) )
	{
	    File [] files = f.listFiles( );
	    if( files != null )
	    {
		for( File file : files )
		{
		    if( file.exists( ) )
		    {
			file.delete( );
		    }
		}
	    }
	}
    }

    /**
     * 创建快捷方式
     * 
     * @param draw
     * @param name
     * @param packageName
     */

    public static void addShortCut( int drawResId , CharSequence name , String packageName , Context context )
    {
	Intent thisIntent = new Intent( );
	thisIntent = getStartPackage( packageName , context );
	if( thisIntent == null )
	{

	    return;
	}
	thisIntent.setAction( android.content.Intent.ACTION_MAIN );

	Intent shortcut = new Intent( "com.android.launcher.action.INSTALL_SHORTCUT" );
	// 快捷方式的名称
	shortcut.putExtra( Intent.EXTRA_SHORTCUT_NAME , name );

	try
	{
	    Context resContext = null;
	    String selfPackageName = context.getPackageName( );
	    if( !packageName.equals( selfPackageName ) )
	    {
		resContext = context.createPackageContext( packageName , Context.CONTEXT_IGNORE_SECURITY );
	    }
	    else
	    {
		resContext = context;
	    }
	    // 快捷方式的图标
	    Parcelable icon = Intent.ShortcutIconResource.fromContext( resContext , drawResId );
	    shortcut.putExtra( Intent.EXTRA_SHORTCUT_ICON_RESOURCE , icon );
	    shortcut.putExtra( Intent.EXTRA_SHORTCUT_INTENT , thisIntent );
	    shortcut.putExtra( "duplicate" , false ); // 不允许重复创建
	    context.sendBroadcast( shortcut );
	}
	catch ( Exception e )
	{
	    e.printStackTrace( );
	    return;
	}

    }

    /**
     * 判断是否已经存在快捷方式
     * 
     * @param context
     * @param pkg
     * @return
     */
    public static boolean hasShortCutByApkName( Context context , String pkg )
    {

	String url = "";

	int systemVersion = Integer.parseInt( Build.VERSION.SDK ); // 获取系统版本

	if( systemVersion < 8 )
	{

	    url = "content://com.android.launcher.settings/favorites?notify=true";

	}
	else
	{

	    url = "content://com.android.launcher2.settings/favorites?notify=true";

	}

	String strAppName = "";
	PackageManager pkgMag = context.getPackageManager( );
	try
	{
	    PackageInfo packageInfo = pkgMag.getPackageInfo( pkg , 0 );
	    strAppName = packageInfo.applicationInfo.loadLabel( pkgMag ).toString( );
	}
	catch ( NameNotFoundException e )
	{
	    e.printStackTrace( );
	}

	ContentResolver resolver = context.getContentResolver( );

	Cursor cursor = resolver.query( Uri.parse( url ) , new String [] { "title" , "iconResource" } , "title=?" ,

	new String [] { strAppName } , null );

	boolean bRet = false;
	if( cursor != null )
	{
	    if( cursor.moveToFirst( ) )
	    {
		bRet = true;
	    }
	    cursor.close( );
	}

	return bRet;

    }

    /**
     * 获取图标的储存路径
     * 
     * @param isBig
     *            是大图标的路径
     * @return
     */
    public static String getAvaiableIconStorePath( boolean isBig , Context context )
    {
	if( isBig )
	{
	    return getStorePath( ICON_BIG_CACHE_PATH , context );
	}
	else
	{
	    return getStorePath( ICON_CACHE_PATH , context );
	}
    }

    /**
     * 把网络路径转化为本地路径,把其中的特殊符号用_表示
     * 
     * @param url
     * @return
     */
    public static String convertUrlToLocalFile( String url )
    {
	Uri uri = Uri.parse( url );
	String path = uri.getPath( ) + "_" + uri.getQuery( );
	if( path != null )
	{
	    path = path.replace( "/" , "_" );
	    path = path.replace( "\\" , "_" );
	    path = path.replace( "*" , "_" );
	    path = path.replace( "?" , "_" );
	    path = path.replace( "=" , "_" );
	    path = path.replace( "." , "_" );
	    path += FileUtils.ICON_EXT_NAME;
	}
	// Log.v(TAG, path);
	return path;

    }

    /**
     * @param zoonName 分区名字
     * @return 返回分区剩余大小，单位MB
     */
    public static long getZoneFreeSize( String zoonName )
    {
	StatFs sf = new StatFs( zoonName );
	long blockSize = sf.getBlockSize( );
	long blockCount = sf.getBlockCount( );
	long availCount = sf.getAvailableBlocks( );
	Log.d( "" , "block大小:" + blockSize + ",block数目:" + blockCount + ",总大小:" + blockSize * blockCount / 1024 + "KB" );
	Log.d( "" , "可用的block数目：:" + availCount + ",可用大小:" + availCount * blockSize / 1024 + "KB" );
	return ( ( availCount * blockSize ) / ( 1024 * 1024 ) );
    }

    /**
     * 判断是否已经存在快捷方式
     * 
     * @return
     */
    public static int hasShortcutByAppName( Context context , String appName )
    {
	if( appName == null )
	{
	    return -1;
	}

	Cursor c = null;
	try
	{
	    final ContentResolver cr = context.getContentResolver( );
	    Integer sdkInt = android.os.Build.VERSION.SDK_INT;
	    String l2 = "content://com.android.launcher2.settings/favorites?notify=true";
	    String l = "content://com.android.launcher.settings/favorites?notify=true";
	    String str = "";
	    if( sdkInt > 7 )
	    {
		str = l2;
	    }
	    else
	    {
		str = l;
	    }

	    c = cr.query( Uri.parse( str ) , new String [] { "title" , "iconResource" } , "title=?" , new String [] { appName } , null );
	    if( c == null )
	    {
		if( sdkInt > 7 )
		{
		    str = l;
		}
		else
		{
		    str = l2;
		}
		c = cr.query( Uri.parse( str ) , new String [] { "title" , "iconResource" } , "title=?" , new String [] { appName } , null );
	    }
	    int count = -1;
	    if( c != null && c.getCount( ) > 0 )
	    {
		count = c.getCount( );
	    }
	    if( c != null )
	    {
		c.close( );
		c = null;
	    }
	    return count;
	}
	catch ( Exception e )
	{
	    e.printStackTrace( );
	    if( c != null )
	    {
		c.close( );
		c = null;
	    }
	}
	return -1;
    }

    /**
     * 删除文件
     * @param path 文件路径
     */
    public synchronized static void deleteFile( String path )
    {
	File file = new File( path );
	if( file.isFile( ) )
	{
	    file.delete( );
	    return;
	}
	if( file.isDirectory( ) )
	{
	    File [] childFile = file.listFiles( );
	    if( childFile == null || childFile.length == 0 )
	    {
		file.delete( );
		return;
	    }
	    for( File f : childFile )
	    {
		deleteFile( f.toString( ) );
	    }
	    file.delete( );
	}

    }

}

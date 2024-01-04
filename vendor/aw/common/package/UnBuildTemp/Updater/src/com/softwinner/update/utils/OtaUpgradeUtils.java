package com.softwinner.update.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.security.GeneralSecurityException;

import android.content.Context;
import android.os.Build;
import android.os.RecoverySystem;

import com.lidroid.xutils.util.LogUtils;

/**
 * Recovery补助类
 * @author greatzhang
 *
 */
public class OtaUpgradeUtils
{

    public static final int ERROR_INVALID_UPGRADE_PACKAGE = 0;

    public static final int ERROR_FILE_DOES_NOT_EXIT = 1;

    public static final int ERROR_FILE_IO_ERROR = 2;

    public static final int ERROR_FILE_COPY_IO_ERROR = 3;

    public static final int ERROR_FILE_COPY_DOES_NOT_EXIT = 4;

    public static final int ERROR_INSTALL_IO_ERROR = 5;

    public static final String DEFAULT_PACKAGE_NAME = "update_local.zip";

    private Context mContext;

    private static String mOtaPackFileName = "";

    private boolean mDeleteSource = false;

    public OtaUpgradeUtils( Context context )
    {
	mContext = context;
    }

    public static void setOtaPackFile( String FilePath )
    {
	mOtaPackFileName = FilePath;
    }

    /*
     * 
     * 
     * */
    public interface ProgressListener extends RecoverySystem.ProgressListener
    {
	@Override
	public void onProgress( int progress );

	public void onVerifyFailed( int errorCode , Object object );

	public void onCopyProgress( int progress );

	public void onCopyFailed( int errorCode , Object object );

	public void onInstallFailed( int errorCode , Object object );

	public void onInstallSucceed();
    }

    public boolean upgredeFromOta( File packageFile , ProgressListener progressListener )
    {
	try
	{
	    RecoverySystem.verifyPackage( packageFile , progressListener , null );
	}
	catch ( IOException e )
	{
	    progressListener.onVerifyFailed( ERROR_FILE_DOES_NOT_EXIT , packageFile.getPath( ) );
	    e.printStackTrace( );
	    return false;
	}
	catch ( GeneralSecurityException e )
	{
	    progressListener.onVerifyFailed( ERROR_INVALID_UPGRADE_PACKAGE , packageFile.getPath( ) );
	    e.printStackTrace( );
	    return false;
	}

	//在升级之前需要先删除原有的Update_local.zip文件
	FileUtils.deleteFile( Constants.DOWNLOAD_PATH + DEFAULT_PACKAGE_NAME );
	packageFile.renameTo( new File( Constants.DOWNLOAD_PATH + DEFAULT_PACKAGE_NAME ) ); //重命名为Update_local.zip

	if( packageFile.getAbsolutePath( ).equals( Constants.DOWNLOAD_PATH + mOtaPackFileName ) && mDeleteSource )
	{
	    packageFile.delete( );
	}
	installPackage( mContext , new File( Constants.DOWNLOAD_PATH + DEFAULT_PACKAGE_NAME ) , progressListener );
	return false;
	//在升级之前需要先删除原有的Update_local.zip文件
	//	FileUtils.deleteFile( DATA_PARTITION + DEFAULT_PACKAGE_NAME );
	//
	//	boolean b = copyFile( packageFile , new File( DATA_PARTITION + DEFAULT_PACKAGE_NAME ) , progressListener );
	//	if( b && packageFile.getAbsolutePath( ).equals( Constants.DOWNLOAD_PATH + mOtaPackFileName ) && mDeleteSource )
	//	{
	//	    packageFile.delete( );
	//	}
	//	if( b )
	//	{
	//	    installPackage( mContext , new File( DATA_PARTITION + DEFAULT_PACKAGE_NAME ) , progressListener );
	//	    return true;
	//	}
	//	return false;
    }

    public boolean upgradeFromOta( String packagePath , ProgressListener progressListener )
    {
	return upgredeFromOta( new File( packagePath ) , progressListener );
    }

    public void deleteSource( boolean b )
    {
	mDeleteSource = b;
    }

    public static boolean copyFile( File src , File dst , ProgressListener listener )
    {
	long inSize = src.length( );
	long outSize = 0;
	int progress = 0;
	listener.onCopyProgress( progress );
	try
	{
	    LogUtils.i( "dst.getParentFile( ) = " + dst.getParentFile( ) );
	    if( !dst.getParentFile( ).exists( ) )
	    {
		//		Runtime.getRuntime( ).exec( "mkdir /data/"+ dst.getParentFile( ).toString( ));
		dst.getParentFile( ).mkdirs( );
	    }
	    if( !dst.exists( ) )
	    {
		dst.createNewFile( );
	    }
	    FileInputStream in = new FileInputStream( src );
	    FileOutputStream out = new FileOutputStream( dst );
	    int length = -1;
	    byte [] buf = new byte [1024];
	    while ( ( length = in.read( buf ) ) != -1 )
	    {
		out.write( buf , 0 , length );
		outSize += length;
		int temp = (int) ( ( (float)outSize ) / inSize * 100 );
		if( temp != progress )
		{
		    progress = temp;
		    listener.onCopyProgress( progress );
		}
	    }
	    out.flush( );
	    in.close( );
	    out.close( );
	}
	catch ( FileNotFoundException e )
	{
	    listener.onVerifyFailed( ERROR_FILE_COPY_DOES_NOT_EXIT , src.getPath( ) );
	    e.printStackTrace( );
	    return false;
	}
	catch ( IOException e )
	{
	    listener.onVerifyFailed( ERROR_FILE_COPY_IO_ERROR , src.getPath( ) );
	    e.printStackTrace( );
	    return false;
	}
	return true;
    }

    public static boolean copyFile( String src , String dst , ProgressListener listener )
    {
	return copyFile( new File( src ) , new File( dst ) , listener );
    }

    public static void installPackage( Context context , File packageFile , ProgressListener listener )
    {
	try
	{
	    listener.onInstallSucceed( );
	    RecoverySystem.installPackage( context , packageFile );
	}
	catch ( IOException e )
	{
	    listener.onInstallFailed( ERROR_INSTALL_IO_ERROR , packageFile.getPath( ) );
	    e.printStackTrace( );
	}
    }

    public static boolean checkVersion( long newVersion , String product )
    {
	return ( Build.TIME <= newVersion * 1000 && ( Build.DEVICE.equals( product ) || Build.PRODUCT.equals( product ) ) );
    }

    public static boolean checkIncVersion( String fingerprinter , String product )
    {
	return ( Build.FINGERPRINT.equals( fingerprinter ) && ( Build.DEVICE.equals( product ) || Build.PRODUCT
		.equals( product ) ) );
    }
}

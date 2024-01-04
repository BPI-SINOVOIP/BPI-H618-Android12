package com.softwinner.update.entity;

import android.content.Context;
import android.content.SharedPreferences;

import com.softwinner.protocol.net.DownloadTask;
import com.softwinner.protocol.net.DownloadTask.TaskState;
import com.softwinner.update.App;
import com.lidroid.xutils.util.LogUtils;

/**
 * OTA包信息类
 * @author greatzhang
 *
 */
public class UpdatePackageInfo
{
    //升级包信息 
    private UpdateBean mUpdateBean = null;
    // Shared preference名称
    private String mPreferenceName = "LocalPackInfo";
    private static UpdatePackageInfo mUpdatePackageInfo = null;
    private SharedPreferences sp = null;
    private SharedPreferences.Editor ed = null;

    private static final String OLD_ROM_VERSION = "OldRomVersion";

    public synchronized static UpdatePackageInfo getInstance()
    {
	if( mUpdatePackageInfo == null )
	{
	    mUpdatePackageInfo = new UpdatePackageInfo( );
	}
	return mUpdatePackageInfo;
    }

    public UpdatePackageInfo()
    {
	sp = App.getAppContext( ).getSharedPreferences( mPreferenceName , Context.MODE_PRIVATE );
	ed = App.getAppContext( ).getSharedPreferences( mPreferenceName , Context.MODE_PRIVATE ).edit( );
	mUpdateBean = new UpdateBean( );
    }

    public UpdateBean getmUpdateBean()
    {
	return mUpdateBean;
    }

    public void setUpdateBean( UpdateBean mUpdateBean )
    {
	this.mUpdateBean = mUpdateBean;
    }

    /**
     * 获取以前的版本号
     * @return 返回以前的版本号
     */
    public String getOldRomVersion()
    {
	return sp.getString( OLD_ROM_VERSION , "" );
    }

    /**
     * 在上报成功后，更新oldRomVersion
     * @param oldRomVersion 旧版本号
     */
    public void updateOldRomVerison( String oldRomVersion )
    {
	LogUtils.i( "oldRomVersion = " + oldRomVersion );
	ed.putString( OLD_ROM_VERSION , oldRomVersion );
	ed.commit( );
    }

    /**
     * 从shared preference加载数据
     */
    public void load()
    {
	mUpdateBean.setOldRomType( DeviceInfo.getRomType( ) );
	mUpdateBean.setOldRomVersion( DeviceInfo.getRomVersion( ) );
	mUpdateBean.setNewRomName( sp.getString( "NewRomName" , "" ) );
	mUpdateBean.setNewRomType( sp.getString( "NewRomType" , "" ) );
	mUpdateBean.setNewRomVersion( sp.getString( "NewRomVersion" , "" ) );
	mUpdateBean.setPackId( sp.getInt( "PackId" , 0 ) );
	mUpdateBean.setPackType( sp.getInt( "PackType" , 0 ) );
	mUpdateBean.setPackSize( sp.getInt( "PackSize" , 0 ) );
	mUpdateBean.setPackMD5( sp.getString( "PackMD5" , "" ) );
	mUpdateBean.setPackUrl( sp.getString( "PackUrl" , "" ) );
	mUpdateBean.setPubTime( sp.getString( "PubTime" , "" ) );
	mUpdateBean.setUpdatePrompt( sp.getString( "UpdatePrompt" , "" ) );
	mUpdateBean.setUpdateDesc( sp.getString( "UpdateDesc" , "" ) );
    }

    /**
     * 保存数据到shared preference
     */
    public void save()
    {
	ed.putString( "OldRomType" , mUpdateBean.getOldRomType( ) );
	ed.putString( OLD_ROM_VERSION , mUpdateBean.getOldRomVersion( ) );
	ed.putString( "NewRomName" , mUpdateBean.getNewRomName( ) );
	ed.putString( "NewRomType" , mUpdateBean.getNewRomType( ) );
	ed.putString( "NewRomVersion" , mUpdateBean.getNewRomVersion( ) );
	ed.putInt( "PackId" , mUpdateBean.getPackId( ) );
	ed.putInt( "PackType" , mUpdateBean.getPackType( ) );
	ed.putInt( "PackSize" , mUpdateBean.getPackSize( ) );
	ed.putString( "PackMD5" , mUpdateBean.getPackMD5( ) );
	ed.putString( "PackUrl" , mUpdateBean.getPackUrl( ) );
	ed.putString( "PubTime" , mUpdateBean.getPubTime( ) );
	ed.putString( "UpdatePrompt" , mUpdateBean.getUpdatePrompt( ) );
	ed.putString( "UpdateDesc" , mUpdateBean.getUpdateDesc( ) );
	ed.commit( );
    }

    public void changTaskState( int state )
    {
	ed.putInt( "state" , state );
	ed.commit( );
    }

    /**
     * 保存DownloadTask
     */
    public void saveDownloadTask( DownloadTask task )
    {
	if( task != null )
	{
	    ed.putString( "otaDownloadURL" , task.otaDownloadURL );
	    ed.putString( "fileSavePath" , task.fileSavePath );
	    ed.putLong( "progress" , task.progress );
	    ed.putLong( "fileLength" , task.fileLength );
	    ed.putString( "otaName" , task.otaName );
	    ed.putInt( "state" , task.getState( ).value( ) );
	    ed.commit( );
	}
    }

    /**
     * 获取下载任务
     * @return	放回上次下载的DownloadTask
     */
    public DownloadTask getDownloadTask()
    {
	DownloadTask task = new DownloadTask( );
	task.otaDownloadURL = sp.getString( "otaDownloadURL" , "" );
	task.fileSavePath = sp.getString( "fileSavePath" , "" );
	task.progress = sp.getLong( "progress" , 0 );
	task.fileLength = sp.getLong( "fileLength" , 0 );
	task.otaName = sp.getString( "otaName" , "" );
	task.setState( TaskState.valueOf( sp.getInt( "state" , 0 ) ) );

	return task;
    }

    /**
     * 清除shared preference
     */
    public void clear()
    {
	sp.edit( ).clear( ).commit( );
	mUpdateBean = null;
    }
}

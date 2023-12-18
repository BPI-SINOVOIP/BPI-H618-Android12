package com.softwinner.protocol.net;

import java.io.File;

import android.content.Context;
import android.os.PowerManager;
import android.text.TextUtils;

import com.softwinner.protocol.net.DownloadTask.TaskState;
import com.softwinner.update.App;
import com.softwinner.update.entity.UpdateBean;
import com.softwinner.update.entity.UpdatePackageInfo;
import com.softwinner.update.utils.Constants;
import com.softwinner.update.utils.FileUtils;
import com.softwinner.update.utils.ThreadTask;
import com.softwinner.update.utils.Utils;
import com.lidroid.xutils.HttpUtils;
import com.lidroid.xutils.exception.HttpException;
import com.lidroid.xutils.http.HttpHandler;
import com.lidroid.xutils.http.ResponseInfo;
import com.lidroid.xutils.http.callback.RequestCallBack;
import com.lidroid.xutils.util.LogUtils;

/**
 * @author greatzhang
 */
public class DownloadManager
{

    private static DownloadManager mApkDownloadManager = null;

    private static final int MAX_DOWNLOAD_THREAD = 1;

    private DownloadTaskManager mDownloadInfoManager = null;
    private HttpUtils mHttpUtils = null;
    private DownloadTask task = null;
    private UpdatePackageInfo infoDB = null;

    /**
     * 下载状态改变监听事件
     */
    private IDownloadTaskStateListener taskStateListener = null;

    public synchronized static DownloadManager getInstance()
    {
	if( null == mApkDownloadManager )
	{
	    mApkDownloadManager = new DownloadManager( );
	}
	return mApkDownloadManager;
    }

    public DownloadManager()
    {
	mDownloadInfoManager = DownloadTaskManager.getInstance( );
	//	mUpdatePackageInfo = new UpdatePackageInfo( App.getAppContext( ) );

	PowerManager pm = (PowerManager)App.getAppContext( ).getSystemService( Context.POWER_SERVICE );
	mHttpUtils = new HttpUtils( pm );
	mHttpUtils.configRequestThreadPoolSize( MAX_DOWNLOAD_THREAD );
	infoDB = UpdatePackageInfo.getInstance( );
    }

    /**
     * 开始下载 
     * @param otaInfo UpdateBean类型 
     */
    public void startDownload( UpdateBean otaInfo )
    {
	if( otaInfo == null )
	{
	    LogUtils.e( "appinfo is null!" );
	    return;
	}
	//创建下载task
	task = mDownloadInfoManager.createTask( otaInfo );
	if( task != null && task.getDownloadUrl( ) != null )
	{
	    LogUtils.i( "DownloadTask != null it`s normal" );
	    updateTaskState( task , TaskState.WAITING );
	    doDownload( task );
	    infoDB.saveDownloadTask( task );
	}
    }

    /**
     * 下载文件代码
     * @param dinfo
     */
    private void doDownload( DownloadTask dinfo )
    {
	LogUtils.i( "doDownload" );
	if( dinfo == null )
	{
	    LogUtils.e( "di == null " );
	    return;
	}

	if( dinfo.getDownloadUrl( ) == null )
	{
	    LogUtils.e( "di.getDownloadUrl( ) == null | di=" + dinfo );
	    return;
	}

	if( !isDiskFreeSpaceAvailable( dinfo.fileLength ) )
	{
	    LogUtils.e( "Disk is full! diskFreeSpace = " + FileUtils.getAvailableStorageSize( Constants.DATA_PARITION ) );
	    dinfo.setHttpHandler( null );
	    updateTaskState( dinfo , TaskState.FAILED_NOFREESPACE );
	    return;
	}

	dinfo.fileSavePath = Constants.DOWNLOAD_PATH + Utils.getFileNameFromUrl( dinfo.otaDownloadURL );

	LogUtils.i( "dinfo.fileSavePath = " + dinfo.fileSavePath );

	HttpHandler< File > handler = mHttpUtils
		.download( dinfo.getDownloadUrl( ) , dinfo.fileSavePath , true , false , new DownloadRequestCallBack(
			dinfo ) );
	dinfo.setHttpHandler( handler );

	updateTaskState( dinfo , TaskState.STARTED );

    }

    /**
     * 下载回调事件
     * @author greatzhang
     */
    private class DownloadRequestCallBack extends RequestCallBack< File >
    {
	private final DownloadTask dinfo;

	private long partialLen = 0;

	private DownloadRequestCallBack( DownloadTask downloadInfo )
	{
	    this.dinfo = downloadInfo;
	}

	@Override
	public void onStart()
	{
	    updateTaskState( dinfo , TaskState.LOADING );

	    if( dinfo.progress > 0 )
	    {
		partialLen = dinfo.progress;
	    }
	}

	@Override
	public void onStopped()
	{
	    dinfo.setHttpHandler( null );
	    updateTaskState( dinfo , TaskState.STOPPED );
	}

	@Override
	public void onLoading( long total , long current , boolean isUploading )
	{
	    if( current < partialLen )
	    {
		partialLen = current;
	    }
	    dinfo.progress = current;
	    dinfo.fileLength = total;

	    //更新进度
	    updateTaskProgress( dinfo );

	}

	@Override
	public void onSuccess( ResponseInfo< File > responseInfo )
	{
	    dinfo.setHttpHandler( null );

	    File srcFile = responseInfo.result;
	    doSuccessed( srcFile );
	}

	private void doSuccessed( File srcFile )
	{

	    updateTaskState( dinfo , TaskState.SUCCEEDED );

	    //播放下载完成提示语音
	    App.playPrompt( );

	}

	private final static int HTTPEXCEPTION_CODE_FILE_NOEXIST = 404;

	private final static int HTTPEXCEPTION_CODE_5XX_START = 500;
	private final static int HTTPEXCEPTION_CODE_5XX_END = 599;

	private final static int HTTPEXCEPTION_CODE_4XX_START = 400;
	private final static int HTTPEXCEPTION_CODE_4XX_END = 499;

	@Override
	public void onFailure( HttpException error , String msg )
	{
	    dinfo.setHttpHandler( null );

	    String errorMsg = "errorCode=" + error.getExceptionCode( ) + "|msg=" + msg;
	    LogUtils.e( "HttpException error, |" + errorMsg );

	    int errorCode = error.getExceptionCode( );
	    if( errorCode == HTTPEXCEPTION_CODE_FILE_NOEXIST )
	    {
		updateTaskState( dinfo , TaskState.FAILED_NOEXIST );
	    }
	    else if( errorCode >= HTTPEXCEPTION_CODE_4XX_START && errorCode <= HTTPEXCEPTION_CODE_4XX_END )
	    {
		//当前ngix服务器，对于416的错误是返回400的，文件有可能刚好下载完成后出错，导致当成文件损坏来处理
		File desFile = new File( dinfo.getFileSavePath( ) );
		if( desFile.length( ) == dinfo.getFileLength( ) && dinfo.getFileLength( ) > 0 )
		{
		    dinfo.progress = dinfo.getFileLength( );
		    doSuccessed( desFile );
		    return;
		}

		updateTaskState( dinfo , TaskState.FAILED_BROKEN );
	    }
	    else if( errorCode >= HTTPEXCEPTION_CODE_5XX_START && errorCode <= HTTPEXCEPTION_CODE_5XX_END )
	    {
		updateTaskState( dinfo , TaskState.FAILED_SERVER );
	    }
	    else
	    {
		updateTaskState( dinfo , TaskState.FAILED_NETWORK );

		LogUtils.e( "TaskState.FAILED_NETWORK =" + dinfo.toString( ) );

	    }
	}
    };

    /**
     * 继续下载
     * @param dinfo 下载任务信息
     */
    public void resumeDownload( DownloadTask dinfo )
    {
	if( dinfo == null )
	{
	    return;
	}
	task = dinfo;
	if( !canResume( dinfo.getState( ) ) )
	{
	    LogUtils.e( "state is error ! state=" + dinfo.getState( ).toString( ) );
	    return;
	}

	doDownload( dinfo );
    }

    private boolean canResume( TaskState state )
    {
	if( state == TaskState.STOPPED || state == TaskState.FAILED_NETWORK || state == TaskState.FAILED_SERVER
		|| state == TaskState.FAILED_NOFREESPACE )
	{
	    return true;
	}

	return false;
    }

    /**
     * 重新开始下载
     * @param dinfo 需要删除和停止的DownloadTask
     * @param otaInfo 需要重新下载ota信息
     */
    public void restartDownload( DownloadTask dinfo , UpdateBean otaInfo )
    {
	if( dinfo != null )
	{
	    //停止任务
	    stopDownload( dinfo );
	}
	//删除源文件
	deleteFile( dinfo );
	//DownloadTask重置
	if( dinfo != null )
	{
	    dinfo.reset( );
	}

	startDownload( otaInfo );
    }

    // 删除已下载文件
    private void deleteFile( DownloadTask dinfo )
    {
	if( dinfo == null )
	{
	    LogUtils.e( "dinfo is null" );
	    return;
	}

	if( TextUtils.isEmpty( dinfo.getFileSavePath( ) ) )
	    return;

	// IO操作，用子线程异步操作
	final String deleteFilePath = dinfo.getFileSavePath( );
	ThreadTask.postTask( new Runnable( )
	{
	    @Override
	    public void run()
	    {
		FileUtils.deleteFile( deleteFilePath );
		//		File file = new File( deleteFilePath );
		//		if( file.exists( ) )
		//		{
		//		    file.delete( );
		//		}
	    }
	} );
    }

    /**
     * 返回下载DownloadTask
     */
    public DownloadTask getDownloadTask()
    {
	return task;
    }

    /**
     * 是否可以停止下载
     * @param state 下载状态
     * @return true代表可以停止
     */
    private boolean canStop( TaskState state )
    {
	if( state == TaskState.STARTED || state == TaskState.LOADING )
	{
	    return true;
	}

	return false;
    }

    /**
     * 停止下载
     * @param dinfo
     */
    public void stopDownload( DownloadTask dinfo )
    {
	if( null == dinfo.getState( ) && !canStop( dinfo.getState( ) ) )
	{
	    LogUtils.e( "state is error ! state=" + dinfo.getState( ).toString( ) );
	    return;
	}

	HttpHandler< File > handler = dinfo.getHttpHandler( );
	if( handler != null && !handler.isStopped( ) )
	{
	    handler.stop( );
	}
	else
	{
	    LogUtils.e( "handler = null" );
	}

    }

    /**
     * 更新进度条
     * @param task
     */
    private void updateTaskProgress( DownloadTask task )
    {
	taskStateListener.onUpdateTaskProgress( task );
    }

    /**
     * 更新下载状态改变
     * @param info DownloadTask
     * @param state 下载状态值
     */
    private void updateTaskState( DownloadTask info , TaskState state )
    {
	info.setState( state );
	infoDB.changTaskState( state.value( ) );
	taskStateListener.onUpdateTaskState( info );
    }

    /**	
     * 监听下载状态改变事件
     * @param listener IDownloadTaskStateListener类型
     */
    public void setUpdateTaskStateListener( IDownloadTaskStateListener listener )
    {
	taskStateListener = listener;
    }

    /**
     * 判断是否有足够的空间
     * @param nSize 文件大小
     * @return true为有足够的空间
     */
    private boolean isDiskFreeSpaceAvailable( long nSize )
    {
	// 检查当前设备存储状况
	long size = FileUtils.getAvailableStorageSize( Constants.DATA_PARITION );
	if( size < nSize + 1024 * 1024 * 10 )
	{ // 预留10M空间
	    return false;
	}

	return true;
    }
}

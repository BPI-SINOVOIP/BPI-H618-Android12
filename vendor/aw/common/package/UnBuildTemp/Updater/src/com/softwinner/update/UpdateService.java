package com.softwinner.update;

import java.io.File;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.text.TextUtils;
import android.widget.RemoteViews;

import com.softwinner.protocol.controller.ProtocolListener.ERROR;
import com.softwinner.protocol.controller.ReportOta;
import com.softwinner.protocol.controller.UpdateProtocol;
import com.softwinner.protocol.net.DownloadManager;
import com.softwinner.protocol.net.DownloadTask;
import com.softwinner.protocol.net.DownloadTask.TaskState;
import com.softwinner.protocol.net.IDownloadTaskStateListener;
import com.softwinner.update.entity.DeviceInfo;
import com.softwinner.update.entity.UpdateBean;
import com.softwinner.update.entity.UpdatePackageInfo;
import com.softwinner.update.utils.Constants;
import com.softwinner.update.utils.CustomNotification;
import com.softwinner.update.utils.FileUtils;
import com.softwinner.update.utils.NetUtils;
import com.softwinner.update.utils.StringUtils;
import com.softwinner.update.utils.Utils;
import com.lidroid.xutils.http.callback.RequestCallBack;
import com.lidroid.xutils.util.LogUtils;

/**
 * 用于后台检查更新、下载更新、保存状态的Service
 * @author Greatzhang
 *
 */
public class UpdateService extends Service implements IDownloadTaskStateListener
{
    private Context mContext;
    // 当前的状态
    public static int mState = Constants.STATE_READY;
    // 当前电量(0-100)
    public static int mBatLevel = 0;
    public static int mPlugType = -1;
    // 已下载升级包信息
    private UpdatePackageInfo mPackageInfo;
    private ServerCallBack mServerCallBack;
    private NotificationManager mNotificationManager;
    private RequestCallBack< File > mRequestCallBack;
    private DownloadManager mDownloadManager = null;

    private boolean isDownloading = false; //是否进行下载操作
    private boolean isRestartDownload = false;

    public static final int DOWNLOAD_CONNECTING = 0; //下载链接
    public static final int DOWNLOAD_DOWNING = 1; //下载中
    public static final int DOWNLOAD_PAUSE = 2; //下载暂停
    public static final int DOWNLOAD_FAIL = 3; //下载错误
    public static final int DOWNLOAD_FINSH = 4; //下载完毕
    public static final int DWONLOAD_CONTINUE_RECHECK = 3;

    int mCurrentDownPersent = 0; //当前下载进度
    String CurrentDownloadingStr = "";
    private boolean mDealyCheckFlag = false;
    private boolean isReportSuccess = true;

    public interface ServerCallBack
    {
	public void UpdateResponse( int errcode , Object msg );

	public void NetworkNotAviable();
    }

    @Override
    public IBinder onBind( Intent arg0 )
    {
	LogUtils.d( "onBind" );
	if( mMyMsgHandler == null )
	{
	    LogUtils.e( "mMyMsgHandler is null" );
	}
	return mMyMsgHandler.getBinder( );
    }

    /*
     * 比较本地缓冲的版本和服务器返回的版本信息是否一致
     */
    private int comparePackage( UpdateBean pack , String localVersion )
    {
	int informationModed = ERROR.NO_ERROR;
	String localNewRomName = mPackageInfo.getmUpdateBean( ).getNewRomName( );
	LogUtils.i( localVersion + " localNewRomName " + localNewRomName );
	//检查是否有本地缓冲
	if( localNewRomName != null  )
	{
		LogUtils.d("local localNewRomName no name");
	    UpdateBean localPack = (UpdateBean)mPackageInfo.getmUpdateBean( );
	    String localDownUrl = localPack.getPackUrl( );
	    String newDownUrl = pack.getPackUrl( );
	    String newVersion = pack.getNewRomVersion( );
	    if( Utils.compareVersion( newVersion , localVersion ) == 1 ) //返回1表示有更新的版本
	    {
		LogUtils.d( " Utils.compareVersion( newVersion , localVersion ) == 1" );
		informationModed = ERROR.PACK_INFORMATION_MODIFYED;
	    }
	    //检查下载地址是否有变化
	    if( !Utils.comareString( newDownUrl , localDownUrl ) )
	    {
		//url 发生变化
		LogUtils.d( "Not Same Url" );
		informationModed = ERROR.DOWNLOAD_URL_MODIFYED;
	    }
	}
	else
	{
	    mPackageInfo.load( );
	    if( !TextUtils.isEmpty( mPackageInfo.getmUpdateBean( ).getNewRomName( ) ) )
	    {
		LogUtils.d( "Read Local Cache" );
		UpdateBean localPack = (UpdateBean)mPackageInfo.getmUpdateBean( );
		String localDownUrl = localPack.getPackUrl( );
		String newDownUrl = pack.getPackUrl( );
		String newVersion = pack.getNewRomVersion( );
		LogUtils.d( "localDownUrl:" + localDownUrl );
		LogUtils.d( "localVersion:" + localVersion );
		LogUtils.d( "newDownUrl:" + newDownUrl );
		LogUtils.d( "newVersion:" + newVersion );
		if( Utils.compareVersion( newVersion , localVersion ) == 1 )
		{
		    LogUtils.d( "Server call back version is high" );
		    informationModed = ERROR.PACK_INFORMATION_MODIFYED;
		}
		//检查下载地址是否有变化
		if( !Utils.comareString( newDownUrl , localDownUrl ) )
		{
		    //url 发生变化
		    LogUtils.d( "Not Same Url" );
		    informationModed = ERROR.DOWNLOAD_URL_MODIFYED;
		}
	    }
	}
	return informationModed;
    }
    public void ShowAlertDialog(){
        Intent intent = new Intent();  
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);   
        intent.setClass(getApplicationContext(),UpdateActivity.class);  
        startActivity(intent);  
    }
    
    //检测返回
    UpdateProtocol.HandlResponse mCheckResultHandler = new UpdateProtocol.HandlResponse( )
    {

	@Override
	public void handleResponseBean( int errcode , Object msg )
	{
	    LogUtils.d( "handleResponseBean errcode:" + errcode );
	    boolean isUpdateCache = false;
	    int packErrcode = errcode;
	    mState = Constants.STATE_READY;
	    //返回正常开始进行处理
	    if( packErrcode == ERROR.NO_ERROR )
	    {
		UpdateBean uMsg = (UpdateBean)msg;
		LogUtils.d( "uMsg = " + ( (UpdateBean)msg ) );

		if( uMsg.getRescode( ) != 0 || uMsg.getUpdateType( ) != 1 ) //不存在更新
		{
		    //mServerCallBack.UpdateResponse( ERROR.NOT_FIND_AVIABLE_PACK , null );
		    serverCallBack( ERROR.NOT_FIND_AVIABLE_PACK , null );
		    return;
		}
		//比较本地缓冲的版本是否一致
		int compareResult = comparePackage( (UpdateBean)msg , mPackageInfo.getmUpdateBean( ).getNewRomVersion( )/*DeviceUtil.getRomVersion( )*/);
		if( compareResult == ERROR.PACK_INFORMATION_MODIFYED || compareResult == ERROR.DOWNLOAD_URL_MODIFYED )
		{
		    LogUtils.d( "pack information not match" );
		    isUpdateCache = true;
		}
		else
		{
		    LogUtils.d( "pack information match" );
		}
		//本地信息和服务器信息不一致发生变化
		if( isUpdateCache )
		{
		    //升级包信息发生变化
		    if( Constants.DEBUG )
			LogUtils.d( "Pack Information Modifyded" );
		    packErrcode = compareResult;

		}
		//服务器返回的信息存储到SharedPreferences中
		setUpdateInfo( (UpdateBean)msg );
	    }
	    LogUtils.d( "packErrcode " + packErrcode + "   mPackageInfo = " + mPackageInfo );
	    serverCallBack( packErrcode , mPackageInfo );

	    //通知栏提示有最新版本
	    if( ( mDealyCheckFlag && packErrcode == ERROR.PACK_INFORMATION_MODIFYED )
		    || ( mDealyCheckFlag && packErrcode == ERROR.DOWNLOAD_URL_MODIFYED ) )
	    {
		LogUtils.i( "ShowNotification  --- packErrcode = " + packErrcode + "mDealyCheckFlag = "
			+ mDealyCheckFlag );
		mDealyCheckFlag = false;
		//if( mPackageInfo != null )
		    ShowNotification( ((UpdateBean) msg).getUpdatePrompt() );
		    ShowAlertDialog();
	    }
	    if( isRestartDownload( ) )
	    {
		setRestartDownload( false );
		mHandler.sendEmptyMessage( MSG_RESTART_DOWNLOAD );
	    }
	}

    };

    private void serverCallBack( int errcode , Object msg )
    {
	if( mServerCallBack != null )
	{
	    mServerCallBack.UpdateResponse( errcode , msg );
	}
    }

    private static final int MSG_RESTART_DOWNLOAD = 1000;

    private Handler mHandler = new Handler( )
    {
	public void handleMessage( Message msg )
	{
	    LogUtils.i( "MSG.WHAT = " + msg.what );
	    switch ( msg.what )
	    {
		case MSG_RESTART_DOWNLOAD :
		    mDownloadManager.restartDownload( mDownloadManager.getDownloadTask( ) , mPackageInfo
			    .getmUpdateBean( ) );
		    break;

		default :
		    break;
	    }
	};
    };

    //显示通知
    private void ShowNotification( String message )
    {
	Intent notificationIntent = new Intent( this , UpdateActivity.class );
	notificationIntent.setAction( Constants.ACTION_CHECK );
	CustomNotification mNotif = new CustomNotification( this , notificationIntent , message ,
		Constants.CUSTOM_NOTIFICATION_CHECK_AVAIABLE , Constants.CUSTOM_NOTIFICATION_CHECK_AVAIABLE_REQ );
	mNotif.showCustomNotification( );
    }

    //TODO  状态栏显示下载相关的信息
    private void displayDownNotificationMessage( int flag )
    {
	// Notification的Intent，即点击后转向的Activity
	Intent notificationIntent1 = new Intent( this , UpdateActivity.class );
	notificationIntent1.addFlags( Intent.FLAG_ACTIVITY_SINGLE_TOP );
	if( flag == DOWNLOAD_DOWNING )
	{
	    notificationIntent1.setAction( Constants.ACTION_DOWNLOAD );
	}
	else if( flag == DOWNLOAD_PAUSE )
	{
	    notificationIntent1.setAction( Constants.ACTION_DOWNLOAD_PAUSE );
	}
	else if( flag == DOWNLOAD_FAIL )
	{
	    notificationIntent1.setAction( Constants.ACTION_DOWNLOAD_FAIL );
	}
	else if( flag == DOWNLOAD_FINSH )
	{
	    if( !isDownloading )
		notificationIntent1.setAction( Constants.ACTION_DOWNLOAD_FINSH );
	}
	//		addflag设置跳转类型
	PendingIntent contentIntent1 = PendingIntent.getActivity( this , 0 , notificationIntent1 , 0 );
	// 创建Notifcation对象，设置图标，提示文字
	Notification notification = new Notification( R.drawable.ic_launcher , "DownLoadManager" , System
		.currentTimeMillis( ) );// 设定Notification出现时的声音，一般不建议自定义
	if( flag == DOWNLOAD_FINSH )
	{
	    notification.defaults |= Notification.DEFAULT_SOUND;// 设定是否振动
	    notification.defaults |= Notification.DEFAULT_VIBRATE;
	}
	else
	    notification.flags |= Notification.FLAG_ONGOING_EVENT;

	LogUtils.d( "displayDownNotificationMessage flag = " + flag );
	//创建一个自定义的Notification，可以使用RemoteViews 要定义自己的扩展消息，首先要初始化一个RemoteViews对象，然后将它传递给Notification的contentView字段，再把PendingIntent传递给contentIntent字段
	RemoteViews contentView = new RemoteViews( getPackageName( ) , R.layout.custom_message_notification );
	if( flag == DOWNLOAD_PAUSE )
	    contentView.setTextViewText( R.id.custom_notif_title , mContext.getString( R.string.download_pause_notif ) );
	else if( flag == DOWNLOAD_FINSH )
	    contentView.setTextViewText( R.id.custom_notif_title , mContext
		    .getString( R.string.download_finsh_notif_title ) );
	else if( flag == DOWNLOAD_FAIL )
	    contentView.setTextViewText( R.id.custom_notif_title , mContext
		    .getString( R.string.download_fail_connect_error ) );
	else
	    contentView.setTextViewText( R.id.custom_notif_title , mContext.getString( R.string.now_downloading_pack ) );
	if( flag == DOWNLOAD_CONNECTING )
	    contentView
		    .setTextViewText( R.id.custom_notif_message , mContext.getString( R.string.connecting_download ) );
	else if( flag == DOWNLOAD_FINSH )
	    contentView.setTextViewText( R.id.custom_notif_message , mContext
		    .getString( R.string.download_finsh_notif_content ) );
	else
	{
	    contentView.setTextViewText( R.id.custom_notif_message , CurrentDownloadingStr );
	}
	contentView.setProgressBar( R.id.notif_pack_downpb , 100 , mCurrentDownPersent , false );

	notification.contentView = contentView;
	notification.contentIntent = contentIntent1;

	mNotificationManager.notify( Constants.CUSTOM_NOTIFICATION_DOWN , notification );
    }

    //检查更新
    private void CheckUpdate()
    {
	if( NetUtils.isNetworkconnected( mContext ) )
	{
	    LogUtils.d( "CheckUpdate  DeviceUtil.getRomVersion( )  = " + DeviceInfo.getRomVersion( ) );
	    UpdateProtocol update = new UpdateProtocol( DeviceInfo.getRomName( ) , DeviceInfo.getRomType( ) ,
		    DeviceInfo.getRomVersion( ) , mCheckResultHandler );
	    update.doRequest( );
	}
    }

    //是否有升级
    private boolean initLocalPack()
    {
	boolean result = false;
	//检测是否有包
	if( !TextUtils.isEmpty( mPackageInfo.getmUpdateBean( ).getNewRomName( ) ) )
	{//如果有本地检测记录
	    String newVersion = mPackageInfo.getmUpdateBean( ).getNewRomVersion( );
	    String oldVersion = DeviceInfo.getRomVersion( );
	    LogUtils.d( "newVersion:" + newVersion + " oldVersion:" + oldVersion );
	    if( Utils.compareVersion( newVersion , oldVersion ) == 0 )
	    {
		result = true;
	    }
	    else
	    {
		result = false;
	    }
	}
	else
	    result = false;
	return result;
    }

    private void syncBootInformation()
    {
	mPackageInfo.load( );
	if( initLocalPack( ) )
	{
	    LogUtils.d( "Update Sucess" );
	    //	    ReportStatus( );
	}
	else
	{
	    LogUtils.d( "Nothing do" );
	}
    }

    @Override
    public void onRebind( Intent intent )
    {
	LogUtils.d( "onRebind" );
	super.onRebind( intent );
    }

    @Override
    public boolean onUnbind( Intent intent )
    {
	boolean ret = super.onUnbind( intent );
	LogUtils.d( "onUnbind:" + ret );
	return true;
    }

    @Override
    public void onCreate()
    {

	if( Constants.DEBUG )
	    LogUtils.d( "onCreate" );
	super.onCreate( );
	mContext = getBaseContext( );

	mDownloadManager = DownloadManager.getInstance( ); //初始化下载Manager
	mDownloadManager.setUpdateTaskStateListener( this );
	mNotificationManager = (NotificationManager)mContext.getSystemService( Context.NOTIFICATION_SERVICE );

	mPackageInfo = UpdatePackageInfo.getInstance( );
	TaskState state = mPackageInfo.getDownloadTask( ).getState( );
	if( state != null )
	{
	    if( state != TaskState.SUCCEEDED && state != TaskState.WAITING )
	    {
		mPackageInfo.changTaskState( TaskState.STOPPED.value( ) );
	    }
	}

    }

    @Override
    public int onStartCommand( Intent intent , int flags , int startId )
    {
	LogUtils.i( "onStartCommand flags:" + flags + " startId" + startId );
	if( intent == null )
	    return 0;
	mContext = getBaseContext( );
	int cmd = intent.getIntExtra( Constants.KEY_START_COMMAND , 0 );
	if( Constants.DEBUG )
	    LogUtils.i( "get a start cmd : " + cmd );
	switch ( cmd )
	{
	    case Constants.START_COMMAND_START_CHECKING :
	    case Constants.START_COMMAND_START_CONNECT_CHANGE :
		LogUtils.i( "Start Dealy Checking!" );
		mDealyCheckFlag = true;
		//上报安装成功
		reportInstallSucceed( );
		if( Utils.isFastDouble( ) )
		{
		    CheckUpdate( );
		}
		break;
	    //检测网络链接
	    case Constants.START_COMMAND_START_CHECK_ACTIVITY_CONNECTED :
		LogUtils.i( "Check Activity Connected" );
		if( isDownloading )
		{
		    LogUtils.i( "is downloading" );
		}
		break;
	    //开机自启动
	    case Constants.START_COMMAND_START_BOOT_COMPLET :
		LogUtils.i( "Start from BootComplet" );
		mDealyCheckFlag = true;
		syncBootInformation( );
		break;
	    default :
		break;

	}
	return 0;
    }

    @Override
    public void onDestroy()
    {
	LogUtils.e( "Service is onDestroy" );
	if( mPackageInfo != null )
	{
	    mPackageInfo.save( ); //保存包信息

	    mPackageInfo.saveDownloadTask( mDownloadManager.getDownloadTask( ) ); //退出时需要将下载信息进行保存
	}
	mPackageInfo = null;

	super.onDestroy( );
    }

    private Messenger mMyMsgHandler = new Messenger( new Handler( )
    {
	@SuppressWarnings( "unchecked" )
	public void handleMessage( Message msg )
	{
	    LogUtils.i( "msg.arg1:" + msg.arg1 + "-----msg.arg2:" + msg.arg2 );
	    LogUtils.i( "msg.what:" + msg.what );
	    switch ( msg.what )
	    {
		case Constants.MSG_SERVICE_CONNECTED :
		case Constants.MSG_CHECK_UPDATE :
		    if( NetUtils.isNetworkconnected( mContext ) )
		    {
			mState = Constants.STATE_CHECKING;
			CheckUpdate( );
		    }
		    else
		    {
			if( mServerCallBack != null )
			    mServerCallBack.NetworkNotAviable( );
		    }
		    break;
		case Constants.MSG_START_DOWN_PACK :
		    LogUtils.d( "MSG_START_DOWN_PACK  first download" );
		    mNotificationManager.cancel( Constants.CUSTOM_NOTIFICATION_CHECK_AVAIABLE );

		    mDownloadManager.startDownload( mPackageInfo.getmUpdateBean( ) ); //开始下载PACK	

		    break;

		case Constants.MSG_RESUME_DOWNLOAD : //恢复下载
		    LogUtils.i( "MSG_RESUME_DOWNLOAD resume download" );
		    mNotificationManager.cancel( Constants.CUSTOM_NOTIFICATION_CHECK_AVAIABLE );

		    mDownloadManager.resumeDownload( getDownloadTask( ) );
		    break;

		case Constants.MSG_PAUSE_DOWN_PACK :
		    LogUtils.i( " MSG_PAUSE_DOWN_PACK paused download " );
		    mState = Constants.STATE_DOWNLOADPAUSE;
		    //		    mDownHandler.stop( );
		    mDownloadManager.stopDownload( mDownloadManager.getDownloadTask( ) );
		    displayDownNotificationMessage( DOWNLOAD_PAUSE );
		    break;

		case Constants.MSG_AFRESH_DOWNLOAD : //重新下载
		    restartDownload( );//开始重新下载需要在跟新完成之后重新下载
		    break;
		    
		case Constants.MSG_CLEAN_DOWNLOAD: //清除原有的下载包
			FileUtils.deleteFile( Constants.DOWNLOAD_PATH );
			break;
			
		case Constants.MSG_FINSH_DOWN_PACK :
		    mState = Constants.STATE_DOWNLOADED;
		    break;
		case Constants.MSG_ACTIVITY_EXIT :
		    mRequestCallBack = null;
		    break;
		case Constants.REGIST_REQUEST_CALLBACK :
		    mRequestCallBack = (RequestCallBack< File >)msg.obj;
		    break;

		case Constants.REGIST_SERVER_CALLBACK :
		    mServerCallBack = (ServerCallBack)msg.obj;
		    break;

		default :
		    super.handleMessage( msg );
		    break;
	    }
	}
    } );

    private DownloadTask getDownloadTask()
    {
	DownloadTask task = null;
	if( null == mDownloadManager.getDownloadTask( ) )
	{
	    task = mPackageInfo.getDownloadTask( ); //重保存的XML中获取
	}
	else
	{
	    task = mDownloadManager.getDownloadTask( );
	}
	return task;
    }

    /**
     * 重新下载包
     * @param otaInfo 需要重新下载的ota信息
     */
    private void restartDownload()
    {
	//重新下载之前需要先把以前的文件进行删除
	FileUtils.deleteFile( Constants.DOWNLOAD_PATH );

	setRestartDownload( true );
	//重新开始检查数据,在检查完数据后开始重新
	CheckUpdate( );

    }

    /**
     * 取消下载
     */
    private void DeviceShutDownCancelDownload()
    {
	if( Constants.DEBUG )
	    LogUtils.d( "DeviceShutDownCancelDownload" );

    }

    /**
     * 由检查更新线程设置更新信息
     * @param resp
     */
    public void setUpdateInfo( UpdateBean resp )
    {
	if( resp.getRescode( ) != 0 )
	    return;
	mPackageInfo.setUpdateBean( resp );
	// 这个时候可以写文件
	mPackageInfo.save( );
    }

    /**
     * 下载进度监听事件
     */
    @Override
    public void onUpdateTaskProgress( DownloadTask task )
    {
	isDownloading = true;
	long current = task.progress;
	long total = task.fileLength;

	//	LogUtils.d( "current = " + current + " total = " + total );
	mCurrentDownPersent = (int) ( current * 100 / total );

	CurrentDownloadingStr = String
		.format( mContext.getResources( ).getString( R.string.current_downloaded ) , StringUtils
			.byteToString( current ) , StringUtils.byteToString( total ) );
	displayDownNotificationMessage( DOWNLOAD_DOWNING );
	if( mRequestCallBack != null )
	    mRequestCallBack.onLoading( total , current , true );
    }

    /**
     * 下载状态改变事件
     */
    @Override
    public void onUpdateTaskState( DownloadTask task )
    {
	LogUtils.i( "task = " + task.getState( ).toString( ) );
	switch ( task.getState( ) )
	{
	    case WAITING :
		//上报下载次数
		reportLoadStatus( ReportOta.REPORT_DOWNLOAD_ACTION , ReportOta.REPORT_STATE_START , "" );
		break;

	    case STARTED :
	    case LOADING : //正在下载或者准备下载
		LogUtils.i( "task = " + task.toString( ) );
		if( mRequestCallBack != null )
		    mRequestCallBack.onStart( );
		displayDownNotificationMessage( DOWNLOAD_CONNECTING );
		break;

	    case SUCCEEDED :
		isDownloading = false;
		displayDownNotificationMessage( DOWNLOAD_FINSH );
		if( mRequestCallBack != null )
		    mRequestCallBack.onSuccess( null );
		reportLoadStatus( ReportOta.REPORT_DOWNLOAD_ACTION , ReportOta.REPORT_STATE_SUCCESS , task.getState( )
			.toString( ) );
		break;
	    case STOPPED :
		mRequestCallBack.onStopped( );
		break;

	    case FAILED_NETWORK :
	    case FAILED_SERVER :
	    case FAILED_NOFREESPACE :
	    case FAILED_BROKEN :
	    case DELETED :
	    case FAILED_NOEXIST :
		LogUtils.e( "=========================== error" + task.getState( ) );
		reportLoadStatus( ReportOta.REPORT_DOWNLOAD_ACTION , ReportOta.REPORT_STATE_ERROR , task.getState( )
			.toString( ) );
		mRequestCallBack.onStopped( ); //以上错误需要重新开始下载
		mRequestCallBack.onFailure( null , task.getState( ).toString( ) );
		break;
	    default :
		LogUtils.e( "Download error is not know" );
		break;
	}

    }

    /**
     * 上报服务器状态
     * @param action 30001表示下载，30002表示安装
     * @param result 1表示请求， 2表示失败， 3表示成功
     * @param error 错误的原因
     */
    private void reportLoadStatus( int action , int result , String msg )
    {
	if( !NetUtils.isNetworkconnected( mContext ) )
	{
	    return;
	}
	if( mPackageInfo == null || mPackageInfo.getmUpdateBean( ) == null )
	{
	    return;
	}
	ReportOta report = new ReportOta( action , result , mPackageInfo.getmUpdateBean( ).getNewRomName( ) + "."
		+ mPackageInfo.getmUpdateBean( ).getNewRomType( ) , mPackageInfo.getmUpdateBean( ).getNewRomVersion( ) ,
		DeviceInfo.getRomVersion( ) , mPackageInfo.getmUpdateBean( ).getPackType( ) + "" , msg );
	report.doRequest( );
    }

    /**
     * 上报安装成功
     */
    private synchronized void reportInstallSucceed()
    {
	if( !NetUtils.isNetworkconnected( mContext ) || !isReportSuccess )
	{
	    return;
	}

	String oldRomVersion = mPackageInfo.getOldRomVersion( );
	LogUtils.i( "oldRomVersion = " + oldRomVersion );
	if( oldRomVersion == null || oldRomVersion.equals( "" ) )
	{
	    mPackageInfo.updateOldRomVerison( DeviceInfo.getRomVersion( ) );
	    ReportOta report = new ReportOta( ReportOta.REPORT_UPDATE_ACTION , ReportOta.REPORT_STATE_SUCCESS ,
		    DeviceInfo.getRomName( ) + "." + DeviceInfo.getRomType( ) , DeviceInfo.getRomVersion( ) , "" , "" ,
		    "" );
	    report.doRequest( );
	}
	else
	{
	    if( Utils.compareVersion( DeviceInfo.getRomVersion( ) , oldRomVersion ) == 1 )
	    {
		reportInstall( oldRomVersion );
		mPackageInfo.updateOldRomVerison( DeviceInfo.getRomVersion( ) );
	    }
	    else
	    {
		LogUtils.d( "not report install success" );
	    }
	}
	isReportSuccess = false;
    }

    private void reportInstall( String oldVersion )
    {
	LogUtils.i( "ReportStatus" );
	ReportOta report = new ReportOta( ReportOta.REPORT_UPDATE_ACTION , ReportOta.REPORT_STATE_SUCCESS , DeviceInfo
		.getRomName( )
		+ "." + DeviceInfo.getRomType( ) , DeviceInfo.getRomVersion( ) , oldVersion , mPackageInfo
		.getmUpdateBean( ).getPackType( )
		+ "" , "" );
	report.doRequest( );
    }

    private boolean isRestartDownload()
    {
	return isRestartDownload;
    }

    private void setRestartDownload( boolean isRestartDownload )
    {
	this.isRestartDownload = isRestartDownload;
    }

}

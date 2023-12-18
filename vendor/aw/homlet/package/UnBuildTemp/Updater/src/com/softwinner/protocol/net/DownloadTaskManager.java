package com.softwinner.protocol.net;

import com.softwinner.protocol.net.DownloadTask.TaskState;
import com.softwinner.update.entity.UpdateBean;
import com.lidroid.xutils.util.LogUtils;

public class DownloadTaskManager
{

    private static DownloadTaskManager mDownloadInfoManager;

    private DownloadTaskManager()
    {}

    public static DownloadTaskManager getInstance()
    {
	if( mDownloadInfoManager == null )
	{
	    mDownloadInfoManager = new DownloadTaskManager( );
	}
	return mDownloadInfoManager;
    }

    //创建下载任务
    public DownloadTask createTask( UpdateBean otaInfo )
    {
	if( otaInfo == null )
	{
	    LogUtils.e( "otaInfo is null!" );
	    return null;
	}

	DownloadTask dinfo = new DownloadTask( );

	dinfo.otaName = otaInfo.getNewRomName( );
	dinfo.progress = 0;
	dinfo.setState( TaskState.WAITING );

	dinfo.otaDownloadURL = otaInfo.getPackUrl( );
	dinfo.fileLength = otaInfo.getPackSize( );

	return dinfo;
    }

}

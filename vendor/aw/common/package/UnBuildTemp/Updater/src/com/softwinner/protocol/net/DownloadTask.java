package com.softwinner.protocol.net;

import java.io.File;

import com.lidroid.xutils.http.HttpHandler;
import com.lidroid.xutils.util.LogUtils;

public class DownloadTask
{

    public DownloadTask()
    {}

    public HttpHandler< File > handler;
    public String otaDownloadURL;
    public String fileSavePath;
    public long progress;
    public long fileLength;
    public String otaName;
    // 当前的下载状态̬
    private TaskState state;

    //从什么入口启动的下载，用于统计
    public int nFromPos = -1;

    //for debug
    public String stateMsg;

    public TaskState getState()
    {
	return state;
    }

    public enum TaskState
    {
	WAITING( 0 ) , STARTED( 1 ) , LOADING( 2 ) , STOPPED( 3 ) , SUCCEEDED( 4 ) , DELETED( 5 ) , FAILED_BROKEN( -1 ) , FAILED_NOEXIST( -2 ) , FAILED_NETWORK( -3 ) , FAILED_SERVER( -4 ) , FAILED_NOFREESPACE( -5 );

	private int value = 0;

	TaskState( int value )
	{
	    this.value = value;
	}

	public static TaskState valueOf( int value )
	{
	    switch ( value )
	    {
		case 0 :
		    return WAITING;
		case 1 :
		    return STARTED;
		case 2 :
		    return LOADING;
		case 3 :
		    return STOPPED;
		case 4 :
		    return SUCCEEDED;
		case 5 :
		    return DELETED;

		case -1 :
		    return FAILED_BROKEN;
		case -2 :
		    return FAILED_NOEXIST;
		case -3 :
		    return FAILED_NETWORK;
		case -4 :
		    return FAILED_SERVER;
		case -5 :
		    return FAILED_NOFREESPACE;
		default :
		    return null;
	    }
	}

	public int value()
	{
	    return this.value;
	}

	public static TaskState ErrorValueOf( String task )
	{
	    TaskState state = null;
	    if( task.equals( "FAILED_BROKEN" ) )
	    {
		state = FAILED_BROKEN;
	    }
	    else if( task.equals( "FAILED_NOEXIST" ) )
	    {
		state = FAILED_NOEXIST;
	    }
	    else if( task.equals( "FAILED_NETWORK" ) )
	    {
		state = FAILED_NETWORK;
	    }
	    else if( task.equals( "FAILED_SERVER" ) )
	    {
		state = FAILED_SERVER;
	    }
	    else if( task.equals( "FAILED_NOFREESPACE" ) )
	    {
		state = FAILED_NOFREESPACE;
	    }
	    else
	    {
		LogUtils.e( "taskError = " + task );
	    }
	    return state;

	}

	@Override
	public String toString()
	{
	    switch ( value )
	    {
		case 0 :
		    return "WAITING";
		case 1 :
		    return "STARTED";
		case 2 :
		    return "LOADING";
		case 3 :
		    return "STOPPED";
		case 4 :
		    return "SUCCEEDED";
		case 5 :
		    return "DELETED";

		case -1 :
		    return "FAILED_BROKEN";
		case -2 :
		    return "FAILED_NOEXIST";
		case -3 :
		    return "FAILED_NETWORK";
		case -4 :
		    return "FAILED_SERVER";
		case -5 :
		    return "FAILED_NOFREESPACE";
		default :
		    return "state_none";
	    }
	}
    }

    @Override
    public String toString()
    {
	return "DownloadInfo [fileSavePath=" + fileSavePath + ", url=" + otaDownloadURL + " otaName=" + otaName + ", totalSize=" + fileLength + ", dlSize=" + progress + ", state=" + state.toString( )
		+ "]";
    }

    public void reset()
    {
	progress = 0;
	//fileLength = 0;
	state = TaskState.WAITING;
	fileSavePath = null;
    }

    public void resetForDeleted()
    {
	progress = 0;
	state = TaskState.DELETED;
	handler = null;
	fileSavePath = null;
    }

    public HttpHandler< File > getHttpHandler()
    {
	return handler;
    }

    public void setHttpHandler( HttpHandler< File > handler )
    {
	this.handler = handler;
    }

    public void setState( TaskState state )
    {
	this.state = state;
    }

    public String getDownloadUrl()
    {
	return otaDownloadURL;
    }

    public void setDownloadUrl( String downloadUrl )
    {
	this.otaDownloadURL = downloadUrl;
    }

    public String getFileSavePath()
    {
	return fileSavePath;
    }

    public void setFileSavePath( String fileSavePath )
    {
	this.fileSavePath = fileSavePath;
    }

    public long getProgress()
    {
	return progress;
    }

    public void setProgress( long progress )
    {
	this.progress = progress;
    }

    public long getFileLength()
    {
	return fileLength;
    }

    public void setFileLength( long fileLength )
    {
	this.fileLength = fileLength;
    }
}

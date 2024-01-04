/**
 * 
 */
package com.softwinner.protocol.net;

/**
 * @author springsu
 *
 */
public interface IDownloadTaskStateListener
{
    /**
     *  需要收到task通知的地方要实现
     */
    abstract public void onUpdateTaskProgress( DownloadTask task );

    /**
     *需要收到task通知的地方要实现
     */
    abstract public void onUpdateTaskState( DownloadTask task );

}

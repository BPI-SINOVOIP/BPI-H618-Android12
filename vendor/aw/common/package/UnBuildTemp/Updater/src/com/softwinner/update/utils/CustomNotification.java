package com.softwinner.update.utils;

import com.softwinner.update.R;
import com.lidroid.xutils.util.LogUtils;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.widget.RemoteViews;
/**
 * 自定义通知栏
 * @author Nurmuhammad
 *
 */
public class CustomNotification {
	private NotificationManager mNotificationManager;
	private Context mContext;
	Notification mNotification;
	RemoteViews mRemoteContentViews = null;
	Intent mNotificationIntent;
	private int mCustomNotifID;
	private CharSequence mMessageContent = null;
	int mRequestCode;
	public CustomNotification(Context context){
		mContext=context;
		mNotificationManager = (NotificationManager)mContext.getSystemService(Context.NOTIFICATION_SERVICE);
		mNotification = new Notification();
	}
	public CustomNotification(Context context,Intent NotificationIntent,RemoteViews RemoteContentViews,
			int NotifID,int requestCode){
		mContext=context;
		mNotificationManager = (NotificationManager)mContext.getSystemService(Context.NOTIFICATION_SERVICE);
		mNotificationIntent = NotificationIntent;
		mRemoteContentViews = RemoteContentViews;
		mNotification = new Notification(R.drawable.icon_detect2,mContext.getString(R.string.find_aviable_pack_notification_title),System.currentTimeMillis());
		mCustomNotifID = NotifID;
		mRequestCode = requestCode;
	}
	public CustomNotification(Context context,Intent NotificationIntent,String MessageContent,
			int NotifID,int requestCode){
		mContext=context;
		mNotificationManager = (NotificationManager)mContext.getSystemService(Context.NOTIFICATION_SERVICE);
		mNotificationIntent = NotificationIntent;
		mNotification = new Notification(R.drawable.icon_detect2,mContext.getString(R.string.find_aviable_pack_notification_title),System.currentTimeMillis());
		mCustomNotifID = NotifID;
		mMessageContent = MessageContent;
		mRequestCode = requestCode;
	}
	private Intent getNotificationIntent() {
		return mNotificationIntent;
	}
	private int getCustomNotifID() {
		return mCustomNotifID;
	}
	private void setmCustomNotifID(int CustomNotifID) {
		this.mCustomNotifID = CustomNotifID;
	}
	public void setNotificationIntent(Intent NotificationIntent) {
		this.mNotificationIntent = NotificationIntent;
	}

	private RemoteViews getRemoteContentViews() {
		return mRemoteContentViews;
	}
	private int getRequestCode() {
		return mRequestCode;
	}
	private void setRequestCode(int RequestCode) {
		this.mRequestCode = RequestCode;
	}
	public void setRemoteContentViews(RemoteViews RemoteContentViews) {
		this.mRemoteContentViews = RemoteContentViews;
	}
	private CharSequence getMessageContent() {
		return mMessageContent;
	}
	private void setMessageContent(String MessageContent) {
		this.mMessageContent = MessageContent;
	}
	public void showCustomNotification(){
		PendingIntent contentIntent = PendingIntent.getActivity(mContext,getRequestCode(), getNotificationIntent(), 0);
		if(getRemoteContentViews()!=null)
			mNotification.contentView = getRemoteContentViews();
		mNotification.contentIntent = contentIntent;
		mNotification.defaults |= Notification.DEFAULT_SOUND;//设置声音提醒
		mNotification.flags = Notification.FLAG_NO_CLEAR;
		mNotification.flags |= Notification.FLAG_ONGOING_EVENT;
		mNotification.when = System.currentTimeMillis();
		if(getMessageContent()!=null)
			mNotification.setLatestEventInfo(mContext, mContext.getString(R.string.find_aviable_pack_notification_title), getMessageContent(), contentIntent);
		mNotificationManager.notify(getCustomNotifID(), mNotification);
	}
	public void closeNotification(int notifID){
		mNotificationManager.cancel(notifID);
	}
}

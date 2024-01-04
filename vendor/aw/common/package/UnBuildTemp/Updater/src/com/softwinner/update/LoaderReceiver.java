package com.softwinner.update;

/**
 * Wifi状态检查和启动状态检查处理
 * @author Nurmuhammad
 *
 */

import java.util.Calendar;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;

import com.softwinner.update.utils.Constants;
import com.softwinner.update.utils.NetUtils;
import com.lidroid.xutils.util.LogUtils;

public class LoaderReceiver extends BroadcastReceiver
{
    private static final String TAG = "LoaderReceiver";

    private void DailyCheck( Context context )
    {
	AlarmManager alarmManager = (AlarmManager)context.getSystemService( Context.ALARM_SERVICE );
	Calendar mCalendar = Calendar.getInstance( );
	mCalendar.setTimeInMillis( System.currentTimeMillis( ) );
	mCalendar.set( mCalendar.DAY_OF_YEAR , mCalendar.get( mCalendar.DAY_OF_YEAR ) + Constants.CHECK_CYCLE_DAY );

	PendingIntent mPendingIntent = getPendingIntent( context , 1 );
	alarmManager
		.setRepeating( AlarmManager.RTC , mCalendar.getTimeInMillis( ) , Constants.CHECK_REPEATE_TIME , mPendingIntent );
    }

    private PendingIntent getPendingIntent( Context paramContext , int paramInt )
    {
	Intent localIntent = new Intent( paramContext , UpdateService.class );
	localIntent.putExtra( Constants.KEY_START_COMMAND , Constants.START_COMMAND_START_CHECKING );
	return PendingIntent.getService( paramContext , 0 , localIntent , paramInt );
    }

    private void startService( Context context )
    {
	if( Constants.DEBUG )
	    LogUtils.d( "startService" );
	Intent mIntent = new Intent( context , UpdateService.class );
	mIntent.putExtra( Constants.KEY_START_COMMAND , Constants.START_COMMAND_START_CONNECT_CHANGE );
	context.startService( mIntent );
    }

    private void startServiceFromBootComplet( Context context )
    {
	if( Constants.DEBUG )
	    LogUtils.d( "startServiceFromBootComplet" );
	Intent mIntent = new Intent( context , UpdateService.class );
	mIntent.putExtra( Constants.KEY_START_COMMAND , Constants.START_COMMAND_START_BOOT_COMPLET );
	context.startService( mIntent );
    }

    public boolean checkConnectivity( Context context )
    {
	return NetUtils.isNetworkconnected( context );
    }

    @Override
    public void onReceive( Context context , Intent intent )
    {
	String action = intent.getAction( );
	LogUtils.i( "receive a new action : " + action );
	if( action.equals( ConnectivityManager.CONNECTIVITY_ACTION ) && checkConnectivity( context ) )
	{
	    LogUtils.d( "Connectivity change" );
	    startService( context );
	}
	else if( action.equals( Intent.ACTION_BOOT_COMPLETED ) )
	{
	    LogUtils.d( "Boot Complet" );
	    startServiceFromBootComplet( context );
	    DailyCheck( context );
	}
    }

}

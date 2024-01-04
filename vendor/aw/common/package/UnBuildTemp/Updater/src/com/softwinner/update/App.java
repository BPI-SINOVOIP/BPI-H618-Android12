package com.softwinner.update;

import android.app.Application;
import android.content.Context;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.media.SoundPool;
import com.softwinner.update.R;

public class App extends Application
{
    static Context mContext;
    static String mPackName;

    private static SoundPool sp;
    private static int music;

    @Override
    public void onConfigurationChanged( Configuration newConfig )
    {
	// TODO Auto-generated method stub
	super.onConfigurationChanged( newConfig );
    }

    @Override
    public void onCreate()
    {
	// TODO Auto-generated method stub
	super.onCreate( );
	mContext = getApplicationContext( );
	mPackName = getPackageName( );
	//下载完成语音播放
	sp = new SoundPool( 2 , AudioManager.STREAM_SYSTEM , 2 );

	music = sp.load( mContext , R.raw.download_complete , 2 );

    }

    public static Context getAppContext()
    {
	return mContext;
    }

    public static String getPackageNames()
    {
	return mPackName;
    }

    public static void playPrompt()
    {
	sp.play( music , 1 , 1 , 0 , 0 , 1 );
    }
}

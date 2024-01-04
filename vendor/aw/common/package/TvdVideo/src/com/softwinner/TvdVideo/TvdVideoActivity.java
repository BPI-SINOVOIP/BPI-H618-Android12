package com.softwinner.TvdVideo;

///import static android.app.ActivityManager.StackId.PINNED_STACK_ID;

import java.util.List;
import android.app.Activity;
import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.media.AudioManager;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.content.Context;
import androidx.core.content.ContextCompat;
import androidx.core.app.ActivityCompat;
import android.Manifest;
import android.content.pm.PackageManager;
import android.content.BroadcastReceiver;

public class TvdVideoActivity extends Activity {
    private static final String TAG = "TvdVideoActivity";
    private static final boolean TRACE =DebugConfigs.VERBOSE_INFO;

    private MovieViewControl mControl;
    private IActivityManager mActivityManager;
    private PIPModeMediaButtonReceiver mReceiver = new PIPModeMediaButtonReceiver();
    private int mScreenWidth;
    private int mScreenHeight;
    private View mRootView;
    private String mPath000;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle bundle) {
        if(TRACE) Log.v(TAG, "onCreate in");
    super.onCreate(bundle);

    requestWindowFeature(Window.FEATURE_NO_TITLE);
    getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN);

    mPath000 = getIntent().getStringExtra("VideoPath000");
    Log.v(TAG, "video path000= " + mPath000);
    setContentView(R.layout.movie_view);

    mActivityManager = ActivityManagerNative.getDefault();
    //MediaButtonReceiver的注册和其他Receiver的注册稍微不一样，这个注册需要在AudioManager和Manifest.xml中一起都要注册
    ((AudioManager)getSystemService(AUDIO_SERVICE)).registerMediaButtonEventReceiver(new ComponentName(this,MediaButtonReceiver.class));

    mRootView = findViewById(R.id.root);
    final int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
        | View.SYSTEM_UI_FLAG_FULLSCREEN
        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

    getWindow().getDecorView().setSystemUiVisibility(flags);
    {

    final View decorView = getWindow().getDecorView();
        decorView
            .setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener()
            {

                @Override
                public void onSystemUiVisibilityChange(int visibility)
                {
                    if((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0)
                    {
                        decorView.setSystemUiVisibility(flags);
                    }
                }
            });
    }
    DisplayMetrics dm = getResources().getDisplayMetrics();
    mScreenWidth = dm.widthPixels;
    mScreenHeight = dm.heightPixels;
    Log.v(TAG, "mScreenWidth = " + mScreenWidth + "mScreenHeight = " + mScreenHeight);
    if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            initMovieControl();
    }
    if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
            || ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
        ActivityCompat.requestPermissions(this, new String[]{
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.READ_EXTERNAL_STORAGE},
        1);
    }
}

public void initMovieControl(){
    mControl = new MovieViewControl(mRootView, this, getIntent(), mPath000, mScreenWidth,
        mScreenHeight) {
        @Override
        public void onCompletion() {
            super.onCompletion();
            if (super.toQuit()) {
                finish();
            }
        }
    };
}

  @Override
public void onRequestPermissionsResult(int requestCode,  String[] permissions,  int[] grantResults) {
    if (requestCode == 1) {
        Log.e(TAG, "onRequestPermissionsResult");
        if (permissions.length > 0 && grantResults.length > 0) {
            for (int i = 0; i < permissions.length; i++) {
                if (permissions[i].equals(Manifest.permission.READ_EXTERNAL_STORAGE)) {
                    if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                         initMovieControl();
                    } else if (grantResults[i] == PackageManager.PERMISSION_DENIED) {
                        Log.e(TAG, "App need permission Manifest.permission.WRITE_EXTERNAL_STORAGE to scan media!");
                        finish();
                    }
                }
            }
        }
    }
}
    @Override
public void onWindowFocusChanged(boolean hasFocus)
{
    super.onWindowFocusChanged(hasFocus);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
}

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
    }

    @Override
    public void onPause() {
        if(TRACE) Log.v(TAG, "onPause in");
        overridePendingTransition(0,0);
        mControl.onPause();
        super.onPause();
    }

    @Override
    public void onResume() {
        super.onResume();
        if(TRACE) Log.v(TAG, "onResume in");
        mControl.setIntent(getIntent());
        mControl.onResume();

    }

    public void onStop() {
        if(TRACE) Log.v(TAG, "onStop in");
        mControl.onStop();
        super.onStop();
    }

    @Override
    public void onDestroy() {
        if(TRACE) Log.v(TAG, "onDestroy in");
        mControl.onDestroy();
        super.onDestroy();
    }

    @Override
    public void onPictureInPictureModeChanged(boolean isInPictureInPictureMode) {
        if(TRACE) Log.v(TAG, "onPictureInPictureModeChanged in");
        if (isInPictureInPictureMode) {
            if(TRACE) Log.v(TAG, "onPictureInPictureModeChanged isInPictureInPictureMode ");
            if(mControl != null)
                mControl.hideAllControls(); //隐藏控制条
                mControl.setSilentVolume();
                //回到其他界面，开始监听MediaButton
                IntentFilter filter = new IntentFilter();
                filter.addAction("android.intent.action.MEDIABUTTON_BROADCAST");
                registerReceiver(mReceiver, filter);
        } else {
            if(TRACE) Log.v(TAG, "onPictureInPictureModeChanged InPictureInPictureMode out");
            unregisterReceiver(mReceiver);
            if(mControl != null){
                //其他需要整理的
                mControl.setActivityOutFromPIPMode(true);
            }
        }
    }

    //在PIP模式时，按的多媒体键将会通知过来
    public class PIPModeMediaButtonReceiver extends BroadcastReceiver{
        @Override
        public void onReceive(Context c,Intent i){
            if(!isInPictureInPictureMode()) return;

            int keycode = i.getIntExtra("keycode",-1);
            if(TRACE) Log.d(TAG, "PIPModeMediaButtonReceiver onReceive="+keycode);
            if(keycode == KeyEvent.KEYCODE_MEDIA_STOP) //按的停止键，关闭播放器
            {
                if(mControl != null)
                    mControl.saveTimeBookmark();//记录退出的时间点，作为下次启动时的书签时间
                onStop();
                onDestroy(); //清理
                finish();
                System.exit(0);
            }else if(keycode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE){ //播放或者暂停键，切换回全屏
                if(TRACE) Log.v(TAG, "PIPModeMediaButtonReceiver onReceive back to full screen");
                try{
                    //mActivityManager.resizeStack(0, null,true, true, true, -1);//这个切换动作参考ActivityManagerNative
                    mActivityManager.resizeTask(0,null,1);
                }catch(RemoteException e){
                    Log.e(TAG, "resizeStack failed", e);
                }
            }else if(keycode == KeyEvent.KEYCODE_MEDIA_REWIND){ //快退
                if(TRACE) Log.v(TAG, "PIPModeMediaButtonReceiver onReceive KEYCODE_MEDIA_REWIND");
            }else if(keycode == KeyEvent.KEYCODE_MEDIA_FAST_FORWARD){ //快进
                if(TRACE) Log.v(TAG, "PIPModeMediaButtonReceiver onReceive KEYCODE_MEDIA_FAST_FORWARD");
            }else if(keycode == KeyEvent.KEYCODE_MEDIA_PREVIOUS){  //上一个
                if(TRACE) Log.v(TAG, "PIPModeMediaButtonReceiver onReceive KEYCODE_MEDIA_PREVIOUS");
            }else if(keycode == KeyEvent.KEYCODE_MEDIA_NEXT){ //下一个
                if(TRACE) Log.v(TAG, "PIPModeMediaButtonReceiver onReceive KEYCODE_MEDIA_NEXT");
            }
        }
    }
}

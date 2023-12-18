package com.allwinner.camera;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.res.Configuration;

import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.provider.MediaStore;
import android.support.annotation.RequiresApi;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

import com.allwinner.camera.cameracontrol.CameraManager;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.receiver.PowerReceiver;
import com.allwinner.camera.ui.UIManager;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.SharedPreferencesUtils;
import com.allwinner.camera.views.PaperFragment;

import java.lang.ref.WeakReference;

import static android.view.KeyEvent.KEYCODE_BACK;


public class CameraActivity extends Activity implements PowerReceiver.Callback {

    private PaperFragment mViewPagerFragment;
    private static final String TAG = "CameraActivity";
    private UIManager mUiManager;
    private OrientationEventListener mOrientationEventListener;
    private static final int MSG_CLEAR_SCREEN_ON_FLAG = 2;
    private static final long SCREEN_DELAY_MS = 2 * 60 * 1000; // 2 mins.
    private int mOrientationCompensation = 0;
    private MainHandler mMainHandler;
    private boolean mPaused;
    private boolean mKeepScreenOn;
    private boolean mHasCriticalPermissions;

    private PowerReceiver mPowerReceiver = null;
    private static final int LOW_BATTERY_LIMIT = 5;

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.e(TAG,"onCreate");
       // setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_camera);
        checkPermissions();
        if (!mHasCriticalPermissions) {
            Log.v(TAG, "onCreate: Missing critical permissions.");
            finish();
            return;
        }

        checkPowerStatus();

        CameraData.getInstance().initData(getApplicationContext());
        String action = getIntent().getAction();
        CameraData.getInstance().setImageCaptureIntent(MediaStore.ACTION_IMAGE_CAPTURE.equals(action)||MediaStore.ACTION_IMAGE_CAPTURE_SECURE.equals(action));
        CameraData.getInstance().setSecureCameraIntent(MediaStore.ACTION_IMAGE_CAPTURE_SECURE.equals(action)||MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA.equals(action));
        CameraData.getInstance().setVideoCaptureIntent((MediaStore.ACTION_VIDEO_CAPTURE.equals(action)));
        CameraManager.getInstance().startThread();
        View rootView = findViewById(R.id.root_camera);
        mUiManager = new UIManager(getApplicationContext(), this);
        mMainHandler = new MainHandler(this, getMainLooper());
        mUiManager.initView(rootView);
        Configuration config = getResources().getConfiguration();
        mViewPagerFragment = new PaperFragment();
        mViewPagerFragment.setUiManager(mUiManager);
        getFragmentManager().beginTransaction()
                .replace(R.id.paper_frament_container, mViewPagerFragment).commit();

        mOrientationEventListener = new OrientationEventListener(this) {
            @Override
            public void onOrientationChanged(int orientation) {
                CameraData.getInstance().setOrientation(orientation);
                int orientationCompensation = (CameraData.getInstance().getOrientation() +CameraData.getInstance().getDisplayRotation() )% 360;

                if(mOrientationCompensation != orientationCompensation){
                    mOrientationCompensation = orientationCompensation;
                    mUiManager.onOrientationChanged(mOrientationCompensation);
                    CameraData.getInstance().setOrientationCompensation(mOrientationCompensation);
                }
            }
        };
    }

    private static class MainHandler extends Handler {
        final WeakReference<CameraActivity> mActivity;

        public MainHandler(CameraActivity activity, Looper looper) {
            super(looper);
            mActivity = new WeakReference<CameraActivity>(activity);
        }

        @Override
        public void handleMessage(Message msg) {
            CameraActivity activity = mActivity.get();
            if (activity == null) {
                return;
            }
            switch (msg.what) {

                case MSG_CLEAR_SCREEN_ON_FLAG: {
                    if (!activity.mPaused) {
                        activity.getWindow().clearFlags(
                                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    }
                    break;
                }
            }
        }
    }
    public void enableKeepScreenOn(boolean enabled) {
        if (mPaused) {
            return;
        }
        mKeepScreenOn = enabled;
        if (mKeepScreenOn) {
            mMainHandler.removeMessages(MSG_CLEAR_SCREEN_ON_FLAG);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        } else {
            keepScreenOnForAWhile();
        }
    }
    private void keepScreenOnForAWhile() {
        if (mKeepScreenOn) {
            return;
        }
        mMainHandler.removeMessages(MSG_CLEAR_SCREEN_ON_FLAG);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        mMainHandler.sendEmptyMessageDelayed(MSG_CLEAR_SCREEN_ON_FLAG, SCREEN_DELAY_MS);
    }

    private void resetScreenOn() {
        mKeepScreenOn = false;
        if(mMainHandler!=null) {
            mMainHandler.removeMessages(MSG_CLEAR_SCREEN_ON_FLAG);
        }
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
    @Override
    public void onUserInteraction(){
        Log.e(TAG,"onUserInteraction");
        if (!isFinishing()) {
            keepScreenOnForAWhile();
        }

    }

    @Override
    protected void onStart() {
        Log.e(TAG,"onStart");
        super.onStart();
        if(mUiManager!=null) {
            mUiManager.onStart();
        }
    }

    @Override
    protected void onPause() {
        Log.e(TAG,"onPause");
        super.onPause();
        mPaused = true;
        resetScreenOn();
        if(mOrientationEventListener!=null) {
            mOrientationEventListener.disable();
        }
        if(mUiManager!= null) {
            mUiManager.onPause();
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onResume() {
        Log.e(TAG,"onResume");
        checkPermissions();
        if (!mHasCriticalPermissions) {
            Log.v(TAG, "onResume: Missing critical permissions.");
            finish();
            return;
        }
        super.onResume();
        mPaused = false;
        mOrientationEventListener.enable();
        keepScreenOnForAWhile();
        mUiManager.onResume();
    }

    @Override
    protected void onStop() {
        Log.e(TAG,"onStop");
        super.onStop();
        if(mUiManager!= null) {
            mUiManager.onStop();
        }
    }

    @Override
    protected void onDestroy() {
        Log.e(TAG,"onDestroy");
        super.onDestroy();
        mPaused = false;
        new UiManagerReleaseThread(this).start();
        if(CameraManager.getInstance() != null) {
            CameraManager.getInstance().onDestroy();
        }
    }
    public void release(){
        if(mUiManager!= null) {
            mUiManager.onDestroy();
        }
    }
    private static class UiManagerReleaseThread extends Thread {
        WeakReference<CameraActivity> mThreadActivityRef;

        public UiManagerReleaseThread(CameraActivity activity) {
            mThreadActivityRef = new WeakReference<CameraActivity>(
                    activity);
        }

        @Override
        public void run() {
            super.run();
            if (mThreadActivityRef == null)
                return;
            if (mThreadActivityRef.get() != null)
                mThreadActivityRef.get().release();
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    private void checkPermissions() {
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED &&
            checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED &&
            (boolean) SharedPreferencesUtils.getParam(this, Contants.KEY_HAS_SEEN_PERMISSIONS_DIALOGS, false)) {
            mHasCriticalPermissions = true;
        } else {
            mHasCriticalPermissions = false;
        }

        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.Q && mHasCriticalPermissions) {
            mHasCriticalPermissions =
                checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        }
        if (!mHasCriticalPermissions) {
            // TODO: Convert PermissionsActivity into a dialog so we
            // don't lose the state of CameraActivity.
            Intent intent = new Intent(this, PermissionActivity.class);
            startActivity(intent);
            finish();
        }
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        Log.i(TAG,"dispatchTouchEvent");
        if(!CameraData.getInstance().getIsPreviewDone()||CameraData.getInstance().getIsStorageError()){
            return false;
        }
        return super.dispatchTouchEvent(ev);
     //
    }


    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        Log.i(TAG,"dispatchKeyEvent :"+CameraData.getInstance().getIsPreviewDone());
        if(CameraData.getInstance().getIsStorageError() && event.getKeyCode()!=KEYCODE_BACK   ){
            return false;
        }
        if(!CameraData.getInstance().getIsPreviewDone()){
            return false;
        }

        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchTrackballEvent(MotionEvent ev) {
        if(!CameraData.getInstance().getIsPreviewDone()||CameraData.getInstance().getIsStorageError()){
            return false;
        }
        return super.dispatchTrackballEvent(ev);
    }

    public void checkPowerStatus() {
        mPowerReceiver = new PowerReceiver();
        mPowerReceiver.setCallback(this);
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_BATTERY_CHANGED);
        registerReceiver(mPowerReceiver, intentFilter);
    }

    @Override
    public void remainLevel(int level) {
        if (mPowerReceiver != null) unregisterReceiver(mPowerReceiver);
        if (level < LOW_BATTERY_LIMIT) {
            Toast.makeText(this, R.string.battery_remain_too_low, Toast.LENGTH_LONG).show();
            finish();
        }
    }
}

package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;
import java.io.File;

import android.content.Context;
import android.hardware.Camera;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.TextView;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;


import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.CameraManager;

public class CaseCamera extends IBaseCase implements SurfaceHolder.Callback{
    CameraManager mCameraManager;
    private static final String TAG = "DragonBox-CaseCamera";
    private static final int COUNT_DOWN = 1;
    private static final int COUNT_DOWN_FINISH = 2;

    private static final int ARGS_TYPE_RECORD = 1;
    private static final int AEGS_TYPE_PLAY = 2;

    SurfaceView mMaxSurfaceV;
    TextView mMaxInfoTV;
    TextView mMinStatusTV;
    boolean stillTesting = false;
    public int miRecordWidth = 1280;
    public int miRecordHeight = 720;

    private int mrecordTime = 5;

    Handler mHandler = new Handler(Looper.myLooper()) {
        @Override
        public void handleMessage(Message message) {
            switch (message.what) {
                case COUNT_DOWN:
                    if (message.arg1 == ARGS_TYPE_RECORD) {
                        mMaxInfoTV.setText(mContext.getString(R.string.case_camera_info, message.arg2));
                    } else if (message.arg1 == AEGS_TYPE_PLAY) {
                        mMaxInfoTV.setText(mContext.getString(R.string.case_camera_info_play, message.arg2));
                    }

                    break;
                case COUNT_DOWN_FINISH:
                    if (message.arg1 == ARGS_TYPE_RECORD) {
                        mCameraManager.stopRecording();
                        try {
                            setDialogPositiveButtonEnable(true);
                            mCameraManager.startPlayVideo(mCameraManager.mFilePath, mMaxSurfaceV);
                            startCountDown(mrecordTime, AEGS_TYPE_PLAY);
                        } catch (Exception e) {
                            // TODO Auto-generated catch block
                            e.printStackTrace();
                        }
                    } else if (message.arg1 == AEGS_TYPE_PLAY) {
                        mMaxInfoTV.setText(mContext.getString(R.string.case_camera_play_info, message.arg2));
                    }
                    break;
            }
        }
    };

    public CaseCamera(Context context) {
        super(context, R.string.case_camera_name, R.layout.case_camera_max,
                R.layout.case_camera_min, TYPE_MODE_MANUAL);
        mCameraManager = new CameraManager(context);
        mMaxSurfaceV = (SurfaceView) mMaxView.findViewById(R.id.case_camera_surface);
        mMaxSurfaceV.setZOrderOnTop(true);
        LayoutParams lp = mMaxSurfaceV.getLayoutParams();
        WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
        lp.width = (int)context.getResources().getDimension(R.dimen.camera_size_width);
        lp.height = (int)context.getResources().getDimension(R.dimen.camera_size_height);
        mMaxSurfaceV.setLayoutParams(lp);
        mMaxInfoTV = (TextView) mMaxView.findViewById(R.id.case_camera_info);
        mMinStatusTV = (TextView) mMinView.findViewById(R.id.case_camera_status);
    }


    public CaseCamera(Context context, XmlPullParser xmlParser) {
        this(context);
        String time = xmlParser.getAttributeValue(null, "recordtime");
        String sWidth = xmlParser.getAttributeValue(null, "recordWidth");
        String sHeight = xmlParser.getAttributeValue(null, "recordHeight");
        if (sWidth != null && sHeight != null) {
            miRecordWidth = Integer.parseInt(sWidth);
            miRecordHeight = Integer.parseInt(sHeight);
        }
        try{
            mrecordTime = Integer.parseInt(time);
        } catch (Exception e) {
            e.printStackTrace();
            mrecordTime = 5;
        }
    }

    @Override
    public void onStartCase() {
        SurfaceHolder surfaceHolder;
        surfaceHolder = mMaxSurfaceV.getHolder();
        //surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        surfaceHolder.addCallback(this);
        setDialogPositiveButtonEnable(false);
    }

    @Override
    public void onStopCase() {
        try{
            mCameraManager.releaseRecord();
        } catch (Exception e) {
            Log.e(TAG,"mediarecorder stop error!");
            e.printStackTrace();
        }
        mCameraManager.releasePlayVideo();
        mMinStatusTV.setText(getCaseResult() ? R.string.case_camera_status_success_text
                : R.string.case_camera_status_fail_text);
        stillTesting = false;
        File file =new File(mCameraManager.mFilePath);
        if(file.exists()){
            file.delete();
        }

    }

    @Override
    public void reset() {
        super.reset();
        mMinStatusTV.setText(R.string.case_camera_status_text);
    }

    @Override
    public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {
        Log.d(TAG, "surfaceChanged");

    }

    @Override
    public void surfaceCreated(SurfaceHolder arg0) {
        int numCamera = Camera.getNumberOfCameras();
        Log.d(TAG, "surfaceCreated and cameraNUM =" + numCamera);
        if (numCamera < 1) {
            mMaxInfoTV.setText(R.string.case_camera_notfound_camera);
            return;
        }
        try {
            mCameraManager.startRecording(mMaxSurfaceV,miRecordWidth,miRecordHeight);
            startCountDown(mrecordTime, ARGS_TYPE_RECORD);
            mMaxInfoTV.setText(mContext.getString(R.string.case_camera_info, 5));
        } catch (Exception e) {
            mMaxInfoTV.setText(R.string.case_camera_exception);
            e.printStackTrace();
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder arg0) {
        Log.d(TAG, "surfaceDestroyed");

    }

    public void startCountDown(final int totalTime,final int type) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                stillTesting = true;
                for (int i = 0; i < totalTime; i++) {
                    if (!stillTesting) {
                        return;
                    }
                    final int time = totalTime - i;
                    //Message msg = mHandler.obtainMessage(WHAT_COUNT_DOWN);
                    Message msg = new Message();
                    msg.what = COUNT_DOWN;
                    msg.arg1 = type;
                    msg.arg2 = time;
                    mHandler.sendMessage(msg);
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                if (!stillTesting) {
                    return;
                }
                Message msg = mHandler.obtainMessage(COUNT_DOWN_FINISH);
                msg.arg1 = type;
                mHandler.sendMessage(msg);
            }
        }).start();
    }
}

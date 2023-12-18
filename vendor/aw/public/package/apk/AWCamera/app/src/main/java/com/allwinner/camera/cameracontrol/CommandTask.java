package com.allwinner.camera.cameracontrol;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.Log;

import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.ui.UIManager;
import static com.allwinner.camera.data.Contants.CameraCommand.CLOSECAMERA;
import static com.allwinner.camera.data.Contants.CameraCommand.OPENCAMERA;
import static com.allwinner.camera.data.Contants.CameraCommand.STARTPREVIEW;
import static com.allwinner.camera.data.Contants.CameraCommand.STOPPREVIEW;
import static com.allwinner.camera.data.Contants.CameraCommand.TAKEPICTURE;
import static com.allwinner.camera.data.Contants.CameraCommand.UPDATESIZE;

public class CommandTask {
    private Handler mCommandTaskHandler;
    private HandlerThread mCommandTaskThread;
    private UIManager mUiManager;
    private static  final  int mTryCount = 100;
    private int mTryTime = 0;
    private static final String TAG = "CommandTask";
    public CommandTask(UIManager uiManager){
        mUiManager = uiManager;
        mCommandTaskThread = new HandlerThread("CameraManager");
        mCommandTaskThread.start();
        mCommandTaskHandler = new Handler(mCommandTaskThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                handleCommand(msg);
            }
        };

    }

    public void quit() {
        if (mCommandTaskThread.getLooper() != null) {
            mCommandTaskThread.getLooper().quit();
        }
    }

    public void sendCommand(int command){
        sendCommand(command,null);
    }

    public void sendCommand(int command,Object arg){
        Message message = new Message();
        message.what = command;
        if (arg !=null) {
            message.obj = arg;
        }
        mCommandTaskHandler.sendMessage(message);
    }
    public void sendCommand(int command,Object arg,Object arg2){
        Message message = new Message();
        message.what = command;
        if (arg !=null) {
           // message.setData(arg);
        }
        mCommandTaskHandler.sendMessage(message);
    }
    public void handleCommand(Message msg){
        Message message = msg;
        switch (msg.what) {
            case OPENCAMERA:
                int cameraid = (int) msg.obj;
                CameraManager.getInstance().openCamera(cameraid);
                return;
            case CLOSECAMERA:
                CameraManager.getInstance().closeCamera();
                return;
            case STARTPREVIEW:
                Contants.ModeType type = (Contants.ModeType) msg.obj;
                Log.i(TAG,"STARTPREVIEW: mUiManager.isCameraOpen() "  + mUiManager.isCameraOpen()
                +" mUiManager.getSurfaceTexture() :"+ mUiManager.getSurfaceTexture() );
                if (mUiManager.isCameraOpen() && mUiManager.getSurfaceTexture() != null) {
                    mCommandTaskHandler.removeMessages(STARTPREVIEW);

                    CameraManager.getInstance().updateSizeParamater(CameraData.getInstance().getCameraRatio(),type);
                    CameraManager.getInstance().startPreview(type);
                    mTryTime = 0;
                } else {
                    Message message2 = new Message();
                    message2.what = STARTPREVIEW;
                    message2.obj = type;
                    mTryTime ++ ;
                    if (mTryTime == mTryCount) {
                        mTryTime = 0;
                        mCommandTaskHandler.removeMessages(STARTPREVIEW);
                        Log.e(TAG,"STARTPREVIEW: fail !: " + mTryTime);
                        mUiManager.onStartPreviewFail();
                    } else {
                        mCommandTaskHandler.sendMessageDelayed(message2, 100);
                    }
                }
                return;
            case STOPPREVIEW:
                CameraManager.getInstance().stopPreview();
                return;
//            case SETZOOM:
//                float detal = (float) msg.obj;
//                CameraManager.getInstance().setZoom(detal);
//                return;
//            case ONZOOMEND:
//                float detalEnd = (float) msg.obj;
//                CameraManager.getInstance().onZoomEnd(detalEnd);
//                return;
//            case AUTOFOCUS:
//               // CameraManager.getInstance().autoFocus();
//                return;
//            case CANCELAUTOFOCUS:
//                CameraManager.getInstance().cancelAutoFocus();
//                return;
            case TAKEPICTURE:
                CameraManager.getInstance().takePicture();
                return;
     /*       case UPDATESIZE:
                Contants.ModeType type2 = (Contants.ModeType) msg.obj;
                if (mUiManager.isCameraOpen()) {
                    CameraManager.getInstance().updateSizeParamater(CameraData.getInstance().getCameraRatio(),type2);
                } else {
                    Message message2 = new Message();
                    message2.what = UPDATESIZE;
                    message2.obj = type2;
                    mCommandTaskHandler.sendMessageDelayed(message2, 50);
                }
                return;*/
            default:
                return;
        }
    }

}

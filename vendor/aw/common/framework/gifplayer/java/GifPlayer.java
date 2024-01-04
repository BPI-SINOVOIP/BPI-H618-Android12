package com.softwinner;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.HandlerThread;
import java.io.File;

public class GifPlayer {

    static {
        System.loadLibrary("gifplayer_jni");
    }

    /**************************************JNI接口*****************************************/
    private native long native_create();

    private native boolean native_load(long ptr,String gifPath);

    private native void native_play(long ptr, boolean loop, Bitmap bitmap,Runnable runnable);

    private native int native_getWidth(long ptr);

    private native int native_getHeight(long ptr);

    private native void native_pause(long ptr);

    private native void native_resume(long ptr);

    private native void native_stop(long ptr);

    private native void native_release(long ptr);
    /**************************************JNI接口*****************************************/

    public interface OnDrawGifCallBack {
        void onDrawFrame(Bitmap bitmap);
        void onError(String msg);
    }

    private Bitmap mBitmap;
    private OnDrawGifCallBack mOnDrawGifCallBack;
    private HandlerThread mHandlerThread;
    private Handler mHandler;
    private long mGifPlayerPtr = 0L;
    private final static String TAG = "GifPlayer";

    public GifPlayer(){
        mGifPlayerPtr = native_create();
        mHandlerThread = new HandlerThread("GifPlayer");
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper());
    }

    public void setOnDrawGifCallBack(OnDrawGifCallBack onDrawGifCallBack){
        this.mOnDrawGifCallBack = onDrawGifCallBack;
    }

    public void play(final String gifPath, final boolean loop){
        boolean ret = checkIfFileExist(gifPath);
        if(!ret){
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if(mOnDrawGifCallBack != null){
                        mOnDrawGifCallBack.onError("file no exist");
                    }
                }
            });
        }else {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (native_load(mGifPlayerPtr, gifPath)) {
                        mBitmap = Bitmap.createBitmap(native_getWidth(mGifPlayerPtr), native_getHeight(mGifPlayerPtr), Bitmap.Config.ARGB_8888);
                        native_play(mGifPlayerPtr, loop, mBitmap, new Runnable() {
                            @Override
                            public void run() {
                                if (mOnDrawGifCallBack != null) {
                                    mOnDrawGifCallBack.onDrawFrame(mBitmap);
                                }
                            }
                        });
                    }
                }
            });
        }
    }

    public void pause() {
        native_pause(mGifPlayerPtr);
    }

    public void resume() {
        native_resume(mGifPlayerPtr);
    }

    public void stop() {
        native_stop(mGifPlayerPtr);
    }

    public void release() {
        native_stop(mGifPlayerPtr);
        //在哪个线程创建的就要在对应线程去销毁对象
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                native_release(mGifPlayerPtr);
                mBitmap = null;
                mHandler = null;
                mHandlerThread.quit();
                mHandlerThread = null;
            }
        });
    }

    private boolean checkIfFileExist(String gifPath){
        if (gifPath != null && gifPath.length() != 0) {
            File file = new File(gifPath);
            if(file.exists() && _getFormatName(gifPath).equals("gif")){
                return true;
            }
        }
        return false;
    }

    private String _getFormatName(String gifPath) {
        if (gifPath != null && gifPath.length() != 0) {
            gifPath = gifPath.trim();
            String s[] = gifPath.split("\\.");
            if (s.length >= 2) {
                return s[s.length - 1];
            }
        }
        return "";
    }

}

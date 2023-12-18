package com.allwinner.camera.views;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.PointF;
import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import android.util.Log;

import com.alibaba.android.mnnkit.actor.FaceDetector;
import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.data.FaceResultData;
import com.allwinner.camera.ui.UIManager;
import com.allwinner.camera.utils.SharedPreferencesUtils;


import java.lang.reflect.Constructor;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class PreviewRenderer implements GLSurfaceView.Renderer {


    private final UIManager mUiManager;
    private PreviewDrawer mDrawer = null;
    private static final String TAG = "PreviewRenderer";
    private boolean mAttached = false;
    private SurfaceTexture mSurfaceTexture = null;
    private int mTextureId = -1;
    private Context mContext;

    private static Constructor<SurfaceTexture> sSurfaceTextureCtor;
    private SurfaceTextureListener mListener = null;

    static {
        try {
            sSurfaceTextureCtor = SurfaceTexture.class.getDeclaredConstructor(boolean.class);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
    }

    private SurfaceTexture mMosaicSurfaceTexture;
    private int mWidth;
    private int mHeight;
    private FaceDetector mFaceDetector;

    public void setYuvData(byte[] yuvData, int width, int height, int jpegRotation, boolean mirror) {
        mDrawer.setYuvData(yuvData, width, height, jpegRotation, mirror);
    }

    public void cleanData() {
        if(mDrawer != null) {
            mDrawer.cleanData();
        }

    }

    public void createKitSuccess(FaceDetector faceDetector) {
      mFaceDetector = faceDetector;
      if(mDrawer !=null){
          mDrawer.createKitSuccess(faceDetector);
      }
    }

    public void onCameraOpen() {
        if(mDrawer!= null){
            mDrawer.onCameraOpen();
        }

    }

    public interface SurfaceTextureListener {
        void onSurfaceTextureAvailable(SurfaceTexture surface);

        void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height);

        boolean onSurfaceTextureDestroyed(SurfaceTexture surface);
    }


    public PreviewRenderer(Context context, UIManager uiManager) {
        this.mContext = context;
        this.mUiManager = uiManager;
    }

    public void setListener(SurfaceTextureListener mListener) {
        this.mListener = mListener;
    }


    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        if (mDrawer == null) {
            mDrawer = new PreviewDrawer(mContext,mUiManager);
            mDrawer.init(mFaceDetector);
        }
        if(CameraData.getInstance().getCurrentModeType().equals(Contants.ModeType.VideMode)){
            setFilterType(Contants.FilterType.Normal);
        }
        setFaceBeautyOn((Boolean) SharedPreferencesUtils.getParam(mContext,Contants.ISBEAUTY,false));
        mTextureId = mDrawer.getTextureId();

        if (mSurfaceTexture == null) {
            try {
                mSurfaceTexture = new SurfaceTexture(mTextureId);
                mAttached = false;
                if (mListener != null) {
                    mListener.onSurfaceTextureAvailable(mSurfaceTexture);
                }
            } catch (Exception e) {
                Log.e(TAG, "error:" + e.getMessage());
            }
        } else {
            Log.i(TAG, "useInstance SurfaceTexture:" + mSurfaceTexture);
            if (mListener != null) {
                mListener.onSurfaceTextureAvailable(mSurfaceTexture);
            }
        }
        Log.i(TAG, "onSurfaceCreated: mTextureId " + mTextureId);

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        mWidth = width;
        mHeight = height;
       // MosaicRenderer.reset(width, height, true);
        if (mListener != null) {
            mListener.onSurfaceTextureSizeChanged(mSurfaceTexture, width, height);
        }
        mDrawer.setViewSize(width, height);
        Log.i(TAG, "onSurfaceChanged: size: " + width + "x" + height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        float[] mtx = new float[16];

/*            if (!mAttached) {
                mSurfaceTexture.attachToGLContext(mTextureId);
                mAttached = true;
            }*/
//从Image Stream中捕获帧数据，用作OpenGLES的纹理，其中Image Stream来自相机预览,
// SurfaceTexture对象所关联的OpenGLES中纹理对象的内容将被更新为Image Stream中最新的图片

     if (mSurfaceTexture != null && (mTextureId != -1) && mDrawer != null) {
            try {
                mSurfaceTexture.updateTexImage();
                mSurfaceTexture.getTransformMatrix(mtx);
            } catch (Exception e) {
                e.printStackTrace();
            }
            mDrawer.drawFrame(mtx);
        }

    }



    public void release() {
        if (mDrawer != null) {
            mDrawer.release();
            mDrawer = null;
        }
        if (mSurfaceTexture != null) {
            mSurfaceTexture.release();
        }
        if (mMosaicSurfaceTexture != null) {
            mMosaicSurfaceTexture.release();
        }
    }

    public void releaseMosaic() {

        if (mMosaicSurfaceTexture != null) {
            mMosaicSurfaceTexture.release();
        }
    }

    public Bitmap getPreviewBitmap() {
        Bitmap bitmap = null;
        if (mDrawer != null) {
            bitmap = mDrawer.getPreviewBitmap();
        }
        return bitmap;
    }

    public void generatePrevieBitmap() {
        if (mDrawer != null) {
            mDrawer.generatePrevieBitmap();
        }
    }

    public void setFilterType(Contants.FilterType filtertype) {
        if (mDrawer != null) {
            mDrawer.setFilterType(filtertype);
        }
    }

    public void setFaceBeautyOn(boolean fbOn) {
        Log.e(TAG,"setFaceBeautyOn:"+fbOn+"mDrawer:"+mDrawer);
        if (mDrawer != null) {
            mDrawer.setFaceBeautyOn(fbOn);
        }
    }


    public void setFacePoint(FaceDetectionReport[] result) {
        if (mDrawer != null) {
            mDrawer.setFaceResult(result);
        }
    }
}

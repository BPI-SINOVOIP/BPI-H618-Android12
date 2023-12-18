package com.allwinner.camera.views;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.os.AsyncTask;
import android.util.Log;

import com.alibaba.android.mnnkit.actor.FaceDetector;
import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.data.DynamicSticker;
import com.allwinner.camera.data.DynamicStickerNormalData;
import com.allwinner.camera.drawer.BaseDrawer;
import com.allwinner.camera.drawer.BigEyeDrawer;
import com.allwinner.camera.drawer.DrawerFactory;
import com.allwinner.camera.drawer.LutDrawer;
import com.allwinner.camera.program.BaseProgram;
import com.allwinner.camera.stickers.DynamicStickerLoader;
import com.allwinner.camera.ui.UIManager;
import com.allwinner.camera.utils.BitmapUtils;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.EGLUtils;
import com.allwinner.camera.utils.SystemPropertyUtils;
import com.allwinner.camera.utils.YuvUtils;


import org.json.JSONException;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static android.opengl.GLES11Ext.GL_TEXTURE_EXTERNAL_OES;
import static android.opengl.GLES20.GL_BLEND;
import static android.opengl.GLES20.GL_COLOR_BUFFER_BIT;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.glClear;
import static android.opengl.GLES20.glClearColor;
import static com.allwinner.camera.utils.EGLUtils.recycleTexture;

public class PreviewDrawer {
    private final UIManager mUiManager;
    BaseProgram mRenderProgram = null;
    private int mTextureId = -1;
    private int mLutTextureId = -1;
    private int mIdFBO = -1;
    private int mShareTextureId = -1;
    private int mIdFBO2 = -1;
    private int mIdFBO3 = -1;
    private int mShareTextureId2 = -1;
    private int mShareTextureId3 = -1;
    private final static String TAG = "PreviewDrawer";
    private int mViewWidth;
    private int mViewHeight;
    private int mPreviewWidth = 0;
    private int mPreviewHeight = 0;
    private final float[] mTexMatrix = new float[16];
    private final float[] mRotateMatrix = new float[16];
    private final float[] mMirrorMatrix = new float[16];
    private final float[] mYuvFrontMatrix = new float[16];
    private final float[] mYuvBackMatrix = new float[16];
    private final float[] mYuvDataMatrix = new float[16];
    private Bitmap mPreviewBitmap = null;
    private Object mSyncObject = new Object();
    private Object mFaceResultSyncObject = new Object();
    private Object mSyncObjectForPreview = new Object();
    private Boolean mGetBitmap = false;
    final Object mWaitLock = new Object();
    private boolean mNeedChangeFilter = false;
    private boolean mNeedChangeProgram = false;
    private boolean mFaceBeautyOn = false;
    private Contants.FilterType mCurrentFilterType = Contants.FilterType.YUV;
    private Context mContext;
    private int mYTextureId = -1;
    private int mUVTextureId = -1;
    private byte[] mData;
    private boolean mDoBeauty = false;
    private int mTexWidth;
    private int mTexHeight;
    private float[] mFrameMirrorMatrix = new float[16];
    private float[] mIndentityMatrix = new float[16];
    private boolean mNeedBigEyes = true;
    private ByteBuffer mPixelBuffer;
    private int i = 0;

    private FaceDetector mFaceDetector;
    private byte[] nv21data;
    private int mPreNv21Size;
    private DynamicSticker mDynamicSticker;


    private DrawerFactory mDrawerFactory = null;
    private final String READPIXEL = "readpixel";
    private FaceDetectionReport[] mFaceResult;
    private float mRatio;
    private float[] mProjectionMatrix = new float[16];
    private float[] mViewMatrix = new float[16];
    public PreviewDrawer(Context context, UIManager uiManager) {
        mContext = context;
        mUiManager = uiManager;
        mDrawerFactory = new DrawerFactory(context);
    }

    public void init(FaceDetector faceDetector) {
        mFaceDetector = faceDetector;
        mNeedBigEyes = SystemPropertyUtils.getBoolean(mContext, "sys.camera.facedetection.enable", true);


        mTextureId = EGLUtils.createTextureId(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
        Matrix.setIdentityM(mTexMatrix, 0);
        Matrix.setIdentityM(mFrameMirrorMatrix, 0);
        Matrix.setIdentityM(mIndentityMatrix, 0);
        Matrix.setIdentityM(mRotateMatrix, 0);
        Matrix.setIdentityM(mYuvFrontMatrix, 0);
        Matrix.setIdentityM(mYuvBackMatrix, 0);
        Matrix.setIdentityM(mYuvDataMatrix, 0);
        Matrix.rotateM(mYuvFrontMatrix, 0, CameraUtils.getPreviewRotation(CameraData.getInstance().getFrontCameraId()), 0, 0, 1);
        Matrix.rotateM(mYuvFrontMatrix, 0, 180, 0, 1, 0);

        Matrix.rotateM(mYuvBackMatrix, 0, CameraUtils.getPreviewRotation(CameraData.getInstance().getFrontCameraId()), 0, 0, 1);
        Matrix.rotateM(mRotateMatrix, 0, 360 - CameraData.getInstance().getDisplayRotation(), 0, 0, 1);

        Matrix.rotateM(mFrameMirrorMatrix, 0, 180, 0, 1, 0);
        Matrix.rotateM(mFrameMirrorMatrix, 0, 180, 0, 0, 1);
        Matrix.rotateM(mYuvDataMatrix, 0, 360 - CameraData.getInstance().getDisplayRotation(), 0, 0, 1);
        if (!CameraData.getInstance().isCurrentFront()) {
            Matrix.rotateM(mYuvDataMatrix, 0, 180, 0, 1, 0);
        }
        Matrix.rotateM(mYuvDataMatrix, 0, 180, 0, 0, 1);


 /*       if (mNormalDrawer == null) {
            mNormalDrawer = new BaseDrawer(EGLUtils.TEXTURETYPE.OESTEXTURE, EGLUtils.TEXTURETYPE.TEXTURE2D);
            mNormalDrawer.initEGL(mContext);
        }

        if (mLutDrawer == null) {
            mLutDrawer = new LutDrawer(EGLUtils.TEXTURETYPE.OESTEXTURE, EGLUtils.TEXTURETYPE.TEXTURE2D);
            mLutDrawer.initEGL(mContext);
        }

        if (mBigEyeDrawer == null) {
            mBigEyeDrawer = new BigEyeDrawer(EGLUtils.TEXTURETYPE.OESTEXTURE, EGLUtils.TEXTURETYPE.TEXTURE2D);
            mBigEyeDrawer.initEGL(mContext);
        }

        if (mSmoothDrawer == null) {
            mSmoothDrawer = new SmoothDrawer(EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.TEXTURE2D);
            mSmoothDrawer.initEGL(mContext);
        }

        if (mStickerDrawer == null) {
            mStickerDrawer = new BaseDrawer(EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.TEXTURE2D);
            mStickerDrawer.initEGL(mContext);
        }

        if (mReadPixelDrawer == null) {
            mReadPixelDrawer = new BaseDrawer(EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.YUV);
            mReadPixelDrawer.initEGL(mContext);
        }*/

    }

    public void setViewSize(int width, int height) {
        mViewWidth = width;
        mViewHeight = height;
        mRatio = (float) width / height;
        Matrix.frustumM(mProjectionMatrix, 0, -mRatio, mRatio, -1.0f, 1.0f, 3.0f, 9.0f);
        Matrix.setLookAtM(mViewMatrix, 0, 0, 0, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    }

    public int getTextureId() {
        return mTextureId;
    }

    public void setYuvData(byte[] yuvData, int width, int height, int jpegRotation, boolean mirror) {
        synchronized (mSyncObjectForPreview) {
            if ((mData == null) || (mPreviewHeight != height)) {
                mData = new byte[width * height * 3 / 2];
            }
            mPreviewWidth = width;
            mPreviewHeight = height;
            mData = yuvData;
        }
    }

    public void drawFrame(float[] texMatrix) {
        synchronized (mSyncObjectForPreview) {
            glClear(GL_COLOR_BUFFER_BIT);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            GLES20.glEnable(GL_BLEND);
            GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);  // s *alpha +d (1-appha)
            GLES20.glViewport(0, 0, mViewWidth, mViewHeight);
            if (mNeedChangeFilter) {
                if (mCurrentFilterType.isLut()) {
                    mLutTextureId = getLubTexture();
                }
                mNeedChangeFilter = false;
            }
            boolean needDoSticker = CameraUtils.needDoSticker();
            if (needDofaceBeauty() || needDoSticker) {
                readPixelAndDoFaceDetector(texMatrix);
            }
            //这个分支暂时不会走
            if (needDrawYuv()) {
                synchronized (mSyncObjectForPreview) {
                    if (mData != null) {
                        drawYuv(mData, texMatrix);
                    }
                }
            } else if (needDofaceBeauty()) {
                drawFB(GL_TEXTURE_EXTERNAL_OES, mTextureId, texMatrix, mRotateMatrix);
            } else {
                if (mCurrentFilterType.isLut()) {
                    BaseDrawer drawer = mDrawerFactory.getDrawer(mDrawerFactory.LUT, EGLUtils.TEXTURETYPE.OESTEXTURE, EGLUtils.TEXTURETYPE.TEXTURE2D);
                    ((LutDrawer) drawer).setLutTextureId(mLutTextureId);
                    drawer.drawTexture(GL_TEXTURE_EXTERNAL_OES, mTextureId, mViewWidth, mViewHeight, texMatrix, mRotateMatrix);
                } else {
                    BaseDrawer drawer = mDrawerFactory.getDrawer(mDrawerFactory.NORMAL, EGLUtils.TEXTURETYPE.OESTEXTURE, EGLUtils.TEXTURETYPE.TEXTURE2D);
                    drawer.drawTexture(GL_TEXTURE_EXTERNAL_OES, mTextureId, mViewWidth, mViewHeight, texMatrix, mRotateMatrix);
                }
                //drawTexture(GL_TEXTURE_EXTERNAL_OES, mTextureId, texMatrix, mRotateMatrix, false, mViewWidth, mViewHeight);
            }
            if (needDoSticker) {
                BaseDrawer drawer = mDrawerFactory.getDrawer(Contants.STICKER, EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.TEXTURE2D);
                drawer.drawSticker(CameraData.getInstance().getStickerName(),mFaceResultSyncObject,mFaceResult,mViewWidth,mViewHeight,
                        mProjectionMatrix,mViewMatrix,Contants.FACE_DATA_WIGHT, Contants.FACE_DATA_HEIGHT,false);
            }
            if (mGetBitmap) {
                createPreviewBitmap(mViewWidth, mViewHeight);
                mGetBitmap = false;
            }
            GLES20.glDisable(GLES20.GL_BLEND);
        }
    }

    public void readPixelAndDoFaceDetector(float[] texMatrix) {
        initEffectTexture(CameraData.getInstance().getPreviewWidth(), CameraData.getInstance().getPreviewHeight());
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mIdFBO3);
        BaseDrawer drawer = mDrawerFactory.getDrawer(READPIXEL, EGLUtils.TEXTURETYPE.OESTEXTURE, EGLUtils.TEXTURETYPE.YUV);
        drawer.drawTexture(GL_TEXTURE_EXTERNAL_OES, mTextureId, Contants.FACE_DATA_WIGHT, Contants.FACE_DATA_HEIGHT, texMatrix, mYuvDataMatrix);
        doFaceDetector();
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
    }

    private void drawFB(int target, int texture, float[] texMatrix, float[] mvpMatrix) {
        if (mNeedBigEyes) {
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mIdFBO);
            BaseDrawer bigeyedrawer = mDrawerFactory.getDrawer(mDrawerFactory.BIGEYE, EGLUtils.TEXTURETYPE.OESTEXTURE, EGLUtils.TEXTURETYPE.TEXTURE2D);
            bigeyedrawer.drawTexture(target, texture, mViewWidth, mViewHeight, texMatrix, mvpMatrix);
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
            if (mCurrentFilterType.isLut()) {
                GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mIdFBO2);
                BaseDrawer smoothDrawer = mDrawerFactory.getDrawer(mDrawerFactory.SMOOTH, EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.TEXTURE2D);
                smoothDrawer.drawTexture(GL_TEXTURE_2D, mShareTextureId, mViewWidth, mViewHeight, mIndentityMatrix, mIndentityMatrix);
                GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
                BaseDrawer lutDrawer = mDrawerFactory.getDrawer(mDrawerFactory.LUT, EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.TEXTURE2D);
                ((LutDrawer) lutDrawer).setLutTextureId(mLutTextureId);
                lutDrawer.drawTexture(GL_TEXTURE_2D, mShareTextureId2, mViewWidth, mViewHeight, mIndentityMatrix, mIndentityMatrix);
            } else {
                BaseDrawer smoothDrawer = mDrawerFactory.getDrawer(mDrawerFactory.SMOOTH, EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.TEXTURE2D);
                smoothDrawer.drawTexture(GL_TEXTURE_2D, mShareTextureId, mViewWidth, mViewHeight, mIndentityMatrix, mFrameMirrorMatrix);
            }
        } else {
            //输入的宽高暂时无用
            initEffectTexture(CameraData.getInstance().getPreviewWidth(), CameraData.getInstance().getPreviewHeight());
            if (mCurrentFilterType.isLut()) {
                GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mIdFBO2);
                BaseDrawer smoothDrawer = mDrawerFactory.getDrawer(mDrawerFactory.SMOOTH, EGLUtils.TEXTURETYPE.OESTEXTURE, EGLUtils.TEXTURETYPE.TEXTURE2D);
                smoothDrawer.drawTexture(target, texture, mViewWidth, mViewHeight, texMatrix, mRotateMatrix);
                GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
                BaseDrawer lutDrawer = mDrawerFactory.getDrawer(mDrawerFactory.LUT, EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.TEXTURE2D);
                ((LutDrawer) lutDrawer).setLutTextureId(mLutTextureId);
                lutDrawer.drawTexture(GL_TEXTURE_2D, mShareTextureId2, mViewWidth, mViewHeight, mIndentityMatrix, mFrameMirrorMatrix);
            } else {
                BaseDrawer smoothDrawer = mDrawerFactory.getDrawer(mDrawerFactory.SMOOTH, EGLUtils.TEXTURETYPE.OESTEXTURE, EGLUtils.TEXTURETYPE.TEXTURE2D);
                smoothDrawer.drawTexture(target, texture, mViewWidth, mViewHeight, texMatrix, mRotateMatrix);
            }
        }
    }

    public void setFaceResult(FaceDetectionReport[] result) {
        synchronized (mFaceResultSyncObject) {
            mFaceResult = result;
            BaseDrawer smoothDrawer = mDrawerFactory.getDrawer(mDrawerFactory.BIGEYE);
            if (smoothDrawer != null) {
                ((BigEyeDrawer) smoothDrawer).setFaceResult(result, false, Contants.FACE_DATA_WIGHT, Contants.FACE_DATA_HEIGHT);
            }
        }
    }

    public void doFaceDetector() {
        if (mPixelBuffer == null) {
            mPixelBuffer = ByteBuffer.allocate(Contants.FACE_DATA_WIGHT * Contants.FACE_DATA_HEIGHT * 4);
        }
        //左下角为原点
        GLES20.glReadPixels(0, 0, Contants.FACE_DATA_WIGHT, Contants.FACE_DATA_HEIGHT, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, mPixelBuffer);
        byte[] data = mPixelBuffer.array();
        int nv21Size = Contants.FACE_DATA_WIGHT * Contants.FACE_DATA_HEIGHT * 3 / 2;
        if (nv21Size != mPreNv21Size) {
            nv21data = new byte[nv21Size];
        }
        mPreNv21Size = nv21Size;
        YuvUtils.changeYuvToNv21(data, nv21data, Contants.FACE_DATA_WIGHT, Contants.FACE_DATA_HEIGHT, 4);
        int inAngle = CameraUtils.getOutAngle();
        int outAngle = CameraUtils.getOutAngle();
        if (mFaceDetector != null) {
            CameraUtils.doFaceDetector(mUiManager, mFaceDetector, nv21data, Contants.FACE_DATA_WIGHT, Contants.FACE_DATA_HEIGHT, inAngle, outAngle,
                    CameraData.getInstance().isCurrentFront(), false);
        }
    }

    public void createKitSuccess(FaceDetector faceDetector) {
        mFaceDetector = faceDetector;
    }

    public void onCameraOpen() {
        Matrix.setIdentityM(mYuvDataMatrix, 0);
        Matrix.rotateM(mYuvDataMatrix, 0, 360 - CameraData.getInstance().getDisplayRotation(), 0, 0, 1);
        if (!CameraData.getInstance().isCurrentFront()) {
            Matrix.rotateM(mYuvDataMatrix, 0, 180, 0, 1, 0);
        }
        Matrix.rotateM(mYuvDataMatrix, 0, 180, 0, 0, 1);
    }

    private class FaceTrackTask extends AsyncTask<Void, Void, Void> {
        private byte[] mData = null;
        private int mJpegRotation = 0;
        private byte[] nv21data;
        private int width;
        private int height;

        public FaceTrackTask(final byte[] data, int width, int height) {
            this.mData = data;
            this.width = width;
            this.height = height;
        }

        @Override
        protected Void doInBackground(Void... voids) {
            nv21data = new byte[width * height * 3 / 2];
            YuvUtils.changeYuvToNv21(mData, nv21data, width, height, 4);
            int inAngle = CameraUtils.getOutAngle();
            int outAngle = CameraUtils.getOutAngle();
            if (mFaceDetector != null)
                CameraUtils.doFaceDetector(mUiManager, mFaceDetector, nv21data, width, height, inAngle, outAngle, CameraData.getInstance().isCurrentFront(), false);
            return null;
        }
    }


    private void drawYuv(byte[] data, float[] texMatrix) {
        BaseDrawer normalDrawer = mDrawerFactory.getDrawer(mDrawerFactory.NORMAL, EGLUtils.TEXTURETYPE.YUV, EGLUtils.TEXTURETYPE.TEXTURE2D);
        if (CameraData.getInstance().getCameraId() == CameraData.getInstance().getBackCameraId()) {
            normalDrawer.drawYuv(data, mPreviewWidth, mPreviewHeight, mViewWidth, mViewHeight, mIndentityMatrix, mYuvBackMatrix);
        } else {
            normalDrawer.drawYuv(data, mPreviewWidth, mPreviewHeight, mViewWidth, mViewHeight, mIndentityMatrix, mYuvFrontMatrix);
        }
    }


    public void release() {
        if (mRenderProgram != null) {
            recycleTexture(mTextureId);
            recycleTexture(mLutTextureId);
            recycleTexture(mShareTextureId);
            EGLUtils.deleteFrameBuffer(mIdFBO);
            recycleTexture(mShareTextureId2);
            EGLUtils.deleteFrameBuffer(mIdFBO2);
            recycleTexture(mShareTextureId3);
            EGLUtils.deleteFrameBuffer(mIdFBO3);

            if (mDrawerFactory != null) {
                mDrawerFactory.release();
                mDrawerFactory = null;
            }
        }
    }

    public void initEffectTexture(int previewWidth, int previewHeight) {
        if ((mTexWidth == mViewWidth) && (mTexHeight == mViewHeight)) {
            return;
        }
        Log.d(TAG, "initEffectTexture start: " + mViewWidth + "x" + mViewHeight);
        mTexWidth = mViewWidth;
        mTexHeight = mViewHeight;
        recycleTexture(mShareTextureId);
        EGLUtils.deleteFrameBuffer(mIdFBO);
        mShareTextureId = EGLUtils.createTextureId(true, mViewWidth, mViewHeight);
        mIdFBO = EGLUtils.setupBuffers(mShareTextureId);
        recycleTexture(mShareTextureId2);
        EGLUtils.deleteFrameBuffer(mIdFBO2);
        mShareTextureId2 = EGLUtils.createTextureId(true, mViewWidth, mViewHeight);
        mIdFBO2 = EGLUtils.setupBuffers(mShareTextureId2);
        recycleTexture(mShareTextureId3);
        EGLUtils.deleteFrameBuffer(mIdFBO3);
        mShareTextureId3 = EGLUtils.createTextureId(true, Contants.FACE_DATA_WIGHT, Contants.FACE_DATA_HEIGHT);
        mIdFBO3 = EGLUtils.setupBuffers(mShareTextureId3);
        Log.d(TAG, "initShareTexture texture id:" + mShareTextureId);
        Log.i(TAG, "initEffectTexture end");
    }

    public Bitmap getPreviewBitmap() {
        synchronized (mSyncObject) {
            Log.i(TAG, "getPreviewBitmap: " + mPreviewBitmap);
            return mPreviewBitmap;
        }

    }

    public void generatePrevieBitmap() {
        Log.i(TAG, "generatePrevieBitmap");
        mGetBitmap = true;
        try {
            synchronized (mWaitLock) {
                Log.i(TAG, "mWaitLock wait");
                mWaitLock.wait();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

    }

    public void setFilterType(Contants.FilterType filtertype) {
        Log.i(TAG, " set filtertype: " + filtertype.getType() + "mCurrentFilterType.getType():" + mCurrentFilterType.getType());
        synchronized (mSyncObjectForPreview) {
            if (mCurrentFilterType != filtertype) {
                mCurrentFilterType = filtertype;
                mNeedChangeFilter = true;
            }
            CameraData.getInstance().setFilterType(mCurrentFilterType);
        }
    }

    public void setFaceBeautyOn(boolean fbOn) {
        Log.i(TAG, "setFaceBeautyOn: " + fbOn);
        mFaceBeautyOn = fbOn;
        CameraData.getInstance().setFaceBeautyOn(fbOn);
    }

    private boolean needDofaceBeauty() {
        return mFaceBeautyOn && CameraUtils.isSupprtFaceBeauty(CameraData.getInstance().getCurrentModeType());
    }

    private boolean needDrawYuv() {
        return false;
    }

    private void createPreviewBitmap(int width, int height) {
        synchronized (mSyncObject) {
            Log.i(TAG, "createPreviewBitmap:" + width + "x" + height);
            ByteBuffer pixelBuffer = ByteBuffer.allocate(width * height * 4);
            //左下角为原点
            GLES20.glReadPixels(0, 0, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, pixelBuffer);
            ByteBuffer buffer = ByteBuffer.wrap(pixelBuffer.array());
            if (mPreviewBitmap != null && !mPreviewBitmap.isRecycled()) {
                mPreviewBitmap.recycle();
            }
            mPreviewBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            mPreviewBitmap.copyPixelsFromBuffer(buffer);
            mPreviewBitmap = BitmapUtils.verticalConvert(mPreviewBitmap);

            synchronized (mWaitLock) {
                Log.i(TAG, "mWaitLock notifyAll");
                mWaitLock.notifyAll();
            }
        }
    }

    private int getLubTexture() {
        recycleTexture(mLutTextureId);
        int lutTextureId = createLutTexture();
        Log.d(TAG, "getLubTexture:" + mCurrentFilterType.getName() + " id:" + lutTextureId);
        return lutTextureId;
    }

    private int createLutTexture() {
        Log.d(TAG, "createLutTexture");
        int lutTextureId = -1;
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        Bitmap bitmap = BitmapFactory.decodeResource(mContext.getResources(), mCurrentFilterType.getId(), options);
        lutTextureId = EGLUtils.createTextureIdFromBitmap(bitmap, bitmap.getWidth(), bitmap.getHeight());
        return lutTextureId;
    }


    public void cleanData() {
        synchronized (mSyncObjectForPreview) {
            if (mData != null) {
                mData = null;
            }
        }
    }

    public void setNeedBigEyes(boolean needBigEyes) {
        mNeedBigEyes = needBigEyes;
    }

    public boolean getNeedBigEyes() {
        return mNeedBigEyes;
    }


}

package com.allwinner.camera.program;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.Rect;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.Log;
import android.util.SparseArray;

import com.alibaba.android.mnnkit.actor.FaceDetector;
import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.drawer.BaseDrawer;
import com.allwinner.camera.drawer.BigEyeDrawer;
import com.allwinner.camera.drawer.DrawerFactory;
import com.allwinner.camera.drawer.LutDrawer;
import com.allwinner.camera.ui.UIManager;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.EGLUtils;
import com.allwinner.camera.utils.YuvUtils;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import static android.opengl.GLES11Ext.GL_TEXTURE_EXTERNAL_OES;
import static android.opengl.GLES20.GL_BLEND;
import static android.opengl.GLES20.GL_COLOR_BUFFER_BIT;
import static android.opengl.GLES20.GL_DEPTH_BUFFER_BIT;
import static android.opengl.GLES20.GL_LUMINANCE;
import static android.opengl.GLES20.GL_LUMINANCE_ALPHA;
import static android.opengl.GLES20.GL_TEXTURE0;
import static android.opengl.GLES20.GL_TEXTURE1;
import static android.opengl.GLES20.GL_TEXTURE2;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.GL_UNSIGNED_BYTE;
import static android.opengl.GLES20.glActiveTexture;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glClear;
import static android.opengl.GLES20.glClearColor;
import static android.opengl.GLES20.glGetAttribLocation;
import static android.opengl.GLES20.glGetUniformLocation;
import static android.opengl.GLES20.glTexImage2D;
import static android.opengl.GLES20.glUniform1f;
import static android.opengl.GLES20.glUniform1i;
import static android.opengl.GLES20.glUniform2f;
import static com.allwinner.camera.data.Contants.FilterType.Normal;
import static com.allwinner.camera.data.Contants.FilterType.YUV;

public class FilterCaptureEngine {

    private UIManager mUiManager;
    private boolean mNeedChangeProgram = false;
    private FaceDetector mFaceDetector;
    private FaceDetectionReport[] mFaceResult;
    private float mRatio;
    public interface FilterDataListener {
        void onFilterComplete(PramData pramData, byte[] yuvdata, byte[] nv21data);

        void onFilterDoing();
    }

    public static class PramData {
        public int mJpegRotation;
        public long mCaptureTime;
        public int mWidth;
        public int mHeight;
        public boolean mMirror;
    }

    private final Object mSyncObject = new Object();
    private final static String TAG = "FilterCaptureEngine";
    private CameraEffectRender mGLRenderer;
    private boolean mThreadinited = false;
    private int mShareTextureId = -1;
    private int mShareTextureId2 = -1;
    private int mLutTextureId = -1;
    private int mWaterSignTextureId = -1;
    private int mIdFBO = -1;
    private int mIdFBO2 = -1;

    private FilterDataListener mListener;
    private int mPicWidth = 0;
    private int mPicHeight = 0;
    private Context mContext;
    private final float[] mMVPMatrix = new float[16];
    private float[] mIndentityMatrix = new float[16];
    private final float[] mMirrorMatrix = new float[16];
    private float[] mProjectionMatrix = new float[16];
    private float[] mViewMatrix = new float[16];
    private Object mFaceResultSyncObject = new Object();
    private SparseArray<PramData> mPramArray = new SparseArray<>();
    private final AtomicInteger mPramIndex = new AtomicInteger(1);

    private boolean mNeedChangeFilter = false;

    private boolean mFaceBeautyOn = false;

    private Contants.FilterType mCurrentFilterType = YUV;

    public FilterCaptureEngine(Context context, UIManager uiManager) {
        mContext = context;
        mUiManager = uiManager;
        mGLRenderer = new CameraEffectRender();
        Matrix.setIdentityM(mIndentityMatrix,0);
        Matrix.setIdentityM(mMVPMatrix, 0);
        Matrix.rotateM(mMVPMatrix, 0, 180, 0, 1, 0);
        Matrix.rotateM(mMVPMatrix, 0, 180, 0, 0, 1);
        Matrix.setIdentityM(mMirrorMatrix, 0);
    }


    public void init() {
        // mfaceTracking =  new FaceTracking("/sdcard/ZeuseesFaceTracking/models");
        synchronized (mSyncObject) {
            Log.i(TAG, "init");
            if (!mThreadinited) {
                if (mGLRenderer != null) {
                    mThreadinited = true;
                    mGLRenderer.init();
                }
            }
            Log.i(TAG, "init end");
        }
    }

    public void setPictureSize(int width, int height) {
        mPicWidth = width;
        mPicHeight = height;
        Log.i(TAG, "setPictureSize:" + mPicWidth + "x" + mPicHeight);
        mRatio = (float) width / height;
        Matrix.frustumM(mProjectionMatrix, 0, -mRatio, mRatio, -1.0f, 1.0f, 3.0f, 9.0f);
        Matrix.setLookAtM(mViewMatrix, 0, 0, 0, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    }

    public void doFilter(PramData pramData, byte[] data) {
        Log.i(TAG, "doFilter:" + "height:" + pramData.mHeight + "width:" + pramData.mWidth);
        if (mFaceBeautyOn||CameraUtils.needDoSticker()) {
            int inAngle = CameraUtils.getInAngle();
            int outAngle = 0;
            mFaceResult = CameraUtils.doFaceDetector(mUiManager, mFaceDetector, data, pramData.mWidth, pramData.mHeight, inAngle, outAngle, CameraData.getInstance().isCurrentFront(), true);
        }
        int index = mPramIndex.getAndIncrement();
        Log.i(TAG, "doFilter put pramData for index: " + index);
        data = YuvUtils.rotateNV21(data, pramData.mWidth, pramData.mHeight, pramData.mJpegRotation, pramData.mMirror);
        if (pramData.mJpegRotation == 90 || pramData.mJpegRotation == 270) {
            int temp = pramData.mWidth;
            pramData.mWidth = pramData.mHeight;
            pramData.mHeight = temp;
        }
        setPictureSize(pramData.mWidth, pramData.mHeight);
        mPramArray.put(index, pramData);
        mGLRenderer.draw(data, index);
    }

    public void createKitSuccess(FaceDetector faceDetector) {
        mFaceDetector = faceDetector;
    }

    public void setFilterDataListener(FilterDataListener listener) {
        mListener = listener;
    }

    public void release() {
        Log.i(TAG, "release");
        if (mGLRenderer != null) {
            mGLRenderer.release();
        }
        if (mGLRenderer != null && mThreadinited) {
            mGLRenderer.destroy();
            mThreadinited = false;
        }
    }

    public void setFilterType(Contants.FilterType filtertype) {

        if (!mCurrentFilterType.getType().equals(filtertype.getType())) {
            mNeedChangeProgram = true;
        }
        if (mCurrentFilterType  != filtertype || (filtertype.isLut() && !needDofaceBeauty())) {
            Log.i(TAG, "mCurrentFilterType: " + mCurrentFilterType + "filtertype:" + filtertype);
            mCurrentFilterType = filtertype;
            mNeedChangeFilter = true;
        }
        CameraData.getInstance().setFilterType(mCurrentFilterType);
    }

    public void setFaceBeautyOn(boolean fbOn) {
        mFaceBeautyOn = fbOn;
        mNeedChangeFilter = true;
    }

    private boolean needDofaceBeauty() {
        return mFaceBeautyOn && CameraUtils.isSupprtFaceBeauty(CameraData.getInstance().getCurrentModeType());
    }

    public boolean getFaceBeautyOn() {
        return needDofaceBeauty();
    }

    public Contants.FilterType getFilterType() {
        return mCurrentFilterType;
    }

    private class CameraEffectRender {
        private HandlerThread mHandlerThread;

        private Handler mHandler;
        private EglSurfaceManager mSurfaceManager = null;
        private static final int MSG_INIT = 0;
        private static final int MSG_RENDER = 1;
        private static final int MSG_DEINIT = 2;
        private static final int MSG_ATTACH = 3;
        private static final int MSG_RELEASE = 4;
        private static final int MSG_INITEFFECTTEX = 5;
        private int mTexWidth = 0;
        private int mTexHeight = 0;

        private DrawerFactory mDrawerFactory = null;
        private final String WATERSIGN = "watersign";



        public void init() {
            Log.d(TAG, "init");
            mHandlerThread = new HandlerThread("Effect Renderer Thread");
            mHandlerThread.start();
            mHandler = new Handler(mHandlerThread.getLooper()) {
                @Override
                public void handleMessage(Message msg) {
                    switch (msg.what) {
                        case MSG_INIT:
                            initEGL();
                            return;
                        case MSG_RENDER:
                            byte[] data = (byte[]) msg.obj;
                            Bundle data2 = msg.getData();
                            int index = data2.getInt("index");
                            Log.i(TAG, "get pramData for index: " + index);
                            PramData pramData = mPramArray.get(index);
                            if (pramData != null) {
                                drawFrame(data, pramData);
                                mPramArray.remove(index);
                            } else {
                                Log.d(TAG, "no send pramData");
                            }
                            return;
                        case MSG_RELEASE:
                            onRelease();
                            return;
                        case MSG_DEINIT:
                            onDestroyGL();
                            return;
                        default:
                            return;
                    }
                }
            };
            //初始化EGL环境
            mHandler.sendEmptyMessage(MSG_INIT);
        }

        private void draw(byte[] data, int index) {
            if (mHandler != null) {
                Message msg = new Message();
                msg.obj = data;
                msg.what = MSG_RENDER;
                Bundle bundle = new Bundle();
                bundle.putInt("index", index);
                msg.setData(bundle);
                mHandler.sendMessage(msg);
            }
        }

        private void release() {
            if (mHandler != null) {
                mHandler.sendEmptyMessage(MSG_RELEASE);
            }
        }

        private void destroy() {
            if (mHandler != null) {
                mHandler.sendEmptyMessage(MSG_DEINIT);
            }
        }


        private void initEGL() {
            Log.d(TAG, "initEGL");
            mSurfaceManager = new EglSurfaceManager();
            mSurfaceManager.makeCurrent();
            mDrawerFactory = new DrawerFactory(mContext);
        }

        /**
         * 销毁OpenGL环境
         */
        private void onDestroyGL() {
            Log.i(TAG, "onDestroyGL");
            //recycleShareTexture();
            recycleTexture(mShareTextureId);
            EGLUtils.deleteFrameBuffer(mIdFBO);
            recycleTexture(mShareTextureId2);
            EGLUtils.deleteFrameBuffer(mIdFBO2);
            recycleTexture(mLutTextureId);
            recycleTexture(mWaterSignTextureId);
            mShareTextureId = -1;
            mShareTextureId2 = -1;
            mLutTextureId = -1;
            mWaterSignTextureId = -1;
            if (mDrawerFactory != null) {
                mDrawerFactory.release();
                mDrawerFactory = null;
            }
            mSurfaceManager.release();
            if (mHandlerThread.getLooper() != null) {
                Log.i(TAG, "quit");
                mHandlerThread.getLooper().quit();
            }
        }

        private void onRelease() {
            Log.i(TAG, "onRelease");
        }

        public void initEffectTexture() {
            if ((mTexWidth == mPicWidth) && (mTexHeight == mPicHeight) && (mShareTextureId != -1)) {
                Log.d(TAG, "the same size ,no need to init texture:" + mTexWidth + "x" + mTexHeight);
                return;
            }
            mTexWidth = mPicWidth;
            mTexHeight = mPicHeight;
            Log.d(TAG, "initEffectTexture start: " + mTexWidth + "x" + mTexHeight);
            recycleTexture(mShareTextureId);
            EGLUtils.deleteFrameBuffer(mIdFBO);

            mShareTextureId = EGLUtils.createTextureId(true, mTexWidth, mTexHeight);
            mShareTextureId2 = EGLUtils.createTextureId(true, mTexWidth, mTexHeight);
            mIdFBO = EGLUtils.setupBuffers(mShareTextureId);
            mIdFBO2 = EGLUtils.setupBuffers(mShareTextureId2);
            Log.i(TAG, "initEffectTexture end");
        }

        private void recycleTexture(int textureid) {
            Log.e(TAG, "glDeleteTextures texture:" + textureid);
            if (textureid != -1) {
                GLES20.glDeleteTextures(1, new int[]{textureid}, 0);
                EGLUtils.checkEglError("glDeleteTextures");
                textureid = -1;
            }
        }

        private void drawFrame(byte[] data, PramData pramData) {
            mListener.onFilterDoing();
            initEffectTexture();
            if (mNeedChangeFilter) {
                if (mCurrentFilterType.isLut()) {
                    mLutTextureId = getLubTexture();
                }
                mNeedChangeFilter = false;
            }
            ByteBuffer yuvbuffer = ByteBuffer.allocate(mPicWidth * mPicHeight * 4);
            yuvbuffer.clear();
            yuvbuffer.position(0);
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mIdFBO);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            GLES20.glEnable(GL_BLEND);
            GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA,GLES20.GL_ONE_MINUS_SRC_ALPHA);  // s *alpha +d (1-appha)
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

            if (needDofaceBeauty()) {
                BaseDrawer bigeyeDrawer = mDrawerFactory.getDrawer(mDrawerFactory.BIGEYE,EGLUtils.TEXTURETYPE.YUV, EGLUtils.TEXTURETYPE.TEXTURE2D);
                ((BigEyeDrawer) bigeyeDrawer).setFaceResult(mFaceResult, true, mPicWidth, mPicHeight);
                bigeyeDrawer.drawYuv(data, mPicWidth, mPicHeight, mPicWidth, mPicHeight, mIndentityMatrix, mMVPMatrix);
                GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
                GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mIdFBO2);
                if (mCurrentFilterType.isLut()) {
                    BaseDrawer smoothDrawer = mDrawerFactory.getDrawer(mDrawerFactory.SMOOTH,EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.TEXTURE2D);
                    smoothDrawer.drawTexture(GL_TEXTURE_2D, mShareTextureId,mPicWidth, mPicHeight,mIndentityMatrix, mIndentityMatrix);
                    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
                    GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mIdFBO);
                    BaseDrawer lutDrawer = mDrawerFactory.getDrawer(mDrawerFactory.LUT,EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.YUV);
                    ((LutDrawer) lutDrawer).setLutTextureId(mLutTextureId);
                    lutDrawer.drawTexture(GL_TEXTURE_2D, mShareTextureId2,mPicWidth, mPicHeight, mIndentityMatrix, mIndentityMatrix);
                } else {
                    BaseDrawer smoothDrawer = mDrawerFactory.getDrawer(mDrawerFactory.SMOOTH,EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.YUV);
                    smoothDrawer.drawTexture(GL_TEXTURE_2D, mShareTextureId, mPicWidth, mPicHeight, mIndentityMatrix, mMVPMatrix);
                }
            } else {
                if (mCurrentFilterType.isLut()) {
                    BaseDrawer drawer = mDrawerFactory.getDrawer(mDrawerFactory.LUT, EGLUtils.TEXTURETYPE.YUV, EGLUtils.TEXTURETYPE.YUV);
                    ((LutDrawer) drawer).setLutTextureId(mLutTextureId);
                    drawer.drawYuv(data, mPicWidth, mPicHeight, mPicWidth, mPicHeight, mIndentityMatrix, mMVPMatrix);
                } else {
                    BaseDrawer drawer = mDrawerFactory.getDrawer(mDrawerFactory.NORMAL, EGLUtils.TEXTURETYPE.YUV, EGLUtils.TEXTURETYPE.YUV);
                    drawer.drawYuv(data, mPicWidth, mPicHeight, mPicWidth, mPicHeight, mIndentityMatrix, mMVPMatrix);
                }
            }
            if (!CameraData.getInstance().getModelWaterSignType().equals(Contants.ModelWatersignType.TYPE_OFF)) {
                drawWaterSign(0, 8 * mPicHeight / 9, mPicWidth / 4, mPicHeight / 9,false);
            }
            if(CameraData.getInstance().getIsTimeWaterSign()) {
                drawWaterSign(mPicWidth * 3 / 4, 8 * mPicHeight / 9, mPicWidth / 4, mPicHeight / 9,true);
            }
            if (CameraUtils.needDoSticker()) {
                BaseDrawer drawer = mDrawerFactory.getDrawer(Contants.STICKER, EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.YUV);
                Matrix.multiplyMM(mViewMatrix, 0, mViewMatrix, 0, mMVPMatrix, 0);
                drawer.drawSticker(CameraData.getInstance().getStickerName(), mFaceResultSyncObject, mFaceResult, mPicWidth, mPicHeight, mProjectionMatrix, mViewMatrix, mPicWidth, mPicHeight, true);
            }
            yuvbuffer.rewind();
            yuvbuffer.position(0);
            yuvbuffer.clear();
            GLES20.glReadPixels(0, 0, mPicWidth, mPicHeight, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, yuvbuffer);

            Log.i(TAG, "glReadPixels endmPicWidth:" + mPicWidth + "mPicHeight:" + mPicHeight);
            byte[] argbdata = new byte[yuvbuffer.remaining()];//拿到的是yuv数据
            yuvbuffer.get(argbdata, 0, argbdata.length);
            Log.i(TAG, "glReadPixels data length:" + argbdata.length);
            if (argbdata != null) {
                mListener.onFilterComplete(pramData, argbdata, data);
            }
            yuvbuffer.rewind();
            yuvbuffer.clear();
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
            EGLUtils.checkFrameBufferStatus();
            GLES20.glDisable(GLES20.GL_BLEND);
        }

        private void drawWaterSign(int viewX, int viewY, int viewWidth, int viewHeight, boolean isTimeWatersign) {
            BaseDrawer waterDrawer = mDrawerFactory.getDrawer(WATERSIGN, EGLUtils.TEXTURETYPE.TEXTURE2D, EGLUtils.TEXTURETYPE.YUV);
            mWaterSignTextureId = getWaterSignTexture(isTimeWatersign);
            waterDrawer.drawTexture(GL_TEXTURE_2D, mWaterSignTextureId, viewX, viewY, viewWidth, viewHeight, mIndentityMatrix, mMVPMatrix);
        }

        private int getLubTexture() {
            EGLUtils.recycleTexture(mLutTextureId);
            int lutTextureId = createBitmapTexture(mCurrentFilterType.getId());
            Log.d(TAG, "getLubTexture:" + mCurrentFilterType.getName() + " id:" + lutTextureId);
            return lutTextureId;
        }

        private int getWaterSignTexture(boolean isTimeWatersign) {
            EGLUtils.recycleTexture(mWaterSignTextureId);
            int width = mPicWidth/4;
            int height = mPicHeight/9;
            float rate = 300f/(float)width;
            Bitmap textBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            textBitmap.setHasAlpha(true);
            Canvas canvas = new Canvas(textBitmap);
            // canvas.drawColor(Color.BLACK);
            Paint paint = new Paint();
            paint.setColor(Color.WHITE);
            paint.setAntiAlias(true);
            paint.setTextAlign(Paint.Align.LEFT);
            Rect rect = new Rect(0, 0, width, height);
            Paint.FontMetricsInt fontMetrics = paint.getFontMetricsInt();
            // 将文字绘制在矩形区域的正中间
            int baseline = (rect.bottom + rect.top - fontMetrics.bottom - fontMetrics.top)/4;
            if(isTimeWatersign){
                paint.setFakeBoldText(false);
                paint.setTextSize(20/rate);
                SimpleDateFormat formatter  =  new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");
                Date curDate =  new Date(System.currentTimeMillis());
                String  dateStr   =   formatter.format(curDate);
                canvas.drawText(dateStr, 50/rate, baseline + 30/rate, paint);
            }else{
                paint.setFakeBoldText(true);
                paint.setTextSize(23/rate);
                canvas.drawText("SHOT BY", 50/rate, baseline, paint);
                canvas.drawText(android.os.Build.MODEL, 50/rate,  baseline+ 30/rate, paint);
                if(CameraData.getInstance().getModelWaterSignType().equals(Contants.ModelWatersignType.TYPE_CUSTOM)){
                    canvas.drawText(CameraData.getInstance().getCustomText(), 50/rate,  baseline + 60/rate, paint);
                }
            }
            int waterSignTextureId = createBitmapTexture(textBitmap);
            Log.d(TAG, "waterSignTextureId:" + waterSignTextureId);
            return waterSignTextureId;
        }

        private int createBitmapTexture(int id) {
            Log.d(TAG, "createBitmapTexture");
            int lutTextureId = -1;
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inScaled = false;
            Bitmap bitmap = BitmapFactory.decodeResource(mContext.getResources(), id, options);
            lutTextureId = EGLUtils.createTextureIdFromBitmap(bitmap, bitmap.getWidth(), bitmap.getHeight());
            return lutTextureId;
        }
        private int createBitmapTexture(Bitmap bitmap) {
            Log.d(TAG, "createBitmapTexture");
            int lutTextureId = -1;
            lutTextureId = EGLUtils.createTextureIdFromBitmap(bitmap, bitmap.getWidth(), bitmap.getHeight());
            return lutTextureId;
        }
    }
}
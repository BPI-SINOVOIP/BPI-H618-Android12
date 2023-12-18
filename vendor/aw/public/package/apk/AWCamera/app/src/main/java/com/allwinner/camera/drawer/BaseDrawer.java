package com.allwinner.camera.drawer;

import android.content.Context;
import android.opengl.Matrix;
import android.os.AsyncTask;
import android.util.Log;

import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.allwinner.camera.data.DynamicSticker;
import com.allwinner.camera.data.DynamicStickerNormalData;
import com.allwinner.camera.program.BaseProgram;
import com.allwinner.camera.stickers.DynamicStickerLoader;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.EGLUtils;

import org.json.JSONException;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static android.opengl.GLES20.GL_TEXTURE_2D;

public class BaseDrawer {
    private int mOutputTextureId = -1;
    private int mYTextureId = -1;
    private int mUVTextureId = -1;
    protected Context mContext = null;
    Map<String, List<DynamicStickerLoader>> mMap = new HashMap<String, List<DynamicStickerLoader>>();

    // 贴纸顶点
    private float[] mStickerVertices = new float[8];
    private float[] mMVPMatrix = new float[16];
    private float[] mIndentityMatrix = new float[16];

    protected BaseProgram mRenderProram = null;

    protected EGLUtils.TEXTURETYPE mInputType = EGLUtils.TEXTURETYPE.OESTEXTURE;

    protected EGLUtils.TEXTURETYPE mOutputType = EGLUtils.TEXTURETYPE.OESTEXTURE;
    public static final float TextureVertices_flipx[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
    };
    public BaseDrawer(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        mInputType = inputType;
        mOutputType = outputType;
    }

    public boolean setType(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        boolean mIsChange = false;
        mInputType = inputType;
        mOutputType = outputType;
        if (mRenderProram != null && (mRenderProram.getInputType() != mInputType || mRenderProram.getOutputType() != mOutputType)) {
            mRenderProram.release();
            mRenderProram = new BaseProgram(mInputType, mOutputType);
            mRenderProram.initProgram(mContext);
            mIsChange = true;
        }
        return mIsChange;
    }

    /**
    must be call by EGLThread
    * */
    public void initEGL(Context context){
        mContext = context;
        if (mRenderProram == null) {
            mRenderProram= new BaseProgram(mInputType,mOutputType);
            mRenderProram.initProgram(context);
        }
        Matrix.setIdentityM(mMVPMatrix, 0);
        Matrix.setIdentityM(mIndentityMatrix, 0);
    }

    public int getOutTexture() {
        return mOutputTextureId;
    }

    public void drawTexture(int traget, int texture, int viewWidth, int viewHeight, float[] texMatrix, float[] mvpMartix) {
        drawTexture(traget, texture, 0, 0, viewWidth, viewHeight, texMatrix, mvpMartix);
    }

    private void parseJson(String stickerName){
        DynamicSticker dynamicSticker = null;
         List<DynamicStickerLoader> stickerLoaderList = new ArrayList<>();
        try {
            dynamicSticker = CameraUtils.decodeStickerData(mContext, stickerName);
        } catch (IOException e) {
            e.printStackTrace();
        } catch (JSONException e) {
            e.printStackTrace();
        }
        // 创建贴纸加载器列表
        if (dynamicSticker != null && dynamicSticker.dataList != null) {
            for (int i = 0; i < dynamicSticker.dataList.size(); i++) {
                if (dynamicSticker.dataList.get(i) instanceof DynamicStickerNormalData) {
                    String path = dynamicSticker.folderPath + "/" + dynamicSticker.dataList.get(i).childStickerName;
                    stickerLoaderList.add(new DynamicStickerLoader(dynamicSticker.dataList.get(i), path));
                }
            }
            if (stickerLoaderList != null) {
                mMap.put(stickerName, stickerLoaderList);
            }
        }
    }
    public void drawSticker(String stickerName, Object faceResultSyncObject, FaceDetectionReport[] faceResult, int viewWidth, int viewHeight, float[] projectionMatrix,
                            float[] viewMatrix, int faceWidth,int faceHeight,boolean isCapture) {
        if (stickerName.equals("")) {
            return;
        }
        if (mMap != null && !mMap.containsKey(stickerName)) {
            parseJson(stickerName);
        }
        synchronized (faceResultSyncObject) {
            if (mMap != null && mMap.containsKey(stickerName)) {
                if (mMap.get(stickerName).size() > 0 && faceResult != null && faceResult.length > 0) {
                    if (faceResult[0].score > 0.5f) {
                        for (int stickerIndex = 0; stickerIndex < this.mMap.get(stickerName).size(); stickerIndex++) {
                            mMap.get(stickerName).get(stickerIndex).updateStickerTexture(faceResult, mContext);
                            CameraUtils.calculateStickerVertices((DynamicStickerNormalData) mMap.get(stickerName).get(stickerIndex).getStickerData(), faceResult, viewWidth, viewHeight, mStickerVertices,
                                    projectionMatrix, viewMatrix, mMVPMatrix,faceWidth,faceHeight,isCapture);
                            drawStickerbyTexture(mMap.get(stickerName).get(stickerIndex).getStickerTexture(),viewWidth,viewHeight);
                        }
                    }
                }
            }
        }
    }

    public void drawStickerbyTexture(int stickerTexture,int viewWidth,int viewHeight) {
        updateVeticesBuffer(mStickerVertices);
        updateUVBuffer(TextureVertices_flipx);
        drawTexture(GL_TEXTURE_2D, stickerTexture, viewWidth, viewHeight, mIndentityMatrix, mMVPMatrix);
    }

    public void drawTexture(int traget, int texture, int offsetX, int offsetY, int viewWidth, int viewHeight, float[] texMatrix, float[] mvpMartix) {
        mOutputTextureId = texture;
        if (mRenderProram != null) {
            mRenderProram.drawTexture(traget, texture, offsetX, offsetY, viewWidth, viewHeight, texMatrix, mvpMartix);
        }
    }

    public void drawYuv(byte[] yuvdata, int datWidth ,int dataHeight, int viewWidth, int viewHeight, float[] texMatrix, float[] mvpMartix) {
        if (mYTextureId == -1) {
            mYTextureId = EGLUtils.createTextureId(GL_TEXTURE_2D);
        }
        if (mUVTextureId == -1) {
            mUVTextureId = EGLUtils.createTextureId(GL_TEXTURE_2D);
        }
        ByteBuffer byteBuffer = ByteBuffer.wrap(yuvdata);
        byteBuffer.clear();
        byteBuffer.position(0);
        EGLUtils.setYuv(yuvdata,byteBuffer,datWidth,dataHeight,mYTextureId,mUVTextureId);
        if (mRenderProram != null) {
            mRenderProram.drawYuv(mYTextureId,mUVTextureId,viewWidth,viewHeight,texMatrix,mvpMartix);
        }
    }


    public void updateVeticesBuffer(float[] triangleVertices){
        if (mRenderProram != null) {
            mRenderProram.updateVeticesBuffer(triangleVertices);
        }
    }

    public void updateUVBuffer(float[] triangleUV){
        if (mRenderProram != null) {
            mRenderProram.updateUVBuffer(triangleUV);
        }
    }

    public FloatBuffer getPostionBuffer() {
        return mRenderProram.getPostionBuffer();
    }

    public FloatBuffer getCoordBuffer() {
        return mRenderProram.getCoordBuffer();
    }

    public void release() {
        EGLUtils.recycleTexture(mYTextureId);
        EGLUtils.recycleTexture(mUVTextureId);
        if (mRenderProram != null) {
            mRenderProram.release();
        }
    }

}

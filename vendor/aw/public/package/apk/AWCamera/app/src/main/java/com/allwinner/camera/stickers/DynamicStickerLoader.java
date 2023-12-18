package com.allwinner.camera.stickers;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.Log;

import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.allwinner.camera.data.DynamicStickerData;
import com.allwinner.camera.utils.BitmapUtils;
import com.allwinner.camera.utils.EGLUtils;

import java.io.IOException;
import java.io.InputStream;


/**
 * 动态贴纸加载器
 */
public class DynamicStickerLoader {

    private static final String TAG = "DynamicStickerLoader";

    // 贴纸纹理
    private int mStickerTexture;
    // 暂存纹理id，用于复用
    private int mRestoreTexture;
    // 贴纸所在的文件夹
    private String mFolderPath;
    // 贴纸数据
    private DynamicStickerData mStickerData;
    // 当前索引
    private int mFrameIndex = -1;
    // 当前时间
    private long mCurrentTime = -1L;

    public DynamicStickerLoader( DynamicStickerData stickerData, String folderPath) {

        mStickerTexture = EGLUtils.GL_NOT_TEXTURE;
        mRestoreTexture = EGLUtils.GL_NOT_TEXTURE;
        mFolderPath =  folderPath;
        mStickerData = stickerData;
        mStickerTexture = EGLUtils.GL_NOT_TEXTURE;
        mRestoreTexture =EGLUtils.GL_NOT_TEXTURE;

    }

    /**
     * 更新贴纸纹理
     */
    public void updateStickerTexture(FaceDetectionReport[] faceResult, Context context) {
        // 判断人脸是否存在
        if (faceResult.length<=0) {
            mCurrentTime = -1L;
            return;
        }

        // 处理贴纸索引
        if (mCurrentTime == -1L) {
            mCurrentTime = System.currentTimeMillis();
        }
        int frameIndex = (int) ((System.currentTimeMillis() - mCurrentTime) / mStickerData.duration);
        if (frameIndex >= mStickerData.frames) {
            if (!mStickerData.stickerLooping) {
                mCurrentTime = -1L;
                mRestoreTexture = mStickerTexture;
                mStickerTexture = EGLUtils.GL_NOT_TEXTURE;
                mFrameIndex = -1;
                return;
            }
            frameIndex = 0;
            mCurrentTime = System.currentTimeMillis();
        }
        if (frameIndex < 0) {
            frameIndex = 0;
        }
        if (mFrameIndex == frameIndex) {
            return;
        }
        // 根据帧索引读取贴纸
        Bitmap bitmap = null;
        if (bitmap == null) {
            String name = String.format(mStickerData.childStickerName + "_%03d.png", new Object[]{frameIndex});

            InputStream is = null;
            try {
               is = context.getAssets().open(mFolderPath+"/"+name);
            } catch (IOException e) {
                if(is != null) {
                    try {
                        is.close();
                    } catch (IOException e1) {
                        e1.printStackTrace();
                    }
                }
                e.printStackTrace();
            }
            bitmap = BitmapUtils.getBitmapFromStream(is);
        }
        if (null != bitmap) {
            // 如果此时暂存的纹理ID存在，则复用该ID
            if (mStickerTexture == EGLUtils.GL_NOT_TEXTURE
                    && mRestoreTexture != EGLUtils.GL_NOT_TEXTURE) {
                mStickerTexture = mRestoreTexture;
            }
            if (mStickerTexture == EGLUtils.GL_NOT_TEXTURE) {
                mStickerTexture = EGLUtils.createTextureIdFromBitmap(bitmap, bitmap.getWidth(), bitmap.getHeight());
            } else {
                EGLUtils.setBitmap(bitmap,  bitmap.getWidth(), bitmap.getHeight(),mStickerTexture);
              //  mStickerTexture = EGLUtils.createTexture(bitmap, mStickerTexture, bitmap.getWidth(), bitmap.getHeight());
            }
            mRestoreTexture = mStickerTexture;
            mFrameIndex = frameIndex;
            bitmap.recycle();
        } else {
            mRestoreTexture = mStickerTexture;
            mStickerTexture = EGLUtils.GL_NOT_TEXTURE;
            mFrameIndex = -1;
        }
    }

    /**
     * 释放资源
     */
    public void release() {
        if (mStickerTexture == EGLUtils.GL_NOT_TEXTURE) {
            mStickerTexture = mRestoreTexture;
        }
        EGLUtils.recycleTexture(mStickerTexture);
        mStickerTexture = EGLUtils.GL_NOT_TEXTURE;
        mRestoreTexture = EGLUtils.GL_NOT_TEXTURE;

    }

    /**
     * 获取贴纸纹理
     * @return
     */
    public int getStickerTexture() {
        return mStickerTexture;
    }

    /**
     * 最大贴纸渲染次数
     * @return
     */
    public int getMaxCount() {
        return mStickerData == null ? 0 : mStickerData.maxCount;
    }

    /**
     * 获取贴纸参数对象
     * @return
     */
    public DynamicStickerData getStickerData() {
        return mStickerData;
    }
}

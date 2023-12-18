package com.allwinner.camera.utils;

import android.graphics.Bitmap;
import android.opengl.EGL14;
import android.opengl.GLES20;
import android.opengl.GLES30;
import android.util.Log;

import java.nio.ByteBuffer;

import static android.opengl.GLES20.GL_LUMINANCE;
import static android.opengl.GLES20.GL_LUMINANCE_ALPHA;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.GL_UNSIGNED_BYTE;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glTexImage2D;

public class EGLUtils {
    private final static String TAG = "EGLUtils";
    public static final int GL_NOT_TEXTURE = -1;

    public enum TEXTURETYPE {
        YUV,
        TEXTURE2D,
        OESTEXTURE
    }

    public static int createTextureId(int target) {
        int[] textures = new int[1];
        //参数1：数量 2：纹理数组 3：偏移量 创建纹理
        GLES20.glGenTextures(1, textures, 0);
        //绑定纹理
        glBindTexture(target, textures[0]);
        GLES20.glTexParameterf(target,
                GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(target,
                GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(target,
                GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameterf(target,
                GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        int error;
        if ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
            Log.e(TAG, "Create Texture failed!:" + error);
        }
        glBindTexture(target, 0);
        return textures[0];
    }
    public static int createTextureId(boolean needInit, int mTexWidth, int mTexHeight) {
        int[] textures = new int[1];
        //第一个参数为需要的纹理数，第二个参数为存储获得的纹理ID的数组，第三个参数为数组的起始位置。
        GLES20.glGenTextures(1, textures, 0);
        glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
        if (needInit) {
            glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, mTexWidth, mTexHeight, 0, GLES20.GL_RGBA, GL_UNSIGNED_BYTE, null);
        }
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
                GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
                GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
                GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D,
                GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        int error;
        //if (GLES20.glGetError() != GLES20.GL_NO_ERROR) {
        if ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
            Log.i(TAG, "Create Texture failed!:" + error);
        }
        glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        Log.d(TAG, "createTextureId:" + textures[0]);
        return textures[0];
    }

    public static void recycleTexture(int textureid) {
        Log.e(TAG, "glDeleteTextures texture:" + textureid);
        if (textureid != -1) {
            GLES20.glDeleteTextures(1, new int[]{textureid}, 0);
            checkEglError("glDeleteTextures");
            textureid = -1;
        }
    }

    public static void checkEglError(String msg) {
        int error;
        if ((error = EGL14.eglGetError()) != EGL14.EGL_SUCCESS) {
            Log.d(TAG, msg + ": EGL error: 0x" + Integer.toHexString(error));
            throw new RuntimeException(msg + ": EGL error: 0x" + Integer.toHexString(error));
        }
    }

    public static int setupBuffers(int textureId) {
        Log.d(TAG, "setupBuffers");
        //framebuffer
        int buffers[] = new int[1];
        //生成FrameBuffer
        GLES20.glGenFramebuffers(1, buffers, 0);
        int idFBO = buffers[0];
        //绑定FrameBuffer
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, idFBO);
        //为FrameBuffer挂载mShareTextureId来存储颜色
        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D, textureId, 0);
        EGLUtils.checkEglError("glFrameBufferTexture2D");
        EGLUtils.checkFrameBufferStatus();
        EGLUtils.checkEglError("glClear setupBuffers");
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        return idFBO;
    }

    public static void deleteFrameBuffer(int idFBO) {
        if (idFBO != -1) {
            GLES20.glDeleteBuffers(1, new int[]{idFBO}, 0);
            EGLUtils.checkEglError("glDeleteBuffers deleteFrameBuffer");
        }
    }
    public static void setBitmap(Bitmap bitmap, int w, int h, int mTextureID) {
        if (bitmap != null) {
            glBindTexture(GL_TEXTURE_2D, mTextureID);
            EGLUtils.checkEglError("glBindTexture");

            ByteBuffer buffer = ByteBuffer.allocate(w * h * 4);
            bitmap.copyPixelsToBuffer(buffer);
            buffer.position(0);
            //指定一个二维图片
            glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, w, h, 0, GLES20.GL_RGBA, GL_UNSIGNED_BYTE, buffer);
            EGLUtils.checkEglError("glTexImage2D");

            GLES20.glTexParameteri(GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
            EGLUtils.checkEglError("glTexParameteri");

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    public static void set3DBitmap(Bitmap bitmap, int width, int height, int depth, int mTextureId) {
        if (width > 0 && height > 0 && depth > 0 && bitmap != null) {
            glBindTexture(GLES30.GL_TEXTURE_3D, mTextureId);
            checkEglError("glBindTexture");

            ByteBuffer buffer = ByteBuffer.allocate(width * height * depth * 4);
            bitmap.copyPixelsToBuffer(buffer);
            buffer.position(0);

            GLES30.glTexImage3D(GLES30.GL_TEXTURE_3D, 0, GLES30.GL_RGBA, width, height, depth, 0, GLES30.GL_RGBA, GL_UNSIGNED_BYTE, buffer);
            checkEglError("glTexImage3D");

            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR);
            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR);
            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE);
            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE);
            GLES30.glTexParameteri(GLES30.GL_TEXTURE_3D, GLES30.GL_TEXTURE_WRAP_R, GLES30.GL_CLAMP_TO_EDGE);
            checkEglError("glTexParameteri");

            glBindTexture(GLES30.GL_TEXTURE_3D, 0);
            checkEglError("glBindTexture");
        }
    }

    /**
     * 使用旧的Texture 创建新的Texture (宽高不能大于旧Texture的宽高，主要用于贴纸不断切换图片)
     * @param bitmap
     * @param texture
     * @return
     */
    public static int createTexture(Bitmap bitmap, int texture,int w,int h) {
        int[] result = new int[1];
        if (texture == GL_NOT_TEXTURE) {
            result[0] = createTextureIdFromBitmap(bitmap,w, h);
        } else {
            result[0] = texture;
            if (bitmap != null && !bitmap.isRecycled()) {
                glBindTexture(GLES20.GL_TEXTURE_2D, result[0]);
                EGLUtils.checkEglError("glBindTexture");
                ByteBuffer buffer = ByteBuffer.allocate(w * h * 4);
                bitmap.copyPixelsToBuffer(buffer);
                buffer.position(0);
                //指定一个二维图片
                GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, w, h, 0, GLES20.GL_RGBA, GL_UNSIGNED_BYTE, buffer);
            }
        }
        return result[0];
    }
    public static int createTextureIdFromBitmap(Bitmap bitmap, int w, int h) {
        int textureid = -1;
        if (bitmap != null) {
            textureid = EGLUtils.createTextureId(GLES20.GL_TEXTURE_2D);
            setBitmap(bitmap, w, h,textureid);
        }
        Log.d(TAG, "createTextureFromBitmap: "  + w + "x" + h + " textureid :" + textureid);
        return textureid;
    }

    public static void checkFrameBufferStatus() {
        int status = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);
        checkEglError("glCheckFramebufferStatus");
        switch (status) {
            case GLES20.GL_FRAMEBUFFER_COMPLETE:
                Log.d(TAG, "complete");
                break;
            case GLES20.GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                Log.e(TAG, "incomplete attachment");
                break;
            case GLES20.GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                Log.e(TAG, "incomplete missing attachment");
                break;
            case GLES20.GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                Log.e(TAG, "incomplete dimensions");
                break;
            case GLES20.GL_FRAMEBUFFER_UNSUPPORTED:
                Log.e(TAG, "framebuffer unsupported");
                break;

            default:
                Log.d(TAG, "default");
        }
    }

    public static void setYuv(byte[] data, ByteBuffer yuvbuffer, int width, int height, int yTextureId, int uvTextureId) {
        //Log.i(TAG, "process data length:" + data.length);
        yuvbuffer.clear();
        yuvbuffer.position(0);
        yuvbuffer.put(data, 0, width * height).position(0);
        glBindTexture(GL_TEXTURE_2D, yTextureId);
        yuvbuffer.position(0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width,
                height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuvbuffer);
        glBindTexture(GL_TEXTURE_2D, uvTextureId);
        yuvbuffer.clear();
        yuvbuffer.position(0);
        yuvbuffer.put(data, width * height, width * height / 2).position(0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width / 2,
                height / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, yuvbuffer);
    }

}

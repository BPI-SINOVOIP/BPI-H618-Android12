package com.allwinner.camera.program;

import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLExt;
import android.opengl.EGLSurface;
import android.util.Log;
import android.view.SurfaceHolder;

public class EglSurfaceManager {


    public final static String TAG = "EglSurfaceManager";

    private static final int EGL_RECORDABLE_ANDROID = 0x3142;

    private EGLContext mEGLContext = null;
    private EGLContext mEGLSharedContext = null;
    private EGLSurface mEGLSurface = null;
    private EGLDisplay mEGLDisplay = null;
    private EGLConfig[] mEGLConfig = new EGLConfig[1];
    private int[] mSurfaceAttribs;

    private EGLDisplay mSavedEglDisplay = null;
    private EGLSurface mSavedEglDrawSurface = null;
    private EGLSurface mSavedEglReadSurface = null;
    private EGLContext mSavedEglContext = null;

    private boolean mShare = true;

    /**
     * Creates an EGL context and an EGL surface.
     */
    public EglSurfaceManager() {
        EGLContext shared = EGL14.eglGetCurrentContext();
        mEGLSharedContext = shared;
        Log.d("SurfaceManager", "eglsetup");
        eglSetup();
    }

    public EglSurfaceManager(boolean share) {
        mShare = false;
        Log.d("SurfaceManager", "eglsetup");
        eglSetup();
    }

    public void makeCurrent(SurfaceHolder holder) {
        Log.d("SurfaceManager", "makeCurrent: " + holder);
        if (mEGLSurface != null) {
            EGL14.eglDestroySurface(mEGLDisplay, mEGLSurface);
        }
        mEGLSurface = EGL14.eglCreateWindowSurface(mEGLDisplay, mEGLConfig[0], holder, mSurfaceAttribs, 0);
        checkEglError("eglCreateWindowSurface");
        if (!EGL14.eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext))
            throw new RuntimeException("eglMakeCurrent failed");

    }

    public void makeCurrent() {
        if (mEGLSurface!= null ){
            EGL14.eglDestroySurface(mEGLDisplay, mEGLSurface);
        }
        mEGLSurface = EGL14.eglCreatePbufferSurface(mEGLDisplay, mEGLConfig[0], mSurfaceAttribs, 0);
        //mEGLSurface = EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW);
        if (!EGL14.eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext))
            throw new RuntimeException("eglMakeCurrent failed");
    }

    public void swapBuffers() {
        EGL14.eglSwapBuffers(mEGLDisplay, mEGLSurface);
    }

    public void setPresentationTime(long nsecs) {
        EGLExt.eglPresentationTimeANDROID(mEGLDisplay, mEGLSurface, nsecs);
        checkEglError("setPresentationTime");
    }

    private void eglSetup() {
        mEGLDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        if (mEGLDisplay == EGL14.EGL_NO_DISPLAY) {
            Log.d("SurfaceManager", "unable to get EGL14 display");
            throw new RuntimeException("unable to get EGL14 display");
        }
        int[] version = new int[2];
        if (!EGL14.eglInitialize(mEGLDisplay, version, 0, version, 1)) {
            Log.d("SurfaceManager", "unable to initialize EGL14");
            throw new RuntimeException("unable to initialize EGL14");
        }

        // Configure EGL for recording and OpenGL ES 2.0.
        int[] attriblist = new int[]{
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
                EGL_RECORDABLE_ANDROID, 1,
                EGL14.EGL_NONE
        };
        int[] numConfigs = new int[1];

        EGL14.eglChooseConfig(mEGLDisplay, attriblist, 0, mEGLConfig, 0, mEGLConfig.length,
                numConfigs, 0);
        checkEglError("eglCreateContext RGB888+recordable ES2");

        // Configure context for OpenGL ES 2.0.
        int[] attrib_list = {
                EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL14.EGL_NONE
        };
        if (mShare) {
            mEGLContext = EGL14.eglCreateContext(mEGLDisplay, mEGLConfig[0], mEGLSharedContext, attrib_list, 0);
        } else {
            mEGLContext = EGL14.eglCreateContext(mEGLDisplay, mEGLConfig[0], EGL14.EGL_NO_CONTEXT, attrib_list, 0);
        }

        mSurfaceAttribs = new int[]{
                //EGL14.EGL_WIDTH,width,
                //EGL14.EGL_HEIGHT,height,
                EGL14.EGL_NONE
        };

        checkEglError("eglCreateContext");

        // Create a window surface, and attach it to the Surface we received.
    }

    /**
     * Discards all resources held by this class, notably the EGL context.  Also releases the
     * Surface that was passed to our constructor.
     */
    public void release() {
        if (mEGLDisplay != EGL14.EGL_NO_DISPLAY) {
            EGL14.eglMakeCurrent(mEGLDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE,
                    EGL14.EGL_NO_CONTEXT);
            EGL14.eglDestroySurface(mEGLDisplay, mEGLSurface);
            EGL14.eglDestroyContext(mEGLDisplay, mEGLContext);
            EGL14.eglReleaseThread();
            EGL14.eglTerminate(mEGLDisplay);
        }
        mEGLDisplay = EGL14.EGL_NO_DISPLAY;
        mEGLContext = EGL14.EGL_NO_CONTEXT;
        mEGLSurface = EGL14.EGL_NO_SURFACE;
        //mSurface.release();
    }

    /**
     * Checks for EGL errors. Throws an exception if one is found.
     */
    private void checkEglError(String msg) {
        int error;
        if ((error = EGL14.eglGetError()) != EGL14.EGL_SUCCESS) {
            Log.d("SurfaceManager", msg + ": EGL error: 0x" + Integer.toHexString(error));
            throw new RuntimeException(msg + ": EGL error: 0x" + Integer.toHexString(error));
        }
    }

    public void saveRenderState() {
        mSavedEglDisplay     = EGL14.eglGetCurrentDisplay();
        mSavedEglDrawSurface = EGL14.eglGetCurrentSurface(EGL14.EGL_DRAW);
        mSavedEglReadSurface = EGL14.eglGetCurrentSurface(EGL14.EGL_READ);
        mSavedEglContext     = EGL14.eglGetCurrentContext();
    }

    public void restoreRenderState() {
        if (!EGL14.eglMakeCurrent(
                mSavedEglDisplay,
                mSavedEglDrawSurface,
                mSavedEglReadSurface,
                mSavedEglContext)) {
            throw new RuntimeException("eglMakeCurrent failed");
        }
    }
}
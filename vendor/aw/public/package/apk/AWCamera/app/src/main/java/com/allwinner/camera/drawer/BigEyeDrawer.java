package com.allwinner.camera.drawer;

import android.content.Context;

import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.allwinner.camera.program.BigEyesProgram;
import com.allwinner.camera.utils.EGLUtils;

public class BigEyeDrawer extends BaseDrawer {
    public BigEyeDrawer(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        super(inputType, outputType);
    }

    @Override
    public void initEGL(Context context) {
        if (mRenderProram == null) {
            mRenderProram= new BigEyesProgram(mInputType,mOutputType);
            mRenderProram.initProgram(context);
        }
    }

    public boolean setType(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        boolean mIsChange = false;
        mInputType = inputType;
        mOutputType = outputType;
        if (mRenderProram != null && (mRenderProram.getInputType() != mInputType || mRenderProram.getOutputType() != mOutputType)) {
            mRenderProram.release();
            mRenderProram = new BigEyesProgram(mInputType, mOutputType);
            mRenderProram.initProgram(mContext);
            mIsChange = true;
        }
        return mIsChange;
    }

    public void setFaceResult(FaceDetectionReport[] faceResult, boolean isCapture, int faceWidth, int faceHeight) {
        if (mRenderProram != null) {
            ((BigEyesProgram) mRenderProram).setFaceResult(faceResult, isCapture, faceWidth, faceHeight);
        }
    }
}

package com.allwinner.camera.drawer;

import android.content.Context;

import com.allwinner.camera.program.SmoothProgram;
import com.allwinner.camera.utils.EGLUtils;

public class SmoothDrawer extends BaseDrawer {

    public SmoothDrawer(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        super(inputType, outputType);
    }

    @Override
    public void initEGL(Context context) {
        if (mRenderProram == null) {
            mRenderProram= new SmoothProgram(mInputType,mOutputType);
            mRenderProram.initProgram(context);
        }
    }

    public boolean setType(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        boolean mIsChange = false;
        mInputType = inputType;
        mOutputType = outputType;
        if (mRenderProram != null && (mRenderProram.getInputType() != mInputType || mRenderProram.getOutputType() != mOutputType)) {
            mRenderProram.release();
            mRenderProram = new SmoothProgram(mInputType, mOutputType);
            mRenderProram.initProgram(mContext);
            mIsChange = true;
        }
        return mIsChange;
    }

}

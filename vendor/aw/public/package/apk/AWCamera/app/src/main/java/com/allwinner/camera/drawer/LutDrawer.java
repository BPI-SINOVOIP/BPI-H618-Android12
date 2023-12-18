package com.allwinner.camera.drawer;

import android.content.Context;

import com.allwinner.camera.program.LutProgram;
import com.allwinner.camera.utils.EGLUtils;

public class LutDrawer extends BaseDrawer {
    public LutDrawer(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        super(inputType, outputType);
    }

    @Override
    public void initEGL(Context context) {
        if (mRenderProram == null) {
            mRenderProram= new LutProgram(mInputType,mOutputType);
            mRenderProram.initProgram(context);
        }
    }

    public boolean setType(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        boolean mIsChange = false;
        mInputType = inputType;
        mOutputType = outputType;
        if (mRenderProram != null && (mRenderProram.getInputType() != mInputType || mRenderProram.getOutputType() != mOutputType)) {
            mRenderProram.release();
            mRenderProram = new LutProgram(mInputType, mOutputType);
            mRenderProram.initProgram(mContext);
            mIsChange = true;
        }
        return mIsChange;
    }

    public void setLutTextureId(int textureId) {
        if (mRenderProram != null ) {
            ((LutProgram)mRenderProram).setLutTextureId(textureId);
        }
    }
}

package com.allwinner.camera.program;

import android.content.Context;
import android.graphics.PointF;
import android.util.Log;

import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.EGLUtils;

import static android.opengl.GLES20.glGetUniformLocation;
import static android.opengl.GLES20.glUniform1f;
import static android.opengl.GLES20.glUniform2f;

public class BigEyesProgram extends BaseProgram {

    public static final String SCALERATIO = "scaleRatio";
    public static final String RADIUS = "radius";
    public static final String LEFTEYECENTERPOSITION = "leftEyeCenterPosition";
    public static final String LEFTFACECENTERPOSITION0 = "leftContourPoints_vec[0]";
    public static final String LEFTFACECENTERPOSITION1 = "leftContourPoints_vec[1]";
    public static final String LEFTFACECENTERPOSITION2 = "leftContourPoints_vec[2]";
    public static final String RIGHTFACECENTERPOSITION0 = "rightContourPoints_vec[0]";
    public static final String RIGHTFACECENTERPOSITION1 = "rightContourPoints_vec[1]";
    public static final String RIGHTFACECENTERPOSITION2 = "rightContourPoints_vec[2]";
    public static final String DELTAARRAY0 = "deltaArray[0]";
    public static final String DELTAARRAY1 = "deltaArray[1]";
    public static final String DELTAARRAY2 = "deltaArray[2]";

    public static final String RIGHTEYECENTERPOSITION = "rightEyeCenterPosition";
    public static final String ASPECTRATIO = "aspectRatio";
    public static final String FACEWARPRADIUS = "faceWarpRadius";


    private int mScaleLocation = -1;
    private int mLeftEyeLocation = -1;
    private int mRightEyeLocation = -1;
    private int mRadiusLocation = -1;
    private int mAspectLocation = -1;
    private int mWarpsLocation = -1;
    private int mLeftFace0Location = -1;
    private int mLeftFace1Location = -1;
    private int mLeftFace2Location = -1;
    private int mRightFace0Location = -1;
    private int mRightFace1Location = -1;
    private int mRightFace2Location = -1;
    private int mDelta0Location = -1;
    private int mDelta1Location = -1;
    private int mDelta2Location = -1;

    private FaceDetectionReport[] mFaceResult;
    private boolean mIsCapture = false;
    private int mFaceWidth;
    private int mFaceHeight;
    private float mFaceFactor;
    private float mFaceWarps;


    public BigEyesProgram(EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType) {
        super(inputType, outputType);
    }

    @Override
    protected String getFragmentShader(Context context) {
        if (mInputType == EGLUtils.TEXTURETYPE.YUV && mOutputType == EGLUtils.TEXTURETYPE.TEXTURE2D) {
            return  CameraUtils.loadFromAssets(context,"facebeautycapture.glsl");
        } else {
            return CameraUtils.loadFromAssets(context,"facebeauty.glsl");
        }
    }


    @Override
    protected void getProgramParms() {
        super.getProgramParms();
        mScaleLocation = glGetUniformLocation(getProgram(), BigEyesProgram.SCALERATIO);
        mRadiusLocation = glGetUniformLocation(getProgram(), BigEyesProgram.RADIUS);
        mLeftEyeLocation = glGetUniformLocation(getProgram(), BigEyesProgram.LEFTEYECENTERPOSITION);
        mRightEyeLocation = glGetUniformLocation(getProgram(), BigEyesProgram.RIGHTEYECENTERPOSITION);
        mAspectLocation = glGetUniformLocation(getProgram(), BigEyesProgram.ASPECTRATIO);
        mWarpsLocation = glGetUniformLocation(getProgram(), BigEyesProgram.FACEWARPRADIUS);
        mLeftFace0Location = glGetUniformLocation(getProgram(), BigEyesProgram.LEFTFACECENTERPOSITION0);
        mLeftFace1Location = glGetUniformLocation(getProgram(), BigEyesProgram.LEFTFACECENTERPOSITION1);
        mLeftFace2Location = glGetUniformLocation(getProgram(), BigEyesProgram.LEFTFACECENTERPOSITION2);
        mRightFace0Location = glGetUniformLocation(getProgram(), BigEyesProgram.RIGHTFACECENTERPOSITION0);
        mRightFace1Location = glGetUniformLocation(getProgram(), BigEyesProgram.RIGHTFACECENTERPOSITION1);
        mRightFace2Location = glGetUniformLocation(getProgram(), BigEyesProgram.RIGHTFACECENTERPOSITION2);
        mDelta0Location = glGetUniformLocation(getProgram(), BigEyesProgram.DELTAARRAY0);
        mDelta1Location = glGetUniformLocation(getProgram(), BigEyesProgram.DELTAARRAY1);
        mDelta2Location = glGetUniformLocation(getProgram(), BigEyesProgram.DELTAARRAY2);
    }
    

    @Override
    protected void configParameter(int program) {
        super.configParameter(program);
        if (mFaceResult != null) {
            mFaceFactor = CameraUtils.range(CameraData.getInstance().getBigeyeProgress(),Contants.BIGEYE_START,Contants.BIGEYE_END);
            mFaceWarps = CameraUtils.range(CameraData.getInstance().getFaceliftProgress(),Contants.FACELIFT_START,Contants.FACELIFT_END);
            glUniform1f(mScaleLocation,0.7f *mFaceFactor);
            glUniform1f(mRadiusLocation, 0.15f*mFaceFactor);
            glUniform2f(mLeftEyeLocation, getReadPoint(mFaceResult,Contants.EYELEFTPOINT).x,getReadPoint(mFaceResult,Contants.EYELEFTPOINT).y);
            glUniform2f(mRightEyeLocation, getReadPoint(mFaceResult,Contants.EYERIGHTPOINT).x,getReadPoint(mFaceResult,Contants.EYERIGHTPOINT).y);
            glUniform1f(mAspectLocation, Math.max(CameraData.getInstance().getPreviewHeight(), CameraData.getInstance().getPreviewWidth()) / Math.min(CameraData.getInstance().getPreviewHeight(), CameraData.getInstance().getPreviewWidth()));
            glUniform1f(mWarpsLocation,mFaceWarps);
            glUniform2f(mLeftFace0Location, getReadPoint(mFaceResult,Contants.FACELEFT1POINT).x, getReadPoint(mFaceResult,Contants.FACELEFT1POINT).y);
            glUniform2f(mLeftFace1Location, getReadPoint(mFaceResult,Contants.FACELEFT2POINT).x, getReadPoint(mFaceResult,Contants.FACELEFT2POINT).y);
            glUniform2f(mLeftFace2Location, getReadPoint(mFaceResult,Contants.FACELEFT3POINT).x, getReadPoint(mFaceResult,Contants.FACELEFT3POINT).y);
            glUniform2f(mRightFace0Location,getReadPoint(mFaceResult,Contants.FACERIGHT1POINT).x, getReadPoint(mFaceResult,Contants.FACERIGHT1POINT).y);
            glUniform2f(mRightFace1Location, getReadPoint(mFaceResult,Contants.FACERIGHT2POINT).x, getReadPoint(mFaceResult,Contants.FACERIGHT2POINT).y);
            glUniform2f(mRightFace2Location, getReadPoint(mFaceResult,Contants.FACERIGHT3POINT).x, getReadPoint(mFaceResult,Contants.FACERIGHT3POINT).y);
            glUniform1f(mDelta0Location, mFaceWarps*3);
            glUniform1f(mDelta1Location,mFaceWarps*3);
            glUniform1f(mDelta2Location, mFaceWarps*3);
        } else {
            glUniform1f(mScaleLocation, 0.0f);
            glUniform1f(mRadiusLocation, 0.0f);
            glUniform2f(mLeftEyeLocation, 0.0f, 0.0f);
            glUniform2f(mRightEyeLocation, 0.0f, 0.0f);
            glUniform1f(mAspectLocation, 0.0f);
            glUniform1f(mWarpsLocation, 0.0f);
            glUniform2f(mLeftFace0Location, 0.0f, 0.0f);
            glUniform2f(mLeftFace1Location, 0.0f, 0.0f);
            glUniform2f(mLeftFace2Location, 0.0f, 0.0f);
            glUniform2f(mRightFace0Location, 0.0f, 0.0f);
            glUniform2f(mRightFace1Location, 0.0f, 0.0f);
            glUniform2f(mRightFace2Location, 0.0f, 0.0f);
            glUniform1f(mDelta0Location, 0.0f);
            glUniform1f(mDelta1Location, 0.0f);
            glUniform1f(mDelta2Location, 0.0f);
        }
    }

    public void setFaceResult(FaceDetectionReport[] faceResult, boolean isCapture, int faceWidth, int faceHeight) {
        mFaceResult = faceResult;
        mIsCapture = isCapture;
        mFaceWidth = faceWidth;
        mFaceHeight = faceHeight;
    }

    private PointF getReadPoint(FaceDetectionReport[] faceResult, int index) {
        return CameraUtils.getRealPoint(faceResult, index, mIsCapture, mFaceWidth, mFaceHeight);
    }
}
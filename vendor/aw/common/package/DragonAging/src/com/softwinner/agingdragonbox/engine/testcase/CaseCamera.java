package com.softwinner.agingdragonbox.engine.testcase;

import java.util.Arrays;
import java.util.concurrent.Semaphore;

import android.content.Context;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraDevice.StateCallback;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.TextView;
import android.widget.Toast;

import com.softwinner.agingdragonbox.R;
import com.softwinner.agingdragonbox.engine.BaseCase;
import com.softwinner.agingdragonbox.xml.Node;

public class CaseCamera extends BaseCase implements SurfaceHolder.Callback{

    private static final String TAG = CaseCamera.class.getSimpleName();
	SurfaceView mMaxSurfaceV;
	TextView mMaxInfoTV;
	SurfaceHolder mSurfaceHolder;
	private CameraManager mCameraManager;
	String[] mCameraIds;
	CameraDevice mCameraDevice;
	private CameraCaptureSession mPreviewSession;
	private CaptureRequest.Builder mPreviewBuilder;
	/**
	 * A {@link Handler} for running tasks in the background.
	 */
	private Handler mBackgroundHandler;
	/**                                                                                
     * An additional thread for running tasks that shouldn't block the UI. 
     */    
    private HandlerThread mBackgroundThread;
	/**
	 * A {@link Semaphore} to prevent the app from exiting before closing the camera.
	 */
	private Semaphore mCameraOpenCloseLock = new Semaphore(1);

	protected void onInitialize(Node attr){
	    mCameraManager = (CameraManager)mContext.getSystemService(Context.CAMERA_SERVICE);
	    startBackgroundThread();
        setView(R.layout.case_camera);
        try {
            mCameraIds = mCameraManager.getCameraIdList();
        } catch (CameraAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
		mMaxSurfaceV = (SurfaceView) getView().findViewById(R.id.case_camera_surface);
		mMaxInfoTV = (TextView) getView().findViewById(R.id.case_camera_info);
	}
	public int requestPermission() {
	    //判断当前应用是否已经获取Camera权限。
	    //int granted = mContext.checkCallingPermission(Manifest.permission.CAMERA);
	    return 0;
	}
    

	private void startBackgroundThread() {
        // TODO Auto-generated method stub
        mBackgroundThread = new HandlerThread("CameraBackground");
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
    }
	/**
     * Stops the background thread and its {@link Handler}.
     */
    private void stopBackgroundThread() {
        mBackgroundThread.quitSafely();
        try {
            mBackgroundThread.join();
            mBackgroundThread = null;
            mBackgroundHandler = null;
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }


    @Override
	protected boolean onCaseStarted() {
		mSurfaceHolder = mMaxSurfaceV.getHolder();
		mSurfaceHolder.addCallback(this);
        return false;
	}
    
    private void closeCamera() {
        try {
            mCameraOpenCloseLock.acquire();
            closePreviewSession();
            if (null != mCameraDevice) {
                mCameraDevice.close();
                mCameraDevice = null;
            }
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera closing.");
        } finally {
            mCameraOpenCloseLock.release();
        }
    }
    
    private void closePreviewSession() {
        if (mPreviewSession != null) {
            mPreviewSession.close();
            mPreviewSession = null;
        }
    }
    
	@Override
	protected void onCaseFinished() {
	    closeCamera();
        stopBackgroundThread();
	}
     @Override
    protected void onRelease() {
         closeCamera();
         stopBackgroundThread();
    }

	@Override
	public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {
		Log.d("CaseCamera", "surfaceChanged");
		
	}

	@Override
	public void surfaceCreated(SurfaceHolder arg0) {
        Log.d("CaseCamera", "surfaceCreated + cameraNUM=" + mCameraIds.length);
        if(mCameraIds.length>0){
            openCamera();
        }else{
            mMaxInfoTV.setText(mMaxInfoTV.getText()+"-未检测到Camera！");
        }
	}

	private void openCamera() {
        // TODO Auto-generated method stub
        try {
            mCameraManager.openCamera(mCameraIds[0], new StateCallback() {
                
                @Override
                public void onOpened(CameraDevice camera) {
                    // TODO Auto-generated method stub
                    mCameraDevice = camera;
                    startPreview();
                    mCameraOpenCloseLock.release();
                    
                }
                
                @Override
                public void onError(CameraDevice camera, int error) {
                    // TODO Auto-generated method stub
                    mCameraOpenCloseLock.release();
                    camera.close();
                    mCameraDevice = null;
                    Toast.makeText(mContext, "openCamera onError,error = "+ error, Toast.LENGTH_LONG).show();
                }
                
                @Override
                public void onDisconnected(CameraDevice camera) {
                    // TODO Auto-generated method stub
                    mCameraOpenCloseLock.release();
                    camera.close();
                    mCameraDevice = null;
                }
            }, null);
        } catch (CameraAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }
	protected void startPreview() {
        // TODO Auto-generated method stub
        try {
            mPreviewBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            mPreviewBuilder.addTarget(mSurfaceHolder.getSurface());
            mCameraDevice.createCaptureSession(Arrays.asList(mSurfaceHolder.getSurface()), new CameraCaptureSession.StateCallback() {
                
                @Override
                public void onConfigured(CameraCaptureSession session) {
                    // TODO Auto-generated method stub
                    mPreviewSession = session;
                    updatePreview();
                }
                
                @Override
                public void onConfigureFailed(CameraCaptureSession session) {
                    // TODO Auto-generated method stub
                    Toast.makeText(mContext, "Failed on createCaptureSession", Toast.LENGTH_LONG).show();
                }
            }, mBackgroundHandler);
        } catch (CameraAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    protected void updatePreview() {
        // TODO Auto-generated method stub
        if(null == mCameraDevice) {
            return;
        }
        mPreviewBuilder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
        try {
            mPreviewSession.setRepeatingRequest(mPreviewBuilder.build(),null, mBackgroundHandler);
        } catch (CameraAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        
    }


    @Override
	public void surfaceDestroyed(SurfaceHolder arg0) {
		Log.d("CaseCamera", "surfaceDestroyed");
		
	}
}

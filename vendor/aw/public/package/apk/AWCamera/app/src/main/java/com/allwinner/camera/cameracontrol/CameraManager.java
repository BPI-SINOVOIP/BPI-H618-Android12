package com.allwinner.camera.cameracontrol;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.MeteringRectangle;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.ImageReader;
import android.media.MediaRecorder;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.util.Log;
import android.util.Size;
import android.view.Surface;

import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.CoordinateTransformer;
import com.allwinner.camera.utils.SystemPropertyUtils;

import org.greenrobot.eventbus.EventBus;

import java.util.ArrayList;
import java.util.List;

import static android.hardware.camera2.CameraMetadata.CONTROL_AE_MODE_ON;
import static android.hardware.camera2.CameraMetadata.CONTROL_AE_MODE_ON_ALWAYS_FLASH;
import static android.hardware.camera2.CameraMetadata.FLASH_MODE_OFF;
import static android.hardware.camera2.CameraMetadata.FLASH_MODE_SINGLE;
import static android.hardware.camera2.CameraMetadata.FLASH_MODE_TORCH;
import static com.allwinner.camera.data.Contants.CameraStatus.STATE_PICTURE_TAKEN;
import static com.allwinner.camera.data.Contants.CameraStatus.STATE_PREVIEW;
import static com.allwinner.camera.data.Contants.CameraStatus.STATE_WAITING_LOCK;
import static com.allwinner.camera.data.Contants.CameraStatus.STATE_WAITING_NON_PRECAPTURE;
import static com.allwinner.camera.data.Contants.CameraStatus.STATE_WAITING_PRECAPTURE;

public class CameraManager {

    private SharedPreferences mSp;
    private SharedPreferences.Editor mEditor;
    private CameraStatusListener mCameraStatusListener;
    private static CameraManager instance;
    private CameraExecutor mExecutor;
    private CameraLock mCameraLock;
    private final static String TAG = "CameraManager";
    private android.hardware.camera2.CameraManager mCameraManager;
    private ImageReader mCaptureReader;
    private Context mContext;
    private Handler mCameraHandler;
    private HandlerThread mCameraThread;
    private CameraDevice mCameraDevice = null;
    private CaptureRequest.Builder mPreviewBuilder = null;
    private CameraCaptureSession mCaptureSession = null;
    private CaptureRequest mCameraRequset = null;
    private int mCameraId = 0;
    private CameraCharacteristics mCameraCharacteristics = null;
    private int mSensorOrientation;
    private int mState = STATE_PREVIEW;
    private Surface mPreviewSurface = null;
    private long mCaptureTime;
    private Surface mVideoMediaRecorderSurface;
    private Rect mOrirect;
    private Rect mPreviewRect;
    private Rect mNewzoom;
    private CoordinateTransformer mTransformer;
    private MeteringRectangle[] mFocusArea;
    private MeteringRectangle[] mMeteringArea;
    private Contants.ModeType mModeType;
    private Integer mLatestAfState = 0;
    private List<Surface> mSurfaces = new ArrayList<>();

    private CameraManager() {

        mExecutor = new CameraExecutor();
        mCameraLock = new CameraLock();
    }

    public void startThread() {
        Log.i(TAG, "startThread");
        if (mCameraThread == null) {
            mCameraThread = new HandlerThread("CameraManager");
            mCameraThread.start();
            mCameraHandler = new Handler(mCameraThread.getLooper());
            CameraData.getInstance().setCameraHandler(mCameraHandler);
        }
        initCameraId();
    }

    public void setContext(Context context) {
        mContext = context;
        mSp = PreferenceManager.getDefaultSharedPreferences(mContext);
        mEditor = mSp.edit();
    }

    public void setCameraId(int cameraId) {
        mCameraId = cameraId;
    }

    public void onDestroy() {
        Log.i(TAG, "stopThread");
        if (mCameraThread == null) {
            return;
        }
        mCameraThread.quitSafely();
        mCameraThread = null;
    }

    public void openCamera(final int CameraId) {
        Log.i(TAG, "openCamera");
        mCameraId = CameraId;
        CameraData.getInstance().setCameraId(mCameraId);
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                mCameraLock.lock("openCamera");

                if (mCameraManager == null) {
                    mCameraManager = (android.hardware.camera2.CameraManager) mContext.getSystemService(Context.CAMERA_SERVICE);
                    CameraData.getInstance().setCameraManager(mCameraManager);
                }
                try {
                    if (ActivityCompat.checkSelfPermission(mContext, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                        // TODO: Consider calling
                        //    ActivityCompat#requestPermissions
                        // here to request the missing permissions, and then overriding
                        //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                        //                                          int[] grantResults)
                        // to handle the case where the user grants the permission. See the documentation
                        // for ActivityCompat#requestPermissions for more details.
                        return;
                    }
                    mCameraManager.openCamera(String.valueOf(mCameraId), mOpenCameraStateCallback, mCameraHandler);
                } catch (CameraAccessException e) {
                    e.printStackTrace();
                    mCameraLock.unlock("openCamera");
                }

            }
        }, "openCamera");

    }

    public void closeCamera() {
        Log.i(TAG, "closeCamera");
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                mCameraLock.lock("closeCamera");
                if (mCaptureSession != null) {
                    mCaptureSession.close();
                    mCaptureSession = null;
                }
                if (mCameraDevice != null) {
                    mCameraDevice.close();
                    mCameraDevice = null;
                }
                if (mCaptureReader != null) {
                    mCaptureReader.close();
                    mCaptureReader = null;
                }
            }
        }, "closeCamera");
        mExecutor.waitDone("closeCamera");
        if (mCameraStatusListener != null) {
            mCameraLock.unlock("closeCamera");
            mCameraStatusListener.onCameraClose();
        }
    }

    public void startPreview(final Contants.ModeType type) {
        Log.i(TAG, "startPreview : " + type.toString());
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                if (mCameraDevice != null) {
                    mCameraLock.lock("starPreview");
                    List<Surface> surfaces = new ArrayList<>();
                    mSurfaces.clear();
                    try {
                        mModeType = type;
                        if (type.getId() == Contants.ModeType.VideMode.getId()) {
                            Log.i(TAG, "startPreview : VideMode");
                            mPreviewBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
                            mPreviewBuilder.set(CaptureRequest.SCALER_CROP_REGION,mNewzoom);
                            surfaces = mCameraStatusListener.getSurface();
                            for (Surface surface : surfaces) {
                                mPreviewBuilder.addTarget(surface);
                                //      Log.e(TAG," addTarget surface:"+surface.)
                                mSurfaces.add(surface);
                            }
                            mPreviewSurface = surfaces.get(0);
                            mVideoMediaRecorderSurface = mCameraStatusListener.getVideoSurface();
                            mSurfaces.add(mVideoMediaRecorderSurface);
                            mPreviewBuilder.addTarget(mVideoMediaRecorderSurface);
                            surfaces.add(mVideoMediaRecorderSurface);
                            //surfaces = Arrays.asList(mPreviewSurface, mVideoMediaRecorderSurface);
                            Log.i(TAG, "startPreview : outputs" + surfaces);
                            if (CameraData.getInstance().getIsFlashAvailable()) {
                                applyFlash(mPreviewBuilder, CameraData.getInstance().getVideoFlashMode(), false);
                            }
                        } else {
                            Log.i(TAG, "startPreview : not VideMode　");
                            mPreviewBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                            mPreviewBuilder.set(CaptureRequest.SCALER_CROP_REGION,mNewzoom);
                            surfaces = mCameraStatusListener.getSurface();
                            for (Surface surface : surfaces) {
                                mPreviewBuilder.addTarget(surface);
                                mSurfaces.add(surface);
                            }
                            mPreviewSurface = surfaces.get(0);
                            surfaces.add(mCaptureReader.getSurface());
                            Log.e(TAG, "mCaptureReader.getSurface():" + mCaptureReader.getSurface());
                        }
                        mCameraDevice.createCaptureSession(surfaces, new CameraCaptureSession.StateCallback() {
                            @Override
                            public void onConfigured(@NonNull CameraCaptureSession session) {
                                Log.i(TAG, "createCaptureSession onConfigured");
                                mCaptureSession = session;
                                try {
                                    mPreviewBuilder.set(CaptureRequest.CONTROL_AF_MODE,
                                            CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
                                    mCameraRequset = mPreviewBuilder.build();
                                    mCaptureSession.setRepeatingRequest(mCameraRequset, mCaptureCallback, mCameraHandler);
                                } catch (CameraAccessException e) {
                                    e.printStackTrace();
                                }
                                mCameraStatusListener.onStartPreviewDone();
                                Log.i(TAG, "createCaptureSession unlock");
                                mCameraLock.unlock("starPreview");
                            }

                            @Override
                            public void onConfigureFailed(@NonNull CameraCaptureSession session) {
                                mCameraStatusListener.onStartPreviewFail();
                                mCameraLock.unlock("starPreview");
                            }

                            @Override
                            public void onClosed(@NonNull CameraCaptureSession session) {
                                super.onClosed(session);
                            }
                        }, mCameraHandler);
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                        mCameraLock.unlock("starPreview");
                    }

                }
            }
        }, "starPreview");
        mExecutor.waitDone("starPreview");
    }

    public void stopPreview() {
        Log.i(TAG, "stopPreview");
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                mCameraLock.lock("stopPreview");
                if (mPreviewBuilder != null) {
                    mPreviewBuilder.removeTarget(mPreviewSurface);
                    mPreviewBuilder = null;
                }
                if (mCameraDevice != null && mCaptureSession != null) {
                    try {
                        mCaptureSession.stopRepeating();
                        if (Build.VERSION.SDK_INT < 28) {
                            mCaptureSession.close();
                        }
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    } finally {
                        mCameraLock.unlock("stopPreview");
                    }
                    mCaptureSession = null;
                } else {
                    mCameraLock.unlock("stopPreview");
                }
            }
        }, "stopPreview");
        // mExecutor.waitDone("stopPreview");
        if (mCameraStatusListener != null) {
            mCameraStatusListener.onPreviewStop();
        }
    }

    public void takePicture() {
        Log.i(TAG, "takePicture");
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                mCaptureTime = System.currentTimeMillis();
                CameraData.getInstance().setCaptureTime(mCaptureTime);
                /*
                if (mFlashMode == Contants.FlashMode.FLASH_OFF) {
                    captureStillPicture();
                } else {
                    lockFocus();
                }
                */
                captureStillPicture();
            }
        }, "takePicture");
        // mExecutor.waitDone("takePicture");
    }

    public void setPreviewFlashRequest(int value) {

        applyFlash(mPreviewBuilder, value, false);
        try {
            mCaptureSession.setRepeatingRequest(mPreviewBuilder.build(), mCaptureCallback, mCameraHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void applyFlash(CaptureRequest.Builder builder, int value, boolean isBurst) {

        Integer aeMode = null;
        Integer flashMode = null;

        if (Contants.FlashMode.FLASH_AUTO == value) {
            if (isBurst) {
                // When long shot is active, turn off the flash in auto mode
                aeMode = CONTROL_AE_MODE_ON;
                flashMode = FLASH_MODE_OFF;
            } else {
                aeMode = CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH;
                flashMode = FLASH_MODE_SINGLE;
            }
        } else if (Contants.FlashMode.FLASH_OFF == value) {
            aeMode = CONTROL_AE_MODE_ON;
            flashMode = FLASH_MODE_OFF;
        } else if (Contants.FlashMode.FLASH_ON == value) {
            if (isBurst) {
                // When long shot is active, turn off the flash in auto mode
                aeMode = CONTROL_AE_MODE_ON;
                flashMode = FLASH_MODE_OFF;
            } else {
                aeMode = CONTROL_AE_MODE_ON_ALWAYS_FLASH;
                flashMode = FLASH_MODE_SINGLE;
            }
        } else if (Contants.FlashMode.FLASH_TORCH == value) {
            aeMode = CONTROL_AE_MODE_ON;
            flashMode = FLASH_MODE_TORCH;
        } else {
            Log.e(TAG, "Unable to convert to API 2 flash mode: " + value);
        }

        Log.i(TAG, "apply Flash :" + value);
        builder.set(CaptureRequest.FLASH_MODE, flashMode);
        builder.set(CaptureRequest.CONTROL_AE_MODE, aeMode);

    }

    public void setZoom(final float delta) {
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                float zoomvalue = delta;
                if (zoomvalue <= 100f) {
                    zoomvalue = 100f;
                    mNewzoom = null;
                }
                if (zoomvalue >= CameraData.getInstance().getMaxZoom()) {
                    zoomvalue = CameraData.getInstance().getMaxZoom();
                }
                if (mPreviewBuilder != null && mCameraDevice != null && mCaptureSession != null) {
                    float finalZoomValue = zoomvalue / 100f;
                    int newWidth = (int) (mOrirect.width() / finalZoomValue);
                    int newHeight = (int) (mOrirect.height() / finalZoomValue);
                    int difW = (mOrirect.width() - newWidth) / 2;
                    int difH = (mOrirect.height() - newHeight) / 2;
                    mNewzoom = new Rect(mOrirect.left + difW, mOrirect.top + difH,
                            mOrirect.left + difW + newWidth, mOrirect.top + difH + newHeight);
                    mPreviewBuilder.set(CaptureRequest.SCALER_CROP_REGION, mNewzoom);
                    try {
                        mCaptureSession.setRepeatingRequest(mPreviewBuilder.build(), mCaptureCallback, mCameraHandler);
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    }
                    Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_UPDATE_ZOOMTEXT, finalZoomValue);
                    EventBus.getDefault().post(object);
                }
            }
        }, "setZoom");
    }


    public void setCameraListener(CameraStatusListener listener) {
        mCameraStatusListener = listener;
    }

    public void setPictureSize(final int width, final int height) {
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                if (mCaptureReader != null) {
                    mCaptureReader.close();
                }
                if(SystemPropertyUtils.getBoolean(mContext, "sys.camera.jpeg.debug", false)) {
                    mCaptureReader = ImageReader.newInstance(width, height, ImageFormat.JPEG, 2);
                }else{
                    mCaptureReader = ImageReader.newInstance(width, height, ImageFormat.YUV_420_888, 2);
                }
            }
        }, "setPictureSize");
        //mExecutor.waitDone("setPictureSize");

    }

    public List<Point> getSupportedPictureSizes() {
        final List<Point> result = new ArrayList<>();
        if (mCameraDevice == null || mCameraCharacteristics == null) {
            return result;
        } else if (mCameraDevice != null) {
            List<Point> list = CameraData.getInstance().getSurpportedPictureSizes().get(mCameraId);
            if (list != null && list.size() != 0) {
                return list;
            }
        }
        if (mCameraDevice != null && mCameraCharacteristics != null) {
            StreamConfigurationMap map = mCameraCharacteristics.get(
                    CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) {
                Log.e(TAG, "StreamConfigurationMap is NULL!");
                return result;
            }
            Size[] sizes ;
            if(SystemPropertyUtils.getBoolean(mContext, "sys.camera.jpeg.debug", false)){
                sizes = map.getOutputSizes(ImageFormat.JPEG);
            }else{
                sizes = map.getOutputSizes(ImageFormat.YUV_420_888);
            }
            for (Size size : sizes) {
                result.add(new Point(size.getWidth(), size.getHeight()));
            }
            CameraData.getInstance().addPictureSizetoList(result, mCameraId);
        }
        return result;
    }

    public List<Point> getSupportedVideoSizes() {
        final List<Point> result = new ArrayList<>();
        if (mCameraDevice == null || mCameraCharacteristics == null) {
            return result;
        } else if (mCameraDevice != null) {
            List<Point> list = CameraData.getInstance().getSurpportedVideoSizes().get(mCameraId);
            if (list != null && list.size() != 0) {
                return list;
            }
        }
        if (mCameraDevice != null && mCameraCharacteristics != null) {
            StreamConfigurationMap map = mCameraCharacteristics.get(
                    CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) {
                Log.e(TAG, "StreamConfigurationMap is NULL!");
                return result;
            }

            Size[] sizes = map.getOutputSizes(MediaRecorder.class);
            for (Size size : sizes) {
                result.add(new Point(size.getWidth(), size.getHeight()));
            }
            CameraData.getInstance().addVideoSizetoList(result, mCameraId);
        }
        return result;
    }

    public List<Point> getSupportedPreviewSizes() {
        final List<Point> result = new ArrayList<>();
        if (mCameraDevice == null || mCameraCharacteristics == null) {
            return result;
        } else if (mCameraDevice != null) {
            List<Point> list = CameraData.getInstance().getSurpportedPreviewSizes().get(mCameraId);
            if (list != null && list.size() != 0) {
                return list;
            }
        }
        if (mCameraDevice != null && mCameraCharacteristics != null) {
            StreamConfigurationMap map = mCameraCharacteristics.get(
                    CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) {
                Log.e(TAG, "StreamConfigurationMap is NULL!");
                return result;
            }

            Size[] sizes = map.getOutputSizes(SurfaceTexture.class);
            for (Size size : sizes) {
                result.add(new Point(size.getWidth(), size.getHeight()));
            }
            if (result.size() != 0) {
                CameraData.getInstance().addPreviewSizetoList(result, mCameraId);
            }
        }
        return result;
    }


    @SuppressLint("NewApi")
    public void updateSizeParamater(int ratio, Contants.ModeType type) {
        mCameraLock.lock("updateSizeParamater: " + ratio);
        int videoPreviewWidth = 0;
        int videoPreviewHeight = 0;
        if (type != Contants.ModeType.VideMode) {
            Double relRatio = 0.0;
            if (ratio == Contants.CameraRatio.RATIO_4_TO_3) {
                relRatio = (double) (4f / 3f);
            } else if (ratio == Contants.CameraRatio.RATIO_16_TO_9) {
                relRatio = (double) (16f / 9f);
            } else if (ratio == Contants.CameraRatio.RATIO_18_TO_9) {
                relRatio = (double) (18f / 9f);
            }
            List<Point> previewsizelist = getSupportedPreviewSizes();
            Log.i(TAG, "getSupportedPreviewSizes: " + previewsizelist);
            Point previewsize = CameraData.getInstance().getPreviewSizebyRatio(getSupportedPreviewSizes(), relRatio);
            if(previewsize == null && relRatio == (double) (4f / 3f)){
                relRatio = (double) (16f / 9f);
                previewsize = CameraData.getInstance().getPreviewSizebyRatio(getSupportedPreviewSizes(), relRatio);
                if(mCameraId == CameraData.getInstance().getFrontCameraId()){
                    mEditor.putString(Contants.KEY_FRONT_PICTURE_SCALE,"16:9");
                    mEditor.apply();
                }else {
                    mEditor.putString(Contants.KEY_BACK_PICTURE_SCALE,"16:9");
                    mEditor.apply();
                }

            }else if(previewsize == null && relRatio == (double) (16f / 9f)){
                relRatio = (double) (4f / 3f);
                previewsize = CameraData.getInstance().getPreviewSizebyRatio(getSupportedPreviewSizes(), relRatio);
                if(mCameraId == CameraData.getInstance().getFrontCameraId()){
                    mEditor.putString(Contants.KEY_FRONT_PICTURE_SCALE,"4:3");
                    mEditor.apply();
                }else {
                    mEditor.putString(Contants.KEY_BACK_PICTURE_SCALE,"4:3");
                    mEditor.apply();
                }
            }
            Log.i(TAG, "previewsize.x: " + previewsize.x + "previewsize.y:" + previewsize.y+"relRatio:"+relRatio);
            CameraData.getInstance().setPreviewSize(previewsize.x, previewsize.y);
            List<Point> picturesizelist = getSupportedPictureSizes();
            Log.i(TAG, "getSupportedPictureSizes: " + picturesizelist);
            Point picturesize = CameraData.getInstance().getPictureSizebyRatio(getSupportedPictureSizes(), relRatio);
            CameraData.getInstance().setPictureSize(picturesize.x, picturesize.y);
            // CameraData.getInstance().setPictureSize(640, 480);
            Size displaysize = CameraUtils.getPreviewUiSize(mContext, new Size(CameraData.getInstance().getPreviewWidth(), CameraData.getInstance().getPreviewHeight()));
            mPreviewRect = new Rect(0, 0, displaysize.getWidth(), displaysize.getHeight());
        } else {
            List<Point> videoSizes = getSupportedVideoSizes();
            Log.i(TAG, "getSupportedVideoSizes: " + videoSizes + "CameraData.getInstance().getVideoQuality():" + CameraData.getInstance().getVideoQuality());

            if (CameraData.getInstance().getVideoQuality() == Contants.VideoQuality.Q480P) {
                videoPreviewWidth = 640;
                videoPreviewHeight = 480;
            } else if (CameraData.getInstance().getVideoQuality() == Contants.VideoQuality.Q720P) {
                videoPreviewWidth = 1280;
                videoPreviewHeight = 720;
            } else if (CameraData.getInstance().getVideoQuality() == Contants.VideoQuality.Q1080P) {
                videoPreviewWidth = 1920;
                videoPreviewHeight = 1080;
            } else if (CameraData.getInstance().getVideoQuality() == Contants.VideoQuality.Q4K) {
                videoPreviewWidth = 3840;
                videoPreviewHeight = 2160;
            }
            CameraData.getInstance().setPictureSize(videoPreviewWidth, videoPreviewHeight);
            CameraData.getInstance().setPreviewSize(videoPreviewWidth, videoPreviewHeight);

            Size displaysize = CameraUtils.getPreviewUiSize(mContext, new Size(videoPreviewWidth, videoPreviewHeight));
            mPreviewRect = new Rect(0, 0, displaysize.getWidth(), displaysize.getHeight());
        }

        mTransformer = new CoordinateTransformer(mCameraCharacteristics, rectToRectF(mPreviewRect));
        Log.i(TAG, "getSupportedVideoSizes: " + CameraData.getInstance().getPictureSize().x + ":" + CameraData.getInstance().getPictureSize().y);
        mCameraLock.unlock("updateSizeParamater");
        setPictureSize(CameraData.getInstance().getPictureSize().x, CameraData.getInstance().getPictureSize().y);

    }


    @SuppressLint("NewApi")
    public void autoFocus(final MeteringRectangle focusRect,
                          final MeteringRectangle meteringRect) {
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                try {
                    mCameraLock.lock("autofocus");
                    CaptureRequest.Builder builder;
                    if (mModeType.getId() == Contants.ModeType.VideMode.getId()) {
                        builder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
                    } else {
                        builder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                    }
                    for (Surface surface : mSurfaces) {
                        builder.addTarget(surface);
                    }
                    builder.set(CaptureRequest.SCALER_CROP_REGION,mNewzoom);
                    builder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_AUTO);
                    builder.set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_AUTO);
                    mPreviewBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_AUTO);
                    mPreviewBuilder.set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_AUTO);
                    if (mFocusArea == null) {
                        mFocusArea = new MeteringRectangle[]{focusRect};
                    } else {
                        mFocusArea[0] = focusRect;
                    }
                    if (mMeteringArea == null) {
                        mMeteringArea = new MeteringRectangle[]{meteringRect};
                    } else {
                        mMeteringArea[0] = meteringRect;
                    }
                    if (isMeteringSupport(true)) {
                        builder.set(CaptureRequest.CONTROL_AF_REGIONS, mFocusArea);
                    }
                    if (isMeteringSupport(false)) {
                        builder.set(CaptureRequest.CONTROL_AE_REGIONS, mMeteringArea);
                    }
                    builder.set(CaptureRequest.CONTROL_AF_TRIGGER, CaptureRequest.CONTROL_AF_TRIGGER_START);

                    mCaptureSession.capture(builder.build(), mCaptureCallback, mCameraHandler);
                } catch (CameraAccessException | IllegalStateException e) {
                    Log.e(TAG, "send capture request error:" + e.getMessage());
                } finally {
                    mCameraLock.unlock("autofocus");
                }
            }
        }, "autoFocus");

    }

    @SuppressLint("NewApi")
    public void cancelAutoFocus() {
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                if (mCaptureSession != null && mCameraDevice != null) {
                    try {
                        CaptureRequest.Builder builder;
                        List<Surface> surfaces;
                        if (mModeType.getId() == Contants.ModeType.VideMode.getId()) {
                            builder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
                            for (Surface surface : mSurfaces) {
                                builder.addTarget(surface);
                            }

                            builder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_VIDEO);
                            builder.set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_OFF);
                            mPreviewBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_VIDEO);
                            mPreviewBuilder.set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_OFF);
                        } else {
                            builder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                            for (Surface surface : mSurfaces) {
                                builder.addTarget(surface);
                            }
                            builder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
                            builder.set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_OFF);
                        }
                        builder.set(CaptureRequest.SCALER_CROP_REGION,mNewzoom);
                        builder.set(CaptureRequest.CONTROL_AF_TRIGGER, CaptureRequest.CONTROL_AF_TRIGGER_CANCEL);
                        mCaptureSession.capture(builder.build(), null, mCameraHandler);
                    } catch (CameraAccessException | IllegalStateException e) {
                        Log.e(TAG, "send capture request error:" + e.getMessage());
                    }
                }
            }
        }, "cancelAutoFocus");

    }

    private boolean isMeteringSupport(boolean focusArea) {
        int regionNum;
        if (focusArea) {
            regionNum = mCameraCharacteristics.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AF);
        } else {
            regionNum = mCameraCharacteristics.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AE);
        }
        return regionNum > 0;
    }

    @SuppressLint("NewApi")
    public boolean isAutoAFModeValid() {
        int[] allAFMode = mCameraCharacteristics.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES);
        for (int mode : allAFMode) {
            if (mode == CaptureRequest.CONTROL_AF_MODE_AUTO) {
                return true;
            }
        }
        return false;
    }

    @SuppressLint("NewApi")
    public MeteringRectangle calcTapAreaForCamera2(float currentX, float currentY, int weight,
                                                   boolean isFocusArea) {
        int areaSize = 0;
        if (isFocusArea) {
            areaSize = mPreviewRect.width() / 5;
        } else {
            areaSize = mPreviewRect.width() / 4;
        }
        int left = clamp((int) currentX - areaSize / 2,
                mPreviewRect.left, mPreviewRect.right - areaSize);
        int top = clamp((int) currentY - areaSize / 2,
                mPreviewRect.top, mPreviewRect.bottom - areaSize);
        RectF rectF = new RectF(left, top, left + areaSize, top + areaSize);
        Rect focusRect = new Rect();
        focusRect = toFocusRect(mTransformer.toCameraSpace(rectF));
        return new MeteringRectangle(focusRect, weight);
    }

    private Rect toFocusRect(RectF rectF) {
        Rect focusRect = new Rect();
        focusRect.left = Math.round(rectF.left);
        focusRect.top = Math.round(rectF.top);
        focusRect.right = Math.round(rectF.right);
        focusRect.bottom = Math.round(rectF.bottom);
        return focusRect;
    }

    private int clamp(int x, int min, int max) {
        if (x > max) {
            return max;
        }
        if (x < min) {
            return min;
        }
        return x;
    }

    private RectF rectToRectF(Rect rect) {
        return new RectF(rect);
    }

    public void initCommandFeature() {
        if (mCameraDevice == null || mCameraCharacteristics == null) {
            return;
        }
        if (mCameraDevice != null && mCameraCharacteristics != null) {
            StreamConfigurationMap map = mCameraCharacteristics.get(
                    CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) {
                Log.e(TAG, "StreamConfigurationMap is NULL!");
                return;
            }
            updateFlashAvailableStatus();
            //noinspection ConstantConditions
            mSensorOrientation = mCameraCharacteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
            CameraData.getInstance().setSensorOrientation(mSensorOrientation);
            mOrirect = mCameraCharacteristics.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);
            float maxZoom = mCameraCharacteristics.get(
                    CameraCharacteristics.SCALER_AVAILABLE_MAX_DIGITAL_ZOOM);
            Log.e(TAG, "maxZoom:" + maxZoom);
            if (maxZoom <= 1.0f) {
                CameraData.getInstance().setIsSupportZoom(false);
            } else {
                CameraData.getInstance().setIsSupportZoom(true);
                CameraData.getInstance().setMaxZoom(maxZoom * 100);
            }
        }
        // mIsInitCommonFeature = true;
    }

    public void initCameraId() {
        try {
            //获取可用摄像头列表
            if (mCameraManager == null) {
                mCameraManager = (android.hardware.camera2.CameraManager) mContext.getSystemService(Context.CAMERA_SERVICE);
                CameraData.getInstance().setCameraManager(mCameraManager);
            }
            for (String cameraId : mCameraManager.getCameraIdList()) {
                //获取相机的相关参数
                CameraCharacteristics characteristics = mCameraManager.getCameraCharacteristics(cameraId);
                Integer facing = characteristics.get(CameraCharacteristics.LENS_FACING);
                Camera.CameraInfo info = new Camera.CameraInfo();
                if (facing != null && facing == CameraCharacteristics.LENS_FACING_FRONT) {
                    CameraData.getInstance().setFrontCameraId(Integer.parseInt(cameraId));
                    CameraData.getInstance().setHasFrontCamera(true);
                    Camera.getCameraInfo(Integer.parseInt(cameraId), info);
                    Log.e(TAG, " front info.orientation:" + info.orientation);
                    CameraData.getInstance().setFrontCameraOrientation(info.orientation);
                }
                if (facing != null && facing == CameraCharacteristics.LENS_FACING_BACK) {
                    CameraData.getInstance().setBackCameraId(Integer.parseInt(cameraId));
                    CameraData.getInstance().setHasBackCamera(true);
                    Camera.getCameraInfo(Integer.parseInt(cameraId), info);
                    Log.e(TAG, " back info.orientation:" + info.orientation);
                    CameraData.getInstance().setBackCameraOrientation(info.orientation);
                }
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException e) {
            //不支持Camera2API
        }
    }

    private void lockFocus() {
        mCameraLock.lock("lockFocus");
        try {
            // This is how to tell the camera to lock focus.
            mPreviewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER,
                    CameraMetadata.CONTROL_AF_TRIGGER_START);
            // Tell #mCaptureCallback to wait for the lock.
            mState = STATE_WAITING_LOCK;
            mCaptureSession.capture(mPreviewBuilder.build(), mCaptureCallback,
                    mCameraHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        } finally {
            mCameraLock.unlock("lockFocus");
        }

    }

    private void unlockFocus() {
        try {
            mCameraLock.lock("unlockFocus");
            // Reset the auto-focus trigger
            mPreviewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER,
                    CameraMetadata.CONTROL_AF_TRIGGER_CANCEL);
            //setAutoFlash(mPreviewRequestBuilder);
            mCaptureSession.capture(mPreviewBuilder.build(), mCaptureCallback,
                    mCameraHandler);
            // After this, the camera will go back to the normal state of preview.
            mState = STATE_PREVIEW;
            mCaptureSession.setRepeatingRequest(mPreviewBuilder.build(), mCaptureCallback,
                    mCameraHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        } finally {
            mCameraLock.unlock("unlockFocus");
        }
    }

    private void runPrecaptureSequence() {
        try {
            // This is how to tell the camera to trigger.
            mPreviewBuilder.set(CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER,
                    CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_START);
            // Tell #mCaptureCallback to wait for the precapture sequence to be set.
            mState = STATE_WAITING_PRECAPTURE;
            mCaptureSession.capture(mPreviewBuilder.build(), mCaptureCallback,
                    mCameraHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void captureStillPicture() {
        try {
            if (mCameraDevice == null || mCaptureReader == null) return;
            // This is the CaptureRequest.Builder that we use to take a picture.
            final CaptureRequest.Builder captureBuilder =
                    mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            mCaptureReader.setOnImageAvailableListener(new CatpureImageAvailableListener(
                    mCameraStatusListener.getContext(), mCaptureTime, CameraUtils.getJpegRotation(mCameraId), mCameraStatusListener.getFilterCaptureEngine()), mCameraHandler);
            captureBuilder.addTarget(mCaptureReader.getSurface());
            if (mPreviewSurface != null) {
                captureBuilder.addTarget(mPreviewSurface);
            }
            // Use the same AE and AF modes as the preview.
            captureBuilder.set(CaptureRequest.CONTROL_AF_MODE,
                    CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
            if (CameraData.getInstance().getIsFlashAvailable()) {
                applyFlash(captureBuilder, CameraData.getInstance().getCaptureFlashMode(), false);
            }
            //setAutoFlash(captureBuilder);
            // Orientation
            captureBuilder.set(CaptureRequest.JPEG_ORIENTATION, CameraUtils.getJpegRotation(mCameraId));
            captureBuilder.set(CaptureRequest.SCALER_CROP_REGION, mNewzoom);
            CameraCaptureSession.CaptureCallback CaptureCallback
                    = new CameraCaptureSession.CaptureCallback() {

                @Override
                public void onCaptureCompleted(@NonNull CameraCaptureSession session,
                                               @NonNull CaptureRequest request,
                                               @NonNull TotalCaptureResult result) {
                    if (mCameraStatusListener != null) {
                        mCameraStatusListener.onCatpureCompleted();
                    }
                    // unlockFocus();
                }

                @Override
                public void onCaptureStarted(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, long timestamp, long frameNumber) {
                    if (mCameraStatusListener != null) {
                        mCameraStatusListener.onCaptureStarted();
                    }
                }
            };

            //mCaptureSession.stopRepeating();
            //mCaptureSession.abortCaptures();
            mCaptureSession.capture(captureBuilder.build(), CaptureCallback, mCameraHandler);

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }


    public void prepareVideoCamera(final Surface mediaRecorderSurface) {
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                if (mCaptureSession != null) {
                    try {
                        mPreviewBuilder.addTarget(mPreviewSurface);
                        if (mediaRecorderSurface != null) {
                            mPreviewBuilder.addTarget(mediaRecorderSurface);
                        }
                        mCaptureSession.setRepeatingRequest(mPreviewBuilder.build(), mCaptureCallback, mCameraHandler);
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    }
                }
            }
        }, "prepareVideoCamera");
    }

    public void releaseVideoCamera() {
        mExecutor.execute(new Runnable() {
            @Override
            public void run() {
                if (mCaptureSession != null) {
                    try {
                        mCaptureSession.stopRepeating();
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    }
                }
            }
        }, "releaseVideoCamera");
    }

    public void setCameraStatus(int status) {
        mState = status;
    }

    public synchronized static CameraManager getInstance() {
        if (instance == null) {
            instance = new CameraManager();
        }
        return instance;
    }


    CameraDevice.StateCallback mOpenCameraStateCallback = new CameraDevice.StateCallback() {

        @Override
        public void onClosed(@NonNull CameraDevice camera) {
            super.onClosed(camera);
            mCameraDevice = null;
            // mCameraLock.unlock("openCamera");
        }

        @Override
        public void onOpened(@NonNull CameraDevice camera) {
            mCameraDevice = camera;
            try {
                mCameraCharacteristics = mCameraManager
                        .getCameraCharacteristics(
                                String.valueOf(mCameraId));

            } catch (CameraAccessException e) {
                e.printStackTrace();
            }
            mCameraLock.unlock("openCamera");

            mCameraStatusListener.onCameraOpen();
            updateFlashAvailableStatus();
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice camera) {
            camera.close();
            mCameraDevice = null;
            mCameraStatusListener.onCameraOpenFail(0);
            mCameraLock.unlock("openCamera");
        }

        @Override
        public void onError(@NonNull CameraDevice camera, int error) {
            camera.close();
            mCameraDevice = null;
            mCameraStatusListener.onCameraOpenFail(error);
            mCameraLock.unlock("openCamera");
        }
    };

    public void updateFlashAvailableStatus() {
        Boolean available = mCameraCharacteristics.get(CameraCharacteristics.FLASH_INFO_AVAILABLE);
        Boolean isFlashAvailable = available == null ? false : available;
        CameraData.getInstance().setIsFlashAvailable(isFlashAvailable);
        mCameraStatusListener.onFlashStatus(isFlashAvailable);
    }

    private CameraCaptureSession.CaptureCallback mCaptureCallback
            = new CameraCaptureSession.CaptureCallback() {
        public void updateAfState(CaptureResult result) {
            Integer state = result.get(CaptureResult.CONTROL_AF_STATE);
            if (state != null && mLatestAfState != state) {
                mLatestAfState = state;
                onAFStateChanged(state);
            }
        }

        private void onAFStateChanged(int state) {
            switch (state) {
                case CaptureResult.CONTROL_AF_STATE_ACTIVE_SCAN:
                    Log.i(TAG, "autofocusing");
                    mCameraStatusListener.onAutoFocusing();
                    break;
                case CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED:
                    Log.i(TAG, "autofocus sucess");
                    mCameraStatusListener.onAutoFocusSuccess();
                    break;
                case CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED:
                    Log.i(TAG, "autofocus fail");
                    mCameraStatusListener.onAutoFocusFail();
                    break;
                case CaptureResult.CONTROL_AF_STATE_PASSIVE_FOCUSED:

                    break;
                case CaptureResult.CONTROL_AF_STATE_PASSIVE_SCAN:

                    break;
                case CaptureResult.CONTROL_AF_STATE_PASSIVE_UNFOCUSED:

                    break;
                case CaptureResult.CONTROL_AF_STATE_INACTIVE:

                    break;
            }
        }


        private void process(CaptureResult result) {
            updateAfState(result);
            switch (mState) {
                case STATE_PREVIEW: {
                    // We have nothing to do when the camera preview is working normally.
                    break;
                }
                case STATE_WAITING_LOCK: {
                    Integer afState = result.get(CaptureResult.CONTROL_AF_STATE);
                    if (afState == null) {
                        captureStillPicture();
                    } else if (CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED == afState ||
                            CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED == afState) {
                        // CONTROL_AE_STATE can be null on some devices
                        Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                        if (aeState == null ||
                                aeState == CaptureResult.CONTROL_AE_STATE_CONVERGED) {
                            mState = STATE_PICTURE_TAKEN;
                            captureStillPicture();
                        } else {
                            runPrecaptureSequence();
                        }
                    }
                    break;
                }
                case STATE_WAITING_PRECAPTURE: {
                    // CONTROL_AE_STATE can be null on some devices
                    Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                    if (aeState == null ||
                            aeState == CaptureResult.CONTROL_AE_STATE_PRECAPTURE ||
                            aeState == CaptureRequest.CONTROL_AE_STATE_FLASH_REQUIRED) {
                        mState = STATE_WAITING_NON_PRECAPTURE;
                    }
                    break;
                }
                case STATE_WAITING_NON_PRECAPTURE: {
                    // CONTROL_AE_STATE can be null on some devices
                    Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                    if (aeState == null || aeState != CaptureResult.CONTROL_AE_STATE_PRECAPTURE) {
                        mState = STATE_PICTURE_TAKEN;
                        captureStillPicture();
                    }
                    break;
                }
            }
        }

        @Override
        public void onCaptureStarted(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, long timestamp, long frameNumber) {
            super.onCaptureStarted(session, request, timestamp, frameNumber);
        }

        @Override
        public void onCaptureProgressed(@NonNull CameraCaptureSession session,
                                        @NonNull CaptureRequest request,
                                        @NonNull CaptureResult partialResult) {
            process(partialResult);
        }

        @Override
        public void onCaptureCompleted(@NonNull CameraCaptureSession session,
                                       @NonNull CaptureRequest request,
                                       @NonNull TotalCaptureResult result) {
            process(result);
        }

    };
}

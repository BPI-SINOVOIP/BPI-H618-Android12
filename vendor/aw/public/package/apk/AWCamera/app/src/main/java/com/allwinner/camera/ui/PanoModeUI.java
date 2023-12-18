package com.allwinner.camera.ui;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.media.Image;
import android.media.ImageReader;
import android.os.AsyncTask;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.RelativeLayout;

import com.allwinner.camera.R;
import com.allwinner.camera.cameracontrol.CameraManager;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.panoramic.NativePanoramic;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.YuvUtils;
import com.allwinner.camera.views.CircleRotateImageView;
import com.allwinner.camera.views.PanoPreviewView;

//import org.opencv.android.Utils;
//import org.opencv.core.CvType;
//import org.opencv.core.Mat;
//import org.opencv.imgcodecs.Imgcodecs;
//import org.opencv.imgproc.Imgproc;

import java.io.File;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class PanoModeUI extends BaseModeUI implements View.OnClickListener {
    private static final String TAG = "PanoModeUI";
    private final PanoPreviewView mPreviewImageView;
    private CircleRotateImageView mCaptureImageView;
    private ImageView mBackView;
    private RelativeLayout mCaptureRelativeLayout;
    private boolean mIsCaptureing = false;
    private int mHeight;
    private int mWidth;
    private boolean mMirror = false;
    private ImageReader mImageReader = null;
    private int mCameraId;
    private static final String PART_DIR = "/PanoramaApp/part";
    private static final String MAIN_PREFIX = "/panorama_";
    private static final String PART_PREFIX = "/part_panorama_";
    private static final String PNG = ".png";
    private static final String PATTERN = "yyyyMMddHHmmss";
    private static final String MAIN_DIR = "/PanoramaApp";
    private int mJpegRotation;

    private boolean mDoingConvert = false;
    private boolean mStartCaptrue;
    private int i =0;

//    List<Mat> mListImage = new ArrayList<>();
//
//    Mat mResult = new Mat();

    public PanoModeUI(View rootView, UIManager uiManager) {
        super(rootView, uiManager);
        //mUiManager.mThumbnailView.setVisibility(View.GONE);
        mCaptureRelativeLayout = rootView.findViewById(R.id.rl_capture);
        mCaptureRelativeLayout.setOnClickListener(this);
        mCaptureImageView = rootView.findViewById(R.id.iv_rotateCapture);
        mPreviewImageView = rootView.findViewById(R.id.iv_preview);
        mPreviewImageView.intMatrix();
        mBackView = rootView.findViewById(R.id.panobackground);
        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT,mPreviewImageView.getShowHeight()+50);
        lp.setMargins(0, mPreviewImageView.getTranY()-25, 0, 0);
        mBackView.setLayoutParams(lp);

    }

    @Override
    public void onOrientationChanged(int orientationCompensation) {
        mCaptureImageView.onOrientationChanged(orientationCompensation);

    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {

            case R.id.rl_capture:
                if (mIsCaptureing) {
                    mIsCaptureing = false;
                    //       stopCapture();
                } else {
                    mIsCaptureing = true;
                    //    startCapture();

                }
                break;

            default:
                break;
        }
    }

    @Override
    public Surface getPreviewSurface() {
        if (mImageReader != null) {
            mImageReader.close();
        }

        mImageReader = ImageReader.newInstance(CameraData.getInstance().getPreviewWidth(),
                CameraData.getInstance().getPreviewHeight(), ImageFormat.YUV_420_888, 5);
        mImageReader.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() {
            @Override
            public void onImageAvailable(ImageReader reader) {
                try (Image image = reader.acquireNextImage()) {
                    //因为我们平台的cameraid 0代表后置 1代表前置，跟谷歌的相反
                    mCameraId = CameraData.getInstance().getCameraId();
                    if (mCameraId == CameraData.getInstance().getBackCameraId()) {
                        //后置
                        mMirror = false;
                    } else {
                        mMirror = true;
                    }
                    if (image.getFormat() == ImageFormat.JPEG) {
                        //  onJpegDataTaken(image);
                    } else {
                        onYuvDataTaken(image);
                    }
                }
            }
        }, CameraData.getInstance().getCameraHandler());
        return mImageReader.getSurface();
    }

    public void onYuvDataTaken(Image image) {
        //Log.e(TAG, "onYuvDataTaken");
        mWidth = image.getWidth();
        mHeight = image.getHeight();
        int rowstride = image.getPlanes()[0].getRowStride();
        ByteBuffer ybuffer = image.getPlanes()[0].getBuffer();
        ByteBuffer uvbuffer = image.getPlanes()[2].getBuffer();
        ByteBuffer vubuffer = image.getPlanes()[1].getBuffer();
        byte[] yuvbuffer = new byte[rowstride * mHeight * 3 / 2];
        ybuffer.get(yuvbuffer, 0, rowstride * mHeight);
        uvbuffer.get(yuvbuffer, rowstride * mHeight, uvbuffer.remaining());
        image.close();
        ybuffer.clear();
        uvbuffer.clear();

        mJpegRotation = CameraUtils.getPreviewRotation(mCameraId);

/*
        ByteArrayOutputStream bos = new ByteArrayOutputStream(1024);
        YuvImage im = new YuvImage(yuvbuffer, ImageFormat.NV21, mWidth, mHeight, new int[]{mWidth, mWidth, mWidth});
        im.compressToJpeg(new Rect(0, 0, mWidth, mHeight), 97, bos);
        byte[] jpegdata = bos.toByteArray();
        CameraUtils.writeFile(CameraUtils.generateFilePath("12"), jpegdata);*/

        yuvbuffer = YuvUtils.rotateNV21(yuvbuffer, mWidth, mHeight, mJpegRotation, mMirror);

        if (mJpegRotation == 90 || mJpegRotation == 270) {
            int temp = mWidth;
            mWidth = mHeight;
            mHeight = temp;
        }

        mUiManager.setYuvData(yuvbuffer,mWidth,mHeight,0,mMirror);


        if (mDoingConvert) {
            return;
        }
/*
        if(mIsCaptureing) {

            if (mListImage.size() < 2) {
//                Mat mat = new Mat((int) (mHeight * 1.5), mWidth, CvType.CV_8UC1);//初始化一个矩阵,没数据
//                mat.put(0, 0, yuvbuffer);
                Mat mat = new Mat(mHeight*3/2,mWidth,CvType.CV_8UC1);//,byteBuffer 1440,1080
                int re =  mat.put(0,0,yuvbuffer);
                Mat bgr_i420 = new Mat();
                Imgproc.cvtColor(mat , bgr_i420, Imgproc.COLOR_YUV2BGR_NV21);//COLOR_YUV2BGR_I420
                Log.e(TAG,"i:"+i+"mat:"+bgr_i420.width());

                mListImage.add(bgr_i420);
            }

            if (mListImage.size() == 2 ) {
                mDoingConvert = true;
                new PanoTask(mJpegRotation).execute();

            }
        } else {
            mResult.release();
            mListImage.clear();
        }

*/

    }
    /*
    private class PanoTask extends AsyncTask<Void, Void, Bitmap> {
        private byte[] mData = null;
        private int mJpegRotation = 0;
        public PanoTask(int jpegRotation) {
            this.mJpegRotation = jpegRotation;
        }

        @Override
        protected  Bitmap doInBackground(Void... voids) {
            Log.e(TAG,"I==2:"+ mListImage.size());
            long[] tempObjAddress = new long[mListImage.size()];
            for (int i = 0; i < mListImage.size(); i++) {
                tempObjAddress[i] = mListImage.get(i).getNativeObjAddr();
            }
            //  NativePanoramic.getProgress();
            String[] args = {"part", "orb", "spherical", "dp_color", "no"};
            int status =NativePanoramic.processPanorama(tempObjAddress, mResult.getNativeObjAddr(), args);
            //save to external storage
            String path = null;
            Bitmap bitmap = null;
          Log.e(TAG,"status:"+status);
            if (!mResult.empty()) {
              //  bitmap = Bitmap.createBitmap(mResult.width(), mResult.height(), Bitmap.Config.ARGB_8888);
               // NativePanoramic.getBitmap(bitmap);
                path = savePartResultImageExternal(mResult);
               File file = new File(path);
                if(path!=null &&file.exists()){
                    bitmap = BitmapFactory.decodeFile(path);
                    file.delete();
                }
//                try {
//                    Utils.matToBitmap(mResult,bitmap);
//                } catch (Exception e) {
//                    Log.e(TAG, "Mat type: " + mResult);
//                    Log.e(TAG, "Bitmap type: " + bitmap.getWidth() + "*" + bitmap.getHeight());
//                    Log.e(TAG, "Utils.matToBitmap() throws an exception: " + e.getMessage());
//                }

            }

            for (Mat mat : mListImage) mat.release();
            mListImage.clear();
            //bitmap to mat ,add mat
            if (!mResult.empty()) {
                Mat mat = new Mat();
                Utils.bitmapToMat(bitmap, mat);
                mListImage.add(mat);
            }

            mDoingConvert = false;
            return bitmap;
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            mPreviewImageView.setBitmap(bitmap);
        }
    }
 public static String savePartResultImageExternal(Mat result) {
        Log.e(TAG, "savePartResultImageExternal: begin saving");
        File folder = new File(Environment.getExternalStorageDirectory() + PART_DIR);
        Date date = new Date();
        SimpleDateFormat simple = new SimpleDateFormat(PATTERN, Locale.getDefault());
        final String fileName = folder.getAbsolutePath() + PART_PREFIX + simple.format(date) + PNG;
        Log.e(TAG, "saveResultImageExternal: filename: " + fileName);
        if (isPathCreated(PART_DIR)) {
            try {

                       if( Imgcodecs.imwrite(fileName, result)){
                           return fileName;
                       }
            } catch (Exception e) {
                Log.e(TAG, "Part File saving failed", e);
            }
        } else {
            Log.e(TAG, "Part File saving failed");
        }
        return null;
    }
    */
    public static boolean isPathCreated(String path) {
        File folder = new File(Environment.getExternalStorageDirectory() + path);
        boolean success = true;
        if (!folder.exists()) {
            success = folder.mkdirs();
        }
        return success;
    }
}

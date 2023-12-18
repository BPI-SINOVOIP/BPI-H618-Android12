package com.allwinner.camera.cameracontrol;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.media.Image;
import android.media.ImageReader;
import android.net.Uri;
import android.os.AsyncTask;
import android.util.Log;

import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.program.FilterCaptureEngine;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.ThumbnailUtils;
import com.allwinner.camera.utils.YuvUtils;

import org.greenrobot.eventbus.EventBus;

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;

import static com.allwinner.camera.utils.CameraUtils.getSquarePictrueBitmap;


public class CatpureImageAvailableListener implements ImageReader.OnImageAvailableListener, FilterCaptureEngine.FilterDataListener {
    private int mJpegRotation;
    private long mCaptureTime;
    private static final String TAG = "CatpureImageAvailableListener";
    private Context mContext;
    private int mWidth;
    private int mHeight;
    private String mPath;
    private FilterCaptureEngine mFilterCaptureEngine;
    private boolean mMirror = false;

    public CatpureImageAvailableListener(Context context, long captureTime, int jpegRotation, FilterCaptureEngine filterCaptureEngine) {
        mContext = context;
        mJpegRotation = jpegRotation;
        mCaptureTime = captureTime;
        mFilterCaptureEngine = filterCaptureEngine;
        mFilterCaptureEngine.setFilterDataListener(this);
    }

    @Override
    public void onImageAvailable(ImageReader reader) {
        try (Image image = reader.acquireNextImage()) {
            if (image == null) return;
            if (CameraData.getInstance().getCameraId() == CameraData.getInstance().getFrontCameraId()) {
                //前置
                mMirror = true;
            } else {
                mMirror = false;
            }
            if (image.getFormat() == ImageFormat.JPEG) {
                onJpegDataTaken(image);
            } else {
                onYuvDataTaken(image);
            }
        }
    }

    public void onJpegDataTaken(Image image) {
        mWidth = image.getWidth();
        mHeight = image.getHeight();
        if (mJpegRotation == 90 || mJpegRotation == 270) {
            int temp = mWidth;
            mWidth = mHeight;
            mHeight = temp;
        }
        Log.i(TAG, "onJpegDataTaken: size: " + mWidth + "x" + mHeight + "mJpegRotation：" + mJpegRotation);
        int stride = image.getPlanes()[0].getRowStride();
        int originStride = stride;
        byte[] jpegdata = null;
        ByteBuffer buffer = image.getPlanes()[0].getBuffer();
        jpegdata = new byte[buffer.remaining()];
        buffer.get(jpegdata);
        image.close();
        EventBus.getDefault().post(Contants.MsgType.MSG_ON_CAPTURE_COMPLETE);
        ThumSaveTask thumSaveTask = new ThumSaveTask(jpegdata, mJpegRotation);
        thumSaveTask.execute();
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
        String name = format.format(mCaptureTime);
        mPath = CameraUtils.generateFilePath(name);
        Uri uri;
        if (CameraData.getInstance().getCurrentModeType().equals(Contants.ModeType.SquareMode)) {
            Bitmap bitmap = getSquarePictrueBitmap(jpegdata, mWidth, mHeight);
            int finalSize = Math.min(mWidth, mHeight);
            uri = CameraUtils.insertMedia(mContext.getContentResolver(), name, mCaptureTime, mJpegRotation, mPath, finalSize, finalSize);
            CameraUtils.saveJpeg(mContext.getContentResolver(),uri, bitmap);
        } else {
            uri = CameraUtils.insertMedia(mContext.getContentResolver(), name, mCaptureTime, mJpegRotation, mPath, mWidth, mHeight);
            CameraUtils.writeFile(mContext.getContentResolver(),uri, jpegdata);
        }
        CameraUtils.addExif(mContext, uri, mCaptureTime);
        Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_PICTURETAKEN_FINISH, uri);
        EventBus.getDefault().post(object);
    }

    public void onYuvDataTaken(Image image) {
        mWidth = image.getWidth();
        mHeight = image.getHeight();
        int rowstride = image.getPlanes()[0].getRowStride();
        Log.i(TAG, "onYuvDataTaken:" + mWidth + "x" + mHeight + " rowstrde" + rowstride);
        ByteBuffer ybuffer = image.getPlanes()[0].getBuffer();
        ByteBuffer uvbuffer = image.getPlanes()[2].getBuffer();
        ByteBuffer vubuffer = image.getPlanes()[1].getBuffer();
        byte[] yuvbuffer = new byte[rowstride * mHeight * 3 / 2];
        ybuffer.get(yuvbuffer, 0, rowstride * mHeight);
        uvbuffer.get(yuvbuffer, rowstride * mHeight, uvbuffer.remaining());
        image.close();
        EventBus.getDefault().post(Contants.MsgType.MSG_ON_CAPTURE_COMPLETE);
        if (mFilterCaptureEngine.getFilterType() != Contants.FilterType.Normal  &&
                mFilterCaptureEngine.getFilterType() != Contants.FilterType.YUV
                ||mFilterCaptureEngine.getFaceBeautyOn()||needAddWaterSign() ||CameraUtils.needDoSticker()) {
            Log.i(TAG, "doFilter");
            FilterCaptureEngine.PramData pramData = new FilterCaptureEngine.PramData();
            pramData.mWidth = mWidth;
            pramData.mHeight = mHeight;
            pramData.mCaptureTime = mCaptureTime;
            pramData.mJpegRotation = mJpegRotation;
            pramData.mMirror = mMirror;
            mFilterCaptureEngine.doFilter(pramData, yuvbuffer);
        } else {
            yuvbuffer = YuvUtils.rotateNV21(yuvbuffer, mWidth, mHeight, mJpegRotation, mMirror);
            if (mJpegRotation == 90 || mJpegRotation == 270) {
                int temp = mWidth;
                mWidth = mHeight;
                mHeight = temp;
            }
            ByteArrayOutputStream bos = new ByteArrayOutputStream(1024);
            YuvImage im = new YuvImage(yuvbuffer, ImageFormat.NV21, mWidth, mHeight, new int[]{mWidth, mWidth, mWidth});

            im.compressToJpeg(new Rect(0, 0, mWidth, mHeight), 97, bos);
            byte[] jpegdata = bos.toByteArray();
            if(CameraData.getInstance().isImageCaptureIntent()){

                Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_UPDATE_BYTE, jpegdata);
                EventBus.getDefault().post(object);
             //   onCaptureDone(jpegdata);
            }else {
                ThumSaveTask thumSaveTask = new ThumSaveTask(jpegdata, 0);
                thumSaveTask.execute();
                SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
                String name = format.format(mCaptureTime);
                Uri uri;
                mPath = CameraUtils.generateFilePath(name);

                if (CameraData.getInstance().getCurrentModeType().equals(Contants.ModeType.SquareMode)) {
                    Bitmap bitmap = getSquarePictrueBitmap(jpegdata, mWidth, mHeight);
                   // CameraUtils.saveJpeg(mPath, bitmap);
                    int finalSize = Math.min(mWidth, mHeight);
                    uri = CameraUtils.insertMedia(mContext.getContentResolver(), name, mCaptureTime, 0, mPath, finalSize, finalSize);
                    CameraUtils.saveJpeg(mContext.getContentResolver(),uri, bitmap);
                } else {
                    //
                    uri =CameraUtils.insertMedia(mContext.getContentResolver(), name, mCaptureTime, 0, mPath, mWidth, mHeight);
                    CameraUtils.writeFile(mContext.getContentResolver(),uri, jpegdata);
                   // CameraUtils.updateMedia(mContext.getContentResolver(), name, mCaptureTime, 0, mPath, mWidth, mHeight,uri);
                }
                CameraUtils.addExif(mContext, uri, mCaptureTime);
                Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_PICTURETAKEN_FINISH, uri);
                EventBus.getDefault().post(object);

            }
        }
        yuvbuffer = null;
    }

    public boolean needAddWaterSign(){
        boolean isNeed =!CameraData.getInstance().getModelWaterSignType().equals(Contants.ModelWatersignType.TYPE_OFF)
                || CameraData.getInstance().getIsTimeWaterSign();
        return isNeed;
    }
    @Override
    public void onFilterComplete(FilterCaptureEngine.PramData pramData, byte[] yuvdata, byte[] nv21data) {
        Log.i(TAG, "onFilterComplete");
        YuvUtils.changeYuvToNv21(yuvdata, nv21data, pramData.mWidth, pramData.mHeight, 4);
        pramData.mJpegRotation = 0;
        ByteArrayOutputStream bos = new ByteArrayOutputStream(1024);
        YuvImage im = new YuvImage(nv21data, ImageFormat.NV21, pramData.mWidth, pramData.mHeight, new int[]{pramData.mWidth, pramData.mWidth, pramData.mWidth});
        im.compressToJpeg(new Rect(0, 0, pramData.mWidth, pramData.mHeight), 97, bos);
        byte[] jpegdata = bos.toByteArray();
        Uri uri;
        if(CameraData.getInstance().isImageCaptureIntent()){

            Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_UPDATE_BYTE, jpegdata);
            EventBus.getDefault().post(object);
            //   onCaptureDone(jpegdata);
        }else {
            ThumSaveTask thumSaveTask = new ThumSaveTask(jpegdata, pramData.mJpegRotation);
            thumSaveTask.execute();
            SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
            String name = format.format(mCaptureTime);
            mPath = CameraUtils.generateFilePath(name);
            if (CameraData.getInstance().getCurrentModeType().equals(Contants.ModeType.SquareMode)) {
                Bitmap bitmap = getSquarePictrueBitmap(jpegdata, pramData.mWidth, pramData.mHeight);
                int finalSize = Math.min(pramData.mWidth, pramData.mHeight);
                uri = CameraUtils.insertMedia(mContext.getContentResolver(), name, mCaptureTime, pramData.mJpegRotation, mPath, finalSize, finalSize);
                CameraUtils.saveJpeg(mContext.getContentResolver(),uri, bitmap);
                CameraUtils.addExif(mContext, uri, pramData.mCaptureTime);
            } else {
                uri = CameraUtils.insertMedia(mContext.getContentResolver(), name, mCaptureTime, pramData.mJpegRotation, mPath,pramData.mWidth, pramData.mHeight);
                CameraUtils.writeFile(mContext.getContentResolver(),uri, jpegdata);
                CameraUtils.addExif(mContext, uri, mCaptureTime);
            }
            Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_PICTURETAKEN_FINISH, uri);
            EventBus.getDefault().post(object);
        }
    }

    @Override
    public void onFilterDoing() {

    }


    private class ThumSaveTask extends AsyncTask<Void, Void, Bitmap> {
        private byte[] mData = null;
        private int mJpegRotation = 0;

        public ThumSaveTask(byte[] data, int jpegRotation) {
            this.mData = data;
            this.mJpegRotation = jpegRotation;
        }

        @Override
        protected Bitmap doInBackground(Void... voids) {
            Bitmap bitmap = ThumbnailUtils.getThumbnailBitmap(mData, mWidth, mHeight, mJpegRotation);
            return bitmap;
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            Contants.EventObject object = new Contants.EventObject(Contants.MsgType.MSG_ON_UPDATE_THUMBNAIL, bitmap);
            EventBus.getDefault().post(object);
        }
    }

}

package com.softwinner.wfdsink;

import android.content.Context;
import android.app.Activity;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaDataSource;

import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Gravity;
import android.view.OrientationEventListener;

import android.widget.FrameLayout;
import android.widget.MediaController;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import android.database.ContentObserver;
import android.provider.Settings;

import java.nio.ByteBuffer;
import java.io.IOException;
import java.lang.IllegalStateException;
import java.util.Map;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Collections;
import java.util.Queue;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.ConcurrentLinkedQueue;

import android.view.WindowManager;
import android.util.DisplayMetrics;

public class WFDSinkView extends SurfaceView implements SurfaceHolder.Callback,
                                                        MediaController.MediaPlayerControl, WFDCallback {
    private static final String TAG = "WFDSinkView";
    //private int mOrientation = 0;

    private Context mContext;
    private String mIP = null;
    private int mPort = 0;
    //private ContentObserver mSettingsValueChangeContentObserver;
    //private OrientationEventListener mOrientationEventListener;

    private MediaExtractor mExtractor = null;
    private MyTSDataSource mDataSource = null;

    private WFDManager mWFDManager = null;

    private Map<Integer, CodecState> mCodecStates;
    private CodecState mAudioTrackState;

    private static final int STATE_IDLE = 1;
    private static final int STATE_PREPARING = 2;
    private static final int STATE_PLAYING = 3;
    private static final int STATE_PAUSED = 4;
    private int mState = STATE_IDLE;

    private Handler mHandler = null;
    private static final int EVENT_PREPARE = 1;
    private static final int EVENT_DO_SOME_WORK = 2;
    private static final int EVENT_VIDEO_CROP = 3;
    private static final int EVENT_VIDEO_LANDSCAPE = 4;
    private static final int EVENT_VIDEO_PORTRAIT  = 5; //portrait

    private long mDeltaTimeUs = -1;
    private long mDurationUs = -1;
    private int mViewWidth = -1;
    private int mViewHeight = -1;

    private MediaController mMediaController = null;

    private ViewCallback mViewCallback = null;

    private Activity findActivityForContext(Context context) {
        return (context instanceof Activity) ? (Activity) context : null;
    }

    public void setViewCallback(ViewCallback l) {
        mViewCallback = l;
    }

    public WFDSinkView(Context context) {
        super(context);
        mContext = context;

        getHolder().addCallback(this);

        // If we have an activity for this context we'll add the SurfaceView to it (as a (int width, int height) view
        // in the top-left corner). If not, we warn the user that they may need to add one manually.
        Activity activity = findActivityForContext(mContext);
        if (activity != null) {
            //ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(
            //    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            //FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
            //    FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT);
            //FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
            //    FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
            activity.addContentView(this, params);

            WindowManager windowManager = (WindowManager)mContext.getSystemService(Context.WINDOW_SERVICE);
            DisplayMetrics metrics =new DisplayMetrics();
            windowManager.getDefaultDisplay().getRealMetrics(metrics);
            mViewWidth = metrics.widthPixels;
            mViewHeight = metrics.heightPixels;
            Log.d(TAG,"width = " + mViewWidth + " height = " + mViewHeight);
        } else {
            Log.e(TAG, "Could not find activity for our SurfaceView");
        }

        initWFDSinkView();
    }

    private void initWFDSinkView() {
        mState = STATE_IDLE;

        mHandler = new Handler() {
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case EVENT_PREPARE:
                    {
                        try {
                            prepare();
                            start();
                        } catch (IOException e) {
                            Log.d(TAG, "prepare failed.");
                        }
                        break;
                    }

                    case EVENT_DO_SOME_WORK:
                    {
                        if (mState == STATE_PLAYING) doSomeWork();

                        if (mState == STATE_PLAYING) {
                            mHandler.sendMessageDelayed(
                                    mHandler.obtainMessage(EVENT_DO_SOME_WORK), 5);
                        }
                        break;
                    }
                    case EVENT_VIDEO_CROP:
                    {
                        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams)getLayoutParams();
                        if (msg.arg1 == 0) {
                            params.height = msg.arg2;
                            params.width = params.MATCH_PARENT;
                        } else {
                            params.height = params.MATCH_PARENT;
                            params.width = params.MATCH_PARENT;
                        }
                        Log.d(TAG, "handle message EVENT_VIDEO_CROP");
                        params.gravity = Gravity.CENTER;
                        setLayoutParams(params);
                        break;
                    }
                    case EVENT_VIDEO_LANDSCAPE:
                    {
                        Log.d(TAG, "handle message EVENT_VIDEO_LANDSCAPE");
                        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams)getLayoutParams();
                        params.height = params.MATCH_PARENT;
                        params.width = params.MATCH_PARENT;
                        params.gravity = Gravity.CENTER;
                        setLayoutParams(params);
                        break;
                    }

                    case EVENT_VIDEO_PORTRAIT:
                    {
                        Log.d(TAG,"handle message EVENT_VIDEO_PORTRAIT");
                        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams)getLayoutParams();
                        params.height = mViewHeight > mViewWidth ? mViewHeight / 2 : mViewWidth / 2;
                        params.width = mViewHeight > mViewWidth ? mViewWidth : mViewHeight;
                        params.gravity = Gravity.CENTER;
                        setLayoutParams(params);

                        Log.d(TAG,"view width =" + params.width + "   view height = " +  params.height);
                        break;
                    }

                    default:
                        break;
                }
            }
        };
    }

    //class SettingsValueChangeContentObserver extends ContentObserver {
    //    public SettingsValueChangeContentObserver() {
    //        super(new Handler());
    //    }
    //    @Override
    //    public void onChange(boolean selfChange) {
    //        super.onChange(selfChange);
    //        Log.w(TAG, "observer listen rotation degree is " + Settings.System.getString(mContext.getContentResolver(), Settings.System.USER_ROTATION));
    //        try{
    //            int rotation = Settings.System.getInt(mContext.getContentResolver(), Settings.System.USER_ROTATION);
    //            mWFDManager.nativeWFDManagerSetScreenRotation(rotation);
    //            if (rotation == 0 || rotation == 2) {
    //                Message msg = mHandler.obtainMessage();
    //                msg.what = EVENT_VIDEO_LANDSCAPE;
    //                mHandler.sendMessage(msg);
    //            }
    //        } catch (Exception exception) {
    //        }
    //    }
    //}

    public void setScreenRotationChanged(int orientation) {
        Log.d(TAG, "setScreenRotationChanged = " + orientation);
        if (mWFDManager != null) {
            mWFDManager.nativeWFDManagerSetScreenRotation(orientation);
            Message msg = mHandler.obtainMessage();
            if (orientation == 0 || orientation == 2) {
                msg.what = EVENT_VIDEO_LANDSCAPE;
            } else {
                msg.what = EVENT_VIDEO_PORTRAIT;
            }
            mHandler.sendMessage(msg);
        }
    }

    public void setDataSource(
            Context context, String ip, int port) {
        Log.d(TAG, "setDataSource: rtsp://" + ip + ":" + port);

        reset();

        mIP = ip;
        mPort = port;
        mDataSource = new MyTSDataSource();
        mWFDManager = new WFDManager();

        mWFDManager.registerCallbacks(WFDSinkView.this);
        mWFDManager.WFDManagerSetup(mIP, mPort); // will create a new native thread

        //mSettingsValueChangeContentObserver = new SettingsValueChangeContentObserver();
        //context.getContentResolver().registerContentObserver(Settings.System.getUriFor(Settings.System.USER_ROTATION), true, mSettingsValueChangeContentObserver);

        //mOrientationEventListener = new OrientationEventListener(context) {
        //    @Override
        //    public void onOrientationChanged(int orientation) {
        //        Log.w(TAG, "listener received onOrientationChanged: orientation is " + orientation);
        //        int tempOrientation = -1;
        //        if (orientation == OrientationEventListener.ORIENTATION_UNKNOWN) return;
        //        if (orientation > 340 || orientation < 20) {
        //            tempOrientation = 0;
        //        } else if (orientation > 70 && orientation < 110) { //90度
        //            tempOrientation = 1;
        //        } else if (orientation > 160 && orientation < 200) { //180度
        //            tempOrientation = 2;
        //        } else if (orientation > 250 && orientation < 290) { //270度
        //            tempOrientation = 3;
        //        }

        //        if (tempOrientation != -1 && mOrientation != tempOrientation) {
        //            mOrientation = tempOrientation;
        //            try{
        //                mWFDManager.nativeWFDManagerSetScreenRotation(mOrientation);
        //                if (mOrientation == 0 || mOrientation == 2) {
        //                    Message msg = mHandler.obtainMessage();
        //                    msg.what = EVENT_VIDEO_LANDSCAPE;
        //                    mHandler.sendMessage(msg);
        //                }
        //            } catch (Exception exception) {
        //            }
        //        }

        //    }
        //};

        //if (mOrientationEventListener.canDetectOrientation()) {
        //    Log.w(TAG, "this device has gravity sensor");
        //    mOrientationEventListener.enable();
        //} else {
        //    Log.w(TAG, "this device doesn't have gravity sensor!");
        //    mOrientationEventListener.disable();
        //    mOrientationEventListener = null;
        //}
    }

    private void prepare() throws IOException{
        Log.d(TAG, "prepare");

        try {
            mExtractor = new MediaExtractor();

            mExtractor.setDataSource(mDataSource); // will block
        } catch (IOException e) {
            if (mViewCallback != null) mViewCallback.viewCallback();

            throw e;
        }

        mDataSource.start(); // important

        mCodecStates = new HashMap();

        boolean haveAudio = false;
        boolean haveVideo = false;
        for (int i = mExtractor.getTrackCount(); i-- > 0;) {
            MediaFormat format = mExtractor.getTrackFormat(i);
            Log.d(TAG, "track format #" + i + " is " + format);

            String mime = format.getString(MediaFormat.KEY_MIME);

            boolean isVideo = mime.startsWith("video/");
            boolean isAudio = mime.startsWith("audio/");

            if (!haveAudio && isAudio || !haveVideo && isVideo) {
                mExtractor.selectTrack(i);
                addTrack(i, format);

                if (isAudio) {
                    haveAudio = true;
                } else {
                    haveVideo = true;
                }

                if (format.containsKey(MediaFormat.KEY_DURATION)) {
                    long durationUs = format.getLong(MediaFormat.KEY_DURATION);

                    if (durationUs > mDurationUs) {
                        mDurationUs = durationUs;
                    }
                }

                if (haveAudio && haveVideo) {
                    break;
                }
            }
        }

        mState = STATE_PAUSED;
    }

    private void addTrack(
            int trackIndex, MediaFormat format) {
        String mime = format.getString(MediaFormat.KEY_MIME);

        boolean isVideo = mime.startsWith("video/");
        boolean isAudio = mime.startsWith("audio/");

        //if (isVideo/* || isAudio*/) { // just for debug single a/v
            MediaCodec codec;

            try {
                codec = MediaCodec.createDecoderByType(mime);
            } catch (IOException e) {
                throw new RuntimeException("failed to create decoder for "+ format.getString(MediaFormat.KEY_MIME), e);
            }

            codec.configure(
                    format,
                    isVideo ? getHolder().getSurface() : null,
                    null,
                    0);

            CodecState state =
                new CodecState(this, mExtractor, trackIndex, format, codec);

            mCodecStates.put(new Integer(trackIndex), state);

            if (isAudio) {
                mAudioTrackState = state;
            }
            // set video view size
            invalidate();
        //}
    }

    public void reset() {
        Log.d(TAG, "reset");

        if (mState == STATE_PLAYING) {
            pause();
        }

        if (mMediaController != null) {
            mMediaController.setEnabled(false);
        }

        if (mDataSource != null) { // leave potential r/w loop
            mDataSource.close();
            mDataSource = null;
        }

        if (mExtractor != null) { // leave potential r/w loop
            mExtractor.release();
            mExtractor = null;
        }

        if (mCodecStates != null) {
            for (CodecState state : mCodecStates.values()) {
                state.release();
            }
            mCodecStates = null;
        }

        mDurationUs = -1;
        mState = STATE_IDLE;

        if (mWFDManager != null) {
            mWFDManager.WFDManagerStop();
            mWFDManager.WFDManagerDestroy();
            mWFDManager = null;
        }
        //if (mSettingsValueChangeContentObserver != null)
        //    mContext.getContentResolver().unregisterContentObserver(mSettingsValueChangeContentObserver);

        //if (mOrientationEventListener != null)
        //    mOrientationEventListener.disable();
    }

    private void doSomeWork() {
        for (CodecState state : mCodecStates.values()) {
            if (state.doSomeWork()) {
                if (mViewCallback != null) mViewCallback.viewCallback();
            }
        }
    }

    public long getNowUs() {
        if (mAudioTrackState == null) {
            return System.currentTimeMillis() * 1000;
        }

        return mAudioTrackState.getAudioTimeUs();
    }

    public long getRealTimeUsForMediaTime(long mediaTimeUs) {
        if (mDeltaTimeUs == -1) {
            long nowUs = getNowUs();
            mDeltaTimeUs = nowUs - mediaTimeUs;
        }

        return mDeltaTimeUs + mediaTimeUs;
    }

    public void setMediaController(MediaController ctrl) {
        if (mMediaController != null) {
            mMediaController.hide();
        }
        mMediaController = ctrl;
        attachMediaController();
    }

    private void attachMediaController() {
        if (mMediaController != null) {
            View anchorView =
                this.getParent() instanceof View ?  (View)this.getParent() : this;

            mMediaController.setMediaPlayer(this);
            mMediaController.setAnchorView(anchorView);
            mMediaController.setEnabled(true);
        }
    }

////////////////////////////////////////////////////////////////////////////////
    @Override
    public void start() {
        Log.d(TAG, "start");

        if (mState == STATE_PLAYING || mState == STATE_PREPARING) {
            return;
        } else if (mState == STATE_IDLE) {
            mState = STATE_PREPARING;
            mHandler.sendMessage(mHandler.obtainMessage(EVENT_PREPARE)); // TODO
            return;
        } else if (mState != STATE_PAUSED) {
            throw new IllegalStateException();
        }

        mState = STATE_PLAYING;

        for (CodecState state : mCodecStates.values()) {
            state.start();
        }

        mHandler.sendMessage(mHandler.obtainMessage(EVENT_DO_SOME_WORK));

        mDeltaTimeUs = -1;

        if (mMediaController != null) {
            mMediaController.show();
        }
    }

    @Override
    public void pause() {
        Log.d(TAG, "pause");

        if (mState == STATE_PAUSED) {
            return;
        } else if (mState != STATE_PLAYING) {
            throw new IllegalStateException();
        }

        mState = STATE_PAUSED;

        mHandler.removeMessages(EVENT_DO_SOME_WORK);

        for (CodecState state : mCodecStates.values()) {
            state.pause();
        }
    }

    @Override
    public int getDuration() {
        return (int)((mDurationUs + 500) / 1000);
    }

    public int getCurrentPosition() {
        if (mCodecStates == null) {
            return 0;
        }

        long positionUs = 0;

        for (CodecState state : mCodecStates.values()) {
            long trackPositionUs = state.getCurrentPositionUs();

            if (trackPositionUs > positionUs) {
                positionUs = trackPositionUs;
            }
        }

        return (int)((positionUs + 500) / 1000);
    }

    @Override
    public void seekTo(int timeMs) {
        if (mState != STATE_PLAYING && mState != STATE_PAUSED) {
            return;
        }

        mExtractor.seekTo(timeMs * 1000, MediaExtractor.SEEK_TO_CLOSEST_SYNC);

        for (CodecState state : mCodecStates.values()) {
            state.flush();
        }

        Log.d(TAG, "seek to " + timeMs * 1000);

        mDeltaTimeUs = -1;
    }

    @Override
    public boolean isPlaying() {
        return mState == STATE_PLAYING;
    }

    @Override
    public int getBufferPercentage() {
        if (mExtractor == null) {
            return 0;
        }

        long cachedDurationUs = mExtractor.getCachedDuration();

        if (cachedDurationUs < 0 || mDurationUs < 0) {
            return 0;
        }

        int nowMs = getCurrentPosition();

        int percentage =
            100 * (nowMs + (int)(cachedDurationUs / 1000))
                / (int)(mDurationUs / 1000);

        if (percentage > 100) {
            percentage = 100;
        }

        return percentage;
    }

    @Override
    public boolean canPause() {
        return false; // TODO
    }

    @Override
    public boolean canSeekBackward() {
        return false; // TODO
    }

    @Override
    public boolean canSeekForward() {
        return false; // TODO
    }

    @Override
    public int getAudioSessionId() {
        return 0;
    }

////////////////////////////////////////////////////////////////////////////////
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) { // TODO
        Log.d(TAG, "surfaceChanged: format=" + format + ", width=" + width + ", height=" + height);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) { // TODO
        Log.d(TAG, "surfaceCreated");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) { // TODO
        Log.d(TAG, "surfaceDestroyed");
        reset();
    }

////////////////////////////////////////////////////////////////////////////////not for LOCAL_PLAYER
    @Override
    public final int handleRTSPStatus(int status) {
        Log.d(TAG, "handleRTSPStatus: " + status);

        switch (status) {
            case WFDManager.WFD_SETUP:
                start();
                break;
            case WFDManager.WFD_PLAY:
                start(); // TODO
                break;
            case WFDManager.WFD_PAUSE:
                pause(); // TODO
                break;
            case WFDManager.WFD_TEARDOWN:
                if (mViewCallback != null) mViewCallback.viewCallback();
                break;
            default:
                break;
        }

        return -1;
    }

    @Override
    public final int handleVideoCrop(int cropx, int croph) {
        Log.d(TAG, "handleVideoCrop: cropx: " + cropx + " croph: "+croph);
        Message msg = mHandler.obtainMessage();
        msg.what = EVENT_VIDEO_CROP;
        msg.arg1 = cropx;
        msg.arg2 = croph;
        mHandler.sendMessage(msg);
        return 0;
    }

    @Override
    public final int feedData(byte[] data, int len) {
        //Log.d(TAG, "feedData: " + len);

        if (mDataSource == null) return -1;

        return mDataSource.feedMore(data, len);
    }

////////////////////////////////////////////////////////////////////////////////just for LOCAL_PLAYER
    @Override
    public final void handleException() {
        Log.d(TAG, "handleException");

        if (mViewCallback != null) mViewCallback.viewCallback();
    }

    @Override
    public final Surface requestSurface() {
        Log.d(TAG, "requestSurface");

        return getHolder().getSurface();
    }

////////////////////////////////////////////////////////////////////////////////
    private class MyTSDataSource extends MediaDataSource {
        private static final String TAG = "MyTSDataSource";

        private int totalBufferSize = 1316 * 1024 * 10;
        private byte[] cache = new byte[totalBufferSize];
        private volatile int writePtr = 0;
        private volatile int readPtr = 0;
        private volatile int validDataSize = 0;
        private volatile int validBufferSize = 0;
        private volatile boolean isStart = false;
        private volatile boolean isCache = false;

        public MyTSDataSource() {
            Log.d(TAG, "create");
            writePtr = 0;
            readPtr = 0;
            validDataSize = 0;
            validBufferSize = totalBufferSize;
            isStart = false;
            isCache = false;
        }

        public void start() {
            Log.d(TAG, "start");
            isStart = true;
        };

        private void resync() {
            while (true) { // resync start
                if (cache == null || totalBufferSize <= 0) {
                    return; // reset
                } else if (validDataSize <= 0) {
                } else if (validDataSize <= 188) {
                    if (cache[readPtr] != 0x47) {
                        readPtr++;
                        if (readPtr >= totalBufferSize) {
                            readPtr -= totalBufferSize;
                        }
                        validBufferSize++;
                        validDataSize--;
                    }
                } else {
                    if (cache[readPtr] != 0x47 ||
                            ((readPtr + 188 < totalBufferSize) && cache[readPtr + 188] != 0x47) ||
                            ((readPtr + 188 >= totalBufferSize) && cache[readPtr + 188 - totalBufferSize] != 0x47)) {
                        readPtr++;
                        if (readPtr >= totalBufferSize) {
                            readPtr -= totalBufferSize;
                        }
                        validBufferSize++;
                        validDataSize--;
                    } else {
                        break; // resync done
                    }
                }
                try {
                    Thread.sleep(5);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        };

        public int feedMore(byte[] data, int len) {
            //Log.d(TAG, "feedMore " + len + ", " + validBufferSize);

            int ret = -1;

            while (validBufferSize < len) {
                //Log.d(TAG, "feedMore overflow " + len + " vs " + validBufferSize);
                if (cache == null || totalBufferSize <= 0)
                    return -1;
                try {
                    Thread.sleep(5);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            ret = len;
            int leftDataSize = totalBufferSize - writePtr;
            if (leftDataSize >= ret) {
                System.arraycopy(data, 0, cache, writePtr, ret);
            } else {
                System.arraycopy(data, 0, cache, writePtr, leftDataSize);
                System.arraycopy(data, leftDataSize, cache, 0, ret - leftDataSize);
            }

            writePtr += ret;
            if (writePtr >= totalBufferSize) {
                writePtr -= totalBufferSize;
            }
            validBufferSize -= ret;
            validDataSize += ret;

            return ret;
        };

        public int readAt(long position, byte[] buffer, int offset, int size) {
            if (!isStart) Log.d(TAG, "readAt: position=" + position + ", size=" + size + ", valid=" + validDataSize);

            int ret = -1;

            if (isStart) {
                if (isCache) {
                    readPtr += (int)position;
                    if (readPtr >= totalBufferSize) {
                        readPtr -= totalBufferSize;
                    }
                    validBufferSize += (int)position;
                    validDataSize -= (int)position;
                    isCache = false;
                }

                while (validDataSize < size) { // important
                    //Log.d(TAG, "readAt underflow " + size + " vs " + validDataSize);
                    if (cache == null || totalBufferSize <= 0)
                        return -1;
                    try {
                        Thread.sleep(5);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }

                ret = size;
                int leftDataSize = totalBufferSize - readPtr;
                if (leftDataSize >= ret) {
                    System.arraycopy(cache, readPtr, buffer, offset, ret);
                } else {
                    System.arraycopy(cache, readPtr, buffer, offset, leftDataSize);
                    System.arraycopy(cache, 0, buffer, offset + leftDataSize, ret - leftDataSize);
                }

                readPtr += ret;
                if (readPtr >= totalBufferSize) {
                    readPtr -= totalBufferSize;
                }
                validBufferSize += ret;
                validDataSize -= ret;
            } else {
                isCache = true;
                if (totalBufferSize < (readPtr + (int)position + size)) return -1;

                if (position % 2048 != 0 || size != 2048) { // hack mpeg2ts found
                    ret = 0;
                } else {
                    if (position == 0) resync(); // resync the sync byte 0x47 for ATSParser

                    while (validDataSize < (int)position + size) { // important
                        //Log.d(TAG, "readAt underflow " + size + " vs " + validDataSize);
                        if (cache == null || totalBufferSize <= 0)
                            return -1;
                        try {
                            Thread.sleep(5);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }

                    ret = size;
                    System.arraycopy(cache, readPtr + (int)position, buffer, offset, ret);
                }
            }

            return ret;
        };

        public long getSize() {
            Log.d(TAG, "getSize");
            return -1;
        };

        public void close() {
            Log.d(TAG, "close");
            totalBufferSize = 0;
            cache = null;
            writePtr = 0;
            readPtr = 0;
            validDataSize = 0;
            validBufferSize = totalBufferSize;
            isStart = false;
            isCache = false;
        };
    }

////////////////////////////////////////////////////////////////////////////////
    private class CodecState {
        private static final String TAG = "CodecState";

        private WFDSinkView mView = null;
        private MediaExtractor mExtractor = null;
        private int mTrackIndex = -1;
        private MediaFormat mFormat = null;
        private boolean mSawInputEOS = false, mSawOutputEOS = false;

        private MediaCodec mCodec = null;
        private MediaFormat mOutputFormat = null;
        private ByteBuffer[] mCodecInputBuffers;
        private ByteBuffer[] mCodecOutputBuffers;

        private LinkedList<Integer> mAvailableInputBufferIndices;
        private LinkedList<Integer> mAvailableOutputBufferIndices;
        private LinkedList<MediaCodec.BufferInfo> mAvailableOutputBufferInfos;

        private NonBlockingAudioTrack mAudioTrack = null;

        private long mLastMediaTimeUs = -1;

        private class FeedInputBufferRunnable implements Runnable {
            private static final int Runnable_START = 0;
            private static final int Runnable_STOPPING = 1;
            private static final int Runnable_STOPPED = 2;
            private static final int Runnable_PAUSING = 3;
            private static final int Runnable_PAUSED = 4;

            private volatile int status = Runnable_START;

            public void run() {
                try {
                    while (true) {
                        int index = MediaCodec.INFO_TRY_AGAIN_LATER;

                        if (status == Runnable_START) {
                            index = mCodec.dequeueInputBuffer(0 /* timeoutUs */);

                            if (index != MediaCodec.INFO_TRY_AGAIN_LATER) {
                                mAvailableInputBufferIndices.add(new Integer(index));
                            }

                            feedInputBuffer();
                        } else if (status == Runnable_STOPPING) {
                            status = Runnable_STOPPED;
                            break;
                        } else if (status == Runnable_PAUSING) {
                            status = Runnable_PAUSED;
                        }

                        if (index == MediaCodec.INFO_TRY_AGAIN_LATER) {
                            try {
                                Thread.sleep(5);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                } catch (IllegalStateException e) {
                    e.printStackTrace();
                } finally {
                    status = Runnable_STOPPED;
                    Log.d(TAG, "FeedInputBufferRunnable exit");
                }
            }

            public void stop() {
                if (status != Runnable_STOPPED) status = Runnable_STOPPING;
            }

            public boolean isStopped() {
                return status == Runnable_STOPPED;
            }

            public void pause() {
                if (status == Runnable_START) status = Runnable_PAUSING;
            }

            public boolean isPaused() {
                return status == Runnable_PAUSED;
            }

            public void resume() {
                if (status == Runnable_PAUSED) status = Runnable_START;
            }
        }
        private FeedInputBufferRunnable mFeedInputBufferRunnable = new FeedInputBufferRunnable();
        private Thread mFeedInputBufferThread = new Thread(mFeedInputBufferRunnable, "FeedInputBufferThread");

        private class DrainOutputBufferRunnable implements Runnable {
            private static final int Runnable_START = 0;
            private static final int Runnable_STOPPING = 1;
            private static final int Runnable_STOPPED = 2;
            private static final int Runnable_PAUSING = 3;
            private static final int Runnable_PAUSED = 4;

            private volatile int status = Runnable_START;

            public void run() {
                try {
                    while (true) {
                        int index = MediaCodec.INFO_TRY_AGAIN_LATER;

                        if (status == Runnable_START) {
                            MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
                            index = mCodec.dequeueOutputBuffer(info, 0 /* timeoutUs */);

                            if (index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                                mOutputFormat = mCodec.getOutputFormat();
                                onOutputFormatChanged();
                            } else if (index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                                mCodecOutputBuffers = mCodec.getOutputBuffers();
                            } else if (index != MediaCodec.INFO_TRY_AGAIN_LATER) {
                                mAvailableOutputBufferIndices.add(new Integer(index));
                                mAvailableOutputBufferInfos.add(info);
                            }

                            drainOutputBuffer();
                        } else if (status == Runnable_STOPPING) {
                            status = Runnable_STOPPED;
                            break;
                        } else if (status == Runnable_PAUSING) {
                            status = Runnable_PAUSED;
                        }

                        if (index == MediaCodec.INFO_TRY_AGAIN_LATER) {
                            try {
                                Thread.sleep(5);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                } catch (IllegalStateException e) {
                    e.printStackTrace();
                } finally {
                    status = Runnable_STOPPED;
                    Log.d(TAG, "DrainOutputBufferRunnable exit");
                }
            }

            public void stop() {
                if (status != Runnable_STOPPED) status = Runnable_STOPPING;
            }

            public boolean isStopped() {
                return status == Runnable_STOPPED;
            }

            public void pause() {
                if (status == Runnable_START) status = Runnable_PAUSING;
            }

            public boolean isPaused() {
                return status == Runnable_PAUSED;
            }

            public void resume() {
                if (status == Runnable_PAUSED) status = Runnable_START;
            }
        }
        private DrainOutputBufferRunnable mDrainOutputBufferRunnable = new DrainOutputBufferRunnable();
        private Thread mDrainOutputBufferThread = new Thread(mDrainOutputBufferRunnable, "DrainOutputBufferThread");

        public CodecState(
                WFDSinkView view,
                MediaExtractor extractor,
                int trackIndex,
                MediaFormat format,
                MediaCodec codec) {
            mView = view;
            mExtractor = extractor;
            mTrackIndex = trackIndex;
            mFormat = format;
            mSawInputEOS = mSawOutputEOS = false;

            mCodec = codec;

            mCodec.start();
            mCodecInputBuffers = mCodec.getInputBuffers();
            mCodecOutputBuffers = mCodec.getOutputBuffers();

            mAvailableInputBufferIndices = new LinkedList();
            mAvailableOutputBufferIndices = new LinkedList();
            mAvailableOutputBufferInfos = new LinkedList();

            mLastMediaTimeUs = 0;
        }

        public void release() {
            Log.d(TAG, "release");

            mFeedInputBufferRunnable.stop();
            mDrainOutputBufferRunnable.stop();

            mCodec.stop();

            while (!mFeedInputBufferRunnable.isStopped() || !mDrainOutputBufferRunnable.isStopped()) {}

            try {
                mCodecInputBuffers = null;
                mCodecOutputBuffers = null;
                mOutputFormat = null;

                mAvailableOutputBufferInfos = null;
                mAvailableOutputBufferIndices = null;
                mAvailableInputBufferIndices = null;

                mCodec.release();
                mCodec = null;

                if (mAudioTrack != null) {
                    mAudioTrack.release();
                    mAudioTrack = null;
                }
            } finally {
            }
        }

        public void start() {
            if (mAudioTrack != null) {
                mAudioTrack.play();
            }

            mFeedInputBufferRunnable.resume();
            mFeedInputBufferThread.start();
            mFeedInputBufferRunnable.resume();
            mDrainOutputBufferThread.start();
        }

        public void pause() {
            if (mAudioTrack != null) {
                mAudioTrack.pause();
            }

            mFeedInputBufferRunnable.pause();
            mDrainOutputBufferRunnable.pause();
        }

        public long getCurrentPositionUs() {
            return mLastMediaTimeUs;
        }

        public void flush() {
            Log.d(TAG, "flush");

            mFeedInputBufferRunnable.pause();
            mDrainOutputBufferRunnable.pause();

            mCodec.flush();

            while ((!mFeedInputBufferRunnable.isPaused() && !mFeedInputBufferRunnable.isStopped())
                    || (!mDrainOutputBufferRunnable.isPaused() && !mDrainOutputBufferRunnable.isStopped())) {}

            try {
                mAvailableInputBufferIndices.clear();
                mAvailableOutputBufferIndices.clear();
                mAvailableOutputBufferInfos.clear();

                mSawInputEOS = false;
                mSawOutputEOS = false;

                if (mAudioTrack != null
                        && mAudioTrack.getPlayState() == AudioTrack.PLAYSTATE_STOPPED) {
                    mAudioTrack.play();
                }
            } finally {
            }

            mFeedInputBufferRunnable.resume();
            mFeedInputBufferRunnable.resume();
        }

        public boolean doSomeWork() { // TODO
            return mSawInputEOS;
        }

        /** returns true if more input data could be fed */
        private boolean feedInputBuffer() {
            if (mSawInputEOS || mAvailableInputBufferIndices.isEmpty()) {
                return false;
            }

            int index = mAvailableInputBufferIndices.peekFirst().intValue();

            ByteBuffer codecData = mCodecInputBuffers[index];

            int trackIndex = mExtractor.getSampleTrackIndex();

            if (trackIndex == mTrackIndex) {
                int sampleSize =
                    mExtractor.readSampleData(codecData, 0 /* offset */);

                long sampleTime = mExtractor.getSampleTime();

                int sampleFlags = mExtractor.getSampleFlags();

                mCodec.queueInputBuffer(
                        index, 0 /* offset */, sampleSize, sampleTime,
                        0 /* flags */);

                mAvailableInputBufferIndices.removeFirst();
                mExtractor.advance();

                return true;
            } else if (trackIndex < 0) {
                Log.d(TAG, "saw input EOS on track " + mTrackIndex);

                mSawInputEOS = true;

                mCodec.queueInputBuffer(
                        index, 0 /* offset */, 0 /* sampleSize */,
                        0 /* sampleTime */, MediaCodec.BUFFER_FLAG_END_OF_STREAM);

                mAvailableInputBufferIndices.removeFirst();
            } else { // just for debug single a/v
                //mExtractor.advance();
            }

            return false;
        }

        private void onOutputFormatChanged() {
            String mime = mOutputFormat.getString(MediaFormat.KEY_MIME);

            if (mime.startsWith("audio/")) {
                int sampleRate =
                    mOutputFormat.getInteger(MediaFormat.KEY_SAMPLE_RATE);

                int channelCount =
                    mOutputFormat.getInteger(MediaFormat.KEY_CHANNEL_COUNT);

                mAudioTrack = new NonBlockingAudioTrack(sampleRate, channelCount);
                mAudioTrack.play();
            }
        }

        /** returns true if more output data could be drained */
        private boolean drainOutputBuffer() {
            if (mSawOutputEOS || mAvailableOutputBufferIndices.isEmpty()) {
                return false;
            }

            int index = mAvailableOutputBufferIndices.peekFirst().intValue();
            MediaCodec.BufferInfo info = mAvailableOutputBufferInfos.peekFirst();

            if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                Log.d(TAG, "saw output EOS on track " + mTrackIndex);

                mSawOutputEOS = true;

                if (mAudioTrack != null) {
                    mAudioTrack.stop();
                }
                return false;
            }

            long realTimeUs =
                mView.getRealTimeUsForMediaTime(info.presentationTimeUs);

            long nowUs = mView.getNowUs();

            long lateUs = nowUs - realTimeUs;

            if (mAudioTrack != null) {
                ByteBuffer buffer = mCodecOutputBuffers[index];
                buffer.clear();
                buffer.position(0 /* offset */);

                byte[] audioCopy = new byte[info.size];
                buffer.get(audioCopy, 0, info.size);

                mAudioTrack.write(audioCopy, info.size);

                mCodec.releaseOutputBuffer(index, false /* render */);

                mLastMediaTimeUs = info.presentationTimeUs;

                mAvailableOutputBufferIndices.removeFirst();
                mAvailableOutputBufferInfos.removeFirst();
                return true;
            } else {
                // video
                boolean render;

                if (false) { // do not sync a/v
                    if (lateUs < -10000) {
                        // too early;
                        Log.d(TAG, "video late by " + lateUs + " us.");
                        return false;
                    } else if (lateUs > 30000) {
                        Log.d(TAG, "video late by " + lateUs + " us.");
                        render = false;
                    } else {
                        render = true;
                        mLastMediaTimeUs = info.presentationTimeUs;
                    }
                } else {
                    render = true;
                    mLastMediaTimeUs = info.presentationTimeUs;
                }

                mCodec.releaseOutputBuffer(index, render);

                mAvailableOutputBufferIndices.removeFirst();
                mAvailableOutputBufferInfos.removeFirst();
                return true;
            }
        }

        public long getAudioTimeUs() {
            if (mAudioTrack == null) {
                return 0;
            }

            return mAudioTrack.getAudioTimeUs();
        }
    }

////////////////////////////////////////////////////////////////////////////////
    private class NonBlockingAudioTrack {
        private static final String TAG = "NonBlockingAudioTrack";

        private AudioTrack mAudioTrack;
        private int mSampleRate;
        private int mFrameSize;
        private int mBufferSizeInFrames;
        private int mNumFramesSubmitted = 0;

        class QueueElem {
            byte[] data;
            int offset;
            int size;
        }
        private Queue<QueueElem> mQueue = new ConcurrentLinkedQueue<QueueElem>();

        private class WriteAudioTrackRunnable implements Runnable {
            private static final int Runnable_START = 0;
            private static final int Runnable_STOPPING = 1;
            private static final int Runnable_STOPPED = 2;
            private static final int Runnable_PAUSING = 3;
            private static final int Runnable_PAUSED = 4;

            private volatile int status = Runnable_START;

            public void run() {
                while (true) {
                    if (status == Runnable_START) {
                        writeMore();
                    } else if (status == Runnable_STOPPING) {
                        status = Runnable_STOPPED;
                        break;
                    } else if (status == Runnable_PAUSING) {
                        status = Runnable_PAUSED;
                    }
                }
                status = Runnable_STOPPED;
                Log.d(TAG, "WriteAudioTrackRunnable exit");
            }

            public void stop() {
                if (status != Runnable_STOPPED) status = Runnable_STOPPING;
            }

            public boolean isStopped() {
                return status == Runnable_STOPPED;
            }

            public void pause() {
                if (status == Runnable_START) status = Runnable_PAUSING;
            }

            public boolean isPaused() {
                return status == Runnable_PAUSED;
            }

            public void resume() {
                if (status == Runnable_PAUSED) status = Runnable_START;
            }
        }
        private WriteAudioTrackRunnable mWriteAudioTrackRunnable = new WriteAudioTrackRunnable();
        private Thread mWriteAudioTrackThread = new Thread(mWriteAudioTrackRunnable, "WriteAudioTrackThread");

        public NonBlockingAudioTrack(int sampleRate, int channelCount) {
            int channelConfig;
            switch (channelCount) {
                case 1:
                    channelConfig = AudioFormat.CHANNEL_OUT_MONO;
                    break;
                case 2:
                    channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
                    break;
                case 6:
                    channelConfig = AudioFormat.CHANNEL_OUT_5POINT1;
                    break;
                default:
                    throw new IllegalArgumentException();
            }

            int minBufferSize =
                AudioTrack.getMinBufferSize(
                        sampleRate,
                        channelConfig,
                        AudioFormat.ENCODING_PCM_16BIT);

            int bufferSize = 2 * minBufferSize;

            mAudioTrack = new AudioTrack(
                    AudioManager.STREAM_MUSIC,
                    sampleRate,
                    channelConfig,
                    AudioFormat.ENCODING_PCM_16BIT,
                    bufferSize,
                    AudioTrack.MODE_STREAM);

            mSampleRate = sampleRate;
            mFrameSize = 2 * channelCount;
            mBufferSizeInFrames = bufferSize / mFrameSize;

            mWriteAudioTrackThread.start();
        }

        public long getAudioTimeUs() {
            int numFramesPlayed = mAudioTrack.getPlaybackHeadPosition();

            return (numFramesPlayed * 1000000L) / mSampleRate;
        }

        public void play() {
            mAudioTrack.play();
        }

        public void stop() {
            mWriteAudioTrackRunnable.stop();
            while (!mWriteAudioTrackRunnable.isStopped()) {}

            mAudioTrack.stop();

            mNumFramesSubmitted = 0;
        }

        public void pause() {
            mWriteAudioTrackRunnable.pause();
            while (!mWriteAudioTrackRunnable.isPaused() && !mWriteAudioTrackRunnable.isStopped()) {}

            mAudioTrack.pause();
        }

        public void release() {
            mWriteAudioTrackRunnable.stop();
            while (!mWriteAudioTrackRunnable.isStopped()) {}

            mAudioTrack.release();
            mAudioTrack = null;
        }

        public int getPlayState() {
            return mAudioTrack.getPlayState();
        }

        private void writeMore() {
            if (mQueue.isEmpty()) {
                return;
            }

            int numFramesPlayed = mAudioTrack.getPlaybackHeadPosition();
            int numFramesPending = mNumFramesSubmitted - numFramesPlayed;
            int numFramesAvailableToWrite = mBufferSizeInFrames - numFramesPending;
            int numBytesAvailableToWrite = numFramesAvailableToWrite * mFrameSize;

            while (numBytesAvailableToWrite > 0) {
                QueueElem elem = mQueue.peek();

                int numBytes = elem.size;
                if (numBytes > numBytesAvailableToWrite) {
                    numBytes = numBytesAvailableToWrite;
                }

                int written = mAudioTrack.write(elem.data, elem.offset, numBytes);
                assert(written == numBytes);

                mNumFramesSubmitted += written / mFrameSize;

                elem.size -= numBytes;
                if (elem.size == 0) {
                    mQueue.poll();

                    if (mQueue.isEmpty()) {
                        break;
                    }
                } else {
                    elem.offset += numBytes;
                    break;
                }

                numBytesAvailableToWrite -= numBytes;
            }
        }

        public void write(byte[] data, int size) {
            QueueElem elem = new QueueElem();
            elem.data = data;
            elem.offset = 0;
            elem.size = size;

            mQueue.add(elem);
        }
    }

////////////////////////////////////////////////////////////////////////////////
}

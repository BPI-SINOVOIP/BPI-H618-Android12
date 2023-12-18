package com.android.tv.samples.sampletunertvinput;

import static android.media.tv.TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN;

import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaCodec.BufferInfo;
import android.media.MediaFormat;
import android.media.tv.tuner.dvr.DvrPlayback;
import android.media.tv.tuner.dvr.DvrSettings;
import android.media.tv.tuner.filter.AvSettings;
import android.media.tv.tuner.filter.Filter;
import android.media.tv.tuner.filter.FilterCallback;
import android.media.tv.tuner.filter.FilterEvent;
import android.media.tv.tuner.filter.MediaEvent;
import android.media.tv.tuner.filter.TsFilterConfiguration;
import android.media.tv.tuner.frontend.AtscFrontendSettings;
import android.media.tv.tuner.frontend.DvbtFrontendSettings;
import android.media.tv.tuner.frontend.FrontendSettings;
import android.media.tv.tuner.frontend.OnTuneEventListener;
import android.media.tv.tuner.Tuner;
import android.media.tv.TvInputService;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerExecutor;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.Surface;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;


/** SampleTunerTvInputService */
public class SampleTunerTvInputService extends TvInputService {
    private static final String TAG = "SampleTunerTvInput";
    private static final boolean DEBUG = true;

    private static final int AUDIO_TPID = 257;
    private static final int VIDEO_TPID = 256;
    private static final int STATUS_MASK = 0xf;
    private static final int LOW_THRESHOLD = 0x1000;
    private static final int HIGH_THRESHOLD = 0x07fff;
    private static final int FREQUENCY = 578000;
    private static final int FILTER_BUFFER_SIZE = 16000000;
    private static final int DVR_BUFFER_SIZE = 4000000;
    private static final int INPUT_FILE_MAX_SIZE = 700000;
    private static final int PACKET_SIZE = 188;

    private static final int TIMEOUT_US = 100000;
    private static final boolean SAVE_DATA = true;
    private static final String ES_PATH = "/data/local/tmp/test.es";
    private static final MediaFormat VIDEO_FORMAT;

    static {
        // format extracted for the specific input file
        VIDEO_FORMAT = MediaFormat.createVideoFormat("video/avc", 320, 240);
        VIDEO_FORMAT.setInteger(MediaFormat.KEY_TRACK_ID, 1);
        VIDEO_FORMAT.setLong(MediaFormat.KEY_DURATION, 9933333);
        VIDEO_FORMAT.setInteger(MediaFormat.KEY_LEVEL, 32);
        VIDEO_FORMAT.setInteger(MediaFormat.KEY_PROFILE, 65536);
        ByteBuffer csd = ByteBuffer.wrap(
                new byte[] {0, 0, 0, 1, 103, 66, -64, 20, -38, 5, 7, -24, 64, 0, 0, 3, 0, 64, 0,
                        0, 15, 35, -59, 10, -88});
        VIDEO_FORMAT.setByteBuffer("csd-0", csd);
        csd = ByteBuffer.wrap(new byte[] {0, 0, 0, 1, 104, -50, 60, -128});
        VIDEO_FORMAT.setByteBuffer("csd-1", csd);
    }

    public static final String INPUT_ID =
            "com.android.tv.samples.sampletunertvinput/.SampleTunerTvInputService";
    private String mSessionId;

    @Override
    public TvInputSessionImpl onCreateSession(String inputId, String sessionId) {
        TvInputSessionImpl session =  new TvInputSessionImpl(this);
        if (DEBUG) {
            Log.d(TAG, "onCreateSession(inputId=" + inputId + ", sessionId=" + sessionId + ")");
        }
        mSessionId = sessionId;
        return session;
    }

    @Override
    public TvInputSessionImpl onCreateSession(String inputId) {
        return new TvInputSessionImpl(this);
    }

    class TvInputSessionImpl extends Session {

        private final Context mContext;
        private Handler mHandler;

        private Surface mSurface;
        private Filter mAudioFilter;
        private Filter mVideoFilter;
        private DvrPlayback mDvr;
        private Tuner mTuner;
        private MediaCodec mMediaCodec;
        private Thread mDecoderThread;
        private Deque<MediaEvent> mDataQueue;
        private List<MediaEvent> mSavedData;
        private boolean mDataReady = false;


        public TvInputSessionImpl(Context context) {
            super(context);
            mContext = context;
        }

        @Override
        public void onRelease() {
            if (DEBUG) {
                Log.d(TAG, "onRelease");
            }
            if (mDecoderThread != null) {
                mDecoderThread.interrupt();
                mDecoderThread = null;
            }
            if (mMediaCodec != null) {
                mMediaCodec.release();
                mMediaCodec = null;
            }
            if (mAudioFilter != null) {
                mAudioFilter.close();
            }
            if (mVideoFilter != null) {
                mVideoFilter.close();
            }
            if (mDvr != null) {
                mDvr.close();
            }
            if (mTuner != null) {
                mTuner.close();
            }
            mDataQueue = null;
            mSavedData = null;
        }

        @Override
        public boolean onSetSurface(Surface surface) {
            if (DEBUG) {
                Log.d(TAG, "onSetSurface");
            }
            this.mSurface = surface;
            return true;
        }

        @Override
        public void onSetStreamVolume(float v) {
            if (DEBUG) {
                Log.d(TAG, "onSetStreamVolume " + v);
            }
        }

        @Override
        public boolean onTune(Uri uri) {
            if (DEBUG) {
                Log.d(TAG, "onTune " + uri);
            }
            if (!initCodec()) {
                Log.e(TAG, "null codec!");
                return false;
            }
            mHandler = new Handler();
            mDecoderThread =
                    new Thread(
                            this::decodeInternal,
                            "sample-tuner-tis-decoder-thread");
            mDecoderThread.start();
            return true;
        }

        @Override
        public void onSetCaptionEnabled(boolean b) {
            if (DEBUG) {
                Log.d(TAG, "onSetCaptionEnabled " + b);
            }
        }

        private Filter audioFilter() {
            Filter audioFilter = mTuner.openFilter(Filter.TYPE_TS, Filter.SUBTYPE_AUDIO,
                    FILTER_BUFFER_SIZE, new HandlerExecutor(mHandler),
                    new FilterCallback() {
                        @Override
                        public void onFilterEvent(Filter filter, FilterEvent[] events) {
                            if (DEBUG) {
                                Log.d(TAG, "onFilterEvent audio, size=" + events.length);
                            }
                            for (int i = 0; i < events.length; i++) {
                                if (DEBUG) {
                                    Log.d(TAG, "events[" + i + "] is "
                                            + events[i].getClass().getSimpleName());
                                }
                            }
                        }

                        @Override
                        public void onFilterStatusChanged(Filter filter, int status) {
                            if (DEBUG) {
                                Log.d(TAG, "onFilterEvent audio, status=" + status);
                            }
                        }
                    });
            AvSettings settings =
                    AvSettings.builder(Filter.TYPE_TS, true).setPassthrough(false).build();
            audioFilter.configure(
                    TsFilterConfiguration.builder().setTpid(AUDIO_TPID)
                            .setSettings(settings).build());
            return audioFilter;
        }

        private Filter videoFilter() {
            Filter videoFilter = mTuner.openFilter(Filter.TYPE_TS, Filter.SUBTYPE_VIDEO,
                    FILTER_BUFFER_SIZE, new HandlerExecutor(mHandler),
                    new FilterCallback() {
                        @Override
                        public void onFilterEvent(Filter filter, FilterEvent[] events) {
                            if (DEBUG) {
                                Log.d(TAG, "onFilterEvent video, size=" + events.length);
                            }
                            for (int i = 0; i < events.length; i++) {
                                if (DEBUG) {
                                    Log.d(TAG, "events[" + i + "] is "
                                            + events[i].getClass().getSimpleName());
                                }
                                if (events[i] instanceof MediaEvent) {
                                    MediaEvent me = (MediaEvent) events[i];
                                    mDataQueue.add(me);
                                    if (SAVE_DATA) {
                                        mSavedData.add(me);
                                    }
                                }
                            }
                        }

                        @Override
                        public void onFilterStatusChanged(Filter filter, int status) {
                            if (DEBUG) {
                                Log.d(TAG, "onFilterEvent video, status=" + status);
                                if (status == Filter.STATUS_DATA_READY) {
                                    mDataReady = true;
                                }
                            }
                        }
                    });
            AvSettings settings =
                    AvSettings.builder(Filter.TYPE_TS, false).setPassthrough(false).build();
            videoFilter.configure(
                    TsFilterConfiguration.builder().setTpid(VIDEO_TPID)
                            .setSettings(settings).build());
            return videoFilter;
        }

        private DvrPlayback dvrPlayback() {
            DvrPlayback dvr = mTuner.openDvrPlayback(DVR_BUFFER_SIZE, new HandlerExecutor(mHandler),
                    status -> {
                        if (DEBUG) {
                            Log.d(TAG, "onPlaybackStatusChanged status=" + status);
                        }
                    });
            int res = dvr.configure(
                    DvrSettings.builder()
                            .setStatusMask(STATUS_MASK)
                            .setLowThreshold(LOW_THRESHOLD)
                            .setHighThreshold(HIGH_THRESHOLD)
                            .setDataFormat(DvrSettings.DATA_FORMAT_ES)
                            .setPacketSize(PACKET_SIZE)
                            .build());
            if (DEBUG) {
                Log.d(TAG, "config res=" + res);
            }
            File file = new File(ES_PATH);
            if (file.exists()) {
                try {
                    dvr.setFileDescriptor(
                            ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE));
                } catch (FileNotFoundException e) {
                        Log.e(TAG, "Failed to create FD");
                }
            } else {
                Log.w(TAG, "File not existing");
            }
            return dvr;
        }

        private void tune() {
            DvbtFrontendSettings feSettings = DvbtFrontendSettings.builder()
                    .setFrequency(FREQUENCY)
                    .setTransmissionMode(DvbtFrontendSettings.TRANSMISSION_MODE_AUTO)
                    .setBandwidth(DvbtFrontendSettings.BANDWIDTH_8MHZ)
                    .setConstellation(DvbtFrontendSettings.CONSTELLATION_AUTO)
                    .setHierarchy(DvbtFrontendSettings.HIERARCHY_AUTO)
                    .setHighPriorityCodeRate(DvbtFrontendSettings.CODERATE_AUTO)
                    .setLowPriorityCodeRate(DvbtFrontendSettings.CODERATE_AUTO)
                    .setGuardInterval(DvbtFrontendSettings.GUARD_INTERVAL_AUTO)
                    .setHighPriority(true)
                    .setStandard(DvbtFrontendSettings.STANDARD_T)
                    .build();
            mTuner.setOnTuneEventListener(new HandlerExecutor(mHandler), new OnTuneEventListener() {
                @Override
                public void onTuneEvent(int tuneEvent) {
                    if (DEBUG) {
                        Log.d(TAG, "onTuneEvent " + tuneEvent);
                    }
                    long read = mDvr.read(INPUT_FILE_MAX_SIZE);
                    if (DEBUG) {
                        Log.d(TAG, "read=" + read);
                    }
                }
            });
            mTuner.tune(feSettings);
        }

        private boolean initCodec() {
            if (mMediaCodec != null) {
                mMediaCodec.release();
                mMediaCodec = null;
            }
            try {
                mMediaCodec = MediaCodec.createDecoderByType("video/avc");
                mMediaCodec.configure(VIDEO_FORMAT, mSurface, null, 0);
            } catch (IOException e) {
                Log.e(TAG, "Error: " + e.getMessage());
            }

            if (mMediaCodec == null) {
                Log.e(TAG, "null codec!");
                notifyVideoUnavailable(VIDEO_UNAVAILABLE_REASON_UNKNOWN);
                return false;
            }
            return true;
        }

        private void decodeInternal() {
            mDataQueue = new ArrayDeque<>();
            mSavedData = new ArrayList<>();
            mTuner = new Tuner(mContext, mSessionId,
                    TvInputService.PRIORITY_HINT_USE_CASE_TYPE_LIVE);

            mAudioFilter = audioFilter();
            mVideoFilter = videoFilter();
            mAudioFilter.start();
            mVideoFilter.start();
            // use dvr playback to feed the data on platform without physical tuner
            mDvr = dvrPlayback();
            tune();
            mDvr.start();
            mMediaCodec.start();

            try {
                while (!Thread.interrupted()) {
                    if (!mDataReady) {
                        Thread.sleep(100);
                    }
                    if (!mDataQueue.isEmpty()) {
                        if (queueCodecInputBuffer(mDataQueue.getFirst())) {
                            // data consumed, remove.
                            mDataQueue.pollFirst();
                        }
                    } else if (SAVE_DATA) {
                        mDataQueue.addAll(mSavedData);
                    }
                    releaseCodecOutputBuffer();
                }
            } catch (Exception e) {
                Log.e(TAG, "Error: " + e.getMessage());
            }
        }

        private boolean queueCodecInputBuffer(MediaEvent mediaEvent) {
            int res = mMediaCodec.dequeueInputBuffer(TIMEOUT_US);
            if (res >= 0) {
                ByteBuffer buffer = mMediaCodec.getInputBuffer(res);
                if (buffer == null) {
                    throw new RuntimeException("Null decoder input buffer");
                }

                ByteBuffer data = mediaEvent.getLinearBlock().map();
                int sampleSize = (int) mediaEvent.getDataLength();
                int offset = (int) mediaEvent.getOffset();
                long pts = mediaEvent.getPts();

                if (offset > 0 && offset < data.limit()) {
                    data.position(offset);
                } else {
                    data.position(0);
                }

                if (DEBUG) {
                    Log.d(
                        TAG,
                        "Decoder: Send data to decoder."
                            + " Sample size="
                            + sampleSize
                            + " pts="
                            + pts
                            + " limit="
                            + data.limit()
                            + " pos="
                            + data.position()
                            + " size="
                            + (data.limit() - data.position()));
                }
                while (data.position() < data.limit()) {
                    // fill codec input buffer
                    buffer.put(data.get());
                }

                mMediaCodec.queueInputBuffer(res, 0, sampleSize, pts, 0);
            } else {
                Log.d(TAG, "queueCodecInputBuffer res=" + res);
                return false;
            }
            return true;
        }

        private void releaseCodecOutputBuffer() {
            // play frames
            BufferInfo bufferInfo = new BufferInfo();
            int res = mMediaCodec.dequeueOutputBuffer(bufferInfo, TIMEOUT_US);
            if (res >= 0) {
                mMediaCodec.releaseOutputBuffer(res, true);
                notifyVideoAvailable();
                if (DEBUG) {
                    Log.d(TAG, "notifyVideoAvailable");
                }
            } else if (res == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                MediaFormat format = mMediaCodec.getOutputFormat();
                if (DEBUG) {
                    Log.d(TAG, "releaseCodecOutputBuffer: Output format changed:" + format);
                }
            } else if (res == MediaCodec.INFO_TRY_AGAIN_LATER) {
                if (DEBUG) {
                    Log.d(TAG, "releaseCodecOutputBuffer: timeout");
                }
            } else {
                if (DEBUG) {
                    Log.d(TAG, "Return value of releaseCodecOutputBuffer:" + res);
                }
            }
        }

    }
}
/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.softwinner.TvdVideo;

import android.app.Dialog;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.media.AudioManager;
import android.media.MediaFormat;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnInfoListener;
import android.media.MediaPlayer.OnTimedTextListener;
import android.media.MediaPlayer.TrackInfo;
import android.media.Metadata;
import android.media.SubtitleController;
import android.media.SubtitleTrack.RenderingWidget;
import android.media.TimedText;
import android.media.WebVttRenderer;
import android.net.Uri;
import android.os.Looper;
import android.os.Parcel;
import android.os.RemoteException;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Display;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.media.AudioManager;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Map;
import java.util.Vector;
import android.os.SystemProperties;
import com.softwinner.TvdVideo.MediaController.MediaPlayerControl;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

/**
 * Displays a video file. The VideoView class can load images from various
 * sources (such as resources or content providers), takes care of computing its
 * measurement from the video so that it can be used in any layout manager, and
 * provides various display options such as scaling and tinting.
 */
public class VideoView extends SurfaceView implements MediaPlayerControl,
                SubtitleController.Anchor {
    private static final String TAG = "videoview";
    private static final boolean TRACE = DebugConfigs.VERBOSE_INFO;
    private AudioManager.OnAudioFocusChangeListener afChangeListener;
    private AudioManager mAm;
    private static VideoView mInstance = null;

    // settable by the client
    private Uri mUri;
    private Map<String, String> mHeaders;
    private int mDuration;
    private int mZoomMode = 0;

    // all video zoom mode
    private static final int ZOOM_FULL_SCREEN_VIDEO_RATIO = 0;
    private static final int ZOOM_FULL_SCREEN_SCREEN_RATIO = 1;
    private static final int ZOOM_ORIGIN_SIZE = 2;
    private static final int ZOOM_4R3 = 3;
    private static final int ZOOM_16R9 = 4;
    private static final int ZOOM_PIP = 5;

    // all possible internal states
    private static final int STATE_ERROR = -1;
    private static final int STATE_IDLE = 0;
    private static final int STATE_PREPARING = 1;
    private static final int STATE_PREPARED = 2;
    private static final int STATE_PLAYING = 3;
    private static final int STATE_PAUSED = 4;
    private static final int STATE_PLAYBACK_COMPLETED = 5;

    // mCurrentState is a VideoView object's current state.
    // mTargetState is the state that a method caller intends to reach.
    // For instance, regardless the VideoView object's current state,
    // calling pause() intends to bring the object to a target state
    // of STATE_PAUSED.
    private int mCurrentState = STATE_IDLE;
    private int mTargetState = STATE_IDLE;

    // All the stuff we need for playing and showing a video
    private SurfaceHolder mSurfaceHolder = null;
    private MediaPlayer mMediaPlayer = null;
    private Context mContext;
    private boolean mIsShowDisplayController = false;
    private int mAudioSession;
    private int mVideoWidth;
    private int mVideoHeight;
    private int mSurfaceWidth;
    private int mSurfaceHeight;
    private MediaController mMediaController;
    private OnCompletionListener mOnCompletionListener;
    private MediaPlayer.OnPreparedListener mOnPreparedListener;
    private int mCurrentBufferPercentage;
    private OnErrorListener mOnErrorListener;
    private OnInfoListener mOnInfoListener;
    private int mSeekWhenPrepared; // recording the seek position while
                                   // preparing
    private boolean mCanPause;
    private boolean mCanSeekBack;
    private boolean mCanSeekForward;
    private int mStateWhenSuspended; // state before calling suspend()
    private boolean mBDFolderPlayMode = false;
    private boolean mIsPreparing = false;

    Dialog mErrorDialog;
    IDisplayController mDisplayController;
    /** Subtitle rendering widget overlaid on top of the video. */
    private RenderingWidget mSubtitleWidget;

    /** Listener for changes to subtitle data, used to redraw when needed. */
    private RenderingWidget.OnChangedListener mSubtitlesChangedListener;

    private ArrayList<String> mSrtList = new ArrayList<String>();
    private ArrayList<String> mMediaTypeList = new ArrayList<String>();
    private ArrayList<TrackInfo> mTrackInfosAudio = new ArrayList<TrackInfo>();
    private ArrayList<TrackInfo> mTrackInfosVideo = new ArrayList<TrackInfo>();
    private ArrayList<TrackInfo> mTrackInfosSub = new ArrayList<TrackInfo>();
    private int mTrackSubCur = -1;
    private int mTrackVideoCur = -1;
    private int mTrackAudioCur = -1;
    private Vector<Integer> mTrackSubIndex = new Vector<Integer>();
    private Vector<Integer> mTrackAudioIndex = new Vector<Integer>();
    private Vector<Integer> mTrackVideoIndex = new Vector<Integer>();

    public VideoView(Context context) {
        super(context);
        mInstance = this;
        mContext = context;
        initVideoView();
    }

    public VideoView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
        mInstance = this;
        mContext = context;
        initVideoView();
    }

    public VideoView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mInstance = this;
        mContext = context;
        initVideoView();
    }

    public static VideoView getInstance() {
           return mInstance;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        if(TRACE)Log.v(TAG, "onMeasure widthMeasureSpec="+widthMeasureSpec+",heightMeasureSpec="+heightMeasureSpec);
        int width = getDefaultSize(mVideoWidth, widthMeasureSpec);
        int height = getDefaultSize(mVideoHeight, heightMeasureSpec);
        if(TRACE)Log.v(TAG, "onMeasure width="+width+",height="+height+",videoW="+mVideoWidth+",videoH="+mVideoHeight);
        if (mVideoWidth > 0 && mVideoHeight > 0) {
            setvideoSize(width, height);
        } else {
            setMeasuredDimension(width, height);
        }
    }

    private void setvideoSize(int width, int height) {
        if(TRACE)Log.v(TAG, "setvideoSize width="+width+",height="+height+",mode="+mZoomMode);
        switch (mZoomMode) {
            case ZOOM_FULL_SCREEN_VIDEO_RATIO:
                if (mVideoWidth * height > width * mVideoHeight) {
                    height = width * mVideoHeight / mVideoWidth;
                } else if (mVideoWidth * height < width * mVideoHeight) {
                    width = height * mVideoWidth / mVideoHeight;
                }
                break;
            case ZOOM_FULL_SCREEN_SCREEN_RATIO:
                break;
            case ZOOM_ORIGIN_SIZE:
                if (width > mVideoWidth) {
                    width = mVideoWidth;
                }
                if (height > mVideoHeight) {
                    height = mVideoHeight;
                }

                break;
            case ZOOM_4R3:
                if (width * 3 > 4 * height) {
                    width = height * 4 / 3;
                } else {
                    height = width * 3 / 4;
                }
                break;
            case ZOOM_16R9:
                if (width * 9 > 16 * height) {
                    width = height * 16 / 9;
                } else {
                    height = width * 9 / 16;
                }
                break;
            default:
                break;
        }
        Log.d(TAG, "#########setvideoSize(), result: width = " + width + ", height = " + height);
        setMeasuredDimension(width, height);
    }

    public int resolveAdjustedSize(int desiredSize, int measureSpec) {
        int result = desiredSize;
        int specMode = MeasureSpec.getMode(measureSpec);
        int specSize = MeasureSpec.getSize(measureSpec);

        switch (specMode) {
            case MeasureSpec.UNSPECIFIED:
                /*
                 * Parent says we can be as big as we want. Just don't be larger
                 * than max size imposed on ourselves.
                 */
                result = desiredSize;
                break;

            case MeasureSpec.AT_MOST:
                /*
                 * Parent says we can be as big as we want, up to specSize.
                 * Don't be larger than specSize, and don't be larger than the
                 * max size imposed on ourselves.
                 */
                result = Math.min(desiredSize, specSize);
                break;

            case MeasureSpec.EXACTLY:
                // No choice. Do what we are told.
                result = specSize;
                break;
            default:
                break;
        }
        return result;
    }
    private boolean requestAudioFocus() {
        // Request audio focus for playback
        int result = mAm.requestAudioFocus(afChangeListener,AudioManager.STREAM_MUSIC,
                    AudioManager.AUDIOFOCUS_GAIN);
        return result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;

    }

    private void initVideoView() {
        getHolder().addCallback(mSHCallback);
        // getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        mPendingSubtitleTracks = new Vector<Pair<InputStream, MediaFormat>>();
        mAm = (AudioManager)mContext.getSystemService(Context.AUDIO_SERVICE);
        afChangeListener = new AudioManager.OnAudioFocusChangeListener() {
            public void onAudioFocusChange(int focusChange) {
                if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
                        // Pause playback
                       Log.v(TAG,"AudioManager.AUDIOFOCUS_LOSS_TRANSIENT");
                       //pause();
                } else if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
                        // Resume playback
                        Log.v(TAG,"AudioManager.AUDIOFOCUS_GAIN");
                        play();
                } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS) {
                        // mAm.unregisterMediaButtonEventReceiver(RemoteControlReceiver);
                        Log.v(TAG,"AudioManager.AUDIOFOCUS_LOSS");
                        mAm.abandonAudioFocus(afChangeListener);
                        // Stop playback
                        //stopPlayback();
                }

            }

          };

    }

    public void setVideoPath(String path) {
        setVideoURI(Uri.parse(path));
    }

    public void setVideoURI(Uri uri) {
        setVideoURI(uri, null);
    }
    public void setSilentVolume() {
        if(mMediaPlayer!=null){
            mMediaPlayer.setVolume(0,0);
        }
    }


    /**
     * @hide
     */
    public void setVideoURI(Uri uri, Map<String, String> headers) {
        if(TRACE) Log.v(TAG, "setVideoURI in");
        mUri = uri;
        mHeaders = headers;
        mSeekWhenPrepared = 0;
        openVideo();
        requestLayout();
        invalidate();
    }

    public void setTimedTextPath(ArrayList<String> srtList, ArrayList<String> mediaTypeList) {
        Log.v("setTimedTextPath", "setTimedTextPathsetTimedTextPathsetTimedTextPath");
        mSrtList = srtList;
        mMediaTypeList = mediaTypeList;
    }

    /**
     * Adds an external subtitle source file (from the provided input stream.)
     *
     * Note that a single external subtitle source may contain multiple or no
     * supported tracks in it. If the source contained at least one track in it,
     * one will receive an {@link MediaPlayer#MEDIA_INFO_METADATA_UPDATE} info
     * message. Otherwise, if reading the source takes excessive time, one will
     * receive a {@link MediaPlayer#MEDIA_INFO_SUBTITLE_TIMED_OUT} message. If
     * the source contained no supported track (including an empty source file
     * or null input stream), one will receive a
     * {@link MediaPlayer#MEDIA_INFO_UNSUPPORTED_SUBTITLE} message. One can find
     * the total number of available tracks using
     * {@link MediaPlayer#getTrackInfo()} to see what additional tracks become
     * available after this method call.
     *
     * @param is
     *            input stream containing the subtitle data. It will be closed
     *            by the media framework.
     * @param format
     *            the format of the subtitle track(s). Must contain at least the
     *            mime type ( {@link MediaFormat#KEY_MIME}) and the language (
     *            {@link MediaFormat#KEY_LANGUAGE}) of the file. If the file
     *            itself contains the language information, specify "und" for
     *            the language.
     */
    public void addSubtitleSource(InputStream is, MediaFormat format) {
        if (mMediaPlayer == null) {
            mPendingSubtitleTracks.add(Pair.create(is, format));
        } else {
            try {
                mMediaPlayer.addSubtitleSource(is, format);
            } catch (IllegalStateException e) {
                mInfoListener.onInfo(mMediaPlayer, MediaPlayer.MEDIA_INFO_UNSUPPORTED_SUBTITLE, 0);
            }
        }
    }

    private Vector<Pair<InputStream, MediaFormat>> mPendingSubtitleTracks;

    public void stopPlayback() {
        if (mMediaPlayer != null) {
            mMediaPlayer.stop();
            mMediaPlayer.release();
            mMediaPlayer = null;
            mCurrentState = STATE_IDLE;
            mTargetState = STATE_IDLE;
            mAm.abandonAudioFocus(afChangeListener);
        }
        // Remove the mMediaController from the screen.
        if (mMediaController != null) {
            mMediaController.setHolding(false);
            mMediaController.hide();
        }
        // Remove the mDisplayController from the screen.
        hideDisplayController();
    }

    public void hideDisplayController(){
         mIsShowDisplayController = false;
         if(mDisplayController != null){
            mDisplayController.dismiss();
            mDisplayController = null;
         }
    }

    public void setBDFolderPlayMode(boolean bdMode) {
        mBDFolderPlayMode = bdMode;
    }

    public void clearCurrentStat() {
        Log.v(TAG, "00-----clearCurrentStat-----clearCurrentStat-----");
        mTrackInfosVideo.clear();
        mTrackInfosAudio.clear();
        mTrackInfosSub.clear();
        mTrackSubCur = -1;
        mTrackVideoCur = -1;
        mTrackAudioCur = -1;
        mTrackSubIndex.clear();
        mTrackAudioIndex.clear();
        mTrackVideoIndex.clear();
        mSrtList.clear();
        mMediaTypeList.clear();
        MovieViewControl.getInstance().resetAllTextView();
        MovieViewControl.getInstance().subTitleClear();
        Log.v(TAG, "11-----clearCurrentStat-----clearCurrentStat-----");
    }

    private void openVideo() {
        if(TRACE) Log.v(TAG, "openVideo in");
        if (mUri == null || mSurfaceHolder == null) {
            // not ready for playback just yet, will try again later
            return;
        }
        if (mErrorDialog != null) {
            mErrorDialog.dismiss();
        }
        // Tell the music playback service to pause
        Intent i = new Intent("com.android.music.musicservicecommand");
        i.putExtra("command", "pause");
        mContext.sendBroadcast(i);

        // we shouldn't clear the target state, because somebody might have
        // called start() previously
        release(false);
        // Modify by HM
        // release mTrackInfosSub mTrackInfosAudio
        mTrackInfosSub.clear();
        mTrackInfosAudio.clear();

        // end
        try {
            mMediaPlayer = new MediaPlayer();
            final Context context = getContext();
            final SubtitleController controller = new SubtitleController(context,
                    mMediaPlayer.getMediaTimeProvider(), mMediaPlayer);
            controller.registerRenderer(new WebVttRenderer(context));
            mMediaPlayer.setVideoPlayerFlag();
            mMediaPlayer.setSubtitleAnchor(controller, this);

            if (mAudioSession != 0) {
                mMediaPlayer.setAudioSessionId(mAudioSession);
            } else {
                mAudioSession = mMediaPlayer.getAudioSessionId();
            }
            mMediaPlayer.setOnPreparedListener(mPreparedListener);
            mMediaPlayer.setOnVideoSizeChangedListener(mSizeChangedListener);
            mDuration = -1;
            mMediaPlayer.setOnCompletionListener(mCompletionListener);
            mMediaPlayer.setOnErrorListener(mErrorListener);
            mMediaPlayer.setOnInfoListener(mInfoListener);
            mMediaPlayer.setOnBufferingUpdateListener(mBufferingUpdateListener);
            mMediaPlayer.setOnTimedTextListener(mOnTimedTextListener);
            mCurrentBufferPercentage = 0;
            Log.d(TAG, "============================= videoview mUri = " + mUri);
            if ("file".equals(mUri.getScheme())) {
                String path = mUri.getPath();
                final File file = new File(path);
                if (file.isDirectory()) {
                    path = "bdmv://" + path;
                    Log.d(TAG, "============================= videoview path = " + path);
                }
                // mMediaPlayer.setDataSource(mContext, mUri, mHeaders);
                mMediaPlayer.setDataSource(path, mHeaders);
            } else {
                mMediaPlayer.setDataSource(mContext, mUri, mHeaders);
            }
            if (mSrtList != null && mSrtList.size() > 0) {
                for (int j = 0; j < mSrtList.size(); j++) {
                    /*
                     * String mString = mSrtList.get(j); if
                     * (mString.startsWith("/storage")) { mString =
                     * mString.replaceFirst("storage", "mnt/shell");
                     * Log.d(TAG,"replace path!!!!!!!!" + mString); }
                     */
                    mMediaPlayer.addTimedTextSource(mSrtList.get(j), mMediaTypeList.get(j));
                    Log.v(TAG, "here j = " + j + ", mSrtList.get(j) = " + mSrtList.get(j)
                            + ", mMediaTypeList.get(j) = " + mMediaTypeList.get(j));
                }
            }
            mMediaPlayer.setDisplay(mSurfaceHolder);
            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            if( ((Activity)mContext).isInPictureInPictureMode()){
               mMediaPlayer.setVolume(0,0);
            }
            mMediaPlayer.setScreenOnWhilePlaying(true);
            mIsPreparing = true;
            mMediaPlayer.prepareAsync();
            // end
            for (Pair<InputStream, MediaFormat> pending : mPendingSubtitleTracks) {
                try {
                    mMediaPlayer.addSubtitleSource(pending.first, pending.second);
                } catch (IllegalStateException e) {
                    mInfoListener.onInfo(mMediaPlayer, MediaPlayer.MEDIA_INFO_UNSUPPORTED_SUBTITLE,
                            0);
                }
            }
            // we don't set the target state here either, but preserve the
            // target state that was there before.
            mCurrentState = STATE_PREPARING;
            if (mOnSubFocusItems != null) {
                mOnSubFocusItems.subFocusItems();
            }
            if (mMediaController != null) {
                mMediaController.setMediaPlayer(this);
            }
            // attachMediaController();
        } catch (IOException ex) {
            Log.w(TAG, "Unable to open content: " + mUri, ex);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNSUPPORTED, 0);
            return;
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "Unable to open content: " + mUri, ex);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNSUPPORTED, 0);
            return;
        } finally {
            mPendingSubtitleTracks.clear();
        }
    }

    public void setMediaController(MediaController controller) {
        if (mMediaController != null) {
            mMediaController.hide();
        }
        mMediaController = controller;
        attachMediaController();
    }

    private void attachMediaController() {
        if (mMediaController != null) {
            // if (mMediaPlayer != null && mMediaController != null) {
            // mMediaController.setMediaPlayer(this);
            View anchorView = this.getParent() instanceof View ? (View) this.getParent() : this;
            mMediaController.setAnchorView(anchorView);
            mMediaController.setEnabled(isInPlaybackState());
        }
    }

    MediaPlayer.OnVideoSizeChangedListener mSizeChangedListener
            = new MediaPlayer.OnVideoSizeChangedListener() {
        public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
            mVideoWidth = mp.getVideoWidth();
            mVideoHeight = mp.getVideoHeight();
            if (mVideoWidth > 0 && mVideoHeight > 0) {
                // getHolder().setFixedSize(mVideoWidth, mVideoHeight);
                if (mVideoWidth < 3840 || (mVideoHeight < 2160 && mVideoWidth != 3840)) {
                    getHolder().setFixedSize(mVideoWidth, mVideoHeight);
                } else {
                    getHolder().setFixedSize(1920, 1080);
                }
            }
        }
    };

    MediaPlayer.OnPreparedListener mPreparedListener = new MediaPlayer.OnPreparedListener() {
        public void onPrepared(MediaPlayer mp) {
            mCurrentState = STATE_PREPARED;
            // Get the capabilities of the player for this stream
            Metadata data = mp.getMetadata(MediaPlayer.METADATA_ALL,
                    MediaPlayer.BYPASS_METADATA_FILTER);

            if (data != null) {
                mCanPause = !data.has(Metadata.PAUSE_AVAILABLE)
                        || data.getBoolean(Metadata.PAUSE_AVAILABLE);
                mCanSeekBack = !data.has(Metadata.SEEK_BACKWARD_AVAILABLE)
                        || data.getBoolean(Metadata.SEEK_BACKWARD_AVAILABLE);
                mCanSeekForward = !data.has(Metadata.SEEK_FORWARD_AVAILABLE)
                        || data.getBoolean(Metadata.SEEK_FORWARD_AVAILABLE);

                //20160510,yuanlian, fixed google's bug.
                //no way to recycle the binder parcel in Metadata, just use reflect.
                try {
                    Field f = data.getClass().getDeclaredField("mParcel");
                    f.setAccessible(true);
                    Parcel p = (Parcel) f.get(data);
                    p.recycle();
                } catch (Exception e) {
                    e.printStackTrace();
                }

            } else {
                mCanPause = true;
                mCanSeekBack = true;
                mCanSeekForward = true;
            }

            if (mOnPreparedListener != null) {
                mOnPreparedListener.onPrepared(mMediaPlayer);
            }
            if (mMediaController != null) {
                mMediaController.setEnabled(true);
            }
            if (mOnSubFocusItems != null) {
                mOnSubFocusItems.initSubAndTrackInfo();
            }
            mVideoWidth = mp.getVideoWidth();
            mVideoHeight = mp.getVideoHeight();

            int seekToPosition = mSeekWhenPrepared; // mSeekWhenPrepared may be
                                                    // changed after seekTo()
                                                    // call
            if (seekToPosition != 0) {
                seekTo(seekToPosition);
            }
            if (mVideoWidth > 0 && mVideoHeight > 0) {
                // getHolder().setFixedSize(mVideoWidth, mVideoHeight);
                if (mVideoWidth < 3840 || (mVideoHeight < 2160 && mVideoWidth != 3840)) {
                    getHolder().setFixedSize(mVideoWidth, mVideoHeight);
                } else {
                    getHolder().setFixedSize(1920, 1080);
                }
        }
        if ((mSurfaceWidth == mVideoWidth && mSurfaceHeight == mVideoHeight)
                || mVideoHeight > 1080) {
                // We didn't actually change the size (it was already at the
                // size
                // we need), so we won't get a "surface changed" callback,
                // so
                // start the video here instead of in the callback.
                if (mTargetState == STATE_PLAYING) {
                    start();
                    if (mMediaController != null) {
                        mMediaController.show();
                    }
                } else if (!isPlaying() && (seekToPosition != 0 || getCurrentPosition() > 0)) {
                    if (mMediaController != null) {
                        // Show the media controls when we're paused into a
                        // video and make 'em stick.
                        mMediaController.show(0);
                    }
                }
            } else {
                // We don't know the video size yet, but should start anyway.
                // The video size might be reported to us later.
                if (mTargetState == STATE_PLAYING) {
                    start();
                }
            }
            // add by liuanlong
            Log.d("liuanlong", "------------------prepare done--------------------");
            MediaPlayer.TrackInfo[] trackInfos = mMediaPlayer.getTrackInfo();
            if (trackInfos != null && trackInfos.length > 0) {
                for (int j = 0; j < trackInfos.length; j++) {
                    MediaPlayer.TrackInfo info = trackInfos[j];
                    if (info.getTrackType()
                            == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_VIDEO) {
                        // mMediaPlayer.selectTrack(j);
                        mTrackInfosVideo.add(info);
                    } else if (info.getTrackType()
                            == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_AUDIO) {
                        mTrackInfosAudio.add(info);
                        mTrackAudioIndex.add(j);
                        // mMediaPlayer.selectTrack(j);
                    } else if (info.getTrackType()
                            == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT) {
                        mTrackInfosSub.add(info);
                        mTrackSubIndex.add(j);
                        mMediaPlayer.selectTrack(j);
                        break;
                    }
                }
            }
            if (mTrackInfosSub.size() != 0) {
                if (mTrackInfosSub.size() > mTrackSubCur && mTrackSubCur >= 0) {
                    mMediaPlayer.selectTrack(mTrackSubIndex.get(mTrackSubCur));
                } else {
                    // default
                    mMediaPlayer.selectTrack(mTrackSubIndex.get(0));
                    mTrackSubCur = 0;
                }
            }
            if (mTrackInfosAudio.size() != 0) {
                if (mTrackInfosAudio.size() > mTrackAudioCur && mTrackAudioCur >= 0) {
                    mMediaPlayer.selectTrack(mTrackAudioIndex.get(mTrackAudioCur));
                } else {
                    // default
                    mMediaPlayer.selectTrack(mTrackAudioIndex.get(0));
                    mTrackAudioCur = 0;
                }
            }
            // end
            mIsPreparing = false;
        }
    };

    private MediaPlayer.OnCompletionListener mCompletionListener
            = new MediaPlayer.OnCompletionListener() {
        public void onCompletion(MediaPlayer mp) {
            mCurrentState = STATE_PLAYBACK_COMPLETED;
            mTargetState = STATE_PLAYBACK_COMPLETED;
            if (mMediaController != null) {
                mMediaController.hide();
            }
            if (mOnCompletionListener != null) {
                mOnCompletionListener.onCompletion(mMediaPlayer);
            }
        }
    };

    private MediaPlayer.OnInfoListener mInfoListener = new MediaPlayer.OnInfoListener() {
        public boolean onInfo(MediaPlayer mp, int arg1, int arg2) {
            if (mOnInfoListener != null) {
                mOnInfoListener.onInfo(mp, arg1, arg2);
            }
            return true;
        }
    };
    private MediaPlayer.OnErrorListener mErrorListener = new MediaPlayer.OnErrorListener() {
        public boolean onError(MediaPlayer mp, int frameworkErr, int implErr) {
            Log.d(TAG, "Error: " + frameworkErr + "," + implErr);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            if (mMediaController != null) {
                mMediaController.hide();
            }

            /* If an error handler has been supplied, use it and finish. */
            if (mOnErrorListener != null) {
                if (mOnErrorListener.onError(mMediaPlayer, frameworkErr, implErr)) {
                    return true;
                }
            }

            /*
             * Otherwise, pop up an error dialog so the user knows that
             * something bad has happened. Only try and pop up the dialog if
             * we're attached to a window. When we're going away and no longer
             * have a window, don't bother showing the user an error.
             */
            if (getWindowToken() != null) {
                Resources r = mContext.getResources();
                int messageId = R.string.media_default_error;
                boolean release = false;

                if (frameworkErr == MediaPlayer.MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK) {
                    messageId = R.string.not_valid_for_progressive_playback;
                } else if (frameworkErr == MediaPlayer.MEDIA_ERROR_UNKNOWN) {
                    messageId = R.string.unknown_error;
                } else if (frameworkErr == MediaPlayer.MEDIA_ERROR_SERVER_DIED) {
                    messageId = R.string.mediaserver_died;
                /*} else if (frameworkErr == MediaPlayer.MEDIA_ERROR_OUT_OF_MEMORY) {
                    messageId = R.string.out_of_memory;
                    release = true;*/
                } else if (frameworkErr == MediaPlayer.MEDIA_ERROR_UNSUPPORTED) {
                    messageId = R.string.not_support_video;
                } else if (frameworkErr == MediaPlayer.MEDIA_ERROR_TIMED_OUT) {
                    messageId = R.string.timeout;
                    release = true;
                } else if (frameworkErr == MediaPlayer.MEDIA_ERROR_IO) {
                    messageId = R.string.io_error;
                    release = true;
                } else if (frameworkErr == MediaPlayer.MEDIA_ERROR_MALFORMED) {
                    messageId = R.string.error_malformed;
                }
                if (release) {
                    if (mMediaPlayer != null) {
                        mMediaPlayer.release();
                        mMediaPlayer = null;
                    }
                }
                /* add by liuanlong 14/11/25>>> */
                if (messageId == R.string.mediaserver_died) {
                    if (mOnCompletionListener != null) {
                        mOnCompletionListener.onCompletion(mMediaPlayer);
                    }
                } else { /* <<<end */
                    LayoutInflater inflate = (LayoutInflater) mContext
                            .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                    View errorView = inflate.inflate(R.layout.dialog_replay, null);
                    /* error title */
                    TextView v = (TextView) errorView.findViewById(R.id.replay_title);
                    v.setText(com.android.internal.R.string.VideoView_error_title);
                    /* error message */
                    v = (TextView) errorView.findViewById(R.id.replay_msg);
                    v.setText(messageId);
                    ((Button) errorView.findViewById(R.id.replay_confirm))
                            .setOnClickListener(new View.OnClickListener() {
                                public void onClick(View arg0) {
                                    if (mOnCompletionListener != null) {
                                        mOnCompletionListener.onCompletion(mMediaPlayer);
                                    }
                                    mErrorDialog.dismiss();
                                    ((Activity)mContext).finish();
                                }
                            });
                    ((Button) errorView.findViewById(R.id.replay_cancel)).setVisibility(GONE);
                    mErrorDialog = new Dialog(mContext, R.style.dialog);
                    mErrorDialog.setContentView(errorView);
                    mErrorDialog.setCancelable(false);
                    Window dialogWindow = mErrorDialog.getWindow();
                    WindowManager.LayoutParams lp = dialogWindow.getAttributes();
                    dialogWindow.setGravity(Gravity.CENTER);
                    WindowManager m = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
                    Display d = m.getDefaultDisplay();
                    lp.width = (int) (d.getWidth() * 0.25);
                    lp.height = (int) (d.getHeight() * 0.25);
                    dialogWindow.setAttributes(lp);
                    mErrorDialog.show();
                }
            }
            return true;
        }
    };

    private MediaPlayer.OnBufferingUpdateListener mBufferingUpdateListener
            = new MediaPlayer.OnBufferingUpdateListener() {
        public void onBufferingUpdate(MediaPlayer mp, int percent) {
            mCurrentBufferPercentage = percent;
        }
    };

    private MediaPlayer.OnTimedTextListener mOnTimedTextListener
            = new MediaPlayer.OnTimedTextListener() {
        public void onTimedText(MediaPlayer mp, TimedText text) {
        }
    };

    /**
     * Register a callback to be invoked when the media file is loaded and ready
     * to go.
     *
     * @param l
     *            The callback that will be run
     */
    public void setOnPreparedListener(MediaPlayer.OnPreparedListener l) {
        mOnPreparedListener = l;
    }

    /**
     * Register a callback to be invoked when the end of a media file has been
     * reached during playback.
     *
     * @param l
     *            The callback that will be run
     */
    public void setOnCompletionListener(OnCompletionListener l) {
        mOnCompletionListener = l;
    }

    /**
     * Register a callback to be invoked when an error occurs during playback or
     * setup. If no listener is specified, or if the listener returned false,
     * VideoView will inform the user of any errors.
     *
     * @param l
     *            The callback that will be run
     */
    public void setOnErrorListener(OnErrorListener l) {
        mOnErrorListener = l;
    }

    /**
     * Register a callback to be invoked when an informational event occurs
     * during playback or setup.
     *
     * @param l
     *            The callback that will be run
     */
    public void setOnInfoListener(OnInfoListener l) {
        mOnInfoListener = l;
    }

    /**
     * Register a callback to be invoked when an timedtext event occurs during
     * playing.
     *
     * @param l
     *            The callback that will be run
     *
     *            by maizirong
     */
    public void setOnTimedTextListener(OnTimedTextListener l) {
        mOnTimedTextListener = l;
    }

    SurfaceHolder.Callback mSHCallback = new SurfaceHolder.Callback() {
        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
            mSurfaceWidth = w;
            mSurfaceHeight = h;
            Log.v("maizirong", "=====surfaceChanged====");
            boolean isValidState = (mTargetState == STATE_PLAYING);
            boolean hasValidSize = (mVideoWidth == w && mVideoHeight == h);
            if (mMediaPlayer != null && isValidState && hasValidSize) {
                if (mSeekWhenPrepared != 0) {
                    seekTo(mSeekWhenPrepared);
                }
                if (mMediaController != null) {
                    if (mMediaController.isShowing()) {
                        // ensure the controller will get repositioned later
                        mMediaController.hide();
                    }
                    mMediaController.show();
                }
            }
        }

        public void surfaceCreated(SurfaceHolder holder) {
            mSurfaceHolder = holder;
            openVideo();
            Log.v("maizirong", "=====surfaceCreated====");
        }

        public void surfaceDestroyed(SurfaceHolder holder) {
            // after we return from this we can't use the surface any more
            Log.v("maizirong", "=====surfaceDestroyed====");
            mSurfaceHolder = null;
            if (mMediaController != null) {
                mMediaController.hide();
            }
            release(true);
        }
    };

    /*
     * release the media player in any state
     */
    private void release(boolean cleartargetstate) {
        if (mMediaPlayer != null) {
            mMediaPlayer.reset();
            mMediaPlayer.release();
            mMediaPlayer = null;
            mPendingSubtitleTracks.clear();
            mCurrentState = STATE_IDLE;
            if (cleartargetstate) {
                mTargetState = STATE_IDLE;
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (isInPlaybackState() && mMediaController != null) {
            toggleMediaControlsVisiblity();
        }
        return false;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent ev) {
        if (isInPlaybackState() && mMediaController != null) {
            toggleMediaControlsVisiblity();
        }
        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        boolean isKeyCodeSupported = keyCode != KeyEvent.KEYCODE_BACK
                && keyCode != KeyEvent.KEYCODE_VOLUME_UP && keyCode != KeyEvent.KEYCODE_VOLUME_DOWN
                && keyCode != KeyEvent.KEYCODE_CALL
                && keyCode != KeyEvent.KEYCODE_ENDCALL;
        if (isInPlaybackState() && isKeyCodeSupported && mMediaController != null) {
            if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE) {
                if (mMediaPlayer.isPlaying()) {
                    pause();
                    mMediaController.show();
                } else {
                    start();
                    mMediaController.hide();
                }
                return true;
            } else if (keyCode == /*KeyEvent.KEYCODE_SUBTITLE*/ 10003) { // switch
                                                               // subtitle
                mMediaController.onMediaSubtitleKeyListener();
                return true;
            } else if (keyCode == /*KeyEvent.KEYCODE_AUDIO*/ 10004) { // switch audio
                mMediaController.onMediaAudioKeyListener();
                return true;
            } else if (keyCode == /*KeyEvent.KEYCODE_LOOP*/ 10007) { // repeat
                mMediaController.onMediaRepeatClickListener();
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_PREVIOUS) { // prev
                mMediaController.onMediaPrevClickListener();
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_NEXT) { // next
                mMediaController.onMediaNextClickListener();
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_STOP && mMediaPlayer.isPlaying()) {
                mMediaController.onMediaStopClickListener();
                // pause();
                // mMediaController.show();
            } else if(keyCode == KeyEvent.KEYCODE_MENU ){
                startDisplaySettingsMenu();
            } else {
                toggleMediaControlsVisiblity();
            }
        }

        return super.onKeyDown(keyCode, event);
    }

    public void startDisplaySettingsMenu() {
        mIsShowDisplayController = true;
        if (mDisplayController == null) {
            String platform = SystemProperties.get("ro.build.characteristics");
            if (platform != null) {
                if(platform.equals("tv")) {
                    Class<?> displayClass = null;
                    try {
                        displayClass = Class.forName("com.softwinner.TvdVideo.TvDisplayController");
                        Constructor<?> constructor = null;
                        if(displayClass!=null) {
                            constructor = displayClass.getDeclaredConstructor(Context.class,int.class,int.class);
                        }
                        if(constructor!=null) {
                            mDisplayController = (IDisplayController) constructor.newInstance(mContext,mVideoWidth,mVideoHeight);
                        }
                    } catch (ClassNotFoundException | NoSuchMethodException | IllegalAccessException
                                | InstantiationException | InvocationTargetException e) {
                        e.printStackTrace();
                    }
                } else if(platform.equals("homlet")){
                    mDisplayController  = new StbDisplayController(mContext,mVideoWidth,mVideoHeight);
                }
            }
        }
        if(mDisplayController!=null) {
            mDisplayController.show();
        }
    }

    private void toggleMediaControlsVisiblity() {
        if (mMediaController.isShowing()) {
            mMediaController.hide();
        } else {
            mMediaController.show();
        }
    }

    public void start() {
        if(((Activity)mContext).isInPictureInPictureMode()){
           play();
        }else{
           if(requestAudioFocus()){
              play();
           }
        }
    }
    public void play(){
       if (isInPlaybackState()) {
            mMediaPlayer.start();
            mCurrentState = STATE_PLAYING;
          }
          mTargetState = STATE_PLAYING;
    }
    public void pause() {
        if (isInPlaybackState()) {
            if (mMediaPlayer.isPlaying()) {
                mMediaPlayer.pause();
                mCurrentState = STATE_PAUSED;
                mAm.abandonAudioFocus(afChangeListener);
            }
        }
        mTargetState = STATE_PAUSED;
    }

    public void suspend() {
        release(false);
    }

    public void resume() {
        if(TRACE) Log.v(TAG, "resume in");
        openVideo();
    }

    // cache duration as mDuration for faster access
    public int getDuration() {
        if (isInPlaybackState()) {
            if (mDuration > 0) {
                return mDuration;
            }
            mDuration = mMediaPlayer.getDuration();
            return mDuration;
        }
        mDuration = -1;
        return mDuration;
    }

    public int getCurrentPosition() {
        if (isInPlaybackState()) {
            return mMediaPlayer.getCurrentPosition();
        }
        return 0;
    }

    public void seekTo(int msec) {
        if (isInPlaybackState()) {
            mMediaPlayer.seekTo(msec);
            mSeekWhenPrepared = 0;
        } else {
            mSeekWhenPrepared = msec;
        }
    }

    public boolean isPlaying() {
        return isInPlaybackState() && mMediaPlayer.isPlaying();
    }

    public boolean isShowDisplayController(){
        return mIsShowDisplayController;
    }

    public int getBufferPercentage() {
        if (mMediaPlayer != null) {
            return mCurrentBufferPercentage;
        }
        return 0;
    }

    private boolean isInPlaybackState() {
        return (mMediaPlayer != null && mCurrentState != STATE_ERROR
                 && mCurrentState != STATE_IDLE
                 && mCurrentState != STATE_PREPARING);
    }

    public boolean canPause() {
        return mCanPause;
    }

    public boolean canSeekBackward() {
        return mCanSeekBack;
    }

    public boolean canSeekForward() {
        return mCanSeekForward;
    }

    public TrackInfo[] getSubList() {
        int i = 0;
        TrackInfo[] trackInfo = new TrackInfo[mTrackInfosSub.size()];
        mTrackInfosSub.toArray(trackInfo);
        return trackInfo;
    }

    public int getCurSub() {
        return mTrackSubCur;
        /*
         * if(mMediaPlayer == null){ return -1; } return
         * mMediaPlayer.getCurSub();
         */
    }

    public int switchSub(int index) {
        mTrackSubCur = index;
        if (mMediaPlayer == null) {
            return -1;
        }
        if (mTrackInfosSub.size() != 0 && index >= 0) {
            mMediaPlayer.selectTrack(mTrackSubIndex.get(index));
        }
        return 0;
    }

    public TrackInfo[] getTrackInfo() {
        if (mMediaPlayer != null) {
            TrackInfo[] trackInfos = mMediaPlayer.getTrackInfo();
            return trackInfos;
        }
        return null;
    }

    public TrackInfo[] getTrackList() {
        mTrackInfosVideo.clear();
        mTrackInfosAudio.clear();
        mTrackInfosSub.clear();
        mTrackSubIndex.clear();
        mTrackAudioIndex.clear();
        mTrackVideoIndex.clear();
        MediaPlayer.TrackInfo[] trackInfos = mMediaPlayer.getTrackInfo();
        if (trackInfos != null && trackInfos.length > 0) {
            for (int j = 0; j < trackInfos.length; j++) {
                MediaPlayer.TrackInfo info = trackInfos[j];
                if (info.getTrackType() == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_VIDEO) {
                    mTrackInfosVideo.add(info);
                } else if (info.getTrackType() == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_AUDIO) {
                    mTrackInfosAudio.add(info);
                    mTrackAudioIndex.add(j);
                } else if (info.getTrackType()
                        == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT) {
                    mTrackInfosSub.add(info);
                    mTrackSubIndex.add(j);
                }
            }
        }
        TrackInfo[] trackInfo = new TrackInfo[mTrackInfosAudio.size()];
        mTrackInfosAudio.toArray(trackInfo);
        return trackInfo;
    }

    public int getCurTrack() {
        return mTrackAudioCur;
    }

    public int switchTrack(int index) {
        mTrackAudioCur = index;
        if (mMediaPlayer == null) {
            return -1;
        }
        if (mTrackInfosAudio.size() != 0 && index >= 0) {
            mMediaPlayer.selectTrack(mTrackAudioIndex.get(index));
        }
        return 0;
    }

/*    *//**
     * Set the subtitle’s charset. If the underlying mediaplayer can absolutely
     * parse the charset of the subtitles, still use the parsed charset;
     * otherwise, use the charset argument.
     * <p>
     *
     * @param charset
     *            the canonical name of a charset.
     * @return ==0 means successful, !=0 means failed.
     *//*
    public int setSubCharset(String charset) {
        if (mMediaPlayer == null) {
            return -1;
        }
        return mMediaPlayer.setSubCharset(charset);
    }

    *//**
     * Get the subtitle’s charset.
     * <p>
     *
     * @return the canonical name of a charset.
     *//*
    public String getSubCharset() {
        if (mMediaPlayer == null) {
            return null;
        }
        return mMediaPlayer.getSubCharset();
    }

    *//**
     * Set the subtitle’s delay time.
     * <p>
     *
     * @param time
     *            delay time in milliseconds. It can be <0.
     * @return ==0 means successful, !=0 means failed.
     *//*
    public int setSubDelay(int time) {
        if (mMediaPlayer == null) {
            return -1;
        }
        return mMediaPlayer.setSubDelay(time);
    }

    *//**
     * Get the subtitle’s delay time.
     * <p>
     *
     * @return delay time in milliseconds.
     *//*
    public int getSubDelay() {
        if (mMediaPlayer == null) {
            return -1;
        }
        return mMediaPlayer.getSubDelay();
    }
*/
    /**
     * switch another screen size to show video.
     * <p>
     *
     * @param mode
     *            the zoom's index in the zoom list.
     */
    public void setZoomMode(int mode) {
        if(TRACE)Log.v(TAG, "setZoomMode mode="+mode+",mZoomMode="+mZoomMode);
        if (mode == mZoomMode) {
            return;
        }
        if(mode == ZOOM_PIP){
            if(TRACE)Log.v(TAG, "setZoomMode enterPictureInPictureMode --------");
             ((Activity)mContext).enterPictureInPictureMode();
             return; //这个地方要返回，不然会老是有一个灰色的全屏holderSurface显示
        }
        mZoomMode = mode; //从PIP模式回到全屏时，zoom菜单子项焦点不应该在“画中画”，应该在原来那个位置，所以不改变焦点。
        if (mVideoWidth > 0 && mVideoHeight > 0) {
            if(TRACE)Log.v(TAG, "setZoomMode mode="+mode+",videoW="+mVideoWidth+",videoH="+mVideoHeight);
            // getHolder().setFixedSize(mVideoWidth, mVideoHeight);
            if (mVideoWidth < 3840 || (mVideoHeight < 2160 && mVideoWidth != 3840)) {
                getHolder().setFixedSize(mVideoWidth, mVideoHeight);
            } else {
                getHolder().setFixedSize(1920, 1080);
            }
            requestLayout();
        }
    }
    public MediaPlayer getMediaPlayer(){
        return mMediaPlayer;
    }

    /**
     * get screen size to show video.
     * <p>
     *
     */
    public int getZoomMode() {
        return mZoomMode;
    }

    public int getAudioSessionId() {
        if (mAudioSession == 0) {
            MediaPlayer foo = new MediaPlayer();
            mAudioSession = foo.getAudioSessionId();
            foo.release();
        }
        return mAudioSession;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        if (mSubtitleWidget != null) {
            mSubtitleWidget.onAttachedToWindow();
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        if (mSubtitleWidget != null) {
            mSubtitleWidget.onDetachedFromWindow();
        }
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        if (mSubtitleWidget != null) {
            measureAndLayoutSubtitleWidget();
        }
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        if (mSubtitleWidget != null) {
            final int saveCount = canvas.save();
            canvas.translate(getPaddingLeft(), getPaddingTop());
            mSubtitleWidget.draw(canvas);
            canvas.restoreToCount(saveCount);
        }
    }

    /**
     * Forces a measurement and layout pass for all overlaid views.
     *
     * @see #setSubtitleWidget(RenderingWidget)
     */
    private void measureAndLayoutSubtitleWidget() {
        final int width = getWidth() - getPaddingLeft() - getPaddingRight();
        final int height = getHeight() - getPaddingTop() - getPaddingBottom();

        mSubtitleWidget.setSize(width, height);
    }

    /** @hide */
    @Override
    public void setSubtitleWidget(RenderingWidget subtitleWidget) {
        if (mSubtitleWidget == subtitleWidget) {
            return;
        }

        final boolean attachedToWindow = isAttachedToWindow();
        if (mSubtitleWidget != null) {
            if (attachedToWindow) {
                mSubtitleWidget.onDetachedFromWindow();
            }

            mSubtitleWidget.setOnChangedListener(null);
        }

        mSubtitleWidget = subtitleWidget;

        if (subtitleWidget != null) {
            if (mSubtitlesChangedListener == null) {
                mSubtitlesChangedListener = new RenderingWidget.OnChangedListener() {
                    @Override
                    public void onChanged(RenderingWidget renderingWidget) {
                        invalidate();
                    }
                };
            }

            setWillNotDraw(false);
            subtitleWidget.setOnChangedListener(mSubtitlesChangedListener);

            if (attachedToWindow) {
                subtitleWidget.onAttachedToWindow();
                requestLayout();
            }
        } else {
            setWillNotDraw(true);
        }

        invalidate();
    }

    /** @hide */
    @Override
    public Looper getSubtitleLooper() {
        return Looper.getMainLooper();
    }

    /**
     * Register a callback to be invoked when the media source is ready for
     * playback.
     *
     */
    public interface OnSubFocusItems {
        void subFocusItems();

        void initSubAndTrackInfo();
    }

    public void setOnSubFocusItems(OnSubFocusItems subFocusItems) {
        mOnSubFocusItems = subFocusItems;
    }

    private OnSubFocusItems mOnSubFocusItems;

    /* add by liuanlong 14/11/28 */

    public int getVideoStreamNum() {
        MediaFormat tempFormat = mTrackInfosVideo.get(0).getFormat();
        return tempFormat.getInteger("is-doubleStream") == 0 ? 1 : 2;
    }

    public boolean isPreparing() {
        return mIsPreparing;
    }
    /* end */

    public int getVideoHeight() {
        return mVideoHeight;
    }
}

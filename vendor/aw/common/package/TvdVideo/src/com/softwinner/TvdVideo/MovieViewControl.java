/*
 * Copyright (C) 2009 The Android Open Source Project
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

import android.app.Activity;
import com.android.internal.util.ArrayUtils;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IContentProvider;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.hardware.display.DisplayManager;
import android.hardware.input.InputManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.TrackInfo;
import android.media.TimedText;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import vendor.display.DisplayOutputManager;
import android.provider.MediaStore;
import android.provider.MediaStore.Video;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.IWindowManager;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import android.content.pm.ProviderInfo;
import java.util.List;
import androidx.core.content.FileProvider;

public class MovieViewControl implements MediaPlayer.OnErrorListener,
        MediaPlayer.OnCompletionListener, MediaController.OnListDataChanged,
        MediaPlayer.OnTimedTextListener, VideoView.OnSubFocusItems, MediaPlayer.OnInfoListener {

    @SuppressWarnings("unused")
    private static final String TAG = "MovieViewControl";
    private static final boolean TRACE = DebugConfigs.VERBOSE_INFO;

    private static final String STORE_NAME = "VideoSetting";
    private static final String EDITOR_SUBGATE = "MovieViewControl.SUBGATE";
    private static final String EDITOR_SUBSELECT = "MovieViewControl.SUBSELECT";
    private static final String EDITOR_SUBCHARSET = "MovieViewControl.SUBCHARSET";
    private static final String EDITOR_SUBCOLOR = "MovieViewControl.SUBCOLOR";
    private static final String EDITOR_SUBCHARSIZE = "MovieViewControl.SUBCHARSIZE";
    private static final String EDITOR_SUBOFFSET = "MovieViewControl.SUBOFFSET";
    private static final String EDITOR_SUBDELAY = "MovieViewControl.SUBDELAY";
    private static final String EDITOR_ZOOM = "MovieViewControl.MODEZOOM";
    private static final String EDITOR_MODE3D = "MovieViewControl.MODE3D";
    private static final String EDITOR_MODE2DOR3D = "MovieViewControl.MODE2DOR3D";
    private static final String EDITOR_MODEREPEAT = "MovieViewControl.MODEREPEAT";
    private static final String EDITOR_TRACK = "MovieViewControl.TRACK";

    private static final int TEXTVIEW_UPDATE = 2;

    private static final String[] SUB_EXTS = new String[] {
            ".idx",
            ".sub", // .idx
            ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".js", ".ass", ".vsf", ".tts",
            ".stl", ".zeg", ".ovr", ".dks", ".lrc", ".pan", ".sbt", ".vkt", ".pjs", ".mpl", ".scr",
            ".psb", ".asc", ".rtf", ".s2k", ".sst", ".son", ".ssts", ".sami"};

    private static final String[] MEDIA_MIMETYPE = new String[] {
            "application/idx-sub",
            "application/sub", // .idx
            "application/x-subrip", "text/smi", "text/rt", "text/txt", "text/ssa", "text/aqt",
            "text/jss", "text/js", "text/ass", "text/vsf", "text/tts", "text/stl", "text/zeg",
            "text/ovr", "text/dks", "text/lrc", "text/pan", "text/sbt", "text/vkt", "text/pjs",
            "text/mpl", "text/scr", "text/psb", "text/asc", "text/rtf", "text/s2k", "text/sst",
            "text/son", "text/ssts", "text/smi"};
    private static final int DISMISS_DIALOG = 0x10;
    private static final int DISMISS_DELAY = 5000; // 5 s

    private BookmarkService mBookmarkService;
    private final VideoView mVideoView;
    private final View mProgressView;
    private Dialog mReplayDialog;
    private Message mDismissMsg;
    private Uri mUri;
    private Context mContext;
    DisplayOutputManager displayManager = null;
    private boolean mIsDoubleStream = false;
    private boolean mToQuit = false;
    private boolean mOnPause;
    private boolean mFinishOnCompletion;
    private boolean mIsNetworkVideo = false;
    private String mPlayListType;
    private final SharedPreferences sp;
    private final SharedPreferences.Editor editor;
    private Resources mRes;
    private int mCurrentIndex = 0;
    private int mRepeatMode = 0;
    private int mVideoPosition = 0;
    private ArrayList<String> mPlayList;
    private MediaController mMediaController;
    private static MovieViewControl mInstance = null;
    private Toast mJumpToast;
    private MyToast mSubtitleToast;
    private MyToast mAudioToast;
    private MyToast mPrevToast;
    private MyToast mNextToast;
    private MyToast mStopToast;
    private MyToast mRepeatToast;
    private String[] mTransformSubtitle;
    private String[] mTransformTrack;
    private String[] mListMode3D;

    private BroadcastReceiver mBroadcastReceiver;
    private boolean isBroadcastReceiverRegistered = false; //标识mBroadcastReceiver是否注册了

    private ArrayList<String> mSrtList = new ArrayList<String>();
    private ArrayList<String> mMediaTypeList = new ArrayList<String>();

    // add for handling subtitle
    private ImageView mImageview;
    private Bitmap mBitmap;
    private int mBitmapSubtitleFlag = 0;
    private int mScreenWidth;
    private int mScreenHeight;
    private int mReferenceVideoWidth;
    private int mReferenceVideoHeight;
    private int PICTURE_IN_PICTURE_ITEM=5;
    private SubTitleInfoOps subTitleInfoOps = new SubTitleInfoOps();
    private SubTitleInfo mSubTitleInfo;
    private TextViewInfo[] mTextViewInfo = new TextViewInfo[10];
    private final Handler mSubTitleHandler = new MainHandler(); // handle text

    // SAVE sub and track, add by maizirong
    private int mCurrentTrackSave = -1;
    private int mCurrentSubSave = -1;
    // SAVE subcolor and subfontsize, add by maizirong
    private int mCurrentSubColorSave = -1;
    private int mCurrentSubSizeSave = 32;
    private boolean switchSubOn = true;
    private Intent mIntent;
    private String realPath;
    private boolean isActivityOutFromPIPMode = false; //标识主activity是否是刚从PIP模式中退出来的

    public static MovieViewControl getInstance() {
        return mInstance;
    }

    public MovieViewControl(View rootView, Context context, Intent intent, String path000,
                            int width, int height) {
        Log.v(TAG, "MovieViewControl path000 ="+path000);
        realPath = path000;
        mInstance = this;
        mVideoView = (VideoView) rootView.findViewById(R.id.surface_view);

        // init subtitle textview
        mTextViewInfo[0] = new TextViewInfo();
        mTextViewInfo[1] = new TextViewInfo();
        mTextViewInfo[2] = new TextViewInfo();
        mTextViewInfo[3] = new TextViewInfo();
        mTextViewInfo[4] = new TextViewInfo();
        mTextViewInfo[5] = new TextViewInfo();
        mTextViewInfo[6] = new TextViewInfo();
        mTextViewInfo[7] = new TextViewInfo();
        mTextViewInfo[8] = new TextViewInfo();
        mTextViewInfo[9] = new TextViewInfo();

        mTextViewInfo[0].textView = (TextView) rootView.findViewById(R.id.text_view_0);
        mTextViewInfo[0].used = false;
        mTextViewInfo[1].textView = (TextView) rootView.findViewById(R.id.text_view_1);
        mTextViewInfo[1].used = false;
        mTextViewInfo[2].textView = (TextView) rootView.findViewById(R.id.text_view_2);
        mTextViewInfo[2].used = false;
        mTextViewInfo[3].textView = (TextView) rootView.findViewById(R.id.text_view_3);
        mTextViewInfo[3].used = false;
        mTextViewInfo[4].textView = (TextView) rootView.findViewById(R.id.text_view_4);
        mTextViewInfo[4].used = false;
        mTextViewInfo[5].textView = (TextView) rootView.findViewById(R.id.text_view_5);
        mTextViewInfo[5].used = false;
        mTextViewInfo[6].textView = (TextView) rootView.findViewById(R.id.text_view_6);
        mTextViewInfo[6].used = false;
        mTextViewInfo[7].textView = (TextView) rootView.findViewById(R.id.text_view_7);
        mTextViewInfo[7].used = false;
        mTextViewInfo[8].textView = (TextView) rootView.findViewById(R.id.text_view_8);
        mTextViewInfo[8].used = false;
        mTextViewInfo[9].textView = (TextView) rootView.findViewById(R.id.text_view_9);
        mTextViewInfo[9].used = false;

        mImageview = (ImageView) rootView.findViewById(R.id.image_view);

        mProgressView = rootView.findViewById(R.id.progress_indicator);

        mContext = context;
        mUri = uri2File2Uri(intent.getData());
        mRes = mContext.getResources();
        sp = mContext.getSharedPreferences(STORE_NAME, Context.MODE_PRIVATE);
        editor = sp.edit();
        initToast();

        // For streams that we expect to be slow to start up, show a
        // progress spinner until playback starts.
        String scheme = mUri.getScheme();
        if ("http".equalsIgnoreCase(scheme) || "rtsp".equalsIgnoreCase(scheme)) {
            mHandler.postDelayed(mPlayingChecker, 250);
        } else {
            mProgressView.setVisibility(View.GONE);
        }

        /* create playlist */
        mFinishOnCompletion = intent.getBooleanExtra(MediaStore.EXTRA_FINISH_ON_COMPLETION, true);
        mPlayListType = intent.getStringExtra("android.intent.extra.playListType");
        mPlayList = new ArrayList<String>();
        if (mPlayListType != null) {
            if (mPlayListType.equalsIgnoreCase("curFolder")) {
                /* create playlist from current folder */
                createFolderDispList();
            } else if (mPlayListType.equalsIgnoreCase("mediaProvider")) {
                /* create playlist from mediaprovider */
                createMediaProviderDispList(mUri, mContext);
            }
        } else if (realPath != null) {
            mPlayList.add(mUri.getPath());
        } else if (mPlayListType == null && realPath == null) {
            String path = mUri.getPath();
            if (isOrdinaryMovieFile(path)) { /* ordinary video */
                createFolderDispList();
            } else if (isISOFile(path)) { /* iso */
                mPlayList.add(path);
            }
        } else {
            Log.w(TAG, "****** scheme is null or scheme != file, create playlist failed ******");
        }
        mMediaController = new MediaController(context,intent);
        mVideoView.setOnSubFocusItems(this);
        mVideoView.setOnErrorListener(this);
        mVideoView.setOnInfoListener(this);
        mVideoView.setOnCompletionListener(this);
        mVideoView.setOnTimedTextListener(this);
        mVideoView.setVideoURI(mUri);

        if (realPath == null) { /* ordinary video */
            File file = new File(mUri.getPath());
            boolean isFolder;
            if (file.exists()) {
                isFolder = file.isDirectory();
                if (isFolder) {
                    Log.v(TAG,"isFolder");
                    searchBluraySubTitle(mUri.getPath());
                } else {
                    searchSubTitle(mUri.getPath());
                }
            }
        } else { /* iso video */
            searchSubTitle(realPath);
        }
        if (mSrtList != null && mSrtList.size() > 0) {
            Log.v("mVideoView.setTimedTextPath", "mVideoView.setTimedTextPath");
            mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
        }
        setImageButtonListener();
        mVideoView.setMediaController(mMediaController);
        mMediaController.setFilePathTextView(mUri.getPath());
        // make the video view handle keys for seeking and pausing
        mVideoView.requestFocus();
        mMediaController.setOnListDataChanged(this);

        mBookmarkService = new BookmarkService(mContext);
        final int bookmark = getBookmark();
        // SAVE sub and track, add by maizirong
        mCurrentSubSave = getSubSave();
        mCurrentTrackSave = getTrackSave();
        mCurrentSubColorSave = getSubColorSave();
        mCurrentSubSizeSave = getSubSizeSave();
        Log.v(TAG, "_subBookMark______" + Integer.toString(mCurrentSubSave));
        if (bookmark > 0) {
            replayVideoDialog();
            mVideoView.seekTo(bookmark);
            // deleteBookmark();
        }
        mVideoView.start();
        mScreenWidth = width;
        mScreenHeight = height;
        Log.v(TAG, "======mScreenWidth======" + mScreenWidth);
        Log.v(TAG, "======mScreenHeight======" + mScreenHeight);
        mListMode3D = mRes.getStringArray(R.array.screen_3d_entries);
    }

    Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            // TODO Auto-generated method stub
            if (msg.what == DISMISS_DIALOG && mReplayDialog.isShowing()) {
                mReplayDialog.dismiss();
            }
        }

    };

    Runnable mMode3DRunnable = null;
    Runnable mRepeatRunnable = null;
    Runnable mTrackRunnable = null;
    Runnable mZoomRunnable = null;
    Runnable mSubsetRunnable = null;
    Runnable mPlayingChecker = new Runnable() {
        public void run() {
            if (mVideoView.isPlaying()) {
                mProgressView.setVisibility(View.GONE);
            } else {
                mHandler.postDelayed(mPlayingChecker, 250);
            }
        }
    };

    Runnable mPlayRunnable = new Runnable() {
        @Override
        public void run() {
            setImageButtonListener();
            mVideoView.setVideoURI(mUri);
            Log.v(TAG, "-------mVideoView.setVideoURI(mUri)-----" + mUri.getPath());
            mMediaController.setFilePathTextView(mUri.getPath());
            mMediaController.setHolding(false);
            mVideoView.requestFocus();
            mVideoView.start();
            mHandler.removeCallbacks(mPlayRunnable);
        }
    };

    private final Runnable mUpdateImageView = new Runnable() {
        @Override
        public void run() {
            if(mBitmap != null) {
                mImageview.setVisibility(View.VISIBLE);
                mImageview.setImageBitmap(mBitmap);
            } else {
                mImageview.setVisibility(View.INVISIBLE);
                mImageview.setImageBitmap(null);
            }
        }
    };

    private class MainHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case TEXTVIEW_UPDATE:
                    for (int i = 0; i < mTextViewInfo.length; i++) {
                        mTextViewInfo[i].textView.setText(mTextViewInfo[i].text);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    class MyToast extends Toast {
        View mNextView;
        int mDuration;

        MyToast(Context context) {
            super(context);
            // TODO Auto-generated constructor stub
            LayoutInflater inflate = (LayoutInflater) mContext
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            mNextView = inflate.inflate(R.layout.transient_notification, null);
            mDuration = MyToast.LENGTH_SHORT;
            super.setView(mNextView);
            super.setDuration(mDuration);
            super.setGravity(Gravity.LEFT | Gravity.TOP, 100, 0);
        }
    }

    public static String formatDuration(final Context context, int durationMs) {
        int duration = durationMs / 1000;
        int h = duration / 3600;
        int m = (duration - h * 3600) / 60;
        int s = duration - (h * 3600 + m * 60);
        String durationValue;
        if (h == 0) {
            durationValue = String.format("%02d:%02d", m, s);
        } else {
            durationValue = String.format("%d:%02d:%02d", h, m, s);
        }
        return durationValue;
    }

    public void resetAllTextView() {
        Log.v(TAG, "00-----resetAllTextView-----resetAllTextView-----");
        if (subTitleInfoOps != null) {
            subTitleInfoOps.removeAllSubTitleInfo();
        }
        int i = 0;
        for (i = 0; i < 10; i++) {
            mTextViewInfo[i].text = "";
            mTextViewInfo[i].textView.setText(mTextViewInfo[i].text);
            mTextViewInfo[i].used = false;
        }
        mImageview.setImageBitmap(null);
        Log.v(TAG, "11-----resetAllTextView-----resetAllTextView-----");
    }

    public void setSilentVolume(){
        mVideoView.setSilentVolume();
    }

    private Uri uri2File2Uri(Uri videoUri) {
        String scheme = videoUri.getScheme();
        String mPathName = null;
        IContentProvider mMediaProvider;
        if (scheme == null) {
            return videoUri;
        }
        String[] streaming = {"rtsp", "http", "https", "windvine", "mmsh", "mmst", "rtmp", "mms", "udp", "hls", "rtp"};
        if (ArrayUtils.contains(streaming, scheme)) {
            //  mPathName = uri.toString();
            mIsNetworkVideo = true;
            return videoUri;

        }
        if (scheme.equals("content")) {
            String path = null;
            path=getFPUriToPath(mContext,videoUri);
            if (path != null) {
                return Uri.fromFile(new File(path));
            } else {
                Log.w(TAG, "************ uri2File2Uri failed ***************");
                return videoUri;
            }

        } else if (scheme.equals("file")) {
            if (realPath == null || realPath.trim().length() == 0) {
                mPathName = videoUri.getPath();
            } else {
                mPathName = realPath;
            }
            Log.v(TAG, "_2Uri___mPathName___" + mPathName);
        }
        Log.v(TAG, "00-----uri2File2Uri-----uri2File2Uri-----");
        File file = new File(mPathName);
        boolean isFolder;
        if (file.exists()) {
            isFolder = file.isDirectory();
            if (isFolder) {
                Log.v(TAG,"isFolder");
                searchBluraySubTitle(mPathName);
            } else {
                searchSubTitle(mPathName);
            }
        }
        return videoUri;

    }
    private static String getFPUriToPath(Context context, Uri uri) {
        try {
            List<PackageInfo> packs = context.getPackageManager().getInstalledPackages(PackageManager.GET_PROVIDERS);
            if (packs != null) {
                String fileProviderClassName = FileProvider.class.getName();
                for (PackageInfo pack : packs) {
                    ProviderInfo[] providers = pack.providers;
                    if (providers != null) {
                        for (ProviderInfo provider : providers) {
                            if (uri.getAuthority().equals(provider.authority)) {
                                if (provider.name.equalsIgnoreCase(fileProviderClassName)) {
                                    Class<FileProvider> fileProviderClass = FileProvider.class;
                                    try {
                                        Method getPathStrategy = fileProviderClass.getDeclaredMethod("getPathStrategy", Context.class, String.class);
                                        getPathStrategy.setAccessible(true);
                                        Object invoke = getPathStrategy.invoke(null, context, uri.getAuthority());
                                        if (invoke != null) {
                                            String PathStrategyStringClass = FileProvider.class.getName() + "$PathStrategy";
                                            Class<?> PathStrategy = Class.forName(PathStrategyStringClass);
                                            Method getFileForUri = PathStrategy.getDeclaredMethod("getFileForUri", Uri.class);
                                            getFileForUri.setAccessible(true);
                                            Object invoke1 = getFileForUri.invoke(invoke, uri);
                                            if (invoke1 instanceof File) {
                                                String filePath = ((File) invoke1).getAbsolutePath();
                                                return filePath;
                                            }
                                        }
                                    } catch (NoSuchMethodException e) {
                                        e.printStackTrace();
                                    } catch (InvocationTargetException e) {
                                        e.printStackTrace();
                                    } catch (IllegalAccessException e) {
                                        e.printStackTrace();
                                    } catch (ClassNotFoundException e) {
                                        e.printStackTrace();
                                    }
                                    break;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    private void searchBluraySubTitle(String mPathName) {
        File directory = new File(mPathName);
        if (directory.isDirectory()) {
            File[] files = directory.listFiles();
            for (File f : files) {
                if (f.exists()) {
                    String fileName = f.getName();
                    int idxA = fileName.indexOf(".");
                    int idxB = fileName.lastIndexOf(".");
                    int idxC = fileName.length();
                    if (idxA > 0 && idxB > 0 && idxC > 0) {
                        String name2 = fileName.substring(idxB, idxC);
                        Log.v(TAG, "name2:"+name2);
                        for (int i = 0; i < SUB_EXTS.length; i++) {
                            if (name2.toLowerCase().equals(SUB_EXTS[i])) {
                                Log.v(TAG, "filePath:"+mPathName + "/" + fileName+"MEDIA_MIMETYPE[i]"+MEDIA_MIMETYPE[i]);
                                mSrtList.add(mPathName + "/" + fileName);
                                mMediaTypeList.add(MEDIA_MIMETYPE[i]);
                            }
                        }
                    }
                }
            }
        }
    }
    private void searchSubTitle(String mPathName) {
        if (mPathName != null && mPathName.length() != 0) {
            Log.v(TAG, "00-----searchSubTitle-----searchSubTitle-----");
            int idx = mPathName.lastIndexOf("."); // position of last .
            int idx1 = mPathName.lastIndexOf("/"); // position of last /
            Log.v(TAG, "__2Uri___idx___" + idx + "___idx1___" + idx1);
            if (idx1 > 0 && idx > 0 && idx > idx1) {
                String folder = mPathName.substring(0, idx1); // storage/emulated/0/DCIM
                // Log.v(TAG, "__2Uri___folder___" + folder);
                String name = mPathName.substring(idx1 + 1, idx); // a
                // Log.v(TAG, "__2Uri___name___" + name);
                File directory = new File(folder);
                if (directory.isDirectory()) {
                    File[] files = directory.listFiles();
                    for (File f : files) {
                        if (f.exists()) {
                            String fileName = f.getName();
                            // Log.v(TAG, "__2Uri___fileName___" + fileName);
                            int idxA = fileName.indexOf(".");
                            int idxB = fileName.lastIndexOf(".");
                            int idxC = fileName.length();
                            // Log.v(TAG, "__2Uri___idxA___" + idxA +
                            // "___idxB___"
                            // + idxB + "___idxC___" + idxC);
                            if (idxA > 0 && idxB > 0 && idxC > 0) {
                                for (; idxA > 0;) {
                                    String name1 = fileName.substring(0, idxA);
                                    // Log.v(TAG, "__2Uri___name1___" + name1);
                                    String name2 = fileName.substring(idxB, idxC);
                                    // Log.v(TAG, "__2Uri___name2___" + name2);
                                    // Log.v(TAG, "__2Uri___SUB_EXTS.length___"
                                    // +
                                    // SUB_EXTS.length);
                                    if (name1.equals(name)) {
                                        for (int i = 0; i < SUB_EXTS.length; i++) {
                                            if (name2.toLowerCase().equals(SUB_EXTS[i])) {
                                                mSrtList.add(folder + "/" + fileName);
                                                Log.e(TAG,"mSrtList:"+folder + "/" + fileName);
                                                mMediaTypeList.add(MEDIA_MIMETYPE[i]);
                                                Log.e(TAG,"MEDIA_MIMETYPE[i]:"+MEDIA_MIMETYPE[i]);
                                            }
                                        }
                                    }
                                    idxA = fileName.indexOf(".", idxA + 1);
                                    // Log.v(TAG, "__2Uri___idxA___" + idxA);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // this function add by liuanlong 14/10/16
    private boolean isOrdinaryMovieFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("avi") || ext.equalsIgnoreCase("wmv")
                    || ext.equalsIgnoreCase("rmvb") || ext.equalsIgnoreCase("mkv")
                    || ext.equalsIgnoreCase("m4v") || ext.equalsIgnoreCase("m1v")
                    || ext.equalsIgnoreCase("mov") || ext.equalsIgnoreCase("mpg")
                    || ext.equalsIgnoreCase("rm") || ext.equalsIgnoreCase("flv")
                    || ext.equalsIgnoreCase("pmp") || ext.equalsIgnoreCase("vob")
                    || ext.equalsIgnoreCase("dat") || ext.equalsIgnoreCase("asf")
                    || ext.equalsIgnoreCase("psr") || ext.equalsIgnoreCase("3gp")
                    || ext.equalsIgnoreCase("mpeg") || ext.equalsIgnoreCase("ram")
                    || ext.equalsIgnoreCase("divx") || ext.equalsIgnoreCase("m4p")
                    || ext.equalsIgnoreCase("m4b") || ext.equalsIgnoreCase("mp4")
                    || ext.equalsIgnoreCase("f4v") || ext.equalsIgnoreCase("3gpp")
                    || ext.equalsIgnoreCase("3g2") || ext.equalsIgnoreCase("3gpp2")
                    || ext.equalsIgnoreCase("webm") || ext.equalsIgnoreCase("ts")
                    || ext.equalsIgnoreCase("tp") || ext.equalsIgnoreCase("m2ts")
                    || ext.equalsIgnoreCase("3dv") || ext.equalsIgnoreCase("3dm")
                    || ext.equalsIgnoreCase("m2v")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }

        return false;
    }

    // this function add by liuanlong 14/10/16
    private boolean isISOFile(String path) {
        try {
            if (path.indexOf("iso") != -1) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }
    public void setIntent(Intent intent) {
        mIntent=intent;
    }
    private void replayVideoDialog() {
        LayoutInflater inflate = (LayoutInflater) mContext
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View replayView = inflate.inflate(R.layout.dialog_replay, null);
        ((Button) replayView.findViewById(R.id.replay_confirm))
                .setOnClickListener(new View.OnClickListener() {
                    public void onClick(View arg0) {
                        resetAllTextView();
                        mHandler.removeMessages(DISMISS_DIALOG);
                        mReplayDialog.dismiss();
                        Boolean i = deleteBookmark();
                        switchSubOn = true;
                        mCurrentSubSave = 0;
                        mCurrentTrackSave = 0;
                        mCurrentSubColorSave = -1;
                        mCurrentSubSizeSave = 32;
                        Log.v(TAG, "___deleteBookmark()_____" + Boolean.toString(i));
                        mVideoView.setVideoURI(mUri);
                        mVideoView.start();
                        setSaves();
                    }
                });
        ((Button) replayView.findViewById(R.id.replay_cancel))
                .setOnClickListener(new View.OnClickListener() {
                    public void onClick(View arg0) {
                        mHandler.removeMessages(DISMISS_DIALOG);
                        mReplayDialog.dismiss();
                    }
                });
        mReplayDialog = new Dialog(mContext, R.style.dialog);
	mReplayDialog.setContentView(replayView);
        mReplayDialog.getWindow().getDecorView()
                .setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide
                        // nav
                        // bar
                        | View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
                );
        Window dialogWindow = mReplayDialog.getWindow();
        WindowManager.LayoutParams lp = dialogWindow.getAttributes();
        dialogWindow.setGravity(Gravity.CENTER);
        WindowManager m = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        Display d = m.getDefaultDisplay();
        lp.width = (int) (d.getWidth() * 0.25);
        lp.height = (int) (d.getHeight() * 0.25);
        dialogWindow.setAttributes(lp);
        mReplayDialog.show();

        mDismissMsg = mHandler.obtainMessage(DISMISS_DIALOG);
        mHandler.sendMessageDelayed(mDismissMsg, DISMISS_DELAY);
    }

    private void initToast() {
        mJumpToast = Toast.makeText(mContext, "", Toast.LENGTH_SHORT);
        mJumpToast.setGravity(Gravity.CENTER, 0, 0);
        mSubtitleToast = new MyToast(mContext);
        mAudioToast = new MyToast(mContext);
        mPrevToast = new MyToast(mContext);
        mNextToast = new MyToast(mContext);
        mStopToast = new MyToast(mContext);
        mRepeatToast = new MyToast(mContext);

        ImageView imageView;
        imageView = (ImageView) mSubtitleToast.getView().findViewById(R.id.message_image);
        imageView.setImageResource(R.drawable.button_subtitle);
        imageView = (ImageView) mAudioToast.getView().findViewById(R.id.message_image);
        imageView.setImageResource(R.drawable.button_track);
        imageView = (ImageView) mPrevToast.getView().findViewById(R.id.message_image);
        imageView.setImageResource(R.drawable.button_prev);
        imageView = (ImageView) mNextToast.getView().findViewById(R.id.message_image);
        imageView.setImageResource(R.drawable.button_next);
        imageView = (ImageView) mStopToast.getView().findViewById(R.id.message_image);
        imageView.setImageResource(R.drawable.button_stop);
        imageView = (ImageView) mRepeatToast.getView().findViewById(R.id.message_image);
        imageView.setImageResource(R.drawable.button_repeat);
        TextView textView;
        textView = (TextView) mPrevToast.getView().findViewById(R.id.message);
        textView.setVisibility(View.GONE);
        textView = (TextView) mPrevToast.getView().findViewById(R.id.message);
        textView.setVisibility(View.GONE);
    }

    @Override
    public void subFocusItems() {
        /* repeat mode */
        mRepeatMode = sp.getInt(EDITOR_MODEREPEAT, 0);

        /* sub gate */
        boolean gate = sp.getBoolean(EDITOR_SUBGATE, true);
        // mVideoView.setSubGate(gate);
        switchSubOn = ((Activity)mContext).isInPictureInPictureMode() ? false : gate;
        /* sub color */
        int[] listColor = mRes.getIntArray(R.array.screen_color_values);
        int clorIndex = sp.getInt(EDITOR_SUBCOLOR, 0);
        // mVideoView.setSubColor(listColor[clorIndex]);
        mCurrentSubColorSave = listColor[clorIndex];

        /* sub char size */
        int[] listCharsize = mRes.getIntArray(R.array.screen_charsize_values);
        int charsizeIndex = sp.getInt(EDITOR_SUBCHARSIZE, 2);
        // mVideoView.setSubFontSize(listCharsize[charsizeIndex]);
        mCurrentSubSizeSave = listCharsize[charsizeIndex];

        /* zoom mode */
        int zoom = sp.getInt(EDITOR_ZOOM, 0);
        mVideoView.setZoomMode(zoom);
        Log.v(TAG, "========subFocusItems()=========");
    }

    private static boolean uriSupportsBookmarks(Uri uri) {
        if (uri.getScheme() == null) {
            return false;
        }
        return ("file".equalsIgnoreCase(uri.getScheme()));
    }

    private int getBookmark() {
        return mBookmarkService.findByPathReturnSeek(mUri.getPath());
    }

    // SAVE sub and track, add by maizirong
    private int getSubSave() {
        return mBookmarkService.findByPathReturnSubSave(mUri.getPath());
    }

    private int getTrackSave() {
        return mBookmarkService.findByPathReturnTrackSave(mUri.getPath());
    }

    // SAVE subColor and subFontSize, add by maizirong
    private int getSubColorSave() {
        return mBookmarkService.findByPathReturnSubColorSave(mUri.getPath());
    }

    private int getSubSizeSave() {
        return mBookmarkService.findByPathReturnSubSizeSave(mUri.getPath());
    }

    private boolean deleteBookmark() {
        return mBookmarkService.delete(mUri.getPath());
    }

    private void setBookmark(int bookmark) {
        if (!uriSupportsBookmarks(mUri)) {
            return;
        }

        String path = mUri.getPath();
        if (mBookmarkService.findByPathReturnSeek(path) != 0) {
            mBookmarkService.update(path, bookmark, mCurrentSubSave, mCurrentTrackSave,
                    mCurrentSubColorSave, mCurrentSubSizeSave);
        } else {
            mBookmarkService.save(path, bookmark, mCurrentSubSave, mCurrentTrackSave,
                    mCurrentSubColorSave, mCurrentSubSizeSave);
        }
    }

    public int getListMode3D (){
        return sp.getInt(EDITOR_MODE2DOR3D, 0);
    }

    @Override
    public void initSubAndTrackInfo() {
        // TODO Auto-generated method stub
        resetAllTextView();
        Log.v(TAG, "initSubAndTrackInfo-----resetAllTextView-----");

        /* get track info */
        mTransformTrack = null;
        TrackInfo[] trackList = mVideoView.getTrackList();
        if (trackList != null) {
            int trackCount = trackList.length;
            mTransformTrack = new String[trackList.length];
            for (int i = 0; i < trackCount; i++) {
                Log.v(TAG, "____Track___" + trackList[i].getLanguage());
                if (trackList[i].getLanguage().length() == 0
                        && trackList[i].getLanguage() != null) {
                    mTransformTrack[i] = new String("Audio Track " + (i + 1));
                } else {
                    mTransformTrack[i] = new String(trackList[i].getLanguage());
                }
            }
        }

        /* get sub info */
        mTransformSubtitle = null;
        TrackInfo[] subList = mVideoView.getSubList();

        if (subList != null) {
            int subCount = subList.length;
            mTransformSubtitle = new String[subList.length];
            for (int i = 0; i < subCount; i++) {
                Log.v(TAG, "___Sub___" + subList[i].getLanguage());
                mTransformSubtitle[i] = new String(subList[i].getLanguage());
            }
        }
    }

    private void sendKeyIntent(int keycode) {
        final int keyCode = keycode;
        // to avoid deadlock, start a thread to perform operations
        Thread sendKeyDelay = new Thread() {
            public void run() {
                try {
                    int count = 1;

                    IWindowManager wm = IWindowManager.Stub.asInterface(ServiceManager
                            .getService("window"));
                    for (int i = 0; i < count; i++) {
                        Thread.sleep(100);
                        long now = SystemClock.uptimeMillis();
                        if (!mOnPause) {
                            KeyEvent keyDown = new KeyEvent(now, now, KeyEvent.ACTION_DOWN,
                                    keyCode, 0);
                            InputManager.getInstance().injectInputEvent(keyDown,
                                    InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
                            // wm.injectKeyEvent(keyDown, false);
                            KeyEvent keyUp = new KeyEvent(now, now, KeyEvent.ACTION_UP, keyCode, 0);
                            InputManager.getInstance().injectInputEvent(keyUp,
                                    InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
                            // wm.injectKeyEvent(keyUp, false);
                        }
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                } /*
                 * catch (RemoteException e) { e.printStackTrace(); }
                 */
            }
        };
        sendKeyDelay.start();
    }

    /* sync with MediaProvider.java of TvdFileManager */
    public static final String RETURN = "../";
    public static final Comparator ALPH = new Comparator<String>() {
        @Override
        public int compare(String arg0, String arg1) {
            File f0 = new File(arg0);
            File f1 = new File(arg1);

            if (arg0.equals(RETURN)) {
                return -1;
            }
            if (arg1.equals(RETURN)) {
                return 1;
            }

            try {
                if (f0.isDirectory() && !f1.isDirectory()) {
                    return -1;
                }
                if (!f0.isDirectory() && f1.isDirectory()) {
                    return 1;
                }
            } catch (SecurityException e) {
                e.printStackTrace();
                return -1;
            }

            try {
                String str0 = arg0.substring(arg0.lastIndexOf("/") + 1);
                String str1 = arg1.substring(arg1.lastIndexOf("/") + 1);
                return str0.compareToIgnoreCase(str1);
            } catch (IndexOutOfBoundsException e) {
                e.printStackTrace();
                return -1;
            }
        }
    };

    private void sorting() {
        Object[] t = mPlayList.toArray();
        Arrays.sort(t, ALPH);
        mPlayList.clear();
        for (Object a : t) {
            mPlayList.add((String) a);
        }
    }

    private void createFolderDispList() {
        String fileNameText;
        String filePathText;
        File filePath;

        String[] fileEndingVideo = mRes.getStringArray(R.array.fileEndingVideo);
        fileNameText = mUri.getPath();
        int index = fileNameText.lastIndexOf('/');
        if (index >= 0) {
            filePathText = fileNameText.substring(0, index);
            filePath = new File(filePathText);
            File[] fileList = filePath.listFiles();
            if (fileList != null && filePath.isDirectory()) {
                for (File currenFile : fileList) {
                    String fileName = currenFile.getName();
                    int indexPoint = fileName.lastIndexOf('.');
                    if (indexPoint > 0 && currenFile.isFile()) {
                        String fileEnd = fileName.substring(indexPoint + 1);
                        for (int i = 0; i < fileEndingVideo.length; i++) {
                            if (fileEnd.equalsIgnoreCase(fileEndingVideo[i])) {
                                mPlayList.add(currenFile.getPath());
                                break;
                            }
                        }
                    }
                }
            }
        }
        Collections.sort(mPlayList);
        sorting();

        /* get current index */
        mCurrentIndex = 0;
        String mCurrentPath = mUri.getPath();
        for (int i = 0; i < mPlayList.size(); i++) {
            if (mCurrentPath.equalsIgnoreCase(mPlayList.get(i))) {
                mCurrentIndex = i;
                break;
            }
        }
    }

    private void createMediaProviderDispList(Uri uri, Context context) {
        Cursor c = null;
        IContentProvider mMediaProvider = context.getContentResolver().acquireProvider("media");
        Uri mVideoUri = Video.Media.getContentUri("external");
        String[] videoProjection = new String[] {Video.Media.DATA};

        /* get playlist */
        try {
            c = mMediaProvider.query(null, mVideoUri, videoProjection, null, null);
        } catch (RemoteException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        if (c != null) {
            try {
                while (c.moveToNext()) {
                    String path = c.getString(0);
                    if (new File(path).exists()) {
                        mPlayList.add(path);
                    }
                }
            } finally {
                c.close();
                c = null;
            }

            /* get current index */
            mCurrentIndex = 0;
            String mCurrentPath = mUri.getPath();
            for (int i = 0; i < mPlayList.size(); i++) {
                if (mCurrentPath.equalsIgnoreCase(mPlayList.get(i))) {
                    mCurrentIndex = i;
                    break;
                }
            }
        }
    }

    private void updateDispList(String mediaPath) {
        int size = mPlayList.size();
        String path;
        if (size > 0) {
            for (int i = size - 1; i >= 0; i--) {
                path = mPlayList.get(i);
                if (path.startsWith(mediaPath)) {
                    mPlayList.remove(i);
                }
            }
        }
    }

    private void setImageButtonListener() {
        String scheme = mUri.getScheme();
        Log.d(TAG,"setImageButtonListener scheme:"+scheme);
        if (scheme != null && scheme.equalsIgnoreCase("file")) {
            mMediaController.setPrevNextListeners(mNextListener, mPrevListener);
        } else {
            mMediaController.setPrevNextListeners(null, null);
        }
        if (mPlayList.size() > 0) {
            mMediaController.setRepeatListener(mRepeatModeListener);
        }
        mMediaController.set3DListener(mMode3DListener);
        mMediaController.setTrackListener(mTrackListener);
        mMediaController.setZoomListener(mZoomListener);
        mMediaController.setSubsetListener(mSubsetListener);
    }

    View.OnClickListener mNextListener = new View.OnClickListener() {
        public void onClick(View v) {
            if(mVideoView.isPreparing() == false) {
                if(TRACE)Log.d(TAG,"mNextListener in");
                playNext();
            } else {
                Log.d("debug","VideoView is prepareing!! can not click next !!");
            }
        }
    };
    View.OnClickListener mPrevListener = new View.OnClickListener() {
        public void onClick(View v) {
            if(mVideoView.isPreparing() == false) {
                if(TRACE)Log.d(TAG,"mPrevListener in");
                playPrev();
            } else {
                Log.d("debug","VideoView is prepareing!! can not click prev !!");
            }
        }
    };

    @Override
    public void onMediaDisplaySettingsClickListener(){
        Log.d(TAG,"startDisplaySettingsMenu");
        mVideoView.startDisplaySettingsMenu();
    }

    @Override
    public void onMediaPrevKeyClickListener() {
        // TODO Auto-generated method stub
        mPrevToast.show();
        playPrev();
    }

    @Override
    public void onMediaNextKeyClickListener() {
        // TODO Auto-generated method stub
        mNextToast.show();
        playNext();
    }

    @Override
    public void onMediaStopKeyClickListener() {
        // TODO Auto-generated method stub
        mStopToast.show();
        sendKeyIntent(KeyEvent.KEYCODE_BACK);
    }

    @Override
    public void onMediaRepeatKeyClickListener() {
        // TODO Auto-generated method stub
        if (mPlayList.size() > 0) {
            String[] listModeRepeat = mRes.getStringArray(R.array.screen_repeat_entries);
            mRepeatMode = (mRepeatMode + 1) % listModeRepeat.length;
            editor.putInt(EDITOR_MODEREPEAT, mRepeatMode);
            editor.commit();

            TextView tv = (TextView) mRepeatToast.getView().findViewById(R.id.message);
            tv.setText(listModeRepeat[mRepeatMode]);
        } else {
            TextView tv = (TextView) mRepeatToast.getView().findViewById(R.id.message);
            tv.setText(R.string.playlist_miss);
        }
        mRepeatToast.show();
    }

    @Override
    public void onMediaSubtitleKeyClickListener() {
        // TODO Auto-generated method stub

        TextView tv = (TextView) mSubtitleToast.getView().findViewById(R.id.message);
        if (mTransformSubtitle != null && mTransformSubtitle.length != 0) {
            int focus;
            int subCount = mTransformSubtitle.length;
            focus = mVideoView.getCurSub();
            String currentSub = tv.getText().toString();
            for (int i = 0; i < mTransformSubtitle.length; i++) {
                if (currentSub.equals(mTransformSubtitle[i])) {
                    focus = i;
                    break;
                }
            }
            focus = (focus + 1) % subCount;
            if (focus < subCount) {
                tv.setText(mTransformSubtitle[focus]);
            }
        } else {
            String[] subList = mRes.getStringArray(R.array.screen_select_entries);
            tv.setText(subList[0]);
        }
        mSubtitleToast.show();
    }

    @Override
    public void onMediaAudioKeyClickListener() {
        // TODO Auto-generated method stub
        TextView tv = (TextView) mAudioToast.getView().findViewById(R.id.message);

        if (mTransformTrack != null) {
            int trackCount = mTransformTrack.length;
            int focus = mVideoView.getCurTrack();

            String currentTrack = tv.getText().toString();
            for (int i = 0; i < mTransformTrack.length; i++) {
                if (currentTrack.equals(mTransformTrack[i])) {
                    focus = i;
                    break;
                }
            }
            focus = (focus + 1) % trackCount;

            if (focus < trackCount) {
                tv.setText(mTransformTrack[focus]);
            }
        } else {
            String[] trackList = mRes.getStringArray(R.array.screen_track_entries);
            tv.setText(trackList[0]);
        }
        mAudioToast.show();
    }

    private void playPrev() {
        Log.d(TAG,"playPrev mIsNetworkVideo:"+mIsNetworkVideo);
        if(mIsNetworkVideo){
            return;
        }
        mMediaController.hide(false);
        int size = mPlayList.size();
        Log.d(TAG,"playPrev mCurrentIndex:"+mCurrentIndex+" ,size:"+size);
        if (mCurrentIndex >= 0 && size > 0) {
            mVideoView.clearCurrentStat();
            if (subTitleInfoOps != null) {
                subTitleInfoOps.removeAllSubTitleInfo();
            }
            Log.v(TAG, "-----playPrev-----mVideoView.clearCurrentStat()");
            if (size == 1) {
                mCurrentIndex = 0;
            } else if(mRepeatMode == 3){
                mCurrentIndex = (int) (Math.random() * size);
            } else if (mCurrentIndex == 0) {
                mCurrentIndex = size - 1;
            } else {
                mCurrentIndex = (mCurrentIndex - 1) % size;
            }
            mUri = Uri.fromFile(new File(mPlayList.get(mCurrentIndex)));
            // modified by liuanlong 14/10/8
            if (realPath == null) { /* ordinary video */
                File file = new File(mUri.getPath());
                boolean isFolder;
                if (file.exists()) {
                    isFolder = file.isDirectory();
                    if (isFolder) {
                        Log.v(TAG,"isFolder");
                        searchBluraySubTitle(mUri.getPath());
                    } else {
                        searchSubTitle(mUri.getPath());
                    }
                }
            } else { /* iso video */
                searchSubTitle(realPath);
            }
            // end
            if (mSrtList != null && mSrtList.size() > 0) {
                mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
            }
            /*
             * add by liuanlong 14/10/21 set listlayout containing list gone
             * when prev or next
             */
            mMediaController.setListLayoutGone();
            playFile();
        }
    }

    private void playNext() {
        Log.d(TAG,"playNext mIsNetworkVideo:"+mIsNetworkVideo);
        if(mIsNetworkVideo){
            return;
        }
        mMediaController.hide(false);
        int size = mPlayList.size();
        Log.d(TAG,"playNext mCurrentIndex:"+mCurrentIndex+" ,size:"+size);
        if (mCurrentIndex >= 0 && size > 0) {
            mVideoView.clearCurrentStat();
            if (subTitleInfoOps != null) {
                subTitleInfoOps.removeAllSubTitleInfo();
            }
            Log.v(TAG, "-----PplayNext-----mVideoView.clearCurrentStat()");
            if(mRepeatMode == 3) {
                // random play
                mCurrentIndex = (int) (Math.random() * size);
            }else{
                mCurrentIndex = (mCurrentIndex + 1) % size;
            }
            mUri = Uri.fromFile(new File(mPlayList.get(mCurrentIndex)));
            // modified by liuanlong 14/10/8
            if (realPath == null) { /* ordinary video */
                File file = new File(mUri.getPath());
                boolean isFolder;
                if (file.exists()) {
                    isFolder = file.isDirectory();
                    if (isFolder) {
                        Log.v(TAG,"isFolder");
                        searchBluraySubTitle(mUri.getPath());
                    } else {
                        searchSubTitle(mUri.getPath());
                    }
                }

            } else { /* iso video */
                searchSubTitle(realPath);
            }
            // end
            Log.v(TAG, "--------playNext-------mUri-" + mUri.getPath());
            if (mSrtList != null && mSrtList.size() > 0) {
                mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
            }
            /*
             * add by liuanlong 14/10/21 set listlayout containing list gone
             * when prev or next
             */
            mMediaController.setListLayoutGone();
            playFile();
        }
    }

    private void playFile() {
        // mWakeLock.acquire();
        Log.d(TAG, "---------- No matter what model it is now,setDisplay to 2D model-----");
        displayManager = DisplayOutputManager.getInstance();
        displayManager.setDisplay3DMode(0, DisplayOutputManager.DISPLAY_2D_ORIGINAL);
        resetAllTextView();
        Log.v(TAG, "playFile-----resetAllTextView-----");
        mHandler.postDelayed(mPlayRunnable, 100);

        // mWakeLock.release();
    }

    View.OnFocusChangeListener mMode3DListener = new View.OnFocusChangeListener() {
        @Override
        public void onFocusChange(View v, boolean hasFocus) {
            // TODO Auto-generated method stub

            if (mMediaController.getMediaControlFocusId() == R.id.mode3D) {
                return;
            }

            if (hasFocus) {
                mMode3DRunnable = new Runnable() {

                    @Override
                    public void run() {
                        // TODO Auto-generated method stub
                        Log.w(TAG, "Mode3D has focused..............................................");
                        int currentMode = getListMode3D();
                        mMediaController.setListViewData(R.id.mode3D, currentMode, mListMode3D,true);
                    }

                };
                mHandler.postDelayed(mMode3DRunnable, 200);
            } else {
                if (mMode3DRunnable != null) {
                    mHandler.removeCallbacks(mMode3DRunnable);
                    mMode3DRunnable = null;
                }
            }
        }
    };

    View.OnFocusChangeListener mRepeatModeListener = new View.OnFocusChangeListener() {
        @Override
        public void onFocusChange(View v, boolean hasFocus) {
            // TODO Auto-generated method stub

            if (mMediaController.getMediaControlFocusId() == R.id.repeat) {
                return;
            }

            if (hasFocus) {
                mRepeatRunnable = new Runnable() {

                    @Override
                    public void run() {
                        // TODO Auto-generated method stub
                        Log.w(TAG, "Mode repeat has focused");
                        int currentMode = sp.getInt(EDITOR_MODEREPEAT, 0);
                        String[] listModeRepeat = mRes
                                .getStringArray(R.array.screen_repeat_entries);
                        mMediaController.setListViewData(R.id.repeat, currentMode, listModeRepeat,true);
                    }
                };
                mHandler.postDelayed(mRepeatRunnable, 200);
            } else {
                if (mRepeatRunnable != null) {
                    mHandler.removeCallbacks(mRepeatRunnable);
                    mRepeatRunnable = null;
                }
            }
        }
    };

    View.OnFocusChangeListener mTrackListener = new View.OnFocusChangeListener() {
        @Override
        public void onFocusChange(View v, boolean hasFocus) {
            // TODO Auto-generated method stub

            if (mMediaController.getMediaControlFocusId() == R.id.track) {
                return;
            }

            if (hasFocus) {
                mTrackRunnable = new Runnable() {

                    @Override
                    public void run() {
                        // TODO Auto-generated method stub
                        if (mTransformTrack != null && mTransformTrack.length != 0) {
                            int currentTrack = mVideoView.getCurTrack();
                            mMediaController.setListViewData(R.id.track, currentTrack,
                                    mTransformTrack,true);
                        } else {
                            mMediaController.setListViewData(R.id.track, 0,
                                    mRes.getStringArray(R.array.screen_track_entries),true);
                        }
                    }
                };
                mHandler.postDelayed(mTrackRunnable, 200);
            } else {
                if (mTrackRunnable != null) {
                    mHandler.removeCallbacks(mTrackRunnable);
                    mTrackRunnable = null;
                }
            }
        }
    };

    View.OnFocusChangeListener mZoomListener = new View.OnFocusChangeListener() {
        @Override
        public void onFocusChange(View v, boolean hasFocus) {
            // TODO Auto-generated method stub

            if (mMediaController.getMediaControlFocusId() == R.id.zoom) {
                return;
            }

            if (hasFocus) {
                mZoomRunnable = new Runnable() {

                    @Override
                    public void run() {
                        // TODO Auto-generated method stub
                        int currentMode = sp.getInt(EDITOR_ZOOM, 0);
                        String[] listModeZoom;
                        if(SystemProperties.get("ro.config.low_ram", "false").equals("true")){
                            listModeZoom = mRes.getStringArray(R.array.screen_zoom_entries_lowmemory);
                        }else{
                            listModeZoom = mRes.getStringArray(R.array.screen_zoom_entries);
                        }
                        mMediaController.setListViewData(R.id.zoom, currentMode, listModeZoom,true);
                    }
                };
                mHandler.postDelayed(mZoomRunnable, 200);
            } else {
                if (mZoomRunnable != null) {
                    mHandler.removeCallbacks(mZoomRunnable);
                    mZoomRunnable = null;
                }
            }
        }
    };

    View.OnFocusChangeListener mSubsetListener = new View.OnFocusChangeListener() {
        @Override
        public void onFocusChange(View v, boolean hasFocus) {
            // TODO Auto-generated method stub

            if (mMediaController.getMediaControlFocusId() == R.id.subset) {
                return;
            }

            if (hasFocus) {
                mSubsetRunnable = new Runnable() {

                    @Override
                    public void run() {
                        // TODO Auto-generated method stub
                        String[] listSubset = mRes.getStringArray(R.array.screen_subset_entries);
                        mMediaController.setListViewData(R.id.subset, 0, listSubset,true);
                    }
                };
                mHandler.postDelayed(mSubsetRunnable, 200);
            } else {
                if (mSubsetRunnable != null) {
                    mHandler.removeCallbacks(mSubsetRunnable);
                    mSubsetRunnable = null;
                }
            }
        }
    };

    public void onCompletion(MediaPlayer arg0) {
        mIsDoubleStream = false;
        onCompletion();
    }

    public void onCompletion() {
        mVideoView.suspend();
        deleteBookmark(); // when complet play should delete this bookmark;
        // TODO Auto-generated method stub
        mVideoView.setOnErrorListener(this);
        int size = mPlayList.size();
        Log.i(TAG, "************************ in onCompletion, mToQuit: " + mToQuit
                + ", mPlayList size: " + size + ", mRepeatMode" + mRepeatMode);
        if (mCurrentIndex >= 0 && size > 0 && !mIsNetworkVideo) {
            switch (mRepeatMode) {
                case 0: // repeat all
                    mCurrentIndex = (mCurrentIndex + 1) % size;
                    break;
                case 1: // sequence play
                    if (mCurrentIndex + 1 < size) {
                        mCurrentIndex++;
                    } else {
                        mToQuit = true;
                        return;
                    }
                    break;
                case 2: // repeat one
                    break;
                case 3: // random play
                    mCurrentIndex = (int) (Math.random() * size);
                    break;
                default:
                    break;
            }

            mVideoView.clearCurrentStat();
            mMediaController.restoreUI();
            Log.v(TAG, "-----onCompletion-----mVideoView.clearCurrentStat()");
            File nextFile = new File(mPlayList.get(mCurrentIndex));
            if (!nextFile.exists()) {
                mToQuit = true;
            } else {
                mUri = Uri.fromFile(nextFile);
                if (realPath == null) { // normal
                    File file = new File(mUri.getPath());
                    boolean isFolder;
                    if (file.exists()) {
                        isFolder = file.isDirectory();
                        if (isFolder) {
                            Log.v(TAG,"isFolder");
                            searchBluraySubTitle(mUri.getPath());
                        } else {
                            searchSubTitle(mUri.getPath());
                        }
                    }

                } else { // BD
                    searchSubTitle(realPath);
                }
                if (mSrtList != null && mSrtList.size() > 0) {
                    mVideoView.setTimedTextPath(mSrtList, mMediaTypeList);
                }
                /*
                 * add by liuanlong 15/01/07 set listlayout containing list gone
                 * when >>>
                 */
                mMediaController.setListLayoutGone();
                /* <<<end */
                playFile();
            }
        }

        if (size == 0 && mFinishOnCompletion) {
            mToQuit = true;
        }
    }

    public void onPause() {
        if(TRACE) Log.v(TAG, "onPause in");
        mOnPause = true;

        if(isBroadcastReceiverRegistered){ //这个一定要判断才能unregister，不然会crash
            mContext.unregisterReceiver(mBroadcastReceiver);
            isBroadcastReceiverRegistered = false;
        }

        mVideoPosition = mVideoView.getCurrentPosition();
        int duration = mVideoView.getDuration();
        if (subTitleInfoOps != null) {
            subTitleInfoOps.removeAllSubTitleInfo();
        }

        // current time > 10s and save current position
        if (mVideoPosition > 10 * 1000 && duration - mVideoPosition > 10 * 1000 &&!((Activity)mContext).isInPictureInPictureMode()) {
            mVideoView.suspend();
            deleteBookmark();
            setBookmark(mVideoPosition - 3 * 1000);
        } else {
            deleteBookmark();
        }

        mHandler.removeCallbacksAndMessages(null);
        //如果是在PIP模式，就不要对videoView进行操作
        if(!((Activity)mContext).isInPictureInPictureMode()){
            // current time > 10s and save current position
            if (mVideoPosition > 10 * 1000 && duration - mVideoPosition > 10 * 1000) {
                mVideoView.suspend();
                deleteBookmark();
                setBookmark(mVideoPosition - 3 * 1000);
            } else {
                mVideoView.suspend();
                deleteBookmark();
            }
        }
        if(mReplayDialog!=null){
            mReplayDialog.dismiss();
        }
        // when pause, reset to normal 2d
        displayManager = DisplayOutputManager.getInstance();
        displayManager.setDisplay3DMode(0, DisplayOutputManager.DISPLAY_2D_ORIGINAL);
        mIsDoubleStream = false;
    }

    public void onResume() {
        if(TRACE) Log.v(TAG, "onResume in");
        resetAllTextView();
        if (mOnPause) {
            if(!isActivityOutFromPIPMode){
                mVideoView.resume();
                mVideoView.seekTo(mVideoPosition);
            }else{
                boolean gate = sp.getBoolean(EDITOR_SUBGATE, true);
                if(TRACE) Log.v(TAG, "=====gate==="+gate);
                if(gate)
                    switchSubOn = true;
                mUri = uri2File2Uri(mIntent.getData());
                playFile();
            }
            mOnPause = false;
        }

        /* receive the device eject message */
        mBroadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                // TODO Auto-generated method stub
                final String action = intent.getAction();
                final String mediaPath = intent.getData().getPath();
                if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
                    String path = mUri.getPath();
                    Log.i(TAG, "*********** media eject **********");
                    /* the current video's media was eject */
                    if (path.startsWith(mediaPath)) {
                        Toast toast = Toast.makeText(mContext, R.string.play_finish,
                                Toast.LENGTH_SHORT);
                        toast.setGravity(Gravity.CENTER, 0, 0);
                        toast.show();
                        onCompletion();
                    } else if (mPlayList.size() > 0) {
                        updateDispList(mediaPath);
                    }
                }
            }
        };

        IntentFilter iFilter = new IntentFilter();
        iFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        iFilter.addDataScheme("file");
        mContext.registerReceiver(mBroadcastReceiver, iFilter);
        isBroadcastReceiverRegistered = true;
        setSaves();
    }

    public void setSaves() {
        if(TRACE) Log.v(TAG, "setSaves in");
        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                Log.v(TAG, "___run__" + Boolean.toString(mVideoView.isPlaying()));

                mVideoView.switchSub(mCurrentSubSave);
                mVideoView.switchTrack(mCurrentTrackSave);
                // mVideoView.setSubColor(mCurrentSubColorSave);
                // mVideoView.setSubFontSize(mCurrentSubSizeSave);

                Log.v(TAG, "___setSaves___mCurrentSubSave:" + mCurrentSubSave
                        + "___mCurrentTrackSave" + mCurrentTrackSave + "___mCurrentSubColorSave"
                        + mCurrentSubColorSave + "___mCurrentSubSizeSave" + mCurrentSubSizeSave);
            }
        }, 0);
    }

    public void onStop() {
        if(TRACE) Log.d(TAG, "on Stop");
    }

    public void onDestroy() {
        mVideoView.stopPlayback();
        mBookmarkService.close();
        editor.remove(EDITOR_MODE2DOR3D);
        editor.commit();
    }

    @Override
    public boolean onError(MediaPlayer arg0, int arg1, int arg2) {
        // TODO Auto-generated method stub
        mHandler.removeCallbacksAndMessages(null);
        mProgressView.setVisibility(View.GONE);
        //mToQuit = true;

        return false;
    }

    public boolean toQuit() {
        return mToQuit;
    }

    @Override
    public void onListDataChangedListener(int mediaControlFocusId, int selectedIterm) {
        // TODO Auto-generated method stub
        if(TRACE) Log.v(TAG, "onListDataChangedListener mediaControlFocusId="+mediaControlFocusId+",selectedIterm="+selectedIterm);
        switch (mediaControlFocusId) {
            case R.id.mode3D:
                if (selectedIterm > 1) {
                    setSublistData(selectedIterm);
                } else {
                    mMediaController.setGridViewData(selectedIterm, 0);
                }
                break;
            case R.id.repeat:
                mRepeatMode = selectedIterm;
                editor.putInt(EDITOR_MODEREPEAT, selectedIterm);
                editor.commit();

                break;
            case R.id.track:
                if (selectedIterm == mVideoView.getCurTrack()) {
                    break;
                }
                if (mVideoView.switchTrack(selectedIterm) != 0) {
                    Log.w(TAG, "*********** change the sub track failed !");
                } else {
                    mCurrentTrackSave = selectedIterm;
                }

                break;
            case R.id.zoom:
                if (selectedIterm == mVideoView.getZoomMode()) {
                    break;
                }
                mVideoView.setZoomMode(selectedIterm);
                if(selectedIterm==PICTURE_IN_PICTURE_ITEM){
                    mMediaController.setHolding(false);
                    switchSubOn = false;
                    subTitleClear();
                }else{ //画中画模式不要去记录数据库，不然下次重新进入会直接进到PIP
                    editor.putInt(EDITOR_ZOOM, selectedIterm);
                    editor.commit();
                }
                break;
            case R.id.subset:
                setSublistData(selectedIterm);

                break;
            default:
                break;
        }
    }
    public class SubTitleInfo {
        String text;
        int id;
        int hideSubFlag;
        int subDispPos;
        Rect textScreenBound;
        Rect textBound;
        List<TimedText.Style> styleList;
        int textViewID;
    }

    public class TextViewInfo {
        TextView textView;
        boolean used;
        String text;
    }

    public class SubTitleInfoOps {
        private List<SubTitleInfo> mSubTitleInfoList = null;

        public SubTitleInfoOps() {
            mSubTitleInfoList = new ArrayList<SubTitleInfo>();
        }

        public void addSubTitleInfo(SubTitleInfo subTitleInfo) {
            mSubTitleInfoList.add(subTitleInfo);
        }

        public void removeSubTitleInfo(SubTitleInfo subTitleInfo) {
            for (int i = 0; i < mSubTitleInfoList.size(); i++) {
                if (mSubTitleInfoList.get(i).id == subTitleInfo.id) {
                    mTextViewInfo[mSubTitleInfoList.get(i).textViewID].used = false;
                    mTextViewInfo[mSubTitleInfoList.get(i).textViewID].text = null;
                    subTitleNoDraw();
                    mSubTitleInfoList.remove(mSubTitleInfoList.get(i));
                }
            }
        }

        public void removeAllSubTitleInfo() {
            mSubTitleInfoList.clear();
        }

        public int getNumOfSubTitle() {
            return mSubTitleInfoList.size();
        }

        public SubTitleInfo getSubTitleInfo(int index) {
            return mSubTitleInfoList.get(index);
        }
    }

    @Override
    public void onTimedText(MediaPlayer mp, TimedText text) {
        if(!switchSubOn){
            subTitleClear();
            return;
        }
        if (text != null) {
            mBitmapSubtitleFlag = text.AWExtend_getBitmapSubtitleFlag();
            if (mBitmapSubtitleFlag == 0) {
                SubTitleInfo subTitleInfo = new SubTitleInfo();
                subTitleInfo.text = text.getText();
                // Log.d(TAG,"text = " + subTitleInfo.text);
                subTitleInfo.id = text.AWExtend_getSubtitleID();
                subTitleInfo.hideSubFlag = text.AWExtend_getHideSubFlag();
                subTitleInfo.subDispPos = text.AWExtend_getSubDispPos();
                subTitleInfo.textScreenBound = text.AWExtend_getTextScreenBounds();
                subTitleInfo.textBound = text.getBounds();
                subTitleInfo.styleList = text.AWExtend_getStyleList();

                if (subTitleInfo.text != null) {
                    for (int i = 0; i < subTitleInfoOps.getNumOfSubTitle(); i++) {
                        if ((subTitleInfoOps.getSubTitleInfo(i).textBound != null
                                && subTitleInfo.textBound != null
                                && subTitleInfoOps.getSubTitleInfo(i).textBound.left
                                == subTitleInfo.textBound.left
                                && subTitleInfoOps.getSubTitleInfo(i).textBound.right
                                == subTitleInfo.textBound.right
                                && subTitleInfoOps.getSubTitleInfo(i).textBound.bottom
                                == subTitleInfo.textBound.bottom && subTitleInfoOps
                                .getSubTitleInfo(i).textBound.top
                                == subTitleInfo.textBound.top)
                                || (subTitleInfoOps.getSubTitleInfo(i).textBound == null
                                && subTitleInfo.textBound == null
                                && subTitleInfoOps
                                .getSubTitleInfo(i).subDispPos
                                == subTitleInfo.subDispPos)) {
                            Log.d(TAG, "two subtitles with same position !!!");
                            subTitleInfo.text = subTitleInfoOps.getSubTitleInfo(i).text + "\n\r"
                                    + subTitleInfo.text;
                            subTitleInfoOps.removeSubTitleInfo(subTitleInfoOps.getSubTitleInfo(i));
                        }
                    }
                    for (int i = 0; i < mTextViewInfo.length; i++) {
                        if (!mTextViewInfo[i].used) {
                            subTitleInfo.textViewID = i;
                            mTextViewInfo[i].used = true;
                            if (switchSubOn) {
                                mTextViewInfo[i].text = subTitleInfo.text;
                            }

                            subTitleDraw(i, subTitleInfo.subDispPos, subTitleInfo.textScreenBound,
                                    subTitleInfo.textBound, subTitleInfo.styleList);
                            break;
                        }
                    }
                    subTitleInfoOps.addSubTitleInfo(subTitleInfo);
                } else if (subTitleInfo.text == null) {
                    subTitleInfoOps.removeSubTitleInfo(subTitleInfo);
                }
            } else if (mBitmapSubtitleFlag == 1) {
                // Log.d(TAG,"======================== bit map!!!!");
                mBitmap = text.AWExtend_getBitmap();
                mReferenceVideoWidth = text.AWExtend_getReferenceVideoWidth();
                mReferenceVideoHeight = text.AWExtend_getReferenceVideoHeight();
                // Log.d(TAG,"ref 1  = " + mReferenceVideoWidth +
                // ", 2  = " +
                // mReferenceVideoHeight);
                // Log.d(TAG,"scr 1  = " + mScreenWidth+ ", 2  = " +
                // mScreenHeight);
                int width = mBitmap.getWidth();
                int height = mBitmap.getHeight();
                float widthratio = 0;
                float heightratio = 0;
                if (mReferenceVideoWidth > 0 && mReferenceVideoHeight > 0 && mScreenWidth > 0
                        && mScreenHeight > 0) {
                    widthratio = (float) mScreenWidth / (float) mReferenceVideoWidth;
                    heightratio = (float) mScreenHeight / (float) mReferenceVideoHeight;
                }
                // Log.d(TAG,"ratio 1 = " + widthratio + " , 2 = " +
                // heightratio);

                Matrix matrix = new Matrix();
                matrix.postScale(widthratio, heightratio);

                mBitmap = Bitmap.createBitmap(mBitmap, 0, 0, width, height, matrix, true);

                if (mBitmap != null) {
                    mHandler.removeCallbacks(mUpdateImageView);
                    mHandler.post(mUpdateImageView);
                }
            }
        } else {
            subTitleClear();
        }
    }

    public void subTitleClear() {
        subTitleInfoOps.removeAllSubTitleInfo();
        for (int i = 0; i < mTextViewInfo.length; i++) {
            if (mTextViewInfo[i].used) {
                mTextViewInfo[i].used = false;
                mTextViewInfo[i].text = null;
                subTitleNoDraw();
            }
        }
        mBitmap = null;
        mHandler.removeCallbacks(mUpdateImageView);
        mHandler.post(mUpdateImageView);
    }

    private void subTitleDraw(int numOfTextViewDrawed, int subDispPos, Rect screenBound,
                              Rect textBound, List<TimedText.Style> styleList) {
        /*
         * SUB_DISPPOS_DEFAULT SUB_DISPPOS_BOT_LEFT SUB_DISPPOS_BOT_MID
         * SUB_DISPPOS_BOT_RIGHT SUB_DISPPOS_MID_LEFT SUB_DISPPOS_MID_MID
         * SUB_DISPPOS_MID_RIGHT SUB_DISPPOS_TOP_LEFT SUB_DISPPOS_TOP_MID
         * SUB_DISPPOS_TOP_RIGHT
         */
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        int screenleft = 0;
        int screentop = 0;
        int screenright = 0;
        int screenbottom = 0;
        int textleft = 0;
        int texttop = 0;
        int textright = 0;
        int textbottom = 0;
        boolean isBold = false;
        boolean isItalic = false;
        boolean isUnderlined = false;
        int fontSize = 40;
        int colorRGBA = -1;
        float zoomWidth = 1;
        float zoomHeight = 1;

        if (screenBound != null && textBound != null) {
            screenleft = screenBound.left;
            screentop = screenBound.top;
            screenright = screenBound.right;
            screenbottom = screenBound.bottom;
            if ((screenright - screenleft) > 0 && (screenbottom - screentop) > 0) {
                /*
                 * Log.d(TAG, "0...screenleft:" + screenleft +
                 * " screentop:" + screentop + " screenright:" + screenright +
                 * " screenbottom:" + screenbottom + " mScreenWidth:" +
                 * mScreenWidth + " mScreenHeight:" + mScreenHeight);
                 */
                zoomWidth = (float) mScreenWidth / (float) (screenright - screenleft);
                zoomHeight = (float) mScreenHeight / (float) (screenbottom - screentop);
                // Log.d(TAG, "==0==zoomwidth=" + zoomWidth);
                // Log.d(TAG, "==0==zoomwheight=" + zoomHeight);

                if (zoomWidth == 0) {
                    zoomWidth = (float) 1.0;
                    // Log.d(TAG, "==00==zoomidth=" + zoomWidth);
                }
                if (zoomHeight == 0) {
                    zoomHeight = (float) 1.0;
                    // Log.d(TAG, "==00==zooheight=" + zoomHeight);
                }
                // Log.v(TAG, "==1==zoom=" + zoom);
                screenleft = (int) (screenleft * zoomWidth);
                screentop = (int) (screentop * zoomHeight);
                screenright = (int) (screenright * zoomWidth);
                screenbottom = (int) (screenbottom * zoomHeight);
                /*
                 * Log.d(TAG, "1...screenleft:" + screenleft +
                 * " screentop:" + screentop + " screenright:" + screenright +
                 * " screenbottom:" + screenbottom);
                 */
                // Log.d(TAG,"@@1 " + textBound.left + ", 2 " +
                // textBound.top
                // + ", 3 " + textBound.right + ", 4 " + textBound.bottom);
                textleft = (int) (textBound.left * zoomWidth);
                texttop = (int) (textBound.top * zoomHeight);
                textright = (int) (textBound.right * zoomWidth);
                textbottom = (int) (textBound.bottom * zoomHeight);

                // Log.d(TAG, "2...textleft:" + textleft + " texttop:" +
                // texttop
                // + " textright:" + textright + " textbottom:" + textbottom);
            }
        }
        // Log.d(TAG,"subDispPos = " + subDispPos);
        if (subDispPos == TimedText.SUB_DISPPOS_DEFAULT) {
            params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
            params.setMargins(0, 0, 0, 60);

        } else if (subDispPos == TimedText.SUB_DISPPOS_BOT_LEFT) {
            params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
            params.setMargins(textleft, 0, 0, screenbottom - textbottom);
        } else if (subDispPos == TimedText.SUB_DISPPOS_BOT_MID) {
            params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
            params.setMargins(0, 0, 0, screenbottom - textbottom);
        } else if (subDispPos == TimedText.SUB_DISPPOS_BOT_RIGHT) {
            params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
            params.setMargins(0, 0, screenright - textright, screenbottom - textbottom);
        } else if (subDispPos == TimedText.SUB_DISPPOS_MID_LEFT) {
            params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.CENTER_VERTICAL, RelativeLayout.TRUE);
            params.setMargins(textleft, 0, 0, 0);
        } else if (subDispPos == TimedText.SUB_DISPPOS_MID_MID) {
            params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.CENTER_VERTICAL, RelativeLayout.TRUE);
            params.setMargins(0, 0, 0, 0);
        } else if (subDispPos == TimedText.SUB_DISPPOS_MID_RIGHT) {
            params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.CENTER_VERTICAL, RelativeLayout.TRUE);
            params.setMargins(0, 0, screenright - textright, 0);
        } else if (subDispPos == TimedText.SUB_DISPPOS_TOP_LEFT) {
            params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
            params.setMargins(textleft, texttop, 0, 0);
        } else if (subDispPos == TimedText.SUB_DISPPOS_TOP_MID) {
            params.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
            params.setMargins(0, texttop, 0, 0);
        } else if (subDispPos == TimedText.SUB_DISPPOS_TOP_RIGHT) {
            params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
            params.setMargins(0, texttop, screenright - textright, 0);
        }
        mTextViewInfo[numOfTextViewDrawed].textView.setLayoutParams(params);

        colorRGBA = mCurrentSubColorSave;
        fontSize = mCurrentSubSizeSave;

        mTextViewInfo[numOfTextViewDrawed].textView.setTextColor(colorRGBA);
        mTextViewInfo[numOfTextViewDrawed].textView.setTextSize(fontSize);
        // Log.v(TAG, "-------------zoom-------------" + zoom);
        Log.v(TAG, "-------------getTextSize-------------"
                + mTextViewInfo[numOfTextViewDrawed].textView.getTextSize());
        if (isBold && !isItalic) {
            mTextViewInfo[numOfTextViewDrawed].textView.setTypeface(Typeface.create(Typeface.SERIF,
                    Typeface.BOLD));
        } else if (isItalic && !isBold) {
            mTextViewInfo[numOfTextViewDrawed].textView.setTypeface(Typeface.create(Typeface.SERIF,
                    Typeface.ITALIC));
        } else if (isItalic && isBold) {
            mTextViewInfo[numOfTextViewDrawed].textView.setTypeface(Typeface.create(Typeface.SERIF,
                    Typeface.BOLD_ITALIC));
        } else {
            mTextViewInfo[numOfTextViewDrawed].textView.setTypeface(Typeface.create(Typeface.SERIF,
                    Typeface.NORMAL));
        }
        mSubTitleHandler.sendEmptyMessage(TEXTVIEW_UPDATE);
    }

    private void subTitleNoDraw() {
        mSubTitleHandler.sendEmptyMessage(TEXTVIEW_UPDATE);
    }

    private void setSublistData(int selectedIterm) {
        switch (selectedIterm) {
            case 0:
                // set sub gate
                if (mMediaController.getMediaControlFocusId() == R.id.subset) {
                    boolean gate = sp.getBoolean(EDITOR_SUBGATE, true);
                    Log.v(TAG, "__gate__" + gate);
                    int focus = gate ? 0 : 1;
                    Log.v(TAG, "__focus__" + focus);
                    String[] listGate = mRes.getStringArray(R.array.screen_gate_entries);
                    mMediaController.setSublistViewData(0, focus, listGate);
                }
                break;
            case 1:
                // set sub select
                if (mMediaController.getMediaControlFocusId() == R.id.subset) {
                    int focus = 0;
                    if (mTransformSubtitle != null && mTransformSubtitle.length != 0) {
                        int subCount = mTransformSubtitle.length;
                        focus = mVideoView.getCurSub();
                        mMediaController.setSublistViewData(1, focus, mTransformSubtitle);
                    } else {
                        String[] transformSub = mRes.getStringArray(R.array.screen_select_entries);
                        mMediaController.setSublistViewData(1, focus, transformSub);
                    }
                }
                break;
            case 2:
                // set sub color
                if (mMediaController.getMediaControlFocusId() == R.id.subset) {
                    int focus = sp.getInt(EDITOR_SUBCOLOR, 0);
                    String[] listColor = mRes.getStringArray(R.array.screen_color_entries);
                    mMediaController.setSublistViewData(2, focus, listColor);
                } else if (mMediaController.getMediaControlFocusId() == R.id.mode3D) {
                    /* set the 3D anaglagh */
                    int[] listAnaglaghValue = mRes.getIntArray(R.array.screen_3d_anaglagh_values);
                    int currentAnaglagh = 0; // mVideoView.getAnaglaghType();
                    int focus = 0;
                    for (int i = 0; i < listAnaglaghValue.length; i++) {
                        if (currentAnaglagh == listAnaglaghValue[i]) {
                            focus = i;
                            break;
                        }
                    }
                    String[] listAnaglaghEntry = mRes
                            .getStringArray(R.array.screen_3d_anaglagh_entries);
                    mMediaController.setSublistViewData(2, focus, listAnaglaghEntry);
                }

                break;
            case 3:
                // set sub charsize
                if (mMediaController.getMediaControlFocusId() == R.id.subset) {
                    int focus = sp.getInt(EDITOR_SUBCHARSIZE, 2);
                    String[] listcharsize = mRes.getStringArray(R.array.screen_charsize_entries);
                    mMediaController.setSublistViewData(3, focus, listcharsize);
                } else if (mMediaController.getMediaControlFocusId() == R.id.mode3D) {
                    /* set the 3D anaglagh */
                    int[] listAnaglaghValue = mRes.getIntArray(R.array.screen_3d_anaglagh_values);
                    int currentAnaglagh = 0; // mVideoView.getAnaglaghType();
                    int focus = 0;
                    for (int i = 0; i < listAnaglaghValue.length; i++) {
                        if (currentAnaglagh == listAnaglaghValue[i]) {
                            focus = i;
                            break;
                        }
                    }
                    String[] listAnaglaghEntry = mRes
                            .getStringArray(R.array.screen_3d_anaglagh_entries);
                    mMediaController.setSublistViewData(3, focus, listAnaglaghEntry);
                }

                break;
            case 4: // set sub charset
                int focus = 0;
/*                String currentCharset = mVideoView.getSubCharset();
                // Log.d(TAG,"============================ currentCharset = "
                // +
                // currentCharset);
                String[] charsetValue = mRes.getStringArray(R.array.screen_charset_values);
                String[] listCharset = mRes.getStringArray(R.array.screen_charset_entries);
                for (int i = 0; i < charsetValue.length; i++) {
                    if (currentCharset.equalsIgnoreCase(charsetValue[i])) {
                        focus = i;
                        break;
                    }
                }
                mMediaController.setSublistViewData(4, focus, listCharset);*/

                break;
            case 5: // set sub delay
                int focus1 = 3;
/*                int subDelay = mVideoView.getSubDelay() / 1000;
                int[] listValue = mRes.getIntArray(R.array.screen_delay_values);
                for (int i = 0; i < listValue.length; i++) {
                    if (subDelay == listValue[i]) {
                        focus1 = i;
                        break;
                    }
                }
                String[] listDelay = mRes.getStringArray(R.array.screen_delay_entries);
                mMediaController.setSublistViewData(5, focus1, listDelay);*/

                break;
            case 6:
                String[] listOffset = mRes.getStringArray(R.array.screen_offset_entries);
                mMediaController.setSublistViewData(6, 0, listOffset);

                break;
            default:
                break;
        }
    }

    @Override
    public void onSublistDataChangedListener(int listFoucsIndex, int selectedIterm) {
        // TODO Auto-generated method stub
        if (mMediaController.getMediaControlFocusId() == R.id.subset) {
            setSubsetListData(listFoucsIndex, selectedIterm);
        } else if (mMediaController.getMediaControlFocusId() == R.id.mode3D) {

            setMode3DListData(listFoucsIndex, selectedIterm);
        }
    }

    private void setSubsetListData(int listFoucsIndex, int selectedIterm) {
        switch (listFoucsIndex) {
            case 0: // set subgate
                if (selectedIterm == 0) {
                    switchSubOn = true;
                } else {
                    switchSubOn = false;
                }
                // mVideoView.setSubGate(switchOn);

                editor.putBoolean(EDITOR_SUBGATE, switchSubOn);
                editor.commit();

                break;
            case 1: // set sub select
                if (mVideoView.switchSub(selectedIterm) != 0) {
                    Log.w(TAG, "*********** change the sub select failed !");
                } else {
                    mCurrentSubSave = selectedIterm;
                    Log.v(TAG,
                            "______setSubsetListData_mCurrentSubSave:______"
                                    + Integer.toString(selectedIterm));
                }
                editor.putInt(EDITOR_SUBSELECT, selectedIterm);
                editor.commit();

                break;
            case 2: // set sub color
                Log.v(TAG, "set_sub_color________" + Boolean.toString(mVideoView.isPlaying()));
                int[] listColor = mRes.getIntArray(R.array.screen_color_values);

                mCurrentSubColorSave = listColor[selectedIterm];
                setSubColor(mCurrentSubColorSave);
                Log.v(TAG,
                        "______setSubsetListData_mCurrentSubColorSave:______"
                                + Integer.toString(listColor[selectedIterm]));
                editor.putInt(EDITOR_SUBCOLOR, selectedIterm);
                editor.commit();

                break;
            case 3: // set sub charsize
                int[] listCharsize = mRes.getIntArray(R.array.screen_charsize_values);

                mCurrentSubSizeSave = listCharsize[selectedIterm];
                setSubFontSize(mCurrentSubSizeSave);
                Log.v(TAG,
                        "______setSubsetListData_mCurrentSubSizeSave:______"
                                + Integer.toString(listCharsize[selectedIterm]));
                editor.putInt(EDITOR_SUBCHARSIZE, selectedIterm);
                editor.commit();

                break;
            case 4: // set sub charset
//                String[] listCharSet = mRes.getStringArray(R.array.screen_charset_values);
//                if (mVideoView.setSubCharset(listCharSet[selectedIterm]) != 0) {
//                    Log.w(TAG, "*********** change the sub charset failed !");
//                } else {
//                    editor.putInt(EDITOR_SUBCHARSET, selectedIterm);
//                    editor.commit();
//                }

                break;
            case 5: // set sub delay
/*                int[] listDelay = mRes.getIntArray(R.array.screen_delay_values);
                if (mVideoView.setSubDelay(listDelay[selectedIterm]) != 0) {
                    Log.w(TAG, "*********** change the sub delay failed !");
                }
*/
                break;
            case 6: // set sub offset
                /*
                 * int[] listOffset =
                 * mRes.getIntArray(R.array.screen_offset_values); if
                 * (mVideoView.setSubPosition(listOffset[selectedIterm]) != 0) {
                 * Log.w(TAG, "*********** change the sub offset failed !"); }
                 */

                break;
            default:
                break;
        }
    }

    private void setSubFontSize(int size) {
        for (int i = 0; i < mTextViewInfo.length; i++) {
            TextView view = mTextViewInfo[i].textView;
            if (view != null) {
                view.setTextSize(size);
            }
        }
    }

    private void setSubColor(int color) {
        for (int i = 0; i < mTextViewInfo.length; i++) {
            TextView view = mTextViewInfo[i].textView;
            if (view != null) {
                view.setTextColor(color);
            }
        }
    }

    private void setMode3DListData(int listFoucsIndex, int selectedIterm) {
        Log.d(TAG,"setMode3DListData listFoucsIndex:"+listFoucsIndex+" ,selectedIterm:"+selectedIterm);
        if (listFoucsIndex >= 2) {
            switch (selectedIterm) {
                case 0:
                    // mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_BLUE);
                    break;
                case 1:
                    // mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_GREEN);
                    break;
                case 2:
                    // mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_CYAN);
                    break;
                case 3:
                    // mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_COLOR);
                    break;
                case 4:
                    // mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_HALF_COLOR);
                    break;
                case 5:
                    // mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_OPTIMIZED);
                    break;
                case 6:
                    // mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_YELLOW_BLUE);
                    break;
                default:
                    break;
            }
            // mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
        } else { // 2D | 3D
            displayManager = DisplayOutputManager.getInstance();
            if (listFoucsIndex == 0) { // 2D
                Log.d(TAG, "-------2D---");
                displayManager.setDisplay3DLayerOffset(0, 0);
                editor.putInt(EDITOR_MODE2DOR3D, 0);
                editor.commit();
                switch (selectedIterm) {
                    case 0:
                        displayManager.setDisplay3DMode(0,
                                DisplayOutputManager.DISPLAY_2D_LEFT);
                        if(TRACE)Log.d(TAG,"DISPLAY_2D_LEFT");
                        break;
                    case 1:
                        displayManager.setDisplay3DMode(0,
                                DisplayOutputManager.DISPLAY_2D_TOP);
                        if(TRACE)Log.d(TAG,"DISPLAY_2D_TOP");
                        break;
                    case 2:
                        mVideoView.setZoomMode(0);
                        editor.putInt(EDITOR_ZOOM, 0);
                        editor.commit();
                        if (mIsDoubleStream) {
                            displayManager.setDisplay3DMode(0,
                                    DisplayOutputManager.DISPLAY_2D_DUAL_STREAM);
                            if(TRACE)Log.d(TAG,"DISPLAY_2D_DUAL_STREAM");
                        } else {
                            displayManager.setDisplay3DMode(0,
                                    DisplayOutputManager.DISPLAY_2D_ORIGINAL);
                            if(TRACE)Log.d(TAG,"DISPLAY_2D_ORIGINAL");
                        }
                        break;
                    default:
                        break;
                }
            } else if (listFoucsIndex == 1) { // 3D
                // check Mode 3D enable
                Log.d(TAG, "-------3D---");
                int displayType = displayManager.getDisplayOutputType(0);
                if (displayType != DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI
                        || displayType == DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI
                        && displayManager.getDisplaySupport3DMode(0) <= 0) {
                    int id = R.string.not_hdmi;
                    if (displayType == DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI) {
                        id = R.string.not_support;
                    }
                    Toast toast = Toast.makeText(mContext, id, Toast.LENGTH_SHORT);
                    toast.setGravity(Gravity.CENTER, 0, 0);
                    toast.show();
                    return;
                }
                displayManager.setDisplay3DLayerOffset(0, 31);
                editor.putInt(EDITOR_MODE2DOR3D, 1);
                editor.commit();
                switch (selectedIterm) {
                    case 0:
                        displayManager.setDisplay3DMode(0,
                                DisplayOutputManager.DISPLAY_3D_LEFT_RIGHT_HDMI);
                        if(TRACE)Log.d(TAG,"DISPLAY_3D_LEFT_RIGHT_HDMI");
                        break;
                    case 1:
                        int isDoubleStream = SystemProperties.getInt("mediasw.doublestream", 0);
                        Log.d(TAG, "isDoubleStream=" + isDoubleStream);
                        if (isDoubleStream == 1) {
                            int videoHeight = mVideoView.getVideoHeight();
                            Log.d(TAG, "3d top/bottom videoHeight=" + videoHeight);
                            displayManager.setDisplay3DMode(0,
                                    DisplayOutputManager.DISPLAY_3D_TOP_BOTTOM_HDMI, videoHeight*2);
                        } else {
                            displayManager.setDisplay3DMode(0,
                                    DisplayOutputManager.DISPLAY_3D_TOP_BOTTOM_HDMI);
                        }
                        if(TRACE)Log.d(TAG,"DISPLAY_3D_TOP_BOTTOM_HDMI");
                        break;
                    case 2:
                        /* add by liuanlong 14/11/26>>> */
                        int videoStreamNum = mVideoView.getVideoStreamNum();
                        Log.d(TAG, "-------------videoStreamNum--------------" + videoStreamNum);
                        if (videoStreamNum < 2) {
                            Log.d(TAG, "-------------videoStreamNum< 2--------------");
                            Toast.makeText(mContext, "not double stream", Toast.LENGTH_SHORT)
                                    .show();
                        } else if (videoStreamNum == 2) { /* <<<end */
                            displayManager.setDisplay3DMode(0,
                                    DisplayOutputManager.DISPLAY_3D_DUAL_STREAM);
                            if(TRACE)Log.d(TAG,"DISPLAY_3D_DUAL_STREAM");
                        }
                        break;
                    case 3:
                        break;
                    case 4:
                        break;
                    default:
                        break;
                }
            }
        }
    }

    @Override
    public boolean onInfo(MediaPlayer mp, int what, int extra) {
        // TODO Auto-generated method stub
        Log.v(TAG, "=====onInfo=====what=" + what + "==extra=" + extra);
        // dual stream display in 3D mode
        setDisplay3DModeForDualStream();
        return true;
    }

    //hide all media controls while into the PIP mode
    public void hideAllControls(){
        if(mMediaController != null){
            mMediaController.hide();
        }
    }
    //上层设置当前是否是刚从PIP模式中退出。  因为context.isInPictureInPictureMode这个是在onResume之前发生的
    //如果按播放键进入全屏，将调用onResume，但是isInPictureInPictureMode判断的是已经退出了PIP，所以会调用videoView.resume重头播放
    public void setActivityOutFromPIPMode(boolean inPIP){
        isActivityOutFromPIPMode = inPIP;
    }
    public void saveTimeBookmark(){
        if(mVideoView != null){
            // current time > 10s and save current position
            int duration = mVideoView.getDuration();
            mVideoPosition = mVideoView.getCurrentPosition();
            if (mVideoPosition > 10 * 1000 && duration - mVideoPosition > 10 * 1000) {
                mVideoView.suspend();
                deleteBookmark();
                setBookmark(mVideoPosition - 3 * 1000);
            } else {
                mVideoView.suspend();
                deleteBookmark();
            }
        }
    }
    //add for display 3D mode for double stream movies
    public void setDisplay3DModeForDualStream() {
        int isDoubleStream = SystemProperties.getInt("mediasw.doublestream", 0);
        Log.d(TAG, "onInfo isDoubleStream=" + isDoubleStream);
        if (isDoubleStream == 1) {
            displayManager = DisplayOutputManager.getInstance();
            int videoHeight = mVideoView.getVideoHeight();
            Log.d(TAG, "3d top/bottom videoHeight=" + videoHeight);
            displayManager.setDisplay3DMode(0,
                    DisplayOutputManager.DISPLAY_3D_TOP_BOTTOM_HDMI, videoHeight*2);
            setMode2dOr3dUI(1,false);
        } else {
            setMode2dOr3dUI(0,false);
        }
    }

    //0 means 2d , 1 means 3d
    private void setMode2dOr3dUI(int mode,boolean useAnim) {
        Log.d(TAG,"setMode2dOr3dUI mode:"+mode+" ,useAnim:"+useAnim);
        editor.putInt(EDITOR_MODE2DOR3D, mode);
        editor.commit();
        mMediaController.setListViewData(R.id.mode3D, mode, mListMode3D,useAnim);
    }
}

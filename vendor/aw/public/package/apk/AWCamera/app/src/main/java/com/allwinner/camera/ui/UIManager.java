package com.allwinner.camera.ui;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.content.ActivityNotFoundException;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.params.MeteringRectangle;
import android.media.AudioManager;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.support.annotation.RequiresApi;
import android.support.design.widget.TabLayout;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.RelativeLayout;

import com.alibaba.android.mnnkit.actor.FaceDetector;
import com.alibaba.android.mnnkit.entity.FaceDetectionReport;
import com.alibaba.android.mnnkit.intf.InstanceCreatedListener;
import com.allwinner.camera.CameraActivity;
import com.allwinner.camera.R;
import com.allwinner.camera.WeakReferenceHandler;
import com.allwinner.camera.cameracontrol.CameraManager;
import com.allwinner.camera.cameracontrol.CameraStatusListener;
import com.allwinner.camera.cameracontrol.CommandTask;
import com.allwinner.camera.data.CameraData;
import com.allwinner.camera.data.Contants;
import com.allwinner.camera.program.FilterCaptureEngine;
import com.allwinner.camera.utils.BitmapUtils;
import com.allwinner.camera.utils.CameraUtils;
import com.allwinner.camera.utils.SharedPreferencesUtils;
import com.allwinner.camera.utils.SoundPlayerUtil;
import com.allwinner.camera.utils.ThumbnailUtils;
import com.allwinner.camera.views.CardRvAdapter;
import com.allwinner.camera.views.CountDownView;
import com.allwinner.camera.views.CustomSnapHelper;
import com.allwinner.camera.views.FocusView;
import com.allwinner.camera.views.OnScreenHint;
import com.allwinner.camera.views.PointView;
import com.allwinner.camera.views.PreviewRenderer;
import com.allwinner.camera.views.CircleRotateImageView;
import com.allwinner.camera.views.RotateTextView;


import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.OutputStream;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;


import static android.hardware.Camera.ACTION_NEW_PICTURE;
import static com.allwinner.camera.data.Contants.CameraCommand.CLOSECAMERA;
import static com.allwinner.camera.data.Contants.CameraCommand.OPENCAMERA;
import static com.allwinner.camera.data.Contants.CameraCommand.STARTPREVIEW;
import static com.allwinner.camera.data.Contants.CameraCommand.STOPPREVIEW;
import static com.allwinner.camera.utils.ThumbnailUtils.readBitmapFromByteArray;

public class UIManager implements CameraStatusListener,
        CountDownView.CountDownListener, Handler.Callback {
    private static final int MSG_HIDE_FOCUS = 1;
    private static final long HIDE_FOCUS_DELAY = 4000;
    private static final boolean SHOW_FACEPOINT = false ;
    private static final boolean SHOW_FACEPOINT_INDEX = false ;
    private AudioManager mAudioManager;
    private Context mContext;
    private View mRootView;
    private GLSurfaceView mGlCameraPreview;
    private PreviewRenderer mRenderer;
    private SquareModeUI mSquareModeUI;
    private ProfessionModeUI mProfessionModeUI;
    private AutoModeUI mAutoModeUI;
    private VideoModeUI mVideoModeUI;
    private PanoModeUI mPanoModeUI;
    private CameraActivity mActivity;
    private static final String TAG = "UIManager";
    private int mViewWidth;
    final Object mWaitOpenLock = new Object();
    final Object mWaitCloseLock = new Object();
    private int mViewHeight;
    private Surface mPreviewSurface;
    private SurfaceTexture mPreviewSurfaceTexture;
    private CommandTask mCommandTask;
    boolean mIsCameraOpen = false;
    private Contants.ModeType mPreMode = null;
    private Contants.ModeType mCurrentMode = null;
    private boolean mIsPreviewStart = false;
    public CircleRotateImageView mThumbnailView;
    private CountDownView mCountDownView = null;
    private View mGridlineView;
    private ImageView mCoverLayer;
    private boolean isModePause = false;
    private FilterCaptureEngine mFilterCaptureEngine = null;
    public String[] allFiles;
    private String SCAN_PATH;
    private static final String FILE_TYPE = "video/*";
    private RelativeLayout mFilterSelectRelativeLayout;
    private RecyclerView mFilterRecyclerView;
    private List<Integer> mFilterDataList;
    private CardRvAdapter mFilterAdapter;
    private Contants.FilterType mCurrentFilterType = Contants.FilterType.Normal;
    private boolean mIsFling = true;
    private int[] mFilterEffectImageArray = {R.mipmap.camera_effect_autofix, R.mipmap.camera_autumn, R.mipmap.camera_blue,
            R.mipmap.camera_film, R.mipmap.camera_bleak, R.mipmap.camera_green, R.mipmap.camera_tealorange, R.mipmap.camera_amber,
            R.mipmap.camera_sunset};
    private int[] mFilterEffectText = {R.string.filter_normal, R.string.filter_autumn, R.string.filter_blue, R.string.filter_film, R.string.filter_bleak, R.string.filter_green,
            R.string.filter_green_orange, R.string.filter_amber, R.string.filter_sunset};

    private int[] mFilterEffectTextcolor = {R.color.colorLightSeaGreen, R.color.colorMediumPurple, R.color.colorLightCoral, R.color.colorCadetBlue,
            R.color.colorPeachPuff, R.color.colorMediumOrchid, R.color.colorPaleVioletRed, R.color.colorRosyBrown, R.color.colorDarkCyan, R.color.colorMediumPurple};
    private List<Integer> mStickerDataList;
    private int[] mStickerImageArray = {R.mipmap.none, R.mipmap.sheep,R.mipmap.rabbit,R.mipmap.fairy};
    private int[] mStickerText = {R.string.stickers_none, R.string.stickers_sheep,R.string.stickers_rabbit,R.string.stickers_fairy};
    private int[] mStickerTextcolor = {R.color.colorLightSeaGreen,R.color.colorLightCoral,R.color.colorDarkCyan,R.color.colorMediumPurple};
    private String[] mStickerNameArray = {"","sheep","rabbit","fairy"};
    private boolean mFilterIsVisible = false;
    private Uri mSaveUri;
    private FrameLayout mIntentLayout;
    private ImageView mIntentImageView;
    private RotateTextView mTextView;
    private Button mRetryButton;
    private Button mFinishButton;
    private byte[] mBytes;
    private boolean mFaceHandle = false;
    private PreviewRenderer.SurfaceTextureListener mSurfaceTextureListener;
    private int mIndex = 1;
    private static String ZOOMDEFAULTTEXT = "1.0x";
    private boolean mFirstMeasure = true;
    private FocusView mFocusView;
    private FrameLayout mFrameLayoutView;
    private int mSquareTopMargin;
    private int mSquareBottomMargin;
    private final Object mStorageSpaceLock = new Object();
    private FaceDetector mFaceDetector;
    private FaceDetector mCaptureFaceDetector;
    private long mStorageSpaceBytes;
    private boolean mPaused;
    private OnScreenHint mStorageHint;
    private float mZoomValue = 1.0f;
    private TabLayout mTabLayout;
    private ViewPager mViewPager;
    private List<View> mViewList = new ArrayList<>(2);
    private int[] mStrIds = {R.string.filters,R.string.stickers};
    private RecyclerView mStickerRecyclerView;
    private CardRvAdapter mStickerAdapter;
    private WeakReferenceHandler<UIManager> mUiManagerWeakReferenceHandler;
    private boolean mIsPreviewSend =false;
    private PointView mPointView;
    @Override
    public boolean handleMessage( Message message) {

        switch (message.what) {
            case MSG_HIDE_FOCUS:
                mFocusView.hideFocusView();
                break;

            default:
                break;
        }
        return false;
    }

    public UIManager(Context context, Activity activity) {
        mContext = context;
        mActivity = (CameraActivity) activity;
        mCommandTask = new CommandTask(this);
        mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        CameraManager.getInstance().setCameraListener(this);
        mUiManagerWeakReferenceHandler = new WeakReferenceHandler<UIManager>(this) ;
    }

    public void initView(View rootView) {
        Intent intent = mActivity.getIntent();
        if (intent.getExtras() != null) {
            boolean front = intent.getBooleanExtra("android.intent.extra.USE_FRONT_CAMERA", false);
            boolean assistant_front = intent.getBooleanExtra("com.google.assistant.extra.USE_FRONT_CAMERA", false);
            boolean back = intent.getBooleanExtra("android.intent.extra.CAMERA_OPEN_ONLY", false);
            boolean assistant_back = intent.getBooleanExtra("com.google.assistant.extra.CAMERA_OPEN_ONLY", false);
            if (front || assistant_front) {
                CameraData.getInstance().setCameraId(CameraData.getInstance().getFrontCameraId());
            } else if (back || assistant_back) {
                CameraData.getInstance().setCameraId(CameraData.getInstance().getBackCameraId());
            }
        }

        if (CameraData.getInstance().getIsSecureCameraIntent()) {
            Window win = mActivity.getWindow();
            WindowManager.LayoutParams params = win.getAttributes();
            params.flags |= WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED;
            win.setAttributes(params);
        }
        mRootView = rootView;
        ViewStub viewStub = rootView.findViewById(R.id.previewstub);
        if (viewStub != null) {
            viewStub.inflate();
        }
        mGridlineView = rootView.findViewById(R.id.gridlineView);
        mFrameLayoutView = rootView.findViewById(R.id.fl_root);
        mFilterSelectRelativeLayout = rootView.findViewById(R.id.rl_filter_select);
        View filterView = null;
        View stickerView = null;
        filterView = LayoutInflater.from(mContext).inflate(R.layout.child_view,null);
        stickerView = LayoutInflater.from(mContext).inflate(R.layout.child_view,null);
        mFilterRecyclerView = (RecyclerView) filterView.findViewById(R.id.rv);
        mStickerRecyclerView = (RecyclerView) stickerView.findViewById(R.id.rv);
        mViewList.add(filterView);
        mViewList.add(stickerView);
        mTabLayout = (TabLayout) rootView.findViewById(R.id.tab_layout);
        mViewPager = (ViewPager) rootView.findViewById(R.id.vp_viewpager);
        //mFilterRecyclerView = (RecyclerView) rootView.findViewById(R.id.rv);
        mIntentLayout = (FrameLayout) rootView.findViewById(R.id.intent_layout);
        mRetryButton = (Button) rootView.findViewById(R.id.retry);
        mFinishButton = (Button) rootView.findViewById(R.id.finish);
        mRetryButton.setOnClickListener(new ButtonClickListener());
        mFinishButton.setOnClickListener(new ButtonClickListener());
        mIntentImageView = rootView.findViewById(R.id.intent_iv);
        mTextView = (RotateTextView) rootView.findViewById(R.id.text);
        mTextView.setOnClickListener(new ButtonClickListener());
        if (CameraData.getInstance().showReferenceLine()) {
            mGridlineView.setVisibility(View.VISIBLE);
        } else {
            mGridlineView.setVisibility(View.GONE);
        }
        mCoverLayer = rootView.findViewById(R.id.cover_layer);
        mGlCameraPreview = rootView.findViewById(R.id.preview2);
        mGlCameraPreview.setEGLContextClientVersion(2);
       // mGlCameraPreview.getHolder().setFormat(PixelFormat.RGBA_8888);
        mRenderer = new PreviewRenderer(mContext, this);
        mSurfaceTextureListener = new PreviewRenderer.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface) {
                mPreviewSurfaceTexture = surface;
                mPreviewSurfaceTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
                    @Override
                    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                        mGlCameraPreview.requestRender();
                    }
                });
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        configureTransform();
                        mGlCameraPreview.setVisibility(View.VISIBLE);
                    }
                });

            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                return false;
            }
        };
        mRenderer.setListener(mSurfaceTextureListener);
        mGlCameraPreview.setRenderer(mRenderer);
        mGlCameraPreview.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        mThumbnailView = rootView.findViewById(R.id.thumbnail);
        mThumbnailView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startGallery();
            }
        });
        mCountDownView = rootView.findViewById(R.id.countdown);
        mCountDownView.setListener(this);
        if (CameraData.getInstance().isImageCaptureIntent() || CameraData.getInstance().isVideoCaptureIntent()) {
            mThumbnailView.setVisibility(View.GONE);
            setupCaptureParams();
        }
        mFilterCaptureEngine = new FilterCaptureEngine(mContext, this);
        mFilterCaptureEngine.init();
        mFocusView = new FocusView(mContext);
        mFocusView.setVisibility(View.GONE);
        mFrameLayoutView.addView(mFocusView);
        initData();
        createKitInstance(mContext);
        createCaptureKitInstance(mContext);
        if(SHOW_FACEPOINT) {
            mPointView = rootView.findViewById(R.id.pointView);
            mPointView.init();
            mPointView.setVisibility(View.VISIBLE);
        }
    }

    private void initData() {
        SoundPlayerUtil.init(getContext());
        mFilterDataList = new ArrayList<>();
        for (int i = 0; i < mFilterEffectImageArray.length; i++) {
            mFilterDataList.add(mFilterEffectImageArray[i]);
        }
        mStickerDataList = new ArrayList<>();
        for (int i = 0; i < mStickerImageArray.length; i++) {
            mStickerDataList.add(mStickerImageArray[i]);
        }
        mFilterRecyclerView.setLayoutManager(new LinearLayoutManager(getContext(), LinearLayoutManager.HORIZONTAL, false));
        mFilterAdapter = new CardRvAdapter(getContext(), mFilterDataList, mFilterEffectText, mFilterEffectTextcolor);
       // mFilterAdapter = new CardRvAdapter(getContext(), mStickerDataList, mStickerText, mStickerTextcolor);
        mFilterRecyclerView.setAdapter(mFilterAdapter);
        mFilterAdapter.setOnItemClickListener(new CardRvAdapter.OnItemClickListener() {
            @Override
            public void onItemClick(View view, int position) {
                smoothMoveToPosition(position, mFilterAdapter);
                setFilterType(mCurrentFilterType.convertFilterType(position));
                CameraData.getInstance().setFilterType(mCurrentFilterType.convertFilterType(position));
            }
        });
        mStickerRecyclerView.setLayoutManager(new LinearLayoutManager(getContext(), LinearLayoutManager.HORIZONTAL, false));
        mStickerAdapter = new CardRvAdapter(getContext(), mStickerDataList, mStickerText, mStickerTextcolor);
        mStickerRecyclerView.setAdapter(mStickerAdapter);
        mStickerAdapter.setOnItemClickListener(new CardRvAdapter.OnItemClickListener() {
            @Override
            public void onItemClick(View view, int position) {
                smoothMoveToPosition(position,mStickerAdapter);
                CameraData.getInstance().setStickerName(mStickerNameArray[position]);
            }
        });
        mViewPager.setAdapter(new PagerAdapter() {
            @Override
            public int getCount() {
                return mViewList.size();
            }

            @Override
            public boolean isViewFromObject(View view, Object object) {
                return view == object;
            }

            @Override
            public Object instantiateItem(ViewGroup container, int position) {
                View view = mViewList.get(position);
                container.addView(view);
                return view;
            }

            @Override
            public void destroyItem(ViewGroup container, int position, Object object) {
                container.removeView((View) object);
            }

            @Override
            public CharSequence getPageTitle(int position) {
                return getContext().getString(mStrIds[position]);
            }
        });
        mTabLayout.setupWithViewPager(mViewPager);
        CustomSnapHelper mMySnapHelper = new CustomSnapHelper();
        mMySnapHelper.attachToRecyclerView(mFilterRecyclerView);
    }

    public void createKitInstance(Context context) {
        FaceDetector.FaceDetectorCreateConfig createConfig = new FaceDetector.FaceDetectorCreateConfig();
        createConfig.mode = FaceDetector.FaceDetectMode.MOBILE_DETECT_MODE_VIDEO;
        FaceDetector.createInstanceAsync(context, createConfig, new InstanceCreatedListener<FaceDetector>() {
            @Override
            public void onSucceeded(FaceDetector faceDetector) {
                mFaceDetector = faceDetector;
                if (mRenderer != null) {
                    mRenderer.createKitSuccess(mFaceDetector);
                }
            }

            @Override
            public void onFailed(int i, Error error) {
                Log.e(TAG, "create face detetector failed: " + error);
            }
        });
    }

    public void createCaptureKitInstance(Context context) {
        FaceDetector faceDetector = null;
        FaceDetector.FaceDetectorCreateConfig createConfig = new FaceDetector.FaceDetectorCreateConfig();
        createConfig.mode = FaceDetector.FaceDetectMode.MOBILE_DETECT_MODE_IMAGE;
        FaceDetector.createInstanceAsync(context, createConfig, new InstanceCreatedListener<FaceDetector>() {
            @Override
            public void onSucceeded(FaceDetector faceDetector) {
                mCaptureFaceDetector = faceDetector;
                if (mFilterCaptureEngine != null) {
                    mFilterCaptureEngine.createKitSuccess(mCaptureFaceDetector);
                }
            }

            @Override
            public void onFailed(int i, Error error) {
                Log.e(TAG, "create face detetector failed: " + error);
            }
        });
    }

    public void onScale(ScaleGestureDetector detector) {
        float scale = detector.getScaleFactor();
        float delta = caculateDelta(scale);
        Log.d(TAG, "onScale=" + detector.getScaleFactor() + "delta:" + delta);
        if (CameraData.getInstance().getIsSupportZoom()) {
            CameraManager.getInstance().setZoom((mZoomValue+delta) * 100f);
        }
    }

    public void onScaleBegin(ScaleGestureDetector detector) {
        if (CameraManager.getInstance().isAutoAFModeValid()) {
            mFocusView.hideFocusView();
            CameraManager.getInstance().cancelAutoFocus();
        }
        Log.d(TAG, "onScaleBegin=" + detector.getScaleFactor());
    }

    public void onScaleEnd(ScaleGestureDetector detector) {
        Log.d(TAG, "onScaleEnd=" + detector.getScaleFactor());
        float scale = detector.getScaleFactor();
        float delta = caculateDelta(scale);
        mZoomValue = mZoomValue + delta;
    }

    public float caculateDelta(float scale) {
        float delta;
        if (scale >= 1.0f) {
            delta = (scale - 1f) / ((float) (Contants.MAXFORWARDDISTANCE - 1f) / (CameraData.getInstance().getMaxZoom() / 100f));
        } else {
            delta = (scale - 1f) / ((float) (1f - Contants.MAXREVERSEDISTANCE) / (CameraData.getInstance().getMaxZoom() / 100f));
        }
        return delta;
    }

    public void setPreviewFlashRequest(int flashMode){
        CameraManager.getInstance().setPreviewFlashRequest(flashMode);
    }
    class ButtonClickListener implements View.OnClickListener {
        @Override
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.retry:
                    mIntentLayout.setVisibility(View.GONE);
                    break;
                case R.id.finish:
                    if (CameraData.getInstance().isImageCaptureIntent()) {
                        onCaptureDone(mBytes);
                    } else if (CameraData.getInstance().isVideoCaptureIntent()) {
                        onVideoDone();
                    }
                    break;
                case R.id.text:
                    int maxValue = (int) Math.floor(CameraData.getInstance().getMaxZoom() / 100f);
                    mIndex = (mIndex + 1) % maxValue;
                    if (mIndex == 0) {
                        mIndex = maxValue;
                    }
                    mTextView.setText(mIndex + ".0x");
                    CameraManager.getInstance().setZoom(mIndex * 100f);

                default:
                    break;
            }
        }
    }

    private void onVideoDone() {
        Intent resultIntent = new Intent();
        int resultCode;
        resultCode = Activity.RESULT_OK;
        resultIntent.setData(CameraData.getInstance().getIntentUri());
        resultIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        mActivity.setResult(resultCode, resultIntent);
        mActivity.finish();
    }

    private void setupCaptureParams() {
        Intent intent = mActivity.getIntent();
        Bundle myExtras =intent.getExtras();
        if (myExtras != null) {
            if(intent.hasExtra(MediaStore.EXTRA_OUTPUT)) {
                mSaveUri = (Uri) myExtras.getParcelable(MediaStore.EXTRA_OUTPUT);
                CameraData.getInstance().setIntentUri(mSaveUri);
            }
            if(intent.hasExtra(MediaStore.EXTRA_DURATION_LIMIT)){
                int seconds = intent.getIntExtra(MediaStore.EXTRA_DURATION_LIMIT, 0);
                CameraData.getInstance().setIntentDuration(seconds);
            }
            if(intent.hasExtra(MediaStore.EXTRA_SIZE_LIMIT)){
                long requestedSizeLimit = myExtras.getLong(MediaStore.EXTRA_SIZE_LIMIT, 0);
                CameraData.getInstance().setIntentSize(requestedSizeLimit);
            }
            if(intent.hasExtra(MediaStore.EXTRA_VIDEO_QUALITY)){
                int quality = intent.getIntExtra(MediaStore.EXTRA_VIDEO_QUALITY, 0);
                CameraData.getInstance().setIntentQuality(quality);
            }

        }
    }

    private void smoothMoveToPosition(final int position, CardRvAdapter adapter) {
        adapter.setSelectedPosition(position);
        int selectedPosition = adapter.getSelectedPosition();
        Log.d(TAG, "selectedPosition=" + selectedPosition);
        adapter.notifyDataSetChanged();
        int firstItem = mFilterRecyclerView.getChildLayoutPosition(mFilterRecyclerView.getChildAt(0));

        int lastItem = mFilterRecyclerView.getChildLayoutPosition(mFilterRecyclerView.getChildAt(mFilterRecyclerView.getChildCount() - 1));
        int distance = mFilterRecyclerView.getChildAt(1).getRight() - mFilterRecyclerView.getChildAt(1).getLeft();
        if (position == lastItem) {
            mFilterRecyclerView.smoothScrollBy(distance, 0);
        } else if (position == firstItem) {
            mFilterRecyclerView.smoothScrollBy(-distance, 0);
        }
    }

    public void startGallery() {
        Intent intent = new Intent(android.content.Intent.ACTION_VIEW);
        if(CameraData.getInstance().getPhotoUri() != null) {
            try {
                intent.setData(CameraData.getInstance().getPhotoUri());
                intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                mContext.startActivity(intent);
            } catch(RuntimeException e) {
                e.printStackTrace();
                clearThumb();
            }
        }
    }

    private void clearThumb() {
        CameraData.getInstance().setPhotoUri(null);
        updateThumbnail(null);
    }

    public void initModeView(View rootView, Contants.ModeType type) {
        switch (type) {
            case SquareMode:
                initSquareModeView(rootView);
                break;
            case ProfessionMode:
                initProfessionModeView(rootView);
                break;
            case AutoMode:
                initAutoModeView(rootView);
                break;
            case VideMode:
                initVideoModeView(rootView);
                break;
            case PanoMode:
                initPanoModeView(rootView);
                break;
            default:
                break;
        }
        if (CameraData.getInstance().getCurrentModeType() != null && (getModeUI(CameraData.getInstance().getCurrentModeType())) != null) {
            getModeUI(CameraData.getInstance().getCurrentModeType()).onOrientationChanged(CameraData.getInstance().getOrientationCompensation());
        }
    }


    public SquareModeUI initSquareModeView(View rootview) {
        if (mSquareModeUI != null) {
            // mSquareModeUI.release();
        }
        mSquareModeUI = new SquareModeUI(rootview, this);
        return mSquareModeUI;
    }

    public ProfessionModeUI initProfessionModeView(View rootview) {
        if (mProfessionModeUI != null) {
            //    mProfessionModeUI.release();
        }
        mProfessionModeUI = new ProfessionModeUI(rootview, this);
        return mProfessionModeUI;
    }

    public AutoModeUI initAutoModeView(View rootview) {
        if (mAutoModeUI != null) {
            //   mAutoModeUI.release();
        }
        mAutoModeUI = new AutoModeUI(rootview, this);

        return mAutoModeUI;
    }

    public VideoModeUI initVideoModeView(View rootview) {
        if (mVideoModeUI != null) {
            //   mVideoModeUI.release();
        }
        mVideoModeUI = new VideoModeUI(rootview, this);
        return mVideoModeUI;
    }

    public CameraActivity getActivity(){
        return mActivity;
    }

    public AudioManager getAudioManager(){
        return mAudioManager;
    }

    public PanoModeUI initPanoModeView(View rootview) {
        if (mPanoModeUI != null) {
            //    mPanoModeUI.release();
        }
        mPanoModeUI = new PanoModeUI(rootview, this);
        return mPanoModeUI;
    }

    public void setCurrentMode(Contants.ModeType type) {
        mPreMode = CameraData.getInstance().getCurrentModeType();
        if (type != mCurrentMode) {
            if (getModeUI(mPreMode) != null) {
                getModeUI(mPreMode).release();
            }
            CameraData.getInstance().setCurrentMode(type);
            if (getModeUI(type) != null) {
                getModeUI(type).enter();
                getModeUI(type).onOrientationChanged(CameraData.getInstance().getOrientationCompensation());
            }
            mCurrentMode = type;
            onModeChanged();
        }
    }

    public void onModeChanged() {
        Log.i(TAG, "onModeChanged");
        if (mFilterSelectRelativeLayout != null) {
            setFilterViewVisible(false);
        }
        if (mCurrentMode == Contants.ModeType.AutoMode ) {
            setFilterType(Contants.FilterType.Normal);
            if(mAutoModeUI!=null) {
                mAutoModeUI.onModeChanged();
            }
        } else if(mCurrentMode == Contants.ModeType.PanoMode){
            setFilterType(Contants.FilterType.YUV);
        }else {
            setFilterType(Contants.FilterType.Normal);
            //  setFaceBeautyOn(false);
        }

        stopPreview();
        if (mFocusView != null) {
            mFocusView.hideFocusView();
        }

        startPreview();

    }

    public void switchCamera() {
        setFaceResult(null);
        int cameraId = CameraData.getInstance().getCameraId();
        int switchCameraid;
        if (cameraId == CameraData.getInstance().getBackCameraId()) {
            switchCameraid = CameraData.getInstance().getFrontCameraId();
        } else {
            switchCameraid = CameraData.getInstance().getBackCameraId();
        }

        stopPreview();
        setFilterViewVisible(false);
        if(mAutoModeUI!=null){
            mAutoModeUI.setFaceAdjustInVisible();
        }
        mCommandTask.sendCommand(CLOSECAMERA);
        mCommandTask.sendCommand(OPENCAMERA, switchCameraid);
        // CameraData.getInstance().updateCameraRatio(CameraData.getInstance().getCurrentRadioStr());//sharePrefence
        //updateCameraSize();
        startPreview();
    }

    public void onModeResume() {
        Log.i(TAG, "onModeResume");
        isModePause = false;
        //CameraData.getInstance().updateCameraRatio(CameraData.getInstance().getCurrentRadioStr());
        if (getModeUI(CameraData.getInstance().getCurrentModeType()) != null) {
            getModeUI(CameraData.getInstance().getCurrentModeType()).onResume();
        }
        startPreview();

    }

    public void onModePause() {
        Log.i(TAG, "onModePause");
        isModePause = true;
        //CameraData.getInstance().updateCameraRatio(CameraData.getInstance().getCurrentRadioStr());
        stopPreview();
        if (getModeUI(CameraData.getInstance().getCurrentModeType()) != null) {
            getModeUI(CameraData.getInstance().getCurrentModeType()).release();
        }
    }

    public BaseModeUI getModeUI(Contants.ModeType type) {
        switch (type) {
            case SquareMode:
                return mSquareModeUI;
            case ProfessionMode:
                return mProfessionModeUI;
            case AutoMode:
                return mAutoModeUI;
            case VideMode:
                return mVideoModeUI;
            case PanoMode:
                return mPanoModeUI;
            default:
                return mAutoModeUI;
        }

    }

    public void openCamera(int cameraid) {
        if (mIsCameraOpen) {
            return;
        }
        sendCommand(OPENCAMERA, cameraid);
        try {
            synchronized (mWaitOpenLock) {
                Log.i(TAG, "mWaitOpenLock wait");
                mWaitOpenLock.wait();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        Log.i(TAG, "mWaitOpenLock exit");
    }

    public void closeCamera() {
        if (!mIsCameraOpen) {
            return;
        }
        sendCommand(CLOSECAMERA, null);
        try {
            synchronized (mWaitCloseLock) {
                Log.i(TAG, "mWaitCloseLock wait");
                mWaitCloseLock.wait();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public void startPreview() {
        Log.d(TAG, "startPreview： mIsPreviewSend" + mIsPreviewSend);

        if (mIsPreviewSend) {
            return;
        }
        mTextView.setText(ZOOMDEFAULTTEXT);
        CameraManager.getInstance().setZoom(1.0f * 100f);
        mIndex = 1;
        mZoomValue = mIndex;
        sendCommand(STARTPREVIEW, CameraData.getInstance().getCurrentModeType());
        mIsPreviewSend = true;
    }

    public void stopPreview() {
        if (!mIsPreviewStart) {
            Log.d(TAG, "mIsPreviewStart:" + mIsPreviewStart);
            CameraData.getInstance().setIsPreviewDone(mIsPreviewStart);
            return;
        }
        sendCommand(STOPPREVIEW, CameraData.getInstance().getCurrentModeType());
        mIsPreviewStart = false;
        mIsPreviewSend = false;
        CameraData.getInstance().setIsPreviewDone(mIsPreviewStart);
        Log.d(TAG, "stopPreview");
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onCameraNotify(Integer msgtype) {
        if (msgtype.equals(Contants.MsgType.MSG_ON_PREVIEWSTART_FINISH)) {
            configureTransform();
            mCoverLayer.setVisibility(View.GONE);
        } else if (msgtype.equals(Contants.MsgType.MSG_ON_CAPTURE_COMPLETE)) {
            getModeUI(CameraData.getInstance().getCurrentModeType()).setCaptureEnable();
        } else if (msgtype.equals(Contants.MsgType.MSG_ON_PREVIEWSTOP)) {
//            showCoverLayer();
        }

    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onCameraContentNotify(Contants.EventObject object) {
        if (object.mMsgType == Contants.MsgType.MSG_ON_PICTURETAKEN_FINISH) {
            Uri uri = (Uri) object.mMsgContent;
            updateFile(uri);
            mContext.sendBroadcast(new Intent(ACTION_NEW_PICTURE, uri));
            mContext.sendBroadcast(new Intent("com.android.camera.NEW_PICTURE", uri));
            mCoverLayer.setVisibility(View.GONE);
        } else if (object.mMsgType == Contants.MsgType.MSG_ON_UPDATE_THUMBNAIL) {
            Bitmap bitmap = (Bitmap) object.mMsgContent;
            updateThumbnail(bitmap);
        } else if (object.mMsgType == Contants.MsgType.MSG_ON_UPDATE_BYTE) {
            mBytes = (byte[]) object.mMsgContent;
            Bitmap bitmap = readBitmapFromByteArray(mBytes, 1);
            if (mIntentLayout != null) {
                mIntentLayout.setVisibility(View.VISIBLE);
                mIntentImageView.setImageBitmap(bitmap);
                // mIntentImageView.setVisibility(View.VISIBLE);
            }
            // updateThumbnail(bitmap);
        }else if (object.mMsgType==(Contants.MsgType.MSG_ON_UPDATE_BITMAP)) {
            Bitmap bitmap = (Bitmap) object.mMsgContent;
            if (mIntentLayout != null) {
                mIntentLayout.setVisibility(View.VISIBLE);
                mIntentImageView.setImageBitmap(bitmap);
                // mIntentImageView.setVisibility(View.VISIBLE);
            }
        }
        else if (object.mMsgType == Contants.MsgType.MSG_ON_UPDATE_ZOOMTEXT) {
            float zoomValue = (float) object.mMsgContent;
            DecimalFormat decimalFormat = new DecimalFormat(".0");//构造方法的字符格式这里如果小数不足2位,会以0补足.
            String zoomText = decimalFormat.format(zoomValue);
            mTextView.setText(zoomText + "x");
        } else if (object.mMsgType == Contants.MsgType.MSG_ON_UPDATE_ZOOMVIEW) {
            Boolean isVisible = (Boolean) object.mMsgContent;
            if (isVisible) {
                mTextView.setVisibility(View.VISIBLE);
            } else {
                mTextView.setVisibility(View.GONE);
            }
        }
    }

    public void onCaptureDone(byte[] data) {
        ContentResolver contentResolver = mActivity.getContentResolver();
        Uri uri = CameraData.getInstance().getIntentUri();
        if (uri != null) {
            OutputStream outputStream = null;

            try {
                outputStream = contentResolver.openOutputStream(uri);
                outputStream.write(data);
                outputStream.flush();

            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }finally {
                try {
                    if(outputStream!= null) {
                        outputStream.close();
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            CameraUtils.addExif(mContext, uri, CameraData.getInstance().getCaptureTime());
            Log.v(TAG, "saved result to URI: " + uri);
            mActivity.setResult(Activity.RESULT_OK);
            mActivity.finish();
        } else {
            Bitmap bitmap = readBitmapFromByteArray(data, 1);
            Log.v(TAG, "inlined bitmap into capture intent result");
            mActivity.setResult(Activity.RESULT_OK,
                    new Intent("inline-data").putExtra("data", bitmap));
            mActivity.finish();
        }
    }

    public void onPause() {
        Log.e(TAG, "onPause");
        if (mGlCameraPreview != null) {
            mGlCameraPreview.setVisibility(View.GONE);
        }
        if (mGridlineView != null) {
            mGridlineView.setVisibility(View.GONE);
        }
        closeCamera();
        if (mCountDownView != null) {
            mCountDownView.cancelCountDown();
        }
        mPaused = true;
//       if(mVideoModeUI != null){
//            mVideoModeUI.release();
//        }

    }

    public void onResume() {
        mPaused = false;
        updateStorageSpaceAndHint(null);
        if (mGlCameraPreview != null) {
            mGlCameraPreview.setVisibility(View.VISIBLE);
        }
        if (mGridlineView != null) {
            if (CameraData.getInstance().showReferenceLine()) {
                mGridlineView.setVisibility(View.VISIBLE);
            } else {
                mGridlineView.setVisibility(View.GONE);
            }
        }
        openCamera(CameraData.getInstance().getCameraId());
        //mFirstOpenCamera =true;
        new GetBitmapTask().execute();
        //new GetBitmapTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }


    public void onStart() {

        EventBus.getDefault().register(this);
    }


    public void onStop() {
        EventBus.getDefault().unregister(this);
    }

    public void onDestroy() {
        Log.i(TAG, "onDestroy");
        mCommandTask.quit();
        if (mRenderer != null) {
            mRenderer.setListener(null);
            mRenderer.release();
            mRenderer = null;
        }
        if (mFilterCaptureEngine != null) {
            mFilterCaptureEngine.release();
            mFilterCaptureEngine = null;
        }
        if (mFaceDetector != null) {
            mFaceDetector.release();
            mFaceDetector = null;
        }
        if (mCaptureFaceDetector != null) {
            mCaptureFaceDetector.release();
            mCaptureFaceDetector = null;
        }
        mPaused = true;
    }

    @Override
    public List<Surface> getSurface() {
        if (mPreviewSurface != null) {
            mPreviewSurface.release();
            mPreviewSurface = null;
        }
        // mCameraPreview.setAspectRatio(CameraData.getInstance().getPreviewSize().x,CameraData.getInstance().getPreviewSize().y);
        List<Surface> surfaces = new ArrayList<>();
        if (mPreviewSurfaceTexture != null) {
            if (CameraData.getInstance().getPreviewSize() != null) {
                Log.e(TAG, "CameraData.getInstance().getPreviewSize().x:" + CameraData.getInstance().getPreviewSize().x + "y:" + CameraData.getInstance().getPreviewSize().y);
                mPreviewSurfaceTexture.setDefaultBufferSize(CameraData.getInstance().getPreviewSize().x, CameraData.getInstance().getPreviewSize().y);

            }
            mPreviewSurface = new Surface(mPreviewSurfaceTexture);
        }
        Log.i(TAG, "getSurface " + mPreviewSurface);
        //surfaces.add(mPreviewSurface);
        //return surfaces;
        if (getModeUI(CameraData.getInstance().getCurrentModeType()) != null) {
            Surface surface = getModeUI(CameraData.getInstance().getCurrentModeType()).getPreviewSurface();
            Log.i(TAG, "surface:" + surface);
            if (surface != null) {
                surfaces.add(surface);
            }
        }
        if (surfaces.size() > 0) {
            Log.i(TAG, "surfaces.size() >0");
            return surfaces;
        } else {
            Log.i(TAG, "getSurface else");
            surfaces.add(mPreviewSurface);
            return surfaces;
        }

    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    public Surface getVideoSurface() {
        return mVideoModeUI.getVideoSurface();
    }

    @Override
    public SurfaceTexture getSurfaceTexture() {
        return mPreviewSurfaceTexture;
    }


    public boolean isCameraOpen() {
        return mIsCameraOpen;
    }

    private void configureTransform() {
        Log.d(TAG, "setPreviewTransform:ratio :" + CameraData.getInstance().getCameraRatio());
        int topMargin = 0;
        int bottomMargin = CameraData.getInstance().getScreenHeight() - topMargin - CameraData.getInstance().getViewVissableHeight();
        int filterBottomMargin = CameraData.getInstance().getScreenHeight() - topMargin - CameraData.getInstance().getScreenHeight_4_TO_3();
        mSquareTopMargin = (CameraData.getInstance().getScreenHeight_4_TO_3() - CameraData.getInstance().getSreenWidth()) / 2;
        mSquareBottomMargin =CameraData.getInstance().getScreenHeight()- CameraData.getInstance().getScreenHeight_4_TO_3() + mSquareTopMargin;
        if (mGlCameraPreview != null) {
            if (mGlCameraPreview.getLayoutParams() instanceof FrameLayout.LayoutParams) {
                FrameLayout.LayoutParams textureViewParams = (FrameLayout.LayoutParams)
                        mGlCameraPreview.getLayoutParams();
                //textureViewParams.setMargins(0,topMargin,0,bottomMargi
                Log.e(TAG, "mGlCameraPreview CameraData.getInstance().getViewVissableHeight()" + CameraData.getInstance().getViewVissableHeight());
                mGlCameraPreview.setLayoutParams(new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, CameraData.getInstance().getViewVissableHeight()));
            }
        }
        if (mGridlineView != null) {
            if (mGridlineView.getLayoutParams() instanceof FrameLayout.LayoutParams) {
                FrameLayout.LayoutParams gridViewParams = (FrameLayout.LayoutParams)
                        mGridlineView.getLayoutParams();
                if (CameraData.getInstance().getCurrentModeType().equals(Contants.ModeType.SquareMode)) {
                    gridViewParams.topMargin = mSquareTopMargin;
                    gridViewParams.bottomMargin = mSquareBottomMargin;

                } else {
                    gridViewParams.topMargin = topMargin;
                    gridViewParams.bottomMargin = bottomMargin;
                }
                mGridlineView.setLayoutParams(gridViewParams);
            }
        }
        if (mCoverLayer != null) {
            if (mCoverLayer.getLayoutParams() instanceof FrameLayout.LayoutParams) {
                FrameLayout.LayoutParams coverLayerParams = (FrameLayout.LayoutParams)
                        mCoverLayer.getLayoutParams();
                coverLayerParams.topMargin = topMargin;
                coverLayerParams.bottomMargin = bottomMargin;
                mCoverLayer.setLayoutParams(coverLayerParams);
            }
        }
        if (mFilterSelectRelativeLayout != null) {
            if (mFilterSelectRelativeLayout.getLayoutParams() instanceof FrameLayout.LayoutParams) {
                FrameLayout.LayoutParams filterViewParams = (FrameLayout.LayoutParams)
                        mFilterSelectRelativeLayout.getLayoutParams();
                if (CameraData.getInstance().getCurrentModeType().equals(Contants.ModeType.SquareMode)) {
                    filterViewParams.bottomMargin = mSquareBottomMargin;
                } else {
                    filterViewParams.bottomMargin = filterBottomMargin;
                }
                mFilterSelectRelativeLayout.setLayoutParams(filterViewParams);
            }
        }
        if (mTextView != null) {
            if (mTextView.getLayoutParams() instanceof FrameLayout.LayoutParams) {
                FrameLayout.LayoutParams textViewParams = (FrameLayout.LayoutParams)
                        mTextView.getLayoutParams();
                textViewParams.topMargin = CameraData.getInstance().getScreenHeight_4_TO_3() - 2 * mTextView.getHeight();
                mTextView.setLayoutParams(textViewParams);

            }
        }
    }

    public void sendCommand(int command, Object arg) {
        if (mCommandTask != null) {
            mCommandTask.sendCommand(command, arg);
        }
    }

    public void setFilterVisibility() {
        if (mFilterIsVisible) {
            setFilterViewVisible(false);

        } else {
            setFilterViewVisible(true);
        }
    }
    public boolean getFilterViewVisbility(){
        return mFilterSelectRelativeLayout.isShown();
    }
    public void setFilterViewVisible(boolean isVisible) {
        if (isVisible) {
            if(CameraData.getInstance().getFaceAdjustVisibility()){
                if(mAutoModeUI!=null){
                    mAutoModeUI.setFaceAdjustInVisible();
                }
            }
            mFilterSelectRelativeLayout.setVisibility(View.VISIBLE);
            setIsFling(false);
            resetZoomView(true,mFilterSelectRelativeLayout);
            mFilterIsVisible = true;

        } else {
            mFilterSelectRelativeLayout.setVisibility(View.GONE);
            resetZoomView(false,mFilterSelectRelativeLayout);
            setIsFling(true);
            mFilterIsVisible = false;
        }
    }
    public void resetZoomView(boolean needMoveUp,ViewGroup viewGroup){
        if(needMoveUp){
            if (mFirstMeasure) {
                int w = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
                int h = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
                viewGroup.measure(w, h);
                mFirstMeasure = false;
            }
            resetZoomView(CameraData.getInstance().getScreenHeight_4_TO_3() - 2 * mTextView.getHeight() - viewGroup.getMeasuredHeight());
        }else{
            resetZoomView(CameraData.getInstance().getScreenHeight_4_TO_3() - 2 * mTextView.getHeight());
        }
    }

    public void resetZoomView(int topMargin) {
        if(CameraData.getInstance().getCurrentModeType().equals(Contants.ModeType.SquareMode)){
            return;
        }
        if (mTextView != null) {
            if (mTextView.getLayoutParams() instanceof FrameLayout.LayoutParams) {
                FrameLayout.LayoutParams textViewParams = (FrameLayout.LayoutParams)
                        mTextView.getLayoutParams();
                textViewParams.topMargin = topMargin;
                mTextView.setLayoutParams(textViewParams);
            }
        }
    }


    @Override
    public void onCameraOpen() {
        Log.i(TAG, "onCameraOpen");
        if (!CameraData.getInstance().getFirstOpenCamera()) {
            mIsCameraOpen = true;
        }
        synchronized (mWaitOpenLock) {
                Log.i(TAG, "mWaitOpenLock notifyAll");
                mWaitOpenLock.notifyAll();
        }

        CameraManager.getInstance().initCommandFeature();
        CameraManager.getInstance().getSupportedPictureSizes();
        CameraManager.getInstance().getSupportedPreviewSizes();
        CameraManager.getInstance().getSupportedVideoSizes();
        if (CameraData.getInstance().getFirstOpenCamera()) {
            mCommandTask.sendCommand(CLOSECAMERA);
            mCommandTask.sendCommand(OPENCAMERA, CameraData.getInstance().getCameraId());
            CameraData.getInstance().setFirstOpenCamera(false);
        }
        Contants.EventObject object;
        if (CameraData.getInstance().getIsSupportZoom()) {
            object = new Contants.EventObject(Contants.MsgType.MSG_ON_UPDATE_ZOOMVIEW, true);
        } else {
            object = new Contants.EventObject(Contants.MsgType.MSG_ON_UPDATE_ZOOMVIEW, false);
        }
        CameraManager.getInstance().setZoom(1.0f * 100f);
        EventBus.getDefault().post(object);
        if(mRenderer!=null) {
            mRenderer.onCameraOpen();
        }
    }

    @Override
    public void onCameraOpenFail(int error) {
        synchronized (mWaitOpenLock) {
                Log.i(TAG, "mWaitOpenLock notifyAll");
                mWaitOpenLock.notifyAll();
        }
        Log.i(TAG, "onCameraOpenFail " + error);
        throw new RuntimeException("onCameraOpenFail");
    }

    @Override
    public void onCameraClose() {
        Log.i(TAG, "onCameraClose ");
        mIsCameraOpen = false;
        synchronized (mWaitCloseLock) {
                Log.i(TAG, "mWaitCloseLock notifyAll");
                mWaitCloseLock.notifyAll();
        }
    }

    @Override
    public void onFlashStatus(boolean isFlashAvaliable) {
        if(getModeUI(CameraData.getInstance().getCurrentModeType())!=null){
            getModeUI(CameraData.getInstance().getCurrentModeType()).onFlashStatus(isFlashAvaliable);
        }
    }


    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    public void onStartPreviewDone() {
        Log.i(TAG, "onStartPreviewDone!!!  mCurrentMode:"+mCurrentMode+"    mAutoModeUI:"+mAutoModeUI);
        EventBus.getDefault().post(Contants.MsgType.MSG_ON_PREVIEWSTART_FINISH);
        mIsPreviewStart = true;
        CameraData.getInstance().setIsPreviewDone(mIsPreviewStart);

        if(mCurrentMode == Contants.ModeType.AutoMode && mAutoModeUI != null){
            mAutoModeUI.handleVoiceTackPictrueIntent();
        }
    }

    @Override
    public void onStartPreviewFail() {
        Log.i(TAG, "onStartPreviewFail");
        throw new RuntimeException("onStartPreviewFail");
    }

    @Override
    public void onCaptureStarted() {

    }

    @Override
    public void onCatpureCompleted() {
    }

    @Override
    public void onPreviewStop() {
        Log.i(TAG, "onPreviewStop");
        mIsPreviewStart = false;
        CameraData.getInstance().setIsPreviewDone(mIsPreviewStart);
        if (mRenderer != null) {
            mRenderer.cleanData();
        }
        EventBus.getDefault().post(Contants.MsgType.MSG_ON_PREVIEWSTOP);
    }

    public void updateFile(Uri file) {
        mActivity.sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, file));
    }

    public void updateThumbnail(Bitmap bitmap) {
        Log.i(TAG, "updateThumbnail: " + bitmap);
        if (mThumbnailView == null) return;
        if (bitmap != null) {
            mThumbnailView.setImageBitmap(bitmap);
        } else {
            mThumbnailView.setImageResource(R.drawable.iv_circle);
        }
    }

    public Context getContext() {
        return mActivity;
    }

    public void startCountDown(final int time) {
        if (mCountDownView != null) {
            updateStorageSpaceAndHint(new OnStorageUpdateDoneListener() {
                @Override
                public void onStorageUpdateDone(long bytes) {
                    if(bytes <= Contants.LOW_STORAGE_THRESHOLD_BYTES){
                        Log.w(TAG, "Storage issue, ignore the start request");
                    }else{
                        mCountDownView.startCountDown(time);
                    }
                }
            });
        }
    }

    public void cancelCountDown() {
        if (mCountDownView != null) {
            mCountDownView.cancelCountDown();
        }
    }

    public void setIsFling(boolean isFling) {
        mIsFling = isFling;
    }

    public boolean getIsFling() {
        return mIsFling;
    }

    public void onOrientationChanged(int orientationCompensation) {
        Log.i(TAG, "onOrientationChanged: " + orientationCompensation);
        if (CameraData.getInstance().getCurrentModeType() != null && (getModeUI(CameraData.getInstance().getCurrentModeType())) != null) {
            getModeUI(CameraData.getInstance().getCurrentModeType()).onOrientationChanged(orientationCompensation);
            mThumbnailView.onOrientationChanged(orientationCompensation);
            mCountDownView.onOrientationChanged(orientationCompensation);
        }
        if(mTextView != null){
            mTextView.onOrientationChanged(orientationCompensation);
        }

    }

    private class GetBitmapTask extends AsyncTask<Void, Void, Bitmap> {

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            updateThumbnail(bitmap);
        }

        @Override
        protected Bitmap doInBackground(Void... voids) {
            return ThumbnailUtils.getLastThumbnailFromCR(mActivity.getContentResolver(), -1);
        }
    }

    public void onSingleTapUp(MotionEvent e) {

        Log.i(TAG, "#####roi.weight onSingleTapUp:X:" + e.getRawX() + "Y2:" + e.getRawY());
        if (CameraManager.getInstance().isAutoAFModeValid()) {
            // CameraManager.getInstance().calcTapAreaForCamera2(e.getRawX(),e.getRawY(),1000,)
            if (mFocusView != null && isVaildTouch(e.getRawX(), e.getRawY())) {
                MeteringRectangle focusRect = getFocusArea(e.getRawX(), e.getRawY(), true);
                MeteringRectangle meterRect = getFocusArea(e.getRawX(), e.getRawY(), false);
                // mSession.applyRequest(Session.RQ_AF_AE_REGIONS, focusRFect, meterRect);
                CameraManager.getInstance().autoFocus(focusRect, meterRect);

                mFocusView.moveToPosition(e.getRawX(), e.getRawY());
                mFocusView.setVisibility(View.VISIBLE);
                mUiManagerWeakReferenceHandler.removeMessages(MSG_HIDE_FOCUS);
                mUiManagerWeakReferenceHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        Message msg = new Message();
                        msg.what = MSG_HIDE_FOCUS;
                        mUiManagerWeakReferenceHandler.sendMessage(msg);
                    }
                }, HIDE_FOCUS_DELAY);
            } else if (mFocusView != null) {
                mFocusView.setVisibility(View.GONE);
            }
        }
        cancelCountDown();
    }
    public boolean isVaildTouch(float x, float y) {
        Resources resources = mContext.getResources();
        int topViewHeight = resources.getDimensionPixelSize(R.dimen.top_host_height);
        if (CameraData.getInstance().getCurrentModeType() == Contants.ModeType.SquareMode) {
            if (y > mSquareTopMargin && y < (CameraData.getInstance().getScreenHeight() - mSquareBottomMargin)) {
                return true;
            }
        } else {
            if (y < CameraData.getInstance().getScreenHeight_4_TO_3() && y > topViewHeight) {
                return true;
            }
        }
        return false;
    }

    public MeteringRectangle getFocusArea(float x, float y, boolean isFocusArea) {
        return CameraManager.getInstance().calcTapAreaForCamera2(x, y, 1000, isFocusArea);
    }



    public void onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        Log.i(TAG, "onScroll");
        if(getFilterViewVisbility()) {
            setFilterViewVisible(false);
        }
        cancelCountDown();
    }


    public void onLongPress(MotionEvent e) {
        Log.i(TAG, "onLongPress");
    }


    public void onFling(boolean isLeft) {

    }

    @Override
    public void onAutoFocusSuccess() {
    }

    @Override
    public void onAutoFocusFail() {

    }

    @Override
    public void onAutoFocusing() {

    }

    @Override
    public void onCountDownStart() {
        Log.i(TAG, "onCountDownStart");
        getModeUI(CameraData.getInstance().getCurrentModeType()).onCountDownStart();
    }

    @Override
    public void onCountDownEnd() {
        Log.i(TAG, "onCountDownEnd");
        getModeUI(CameraData.getInstance().getCurrentModeType()).onCountDownEnd();
    }

    @Override
    public void onCountDownCancel() {
        Log.i(TAG, "onCountDownCancel");
        if (getModeUI(CameraData.getInstance().getCurrentModeType()) != null) {
            getModeUI(CameraData.getInstance().getCurrentModeType()).onCountDownCancel();
        }
    }

    public Bitmap getPreviewBitmap() {
        return mRenderer.getPreviewBitmap();
    }

    public void generatePrevieBitmap() {
        mRenderer.generatePrevieBitmap();
    }

    private void showCoverLayer() {
        Log.i(TAG, "showCoverLayer");
        if (!isModePause) {
            BlurTask blurTask = new BlurTask();
            blurTask.execute();
        }
    }

    public boolean isModePause() {
        return isModePause;
    }

    public void setFilterType(Contants.FilterType filtertype) {
        if (mRenderer != null) {
            mRenderer.setFilterType(filtertype);
        }
        if (mFilterCaptureEngine != null) {
            mFilterCaptureEngine.setFilterType(filtertype);
        }
    }

    public void setFaceBeautyOn(boolean fbOn) {
        Log.e(TAG, "setFaceBeautyOn:" + fbOn + "mRenderer:" + mRenderer);
        if (mRenderer != null) {
            mRenderer.setFaceBeautyOn(fbOn);
        }
        if (mFilterCaptureEngine != null) {
            mFilterCaptureEngine.setFaceBeautyOn(fbOn);
        }
    }

    public void setFaceResult(FaceDetectionReport[] result) {
        if (mRenderer != null) {
            mRenderer.setFacePoint(result);
        }
        if(SHOW_FACEPOINT && mPointView !=null ){
            mPointView.setFaceResult(result,mGlCameraPreview.getWidth(),mGlCameraPreview.getHeight(),CameraData.getInstance().getOrientationCompensation(),SHOW_FACEPOINT_INDEX);
        }
        /*
        if (mFilterCaptureEngine != null) {
            mFilterCaptureEngine.setFaceResult(lists);
        }*/
    }
    public void setYuvData(byte[] yuvData, int width, int height, int jpegRotation, boolean mirror) {
        if (mRenderer != null && mIsPreviewStart) {
            mRenderer.setYuvData(yuvData, width, height, jpegRotation, mirror);
            mGlCameraPreview.requestRender();
        }
    }

    public void updateTimerView(ImageView timerImageView) {
        int index = 0;
        Log.e(TAG, "time:" + SharedPreferencesUtils.getParam(getContext(), "time", 0));
        switch ((Integer) SharedPreferencesUtils.getParam(getContext(), "time", 0)) {
            case 0:
                index = 0;
                break;
            case 3:
                index = 1;
                break;
            case 10:
                index = 2;
                break;
        }
        Log.e(TAG, "index:" + index);
        timerImageView.setImageResource(Contants.TIMEIMAGEARRAY[index]);
    }

    public void updateFlashView(RelativeLayout flashRelativeLayout, ImageView imageView){
        if (CameraData.getInstance().getIsFlashAvailable()) {
            flashRelativeLayout.setVisibility(View.VISIBLE);
            imageView.setImageResource(Contants.FLASHIMAGEARRAY[CameraData.getInstance().getCaptureFlashMode()]);
        } else {
            flashRelativeLayout.setVisibility(View.GONE);
        }
    }

    @Override
    public FilterCaptureEngine getFilterCaptureEngine() {
        return mFilterCaptureEngine;
    }

    private class BlurTask extends AsyncTask<Void, Void, Bitmap> {
        @Override
        protected Bitmap doInBackground(Void... voids) {
            Log.i(TAG, "BlurTask doInBackground");
            generatePrevieBitmap();
            Bitmap bitmap = getPreviewBitmap();
            if (bitmap != null) {
                bitmap = BitmapUtils.scaleBitmap(bitmap, 0.125f);
                Log.i(TAG, "createBlurBitmap start");
                bitmap = BitmapUtils.createBlurBitmap(bitmap, 20);
                Log.i(TAG, "createBlurBitmap end");
            }
            return bitmap;
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            mCoverLayer.setImageBitmap(bitmap);
            Log.i(TAG, "setVisibility VISIBLE");
            if (!mIsPreviewStart) {
                mCoverLayer.setVisibility(View.VISIBLE);
            }

        }
    }

    public long getStorageSpaceBytes() {
        synchronized (mStorageSpaceLock) {
            return mStorageSpaceBytes;
        }
    }

    public interface OnStorageUpdateDoneListener {
        public void onStorageUpdateDone(long bytes);
    }

    public void updateStorageSpaceAndHint(final OnStorageUpdateDoneListener callback) {
        /*
         * We execute disk operations on a background thread in order to
         * free up the UI thread.  Synchronizing on the lock below ensures
         * that when getStorageSpaceBytes is called, the main thread waits
         * until this method has completed.
         *
         * However, .execute() does not ensure this execution block will be
         * run right away (.execute() schedules this AsyncTask for sometime
         * in the future. executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)
         * tries to execute the task in parellel with other AsyncTasks, but
         * there's still no guarantee).
         * e.g. don't call this then immediately call getStorageSpaceBytes().
         * Instead, pass in an OnStorageUpdateDoneListener.
         */
        (new AsyncTask<Void, Void, Long>() {
            @Override
            protected Long doInBackground(Void ... arg) {
                synchronized (mStorageSpaceLock) {
                    mStorageSpaceBytes = CameraUtils.getAvailableSpace();
                    android.util.Log.e(TAG,"mStorageSpaceBytes:"+mStorageSpaceBytes);
                    return mStorageSpaceBytes;
                }
            }

            @Override
            protected void onPostExecute(Long bytes) {
                updateStorageHint(bytes);
                // This callback returns after I/O to check disk, so we could be
                // pausing and shutting down. If so, don't bother invoking.
                if (callback != null && !mPaused) {
                    callback.onStorageUpdateDone(bytes);
                } else {
                    Log.v(TAG, "ignoring storage callback after activity pause");
                }
            }
        }).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void updateStorageHint(long storageSpace) {
        String message = null;
        if (storageSpace == Contants.UNAVAILABLE) {
            message = mContext.getString(R.string.no_storage);
        } else if (storageSpace == Contants.PREPARING) {
            message = mContext.getString(R.string.preparing_sd);
        } else if (storageSpace == Contants.UNKNOWN_SIZE) {
            message = mContext.getString(R.string.access_sd_fail);
        } else if (storageSpace <= Contants.LOW_STORAGE_THRESHOLD_BYTES) {
            message = mContext.getString(R.string.spaceIsLow_content);
        }
        if (message != null) {
            Log.w(TAG, "Storage warning: " + message);
            if (mStorageHint == null) {
                mStorageHint = OnScreenHint.makeText(mActivity, message);
            } else {
                mStorageHint.setText(message);
            }
            mStorageHint.show();
            CameraData.getInstance().setIsStorageError(true);
        } else if (mStorageHint != null) {
            mStorageHint.cancel();
            mStorageHint = null;
            CameraData.getInstance().setIsStorageError(false);
        }
    }

    public void startCoverAnimation(){
        mCoverLayer.setVisibility(View.VISIBLE);
        mCoverLayer.setBackgroundColor(Color.BLACK);
        AlphaAnimation alphaAnimation = new AlphaAnimation(1, 0);//初始化操作，参数传入1和0，即由透明度1变化到透明度为0
        mCoverLayer.startAnimation(alphaAnimation);//开始动画
        alphaAnimation.setFillAfter(true);//动画结束后保持状态
        alphaAnimation.setDuration(100);//动画持续时间
    }

}

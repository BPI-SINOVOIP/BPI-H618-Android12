package com.softwinner.TvdVideo;


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
import android.hardware.input.InputManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.TrackInfo;
import android.media.TimedText;
import android.net.Uri;
import vendor.display.DisplayOutputManager;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.provider.MediaStore;
import android.provider.MediaStore.Video;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.IWindowManager;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.ImageView;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;
import android.widget.Toast;
import android.util.DisplayMetrics;
import android.view.WindowManager;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class StbDisplayController extends IDisplayController implements SeekBar.OnSeekBarChangeListener,View.OnClickListener {

    private static final int NOISE_DISABLE_2D = 0;
    private static final int NOISE_LOW_2D = 1;
    private static final int NOISE_MIDDLE_2D = 2;
    private static final int NOISE_HIGH_2D = 3;
    private static final int NOISE_DISABLE_3D = 0;
    private static final int NOISE_ADAPTIVE_3D = 1;
    private static final int NOISE_FIX_3D = 2;
    private static final int NOISE_3D_LEVEL_HIGH = 0;
    private static final int NOISE_3D_LEVEL_MIDDLE = 1;
    private static final int NOISE_3D_LEVEL_LOW = 2;
    private final String TAG = "StbDisplayController";
    private ImageButton mNext2d;
    private ImageButton mPrev2d;
    private TextView mTextStatus2d;
    private String[] m2dNoiseEntries;
    private String[] mDemoEntries;
    private TextView mTextStatus;
    private ImageButton mPrev;
    private ImageButton mNext;
    private TextView mTextStatus1;
    private ImageButton mPrev1;
    private ImageButton mNext1;
    private TextView mTextStatus3d;
    private ImageButton mPrev3d;
    private ImageButton mNext3d;
    private TextView mTextStatus3dStrength;
    private ImageButton mPrev3dStrength;
    private ImageButton mNext3dStrength;
    private TextView mTextStatusMovie;
    private ImageButton mPrevMovie;
    private ImageButton mNextMovie;
    private SeekBar mBrightness;
    private SeekBar mContrast;
    private SeekBar mSaturation;
    private SeekBar mDetail;
    private SeekBar mEdgeSharp;
    private LinearLayout[] mLayout;
    private  String[] mModeEntries;
    private  String[] mColorEntries;
    private DisplayOutputManager mDisplayManager;
    private int mDisplayType = 0;

    private int mCtrlIndex;
    private int mMaxCtrl = 10;

    private int mEnhanceModeId;  /* 0 disable;  1 enable ; 2 demo */
    private int mMaxEnhanceModeNum;
    private final int ENHANCE_DISABLE = 0;
    private final int ENHANCE_ENABLE = 1;
    private final int ENHANCE_DEMO = 2;

    private int mColorId;
    private int mMaxColorModeNum;
    private final int DEFAULT_VALUE = 0;
    private final int COLOR_STANDAR = 0;
    private final int COLOR_SOFT = 1;
    private final int COLOR_VIVID = 2;
    private final int COLOR_CUSTOM = 3;
    private final String STORE_NAME = "StbDisplayController";
    private final String EDITOR_COLOR_MODE = "enhance.colormode";
    private final String EDITOR_COLOR_CUSTOM_VALUES = "colormode.custom";
    private final String EDITOR_2D_NOISE_MODE = "2d.noise.mode";
    private final String EDITOR_3D_NOISE_MODE = "3d.noise.mode";
    private final String EDITOR_3D_NOISE_STRENGTH = "3d.noise.strength";
    private final String EDITOR_MOVIE_MODE = "movie.mode";
    private final SharedPreferences mSp;
    private final SharedPreferences.Editor mEditor;
    private final ColorMode mColorStandard = new ColorMode(5, 0, 4,  2, 2);
    private final ColorMode mColorSoft = new ColorMode(4, 0, 3, 0, 0);
    private final ColorMode mColorVivid = new ColorMode(5, 5, 5,  5, 5);
    private ColorMode mColorCustom = new ColorMode(5, 0, 4, 2, 2);
    private int[] mSaveModeArray;
    private int m2dNoiseId;
    private int mMax2dNoiseModeNum;
    private boolean mSupportedSNRSetting;
    private int mMax3dNoiseModeNum;
    private int mMax3dNoiseStrengthModeNum;
    private int mMaxmovieModeNum;
    private String[] m3dNoiseEntries;
    private String[] m3dNoiseStrengthEntries;
    private String[] mMovieEntries;
    private int m3dNoiseId;
    private int m3dNoiseStrengthId;
    private int mMovieId;
    private int mSreenWidth;
    private int mScreenHeight;
    private WindowManager mWindowManager;

    private class ColorMode {
        private int mBright;
        private int mContrast;
        private int mSaturation;
        private int mDenoise;
        private int mDetail;
        private int mEdge;

        public ColorMode(int brigth, int contrast, int saturation,
                         int detail, int edge) {
            mBright = brigth;
            mContrast = contrast;
            mSaturation = saturation;
            mDetail = detail;
            mEdge = edge;
        };

        public int getBringt() { return mBright; }
        public int getContrast() { return mContrast; }
        public int getSaturation() { return mSaturation; }
        public int getDetail() { return mDetail; }
        public int getEdge() { return mEdge; }

        public void setBringt(int value) { mBright = value; }
        public void setContrast(int value) { mContrast = value; }
        public void setSaturation(int value) { mSaturation = value; }
        public void setDetail(int value) { mDetail = value; }
        public void setEdge(int value) { mEdge = value; }

    };

    public StbDisplayController(Context context,int videoWidth, int videoHeight) {
        super(context, videoWidth,videoHeight);

        mModeEntries = context.getResources().getStringArray(R.array.display_settings_state);
        mColorEntries = context.getResources().getStringArray(R.array.color_settings_state);
        mDemoEntries = context.getResources().getStringArray(R.array.demo_mode);
        m2dNoiseEntries = context.getResources().getStringArray(R.array.de_2d_noise_mode);
        m3dNoiseEntries = context.getResources().getStringArray(R.array.de_3d_noise_mode);
        m3dNoiseStrengthEntries = context.getResources().getStringArray(R.array.de_3d_noise_strength_mode);
        mMovieEntries = context.getResources().getStringArray(R.array.movie_mode);
        mMaxEnhanceModeNum = mModeEntries.length;
        Log.d(TAG,"mMaxEnhanceModeNum:"+mMaxEnhanceModeNum);
        mMax2dNoiseModeNum =m2dNoiseEntries.length;
        mMax3dNoiseModeNum =m3dNoiseEntries.length;
        mMax3dNoiseStrengthModeNum = m3dNoiseStrengthEntries.length;
        mMaxmovieModeNum =mMovieEntries.length;
        LayoutInflater inflate = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View menuView = inflate.inflate(R.layout.dialog_stb_dispsettings, null);
        this.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide
                // nav
                // bar
                | View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
        );
        Log.d(TAG, "DislapyController -/-/");
        DisplayMetrics metrics = new DisplayMetrics();
        mWindowManager = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        mWindowManager.getDefaultDisplay().getMetrics(metrics);
        mSreenWidth = Math.min(metrics.heightPixels, metrics.widthPixels);
        mScreenHeight = Math.max(metrics.heightPixels, metrics.widthPixels);
        mDisplayManager = DisplayOutputManager.getInstance();

        this.setContentView(menuView);

        mTextStatus = (TextView)findViewById(R.id.result0);
        mPrev = (ImageButton)findViewById(R.id.switch_prev0);
        mPrev.setOnClickListener(this);
        mNext = (ImageButton)findViewById(R.id.switch_next0);
        mNext.setOnClickListener(this);

        mTextStatus1 = (TextView)findViewById(R.id.result1);
        mPrev1 = (ImageButton)findViewById(R.id.switch_prev1);
        mPrev1.setOnClickListener(this);
        mNext1 = (ImageButton)findViewById(R.id.switch_next1);
        mNext1.setOnClickListener(this);
        mTextStatus2d = (TextView)findViewById(R.id.result_2d);
        mPrev2d = (ImageButton)findViewById(R.id.switch_prev_2d);
        mPrev2d.setOnClickListener(this);
        mNext2d = (ImageButton)findViewById(R.id.switch_next_2d);
        mNext2d.setOnClickListener(this);
        mTextStatus3d = (TextView)findViewById(R.id.result_3d);
        mPrev3d = (ImageButton)findViewById(R.id.switch_prev_3d);
        mPrev3d.setOnClickListener(this);
        mNext3d = (ImageButton)findViewById(R.id.switch_next_3d);
        mNext3d.setOnClickListener(this);
        mTextStatus3dStrength = (TextView)findViewById(R.id.result_3d_strength);
        mPrev3dStrength = (ImageButton)findViewById(R.id.switch_prev_3d_strength);
        mPrev3dStrength.setOnClickListener(this);
        mNext3dStrength = (ImageButton)findViewById(R.id.switch_next_3d_strength);
        mNext3dStrength.setOnClickListener(this);
        mTextStatusMovie = (TextView)findViewById(R.id.result_movie);
        mPrevMovie = (ImageButton)findViewById(R.id.switch_prev_movie);
        mPrevMovie.setOnClickListener(this);
        mNextMovie = (ImageButton)findViewById(R.id.switch_next_movie);
        mNextMovie.setOnClickListener(this);

        mCtrlIndex = 0;

        mSp = mContext.getSharedPreferences(STORE_NAME, Context.MODE_PRIVATE);
        mEditor = mSp.edit();
        mSaveModeArray = getSavedMode();
        mColorId = mSaveModeArray[0] % mColorEntries.length;
        m2dNoiseId =mSaveModeArray[1] % m2dNoiseEntries.length;
        m3dNoiseId =mSaveModeArray[2] % m3dNoiseEntries.length;
        m3dNoiseStrengthId =mSaveModeArray[3] % m3dNoiseStrengthEntries.length;
        mMovieId =mSaveModeArray[4] % mModeEntries.length;
        syncColorModeState();
        setupColorCustomValues();
        sync3dNoiseState(m3dNoiseId);
        sync3dNoiseStrengthState(m3dNoiseStrengthId);
        syncMovieState(mMovieId);
        //setDisplayColorMode();

        mLayout = new LinearLayout[10];
        mLayout[0] = (LinearLayout)findViewById(R.id.display_effect_layout);
        mLayout[1] = (LinearLayout)findViewById(R.id.video_effect_layout);
        mLayout[2] = (LinearLayout)findViewById(R.id.brightness_layout);
        mLayout[3] = (LinearLayout)findViewById(R.id.contrast_layout);
        mLayout[4] = (LinearLayout)findViewById(R.id.saturation_layout);
        mLayout[5] = (LinearLayout)findViewById(R.id.detail_enhance_layout);
        mLayout[6] = (LinearLayout)findViewById(R.id.de_2d_noise_layout);
        mLayout[7] = (LinearLayout)findViewById(R.id.de_3d_noise_layout);
        mLayout[8] = (LinearLayout)findViewById(R.id.de_3d_noise_strength_layout);
        mLayout[9] = (LinearLayout)findViewById(R.id.movie_layout);
        if (mDisplayManager.supportedSNRSetting(mDisplayType)) {
            sync2dNoiseState(m2dNoiseId);
            mSupportedSNRSetting = true;
        } else{
            mLayout[6].setVisibility(View.GONE);
            //mMaxCtrl = mMaxCtrl-1;
            mSupportedSNRSetting = false;
        }
        setLayoutSelected(mCtrlIndex);

        mBrightness = (SeekBar)findViewById(R.id.brightness_bar);
        mBrightness.setMax(10);
        mContrast = (SeekBar)findViewById(R.id.contrast_bar);
        mContrast.setMax(10);
        mSaturation = (SeekBar)findViewById(R.id.saturation_bar);
        mSaturation.setMax(10);
        mDetail = (SeekBar)findViewById(R.id.detail_enhance_bar);
        mDetail.setMax(10);
        mEdgeSharp = (SeekBar)findViewById(R.id.edge_sharpening_bar);
        mEdgeSharp.setMax(10);
        setAllSeekBarProgress();
        mBrightness.setOnSeekBarChangeListener(this);
        mContrast.setOnSeekBarChangeListener(this);
        mSaturation.setOnSeekBarChangeListener(this);
        mDetail.setOnSeekBarChangeListener(this);
        mEdgeSharp.setOnSeekBarChangeListener(this);
    }

    private void setLayoutSelected(int index) {
        for (int i = 0; i < mMaxCtrl; i++)
            mLayout[i].setBackgroundColor(0x70666666);
        mLayout[index].requestFocus();
        mLayout[index].setBackgroundColor(0xc00845cc);
    }

    private void setAllSeekBarState(boolean state){
        mBrightness.setEnabled(state);
        mContrast.setEnabled(state);
        mSaturation.setEnabled(state);
        mDetail.setEnabled(state);
        mEdgeSharp.setEnabled(state);
    }

    private void setAllSeekBarProgress() {

        int value = mDisplayManager.getDisplayBright(mDisplayType);
        if (0 <= value) {
            mBrightness.setProgress(value);
            if (COLOR_CUSTOM == mColorId)
                mColorCustom.setBringt(value);
        }

        value = mDisplayManager.getDisplayContrast(mDisplayType);
        if (0 <= value) {
            mContrast.setProgress(value);
            if (COLOR_CUSTOM == mColorId)
                mColorCustom.setContrast(value);
        }

        value = mDisplayManager.getDisplaySaturation(mDisplayType);
        if (0 <= value) {
            mSaturation.setProgress(value);
            if (COLOR_CUSTOM == mColorId)
                mColorCustom.setSaturation(value);
        }

        value = mDisplayManager.getDisplayDetail(mDisplayType);
        if (0 <= value) {
            mDetail.setProgress(value);
            if (COLOR_CUSTOM == mColorId)
                mColorCustom.setDetail(value);
        }

        value = mDisplayManager.getDisplayEdge(mDisplayType);
        if (0 <= value) {
            mEdgeSharp.setProgress(value);
            if (COLOR_CUSTOM == mColorId)
                mColorCustom.setEdge(value);
        }

        if (COLOR_CUSTOM == mColorId)
            saveColorCustomValues();
    }

    private void syncAllSeekBarState() {
        if (((mEnhanceModeId == ENHANCE_ENABLE)
                || (mEnhanceModeId == ENHANCE_DEMO))
                && (mColorId == COLOR_CUSTOM)){
            setAllSeekBarState(true);
        } else {
            setAllSeekBarState(false);
        }
    }
    private void sync2dNoiseState(int noiseMode2d) {
        mTextStatus2d.setText(m2dNoiseEntries[noiseMode2d]);
        switch (noiseMode2d){
            case NOISE_DISABLE_2D:
                mDisplayManager.setSNRConfig(mDisplayType,mDisplayManager.SNR_FEATURE_MODE_DISABLE,0,0,0);
                break;
            case NOISE_LOW_2D:
                mDisplayManager.setSNRConfig(mDisplayType,mDisplayManager.SNR_FEATURE_MODE_LEVEL1,0,0,0);
                int noiseMode2d2=mDisplayManager.getSNRFeatureMode(mDisplayType);
                break;
            case NOISE_MIDDLE_2D:
                mDisplayManager.setSNRConfig(mDisplayType,mDisplayManager.SNR_FEATURE_MODE_LEVEL2,0,0,0);
                break;
            case NOISE_HIGH_2D:
                mDisplayManager.setSNRConfig(mDisplayType,mDisplayManager.SNR_FEATURE_MODE_LEVEL3,0,0,0);
                break;
        }
    }
    private void sync3dNoiseState(int noiseMode3d) {
        mTextStatus3d.setText(m3dNoiseEntries[noiseMode3d]);
        switch (noiseMode3d){
            case NOISE_DISABLE_3D:
            /**
　　            * @enable: open or close tnr, false: close; true: open
　　            * @mode: 1: MODE_ADAPTIVE; 2: MODE_FIX
　　         * @strength: 0: LEVEL_HIGH; 1: LEVEL_MIDDLE; 2: LEVEL_LOW
　　          */
                //VideoView.getInstance().getMediaPlayer().setTnr(false,1,0);
                break;
            case NOISE_ADAPTIVE_3D:
                //VideoView.getInstance().getMediaPlayer().setTnr(true,1,0);
                if(mEnhanceModeId == ENHANCE_DEMO){
                   //VideoView.getInstance().getMediaPlayer().setDiDemo(true, 0, 0, (int)mVideoWidth/2, (int)mVideoHeight);
                }
                break;
            case NOISE_FIX_3D:
                //VideoView.getInstance().getMediaPlayer().setTnr(true,2,0);
                if(mEnhanceModeId == ENHANCE_DEMO){
                   //VideoView.getInstance().getMediaPlayer().setDiDemo(true, 0, 0, (int)mVideoWidth/2, (int)mVideoHeight);
                }
                break;
        }

    }
    private void sync3dNoiseStrengthState(int noiseStrengthMode3d) {
        mTextStatus3dStrength.setText(m3dNoiseStrengthEntries[noiseStrengthMode3d]);
        if(m3dNoiseId == NOISE_DISABLE_3D){
            //VideoView.getInstance().getMediaPlayer().setTnr(false,1,0);
        }else{
            switch (noiseStrengthMode3d){
                case NOISE_3D_LEVEL_HIGH:
                    //VideoView.getInstance().getMediaPlayer().setTnr(true,m3dNoiseId,1);
                    break;
                case NOISE_3D_LEVEL_MIDDLE:
                    //VideoView.getInstance().getMediaPlayer().setTnr(true,m3dNoiseId,2);
                    break;
                case NOISE_3D_LEVEL_LOW:
                    //VideoView.getInstance().getMediaPlayer().setTnr(true,m3dNoiseId,3);
                    break;
            }
        }
    }
    private void syncMovieState(int movieMode) {
        mTextStatusMovie.setText(mMovieEntries[movieMode]);
        boolean isMovieMode=(movieMode==1)?true:false;
        //VideoView.getInstance().getMediaPlayer().setFmd(isMovieMode);
    }


    private void syncEnhanceModeState() {
        Log.d(TAG,"mDisplayManager.getDisplayEnhanceMode("+mDisplayType+") = "+mDisplayManager.getDisplayEnhanceMode(mDisplayType));
        mEnhanceModeId = mDisplayManager.getDisplayEnhanceMode(mDisplayType)
                % mMaxEnhanceModeNum;
        Log.d(TAG,"mEnhanceModeId:"+mEnhanceModeId);
        if(mEnhanceModeId < 0) {
            Log.e(TAG,"mEnhanceModeId error,check!");
            return;
        }
        mTextStatus.setText(mModeEntries[mEnhanceModeId]);
        if(mEnhanceModeId == ENHANCE_DEMO){
            enterDemoMode();
        }
    }

    private void syncColorModeState() {
        mTextStatus1.setText(mColorEntries[mColorId]);
    }

    private void syncControllerState() {
        syncEnhanceModeState();
        syncAllSeekBarState();
    }

    private int[] getSavedMode() {
        int colorMode = mSp.getInt(EDITOR_COLOR_MODE, DEFAULT_VALUE);
        int noise2dMode = mSp.getInt(EDITOR_2D_NOISE_MODE, NOISE_HIGH_2D);
        int noise3dMode = mSp.getInt(EDITOR_3D_NOISE_MODE, NOISE_ADAPTIVE_3D);
        int noise3dModeStrength = mSp.getInt(EDITOR_3D_NOISE_STRENGTH, NOISE_3D_LEVEL_HIGH);
        int movieMode = mSp.getInt(EDITOR_MOVIE_MODE, 1);
        int [] modeArray = {colorMode,noise2dMode,noise3dMode,noise3dModeStrength,movieMode};
        return modeArray;
    }

    private void saveMode() {
        mEditor.putInt(EDITOR_COLOR_MODE, mColorId);
        mEditor.putInt(EDITOR_2D_NOISE_MODE, m2dNoiseId);
        mEditor.putInt(EDITOR_3D_NOISE_MODE, m3dNoiseId);
        mEditor.putInt(EDITOR_3D_NOISE_STRENGTH, m3dNoiseStrengthId);
        mEditor.putInt(EDITOR_MOVIE_MODE, mMovieId);
        mEditor.commit();
    }

    private void saveColorCustomValues() {
        String values = String.format("%d,%d,%d,%d,%d",
                mColorCustom.getBringt(),
                mColorCustom.getContrast(),
                mColorCustom.getSaturation(),
                mColorCustom.getDetail(),
                mColorCustom.getEdge());
        mEditor.putString(EDITOR_COLOR_CUSTOM_VALUES, values);
        mEditor.commit();
    }

    private void setupColorCustomValues() {
        String values = mSp.getString(EDITOR_COLOR_CUSTOM_VALUES, null);
        if (null != values) {
            String[] valueArray = values.split(",");
            if (6 <= valueArray.length) {
                mColorCustom.setBringt(Integer.parseInt(valueArray[0]));
                mColorCustom.setContrast(Integer.parseInt(valueArray[1]));
                mColorCustom.setSaturation(Integer.parseInt(valueArray[2]));
                mColorCustom.setDetail(Integer.parseInt(valueArray[4]));
                mColorCustom.setEdge(Integer.parseInt(valueArray[5]));
            } else {
                Log.w(TAG, "valueArray.lenth=" + valueArray.length
                        + " is less than 6");
            }
            //} else {
            //    Log.d(TAG, "not find " + EDITOR_COLOR_CUSTOM_VALUES + " values");
        }
    };

    private void setDisplayColorMode(ColorMode colorMode) {
        mDisplayManager.setDisplayBright(mDisplayType, colorMode.getBringt());
        mDisplayManager.setDisplayContrast(mDisplayType, colorMode.getContrast());
        mDisplayManager.setDisplaySaturation(mDisplayType, colorMode.getSaturation());
        mDisplayManager.setDisplayDetail(mDisplayType, colorMode.getDetail());
        mDisplayManager.setDisplayEdge(mDisplayType, colorMode.getEdge());
    }

    private void setDisplayColorMode() {
        if (COLOR_STANDAR == mColorId)
            setDisplayColorMode(mColorStandard);
        else if (COLOR_SOFT == mColorId)
            setDisplayColorMode(mColorSoft);
        else if (COLOR_VIVID == mColorId)
            setDisplayColorMode(mColorVivid);
        else if (COLOR_CUSTOM == mColorId)
            setDisplayColorMode(mColorCustom);
        else
            Log.w(TAG, "unkown colorid(" + mColorId + ")");
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromTouch) {
        Log.d(TAG, "on seek bar id = " + seekBar.getId() +  " progress = " + progress );
        switch (seekBar.getId()) {
            case R.id.brightness_bar:
                Log.d(TAG, "on brightness_bar progress = " + progress);
                mDisplayManager.setDisplayBright(mDisplayType, progress);
                break;
            case R.id.contrast_bar:
                Log.d(TAG, "on contrast_bar progress = " + progress);
                mDisplayManager.setDisplayContrast(mDisplayType, progress);
                break;
            case R.id.saturation_bar:
                Log.d(TAG, "on saturation_bar progress = " + progress);
                mDisplayManager.setDisplaySaturation(mDisplayType, progress);
                break;
            case R.id.detail_enhance_bar:
                Log.d(TAG, "on detail_enhance_bar progress = " + progress);
                mDisplayManager.setDisplayDetail(mDisplayType, progress);
                break;
            case R.id.edge_sharpening_bar:
                Log.d(TAG, "on edge_sharpening_bar progress = " + progress);
                mDisplayManager.setDisplayEdge(mDisplayType, progress);
                break;
            default:
                Log.d(TAG, "on unknown progress = " + progress);
                return;
        }
        setAllSeekBarProgress();
        syncControllerState();
    }

    @Override
    public void onStart(){
        Log.d(TAG, "onStart");
        syncControllerState();
    }

    @Override
    public void dismiss() {
        Log.d(TAG, "on dismiss");
        super.dismiss();
    }

    public void enterDemoMode(){
        Log.i(TAG,"enterDemoMode");
        mDisplayManager.setDisplayEnhanceMode(mDisplayType, mEnhanceModeId);
        mDisplayManager.setSNRConfig(mDisplayType,mDisplayManager.SNR_FEATURE_MODE_DEMO,0,0,0);
        //VideoView.getInstance().getMediaPlayer().setDiDemo(true, 0, 0, (int)mVideoWidth/2, (int)mVideoHeight);
        Log.i(TAG,"demowidth:"+(int)mVideoWidth/2+"demoHeight"+(int)mVideoHeight);
    }
    public void exitDemoMode(){
        sync2dNoiseState(m2dNoiseId);
        //VideoView.getInstance().getMediaPlayer().setDiDemo(true, 0, 0,0,0 );
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {}

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {}

    @Override
    public void onClick(View v) {
        Log.d(TAG, "on click id = " + v.getId());
        syncControllerState();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_UP:
                if (mCtrlIndex > 0) {
                    if (mCtrlIndex == 7 && !mSupportedSNRSetting) {
                        mCtrlIndex = 5;
                    } else {
                        mCtrlIndex--;
                    }
                }
                setLayoutSelected(mCtrlIndex);
                break;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                if (mCtrlIndex < mMaxCtrl - 1) {
                    if (mCtrlIndex == 5 && !mSupportedSNRSetting) {
                        mCtrlIndex = 7;
                    } else {
                        mCtrlIndex++;
                    }
                }
                setLayoutSelected(mCtrlIndex);
                break;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                Log.d(TAG,"left,mCtrlIndex:"+mCtrlIndex+" ,before mEnhanceModeId:"+mEnhanceModeId);
                if (mCtrlIndex == 0) {
                    mEnhanceModeId = (mEnhanceModeId + mMaxEnhanceModeNum - 1)
                            % mMaxEnhanceModeNum;
                    Log.d(TAG,"left,after mEnhanceModeId:"+mEnhanceModeId);
                    if(mEnhanceModeId == ENHANCE_DEMO){
                        enterDemoMode();
                    }else{
                        if (0 != mDisplayManager.setDisplayEnhanceMode(
                                mDisplayType, mEnhanceModeId)) {
                            Log.w(TAG, "#setDisplayEnhanceMode(" + mEnhanceModeId + ") failed");
                        }
                        exitDemoMode();
                    }
                    syncControllerState();
                } else if (mCtrlIndex == 1) {
                    syncEnhanceModeState();
                    if ((mEnhanceModeId == ENHANCE_DEMO)
                            || (mEnhanceModeId == ENHANCE_ENABLE)) {
                        mColorId = (mColorId == 0) ?
                                (mColorEntries.length - 1) : (mColorId - 1);
                        setDisplayColorMode();
                        setAllSeekBarProgress();
                        syncAllSeekBarState();
                        syncColorModeState();
                    }
                } else if(mCtrlIndex == 6){
                    if((mEnhanceModeId != ENHANCE_DISABLE) && mSupportedSNRSetting){
                        m2dNoiseId = (m2dNoiseId +mMax2dNoiseModeNum - 1) % mMax2dNoiseModeNum;
                        sync2dNoiseState(m2dNoiseId);
                    }
                } else if(mCtrlIndex == 7){
                    if(mEnhanceModeId != ENHANCE_DISABLE){
                        m3dNoiseId = (m3dNoiseId +mMax3dNoiseModeNum - 1) % mMax3dNoiseModeNum;
                        sync3dNoiseState(m3dNoiseId);
                    }
                } else if(mCtrlIndex == 8){
                    if(mEnhanceModeId != ENHANCE_DISABLE && m3dNoiseId != NOISE_DISABLE_3D){
                        m3dNoiseStrengthId = (m3dNoiseStrengthId +mMax3dNoiseStrengthModeNum- 1) % mMax3dNoiseStrengthModeNum;
                        sync3dNoiseStrengthState(m3dNoiseStrengthId);
                    }
                } else if(mCtrlIndex == 9){
                    if(mEnhanceModeId != ENHANCE_DISABLE){
                        mMovieId = (mMovieId +mMaxmovieModeNum - 1) % mMaxmovieModeNum;
                        syncMovieState(mMovieId);
                    }
                }
                saveMode();
                break;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                Log.d(TAG,"right,mCtrlIndex:"+mCtrlIndex+" ,before mEnhanceModeId:"+mEnhanceModeId);
                if(mCtrlIndex == 0) {
                    mEnhanceModeId = (mEnhanceModeId + 1) % mMaxEnhanceModeNum;
                    Log.d(TAG,"right,after mEnhanceModeId:"+mEnhanceModeId);
                    if(mEnhanceModeId == ENHANCE_DEMO){
                        enterDemoMode();
                    }else{
                        if (0 != mDisplayManager.setDisplayEnhanceMode(
                                mDisplayType, mEnhanceModeId)) {
                            Log.w(TAG, "&setDisplayEnhanceMode(" + mEnhanceModeId + ") failed");
                        }
                        exitDemoMode();
                    }
                    syncControllerState();
                } else if (mCtrlIndex == 1) {
                    syncEnhanceModeState();
                    if ((mEnhanceModeId == ENHANCE_DEMO)
                            || (mEnhanceModeId == ENHANCE_ENABLE)) {
                        mColorId = (mColorId == mColorEntries.length - 1) ?
                                0 : (mColorId + 1);
                        setDisplayColorMode();
                        setAllSeekBarProgress();
                        syncAllSeekBarState();
                        syncColorModeState();
                    }
                } else if(mCtrlIndex == 6){
                    if((mEnhanceModeId != ENHANCE_DISABLE) && mSupportedSNRSetting){
                        m2dNoiseId = (m2dNoiseId + 1) % mMax2dNoiseModeNum;
                        sync2dNoiseState(m2dNoiseId);
                    }
                } else if(mCtrlIndex == 7){
                    if(mEnhanceModeId != ENHANCE_DISABLE){
                        m3dNoiseId = (m3dNoiseId + 1) % mMax3dNoiseModeNum;
                        sync3dNoiseState(m3dNoiseId);
                    }
                } else if(mCtrlIndex == 8){
                    if(mEnhanceModeId != ENHANCE_DISABLE && m3dNoiseId != NOISE_DISABLE_3D){
                        m3dNoiseStrengthId = (m3dNoiseStrengthId + 1) % mMax3dNoiseStrengthModeNum;
                        sync3dNoiseStrengthState(m3dNoiseStrengthId);
                    }
                } else if(mCtrlIndex == 9){
                    if(mEnhanceModeId != ENHANCE_DISABLE){
                        mMovieId = (mMovieId + 1) % mMaxmovieModeNum;
                        syncMovieState(mMovieId);
                    }
                }
                saveMode();
                break;
            case KeyEvent.KEYCODE_MENU:
                this.dismiss();
                break;
            case KeyEvent.KEYCODE_BACK:
                this.dismiss();
                break;
        }

        return true;
    }

}

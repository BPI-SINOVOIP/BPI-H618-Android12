package com.softwinner.TvdVideo;


import android.content.Context;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.IWindowManager;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.util.DisplayMetrics;
import android.view.WindowManager;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import com.softwinner.PQControl;
import com.softwinner.PQControl.*;
import java.util.LinkedHashMap;
import java.util.Map;

public class TvDisplayController extends IDisplayController implements SeekBar.OnSeekBarChangeListener,View.OnClickListener,PQControl.PQEventListener {

    private final String TAG = "TvDisplayController";

    private LinearLayout[] mLayout;
    private int mCtrlIndex;

    private TextView tv_picture_mode;
    private ImageButton ib_pm_switch_prev;
    private TextView tv_pm_result;
    private ImageButton ib_pm_switch_next;

    private TextView tv_backlight;
    private SeekBar sb_backlight;

    private TextView tv_contrast;
    private SeekBar sb_contrast;

    private TextView tv_brightness;
    private SeekBar sb_brightness;

    private TextView tv_sharpness;
    private SeekBar sb_sharpness;

    private TextView tv_color;
    private SeekBar sb_color;

    private TextView tv_tint;
    private SeekBar sb_tint;

    private TextView tv_dynamic_backlight;
    private ImageButton ib_db_switch_prev;
    private TextView tv_db_result;
    private ImageButton ib_db_switch_next;

    private TextView tv_DCI;
    private ImageButton ib_DCI_switch_prev;
    private TextView tv_DCI_result;
    private ImageButton ib_DCI_switch_next;

    private TextView tv_black_extension;
    private ImageButton ib_be_switch_prev;
    private TextView tv_be_result;
    private ImageButton ib_be_switch_next;

    private TextView tv_TNR;
    private ImageButton ib_TNR_switch_prev;
    private TextView tv_TNR_result;
    private ImageButton ib_TNR_switch_next;

    private TextView tv_SNR;
    private ImageButton ib_SNR_switch_prev;
    private TextView tv_SNR_result;
    private ImageButton ib_SNR_switch_next;

    private TextView tv_color_temperature;
    private ImageButton ib_ct_switch_prev;
    private TextView tv_ct_result;
    private ImageButton ib_ct_switch_next;

    private TextView tv_gamma;
    private ImageButton ib_gamma_switch_prev;
    private TextView tv_gamma_result;
    private ImageButton ib_gamma_switch_next;

    private String[] gammaList;
    private String[] colorTemperatureModeList;
    private String[] levelChoicesList;
    private String[] settingEnableChoicesList;

    private PQControl mPQControl;
    private List<String> mPictureModeListValues = null;
    private List<String> mPictureModeListKeys = null;
    private Map<String,String> mPictureModeMap = null;
    private int mMaxCtrl = 14;

    private int curPictureModePos = 0;
    private String curPictureModeName = "";
    private String curPictureModeKey = "";

    private int curDynamicBacklightPos = 0;
    private String curDynamicBacklightName;

    private int curDCIPos = 0;
    private String curDCIName;

    private int curBlackExtensionPos = 0;
    private String curBlackExtensionName;

    private int curTNRPos = 0;
    private String curTNRName;

    private int curSNRPos = 0;
    private String curSNRName;

    private int curColorTemperaturePos = 0;
    private String curColorTemperatureName;

    private int curGammaPos = 0;
    private String curGammaName;

    public TvDisplayController(Context context,int videoWidth, int videoHeight) {
        super(context, videoWidth,videoHeight);
    }

    private void initView() {
        gammaList = getSettingGammaChoicesList();
        colorTemperatureModeList = getColorTemperatureModeList();
        levelChoicesList = getLevelChoicesList();
        settingEnableChoicesList = getSettingEnableChoicesList();

        LayoutInflater inflate = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View menuView = inflate.inflate(R.layout.dialog_tv_dispsettings, null);
        this.getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide
                        |View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
        );
        this.setContentView(menuView);

        mPictureModeMap = getPictureModeList();
        mPictureModeListValues = new ArrayList<String>(mPictureModeMap.values());
        mPictureModeListKeys = new ArrayList<String>(mPictureModeMap.keySet());

        mLayout = new LinearLayout[14];
        mLayout[0] = (LinearLayout)findViewById(R.id.ll_picture_mode);
        mLayout[1] = (LinearLayout)findViewById(R.id.ll_backlight);
        mLayout[2] = (LinearLayout)findViewById(R.id.ll_contrast);
        mLayout[3] = (LinearLayout)findViewById(R.id.ll_brightness);
        mLayout[4] = (LinearLayout)findViewById(R.id.ll_sharpness);
        mLayout[5] = (LinearLayout)findViewById(R.id.ll_color);
        mLayout[6] = (LinearLayout)findViewById(R.id.ll_tint);
        mLayout[7] = (LinearLayout)findViewById(R.id.ll_dynamic_backlight);
        mLayout[8] = (LinearLayout)findViewById(R.id.ll_DCI);
        mLayout[9] = (LinearLayout)findViewById(R.id.ll_black_extension);
        mLayout[10] = (LinearLayout)findViewById(R.id.ll_TNR);
        mLayout[11] = (LinearLayout)findViewById(R.id.ll_SNR);
        mLayout[12] = (LinearLayout)findViewById(R.id.ll_color_temperature);
        mLayout[13] = (LinearLayout)findViewById(R.id.ll_gamma);

        tv_picture_mode = (TextView)findViewById(R.id.tv_picture_mode);
        ib_pm_switch_prev = (ImageButton)findViewById(R.id.ib_pm_switch_prev);
        ib_pm_switch_prev.setOnClickListener(this);
        tv_pm_result = (TextView)findViewById(R.id.tv_pm_result);
        ib_pm_switch_next = (ImageButton)findViewById(R.id.ib_pm_switch_next);
        ib_pm_switch_next.setOnClickListener(this);
        tv_picture_mode.setText(mContext.getResources().getString(R.string.setting_preset_name));

        tv_backlight = (TextView)findViewById(R.id.tv_backlight);
        sb_backlight = (SeekBar)findViewById(R.id.sb_backlight);
        sb_backlight.setMax(100);
        sb_backlight.setOnSeekBarChangeListener(this);
        tv_backlight.setText(mContext.getResources().getString(R.string.setting_backlight_name));

        tv_contrast = (TextView)findViewById(R.id.tv_contrast);
        sb_contrast = (SeekBar)findViewById(R.id.sb_contrast);
        sb_contrast.setMax(100);
        sb_contrast.setOnSeekBarChangeListener(this);
        tv_contrast.setText(mContext.getResources().getString(R.string.setting_contrast_name));

        tv_brightness = (TextView)findViewById(R.id.tv_brightness);
        sb_brightness = (SeekBar)findViewById(R.id.sb_brightness);
        sb_brightness.setMax(100);
        sb_brightness.setOnSeekBarChangeListener(this);
        tv_brightness.setText(mContext.getResources().getString(R.string.setting_brightness_name));

        tv_sharpness = (TextView)findViewById(R.id.tv_sharpness);
        sb_sharpness = (SeekBar)findViewById(R.id.sb_sharpness);
        sb_sharpness.setMax(100);
        sb_sharpness.setOnSeekBarChangeListener(this);
        tv_sharpness.setText(mContext.getResources().getString(R.string.setting_sharpness_name));

        tv_color = (TextView)findViewById(R.id.tv_color);
        sb_color = (SeekBar)findViewById(R.id.sb_color);
        sb_color.setMax(100);
        sb_color.setOnSeekBarChangeListener(this);
        tv_color.setText(mContext.getResources().getString(R.string.setting_color_name));

        tv_tint = (TextView)findViewById(R.id.tv_tint);
        sb_tint = (SeekBar)findViewById(R.id.sb_tint);
        sb_tint.setMax(100);
        sb_tint.setOnSeekBarChangeListener(this);
        tv_tint.setText(mContext.getResources().getString(R.string.setting_tint_name));

        tv_dynamic_backlight = (TextView)findViewById(R.id.tv_dynamic_backlight);
        ib_db_switch_prev = (ImageButton)findViewById(R.id.ib_db_switch_prev);
        ib_db_switch_prev.setOnClickListener(this);
        tv_db_result = (TextView)findViewById(R.id.tv_db_result);
        ib_db_switch_next = (ImageButton)findViewById(R.id.ib_db_switch_next);
        ib_db_switch_next.setOnClickListener(this);
        tv_dynamic_backlight.setText(mContext.getResources().getString(R.string.setting_dynamic_backlight_name));

        tv_DCI = (TextView)findViewById(R.id.tv_DCI);
        ib_DCI_switch_prev = (ImageButton)findViewById(R.id.ib_DCI_switch_prev);
        ib_DCI_switch_prev.setOnClickListener(this);
        tv_DCI_result = (TextView)findViewById(R.id.tv_DCI_result);
        ib_DCI_switch_next = (ImageButton)findViewById(R.id.ib_DCI_switch_next);
        ib_DCI_switch_next.setOnClickListener(this);
        tv_DCI.setText(mContext.getResources().getString(R.string.setting_dci_name));

        tv_black_extension = (TextView)findViewById(R.id.tv_black_extension);
        ib_be_switch_prev = (ImageButton)findViewById(R.id.ib_be_switch_prev);
        ib_be_switch_prev.setOnClickListener(this);
        tv_be_result = (TextView)findViewById(R.id.tv_be_result);
        ib_be_switch_next = (ImageButton)findViewById(R.id.ib_be_switch_next);
        ib_be_switch_next.setOnClickListener(this);
        tv_black_extension.setText(mContext.getResources().getString(R.string.setting_blackext_name));

        tv_TNR = (TextView)findViewById(R.id.tv_TNR);
        ib_TNR_switch_prev = (ImageButton)findViewById(R.id.ib_TNR_switch_prev);
        ib_TNR_switch_prev.setOnClickListener(this);
        tv_TNR_result = (TextView)findViewById(R.id.tv_TNR_result);
        ib_TNR_switch_next = (ImageButton)findViewById(R.id.ib_TNR_switch_next);
        ib_TNR_switch_next.setOnClickListener(this);
        tv_TNR.setText(mContext.getResources().getString(R.string.setting_tnr_name));

        tv_SNR = (TextView)findViewById(R.id.tv_SNR);
        ib_SNR_switch_prev = (ImageButton)findViewById(R.id.ib_SNR_switch_prev);
        ib_SNR_switch_prev.setOnClickListener(this);
        tv_SNR_result = (TextView)findViewById(R.id.tv_SNR_result);
        ib_SNR_switch_next = (ImageButton)findViewById(R.id.ib_SNR_switch_next);
        ib_SNR_switch_next.setOnClickListener(this);
        tv_SNR.setText(mContext.getResources().getString(R.string.setting_snr_name));

        tv_color_temperature = (TextView)findViewById(R.id.tv_color_temperature);
        ib_ct_switch_prev = (ImageButton)findViewById(R.id.ib_ct_switch_prev);
        ib_ct_switch_prev.setOnClickListener(this);
        tv_ct_result = (TextView)findViewById(R.id.tv_ct_result);
        ib_ct_switch_next = (ImageButton)findViewById(R.id.ib_ct_switch_next);
        ib_ct_switch_next.setOnClickListener(this);
        tv_color_temperature.setText(mContext.getResources().getString(R.string.setting_colortemp_name));

        tv_gamma = (TextView)findViewById(R.id.tv_gamma);
        ib_gamma_switch_prev = (ImageButton)findViewById(R.id.ib_gamma_switch_prev);
        ib_gamma_switch_prev.setOnClickListener(this);
        tv_gamma_result = (TextView)findViewById(R.id.tv_gamma_result);
        ib_gamma_switch_next = (ImageButton)findViewById(R.id.ib_gamma_switch_next);
        ib_gamma_switch_next.setOnClickListener(this);
        tv_gamma.setText(mContext.getResources().getString(R.string.setting_gamma_name));
    }

    private void refreshView() {
        curPictureModeName = getPictureModeName();
        curPictureModePos = findCurPictureModePos(curPictureModeName);
        tv_pm_result.setText(mPictureModeListValues.get(curPictureModePos));

        curDynamicBacklightName = getDynamicBacklight();
        tv_db_result.setText(curDynamicBacklightName);
        curDynamicBacklightPos = findCurSettingEnableChoicesPos(curDynamicBacklightName);

        curDCIName = getDCI();
        tv_DCI_result.setText(curDCIName);
        curDCIPos = findCurLevelChoicesPos(curDCIName);

        curBlackExtensionName = getBlackExtension();
        tv_be_result.setText(curBlackExtensionName);
        curBlackExtensionPos = findCurLevelChoicesPos(curBlackExtensionName);

        curTNRName = getTNR();
        tv_TNR_result.setText(curTNRName);
        curTNRPos = findCurLevelChoicesPos(curTNRName);

        curSNRName = getSNR();
        tv_SNR_result.setText(curSNRName);
        curSNRPos = findCurLevelChoicesPos(curSNRName);

        curColorTemperatureName = getColorTemperature();
        tv_ct_result.setText(curColorTemperatureName);
        curColorTemperaturePos = findCurColorTemperatureModePos(curColorTemperatureName);

        curGammaName = getGammaFactor();
        tv_gamma_result.setText(curGammaName);
        curGammaPos = findCurLevelChoicesPos(curGammaName);

        Log.d(TAG,"data: curPictureModePos:"+curPictureModePos);
        Log.d(TAG,"data: getPictureModeName:"+curPictureModeName);
        Log.d(TAG,"data: getDynamicBacklight:"+curDynamicBacklightName);
        Log.d(TAG,"data: getDCI:"+curDCIName);
        Log.d(TAG,"data: getBlackExtension:"+curBlackExtensionName);
        Log.d(TAG,"data: getTNR:"+curTNRName);
        Log.d(TAG,"data: getSNR:"+curSNRName);
        Log.d(TAG,"data: getColorTemperature:"+curColorTemperatureName);
        Log.d(TAG,"data: getGammaFactor:"+curGammaName);

        int sb_backlight_progress = getBasicControl(PQControl.PQ_TYPE_BACKLIGHT);
        int sb_contrast_progress = getBasicControl(PQControl.PQ_BASIC_CONTRAST);
        int sb_brightness_progress = getBasicControl(PQControl.PQ_BASIC_BRIGHTNESS);
        int sb_sharpness_progress = getBasicControl(PQControl.PQ_BASIC_SHARPNESS);
        int sb_color_progress = getBasicControl(PQControl.PQ_BASIC_SATURATION);
        int sb_tint_progress = getBasicControl(PQControl.PQ_BASIC_HUE);

        Log.d(TAG,"data: PQ_TYPE_BACKLIGHT:"+sb_backlight_progress);
        Log.d(TAG,"data: PQ_BASIC_CONTRAST:"+sb_contrast_progress);
        Log.d(TAG,"data: PQ_BASIC_BRIGHTNESS:"+sb_brightness_progress);
        Log.d(TAG,"data: PQ_BASIC_SHARPNESS:"+sb_sharpness_progress);
        Log.d(TAG,"data: PQ_BASIC_SATURATION:"+sb_color_progress);
        Log.d(TAG,"data: PQ_BASIC_HUE:"+sb_tint_progress);

        sb_backlight.setProgress(sb_backlight_progress);
        sb_contrast.setProgress(sb_contrast_progress);
        sb_brightness.setProgress(sb_brightness_progress);
        sb_sharpness.setProgress(sb_sharpness_progress);
        sb_color.setProgress(sb_color_progress);
        sb_tint.setProgress(sb_tint_progress);
    }

    @Override
    public void onStart(){
        Log.d(TAG, "onStart");
        mPQControl = new PQControl(mContext);
        mPQControl.setEventListener(this,true);
        mCtrlIndex = 0;
        initView();
        refreshView();
        setLayoutSelected(mCtrlIndex);
    }

    @Override
    public void onEvent(){
        Log.d(TAG,"pqcontrol onEvent,need refreshView");
        if(tv_pm_result != null) {
            tv_pm_result.postDelayed(new Runnable() {
                @Override
                public void run() {
                    refreshView();
                }
            },500);
        }
    }

    @Override
    public void dismiss() {
        Log.d(TAG, "on dismiss");
        if (mPQControl != null) {
            Log.d(TAG,"unEventListener");
            mPQControl.setEventListener(this,false);
            mPQControl = null;
        }
        super.dismiss();
    }

    private boolean isCustomMode() {
        if(curPictureModeName!=null && curPictureModeName.equals("custom")) {
            return true;
        }
        return false;
    }

    private void changeCustomMode() {
        Log.d(TAG,"data: changeCustomMode");
        refreshView();
        mCtrlIndex = 0;
        setLayoutSelected(mCtrlIndex);
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromTouch) {
        Log.d(TAG,"data: onProgressChanged progress:"+progress+" ,fromTouch:"+fromTouch);
        if(fromTouch) {
            switch (seekBar.getId()) {
                case R.id.sb_backlight:
                    Log.d(TAG, "on sb_backlight progress = " + progress);
                    setBasicControl(PQControl.PQ_TYPE_BACKLIGHT,progress);
                    break;
                case R.id.sb_contrast:
                    Log.d(TAG, "on sb_contrast progress = " + progress);
                    setBasicControl(PQControl.PQ_BASIC_CONTRAST,progress);
                    break;
                case R.id.sb_brightness:
                    Log.d(TAG, "on sb_brightness progress = " + progress);
                    setBasicControl(PQControl.PQ_BASIC_BRIGHTNESS,progress);
                    break;
                case R.id.sb_sharpness:
                    Log.d(TAG, "on sb_sharpness progress = " + progress);
                    setBasicControl(PQControl.PQ_BASIC_SHARPNESS,progress);
                    break;
                case R.id.sb_color:
                    Log.d(TAG, "on sb_color progress = " + progress);
                    setBasicControl(PQControl.PQ_BASIC_SATURATION,progress);
                    break;
                case R.id.sb_tint:
                    Log.d(TAG, "on sb_tint progress = " + progress);
                    setBasicControl(PQControl.PQ_BASIC_HUE,progress);
                    break;
                default:
                    Log.d(TAG, "on unknown progress = " + progress);
                    return;
            }
            if(!isCustomMode()) {
                changeCustomMode();
            }
        }
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        Log.d(TAG,"onStopTrackingTouch");
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        Log.d(TAG,"onStartTrackingTouch");
    }

    @Override
    public void onClick(View v) {

    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_UP:
                if (mCtrlIndex > 0)
                    mCtrlIndex--;
                setLayoutSelected(mCtrlIndex);
                break;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                if (mCtrlIndex < mMaxCtrl - 1)
                    mCtrlIndex++;
                setLayoutSelected(mCtrlIndex);
                break;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                onKeyDrap(true);
                break;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                onKeyDrap(false);
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

    private void onKeyDrap(boolean isLeft) {
        if(isLeft){
            Log.d(TAG,"data: <------------------- onKeyDrapLeft");
        } else {
            Log.d(TAG,"data: onKeyDrapRight------------------->");
        }
        if(mCtrlIndex == 0) {
            if(isLeft) {
                if(curPictureModePos > 0) {
                    curPictureModePos--;
                } else if(curPictureModePos == 0) {
                    curPictureModePos = mPictureModeMap.size() - 1;
                }
            } else {
                if(curPictureModePos < mPictureModeMap.size() - 1) {
                    curPictureModePos ++;
                } else if(curPictureModePos == mPictureModeMap.size() - 1) {
                    curPictureModePos = 0;
                }
            }
            setPictureMode(mPictureModeListKeys.get(curPictureModePos));
            tv_pm_result.setText(mPictureModeListValues.get(curPictureModePos));
            refreshView();
        } else if(mCtrlIndex == 7){
            if(!isCustomMode()) {
                changeCustomMode();
            } else {
                if(isLeft) {
                    if(curDynamicBacklightPos > 0) {
                        curDynamicBacklightPos--;
                    } else if(curDynamicBacklightPos == 0) {
                        curDynamicBacklightPos = settingEnableChoicesList.length - 1;
                    }
                } else {
                    if(curDynamicBacklightPos < settingEnableChoicesList.length - 1) {
                        curDynamicBacklightPos++;
                    } else if(curDynamicBacklightPos == settingEnableChoicesList.length - 1) {
                        curDynamicBacklightPos = 0;
                    }
                }
                curDynamicBacklightName = settingEnableChoicesList[curDynamicBacklightPos];
                setDynamicBacklight(curDynamicBacklightPos);
                tv_db_result.setText(curDynamicBacklightName);
            }
        } else if(mCtrlIndex == 8){
            if(!isCustomMode()) {
                changeCustomMode();
            } else {
                if(isLeft) {
                    if(curDCIPos > 0) {
                        curDCIPos--;
                    } else if(curDCIPos == 0) {
                        curDCIPos = levelChoicesList.length - 1;
                    }
                } else {
                    if(curDCIPos < levelChoicesList.length - 1) {
                        curDCIPos++;
                    } else if(curDCIPos == levelChoicesList.length - 1) {
                        curDCIPos = 0;
                    }
                }
                curDCIName = levelChoicesList[curDCIPos];
                setDCI(curDCIPos);
                tv_DCI_result.setText(curDCIName);
            }
        } else if(mCtrlIndex == 9){
            if(!isCustomMode()) {
                changeCustomMode();
            } else {
                if(isLeft) {
                    if(curBlackExtensionPos > 0) {
                        curBlackExtensionPos--;
                    } else if(curBlackExtensionPos == 0) {
                        curBlackExtensionPos = levelChoicesList.length - 1;
                    }
                } else {
                    if(curBlackExtensionPos < levelChoicesList.length - 1) {
                        curBlackExtensionPos++;
                    } else if(curBlackExtensionPos == levelChoicesList.length - 1) {
                        curBlackExtensionPos = 0;
                    }
                }
                curBlackExtensionName = levelChoicesList[curBlackExtensionPos];
                setBlackExtension(curBlackExtensionPos);
                tv_be_result.setText(curBlackExtensionName);
            }
        } else if(mCtrlIndex == 10){
            if(!isCustomMode()) {
                changeCustomMode();
            } else {
                if(isLeft) {
                    if(curTNRPos > 0) {
                        curTNRPos--;
                    } else if(curTNRPos == 0) {
                        curTNRPos = levelChoicesList.length - 1;
                    }
                } else {
                    if(curTNRPos < levelChoicesList.length - 1) {
                        curTNRPos++;
                    } else if(curTNRPos == levelChoicesList.length - 1) {
                        curTNRPos = 0;
                    }
                }
                curTNRName = levelChoicesList[curTNRPos];
                setTNR(curTNRPos);
                tv_TNR_result.setText(curTNRName);
            }
        } else if(mCtrlIndex == 11){
            if(!isCustomMode()) {
                changeCustomMode();
            } else {
                if(isLeft) {
                    if(curSNRPos > 0) {
                        curSNRPos--;
                    } else if(curSNRPos == 0) {
                        curSNRPos = levelChoicesList.length - 1;
                    }
                } else {
                    if(curSNRPos < levelChoicesList.length - 1) {
                        curSNRPos++;
                    } else if(curSNRPos == levelChoicesList.length - 1) {
                        curSNRPos = 0;
                    }
                }
                curSNRName = levelChoicesList[curSNRPos];
                setDCI(curSNRPos);
                tv_SNR_result.setText(curSNRName);
            }
        } else if(mCtrlIndex == 12){
            if(!isCustomMode()) {
                changeCustomMode();
            } else {
                if(isLeft) {
                    if(curColorTemperaturePos > 0) {
                        curColorTemperaturePos--;
                    } else if(curColorTemperaturePos == 0) {
                        curColorTemperaturePos = colorTemperatureModeList.length - 1;
                    }
                } else {
                    if(curColorTemperaturePos < colorTemperatureModeList.length - 1) {
                        curColorTemperaturePos++;
                    } else if(curColorTemperaturePos == colorTemperatureModeList.length - 1) {
                        curColorTemperaturePos = 0;
                    }
                }
                curColorTemperatureName = colorTemperatureModeList[curColorTemperaturePos];
                setColorTemperature(curColorTemperaturePos);
                tv_ct_result.setText(curColorTemperatureName);
            }
        } else if(mCtrlIndex == 13){
            if(!isCustomMode()) {
                changeCustomMode();
            } else {
                if(isLeft) {
                    if(curGammaPos > 0) {
                        curGammaPos--;
                    } else if(curGammaPos == 0) {
                        curGammaPos = gammaList.length - 1;
                    }
                } else {
                    if(curGammaPos < gammaList.length - 1) {
                        curGammaPos++;
                    } else if(curGammaPos == gammaList.length - 1) {
                        curGammaPos = 0;
                    }
                }
                curGammaName = gammaList[curGammaPos];
                setGammaFactor(curGammaPos);
                tv_gamma_result.setText(curGammaName);
            }
        }
    }

    private void setLayoutSelected(int index) {
        int size = mLayout.length;
        if(index >= size) return;
        for (int i = 0; i < size; i++){
            mLayout[i].setBackgroundColor(0x70666666);
        }
        mLayout[index].requestFocus();
        mLayout[index].setBackgroundColor(0xc00845cc);
    }

    public String[] getLevelChoicesList() {
        if(mPQControl == null) return null;
        return mPQControl.getLevelChoicesList();
    }

    public String[] getSettingEnableChoicesList() {
        if(mPQControl == null) return null;
        return mPQControl.getSettingEnableChoicesList();
    }

    public String[] getColorTemperatureModeList() {
        if(mPQControl == null) return null;
        return mPQControl.getColorTemperatureModeList();
    }

    public String[] getSettingGammaChoicesList() {
        if(mPQControl == null) return null;
        return mPQControl.getSettingGammaChoicesList();
    }

    public Map<String,String> getPictureModeList() {
        if(mPQControl == null) return null;
        return mPQControl.getPictureModeList();
    }

    public int setPictureMode(String mode){
        if(mPQControl == null) return -1;
        return mPQControl.setPictureMode(mode);
    }

    public String getPictureModeName() {
        if(mPQControl == null) return null;
        return mPQControl.getPictureModeName();
    }

    public int setBasicControl(int type, int level) {
        if(mPQControl == null) return -1;
        return mPQControl.setBasicControl(type , level);
    }

    public int getBasicControl(int type) {
        if(mPQControl == null) return -1;
        return mPQControl.getBasicControl(type);
    }

    public int setDynamicBacklight(int level) {
        if(mPQControl == null) return -1;
        return mPQControl.setDynamicBacklight(level);
    }

    public String getDynamicBacklight() {
        if(mPQControl == null) return null;
        int key = mPQControl.getDynamicBacklight();
        return settingEnableChoicesList[key];
    }

    public int setDCI(int level) {
        if(mPQControl == null) return -1;
        return mPQControl.setDCI(level);
    }

    public String getDCI() {
        if(mPQControl == null) return null;
        int key = mPQControl.getDCI();
        return levelChoicesList[key];
    }

    public int setBlackExtension(int level) {
        if(mPQControl == null) return -1;
        return mPQControl.setBlackExtension(level);
    }

    public String getBlackExtension() {
        if(mPQControl == null) return null;
        int key = mPQControl.getBlackExtension();
        return levelChoicesList[key];
    }

    public int setTNR(int level) {
        if(mPQControl == null) return -1;
        return mPQControl.setTNR(level);
    }
    public String getTNR() {
        if(mPQControl == null) return null;
        int key = mPQControl.getTNR();
        return levelChoicesList[key];
    }

    public int setSNR(int level) {
        if(mPQControl == null) return -1;
        return mPQControl.setSNR(level);
    }
    public String getSNR() {
        if(mPQControl == null) return null;
        int key = mPQControl.getSNR();
        return levelChoicesList[key];
    }

    public int setColorTemperature(int level) {
        if(mPQControl == null) return -1;
        return mPQControl.setColorTemperature(level);
    }

    public String getColorTemperature() {
        if(mPQControl == null) return null;
        int key = mPQControl.getColorTemperature();
        return colorTemperatureModeList[key];
    }

    public int setGammaFactor(int level) {
        if(mPQControl == null) return -1;
        return mPQControl.setGammaFactor(level);
    }

    public String getGammaFactor() {
        if(mPQControl == null) return null;
        int key = mPQControl.getGammaFactor();
        return gammaList[key];
    }

    private int findCurPictureModePos(String pictureModeKey) {
        int pos = 0,size = mPictureModeListKeys.size();
        for(int i = 0; i< size; i++) {
            //Log.d(TAG,"data: mPictureModeListKeys["+i+"]:"+mPictureModeListKeys.get(i));
            if(mPictureModeListKeys.get(i).equals(pictureModeKey)) {
                pos = i;
            }
        }
        return pos;
    }

    private int findCurSettingEnableChoicesPos(String settingEnableChoicesName) {
        int size = settingEnableChoicesList.length;
        int pos = 0;
        for(int i = 0; i < size; i++) {
            if(settingEnableChoicesList[i].equals(settingEnableChoicesName)) {
                pos = i;
            }
        }
        return pos;
    }

    private int findCurLevelChoicesPos(String levelChoicesName) {
        int size = levelChoicesList.length;
        int pos = 0;
        for(int i = 0; i < size; i++) {
            if(levelChoicesList[i].equals(levelChoicesName)) {
                pos = i;
            }
        }
        return pos;
    }

    private int findCurColorTemperatureModePos(String colorTemperatureModeName) {
        int size = colorTemperatureModeList.length;
        int pos = 0;
        for(int i = 0; i < size; i++) {
            if(colorTemperatureModeList[i].equals(colorTemperatureModeName)) {
                pos = i;
            }
        }
        return pos;
    }

    private int findCurGammaPos(String gammaName) {
        int size = gammaList.length;
        int pos = 0;
        for(int i = 0; i < size; i++) {
            if(gammaList[i].equals(gammaName)) {
                pos = i;
            }
        }
        return pos;
    }
}

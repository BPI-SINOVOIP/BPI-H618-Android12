package com.softwinner.hdmi_cts;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import com.softwinner.utils.HdmiCapability;
import com.softwinner.utils.HdmiOutputConfig;
import com.softwinner.utils.HdmiCtsUtils;

import android.media.AudioManager;
import android.media.AudioManagerEx;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.SimpleAdapter;
import android.widget.Toast;
import android.widget.TextView;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import android.content.ActivityNotFoundException;
import android.net.Uri;
import android.media.MediaPlayer;
import java.io.IOException;
import android.provider.Settings;

import android.graphics.Color;

public class MainActivity extends Activity implements OnClickListener, OnCheckedChangeListener, View.OnFocusChangeListener, android.content.DialogInterface.OnClickListener {
    final static String LOG_TAG = "HDMI_CTS";
    private static String HDMIINTENT = "android.intent.action.HDMI_PLUGGED";
    private static int MSG_HDMI_IN = 1;

    private String mMusicPath;
    int REQUEST_MUSIC=1;
    public RadioGroup mRgDeviceType;
    public RadioButton mRbDeviceTypeHdmi;
    public RadioButton mRbDeviceTypeDvi;

    public RadioGroup mRgColorSpace;
    public RadioButton mRbColorSpaceRG888;
    public RadioButton mRbColorSpaceYUV444;
    public RadioButton mRbColorSpaceYUV422;
    public RadioButton mRbColorSpaceYUV420;

    public RadioGroup mRgColorDepth;
    public RadioButton mRbColorDepth24;
    public RadioButton mRbColorDepth30;
    public RadioButton mRbColorDepth36;

    public RadioGroup mRgColorimetry;
    public RadioButton mRbColorimetryBT601;
    public RadioButton mRbColorimetryBT709;
    public RadioButton mRbColorimetryBT2020;

    public RadioGroup mRgValueRange;
    public RadioButton mRbValueRangeFull;
    public RadioButton mRbValueRangeLimit;
    public RadioButton mRbValueRangeDefault;

    public RadioGroup mRgScanMode;
    public RadioButton mRbScanModeDefault;
    public RadioButton mRbScanModeOver;
    public RadioButton mRbScanModeUnder;

    public RadioGroup mRgHdrMode;
    public RadioButton mRbHdrModeSDR;
    public RadioButton mRbHdrModeHDR;
    public RadioButton mRbHdrModeHLG;

    public RadioGroup mRgAspectRatio;
    public RadioButton mRbAspectRatioSameAsPicture;
    public RadioButton mRbAspectRatio43;
    public RadioButton mRbAspectRatio169;
    public RadioButton mRbAspectRatio149;

    public RadioGroup mRgHdcp;
    public RadioButton mRbHdcpTrue;
    public RadioButton mRbHdcpFalse;

    public RadioGroup mRgMusic;
    public RadioButton mRbMusicPassThrough;
    public RadioButton mRbMusicNotPassThrough;

    public Button mBtOK;
    public Button mBtSelect;

    public TextView mTextMusicPath;
    public Button mSelectMusic;
    public Button mStartStopMusic;
    public MediaPlayer mp;
    public HdmiCapability mHdmiCapability;
    public HdmiOutputConfig mHdmiOutputConfig;

    private AudioManagerEx mAudioMangerEx;
    private ArrayList<String> mChannels;

    public RadioGroup mRgHpd;
    public RadioButton mRbHpdEnable;
    public RadioButton mRbHpdDisable;
    public int mHpdEnable;

    public RadioGroup mRgCecStandby;
    public RadioButton mRbCecStandbyEnable;
    public RadioButton mRbCecStandbyDisable;
    public int mCecStandbyEnable;

    private boolean StartOrStop;

    List<Map<String, String>> dataList = new ArrayList<Map<String, String>>();
    public  final static String[] hdmiOutputInfos = {
        "DISP_TV_MOD_480I",
        "DISP_TV_MOD_576I",
        "DISP_TV_MOD_480P",
        "DISP_TV_MOD_576P",
        "DISP_TV_MOD_720P_50HZ",
        "DISP_TV_MOD_720P_60HZ" ,
        "DISP_TV_MOD_1080I_50HZ" ,
        "DISP_TV_MOD_1080I_60HZ",
        "DISP_TV_MOD_1080P_24HZ",
        "DISP_TV_MOD_1080P_50HZ",
        "DISP_TV_MOD_1080P_60HZ",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "Invalid Value",
        "DISP_TV_MOD_1080P_24HZ_3D_FP",
        "DISP_TV_MOD_720P_50HZ_3D_FP",
        "DISP_TV_MOD_720P_60HZ_3D_FP",
        "DISP_TV_MOD_1080P_25HZ",
        "DISP_TV_MOD_1080P_30HZ",
        "DISP_TV_MOD_3840_2160P_30HZ",
        "DISP_TV_MOD_3840_2160P_25HZ",
        "DISP_TV_MOD_3840_2160P_24HZ",
        "DISP_TV_MOD_4096_2160P_24HZ",
        "DISP_TV_MOD_4096_2160P_25HZ",
        "DISP_TV_MOD_4096_2160P_30HZ",
        "DISP_TV_MOD_3840_2160P_60HZ",
        "DISP_TV_MOD_4096_2160P_60HZ",
        "DISP_TV_MOD_3840_2160P_50HZ",};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //find vid id for UI
        initViewId();

        mHdmiCapability = HdmiCtsUtils.getHdmiCapability();
        parseHdmiCapability(mHdmiCapability);
        int[] supportmodes = HdmiCtsUtils.getHdmiSupportMode();
        Arrays.sort(supportmodes);
        for(int i=0;i<supportmodes.length;i++) {
            Map<String, String> map = new HashMap<String,String>();
            map.put("name", hdmiOutputInfos[supportmodes[i]]);
            dataList.add(map);
        }

        Log.d(LOG_TAG, "Supported Mode cnt: " + supportmodes.length);
        for (int i = 0; i < supportmodes.length; i++) {
            Log.d(LOG_TAG, "   " + Integer.toHexString(supportmodes[i]));
        }
        mHdmiOutputConfig = HdmiCtsUtils.getHdmiOutputConfig();
        parseHdmiOutputConfig(mHdmiOutputConfig);
        //hdmictsUtils.setHdmiOutputConfig(testconfig);
        mAudioMangerEx = new AudioManagerEx(getApplicationContext());
        mChannels = new ArrayList<String>();
        mChannels.add("AUDIO_HDMI");
        Settings.System.putInt(getContentResolver(), Settings.System.ENABLE_PASS_THROUGH,0);
        mAudioMangerEx.setAudioDeviceActive(mChannels, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);

        mHpdEnable = HdmiCtsUtils.getHotplugReportEnabled();
        if(mHpdEnable == 0) {
            mRbHpdDisable.setChecked(true);
        }else {
            mRbHpdEnable.setChecked(true);
        }

        mCecStandbyEnable = HdmiCtsUtils.getCecStandbyEnabled();
        if(mCecStandbyEnable == 0) {
            mRbCecStandbyDisable.setChecked(true);
        }else {
            mRbCecStandbyEnable.setChecked(true);
        }

        IntentFilter filter = new IntentFilter(HDMIINTENT);
        registerReceiver(HdmiReceiver,filter);
        StartOrStop = true;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(HdmiReceiver);
        if(mp != null){
            mp.stop();
            mp.release();
            mp = null;
            StartOrStop = true;
        }
    }


    public void initViewId() {
        mRgDeviceType = (RadioGroup)findViewById(R.id.rg_device_type);
        mRbDeviceTypeHdmi = (RadioButton) findViewById(R.id.rb_device_type_hdmi);
        mRbDeviceTypeDvi = (RadioButton) findViewById(R.id.rb_device_type_dvi);
        mRgDeviceType.setOnCheckedChangeListener(this);
        mRbDeviceTypeHdmi.setOnFocusChangeListener(this);
        mRbDeviceTypeDvi.setOnFocusChangeListener(this);

        mRgColorSpace= (RadioGroup)findViewById(R.id.rg_color_space);
        mRbColorSpaceRG888 = (RadioButton) findViewById(R.id.rb_color_space_rgb888);
        mRbColorSpaceYUV444 = (RadioButton) findViewById(R.id.rb_color_space_yuv444);
        mRbColorSpaceYUV422 = (RadioButton) findViewById(R.id.rb_color_space_yuv422);
        mRbColorSpaceYUV420 = (RadioButton) findViewById(R.id.rb_color_space_yuv420);
        mRgColorSpace.setOnCheckedChangeListener(this);
        mRbColorSpaceRG888.setOnFocusChangeListener(this);
        mRbColorSpaceYUV444.setOnFocusChangeListener(this);
        mRbColorSpaceYUV422.setOnFocusChangeListener(this);
        mRbColorSpaceYUV420.setOnFocusChangeListener(this);


        mRgColorDepth= (RadioGroup)findViewById(R.id.rg_color_depth);
        mRbColorDepth24 = (RadioButton) findViewById(R.id.rb_color_depth_24);
        mRbColorDepth30 = (RadioButton) findViewById(R.id.rb_color_depth_30);
        mRbColorDepth36 = (RadioButton) findViewById(R.id.rb_color_depth_36);
        mRgColorDepth.setOnCheckedChangeListener(this);
        mRbColorDepth24.setOnFocusChangeListener(this);
        mRbColorDepth30.setOnFocusChangeListener(this);
        mRbColorDepth36.setOnFocusChangeListener(this);

        mRgColorimetry= (RadioGroup)findViewById(R.id.rg_colorimetry);
        mRbColorimetryBT601 = (RadioButton) findViewById(R.id.rb_colorimetry_bt601);
        mRbColorimetryBT709 = (RadioButton) findViewById(R.id.rb_colorimetry_bt709);
        mRbColorimetryBT2020 = (RadioButton) findViewById(R.id.rb_colorimetry_bt2020);
        mRgColorimetry.setOnCheckedChangeListener(this);
        mRbColorimetryBT601.setOnFocusChangeListener(this);
        mRbColorimetryBT709.setOnFocusChangeListener(this);
        mRbColorimetryBT2020.setOnFocusChangeListener(this);



        mRgValueRange= (RadioGroup)findViewById(R.id.rg_value_range);
        mRbValueRangeFull = (RadioButton) findViewById(R.id.rb_value_range_full);
        mRbValueRangeLimit = (RadioButton) findViewById(R.id.rb_value_range_limit);
        mRbValueRangeDefault = (RadioButton) findViewById(R.id.rb_value_range_default);
        mRgValueRange.setOnCheckedChangeListener(this);
        mRbValueRangeFull.setOnFocusChangeListener(this);
        mRbValueRangeLimit.setOnFocusChangeListener(this);
        mRbValueRangeDefault.setOnFocusChangeListener(this);


        mRgScanMode= (RadioGroup)findViewById(R.id.rg_scan_mode);
        mRbScanModeDefault = (RadioButton) findViewById(R.id.rb_scan_mode_default);
        mRbScanModeOver = (RadioButton) findViewById(R.id.rb_scan_mode_over);
        mRbScanModeUnder = (RadioButton) findViewById(R.id.rb_scan_mode_under);
        mRgScanMode.setOnCheckedChangeListener(this);
        mRbScanModeDefault.setOnFocusChangeListener(this);
        mRbScanModeOver.setOnFocusChangeListener(this);
        mRbScanModeUnder.setOnFocusChangeListener(this);

        mRgHdrMode= (RadioGroup)findViewById(R.id.rg_hdr_mode);
        mRbHdrModeHDR = (RadioButton) findViewById(R.id.rb_hdr_mode_hdr);
        mRbHdrModeSDR = (RadioButton) findViewById(R.id.rb_hdr_mode_sdr);
        mRbHdrModeHLG = (RadioButton) findViewById(R.id.rb_hdr_mode_hlg);
        mRgHdrMode.setOnCheckedChangeListener(this);
        mRbHdrModeHDR.setOnFocusChangeListener(this);
        mRbHdrModeSDR.setOnFocusChangeListener(this);
        mRbHdrModeHLG.setOnFocusChangeListener(this);

        mRgAspectRatio = (RadioGroup) findViewById(R.id.rg_aspect_ratio);
        mRbAspectRatioSameAsPicture = (RadioButton) findViewById(R.id.rb_aspect_ratio_same_as_picture);
        mRbAspectRatio43 = (RadioButton) findViewById(R.id.rb_aspect_ratio_4_3);
        mRbAspectRatio169 = (RadioButton) findViewById(R.id.rb_aspect_ratio_16_9);
        mRbAspectRatio149 = (RadioButton) findViewById(R.id.rb_aspect_ratio_14_9);
        mRgAspectRatio.setOnCheckedChangeListener(this);
        mRbAspectRatioSameAsPicture.setOnFocusChangeListener(this);
        mRbAspectRatio43.setOnFocusChangeListener(this);
        mRbAspectRatio169.setOnFocusChangeListener(this);
        mRbAspectRatio149.setOnFocusChangeListener(this);


        mRgHdcp = (RadioGroup) findViewById(R.id.rg_hdcp);
        mRbHdcpTrue = (RadioButton) findViewById(R.id.rb_hdcp_true);
        mRbHdcpFalse = (RadioButton) findViewById(R.id.rb_hdcp_false);
        mRgHdcp.setOnCheckedChangeListener(this);
        mRbHdcpTrue.setOnFocusChangeListener(this);
        mRbHdcpFalse.setOnFocusChangeListener(this);


        mRgMusic = (RadioGroup) findViewById(R.id.music_radio);
        mRbMusicPassThrough = (RadioButton) findViewById(R.id.music_radio_true);
        mRbMusicNotPassThrough = (RadioButton) findViewById(R.id.music_radio_false);
        mRbMusicNotPassThrough.setChecked(true);
        mRgMusic.setOnCheckedChangeListener(this);
        mRbMusicPassThrough.setOnFocusChangeListener(this);
        mRbMusicNotPassThrough.setOnFocusChangeListener(this);

        mBtOK = (Button) findViewById(R.id.bt_ok);
        mBtOK.setOnClickListener(this);
        mBtSelect = (Button) findViewById(R.id.bt_get_hdmi_output_mode);
        mBtSelect.setOnClickListener(this);

        mTextMusicPath = (TextView) findViewById(R.id.music_showpath);

        mSelectMusic = (Button) findViewById(R.id.bt_music);
        mSelectMusic.setOnClickListener(this);

        mStartStopMusic = (Button) findViewById(R.id.bt_start_stop_music);
        mStartStopMusic.setOnClickListener(this);

        mRgHpd = (RadioGroup) findViewById(R.id.rg_hpd);
        mRbHpdEnable = (RadioButton) findViewById(R.id.rb_hpd_enable);
        mRbHpdDisable = (RadioButton) findViewById(R.id.rb_hpd_disable);
        mRgHpd.setOnCheckedChangeListener(this);
        mRbHpdEnable.setOnFocusChangeListener(this);
        mRbHpdDisable.setOnFocusChangeListener(this);

        mRgCecStandby = (RadioGroup) findViewById(R.id.rg_cecstandby);
        mRbCecStandbyEnable = (RadioButton) findViewById(R.id.rb_cecstandby_enable);
        mRbCecStandbyDisable = (RadioButton) findViewById(R.id.rb_cecstandby_disable);
        mRgCecStandby.setOnCheckedChangeListener(this);
        mRbCecStandbyEnable.setOnFocusChangeListener(this);
        mRbCecStandbyDisable.setOnFocusChangeListener(this);
    }

    public void parseHdmiCapability(HdmiCapability hdmiCapability) {
        int deviceType = hdmiCapability.deviceType;
        if((deviceType&HdmiCapability.SINK_DVI_MASK) == 0) {
            mRbDeviceTypeDvi.setEnabled(false);
            mRbDeviceTypeDvi.setTextColor(Color.parseColor("#BBBBBB"));
            mRbDeviceTypeDvi.setFocusable(false);
        }
        else{
            mRbDeviceTypeDvi.setEnabled(true);
            mRbDeviceTypeDvi.setFocusable(true);
        }
        if((deviceType&HdmiCapability.SINK_HDMI_MASK) == 0) {
            mRbDeviceTypeHdmi.setEnabled(false);
            mRbDeviceTypeHdmi.setTextColor(Color.parseColor("#BBBBBB"));
            mRbDeviceTypeHdmi.setFocusable(false);
        }
        else{
            mRbDeviceTypeHdmi.setEnabled(true);
            mRbDeviceTypeHdmi.setFocusable(true);
        }

        int colorSpace = hdmiCapability.supportedColorSpace;
        if((colorSpace&HdmiCapability.SAMPLING_MODE_RGB888) == 0) {
            mRbColorSpaceRG888.setEnabled(false);
            mRbColorSpaceRG888.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorSpaceRG888.setFocusable(false);
        }
        else{
            mRbColorSpaceRG888.setEnabled(true);
            mRbColorSpaceRG888.setFocusable(true);
        }
        if((colorSpace&HdmiCapability.SAMPLING_MODE_YUV444) == 0) {
            mRbColorSpaceYUV444.setEnabled(false);
            mRbColorSpaceYUV444.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorSpaceYUV444.setFocusable(false);
        }
        else{
            mRbColorSpaceYUV444.setEnabled(true);
            mRbColorSpaceYUV444.setFocusable(true);
        }
        if((colorSpace&HdmiCapability.SAMPLING_MODE_YUV422) == 0) {
            mRbColorSpaceYUV422.setEnabled(false);
            mRbColorSpaceYUV422.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorSpaceYUV422.setFocusable(false);
        }
        else{
            mRbColorSpaceYUV422.setEnabled(true);
            mRbColorSpaceYUV444.setFocusable(true);
        }
        if((colorSpace&HdmiCapability.SAMPLING_MODE_YUV420) == 0) {
            mRbColorSpaceYUV420.setEnabled(false);
            mRbColorSpaceYUV420.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorSpaceYUV420.setFocusable(false);
        }
        else{
            mRbColorSpaceYUV420.setEnabled(true);
            mRbColorSpaceYUV420.setFocusable(true);
        }

        int colorDepth = hdmiCapability.supportedColorDepth;
        if((colorDepth&HdmiCapability.COLOR_DEPTH_24BITS) == 0) {
            mRbColorDepth24.setEnabled(false);
            mRbColorDepth24.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorDepth24.setFocusable(false);
        }
        else{
            mRbColorDepth24.setEnabled(true);
            mRbColorDepth24.setFocusable(true);
        }
        if((colorDepth&HdmiCapability.COLOR_DEPTH_30BITS) == 0) {
            mRbColorDepth30.setEnabled(false);
            mRbColorDepth30.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorDepth30.setFocusable(false);
        }
        else{
            mRbColorDepth30.setEnabled(true);
            mRbColorDepth30.setFocusable(true);
        }
        if((colorDepth&HdmiCapability.COLOR_DEPTH_36BITS) == 0) {
            mRbColorDepth36.setEnabled(false);
            mRbColorDepth36.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorDepth36.setFocusable(false);
        }
        else{
            mRbColorDepth36.setEnabled(true);
            mRbColorDepth36.setFocusable(true);
        }

        int colorimetry = hdmiCapability.supportedColorimetry;
        if((colorimetry&HdmiCapability.COLORIMETRY_BT601) == 0) {
            mRbColorimetryBT601.setEnabled(false);
            mRbColorimetryBT601.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorimetryBT601.setFocusable(false);
        }
        else{
            mRbColorimetryBT601.setEnabled(true);
            mRbColorimetryBT601.setFocusable(true);
        }
        if((colorimetry&HdmiCapability.COLORIMETRY_BT709) == 0) {
            mRbColorimetryBT709.setEnabled(false);
            mRbColorimetryBT709.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorimetryBT709.setFocusable(false);
        }
        else{
            mRbColorimetryBT709.setEnabled(true);
            mRbColorimetryBT709.setFocusable(true);
        }
        if((colorimetry&HdmiCapability.COLORIMETRY_BT2020) == 0) {
            mRbColorimetryBT2020.setEnabled(false);
            mRbColorimetryBT2020.setTextColor(Color.parseColor("#BBBBBB"));
            mRbColorimetryBT2020.setFocusable(false);
        }
        else{
            mRbColorimetryBT2020.setEnabled(true);
            mRbColorimetryBT2020.setFocusable(true);
        }

        int valueRange = hdmiCapability.supportedValueRange;
        if((valueRange&HdmiCapability.VALUE_RANGE_FULL) == 0) {
            mRbValueRangeFull.setEnabled(false);
            mRbValueRangeFull.setTextColor(Color.parseColor("#BBBBBB"));
            mRbValueRangeFull.setFocusable(false);
        }
        else{
            mRbValueRangeFull.setEnabled(true);
            mRbValueRangeFull.setFocusable(true);
        }
        if((valueRange&HdmiCapability.VALUE_RANGE_LIMITED) == 0) {
            mRbValueRangeLimit.setEnabled(false);
            mRbValueRangeLimit.setTextColor(Color.parseColor("#BBBBBB"));
            mRbValueRangeLimit.setFocusable(false);
        }
        else{
            mRbValueRangeLimit.setEnabled(true);
            mRbValueRangeLimit.setFocusable(true);
        }

        int scanMode = hdmiCapability.supportedScanMode;
        if((scanMode&HdmiCapability.OVER_SCAN) == 0) {
            mRbScanModeOver.setEnabled(false);
            mRbScanModeOver.setTextColor(Color.parseColor("#BBBBBB"));
            mRbScanModeOver.setFocusable(false);
        }
        else{
            mRbScanModeOver.setEnabled(true);
            mRbScanModeOver.setFocusable(true);
        }
        if((scanMode&HdmiCapability.UNDER_SCAN) == 0) {
            mRbScanModeUnder.setEnabled(false);
            mRbScanModeUnder.setTextColor(Color.parseColor("#BBBBBB"));
            mRbScanModeUnder.setFocusable(false);
        }
        else{
            mRbScanModeUnder.setEnabled(true);
            mRbScanModeUnder.setFocusable(true);
        }

        int dataspaceMode = hdmiCapability.supportedHdrMode;
        if((dataspaceMode&HdmiCapability.DATASPACE_SDR) == 0) {
            mRbHdrModeSDR.setEnabled(false);
            mRbHdrModeSDR.setTextColor(Color.parseColor("#BBBBBB"));
            mRbHdrModeSDR.setFocusable(false);
        }
        else{
            mRbHdrModeSDR.setEnabled(true);
            mRbHdrModeSDR.setFocusable(true);
        }
        if((dataspaceMode&HdmiCapability.DATASPACE_HDR) == 0) {
            mRbHdrModeHDR.setEnabled(false);
            mRbHdrModeHDR.setTextColor(Color.parseColor("#BBBBBB"));
            mRbHdrModeHDR.setFocusable(false);
        }
        else{
            mRbHdrModeHDR.setEnabled(true);
            mRbHdrModeHDR.setFocusable(true);
        }
        if((dataspaceMode&HdmiCapability.DATASPACE_HLG) == 0) {
            mRbHdrModeHLG.setEnabled(false);
            mRbHdrModeHLG.setTextColor(Color.parseColor("#BBBBBB"));
            mRbHdrModeHLG.setFocusable(false);
        }
        else{
            mRbHdrModeHLG.setEnabled(true);
            mRbHdrModeHLG.setFocusable(true);
        }
    }


    public void parseHdmiOutputConfig(HdmiOutputConfig hdmiOutputConfig){
        int deviceType = hdmiOutputConfig.deviceType;
        if((deviceType&HdmiOutputConfig.SINK_DVI_MASK) != 0) {
            mRbDeviceTypeDvi.setChecked(true);
        }
        else if((deviceType&HdmiOutputConfig.SINK_HDMI_MASK) != 0) {
            mRbDeviceTypeHdmi.setChecked(true);
        }

        int colorSpace = hdmiOutputConfig.colorSpace;
        if((colorSpace&HdmiOutputConfig.SAMPLING_MODE_RGB888) != 0) {
            mRbColorSpaceRG888.setChecked(true);
        }
        else if((colorSpace&HdmiOutputConfig.SAMPLING_MODE_YUV444) != 0) {
            mRbColorSpaceYUV444.setChecked(true);
        }
        else if((colorSpace&HdmiOutputConfig.SAMPLING_MODE_YUV422) != 0) {
            mRbColorSpaceYUV422.setChecked(true);
        }
        else if((colorSpace&HdmiOutputConfig.SAMPLING_MODE_YUV420) != 0) {
            mRbColorSpaceYUV420.setChecked(true);
        }

        int colorDepth = hdmiOutputConfig.colorDepth;
        if((colorDepth&HdmiOutputConfig.COLOR_DEPTH_24BITS) != 0) {
            mRbColorDepth24.setChecked(true);
        }
        else if((colorDepth&HdmiOutputConfig.COLOR_DEPTH_30BITS) != 0) {
            mRbColorDepth30.setChecked(true);
        }
        else if((colorDepth&HdmiOutputConfig.COLOR_DEPTH_36BITS) != 0) {
            mRbColorDepth36.setChecked(true);
        }

        int colorimetry = hdmiOutputConfig.colorimetry;
        if((colorimetry&HdmiOutputConfig.COLORIMETRY_BT601) != 0) {
            mRbColorimetryBT601.setChecked(true);
        }
        else if((colorimetry&HdmiOutputConfig.COLORIMETRY_BT709) != 0) {
            mRbColorimetryBT709.setChecked(true);
        }
        else if((colorimetry&HdmiOutputConfig.COLORIMETRY_BT2020) != 0) {
            mRbColorimetryBT2020.setChecked(true);
        }

        int valueRange = hdmiOutputConfig.valueRange;
        if(valueRange == HdmiOutputConfig.VALUE_RANGE_DEFAULT) {
            mRbValueRangeDefault.setChecked(true);
        }
        else if((valueRange&HdmiOutputConfig.VALUE_RANGE_LIMITED) != 0) {
            mRbValueRangeLimit.setChecked(true);
        }
        else if((valueRange&HdmiOutputConfig.VALUE_RANGE_FULL) != 0) {
            mRbValueRangeFull.setChecked(true);
        }

        int scanMode = hdmiOutputConfig.scanMode;
        if(scanMode == HdmiOutputConfig.DEFAULT_SCAN) {
            mRbScanModeDefault.setChecked(true);
        }
        else if((scanMode&HdmiOutputConfig.OVER_SCAN) != 0) {
            mRbScanModeOver.setChecked(true);
        }
        else if((scanMode&HdmiOutputConfig.UNDER_SCAN) != 0) {
            mRbScanModeUnder.setChecked(true);
        }
        int hdrMode = hdmiOutputConfig.hdrMode;
        //how to get hdrMode?
        if((hdrMode&HdmiOutputConfig.DATASPACE_SDR) != 0) {
            mRbHdrModeSDR.setChecked(true);
        }
        else if((hdrMode&HdmiOutputConfig.DATASPACE_HDR) != 0) {
            mRbHdrModeHDR.setChecked(true);
        }
        else if((hdrMode&HdmiOutputConfig.DATASPACE_HLG) != 0) {
            mRbHdrModeHLG.setChecked(true);
        }
        //set hdmi output mode
        int hdmiMode = hdmiOutputConfig.hdmiMode;
        mBtSelect.setText(hdmiOutputInfos[hdmiMode]);

        int aspectRatio = hdmiOutputConfig.aspectRatio;
        if((aspectRatio&HdmiOutputConfig.RATIO_SAME) != 0){
            mRbAspectRatioSameAsPicture.setChecked(true);
        }
        else if((aspectRatio&HdmiOutputConfig.RATIO_4_3) != 0){
            mRbAspectRatio43.setChecked(true);
        }
        else if((aspectRatio&HdmiOutputConfig.RATIO_16_9) != 0){
            mRbAspectRatio169.setChecked(true);
        }
        else if((aspectRatio&HdmiOutputConfig.RATIO_14_9) != 0){
            mRbAspectRatio149.setChecked(true);
        }

        int hdcpstate = HdmiCtsUtils.getHdcpState();
        if(hdcpstate == 1){
            mRbHdcpTrue.setChecked(true);
        }
        else if (hdcpstate == 0){
            mRbHdcpFalse.setChecked(true);
        }

        Log.d(LOG_TAG, "Hdmi deviceType: " + deviceType);
        Log.d(LOG_TAG, "Hdmi hdmimode: " + hdmiMode);
        Log.d(LOG_TAG, "Hdmi supportedColorSpace: " + colorSpace);
        Log.d(LOG_TAG, "Hdmi supportedColorDepth: " + colorDepth);
        Log.d(LOG_TAG, "Hdmi supportedColorimetry: " + colorimetry);
        Log.d(LOG_TAG, "Hdmi supportedValueRange: " + valueRange);
        Log.d(LOG_TAG, "Hdmi supportedScanMode: " + scanMode);
        Log.d(LOG_TAG, "Hdmi supportedHdrMode: " + hdrMode);
        Log.d(LOG_TAG, "Hdmi ratio: " + aspectRatio);
    }


    @Override
    public void onFocusChange(View v, boolean hasFocus){
        switch(v.getId()){
            case R.id.rb_device_type_hdmi:
                if(hasFocus){
                    mRbDeviceTypeHdmi.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbDeviceTypeHdmi.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_device_type_dvi:
                if(hasFocus){
                    mRbDeviceTypeDvi.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbDeviceTypeDvi.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_color_space_rgb888:
                if(hasFocus){
                    mRbColorSpaceRG888.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorSpaceRG888.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_color_space_yuv444:
                if(hasFocus){
                    mRbColorSpaceYUV444.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorSpaceYUV444.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_color_space_yuv422:
                if(hasFocus){
                    mRbColorSpaceYUV422.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorSpaceYUV422.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_color_space_yuv420:
                if(hasFocus){
                    mRbColorSpaceYUV420.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorSpaceYUV420.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_color_depth_36:
                if(hasFocus){
                    mRbColorDepth36.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorDepth36.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_color_depth_30:
                if(hasFocus){
                    mRbColorDepth30.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorDepth30.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_color_depth_24:
                if(hasFocus){
                    mRbColorDepth24.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorDepth24.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_colorimetry_bt601:
                if(hasFocus){
                    mRbColorimetryBT601.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorimetryBT601.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_colorimetry_bt709:
                if(hasFocus){
                    mRbColorimetryBT709.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorimetryBT709.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_colorimetry_bt2020:
                if(hasFocus){
                    mRbColorimetryBT2020.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbColorimetryBT2020.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_value_range_default:
                if(hasFocus){
                    mRbValueRangeDefault.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbValueRangeDefault.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_value_range_full:
                if(hasFocus){
                    mRbValueRangeFull.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbValueRangeFull.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_value_range_limit:
                if(hasFocus){
                    mRbValueRangeLimit.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbValueRangeLimit.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_scan_mode_default:
                if(hasFocus){
                    mRbScanModeDefault.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbScanModeDefault.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_scan_mode_over:
                if(hasFocus){
                    mRbScanModeOver.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbScanModeOver.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_scan_mode_under:
                if(hasFocus){
                    mRbScanModeUnder.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbScanModeUnder.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_hdr_mode_sdr:
                if(hasFocus){
                    mRbHdrModeSDR.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbHdrModeSDR.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_hdr_mode_hdr:
                if(hasFocus){
                    mRbHdrModeHDR.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbHdrModeHDR.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_hdr_mode_hlg:
                if(hasFocus){
                    mRbHdrModeHLG.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbHdrModeHLG.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_aspect_ratio_same_as_picture:
                if(hasFocus){
                    mRbAspectRatioSameAsPicture.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbAspectRatioSameAsPicture.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_aspect_ratio_4_3:
                if(hasFocus){
                    mRbAspectRatio43.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbAspectRatio43.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_aspect_ratio_16_9:
                if(hasFocus){
                    mRbAspectRatio169.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbAspectRatio169.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_aspect_ratio_14_9:
                if(hasFocus){
                    mRbAspectRatio149.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbAspectRatio149.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_hdcp_true:
                if(hasFocus){
                    mRbHdcpTrue.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbHdcpTrue.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_hdcp_false:
                if(hasFocus){
                    mRbHdcpFalse.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbHdcpFalse.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.music_radio_false:
                if(hasFocus){
                    mRbMusicNotPassThrough.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbMusicNotPassThrough.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.music_radio_true:
                if(hasFocus){
                    mRbMusicPassThrough.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbMusicPassThrough.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_hpd_enable:
                if(hasFocus){
                    mRbHpdEnable.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbHpdEnable.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_hpd_disable:
                if(hasFocus){
                    mRbHpdDisable.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbHpdDisable.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_cecstandby_enable:
                if(hasFocus){
                    mRbCecStandbyEnable.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbCecStandbyEnable.setTextColor(Color.parseColor("#000000"));
                }
                break;
            case R.id.rb_cecstandby_disable:
                if(hasFocus){
                    mRbCecStandbyDisable.setTextColor(Color.parseColor("#FF0000"));
                }
                else{
                    mRbCecStandbyDisable.setTextColor(Color.parseColor("#000000"));
                }
                break;
        }
    }


    @Override
    public void onClick(View v) {
        // TODO Auto-generated method stub
        switch (v.getId()) {
            case R.id.bt_get_hdmi_output_mode:
                AlertDialog.Builder builder = new AlertDialog.Builder(this)
                    .setAdapter(new SimpleAdapter(this, dataList,
                                R.layout.hdmi_output_item, new String[] { "name" }, new int[] { R.id.hdmi_output_name }), this)
                    .setTitle(getString(R.string.dialog_title));
                builder.create();
                builder.show();
                break;
            case R.id.bt_ok:
                Log.d(LOG_TAG, "HdmiCapability deviceType: " + mHdmiOutputConfig.deviceType);
                Log.d(LOG_TAG, "HdmiCapability hdmiMode: " + mHdmiOutputConfig.hdmiMode);
                Log.d(LOG_TAG, "HdmiCapability supportedColorSpace: " + mHdmiOutputConfig.colorSpace);
                Log.d(LOG_TAG, "HdmiCapability supportedColorDepth: " + mHdmiOutputConfig.colorDepth);
                Log.d(LOG_TAG, "HdmiCapability supportedColorimetry: " + mHdmiOutputConfig.colorimetry);
                Log.d(LOG_TAG, "HdmiCapability supportedValueRange: " + mHdmiOutputConfig.valueRange);
                Log.d(LOG_TAG, "HdmiCapability supportedScanMode: " + mHdmiOutputConfig.scanMode);
                Log.d(LOG_TAG, "HdmiCapability supportedHdrMode: " + mHdmiOutputConfig.hdrMode);
                Log.d(LOG_TAG, "HdmiCapability supportedScanMode: " + mHdmiOutputConfig.aspectRatio);
                HdmiCtsUtils.setHdmiOutputConfig(mHdmiOutputConfig);
                break;
            case R.id.bt_music:
                Intent i = new Intent("com.softwinner.action.GET_FILE");
                try {
                    startActivityForResult(i, REQUEST_MUSIC);
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(MainActivity.this, "fail activitynotfount exception", Toast.LENGTH_SHORT).show();
                } catch (SecurityException e) {
                    Toast.makeText(MainActivity.this, "fail secureity exception", Toast.LENGTH_SHORT).show();
                }
                break;
            case R.id.bt_start_stop_music:
                if(StartOrStop){
                    // play music
                    if(mMusicPath == null){
                        Log.w(LOG_TAG, "do not select a music file");
                        Toast.makeText(MainActivity.this, R.string.no_music, Toast.LENGTH_SHORT).show();
                        return;
                    }
                    Log.w(LOG_TAG, "start playing music");
                    if(mp != null){
                        mp.stop();
                        mp.release();
                    }
                    mp = new MediaPlayer();
                    try{
                        mp.setDataSource(mMusicPath);
                        mp.setAudioStreamType(AudioManager.STREAM_RING);
                        mp.prepare();
                        mp.start();
                        mp.setOnCompletionListener(new MediaPlayer.OnCompletionListener(){
                            @Override
                            public void onCompletion(MediaPlayer mediaPlayer) {
                                mStartStopMusic.setText(R.string.start_music);
                                StartOrStop = true;
                            }
                        });
                        mStartStopMusic.setText(R.string.stop_music);
                        StartOrStop = false;
                    }
                    catch (IOException e) {
                        Log.w(LOG_TAG, "select music exception" + e);
                    }
                }
                else{
                    // stop music
                    if(mp != null){
                        Log.w(LOG_TAG, "stop music");
                        mp.stop();
                        mp.release();
                        mp = null;
                        mStartStopMusic.setText(R.string.start_music);
                        StartOrStop = true;
                    }
                }
                break;
            default:
                break;
        }
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        switch (group.getId()) {
            case R.id.rg_device_type:
                switch (checkedId) {
                    case R.id.rb_device_type_dvi:
                        mHdmiOutputConfig.deviceType=1;
                        break;
                    case R.id.rb_device_type_hdmi:
                        mHdmiOutputConfig.deviceType=2;
                        break;
                }
                break;
            case R.id.rg_color_space:
                switch (checkedId) {
                    case R.id.rb_color_space_rgb888:
                        mHdmiOutputConfig.colorSpace=1;
                        break;
                    case R.id.rb_color_space_yuv420:
                        mHdmiOutputConfig.colorSpace=8;
                        break;
                    case R.id.rb_color_space_yuv422:
                        mHdmiOutputConfig.colorSpace=4;
                        break;
                    case R.id.rb_color_space_yuv444:
                        mHdmiOutputConfig.colorSpace=2;
                        break;
                }
                break;
            case R.id.rg_color_depth:
                switch (checkedId) {
                    case R.id.rb_color_depth_24:
                        mHdmiOutputConfig.colorDepth=1;
                        break;
                    case R.id.rb_color_depth_30:
                        mHdmiOutputConfig.colorDepth=2;
                        break;
                    case R.id.rb_color_depth_36:
                        mHdmiOutputConfig.colorDepth=4;
                        break;
                }
                break;
            case R.id.rg_colorimetry:
                switch (checkedId) {
                    case R.id.rb_colorimetry_bt601:
                        mHdmiOutputConfig.colorimetry=1;
                        break;
                    case R.id.rb_colorimetry_bt709:
                        mHdmiOutputConfig.colorimetry=2;
                        break;
                    case R.id.rb_colorimetry_bt2020:
                        mHdmiOutputConfig.colorimetry=4;
                        break;
                }
                break;
            case R.id.rg_value_range:
                switch (checkedId) {
                    case R.id.rb_value_range_default:
                        mHdmiOutputConfig.valueRange=0;
                        break;
                    case R.id.rb_value_range_full:
                        mHdmiOutputConfig.valueRange=1;
                        break;
                    case R.id.rb_value_range_limit:
                        mHdmiOutputConfig.valueRange=2;
                        break;
                }
                break;
            case R.id.rg_scan_mode:
                switch (checkedId) {
                    case R.id.rb_scan_mode_over:
                        mHdmiOutputConfig.scanMode=1;
                        break;
                    case R.id.rb_scan_mode_under:
                        mHdmiOutputConfig.scanMode=2;
                        break;
                }
                break;
            case R.id.rg_hdr_mode:
                switch (checkedId) {
                    case R.id.rb_hdr_mode_sdr:
                        mHdmiOutputConfig.hdrMode=1;
                        break;
                    case R.id.rb_hdr_mode_hdr:
                        mHdmiOutputConfig.hdrMode=2;
                        break;
                    case R.id.rb_hdr_mode_hlg:
                        mHdmiOutputConfig.hdrMode=4;
                        break;
                }
                break;
            case R.id.rg_aspect_ratio:
                switch (checkedId) {
                    case R.id.rb_aspect_ratio_same_as_picture:
                        mHdmiOutputConfig.aspectRatio=0x08;
                        break;
                    case R.id.rb_aspect_ratio_4_3:
                        mHdmiOutputConfig.aspectRatio=0x09;
                        break;
                    case R.id.rb_aspect_ratio_16_9:
                        mHdmiOutputConfig.aspectRatio=0x0a;
                        break;
                    case R.id.rb_aspect_ratio_14_9:
                        mHdmiOutputConfig.aspectRatio=0x0b;
                        break;
                }
                break;
            case R.id.rg_hdcp:
                switch(checkedId){
                    case R.id.rb_hdcp_true:
                        HdmiCtsUtils.setHdcpState(1);
                        break;
                    case R.id.rb_hdcp_false:
                        HdmiCtsUtils.setHdcpState(0);
                        break;
                }
                break;
            case R.id.music_radio:
                if(checkedId == R.id.music_radio_true ){
                    Log.w(LOG_TAG, "pass throuth true");
                    Settings.System.putInt(getContentResolver(), Settings.System.ENABLE_PASS_THROUGH,1);
                }
                else if(checkedId == R.id.music_radio_false){
                    Log.w(LOG_TAG, "pass throuth false");
                    Settings.System.putInt(getContentResolver(), Settings.System.ENABLE_PASS_THROUGH,0);
                }
                mAudioMangerEx.setAudioDeviceActive(mChannels, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
                try{
                    Log.w(LOG_TAG, "pass throuth value = " + Settings.System.getInt(getContentResolver(),"enable_pass_through"));
                }
                catch (Settings.SettingNotFoundException e) {
                    Log.w(LOG_TAG, "can not find the pass through value" + e);
                }
                break;
            case R.id.rg_hpd:
                switch (checkedId) {
                    case R.id.rb_hpd_disable:
                        HdmiCtsUtils.setHotplugReportEnabled(0);
                        break;
                    case R.id.rb_hpd_enable:
                        HdmiCtsUtils.setHotplugReportEnabled(1);
                        break;
                    default:
                        break;
                }
            case R.id.rg_cecstandby:
                switch (checkedId) {
                    case R.id.rb_cecstandby_disable:
                        HdmiCtsUtils.setCecStandbyEnabled(0);
                        break;
                    case R.id.rb_cecstandby_enable:
                        HdmiCtsUtils.setCecStandbyEnabled(1);
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        // TODO Auto-generated method stub
        String selectedOutputMode = dataList.get(which).get("name");
        mBtSelect.setText(selectedOutputMode);
        for(int i=0;i<hdmiOutputInfos.length;i++) {
            if(selectedOutputMode.equals(hdmiOutputInfos[i])) {
                mHdmiOutputConfig.hdmiMode=i;
                break;
            }
        }
        dialog.dismiss();

        mHdmiCapability = HdmiCtsUtils.getHdmiCapability();
        parseHdmiCapability(mHdmiCapability);
        HdmiCapability hdmiC=  HdmiCtsUtils.getConstraintByHdmiMode(mHdmiOutputConfig.hdmiMode);
        //select the specified radio
        int colorSpace = hdmiC.supportedColorSpace;
        if((colorSpace&HdmiCapability.SAMPLING_MODE_RGB888) != 0) {
            mRbColorSpaceRG888.setEnabled(true);
            mRbColorSpaceRG888.setFocusable(true);
        }
        else{
            mRbColorSpaceRG888.setEnabled(false);
            mRbColorSpaceRG888.setFocusable(false);
            mRbColorSpaceRG888.setTextColor(Color.parseColor("#BBBBBB"));
            //mRbColorSpaceRG888.setBackgroundColor(Color.parseColor("#AAAAAA"));
        }
        if((colorSpace&HdmiCapability.SAMPLING_MODE_YUV444) != 0) {
            mRbColorSpaceYUV444.setEnabled(true);
            mRbColorSpaceYUV444.setFocusable(true);
        }
        else{
            mRbColorSpaceYUV444.setEnabled(false);
            mRbColorSpaceYUV444.setFocusable(false);
            mRbColorSpaceYUV444.setTextColor(Color.parseColor("#BBBBBB"));
            //mRbColorSpaceYUV444.setBackgroundColor(Color.parseColor("#AAAAAA"));
        }
        if((colorSpace&HdmiCapability.SAMPLING_MODE_YUV422) != 0) {
            mRbColorSpaceYUV422.setEnabled(true);
            mRbColorSpaceYUV422.setFocusable(true);
        }
        else{
            mRbColorSpaceYUV422.setEnabled(false);
            mRbColorSpaceYUV422.setFocusable(false);
            mRbColorSpaceYUV422.setTextColor(Color.parseColor("#BBBBBB"));
            //mRbColorSpaceYUV422.setBackgroundColor(Color.parseColor("#AAAAAA"));
        }
        if((colorSpace&HdmiCapability.SAMPLING_MODE_YUV420) != 0) {
            mRbColorSpaceYUV420.setEnabled(true);
            mRbColorSpaceYUV420.setFocusable(true);
            mRbColorSpaceYUV420.setTextColor(Color.parseColor("#000000"));
        }
        else{
            mRbColorSpaceYUV420.setEnabled(false);
            mRbColorSpaceYUV420.setFocusable(false);
            mRbColorSpaceYUV420.setTextColor(Color.parseColor("#BBBBBB"));
            //mRbColorSpaceYUV420.setBackgroundColor(Color.parseColor("#AAAAAA"));
        }

        int colorDepth = hdmiC.supportedColorDepth;
        if((colorDepth&HdmiCapability.COLOR_DEPTH_24BITS) != 0) {
            mRbColorDepth24.setEnabled(true);
            mRbColorDepth24.setFocusable(true);
        }
        else{
            mRbColorDepth24.setEnabled(false);
            mRbColorDepth24.setFocusable(false);
            mRbColorDepth24.setTextColor(Color.parseColor("#BBBBBB"));
            //mRbColorDepth24.setBackgroundColor(Color.parseColor("#AAAAAA"));
        }
        if((colorDepth&HdmiCapability.COLOR_DEPTH_30BITS) != 0) {
            mRbColorDepth30.setEnabled(true);
            mRbColorDepth30.setFocusable(true);
        }
        else{
            mRbColorDepth30.setEnabled(false);
            mRbColorDepth30.setFocusable(false);
            mRbColorDepth30.setTextColor(Color.parseColor("#BBBBBB"));
            //mRbColorDepth30.setBackgroundColor(Color.parseColor("#AAAAAA"));
        }
        if((colorDepth&HdmiCapability.COLOR_DEPTH_36BITS) != 0) {
            mRbColorDepth36.setEnabled(true);
            mRbColorDepth36.setFocusable(true);
        }
        else{
            mRbColorDepth36.setEnabled(false);
            mRbColorDepth36.setFocusable(false);
            mRbColorDepth36.setTextColor(Color.parseColor("#BBBBBB"));
            //mRbColorDepth36.setBackgroundColor(Color.parseColor("#AAAAAA"));
        }
    }
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(android.os.Message msg) {
            mHdmiCapability = HdmiCtsUtils.getHdmiCapability();
            parseHdmiCapability(mHdmiCapability);

            //update the support list
            int[] supportmodes = HdmiCtsUtils.getHdmiSupportMode();
            Arrays.sort(supportmodes);
            dataList.clear();
            for(int i=0;i<supportmodes.length;i++) {
                Map<String, String> map = new HashMap<String,String>();
                map.put("name", hdmiOutputInfos[supportmodes[i]]);
                dataList.add(map);
            }

            mHdmiOutputConfig = HdmiCtsUtils.getHdmiOutputConfig();
            parseHdmiOutputConfig(mHdmiOutputConfig);
            mHpdEnable = HdmiCtsUtils.getHotplugReportEnabled();
            if(mHpdEnable == 0) {
                mRbHpdDisable.setChecked(true);
            }else {
                mRbHpdEnable.setChecked(true);
            }

            mCecStandbyEnable = HdmiCtsUtils.getCecStandbyEnabled();
            if(mCecStandbyEnable == 0) {
                mRbCecStandbyDisable.setChecked(true);
            }else {
                mRbCecStandbyEnable.setChecked(true);
            }
        }
    };
    private final BroadcastReceiver HdmiReceiver = new BroadcastReceiver(){
        @Override
        public void onReceive(Context context, Intent intent){
            String action = intent.getAction();
            if(action.equals(HDMIINTENT)){
                boolean state = intent.getBooleanExtra("state", false);
                if(state){
                    Log.w(LOG_TAG,"Hdmi pluged!!!");
                    mHandler.sendEmptyMessageDelayed(MSG_HDMI_IN, 2*1000);
                }
                else{
                    Log.w(LOG_TAG,"Hdmi pluged off!!");
                }
            }
        }
    };

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Uri uri = data.getData();
        if(uri == null){
            return;
        }
        Log.w(LOG_TAG, "uri path" + uri.getPath());
        Log.w(LOG_TAG, "uri scheme" + uri.getScheme());
        if(uri != null && uri.getScheme().equals("file") ){
            mMusicPath = uri.getPath();
            Log.w(LOG_TAG, "select music" + mMusicPath);
            mTextMusicPath.setText(uri.getPath());
        }
    }
}

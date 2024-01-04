package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.os.Build;
import android.util.Log;
import android.widget.TextView;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.entity.VersionInfo;
import com.softwinner.dragonbox.entity.VersionInfoMore;
import com.softwinner.dragonbox.manager.VersionManager;

public class CaseVersion extends IBaseCase {
    public static final String TAG = "DragonBox-CaseVersion";
    public VersionManager versiomManager;

    private TextView mMinAndroidVer;
    private TextView mMinDisplay;
    private TextView mMinModel;
    private TextView mMinWifiMac;
    private TextView mMinEthMac;
    private TextView mMinNand;
    private TextView mMinDDR;

    private TextView mMaxAndroidVer;
    private TextView mMaxAndroidVerConf;
    private TextView mMaxModel;
    private TextView mMaxModelConf;
    private TextView mMaxDisplay;
    private TextView mMaxDisplayConf;

    private TextView mMaxWifiMac;
    private TextView mMaxEthMac;
    private TextView mMaxNand;
    private TextView mMaxDDR;

    public static String ddr="null";
    public static String flash="null";
    public static String ver="null";
    public static String display="null";

    private CaseVersion(Context context) {
        super(context, R.string.case_version_name, R.layout.case_version_max,
                R.layout.case_version_min, TYPE_MODE_AUTO);
        versiomManager = new VersionManager(context);
        mMinAndroidVer = (TextView) mMinView
                .findViewById(R.id.case_min_version_android_ver);
        mMinDisplay = (TextView) mMinView
                .findViewById(R.id.case_version_display);
        mMinModel = (TextView) mMinView.findViewById(R.id.case_version_model);
        mMinWifiMac = (TextView) mMinView
                .findViewById(R.id.case_version_wifi_mac);
        mMinEthMac = (TextView) mMinView
                .findViewById(R.id.case_version_eth_mac);
        mMinNand = (TextView) mMinView.findViewById(R.id.case_version_nand);
        mMinDDR = (TextView) mMinView.findViewById(R.id.case_version_ddr);

        mMaxAndroidVer = (TextView) mMaxView
                .findViewById(R.id.case_max_version_android_ver);
        mMaxAndroidVerConf = (TextView) mMaxView
                .findViewById(R.id.case_max_version_android_ver_conf);
        mMaxModel = (TextView) mMaxView.findViewById(R.id.case_version_model);
        mMaxModelConf = (TextView) mMaxView
                .findViewById(R.id.case_version_model_conf);
        mMaxDisplay = (TextView) mMaxView
                .findViewById(R.id.case_version_display);
        mMaxDisplayConf = (TextView) mMaxView
                .findViewById(R.id.case_version_display_conf);

        mMaxWifiMac = (TextView) mMaxView
                .findViewById(R.id.case_version_wifi_mac);
        mMaxEthMac = (TextView) mMaxView
                .findViewById(R.id.case_version_eth_mac);
        mMaxNand = (TextView) mMaxView.findViewById(R.id.case_version_nand);
        mMaxDDR = (TextView) mMaxView.findViewById(R.id.case_version_ddr);
    }

    public CaseVersion(Context context, XmlPullParser xmlParser) {
        this(context);
        VersionInfo vInfo = new VersionInfo();
        String strVer=xmlParser.getAttributeValue(null, "version");
        String strDisplay=xmlParser.getAttributeValue(null, "display");
        vInfo.setFireware(strVer);
        vInfo.setDispaly(strDisplay);
        vInfo.setModel(xmlParser.getAttributeValue(null, "model"));
        String strDDR = xmlParser.getAttributeValue(null, "ddr");
        if(strDDR==null) {
            vInfo.setDDR(0.0);
        }else {
            vInfo.setDDR(Double.parseDouble(strDDR));
        }
        String strFlash = xmlParser.getAttributeValue(null, "flash");
        if(strFlash == null) {
            vInfo.setFlash(0.0);
        }else {
            vInfo.setFlash(Double.parseDouble(strFlash));
        }
        ddr = strDDR;
        flash = strFlash;
        ver = strVer;
        display = strDisplay;
        Log.w(TAG,"VersionInfo is "+vInfo.toString());
        versiomManager.setConfVersionInfo(vInfo);
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseVersion");
        updateView();
        setDialogPositiveButtonEnable(false);
    }

    private void updateView() {
        VersionInfo confInfo = versiomManager.getConfVersionInfo();
        VersionInfoMore sysInfo = versiomManager.getSysVersionInfo();
        Log.w(TAG,"sysInfo is "+((VersionInfo)sysInfo).toString());

        // String versionInfo = mContext.getString(R.string.case_version_info,
        // vInfo.getFireware(), vInfo.getDispaly(), vInfo.getModle());
        // String curVersionInfo =
        // mContext.getString(R.string.case_version_info,
        // curVInfo.getFireware(), curVInfo.getDispaly(), curVInfo.getModle());

        mMinAndroidVer.setText(mContext
                .getString(R.string.case_version_info_android_ver)
                + sysInfo.getFireware());
        mMinDisplay.setText(mContext
                .getString(R.string.case_version_info_display)
                + sysInfo.getDispaly());
        mMinModel.setText(mContext.getString(R.string.case_version_info_model)
                + sysInfo.getModel());
        mMinWifiMac
                .setText(mContext.getString(
                        R.string.case_version_other_info_wifi_mac,
                        sysInfo.getWifiMac()));
        mMinEthMac
                .setText(mContext.getString(
                        R.string.case_version_other_info_eth_mac,
                        sysInfo.getEth0Mac()));
        mMinNand.setText(mContext.getString(
                R.string.case_version_other_info_nand,String.format("%.2f", sysInfo.getFlash())+"GB"));
        mMinDDR.setText(mContext.getString(
                R.string.case_version_other_info_ddr, String.format("%.2f", sysInfo.getDDR())+"GB"));

        mMaxAndroidVer.setText(sysInfo.getFireware());
        mMaxAndroidVerConf.setText(confInfo.getFireware());
        mMaxModel.setText(sysInfo.getModel());
        mMaxModelConf.setText(confInfo.getModel());
        mMaxDisplay.setText(sysInfo.getDispaly());
        mMaxDisplayConf.setText(confInfo.getDispaly());

        mMaxWifiMac
                .setText(mContext.getString(
                        R.string.case_version_other_info_wifi_mac,
                        sysInfo.getWifiMac()));
        mMaxEthMac
                .setText(mContext.getString(
                        R.string.case_version_other_info_eth_mac,
                        sysInfo.getEth0Mac()));
        mMaxNand.setText(mContext.getString(
                R.string.case_version_other_info_nand,String.format("%.2f", sysInfo.getFlash())+"GB"));
        mMaxDDR.setText(mContext.getString(
                R.string.case_version_other_info_ddr, String.format("%.2f", sysInfo.getDDR())+"GB"));
        boolean result = isEqual(confInfo, sysInfo);
        stopCase();
        setCaseResult(result);
        Log.w(TAG,"CaseVersion test over ,test result is "+getCaseResult());
        setDialogPositiveButtonEnable(result);
    }

    public boolean isEqual(VersionInfo configInfo, VersionInfo systemInfo) {
        int errs = 0;//失败项数目
        if (configInfo == systemInfo)
            return true;
        if (systemInfo == null)
            return false;

        if (configInfo.mDispaly == null) {
            if (systemInfo.mDispaly != null) {
                mMinDisplay.setBackgroundResource(R.drawable.result_fail_bg);
                errs++;
            }
        } else if (!configInfo.mDispaly.equals(systemInfo.mDispaly)) {
            mMinDisplay.setBackgroundResource(R.drawable.result_fail_bg);
            errs++;
        }

        if (configInfo.mFireware == null) {
            if (systemInfo.mFireware != null) {
                mMinAndroidVer.setBackgroundResource(R.drawable.result_fail_bg);
                errs++;
            }
        } else if (!configInfo.mFireware.equals(systemInfo.mFireware)) {
            mMinAndroidVer.setBackgroundResource(R.drawable.result_fail_bg);
            errs++;
        }

        if (configInfo.mModel == null) {
            if (systemInfo.mModel != null) {
                mMinModel.setBackgroundResource(R.drawable.result_fail_bg);
                errs++;
            }
        } else if (!configInfo.mModel.equals(systemInfo.mModel)) {
            mMinModel.setBackgroundResource(R.drawable.result_fail_bg);
            errs++;
        }
        if(configInfo.mDDR==0) {
            mMinDDR.setBackgroundResource(R.drawable.result_fail_bg);
            errs++;
        }else if(systemInfo.mDDR/configInfo.mDDR<VersionInfo.RANGE||systemInfo.mDDR>configInfo.mDDR) {
            mMinDDR.setBackgroundResource(R.drawable.result_fail_bg);
            errs++;
        }
        if(configInfo.mFlash==0) {
            mMinNand.setBackgroundResource(R.drawable.result_fail_bg);
            errs++;
        }else if(systemInfo.mFlash/configInfo.mFlash<VersionInfo.RANGE||systemInfo.mFlash>configInfo.mFlash) {
            mMinNand.setBackgroundResource(R.drawable.result_fail_bg);
            errs++;
        }
        if(errs>0)
            return false;
        else
            return true;
    }

    @Override
    public void onStopCase() {

    }
    @Override
    public void onSetResult() {
        super.onSetResult();
        //SystemUtil.getTotalMemory(context),SystemUtil.getFlashCapability()
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        VersionInfoMore sysInfo = versiomManager.getSysVersionInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[1], Build.VERSION.RELEASE);//version
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[2], Build.DISPLAY);//display
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[3], Build.MODEL);//model
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[4], String.format("%.2f", sysInfo.getDDR())+"GB");//ddr
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[5], String.format("%.2f", sysInfo.getFlash())+"GB");//flash
    };

    @Override
    public void reset() {
        super.reset();
    }
}

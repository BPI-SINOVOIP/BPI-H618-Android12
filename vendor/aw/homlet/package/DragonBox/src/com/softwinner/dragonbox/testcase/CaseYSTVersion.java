package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.text.format.Formatter;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.VersionInfo;
import com.softwinner.dragonbox.entity.VersionInfoMore;
import com.softwinner.dragonbox.entity.YSTVersionInfo;
import com.softwinner.dragonbox.manager.VersionManager;
import com.softwinner.dragonbox.manager.YSTVersionManager;

public class CaseYSTVersion extends IBaseCase {
    public YSTVersionManager versiomManager;

    private TextView mMinFireware;
    private TextView mMinMemSize;
    private TextView mMinCpuFreq;

    private TextView mMaxFireware;
    private TextView mMaxMemSize;
    private TextView mMaxCpuFreq;

    private TextView mMaxFirewareSys;
    private TextView mMaxMemSizeSys;
    private TextView mMaxCpuFreqSys;

    private CaseYSTVersion(Context context) {
        super(context, R.string.case_version_name,
                R.layout.case_yst_version_max, R.layout.case_yst_version_min,
                TYPE_MODE_AUTO);

        mMinFireware = (TextView) mMinView
                .findViewById(R.id.case_version_version);
        mMinMemSize = (TextView) mMinView
                .findViewById(R.id.case_version_memsize);
        mMinCpuFreq = (TextView) mMinView
                .findViewById(R.id.case_version_cpufreq);

        mMaxFireware = (TextView) mMaxView
                .findViewById(R.id.case_version_fireware);
        mMaxMemSize = (TextView) mMaxView.findViewById(R.id.case_version_ddr);
        mMaxCpuFreq = (TextView) mMaxView
                .findViewById(R.id.case_version_cpu_freq);

        mMaxFirewareSys = (TextView) mMaxView
                .findViewById(R.id.case_version_fireware_sys);
        mMaxMemSizeSys = (TextView) mMaxView
                .findViewById(R.id.case_version_ddr_sys);
        mMaxCpuFreqSys = (TextView) mMaxView
                .findViewById(R.id.case_version_cpu_freq_sys);
    }

    public CaseYSTVersion(Context context, XmlPullParser xmlParser) {
        this(context);
        YSTVersionInfo vInfo = new YSTVersionInfo();
        versiomManager = new YSTVersionManager(context);
        vInfo.setVersion(xmlParser.getAttributeValue(null, "version"));
        vInfo.setMemSize(xmlParser.getAttributeValue(null, "memsize"));
        vInfo.setCpuFreq(xmlParser.getAttributeValue(null, "cpufreq"));
        versiomManager.setConfVersionInfo(vInfo);
    }

    @Override
    public void onStartCase() {
        updateView();
    }

    private void updateView() {
        YSTVersionInfo confInfo = versiomManager.getConfVersionInfo();
        YSTVersionInfo sysInfo = versiomManager.getSysVersionInfo();

        mMinFireware.setText(mContext
                .getString(R.string.case_version_info_android_ver)
                + sysInfo.getVersion());
        mMinMemSize.setText(mContext.getString(
                R.string.case_version_other_info_ddr, sysInfo.getMemSize()));
        mMinCpuFreq.setText(mContext.getString(
                R.string.case_version_info_cpufreq, sysInfo.getCpuFreq()));

        mMaxFireware.setText(mContext
                .getString(R.string.case_version_info_android_ver)
                + confInfo.getVersion());
        mMaxMemSize.setText(mContext.getString(
                R.string.case_version_other_info_ddr, confInfo.getMemSize()));
        mMaxCpuFreq.setText(mContext.getString(
                R.string.case_version_info_cpufreq, confInfo.getCpuFreq()));

        mMaxFirewareSys.setText(mContext
                .getString(R.string.case_version_info_android_ver)
                + sysInfo.getVersion());
        mMaxMemSizeSys.setText(mContext.getString(
                R.string.case_version_other_info_ddr, sysInfo.getMemSize()));
        mMaxCpuFreqSys.setText(mContext.getString(
                R.string.case_version_info_cpufreq, sysInfo.getCpuFreq()));

        boolean result = versiomManager.isYSTInfoEquals();
        stopCase();
        setCaseResult(result);
        setDialogPositiveButtonEnable(result);
    }

    @Override
    public void onStopCase() {

    }

    @Override
    public void reset() {
        super.reset();
    }
}

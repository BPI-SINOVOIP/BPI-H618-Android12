package com.softwinner.dragonbox.manager;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.content.Context;
import android.util.Log;

import com.softwinner.dragonbox.entity.YSTVersionInfo;
import com.softwinner.dragonbox.utils.IniReaderUtil;

public class YSTVersionManager {
    private Context mContext;
    private YSTVersionInfo mConfVersionInfo;
    private YSTVersionInfo mSysVersionInfo;

    public YSTVersionManager(Context context) {
        mContext = context;
        mSysVersionInfo = new YSTVersionInfo();
        mSysVersionInfo.setMemSize(getDramSize());
        mSysVersionInfo.setVersion(getYSTVersion());
        mSysVersionInfo.setCpuFreq(getMaxCpuFreq());
    }

    public void setConfVersionInfo(YSTVersionInfo confVersionInfo) {
        mConfVersionInfo = confVersionInfo;
    }

    public boolean isVersionEquals() {
        return mSysVersionInfo.equals(mConfVersionInfo);
    }

    public YSTVersionInfo getSysVersionInfo() {
        return mSysVersionInfo;
    }

    public YSTVersionInfo getConfVersionInfo() {
        return mConfVersionInfo;
    }

    private String readLine(String filename) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(filename),
                256);
        try {
            return reader.readLine();
        } finally {
            reader.close();
        }
    }

    private String getYSTVersion() {
        String path = "/system/etc/ini/RomInfo.ini";
        String section = "VERSION";
        String item = "VersionInfo";
        String version = IniReaderUtil.getValue(section, item, path);
        return version;
    }

    private String getMaxCpuFreq(){
        String maxFreq;
        Integer freq;
        try {
            maxFreq = readLine("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
            freq = Integer.parseInt(maxFreq) / 1000;
        } catch (IOException e) {
            e.printStackTrace();
            freq = 0;
        } catch (Exception e) {
            e.printStackTrace();
            freq = 0;
        }
        return freq + "MHZ";
    }

    private String getDramSize() {
        String proc_mem = null;
        String realsize = null;
        try {
            proc_mem = readLine("/proc/meminfo");
        } catch (IOException e) {
            e.printStackTrace();
        }
        final String regx = "[0-9]+"; // total memory
        if (proc_mem == null)
            return null;
        String rs[] = proc_mem.split("\n");
        Pattern pat = Pattern.compile(regx);
        Matcher mat = pat.matcher(rs[0]);
        if (mat.find()) {
            realsize = mat.group(0);
        } else {
            realsize = "0";
        }
        int size = Integer.parseInt(realsize);
        if (size > 0 && size < 524288) {
            return "512MB";
        } else if (size > 524288 && size < 1048576) {
            return "1024MB";
        } else if (size > 1048576 && size < 2097152) {
            return "2048MB";
        } else {
            return realsize;
        }
    }

    public boolean isYSTInfoEquals() {
        return mConfVersionInfo != null
                && mSysVersionInfo != null
                && mConfVersionInfo.getVersion().equals(
                mSysVersionInfo.getVersion())
                && mConfVersionInfo.getCpuFreq().equals(
                mSysVersionInfo.getCpuFreq())
                && mConfVersionInfo.getMemSize().equals(
                mSysVersionInfo.getMemSize());

    }

}

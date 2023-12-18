package com.softwinner.dragonbox.entity;

public class YSTVersionInfo {
    private String mVersion = "";
    private String mMemSize = "";
    private String mCpuFreq = "";


    public String getVersion() {
        return mVersion;
    }
    public void setVersion(String version) {
        this.mVersion = version;
    }
    public String getMemSize() {
        return mMemSize;
    }
    public void setMemSize(String memSize) {
        this.mMemSize = memSize;
    }
    public String getCpuFreq() {
        return mCpuFreq;
    }
    public void setCpuFreq(String cpuFreq) {
        this.mCpuFreq = cpuFreq;
    }


}

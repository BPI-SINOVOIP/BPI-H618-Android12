package com.softwinner.dragonbox.entity;

public class VersionInfo {

    public String mFireware = "";
    public String mModel = "";
    public String mDispaly = "";
    public String mCpuFreq = "";
    public double mDDR = 0.0;//GB
    public double mFlash = 0.0;//GB
    public static final double RANGE= 0.85;//flash and ddr's error should be less than 15%

    public VersionInfo() {

    }

    public VersionInfo(String fireware, String model,
                       String dispaly, String cpuFreq, double ddr, double flash) {
        super();
        this.mFireware = fireware;
        this.mModel = model;
        this.mDispaly = dispaly;
        this.mCpuFreq = cpuFreq;
        this.mDDR = ddr;
        this.mFlash = flash;
    }

    public String getFireware() {
        return mFireware;
    }
    public void setFireware(String fireware) {
        this.mFireware = fireware;
    }
    public String getModel() {
        return mModel;
    }
    public void setModel(String model) {
        this.mModel = model;
    }
    public String getDispaly() {
        return mDispaly;
    }
    public void setDispaly(String dispaly) {
        this.mDispaly = dispaly;
    }
    public String getCpuFreq() {
        return mCpuFreq;
    }
    public void setCpuFreq(String mCpuFreq) {
        this.mCpuFreq = mCpuFreq;
    }
    public double getDDR() {
        return mDDR;
    }
    public void setDDR(double dDR) {
        mDDR = dDR;
    }

    public double getFlash() {
        return mFlash;
    }

    public void setFlash(double flash) {
        this.mFlash = flash;
    }
    public String toString() {
        String result = "mFireware = "+mFireware+
                "\nmModel = "+mModel+
                "\nmDisplay = "+mDispaly+
                "\nmDDR = "+mDDR+
                "\nmFlash = "+mFlash;
        return result;
    }
    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result
                + ((mCpuFreq == null) ? 0 : mCpuFreq.hashCode());
        result = prime * result
                + ((mDispaly == null) ? 0 : mDispaly.hashCode());
        result = prime * result
                + ((mFireware == null) ? 0 : mFireware.hashCode());
        result = prime * result + ((mModel == null) ? 0 : mModel.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        VersionInfo other = (VersionInfo) obj;
        if (mCpuFreq == null) {
            if (other.mCpuFreq != null)
                return false;
        } else if (!mCpuFreq.equals(other.mCpuFreq))
            return false;

        if (mDispaly == null) {
            if (other.mDispaly != null)
                return false;
        } else if (!mDispaly.equals(other.mDispaly))
            return false;

        if (mFireware == null) {
            if (other.mFireware != null)
                return false;
        } else if (!mFireware.equals(other.mFireware))
            return false;

        if (mModel == null) {
            if (other.mModel != null)
                return false;
        } else if (!mModel.equals(other.mModel))
            return false;
        if(mDDR==0) {
            return false;
        }else if(other.mDDR/mDDR<RANGE||other.mDDR>mDDR)
            return false;
        if(mFlash==0) {
            return false;
        }else if(other.mFlash/mFlash<RANGE||other.mFlash>mFlash)
            return false;

        return true;
    }
}

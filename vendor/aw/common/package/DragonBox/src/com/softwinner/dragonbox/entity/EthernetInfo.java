package com.softwinner.dragonbox.entity;

public class EthernetInfo {
    private boolean isLineConn;
    private String mIpAdd;


    public boolean isLineConn() {
        return isLineConn;
    }
    public void setLineConn(boolean isLineConn) {
        this.isLineConn = isLineConn;
    }
    public String getIpAdd() {
        return mIpAdd;
    }
    public void setIpAdd(String ipAdd) {
        this.mIpAdd = ipAdd;
    }


}

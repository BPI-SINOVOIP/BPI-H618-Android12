package com.bigbigcloud.devicehive.entity;

import java.io.Serializable;

/**
 * Created by Thomas.yang on 2016/7/7.
 */
public class UpdateFailedInfo implements Serializable {
    private String curVersion;
    private int errcode;

    public UpdateFailedInfo(String curVersion, int errcode){
        this.curVersion = curVersion;
        this.errcode = errcode;
    }

    public String getCurVersion() {
        return curVersion;
    }

    public void setCurVersion(String curVersion) {
        this.curVersion = curVersion;
    }

    public int getErrcode() {
        return errcode;
    }

    public void setErrcode(int errcode) {
        this.errcode = errcode;
    }
}

package com.softwinner.dragonbox.entity;

public class YSTWanConfigInfo {

    private String mRouterIP;
    private int mTimeOut = 5000;
    private String wanSetParaUrl = "http://%s/boafrm/set_parameter";
    private String wanGetStatusUrl = "http://%s/boafrm/get_status";

    private String wanConnPara = "[wan_connect_cmd=1,operation_mode=0,wan_connection_mode=DHCP]";
    private String wanDisconnPara = "[wan_connect_cmd=2,operation_mode=0,wan_connection_mode=DHCP,]";

    public YSTWanConfigInfo(String routerIP, int timeout) {
        super();
        this.mRouterIP = routerIP;
        this.mTimeOut = timeout;
    }

    public String getWanSetParaUrl() {
        return String.format(wanSetParaUrl, mRouterIP);
    }

    public String getWanGetStatusUrl() {
        return String.format(wanGetStatusUrl, mRouterIP);
    }

    public String getWanConnPara() {
        return wanConnPara;
    }

    public String getWanDisconnPara(){
        return wanDisconnPara;
    }

    public String getRouterIP() {
        return mRouterIP;
    }

    public int getTimeOut(){
        return mTimeOut;
    }


}

package com.softwinner.dragonbox.entity;

public class YSTWifiConnConfigInfo {

    private String mRouterIP;
    private String mApName;
    private String mApPSW;
    private String mApSecurity;
    private int mTimeOut;
    private String wifiSetParaUrl = "http://%s/boafrm/set_parameter";
    private String wifiGetParaUrl = "http://%s/boafrm/get_parameter";

    private String wifiDhcpPara = "[wan_connect_cmd=1,operation_mode=2,wan_connection_mode=DHCP,wisp_ap_name=%s,wisp_ap_password=%s,wisp_ap_security_mode=%s]";
    private String wifiStatPara = "[operation_mode,wan_connection_mode,wisp_connect_state,wisp_ap_name,wisp_ap_password,wisp_ap_security_mode,wisp_ap_signal_strength,wan_connect_state,wan_ipaddr,wan_netmask,wan_gateway,wan_primary_dns,wan_secondary_dns]";
    private String wifiDisConnPara = "[wan_connect_cmd=2,operation_mode=2,wisp_ap_name=%s]";

    public YSTWifiConnConfigInfo(String routerIP, String apName, String apPSW,
                                 String apSecurity, int timeout) {
        super();
        this.mRouterIP = routerIP;
        this.mApName = apName;
        this.mApPSW = apPSW;
        this.mApSecurity = apSecurity;
        this.mTimeOut = timeout;
    }

    public String getWifiSetParaUrl() {
        return String.format(wifiSetParaUrl, mRouterIP);
    }

    public String getWifiGetParaUrl() {
        return String.format(wifiGetParaUrl, mRouterIP);
    }

    public String getWifiDhcpPara() {
        return String.format(wifiDhcpPara, mApName, mApPSW, mApSecurity);
    }

    public String getWifiDisConnPara() {
        return String.format(wifiDisConnPara, mApName);
    }

    public String getApName() {
        return mApName;
    }

    public String getWifiStatPara() {
        return wifiStatPara;
    }

    public String getRouterIP(){
        return mRouterIP;
    }

    public int getTimeOut(){
        return mTimeOut;
    }

}

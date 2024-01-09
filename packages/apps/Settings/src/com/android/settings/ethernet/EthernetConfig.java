package com.android.settings.ethernet;

public class EthernetConfig {

    public EthernetConfig(String ip) {
        this.ip = ip;
    }

    private String ip = "0.0.0.0";
    private String gw = "0.0.0.0";
    private String dns1 = "0.0.0.0";
    private String dns2 = "0.0.0.0";
    private String mask = "0";

    public void setIp(String ip) {
        this.ip = ip;
    }

    public void setGw(String gw) {
        this.gw = gw;
    }

    public void setDns1(String dns) {
        this.dns1 = dns;
    }

    public void setDns2(String dns) {
        this.dns2 = dns;
    }

    public void setMask(String mask) {
        this.mask = mask;
    }

    public String getIp() {
        return ip;
    }

    public String getGw() {
        return gw;
    }

    public String getDns1() {
        return dns1;
    }

    public String getDns2() {
        return dns2;
    }

    public String getMask() {
        return mask;
    }

}

package com.allwinnertech.socs.wifi;

import android.content.Context;
import android.net.NetworkInfo.DetailedState;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.net.wifi.WifiInfo;
import android.util.Log;

import com.allwinnertech.socs.R;

public class AccessPoint implements Comparable<AccessPoint>{
    public static final int SECURITY_NONE = 0;
    public static final int SECURITY_WEP = 1;
    public static final int SECURITY_PSK = 2;
    public static final int SECURITY_EAP = 3;

    public enum PskType {
        UNKNOWN,
        WPA,
        WPA2,
        WPA_WPA2
    }

    public static final int CONNECTION_UNKNOWN = 0;
    public static final int CONNECTION_CONNECTED = 1;
    public static final int CONNECTION_CONNECTTING_FAILED = 2;

    /* SSID 网络ID号*/
    public String ssid;
    /* RSSI 信号强度*/
    public int rssi;
    /* 安全类型 */
    public int security;
    /* 经过配置后生成的网络id号 */
    public int networkId;
    /* 经过配置之后的wifi配置信息 */
    public WifiConfiguration mConfig;
    /* wps密码类型是否可用 */
    public boolean wpsAvailable = false;
    /* psk密码类型*/
    public PskType pskType;
    /* Basic Service Set */
    public String bssid;
    /* 该ap对应的扫描结果 */
    public ScanResult mResult;
    /* 连接状态 */
    public int connectionType;
    /* Wifi 信息*/
    public WifiInfo mInfo;
    /* 详细信息 */
    public DetailedState mState;
    public Context mContext;

    private AccessPoint(Context context){
        mContext = context;
    }

    public AccessPoint(Context context, ScanResult result){
        this(context);
        loadResult(result);

    }

    public AccessPoint(Context context, WifiConfiguration config){
        this(context);
        loadConfig(config);
    }

    @Override
    public boolean equals(Object o) {
        if(o instanceof AccessPoint){
            AccessPoint info = (AccessPoint)o;
            if(ssid == null){
                return info.ssid == null ? true :false;
            }
            return ssid.equals(info.ssid);
        }
        return super.equals(o);
    }

    @Override
    public int compareTo(AccessPoint another) {
        AccessPoint wifiInfo = another;
        return wifiInfo.rssi - rssi ;
    }

    //为加密类型获得对应的字符串
    public String getSecurityString(int security, PskType pskType, boolean concise) {
        Context context = mContext;
        switch(security) {
            case SECURITY_EAP:
                return concise ? context.getString(R.string.wifi_security_short_eap) :
                    context.getString(R.string.wifi_security_eap);
            case SECURITY_PSK:
                if(pskType == null)
                    return "";
                switch (pskType) {
                    case WPA:
                        return concise ? context.getString(R.string.wifi_security_short_wpa) :
                            context.getString(R.string.wifi_security_wpa);
                    case WPA2:
                        return concise ? context.getString(R.string.wifi_security_short_wpa2) :
                            context.getString(R.string.wifi_security_wpa2);
                    case WPA_WPA2:
                        return concise ? context.getString(R.string.wifi_security_short_wpa_wpa2) :
                            context.getString(R.string.wifi_security_wpa_wpa2);
                    case UNKNOWN:
                    default:
                        return concise ? context.getString(R.string.wifi_security_short_psk_generic)
                                : context.getString(R.string.wifi_security_psk_generic);
                }
            case SECURITY_WEP:
                return concise ? context.getString(R.string.wifi_security_short_wep) :
                    context.getString(R.string.wifi_security_wep);
            case SECURITY_NONE:
            default:
                return concise ? "" : context.getString(R.string.wifi_security_none);
        }
    }

    //从Wifi配置文件中载入信息
    private void loadConfig(WifiConfiguration config) {
        ssid = (config.SSID == null ? "" : removeDoubleQuotes(config.SSID));
        bssid = config.BSSID;
        security = getSecurity(config);
        networkId = config.networkId;
        rssi = Integer.MIN_VALUE;
        mConfig = config;
    }

    //从Wifi扫描结果中载入信息
    private void loadResult(ScanResult result) {
        ssid = result.SSID;
        bssid = result.BSSID;
        security = getSecurity(result);
        wpsAvailable = security != SECURITY_EAP && result.capabilities.contains("WPS");
        if (security == SECURITY_PSK)
            pskType = getPskType(result);
        networkId = -1;
        rssi = result.level;
        mResult = result;
        connectionType = CONNECTION_UNKNOWN;
    }

    //从Wifi扫描结果中更新信息
    public boolean update(ScanResult result) {
        if (ssid.equals(result.SSID) && security == getSecurity(result)) {
                rssi = result.level;
            // 该标志仅能从扫描中获取，不容易保存在配置中
            if (security == SECURITY_PSK) {
                pskType = getPskType(result);
            }
            return true;
        }
        return false;
    }

    public void update(WifiInfo info, DetailedState state) {
        if (info != null && networkId != WifiConfiguration.INVALID_NETWORK_ID
                && networkId == info.getNetworkId()) {
            if (0 == info.getRssi()) {
                rssi = info.getRssi();
            }
            mInfo = info;
            mState = state;
        }else if (mInfo != null) {
            mInfo = null;
            mState = null;
        }
        reflash();
    }

    private void reflash() {
        if (mState != null) {
            connectionType = CONNECTION_CONNECTED;
        } else if (mConfig != null
                && mConfig.status == WifiConfiguration.Status.DISABLED) {
            connectionType = CONNECTION_CONNECTTING_FAILED;
        } else {
            connectionType = CONNECTION_UNKNOWN;
        }
    }

    // 移除字符串的双引号
    static String removeDoubleQuotes(String string) {
        int length = string.length();
        if ((length > 1) && (string.charAt(0) == '"')
                && (string.charAt(length - 1) == '"')) {
            return string.substring(1, length - 1);
        }
        return string;
    }

    // 为字符串增加引号
    public static String convertToQuotedString(String string) {
        return "\"" + string + "\"";
    }

    // 获得access point 的加密类型
    static int getSecurity(WifiConfiguration config) {
        if (config.allowedKeyManagement.get(KeyMgmt.WPA_PSK)) {
            return SECURITY_PSK;
        }
        if (config.allowedKeyManagement.get(KeyMgmt.WPA_EAP) ||
                config.allowedKeyManagement.get(KeyMgmt.IEEE8021X)) {
            return SECURITY_EAP;
        }
        return (config.wepKeys[0] != null) ? SECURITY_WEP : SECURITY_NONE;
    }

    // 获得access point 的加密类型
    private static int getSecurity(ScanResult result) {
        if (result.capabilities.contains("WEP")) {
            return SECURITY_WEP;
        } else if (result.capabilities.contains("PSK")) {
            return SECURITY_PSK;
        } else if (result.capabilities.contains("EAP")) {
            return SECURITY_EAP;
        }
        return SECURITY_NONE;
    }

    // 获得PskType的类型
    private static PskType getPskType(ScanResult result) {
        boolean wpa = result.capabilities.contains("WPA-PSK");
        boolean wpa2 = result.capabilities.contains("WPA2-PSK");
        if (wpa2 && wpa) {
            return PskType.WPA_WPA2;
        } else if (wpa2) {
            return PskType.WPA2;
        } else if (wpa) {
            return PskType.WPA;
        } else {
            return PskType.UNKNOWN;
        }
    }
}

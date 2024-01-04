package com.softwinner.utils;

/**
 * Class that provides some api to get system config.
 *
 * {@hide}
 */
public class Config {
    public static final int TARGET_BOARD_PLATFORM = 0x00;
    public static final int TARGET_BUSINESS_PLATFORM = 0x01;
    public static final int PLATFORM_JAWS = 0x00;
    public static final int PLATFORM_FIBER = 0x01;
    public static final int PLATFORM_EAGLE   = 0x02;
    public static final int PLATFORM_DOLPHIN = 0x03;
    public static final int PLATFORM_RABBIT = 0x04;
    public static final int PLATFORM_CHEETAH = 0x05;
    public static final int PLATFORM_CMCCWASU = 0x08;
    public static final int PLATFORM_ALIYUN = 0x09;
    public static final int PLATFORM_TVD = 0x0A;
    public static final int PLATFORM_IPTV = 0x0B;          //IPTV
    public static final int PLATFORM_UNKNOWN = 0xFF;
    
    static {
        System.loadLibrary("config_jni");
    }
    
    public static int getTargetPlatform(int platformType) {
        return nativeGetTargetPlatform(platformType);    
    }
    
    private static native int nativeGetTargetPlatform(int type);
}

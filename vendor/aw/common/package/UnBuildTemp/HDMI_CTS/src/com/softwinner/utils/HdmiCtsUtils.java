
package com.softwinner.utils;

import com.softwinner.utils.HdmiCapability;
import com.softwinner.utils.HdmiOutputConfig;

public class HdmiCtsUtils {

    static {
        System.loadLibrary("hdmicts_jni");
    }

    public static native HdmiCapability getHdmiCapability();
    public static native int[] getHdmiSupportMode();
    public static native int setHdmiOutputConfig(HdmiOutputConfig config);
    public static native HdmiOutputConfig getHdmiOutputConfig();
    public static native HdmiCapability getConstraintByHdmiMode(int mode);
    public static native int getHdcpState();
    public static native int setHdcpState(int enable);
    public static native int getHotplugReportEnabled();
    public static native int setHotplugReportEnabled(int enable);
    public static native int getCecStandbyEnabled();
    public static native int setCecStandbyEnabled(int enable);
    public static native int getOutputType();
}

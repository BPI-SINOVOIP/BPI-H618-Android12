package com.softwinner.dragonbox.entity;

public class StorageInfo {
    public static final String PATH_EXDSD_PREFIX = "/mnt/extsd";
    public static final String PATH_USB_DEVICE_PREFIX = "/mnt/usbhost";

    private String mPath;
    private boolean isRWable;
    private boolean isMounted;

    public String getPath() {
        return mPath;
    }

    public void setPath(String path) {
        this.mPath = path;
    }

    public boolean isRWable() {
        return isRWable;
    }

    public void setRWable(boolean isRWable) {
        this.isRWable = isRWable;
    }

    public boolean isMounted() {
        return isMounted;
    }

    public void setMounted(boolean isMounted) {
        this.isMounted = isMounted;
    }

    public static String getPathExdsdPrefix() {
        return PATH_EXDSD_PREFIX;
    }

    public static String getPathUsbDevicePrefix() {
        return PATH_USB_DEVICE_PREFIX;
    }


}

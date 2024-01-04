package com.softwinner.dragonbox.entity;

public class StorageInfo {
    public static final String SD_FLAG = "public:179_";
    public static final String USB_FLAG = "public:8_";

    private String mPath;
    private String mId;//for example: 179:24,8:1
    private boolean isRWable;
    private boolean isMounted;

    public String getId() {
        return mId;
    }

    public void setId(String id) {
        this.mId = id;
    }

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

    public static String getSDFlag() {
        return SD_FLAG;
    }

    public static String getUSBFlag() {
        return USB_FLAG;
    }


}

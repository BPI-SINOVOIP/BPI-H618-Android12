package com.softwinner;

public class ISOMountManager {
    private static final String TAG = "ISOMountManager";
            
    public static class MountInfo {
        private String mMountPath;
        private String mISOPath;

        public MountInfo(String mountPath, String isoPath) {
            mMountPath = mountPath;
            mISOPath = isoPath;
        }

        public String getMountPath() {
            return mMountPath;
        }

        public String getISOPath() {
            return mISOPath;
        }
    }
        
    static {
        System.loadLibrary("isomountmanager_jni");
        init();
    }
    
    private static native void         init();
    
    public static native int                 mount(String mountPath, String isoPath);
    public static native int                 umount(String mountPath);
    public static native void                 umountAll();
    public static native MountInfo[]        getMountInfoList();
    public static native String             getIsoPath(String mountPath);
    public static native String[]           getMountPoints(String isoPath);
}

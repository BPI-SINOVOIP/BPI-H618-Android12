package com.softwinner;

import android.util.Log;

import java.lang.ref.WeakReference;

public class SecureFile {
    private static final String TAG = "SecureFile";

    private String mAbsolutePath;
    private static final String PRIVATE_DIRECTORY = getDirectory("PRIVATE_STORAGE", "/private");
    private static String getDirectory(String variableName, String defaultPath) {
        String path = System.getenv(variableName);
        return path == null ? defaultPath : path;
    }
    
    private int mSecureFileService; // accessed by native methods

    static {
        System.loadLibrary("securefile_jni");
        nativeInit();
    }

    private boolean isAbsolutePath(String path) {
        if (path.startsWith("/")) {
            return true;
        }
        return false;
    }
    
    private String changePathToAbsolute(String path) {
        String absolutePath = path;
        String parent = PRIVATE_DIRECTORY;
        if (path == null) {
            return null;
        }
        if (!isAbsolutePath(path)) {
            absolutePath = parent + "/" + path;
        }
        return absolutePath;
    }
    
    public SecureFile(String path) {
        mAbsolutePath = changePathToAbsolute(path);
        nativeSetup(new WeakReference<SecureFile>(this));
    }

    public boolean createFile() {
        if (!exists()) {
            return nativeCreateFile(mAbsolutePath);
        }
        return false;
    }

    public boolean delete() {
        return nativeDelete(mAbsolutePath);
    }

    public boolean exists() {
        return nativeExists(mAbsolutePath);
    }

    public String getParent() {
        String parent = null;
        try {
            parent = mAbsolutePath.substring(0, mAbsolutePath.lastIndexOf("/"));
        } catch (IndexOutOfBoundsException e) {
            Log.e(TAG, "get parent fail");
        }
        return parent;
    }
    
    public String getPath() {
        return mAbsolutePath;
    }

    public long getTotalSpace() {
        return nativeGetTotalSpace(mAbsolutePath);
    }

    public long getUsableSpace() {
        return nativeGetUsableSpace(mAbsolutePath);
    }
    
    public boolean  isDirectory() {
        return nativeIsDirectory(mAbsolutePath);
    }

    public boolean isFile() {
        return nativeIsFile(mAbsolutePath);
    }

    public long length() {
        return nativeLength(mAbsolutePath);
    }

    public String[] list() {
        return nativeList(mAbsolutePath);
    }

    public boolean  mkdir() {
        return nativeMkdir(mAbsolutePath);
    }

    public boolean mkdirs() {
        /* If the terminal directory already exists, answer false */
        if (exists()) {
            return false;
        }

        /* If the receiver can be created, answer true */
        if (mkdir()) {
            return true;
        }

        String parentDir = getParent();
        /* If there is no parent and we were not created, answer false */
        if (parentDir == null) {
            return false;
        }

        /* Otherwise, try to create a parent directory and then this directory */
        return (new SecureFile(parentDir).mkdirs() && mkdir());
    }

    public long lastModified() {
        return nativeLastModified(mAbsolutePath);
    }

    public boolean renameTo(String newPath) {
        String newFilePath = changePathToAbsolute(newPath);
        if (newFilePath.isEmpty()) {
            return false;
        }
        return nativeRenameTo(mAbsolutePath, newFilePath);
    }

    public boolean setLastModified(long time) {
        return nativeSetLastModified(mAbsolutePath, time);
    }

    public boolean write(String srcFilePath, boolean append) {
        return nativeWriteToFile(changePathToAbsolute(srcFilePath), mAbsolutePath, append);
    }

    public boolean write(byte[] srcData, boolean append) {
        int count = srcData.length;
        return nativeWriteInData(srcData, count, mAbsolutePath, append);
    }

    public boolean read(String destFilePath) {
        return nativeWriteToFile(mAbsolutePath, changePathToAbsolute(destFilePath), false);
    }

    public boolean read(byte[] destData) {
        int count = destData.length;
        return nativeRead(destData, count, mAbsolutePath);
    }
    
    
    @Override
    protected void finalize() {
        nativeFinalize();
    }

    private static native void nativeInit();
    private native void nativeFinalize();
    private native void nativeSetup(Object mediaplayerThis);
    private native boolean nativeCreateFile(String path);
    private native boolean nativeDelete(String path);
    private native boolean nativeExists(String path);
    private native long    nativeGetTotalSpace(String path);
    private native long       nativeGetUsableSpace(String path);
    private native boolean nativeIsDirectory(String path);
    private native boolean nativeIsFile(String path);
    private native long nativeLength(String path);
    private native String[] nativeList(String path);
    private native boolean nativeMkdir(String path);
    private native long    nativeLastModified(String path);
    private native boolean nativeRenameTo(String oldPath, String newPath);
    private native boolean nativeSetLastModified(String path, long time);
    private native boolean nativeWriteToFile(String srcFilePath,
        String desFilePath, boolean append);
    private native boolean nativeWriteInData(byte[] srcData, int count,
        String desFilePath, boolean append);
    private native boolean nativeRead(byte[] desData, int count, String srcFilePath);
}

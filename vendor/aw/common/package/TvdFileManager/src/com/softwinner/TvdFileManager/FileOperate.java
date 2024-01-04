package com.softwinner.TvdFileManager;

import android.content.Context;
import android.os.SystemClock;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.SecurityException;

public class FileOperate {
    private static final int BUFFER = 65536;
    private static final String TAG = "FileOperate";
    private long copySize = 0;
    private long copyNum = 0;
    private long deleteSize = 0;
    private long deleteNum = 0;
    private long fileSize = 0;
    private long fileNum = 0;
    private boolean isCancel = false;
    private Context mContext;
    private boolean isCopying=false;

    public FileOperate() {
    }

    public FileOperate(Context context) {
        mContext = context;
    }

    /**
     *
     * @param old
     *            the file to be copied
     * @param newDir
     *            the directory to move the file to
     * @return 0:success -1:fail
     */
    public int copyToDirectory(String old, String newDir) {
        isCopying = true;
        if (isCancel) {
            return 0;
        }
        copyNum++;
        File oldFile = new File(old);
        File tempDir = new File(newDir);
        if(oldFile.getParent().equals(newDir)){
            return 0;
        }
        byte[] data = new byte[BUFFER];
        int read = 0;
        BufferedOutputStream oStream = null;
        BufferedInputStream iStream = null;

        if (oldFile.isFile() && tempDir.isDirectory() && tempDir.canWrite()) {
            String fileName = old.substring(old.lastIndexOf("/"), old.length());
            String newName = newDir + fileName;
            File cpFile = new File(newName);
            try {
                oStream = new BufferedOutputStream(new FileOutputStream(
                        cpFile));
                iStream = new BufferedInputStream(
                        new FileInputStream(oldFile));
                while ((read = iStream.read(data, 0, BUFFER)) != -1) {
                    if (isCancel) {
                        return 0;
                    }

                    oStream.write(data, 0, read);
                    SystemClock.sleep(2); //sleep 5ms -> 2ms
                    copySize += read;
                }
                oStream.flush();
                iStream.close();
                oStream.close();

                RefreshMedia mRefresh = new RefreshMedia(mContext);
                mRefresh.notifyMediaAdd(newName);
            } catch (FileNotFoundException e) {
                Log.d(TAG, e.getMessage());
                return -1;
            } catch (IOException e) {
                Log.d(TAG, e.getMessage());
                return -1;
            }
            finally{
                try{
                    if(iStream != null){
                        iStream.close();
                    }
                    if(oStream != null){
                        oStream.close();
                    }
                }
                catch(IOException ie){
                    Log.d(TAG, ie.getMessage());
                }
            }
        } else if (oldFile.isDirectory() && tempDir.isDirectory() && tempDir.canWrite()) {
            String[] files = oldFile.list();
            String dir = newDir + old.substring(old.lastIndexOf("/"), old.length());
            int len = files.length;
            File f = new File(dir);
            if (!f.exists()) {
                if (!f.mkdir()) {
                    Log.d(TAG, "fail to make dir:" + dir);
                    return -1;
                }
            }
            for (int i = 0; i < len; i++) {
                copyToDirectory(old + "/" + files[i], dir);
            }
        } else if (!tempDir.canWrite()) {
            Log.d(TAG, "has not permission to write to " + newDir);
            return -1;
        }
        runCmd("sync");
        isCopying = false;
        return 0;
    }

    /**
     * @param filePath
     * @param newName
     * @return -1:newName file is exist; -2:rename fail; 0:rename success;
     */
    public int renameTarget(String filePath, String newName) throws SecurityException{
        File src = new File(filePath);
        String ext = "";
        File dest;

        if (src.isFile()) {
            try {
                ext = filePath.substring(filePath.lastIndexOf("."), filePath.length());
            } catch (IndexOutOfBoundsException e) {
                e.printStackTrace();
            }
        }
        if (newName.length() < 1) {
            return -2;
        }

        String temp = filePath.substring(0, filePath.lastIndexOf("/"));
        String destPath = temp + "/" + newName + ext;
        dest = new File(destPath);
        if (dest.exists()) {
            return -1;
        }
        if (src.renameTo(dest)) {
            RefreshMedia mRefresh = new RefreshMedia(mContext);
            mRefresh.notifyMediaAdd(destPath);
            mRefresh.notifyMediaDelete(filePath);
            runCmd("sync");
            return 0;
        } else {
            return -2;
        }
    }

    public boolean mkdirTarget(String parent, String newDir) {
        File parentFile = new File(parent);
        File newFile = new File(parentFile, newDir);
        runCmd("sync");
        return newFile.mkdir();
    }

    private void runCmd(String cmd) {
        try {
            Runtime runTime = Runtime.getRuntime();
            Process proc = runTime.exec(cmd);
            proc.waitFor();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public int deleteTarget(String path) {
        if (isCancel) {
            return 0;
        }
        File target = new File(path);
        if (target.exists() && target.isFile() && target.canWrite()) {
            deleteNum++;
            if (target.delete()) {
                runCmd("sync");
            }
            RefreshMedia mRefresh = new RefreshMedia(mContext);
            mRefresh.notifyMediaDelete(path);
            return 0;
        } else if (target.exists() && target.isDirectory() && target.canRead()) {
            String[] fileList = target.list();
            if (fileList != null && fileList.length == 0) {
                deleteNum++;
                target.delete();
                runCmd("sync");
                return 0;
            } else if (fileList != null && fileList.length > 0) {
                for (int i = 0; i < fileList.length; i++) {
                    String filePath = target.getAbsolutePath() + "/" + fileList[i];
                    File tempF = new File(filePath);
                    if (tempF.isDirectory()) {
                        deleteTarget(tempF.getAbsolutePath());
                    } else if (tempF.isFile()) {
                        deleteNum++;
                        tempF.delete();
                        runCmd("sync");
                        RefreshMedia mRefresh = new RefreshMedia(mContext);
                        mRefresh.notifyMediaDelete(filePath);
                    }
                }
            }
            if (target.exists()) {
                if (target.delete()) {
                    runCmd("sync");
                    return 0;
                }
            }
        }
        return -1;
    }

    public void scanFiles(String path) {
        fileSize = 0;
        fileNum = 0;
        File file = new File(path);
        try {
            if (file.exists()) {
                scanFiles(file);
            } else {
                return;
            }
        } catch (SecurityException e) {
            return;
        }
    }

    private void scanFiles(File file) {
        fileNum++;
        if (file.isFile()) {
            fileSize += file.length();
        } else if (file.isDirectory() && file.canRead()) {
            File[] list = file.listFiles();
            for (int i = 0; i < list.length; i++) {
                scanFiles(list[i]);
            }
        }
    }

    /*   */
    public long getScanSize() {
        return fileSize;
    }

    public long getScanNum() {
        return fileNum;
    }

    public long getDeletedNum() {
        return deleteNum;
    }

    public long getCopySize() {
        return copySize;
    }

    public long getCopyNum() {
        return copyNum;
    }

    public void setCancel() {
        isCancel = true;
    }
    public boolean isCopying() {
        return isCopying;
    }
}

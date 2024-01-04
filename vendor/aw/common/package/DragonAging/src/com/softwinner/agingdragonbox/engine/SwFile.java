package com.softwinner.agingdragonbox.engine;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class SwFile {

    public static final String TAG                 = "SwFile";

    public static final int    DEFAULT_BUFFER_SIZE = 512 * 1024;

    public static final String SIZE_KB_STR         = "KB";
    public static final String SIZE_MB_STR         = "MB";
    public static final String SIZE_GB_STR         = "GB";

    public static final int    SIZE_KB             = 1024;
    public static final int    SIZE_MB             = 1024 * SIZE_KB;
    public static final int    SIZE_GB             = 1024 * SIZE_MB;

    public static long createRamdomFile(File to, long size, OnProgressListener listener) {
        return size;
    }

    public static boolean copyAssetTo(Context context, String from, String to) throws IOException {
        AssetManager am = context.getAssets();
        InputStream in = am.open(from);
        BufferedInputStream bufferedIn = new BufferedInputStream(in);

        File toFile = new File(to);
        FileOutputStream fout = new FileOutputStream(toFile);
        BufferedOutputStream bufferedOut = new BufferedOutputStream(fout);

        long avil = in.available();
        long write = copyStream(bufferedIn, bufferedOut);
        in.close();
        fout.close();
        bufferedIn.close();
        bufferedOut.close();
        return avil == write;
    }

    public static long copyFileTo(String from, String to, int flag, OnProgressListener listener,
            long max) throws IOException {
        return copyFileTo(new File(from), new File(to), flag, listener, max);
    }

    public static long copyFileTo(File from, File to, int flag, OnProgressListener listener,
            long max) throws IOException {
        FileInputStream fis = new FileInputStream(from);
        BufferedInputStream bis = new BufferedInputStream(fis);
        FileOutputStream fos = new FileOutputStream(to);
        BufferedOutputStream bos = new BufferedOutputStream(fos);

        byte[] buf = new byte[DEFAULT_BUFFER_SIZE];
        long fileLen = from.length();
        int rlen = 0;
        int wlen = 0;
        long rtotal = 0;
        long wtotal = 0;
        long progress = 0;

        long t = System.currentTimeMillis();

        while ((rlen = bis.read(buf)) > 0) {
            bos.write(buf, 0, rlen);
            wlen = rlen;
            rtotal += rlen;
            wtotal += wlen;
            if (listener != null) {
                progress = wtotal * max / fileLen;
                listener.onProgressChange(progress);
            }
        }

        long cost = System.currentTimeMillis() - t;

        fis.close();
        bis.close();
        fos.close();
        bos.close();
        return cost;
    }

    public static long copyStream(InputStream in, OutputStream out) throws IOException {
        byte[] buf = new byte[DEFAULT_BUFFER_SIZE];
        int rlen = 0;
        long wtotal = 0;
        while ((rlen = in.read(buf)) > 0) {
            wtotal += rlen;
            out.write(buf, 0, rlen);
        }
        return wtotal;
    }

    public static long readSpeed(File file, OnProgressListener listener, long max)
            throws IOException {
        FileInputStream fis = new FileInputStream(file);
        BufferedInputStream bis = new BufferedInputStream(fis);
        byte[] buf = new byte[DEFAULT_BUFFER_SIZE];
        long fileLen = file.length();
        long progress = 0;
        int length;
        long rTotal = 0;
        long t = System.currentTimeMillis();
        while ((length = bis.read(buf)) > 0) {
            rTotal += length;
            if (listener != null) {
                progress = rTotal * max / fileLen;
                listener.onProgressChange(progress);
            }
        }
        long cost = System.currentTimeMillis() - t;
        fis.close();
        return cost;
    }

    public static long readDualSpeed(File first, File second, OnProgressListener listener1,
            OnProgressListener listener2, long max, int offsetPercent) throws IOException {
        FileInputStream fis1 = new FileInputStream(first);
        BufferedInputStream bis1 = new BufferedInputStream(fis1);

        FileInputStream fis2 = new FileInputStream(second);
        BufferedInputStream bis2 = new BufferedInputStream(fis2);

        byte[] buf = new byte[480 * 1024];
        long firstFileLen = first.length();
        long secondFileLen = second.length();
        long progress = 0;
        int length1;
        int length2;
        long rTotal1 = 0;
        long rTotal2 = 0;
        long t = System.currentTimeMillis();
        long skipOffset1 = offsetPercent * firstFileLen / 100;
        long skipOffset2 = offsetPercent * secondFileLen / 100;
        bis1.skip(skipOffset1);
        bis2.skip(skipOffset2);
        Log.v(TAG, "skip1:" + skipOffset1 + " skip2:" + skipOffset2 + " percent:" + offsetPercent);
        boolean rcompleted1 = false;
        boolean rcompleted2 = false;
        while (true) {
            length1 = rcompleted1 ? 0 : bis1.read(buf);
            length2 = rcompleted2 ? 0 : bis2.read(buf);
            rcompleted1 = length1 <= 0;
            rcompleted2 = length2 <= 0;
            rTotal1 += length1;
            rTotal2 += length2;
            if (listener1 != null && !rcompleted1) {
                progress = (rTotal1 + skipOffset1) * max / firstFileLen;
                listener1.onProgressChange(progress);
            }
            if (listener2 != null && !rcompleted2) {
                progress = (rTotal2 + skipOffset2) * max / secondFileLen;
                listener2.onProgressChange(progress);
            }
            if (rcompleted1 && rcompleted2) {
                break;
            }
        }
        long cost = System.currentTimeMillis() - t;
        fis1.close();
        bis1.close();
        fis2.close();
        bis2.close();
        return cost;
    }

    public static long createZeroFile(File file, long size, OnProgressListener listener, long max)
            throws IOException {
        FileOutputStream fis = new FileOutputStream(file);
        BufferedOutputStream bis = new BufferedOutputStream(fis);
        byte[] buf = new byte[DEFAULT_BUFFER_SIZE];
        long wTotal = 0;
        int wSize = 0;
        long progress = 0;
        long t = System.currentTimeMillis();
        while (wTotal < size) {
            wSize = (int) ((size - wTotal > DEFAULT_BUFFER_SIZE) ? DEFAULT_BUFFER_SIZE
                    : (size - wTotal));
            fis.write(buf, 0, wSize);
            wTotal += wSize;
            if (listener != null) {
                progress = wTotal * max / size;
                listener.onProgressChange(progress);
            }
        }
        long cost = System.currentTimeMillis() - t;
        fis.close();
        bis.close();
        return cost;
    }

    public static long writeSpeed(File file, long size, OnProgressListener listener, long max)
            throws IOException {
        return createZeroFile(file, size, listener, max);
    }

    public static boolean deleteDir(File dir) {
        if (dir.isDirectory()) {
            String[] children = dir.list();
            if (children != null) {
                for (int i = 0; i < children.length; i++) {
                    boolean success = deleteDir(new File(dir, children[i]));
                    if (!success) {
                        return false;
                    }
                }
            }
        }
        return dir.delete();
    }

    public static String byteToSize(long byteSize) {
        String s = byteToGb(byteSize);
        if (s.startsWith("0.")) {
            s = byteToMb(byteSize);
            if (s.startsWith("0.")) {
                s = byteToKb(byteSize);
            }
        }
        return s;
    }

    public static String byteToKb(long byteSize) {
        String kb = String.format("%.2f %s ", (double) byteSize / 1024, SIZE_KB_STR);
        return kb;
    }

    public static String byteToMb(long byteSize) {
        String mb = String.format("%.2f %s ", (double) byteSize / (1024 * 1024), SIZE_MB_STR);
        return mb;
    }

    public static String byteToGb(long byteSize) {
        String gb = String
                .format("%.2f %s ", (double) byteSize / (1024 * 1024 * 1024), SIZE_GB_STR);
        return gb;
    }

    public interface OnProgressListener {
        void onProgressChange(long progress);
    }
}

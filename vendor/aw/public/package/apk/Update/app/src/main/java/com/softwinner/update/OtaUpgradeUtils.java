package com.softwinner.update;

import android.content.Context;
import android.os.Build;
import android.os.RecoverySystem;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.security.GeneralSecurityException;

public class OtaUpgradeUtils {

    public static final int ERROR_INVALID_UPGRADE_PACKAGE = 0;

    public static final int ERROR_FILE_DOES_NOT_EXIT = 1;

    public static final int ERROR_FILE_IO_ERROR = 2;

    public static final String CHCHE_PARTITION = "/cache/";

    public static final String DEFAULT_PACKAGE_NAME = "update.zip";

    private Context mContext;

    private boolean mDeleteSource = false;

    public OtaUpgradeUtils(Context context) {
        mContext = context;
    }

    public interface ProgressListener extends RecoverySystem.ProgressListener {
        @Override
        public void onProgress(int progress);

        public void onVerifyFailed(int errorCode, Object object);

        public void onCopyProgress(int progress);

        public void onCopyFailed(int errorCode, Object object);
    }

    public boolean upgredeFromOta(File packageFile, ProgressListener progressListener) {
        try {
            RecoverySystem.verifyPackage(packageFile, progressListener, null);
        } catch (IOException e) {
            progressListener.onVerifyFailed(ERROR_FILE_DOES_NOT_EXIT, packageFile.getPath());
            e.printStackTrace();
            return false;
        } catch (GeneralSecurityException e) {
            progressListener.onVerifyFailed(ERROR_INVALID_UPGRADE_PACKAGE, packageFile.getPath());
            e.printStackTrace();
            return false;
        }

        boolean b = copyFile(packageFile, new File(CHCHE_PARTITION + DEFAULT_PACKAGE_NAME),
                progressListener);
        if (b && packageFile.getAbsolutePath().equals(Configs.DOWNLOAD_PATH) && mDeleteSource) {
            packageFile.delete();
        }
        if (b) {
            installPackage(mContext, new File(CHCHE_PARTITION + DEFAULT_PACKAGE_NAME));
            return true;
        }
        return false;
    }

    public boolean upgradeFromOta(String packagePath, ProgressListener progressListener) {
        return upgredeFromOta(new File(packagePath), progressListener);
    }

    public void deleteSource(boolean b) {
        mDeleteSource = b;
    }

    public static boolean copyFile(File src, File dst, ProgressListener listener) {
        long inSize = src.length();
        long outSize = 0;
        int progress = 0;
        listener.onCopyProgress(progress);
        try {
            if(dst.exists()){
                dst.delete();
            }
            dst.createNewFile();
            FileInputStream in = new FileInputStream(src);
            FileOutputStream out = new FileOutputStream(dst);
            int length = -1;
            byte[] buf = new byte[1024];
            while ((length = in.read(buf)) != -1) {
                out.write(buf, 0, length);
                outSize += length;
                int temp = (int) (((float) outSize) / inSize * 100);
                if (temp != progress) {
                    progress = temp;
                    listener.onCopyProgress(progress);
                }
            }
            out.flush();
            in.close();
            out.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return false;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public static boolean copyFile(String src, String dst, ProgressListener listener) {
        return copyFile(new File(src), new File(dst), listener);
    }

    public static void installPackage(Context context, File packageFile) {
        try {
            RecoverySystem.installPackage(context, packageFile);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static boolean checkVersion(long newVersion, String product) {
        return (Build.TIME <= newVersion * 1000 && (Build.DEVICE.equals(product) || Build.PRODUCT
                .equals(product)));
    }

    public static boolean checkIncVersion(String fingerprinter, String product) {
        return (Build.FINGERPRINT.equals(fingerprinter) && (Build.DEVICE.equals(product) || Build.PRODUCT
                .equals(product)));
    }
}

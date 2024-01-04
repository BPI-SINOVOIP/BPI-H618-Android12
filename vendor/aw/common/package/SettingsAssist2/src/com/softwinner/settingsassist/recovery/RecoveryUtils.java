
package com.softwinner.settingsassist.recovery;

import android.content.Context;
import android.os.Build;
import android.os.PowerManager;
import com.softwinner.os.RecoverySystemEx;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.security.GeneralSecurityException;

public class RecoveryUtils {

    public static final String TAG = "RecoveryUtils";

    public static final int ERROR_INVALID_UPGRADE_PACKAGE = 0;
    public static final int ERROR_FILE_DOES_NOT_EXIT = 1;
    public static final int ERROR_FILE_IO_ERROR = 2;

    /* cache分区挂载目录 */
    public static final String CHCHE_PARTITION = "/cache";
    /* 默认升级包名字 */
    public static final String DEFAULT_PACKAGE_NAME = "update.zip";

    /* recovery相关信息存放目录 */
    private static File RECOVERY_DIR = new File("/cache/recovery");
    /* recovery命令文件 */
    private static File COMMAND_FILE = new File(RECOVERY_DIR, "command");
    /* recovery日志文件 */
    private static File LOG_FILE = new File(RECOVERY_DIR, "log");
    private static String LAST_PREFIX = "last_";

    private Context mContext;

    public RecoveryUtils(Context context) {
        mContext = context;
    }

    /**
     * 执行recoverySystem时的进度回调
     */
    public interface RecoveryProgress {
        public void onVerifyProgress(int progress);

        public void onVerifyFailed(int errorCode, Object object);

        public void onCopyProgress(int progress);

        public void onCopyFailed(int errorCode, Object object);
    }

    /**
     * 开始进行recovery升级/恢复系统，流程是校验文件-->拷贝文件至cache分区-->重启进入recovery
     *
     * @param packageFile 用于recovery升级/恢复系统的升级包
     * @param progressListener recovery进度回调
     * @return 成功返回true，失败返回false
     */
    public boolean recoverySystem(File packageFile,
            final RecoveryProgress progressListener) {
        RecoverySystemEx.ProgressListener tmpProgresslistener = new RecoverySystemEx.ProgressListener() {
            @Override
            public void onProgress(int progress) {
                progressListener.onVerifyProgress(progress);
            }
        };
        try {
            RecoverySystemEx.verifyPackage(packageFile, tmpProgresslistener,
                    null);
        } catch (IOException e) {
            e.printStackTrace();
            progressListener.onVerifyFailed(ERROR_FILE_DOES_NOT_EXIT,
                    packageFile.getPath());
            return false;
        } catch (GeneralSecurityException e) {
            e.printStackTrace();
            progressListener.onVerifyFailed(ERROR_INVALID_UPGRADE_PACKAGE,
                    packageFile.getPath());
            return false;
        }

        try {
            Log.w(TAG, "filePath:" + packageFile.getCanonicalPath());
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
        try {
            if (packageFile.getCanonicalPath().startsWith("/storage"))
            {
                Log.w(TAG, "use installPackage");
				if (packageFile.getCanonicalPath().startsWith("/storage/emulated")){
					Log.w(TAG, "/storage/emulated use installPackage!");
					RecoverySystemEx.installPackage(mContext, packageFile);
				}
				else{
					Log.w(TAG, "not /storage/emulated use installPackageEx!");
					RecoverySystemEx.installPackageFromUsbDisk(mContext, packageFile);
				}
            }
            else
            {
                Log.w(TAG, "use installPackageEx");
                RecoverySystemEx.installPackageEx(mContext, packageFile);
            }

        } catch (IOException e) {
        }
        return false;
    }

    /**
     * 开始进行recovery升级/恢复系统，流程是校验文件-->拷贝文件至cache分区-->重启进入recovery
     *
     * @param packagePath 用于recovery升级/恢复系统的升级包
     * @param progressListener recovery进度回调
     * @return 成功返回true，失败返回false
     */
    public boolean recoverySystem(String packagePath, RecoveryProgress progressListener) {
        return recoverySystem(new File(packagePath), progressListener);
    }

    /**
     * 拷贝文件到指定位置
     *
     * @param src 原文件
     * @param dst 目标文件
     * @param listener
     * @return 成功返回ture，失败返回false
     */
    private static boolean copyFile(File src, File dst, RecoveryProgress listener) {
        long inSize = src.length();
        long outSize = 0;
        int progress = 0;
        listener.onCopyProgress(progress);
        try {
            if (!dst.exists()) {
                dst.createNewFile();
            }
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
            Log.w(TAG, "copy package failed " + e);
            return false;
        } catch (IOException e) {
            Log.w(TAG, "copy package failed " + e);
            return false;
        }
        return true;
    }

    private static void installPackage(Context context, File packageFile) {
        try {
            String filename = packageFile.getPath();
            Log.w(TAG, "!!! REBOOTING TO INSTALL " + filename + " !!!");
            String arg = "--update_package=" + filename;
            bootCommand(context, arg);
        } catch (IOException e) {
            Log.w(TAG, "install package failed " + e);
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

    private static void bootCommand(Context context, String arg) throws IOException {
        RECOVERY_DIR.mkdirs(); // In case we need it
        COMMAND_FILE.delete(); // In case it's not writable
        LOG_FILE.delete();

        FileWriter command = new FileWriter(COMMAND_FILE);
        try {
            command.write(arg);
            command.write("\n");
        } finally {
            command.close();
        }

        // Having written the command file, go ahead and reboot
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        pm.reboot("recovery");

        throw new IOException("Reboot failed (no permissions?)");
    }
}

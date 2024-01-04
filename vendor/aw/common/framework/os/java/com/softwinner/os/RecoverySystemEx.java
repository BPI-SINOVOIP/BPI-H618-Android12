
package com.softwinner.os;

import android.content.Context;
import android.os.PowerManager;
import android.os.RecoverySystem;
import android.util.Log;

import java.io.File;
import java.io.IOException;

public class RecoverySystemEx extends RecoverySystem {

    private static final String TAG = "RecoverySystemEx";

    protected static final File RECOVERY_DIR = new File("/cache/recovery");
    protected static final File COMMAND_FILE = new File(RECOVERY_DIR, "command");
    protected static final String USB_RECOVERY_CMD = "usb-recovery";
    protected static final String INSTALL_PACKAGE_EX_CMD = "--ex_update_package=%s,%s,%s";

    static {
        System.loadLibrary("jni_swos");
    }

    public static boolean rebootToUSBRecovery(Context context) {
        Log.w(TAG, "!!! REBOOTING TO RECOVERY ON USB !!! ");
        PowerManager pm = (PowerManager) context
                .getSystemService(Context.POWER_SERVICE);
        pm.reboot(USB_RECOVERY_CMD);
        return true;
    }

    public static boolean installPackageEx(Context context, String path)
            throws IOException {
        if (path != null && nativeInstallPackageEx(context, path)) {
            PowerManager pm = (PowerManager) context
                    .getSystemService(Context.POWER_SERVICE);
            pm.reboot("recovery");
            return true;
        }
        return false;
    }

    public static boolean installPackageEx(Context context, File file)
            throws IOException {
        if (file != null) {
            return installPackageEx(context, file.getCanonicalPath());
        }
        return false;
    }

    private static native boolean nativeInstallPackageEx(Context context,
            String path);


	public static boolean installPackageFromUsbDisk(Context context, File file)
			throws IOException{
			if(file != null){
				return installPackageFromUsbDisk(context, file.getCanonicalPath());
			}
			return false;
	}

	public static boolean installPackageFromUsbDisk(Context context, String path)
		throws IOException{
	     if (path != null && nativeInstallPackageFromUsbDisk(context, path)) {
            PowerManager pm = (PowerManager) context
                    .getSystemService(Context.POWER_SERVICE);
            pm.reboot("recovery");
            return true;
        }
        return false;
	}

	private static native boolean nativeInstallPackageFromUsbDisk(Context context, String path);

}

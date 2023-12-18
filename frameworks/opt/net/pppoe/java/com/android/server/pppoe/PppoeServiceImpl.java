package com.android.server.pppoe;

import android.content.Context;
import android.content.pm.PackageManager;
import com.android.internal.util.IndentingPrintWriter;
import android.net.ConnectivityManager;
import android.net.IEthernetManager;
import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.IpConfiguration.ProxySettings;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.NetworkAgent;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.net.RouteInfo;
import android.net.StaticIpConfiguration;
import android.os.Binder;
import android.os.IBinder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.INetworkManagementService;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.PrintWriterPrinter;

import android.content.Intent;
import android.content.IntentFilter;
import android.net.PppoeManager;
import android.net.IPppoeManager;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.concurrent.atomic.AtomicBoolean;
import android.provider.Settings;
import android.content.ContentResolver;
import android.net.IEthernetManager;
import java.io.FileOutputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.attribute.PosixFilePermission;
import java.util.HashSet;
import java.util.Set;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
/**
 * @hide
 */
public class PppoeServiceImpl extends IPppoeManager.Stub {
    private static final String TAG = "PppoeServiceImpl";
    public static final boolean DEBUG = true;
    public Context mContext;
    public String mIface="";
    public Handler mHandler;
    public PppoeNetworkFactory mTracker;

    private final String PPPOE_CONFIG_FILE      = "/data/misc/pppoe/pppoe-secrets";
    private final String PPPOE_INTERFACE_FILE   = "/data/misc/pppoe/pppoe-interface";

    private File config_file;
    private File interface_file;
    private File directory;

    private final INetworkManagementService mNMService;
    private final AtomicBoolean mStarted = new AtomicBoolean(false);
    private ConnectivityManager mCM;
    private static void LOG(String msg) {
        if (DEBUG) {
            Log.d(TAG, msg);
        }
    }

    private void enforceAccessPermission() {
        mContext.enforceCallingOrSelfPermission(
                        android.Manifest.permission.ACCESS_NETWORK_STATE,
                        "PppoeService");
    }

    private void enforceChangePermission() {
        mContext.enforceCallingOrSelfPermission(
                        android.Manifest.permission.CHANGE_NETWORK_STATE,
                        "PppoeService");
    }

    private void enforceConnectivityInternalPermission() {
        mContext.enforceCallingOrSelfPermission(
                        android.Manifest.permission.CONNECTIVITY_INTERNAL,
                        "ConnectivityService");
    }


    public PppoeServiceImpl(Context context) {
        Log.i(TAG, "Creating PppoeServiceImpl");
        mContext=context;
        IBinder b  = ServiceManager.getService(Context.NETWORKMANAGEMENT_SERVICE);
        mNMService = INetworkManagementService.Stub.asInterface(b);

        mTracker = new PppoeNetworkFactory();
    }

    public int getPppoeState(String iface) {
        return mTracker.getPppoeState(iface);
    }

    public boolean setupPppoe(String iface ,String user,String password) {
        String config_file_path;
        String interface_file_path = PPPOE_INTERFACE_FILE;

        directory = new File("/data/misc/pppoe");
        if (!directory.isDirectory()) {
            Log.w(TAG,"create folder");
            directory.mkdir();
        }
        if (TextUtils.isEmpty(iface)) {
            config_file_path = PPPOE_CONFIG_FILE;
        } else {
            config_file_path = String.format("%s-%s", PPPOE_CONFIG_FILE, iface);
        }
        config_file = new File(config_file_path);
        interface_file  = new File(interface_file_path);
        try {
            // 1. Save pppoe-secrets
            BufferedOutputStream out = new BufferedOutputStream(
                    new FileOutputStream(config_file));
            out.write(user.getBytes());
            out.write("\n".getBytes());
            out.write(password.getBytes());
            Log.d(TAG, "write to " + config_file_path
                    + " user: " + user + " password: " + password);
            out.flush();
            out.close();
            // 2. Save pppoe-interface
            out = new BufferedOutputStream(
                    new FileOutputStream(interface_file));
            out.write(iface.getBytes());
            Log.d(TAG, "write to " + PPPOE_INTERFACE_FILE
                    + " interface info = " + iface);
            out.flush();
            out.close();

            return true;
        } catch (IOException e) {
            Log.e(TAG, "Write PPPoE config failed!" + e);
            return false;
        }
    }
    public boolean startPppoe(String iface) {
        Log.w(TAG,"statPppoe");
        List<String> li = getPppoeUserInfo(iface);
        final ContentResolver cr = mContext.getContentResolver();
        try {
            Settings.Global.putInt(cr, Settings.Global.PPPOE_ENABLED, 1);
        } catch (Exception ex) {
            Log.e(TAG, "setEthernetMode error!");
        }
        changeFolderPermission(config_file);
        changeFolderPermission(interface_file);
        changeFolderPermission(directory);
        return mTracker.startPppoe(iface);
    }

    public boolean stopPppoe(String iface) {
        final ContentResolver cr = mContext.getContentResolver();
        try {
            Settings.Global.putInt(cr, Settings.Global.PPPOE_ENABLED, 0);
        } catch (Exception ex) {
            Log.e(TAG, "setEthernetMode error!");
        }
        return mTracker.stopPppoe(iface);
    }

    public boolean isPppoeEnabled() {
        final ContentResolver cr = mContext.getContentResolver();
        try {
            return Settings.Global.getInt(cr, Settings.Global.PPPOE_ENABLED, 0) == 1;
        } catch (Exception ex) {
            Log.e(TAG, "getEthernetMode error!");
            return false;
        }
    }

    public List<String> getPppoeUserInfo(String iface) {
        String file_path;
        FileReader in;
        List<String> list = new ArrayList<String>();

        if (TextUtils.isEmpty(iface)) {
            file_path = PPPOE_CONFIG_FILE;
        } else {
            file_path = String.format("%s-%s", PPPOE_CONFIG_FILE, iface);
        }
        File file = new File(file_path);
        if (!file.exists()) {
            Log.w(TAG, "pppoe login file not exist!");
            return null;
        }

        try {
            in = new FileReader(file);
            BufferedReader bufferReader = new BufferedReader(in);
            String tmp;
            while ((tmp = bufferReader.readLine()) != null) {
                list.add(tmp);
            }
            bufferReader.close();
            in.close();
            return list;
        } catch (IOException e) {
            Log.e(TAG, "get Pppoe user info failed" + e);
            return null;
        }
    }

    public String getPppoeInterfaceName() {
        String file_path;
        String iface = new String();
        FileReader in;

        file_path = PPPOE_INTERFACE_FILE;
        File file = new File(file_path);
        if (!file.exists()) {
            Log.w(TAG, "pppoe interface file not exist!");
            return null;
        }
        try {
            in = new FileReader(file);
            BufferedReader bufferReader = new BufferedReader(in);
            iface = bufferReader.readLine();
            bufferReader.close();
            in.close();
            return iface;
        } catch (IOException e) {
            Log.e(TAG, "get Pppoe interface info failed" + e);
            return null;
        }
    }

    public LinkProperties getPppoelinkProperties() {
        return mTracker.getPppoelinkProperties();
    }

    public void start() {
        Log.i(TAG, "Starting PPPOE service");
        mCM = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        HandlerThread handlerThread = new HandlerThread("PppoeServiceThread");
        handlerThread.start();
        mHandler = new Handler(handlerThread.getLooper());
        mTracker.start(mContext, mHandler);
        mStarted.set(true);
    }

    //Add for android P because of selinux
    private void changeFolderPermission(File dirFile) {
        Set<PosixFilePermission> perms = new HashSet<PosixFilePermission>();
        //for OWNER
        perms.add(PosixFilePermission.OWNER_READ);
        perms.add(PosixFilePermission.OWNER_WRITE);
        perms.add(PosixFilePermission.OWNER_EXECUTE);
        //for GROUP
        perms.add(PosixFilePermission.GROUP_READ);
        perms.add(PosixFilePermission.GROUP_WRITE);
        perms.add(PosixFilePermission.GROUP_EXECUTE);
        //for OTHERS
        perms.add(PosixFilePermission.OTHERS_READ);
        perms.add(PosixFilePermission.OTHERS_WRITE);
        perms.add(PosixFilePermission.OTHERS_EXECUTE);
        try {
            Path path = Paths.get(dirFile.getAbsolutePath());
            Files.setPosixFilePermissions(path, perms);
        } catch (Exception e) {
            Log.e(TAG, "Change folder " + dirFile.getAbsolutePath() + " permission failed.", e);
          }
    }
}

package com.softwinner.TvdFileManager.net;

import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.database.Cursor;
import android.net.LinkAddress;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import java.io.File;
import java.net.MalformedURLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.net.UnknownHostException;
import com.softwinner.TvdFileManager.R;
import com.softwinner.TvdFileManager.R.style;
import com.softwinner.SystemMix;

import jcifs.netbios.NbtAddress;
import jcifs.smb.NtStatus;
import jcifs.smb.NtlmPasswordAuthentication;
import jcifs.smb.SmbException;
import jcifs.smb.SmbFile;
import jcifs.smb.SmbFileFilter;
import jcifs.smb.WinError;
import jcifs.util.LocalNetwork;

public class SambaManager {
    private static final int SMB_PATH_COLUME = 0;
    private static final int DOMAIN_COLUME = 1;
    private static final int USERNAME_COLUME = 2;
    private static final int PASSWORD_COLUME = 3;

    private Context mContext;
    private ArrayList<SmbFile> mWorkgroupList = null;
    private ArrayList<SmbFile> mServiceList = null;
    private ArrayList<SmbFile> mShareList = null;
    private static ArrayList<String> mMountedPointList = new ArrayList<String>();
    private static HashMap<String, String> mMap = new HashMap<String, String>();

    private SmbFile[] mSmbList = null;
    private SmbLoginDB mLoginDB = null;
    private static File mountRoot = null;
    private ProgressDialog mPrDialog;
    private static LinkAddress mLinkAddress = null;

    private static final boolean V_TAG = true;
    private static final String TAG = "SambaManager";

    public SambaManager(Context context) {
        mContext = context;
        mWorkgroupList = new ArrayList<SmbFile>();
        mServiceList = new ArrayList<SmbFile>();
        mShareList = new ArrayList<SmbFile>();
        mLoginDB = new SmbLoginDB(context);
        mountRoot = new File("/mnt/Samba");;

        initSambaProp();
    }

    private void initSambaProp() {
        jcifs.Config.setProperty("jcifs.encoding", "GBK");
        jcifs.Config.setProperty("jcifs.util.loglevel", "0");
        LocalNetwork.setCurIpv4("172.16.10.66", 16);
    }

    /* Search neighborhood, return results by callback */
    public void startSearch(final String smbUrl, final OnSearchListener ls) {
        Log.d(TAG, "search smb:" + smbUrl);
        // use replaceAll to match the some incorrect input
        String tmpSmbUrl = smbUrl.replaceAll("\t|\n|\r","");
        /* Search wait dialog */
        mPrDialog = showProgressDialog(R.drawable.icon,
                mContext.getResources().getString(R.string.search), null,
                ProgressDialog.STYLE_SPINNER, true);
        mPrDialog.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                ls.onFinish(false);
            }
        });
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                SmbFile smbFile = null;
                try {
                    if (tmpSmbUrl.equals("smb://")) {
                        /* Search all workgroup */
                        smbFile = new SmbFile("smb://");

                    } else if (isSambaWorkgroup(tmpSmbUrl)) {
                        /* Search all server */
                        smbFile = getSambaWorkgroup(tmpSmbUrl);
                    } else if (isSambaServices(tmpSmbUrl)) {
                        /* Search all shared folder */
                        smbFile = getSambaService(tmpSmbUrl);
                    } else if (isSambaShare(tmpSmbUrl)) {
                        /* mount shared folder */
                        smbFile = getSambaShare(tmpSmbUrl);
                    } else {
                        smbFile = new SmbFile(tmpSmbUrl);
                    }
                    boolean ret = startLogin(smbFile, ls);

                    final SmbFile f = smbFile;
                    mPrDialog.cancel();
                    if (!ret) {
                        ((Activity) mContext).runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                createLoginDialog(f, ls);
                            }
                        });

                    }
                } catch (MalformedURLException e) {
                    mSmbList = null;
                    mPrDialog.cancel();
                    Log.d(TAG, e.getMessage());
                    e.printStackTrace();
                }
            }
        });
        thread.start();
    }

    private ProgressDialog showProgressDialog(int icon, String title, String message, int style,
            final boolean cancelable) {
        ProgressDialog prDialog = null;
        prDialog = new ProgressDialog(mContext);
        prDialog.setProgressStyle(style);
        prDialog.setIcon(icon);
        prDialog.setTitle(title);
        prDialog.setIndeterminate(false);
        prDialog.setCancelable(cancelable);
        if (message != null) {
            prDialog.setMessage(message);
        }
        prDialog.show();
        prDialog.getWindow().setLayout(800, 480);
        return prDialog;
    }

    private int addFileList(SmbFile samba, final ArrayList<SmbFile> list,
                                   final OnSearchListener ls) {
        Log.d(TAG, "start search " + samba.getPath());
        SmbFileFilter filter = new SmbFileFilter() {
            @Override
            public boolean accept(SmbFile file) throws SmbException {
                if (mPrDialog.isShowing()) {
                    if (!file.getPath().endsWith("$") && !file.getPath().endsWith("$/")) {
                        list.add(file);
                        Log.d(TAG, "child:" + file.getPath());
                        Log.d(TAG, file.getURL().getHost().toString());
                        ls.onReceiver(file.getPath());
                        return true;
                    }
                }
                return false;
            }
        };
        int netStatus = NtStatus.NT_STATUS_UNSUCCESSFUL;
        try {
            SmbFile[] smbList = samba.listFiles(filter);
            if (smbList != null) {
                netStatus = NtStatus.NT_STATUS_OK;
            } else {
                netStatus = NtStatus.NT_STATUS_UNSUCCESSFUL;
            }
        } catch (SmbException e) {
            netStatus = e.getNtStatus();
            e.printStackTrace();
            Log.d(TAG, "Disconnect in excpetion");
            // samba.doDisconnect();
        }
        Log.d(TAG, String.format("search end, return %x, means net status %s, win status %s",
                netStatus, statusToStr(netStatus), SmbException.getMessageByWinerrCode(netStatus)));
        return netStatus;
    }

    private String statusToStr(int statusCode) {
        for (int i = 0; i < NtStatus.NT_STATUS_CODES.length; i++) {
            if (statusCode == NtStatus.NT_STATUS_CODES[i]) {
                return NtStatus.NT_STATUS_MESSAGES[i];
            }
        }
        return null;
    }

    private boolean login(SmbFile samba, final ArrayList<SmbFile> list, final OnSearchListener ls) {

        NtlmPasswordAuthentication ntlm = getLoginDataFromDB(samba);
        String path = samba.getPath();
        if (ntlm != null) {
            try {
                samba = new SmbFile(path, ntlm);
            } catch (MalformedURLException e) {
                // TODO Auto-generated catch block
                Log.e(TAG, e.getMessage());
                e.printStackTrace();
                return false;
            }
            Log.d("Samba",
                    String.format("ntlm domain=%s, usr=%s, psw=%s", ntlm.getDomain(),
                            ntlm.getUsername(), ntlm.getPassword()));
        }
        int ret = addFileList(samba, list, ls);
        if(list.isEmpty())
            showResultMessage();
        switch (ret) {
            case NtStatus.NT_STATUS_OK:
                ls.onFinish(true);
                return true;
            case NtStatus.NT_STATUS_ACCESS_DENIED:
            case WinError.ERROR_ACCESS_DENIED:
                mLoginDB.delete(path);
                return false;
            default:
                Log.e(TAG, String.format("other error code: %x", ret));
                if (!ls.onFinish(false)) {
                    showMessage(R.string.access_fail);
                }
        }
        return true;
    }

    private void showMessage(final int resId) {
        ((Activity) mContext).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    Toast.makeText(mContext, resId, Toast.LENGTH_SHORT).show();
                } catch (Exception e) {
                }
            }
        });
    }
    private void showResultMessage(){
        showMessage(R.string.result_null);
    }

    /**
     *
     * @param samba
     * @param ls
     * @return true: login success false: login failed
     */
    public boolean startLogin(SmbFile samba, final OnSearchListener ls) {
        NtlmPasswordAuthentication ntlm = null;
        // fix me:when create samba with url 'smb://', the value of
        // samba.getPath() is 'smb:////'
        if ("smb:////".equals(samba.getPath())) {
            // first clear all cached info
            mServiceList.clear();
            mWorkgroupList.clear();
            return login(samba, mWorkgroupList, ls);
        }
        int type;
        try {
            type = samba.getType();
        } catch (SmbException e1) {
            // TODO Auto-generated catch block
            Log.e(TAG, e1.getMessage());
            e1.printStackTrace();
            return false;
        }
        Log.d(TAG, "getType " + type);
        try {
            switch (type) {
                case SmbFile.TYPE_WORKGROUP:
                    mServiceList.clear();
                    return login(samba, mServiceList, ls);
                case SmbFile.TYPE_SERVER:
                    mShareList.clear();
                    return login(samba, mShareList, ls);
                case SmbFile.TYPE_SHARE:
                    /* If have logged in, go into mounted point folder directly */
                    String mountPoint = getSambaMountedPoint(samba.getPath());
                    if (mountPoint != null) {
                        ls.onReceiver(mountPoint);
                        return true;
                    }
                    /*
                     * if have not logged in, first try login in without
                     * username and password.
                     */
                    String serverName = samba.getServer();
                    Log.d(TAG, "server name of " + samba.getPath() + " is " + serverName);
                    NbtAddress addr = NbtAddress.getByName(serverName);
                    Log.d(TAG, "nbt address is " + addr.getHostAddress());
                    mountPoint = createNewMountedPoint(samba.getPath().substring(0,
                            samba.getPath().length() - 1));
                    umountSmb(mountPoint);
                    int ret = mountSmb(samba.getPath(), mountPoint, "", "", addr.getHostAddress());
                    if (ret == 0) {
                        mMountedPointList.add(mountPoint);
                        mMap.put(samba.getPath(), mountPoint);
                        ls.onReceiver(mountPoint);
                        return true;
                    } else {
                        String path = samba.getPath();
                        ntlm = getLoginDataFromDB(samba);
                        if (ntlm != null) {
                            samba = new SmbFile(path, ntlm);
                        } else {
                            String pPath = samba.getParent();
                            SmbFile parent;
                            parent = new SmbFile(pPath,
                                    (NtlmPasswordAuthentication) samba.getPrincipal());
                            ntlm = getLoginDataFromDB(parent);
                            if (ntlm != null) {
                                // or use username and password of its's server
                                samba = new SmbFile(path, ntlm);
                            } else {
                                return false;
                            }
                        }
                        umountSmb(mountPoint);
                        ret = mountSmb(samba.getPath(), mountPoint, ntlm.getUsername(),
                                ntlm.getPassword(), addr.getHostAddress());
                        if (ret == 0) {
                            mMountedPointList.add(mountPoint);
                            mMap.put(samba.getPath(), mountPoint);
                            ls.onReceiver(mountPoint);
                            return true;
                        } else {
                            mLoginDB.delete(path);
                            return false;
                        }
                    }
                default:
                    break;
            }
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
        return false;
    }

    private NtlmPasswordAuthentication getLoginDataFromDB(SmbFile file) {
        if (file == null) {
            return null;
        }

        String[] columns = null;
        String selection = SmbLoginDB.SMB_PATH + "=?";
        String smbPath = file.getPath();
        smbPath = smbPath.endsWith("/") ? smbPath.substring(0, smbPath.length() - 1) : smbPath;
        String[] selectionArgs = {smbPath};
        String domain = null;
        String username = null;
        String password = null;
        NtlmPasswordAuthentication ntlm = null;
        Cursor cr = mLoginDB.query(columns, selection, selectionArgs, null);
        if (cr != null) {
            try {
                while (cr.moveToNext()) {
                    domain = cr.getString(DOMAIN_COLUME);
                    Log.d(TAG, "------------------get ntlm ------------------");
                    Log.d(TAG,
                            "fileDom  "
                                    + ((NtlmPasswordAuthentication) file.getPrincipal())
                                            .getDomain());
                    Log.d(TAG, "path     " + cr.getString(SMB_PATH_COLUME));
                    Log.d(TAG, "domain   " + domain);
                    Log.d(TAG, "username " + cr.getString(USERNAME_COLUME));
                    Log.d(TAG, "password " + cr.getString(PASSWORD_COLUME));
                    Log.d(TAG, "------------------end ntlm ------------------");
                    if (domain != null
                            && domain.equals(((NtlmPasswordAuthentication) file.getPrincipal())
                                    .getDomain())) {
                        username = cr.getString(USERNAME_COLUME);
                        password = cr.getString(PASSWORD_COLUME);
                        ntlm = new NtlmPasswordAuthentication(domain, username, password);
                        break;
                    }
                }
            } finally {
                cr.close();
                cr = null;
            }
        }
        return ntlm;
    }

    private void createLoginDialog(final SmbFile samba, final OnSearchListener ls) {
        final Dialog dg = new Dialog(mContext, R.style.menu_dialog);
        dg.setCancelable(true);
        LayoutInflater infrater = LayoutInflater.from(mContext);
        View v = infrater.inflate(R.layout.login_dialog, null);
        dg.setContentView(v);
        final EditText account = (EditText) v.findViewById(R.id.account);
        final EditText password = (EditText) v.findViewById(R.id.password);
        Button ok = (Button) v.findViewById(R.id.login_ok);
        ok.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                dg.dismiss();
                /* Starting login */
                final ProgressDialog pdg = showProgressDialog(R.drawable.icon, mContext
                        .getResources().getString(R.string.login), null,
                        ProgressDialog.STYLE_SPINNER, true);
                Thread thread = new Thread(new Runnable() {

                    @Override
                    public void run() {
                        try {
                            // TODO Auto-generated method stub
                            String inputStr = account.getEditableText().toString();
                            String domain = null;
                            String user = null;
                            int i = inputStr.indexOf("\\");
                            if (i > 0) {
                                domain = inputStr.substring(0, i);
                                user = inputStr.substring(i + 1, inputStr.length());
                            } else {
                                domain = ((NtlmPasswordAuthentication) samba.getPrincipal())
                                        .getDomain();
                                user = inputStr;
                            }
                            NtlmPasswordAuthentication ntlm = new NtlmPasswordAuthentication(
                                    domain, user, password.getEditableText().toString());
                            SmbFile smbfile;
                            Log.d(TAG, String.format(
                                    "create dialog: ntlm domain=%s, usr=%s, psw=%s",
                                    ntlm.getDomain(), ntlm.getUsername(), ntlm.getPassword()));
                            smbfile = new SmbFile(samba.getPath(), ntlm);
                            int type = samba.getType();
                            switch (type) {
                            /**
                             * if the type is Samba Server, then list all shared
                             * folders
                             */
                                case SmbFile.TYPE_SERVER:
                                    SmbFile[] shareList = smbfile.listFiles();
                                    if (shareList == null) {
                                        showMessage(R.string.login_fail);
                                    } else {
                                        String smbPath = smbfile.getPath();
                                        smbPath = smbPath.endsWith("/") ? smbPath.substring(0,
                                                smbPath.length() - 1) : smbPath;
                                        addLoginMessage(ntlm, smbPath);
                                        mShareList.clear();
                                        for (SmbFile file : shareList) {
                                            if (!file.getPath().endsWith("$")) {
                                                mShareList.add(file);
                                                ls.onReceiver(file.getPath());
                                            }
                                        }
                                        ls.onFinish(true);
                                    }
                                    break;
                                /**
                                 * If the type is Samba shared folder, then
                                 * mount it and go into the directory
                                 */
                                case SmbFile.TYPE_SHARE:
                                    // for test
                                    String smbPath = samba.getPath();
                                    smbPath = smbPath.substring(0, smbPath.length() - 1);
                                    String mountedPoint = createNewMountedPoint(smbPath);
                                    String serverName = samba.getServer();
                                    Log.d(TAG, "server name of " + samba.getPath() + " is "
                                            + serverName);
                                    NbtAddress addr = NbtAddress.getByName(serverName);
                                    Log.d(TAG, "nbt address is " + addr.getHostAddress());
                                    umountSmb(mountedPoint);
                                    int success = mountSmb(smbPath, mountedPoint,
                                            ntlm.getUsername(), ntlm.getPassword(),
                                            addr.getHostAddress());
                                    /* Mount success,list all sub file */
                                    if (success == 0) {
                                        addLoginMessage(ntlm, smbPath);
                                        mMountedPointList.add(mountedPoint);
                                        mMap.put(samba.getPath(), mountedPoint);
                                        ls.onReceiver(mountedPoint);
                                        ls.onFinish(true);
                                    } else {
                                        showMessage(R.string.login_fail);
                                    }
                                    break;
                                default:
                                    break;
                            }
                            pdg.dismiss();
                        } catch (Exception e) {
                            e.printStackTrace();
                            pdg.dismiss();
                        }
                    }

                });
                thread.start();
            }
        });
        Button cancel = (Button) v.findViewById(R.id.login_cancel);
        cancel.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                dg.dismiss();
            }
        });

        Window dialogWindow = dg.getWindow();
        WindowManager.LayoutParams lp = dialogWindow.getAttributes();
        lp.width = 600;
        dialogWindow.setAttributes(lp);
        dg.show();
    }

    /**
     * Add login information to database
     *
     * @param ntlm
     * @param smbPath
     */
    public void addLoginMessage(NtlmPasswordAuthentication ntlm, String smbPath) {
        Log.d(TAG, "-------------add login message------------");
        Log.d(TAG, "path     " + smbPath);
        Log.d(TAG, "domain   " + ntlm.getDomain());
        Log.d(TAG, "username " + ntlm.getUsername());
        Log.d(TAG, "password " + ntlm.getPassword());
        Log.d(TAG, "-------------end adding-------------------");
        mLoginDB.insert(smbPath, ntlm.getDomain(), ntlm.getUsername(), ntlm.getPassword());
    }

    /**
     * Umount all mounted point
     */
    public void clear() {
        for (String mountPoint : mMountedPointList) {
            umountSmb(mountPoint);
        }
        mMountedPointList.clear();
        mLoginDB.closeDB();
    }

    public boolean isSambaServices(String path) {
        String[] attrs = path.split("\\|");
        // if path format is "smb|192.168.1.105"
        if (attrs.length == 2 && attrs[0].equals("smb")) {
            path = attrs[1];
            SmbFile file = null;
            boolean found = false;
            try {
                if (LocalNetwork.getIpType(path) != LocalNetwork.IP_TYPE_UNKNOWN) {
                    path = "smb://" + path + "/";
                }
                for (int i = 0; i < mServiceList.size(); i++) {
                    if (mServiceList.get(i).getPath().equals(path)) {
                        found = true;
                        return true;
                    }
                }
                if (!found) {
                    file = new SmbFile(path);
                    mServiceList.add(file);
                    return true;
                }
            } catch (MalformedURLException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
        if (LocalNetwork.getIpType(path) != LocalNetwork.IP_TYPE_UNKNOWN) {
            path = "smb://" + path + "/";
        }
        for (int i = 0; i < mServiceList.size(); i++) {
            if (mServiceList.get(i).getPath().equals(path)) {
                return true;
            }
        }
        return false;
    }

    public boolean isSambaShare(String path) {
        for (int i = 0; i < mShareList.size(); i++) {
            if (mShareList.get(i).getPath().equals(path)) {
                return true;
            }
        }
        return false;
    }

    public boolean isSambaWorkgroup(String path) {
        for (int i = 0; i < mWorkgroupList.size(); i++) {
            if (mWorkgroupList.get(i).getPath().equals(path)) {
                return true;
            }
        }
        return false;
    }

    public void clearSambaList() {
        mWorkgroupList.clear();
        mServiceList.clear();
        mShareList.clear();
    }

    public boolean isSambaMountDir(String path) {
        return path.startsWith(mountRoot.getAbsolutePath() + "/smb");
    }

    public SmbFile getSambaWorkgroup(String sambaFile) {
        for (int i = 0; i < mWorkgroupList.size(); i++) {
            SmbFile file = mWorkgroupList.get(i);
            if (file.getPath().equals(sambaFile)) {
                return file;
            }
        }
        return null;
    }

    public SmbFile getSambaService(String sambaFile) {
        String[] attrs = sambaFile.split("\\|");
        if (attrs.length == 2 && attrs[0].equals("smb")) {
            sambaFile = attrs[1];
        }
        if (LocalNetwork.getIpType(sambaFile) != LocalNetwork.IP_TYPE_UNKNOWN) {
            sambaFile = "smb://" + sambaFile + "/";
        }
        for (int i = 0; i < mServiceList.size(); i++) {
            SmbFile file = mServiceList.get(i);
            if (file.getPath().equals(sambaFile)) {
                return file;
            }
        }
        return null;
    }

    public SmbFile getSambaShare(String sambaFile) {
        for (int i = 0; i < mShareList.size(); i++) {
            SmbFile file = mShareList.get(i);
            if (file.getPath().equals(sambaFile)) {
                return file;
            }
        }
        return null;
    }

    public ArrayList<SmbFile> getAllSmbWorkgroup() {
        return (ArrayList<SmbFile>) mWorkgroupList.clone();
    }

    public ArrayList<SmbFile> getAllSmbServices() {
        return (ArrayList<SmbFile>) mServiceList.clone();
    }

    /**
     * Get current all shared folder
     *
     * @return
     */
    public ArrayList<SmbFile> getAllSmbShared() {
        return (ArrayList<SmbFile>) mShareList.clone();
    }

    /**
     * Create smb mounted point on local disk
     *
     * @param path
     * @return
     */
    public static String createNewMountedPoint(String path) {
        String mountedPoint = null;

        /* for test */
        path = path.replaceFirst("smb://", "smb_");
        path = path.replace("/", "_");
        mountedPoint = path;
        File file = new File(mountRoot, mountedPoint);
        Log.d(TAG, "mounted create:  " + file.getPath());
        if (!file.exists()) {
            try {
                file.mkdir();
            } catch (Exception e) {
                Log.e(TAG, "create " + mountedPoint + " fail");
                e.printStackTrace();
            }
        }
        return file.getAbsolutePath();
    }

    private String getSambaMountedPoint(String samba) {
        String mountedPoint = null;
        mountedPoint = mMap.get(samba);
        return mountedPoint;
    }

    public boolean isSambaMountedPoint(String mountedPoint) {
        Log.d(TAG, "mountedPoint   " + mountedPoint);
        if (mMountedPointList.size() == 0) {
            Log.d(TAG, "list is 0");
        }
        for (String item : mMountedPointList) {
            Log.d(TAG, "list.....  " + item);
            if (item.equals(mountedPoint)) {
                return true;
            }
        }
        return false;
    }

    public interface OnLoginFinishListenner {
        void onLoginFinish(String mountedPoint);
    }

    /**
     * Mount smb shared folder on local disk
     *
     * @param source
     * @param target
     * @param username
     * @param password
     * @param ip
     * @return
     */
    private int mountSmb(String source, String target, String username,
                               String password, String ip) {
        if (source.endsWith("/")) {
            source = source.substring(0, source.length() - 1);
        }
        int begin = source.lastIndexOf("smb:") + "smb:".length();
        String src = source.substring(begin, source.length());
        String mountPoint = target;
        String fs = "cifs";
        int flag = 64;
        String sharedName = src.substring(src.lastIndexOf("/") + 1, src.length());
        String server = src.substring(0, src.lastIndexOf("/"));
        String unc = String.format("%s\\%s", server, sharedName);
        String ver = "1";

        String options = String.format("unc=%s,ver=%s,user=%s,pass=%s,ip=%s,iocharset=%s,sec=%s",
                unc, ver, username, password, ip, "utf8", "ntlmssp");
        int ret = SystemMix.mount(src, mountPoint, fs, flag, options);
        Log.d(TAG, "------------------------");
        Log.d(TAG, "src            " + src);
        Log.d(TAG, "mountPoint     " + mountPoint);
        Log.d(TAG, "fs             " + fs);
        Log.d(TAG, "flag           " + flag);
        Log.d(TAG, "options        " + options);
        Log.d(TAG, "ret            " + ret);
        Log.d(TAG, "------------------------");
        return ret;
    }

    /**
     * Umount smb shared folder from local disk
     *
     * @param target
     * @return
     */
    public int umountSmb(String target) {
        int ret = SystemMix.umount(target);
        if (ret == 0) {
            for (int i = 0; i < mMountedPointList.size(); i++) {
                if (mMountedPointList.get(i).equals(target)) {
                    mMountedPointList.remove(i);
                    break;
                }
            }
            Iterator it = mMap.keySet().iterator();
            while (it.hasNext()) {
                Object key = it.next();
                if (mMap.get(key).equals(target)) {
                    mMap.remove(key);
                    break;
                }
            }
        }
        Log.d(TAG, "Umount:" + target + " ret:" + ret);
        return ret;
    }
}

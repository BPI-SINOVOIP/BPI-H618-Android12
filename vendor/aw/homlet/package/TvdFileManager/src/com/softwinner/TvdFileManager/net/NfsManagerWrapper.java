package com.softwinner.TvdFileManager.net;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.util.ArrayList;

import com.softwinner.TvdFileManager.R;
import com.softwinner.tmp.nfs.NFSFolder;
import com.softwinner.tmp.nfs.NFSServer;
import com.softwinner.tmp.nfs.NfsManager;

public class NfsManagerWrapper {
    private static final String TAG = "NFS_ADAPTER";
    private static final boolean V_BUG = true;

    // Mount point root path exam:
    // /data/data/com.softwinner.TvdFileManager/app_share
    private static File mountRoot = null;
    private Context mContext = null;
    private NfsManager nm;
    private boolean manualCancel = false;
    // When running in background, Show this widget.
    private ProgressDialog mPrDialog = null;

    // NFSServer object, contain server ip address, exam: 192.168.1.1
    private ArrayList<NFSServer> mServerList = new ArrayList<NFSServer>();

    public static final int TYPE_SERVER = 0x01;
    public static final int TYPE_FOLDER = 0x02;
    public static final String NFS_SPLIT = "|";
    public static final String NFS_MARK = "nfs";

    public NfsManagerWrapper(Context context) {
        mContext = context;
        nm = NfsManager.getInstance(context);
        mServerList = new ArrayList<NFSServer>();
        mountRoot = new File("/mnt/Nfs");; // obtain the mount root
    }

    /**
     * Clear all mounted point from local disk
     */
    public void clear() {
        ArrayList<NFSFolder> fList = null;
        for (NFSServer s : mServerList) {
            fList = s.getFolderList();
            for (int i = 0; i < fList.size(); i++) {
                if (fList.get(i).isMounted()) {
                    nm.nfsUnmount(fList.get(i).getMountedPoint());
                    fList.get(i).setMounted(false);
                    if (new File(fList.get(i).getMountedPoint()).delete()) {
                        if (V_BUG) {
                            Log.d(TAG, "Delete directory " + fList.get(i) + " OK!!");
                        }
                    } else {
                        if (V_BUG) {
                            Log.d(TAG, "Delete directory " + fList.get(i) + " Fail!!");
                        }
                    }
                }
            }
        }
    }

    /**
     * create mount point folder for SystemMix.Mount function.
     *
     * @param path
     *            name for creating directory. exam:
     *            192.168.1.105|/home/wanran/share
     * @return the full path of the created folder. exam:
     *         /data/data/com.softwinner
     *         .TvdFileManager/app_share/nfs_home_wanran_share
     */
    public static String createNewMountedPoint(String path) {
        String mountedPoint = null;
        path = path.replace("/", "_"); // result:
                                       // 192.168.1.105|/home/wanran/share
                                       // ----->
                                       // 192.168.1.105|_home_wanran_share
        path = path.replace(".", "_"); // result:
                                       // 192.168.1.105|_home_wanran_share
                                       // ----->
                                       // 192_168_1_105|_home_wanran_share
        path = path.replace("|", "_"); // result:
                                       // 192_168_1_105|_home_wanran_share
                                       // ----->
                                       // 192_168_1_105__home_wanran_share
        path = "nfs_" + path;
        mountedPoint = path;
        File file = new File(mountRoot, mountedPoint);

        if (!file.exists()) {
            try {
                file.mkdir();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        if (V_BUG) {
            Log.d(TAG, "Create mounted point: " + file.getAbsolutePath());
        }
        return file.getAbsolutePath();
    }

    /**
     * Obtain current all nfs server list
     *
     * @return ArrayList<NFSServer>: nfs server list
     */
    public ArrayList<NFSServer> getAllNfsServers() {
        return (ArrayList<NFSServer>) mServerList.clone();
    }

    /**
     * If current folder is mounted point, back to up folder operation need
     * fetch the server by the mounted point.
     *
     * @param mountedPoint
     *            : such as
     *    "/data/data/com.softwinner.TvdFileManager/app_share/nfs_nfs_192_168_1_103__d_sharenfs"
     * @return NFSServer contain the mounted point
     */
    public NFSServer getServerByMountedPoint(String mountedPoint) {
        for (NFSServer s : mServerList) {
            for (NFSFolder f : s.getFolderList()) {
                if (f.isMounted() && f.getMountedPoint().equals(mountedPoint)) {
                    Log.d(TAG, "Mounted Point " + mountedPoint + " mapping " + s.getServerIP());
                    return s;
                }
            }
        }
        return null;
    }

    /**
     * judge the mount point whether a nfs mount point such as
     * "/data/data/com.softwinner.TvdFileManager/app_share/nfs_192_168_1_105__home_wanran_share"
     *
     * @param mountedPoint
     * @return true: the mount point is belong to nfs false: the mount point is
     *         not belong to nfs
     */
    public boolean isNfsMountedPoint(String mountedPoint) {
        for (NFSServer s : mServerList) {
            for (NFSFolder f : s.getFolderList()) {
                if (f.isMounted() && f.getMountedPoint().equals(mountedPoint)) {
                    if (V_BUG) {
                        Log.d(TAG, mountedPoint + " is NFS mounted point");
                    }
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Judge selected item whether a nfs server, such as "nfs|192.168.1.105"
     *
     * @param filePath
     * @return true: path is a nfs server ip false: path is not a nfs server ip
     */
    public boolean isNfsServer(String filePath) {
        String[] attrs = filePath.split("\\" + NFS_SPLIT);
        if (attrs.length != 2 || !attrs[0].equals(NFS_MARK)) {
            return false;
        }
        for (NFSServer server : mServerList) {
            if (server.getServerIP().equals(attrs[1])) {
                if (V_BUG) {
                    Log.d(TAG, filePath + " is NFS server(auto search or allready added).");
                }
                return true;
            }
        }
        /**
         * Manual added NFS server not exits in mServerList, need add it in.
         */
        NFSServer s = new NFSServer();
        s.setServerIP(attrs[1]);
        mServerList.add(s);
        if (V_BUG) {
            Log.d(TAG, filePath + " is NFS server(Manual add)");
        }
        return true;
    }

    /**
     * Judge input string is nfs shared folder.
     *
     * @param filePath
     *            : the shared folder path, such as
     *            "nfs|192.168.1.105|/home/wanran/share"
     * @return true : the input string is one of nfs shared folder. false: the
     *         input string not a nfs shared folder.
     */
    public boolean isNfsShare(String filePath) {
        String[] attrs = filePath.split("\\" + NFS_SPLIT);
        if (attrs.length != 3) {
            return false;
        }
        for (NFSServer s : mServerList) {
            if (s.getServerIP().equals(attrs[1])) {
                for (NFSFolder f : s.getFolderList()) {
                    if (f.getFolderPath().equals(attrs[2])) {
                        if (V_BUG) {
                            Log.d(TAG, filePath + " is NFS share folder.");
                         }
                        return true;
                    }
                }
            }
        }
        return false;
    }

    private void addServers(ArrayList<NFSServer> list) {
        boolean found = false;
        ArrayList<NFSServer> tmpList = new ArrayList<NFSServer>();
        for (NFSServer s1 : list) {
            found = false;
            for (NFSServer s2 : mServerList) {
                if (s2.getServerIP().equals(s1.getServerIP())) {
                    found = true;
                    tmpList.add(s2);
                    break;
                }
            }
            if (!found) {
                tmpList.add(s1);
            }
        }
        mServerList = tmpList;
    }

    /**
     * When user select a item and click it, the EventHandler will call this
     * function, the func is the core.
     *
     * @param nfsUrl
     *            : current directory name(From item's datasource) exam:
     * @param ls
     *            : Callback object
     */
    public void startSearch(final String nfsUrl, final OnSearchListener ls) {
        if (V_BUG) {
            Log.d(TAG, "NFS Search: " + nfsUrl);
        }
        manualCancel = false;

        mPrDialog = showProgressDialog(R.drawable.icon,
                mContext.getResources().getString(R.string.search), null,
                ProgressDialog.STYLE_SPINNER, true);
        mPrDialog.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                // ls.onFinish(false);
                manualCancel = true;
            }
        });

        Thread thd = new Thread(new Runnable() {
            @Override
            public void run() {
                /*
                 * If request is "NFS Share", then start search nfs server on
                 * lan.
                 */
                if (nfsUrl.equals(mContext.getResources().getString(R.string.nfs))) {
                    addServers(nm.getServers());
                    if (manualCancel) {
                        ls.onFinish(false);
                        return;
                    }

                    /*
                     * If no server in this lan, not refresh UI.
                     */
                    if (mServerList.size() <= 0) {
                        showResultMessage();
                        ls.onFinish(false);
                    } else {
                        for (NFSServer server : mServerList) {
                            ls.onReceiver("nfs|" + server.getServerIP());
                        }
                        ls.onFinish(true);
                    }

                } else if (isNfsServer(nfsUrl)) {
                    String[] attrs = nfsUrl.split("\\" + NFS_SPLIT);
                    ArrayList<NFSFolder> fList = null;
                    for (NFSServer server : mServerList) {
                        if (server.getServerIP().equals(attrs[1])) {
                            fList = nm.getSharedFolders(server);
                            for (int i = 0; i < fList.size(); i++) {
                                for (NFSFolder f : server.getFolderList()) {
                                    if (f.getFolderPath().equals(fList.get(i).getFolderPath())) {
                                        fList.set(i, f);
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }
                    /*
                     * If the server not shared any folder, not refresh UI.
                     */
                    if ((fList != null) && (fList.size() > 0)) {
                        if (manualCancel) {
                            ls.onFinish(false);
                            return;
                        }

                        for (NFSFolder folder : fList) {
                            if (V_BUG) {
                                Log.d(TAG,
                                        "Server " + attrs[1] + " Shared folder: "
                                                + folder.getFolderPath());
                            }
                            ls.onReceiver(nfsUrl + NFS_SPLIT + folder.getFolderPath());
                        }
                        ls.onFinish(true);
                    } else {
                        ls.onFinish(false);
                    }
                    mPrDialog.cancel();

                } else if (isNfsShare(nfsUrl)) {
                    if (manualCancel) {
                        ls.onFinish(false);
                        return;
                    }
                    /*
                     * attrs[0] = "nfs" attrs[1]="192.168.1.105"
                     * attrs[2]="/home/wanran/share"
                     */
                    String[] attrs = nfsUrl.split("\\" + NFS_SPLIT);

                    for (NFSServer server : mServerList) {
                        if (server.getServerIP().equals(attrs[1])) {
                            for (NFSFolder folder : server.getFolderList()) {
                                if (folder.getFolderPath().equals(attrs[2])) {
                                    if (folder.isMounted()) {
                                        if (V_BUG) {
                                            Log.d(TAG, "nfsUrl" + " allready mounted.");
                                        }
				        nm.nfsUnmount(folder.getMountedPoint());
                                        nm.nfsMount(attrs[2], folder.getMountedPoint(),
                                                server.getServerIP());
                                        ls.onReceiver(folder.getMountedPoint());
                                        ls.onFinish(true);
                                        break;
                                    } else {
                                        if (V_BUG) {
                                            Log.d(TAG, "nfsUrl" + " not mounted");
                                        }
                                        // pass the original path, exam:
                                        // 192.168.1.105|/home/wanran/share
                                        String mountPoint = createNewMountedPoint(nfsUrl);
                                        nm.nfsUnmount(mountPoint); // first try
                                                                   // to umount
                                                                   // the point
                                        boolean success = nm.nfsMount(attrs[2], mountPoint,
                                                server.getServerIP());

                                        if (success) {
                                            if (V_BUG) {
                                                Log.d(TAG, "Mount folder " + nfsUrl + " success.");
                                            }
                                            folder.setMounted(true);
                                            folder.setMountedPoint(mountPoint);
                                            if (manualCancel) {
                                                break;
                                            }
                                            ls.onReceiver(mountPoint);
                                            ls.onFinish(true);
                                            break;
                                        } else {
                                            if (V_BUG) {
                                                Log.d(TAG, "Mount folder " + nfsUrl + " failed.");
                                            }
                                            showMessage(R.string.mount_fail);
                                            ls.onFinish(false);
                                            break;
                                        }
                                    }
                                }
                            } // end for
                            break;
                        } // end if
                    } // end for
                }
                mPrDialog.cancel();
            }

        });
        thd.start();

    }

    /**
     * Show progress widget
     *
     * @param icon
     * @param title
     * @param message
     * @param style
     * @param cancelable
     * @return
     */
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

    /**
     * Show message from R.String.id
     *
     * @param resId
     */
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
}

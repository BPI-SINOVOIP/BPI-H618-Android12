package com.softwinner.tmp.nfs;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Enumeration;

import com.softwinner.SystemMix;

/**
 *
 * @author Ethan Shan
 *
 */
public final class NfsManager {
    private static final String TAG = "NFS_LIB";
    private static final boolean V_BUG = true;

    private static final int NFS_FLAGS = 32768; // NFS mount flags
    private static final String NFS_OPTS = "nolock,addr="; // NFS mount options
    private static final String NFS_TYPE = "nfs"; // NFS mount type
    private static NfsManager sm = null;
    private Context context = null;
    private ArrayList<NFSServer> servers = null;
    private static final int NFS_PORT = 111; // NFS server listen port

    /* bits in the flags field visible to user space */
    public static final int NFS_MOUNT_SOFT = 0x0001; /* 1 */
    public static final int NFS_MOUNT_INTR = 0x0002; /* 1 *//*
                                                             * now unused, but
                                                             * ABI
                                                             */
    public static final int NFS_MOUNT_SECURE = 0x0004; /* 1 */
    public static final int NFS_MOUNT_POSIX = 0x0008; /* 1 */
    public static final int NFS_MOUNT_NOCTO = 0x0010; /* 1 */
    public static final int NFS_MOUNT_NOAC = 0x0020; /* 1 */
    public static final int NFS_MOUNT_TCP = 0x0040; /* 2 */
    public static final int NFS_MOUNT_VER3 = 0x0080; /* 3 */
    public static final int NFS_MOUNT_KERBEROS = 0x0100; /* 3 */
    public static final int NFS_MOUNT_NONLM = 0x0200; /* 3 */
    public static final int NFS_MOUNT_BROKEN_SUID = 0x0400; /* 4 */
    public static final int NFS_MOUNT_NOACL = 0x0800; /* 4 */
    public static final int NFS_MOUNT_STRICTLOCK = 0x1000; /* reserved for NFSv4 */
    public static final int NFS_MOUNT_SECFLAVOUR = 0x2000; /* 5 */
    public static final int NFS_MOUNT_NORDIRPLUS = 0x4000; /* 5 */
    public static final int NFS_MOUNT_UNSHARED = 0x8000; /* 5 */
    public static final int NFS_MOUNT_FLAGMASK = 0xFFFF;

    /*
     * These are the fs-independent mount-flags: up to 32 flags are supported
     */
    public static final int MS_RDONLY = 1; /* Mount read-only */
    public static final int MS_NOSUID = 2; /* Ignore suid and sgid bits */
    public static final int MS_NODEV = 4; /*
                                           * Disallow access to device special
                                           * files
                                           */
    public static final int MS_NOEXEC = 8; /* Disallow program execution */
    public static final int MS_SYNCHRONOUS = 16; /* Writes are synced at once */
    public static final int MS_REMOUNT = 32; /* Alter flags of a mounted FS */
    public static final int MS_MANDLOCK = 64; /* Allow mandatory locks on an FS */
    public static final int MS_DIRSYNC = 128; /*
                                               * Directory modifications are
                                               * synchronous
                                               */
    public static final int MS_NOATIME = 1024; /* Do not update access times. */
    public static final int MS_NODIRATIME = 2048; /*
                                                   * Do not update directory
                                                   * access times
                                                   */
    public static final int MS_BIND = 4096;
    public static final int MS_MOVE = 8192;
    public static final int MS_REC = 16384;
    public static final int MS_VERBOSE = 32768; /*
                                                 * War is peace. Verbosity is
                                                 * silence. MS_VERBOSE is
                                                 * deprecated.
                                                 */
    public static final int MS_SILENT = 32768;
    public static final int MS_POSIXACL = (1 << 16); /*
                                                      * VFS does not apply the
                                                      * umask
                                                      */
    public static final int MS_UNBINDABLE = (1 << 17); /* change to unbindable */
    public static final int MS_PRIVATE = (1 << 18); /* change to private */
    public static final int MS_SLAVE = (1 << 19); /* change to slave */
    public static final int MS_SHARED = (1 << 20); /* change to shared */
    public static final int MS_RELATIME = (1 << 21); /*
                                                      * Update atime relative to
                                                      * mtime/ctime.
                                                      */
    public static final int MS_KERNMOUNT = (1 << 22); /*
                                                       * this is a kern_mount
                                                       * call
                                                       */
    public static final int MS_I_VERSION = (1 << 23); /*
                                                       * Update inode I_version
                                                       * field
                                                       */
    public static final int MS_STRICTATIME = (1 << 24); /*
                                                         * Always perform atime
                                                         * updates
                                                         */
    public static final int MS_NOSEC = (1 << 28);
    public static final int MS_BORN = (1 << 29);
    public static final int MS_ACTIVE = (1 << 30);
    public static final int MS_NOUSER = (1 << 31);

    /* Error Number */
    public static final int EPERM = 1; /* Operation not permitted */
    public static final int ENOENT = 2; /* No such file or directory */
    public static final int ESRCH = 3; /* No such process */
    public static final int EINTR = 4; /* Interrupted system call */
    public static final int EIO = 5; /* I/O error */
    public static final int ENXIO = 6; /* No such device or address */
    public static final int E2BIG = 7; /* Argument list too long */
    public static final int ENOEXEC = 8; /* Exec format error */
    public static final int EBADF = 9; /* Bad file number */
    public static final int ECHILD = 10; /* No child processes */
    public static final int EAGAIN = 11; /* Try again */
    public static final int ENOMEM = 12; /* Out of memory */
    public static final int EACCES = 13; /* Permission denied */
    public static final int EFAULT = 14; /* Bad address */
    public static final int ENOTBLK = 15; /* Block device required */
    public static final int EBUSY = 16; /* Device or resource busy */
    public static final int EEXIST = 17; /* File exists */
    public static final int EXDEV = 18; /* Cross-device link */
    public static final int ENODEV = 19; /* No such device */
    public static final int ENOTDIR = 20; /* Not a directory */
    public static final int EISDIR = 21; /* Is a directory */
    public static final int EINVAL = 22; /* Invalid argument */
    public static final int ENFILE = 23; /* File table overflow */
    public static final int EMFILE = 24; /* Too many open files */
    public static final int ENOTTY = 25; /* Not a typewriter */
    public static final int ETXTBSY = 26; /* Text file busy */
    public static final int EFBIG = 27; /* File too large */
    public static final int ENOSPC = 28; /* No space left on device */
    public static final int ESPIPE = 29; /* Illegal seek */
    public static final int EROFS = 30; /* Read-only file system */
    public static final int EMLINK = 31; /* Too many links */
    public static final int EPIPE = 32; /* Broken pipe */
    public static final int EDOM = 33; /* Math argument out of domain of func */
    public static final int ERANGE = 34; /* Math result not representable */

    /**
     *
     * @param context
     *            : context object(Need it to judge which interface is connected
     *            internet)
     */
    private NfsManager(Context context1) {
        if (context1 != null) {
            this.context = context1;
        }
    }

    public static NfsManager getInstance(Context context) {
        if (sm == null) {
            sm = new NfsManager(context);
        }
        return sm;
    }

    /**
     * Obtain self ip. Some times, device will have more than one network
     * interface(physical/virtual). Other situation not test, current this code
     * run well.(Need Test)
     *
     * @return success: String object contain connected interface ip address
     *         fail: null
     */
    private String getSelfIP() {

        Enumeration<NetworkInterface> allNetInterfaces = null;
        InetAddress ia = null;
        String ip = null;
        String ifName = null;
        NetworkInfo ni = null;

        try {
            allNetInterfaces = NetworkInterface.getNetworkInterfaces();
        } catch (SocketException e) {
            e.printStackTrace();
        }

        // Obtain ConnectivityManager to verify the several interface state
        ConnectivityManager cm = (ConnectivityManager) context
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        while (allNetInterfaces.hasMoreElements()) {
            NetworkInterface netInterface = (NetworkInterface) allNetInterfaces.nextElement();
            Enumeration<InetAddress> addresses = netInterface.getInetAddresses();
            ifName = netInterface.getName();
            if (ifName.equals("eth0")) {
                ni = cm.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);
                if (!ni.isConnected()) {
                    continue;
                }
            } else if (ifName.equals("wlan0")) {
                ni = cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
                if (!ni.isConnected()) {
                    continue;
                }
            } else {
                continue;
            }
            while (addresses.hasMoreElements()) {
                ia = (InetAddress) addresses.nextElement();
                if (ia != null && ia instanceof Inet4Address) {
                    ip = ia.getHostAddress();
                    if (V_BUG) {
                        Log.d(TAG, "Interface: " + ifName + "\t" + "IP:" + ip);
                    }
                    break;
                }
            } // end while
        } // end while

        return ip;
    }

    /**
     * Obtain self subnet Example: if IP=192.168.1.192, Result is 192.168.1 This
     * function not run very well in subnet mask not /24.(Need Solve)
     *
     * @return success: String object contain subnet fail: null
     */

    private String getSubnet() {
        String ip = getSelfIP();
        if (ip == null) {
            return null;
        }
        int i = 0;
        for (i = ip.length() - 1; i > 0; i--) {
            if (ip.charAt(i) == '.') {
                break;
            }
        }
        if (V_BUG) {
            Log.d(TAG, "Get subnet is " + ip.substring(0, i));
        }
        return ip.substring(0, i);
    }

    /**
     * Get local domain all NFS Server information
     *
     * @return success: ArrayList object contain NFSServer object which running
     *         NFS Server fail: ArrayList object contain 0 NFSServer object
     */
    public ArrayList<NFSServer> getServers() {
        if (V_BUG) {
            Log.d(TAG, "-----------start scan servers-----------------");
        }
        servers = new ArrayList<NFSServer>();
        String subnet = getSubnet();
        String ip = "";

        ScanThread[] threads = new ScanThread[255];

        for (int i = 0; i < 255; i++) {
            ip = subnet + "." + i;
            threads[i] = new ScanThread(ip, NFS_PORT);
            threads[i].start();
        }
        for (int i = 0; i < 255; i++) {
            try {
                threads[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        if (V_BUG) {
            Log.d(TAG, "All thread over");
            Log.d(TAG, "-------------end scan servers---------------");
        }
        return servers;
    }

    /**
     * Get the NFS Server export foler list
     *
     * @param server
     *            : NFSServer object
     * @return success: ArrayList object contains all shared folder of the
     *         server fail: ArrayList object contain 0 folder object
     */
    public ArrayList<NFSFolder> getSharedFolders(NFSServer server) {
        String[] cmd = {"/system/bin/nfsprobe", "-e", server.getServerIP()};
        String line = "";
        InputStream is = null;
        NFSFolder folder = null;
        try {
            Runtime runtime = Runtime.getRuntime();
            Process proc = runtime.exec(cmd);
            is = proc.getInputStream();
            BufferedReader buf = new BufferedReader(new InputStreamReader(is));
            proc.waitFor();
            ArrayList<NFSFolder> newList = new ArrayList<NFSFolder>();
            ArrayList<NFSFolder> tmpList = new ArrayList<NFSFolder>();

            // return result contains permission info, there trim it.
            while ((line = buf.readLine()) != null) {

                int i = line.length();
                for (i = i - 1; i > 0; i--) {
                    if (line.charAt(i) == 32) {
                        break;
                    }
                }
                line = line.substring(0, i).trim();
                folder = new NFSFolder();
                folder.setFolderPath(line);
                newList.add(folder);
            }
            for (NFSFolder f1 : newList) {
                boolean found = false;
                for (NFSFolder f2 : server.getFolderList()) {
                    if (f1.getFolderPath().equals(f2.getFolderPath())) {
                        found = true;
                        tmpList.add(f2);
                        break;
                    }
                }
                if (!found) {
                    tmpList.add(f1);
                }
            }
            server.setFolderList(tmpList);
        } catch (Exception ex) {
            System.out.println(ex.getStackTrace());
        }
        return server.getFolderList();
    }

    /**
     * Mount the server's all shared folder to local disk busybox mount -o
     * nolock -t nfs 192.168.99.112:/home/wanran/share /sdcard/share There will
     * use jni call system function mount.
     *
     * @param source
     *            : source(remote) dir, example: /home/wanran/share
     * @param target
     *            : target(local) dir, example: /sdcard/share
     * @param sourceIp
     *            : source(remote) ip addr, example: 192.168.99.112
     * @return success: true fail: false
     */
    public boolean nfsMount(String source, String target, String sourceIp) {
        source = sourceIp + ":" + source;
        String opts = NFS_OPTS + sourceIp;
        int ret = 0;
        // First try read and write permission
        ret = SystemMix.mount(source, target, NFS_TYPE, NFS_FLAGS, opts);
        if (ret == 0) {
            return true;
        }
        if (V_BUG) {
            Log.d(TAG, "Mount error, errno=" + ret);
        }
        return false;
    }

    /**
     * Unmount the server's all shared folder from local disk busybox umount
     * /sdcard/share There will use jni call system function unmount
     *
     * @param target
     *            : target(local) dir, example: /sdcard/share
     * @return success: true fail: false
     */
    public boolean nfsUnmount(String target) {
        SystemMix.umount(target);
        return true;
    }

    /**
     * Thread to scan a host whether running NFS Server
     *
     * @author A
     */
    private class ScanThread extends Thread {
        private String ipAddr = "";
        private int portNum = 0;

        ScanThread(String ipaddr1, int portnum1) {
            this.ipAddr = ipaddr1;
            this.portNum = portnum1;
        }

        @Override
        public void run() {
            // TODO Auto-generated method stub
            boolean isactive = false;
            Socket socket = null;
            try {
                InetAddress ia = InetAddress.getByName(ipAddr);
                if (ia.isReachable(1500)) {
                    isactive = true;
                }
                if (!isactive) {
                    return;
                } else {

                    if (V_BUG) {
                        Log.d(TAG, ipAddr + " is active.");
                    }
                    // If the address can create socket, the host active and
                    // open this port
                    //socket = new Socket(ia, portNum);
                    socket = new Socket();
                    SocketAddress endpoint = new InetSocketAddress(ia, portNum);
                    socket.connect(endpoint, 5000);
                    if (V_BUG) {
                        Log.d(TAG, ipAddr + " open nfs service");
                    }
                    NFSServer server = new NFSServer();
                    server.setServerIP(ipAddr);
                    // server.setServerHostname(ia.getHostName());
                    // General, NFS server is run on linux. For security, This
                    // method can't always success.
                    // Also , it will increase scan time. So discard it.
                    servers.add(server);
                }
            } catch (UnknownHostException e) {
            } catch (IOException e) {
            } finally {
                if (socket != null) {
                    try {
                        socket.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }

        public String getIpAddr() {
            return ipAddr;
        }

        public void setIpAddr(String ipaddr1) {
            this.ipAddr = ipaddr1;
        }

        public int getPortMum() {
            return portNum;
        }

        public void setPortNum(int portnum1) {
            this.portNum = portnum1;
        }

    }

}

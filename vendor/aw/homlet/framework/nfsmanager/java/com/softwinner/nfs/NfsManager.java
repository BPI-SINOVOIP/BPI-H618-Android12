package com.softwinner.nfs;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

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
//import com.softwinner.nfs.NFSServer;

/**
 * 
 * @author Ethan Shan
 *
 */
public final class NfsManager {
    // Test speed
    private static final String TAG = "NFS_SPEED";
    
    private static final int NFS_FLAGS = 32768;                // NFS mount flags
    private static final String NFS_OPTS = "nolock,addr=";    // NFS mount options
    private static final String NFS_TYPE = "nfs";                // NFS mount type
    
    private static NfsManager sm = null;
    
    private Context context = null;
    
    /**
     * 
     * @param context: context object(Need it to judge which interface is connected internet)
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
     * Obtain self ip.
     * Some times, device will have more than one network interface(physical/virtual).
     * Other situation not test, current this code run well.(Need Test)
     * @return
     * success: String object contain connected interface ip address
     * fail: null
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
        ConnectivityManager cm =
            (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        while (allNetInterfaces != null && allNetInterfaces.hasMoreElements()) {
            NetworkInterface netInterface = (NetworkInterface) allNetInterfaces
                    .nextElement();
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
                    Log.d("shanxiaoxi", "IP address: " + ip);
                }
            } // end while
        } // end while
        
        return ip;
    }

    /**
     * Obtain self subnet
     * Example: 
     * if IP=192.168.1.192, Result is 192.168.1
     * This function not run very well in subnet mask not /24.(Need Solve)
     * @return
     * success: String object contain subnet
     * fail: null
     */        
     
    private String getSubnet() {
        String ip = getSelfIP();
        if (ip == null) {
            return null;
        }
        Log.d("shanxiaoxi", "IP:" + ip);
        int i = 0;
        for (i = ip.length() - 1; i > 0; i--) {
            if (ip.charAt(i) == '.') {
                break;
            }
        }
        return ip.substring(0, i);
    }

    private ArrayList<NFSServer> servers = null;

    /**
     * Get local domain all NFS Server information
     * @return
     * success: ArrayList object contain NFSServer object which running NFS Server
     * fail: ArrayList object contain 0 NFSServer object
     */
    public ArrayList<NFSServer> getServers() {
        Log.d(TAG, "----------------------------");
        servers = new ArrayList<NFSServer>();
        String subnet = getSubnet();
        String ip = "";

        ScanThread[] threads = new ScanThread[255];
    
        for (int i = 0; i < 255; i++) {
            ip = subnet + "." + i;
            threads[i] = new ScanThread(ip, 111);
            threads[i].start();
        }
        for (int i = 0; i < 255; i++) {
            try {
                threads[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        Log.d("shanxiaoxi", "All thread over");
        Log.d(TAG, "----------------------------");
        return servers;
    }

    /**
     * Get the NFS Server export foler list
     * @param server: NFSServer object
     * @return
     * success: ArrayList object contains all shared folder of the server
     * fail: ArrayList object contain 0 folder object
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
            BufferedReader buf = new BufferedReader(new InputStreamReader(is, "UTF-8"));
            proc.waitFor();

            // return result contains permission info, there trim it.
            while ((line = buf.readLine()) != null) {
                
                int i = line.length();
                for (i = i - 1; i > 0; i--) {
                    if (line.charAt(i) == 32) {
                        break;
                    }
                }
                line  = line.substring(0, i).trim();
                folder = new NFSFolder();
                folder.setFolderPath(line);
                server.getFolderList().add(folder);

                
            }
            buf.close();
        } catch (Exception ex) {
            System.out.println(ex.getStackTrace());
        }
        return server.getFolderList();
    }


    
    /**
     * Mount the server's all shared folder to local disk
     * busybox mount -o nolock -t nfs 192.168.99.112:/home/wanran/share /sdcard/share
     * There will use jni call system function mount.
     * @param source: source(remote) dir, example: /home/wanran/share
     * @param target: target(local) dir, example: /sdcard/share 
     * @param sourceIp: source(remote) ip addr, example: 192.168.99.112
     * @return
     * success: true
     * fail: false
     */
    public boolean nfsMount(String source, String target, String sourceIp) {
        String source1 = sourceIp + ":" + source;
        String opts = NFS_OPTS + sourceIp;
        if (SystemMix.mount(source1, target, NFS_TYPE, NFS_FLAGS, opts) == 0) {
            return true;
        }
        return false;
    }

    /**
     * Unmount the server's all shared folder from local disk
     * busybox umount /sdcard/share
     * There will use jni call system function unmount
     * @param target: target(local) dir, example: /sdcard/share
     * @return
     * success: true
     * fail: false
     */
    public boolean nfsUnmount(String target) {
        SystemMix.umount(target);
        return true;
    }

    /**
     * Thread to scan a host whether running NFS Server
     * @author A
     */
    private class ScanThread extends Thread {
        private String mIpAddr = "";
        private int mPortNum = 0;

        ScanThread(String ipaddr, int portnum) {
            this.mIpAddr = ipaddr;
            this.mPortNum = portnum;
        }

        @Override
        public void run() {
            // TODO Auto-generated method stub
            boolean isactive = false;
            Socket socket = null;
            try {
                InetAddress ia = InetAddress.getByName(mIpAddr);
                if (ia.isReachable(1000)) {
                    isactive = true;
                }
                if (!isactive) {
                    return;
                } else {
                    Log.d("shanxiaoxi", mIpAddr + "active");
                    socket = new Socket(ia, mPortNum);
                    Log.d("shanxiaoxi", mIpAddr + "open nfs service");
                    NFSServer server = new NFSServer();
                    server.setServerIP(mIpAddr);
                    // server.setServerHostname(ia.getHostName());
                    // General, NFS server is run on linux. For security, 
                    //This method can't always success.
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
            return mIpAddr;
        }

        public void setIpAddr(String ipaddr1) {
            this.mIpAddr = ipaddr1;
        }

        public int getPortNum() {
            return mPortNum;
        }

        public void setPortNum(int portnum1) {
            this.mPortNum = portnum1;
        }

    }

}

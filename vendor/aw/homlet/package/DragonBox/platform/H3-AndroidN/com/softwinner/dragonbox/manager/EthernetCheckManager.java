package com.softwinner.dragonbox.manager;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.EthernetManager;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.util.Log;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.regex.Pattern;

import com.softwinner.dragonbox.entity.EthernetInfo;
import com.softwinner.dragonbox.platform.CommonPlatformUtil;

public class EthernetCheckManager {
    public static final int TEST_TIME = 10;//测试时长10s
    private EthernetManager mEthManager;
    private WifiManager mWifiManager;

    private Context mContext;

    private boolean isRunning = false;

    private Handler mHandler = new Handler();
    private EthernetInfo mEthernetInfo;
    private OnEthernetCheckListener mEthernetListener;

    private static final Pattern IPV4_PATTERN =
            Pattern.compile(
                    "^(25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)(\\.(25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)){3}$");

    public EthernetCheckManager(Context context) {
        mContext = context;
        mEthManager = CommonPlatformUtil.getEthernetManager(context);
        mWifiManager = (WifiManager) mContext
                .getSystemService(Context.WIFI_SERVICE);
        mEthernetInfo = new EthernetInfo();
    }

    private boolean setWifiDisable() {
        final WifiManager wm = (WifiManager) mContext
                .getSystemService(Context.WIFI_SERVICE);

        mWifiManager.setWifiEnabled(false);

        return wm.getWifiState() == WifiManager.WIFI_STATE_DISABLED;
    }

    /*	private boolean setEthernetEnable() {
            if (mEthManager.getState() == EthernetManager.ETHERNET_STATE_DISABLED) {
                mEthManager.setEnabled(true);
            }

            return EthernetManager.ETHERNET_STATE_ENABLED == mEthManager.getState();
        }
    */
    private String getIPAdd() {

        ConnectivityManager connectivityManager = (ConnectivityManager) mContext
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo netInfo = connectivityManager
                .getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);

        if (netInfo.isAvailable()) {
            Log.i("dragonbox", "ethernet network is available");
            Enumeration<NetworkInterface> networkInterfaces = null;
            try {
                networkInterfaces = NetworkInterface.getNetworkInterfaces();
            } catch (SocketException e) {
                e.printStackTrace();
            }

            if (networkInterfaces != null) {
                NetworkInterface network = null;

                while (networkInterfaces.hasMoreElements()) {
                    Log.i("dragonbox", "network interface");
                    network = networkInterfaces.nextElement();
                    Enumeration<InetAddress> inetAddresses = network
                            .getInetAddresses();

                    InetAddress inetAddress = null;
                    while (inetAddresses != null
                            && inetAddresses.hasMoreElements()) {
                        inetAddress = inetAddresses.nextElement();
                        Log.i("dragonbox",
                                "inetAddress:"
                                        + inetAddress.getHostAddress() + "network.getName()=" + network.getName());
                        if (network.getName().equalsIgnoreCase(
                                new String("eth0"))) {
                            if (!inetAddress.isLoopbackAddress()
                                    && EthernetCheckManager.isIPv4Address(inetAddress.getHostAddress())) {
                                return inetAddress.getHostAddress();
                            }
                        }
                    }
                }
            }
        }
        return null;
    }

    public void startEthernetCheck() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                isRunning = true;
                if (mEthernetListener != null) {
                    mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mEthernetListener.onCheckStart();
                        }
                    });
                }
                int loopTimes = TEST_TIME;
                while (isRunning && loopTimes != 0) {
                    //setWifiDisable();
                    //setEthernetEnable();
                    mEthernetInfo.setIpAdd(getIPAdd());

                    mEthernetInfo.setLineConn(CommonPlatformUtil.checkLink(mEthManager, "eth0") == 1);
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    if (mEthernetInfo.getIpAdd() != null
                            && mEthernetInfo.isLineConn()) {
                        isRunning = false;
                    }
                    if (mEthernetListener != null) {
                        mHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                mEthernetListener.onCheckUpdate(mEthernetInfo);
                            }
                        });
                    }
                    loopTimes--;
                }

                if (mEthernetListener != null) {
                    mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mEthernetListener.onCheckEnd(mEthernetInfo);
                        }
                    });
                }
            }
        }).start();
    }

    public void stopEthernetCheck() {
        isRunning = false;
    }
    public static boolean isIPv4Address(final String input) {
        return IPV4_PATTERN.matcher(input).matches();
    }

    public EthernetInfo getEthernetInfo() {
        return mEthernetInfo;
    }

    public void setOnEthernetCheckListener(
            OnEthernetCheckListener ethernetListener) {
        this.mEthernetListener = ethernetListener;
    }

    public interface OnEthernetCheckListener {

        public void onCheckStart();

        public void onCheckUpdate(EthernetInfo ethernetInfo);

        public void onCheckEnd(EthernetInfo ethernetInfo);
    }

}

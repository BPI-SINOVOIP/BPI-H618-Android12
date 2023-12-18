package com.android.server.pppoe;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.DhcpResults;
import android.net.InterfaceConfiguration;
import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.IpConfiguration.ProxySettings;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.NetworkAgent;
import android.net.NetworkCapabilities;
import android.net.NetworkFactory;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.NetworkRequest;
import android.net.EthernetManager;
import android.net.StaticIpConfiguration;
import android.net.NetworkAgentConfig;
import android.net.InetAddresses;
import android.net.IpPrefix;
import android.net.INetd;
import android.net.util.NetdService;
import android.os.Handler;
import android.os.UserHandle;
import android.os.IBinder;
import android.os.INetworkManagementService;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.text.TextUtils;
import android.util.Log;
import android.content.Intent;
import android.os.UserHandle;
import android.os.SystemProperties;
import com.android.internal.util.IndentingPrintWriter;
import com.android.server.net.BaseNetworkObserver;
import com.android.net.module.util.NetdUtils;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.net.InetAddress;
import java.util.List;
import java.util.Objects;
import android.net.RouteInfo;
import android.net.PppoeManager;
import android.net.IPppoeManager;
import java.net.Inet4Address;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 *  track pppoe state
 *  pppoe connect & setup & disconnect
 *  @hide
**/

class PppoeNetworkFactory {
    private static final String NETWORK_TYPE = "PPPOE";
    private static final String TAG = "PppoeNetworkFactory";
    private static final int NETWORK_SCORE = 120;
    private static final boolean DBG = true;
    private static final boolean VDBG= true;

    private static void LOG(String msg) {
        if (DBG) {
            Log.d(TAG, msg);
        }
    }

    private static void LOGD(String msg) {
        if (VDBG) {
            Log.d(TAG,msg);
        }
    }

    /** Tracks interface changes. Called from NetworkManagementService. */
    private InterfaceObserver mInterfaceObserver;

    /** For static IP configuration */
    private PppoeManager mPppoeManager;

    /** To set link state and configure IP addresses. */
    private INetworkManagementService mNMService;

    /* To communicate with ConnectivityManager */
    private NetworkCapabilities mNetworkCapabilities;
    private NetworkAgent mNetworkAgent;
    private LocalNetworkFactory mFactory;
    private Context mContext;
    private IpConfiguration mIpConfig;
    /** Product-dependent regular expression of interface names we track. */
    private static String mIfaceMatch = "ppp\\d";

    /** Data members. All accesses to these must be synchronized(this). */
    private static String mIface = "";
    private String mPhyIface = "eth0";
    private boolean everUpdateAgent = false;
    private String mHwAddr;
    private static boolean mLinkUp;
    private LinkProperties mLinkProperties;
    private int mPppoeCurrentState;
    private String lcpState;
    private static boolean running = false;

    static {
        System.loadLibrary("pppoe-jni");
        registerNatives();
    }

    public int mPppoeState = PppoeManager.PPPOE_STATE_DISCONNECTED;

    private void setPppoeStateAndSendBroadcast(int newState) {
        int preState = mPppoeState;
        int ExitCode;
        mPppoeState = newState;
        final Intent intent = new Intent(PppoeManager.PPPOE_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT |
                        Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intent.putExtra(PppoeManager.EXTRA_PPPOE_STATE, newState);
        if (newState == PppoeManager.PPPOE_EVENT_CONNECT_FAILED) {
            String exitCode=SystemProperties.get("net.eth0-pppoe.ppp-exit");
            LOG("net.eth0-pppoe.ppp-exit"+exitCode);

            if (exitCode != null &&!exitCode.isEmpty()) {
                ExitCode = Integer.parseInt(exitCode);
                if (ExitCode < 1 || ExitCode > 22 || ExitCode == 20)    exitCode = "0";
                intent.putExtra(PppoeManager.EXTRA_PPPOE_ERRMSG, exitCode);
            }
        }
        LOG("setPppoeStateAndSendBroadcast() : preState = " + preState +", curState = " + newState);
        mContext.sendStickyBroadcastAsUser(intent,UserHandle.ALL);
    }

    PppoeNetworkFactory() {
        mLinkProperties = new LinkProperties();
        initNetworkCapabilities();
    }

    private void initNetworkCapabilities() {

        mNetworkCapabilities = new NetworkCapabilities.Builder()
                                    .addTransportType(NetworkCapabilities.TRANSPORT_PPPOE)
                                    .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                                    .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED)
                                    .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VCN_MANAGED)
                                    .setLinkUpstreamBandwidthKbps(100 * 1000)
                                    .setLinkDownstreamBandwidthKbps(100 * 1000).build();
    }

    private class LocalNetworkFactory extends NetworkFactory {
        LocalNetworkFactory(String name, Context context, Looper looper) {
            super(looper, context, name, new NetworkCapabilities());
        }

        protected void startNetwork() {
            onRequestNetwork();
        }

        protected void stopNetwork() {
        }
    }

    private class InterfaceObserver extends BaseNetworkObserver {
        @Override
        public void interfaceLinkStateChanged(String iface, boolean up) {
            Log.w(TAG,"interfaceLinkStateChanged");
            updateInterfaceState(iface, up);
        }

        @Override
        public void interfaceAdded(String iface) {
            Log.w(TAG,"interfaceAdded");
            maybeTrackInterface(iface);
        }

        @Override
        public void interfaceRemoved(String iface) {
            Log.w(TAG,"interfaceRemoved");
            stopTrackingInterface(iface);
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_EVENT_DISCONNECT_SUCCESSED);
        }
    }

    public void updateInterfaceState(String iface, boolean up) {

        LOGD("updateInterface: mIface:" + mIface + " mPhyIface:"+ mPhyIface
                        + " iface:" + iface + " link:" + (up ? "up" : "down"));

        if (!mIface.equals(iface) && !mPhyIface.equals(iface)) {
            LOGD("not tracker interface");
            return;
        }

        if (mPhyIface.equals(iface)) { //需要监所连接物理端口
            if (!up) {
                if (getPppoeState(iface) != PppoeManager.PPPOE_STATE_DISCONNECTED) {
                    stopPppoe(iface);                  //当物理网口断开时,断开pppoe链接
                    stopTrackingInterface(iface);
                    setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_EVENT_CONNECT_FAILED);
                }
            } else {
                if (mPppoeManager.isPppoeEnabled() && (getPppoeState(iface) != PppoeManager.PPPOE_STATE_CONNECTED)) {
                    startPppoe(iface);
                }
            }
            return;
        }
    }

    public boolean maybeTrackInterface(String iface) {
        // If we don't already have an interface, and if this interface matches
        // our regex, start tracking it.
        if (!iface.matches(mIfaceMatch))
            return false;
        LOG("Started tracking interface " + iface);
        setInterfaceUp(iface);
        return true;
    }

    public void updateAgent(DetailedState detailedState) {
        LOG("updateAgent");
        synchronized (PppoeNetworkFactory.this) {
            if (mNetworkAgent == null) {
                LOG("mNetworkAgent is null");
                return;
            }
            if (DBG) {
                Log.i(TAG, "Updating mNetworkAgent with: " +
                      mNetworkCapabilities + ", " +
                      mLinkProperties);
            }

            mNetworkAgent.sendNetworkCapabilities(mNetworkCapabilities);
            mNetworkAgent.sendLinkProperties(mLinkProperties);
            mNetworkAgent.sendNetworkScore(mLinkUp ? NETWORK_SCORE : 0);
            /// add
            // TODO : only accept transitions when the agent is in the correct state (non-null for
            // CONNECTED, DISCONNECTED and FAILED, null for CONNECTED).
            // This will require a way for tests to pretend the VPN is connected that's not
            // calling this method with CONNECTED.
            // It will also require audit of where the code calls this method with DISCONNECTED
            // with a null agent, which it was doing historically to make sure the agent is
            // disconnected as this was a no-op if the agent was null.
            switch (detailedState) {
                case CONNECTED:
                    if (null != mNetworkAgent) {
                        mNetworkAgent.register();
                        mNetworkAgent.markConnected();
                    }
                    break;
                case DISCONNECTED:
                case FAILED:
                    if (null != mNetworkAgent) {
                        mNetworkAgent.unregister();
                        mNetworkAgent = null;
                    }
                    break;
                case CONNECTING:
                    if (null != mNetworkAgent) {
                        throw new IllegalStateException("VPN can only go to CONNECTING state when"
                                + " the agent is null.");
                    }
                    break;
                default:
                    throw new IllegalArgumentException("Illegal state argument " + detailedState);
            }
            /// end
        }
    }

    private void setInterfaceUp(String iface) {
        // Bring up the interface so we get link status indications.
        try {
            mNMService.setInterfaceUp(iface);
            InterfaceConfiguration config = mNMService.getInterfaceConfig(iface); // get eth0/wlan0 mac address
            if (config == null) {
                Log.e(TAG, "Null iterface config for " + iface + ". Bailing out.");
                return;
            }
            synchronized (this) {
                mHwAddr = config.getHardwareAddress();
                Log.w(TAG,"setInterfaceUp ,mHwAddr = "+mHwAddr);
            }
        } catch (Exception e) {
                Log.e(TAG, "Error upping interface " + mIface + ": " + e);
        }
    }

    public void stopTrackingInterface(String iface) {
        LOG("stopTrackingInterface");
        if ((!mIface.equals("")) && (!iface.equals(mIface)))
            return;

        Log.d(TAG, "Stopped tracking interface " + iface);
        // TODO: Unify this codepath with stop().
        synchronized (this) {
            mIface = "";
            everUpdateAgent = false;
            mLinkUp = false;
            updateAgent(DetailedState.DISCONNECTED);
            mNetworkAgent = null;
            mLinkProperties = new LinkProperties();
        }
    }
    public void onRequestNetwork() {
        LOG("onRequestNetwork");
    }


    public void connected(String iface) {
        LOG("connected");
        LinkProperties linkProperties;
        String lcpstate = SystemProperties.get("net."+iface+"-pppoe.lcpup");
        Log.d("connected", "lcpstate:"+lcpstate+",mNetworkAgent != null:"+(mNetworkAgent != null));
        synchronized(PppoeNetworkFactory.this) {
            if ((mNetworkAgent != null) && ("no").equals(lcpstate)) {
                Log.e(TAG, "Already have a NetworkAgent - aborting new request");
                return;
            }
            mIface = SystemProperties.get("net."+iface+"-pppoe.interface");
            linkProperties = new LinkProperties();
            linkProperties.setInterfaceName(this.mIface);

            String[] dnses = new String[2];
            String route;
            String ipaddr;

            dnses[0] = SystemProperties.get("net."+iface+"-"+mIface+".dns1");
            dnses[1] = SystemProperties.get("net."+iface+"-"+mIface+".dns2");

            List<InetAddress>dnsServers = new ArrayList<InetAddress>();
            dnsServers.add(InetAddresses.parseNumericAddress(dnses[0]));
            dnsServers.add(InetAddresses.parseNumericAddress(dnses[1]));
            linkProperties.setDnsServers(dnsServers);

            route = SystemProperties.get("net."+iface+"-"+mIface+".remote-ip");
            RouteInfo routeinfo = new RouteInfo((IpPrefix)null, InetAddresses.parseNumericAddress(route), null, RouteInfo.RTN_UNICAST);
	        Log.d("connected", "InetAddresses.parseNumericAddress(route):"+(InetAddresses.parseNumericAddress(route)));
            linkProperties.addRoute(routeinfo);

            ipaddr = SystemProperties.get("net."+iface+"-"+mIface+".local-ip");
            linkProperties.addLinkAddress(new LinkAddress(InetAddresses.parseNumericAddress(ipaddr),28));

            mLinkProperties = linkProperties;
           // everUpdateAgent = true;
            // Create our NetworkAgent.
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_EVENT_CONNECT_SUCCESSED);
            mNetworkAgent = null;
            final NetworkAgentConfig agentconfig = new NetworkAgentConfig.Builder()
                                                    .setLegacyType(ConnectivityManager.TYPE_PPPOE)
                                                    .setLegacyTypeName(NETWORK_TYPE)
                                                    .setLegacyExtraInfo(mHwAddr)
                                                    .build();
            mNetworkAgent = new NetworkAgent(mContext, mFactory.getLooper(),
                            NETWORK_TYPE, mNetworkCapabilities, mLinkProperties,
                            NETWORK_SCORE,agentconfig, null) {
                public void unwanted() {
                    synchronized(PppoeNetworkFactory.this) {
                        if (this == mNetworkAgent) {
                            LOG("unWanted");
                            mLinkProperties.clear();
                            updateAgent(DetailedState.DISCONNECTED);
                            mNetworkAgent = null;
                        } else {
                            Log.d(TAG, "Ignoring unwanted as we have a more modern " +
                                                "instance");
                            mNetworkAgent = null;
                        }
                    }
                };
            };
            mLinkUp = true;
            updateAgent(DetailedState.CONNECTED);
        }
    }

    /**
     * Begin monitoring connectivity
     */
    public synchronized void start(Context context, Handler target) {

        // The services we use.
        IBinder b = ServiceManager.getService(Context.NETWORKMANAGEMENT_SERVICE);
        mNMService = INetworkManagementService.Stub.asInterface(b);
        mPppoeManager = (PppoeManager) context.getSystemService(Context.PPPOE_SERVICE);

        // Create and register our NetworkFactory.
        mFactory = new LocalNetworkFactory(NETWORK_TYPE, context, target.getLooper());
        mFactory.setCapabilityFilter(mNetworkCapabilities);
        mFactory.setScoreFilter(-1); // this set high when we have an iface
        mFactory.register();
        mContext = context;

        // Start tracking interface change events.
        mInterfaceObserver = new InterfaceObserver();
        try {
            mNMService.registerObserver(mInterfaceObserver);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not register InterfaceObserver " + e);
        }
        synchronized(this) {
            String iface = mPppoeManager.getPppoeInterfaceName();
            if (mPppoeManager.isPppoeEnabled() && (getPppoeState(iface) != PppoeManager.PPPOE_STATE_CONNECTED)) {
                LOGD("start:startPppoe");
                startPppoe(iface);
            }
        }
    }

    public int getPppoeState(String iface) {
        String state = SystemProperties.get("net."+iface+"-pppoe.status");
        if (state.equals("starting")) {
            return PppoeManager.PPPOE_STATE_CONNECTING;
        }
        else if (state.equals("started")) {
            return PppoeManager.PPPOE_STATE_CONNECTED;
        }
        else if (state.equals("stopping")) {
            return PppoeManager.PPPOE_STATE_DISCONNECTING;
        }
        else if (state.equals("stopped")) {
            return PppoeManager.PPPOE_STATE_DISCONNECTED;
        } else {
            return PppoeManager.PPPOE_STATE_DISCONNECTED;
        }

    }

    public boolean startPppoe(String iface) {
        LOG("startPppoe");
        int count;
        mPhyIface = iface;
        running = true;
        final Thread PppoeThread = new Thread(new Runnable() {
            public void run() {
                setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_EVENT_CONNECTING);
                if (0!=startPppoeNative(iface)) {
                    Log.e(TAG,"startPppoe():failed to start pppoe!");
                    setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_EVENT_CONNECT_FAILED);
                    return;
                } else {
                    connected(iface);
                    return;
                }
            }
        });
        PppoeThread.start();

        //MonitorThread used to monitor pppoe hangup.
        final Thread MonitorThread = new Thread(new Runnable() {
            public void run() {
                while(running) {
                    lcpState = SystemProperties.get("net."+iface+"-pppoe.lcpup");
                    if (("yes").equals(lcpState)) {
                        SystemProperties.set("net." + iface +"-pppoe.lcpup","no");
                        connected(iface);
                    }
                    try {
                        Thread.sleep(200);
                    } catch (Exception e) {
                        LOG("PPPoE MonitorThread error!");
                    }
                }
            }
        });
        MonitorThread.start();
        return true;
    }

    public boolean stopPppoe(String iface) {
        setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_EVENT_DISCONNECTING);
        if (0 != stopPppoeNative(iface)) {
            Log.e(TAG,"stopPppoe():failed to stop pppoe!");
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_EVENT_DISCONNECT_SUCCESSED);
            return false;
        } else {
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_EVENT_DISCONNECT_SUCCESSED);
            running = false;
            return true;
        }
    }

    public LinkProperties getPppoelinkProperties() {
        return mLinkProperties;
    }

    public native static int startPppoeNative(String iface);
    public native static int stopPppoeNative(String iface);
    public native static int registerNatives();
}

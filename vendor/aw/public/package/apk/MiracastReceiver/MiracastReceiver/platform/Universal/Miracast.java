package com.softwinner.miracastReceiver;

import java.io.FileReader;
import java.io.BufferedReader;
import java.util.List;
import java.util.ArrayList;
import java.util.Locale;
import java.lang.reflect.Field;
import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.BroadcastReceiver;

import android.net.NetworkInfo;
import android.net.wifi.p2p.WifiP2pWfdInfo;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pDeviceList;
import android.net.wifi.p2p.WifiP2pGroup;
import android.net.wifi.p2p.WifiP2pManager;
import android.net.wifi.p2p.WifiP2pManager.PeerListListener;
import android.net.wifi.p2p.WifiP2pInfo;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

import android.view.View;
import android.view.ViewParent;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.view.KeyEvent;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Animation;
import android.view.animation.Interpolator;
import android.view.animation.TranslateAnimation;

import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Scroller;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.FrameLayout;
import android.widget.MediaController;

import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v4.view.ViewPager.OnPageChangeListener;

import android.os.SystemProperties;
import android.util.Log;
import android.view.WindowManager;
import android.content.res.Configuration;

import com.softwinner.wfdsink.WFDSinkView;
import com.softwinner.wfdsink.WFDManager;
import com.softwinner.wfdsink.ViewCallback;
import android.provider.Settings;

public class Miracast extends Activity{
    Context mContext;
    private static final String TAG = "Miracast";

    private String mDeiveName = "";

    private View mConetentView = null;
    private TextView mTitle = null;
    private LinearLayout mLayout = null;
    private ImageView mSlider_bg = null;
    private ImageView mSlider = null;
    private TranslateAnimation mAnimation = null;
    private TextView mHint = null;
    private Button mButton = null;

    private ViewPager mViewPager = null;
    private ImageView[] mHelpImageViews;
    private List<View> mHelpViews = null;
    private MySpeedScroller mSpeedScroller = null;
    private final int mScrollDelay = 200;
    private int mHelpStep = 0;

    private final static int HOME_PAGE = 0x00;
    private final static int HOME_NOT_FOCUSED_PAGE = 0x01;
    private final static int HELP_PAGE = 0x02;
    private final static int SURFACEVIEW_PAGE = 0x03;
    private int mPageState = HOME_PAGE;

    private WFDSinkView mWFDSinkView = null;
    private WifiP2pManager mWifiP2pManager = null;
    private WifiP2pManager.Channel mChannel = null;
    private int mCTLPort = 0;
    private String mPeerIP = null;
    private WifiP2pDevice mPeerDevice = null;
    private WifiP2pDevice mThisDevice = null;

    private Timer mTimer = null;
    private ScheduledExecutorService mScheduledExecutorService = null;

    private final static int MIRACAST_STOP = 0x02;
    private final static int MIRACAST_START = 0x01;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MIRACAST_STOP:
                Log.d(TAG, "MIRACAST_STOP");

                if (mWFDSinkView != null) {
                    mWFDSinkView.reset();
                    ViewParent p = mWFDSinkView.getParent();
                    if (p instanceof ViewGroup) {
                        ViewGroup group = (ViewGroup)p;
                        Log.i(TAG, "remove WFDSinkView from window");
                        group.removeView(mWFDSinkView);
                    } else {
                        Log.w(TAG, "wfdsinkview parent is null");
                    }
                    //mWFDSinkView.setVisibility(View.GONE);
                    mWFDSinkView = null;

                    mConetentView.setVisibility(View.VISIBLE);
                    startBroadcastWFDSink();
                }

                mConetentView.setVisibility(View.VISIBLE);
                updateHint(true);

                mPageState = HOME_PAGE;
                mCTLPort = 0;
                mPeerIP = null;
                mPeerDevice = null;

                break;
            case MIRACAST_START:
                Log.d(TAG, "MIRACAST_START");

                try {
                    if (mWFDSinkView != null) {
                        mWFDSinkView.reset();
                        ViewParent p = mWFDSinkView.getParent();
                        if (p instanceof ViewGroup) {
                            ViewGroup group = (ViewGroup)p;
                            Log.i(TAG, "remove WFDSinkView from window");
                            group.removeView(mWFDSinkView);
                        } else {
                            Log.w(TAG, "wfdsinkview parent is null");
                        }
                        mWFDSinkView.setVisibility(View.GONE);
                    }

                    mWFDSinkView = new WFDSinkView(mContext);
                    mWFDSinkView.setViewCallback(mViewCallback);
                    mWFDSinkView.setDataSource(
                            mContext,
                            mPeerIP,
                            mCTLPort);

                    //mWFDSinkView.setMediaController(new MediaController(mContext));

                    mWFDSinkView.requestFocus(); // TODO

                    mPageState = SURFACEVIEW_PAGE;

                    stopBroadcastWFDSink();
                    mConetentView.setVisibility(View.GONE);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                break;
            default:
                break;
            }
        };
    };

    private final IntentFilter mIntentFilter = new IntentFilter();
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (action.equals(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION)) {
                // Broadcast intent action to indicate whether Wi-Fi p2p is enabled or disabled
                boolean enabled = (intent.getIntExtra(WifiP2pManager.EXTRA_WIFI_STATE,
                        WifiP2pManager.WIFI_P2P_STATE_DISABLED)) ==
                        WifiP2pManager.WIFI_P2P_STATE_ENABLED;
                Log.d(TAG, "Received WIFI_P2P_STATE_CHANGED_ACTION: enabled=" + enabled);

                handleStateChanged(enabled);
            } else if (action.equals(WifiP2pManager.WIFI_P2P_DISCOVERY_CHANGED_ACTION)) {
                // Broadcast intent action indicating that peer discovery has either started or stopped
                // Note that discovery will be stopped during a connection setup
                // If the application tries to re-initiate discovery during this time, it can fail
                boolean started = (intent.getIntExtra(WifiP2pManager.EXTRA_DISCOVERY_STATE,
                        WifiP2pManager.WIFI_P2P_DISCOVERY_STOPPED)) ==
                        WifiP2pManager.WIFI_P2P_DISCOVERY_STARTED;
                Log.d(TAG, "Received WIFI_P2P_DISCOVERY_CHANGED_ACTION: started=" + started);
            } else if (action.equals(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION)) {
                NetworkInfo networkInfo = (NetworkInfo)intent.getParcelableExtra(WifiP2pManager.EXTRA_NETWORK_INFO);
                Log.d(TAG, "Received WIFI_P2P_CONNECTION_CHANGED_ACTION: networkInfo=" + networkInfo);

                handleConnectionChanged(networkInfo, intent);
            } else if (action.equals(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION)) {
                mThisDevice = (WifiP2pDevice) intent.getParcelableExtra(WifiP2pManager.EXTRA_WIFI_P2P_DEVICE);
                Log.d(TAG, "Received WIFI_P2P_THIS_DEVICE_CHANGED_ACTION: mThisDevice=" + mThisDevice);
            } else if (action.equals(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION)) {
                WifiP2pDeviceList peers = (WifiP2pDeviceList)intent.getParcelableExtra(WifiP2pManager.EXTRA_P2P_DEVICE_LIST);
                Log.d(TAG, "Received WIFI_P2P_PEERS_CHANGED_ACTION: peers=" + peers);
            }
        }
    };

    private void handleStateChanged(boolean enabled) {
        // TODO
        if (!enabled) {
            Message message = new Message();
            message.what = MIRACAST_STOP;
            mHandler.sendMessage(message);
        }
    }

    private void handlePeersChanged() {
        // Even if wfd is disabled, it is best to get the latest set of peers to
        // keep in sync with the p2p framework
        mWifiP2pManager.requestPeers(mChannel, new PeerListListener() {
            @Override
            public void onPeersAvailable(WifiP2pDeviceList peers) {
                Log.d(TAG, "Received list of peers=" + peers);
            }
        });
    }

    private void handleConnectionChanged(NetworkInfo networkInfo, Intent intent) {
        if (networkInfo.isConnected()) {
            Log.d(TAG, "Connected");

            final WifiP2pInfo wifiP2pInfo = (WifiP2pInfo) intent.getParcelableExtra(
                        WifiP2pManager.EXTRA_WIFI_P2P_INFO);

            Log.d(TAG, "found wifiP2pInfo=" + wifiP2pInfo);

            WifiP2pGroup wifiP2pGroup = (WifiP2pGroup) intent.getParcelableExtra(
                        WifiP2pManager.EXTRA_WIFI_P2P_GROUP);

            Log.d(TAG, "found wifiP2pGroup=" + wifiP2pGroup);

            WifiP2pDevice wifiP2pDevice = null;

            if (!wifiP2pGroup.isGroupOwner()) {
                wifiP2pDevice = wifiP2pGroup.getOwner();
            //} else if (!wifiP2pGroup.isClientListEmpty()) {
            //} else if (wifiP2pGroup.getClientList().size() != 0) {
            }else {
                for (WifiP2pDevice client : wifiP2pGroup.getClientList()) {
                    if(!client.equals(new WifiP2pDevice()))
                        wifiP2pDevice = client;
                }
            }

            if (wifiP2pDevice != null) {
                //WifiP2pWfdInfo wfdInfo_tmp = wifiP2pDevice.getWfdInfo();
                //if (wfdInfo_tmp != null)
                if (wifiP2pDevice.wfdInfo != null)
                    Log.d(TAG, "found wifiP2pDevice=" + wifiP2pDevice);
                else {
                    Log.d(TAG, "wifiP2pDevice.wfdInfo is null");
                    return;
                }
            } else {
                Log.d(TAG, "wifiP2pDevice is null");
                return;
            }

            //WifiP2pWfdInfo wfdInfo = wifiP2pDevice.getWfdInfo();
            if(/*wifiP2pDevice.wfdInfo.getDeviceType() == WifiP2pWfdInfo.WFD_SOURCE && */wifiP2pDevice.wfdInfo.isSessionAvailable()) // TODO
            //if (wfdInfo != null && wfdInfo.isSessionAvailable())    // TODO
            {
                Log.d(TAG, "wifiP2pDevice is a sessionAvailable WFD_SOURCE");

                mPeerDevice = new WifiP2pDevice(wifiP2pDevice);

                updateHint(false); // TODO

                // TODO, need a new thread
                new Thread() {
                    @Override
                    public void run() {
                        int port = 0;
                        //port = wfdInfo.getControlPort();
                        port = mPeerDevice.wfdInfo.getControlPort();
                        if (port == 0) port = 7236;

                        String ip = null;
                        if (!wifiP2pInfo.isGroupOwner) {
                            ip = wifiP2pInfo.groupOwnerAddress.toString();
                            ip = ip.substring(ip.lastIndexOf("/") + 1 );
                        } else {
                            int i = 0;
                            for (; i < 10; i++) {
                                ip = WFDManager.WFDManagerGetPeerIP(mPeerDevice.deviceAddress);
                                if(ip == null || ip.equals("")){
                                    try {
                                        Thread.sleep(1000);
                                    } catch (InterruptedException e) {
                                        e.printStackTrace();
                                    }
                                    continue;
                                }
                                break;
                            }

                            if (i == 10) {
                                Log.d(TAG, "WFDManagerGetPeerIP failed");

                                Message message = new Message(); // TODO
                                message.what = MIRACAST_STOP;
                                mHandler.sendMessage(message);
                                return;
                            }
                        }

                        Log.d(TAG, "WFDManagerGetPeerIP " + ip + " success");

                        if (!ip.equals(mPeerIP) || port != mCTLPort) {
                            mPeerIP = ip;
                            mCTLPort = port;

                            Message message = new Message(); // TODO
                            message.what = MIRACAST_START;
                            mHandler.sendMessage(message);
                        } else { // maybe reconnect
                            Log.w(TAG, "ignore the same session request");
                        }
                    }
                }.start();
            }
        } else {
            Log.d(TAG, "disConnected");
            Toast.makeText(Miracast.this, R.string.wifi_p2p_reset_connection_message, Toast.LENGTH_LONG).show();

            Message message = new Message();
            message.what = MIRACAST_STOP;
            mHandler.sendMessage(message);
        }
    }

    public ViewCallback mViewCallback = new ViewCallback() { // anonymous inner classes
            public void viewCallback() {
                Log.d(TAG, "viewCallback");

                Message message = new Message();
                message.what = MIRACAST_STOP;
                mHandler.sendMessage(message);
            }
    };

//* p2p node is not seen from androidQ
/*
    public static String getEthernetMac() {
        BufferedReader br = null;
        try {
            br = new BufferedReader(new FileReader("/sys/class/net/p2p0/address"));
            String line;
            if ((line = br.readLine()) != null) {
                Log.d(TAG, line);
                String mac="";
                String[] splitted = line.split(":");
                for(int i = 0; i<splitted.length; ++i) {
                    mac += splitted[i];
                }
                mac = mac.toLowerCase();
                Log.d(TAG, "getEthernetMac(): " + mac);
                return mac;
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                if(br != null) br.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return null;
    }
*/

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        setContentView(R.layout.main);

        mConetentView = findViewById(R.id.layout);
        mTitle = (TextView)findViewById(R.id.title);
        mLayout = (LinearLayout)findViewById(R.id.layout);

        mSlider_bg = (ImageView)findViewById(R.id.slider_backgroud);
        mSlider = (ImageView)findViewById(R.id.slider);
        int shift =  getResources().getInteger(R.integer.miracast_sliderblock_shift);
        mAnimation = new TranslateAnimation(-shift, shift, 0, 0);
        mAnimation.setInterpolator(new AccelerateDecelerateInterpolator());
        mAnimation.setDuration(2000);
        mAnimation.setStartTime(0);
        mAnimation.setRepeatCount(Integer.MAX_VALUE);
        mAnimation.setRepeatMode(Animation.REVERSE);
        mSlider.startAnimation(mAnimation);

        try{
            mDeiveName = getResources().getString(R.string.default_device_name);
        } catch (Exception e) {
            e.printStackTrace();

            String name = SystemProperties.get("ro.product.name", null);
            String[] tmp = name.split("_");
            mDeiveName = tmp[0];
            if (mDeiveName == null || mDeiveName.equals("")) Log.e(TAG, "get device name failed");
        }
        //String mac = getEthernetMac();
        //mDeiveName += "-";
        //mDeiveName += mac;
        mDeiveName = Settings.Global.getString(mContext.getContentResolver(), Settings.Global.DEVICE_NAME);
        Log.d(TAG, mDeiveName);
        mHint = (TextView)findViewById(R.id.state_hint);
        updateHint(true);

        mButton = (Button)findViewById(R.id.help);
        mButton.setFocusable(true);
        mButton.setFocusableInTouchMode(true);
        mButton.setBackgroundResource(R.drawable.sbutton_focus);
        mButton.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                // TODO Auto-generated method stub
                Log.d(TAG, "help button onClick");
                switch (mPageState) {
                case HOME_PAGE:
                    mPageState = HELP_PAGE;
                    showHelp();
                    break;
                default:
                    break;
                }
            }
        });

        initViewPager();

        mIntentFilter.addAction(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION);
        mIntentFilter.addAction(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION);
        mIntentFilter.addAction(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
        mIntentFilter.addAction(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION);
        mIntentFilter.addAction(WifiP2pManager.WIFI_P2P_DISCOVERY_CHANGED_ACTION);
    }

    @Override
    public void onResume() {
        super.onResume();

        registerReceiver(mReceiver, mIntentFilter);

        mWifiP2pManager = (WifiP2pManager)getSystemService(Context.WIFI_P2P_SERVICE);
        if (mWifiP2pManager != null) {
            mChannel = mWifiP2pManager.initialize(this, getMainLooper(), null);
            if (mChannel == null) {
                // Failure to set up connection
                Log.e(TAG, "Failed to set up connection with wifi p2p service");
            } else {
                mWifiP2pManager.removeGroup(mChannel, new WifiP2pManager.ActionListener() {
                    public void onSuccess() {
                        Log.d(TAG, "removeGroup success");
                    }
                    public void onFailure(int reason) {
                        Log.d(TAG, "removeGroup fail=" + reason);
                    }
                });

                mWifiP2pManager.setDeviceName(mChannel, mDeiveName, new WifiP2pManager.ActionListener() {
                    public void onSuccess() {
                        Log.d(TAG, " device rename success");
                    }
                    public void onFailure(int reason) {
                        Toast.makeText(Miracast.this, R.string.wifi_p2p_failed_rename_message, Toast.LENGTH_LONG).show();
                    }
                });

                WifiP2pWfdInfo wifiP2pWfdInfo = new WifiP2pWfdInfo();
                wifiP2pWfdInfo.setWfdEnabled(true);
                //wifiP2pWfdInfo.setEnabled(true);
                wifiP2pWfdInfo.setDeviceType(WifiP2pWfdInfo.PRIMARY_SINK);
                //wifiP2pWfdInfo.setDeviceType(WifiP2pWfdInfo.DEVICE_TYPE_PRIMARY_SINK);
                wifiP2pWfdInfo.setSessionAvailable(true);
                wifiP2pWfdInfo.setControlPort(0); // not accept rtsp connection
                wifiP2pWfdInfo.setMaxThroughput(50 * 1); // TODO, 50Mbps * 1
                mWifiP2pManager.setWFDInfo(mChannel, wifiP2pWfdInfo, new WifiP2pManager.ActionListener() {
                //mWifiP2pManager.setWfdInfo(mChannel, wifiP2pWfdInfo, new WifiP2pManager.ActionListener() {
                    public void onSuccess() {
                        Log.d(TAG, "setWFDInfo success");
                    }
                    public void onFailure(int reason) {
                        Log.d(TAG, "setWFDInfo failed");
                    }
                });

                startBroadcastWFDSink(); // TODO, not a new thread
            }
        } else {
            Log.e(TAG, "mWifiP2pManager is null");
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    public void onPause() {

        if (mWFDSinkView != null) {
            mWFDSinkView.reset();
            ViewParent p = mWFDSinkView.getParent();
            if (p instanceof ViewGroup) {
                ViewGroup group = (ViewGroup)p;
                Log.i(TAG, "remove WFDSinkView from window");
                group.removeView(mWFDSinkView);
            } else {
                Log.w(TAG, "wfdsinkview parent is null");
            }
            mWFDSinkView = null;
            mConetentView.setVisibility(View.VISIBLE);
        }

        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        // TODO
        Message message = new Message();
        message.what = MIRACAST_STOP;
        mHandler.sendMessage(message);

        unregisterReceiver(mReceiver);

        stopBroadcastWFDSink();
        super.onPause();
    }

/*
    private final Runnable mBroadcastWFDSink = new Runnable() {
        @Override
        public void run() {
            startBroadcastWFDSink();
        }
    };
*/

    private void startBroadcastWFDSink() {
        Log.d(TAG, "startBroadcastWFDSink");

/*
        if (mWifiP2pManager != null && mChannel != null) {
            mWifiP2pManager.discoverPeers(mChannel, null);
        }

        // Broadcast peers periodically until stopped.
        mHandler.postDelayed(mBroadcastWFDSink, 1000 * 60); // 60s
*/
/*
        TimerTask broadcastWFDSink = new TimerTask() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                if (mWifiP2pManager != null && mChannel != null) {
                    mWifiP2pManager.discoverPeers(mChannel, null);
                }
            }
        };
        mTimer = new Timer(true);
        mTimer.schedule(broadcastWFDSink, 0, 1000 * 60);
*/
        Runnable broadcastWFDSink = new Runnable() {
            @Override
            public void run() {
                if (mWifiP2pManager != null && mChannel != null) {
                    mWifiP2pManager.discoverPeers(mChannel, null);
                }
            }
        };
        mScheduledExecutorService = Executors.newSingleThreadScheduledExecutor();
        //mScheduledExecutorService.scheduleAtFixedRate(broadcastWFDSink, 0, 1, TimeUnit.SECONDS);
        mScheduledExecutorService.scheduleWithFixedDelay(broadcastWFDSink, 0, 1000 * 60, TimeUnit.MILLISECONDS);
    }

    private void stopBroadcastWFDSink() {
        Log.d(TAG, "stopBroadcastWFDSink");

/*
        // Cancel automatic broadcast right away.
        mHandler.removeCallbacks(mBroadcastWFDSink);
*/
/*
        if (mTimer != null) {
            mTimer.cancel();
            mTimer = null;
        }
*/
        if (mScheduledExecutorService != null) {
            //mScheduledExecutorService.shutdown();
            mScheduledExecutorService.shutdownNow();
            mScheduledExecutorService = null;
        }

        if (mWifiP2pManager != null && mChannel != null) {
            mWifiP2pManager.stopPeerDiscovery(mChannel, null);
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return false;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (mWFDSinkView != null) {
            int orientation = -1;
            // Checks the orientation of the screenRotation
            if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                Log.w(TAG, "onConfigurationChanged: landscape");
                orientation = 0;
            } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT){
                Log.w(TAG, "onConfigurationChanged: portrait");
                orientation = 1;
            }
            if (orientation != -1) {
                mWFDSinkView.setScreenRotationChanged(orientation);
            }
        }
    }

    @Override
    public boolean onKeyUp (int keyCode, KeyEvent event) {
        if (mPageState == HOME_PAGE) {
            // normal page & help button focused
            switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                finish();
                break;
            default:
                break;
            }
        } else if(mPageState == HOME_NOT_FOCUSED_PAGE) {
            // normal page & help button not focused
            switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                finish();
                break;
            case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_DPAD_RIGHT:
            case KeyEvent.KEYCODE_DPAD_LEFT:
                mPageState = HOME_PAGE;
                mButton.setBackgroundResource(R.drawable.sbutton_focus);
                break;
            default:
                break;
            }
        } else if(mPageState == HELP_PAGE) {
            // help page & manual operating
            switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                mPageState = HOME_PAGE;
                hideHelp();
                break;
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                mHelpStep += 1;
                if (mHelpStep < 0) mHelpStep = 0;
                if (mHelpStep > (mHelpViews.size() - 1)) mHelpStep = 0;
                mViewPager.setCurrentItem(mHelpStep);
                break;
            case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_DPAD_LEFT:
                mHelpStep -= 1;
                if (mHelpStep > (mHelpViews.size() - 1)) mHelpStep = mHelpViews.size() - 1;
                if(mHelpStep < 0) mHelpStep = mHelpViews.size() - 1;
                mViewPager.setCurrentItem(mHelpStep);
                break;
            default:
                break;
            }
        } else if(mPageState == SURFACEVIEW_PAGE) {
            // surfaceview page
            switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                mPageState = HOME_PAGE;
                Message message = new Message();
                message.what = MIRACAST_STOP;
                mHandler.sendMessage(message);
                break;
            default:
                break;
            }
        } else {
            Message message = new Message();
            message.what = MIRACAST_STOP;
            mHandler.sendMessage(message);
            hideHelp();
        }
        return false;
    }

    private void updateHint(boolean searching) {
        if (mHint != null) {
            if (searching) {
                mHint.setText(mDeiveName + "  " + this.getString(R.string.wifi_p2p_menu_wait));
            } else {
                mHint.setText(mPeerDevice.deviceName + "  " + this.getString(R.string.wifi_p2p_menu_connect));
            }
            mHint.setVisibility(View.VISIBLE);
        }
    }

    private boolean isEnglish() {
        Locale locale = getResources().getConfiguration().locale;
        String language = locale.getLanguage();
        if (language.endsWith("en"))
            return true;
        else
            return false;
    }

    private void showHelp() {
        mHint.setVisibility(View.GONE);
        mButton.setVisibility(View.GONE);
        mSlider_bg.setVisibility(View.GONE);
        mSlider.setVisibility(View.GONE);
        mSlider.clearAnimation();
        mTitle.setVisibility(View.GONE);
        if (mViewPager != null) mViewPager.setVisibility(View.VISIBLE);
        if (mLayout != null) mLayout.setBackgroundResource(R.drawable.miracast_help0);
    }

    private void hideHelp() {
        mHint.setVisibility(View.VISIBLE);
        mButton.setVisibility(View.VISIBLE);
        mSlider_bg.setVisibility(View.VISIBLE);
        mSlider.setVisibility(View.VISIBLE);
        mSlider.startAnimation(mAnimation);
        mTitle.setVisibility(View.VISIBLE);
        if (mViewPager != null) mViewPager.setVisibility(View.GONE);
        if (mLayout != null) mLayout.setBackgroundResource(R.drawable.sbackground);
    }

    private void initViewPager() {
        mViewPager = (ViewPager) findViewById(R.id.vpager);
        mHelpViews = new ArrayList<View>();

        View help0 = LayoutInflater.from(this).inflate(R.layout.help_step0, null);
        mHelpViews.add(help0);
        View help1 = LayoutInflater.from(this).inflate(R.layout.help_step1, null);
        mHelpViews.add(help1);
        View help2 = LayoutInflater.from(this).inflate(R.layout.help_step2, null);
        mHelpViews.add(help2);

        mViewPager.setAdapter(new MyHelpAdapter(mHelpViews));
        mViewPager.setOnPageChangeListener(new MyOnPageChangeListener());
        try {
            Field field = ViewPager.class.getDeclaredField("mScroller");
            field.setAccessible(true);
            mSpeedScroller = new MySpeedScroller(mViewPager.getContext(), new AccelerateInterpolator());
            field.set(mViewPager, mSpeedScroller);
        } catch (Exception e) {
            e.printStackTrace();
        }
        mSpeedScroller.setMyDuration(mScrollDelay);

        mHelpImageViews = new ImageView[mHelpViews.size()];
        LinearLayout vg = (LinearLayout) findViewById(R.id.viewGroup);
        TextView tv = new TextView(vg.getContext());

        for (int i = 0; i < mHelpViews.size(); i++) {
            ImageView iv = new ImageView(this);
            int dot= getResources().getInteger(R.integer.miracast_dot);
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(dot, dot);
            int dot_pading= getResources().getInteger(R.integer.miracast_dot_pading);
            lp.setMargins(dot_pading, 0, dot_pading, 0);
            iv.setLayoutParams(lp);
            //iv.setPadding(10, 0, 10, 0);
            mHelpImageViews[i] = iv;
            if (i == 0)
                mHelpImageViews[i].setBackgroundResource(R.drawable.dot_focus);
            else
                mHelpImageViews[i].setBackgroundResource(R.drawable.dot_unfocus);
            vg.addView(iv);
        }
        //mViewPager.setCurrentItem(1);
    }

    private final class MyHelpAdapter extends PagerAdapter{
        private List<View> mViews = null;

        public MyHelpAdapter(List<View> views) {
            mViews = views;
        }

        @Override
        public void destroyItem(View container, int position, Object object) {
            ((ViewPager)container).removeView(mViews.get(position));
        }

        @Override
        public int getCount() {
            return mViews.size();
        }

        @Override
        public Object instantiateItem(View container, int position) {
            ((ViewPager)container).addView(mViews.get(position), 0);
            return mViews.get(position);
        }

        @Override
        public boolean isViewFromObject(View view, Object object) {
            return view == object;
        }
    }

    private final class MySpeedScroller extends Scroller {
        private int mDuration = 1500;

        public MySpeedScroller(Context context) {
            super(context);
        }

        public MySpeedScroller(Context context, Interpolator interpolator) {
            super(context, interpolator);
        }

        @Override
        public void startScroll(int startX, int startY, int dx, int dy, int duration) {
            // Ignore received duration, use fixed one instead
            super.startScroll(startX, startY, dx, dy, mDuration);
        }

        @Override
        public void startScroll(int startX, int startY, int dx, int dy) {
            // Ignore received duration, use fixed one instead
            super.startScroll(startX, startY, dx, dy, mDuration);
        }

        public void setMyDuration(int time) {
            mDuration = time;
        }

        public int getMyDuration() {
            return mDuration;
        }
    }

    private final class MyOnPageChangeListener implements OnPageChangeListener {
        @Override
        public void onPageScrollStateChanged(int mPageState) {
            // TODO Auto-generated method stub
            //mPageState %= list.size();
        }

        @Override
        public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
            // TODO Auto-generated method stub
        }

        @Override
        public void onPageSelected(int position) {
            if (mHelpViews == null || mHelpViews.size() < 1 || mHelpImageViews == null) return;

            if (position > mHelpViews.size()) {
                position %= mHelpViews.size();
            }

            Log.d(TAG, " onPageSelected realItem = " + position);

            for (int i = 0; i < mHelpImageViews.length; i++) {
                if (position == i)
                    mHelpImageViews[i].setBackgroundResource(R.drawable.dot_focus);
                else
                    mHelpImageViews[i].setBackgroundResource(R.drawable.dot_unfocus);
            }
        }
    }
}

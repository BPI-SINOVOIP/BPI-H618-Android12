package com.softwinner.tvdsetting.net.wifi;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.content.IntentFilter;

import com.softwinner.tvdsetting.DisplaySetting;
import com.softwinner.tvdsetting.R;
import com.softwinner.tvdsetting.Settings;
import com.softwinner.tvdsetting.net.NetSettings;
import com.softwinner.tvdsetting.net.wifi.WifiApEnableDialog.WifiApEnableDialogInterface;

import android.content.BroadcastReceiver;
import android.os.Handler;
import android.net.wifi.WifiManager;
import java.util.List;
import android.content.Intent;
import android.net.NetworkInfo.DetailedState;
import android.os.Message;
import java.util.HashMap;
import android.util.Log;
import static android.net.wifi.WifiConfiguration.INVALID_NETWORK_ID;
import android.widget.Toast;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiConfiguration;
import android.net.NetworkInfo;
import android.net.wifi.SupplicantState;
import java.util.ArrayList;
import android.net.wifi.ScanResult;
import java.util.Collection;
import java.util.Collections;
import java.util.concurrent.atomic.AtomicBoolean;

import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.AdapterView;

public class WifiSettingActivity extends Activity implements ListView.OnItemClickListener,ListView.OnKeyListener{

    private final String TAG = "WifiSettingActivity";
    Context mContext;
    ListView mListView;

    public static final String LAUNCHER_FIRST_START_FLAG = "com.softwinner.tvdsetting.WifiSettingActivity.FIRST_START_FLAG";
    private IntentFilter mFilter;
    private BroadcastReceiver mReceiver;
    private Scanner mScanner;
    private WifiManager.ActionListener mConnectListener;
    private WifiManager.ActionListener mSaveListener;
    private WifiManager.ActionListener mForgetListener;
    private WifiManager mWifiManager;
    private AccessPoint mSelectedAccessPoint;
    private DetailedState mLastState;
    private WifiInfo mLastInfo;
    private boolean mlauncherFirstStart = false;
    WifiListAdapter mWifiListAdapter;
    ArrayList<AccessPoint> apList = new ArrayList();

    private final AtomicBoolean mConnected = new AtomicBoolean(false);

    private static final int WIFI_RESCAN_INTERVAL_MS = 10 * 1000;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        mlauncherFirstStart = getIntent().getBooleanExtra(LAUNCHER_FIRST_START_FLAG, false);
        this.setContentView(R.layout.wifilist);
        mListView = (ListView)this.findViewById(R.id.wifilist);
        TextView title = (TextView)this.findViewById(R.id.title);
        LinearLayout titleText = (LinearLayout)this.findViewById(R.id.title_text);
        if(mlauncherFirstStart){
            title.setVisibility(View.GONE);
            titleText.setVisibility(View.VISIBLE);
        }
        mWifiListAdapter = new WifiListAdapter(apList);
        mListView.setOverScrollMode(View.OVER_SCROLL_NEVER);
        mListView.setAdapter(mWifiListAdapter);
        mListView.setOnItemClickListener(this);
        mListView.setOnKeyListener(this);

        mConnectListener = new WifiManager.ActionListener() {
            @Override
            public void onSuccess() {
            }
            @Override
            public void onFailure(int reason) {
                Toast.makeText(mContext,
                        R.string.wifi_failed_connect_message,
                        Toast.LENGTH_SHORT).show();
            }
        };

        mSaveListener = new WifiManager.ActionListener() {
            @Override
            public void onSuccess() {
            }
            @Override
            public void onFailure(int reason) {
                Toast.makeText(mContext,
                        R.string.wifi_failed_save_message,
                        Toast.LENGTH_SHORT).show();
            }
        };

        mForgetListener = new WifiManager.ActionListener() {
            @Override
            public void onSuccess() {
            }
            @Override
            public void onFailure(int reason) {
                Toast.makeText(mContext,
                        R.string.wifi_failed_forget_message,
                        Toast.LENGTH_SHORT).show();
            }
        };

        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        if(mWifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_ENABLED){
            final WifiApEnableDialog waDialog = new WifiApEnableDialog(mContext,R.style.CommonDialog,
                    this.getString(R.string.wifiworning),this.getString(R.string.currentwifistat));

            WifiApEnableDialogInterface waInterface = new WifiApEnableDialogInterface(){
                @Override
                public void onButtonYesClick() {
                    // TODO Auto-generated method stub
                    Intent intent = new Intent();
                    intent.setClass(WifiSettingActivity.this, SoftapActivity.class);
                    startActivity(intent);
                    waDialog.dismiss();
                }

                @Override
                public void onButtonNoClick() {
                    // TODO Auto-generated method stub
                    waDialog.dismiss();
                }
            };

            waDialog.setWifiApEnableDialogInterface(waInterface);
            waDialog.show();
        }
        mFilter = new IntentFilter();
        mFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
        mFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        mFilter.addAction(WifiManager.NETWORK_IDS_CHANGED_ACTION);
        mFilter.addAction(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
        mFilter.addAction(WifiManager.CONFIGURED_NETWORKS_CHANGED_ACTION);
        mFilter.addAction(WifiManager.LINK_CONFIGURATION_CHANGED_ACTION);
        mFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        mFilter.addAction(WifiManager.RSSI_CHANGED_ACTION);

        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                handleEvent(context, intent);
            }
        };

        mScanner = new Scanner();
        int wifiState = mWifiManager.getWifiState();
        if ((wifiState == WifiManager.WIFI_STATE_DISABLING) ||
                    (wifiState == WifiManager.WIFI_STATE_DISABLED)) {
            mWifiManager.setWifiEnabled(true);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) {
        // TODO Auto-generated method stub

        mSelectedAccessPoint = (AccessPoint) mWifiListAdapter.getItem(position);
        DetailedState state = mSelectedAccessPoint.getState();
        if(state == DetailedState.DISCONNECTED || state == DetailedState.DISCONNECTING || state == null) {
            if (mSelectedAccessPoint.security == AccessPoint.SECURITY_NONE &&
                    mSelectedAccessPoint.networkId == INVALID_NETWORK_ID) {
                mSelectedAccessPoint.generateOpenNetworkConfig();
                mWifiManager.connect(mSelectedAccessPoint.getConfig(), mConnectListener);
            } else if(mSelectedAccessPoint.networkId != INVALID_NETWORK_ID) {
                mWifiManager.connect(mSelectedAccessPoint.networkId, mConnectListener);
            } else {
                Intent intent = new Intent();
                intent.putExtra(WifiConnectActivity.HIDE_TITLE, mlauncherFirstStart);
                intent.putExtra(WifiConnectActivity.ACCESS_POINT, mSelectedAccessPoint);
                intent.setClass(WifiSettingActivity.this, WifiConnectActivity.class);
                startActivityForResult(intent, 0);
            }
        } else if(state == DetailedState.CONNECTED || state == DetailedState.CONNECTING) {
            Intent intent = new Intent();
            intent.putExtra(WifiDisconnectActivity.HIDE_TITLE, mlauncherFirstStart);
            intent.setClass(WifiSettingActivity.this, WifiDisconnectActivity.class);
            intent.putExtra(WifiDisconnectActivity.ACCESS_POINT, mSelectedAccessPoint);
            startActivityForResult(intent, 1);
        }
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(data!=null){
            if (resultCode == 0) {
                WifiConfiguration config = data.getParcelableExtra(WifiConnectActivity.PASSWORD);
                submit(config);
            } else if(resultCode == 1) {
                forget();
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        registerReceiver(mReceiver, mFilter);
        updateAccessPoints();
    }

    @Override
    public void onPause() {
        super.onPause();

        unregisterReceiver(mReceiver);
        mScanner.pause();
    }

    private void updateAItem(AccessPoint ap) {
        mWifiListAdapter.notifyDataSetChanged();
    }

    /**
     * Shows the latest access points available with supplimental information like
     * the strength of network and the security for it.
     */
    private void updateAccessPoints() {
        final int wifiState = mWifiManager.getWifiState();

        switch (wifiState) {
            case WifiManager.WIFI_STATE_ENABLED:
                // AccessPoints are automatically sorted with TreeSet.
                final Collection<AccessPoint> accessPoints = constructAccessPoints();
                apList.clear();
                updateAItem(null);
                for (AccessPoint accessPoint : accessPoints) {
                    apList.add(accessPoint);
                    updateAItem(accessPoint);
                }
                break;
        }
    }



    /** Returns sorted list of access points */
    private List<AccessPoint> constructAccessPoints() {
        ArrayList<AccessPoint> accessPoints = new ArrayList<AccessPoint>();
        /** Lookup table to more quickly update AccessPoints by only considering objects with the
         * correct SSID.  Maps SSID -> List of AccessPoints with the given SSID.  */
        Multimap<String, AccessPoint> apMap = new Multimap<String, AccessPoint>();

        final List<WifiConfiguration> configs = mWifiManager.getConfiguredNetworks();
        if (configs != null) {
            for (WifiConfiguration config : configs) {
                AccessPoint accessPoint = new AccessPoint(this, config);
                accessPoint.update(mLastInfo, mLastState);
                accessPoints.add(accessPoint);
                apMap.put(accessPoint.ssid, accessPoint);
            }
        }

        final List<ScanResult> results = mWifiManager.getScanResults();
        if (results != null) {
            for (ScanResult result : results) {
                // Ignore hidden and ad-hoc networks.
                if (result.SSID == null || result.SSID.length() == 0 ||
                        result.capabilities.contains("[IBSS]")) {
                    continue;
                }

                boolean found = false;
                for (AccessPoint accessPoint : apMap.getAll(result.SSID)) {
                    if (accessPoint.update(result))
                        found = true;
                }
                if (!found) {
                    AccessPoint accessPoint = new AccessPoint(this, result);
                    accessPoints.add(accessPoint);
                    apMap.put(accessPoint.ssid, accessPoint);
                }
            }
        }

        // Pre-sort accessPoints to speed preference insertion
        Collections.sort(accessPoints);
        return accessPoints;
    }

    /** A restricted multimap for use in constructAccessPoints */
    private class Multimap<K,V> {
        private final HashMap<K,List<V>> store = new HashMap<K,List<V>>();
        /** retrieve a non-null list of values with key K */
        List<V> getAll(K key) {
            List<V> values = store.get(key);
            return values != null ? values : Collections.<V>emptyList();
        }

        void put(K key, V val) {
            List<V> curVals = store.get(key);
            if (curVals == null) {
                curVals = new ArrayList<V>(3);
                store.put(key, curVals);
            }
            curVals.add(val);
        }
    }

    private void handleEvent(Context context, Intent intent) {
        String action = intent.getAction();
        if (WifiManager.WIFI_STATE_CHANGED_ACTION.equals(action)) {
            updateWifiState(intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE,
                    WifiManager.WIFI_STATE_UNKNOWN));
        } else if (WifiManager.SCAN_RESULTS_AVAILABLE_ACTION.equals(action) ||
                WifiManager.CONFIGURED_NETWORKS_CHANGED_ACTION.equals(action) ||
                WifiManager.LINK_CONFIGURATION_CHANGED_ACTION.equals(action)) {
            updateAccessPoints();
        } else if (WifiManager.SUPPLICANT_STATE_CHANGED_ACTION.equals(action)) {
            //Ignore supplicant state changes when network is connected
            //TODO: we should deprecate SUPPLICANT_STATE_CHANGED_ACTION and
            //introduce a broadcast that combines the supplicant and network
            //network state change events so the apps dont have to worry about
            //ignoring supplicant state change when network is connected
            //to get more fine grained information.
            SupplicantState state = (SupplicantState) intent.getParcelableExtra(
                    WifiManager.EXTRA_NEW_STATE);
            if (!mConnected.get() && SupplicantState.isHandshakeState(state)) {
                updateConnectionState(WifiInfo.getDetailedStateOf(state));
             } else {
                 // During a connect, we may have the supplicant
                 // state change affect the detailed network state.
                 // Make sure a lost connection is updated as well.
                 updateConnectionState(null);
             }
        } else if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            NetworkInfo info = (NetworkInfo) intent.getParcelableExtra(
                    WifiManager.EXTRA_NETWORK_INFO);
            mConnected.set(info.isConnected());
            updateAccessPoints();
            updateConnectionState(info.getDetailedState());
        } else if (WifiManager.RSSI_CHANGED_ACTION.equals(action)) {
            updateConnectionState(null);
        }
    }

    private void updateConnectionState(DetailedState state) {
        /* sticky broadcasts can call this when wifi is disabled */
        if (!mWifiManager.isWifiEnabled()) {
            mScanner.pause();
            return;
        }

        if (state == DetailedState.OBTAINING_IPADDR) {
            mScanner.pause();
        } else {
            mScanner.resume();
        }

        if(state == DetailedState.CONNECTED){
            Log.d(TAG,"connected ");
            if(mlauncherFirstStart)
                this.finish();
        }

        mLastInfo = mWifiManager.getConnectionInfo();
        if (state != null) {
            mLastState = state;
        }

        for (int i = mWifiListAdapter.getCount() - 1; i >= 0; --i) {
            // Maybe there's a WifiConfigPreference
            AccessPoint ap = (AccessPoint) mWifiListAdapter.getItem(i);
            ap.update(mLastInfo, mLastState);
        }
    }

    private void updateWifiState(int state) {
        Activity activity = (Activity)mContext;
        if (activity != null) {
            activity.invalidateOptionsMenu();
        }

        switch (state) {
            case WifiManager.WIFI_STATE_ENABLED:
                mScanner.resume();
                return; // not break, to avoid the call to pause() below
        }

        mLastInfo = null;
        mLastState = null;
        mScanner.pause();
    }

    class WifiListAdapter extends BaseAdapter{

        private ArrayList<AccessPoint> mList;

        public WifiListAdapter(ArrayList<AccessPoint> al){
            mList = al;
        }

        public void setList(ArrayList<AccessPoint> al){
            mList = al;
        }

        @Override
        public int getCount() {
            // TODO Auto-generated method stub
            return mList.size();
        }

        @Override
        public Object getItem(int arg0) {
            // TODO Auto-generated method stub
            return mList.get(arg0);
        }

        @Override
        public long getItemId(int arg0) {
            // TODO Auto-generated method stub
            return 0;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            // TODO Auto-generated method stub
            convertView=LayoutInflater.from(mContext).inflate(R.layout.wifilistitem, null);
            TextView ssid = (TextView)convertView.findViewById(R.id.wifiname);
            ImageView locker = (ImageView)convertView.findViewById(R.id.wifistatusicon);
            ImageView signal = (ImageView)convertView.findViewById(R.id.netitemicon);
            ssid.setText(mList.get(position).ssid);
            if(mList.get(position).getState() == DetailedState.CONNECTED) {
                locker.setImageResource(R.drawable.wifi_linked_ok);
            }
            else if(mList.get(position).security == AccessPoint.SECURITY_NONE) {
                locker.setImageResource(R.drawable.wifi_unlock);
            } else {
                locker.setImageResource(R.drawable.wifi_lock);
            }
            mList.get(position).setView(convertView);
            mList.get(position).bindView();
            return convertView;
        }
    }

    private class Scanner extends Handler {
        private int mRetry = 0;

        void resume() {
            if (!hasMessages(0)) {
                sendEmptyMessage(0);
            }
        }

        void forceScan() {
            removeMessages(0);
            sendEmptyMessage(0);
        }

        void pause() {
            mRetry = 0;
            removeMessages(0);
        }

        @Override
        public void handleMessage(Message message) {
            if (mWifiManager.startScan()) {
                mRetry = 0;
            } else if (++mRetry >= 3) {
                mRetry = 0;
                Activity activity = (Activity)mContext;
                if (activity != null) {
                    Toast.makeText(activity, R.string.wifi_fail_to_scan,
                            Toast.LENGTH_LONG).show();
                }
                return;
            }
            sendEmptyMessageDelayed(0, WIFI_RESCAN_INTERVAL_MS);
        }
    }

    /* package */ void submit(WifiConfiguration config) {
        if (config != null) {
            mWifiManager.connect(config, mConnectListener);
        }

        if (mWifiManager.isWifiEnabled()) {
            mScanner.resume();
        }
        updateAccessPoints();
    }

    /* package */ void forget() {
        if (mSelectedAccessPoint.networkId == INVALID_NETWORK_ID) {
            // Should not happen, but a monkey seems to triger it
            Log.e(TAG, "Failed to forget invalid network " + mSelectedAccessPoint.getConfig());
            return;
        }

        mWifiManager.forget(mSelectedAccessPoint.networkId, mForgetListener);

        if (mWifiManager.isWifiEnabled()) {
            mScanner.resume();
        }
        updateAccessPoints();
    }
    @Override
    public boolean onKeyDown (int keyCode, KeyEvent event){
        TextView title = (TextView) this.findViewById(R.id.title);
        switch(keyCode){
        case KeyEvent.KEYCODE_BACK:
            title.setText(R.string.wireless);
            break;
        }
        return false;
    }

    @Override
    public boolean onKeyUp (int keyCode, KeyEvent event){
        TextView title = (TextView) this.findViewById(R.id.title);
        switch(keyCode){
            case KeyEvent.KEYCODE_BACK:
                title.setText(R.string.wireless);
                finish();
                break;
        }
        return false;
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if(v.getId() == R.id.wifilist){
            boolean down = event.getAction()==KeyEvent.ACTION_DOWN?true:false;
            Log.d(TAG,"onKey down = " + down );
            if(down){
                switch(keyCode) {
                    case KeyEvent.KEYCODE_DPAD_RIGHT:
                    {
                        int position = mListView.getSelectedItemPosition();
                        if(position < 0 || position  > mWifiListAdapter.getCount()) {
                            return false;
                        }
                        mSelectedAccessPoint = (AccessPoint) mWifiListAdapter.getItem(position);
                        DetailedState state = mSelectedAccessPoint.getState();
                        if(mSelectedAccessPoint.getLevel() != -1
                                && mSelectedAccessPoint.getState() == null
                                && mSelectedAccessPoint.networkId == INVALID_NETWORK_ID) {
                            Intent intent = new Intent();
                            intent.putExtra(WifiConnectActivity.HIDE_TITLE, mlauncherFirstStart);
                            intent.putExtra(WifiConnectActivity.ACCESS_POINT, mSelectedAccessPoint);
                            intent.setClass(WifiSettingActivity.this, WifiConnectActivity.class);
                            startActivityForResult(intent, 0);
                        } else if(mSelectedAccessPoint.networkId != INVALID_NETWORK_ID) {
                            Intent intent = new Intent();
                            intent.putExtra(WifiDisconnectActivity.HIDE_TITLE, mlauncherFirstStart);
                            intent.setClass(WifiSettingActivity.this, WifiDisconnectActivity.class);
                            intent.putExtra(WifiDisconnectActivity.ACCESS_POINT, mSelectedAccessPoint);
                            startActivityForResult(intent, 1);
                        }
                    }
                    break;
                }
            }
        }
        return false;
    }
}
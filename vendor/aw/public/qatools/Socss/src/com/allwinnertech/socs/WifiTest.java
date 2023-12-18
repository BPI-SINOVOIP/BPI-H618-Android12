package com.allwinnertech.socs;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiConfiguration.AuthAlgorithm;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import com.allwinnertech.socs.wifi.AccessPoint;
import com.allwinnertech.socs.wifi.WifiDialog;

public class WifiTest extends Activity implements
        ListView.OnItemClickListener, AlertDialog.OnClickListener {

    /* WifiManager */
    private WifiManager mWifiManager;

    /* 扫描结果 */
    private List<ScanResult> results;

    /* 布局解析器 */
    private LayoutInflater mInflater;

    /* Wifi广播的Intent过滤器 */
    private IntentFilter mFilter;

    /* Wifi Ap 扫描器 */
    private Scanner mScanner;

    /* 扫描间隔 */
    private static final int WIFI_RESCAN_INTERVAL_MS = 5 * 1000;

    /* Wifi的原始状态，用于在测试完毕之后还原wifi状态 */
    private boolean mOriginWifiStatus;

    /* access point */
    private ArrayList<AccessPoint> accessPoint;

    /* Wifi 配置对话框 */
    private WifiDialog mWifiDialog;

    private String resultString;

    private String testname;

    CountDownTimer mCountDownTimer = null;

    private long overitme = 10;

    private boolean isPass = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle(R.string.case_wifi_name);
        onInitialize();
        //onCaseStarted();
        mCountDownTimer = getOverTime(overitme, overitme);
        mCountDownTimer.start();
    }

    @Override
    protected void onResume() {
        onCaseStarted();
        super.onResume();
    }

    @Override
    protected void onPause() {
        onCaseFinished();
        super.onPause();
    }

    @Override
    public void finish() {
        mCountDownTimer.cancel();
        //onCaseFinished();
        super.finish();
    }

    private void finishTest() {
        Intent returnIntend =new Intent();
        setResult(RESULT_OK,returnIntend);
        finish();
    }

    private WifiInfo mLastInfo;
    private DetailedState mLastState;

    private int maxRSSI = -65;
    public static final String PASSABLE_MAX_RSSI = "maxRSSI";

    /* 保存通过DragonFire配置的网络id，它们将会在应用退出时清除 */
    private final ArrayList<Integer> mNetworkIds = new ArrayList<Integer>();

    /* wifi ap 扫描器 */
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
                return;
            }
            sendEmptyMessageDelayed(0, WIFI_RESCAN_INTERVAL_MS);
        }
    }

    private WifiAdapter mWifiAdapter = new WifiAdapter();

    private class WifiAdapter extends BaseAdapter {

        private class ViewHolder {
            TextView ssid;
            ProgressBar level;
            TextView strength;
        }

        ArrayList<AccessPoint> mAps;

        public void setAccessPoints(ArrayList<AccessPoint> aps) {
            mAps = aps;
            notifyDataSetChanged();
        }

        @Override
        public int getCount() {
            return mAps != null ? mAps.size() : 0;
        }

        @Override
        public AccessPoint getItem(int position) {
            return mAps != null ? mAps.get(position) : null;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convert, ViewGroup root) {
            if (convert == null) {
                convert = mInflater.inflate(R.layout.wifi_item, null);
                ViewHolder holder = new ViewHolder();
                holder.ssid = (TextView) convert.findViewById(R.id.ssid);
                holder.level = (ProgressBar) convert.findViewById(R.id.level);
                holder.level.setMax(16);
                holder.strength = (TextView) convert.findViewById(R.id.strength);
                convert.setTag(holder);
            }
            ViewHolder holder = (ViewHolder) convert.getTag();
            holder.ssid.setText(getItem(position).ssid);
            int level = getItem(position).rssi != Integer.MIN_VALUE ? WifiManager
                    .calculateSignalLevel(getItem(position).rssi, 16) : 0;
            holder.level.setProgress(level);
            level = getItem(position).rssi != Integer.MIN_VALUE ? WifiManager
                    .calculateSignalLevel(getItem(position).rssi, 4) : 0;
            String strength = getItem(position).rssi == Integer.MIN_VALUE ? "不在范围"
                    : "" + getItem(position).rssi + " DB ";
            holder.strength.setText(strength);

            if((getItem(position).rssi != Integer.MIN_VALUE) && (getItem(position).rssi >= maxRSSI)){
                pass(getString(R.string.pass_wifi_head)+getItem(position).ssid+"#"+strength);
            }

            switch (getItem(position).connectionType) {
            case AccessPoint.CONNECTION_CONNECTED:
                convert.setBackgroundColor(Color.YELLOW);
                break;
            case AccessPoint.CONNECTION_CONNECTTING_FAILED:
                convert.setBackgroundColor(Color.RED);
                break;
            case AccessPoint.CONNECTION_UNKNOWN:
                convert.setBackgroundColor(Color.TRANSPARENT);
                break;
            }
            return convert;
        }

    }

    private BroadcastReceiver mWifiReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context arg0, Intent i) {
            String action = i.getAction();
            NetworkInfo networkInfo = (NetworkInfo) i
                    .getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
            mLastInfo = mWifiManager.getConnectionInfo();
            DetailedState state = networkInfo != null ? networkInfo
                    .getDetailedState() : null;
            if (state != null) {
                mLastState = state;
            }
            if (WifiManager.SCAN_RESULTS_AVAILABLE_ACTION.equals(action)
//                    || WifiManager.CONFIGURED_NETWORKS_CHANGED_ACTION
//                            .equals(action)
//                    || WifiManager.LINK_CONFIGURATION_CHANGED_ACTION
//                            .equals(action)
                    || WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
                accessPoint = updateAccessPoint();
//                if(accessPoint.size() > 0){
//                    setPassable(true);
//                }
                mWifiAdapter.setAccessPoints(accessPoint);
            } else if (WifiManager.SUPPLICANT_STATE_CHANGED_ACTION
                    .equals(action)) {

            }
        }
    };

    private class Multimap<K, V> {
        private HashMap<K, List<V>> store = new HashMap<K, List<V>>();

        /** retrieve a non-null list of values with key K */
        List<V> getAll(K key) {
            List<V> values = store.get(key);
            return values != null ? values : Collections.<V> emptyList();
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

    private ArrayList<AccessPoint> updateAccessPoint() {
        ArrayList<AccessPoint> accessPoints = new ArrayList<AccessPoint>();
        Multimap<String, AccessPoint> apMap = new Multimap<String, AccessPoint>();

        final List<WifiConfiguration> configs = mWifiManager
                .getConfiguredNetworks();
        // 从Wifi配置中获得access point的信息
        if (configs != null) {
            for (WifiConfiguration config : configs) {
                AccessPoint accessPoint = new AccessPoint(this, config);
                accessPoint.update(mLastInfo, mLastState);
                accessPoints.add(accessPoint);
                apMap.put(accessPoint.ssid, accessPoint);
            }
        }

        // 从Wifi扫描结果中获得access point的信息
        final List<ScanResult> results = mWifiManager.getScanResults();
        if (results != null) {
            for (ScanResult result : results) {
                // Ignore hidden and ad-hoc networks.
                if (result.SSID == null || result.SSID.length() == 0
                        || result.capabilities.contains("[IBSS]")) {
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

    @Override
    public void onItemClick(AdapterView<?> arg0, View arg1, int position,
            long id) {
        AccessPoint ap = mWifiAdapter.getItem(position);
        if (ap.security == AccessPoint.SECURITY_NONE) {
            WifiConfiguration config = getConfig(ap);
            config.allowedKeyManagement.set(KeyMgmt.NONE);
            connect(config);
        } else {
            //from hsm
        //    mWifiDialog = new WifiDialog(mContext, ap, this);
        //    mWifiDialog.show();
        }
    }

    private WifiConfiguration getConfig(AccessPoint ap) {
        WifiConfiguration config = new WifiConfiguration();
        config.SSID = AccessPoint.convertToQuotedString(ap.ssid);

        switch (ap.security) {
        case AccessPoint.SECURITY_NONE:
            config.allowedKeyManagement.set(KeyMgmt.NONE);
            break;

        case AccessPoint.SECURITY_WEP:
            config.allowedKeyManagement.set(KeyMgmt.NONE);
            config.allowedAuthAlgorithms.set(AuthAlgorithm.OPEN);
            config.allowedAuthAlgorithms.set(AuthAlgorithm.SHARED);
            String password = mWifiDialog.getPassword();
            int length = password.length();
            // WEP-40, WEP-104, and 256-bit WEP (WEP-232?)
            if ((length == 10 || length == 26 || length == 58)
                    && password.matches("[0-9A-Fa-f]*")) {
                config.wepKeys[0] = password;
            } else {
                config.wepKeys[0] = '"' + password + '"';
            }
            break;

        case AccessPoint.SECURITY_PSK:
            config.allowedKeyManagement.set(KeyMgmt.WPA_PSK);

            password = mWifiDialog.getPassword();
            length = password.length();
            if (password.matches("[0-9A-Fa-f]{64}")) {
                config.preSharedKey = password;
            } else {
                config.preSharedKey = '"' + password + '"';
            }
            break;

        /* 暂时不支持使用EAP加密的wifi节点 */
        /*
         * case AccessPoint.SECURITY_EAP:
         * config.allowedKeyManagement.set(KeyMgmt.WPA_EAP);
         * config.allowedKeyManagement.set(KeyMgmt.IEEE8021X);
         * config.eap.setValue((String) mEapMethodSpinner.getSelectedItem());
         *
         * config.phase2.setValue((mPhase2Spinner.getSelectedItemPosition() ==
         * 0) ? "" : "auth=" + mPhase2Spinner.getSelectedItem());
         * config.ca_cert.setValue((mEapCaCertSpinner.getSelectedItemPosition()
         * == 0) ? "" : KEYSTORE_SPACE + Credentials.CA_CERTIFICATE + (String)
         * mEapCaCertSpinner.getSelectedItem());
         * config.client_cert.setValue((mEapUserCertSpinner
         * .getSelectedItemPosition() == 0) ? "" : KEYSTORE_SPACE +
         * Credentials.USER_CERTIFICATE + (String)
         * mEapUserCertSpinner.getSelectedItem());
         * config.private_key.setValue((mEapUserCertSpinner
         * .getSelectedItemPosition() == 0) ? "" : KEYSTORE_SPACE +
         * Credentials.USER_PRIVATE_KEY + (String)
         * mEapUserCertSpinner.getSelectedItem());
         * config.identity.setValue((mEapIdentityView.length() == 0) ? "" :
         * mEapIdentityView.getText().toString());
         * config.anonymous_identity.setValue((mEapAnonymousView.length() == 0)
         * ? "" : mEapAnonymousView.getText().toString()); if
         * (mPasswordView.length() != 0) {
         * config.password.setValue(mPasswordView.getText().toString()); }
         * break;
         */

        default:
            return null;
        }
//        config.proxySettings = ProxySettings.UNASSIGNED;
//        config.ipAssignment = IpAssignment.UNASSIGNED;
//        config.linkProperties = new LinkProperties();

        return config;
    }

    @Override
    public void onClick(DialogInterface arg0, int button) {
        if (button == DialogInterface.BUTTON_POSITIVE) {
            AccessPoint wifiInfo = mWifiDialog.getWifiInfo();
            WifiConfiguration config = getConfig(wifiInfo);
            connect(config);
        }
    }

    private void connect(WifiConfiguration config) {
        int networkId = mWifiManager.addNetwork(config);
        mWifiManager.enableNetwork(networkId, true);
        mNetworkIds.add(networkId);
    }

    protected void onInitialize() {

        setContentView(R.layout.case_wifi);
        maxRSSI = -65;
        ListView lv = (ListView)findViewById(R.id.wifi_list);
        mInflater = LayoutInflater.from(this);
        mWifiManager = (WifiManager) this
                .getSystemService(Context.WIFI_SERVICE);
        lv.setAdapter(mWifiAdapter);
        lv.setOnItemClickListener(this);

        // 初始化Wifi的IntentFilter
        mFilter = new IntentFilter();
        mFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
        mFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        mFilter.addAction(WifiManager.NETWORK_IDS_CHANGED_ACTION);
        mFilter.addAction(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
//        mFilter.addAction(WifiManager.CONFIGURED_NETWORKS_CHANGED_ACTION);
//        mFilter.addAction(WifiManager.LINK_CONFIGURATION_CHANGED_ACTION);
        mFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        mFilter.addAction(WifiManager.RSSI_CHANGED_ACTION);

        // 打开wifi并保存wifi的状态
        final WifiManager wm = (WifiManager) this
                .getSystemService(Context.WIFI_SERVICE);
        mOriginWifiStatus = wm.isWifiEnabled();
        new Thread(new Runnable() {
            @Override
            public void run() {
                wm.setWifiEnabled(true);
            }
        }).start();
    }

    protected boolean onCaseStarted() {
        // 注册Wifi广播监听器
        try{
            Log.v("wifi", "unregisterReceiver");
            this.registerReceiver(mWifiReceiver, mFilter);
        }catch(Exception e){
            Log.v("wifi", "dragonFire close"+e.getMessage());
        }
        // 启动AP扫描器
        mScanner = new Scanner();
        mScanner.forceScan();
        return false;
    }

    protected void onCaseFinished() {
        try{
            this.unregisterReceiver(mWifiReceiver);
        }catch(Exception e){
            e.printStackTrace();
        }
        if(mScanner!=null)
            mScanner.pause();
        mScanner = null;
    }

    protected void onRelease() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                mWifiManager.setWifiEnabled(mOriginWifiStatus);
                for (Integer i : mNetworkIds) {
                    mWifiManager.removeNetwork(i);
                }
            }
        }).start();
    }
    void fail(String msg) {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("测试失败");
        builder.setMessage(msg);
        builder.setPositiveButton("确定", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                }
        });
        builder.create().show();
     }

    void pass(String msg) {
    if (isPass)
        return;
    isPass = true;
        mCountDownTimer.cancel();
        mCountDownTimer = getShowTime(5, 5);
        mCountDownTimer.start();

    }

    private CountDownTimer getOverTime(long secondInFuture,long countDownInterval){
        return new CountDownTimer(secondInFuture*1000, countDownInterval*1000) {

            public void onTick(long arg0) {
            }

            public void onFinish() {
                fail("测试超时");
            }
        };
    }

    private CountDownTimer getShowTime(long secondInFuture,long countDownInterval){
        return new CountDownTimer(secondInFuture*1000, countDownInterval*1000) {

            public void onTick(long arg0) {
            }

            public void onFinish() {
                finishTest();
            }
        };
    }
}

package com.softwinner.tvdsetting.net;

import com.softwinner.tvdsetting.R;
import android.app.Activity;
import android.os.Bundle;
import android.view.KeyEvent;
import android.widget.ImageView;
import android.net.wifi.WifiManager;
import android.net.ethernet.EthernetManager;
import android.content.Context;
import android.net.ConnectivityManager;
import android.content.BroadcastReceiver;
import android.widget.TextView;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.NetworkInfo;
import android.net.DhcpInfo;

public class NetStatsActivity extends Activity {

    TextView netType;
    TextView macAddress;
    TextView ipAddress;
    TextView netmask;
    TextView gateway;
    TextView dns;
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.setContentView(R.layout.netstat);
        netType = (TextView)this.findViewById(R.id.net_type);
        macAddress = (TextView)this.findViewById(R.id.mac_address);
        ipAddress = (TextView)this.findViewById(R.id.ip_address);
        netmask = (TextView)this.findViewById(R.id.netmask);
        gateway = (TextView)this.findViewById(R.id.gateway);
        dns = (TextView)this.findViewById(R.id.dns);
        filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
   }

    private IntentFilter filter = new IntentFilter();
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            updateView();
        }
    };

    @Override
    protected void onResume() {
        super.onResume();
        updateView();
        registerReceiver(mReceiver, filter);
    }

    @Override
    public void onPause() {
        super.onPause();
        unregisterReceiver(mReceiver);
    }

    private void updateView() {
        final ConnectivityManager connectivity = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivity != null) {
            final NetworkInfo info = connectivity.getActiveNetworkInfo();
            final boolean isConnected = (info != null) && info.isConnected();
            if(isConnected) {
                final int type = info.getType();
                DhcpInfo mDhcpInfo = new DhcpInfo();
                if(type == ConnectivityManager.TYPE_ETHERNET) {
                    EthernetManager mEthManager = EthernetManager.getInstance();
                    mDhcpInfo = mEthManager.getDhcpInfo();
                    netType.setText(R.string.ethernet_connected);
                    macAddress.setText(mEthManager.getDevInfo().getHwaddr().toUpperCase());
                } else if(type == ConnectivityManager.TYPE_WIFI) {
                    WifiManager mWifiManager = (WifiManager)getSystemService(Context.WIFI_SERVICE);
                    mDhcpInfo = mWifiManager.getDhcpInfo();
                    netType.setText(R.string.wifi_connected);
                    macAddress.setText(mWifiManager.getConnectionInfo().getMacAddress().toUpperCase());
                }
                ipAddress.setText(intToIp(mDhcpInfo.ipAddress));
                netmask.setText(intToIp(mDhcpInfo.netmask));
                gateway.setText(intToIp(mDhcpInfo.gateway));
                dns.setText(intToIp(mDhcpInfo.dns1));
            } else {
                netType.setText(R.string.non_connected);
                macAddress.setText("");
                ipAddress.setText("");
                netmask.setText("");
                gateway.setText("");
                dns.setText("");
            }
        }
    }

    private String intToIp(int paramInt) {
        return (paramInt & 0xFF) + "." + (0xFF & paramInt >> 8) + "." + (0xFF & paramInt >> 16) + "."
                + (0xFF & paramInt >> 24);
    }

    @Override
    public boolean onKeyDown (int keyCode, KeyEvent event){
        TextView title = (TextView) this.findViewById(R.id.title);
        switch(keyCode){
            case KeyEvent.KEYCODE_BACK:
                title.setText(R.string.netinfo);
                break;
        }
        return false;
    }

    @Override
    public boolean onKeyUp (int keyCode, KeyEvent event){
        TextView title = (TextView) this.findViewById(R.id.title);
        switch(keyCode){
            case KeyEvent.KEYCODE_BACK:
                title.setText(R.string.netinfo);
                finish();
                break;
        }
        return false;
    }
}

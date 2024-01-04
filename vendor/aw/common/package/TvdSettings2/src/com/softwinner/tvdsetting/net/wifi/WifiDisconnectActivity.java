package com.softwinner.tvdsetting.net.wifi;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.EditText;
import android.net.wifi.WifiConfiguration;
import static android.net.wifi.WifiConfiguration.INVALID_NETWORK_ID;
import android.net.wifi.WifiConfiguration.AuthAlgorithm;
import android.net.IpConfiguration.IpAssignment;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.net.IpConfiguration.ProxySettings;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiEnterpriseConfig.Eap;
import android.net.wifi.WifiEnterpriseConfig.Phase2;
import com.softwinner.tvdsetting.widget.ExtImageButton;
import android.widget.Button;
import android.widget.TextView;
import android.widget.EditText;
import android.content.Intent;
import android.net.LinkProperties;
import android.util.Log;

import com.softwinner.tvdsetting.R;

public class WifiDisconnectActivity extends Activity implements View.OnClickListener {

    public static final String HIDE_TITLE = "com.softwinner.tvdsetting.HIDE_TITLE";
    public static final String ACCESS_POINT = "com.softwinner.tvdsetting.ACCESS_POINT";

    private boolean hide_title = false;
    private AccessPoint mAccessPoint;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.setContentView(R.layout.wifidisconnectactivity);
        hide_title = getIntent().getBooleanExtra(HIDE_TITLE, false);
        mAccessPoint = (AccessPoint)getIntent().getParcelableExtra(ACCESS_POINT);
        TextView title = (TextView)this.findViewById(R.id.title);
        LinearLayout title_text = (LinearLayout)this.findViewById(R.id.title_text);
        if(hide_title){
            title.setVisibility(View.GONE);
            title_text.setVisibility(View.VISIBLE);
        }
        if(mAccessPoint != null) {
            Button mSubmit = (Button)findViewById(R.id.static_submit);
            mSubmit.setOnClickListener(this);
            TextView mSsid = (TextView)findViewById(R.id.ssid);
            mSsid.setText(mAccessPoint.ssid);
            ImageView mSecurity = (ImageView)findViewById(R.id.security);
            if(mAccessPoint.security == AccessPoint.SECURITY_NONE) {
                mSecurity.setImageResource(R.drawable.wifi_unlock);
            } else {
                mSecurity.setImageResource(R.drawable.wifi_lock);
            }
            ImageView mSignal = (ImageView)findViewById(R.id.signal);
            int level = mAccessPoint.getLevel();
            if(level == -1) {
                mSignal.setImageDrawable(null);
            } else {
                mSignal.setImageLevel(level);
            }
            LinearLayout mwifiIpAddressLayout = (LinearLayout)findViewById(R.id.wifi_ip_address_layout);
            mwifiIpAddressLayout.setFocusable(true);
            mwifiIpAddressLayout.setFocusableInTouchMode(true);
            mwifiIpAddressLayout.requestFocus();
            mwifiIpAddressLayout.requestFocusFromTouch();
            TextView mIpAddress = (TextView)findViewById(R.id.wifi_ip_address);
            int ipAddress = 0;
            if(mAccessPoint.getInfo() != null) {
            ipAddress = mAccessPoint.getInfo().getIpAddress();
            }
            mIpAddress.setText(intToIp(ipAddress));
        }
    }

    public void onClick(View view) {
        Intent intent = new Intent();
        setResult(1, intent);
        finish();
    }

    private String intToIp(int paramInt) {
        return (paramInt & 0xFF) + "." + (0xFF & paramInt >> 8) + "." + (0xFF & paramInt >> 16) + "."
                + (0xFF & paramInt >> 24);
    }
}

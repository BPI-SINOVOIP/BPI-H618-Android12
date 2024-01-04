package com.softwinner.tvdsetting.net.wifi;

import com.softwinner.tvdsetting.R;
import com.softwinner.tvdsetting.net.wifi.SoftapActivity.SoftapListAdapter;

import android.app.AlertDialog;
import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.widget.ListView;
import com.softwinner.tvdsetting.widget.ExtImageButton;
import com.softwinner.tvdsetting.widget.ItemSelectView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.view.View;
import android.util.Log;
import android.view.KeyEvent;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.AuthAlgorithm;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.content.DialogInterface;
import android.net.wifi.WifiManager;
import android.content.Intent;
import com.softwinner.tvdsetting.widget.ItemSelectView;
import com.softwinner.tvdsetting.widget.ItemSelectView.ItemSwitch;
import android.widget.Toast;

public class SoftapSettingActivity extends Activity implements View.OnClickListener, View.OnFocusChangeListener{

    public static final int OPEN_INDEX = 0;
    public static final int WPA_INDEX = 1;
    public static final int WPA2_INDEX = 2;
    public static final String SOFTAP_CONFIG = "softap_config";
    private int mSecurityTypeIndex = OPEN_INDEX;

    private Context mContext;
    private LinearLayout mSsidLayout;
    private LinearLayout mSecurityLayout;
    private LinearLayout mPasswdLayout;
    private EditText mSsid;
    private ItemSelectView mSecurity;
    private EditText mPassword;
    private Button mSubmmit;
    private WifiManager mWifiManager;
    private WifiConfiguration mWifiConfig = null;
    ItemSelectView mIsv;
    private ImageView prev;
    private ImageView next;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        mWifiConfig = mWifiManager.getWifiApConfiguration();
        this.setContentView(R.layout.softapsetting);
        mSsidLayout =(LinearLayout) findViewById(R.id.ssid_layout);
        mSecurityLayout =(LinearLayout) findViewById(R.id.security_layout);
        mPasswdLayout =(LinearLayout) findViewById(R.id.passwd_layout);
        mSsid =  (EditText) findViewById(R.id.ssid);
        mSsid.setOnFocusChangeListener(this);
        mSecurity = (ItemSelectView) findViewById(R.id.security);
        mSecurity.setOnFocusChangeListener(this);
        ItemSwitch mItemSwitch = new ItemSwitch() {
            @Override
            public void onItemSwitchPrev(int pos,String text) {
                mSecurityTypeIndex = pos;
            }
            @Override
            public void onItemSwitchNext(int pos,String text) {
                mSecurityTypeIndex = pos;
            }
            @Override
            public void onItemSwitchCenter(int pos,String text) {}
        };
        mSecurity.setItemSwitch(mItemSwitch);
        mPassword = (EditText) findViewById(R.id.passwd);
        mPassword.setOnFocusChangeListener(this);
        mSubmmit = (Button) findViewById(R.id.submmit);
        mSubmmit.setOnClickListener(this);
        if(mWifiConfig != null) {
            mSsid.setText(mWifiConfig.SSID);
            mSecurityTypeIndex = getSecurityTypeIndex(mWifiConfig);
            mSecurity.setPosition(mSecurityTypeIndex);
            if (mSecurityTypeIndex == WPA_INDEX ||
                        mSecurityTypeIndex == WPA2_INDEX) {
                mPassword.setText(mWifiConfig.preSharedKey);
            }
        }
        prev = (ImageView) findViewById(R.id.ic_previous_item);
        next = (ImageView) findViewById(R.id.ic_next_item);
        prev.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				KeyEvent left = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_LEFT);
				mSecurity.dispatchKeyEvent(left);
			}
		});
        next.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				KeyEvent right = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_RIGHT);
				mSecurity.dispatchKeyEvent(right);
			}
		});
    }

    public static int getSecurityTypeIndex(WifiConfiguration wifiConfig) {
        if (wifiConfig.allowedKeyManagement.get(KeyMgmt.WPA_PSK)) {
            return WPA_INDEX;
        } else if (wifiConfig.allowedKeyManagement.get(KeyMgmt.WPA2_PSK)) {
            return WPA2_INDEX;
        }
        return OPEN_INDEX;
    }

    public WifiConfiguration getConfig() {

        WifiConfiguration config = new WifiConfiguration();

        /**
         * TODO: SSID in WifiConfiguration for soft ap
         * is being stored as a raw string without quotes.
         * This is not the case on the client side. We need to
         * make things consistent and clean it up
         */
        config.SSID = mSsid.getText().toString();

        switch (mSecurityTypeIndex) {
            case OPEN_INDEX:
                config.allowedKeyManagement.set(KeyMgmt.NONE);
                return config;

            case WPA_INDEX:
                config.allowedKeyManagement.set(KeyMgmt.WPA_PSK);
                config.allowedAuthAlgorithms.set(AuthAlgorithm.OPEN);
                if (mPassword.length() != 0) {
                    String password = mPassword.getText().toString();
                    config.preSharedKey = password;
                }
                return config;

            case WPA2_INDEX:
                config.allowedKeyManagement.set(KeyMgmt.WPA2_PSK);
                config.allowedAuthAlgorithms.set(AuthAlgorithm.OPEN);
                if (mPassword.length() != 0) {
                    String password = mPassword.getText().toString();
                    config.preSharedKey = password;
                }
                return config;
        }
        return null;
    }

    public void onClick(View view) {
        if(mSsid.length() == 0 || mPassword.length() == 0) {
            Toast.makeText(mContext,R.string.wifi_failed_save_message,Toast.LENGTH_SHORT).show();
            return;
        }
        mWifiConfig = getConfig();
        if (mWifiConfig != null) {
            /**
             * if soft AP is stopped, bring up
             * else restart with new config
             * TODO: update config on a running access point when framework support is added
             */
            if (mWifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_ENABLED) {
                mWifiManager.setWifiApEnabled(null, false);
                mWifiManager.setWifiApEnabled(mWifiConfig, true);
            } else {
                mWifiManager.setWifiApConfiguration(mWifiConfig);
            }
        }
        Intent intent = new Intent();
        intent.putExtra(SOFTAP_CONFIG, mWifiConfig);
        setResult(0, intent);
        finish();
    }

    @Override
    public void onFocusChange(View arg0, boolean hasFocus) {

        switch(arg0.getId()){
            case R.id.ssid:
                if (hasFocus) {
                    mSsidLayout.setBackgroundResource(R.drawable.input_btn_focus);
                } else {
                    mSsidLayout.setBackgroundResource(R.drawable.list_btn_nor);
                }
                break;
            case R.id.security:
                if (hasFocus) {
                    mSecurityLayout.setBackgroundResource(R.drawable.input_btn_focus);
                } else {
                    mSecurityLayout.setBackgroundResource(R.drawable.list_btn_nor);
                }
                break;
            case R.id.passwd:
                if (hasFocus) {
                    mPasswdLayout.setBackgroundResource(R.drawable.input_btn_focus);
                } else {
                    mPasswdLayout.setBackgroundResource(R.drawable.list_btn_nor);
                }
                break;
            default:
                break;
        }
    }

    @Override
    public boolean onKeyDown (int keyCode, KeyEvent event){
        TextView title = (TextView) this.findViewById(R.id.title);
        switch(keyCode){
            case KeyEvent.KEYCODE_BACK:
                title.setText(R.string.softap);
                break;
        }
        return false;
    }

    @Override
    public boolean onKeyUp (int keyCode, KeyEvent event){
        TextView title = (TextView) this.findViewById(R.id.title);
        switch(keyCode){
            case KeyEvent.KEYCODE_BACK:
                title.setText(R.string.softap);
                finish();
                break;
        }
        return false;
    }
}

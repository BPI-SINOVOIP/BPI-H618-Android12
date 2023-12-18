package com.softwinner.tvdsetting.net.wifi;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.EditText;
import android.net.wifi.WifiConfiguration;
import static android.net.wifi.WifiConfiguration.INVALID_NETWORK_ID;
import android.net.wifi.WifiConfiguration.AuthAlgorithm;
//import android.net.wifi.WifiConfiguration.IpAssignment;
import android.net.IpConfiguration.IpAssignment;
import android.net.LinkProperties;

import android.net.wifi.WifiConfiguration.KeyMgmt;
//import android.net.wifi.WifiConfiguration.ProxySettings;
import android.net.IpConfiguration.ProxySettings;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiEnterpriseConfig.Eap;
import android.net.wifi.WifiEnterpriseConfig.Phase2;

import com.softwinner.tvdsetting.widget.ExtImageButton;
import android.widget.TextView;
import android.widget.EditText;
import android.content.Intent;
import android.net.LinkProperties;
import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.util.Log;
import android.widget.Toast;
import android.content.Context;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;

import com.softwinner.tvdsetting.R;

public class WifiConnectActivity extends Activity implements View.OnFocusChangeListener, EditText.OnEditorActionListener {

    public static final String HIDE_TITLE = "com.softwinner.tvdsetting.HIDE_TITLE";
    public static final String ACCESS_POINT = "com.softwinner.tvdsetting.ACCESS_POINT";
    public static final String PASSWORD = "com.softwinner.tvdsetting.PASSWORD";

    private Context mContext;
    private int mAccessPointSecurity;
    private boolean hide_title = false;
    private AccessPoint mAccessPoint;
    private LinearLayout mWifiPasswdLayout;
    private EditText mPassword;
    private WifiConfiguration config;
    private CheckBox mCheckBox;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.setContentView(R.layout.wificonnectactivity);
        mContext = this;
        hide_title = getIntent().getBooleanExtra(HIDE_TITLE, false);
        mAccessPoint = (AccessPoint)getIntent().getParcelableExtra(ACCESS_POINT);
        TextView title = (TextView)this.findViewById(R.id.title);
        LinearLayout title_text = (LinearLayout)this.findViewById(R.id.title_text);
        if(hide_title){
            title.setVisibility(View.GONE);
            title_text.setVisibility(View.VISIBLE);
        }
        mWifiPasswdLayout = (LinearLayout)this.findViewById(R.id.wifi_passwd_layout);
        if(mAccessPoint != null) {
            config = mAccessPoint.getConfig();
            mPassword = (EditText)findViewById(R.id.wifi_passwd);
            mPassword.setTransformationMethod(HideReturnsTransformationMethod.getInstance());
            mPassword.setOnFocusChangeListener(this);
            mPassword.setOnEditorActionListener(this);
            mPassword.setOnClickListener(new View.OnClickListener() {
				
				@Override
				public void onClick(View arg0) {
					// TODO Auto-generated method stub
		            if (mPassword.length() == 0) {
		                Toast.makeText(mContext, R.string.wifi_failed_save_message, Toast.LENGTH_SHORT).show();
		                return ;
		            }
		            Intent intent = new Intent();
		            intent.putExtra(PASSWORD, getConfig());
		            setResult(0, intent);
		            finish();
				}
			});
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
            mAccessPointSecurity = (mAccessPoint == null) ? AccessPoint.SECURITY_NONE :
                    mAccessPoint.security;
        }
    }

    /* package */ WifiConfiguration getConfig() {
        if (mAccessPoint == null) {
            return null;
        }
        if(config == null) {
            config = new WifiConfiguration();
        }
        if (mAccessPoint.networkId == INVALID_NETWORK_ID) {
            config.SSID = AccessPoint.convertToQuotedString(
                    mAccessPoint.ssid);
        } else {
            config.networkId = mAccessPoint.networkId;
        }

        switch (mAccessPointSecurity) {
            case AccessPoint.SECURITY_NONE:
                config.allowedKeyManagement.set(KeyMgmt.NONE);
                break;

            case AccessPoint.SECURITY_WEP:
                config.allowedKeyManagement.set(KeyMgmt.NONE);
                config.allowedAuthAlgorithms.set(AuthAlgorithm.OPEN);
                config.allowedAuthAlgorithms.set(AuthAlgorithm.SHARED);
                if (mPassword.length() != 0) {
                    int length = mPassword.length();
                    String password = mPassword.getText().toString();
                    // WEP-40, WEP-104, and 256-bit WEP (WEP-232?)
                    if ((length == 10 || length == 26 || length == 58) &&
                            password.matches("[0-9A-Fa-f]*")) {
                        config.wepKeys[0] = password;
                    } else {
                        config.wepKeys[0] = '"' + password + '"';
                    }
                }
                break;

            case AccessPoint.SECURITY_PSK:
                config.allowedKeyManagement.set(KeyMgmt.WPA_PSK);
                if (mPassword.length() != 0) {
                    String password = mPassword.getText().toString();
                    if (password.matches("[0-9A-Fa-f]{64}")) {
                        config.preSharedKey = password;
                    } else {
                        config.preSharedKey = '"' + password + '"';
                    }
                }
                break;
            default:
                return null;
        }
        return config;
    }

    private String intToIp(int paramInt) {
        return (paramInt & 0xFF) + "." + (0xFF & paramInt >> 8) + "." + (0xFF & paramInt >> 16) + "."
                + (0xFF & paramInt >> 24);
    }

    @Override
    public void onFocusChange(View arg0, boolean hasFocus) {
        switch(arg0.getId()){
        case R.id.wifi_passwd:
            if (hasFocus) {
                mWifiPasswdLayout.setBackgroundResource(R.drawable.input_btn_focus);
            } else {
                mWifiPasswdLayout.setBackgroundResource(R.drawable.list_btn_nor);
            }
            break;
        default:
            break;
        }
    }

    @Override
    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if(actionId == EditorInfo.IME_ACTION_DONE){
            InputMethodManager imm = (InputMethodManager) v
                    .getContext().getSystemService(
                            Context.INPUT_METHOD_SERVICE);
            if (imm.isActive()) {
                imm.hideSoftInputFromWindow(
                        v.getApplicationWindowToken(), 0);
            }
            if (mPassword.length() == 0) {
                Toast.makeText(mContext, R.string.wifi_failed_save_message, Toast.LENGTH_SHORT).show();
                return true;
            }
            Intent intent = new Intent();
            intent.putExtra(PASSWORD, getConfig());
            setResult(0, intent);
            finish();
            return true;
        }
        return false;
    }
}

package com.allwinnertech.socs.wifi;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.allwinnertech.socs.R;

public class WifiDialog extends AlertDialog {

    private AccessPoint mAccessPoint;

    private EditText mEditText;
    private TextView mStrength;
    private TextView mSecurity;

    private DialogInterface.OnClickListener mListener;

    protected WifiDialog(Context context, AccessPoint ap,
            DialogInterface.OnClickListener listener) {
        super(context, R.style.Theme_WifiDialog);
        mAccessPoint = ap;
        mListener = listener;
        setButton(DialogInterface.BUTTON_POSITIVE, "确认", mListener);
        setButton(DialogInterface.BUTTON_NEGATIVE, "取消", mListener);
        setTitle(ap.ssid);
    }

    protected void onCreate(Bundle savedInstanceState){

        View view = getLayoutInflater().inflate(R.layout.wifi_dialog, null);
        setView(view);
        mEditText = (EditText) view.findViewById(R.id.password);
        mStrength = (TextView) view.findViewById(R.id.strength);
        mSecurity = (TextView) view.findViewById(R.id.security);
        mSecurity.setText(mAccessPoint.getSecurityString(mAccessPoint.security,
                mAccessPoint.pskType, true));

        String [] signal = getContext().getResources().getStringArray(R.array.wifi_signal);
        int signal_level = WifiManager.calculateSignalLevel(mAccessPoint.rssi, 4);
        mStrength.setText(signal[signal_level]);
        setInverseBackgroundForced(true);
        super.onCreate(savedInstanceState);
        if(mAccessPoint.security == AccessPoint.SECURITY_EAP){
            Toast.makeText(getContext(), "不支持使用EAP加密模式的ap", Toast.LENGTH_SHORT).show();
            cancel();
        }
    }

    public String getPassword(){
        String password = mEditText.getEditableText().toString();
        return password;
    }

    public AccessPoint getWifiInfo(){
        return mAccessPoint;
    }
}

package com.softwinner.tvdsetting.net;

import java.util.ArrayList;
import java.util.List;

import com.softwinner.tvdsetting.R;
import com.softwinner.tvdsetting.Settings;
import com.softwinner.tvdsetting.applications.AppManagerActivity;
import com.softwinner.tvdsetting.net.bluetooth.BluetoothListActivity;
import com.softwinner.tvdsetting.net.bluetooth.BluetoothSettingActivity;
import com.softwinner.tvdsetting.net.ethernet.EthernetSettingActivity;
import com.softwinner.tvdsetting.net.wifi.SoftapActivity;
import com.softwinner.tvdsetting.net.wifi.WifiSettingActivity;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.content.pm.PackageManager;
import android.content.BroadcastReceiver;
import android.os.SystemProperties;
import android.content.IntentFilter;
import com.softwinner.tvdsetting.net.bluetooth.Constant;

public class NetSettings extends Activity implements ListView.OnItemClickListener,ListView.OnItemSelectedListener{

    private static final int NETSETTING_BASE = 0;
    private static final int NETSETTING_WIRELESS = NETSETTING_BASE + 0;
    private static final int NETSETTING_ETHERNET = NETSETTING_BASE + 1;
    private static final int NETSETTING_SOTFAP = NETSETTING_BASE + 2;
    private static final int NETSETTING_BLUETOOTH = NETSETTING_BASE +3;
    private static final int NETSETTING_NETINFO = NETSETTING_BASE + 4;
    private boolean isNewFeature = false;
    private static final String TAG = "NetSettings";

    ListView mListView;
    Context mContext;
    List<ImageView> mImageList;

    private NetSettingItemAdapter mAdapter = null;

    private boolean hasBT = true;
    private String[] btModuleList = {"ap6210", "ap6330", "ap6335", "rtl8723bs", "ap6212", "rtl8723bu"};

    private String WIFI = "wifi";
    private String ETH = "eth";
    private String SOFTAP = "softap";
    private String BT = "bt";
    private String INFO = "info";

    private String[] itemsAll = {WIFI, ETH, SOFTAP, BT, INFO};
    private String[] itemsNoBt = {WIFI, ETH, SOFTAP, INFO};
    private String[] items = itemsAll;

    private BroadcastReceiver mUSBBTReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG,"getItem action = " +  action);
            if (action.equals(Constant.ACTION_USBBTDEV_CHANGE)) {
                //处理
                if(mAdapter != null){
                    mAdapter.notifyDataSetChanged();
                }
            }
        }
    };

    private boolean hasBluetooth() {
        String str = SystemProperties.get("wlan.hardware.info", "null");

        if (str.equals("null"))
            return false;
        if (str.equals("unknown"))
            return false;

        String[] val = str.split(":");
        for (String s : btModuleList) {
            if ((val[1]).equals(s)) {
                return true;
            }
        }
        return false;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.netsetting);
        mContext = this;
        isNewFeature = SystemProperties.getInt(Constant.BT_HAS_USB, 33)== 33?false:true;
        mListView = (ListView)this.findViewById(R.id.netitemlist);
        mAdapter = new NetSettingItemAdapter();
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(this);
        mListView.setOnItemSelectedListener(this);
        mImageList = new ArrayList();
    }


    @Override
    protected void onResume() {
        // TODO Auto-generated method stub

        hasBT = hasBluetooth();
        if (!hasBT) {
            items = itemsNoBt;
        } else {
            items = itemsAll;
        }

        IntentFilter filter = new IntentFilter();
        filter.addAction(Constant.ACTION_USBBTDEV_CHANGE);
        //注册广播
        registerReceiver(mUSBBTReceiver, filter);
        if(mAdapter != null){
            mAdapter.notifyDataSetChanged();
        }

        super.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
        unregisterReceiver(mUSBBTReceiver);
    }

    class NetSettingItemAdapter extends BaseAdapter{

        @Override
        public int getCount() {
            // TODO Auto-generated method stub
            return items.length;
        }

        @Override
        public Object getItem(int arg0) {
            // TODO Auto-generated method stub
            Log.d(TAG,"getItem arg0 = " +  arg0);
            return null;
        }

        @Override
        public long getItemId(int arg0) {
            // TODO Auto-generated method stub
            Log.d(TAG,"getItemId arg0 = " +  arg0);
            return 0;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            // TODO Auto-generated method stub
            convertView=LayoutInflater.from(mContext).inflate(R.layout.netsettingitem, null);
            TextView text = (TextView)convertView.findViewById(R.id.netsettingitemtext);
            ImageView image = (ImageView)convertView.findViewById(R.id.netitemicon);
            mImageList.add(image);

            if (items[position].equals(WIFI)) {
                text.setText(R.string.wireless);
                image.setImageResource(R.drawable.ic_wireless_working);
            } else if (items[position].equals(ETH)) {
                text.setText(R.string.ethernet);
                image.setImageResource(R.drawable.ic_ethernet_working);
            } else if (items[position].equals(SOFTAP)) {
                text.setText(R.string.softap);
                image.setImageResource(R.drawable.ic_hotspot_working);
            } else if (items[position].equals(BT)) {
                text.setText(R.string.bluetooth);
                image.setImageResource(R.drawable.ic_bluetooth_working);
            } else if (items[position].equals(INFO)) {
                text.setText(R.string.netinfo);
            }

            image.setVisibility(View.INVISIBLE);
            return convertView;
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id ) {
        // TODO Auto-generated method stub
        Log.d(TAG,"position = " + position);
        Intent intent = new Intent();

        if (items[position].equals(WIFI)) {
            intent.setClass(NetSettings.this, WifiSettingActivity.class);
            startActivity(intent);
        } else if (items[position].equals(ETH)) {
            intent.setClass(NetSettings.this, EthernetSettingActivity.class);
            startActivity(intent);
        } else if (items[position].equals(SOFTAP)) {
            intent.setClass(NetSettings.this, SoftapActivity.class);
            startActivity(intent);
        } else if (items[position].equals(BT)) {
            intent.setClass(NetSettings.this, BluetoothSettingActivity.class);
            startActivity(intent);
        } else if (items[position].equals(INFO)) {
            intent.setClass(NetSettings.this, NetStatsActivity.class);
            startActivity(intent);
        }
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        // TODO Auto-generated method stub
        for(int i=0;i<mImageList.size();i++){
            mImageList.get(i).setVisibility(View.INVISIBLE);
        }
        mImageList.get(position).setVisibility(View.VISIBLE);
    }

    @Override
    public void onNothingSelected(AdapterView<?> arg0) {
        // TODO Auto-generated method stub

    }
    @Override
    public boolean onKeyDown (int keyCode, KeyEvent event){
        TextView title = (TextView) this.findViewById(R.id.title);
        switch(keyCode){
        case KeyEvent.KEYCODE_BACK:
                title.setText(R.string.setting_network);
            break;
        }
        return false;
    }

    @Override
    public boolean onKeyUp (int keyCode, KeyEvent event){
        TextView title = (TextView) this.findViewById(R.id.title);
        switch(keyCode){
            case KeyEvent.KEYCODE_BACK:
                title.setText(R.string.setting_network);
                finish();
                break;
        }
        return false;
    }
}

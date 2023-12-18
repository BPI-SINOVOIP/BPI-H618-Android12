package com.softwinner.tvdsetting.net.wifi;

import com.softwinner.tvdsetting.R;


import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.widget.ImageView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.net.wifi.WifiConfiguration;
import android.content.DialogInterface;
import android.view.View;
import android.util.Log;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.ContentResolver;
import android.provider.Settings;

public class SoftapActivity extends Activity implements ListView.OnItemClickListener, ListView.OnKeyListener {
    Context mContext;
    ListView mListView;
    SoftapListAdapter mSoftapListAdapter;
    private int START_SOFTAP = 0;
    private int SET_SOFTAP = 1;
    private boolean mState;
    Drawable mLeftNor,mLeftPress,mRightNor,mRightPress;
    TextView text1;
    TextView text2;
    TextView smalltext;

    private WifiManager mWifiManager;
    private WifiConfiguration mWifiConfig = null;
    private IntentFilter mIntentFilter;

    private static final int SOFTAP_SUBTEXT = R.string.softap_subtext;
    protected static final int SET_TEXT_OPEN = 0;
    protected static final int SET_TEXT_CLOSE = 1;
    protected static final int SET_TEXT_OPENING = 2;
    protected static final int SET_TEXT_CLOSING = 3;
    private String[] mSecurityType = {"","WPA PSK", "WPA2 PSK"};
    boolean withStaEnabled;

    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg){
            //TextView text1 = null;
            if(null == mListView.getSelectedView()) {
            	//Fix me,just requestFocus maybe wrong
            	//mListView.getChildAt(START_SOFTAP).requestFocus();
            	//return null; //mouse click will lose focuse,
            	               //return here will stop UI refresh
            }
            //text1 = (TextView)mListView.getSelectedView().findViewById(R.id.text2);
            switch(msg.what){
            case SET_TEXT_OPEN:
                if(mSoftapListAdapter!=null){
                    mSoftapListAdapter.setSoftApStat(SET_TEXT_OPEN);
                    mSoftapListAdapter.notifyDataSetChanged();
                }
                //text1.setText(R.string.open);
                break;
            case SET_TEXT_CLOSE:
                if(mSoftapListAdapter!=null){
                    mSoftapListAdapter.setSoftApStat(SET_TEXT_CLOSE);
                    mSoftapListAdapter.notifyDataSetChanged();
                }
                //text1.setText(R.string.close);
                break;
            default:
                break;
            }
        }
    };
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d("SoftapActivity", "mReceiver");
            if (WifiManager.WIFI_AP_STATE_CHANGED_ACTION.equals(action)) {
                handleWifiApStateChanged(intent.getIntExtra(
                        WifiManager.EXTRA_WIFI_AP_STATE, WifiManager.WIFI_AP_STATE_DISABLED));
            } else if (WifiManager.WIFI_STATE_CHANGED_ACTION.equals(action)) {
                handleWifiStateChanged(intent.getIntExtra(
                        WifiManager.EXTRA_WIFI_STATE, WifiManager.WIFI_STATE_UNKNOWN));
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        this.setContentView(R.layout.softapactivity);
        mListView = (ListView)this.findViewById(R.id.softapactivity);
        mSoftapListAdapter = new SoftapListAdapter();

        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        mWifiConfig = mWifiManager.getWifiApConfiguration();
        int sotfapState = mWifiManager.getWifiApState();
        if(sotfapState == WifiManager.WIFI_AP_STATE_ENABLED){
            mSoftapListAdapter.setSoftApStat(SET_TEXT_OPEN);
        }else if(sotfapState == WifiManager.WIFI_AP_STATE_DISABLED){
            mSoftapListAdapter.setSoftApStat(SET_TEXT_CLOSE);
        }else if(sotfapState == WifiManager.WIFI_AP_STATE_ENABLING){
            mSoftapListAdapter.setSoftApStat(SET_TEXT_OPENING);
        }else if(sotfapState == WifiManager.WIFI_AP_STATE_DISABLING){
            mSoftapListAdapter.setSoftApStat(SET_TEXT_CLOSING);
        }

        mListView.setAdapter(mSoftapListAdapter);
        mListView.setOnItemClickListener(this);
        mListView.setOnKeyListener(this);

        mIntentFilter = new IntentFilter(WifiManager.WIFI_AP_STATE_CHANGED_ACTION);
        mIntentFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mContext.registerReceiver(mReceiver, mIntentFilter);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mContext.unregisterReceiver(mReceiver);
    }

    class SoftapListAdapter extends BaseAdapter{

        private int softapstat ;
        @Override
        public int getCount() {
            // TODO Auto-generated method stub
            return 2;
        }

        @Override
        public Object getItem(int arg0) {
            // TODO Auto-generated method stub
            return null;
        }

        @Override
        public long getItemId(int arg0) {
            // TODO Auto-generated method stub
            return 0;
        }

        public void setSoftApStat(int stat){
            softapstat = stat;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            // TODO Auto-generated method stub
            convertView=LayoutInflater.from(mContext).inflate(R.layout.softaplistitem, null);
            text1 = (TextView)convertView.findViewById(R.id.text1);
            text2 = (TextView)convertView.findViewById(R.id.text2);
            smalltext = (TextView)convertView.findViewById(R.id.smalltext);
            LinearLayout ll = (LinearLayout)convertView.findViewById(R.id.rightitem);
            if(position == START_SOFTAP){
                text1.setText(R.string.start_softap);
                if(softapstat == SET_TEXT_OPEN){
                    text2.setText(R.string.open);
                }else if(softapstat == SET_TEXT_CLOSE){
                    text2.setText(R.string.close);
                }else if(softapstat == SET_TEXT_OPENING){
                    text2.setText(R.string.opening);
                }else if(softapstat == SET_TEXT_CLOSING){
                    text2.setText(R.string.closing);
                }
                smalltext.setVisibility(View.GONE);
                ImageView left = (ImageView)convertView.findViewById(R.id.lefticon);
                ImageView right = (ImageView)convertView.findViewById(R.id.righticon);
                left.setOnClickListener(mLeftListener);
                right.setOnClickListener(mRightListener);
            }else if(position == SET_SOFTAP){
                text1.setText(R.string.set_softap);
                if(mWifiConfig != null) {
                    smalltext.setText(String.format(getString(SOFTAP_SUBTEXT),
                            mWifiConfig.SSID,
                            mSecurityType[SoftapSettingActivity.getSecurityTypeIndex(mWifiConfig)]));
                } else {
                    smalltext.setText(R.string.softap_subtext);
                }
                ll.setVisibility(View.INVISIBLE);
            }
            return convertView;
        }
    }
    
    View.OnClickListener mLeftListener = new View.OnClickListener() {
		
		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
            if(!mState){
                mSoftapListAdapter.setSoftApStat(SET_TEXT_OPENING);
                setSoftapEnabled(true);
            }else{
                mSoftapListAdapter.setSoftApStat(SET_TEXT_CLOSING);
                setSoftapEnabled(false);
            }
            mSoftapListAdapter.notifyDataSetChanged();
            Log.d("softAPsetting", "click on left ic");
		}
	};
	
	View.OnClickListener mRightListener = new View.OnClickListener() {
		
		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
            if(mState){
                mSoftapListAdapter.setSoftApStat(SET_TEXT_CLOSING);
                setSoftapEnabled(false);
            }
            else{
                mSoftapListAdapter.setSoftApStat(SET_TEXT_OPENING);
                setSoftapEnabled(true);
            }
            mSoftapListAdapter.notifyDataSetChanged();
		}
	};

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id ) {
        // TODO Auto-generated method stub
        if(position == START_SOFTAP){

        }else if(position == SET_SOFTAP){
            Intent intent = new Intent();
            intent.setClass(SoftapActivity.this, SoftapSettingActivity.class);
            startActivityForResult(intent, 0);
        }
    }

    @Override
    public boolean onKey(View view, int keyCode, KeyEvent key) {
        // TODO Auto-generated method stub

        int index = mListView.getSelectedItemPosition();
        if(index<0 || index > mSoftapListAdapter.getCount() || null == mListView.getSelectedView() ) {
            return false;
        }
        TextView text2 = (TextView)mListView.getSelectedView().findViewById(R.id.text2);
        if(index==START_SOFTAP){
            if(key.getKeyCode()==KeyEvent.KEYCODE_DPAD_LEFT){
                if(key.getAction()==KeyEvent.ACTION_DOWN){
                    if(!mState){
                    	text2.setText(R.string.opening);
                        setSoftapEnabled(true);
                    }else{
                    	text2.setText(R.string.closing);
                        setSoftapEnabled(false);
                    }
            }else if(key.getAction()==KeyEvent.ACTION_UP){

            }
            }else if(key.getKeyCode()==KeyEvent.KEYCODE_DPAD_RIGHT){

                if(key.getAction()==KeyEvent.ACTION_DOWN){
                    if(mState){
                    	text2.setText(R.string.closing);
                        setSoftapEnabled(false);
                    }
                    else{
                    	text2.setText(R.string.opening);
                        setSoftapEnabled(true);
                    }
                }else if(key.getAction()==KeyEvent.ACTION_UP){

                }
            }
        }
        return false;
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

    public void setSoftapEnabled(boolean enable) {
        final ContentResolver cr = mContext.getContentResolver();
        /**
         * Disable Wifi if enabling tethering
         */
        int wifiState = mWifiManager.getWifiState();
        if (enable && ((wifiState == WifiManager.WIFI_STATE_ENABLING) ||
                    (wifiState == WifiManager.WIFI_STATE_ENABLED))) {
            mWifiManager.setWifiEnabled(false);
            Settings.Global.putInt(cr, Settings.Global.WIFI_SAVED_STATE, 1);
        }

        if (mWifiManager.setWifiApEnabled(null, enable)) {
        } else {
            mState = false;
            text2.setText(R.string.close);
        }

        /**
         *  If needed, restore Wifi on tether disable
         */
        if (!enable) {
            int wifiSavedState = 0;
            withStaEnabled = false;
            try {
                wifiSavedState = Settings.Global.getInt(cr, Settings.Global.WIFI_SAVED_STATE);
                withStaEnabled = wifiSavedState == 1? true : false;
            } catch (Settings.SettingNotFoundException e) {
                ;
            }
            if (wifiSavedState == 1) {
                mWifiManager.setWifiEnabled(true);
                Settings.Global.putInt(cr, Settings.Global.WIFI_SAVED_STATE, 0);
            }
        }
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
        if(data!=null){
            if (resultCode == 0) {
                WifiConfiguration wifiConfig = data.getParcelableExtra(SoftapSettingActivity.SOFTAP_CONFIG);
                smalltext.setText(String.format(getString(SOFTAP_SUBTEXT),
                    wifiConfig.SSID,
                    mSecurityType[SoftapSettingActivity.getSecurityTypeIndex(wifiConfig)]));
            }
        }
    }

    private void handleWifiApStateChanged(int state) {
        switch (state) {
            case WifiManager.WIFI_AP_STATE_ENABLED:
                mState = true;
                mHandler.sendEmptyMessage(SET_TEXT_OPEN);
                break;
            case WifiManager.WIFI_AP_STATE_DISABLED:
                if(!withStaEnabled) {
                    mState = false;
                    mHandler.sendEmptyMessage(SET_TEXT_CLOSE);
                }
                break;
            default:
                break;
        }
    }
    private void handleWifiStateChanged(int state) {
        switch (state) {
            case WifiManager.WIFI_STATE_ENABLING:
                mState = false;
                mHandler.sendEmptyMessage(SET_TEXT_CLOSE);
                break;
            default:
                break;
        }
    }
}

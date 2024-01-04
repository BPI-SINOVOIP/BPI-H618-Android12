package com.softwinner.tvdsetting.net.bluetooth;

import com.softwinner.tvdsetting.R;


import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ListView;
import com.softwinner.tvdsetting.net.bluetooth.BluetoothListUtils;
import com.softwinner.tvdsetting.net.bluetooth.BluetoothListUtils.BluetoothDeviceUtils;
import android.util.Log;
import java.util.ArrayList;
import android.widget.TextView;
import android.widget.ImageView;
import android.view.KeyEvent;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.os.SystemProperties;
import android.content.pm.PackageManager;

public class BluetoothListActivity extends BluetoothListUtils implements ListView.OnItemClickListener, ListView.OnKeyListener  {

    private static final String TAG ="BluetoothListActivity";
	Context mContext;
	ListView mListView;
	BluetoothListAdapter mBluetoothListAdapter;
    ArrayList<BluetoothDeviceUtils> btList = new ArrayList();
    public final static int BLUEITEM = 1;
    private boolean isNewFeature = false;
    public final static String BLUEINDEX = "BLUEINDEX";

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
    		String action = intent.getAction();
    		if (action.equals(Constant.ACTION_USBBTDEV_CHANGE)) {
    		    //处理
    			if(isNewFeature&&(SystemProperties.getInt(Constant.BT_HAS_USB,1) == 1)
    					&&(SystemProperties.getInt(Constant.BT_USB_INSERTED, 0) == 0)){
    				finish();
    			}
    		}
        }
    };
        
	@Override
	public void onCreate(Bundle savedInstanceState) {
	    Log.d(TAG,"onCreate ");
		super.onCreate(savedInstanceState);
		mContext = this;
		this.setContentView(R.layout.bluetoothlist);
		isNewFeature = SystemProperties.getInt(Constant.BT_HAS_USB, 33)== 33?false:true;
		mListView = (ListView)this.findViewById(R.id.bluetoothlist);
		mBluetoothListAdapter = new BluetoothListAdapter(btList);
		mListView.setAdapter(mBluetoothListAdapter);
		mListView.setOnItemClickListener(this);
		mListView.setOnKeyListener(this);
		setDeviceListGroup(btList);
	}
	
    @Override
    public void onResume() {
        Log.d(TAG,"onResume ");
        IntentFilter mIntentFilter = new IntentFilter();
        if(mIntentFilter != null)
        	mIntentFilter.addAction(Constant.ACTION_USBBTDEV_CHANGE);
        registerReceiver(mReceiver, mIntentFilter);
		if(isNewFeature&&(SystemProperties.getInt(Constant.BT_HAS_USB,1) == 1)
				&&(SystemProperties.getInt(Constant.BT_USB_INSERTED, 0) == 0)) {
		    finish();
		}
        super.onResume();
	}

    @Override
    public void onPause() {
        Log.d(TAG,"onPause");
        unregisterReceiver(mReceiver);
        super.onPause();
    }

    @Override
    public void onDestroy() {
        Log.d(TAG,"onDestroy");
        super.onDestroy();
    }
    
	private void updateAItem(BluetoothDeviceUtils mDevice) {
		mBluetoothListAdapter.notifyDataSetChanged();
	}

	@Override
	public void updateDeviceView(BluetoothDeviceUtils mDevice) {
	    updateAItem(mDevice);
	}

	@Override
	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) {
		// TODO Auto-generated method stub
		BluetoothDeviceUtils mDevice= (BluetoothDeviceUtils) mBluetoothListAdapter.getItem(position);
		mDevice.onClicked();
	}

	@Override
	public boolean onKey(View view, int keyCode, KeyEvent key) {
		// TODO Auto-generated method stub
		
		int index = mListView.getSelectedItemPosition();
		if(index<0 || index > mBluetoothListAdapter.getCount()) {
			return false;
		}
		BluetoothDeviceUtils mDevice= (BluetoothDeviceUtils) mBluetoothListAdapter.getItem(index);
		Log.d("zjh","onKey position = " + keyCode + " index = " + index);
        if(key.getKeyCode()==KeyEvent.KEYCODE_DPAD_RIGHT){
			if(key.getAction()==KeyEvent.ACTION_DOWN){
			    if(mDevice.isBonded()) {
    				Intent intent = new Intent();
    				intent.setClass(BluetoothListActivity.this, BluetoothLinkSetting.class);
    				intent.putExtra(BLUEINDEX,index);
    				startActivityForResult(intent,BLUEITEM);
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
				title.setText(R.string.bluetooth);
			break;
		}
		return false;
	}
	
	@Override
	public boolean onKeyUp (int keyCode, KeyEvent event){
		TextView title = (TextView) this.findViewById(R.id.title);
		switch(keyCode){
		case KeyEvent.KEYCODE_BACK:
				title.setText(R.string.bluetooth);
				finish();
			break;
		}
		return false;
	}
	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data){
		if(requestCode==BLUEITEM){
			if(resultCode==Activity.RESULT_OK){
				Log.d(TAG,"index = " + data.getIntExtra(BluetoothLinkSetting.SELECT_ITEM, 0));
        		int index = mListView.getSelectedItemPosition();
        		BluetoothDeviceUtils mDevice= (BluetoothDeviceUtils) btList.get(index);
        		mDevice.unpairDevice();
			}
		}
	}
	
	public class BluetoothListAdapter extends BaseAdapter{

		private ArrayList<BluetoothDeviceUtils> mList;
	
		public BluetoothListAdapter(ArrayList<BluetoothDeviceUtils> al){
			mList = al;
		}
		
		public void setList(ArrayList<BluetoothDeviceUtils> al){
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
			convertView=LayoutInflater.from(mContext).inflate(R.layout.bluetoothitem, null);
			TextView tv1 = (TextView)convertView.findViewById(R.id.bluetoothitemtext);
			TextView tv2 = (TextView)convertView.findViewById(R.id.smalltext);
			ImageView iv1 = (ImageView)convertView.findViewById(R.id.bluetoothicon);
			ImageView iv2 = (ImageView) convertView.findViewById(R.id.netitemicon);
			tv1.setText(mList.get(position).getName());
			tv2.setText(mList.get(position).getSummary());
			iv1.setImageResource(mList.get(position).getIcon());
			iv2.setImageResource(R.drawable.right_icon_btn);
			return convertView;
		}
	}
}

package com.softwinner.tvdsetting.net.bluetooth;

import com.softwinner.tvdsetting.R;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.os.SystemProperties;
import android.content.pm.PackageManager;
import android.util.Log;

public class BluetoothLinkSetting extends Activity implements ListView.OnItemClickListener {
	Context mContext;
	ListView mListView;
	private String[] mItem;
	private final int CANCEL_UNPAIR = 0;
    private boolean isNewFeature = false;
    public static final String TAG = "BluetoothLinkSetting";
//	private final int DISCONNECT = 1;
	public static final String SELECT_ITEM = "SELECT_ITEM";

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
    		String action = intent.getAction();
    		if (action.equals(Constant.ACTION_USBBTDEV_CHANGE)) {
    			if(isNewFeature&&(SystemProperties.getInt(Constant.BT_HAS_USB,1) == 1)
    					&&(SystemProperties.getInt(Constant.BT_USB_INSERTED, 0) == 0)){
    				finish();
    			}
    		}
        }
    };
	
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mContext = this;
		this.setContentView(R.layout.bluetoothlink);
		Intent i = this.getIntent();
		int index = i.getIntExtra(BluetoothListActivity.BLUEINDEX, 0);
		isNewFeature = SystemProperties.getInt(Constant.BT_HAS_USB, 33)== 33?false:true;
		mListView = (ListView)this.findViewById(R.id.bluetoothlinklist);
		BluetoothLinkAdapter mBluetoothLinkAdapter = new BluetoothLinkAdapter();
		mListView.setAdapter(mBluetoothLinkAdapter);
		mListView.setOnItemClickListener(this);
	}
	@Override
	public boolean onKeyDown (int keyCode, KeyEvent event){
		ImageView title = (ImageView) this.findViewById(R.id.title);
		switch(keyCode){
		case KeyEvent.KEYCODE_BACK:
				title.setImageResource(R.drawable.ic_word_bluetooth);
			break;
		}
		return false;
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
	class BluetoothLinkAdapter extends BaseAdapter{

		@Override
		public int getCount() {
			// TODO Auto-generated method stub
			return 1;
		}

		@Override
		public Object getItem(int position) {
			// TODO Auto-generated method stub
			return null;
		}

		@Override
		public long getItemId(int position) {
			// TODO Auto-generated method stub
			return 0;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			// TODO Auto-generated method stub
			convertView=LayoutInflater.from(mContext).inflate(R.layout.bluetoothlinkitem, null);
			TextView text = (TextView)convertView.findViewById(R.id.text1);
			switch(position){
			case CANCEL_UNPAIR:
				text.setText(R.string.bluetooth_device_context_unpair);
				break;
//			case DISCONNECT:
//				text.setText(R.string.disconnect_bt);
//				break;
			}
			return convertView;
		}
		
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		// TODO Auto-generated method stub
		Intent intent = new Intent();
		intent.putExtra(SELECT_ITEM,position);
		setResult(Activity.RESULT_OK, intent);
		finish();
	}
}

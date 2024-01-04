package com.softwinner.tvdsetting.about;

import java.util.ArrayList;
import java.util.List;

import com.softwinner.tvdsetting.R;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

public class DeviceNameSetting extends Activity implements 
		ListView.OnItemClickListener {

	Context mContext;
	ListView mListView;
	DeviceNameAdapter deviceNameAdapter;
	
	private SharedPreferences sharedPreferences;
	private final int SITTING_ROOM = 0;
	private final int BEDROOM = 1;
	private final int STUDY = 2;
	private final int OFFICE = 3;
	private final int USER_DEFINED = 4;
	List<ImageView> mImageList;
	
	private static int currentPosition = -1;
	private static int recodePosition = 4;
	private String choose_result;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mContext = this;
		this.setContentView(R.layout.devicenamesetting);
		sharedPreferences = getSharedPreferences("device_name", MODE_PRIVATE);
		mListView = (ListView) this.findViewById(R.id.device_name_setting_list);
		deviceNameAdapter = new DeviceNameAdapter();
		mListView.setAdapter(deviceNameAdapter);
		mListView.setOnItemClickListener(this);
		mImageList = new ArrayList<ImageView>();
		
	}

	class DeviceNameAdapter extends BaseAdapter {

		// ImageView[] choose = new ImageView[USER_DEFINED+1];
		@Override
		public int getCount() {
			// TODO Auto-generated method stub
			return USER_DEFINED + 1;
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
			convertView = LayoutInflater.from(mContext).inflate(
					R.layout.devicenamesettingitem, null);
			TextView text = (TextView) convertView.findViewById(R.id.text1);
			TextView devicename = (TextView) convertView.findViewById(R.id.device_name);
			ImageView img = (ImageView) convertView.findViewById(R.id.choose);
			mImageList.add(img);
			
			switch (position) {
			case SITTING_ROOM:
				text.setText(R.string.sitting_room);
				break;
			case BEDROOM:
				text.setText(R.string.bedroom);
				break;
			case STUDY:
				text.setText(R.string.study);
				break;
			case OFFICE:
				text.setText(R.string.office);
				break;
			case USER_DEFINED:
				text.setText(R.string.user_defined);
			default:
				break;
			}
			
			if(currentPosition == position){
				if(currentPosition == USER_DEFINED) {
					choose_result = sharedPreferences.getString("device_name_defined", "test");
					devicename.setVisibility(View.VISIBLE);
					devicename.setText(choose_result);
				}else{
					img.setVisibility(View.VISIBLE);
					choose_result = text.getText().toString();
				}
				
			}
			return convertView;
		}

	}
	
	@Override
	protected void onResume() {
		sharedPreferences = getSharedPreferences("device_name", MODE_PRIVATE);
		super.onResume();
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		
		currentPosition = position;
		Intent intent = new Intent();
		
		if(position != USER_DEFINED) {
			Editor editor = sharedPreferences.edit();
			String choosed_name = ((TextView)view.findViewById(R.id.text1)).getText().toString();
			editor.putString("device_name_defined", choosed_name);
			UserDefined.device_name_defined = choosed_name;
			recodePosition = position;
			deviceNameAdapter.notifyDataSetChanged();
			
		}else {
			intent.setClass(DeviceNameSetting.this, UserDefined.class);
			intent.putExtra("recodePosition", recodePosition);
			this.startActivityForResult(intent,0);
		}
		
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		if(resultCode == RESULT_OK){
			currentPosition = data.getIntExtra("passPosition", USER_DEFINED);
			deviceNameAdapter.notifyDataSetChanged();
			
		}
		
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		TextView title = (TextView) this.findViewById(R.id.title);
		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK:
			title.setText(R.string.setting_name);
			break;
		}
		return false;
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		TextView title = (TextView) this.findViewById(R.id.title);
		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK:
			title.setText(R.string.setting_name);
			
			Intent data=new Intent();  
            setResult(RESULT_OK, data);
            
            Editor editor = sharedPreferences.edit();
            editor.putString("device_name", choose_result);
            editor.commit();
            
			finish();
			break;
		}
		return false;
	}
	
}

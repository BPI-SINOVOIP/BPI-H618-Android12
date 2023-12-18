package com.softwinner.tvdsetting.about;

import com.softwinner.tvdsetting.R;

import android.os.SystemProperties;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.LinearLayout;

public class AboutSetting extends Activity implements ListView.OnKeyListener {

	Context mContext;
	ListView mListView;
	private SharedPreferences mySharedPreferences;
	private final int DEVICE_NAME = 0;
	private final int PRODUCT_MODEL = 1;
	private final int SOFT_RELEASE = 2;
	private String name;
	private String device_name;
	private AboutSettingAdapter aboutSettingAdapter;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mContext = this;
		name = getString(R.string.sitting_room);
		this.setContentView(R.layout.aboutsetting);
		
		mySharedPreferences = getSharedPreferences("device_name", MODE_PRIVATE);
		device_name = mySharedPreferences.getString("device_name", name);
		
		Log.e("~~~~", device_name + "____AboutSetting");
		mListView = (ListView) this.findViewById(R.id.about_setting_list);
		aboutSettingAdapter = new AboutSettingAdapter();
		mListView.setAdapter(aboutSettingAdapter);
		mListView.setOnKeyListener(this);
	}

	class AboutSettingAdapter extends BaseAdapter {

		@Override
		public int getCount() {
			// TODO Auto-generated method stub
			return SOFT_RELEASE + 1;
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
		public boolean isEnabled(int position) {
			if (position == DEVICE_NAME) {
				return super.isEnabled(position);
			} else {
				return false;
			}

		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			// TODO Auto-generated method stub
			convertView = LayoutInflater.from(mContext).inflate(
					R.layout.aboutsettingitem, null);
			TextView text = (TextView) convertView.findViewById(R.id.text1);
			TextView state = (TextView) convertView.findViewById(R.id.state);
			ImageView righticon = (ImageView) convertView.findViewById(R.id.righticon);
			switch (position) {
			case DEVICE_NAME:
				righticon.setVisibility(View.VISIBLE);
				text.setText(R.string.device_name);
				convertView.setOnClickListener(new View.OnClickListener() {
					
					@Override
					public void onClick(View arg0) {
						// TODO Auto-generated method stub
						startDeviceNameSet();
					}
				});
				if (device_name.equals("")) {
					state.setText(name);
				} else {
					state.setText(device_name);
				}
//				state.setGravity(Gravity.RIGHT | Gravity.CENTER);
//				Drawable drawable = getResources().getDrawable(
//						R.drawable.right_icon_btn);
//				drawable.setBounds(0, 0, drawable.getMinimumWidth(),
//						drawable.getMinimumHeight());
//				state.setCompoundDrawables(null, null, drawable, null);
				break;
			case PRODUCT_MODEL:
				righticon.setVisibility(View.GONE);
				text.setText(R.string.product_model);
				String model = SystemProperties.get("ro.product.device");
				state.setText(model);
//				state.setPadding(0, 0, 50, 0);
				break;
			case SOFT_RELEASE:
				righticon.setVisibility(View.GONE);
				text.setText(R.string.soft_release);
				text.setSingleLine(true);
				String release = SystemProperties.get("ro.product.firmware");
				state.setText(release);
//				state.setPadding(0, 0, 50, 0);
				break;
			default:
				break;
			}
			return convertView;
		}

	}

	@Override
	public boolean onKey(View v, int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		int index = mListView.getSelectedItemPosition();

		boolean down = event.getAction() == KeyEvent.ACTION_DOWN ? true : false;
		boolean right = keyCode == KeyEvent.KEYCODE_DPAD_RIGHT ? true : false;
		boolean ok = (keyCode == KeyEvent.KEYCODE_DPAD_CENTER || 
                      keyCode == KeyEvent.KEYCODE_ENTER) ? true : false;
		switch (index) {
		case DEVICE_NAME:
			if (down) {
				if (right || ok) {
					startDeviceNameSet();
				}
			}
			break;
		default:
			break;
		}
		return false;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		TextView title = (TextView) this.findViewById(R.id.title);
		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK:
			title.setText(R.string.setting_about);
			break;
		}
		return false;
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		TextView title = (TextView) this.findViewById(R.id.title);
		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK:
			title.setText(R.string.setting_about);
			finish();
			break;
		}
		return false;
	}

	private void startDeviceNameSet() {
		Intent intent = new Intent(AboutSetting.this, DeviceNameSetting.class);
		this.startActivityForResult(intent, 0);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		if (resultCode == RESULT_OK) {
			// Bundle bundle = data.getExtras();
			// device_name = bundle.getString("result");
			SharedPreferences sharedPreferences = getSharedPreferences(
					"device_name", MODE_PRIVATE);
			device_name = sharedPreferences.getString("device_name", name);
			aboutSettingAdapter.notifyDataSetChanged();
		}
	}

}

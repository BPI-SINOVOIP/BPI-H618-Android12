package com.allwinnertech.dragonsn;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

import com.allwinnertech.dragonsn.entity.BindedColume;
import com.allwinnertech.dragonsn.view.VerifyListAdatper;

public class DragonSNActivity extends Activity implements OnKeyListener {
	public static final String TAG = "DradonSN";
	BurnManager mBurnManager;
	Config mConfig;

	public static final int MSG_LOAD_FINISH = 1;
	public static final int MSG_UPDATE_FINISH = 2;
	public static final int MSG_CHECK_NETWORK = 11;

	public Handler mHandler = new Handler() {
		public void handleMessage(Message msg) {
			boolean result;
			switch (msg.what) {
			case MSG_LOAD_FINISH:
				result = msg.arg1 == 1;
				mListAdapter.notifyDataSetChanged();
				if (result) {
					result = mBurnManager.isAllRemoteDataValid();
					if (!result) {
						showInfoTV.append(getResources().getText(R.string.show_remote_invalid));
						return;
					}
					showInfoTV.append(getResources().getText(R.string.show_getting_remote_success));
					showInfoTV.append(getResources().getText(R.string.show_setting_local));
					result = mBurnManager.updataLocatData();
					if (result) {
						showInfoTV.append(getResources().getText(R.string.show_getting_local));
						mBurnManager.loadLocalDatas();
						result = mBurnManager.checkAllColumeValidAndSetReturnResult();
						if (result) {
							showInfoTV.append(getResources().getText(R.string.show_check_data_valid_success));
							mBurnManager.updateResult();
						} else {
							showInfoTV.append(getResources().getText(R.string.show_check_data_valid_fail));
						}
					} else {
						showInfoTV.append(getResources().getText(R.string.show_setting_local_fail));
					}
				} else {
					showInfoTV.append(getResources().getText(R.string.show_getting_remote_fail));
				}
				break;

			case MSG_UPDATE_FINISH:
				result = msg.arg1 == 1;
				mListAdapter.notifyDataSetChanged();
				if (result) {
					showInfoTV.append(getResources().getText(R.string.show_setting_remote_success));
					showInfoTV.append(getResources().getText(R.string.show_check_all_success));

				} else {
					showInfoTV.append(getResources().getText(R.string.show_setting_remote_fail));
					showInfoTV.append(getResources().getText(R.string.show_check_all_fail));
				}
				break;
			case MSG_CHECK_NETWORK:
				boolean success = Utils.isNetworkConn(DragonSNActivity.this);
				Log.i(DragonSNActivity.TAG, "MSG_CHECK_NETWORK success=" + success);
				if (success) {
					showInfoTV.setText(R.string.show_network_ready);
					Toast.makeText(DragonSNActivity.this, R.string.show_network_ready, Toast.LENGTH_LONG).show();
				} else {
					showInfoTV.setText(getResources().getText(R.string.show_alert_str));
					sendEmptyMessageDelayed(MSG_CHECK_NETWORK, 1000);
				}

				break;
			default:
				break;
			}
		};
	};

	BroadcastReceiver mReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			if ("android.net.ethernet.LINKED_ACTION".equals(action)) {
				Log.i(DragonSNActivity.TAG, "ethernet linked");
			}
		}
	};

	LinearLayout showDataLL;
	ListView showColumesLV;
	TextView showAlertTV;
	TextView showInfoTV;
	TextView inputType;
	EditText inputPrimValue;

	BaseAdapter mListAdapter;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		setTitle(getResources().getString(R.string.app_version) + Utils.getAppVersionName(this));

		showDataLL = (LinearLayout) findViewById(R.id.show_data_ll);
		showColumesLV = (ListView) findViewById(R.id.show_columes_lv);
		showAlertTV = (TextView) findViewById(R.id.show_alert_tv);
		showInfoTV = (TextView) findViewById(R.id.show_running_info);
		inputType = (TextView) findViewById(R.id.input_type);
		inputPrimValue = (EditText) findViewById(R.id.input_prim_value);

		// Utils.openEthernet();
		mConfig = Config.getConfig();
		List<BindedColume> bindedColumes = new ArrayList<BindedColume>();
		Intent intent = getIntent();
		String configPath = intent.getStringExtra("configPath");
		boolean parseResult = mConfig.parseConfig(bindedColumes, configPath);
		if (!parseResult) {
			showDataLL.setVisibility(View.GONE);
			showAlertTV.setVisibility(View.VISIBLE);
			Toast.makeText(this, R.string.show_alert_exit_str, Toast.LENGTH_LONG).show();
		}

		inputPrimValue.setOnKeyListener(this);
		inputPrimValue.setInputType(InputType.TYPE_NULL);
		inputType.setText(getResources().getString(R.string.show_input_info, mConfig.primKey));

		mBurnManager = new BurnManager(mHandler, bindedColumes, mConfig);
		mListAdapter = new VerifyListAdatper(this, mBurnManager);
		showColumesLV.setAdapter(mListAdapter);

	}

	@Override
	protected void onResume() {
		super.onResume();
		mHandler.sendEmptyMessage(MSG_CHECK_NETWORK);
		mBurnManager.loadLocalDatas();
		mListAdapter.notifyDataSetChanged();
		inputPrimValue.requestFocus();
	}

	@Override
	protected void onPause() {
		mHandler.removeMessages(MSG_CHECK_NETWORK);
		super.onPause();
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_MENU) {
			inputPrimValue.setText("");
			showInfoTV.setText(R.string.show_re_burn);
			mBurnManager.resetData();
			mListAdapter.notifyDataSetChanged();
			inputPrimValue.requestFocus();
		}
		return super.onKeyUp(keyCode, event);
	}

	@Override
	public boolean onKey(View view, int keycode, KeyEvent event) {
		if (keycode == KeyEvent.KEYCODE_ENTER && event.getAction() == KeyEvent.ACTION_DOWN) {
			switch (view.getId()) {
			case R.id.input_prim_value:

				BindedColume primColume = mBurnManager.getPrimColume();
				boolean result = primColume.setPrimValue(inputPrimValue.getText().toString().trim());
				if (result) {
					showInfoTV.append(getResources().getText(R.string.show_getting_remote));
					mBurnManager.loadRemoteData();
				} else {
					inputPrimValue.setText("");
					showInfoTV.append(getResources().getText(R.string.show_need_input_text));
					inputPrimValue.requestFocus();
				}
				break;
			default:
				break;
			}
		}

		return false;
	}

}

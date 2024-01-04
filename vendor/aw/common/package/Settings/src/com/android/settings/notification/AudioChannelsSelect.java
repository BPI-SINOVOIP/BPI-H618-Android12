package com.android.settings.notification;

import java.util.ArrayList;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.media.AudioManager;
import android.media.AudioManagerEx;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.provider.Settings;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.view.WindowManagerPolicy;


import com.android.settings.R;

public class AudioChannelsSelect implements OnItemClickListener, OnCancelListener{
	private static final String TAG = "AudioChannelsSelect";
	private Context mContext;
	private Dialog mDialog;
	private ListView mList;
	private TableRow mTable;
	private ArrayList<String> mChannels;
	private boolean mEnablePassThrough;
	private AudioManager mAudioManager;
	private WindowManager mWindowManager;

	private AudioManagerEx mAudioManagerEx;
	private BroadcastReceiver mReceiver;
	private static final String AUDIO_STATE = "audioState";
	private static final String AUDIO_NAME = "audioName";
	private static final String AUDIO_TYPE = "audioType";
	public AudioChannelsSelect(Context context){
		Log.d(TAG, "AudioChannelsSelect.");
		mContext = context;
		mWindowManager= (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
		mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
		mAudioManagerEx = new AudioManagerEx(context);
		mReceiver = new BroadcastReceiver(){
			@Override
			public void onReceive(Context context,Intent intent){
				if(mDialog == null || !mDialog.isShowing()){
					return;
				}
				Bundle bundle = intent.getExtras();
				final int state = bundle.getInt(AUDIO_STATE);
				final String name = bundle.getString(AUDIO_NAME);
				final int type = bundle.getInt(AUDIO_TYPE);
				try	{
					Thread.currentThread().sleep(500);
				}catch(Exception e) {};

				mChannels = mAudioManagerEx.getActiveAudioDevices(AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
				boolean hasHdmi = audioHasHdmi();
				mTable.clear();
				ArrayList<String> list = mAudioManagerEx.getAudioDevices(AudioManagerEx.AUDIO_OUTPUT_TYPE);
				for(String st:list){
					if(!hasHdmi){
						if(st.contains("HDMI")){
							continue;
						}
					}
					mTable.add(st);
				}

			}
		};
	}

	public void registerAudioReceiver(){
		IntentFilter filter = new IntentFilter();
		filter.addAction(WindowManagerPolicy.ACTION_AUDIO_PLUG_IN_OUT);
		mContext.registerReceiver(mReceiver, filter);
	}

	public void unregisterAudioReceiver(){
		if(mReceiver != null){
			mContext.unregisterReceiver(mReceiver);
		}
	}

	private boolean audioHasHdmi(){
		/*
        	String databaseValue = Settings.System.getString(mContext.getContentResolver(),Settings.System.DISPLY_OUTPUT_FORMAT);
        	if(databaseValue != null && databaseValue.startsWith("HDMI")){
        		return true;
        	}else{
        		return false;
        	}
		*/
		return true;
    	}

	public void showChannelsSelectDialog(){
		Log.d(TAG, "showChannelsSelectDialog.");
		if(mDialog == null){
			mDialog = new Dialog(mContext);
			mList = new ListView(mContext);
			mTable = new TableRow(mContext);
			mList.setAdapter(mTable);
			mList.setOnItemClickListener(this);
			mTable.setNotifyOnChange(true);
			mDialog.setContentView(mList);
			mDialog.setCanceledOnTouchOutside(true);
			mDialog.setOnCancelListener(this);
			mDialog.setTitle(R.string.audio_output_mode_title);
			mChannels = new ArrayList<String>();
		}
		Log.d(TAG, "showChannelsSelectDialog.");
		mChannels = mAudioManagerEx.getActiveAudioDevices(AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
		mTable.clear();
		boolean hasHdmi = audioHasHdmi();
		ArrayList<String> list  = mAudioManagerEx.getAudioDevices(AudioManagerEx.AUDIO_OUTPUT_TYPE);
		for(String st:list){
			if(!hasHdmi){
				if(st.contains("HDMI")){
					continue;
				}
			}
			mTable.add(st);
		}
		mEnablePassThrough = (Settings.System.getInt(mContext.getContentResolver(), Settings.System.ENABLE_PASS_THROUGH, 0) == 1);
		if(mEnablePassThrough == true && mChannels.size() != 1)
		{
			mChannels.clear();
			mChannels.add("AUDIO_CODEC");
		}
		mDialog.show();
		Window w = mDialog.getWindow();
		double wm_height = mWindowManager.getDefaultDisplay().getHeight()* 0.6;
		double wm_width = mWindowManager.getDefaultDisplay().getWidth() * 0.4;
		w.setLayout((int)wm_width, (int)wm_height);
	}

	public void dismissDialog(){
		if(mDialog != null) mDialog.dismiss();
	}

	private class TableRow extends ArrayAdapter<String>{
		private Context context = null;
		public TableRow(Context context) {
	   		super(context, R.layout.audio_channels_select);
	   		this.context = context;
	   	}

		@Override
	   	public View getView(int position, View convertView, ViewGroup parent){
			if(convertView == null){
				LayoutInflater inflater = LayoutInflater.from(context);
				RelativeLayout ly = (RelativeLayout) inflater.inflate(R.layout.audio_channels_select, null);
				convertView = ly;
			}
			TextView text = (TextView)convertView.findViewById(R.id.audio_channel_name);
			CheckBox check = (CheckBox)convertView.findViewById(R.id.check_audio_selected);
			text.setText(this.getItem(position));
			if(mChannels.contains(this.getItem(position))){
				check.setChecked(true);
			}else{
				check.setChecked(false);
			}
			return convertView;
	   	}
	}

	@Override
	public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
		CheckBox check = (CheckBox)arg1.findViewById(R.id.check_audio_selected);
		if(check.isChecked()){
			check.setChecked(false);
			mChannels.remove(mTable.getItem(arg2));
		}else{
			check.setChecked(true);
			for(String st:mChannels){
				if(st.contains("USB")){
					mChannels.clear();
				}
			}
			if (mEnablePassThrough || mTable.getItem(arg2).contains("USB"))
				mChannels.clear();
			mChannels.add(mTable.getItem(arg2));
		}
		mTable.notifyDataSetChanged();

		ArrayList<String> oldActived = mAudioManagerEx.getActiveAudioDevices(AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
		if(oldActived != null && oldActived.equals(mChannels)){
			return;
		}
		mAudioManagerEx.setAudioDeviceActive(mChannels, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
/*
		String st = null;
		for(int i = 0; i < mChannels.size(); i++){
			if(st == null){
				st = mChannels.get(i);
			}
			else{
				st = st + "," + mChannels.get(i);
			}
		}
		Settings.System.putString(mContext.getContentResolver(), Settings.System.AUDIO_OUTPUT_CHANNEL, st);
*/
	}

	@Override
	public void onCancel(DialogInterface dialog) {
		Log.d("chen","onCancel()  save audio output channels");
		//if the data is equals the pre,do not need to switch.
		/*
		ArrayList<String> oldActived = mAudioManager.getActiveAudioDevices(AudioManager.AUDIO_OUTPUT_ACTIVE);
		if(oldActived != null && oldActived.equals(mChannels)){
			return;
		}
		mAudioManager.setAudioDeviceActive(mChannels, AudioManager.AUDIO_OUTPUT_ACTIVE);
		String st = null;
		for(int i = 0; i < mChannels.size(); i++){
			if(st == null){
				st = mChannels.get(i);
			}
			else{
				st = st + "," + mChannels.get(i);
			}
		}
		Settings.System.putString(mContext.getContentResolver(), Settings.System.AUDIO_OUTPUT_CHANNEL, st);
		*/
	}
}

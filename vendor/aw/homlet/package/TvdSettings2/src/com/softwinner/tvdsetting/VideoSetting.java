package com.softwinner.tvdsetting;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

public class VideoSetting extends Activity  implements ListView.OnKeyListener {
	Context mContext;
	ListView mListView;
	private final int VIDEO_QUALITY = 0;
	private final int VIDEO_SIZE = 1;
	private final int VIDEO_JUMP = 2;
	private final int VIDEO_SUBTEXT_DISPLAY = 3;
	private final int VIDEO_SCALE_DYNAMIC = 4;
	private final int VIDEO_MUTE = 5;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mContext = this;
		this.setContentView(R.layout.videosetting);
		mListView = (ListView)this.findViewById(R.id.video_setting_list);
		mListView.setAdapter(new VideoSettingAdapter());
		mListView.setOnKeyListener(this);
	}
	
	class VideoSettingAdapter extends BaseAdapter{

		@Override
		public int getCount() {
			// TODO Auto-generated method stub
			return VIDEO_MUTE + 1;
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
			convertView=LayoutInflater.from(mContext).inflate(R.layout.videosettingitem, null);
			TextView text = (TextView)convertView.findViewById(R.id.text1);
			TextView state = (TextView)convertView.findViewById(R.id.state);
			switch(position){
			case VIDEO_QUALITY:
				text.setText(R.string.video_quality);
				state.setText(R.string.auto);
				break;
			case VIDEO_SIZE:
				text.setText(R.string.video_size);
				state.setText(R.string.self_adaption);
				break;
			case VIDEO_JUMP:
				text.setText(R.string.video_jump);
				state.setText(R.string.video_jump_open);
				break;
			case VIDEO_SUBTEXT_DISPLAY:
				text.setText(R.string.video_subtext_display);
				state.setText(R.string.open);
				break;
			case VIDEO_SCALE_DYNAMIC:
				text.setText(R.string.video_scale_dynamic);
				state.setText(R.string.open);
				break;
			case VIDEO_MUTE:
				text.setText(R.string.audio_output);
				state.setText(R.string.stereo_mode);
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
		TextView state = (TextView)mListView.getSelectedView().findViewById(R.id.state);
		
		boolean down = event.getAction()==KeyEvent.ACTION_DOWN?true:false;
		boolean left = keyCode==KeyEvent.KEYCODE_DPAD_LEFT?true:false;
		boolean right = keyCode==KeyEvent.KEYCODE_DPAD_RIGHT?true:false;
			switch(index){
			case VIDEO_QUALITY:
				if(down){
					if(left){
						state.setText(R.string.auto);
					}else if(right){
						state.setText(R.string.manual);
					}
				}
				break;
			case VIDEO_SIZE:
				if(down){
					if(left){
						state.setText(R.string.auto);
					}else if(right){
						state.setText(R.string.manual);
					}
				}
				break;
			case VIDEO_JUMP:
				if(down){
					if(left){
						state.setText(R.string.auto);
					}else if(right){
						state.setText(R.string.manual);
					}
				}
				break;
			case VIDEO_SUBTEXT_DISPLAY:
				if(down){
					if(left){
						state.setText(R.string.auto);
					}else if(right){
						state.setText(R.string.manual);
					}
				}
				break;
			case VIDEO_SCALE_DYNAMIC:
				if(down){
					if(left){
						state.setText(R.string.auto);
					}else if(right){
						state.setText(R.string.manual);
					}
				}
				break;
			case VIDEO_MUTE:
				if(down){
					if(left){
						state.setText(R.string.auto);
					}else if(right){
						state.setText(R.string.manual);
					}
				}
				break;
			default:
				break;
			}
		return false;
	}
	@Override
	public boolean onKeyDown (int keyCode, KeyEvent event){
		ImageView title = (ImageView) this.findViewById(R.id.title);
		switch(keyCode){
		case KeyEvent.KEYCODE_BACK:
				title.setImageResource(R.drawable.ic_word_play_setting_nor);
			break;
		}
		return false;
	}
	
	@Override
	public boolean onKeyUp (int keyCode, KeyEvent event){
		ImageView title = (ImageView) this.findViewById(R.id.title);
		switch(keyCode){
		case KeyEvent.KEYCODE_BACK:
				title.setImageResource(R.drawable.ic_word_play_setting_nor);
				finish();
			break;
		}
		return false;
	}
}

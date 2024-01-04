package com.softwinner.tvdsetting;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.softwinner.tvdsetting.R;

public class DisplayAdjuestDialog extends Dialog {
	private Context context;
	private String mText1 = "";
	private String mText2 = "";
	private String formattext1 = "";
	private String formattext2 = "";
	private int TIME_OUT = 13;
	TextView  mTv;
	DisplayAdjuestDialog mInstance; 
	DisplayDialogInterface ddInterface;
	
	private final int TEXT_TICK = 1; 
	private int cnt;
	Handler mHandler = new Handler(){
		@Override
		public void handleMessage(Message msg){
			switch(msg.what){
			case TEXT_TICK:
				mText2 = mText2.format(formattext2, --cnt);
				mTv.setText(mText2);
				if(cnt>0){
					mHandler.sendEmptyMessageDelayed(TEXT_TICK,1000);
				}else{
					if(ddInterface!=null){
						ddInterface.onTimeOut();
					}
				}
				break;
			}
		}
	};
	interface DisplayDialogInterface{
		void onButtonYesClick();
		void onButtonNoClick();
		void onTimeOut();
	}
	
	public DisplayAdjuestDialog(Context context) {
	        super(context);
	        // TODO Auto-generated constructor stub
	        this.context = context;
	        mInstance = this;
	}
	public DisplayAdjuestDialog(Context context, int theme, String dispmode ){
	        super(context, theme);
	        this.context = context;
	        Resources res = context.getResources();
	        formattext1 = res.getString(R.string.dispchanged);
	        formattext2 = res.getString(R.string.changedverify);
	        mText1 = mText1.format(formattext1, dispmode);
	        mInstance = this;
	}

	
	public void setDisplayDialogInterface(DisplayDialogInterface ddinterface){
		ddInterface = ddinterface;
	}
	@Override
	protected void onCreate(Bundle savedInstanceState) {
	        // TODO Auto-generated method stub
	        super.onCreate(savedInstanceState);
	        this.setContentView(R.layout.dispchangeddialog);
	        TextView tv1 = (TextView)this.findViewById(R.id.dispdialog_text1);
	        mTv = (TextView)this.findViewById(R.id.dispdialog_text2);
	        ImageButton ib1 = (ImageButton)this.findViewById(R.id.yes);
	        ImageButton ib2 = (ImageButton)this.findViewById(R.id.no);		

	        ib1.setOnClickListener(new ImageButton.OnClickListener(){
				@Override
				public void onClick(View arg0) {
					// TODO Auto-generated method stub
					if(ddInterface!=null){
						ddInterface.onButtonYesClick();
					}
				}
	        });
	        ib2.setOnClickListener(new ImageButton.OnClickListener(){

				@Override
				public void onClick(View arg0) {
					// TODO Auto-generated method stub
					if(ddInterface!=null){
						ddInterface.onButtonNoClick();
					}
				}
	        	
	        });
	        cnt = TIME_OUT;
	        mText2 = mText2.format(formattext2, cnt);
	        tv1.setText(mText1);
	        mTv.setText(mText2);
	        mHandler.sendEmptyMessage(TEXT_TICK);
	        
	}
}

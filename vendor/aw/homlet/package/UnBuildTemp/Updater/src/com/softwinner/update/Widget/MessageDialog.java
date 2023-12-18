package com.softwinner.update.Widget;

import com.softwinner.update.R;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager.LayoutParams;

public class MessageDialog extends Dialog implements View.OnClickListener{
	private Context mContext;
	private String messageText = null;
	private String titleText = null;
	public MessageDialog(Context context) {
		super(context,R.style.cdialog);
		mContext = context;
	}
	public MessageDialog(Context context,String title_text,String messaget_text){
		super(context,R.style.cdialog);
		mContext = context;
		this.messageText = messaget_text;
		this.titleText=title_text;
	}
	public MessageDialog(Context context,int title_text,int messaget_text){
		super(context,R.style.cdialog);
		mContext = context;
		this.messageText = mContext.getString(messaget_text);
		this.titleText=mContext.getString(title_text);
	}
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.message_dialog);
		setCanceledOnTouchOutside(false);
	}
	@Override
	public void show() {	
		super.show();
	}
	@Override
	public void dismiss() {
		// TODO Auto-generated method stub
		super.dismiss();
	}
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		
	}

}

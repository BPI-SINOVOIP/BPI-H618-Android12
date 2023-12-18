package com.softwinner.update.Widget;

import com.softwinner.update.R;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager.LayoutParams;
import android.widget.TextView;

public class CustomDialog extends Dialog{
	private Context mContext;
	private String titleText = null;
	public CustomDialog(Context context) {
		super(context,R.style.cdialog);
		mContext = context;
	}
	public CustomDialog(Context context,String title_text){
		super(context,R.style.cdialog);
		mContext = context;
		this.titleText=title_text;
	}
	public CustomDialog(Context context,int title_text){
		super(context,R.style.cdialog);
		mContext = context;
		this.titleText=mContext.getString(title_text);
	}
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.custom_dialog);
		if(titleText!=null){
			TextView tv =(TextView)findViewById(R.id.UToastText);
			tv.setText(titleText);
		}
		Window win = getWindow();
		LayoutParams params = win.getAttributes();
		params.y = params.y-40;
		win.setAttributes(params);
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

}

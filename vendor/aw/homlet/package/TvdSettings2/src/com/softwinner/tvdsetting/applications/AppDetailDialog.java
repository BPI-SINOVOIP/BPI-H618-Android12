package com.softwinner.tvdsetting.applications;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.softwinner.tvdsetting.R;

public class AppDetailDialog extends Dialog {
	Context context;
	String mText1 = "";
	String mText2 = "";
	Drawable mIcon;
	AppDetailDialogInterface appInterface;
	
	interface AppDetailDialogInterface{
		void onButtonYesClick();
		void onButtonNoClick();
	}
	
	public AppDetailDialog(Context context) {
	        super(context);
	        // TODO Auto-generated constructor stub
	        this.context = context;
	}
	public AppDetailDialog(Context context, int theme){
	        super(context, theme);
	        this.context = context;
	}
	public AppDetailDialog(Context context, int theme,String text1,String text2,Drawable drawable){
        super(context, theme);
        this.context = context;
        mText1 = text1;
        mText2 = text2;
        mIcon = drawable;
	}
	
	public AppDetailDialog(Context context, int theme,String text1,String text2,Drawable drawable,AppDetailDialogInterface appinterface){
        super(context, theme);
        this.context = context;
        mText1 = text1;
        mText2 = text2;
        mIcon = drawable;
        appInterface = appinterface;
	}
	public void setAppDetailDialogInterface(AppDetailDialogInterface appinterface){
		appInterface = appinterface;
	}
	@Override
	protected void onCreate(Bundle savedInstanceState) {
	        // TODO Auto-generated method stub
	        super.onCreate(savedInstanceState);
	        this.setContentView(R.layout.appdialog);
	        TextView tv1 = (TextView)this.findViewById(R.id.appdialog_text1);
	        TextView tv2 = (TextView)this.findViewById(R.id.appdialog_text2);
	        ImageView mImage = (ImageView)this.findViewById(R.id.appdialogicon);
	        ImageButton ib1 = (ImageButton)this.findViewById(R.id.yes);
	        ImageButton ib2 = (ImageButton)this.findViewById(R.id.no);		
	        tv1.setText(mText1);
	        tv2.setText(mText2);
	        ib1.setOnClickListener(new ImageButton.OnClickListener(){
				@Override
				public void onClick(View arg0) {
					// TODO Auto-generated method stub
					if(appInterface!=null){
						appInterface.onButtonYesClick();
					}
				}
	        });
	        ib2.setOnClickListener(new ImageButton.OnClickListener(){

				@Override
				public void onClick(View arg0) {
					// TODO Auto-generated method stub
					if(appInterface!=null){
						appInterface.onButtonNoClick();
					}
				}
	        	
	        });
	        if(mIcon!=null){
	        	mImage.setImageDrawable(mIcon);
	        }
	}
}

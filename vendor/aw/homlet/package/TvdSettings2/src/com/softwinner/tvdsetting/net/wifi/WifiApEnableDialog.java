package com.softwinner.tvdsetting.net.wifi;

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

public class WifiApEnableDialog extends Dialog {
    Context context;
    String mText1 = "";
    String mText2 = "";
    WifiApEnableDialogInterface srInterface;

    interface WifiApEnableDialogInterface{
        void onButtonYesClick();
        void onButtonNoClick();
    }

    public WifiApEnableDialog(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
        this.context = context;
    }
    public WifiApEnableDialog(Context context, int theme){
        super(context, theme);
        this.context = context;
    }
    public WifiApEnableDialog(Context context, int theme,String text1,String text2){
        super(context, theme);
        this.context = context;
        mText1 = text1;
        mText2 = text2;
    }

    public WifiApEnableDialog(Context context, int theme,String text1,String text2,WifiApEnableDialogInterface appinterface){
        super(context, theme);
        this.context = context;
        mText1 = text1;
        mText2 = text2;
        srInterface = appinterface;
    }
    public void setWifiApEnableDialogInterface(WifiApEnableDialogInterface appinterface){
        srInterface = appinterface;
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        this.setContentView(R.layout.wifiapenabledialog);
        TextView tv1 = (TextView)this.findViewById(R.id.appdialog_text1);
        TextView tv2 = (TextView)this.findViewById(R.id.appdialog_text2);
        ImageButton ib1 = (ImageButton)this.findViewById(R.id.yes);
        ImageButton ib2 = (ImageButton)this.findViewById(R.id.no);
        tv1.setText(mText1);
        tv2.setText(mText2);
        ib1.setOnClickListener(new ImageButton.OnClickListener(){
            @Override
            public void onClick(View arg0) {
                // TODO Auto-generated method stub
                if(srInterface!=null){
                    srInterface.onButtonYesClick();
                }
            }
        });

        ib2.setOnClickListener(new ImageButton.OnClickListener(){
            @Override
            public void onClick(View arg0) {
                // TODO Auto-generated method stub
                if(srInterface!=null){
                    srInterface.onButtonNoClick();
                }
            }
        });
    }
}

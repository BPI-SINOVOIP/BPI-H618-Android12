package com.softwinner.firelauncher.otherwidgets;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.View.OnKeyListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;

import com.softwinner.animationview.AnimationText;
import com.softwinner.firelauncher.otherwidgets.BadgeView;
import com.softwinner.firelauncher.utils.ReflectionCall;
import com.softwinner.firelauncher.R;

public class LauncherDialog implements OnClickListener,OnKeyListener{
//	private static final String TAG = "LauncherDialog";
	public static final int  DISMISS = 0;
	public static final int  TOP = 1;
	public static final int  BOTTOM = 2;
	public static final int  LEFT = 3;
	public static final int  RIGHT = 4;
	public static final int CONNECT_NO = 0;
	public static final int CONNECT_WIFI = 1;
	public static final int CONNECT_ETHERNET = 2;
	public static final int  USB_SHOW = 1;
	public static final int  SD_SHOW = 2;
	private static final int DELAY_SHOW_BOTTOM = 123454321;
	private static final int DISMISS_DIALOG = 123222321;
	
	private static LauncherDialog mInstance = null;
	private static Context mContext;
	private View dialogLayout;
	private WifiDialog mWifiDialog;
	private LayoutParams params;
	private Button dialog_button_ok;
	private Button dialog_button_cancel;
	private Button dialog_button_hide;
	private RelativeLayout dialog_position_linear;
	private TextView dialog_button_title;
	private AnimationText connect_name;
	private LauncherDialogListener mListener;
	private int connect_state = CONNECT_NO;
	private int currShowPosition = DISMISS;
	private boolean isShow = false;
	private boolean isNeverShow = false;


	@SuppressLint("HandlerLeak")
	Handler mHandler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			switch (msg.what) {
			case DISMISS_DIALOG:
				mWifiDialog.dismiss();
				break;
			case DELAY_SHOW_BOTTOM:
				//dailog重现动画暂定为Bottom
				mWifiDialog.showDialog(BOTTOM);
				break;
			}
		};
	};
	
	public interface LauncherDialogListener {
		void onOK();
		void onCancel();
	}
	
	public static LauncherDialog getInstance(Context context) {
		if(null == mInstance) {
			mInstance = new LauncherDialog();
			mContext = context;
		}
		return mInstance;
	}
	
	public void initLauncherDialog() {
		dialogLayout = LayoutInflater.from(mContext).inflate(R.layout.launcher_dialog_layout, null);
    	dialog_position_linear = (RelativeLayout)dialogLayout.findViewById(R.id.dialog_position_linear);
    	params = new LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT,
    			(int)mContext.getResources().getDimension(R.dimen.launchrer_dialog_bg_height));
    	
		dialog_button_title = (TextView)dialogLayout.findViewById(R.id.dialog_button_title);
		dialog_button_ok = (Button)dialogLayout.findViewById(R.id.dialog_button_ok);
		dialog_button_cancel = (Button)dialogLayout.findViewById(R.id.dialog_button_cancel);
	}
	
	public void showDialogTop(int connectState, int notifCount,	boolean usbState, boolean sdState) {
		connect_state = connectState;
		currShowPosition = TOP;
		initLauncherDialog();
		params.addRule(RelativeLayout.ALIGN_TOP);
		dialog_position_linear.setLayoutParams(params);
		RelativeLayout dialog_top_layout = (RelativeLayout)dialogLayout.findViewById(R.id.dialog_top_layout);
		dialog_top_layout.setVisibility(View.VISIBLE);
		
		TextView dialog_top_notif = (TextView)dialogLayout.findViewById(R.id.dialog_top_notif);
		dialog_top_notif.setOnClickListener(this);
		dialog_top_notif.setOnKeyListener(this);
		if(notifCount > 0){
			dialog_top_notif.setVisibility(View.VISIBLE);
    		BadgeView badge = new BadgeView(mContext, dialog_top_notif);
    		badge.setText(String.valueOf(notifCount));
    		badge.setTextSize(mContext.getResources().getInteger(R.integer.dialog_badge_view_text_size));
    		badge.setBadgeMargin(mContext.getResources().getInteger(R.integer.dialog_badge_view_margin_horizontal), 
    				mContext.getResources().getInteger(R.integer.dialog_badge_view_margin_vertical));
    		badge.show();
		}
		
		ImageView dialog_top_usb = (ImageView)dialogLayout.findViewById(R.id.dialog_top_usb);
		dialog_top_usb.setOnClickListener(this);
		dialog_top_usb.setOnKeyListener(this);
		dialog_top_usb.setVisibility((usbState || sdState)?View.VISIBLE:View.GONE);
		
		View dialog_top_wifi = dialogLayout.findViewById(R.id.dialog_top_wifi);
		ImageView connect_icon = (ImageView)dialogLayout.findViewById(R.id.dialog_top_wifi_icon);
		connect_icon.setBackgroundResource(NetworkConnectIcon(connectState));
		connect_name = (AnimationText)dialogLayout.findViewById(R.id.dialog_top_wifi_name);
		connect_name.setText(NetworkConnectName(connectState));
		dialog_top_wifi.setOnFocusChangeListener(new OnFocusChangeListener() {
			
			@Override
			public void onFocusChange(View v, boolean hasFocus) {
				String ellipsize = hasFocus?"MARQUEE":"END";
				connect_name.setEllipsize(TextUtils.TruncateAt.valueOf(ellipsize));
			}
		});
		dialog_top_wifi.requestFocus();
		dialog_top_wifi.setOnClickListener(this);
		dialog_top_wifi.setOnKeyListener(this);
		setDialogPosition(TOP);
	}

	public void showDialogBottom() {
		if(isNeverShow){
			return;
		}
		currShowPosition = BOTTOM;
		initLauncherDialog();
		dialog_button_hide = (Button)dialogLayout.findViewById(R.id.dialog_button_hide);
		params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM );
		dialog_position_linear.setLayoutParams(params);
		dialog_button_title.setVisibility(View.VISIBLE);
		dialog_button_ok.setVisibility(View.VISIBLE);
		dialog_button_hide.setVisibility(View.VISIBLE);
		dialog_button_ok.requestFocus();
		dialog_button_cancel.setVisibility(View.VISIBLE);
		setDialogPosition(BOTTOM);
	}

	public void showDialogRigth(int contentId, boolean state) {
		currShowPosition = RIGHT;
		initLauncherDialog();
		params.height = RelativeLayout.LayoutParams.MATCH_PARENT;
		params.width =(int)mContext.getResources().getDimension(R.dimen.launcher_dialog_bg_width);
		params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		dialog_position_linear.setLayoutParams(params);
		dialog_position_linear.setGravity(Gravity.CENTER_HORIZONTAL);
		dialog_position_linear.setBackgroundResource(R.drawable.bg_gradual_black);
		RelativeLayout dialog_right_layout = (RelativeLayout)dialogLayout.findViewById(R.id.dialog_right_layout);
		dialog_right_layout.setVisibility(View.VISIBLE);
		ImageView dialog_right_usb = (ImageView)dialogLayout.findViewById(R.id.dialog_right_usb);
		ImageView dialog_right_sd = (ImageView)dialogLayout.findViewById(R.id.dialog_right_sd);
		switch (contentId) {
		case USB_SHOW:
			dialog_right_sd.setVisibility(View.GONE);
			dialog_right_usb.setBackgroundResource(state?R.drawable.usb_putin:R.drawable.usb_putout);
			dialog_right_usb.setVisibility(View.VISIBLE);
			break;
		case SD_SHOW:
			dialog_right_usb.setVisibility(View.GONE);
			dialog_right_sd.setBackgroundResource(state?R.drawable.sd_putin:R.drawable.sd_putout);
			dialog_right_sd.setVisibility(View.VISIBLE);
			break;

		default:
			break;
		}
		//3s后自动消失
		mHandler.sendEmptyMessageDelayed(DISMISS_DIALOG, 3000);
		setDialogPosition(RIGHT);
		
	}
	
	public void showDialogLeft(){
		currShowPosition = LEFT;
		initLauncherDialog();
		params.height = RelativeLayout.LayoutParams.MATCH_PARENT;
		params.width =(int)mContext.getResources().getDimension(R.dimen.launchrer_dialog_bg_height);
		params.addRule(RelativeLayout.ALIGN_PARENT_LEFT );
		dialog_position_linear.setLayoutParams(params);
		// TODO 组件visible
		RelativeLayout dialog_left_layout = (RelativeLayout)dialogLayout.findViewById(R.id.dialog_left_layout);
		dialog_left_layout.setVisibility(View.VISIBLE);
		
		setDialogPosition(LEFT);
	}
	
    private int NetworkConnectIcon(int connect_state) {
		switch (connect_state) {
		case CONNECT_NO:
			return R.drawable.dialog_wifi_no;
		case CONNECT_WIFI:
			return R.drawable.dialog_wifi_icon;
		case CONNECT_ETHERNET:
			return R.drawable.dialog_ethernet_icon;
		default:
			return R.drawable.dialog_wifi_no;
		}
	}
	
	private String NetworkConnectName(int connect_state) {
		switch (connect_state) {
		case CONNECT_NO:
			return mContext.getString(R.string.no_connected);
		case CONNECT_WIFI:
			WifiManager wifiMgr = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
	        String WifiId = wifiMgr.getConnectionInfo().getSSID().replace("\"", "");
			return WifiId;
		case CONNECT_ETHERNET:
			return mContext.getString(R.string.ethernet);
		default:
			return mContext.getString(R.string.no_connected);
		}
	}
	
	public void setLauncherDialogListener(LauncherDialogListener l) {
		mListener = l;
	}
	
	public boolean isShow() {
		return isShow;
	}
	
	public void dismiss(){
		if(mWifiDialog != null)
			mWifiDialog.dismiss();
	}
	
	public int getShowPosition(){
		return currShowPosition;
	}
	
	public void showLauncherDialog() {
		dialog_button_ok.setOnClickListener(this);
		dialog_button_cancel.setOnClickListener(this);
		if(dialog_button_hide != null){
			dialog_button_hide.setOnClickListener(this);
		}
	}
    
    private void setDialogPosition(int position){
//    	Log.w(TAG, "MM setDialogPosition position = "+position);
    	ExpandNotification(false);
		mWifiDialog = new WifiDialog(mContext, R.style.LauncherDialog);
		mWifiDialog.setContentView(dialogLayout);
		mWifiDialog.showDialog(position);
    }
    
	@Override
	public boolean onKey(View view, int keyCode, KeyEvent event) {
		// TODO 可以添加各种view控件的（上下左右）按键的响应
//		Log.e(TAG, "View_onKey:keyCode="+keyCode+"  KeyEvent="+event);
		switch (view.getId()) {
		case R.id.dialog_top_notif:
		case R.id.dialog_top_usb:
		case R.id.dialog_top_wifi:
			if(KeyEvent.KEYCODE_DPAD_DOWN == keyCode){
				mWifiDialog.dismiss();
				return false;//消费这个按键事件
			}
			break;
		default:
			break;
		}
		return false;
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.dialog_top_notif:
			mWifiDialog.dismiss();
			ExpandNotification(true);
			break;
		case R.id.dialog_top_usb:
			mWifiDialog.dismiss();
			PackageManager pm = mContext.getPackageManager();
			Intent usb_intent = pm.getLaunchIntentForPackage("com.softwinner.TvdFileManager");
			usb_intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			if(usb_intent != null) {
				mContext.startActivity(usb_intent);
				((Activity)mContext).overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
			}
			break;
		case R.id.dialog_top_wifi:
			mWifiDialog.dismiss();
			String url;
			if(CONNECT_ETHERNET == connect_state)
				url = "com.softwinner.tvdsetting.net.ethernet.EthernetSettingActivity";
			else
				url = "com.softwinner.tvdsetting.net.wifi.WifiSettingActivity";
			Intent net_intent = new Intent();  
			net_intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			net_intent.setComponent(new ComponentName("com.softwinner.tvdsetting", url));
			if(net_intent != null) {
				mContext.startActivity(net_intent);
				((Activity)mContext).overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
			}
			break;
			
		case R.id.dialog_button_ok:
			mWifiDialog.dismiss();
			if(null != mListener) {
				mListener.onOK();
			}
			if(!isShow) {
				mHandler.sendEmptyMessageDelayed(DELAY_SHOW_BOTTOM, 1000);
			}
			break;
			
		case R.id.dialog_button_cancel:
			mWifiDialog.dismiss();
			if(null != mListener) {
				mListener.onCancel();
			}
			Intent wifi_intent = new Intent();  
			wifi_intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			wifi_intent.setComponent(new ComponentName("com.softwinner.tvdsetting", 
					"com.softwinner.tvdsetting.net.wifi.WifiSettingActivity"));
			if(wifi_intent != null) {
				mContext.startActivity(wifi_intent);
				((Activity)mContext).overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);
			}
			break;
					
		case R.id.dialog_button_hide:
			mWifiDialog.dismiss();
			isNeverShow = true;
			break;
		default:
			break;
		}
	}
	
	private void ExpandNotification(boolean isExpand){
		String methodName = isExpand? "expandNotificationsPanel":"collapsePanels";
		ReflectionCall.getInstance(mContext).callMethod("android.app.StatusBarManager", 
				methodName, mContext.getSystemService("statusbar"));
	}
	
    private class WifiDialog extends Dialog{
    	private Window window =null;
    	
		public WifiDialog(Context context, int theme) {
			super(context, theme);
		}
		
		public void showDialog( int position){
            windowDeploy(position);
            show();
        }
		
		@Override
		public void show() {
			super.show();
			isShow = true;
		}

		@Override
		public void dismiss() {
			super.dismiss();
			isShow = false;
			currShowPosition = DISMISS;
		}
        //设置窗口显示
        public void windowDeploy( int position){
//        	Log.w(TAG, "MM widowDeploy position = "+position);
            window = getWindow(); //得到对话框
            WindowManager.LayoutParams wl = window.getAttributes();
            wl.dimAmount = 0.80f;  
//            wl.alpha = 0.6f; //设置透明度
         
            // TODO 设置对话框translat动画
        	switch(position){
        	case TOP:
        		window.setWindowAnimations(R.style.dialogWindowAnimTop); //设置窗口弹出动画
        	break;
        	case BOTTOM:
        		 showLauncherDialog();
        		 window.setWindowAnimations(R.style.dialogWindowAnimBottom); 
        	break;
        	case LEFT:
        		 window.setWindowAnimations(R.style.dialogWindowAnimLeft); 
        	break;
        	case RIGHT:
        		 window.setWindowAnimations(R.style.dialogWindowAnimRight); 
        	break;
        	}
        
            window.setAttributes(wl);
        }
     }


	
}

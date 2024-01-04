package com.softwinner.firelauncher.subui;

import java.util.ArrayList;

import android.app.Activity;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.DialogInterface.OnDismissListener;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewTreeObserver;
import android.view.View.OnClickListener;
import android.widget.GridLayout.LayoutParams;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.TextView;

import com.softwinner.animationview.AnimationParent.OnShadowProcessor;
import com.softwinner.animationview.AnimationText;
import com.softwinner.animationview.AnimationView;
import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.section.app.AppBean;
import com.softwinner.firelauncher.section.app.AppListBiz;
import com.softwinner.firelauncher.utils.Constants;

public class InstallGameActivity extends Activity implements OnFocusChangeListener, 
				OnClickListener,OnShadowProcessor {
	
	private String TAG = "InstallGameActivity";
	
	private AppDbDataChangeReceiver receiver;
	public static ArrayList<AppBean> gameApps;
	protected AnimationView mAnimationView;
	private ImageView trash_bt;
	private GridLayout mGridLayout;
	private ArrayList<View> AppListView;
	private PackageManager mPackageManager;
	private Dialog mDeleteAllDialog;
	
	private boolean isMeasured = false;
	private boolean isTrashRturn = false;
	Handler handler = new Handler();
	
	private static int[] BACKGROUND_COLOR = new int[]{R.drawable.bg_green, 
		R.drawable.bg_orange, R.drawable.bg_yellow, R.drawable.bg_bluegreen, 
		R.drawable.bg_blue, R.drawable.bg_darkgreen, R.drawable.bg_red, R.drawable.bg_purple};
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.install_game_gridview);
		
		receiver = new AppDbDataChangeReceiver();
		IntentFilter filter = new IntentFilter();
		filter.addAction(Constants.APP_DB_CHANGE);
		registerReceiver(receiver, filter);
		gameApps = getGameAppList(new AppListBiz(this).getLauncherApps());
		
		trash_bt = (ImageView) findViewById(R.id.trash_bt);
        trash_bt.setOnClickListener(this);
        trash_bt.setOnFocusChangeListener(this);
        trash_bt.setVisibility(gameApps.size() > 0? View.VISIBLE:View.INVISIBLE);
        
        mGridLayout = (GridLayout)findViewById(R.id.functional_zone_gridview);
        mGridLayout.setOnFocusChangeListener(this);
        
	    mAnimationView = new AnimationView(this, findViewById(R.id.animation_layout), false);
        mAnimationView.setitemAnimationIconId(R.id.animation_view);
        mAnimationView.setShadowProcessor(this);
		setViewObserver(mGridLayout);
		updateUI();
		
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		unregisterReceiver(receiver);
	}
	
	@Override
	public void onFocusChange(View view, boolean hasFocus) {
		switch (view.getId()) {
		case R.id.grid_layout_content:
			Log.d(TAG, "mGridLayout  paramBoolean="+hasFocus);
			return;
		case R.id.trash_bt:
			Log.d(TAG, "trash_bt  paramBoolean="+hasFocus);
			return;
		default:
			Log.d(TAG, "viewId="+view.getId()+"  paramBoolean="+hasFocus);
			AnimationText text = (AnimationText) view.findViewById(R.id.item_name);
			text.setEllipsize(TextUtils.TruncateAt.valueOf(hasFocus?"MARQUEE":"END"));
			mAnimationView.ViewOnFocused(view, hasFocus, null);
			return;
		}
	}
	
	@Override
	public void onClick(View view) {
		switch (view.getId()) {
		case R.id.functional_zone_gridview:
			Log.d(TAG, "onClick: mGridLayout");
			return;
		case R.id.trash_bt:
			Log.d(TAG, "onClick: trash_bt");
			isTrashRturn = true;
			trash_bt.setSelected(isTrashRturn);
			mAnimationView.setDeleteMode(isTrashRturn);
			AppListView.get(0).requestFocus();
			return;
		default:
			Log.d(TAG, "onClick: viewId="+view.getId());
			if(AppListView.indexOf(view) < 0) return;
			String mPackageName = gameApps.get(view.getId()).getPackageName();
			if(mAnimationView.isDeleteMode()){
				startUninstallProgress(mPackageName);
			}else{
				Intent intent = mPackageManager.getLaunchIntentForPackage(mPackageName);
				intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				startActivity(intent);
			}
			return;
		}
		
	}

    @Override  
    public boolean onKeyDown(int keyCode, KeyEvent event) {  
    	if(keyCode == KeyEvent.KEYCODE_BACK && isTrashRturn){
    		isTrashRturn = false;
    		mAnimationView.setDeleteMode(isTrashRturn);
    		trash_bt.requestFocus();
    		trash_bt.setSelected(isTrashRturn);
    		if(AppListView.size() > 0){
    			AppListView.get(0).requestFocus();
    			return true;
    		}
		}
        return super.onKeyDown(keyCode, event);  
    } 
    
	public void setViewObserver(final View selectView) {
		ViewTreeObserver observer = selectView.getViewTreeObserver();
		observer.addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
			public boolean onPreDraw() {
				if (!isMeasured) {
					if(gameApps.size() == 0) return true;
					AppListView.get(0).requestFocus();
					trash_bt.setNextFocusDownId(AppListView.get(0).getId());
					isMeasured = true;
				}
				return true;
			
			}
		});
	}
    
	private void updateUI(){
		isMeasured = false;
		mGridLayout.removeAllViews();
		AppListView = new ArrayList<View>();
		resetUi();
	}
	
    private void resetUi(){
		if(gameApps == null || gameApps.size() <= 0){
			trash_bt.setVisibility(View.INVISIBLE);
			return;
		}
		trash_bt.setVisibility(View.VISIBLE);
		View convertView = null;
		int count = mGridLayout.getRowCount();
		int colum = mGridLayout.getColumnCount();
        for (int i = 0; i < gameApps.size(); i++) {
        	LayoutParams gllpTv = new LayoutParams(); 
        	gllpTv.columnSpec = GridLayout.spec(i%colum, 1);//相当于android:layout_columnSpan="5" spec(start,size)参数为起始位置，占几列 
        	gllpTv.rowSpec = GridLayout.spec(count+i/colum);
        	
        	convertView = LayoutInflater.from(this).inflate(R.layout.install_game_griditem, null);
        	convertView.setId(i);
			convertView.setClickable(true);
			convertView.setFocusable(true);
			convertView.setFocusableInTouchMode(true);
			convertView.setOnFocusChangeListener(this);
			convertView.setOnClickListener(this);
			convertView.setNextFocusLeftId(Math.max(0, i-1));
			convertView.setNextFocusRightId(Math.min(i+1, gameApps.size()-1));
			convertView.findViewById(R.id.animation_view).setBackgroundResource(BACKGROUND_COLOR[i%BACKGROUND_COLOR.length]);
			ImageView image = (ImageView) convertView.findViewById(R.id.animation_view_icon);
			image.setImageDrawable(gameApps.get(i).getIcon());
			AnimationText text = (AnimationText) convertView.findViewById(R.id.item_name);
	       	text.setText(gameApps.get(i).getName()); 
        	mGridLayout.addView(convertView,gllpTv);//把控件和布局参数添加到GridLayout 
        	AppListView.add(convertView);
		}
    }
	
	private ArrayList<AppBean> getGameAppList(ArrayList<AppBean> topApps){
		mPackageManager = getPackageManager();
		ArrayList<AppBean> gameList = new ArrayList<AppBean>();
		ContentResolver contentResolver = getContentResolver();
		Uri uri = Uri.parse("content://com.atet.tvmarket.provider/appIsGame");
		for (AppBean appBean : topApps) {
			Cursor c=contentResolver.query(uri,null, "packageName='"+appBean.getPackageName()+"'", null, null);
			if(c!=null && c.getCount()>0){//该包名是ATET的游戏包名
				gameList.add(appBean);
			}
		}

		return gameList;
	}

	//自定义卸载游戏Dialog
	public void showDeleteDialog(final AppBean appBean) {
		View view =LayoutInflater.from(this).inflate(R.layout.dialog_layout, null);
		((TextView)view.findViewById(R.id.disclaimer_content)).setText(
				String.format(getString(R.string.uninstall_app_format), appBean.getName()));
		view.findViewById(R.id.delete_all_ok).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				if (null != mDeleteAllDialog) {
					mDeleteAllDialog.dismiss();
					mDeleteAllDialog = null;
					//TODO：会调用原生卸载的提示框
					startUninstallProgress(appBean.getPackageName());
				}
			}
		});
		view.findViewById(R.id.delete_all_reject).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				if (null != mDeleteAllDialog) {
					mDeleteAllDialog.dismiss();
					mDeleteAllDialog = null;
				}
			}
		});
		
		mDeleteAllDialog = new Dialog(this, R.style.SimpleDialog);
		mDeleteAllDialog.setContentView(view);
		mDeleteAllDialog.setOnDismissListener(new OnDismissListener() {
			@Override
			public void onDismiss(DialogInterface dialog) {
				if (null != dialog){
					dialog.dismiss();
					dialog = null;
				}
			}
		});
		mDeleteAllDialog.show();
	}
	
	private void startUninstallProgress(String PackageName) {
		Uri packageURI = Uri.parse("package:"+PackageName);     
		Intent uninstallIntent = new Intent(Intent.ACTION_UNINSTALL_PACKAGE, packageURI);    
		startActivity(uninstallIntent);  
	}
	
	@Override
	public int getShadowSelect(int onFocusViewId) {
		if(mAnimationView.isDeleteMode)
			return AnimationView.SHADOW_CENTER_ORANGE;
		return AnimationView.SHADOW_CENTER_BLUE;
	}
	
	private class AppDbDataChangeReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			Log.d(TAG, "AppDbDataChangeReceiver: onReceive onReceive onReceive!!!");
			gameApps = getGameAppList(new AppListBiz(InstallGameActivity.this).getLauncherApps());
			updateUI();
		}
	}

}

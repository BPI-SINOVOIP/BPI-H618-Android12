package com.softwinner.firelauncher.subui;

import java.util.ArrayList;

import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.assist.ImageScaleType;
import com.softwinner.animationview.AnimationParent.OnShadowProcessor;
import com.softwinner.animationview.AnimationText;
import com.softwinner.animationview.AnimationView;
import com.softwinner.firelauncher.R;
import com.softwinner.firelauncher.utils.Constants;

import android.app.Activity;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.DialogInterface.OnDismissListener;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.Log;
import android.view.View.OnClickListener;
import android.os.Bundle;
import android.os.Handler;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewTreeObserver;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.GridLayout.LayoutParams;

public class TvHistoryActivity extends Activity implements OnFocusChangeListener, OnClickListener,
								OnShadowProcessor{

	private String TAG = "TvHistoryActivity";
	private boolean DEBUG = true;
	
	private static Context mContext;
	private static Boolean isSendIntent = true;  
	private static int lastItemNumber = -1;
	private Handler mHandler = null;
	private boolean isMeasured = false;
	private boolean isTrashRturn = false;
	private int CreateViewId = 0;
	private GridLayout mGridLayout;
	private ArrayList<Source> mSource;
	private ImageView record_time_line;
	private ImageView today_tab;
	private ImageView yesterday_tab;
	private ImageView before_yesterday_tab;
	private ImageView curr_week_tab;
	private ImageView last_week_tab;
	private ImageView early_tab;
//	private ImageView line_end;
	private ImageView trash_bt;
	private ImageView all_delete_mode;
	private ImageView single_delete_mode;
	private View trash_mode_select;
	private AnimationView mAnimationView;
	private Dialog mDeleteAllDialog;
	private int clickDelayTime ;
	private int trashSetX = 0;
	private TvHistoryTable mTvHistoryTable;
	protected ImageLoader mImageLoader;
	protected DisplayImageOptions optionsH;
	private ArrayList<MangoBean> mTodaySource;
	private ArrayList<MangoBean> mYesterdaySource;
	private ArrayList<MangoBean> mDayBeforeYesterdaySource;
	private ArrayList<MangoBean> mCurrWeekSource;
	private ArrayList<MangoBean> mLastWeekSource;
	private ArrayList<MangoBean> mEarlySource;
	private MangoBean mNextFouseMangoBean;
	private long onKeyDown_lastTime = 0;
	private int lastKeyAction = KeyEvent.ACTION_UP;
	private BroadcastReceiver mHomeKeyEventReceiver = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mContext = getApplicationContext();
		mImageLoader = ImageLoader.getInstance();
		mHandler = new Handler();
		clickDelayTime = getResources().getInteger(R.integer.default_long_click_delay);
		setContentView(R.layout.tv_history_layout);
        
        mGridLayout = (GridLayout)findViewById(R.id.grid_layout_content);
        mGridLayout.setOnFocusChangeListener(this);
        
        record_time_line = (ImageView)findViewById(R.id.record_time_line);
        today_tab = (ImageView)findViewById(R.id.today_tab);
        yesterday_tab = (ImageView)findViewById(R.id.yesterday_tab);
        before_yesterday_tab = (ImageView)findViewById(R.id.before_yesterday_tab);
        curr_week_tab = (ImageView)findViewById(R.id.curr_week_tab);
        last_week_tab = (ImageView)findViewById(R.id.last_week_tab);
        early_tab = (ImageView)findViewById(R.id.early_tab);
//        line_end = (ImageView)findViewById(R.id.line_end);
        trash_bt = (ImageView)findViewById(R.id.trash_bt);
        trash_bt.setOnClickListener(this);
        trash_bt.setOnFocusChangeListener(this);
        trash_mode_select = findViewById(R.id.trash_mode_select);
        all_delete_mode = (ImageView)findViewById(R.id.all_delete_mode);
        all_delete_mode.setOnClickListener(this);
        all_delete_mode.setOnFocusChangeListener(this);
        single_delete_mode = (ImageView)findViewById(R.id.single_delete_mode);
        single_delete_mode.setOnClickListener(this);
        single_delete_mode.setOnFocusChangeListener(this);
        
        mAnimationView = new AnimationView(mContext, findViewById(R.id.animation_layout), false);
        mAnimationView.setitemAnimationIconId(R.id.animation_view);
        mAnimationView.setShadowProcessor(this);
		optionsH = new DisplayImageOptions.Builder()
			.showImageOnLoading(R.drawable.fail_h)
			.showImageForEmptyUri(R.drawable.fail_h)
			.showImageOnFail(R.drawable.fail_h)
			.resetViewBeforeLoading(true)
			.cacheInMemory(true)
			.cacheOnDisk(true)
			.imageScaleType(ImageScaleType.IN_SAMPLE_POWER_OF_2)
			.bitmapConfig(Bitmap.Config.RGB_565)
			.considerExifParams(true)
	//		.displayer(new FadeInBitmapDisplayer(300))
			.build();
		
		setData();
        updateUI();
        setViewObserver(mGridLayout);
        HomeKeyEventRegisterReceiver();
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		if(mHomeKeyEventReceiver != null) {
			unregisterReceiver(mHomeKeyEventReceiver);
			mHomeKeyEventReceiver = null;
		}
	}
	
	private void setData(){
        mTvHistoryTable = TvHistoryTable.getInstance(mContext);
        mTodaySource = mTvHistoryTable.getToday();
        mYesterdaySource = mTvHistoryTable.getYesterday();
        mDayBeforeYesterdaySource = mTvHistoryTable.getDayBeforeYesterday();
        mCurrWeekSource = mTvHistoryTable.getRestOfWeek();
        mLastWeekSource = mTvHistoryTable.getRestOfLastWeek();
        mEarlySource = mTvHistoryTable.getRestOfEarly();
	}
	
	private void updateUI(){
		isMeasured = false;
		CreateViewId = 0;
		mSource = new ArrayList<Source>();
		mGridLayout.removeAllViews();
		resetUi();
		addData(today_tab, mTodaySource);
		addData(yesterday_tab, mYesterdaySource);
		addData(before_yesterday_tab, mDayBeforeYesterdaySource);
		addData(curr_week_tab, mCurrWeekSource);
		addData(last_week_tab, mLastWeekSource);
		addData(early_tab, mEarlySource);
        if(mSource == null || mSource.size() <= 0) {
        	removeAllViewSource();
        }
	}
	
    private void resetUi(){
		record_time_line.setVisibility(View.INVISIBLE);
		trash_mode_select.setVisibility(View.INVISIBLE);
		trash_bt.setVisibility(View.INVISIBLE);
        today_tab.setVisibility(View.INVISIBLE);
        yesterday_tab.setVisibility(View.INVISIBLE);
        before_yesterday_tab.setVisibility(View.INVISIBLE);
        curr_week_tab.setVisibility(View.INVISIBLE);
        last_week_tab.setVisibility(View.INVISIBLE);
        early_tab.setVisibility(View.INVISIBLE);
    }
	
	private void addData(View tabView, ArrayList<MangoBean> sourceData){
		if(sourceData == null || sourceData.size() <= 0) return;
		trash_bt.setVisibility(View.VISIBLE);
		int count = mGridLayout.getRowCount();
		int colum = mGridLayout.getColumnCount();
		Source source = null;
		View convertView = null;
        for (int i = 0; i < sourceData.size(); i++) {
        	LayoutParams gllpTv = new LayoutParams(); 
        	gllpTv.columnSpec = GridLayout.spec(i%colum, 1);//相当于android:layout_columnSpan="5" spec(start,size)参数为起始位置，占几列 
        	gllpTv.rowSpec = GridLayout.spec(count+i/colum);
        	
        	convertView = LayoutInflater.from(mContext).inflate(R.layout.history_grid_layout_item, null);
        	convertView.setId(CreateViewId++);
			convertView.setClickable(true);
			convertView.setFocusable(true);
			convertView.setFocusableInTouchMode(true);
			convertView.setOnFocusChangeListener(this);
			convertView.setOnClickListener(this);
			convertView.setNextFocusLeftId(CreateViewId-2);
			convertView.setNextFocusRightId((i != sourceData.size() - 1)?CreateViewId:(CreateViewId-1));
        	ImageView animation_icon = (ImageView) convertView.findViewById(R.id.animation_view);
        	mImageLoader.displayImage(sourceData.get(i).videoImgUrl, animation_icon, optionsH);
			TextView item_detail = (TextView) convertView.findViewById(R.id.item_detail);
			AnimationText item_name = (AnimationText) convertView.findViewById(R.id.item_name);
			item_detail.setText(String.format(getResources().getString(R.string.episode_count_format), sourceData.get(i).videoIndexCount));
			item_name.setText(sourceData.get(i).videoName);        	
        	mGridLayout.addView(convertView,gllpTv);//把控件和布局参数添加到GridLayout 
        	if(i == 0) 
        		source = new Source(tabView, convertView);
        	source.addSunView(new MangoView(convertView, sourceData.get(i), mSource.size()));  	
		}
        mSource.add(source);   
	}
	
	public void setViewObserver(final View selectView) {
		ViewTreeObserver observer = selectView.getViewTreeObserver();
		observer.addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
			public boolean onPreDraw() {
				if (!isMeasured) {
					if(mSource.size() == 0) return true;
					for (Source source : mSource) {
						source.getTabView().setY(source.getSignView().getTop());
						source.getTabView().setVisibility(View.VISIBLE);
					}
					if(!isTrashRturn){
						setFirstLineFouseUp(R.id.trash_bt);
						mSource.get(0).getSignView().requestFocus();
					}else {
						setFirstLineFouseUp(R.id.single_delete_mode);
						single_delete_mode.setSelected(true);
						trash_mode_select.setVisibility(View.VISIBLE);
						trash_bt.setVisibility(View.VISIBLE);
						getMangoView(mNextFouseMangoBean).view.requestFocus();
					}
					trash_bt.setNextFocusDownId(mSource.get(0).getSignView().getId());
					all_delete_mode.setNextFocusDownId(mSource.get(0).getSignView().getId());
					single_delete_mode.setNextFocusDownId(mSource.get(0).getSignView().getId());
					record_time_line.setVisibility(View.VISIBLE);
//					line_end.setVisibility(View.VISIBLE);
					trashSetX = trash_mode_select.getLeft();
					isMeasured = true;
				}
				return true;
			}
		});
	}
	
	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
    	boolean ret = true; 
        long current = System.currentTimeMillis();
        int keyAction = event.getAction();
        if(keyAction == lastKeyAction &&
        		current - onKeyDown_lastTime < clickDelayTime) {
        	if(DEBUG) Log.d(TAG, "OnLongClickDelta="+ (current - onKeyDown_lastTime)+" || ignored event="+event); 
            ret = true; //consume KeyEvent
        } else {
//        	if(DEBUG) Log.d(TAG, "super.dispatchKeyEvent: delta "+ (current - onKeyDown_lastTime)+" || event="+event); 
            onKeyDown_lastTime = keyAction == lastKeyAction ? current:0;
            lastKeyAction = keyAction;
            ret = super.dispatchKeyEvent(event);
        }
        return ret;
	}
	
    @Override  
    public boolean onKeyDown(int keyCode, KeyEvent event) {  
    	if(keyCode == KeyEvent.KEYCODE_BACK && isTrashRturn){
			mAnimationView.fadeOut(trash_mode_select, trashSetX);
			setFirstLineFouseUp(R.id.trash_bt);
			mAnimationView.setDeleteMode(false);
			single_delete_mode.setSelected(false);
			trash_bt.setNextFocusLeftId(R.id.trash_bt);
			trash_bt.requestFocus();
			((ScrollView)findViewById(R.id.scroll_layout)).smoothScrollTo(0, 0);
			isTrashRturn = false;
			return true;
		}
        return super.onKeyDown(keyCode, event);  
    } 
    
    public void setFirstLineFouseUp(int viewId){
    	if(mSource == null || mSource.size() <= 0) return;
		ArrayList<MangoView> viewFristSource = mSource.get(0).getMangoViewList();
		int length = Math.min(5, viewFristSource.size());
		for (int i = 0; i < length; i++) {
			viewFristSource.get(i).view.setNextFocusUpId(viewId);
		}
    }
    
    private void flyOutTrash(){
    	if(trash_mode_select.getVisibility() != View.INVISIBLE)
    		mAnimationView.fadeOut(trash_mode_select, trashSetX);
		setFirstLineFouseUp(R.id.trash_bt);
		mAnimationView.setDeleteMode(false);
		trash_bt.setNextFocusLeftId(R.id.trash_bt);
		isTrashRturn = false;
    }
    
    private MangoView getMangoView(View view){
		for (Source source : mSource) {
			for (MangoView mangoView : source.getMangoViewList()) {
				if(mangoView.view.equals(view)){
					return mangoView;
				}
			}
		}
		return null;
    }
    
    private MangoView getMangoView(int ViewId){
		for (Source source : mSource) {
			for (MangoView mangoView : source.getMangoViewList()) {
				if(ViewId == mangoView.view.getId()){
					return mangoView;
				}
			}
		}
		return null;
	}
    
    private MangoView getMangoView(MangoBean mangoBean){
		for (Source source : mSource) {
			for (MangoView mangoView : source.getMangoViewList()) {
				if(mangoView.Bean.equals(mangoBean)){
					return mangoView;
				}
			}
		}
		return null;
    }
    
    private void removeViewSource(View view){
    	int pos = -1;
    	MangoBean mMangoBean = getMangoView(view).Bean;
    	MangoView nextMangoView = null;
    	if((nextMangoView = getMangoView(view.getId()+1)) != null)
    		mNextFouseMangoBean = nextMangoView.Bean;
    	else if((nextMangoView = getMangoView(view.getId()-1))!= null)
    		mNextFouseMangoBean = nextMangoView.Bean;
    	else
    		mNextFouseMangoBean = null;
    	
        if((pos = mTodaySource.indexOf(mMangoBean)) != -1){
        	mTodaySource.remove(pos);
        }else if((pos = mYesterdaySource.indexOf(mMangoBean)) != -1){
        	mYesterdaySource.remove(pos);
        }else if((pos = mDayBeforeYesterdaySource.indexOf(mMangoBean)) != -1){
        	mDayBeforeYesterdaySource.remove(pos);
        }else if((pos = mCurrWeekSource.indexOf(mMangoBean)) != -1){
        	mCurrWeekSource.remove(pos);
        }else if((pos = mLastWeekSource.indexOf(mMangoBean)) != -1){
        	mLastWeekSource.remove(pos);
        }else if((pos = mEarlySource.indexOf(mMangoBean)) != -1){
        	mEarlySource.remove(pos);
        }
    }
    
    private void removeAllViewSource(){
        flyOutTrash();
        resetUi();
        mTodaySource = null;
        mYesterdaySource = null;
        mDayBeforeYesterdaySource = null;
        mCurrWeekSource = null;
        mLastWeekSource = null;
        mEarlySource = null;
    }
	
	@Override
	public void onFocusChange(View view, boolean paramBoolean) {
		switch (view.getId()) {
		case R.id.grid_layout:
			Log.d(TAG, "mGridLayout  paramBoolean="+paramBoolean);
			return;
		case R.id.trash_bt:
			Log.d(TAG, "trash_bt  paramBoolean="+paramBoolean);
			if(paramBoolean)
				single_delete_mode.setSelected(false);
			return;
		case R.id.all_delete_mode:
			if(mSource == null || mSource.size() <= 0) return;
			Log.d(TAG, "all_delete_mode  paramBoolean="+paramBoolean);
			if(paramBoolean)
				single_delete_mode.setSelected(false);
			else if(!single_delete_mode.isFocused() && !mAnimationView.isDeleteMode)
				flyOutTrash();
			return;
		case R.id.single_delete_mode:
			Log.d(TAG, "single_delete_mode  paramBoolean="+paramBoolean);
			if(!paramBoolean && !all_delete_mode.isFocused() && !single_delete_mode.isSelected())
				flyOutTrash();
			return;
		default:
			Log.d(TAG, "viewId="+view.getId()+"  paramBoolean="+paramBoolean);
			MangoView mangoView = getMangoView(view.getId());
			if(mangoView != null){
				AnimationText text = (AnimationText) mangoView.view.findViewById(R.id.item_name);
				text.setEllipsize(TextUtils.TruncateAt.valueOf(paramBoolean?"MARQUEE":"END"));
				Source onFouseSource = mSource.get(mangoView.sourceIndex);
				for (Source source : mSource) {
					source.getTabView().setSelected(onFouseSource.equals(source));
				}
			}
			mAnimationView.ViewOnFocused(view, paramBoolean, null);
			return;
		}
	}

	@Override
	public void onClick(View view) {
		switch (view.getId()) {
		case R.id.grid_layout:
			Log.d(TAG, "onClick: mGridLayout");
			return;
		case R.id.trash_bt:
			Log.d(TAG, "onClick: trash_bt");
			if(!isTrashRturn){
				mAnimationView.fadeIn(trash_bt, false, trash_mode_select);
				trash_bt.setNextFocusLeftId(R.id.single_delete_mode);
				single_delete_mode.requestFocus();
				isTrashRturn = true;
			}else{
				flyOutTrash();
				trash_bt.requestFocus();
			}
			return;
		case R.id.all_delete_mode:
			Log.d(TAG, "onClick: all_delete_mode");
			mAnimationView.setDeleteMode(false);
			setFirstLineFouseUp(R.id.all_delete_mode);
			showDeleteAllDialog();
			return;
		case R.id.single_delete_mode:
			Log.d(TAG, "onClick: single_delete_mode");
			mAnimationView.setDeleteMode(true);
			single_delete_mode.setSelected(true);
			setFirstLineFouseUp(R.id.single_delete_mode);
			mSource.get(0).getSignView().requestFocus();
			return;
		default:
			Log.d(TAG, "onClick: viewId="+view.getId());
			Intent intent = new Intent();
			if(mAnimationView.isDeleteMode()){
				intent.setAction("com.tv.totcl.delsingle");
				intent.putExtra("srcApp", "com.softwinner.firelauncher");
				intent.putExtra("videoId",getMangoView(view).Bean.videoId);
				intent.putExtra("identifierType","0");
				mContext.sendBroadcast(intent);
				removeViewSource(view);
				updateUI();
			}else{
				if(!filterClick(getMangoView(view).sourceIndex))return;
				
				intent.setAction("com.starcor.hunan.mgtv");
				intent.putExtra("cmd_ex", "show_video_detail");
				intent.putExtra("video_id", getMangoView(view).Bean.videoId);
				intent.putExtra("video_type", "0");
				intent.putExtra("video_ui_style", "0");
				sendBroadcast(intent);
			}
			return;
		}
		
	}
	
	private Runnable filterClickRun = new Runnable() {
		@Override
		public void run() {
			isSendIntent = true;			
		}
	};
	
	/**
	 * 过滤双击事件
	 * @return true：通过      false：过滤*/
	private boolean filterClick(int position){
		mHandler.removeCallbacks(filterClickRun);
		if(lastItemNumber == position || lastItemNumber == -1){
			mHandler.postDelayed(filterClickRun, Constants.FILTER_CLICK_INTERVAL);
			if(!isSendIntent)return false;
		}
		isSendIntent = false;
		lastItemNumber = position;
		return true;
	}
	
	public void showDeleteAllDialog() {
		View view =LayoutInflater.from(this).inflate(R.layout.dialog_layout, null);
		((TextView)view.findViewById(R.id.disclaimer_content)).setText(R.string.delete_all_text);
		view.findViewById(R.id.delete_all_ok).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				if (null != mDeleteAllDialog) {
					mDeleteAllDialog.dismiss();
					mDeleteAllDialog = null;
					Intent intent = new Intent();
					intent.setAction("com.tv.totcl.delall"); 
					intent.putExtra("srcApp","com.softwinner.firelauncher");
					intent.putExtra("identifierType","0");
					mContext.sendBroadcast(intent); 
					removeAllViewSource();
					updateUI();
				}
			}
		});
		view.findViewById(R.id.delete_all_reject).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				if (null != mDeleteAllDialog) {
					mDeleteAllDialog.dismiss();
					mDeleteAllDialog = null;
					all_delete_mode.requestFocus();
				}
			}
		});
		
		mDeleteAllDialog = new Dialog(this, R.style.SimpleDialog);
		mDeleteAllDialog.setContentView(view);
		mDeleteAllDialog.setOnDismissListener(new OnDismissListener() {
			@Override
			public void onDismiss(DialogInterface dialog) {
				if (null != dialog)
					dialog.dismiss();
			}
		});
		mDeleteAllDialog.show();
	}

	@Override
	public int getShadowSelect(int onFocusViewId) {
		// TODO Auto-generated method stub
		if(mAnimationView.isDeleteMode)
			return AnimationView.SHADOW_CENTER_ORANGE;
		return AnimationView.SHADOW_CENTER_BLUE;
	}
	
	public void HomeKeyEventRegisterReceiver(){
		mHomeKeyEventReceiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {

				String action = intent.getAction();
	            if (action.equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {  
	                String reason = intent.getStringExtra("reason");  
	                if (TextUtils.equals(reason, "homekey")) {  
	                	finish();
	                } 
	            }
			}
		};
		IntentFilter intenFilter = new IntentFilter(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
		registerReceiver(mHomeKeyEventReceiver, intenFilter);
	}
	
	public class Source{
		private View tabView;
		private View signView;
		private ArrayList<MangoView> MangoViewList;
		
		public Source(View tabView, View signView) {
			super();
			this.tabView = tabView;
			this.signView = signView;
			MangoViewList = new ArrayList<MangoView>();
		}

		public View getTabView() {
			return tabView;
		}
		
		public ArrayList<MangoView> getMangoViewList() {
			return MangoViewList;
		}
		
		public View getSignView() {
			return signView;
		}
		
		public void addSunView(MangoView mangoView){
			MangoViewList.add(mangoView);
		}
	}
	
	private class MangoView{
		private View view;
		private MangoBean Bean;
		private int sourceIndex;
		
		public MangoView(View view, MangoBean Bean, int sourceIndex) {
			super();
			this.view = view;
			this.Bean = Bean;
			this.sourceIndex = sourceIndex;
		}
	}
	

	
}

package com.softwinner.firelauncher;

import java.util.ArrayList;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.LinearLayout;
import android.widget.TabHost;
import android.widget.TextView;

import com.android.launcher2.FocusOnlyTabWidget;
import com.softwinner.firelauncher.section.AppSection;
import com.softwinner.firelauncher.section.ChannelSection;
import com.softwinner.firelauncher.section.GameSection;
import com.softwinner.firelauncher.section.RecommendSection;
import com.softwinner.firelauncher.section.SectionParent;
import com.softwinner.firelauncher.section.SettingSection;
import com.softwinner.firelauncher.utils.AppConfig;


public class LauncherTabHost extends TabHost implements
		TabHost.OnTabChangeListener, TabContent.OnPageMovingListener {
	private String TAG = "LauncherTabHost";
	private boolean DEBUG = true;
	private int INVALID_PAGE = -1;
	
	private Context mContext;
	private FocusOnlyTabWidget mTabs;
	private TabContent mContents;
	private ArrayList<SectionParent> mSectionGroup;
	private RecommendSection mRecommend_section;
	private ChannelSection mChannel_section;
	private GameSection mGame_section;
//	private PrecinctSection mPrecinct_section;
	private AppSection mApp_section;
	private SettingSection mSetting_section;
	
	int[] mTabAllLayout = {R.layout.tabindicator_recommand, R.layout.tabindicator_channel, R.layout.tabindicator_game,
					/*R.layout.tabindicator_precinct,*/ R.layout.tabindicator_app, R.layout.tabindicator_settings};
	int[] mTitlesId = { R.string.tab_recommand, R.string.tab_channel, R.string.tab_game, 
			/*R.string.tab_zone,*/ R.string.tab_app, R.string.tab_setting};
	int[] mTabNoGameLayout = {R.layout.tabindicator_recommand, R.layout.tabindicator_channel, 
			R.layout.tabindicator_app, R.layout.tabindicator_settings};
	int[] mTitlesNoGameId = { R.string.tab_recommand, R.string.tab_channel,
			R.string.tab_app, R.string.tab_setting};
	String[] mTitles;
	String[] mTags;
	int[] mTabLayout;
	
	public LauncherTabHost(Context context, AttributeSet attrs) {
		super(context, attrs);
		mContext = context;
	}
	
	@Override
	protected void onFinishInflate() {
		long currentTime = System.currentTimeMillis();
		Log.d(TAG, "~~~~~~~ use " + (currentTime-LauncherApplication.getInstance().getCurrentTime()));
		setup();
		
		mTabs = (FocusOnlyTabWidget)getTabWidget();		
		mContents = (TabContent) findViewById(R.id.launcher_tabcontent);
		mContents.setOnPageMovingListener(this);
		initTitleAndTag();
		initSection();
		for (SectionParent page : mSectionGroup) {
			page.initView();
		}
		
		TabContentFactory contentFactory = new TabContentFactory() {
            public View createTabContent(String tag) {
                return mContents;
            }
        };
        
        int pageNum = mContents.getChildCount();
        if(pageNum > mTitles.length ) {
        	Log.e(TAG, "too many pages");
        }
     
        for(int i=0; i<pageNum; i++) {
        	addTab(newTabSpec(mTags[i]).setIndicator(createTabIndicator(mTitles[i], mTabLayout[i])).setContent(contentFactory));
        }
   
        mTabs.initTab(mSectionGroup);
        setOnTabChangedListener(this);
	}
	
	public FocusOnlyTabWidget getTabView(){
		return mTabs;
	}
	
	@Override
	protected void onDetachedFromWindow() {
//		deinitSection();
		super.onDetachedFromWindow();
	}
	
	private View createTabIndicator(String title, int layoutId){
		View view = LayoutInflater.from(getContext()).inflate(layoutId, null);
		int width = (int)(getResources().getDimension(R.dimen.tabindicator_width));
		int height = (int)(getResources().getDimension(R.dimen.tabindicator_height)*
				getResources().getDisplayMetrics().scaledDensity);
		LinearLayout.LayoutParams tab_layoutparams = new LinearLayout.LayoutParams(MarginLayoutParams.WRAP_CONTENT, height);
		view.setLayoutParams(tab_layoutparams);
		TextView name = (TextView) view.findViewById(R.id.tabindicator_name);
		name.setText(title);
		if(R.layout.tabindicator_recommand == layoutId){
			Animation tabScaleAnimation = AnimationUtils.loadAnimation(mContext, R.anim.tab_scale_anim);  
			tabScaleAnimation.setFillAfter(true);
			name.startAnimation(tabScaleAnimation);
		}else if(R.layout.tabindicator_app == layoutId){
			view.findViewById(R.id.tabindicator_app)
			    .setNextFocusDownId(mApp_section.getAppGridView().get(0).getView().getId());
		}
		return view;
	}
	
	private void initSection(){
		mSectionGroup = new ArrayList<SectionParent>();
		mRecommend_section = (RecommendSection)findViewById(R.id.recommend_section);
		mSectionGroup.add(mRecommend_section);
		mChannel_section = (ChannelSection)findViewById(R.id.channel_section);
		mSectionGroup.add(mChannel_section);
		if(AppConfig.getInstance(mContext).isEnableGame()){
			mGame_section = (GameSection)findViewById(R.id.game_section);
			mSectionGroup.add(mGame_section);
		}
		mApp_section = (AppSection)findViewById(R.id.apps_section);
		mSectionGroup.add(mApp_section);
		mSetting_section = (SettingSection)findViewById(R.id.setting_section);
		mSectionGroup.add(mSetting_section);
	}
	
	public void onResume(){
		for(SectionParent section : mSectionGroup) {
			section.onResume();
		}
	}
	
	public void onPause(){
		for(SectionParent section : mSectionGroup) {
			section.onPause();
		}
	}
	
	public void onDestroy(){
		for(SectionParent section : mSectionGroup) {
			section.onDestroy();
		}
	}
	
	public void updateFont(){
		for(SectionParent section : mSectionGroup) {
			section.updateFont();
		}
	}
	
	private void initTitleAndTag() {
		if(!AppConfig.getInstance(mContext).isEnableGame()){
			mTitles = new String[mTitlesNoGameId.length];
			mTags = new String[mTitlesNoGameId.length];
			mTabLayout = new int[mTabNoGameLayout.length];
			for(int i=0; i<mTitlesNoGameId.length; i++) {
				mTitles[i] = getResources().getString(mTitlesNoGameId[i]);
				mTags[i] = "tag-" + mTitles[i];
				mTabLayout[i] = mTabNoGameLayout[i];
				if(DEBUG) Log.d(TAG, "initTitleAndTag:  tag "+mTags[i] + ", title "+mTitles[i]);
			}
		}else{
			mTitles = new String[mTitlesId.length];
			mTags = new String[mTitlesId.length];
			mTabLayout = new int[mTabAllLayout.length];
			for(int i=0; i<mTitlesId.length; i++) {
				mTitles[i] = getResources().getString(mTitlesId[i]);
				mTags[i] = "tag-" + mTitles[i];
				mTabLayout[i] = mTabAllLayout[i];
				if(DEBUG) Log.d(TAG, "initTitleAndTag:  tag "+mTags[i] + ", title "+mTitles[i]);
			}
		}

	}

	private int getPage(String tag) {
		int page = INVALID_PAGE;
		for(int i=0; i<mContents.getChildCount(); i++) {
			if(tag.equals(mTags[i])) { 
				page = i;
				break;
			}
		}
		return page;
	}
	
	@Override
	public void onTabChanged(String tag) {
		int page = getPage(tag);
		if(DEBUG) Log.d(TAG, "" +page +", " + getCurrentTab());
		if(INVALID_PAGE != page ) 
			mContents.snapToPage2(page);
		else 
			Log.e(TAG, "invalid page");
	}

	@Override
	public void onPageBeginMoving(int curPage, int aimPage) {
		if(DEBUG) Log.d(TAG, ">>>>>>onPageBeginMoving::curPage = "+curPage+"  aimPage = "+aimPage);
		mTabs.setCurrentTab(aimPage);
		for(int i=0; i<mTabs.getChildCount(); i++) {
			TextView name = (TextView) mTabs.getChildAt(i).findViewById(R.id.tabindicator_name);
			if(i == aimPage) {
				if(name.getAnimation() != null)return;
				Animation tabScaleAnimation = AnimationUtils.loadAnimation(mContext, R.anim.tab_scale_anim);  
				tabScaleAnimation.setFillAfter(true);
				name.startAnimation(tabScaleAnimation);
//				name.setTextSize(Utils.px2dip(this, getResources().getDimension(R.dimen.tabindicator_text_size_big)));
			} else {
				name.clearAnimation();
//				name.setTextSize(Utils.px2dip(this, getResources().getDimension(R.dimen.tabindicator_text_size_normal)));
			}
		}
		updateSectionPage(aimPage, curPage);
	}
	
	@Override
	public void onPageEndMoving(int curPage) {
		// TODO Auto-generated method stub

	}

	public void setSelectPage(int aimPage){
		if(aimPage == mContents.getCurrentPageIndex())return;
		getTabView().getChildAt(aimPage).requestFocus();
	}
	
	private void updateSectionPage(int aimPage, int curPage){
		if(aimPage == curPage)return;
		for (int i = 0; i < mSectionGroup.size(); i++) {
			if(i < aimPage){
				mSectionGroup.get(i).reSetSectionSmooth(false);
			}else if(i == aimPage){
				if(mTabs.hasFocus())
					mSectionGroup.get(i).reSetSectionSmooth(true);
				else
					mSectionGroup.get(i).reSetSectionSmooth(aimPage>curPage);
			}else{
				mSectionGroup.get(i).reSetSectionSmooth(true);
			}
		}
	}

}

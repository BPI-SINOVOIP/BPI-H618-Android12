package com.softwinner.firelauncher;


import com.softwinner.animationview.AnimationView;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

public class TabContentFragment extends Fragment implements View.OnFocusChangeListener, View.OnClickListener{
//	private final String TAG = "TabContentFragment";

//	private ArrayList<View> viewList;
//	private ImageView game;
//	private ImageView film;
//	private ImageView setting_startup;
//	private ImageView children_channel;
//	private ImageView featured_poster_1;
//	private ImageView featured_poster_2;
//	private ImageView featured_poster_3;
//	private ImageView featured_poster_4;
//	private ImageView featured_poster_5;	
//	private ImageView featured_poster_6;
//	private ImageView featured_poster_7;
//	private ImageView featured_poster_8;
//	private ImageView featured_poster_9;
//	private ImageView featured_poster_10;
	private AnimationView mAnimationView;
//	private HorizontalScrollView mContentScrollView;

//	private int section_length = 0;

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View view = inflater.inflate(R.layout.tab_content, container, false);
//		viewList = new ArrayList<View>();
		
//		//*******************第一版面************************
//		View section_1 = (View) view.findViewById(R.id.section_1);
//		featured_poster_1 = (ImageView) section_1.findViewById(R.id.featured_poster_1);
//		featured_poster_1.setOnFocusChangeListener(this);
//		featured_poster_1.setOnClickListener(this);
//		viewList.add(featured_poster_1);
//		
//		featured_poster_2 = (ImageView) section_1.findViewById(R.id.featured_poster_2);
//		featured_poster_2.setOnFocusChangeListener(this);
//		featured_poster_2.setOnClickListener(this);
//		viewList.add(featured_poster_2);
//		
//		featured_poster_3 = (ImageView) section_1.findViewById(R.id.featured_poster_3);
//		featured_poster_3.setOnFocusChangeListener(this);
//		featured_poster_3.setOnClickListener(this);
//		viewList.add(featured_poster_3);
//		
//		featured_poster_4 = (ImageView) section_1.findViewById(R.id.history);
//		featured_poster_4.setOnFocusChangeListener(this);
//		featured_poster_4.setOnClickListener(this);
//		viewList.add(featured_poster_4);
//		
//		featured_poster_5 = (ImageView) section_1.findViewById(R.id.featured_poster_5);
//		featured_poster_5.setOnFocusChangeListener(this);
//		featured_poster_5.setOnClickListener(this);
//		viewList.add(featured_poster_5);
//		
//		setting_startup = (ImageView) section_1.findViewById(R.id.collect);
//		setting_startup.setOnFocusChangeListener(this);
//		setting_startup.setOnClickListener(this);
//		viewList.add(setting_startup);
//		
//		game = (ImageView) section_1.findViewById(R.id.search);
//		game.setOnFocusChangeListener(this);
//		game.setOnClickListener(this);
//		viewList.add(game);
//		
//		//*******************第二版面************************
//		featured_poster_6 = (ImageView) section_1.findViewById(R.id.featured_poster_6);
//		featured_poster_6.setOnFocusChangeListener(this);
//		featured_poster_6.setOnClickListener(this);
//		viewList.add(featured_poster_6);
//		
//		featured_poster_7 = (ImageView) section_1.findViewById(R.id.featured_poster_7);
//		featured_poster_7.setOnFocusChangeListener(this);
//		featured_poster_7.setOnClickListener(this);
//		viewList.add(featured_poster_7);
//		
//		featured_poster_8 = (ImageView) section_1.findViewById(R.id.featured_poster_8);
//		featured_poster_8.setOnFocusChangeListener(this);
//		featured_poster_8.setOnClickListener(this);
//		viewList.add(featured_poster_8);
//		
//		featured_poster_9 = (ImageView) section_1.findViewById(R.id.featured_poster_9);
//		featured_poster_9.setOnFocusChangeListener(this);
//		featured_poster_9.setOnClickListener(this);
//		viewList.add(featured_poster_9);
//		
//		featured_poster_10 = (ImageView) section_1.findViewById(R.id.featured_poster_10);
//		featured_poster_10.setOnFocusChangeListener(this);
//		featured_poster_10.setOnClickListener(this);
//		viewList.add(featured_poster_10);
//		
//		film = (ImageView) section_1.findViewById(R.id.hot_video);
//		film.setOnFocusChangeListener(this);
//		film.setOnClickListener(this);
//		viewList.add(film);
//		
//		children_channel = (ImageView) section_1.findViewById(R.id.children_channel);
//		children_channel.setOnFocusChangeListener(this);
//		children_channel.setOnClickListener(this);
//		viewList.add(children_channel);
		
		mAnimationView = new AnimationView(getActivity(), view, false);
//		mContentScrollView = (HorizontalScrollView) view;
		return view;
	}
	
//	public int getScrollX(View paramView){
//		if(0 == section_length)
//			section_length = featured_poster_6.getLeft() - featured_poster_1.getLeft();
//		int rate = (paramView.getLeft() - featured_poster_1.getLeft()) / section_length;
//		return rate * section_length;
//	}
	
	@Override
	public void onClick(View paramView) {
		
	}
	
	@Override
	public void onFocusChange(View paramView, boolean paramBoolean) {
//		if(paramBoolean)
//			mContentScrollView.smoothScrollTo(getScrollX(paramView), paramView.getTop());
		mAnimationView.ViewOnFocused(paramView, paramBoolean, null);
	}

}

package com.softwinner.firelauncher;

import android.content.Context;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;

public class ContentPagerAdapt extends FragmentStatePagerAdapter {
	Context mContext;
	Fragment[] mFragment;
	int[] mTitles = { R.string.tab_recommand, R.string.tab_channel, R.string.tab_zone, R.string.tab_app, R.string.tab_setting	};
	
	public ContentPagerAdapt(FragmentManager fm, Context c) {
		super(fm);
		// TODO Auto-generated constructor stub
		mContext = c;
//		mFragment = new TabContentFragment[mTitles.length];
	}

	@Override
	public Fragment getItem(int pos) {
		// TODO Auto-generated method stub
//		return mFragments[pos];
		return new TabContentFragment();
	}

	@Override
	public CharSequence getPageTitle(int pos) {
		// TODO Auto-generated method stub
		return mContext.getResources().getText(mTitles[pos]);
		
	}
	
	@Override
	public int getCount() {
		// TODO Auto-generated method stub
		return mTitles.length;
	}
	
//	@Override
//	public View getView(int position, View convertView, ViewGroup parent) {  
//        Record record = mRecords.get(position);  
//        if (convertView == null) {  
//            convertView = mInflater.inflate(R.layout.record_list_item,null);  
//            holder = new ViewHolder();  
//            holder.chipView = convertView.findViewById(R.id.chip);  
//            holder.carView = (TextView) convertView.findViewById(R.id.carnumber);  
//            holder.dateView= (TextView) convertView.findViewById(R.id.date);  
//            holder.statusView = (TextView) convertView.findViewById(R.id.status);  
//            holder.subjectView = (TextView) convertView.findViewById(R.id.subject);  
//            holder.selectedView = (ImageView) convertView.findViewById(R.id.selected);  
//            convertView.setTag(holder);  
//        } else {  
//            holder = (ViewHolder) convertView.getTag();  
//        }  
	
//	  public void ShowLauncherDialog(Context mContext,Bundle savedInstanceState) {
//		  LauncherDialog mDialog = new LauncherDialog(mContext);
//			mDialog.onCreate(savedInstanceState);
//			mDialog.show();
//	  }

}

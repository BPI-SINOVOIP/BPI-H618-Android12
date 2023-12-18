package com.softwinner.firelauncher.subui;

import java.util.ArrayList;
import android.content.Context;
import android.util.Log;
import com.softwinner.firelauncher.utils.Constants;
import com.softwinner.firelauncher.utils.DateUtil;

public class TvHistoryTable extends TvTable {
	private String TAG = "TvHistoryTable";
	private boolean DEBUG = false;

	private static TvHistoryTable instance = null;

	public static TvHistoryTable getInstance(Context context) {
		if (null == instance) {
			instance = new TvHistoryTable(context);
		}
		return instance;
	}

	private TvHistoryTable(Context context) {
		super(context, Constants.TVHISTORY_TABLE_NAME);
	}

	public ArrayList<MangoBean> getToday() {
		ArrayList<MangoBean> allBeans = getItems();
		ArrayList<MangoBean> retBeans = new ArrayList<MangoBean>();
		for (MangoBean bean : allBeans) {
			if (DEBUG)
				Log.d(TAG, "getToday " + bean.videoName + ", " + bean.addedDate);
			if (bean.addedDate.equals(DateUtil.getDateDayFormat())) {
				retBeans.add(bean);
			}
		}
		return retBeans;
	}

	public ArrayList<MangoBean> getYesterday() {
		ArrayList<MangoBean> allBeans = getItems();
		ArrayList<MangoBean> retBeans = new ArrayList<MangoBean>();
		for (MangoBean bean : allBeans) {
			long addedDate = DateUtil.parseCalendarDayFormat(bean.addedDate)
					.getTimeInMillis();
			long yesterday = DateUtil.getYesterday();
			if (DEBUG)
				Log.d(TAG, "getYesterday " + bean.videoName + ":  addedDate "
						+ addedDate + ", yesterday " + yesterday); 
			if (yesterday == addedDate) {
				retBeans.add(bean);
			}
		}
		return retBeans;
	}

	public ArrayList<MangoBean> getDayBeforeYesterday() {
		ArrayList<MangoBean> allBeans = getItems();
		ArrayList<MangoBean> retBeans = new ArrayList<MangoBean>();
		for (MangoBean bean : allBeans) {
			long addedDate = DateUtil.parseCalendarDayFormat(bean.addedDate)
					.getTimeInMillis();
			long dayBeforeYesterday = DateUtil.getDayBeforeYesterday();
			if (DEBUG)
				Log.d(TAG, "getDayBeforeYesterday " + bean.videoName
						+ ":  addedDate " + addedDate + ", dayBeforeYesterday "
						+ dayBeforeYesterday);
			if (dayBeforeYesterday == addedDate) {
				retBeans.add(bean);
			}
		}
		return retBeans;
	}

	public ArrayList<MangoBean> getWeek() {
		ArrayList<MangoBean> allBeans = getItems();
		ArrayList<MangoBean> retBeans = new ArrayList<MangoBean>();
		for (MangoBean bean : allBeans) {
			long addedDate = DateUtil.parseCalendarDayFormat(bean.addedDate)
					.getTimeInMillis();
			long firstDayOfWeek = DateUtil
					.getFirstDayOfWeek(
							DateUtil.parseCalendarDayFormat(DateUtil
									.getDateDayFormat())).getTimeInMillis();
			long firstDayOfNextWeek = DateUtil
					.getFirstDayOfNextWeek(
							DateUtil.parseCalendarDayFormat(DateUtil
									.getDateDayFormat())).getTimeInMillis();
			if (DEBUG)
				Log.d(TAG, "getWeek " + bean.videoName + ":  addedDate "
						+ addedDate + ", firstDayOfWeek " + firstDayOfWeek
						+ ", firstDayOfNextWeek " + firstDayOfNextWeek);
			if (addedDate >= firstDayOfWeek && addedDate < firstDayOfNextWeek) {
				retBeans.add(bean);
			}
		}
		return retBeans;
	}
	
	public ArrayList<MangoBean> getRestOfWeek() {
		ArrayList<MangoBean> today = getToday();
		ArrayList<MangoBean> yesterday = getYesterday();
		ArrayList<MangoBean> dayBeforeYesterday = getDayBeforeYesterday();
		ArrayList<MangoBean> week = getWeek();
		ArrayList<MangoBean> notRest = new ArrayList<MangoBean>();
		ArrayList<MangoBean> rest = new ArrayList<MangoBean>();
		
		notRest.addAll(today);
		notRest.addAll(yesterday);
		notRest.addAll(dayBeforeYesterday);
		if (DEBUG)
			for (MangoBean bean : notRest)
				Log.i(TAG, "notRest " + bean.videoName);
		
		for(MangoBean beanX : week) {
			boolean isNotRest = false;
			for(MangoBean beanY : notRest) {
				if(beanX.videoId.equals(beanY.videoId)) {
					isNotRest = true;
					break;
				}
			}
			if( !isNotRest )
				rest.add(beanX);
		}
		
		return rest;
	}

	public ArrayList<MangoBean> getLastWeek() {
		ArrayList<MangoBean> allBeans = getItems();
		ArrayList<MangoBean> retBeans = new ArrayList<MangoBean>();
		for (MangoBean bean : allBeans) {
			long addedDate = DateUtil.parseCalendarDayFormat(bean.addedDate)
					.getTimeInMillis();
			long firstDayOfWeek = DateUtil
					.getFirstDayOfWeek(
							DateUtil.parseCalendarDayFormat(DateUtil
									.getDateDayFormat())).getTimeInMillis();
			long firstDayOfLastWeek = DateUtil.getFirstDayOfLastWeek();
			if (DEBUG)
				Log.d(TAG, "getLastWeek " + bean.videoName + ":  addedDate "
						+ addedDate + ", firstDayOfWeek " + firstDayOfWeek
						+ ", firstDayOfLastWeek " + firstDayOfLastWeek);
			if (firstDayOfLastWeek <= addedDate && addedDate < firstDayOfWeek) {
				retBeans.add(bean);
			}
		}
		return retBeans;
	}
	
	public ArrayList<MangoBean> getRestOfLastWeek() {
		ArrayList<MangoBean> week = getWeek();
		ArrayList<MangoBean> mLastweek = getLastWeek();
		ArrayList<MangoBean> rest = new ArrayList<MangoBean>();
 
		if (DEBUG)
			for (MangoBean bean : week)
				Log.i(TAG, "notRest " + bean.videoName);
		for(MangoBean beanX : mLastweek) {
			boolean isNotRest = false;
			for(MangoBean beanY : week) {
				if(beanX.videoId.equals(beanY.videoId)) {
					isNotRest = true;
					break;
				}
			}
			if( !isNotRest )
				rest.add(beanX);
		}
		
		return rest;
	}

	public ArrayList<MangoBean> getEarly() {
		ArrayList<MangoBean> allBeans = getItems();
		ArrayList<MangoBean> retBeans = new ArrayList<MangoBean>();
		for (MangoBean bean : allBeans) {
			long addedDate = DateUtil.parseCalendarDayFormat(bean.addedDate)
					.getTimeInMillis();
			long firstDayOfLastWeek = DateUtil.getFirstDayOfLastWeek();
			if (DEBUG)
				Log.d(TAG, "getEarly " + bean.videoName + ":  addedDate "
						+ addedDate + ", firstDayOfLastWeek "
						+ firstDayOfLastWeek);
			if (addedDate < firstDayOfLastWeek) {
				retBeans.add(bean);
			} 
		}
		return retBeans;
	}
	
	public ArrayList<MangoBean> getRestOfEarly() {
		ArrayList<MangoBean> early = getEarly();
		ArrayList<MangoBean> mLastweek = getLastWeek();
		ArrayList<MangoBean> rest = new ArrayList<MangoBean>();
 
		if (DEBUG)
			for (MangoBean bean : mLastweek)
				Log.i(TAG, "notRest " + bean.videoName);
		for(MangoBean beanX : early) {
			boolean isNotRest = false;
			for(MangoBean beanY : mLastweek) {
				if(beanX.videoId.equals(beanY.videoId)) {
					isNotRest = true;
					break;
				}
			}
			if( !isNotRest )
				rest.add(beanX);
		}
		
		return rest;
	}
}
 
package com.softwinner.firelauncher.subui;

import android.content.Context;

import com.softwinner.firelauncher.utils.Constants;

public class TvTrackerTable extends TvTable {

	private static TvTrackerTable instance = null;
	
	public static TvTrackerTable getInstance(Context context) {
		if(null == instance) {
			instance = new TvTrackerTable(context);
		}
		return instance;
	}
	
	private TvTrackerTable(Context context) {
		super(context, Constants.TVTRACKER_TABLE_NAME);
	}

}

package com.softwinner.firelauncher.subui;

import android.content.Context;

import com.softwinner.firelauncher.utils.Constants;

public class TvCollectionTable extends TvTable {

	private static TvCollectionTable instance = null;
	
	public static TvCollectionTable getInstance(Context context) {
		if(null == instance) {
			instance = new TvCollectionTable(context);
		}
		return instance;
	}
	
	private TvCollectionTable(Context context) {
		super(context, Constants.TVCOLLECTION_TABLE_NAME);
	}
}
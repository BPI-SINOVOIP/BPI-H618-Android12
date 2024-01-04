/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.launcher2;

import java.util.ArrayList;

import com.softwinner.firelauncher.section.SectionParent;
import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.TabWidget;

public class FocusOnlyTabWidget extends TabWidget {
	private String TAG = "FocusOnlyTabWidget";
	private boolean DEBUG = true;
	private long mTime = System.currentTimeMillis();

	public FocusOnlyTabWidget(Context context) {
		super(context);
	}

	public FocusOnlyTabWidget(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	public FocusOnlyTabWidget(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}
	
	public void initTab(final ArrayList<SectionParent> sectionGroup) {
		final int count = getTabCount();
		for (int i = 0; i < count; ++i) {
			View v = getChildTabViewAt(i);
			v.setOnKeyListener(new OnKeyListener() {
				
				@Override
				public boolean onKey(View view, int keyCode, KeyEvent event) {
					// 焦点从tabwidget回到tabcontent
					if(keyCode == KeyEvent.KEYCODE_DPAD_DOWN){
						View v = sectionGroup.get(getChildTabIndex(view)).getLastFouseView();
						if(v == null)return false;//切页后，焦点清零
						v.requestFocus();
						return true;//消费此事件
					}else {
						return false;
					}
				}
			});
		}
	}

	public View getSelectedTab() {
		final int count = getTabCount();
		for (int i = 0; i < count; ++i) {
			View v = getChildTabViewAt(i);
			if (v.isSelected()) {
				return v;
			}
		}
		return null;
	}
	
	public int getSelectedTabIndex() {
		final int count = getTabCount();
		for (int i = 0; i < count; ++i) {
			View v = getChildTabViewAt(i);
			if (v.isSelected()) {
				return i;
			}
		}
		return -1;
	}

	public int getChildTabIndex(View v) {
		final int tabCount = getTabCount();
		for (int i = 0; i < tabCount; ++i) {
			if (getChildTabViewAt(i) == v) {
				return i;
			}
		}
		return -1;
	}

	public void setCurrentTabToFocusedTab() {
		View tab = null;
		int index = -1;
		final int count = getTabCount();
		for (int i = 0; i < count; ++i) {
			View v = getChildTabViewAt(i);
			if (v.hasFocus()) {
				tab = v;
				index = i;
				break;
			}
		}
		if (index > -1) {
			super.setCurrentTab(index);
			super.onFocusChange(tab, true);
		}
	}

	@Override
	public void onFocusChange(android.view.View v, boolean hasFocus) {
		int index = getChildTabIndex(v);
		long time = System.currentTimeMillis();
		if(DEBUG) Log.i(TAG, "onFocusChange: index "+ index + ", hasFocus " + hasFocus + ",  deltaTime "+ (time-mTime) + ", currentTime "+ time);
		// 通过时间差判断焦点从tabcontent移动到tabwidget
		// FIXME
		if ((time-mTime) > 100 && hasFocus) {
//			getSelectedTab().requestFocus();
			View view = getSelectedTab();
			view.requestFocus();
			super.onFocusChange(view, hasFocus);
		} else {
			super.onFocusChange(v, hasFocus);
		}
		
		mTime = time;
	}

}

/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.launcher.device.apps;

import android.content.Context;
import android.content.Intent.ShortcutIconResource;
import android.content.res.Resources;
import androidx.leanback.widget.ArrayObjectAdapter;
import androidx.leanback.widget.HeaderItem;
import android.util.Log;

import com.android.tv.launcher.BrowseInfoBase;
import com.android.tv.launcher.MenuItem;
import com.android.tv.launcher.R;
import com.android.tv.launcher.util.UriUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;

/**
 * Gets the list of browse headers and browse items.
 */
public class AppsBrowseInfo extends BrowseInfoBase implements ApplicationsState.Callbacks {

	private static final String TAG = "AppsBrowseInfo";
    private static final int DOWNLOADED_ID = 0;
    private static final int SYSTEM_ID = 1;
    private static final int RUNNING_ID = 2;

    private final Context mContext;
    private final HashMap<String, Integer> mAppItemIdList; //保存每一个app的包名，和一个递增的id值
    private int mNextItemId; //上述列表的最大元素的下一个应该用的id值
    private final ApplicationsState mApplicationsState;
    private final ApplicationsState.Session mSession;

    //这个是用来对AppInfo列表进行排序用的对比回调函数
    static class AppInfoComparator implements Comparator<AppInfo> {
        @Override
        public int compare(AppInfo o1, AppInfo o2) {
            return o1.getName().compareToIgnoreCase(o2.getName());
        }
    }

    AppsBrowseInfo(Context context) {
        mContext = context;
        mAppItemIdList = new HashMap<String, Integer>();
        Resources resources = context.getResources();
        mApplicationsState = ApplicationsState.getInstance(context.getApplicationContext());
        mSession = mApplicationsState.newSession(this);
        mNextItemId = 0;
        mRows.put(SYSTEM_ID, new ArrayObjectAdapter());
        mRows.put(DOWNLOADED_ID, new ArrayObjectAdapter());
        mRows.put(RUNNING_ID, new ArrayObjectAdapter());
        updateAppList();
    }
//---------------------------ApplicationsState.Callbacks interface implementions----------
    @Override
    public void onRunningStateChanged(boolean running) {
    }
    @Override
    public void onPackageListChanged() {
        updateAppList();
    }
    @Override
    public void onPackageIconChanged() {
    }
    @Override
    public void onPackageSizeChanged(String packageName) {
    }
    @Override
    public void onAllSizesComputed() {
        updateAppList();
    }
    @Override
    public void onRebuildComplete() {
    }
//---------------------------ApplicationsState.Callbacks interface implementions end----------
    
    //更新显示的图标
    private void updateAppList() {
        synchronized (mApplicationsState.mEntriesMap) {
            ArrayList<ApplicationsState.AppEntry> appEntries = mApplicationsState.mAppEntries;
            if (appEntries != null) {
                ArrayList<AppInfo> appInfos = new ArrayList<AppInfo>(appEntries.size());
                
                for (int i = 0, size = appEntries.size(); i < size; i++) {
                	AppInfo info = new AppInfo(mContext, appEntries.get(i));
                    appInfos.add(info);
                }

                Collections.sort(appInfos, new AppInfoComparator()); //根据appInfo的name进行排序

                ArrayObjectAdapter systemRow = mRows.get(SYSTEM_ID);
                ArrayObjectAdapter downloadedRow = mRows.get(DOWNLOADED_ID);
                ArrayObjectAdapter runningRow = mRows.get(RUNNING_ID);
                systemRow.clear();
                downloadedRow.clear();
                runningRow.clear();
                for (int i = 0, size = appInfos.size(); i < size; i++) {
                    AppInfo info = appInfos.get(i);
                    String packageName = info.getPackageName();
                    Integer itemId = mAppItemIdList.get(packageName);
                    if (itemId == null) {
                        itemId = mNextItemId++;
                        mAppItemIdList.put(packageName, itemId);
                    }
                    ///创建每一个小图标
                    MenuItem menuItem = new MenuItem.Builder()
                            .id(itemId)
                            .title(info.getName())
                            .description(info.getSize())
                            .imageUri(getAppIconUri(mContext, info))
                            .intent(AppManagementActivity.getLaunchIntent(info.getPackageName()))
                            .build();

                    if (info.isSystemApp()) {
                        if (info.isLaunchable()) {
                            systemRow.add(menuItem);
                        }
                    } else {
                        downloadedRow.add(menuItem);
                    }
                    if (!info.isStopped()) {
                        runningRow.add(menuItem);
                    }
                }
            }
        }
    }

    private void loadBrowseHeaders() {
        mHeaderItems.add(new HeaderItem(DOWNLOADED_ID, mContext.getString(R.string.apps_downloaded)));
        mHeaderItems.add(new HeaderItem(SYSTEM_ID, mContext.getString(R.string.apps_system)));
        mHeaderItems.add(new HeaderItem(RUNNING_ID, mContext.getString(R.string.apps_running)));
    }

    void init() {
        mSession.resume();
        loadBrowseHeaders();
    }

    static String getAppIconUri(Context context, AppInfo info) {
        int iconRes = info.getIconResource();
        String iconUri = null;
        if (iconRes != 0) {
            try {
                Resources resources = context.getPackageManager()
                        .getResourcesForApplication(info.getApplicationInfo());
                ShortcutIconResource iconResource = new ShortcutIconResource();
                iconResource.packageName = info.getPackageName();
                iconResource.resourceName = resources.getResourceName(iconRes);
                iconUri = UriUtils.getShortcutIconResourceUri(iconResource).toString();
            } catch (Exception e1) {
                Log.w("AppsBrowseInfo", e1.toString());
            }
        } else {
            iconUri = UriUtils.getAndroidResourceUri(Resources.getSystem(),
                    com.android.internal.R.drawable.sym_def_app_icon); //获取一个默认的icon url
        }

        if (iconUri == null) {
            iconUri = UriUtils.getAndroidResourceUri(context.getResources(),
                    com.android.internal.R.drawable.sym_app_on_sd_unavailable_icon);//获取一个sd卡上不可用的icon url
        }
        return iconUri;
    }
}

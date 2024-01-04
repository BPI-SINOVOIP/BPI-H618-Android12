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

package com.android.tv.launcher;


import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AuthenticatorDescription;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.Intent.ShortcutIconResource;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.inputmethodservice.Keyboard.Row;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ApplicationInfo;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.preference.PreferenceActivity;
import androidx.leanback.widget.ArrayObjectAdapter;
import androidx.leanback.widget.HeaderItem;
import androidx.leanback.widget.ObjectAdapter;
import androidx.leanback.widget.ListRow;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.util.Xml;
import android.view.KeyEvent;
import android.widget.Toast;

import com.android.internal.util.XmlUtils;
import com.android.tv.launcher.accessories.AccessoryUtils;
import com.android.tv.launcher.accessories.BluetoothAccessoryActivity;
import com.android.tv.launcher.accessories.BluetoothConnectionsManager;
import com.android.tv.launcher.accounts.AccountImageUriGetter;
import com.android.tv.launcher.accounts.AccountSettingsActivity;
import com.android.tv.launcher.accounts.AddAccountWithTypeActivity;
import com.android.tv.launcher.accounts.AuthenticatorHelper;
import com.android.tv.launcher.connectivity.ConnectivityStatusIconUriGetter;
import com.android.tv.launcher.connectivity.ConnectivityStatusTextGetter;
import com.android.tv.launcher.connectivity.WifiNetworksActivity;
import com.android.tv.launcher.device.apps.AppInfo;
import com.android.tv.launcher.device.apps.AppManagementActivity;
import com.android.tv.launcher.device.apps.ApplicationsState;
import com.android.tv.launcher.device.sound.SoundActivity;
import com.android.tv.launcher.users.RestrictedProfileActivity;
import com.android.tv.launcher.util.UriUtils;
import com.android.tv.launcher.util.AccountImageHelper;
import com.android.tv.launcher.util.UsbSDCardBroadCastReceiver;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * Gets the list of browse headers and browse items.
 */
public class BrowseInfo extends BrowseInfoBase implements ApplicationsState.Callbacks,
                                                        UsbSDCardBroadCastReceiver.Callbacks{

    private static final String TAG = "BrowseInfo";
    private static final boolean DEBUG = false;

    public static final String EXTRA_ACCESSORY_ADDRESS = "accessory_address";
    public static final String EXTRA_ACCESSORY_NAME = "accessory_name";
    public static final String EXTRA_ACCESSORY_ICON_ID = "accessory_icon_res";

    private static final String ACCOUNT_TYPE_GOOGLE = "com.google";

    private static final String ETHERNET_PREFERENCE_KEY = "ethernet";

    interface XmlReaderListener {
        void handleRequestedNode(Context context, XmlResourceParser parser, AttributeSet attrs)
                throws org.xmlpull.v1.XmlPullParserException, IOException;
    }
    static class XmlReader {
        private final Context mContext;
        private final int mXmlResource;
        private final String mRootNodeName;
        private final String mNodeNameRequested;
        private final XmlReaderListener mListener;
        XmlReader(Context context, int xmlResource, String rootNodeName, String nodeNameRequested,
                XmlReaderListener listener) {
            mContext = context;
            mXmlResource = xmlResource;
            mRootNodeName = rootNodeName;
            mNodeNameRequested = nodeNameRequested;
            mListener = listener;
        }
        void read() {
            XmlResourceParser parser = null;
            try {
                parser = mContext.getResources().getXml(mXmlResource);
                AttributeSet attrs = Xml.asAttributeSet(parser);

                int type;
                while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                        && type != XmlPullParser.START_TAG) {
                    // Parse next until start tag is found
                }

                String nodeName = parser.getName();
                if (!mRootNodeName.equals(nodeName)) {
                    throw new RuntimeException("XML document must start with <" + mRootNodeName
                            + "> tag; found" + nodeName + " at " + parser.getPositionDescription());
                }

                Bundle curBundle = null;
                final int outerDepth = parser.getDepth();
                while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                        && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
                        if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                            continue;
                    }

                    nodeName = parser.getName();
                    if (mNodeNameRequested.equals(nodeName)) {
                        mListener.handleRequestedNode(mContext, parser, attrs);
                    } else {
                        XmlUtils.skipCurrentTag(parser);
                    }
                }
            } catch (XmlPullParserException e) {
                throw new RuntimeException("Error parsing headers", e);
            } catch (IOException e) {
                throw new RuntimeException("Error parsing headers", e);
            } finally {
                if (parser != null)
                    parser.close();
            }
        }
    }

    //-----------------------------ApplicationState.CallBack implements----add by zhangmm----
    @Override
    public void onRunningStateChanged(boolean running)
    {
    }
    @Override
    public void onPackageListChanged()//add or remove a app
    {
        updateInstalledAppList();
    }
    @Override
    public void onRebuildComplete()
    {
    }
    @Override
    public void onPackageIconChanged()
    {
    }
    @Override
    public void onPackageSizeChanged(String packageName)
    {
    }
    @Override
    public void onAllSizesComputed()
    {
        updateInstalledAppList();
    }
   // -----------------------------ApplicationState.CallBack implements end--------
    static class AppInfoComparator implements Comparator<AppInfo> {
        @Override
        public int compare(AppInfo o1, AppInfo o2) {
            return o1.getName().compareToIgnoreCase(o2.getName());
        }
    }
    //更新安装的应用的图标
    public void updateInstalledAppList() {
        synchronized (mApplicationsState.mEntriesMap) {
            int tmpItemId = mAppNextItemId;//从4开始递增id
            //ArrayObjectAdapter appRow = mDefaultAppRow;// 直接赋值会变成追加，貌似是强引用，所以，clone一个,这个地方待优化
            ArrayObjectAdapter appRow = mRows.get(mAppHeaderId);
            appRow.clear();
            for(int i=0;i < mDefaultAppRow.size();i++)
                appRow.add(mDefaultAppRow.get(i));
            ArrayList<ApplicationsState.AppEntry> appEntries = mApplicationsState.mAppEntries;
            if (appEntries != null) {
                ArrayList<AppInfo> appInfos = new ArrayList<AppInfo>(appEntries.size());
                for (int i = 0, size = appEntries.size(); i < size; i++) {
                    AppInfo info = new AppInfo(mContext, appEntries.get(i));
                    appInfos.add(info);
                }
             //  Collections.sort(appInfos, new AppInfoComparator()); //暂时先不排序
                for (int i = 0, size = appInfos.size(); i < size; i++) {
                    AppInfo info = appInfos.get(i);
                    if (info.isSystemApp()) { //跳过系统应用
                        continue;
                    }
                    String packageName = info.getPackageName();
                    ///创建每一个小图标
                    MenuItem menuItem = new MenuItem.Builder()
                            .id(tmpItemId++)
                            .title(info.getName())
                            .description(info.getSize())
                            .imageUri(getAppIconUri(packageName))
                            .intent(getLauncherIntent(info.getPackageName()))
                            .build();
                    appRow.add(menuItem);
                }
            }
            mRows.put(mAppHeaderId,appRow);//替换原来的行内容
        }
    }
    private static final String PREF_KEY_ADD_ACCOUNT = "add_account";
    private static final String PREF_KEY_ADD_ACCESSORY = "add_accessory";
    private static final String PREF_KEY_WIFI = "network";
    private static final String PREF_KEY_DEVELOPER = "developer";
    private static final String PREF_KEY_INPUTS = "inputs";
    private static final String PREF_KEY_FAVS = "love";

    private final Context mContext;
    private final AuthenticatorHelper mAuthenticatorHelper;
    private int mDeviceHeaderId; //设备 一行的ID
    private int mNextItemId;
    private int mAccountHeaderId;
    private int mAppHeaderId; //表示“应用”那一行的ID
    private int mAppNextItemId; //安装的应用图标的递增id值
    private final ArrayObjectAdapter mDefaultAppRow = new ArrayObjectAdapter(); //保存前边4个默认的app

    private final BluetoothAdapter mBtAdapter;
    private final Object mGuard = new Object();
    private MenuItem mWifiItem = null;
    private ArrayObjectAdapter mWifiRow = null;
    private final Handler mHandler = new Handler();
    private ArrayList<String> mFavApps;

    private PreferenceUtils mPreferenceUtils;
    private boolean mDeveloperEnabled;
    private boolean mInputSettingNeeded;

    //add to show the installed app in the "app" row
    private ApplicationsState mApplicationsState;
    private ApplicationsState.Session mSession;

    BrowseInfo(Context context) {
        mContext = context;
        UsbSDCardBroadCastReceiver.getInstance(this,context);
        mApplicationsState = ApplicationsState.getInstance(context.getApplicationContext());
        mSession = mApplicationsState.newSession(this);
        mAuthenticatorHelper = new AuthenticatorHelper();
        mAuthenticatorHelper.updateAuthDescriptions(context);
        mAuthenticatorHelper.onAccountsUpdated(context, null);
        mBtAdapter = BluetoothAdapter.getDefaultAdapter();
        mNextItemId = 0;
        mPreferenceUtils = new PreferenceUtils(context);
        mDeveloperEnabled = mPreferenceUtils.isDeveloperEnabled();
        mInputSettingNeeded = isInputSettingNeeded();
        mAppHeaderId = 0;
        mAppNextItemId = 0;
        mDeviceHeaderId = 0;
    }

    void init() {
        synchronized (mGuard) {
            mAppNextItemId = 0;
            mAppHeaderId = 0;
            mDeviceHeaderId = 0;
            mSession.resume(); //注册监听安装和卸载信息的Receiver
            parseHeaderFromMainXML(); //这个在解析完header之后会继续解析其中的子项，比如device
            updateStorageItem();
        }
    }

    void parseHeaderFromMainXML(){
        mHeaderItems.clear();
        mRows.clear();
        mDefaultAppRow.clear();
        int settingsXml = isRestricted() ? R.xml.restricted_main : R.xml.main;
        new XmlReader(mContext, settingsXml, "preference-headers", "header",new HeaderXmlReaderListener())
            .read();
    }

    //解析main.xml
    private class HeaderXmlReaderListener implements XmlReaderListener {
        @Override
        public void handleRequestedNode(Context context, XmlResourceParser parser,AttributeSet attrs)
            throws XmlPullParserException, IOException {
            TypedArray sa = mContext.getResources().obtainAttributes(attrs,
                    com.android.internal.R.styleable.PreferenceHeader);
            final int headerId = sa.getResourceId(com.android.internal.R.styleable.PreferenceHeader_id,
                    (int) PreferenceActivity.HEADER_ID_UNDEFINED);
            String title = getStringFromTypedArray(sa,
                    com.android.internal.R.styleable.PreferenceHeader_title);
            sa.recycle();
            sa = context.getResources().obtainAttributes(attrs, R.styleable.CanvasSettings);
            int preferenceRes = sa.getResourceId(R.styleable.CanvasSettings_preference, 0);
            sa.recycle();

            mHeaderItems.add(new HeaderItem(headerId, title));
            final ArrayObjectAdapter currentRow = new ArrayObjectAdapter();
            if(headerId == R.id.device)
            {
                mDeviceHeaderId = headerId;
            }
            mRows.put(headerId, currentRow);
            new XmlReader(context, preferenceRes, "PreferenceScreen", "Preference",
                new PreferenceXmlReaderListener(headerId, currentRow)).read();
        }
    }

    private boolean isRestricted() {
        return RestrictedProfileActivity.isRestrictedProfileInEffect(mContext);
    }

    //解析每一行内容
    private class PreferenceXmlReaderListener implements XmlReaderListener {
        private final int mHeaderId;
        private final ArrayObjectAdapter mRow;

        PreferenceXmlReaderListener(int headerId, ArrayObjectAdapter row) {
            mHeaderId = headerId;
            mRow = row;
        }
        @Override
        public void handleRequestedNode(Context context, XmlResourceParser parser,
                AttributeSet attrs) throws XmlPullParserException, IOException {
            TypedArray sa = context.getResources().obtainAttributes(attrs,com.android.internal.R.styleable.Preference);
            String key = getStringFromTypedArray(sa,com.android.internal.R.styleable.Preference_key);
            String title = getStringFromTypedArray(sa,com.android.internal.R.styleable.Preference_title); //"com.softwinner.TvdFileManager"
            int iconRes = sa.getResourceId(com.android.internal.R.styleable.Preference_icon,R.drawable.settings_default_icon);
            sa.recycle();
            //if(key.contains("app")) Error: 这个会在解析device行的apps的时候出错
            if(key.equals("app1") || key.equals("app2") || key.equals("app3") || key.equals("app4") || key.equals("app5")){//"应用"行
                String label = getAppShortName(title);
                MenuItem menuItem = new MenuItem.Builder()
                         .id(mNextItemId++)
                         .title(label)
                         .description(label)
                         .imageUri(getAppIconUri(title))
                         .intent(getLauncherIntent(title))
                         .build();
                 mRow.add(menuItem);
                 mDefaultAppRow.add(menuItem);
                 mAppHeaderId = mHeaderId; //这个ID要在后边用来查找其row之后添加安装的应用时用
                 mAppNextItemId = mNextItemId; //后边添加的app要接着使用这个id
            }else if ((!key.equals(PREF_KEY_DEVELOPER) || mDeveloperEnabled)
                    && (!key.equals(PREF_KEY_INPUTS) || mInputSettingNeeded)) {
                MenuItem.TextGetter descriptionGetter = getDescriptionTextGetterFromKey(key);
                MenuItem.UriGetter uriGetter = getIconUriGetterFromKey(key);
                MenuItem.Builder builder = new MenuItem.Builder()
                        .id(mNextItemId++)
                        .title(title)
                        .descriptionGetter(descriptionGetter)
                        .intent(getIntent(parser, attrs, mHeaderId));
                if(uriGetter == null) {
                    builder.imageResourceId(mContext, iconRes);
                } else {
                    builder.imageUriGetter(uriGetter);
                }
                if (key.equals(PREF_KEY_WIFI)) {
                    mWifiItem = builder.build();
                    mRow.add(mWifiItem);
                    mWifiRow = mRow;
                } else {
                    mRow.add(builder.build());
                }
            }
        }
    }

    final String PKG_APTOIDETV = "cm.aptoidetv.pt";
    final String PKG_CAMERA = "com.android.camera2";
    final String PKG_GALLERY = "com.android.gallery3d";
    final String PKG_MIRACAST = "com.softwinner.miracastReceiver";
    final String PKG_MUSIC = "com.android.music";
    final String PKG_PANOPLAYER = "com.allwinnertech.panoplayer";
    final String PKG_TVDFILEMANAGER = "com.softwinner.TvdFileManager";
    final String PKG_AIRPLAY = "com.hpplay.happyplay.aw";
    final String PKG_HDMI = "com.allwinner.hdmi";

    private final Map<String, String> LOVE_APP_CPN = new HashMap<String, String>() {
        {
            put("cm.aptoidetv.pt", "cm.aptoidetv.pt.activity.MainActivity");
            put("com.softwinner.TvdFileManager", "com.softwinner.TvdFileManager.MainUI");
            put("com.android.gallery3d", "com.android.gallery3d.app.GalleryActivity");
            put("com.softwinner.miracastReceiver", "com.softwinner.miracastReceiver.Miracast");
            put("com.android.music", "com.android.music.MusicBrowserActivity");
        }
    };

    String getAppIconUri(String pkgname) {
        String iconUri = null;
        if(PKG_APTOIDETV.equals(pkgname)){
            iconUri = UriUtils.getAndroidResourceUri(mContext,R.drawable.aw_icon_aptoidetv);
            return iconUri;
        }else if(PKG_CAMERA.equals(pkgname)){
            iconUri = UriUtils.getAndroidResourceUri(mContext,R.drawable.aw_icon_camera);
            return iconUri;
        }else if(PKG_GALLERY.equals(pkgname)){
            iconUri = UriUtils.getAndroidResourceUri(mContext,R.drawable.aw_icon_gallery);
            return iconUri;
        }else if(PKG_MIRACAST.equals(pkgname)){
            iconUri = UriUtils.getAndroidResourceUri(mContext,R.drawable.aw_icon_miracast);
            return iconUri;
        }else if(PKG_MUSIC.equals(pkgname)){
            iconUri = UriUtils.getAndroidResourceUri(mContext,R.drawable.aw_icon_music);
            return iconUri;
        }else if(PKG_PANOPLAYER.equals(pkgname)){
            iconUri = UriUtils.getAndroidResourceUri(mContext,R.drawable.aw_icon_panoplayer);
            return iconUri;
        }else if(PKG_TVDFILEMANAGER.equals(pkgname)){
            iconUri = UriUtils.getAndroidResourceUri(mContext,R.drawable.aw_icon_tvdfilemanager);
            return iconUri;
        }else if(PKG_AIRPLAY.equals(pkgname)){
            iconUri = UriUtils.getAndroidResourceUri(mContext,R.drawable.aw_icon_airplay);
            return iconUri;
        }else if(PKG_HDMI.equals(pkgname)){
            iconUri = UriUtils.getAndroidResourceUri(mContext,R.drawable.aw_icon_hdmi);
            return iconUri;
        }

        PackageManager pm = mContext.getPackageManager();
        ApplicationInfo appinfo = null;
        try{
            appinfo = pm.getApplicationInfo(pkgname,0);
        }catch(PackageManager.NameNotFoundException e){
            Log.e(TAG,"IKE can't find apps");
        }
        if(appinfo!=null){
            try {
                Resources resources = mContext.getPackageManager().getResourcesForApplication(appinfo);
                ShortcutIconResource iconResource = new ShortcutIconResource();
                iconResource.packageName = pkgname;
                iconResource.resourceName = resources.getResourceName(appinfo.icon);
                iconUri = UriUtils.getShortcutIconResourceUri(iconResource).toString();
            } catch (Exception e1) {
                iconUri = UriUtils.getAndroidResourceUri(Resources.getSystem(),
                    com.android.internal.R.drawable.sym_def_app_icon);
                Log.w("AppsBrowseInfo", e1.toString());
            }
        }
        return iconUri;
    }
    String getAppShortName(String pkgname){
      PackageManager pm = mContext.getPackageManager();
      ApplicationInfo appinfo = null;
      try{
         appinfo = pm.getApplicationInfo(pkgname,0);
      }catch(PackageManager.NameNotFoundException e){
        Log.e(TAG,"IKE can't find apps");
      }
      if(appinfo!=null)
          return (String) appinfo.loadLabel(pm);
      else
          return "";
    }

    Intent getLauncherIntent(String pkgname){
        Log.d(TAG,"getLauncherIntent " + pkgname);
        if(pkgname.equals("com.android.camera2")){ //这个包名很奇怪，没有找到intent,只找到了Name和icon,所以这里直接自己构造一个
            Intent it = new Intent();
            ComponentName componentName = new ComponentName("com.android.camera2", "com.android.camera.CameraActivity");
            it.setComponent(componentName);
            it.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
            return it;
        }
        PackageManager pm = mContext.getPackageManager();
        Intent lintent = pm.getLeanbackLaunchIntentForPackage(pkgname);
        if (lintent == null) {
            lintent = pm.getLaunchIntentForPackage(pkgname);
        }

        if (lintent == null && LOVE_APP_CPN.containsKey(pkgname)) {
            lintent = new Intent();
            ComponentName componentName = new ComponentName(pkgname, LOVE_APP_CPN.get(pkgname));
            lintent.setComponent(componentName);
            lintent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }

        Log.d(TAG,"getLauncherIntent is " + lintent);
        return lintent;
    }
    void checkForDeveloperOptionUpdate() {
        final boolean developerEnabled = mPreferenceUtils.isDeveloperEnabled();
        if (developerEnabled != mDeveloperEnabled) {
            mDeveloperEnabled = developerEnabled;
        }
    }
    void rebuildInfo() {
        init();
    }
    void updateAccounts() {
        /*synchronized (mGuard) {
            if (isRestricted()) {
                // We don't display the accounts in restricted mode
                return;
            }
            ArrayObjectAdapter row = mRows.get(mAccountHeaderId);
            // Clear any account row cards that are not "Location" or "Security".
            String dontDelete[] = new String[2];
            dontDelete[0] = mContext.getString(R.string.system_location);
            dontDelete[1] = mContext.getString(R.string.system_security);
            int i = 0;
            while (i < row.size ()) {
                MenuItem menuItem = (MenuItem) row.get(i);
                String title = menuItem.getTitle ();
                boolean deleteItem = true;
                for (int j = 0; j < dontDelete.length; ++j) {
                    if (title.equals(dontDelete[j])) {
                        deleteItem = false;
                        break;
                    }
                }
                if (deleteItem) {
                    row.removeItems(i, 1);
                } else {
                    ++i;
                }
            }
            // Add accounts to end of row.
            addAccounts(row);
        }*/
    }

    void updateAccessories() {
        
       /* synchronized (mGuard) {
            updateAccessories(R.id.accessories);
        }*/
    }

    public void updateWifi(final boolean isEthernetAvailable) {
        if (mWifiItem != null) {
            int index = mWifiRow.indexOf(mWifiItem);
            if (index >= 0) {
                mWifiItem = new MenuItem.Builder().from(mWifiItem)
                        .title(mContext.getString(isEthernetAvailable
                                    ? R.string.connectivity_network : R.string.connectivity_wifi))//网络/WLAN
                        .build();
                mWifiRow.replace(index, mWifiItem);
            }
        }
    }

    private boolean isInputSettingNeeded() {
        TvInputManager manager = (TvInputManager) mContext.getSystemService(
                Context.TV_INPUT_SERVICE);
        if (manager != null) {
            for (TvInputInfo input : manager.getTvInputList()) {
                if (input.isPassthroughInput()) {
                    return true;
                }
            }
        }
        return false;
    }

    private void updateAccessories(int headerId) {
       /* ArrayObjectAdapter row = mRows.get(headerId);
        row.clear();
        addAccessories(row);
        // Add new accessory activity icon
        ComponentName componentName = new ComponentName("com.android.tv.launcher",
                "com.android.tv.launcher.accessories.AddAccessoryActivity");
        Intent i = new Intent().setComponent(componentName);
        i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
        row.add(new MenuItem.Builder().id(mNextItemId++)
                .title(mContext.getString(R.string.accessories_add))
                .imageResourceId(mContext, R.drawable.ic_settings_bluetooth)
                .intent(i).build());*/
    }

    private Intent getIntent(XmlResourceParser parser, AttributeSet attrs, int headerId)
            throws org.xmlpull.v1.XmlPullParserException, IOException {
        Intent intent = null;
        if (parser.next() == XmlPullParser.START_TAG && "intent".equals(parser.getName())) {
            TypedArray sa = mContext.getResources()
                    .obtainAttributes(attrs, com.android.internal.R.styleable.Intent);
            String targetClass = getStringFromTypedArray(
                    sa, com.android.internal.R.styleable.Intent_targetClass);
            String targetPackage = getStringFromTypedArray(
                    sa, com.android.internal.R.styleable.Intent_targetPackage);
            String action = getStringFromTypedArray(
                    sa, com.android.internal.R.styleable.Intent_action);
            if (targetClass != null && targetPackage != null) {
                ComponentName componentName = new ComponentName(targetPackage, targetClass);
                intent = new Intent();
                intent.setComponent(componentName);
            } else if (action != null) {
                intent = new Intent(action);
            }
            XmlUtils.skipCurrentTag(parser);
        }
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
        return intent;
    }

    private String getStringFromTypedArray(TypedArray sa, int resourceId) {
        String value = null;
        TypedValue tv = sa.peekValue(resourceId);
        if (tv != null && tv.type == TypedValue.TYPE_STRING) {
            if (tv.resourceId != 0) {
                value = mContext.getString(tv.resourceId);
            } else {
                value = tv.string.toString();
            }
        }
        return value;
    }

    private MenuItem.TextGetter getDescriptionTextGetterFromKey(String key) {
        if (WifiNetworksActivity.PREFERENCE_KEY.equals(key)) {
            return ConnectivityStatusTextGetter.createWifiStatusTextGetter(mContext);
        }
        if (ETHERNET_PREFERENCE_KEY.equals(key)) {
            return ConnectivityStatusTextGetter.createEthernetStatusTextGetter(mContext);
        }
        return null;
    }

    private MenuItem.UriGetter getIconUriGetterFromKey(String key) {
        if (WifiNetworksActivity.PREFERENCE_KEY.equals(key)) {
            return ConnectivityStatusIconUriGetter.createWifiStatusIconUriGetter(mContext);
        }
        return null;
    }

    private void addAccounts(ArrayObjectAdapter row) {
        AccountManager am = AccountManager.get(mContext);
        AuthenticatorDescription[] authTypes = am.getAuthenticatorTypes();
        ArrayList<String> allowableAccountTypes = new ArrayList<>(authTypes.length);
        PackageManager pm = mContext.getPackageManager();

        int googleAccountCount = 0;
        for (AuthenticatorDescription authDesc : authTypes) {
            Resources resources = null;
            try {
                resources = pm.getResourcesForApplication(authDesc.packageName);
            } catch (NameNotFoundException e) {
                Log.e(TAG, "IKE Authenticator description with bad package name", e);
                continue;
            }

            allowableAccountTypes.add(authDesc.type);

            // Main title text comes from the authenticator description (e.g. "Google").
            String authTitle = null;
            try {
                authTitle = resources.getString(authDesc.labelId);
                if (TextUtils.isEmpty(authTitle)) {
                    authTitle = null;  // Handled later when we add the row.
                }
            } catch (NotFoundException e) {
                Log.e(TAG, "Authenticator description with bad label id", e);
            }
            Account[] accounts = am.getAccountsByType(authDesc.type);

            // Icon URI to be displayed for each account is based on the type of authenticator.
            String imageUri = null;
            if (ACCOUNT_TYPE_GOOGLE.equals(authDesc.type)) {
                googleAccountCount = accounts.length;
                imageUri = googleAccountIconUri(mContext);
            } else {
                imageUri = Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://" +
                        authDesc.packageName + '/' +
                        resources.getResourceTypeName(authDesc.iconId) + '/' +
                        resources.getResourceEntryName(authDesc.iconId))
                        .toString();
            }
            // Display an entry for each installed account we have.
            for (final Account account : accounts) {
                Intent i = new Intent(mContext, AccountSettingsActivity.class)
                        .putExtra(AccountSettingsActivity.EXTRA_ACCOUNT, account.name);
                i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                row.add(new MenuItem.Builder().id(mNextItemId++)
                        .title(authTitle != null ? authTitle : account.name)
                        .imageUri(imageUri)
                        .description(authTitle != null ? account.name : null)
                        .intent(i)
                        .build());
            }
        }

        // Never allow restricted profile to add accounts.
        if (!isRestricted()) {
            // If there's already a Google account installed, disallow installing a second one.
            if (googleAccountCount > 0) {
                allowableAccountTypes.remove(ACCOUNT_TYPE_GOOGLE);
            }
            // If there are available account types, add the "add account" button.
            if (!allowableAccountTypes.isEmpty()) {
                Intent i = new Intent().setComponent(new ComponentName("com.android.tv.launcher",
                        "com.android.tv.launcher.accounts.AddAccountWithTypeActivity"));
                i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                i.putExtra(AddAccountWithTypeActivity.EXTRA_ALLOWABLE_ACCOUNT_TYPES_STRING_ARRAY,
                        allowableAccountTypes.toArray(new String[allowableAccountTypes.size()]));

                row.add(new MenuItem.Builder().id(mNextItemId++)
                        .title(mContext.getString(R.string.add_account))
                        .imageResourceId(mContext, R.drawable.ic_settings_add)
                        .intent(i).build());
            }
        }
    }

    private void addAccessories(ArrayObjectAdapter row) {
       /* if (mBtAdapter != null) {
            Set<BluetoothDevice> bondedDevices = mBtAdapter.getBondedDevices();
            if (DEBUG) {
                
            }

            Set<String> connectedBluetoothAddresses =
                    BluetoothConnectionsManager.getConnectedSet(mContext);

            for (BluetoothDevice device : bondedDevices) {
                if (DEBUG) {
                    
                            device.getBluetoothClass().getDeviceClass());
                }

                int resourceId = AccessoryUtils.getImageIdForDevice(device);
                Intent i = BluetoothAccessoryActivity.getIntent(mContext, device.getAddress(),
                        device.getName(), resourceId);
                i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);

                String desc = connectedBluetoothAddresses.contains(device.getAddress())
                        ? mContext.getString(R.string.accessory_connected)
                        : null;

                row.add(new MenuItem.Builder().id(mNextItemId++).title(device.getName())
                        .description(desc).imageResourceId(mContext, resourceId)
                        .intent(i).build());
            }
        }*/
    }

    private static String googleAccountIconUri(Context context) {
        ShortcutIconResource iconResource = new ShortcutIconResource();
        iconResource.packageName = context.getPackageName();
        iconResource.resourceName = context.getResources().getResourceName(
                R.drawable.ic_settings_google_account);
        return UriUtils.getShortcutIconResourceUri(iconResource).toString();
    }
    private void updateStorageItem(){
        StorageManager manager = (StorageManager) mContext.getSystemService(Context.STORAGE_SERVICE);
        StorageVolume[] volumes = manager.getVolumeList();
        for (StorageVolume sv:volumes) {
            Log.d("IKE-SD",sv.getId() + " " + sv.getPath());
            if(!sv.getState().equals(Environment.MEDIA_MOUNTED)){
                continue;
            }
            if(sv.getId().startsWith("public:179")){
                Log.d("IKE-SD","add sd");
                ArrayObjectAdapter row = mRows.get(mDeviceHeaderId);
                MenuItem temp = new MenuItem.Builder().id( mNextItemId++)
                    .title(mContext.getString(R.string.aw_sdcard_icon_string))
                    .imageResourceId(mContext, R.drawable.aw_card_icon)
                    .intent(getLauncherIntent("com.softwinner.TvdFileManager"))
                    .build();
                mUSBorSDItemList.put(sv.getPath(),temp);
                row.add(temp);
                mRows.put(mDeviceHeaderId,row);
            }else if(sv.getId().startsWith("public:8")){
                Log.d("IKE-SD","add usb");
                ArrayObjectAdapter row = mRows.get(mDeviceHeaderId);
                MenuItem temp = new MenuItem.Builder().id(mNextItemId++)
                    .title(mContext.getString(R.string.aw_usb_icon_string))
                    .imageResourceId(mContext, R.drawable.aw_usb_icon)
                    .intent(getLauncherIntent("com.softwinner.TvdFileManager"))
                    .build();
                mUSBorSDItemList.put(sv.getPath(),temp);
                row.add(temp);
                mRows.put(mDeviceHeaderId,row);
            }
        }
    }

    private static ArrayMap<String,MenuItem> mUSBorSDItemList = new ArrayMap<String,MenuItem>();
    @Override
    public void onUSBMounted(String path)
    {
        //在其它界面拔出sd，回主界面也不需要显示
        if(!mUSBorSDItemList.containsKey(path))
        {
            ArrayObjectAdapter row = mRows.get(mDeviceHeaderId);
            MenuItem temp = new MenuItem.Builder().id(mNextItemId++)
                .title(mContext.getString(R.string.aw_usb_icon_string))
                .imageResourceId(mContext, R.drawable.aw_usb_icon)
                .intent(getLauncherIntent("com.softwinner.TvdFileManager"))
                .build();
            mUSBorSDItemList.put(path, temp);
            row.add(temp);
            mRows.put(mDeviceHeaderId,row);
        }
    }
    @Override
    public void onUSBEject(String path)
    {
        if(mUSBorSDItemList.containsKey(path))
        {
            ArrayObjectAdapter row = mRows.get(mDeviceHeaderId);
            MenuItem temp = mUSBorSDItemList.remove(path);
            row.remove(temp);
            mRows.put(mDeviceHeaderId,row);
        }
    }
    @Override
    public void onSDCardMounted(String path)
    {
        if(!mUSBorSDItemList.containsKey(path))
        {
            ArrayObjectAdapter row = mRows.get(mDeviceHeaderId);
            MenuItem temp = new MenuItem.Builder().id( mNextItemId++)
                .title(mContext.getString(R.string.aw_sdcard_icon_string))
                .imageResourceId(mContext, R.drawable.aw_card_icon)
                .intent(getLauncherIntent("com.softwinner.TvdFileManager"))
                .build();
            mUSBorSDItemList.put(path, temp);
            row.add(temp);
            mRows.put(mDeviceHeaderId,row);
        }
    }
    @Override
    public void onSDCardEject(String path)
    {
        onUSBEject(path);
    }
/////////////////////////////////////////////////////////////////////////////////////
}

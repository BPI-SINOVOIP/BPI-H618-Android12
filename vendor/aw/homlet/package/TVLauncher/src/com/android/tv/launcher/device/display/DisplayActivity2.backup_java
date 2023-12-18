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

package com.android.tv.launcher.device.display;

import static android.app.ActivityManager.StackId.PINNED_STACK_ID;

import android.R.integer;
import android.R.string;
import android.app.Fragment;
import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.app.ActivityManager.StackInfo;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.os.IDisplayOutputManager;
import android.os.DisplayOutputManager;
import android.provider.Settings;
import android.text.format.DateFormat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.widget.NumberPicker;
import android.widget.Toast;

import java.security.PrivilegedActionException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import com.android.tv.launcher.ActionBehavior;
import com.android.tv.launcher.ActionKey;
import com.android.tv.launcher.BaseSettingsActivity;
import com.android.tv.launcher.BrowseInfoFactory;
import com.android.tv.launcher.R;
import com.android.tv.launcher.dialog.old.Action;
import com.android.tv.launcher.dialog.old.ActionAdapter;
import com.android.tv.launcher.dialog.old.ActionFragment;
import com.android.tv.launcher.name.DeviceManager;
import com.android.tv.launcher.system.DateTimeActivity;
import com.android.tv.launcher.device.apps.AppManagementActivity;
import com.android.tv.launcher.device.display.ActionType;
import com.android.tv.launcher.users.RestrictedProfileActivity;
import com.android.tv.launcher.util.SettingsHelper;
import com.android.tv.launcher.widget.picker.DatePicker;
import com.android.tv.launcher.widget.picker.Picker;
import com.android.tv.launcher.widget.picker.TimePicker;

/**
 * Activity allowing the management of display settings.
 */
public class DisplayActivity2 extends BaseSettingsActivity implements ActionAdapter.Listener {

    private static final String TAG = "DisplayActivity2";
    private static final String KYE_DISPLAY_SET_WALLPAPER = "display_set_wallpaper";
    private DisplayOutputManager mDisplayOutputManager = null;
    private String[] mZoomPercentStringArray;
    private int mCurrentZoomPercent;
    private int[] mResolutionIntegerArray;
    private ArrayList<String> mResolutionStringArray = null;
    private int mCurrentResolutionIndex = 0; //当前分辨率在上边数组中的下标

    private IActivityManager mActivityManager;
    private String[] mPIPPositionStringArray;
    private int mCurrentPIPPositionIndex = 1;
    private String[] mPIPSizeStringArray;
    private int mCurrentPIPSizeIndex = 1;

    private int mScreenWidth = 0;  //屏幕大小
    private int mScreenHeight = 0;
    private int mPIPBoundsLeft = 0; //PIP设置的各边参数
    private int mPIPBoundsRight = 0;
    private int mPIPBoundsTop = 0;
    private int mPIPBoundsBottom = 0;
    private int mPIPBoundsWidth = 0;
    private int mPIPBoundsHeight = 0;

    private static final int DEFAULT_PIP_BOUNDS_LEFT = 1328; //默认PIP位置在右上角
    private static final int DEFAULT_PIP_BOUNDS_RIGHT = 1808;
    private static final int DEFAULT_PIP_BOUNDS_TOP = 54;
    private static final int DEFAULT_PIP_BOUNDS_BOTTOM = 324;
    private static final int DEFAULT_PIP_BOUNDS_WIDTH = DEFAULT_PIP_BOUNDS_RIGHT - DEFAULT_PIP_BOUNDS_LEFT;
    private static final int DEFAULT_PIP_BOUNDS_HEIGHT = DEFAULT_PIP_BOUNDS_BOTTOM - DEFAULT_PIP_BOUNDS_TOP;
    private static final String ALLWINNER_PIP_SETTINGS_BOUNDS_LEFT = "persist.sys.pip.bounds.left";
    private static final String ALLWINNER_PIP_SETTINGS_BOUNDS_RIGHT = "persist.sys.pip.bounds.right";
    private static final String ALLWINNER_PIP_SETTINGS_BOUNDS_TOP = "persist.sys.pip.bounds.top";
    private static final String ALLWINNER_PIP_SETTINGS_BOUNDS_BOTTOM = "persist.sys.pip.bounds.bottom";
    private static final String ALLWINNER_PIP_SETTINGS_BOUNDS_POSITION = "persist.sys.pip.bounds.pos";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        //构造DisplayOutputManager
        IBinder b = ServiceManager.getService(Context.DISPLAYOUTPUT_SERVICE);
        IDisplayOutputManager service = IDisplayOutputManager.Stub.asInterface(b);
        if (service == null) {
            Log.w(TAG, "Failed to get device output manager service.");
        }
        mDisplayOutputManager = new DisplayOutputManager(this,service);
        //获取缩放值
        int[] zoomPercent = mDisplayOutputManager.getDisplayMargin(Display.DEFAULT_DISPLAY);
        int edge = mDisplayOutputManager.getDisplayEdge(Display.DEFAULT_DISPLAY);
        mCurrentZoomPercent =  zoomPercent[0]; //宽高为等比缩放，只取其中一个值就行
        mZoomPercentStringArray = getResources().getStringArray(R.array.captions_display_zoom_percent);
        //获取分辨率（显示模式）
        int resolution = mDisplayOutputManager.getDisplayOutput(Display.DEFAULT_DISPLAY);
        //获取屏幕显示尺寸
        DisplayMetrics dm = getResources().getDisplayMetrics();
        mScreenWidth = dm.widthPixels;
        mScreenHeight = dm.heightPixels;
        /*
        <item>0x400</item> <!-- HDMI 480I -->
        <item>0x401</item> <!-- HDMI 576I -->
        <item>0x402</item> <!-- HDMI 480P -->
        <item>0x403</item> <!-- HDMI 576P -->
        <item>0x404</item> <!-- HDMI 720P 50Hz -->
        <item>0x405</item> <!-- HDMI 720P 60Hz -->
        <item>0x406</item> <!-- HDMI 1080I 50Hz -->
        <item>0x407</item> <!-- HDMI 1080I 60Hz -->
        <item>0x408</item> <!-- HDMI 1080P 24Hz -->
        <item>0x409</item> <!-- HDMI 1080P 50Hz-->
        <item>0x40a</item> <!-- HDMI 1080P 60Hz -->
        <item>0x41c</item> <!-- HDMI 4K 30HZ -->
        <item>0x41d</item> <!-- HDMI 4K 25HZ -->
        <item>0x41e</item> <!-- HDMI 4K 24HZ -->
        FIXBUG:getDisplayOutput接口返回的是上述16进制值，下边getSupportModes获取的是0~0x1e这样的值，具体参考displayFormat2Name里边的case
                        所以需要减去0x400转换为DisplayOutputManager里边支持的值.
        */
        resolution = resolution - 0x400;
        int[] tempArray = mDisplayOutputManager.getSupportModes(Display.DEFAULT_DISPLAY,DisplayOutputManager.DISPLAY_OUTPUT_TYPE_HDMI);
        //这一段防止明明支持的分辨率,但EDID里面就是没有
        int num = tempArray.length;
        mResolutionIntegerArray = new int[ num +1];
        boolean flag = false;
        for(int i=0;i<num;i++){
        	mResolutionIntegerArray[i] = tempArray[i];
        	if(resolution==tempArray[i]){
        		flag = true;
        	}
        }
        if(!flag){
        	mResolutionIntegerArray[num] = resolution;
        }else{
        	mResolutionIntegerArray = tempArray;
        }
        if(mResolutionIntegerArray!=null){
            int len = mResolutionIntegerArray.length;
            if(mResolutionStringArray == null)
                mResolutionStringArray = new ArrayList<String>(len);
            else
                mResolutionStringArray.clear();
            for(int i=0;i<len;i++){
                mResolutionStringArray.add(displayFormat2Name(mResolutionIntegerArray[i]));
                Log.d(TAG,"support HDMI Resolution str = " + mResolutionStringArray.get(i));
                //设置默认焦点位置
                if(mResolutionIntegerArray[i] == (Integer)resolution){
                    mCurrentResolutionIndex = i;
                }
            }
        }
	mCurrentPIPPositionIndex = SystemProperties.getInt(ALLWINNER_PIP_SETTINGS_BOUNDS_POSITION, 1);
        mActivityManager =  ActivityManagerNative.getDefault();
        try {
            StackInfo stackInfo = mActivityManager.getStackInfo(PINNED_STACK_ID);
            if (stackInfo == null) {
                Log.w(TAG, "There is no PIP Screen,get bounds from editor");
                mPIPBoundsLeft = SystemProperties.getInt(ALLWINNER_PIP_SETTINGS_BOUNDS_LEFT, DEFAULT_PIP_BOUNDS_LEFT);
                mPIPBoundsRight = SystemProperties.getInt(ALLWINNER_PIP_SETTINGS_BOUNDS_RIGHT, DEFAULT_PIP_BOUNDS_RIGHT);
                mPIPBoundsTop = SystemProperties.getInt(ALLWINNER_PIP_SETTINGS_BOUNDS_TOP, DEFAULT_PIP_BOUNDS_TOP);
                mPIPBoundsBottom = SystemProperties.getInt(ALLWINNER_PIP_SETTINGS_BOUNDS_BOTTOM, DEFAULT_PIP_BOUNDS_BOTTOM);
            }else{
                Rect rect = stackInfo.bounds;
                mPIPBoundsLeft = rect.left;
                mPIPBoundsRight = rect.right;
                mPIPBoundsTop = rect.top;
                mPIPBoundsBottom = rect.bottom;
            }
            //PIP位置设置，默认焦点位置
            setOrgetPIPPositionIndex(false); //false表示获取index

            //PIP大小设置，默认焦点位置
            mPIPBoundsWidth = mPIPBoundsRight - mPIPBoundsLeft; //宽高
            mPIPBoundsHeight = mPIPBoundsBottom - mPIPBoundsTop;
            setOrgetPIPSizeIndex(false);
        } catch (RemoteException e) {
            Log.e(TAG, "getStackInfo failed", e);
        }
        mPIPPositionStringArray = getResources().getStringArray(R.array.captions_picture_in_picture_position_array);
        mPIPSizeStringArray = getResources().getStringArray(R.array.captions_picture_in_picture_size_array);
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onActionClicked(Action action) {
        String key = action.getKey();
        if(key.equals(KYE_DISPLAY_SET_WALLPAPER)) //设置壁纸，直接进入应用
        {
            Intent i1 = action.getIntent();
            startActivity(i1);
            return;
        }
       switch ((ActionType)mState) {
           case DISPLAY_ZOOM:   //在缩放界面的点击，设置缩放
               int percent = Integer.parseInt(key);
               if(mCurrentZoomPercent != percent && percent <= 100){
                   setDisplayZoom(Integer.parseInt(key));
                   updateActionFrament();
               }else if(percent > 100){
                   Toast.makeText(this, "暂时不支持放大",Toast.LENGTH_SHORT).show();
               }
               return;
            case DISPLAY_RESOLUTION: //屏幕缩放的设置界面
               setDisplayResolution(key);
               updateActionFrament();
               return;
            case DISPLAY_PIP_SETTINGS_POS: //画中画模式的位置设置
                setDisplayPIPPosition(key);
                updateActionFrament();
                return;
            case DISPLAY_PIP_SETTINGS_SIZE://画中画模式的大小设置
                setDisplayPIPSize(key);
                updateActionFrament();
                return;
            default:
       }
        ActionKey<ActionType, ActionBehavior> actionKey = new ActionKey<ActionType, ActionBehavior>(
                ActionType.class, ActionBehavior.class, action.getKey());
        final ActionType type = actionKey.getType();
        switch(type){
            default:
                setState(type, true); //界面的切换， 一定要有。比如DISPLAY_OVERVIEW界面点击进入其它界面
                break;
        }
    }

    @Override
    protected Object getInitialState() {
        return ActionType.DISPLAY_OVERVIEW;
    }
    @Override
    protected void updateView() { //左边的区域显示的内容
        refreshActionList();
        switch ((ActionType) mState) {
            case DISPLAY_OVERVIEW:
                setView(R.string.device_display, R.string.header_category_device, 0,
                        R.drawable.ic_settings_display);
                break;
            default:
                setView(((ActionType) mState).getTitle(mResources), getPrevState() != null ?
                        ((ActionType) getPrevState()).getTitle(mResources) : null,
                        ((ActionType) mState).getDescription(mResources), R.drawable.ic_settings_display);
                break;
        }
    }
    @Override
    protected void refreshActionList() {
        mActions.clear();
        switch ((ActionType) mState) {
            case DISPLAY_OVERVIEW:
		if(mResolutionIntegerArray.length!=0){
                mActions.add(ActionType.DISPLAY_RESOLUTION.toAction(mResources,mResolutionStringArray.get(mCurrentResolutionIndex)));
		}
                mActions.add(ActionType.DISPLAY_ZOOM.toAction(mResources, mCurrentZoomPercent + "%"));
                mActions.add(new Action.Builder()
                        .key(KYE_DISPLAY_SET_WALLPAPER)
                        .title(getString(R.string.captions_display_wallpaper))
                        .intent(getLauncherIntent("com.android.gallery3d"))
                        .enabled(true)
                        .build());
                mActions.add(ActionType.DISPLAY_PIP_SETTINGS.toAction(mResources,mPIPPositionStringArray[mCurrentPIPPositionIndex] + "  (" + mPIPBoundsWidth + " " + mPIPBoundsHeight+")"));
                break;
            case DISPLAY_RESOLUTION:
		if(mResolutionIntegerArray.length!=0){
                for(int i=0 ; i < mResolutionStringArray.size() ; i++){   
                    Action tempAction = new Action.Builder()
                            .key(mResolutionStringArray.get(i))
                            .title(mResolutionStringArray.get(i))
                            .build();
                    if(mCurrentResolutionIndex == i)
                        tempAction.setChecked(true);
                    mActions.add(tempAction);
                }
		}
                break;
            case DISPLAY_ZOOM:
                for(int i=0 ; i < mZoomPercentStringArray.length ; i++)
                {
                    Action tempAction = new Action.Builder()
                            .key(mZoomPercentStringArray[i])
                            .title(mZoomPercentStringArray[i] + "%")
                            .build();
                    if(mCurrentZoomPercent == Integer.parseInt(mZoomPercentStringArray[i]))
                        tempAction.setChecked(true);
                    mActions.add(tempAction);
                }
                break;
            case DISPLAY_PIP_SETTINGS: //二级设置菜单
                mActions.add(ActionType.DISPLAY_PIP_SETTINGS_POS.toAction(mResources,mPIPPositionStringArray[mCurrentPIPPositionIndex]));
                mActions.add(ActionType.DISPLAY_PIP_SETTINGS_SIZE.toAction(mResources,"("+mPIPBoundsWidth + " " + mPIPBoundsHeight+")"));
                break;
            case DISPLAY_PIP_SETTINGS_POS:
                for(int i=0 ; i < mPIPPositionStringArray.length ; i++)
                {
                    Action tempAction = new Action.Builder()
                            .key(mPIPPositionStringArray[i])
                            .title(mPIPPositionStringArray[i])
                            .build();
                    if(mCurrentPIPPositionIndex == i)
                        tempAction.setChecked(true);
                    mActions.add(tempAction);
                }
                break;
            case DISPLAY_PIP_SETTINGS_SIZE:
                for(int i=0 ; i < mPIPSizeStringArray.length ; i++)
                {
                    Action tempAction = new Action.Builder()
                            .key(mPIPSizeStringArray[i])
                            .title(mPIPSizeStringArray[i].indexOf("+") > -1 ? mPIPSizeStringArray[i] + "%" : mPIPSizeStringArray[i])
                            .build();
                    if(mCurrentPIPSizeIndex == i)
                        tempAction.setChecked(true);
                    mActions.add(tempAction);
                }
                break;
        }
    }
    @Override
    protected void setProperty(boolean enable) {
    }
    private void setDisplayResolution(String key){
        //设置改变分辨率的地方 
        for(int i=0; i < mResolutionStringArray.size(); i++){
            if(key.equals(mResolutionStringArray.get(i))){
                if(i == mCurrentResolutionIndex)
                    return;
                else
                    mCurrentResolutionIndex = i;
                break;
            }
        }
        //FIXBUG:+0x400
        mDisplayOutputManager.setDisplayOutput(Display.DEFAULT_DISPLAY,mResolutionIntegerArray[mCurrentResolutionIndex] + 0x400);
    }
    private void setDisplayZoom(int  percent){
        mCurrentZoomPercent = percent;
        mDisplayOutputManager.setDisplayMargin(Display.DEFAULT_DISPLAY, mCurrentZoomPercent,mCurrentZoomPercent);
    }

    private void setDisplayPIPPosition(String key) {
        for(int i=0; i < mPIPPositionStringArray.length; i++){
            if(key.equals(mPIPPositionStringArray[i])){
                if(i != mCurrentPIPPositionIndex){
                    mCurrentPIPPositionIndex = i;
                    saveAndSetPIPBounds();
                }
                return;
            }
        }
    }
    private void setDisplayPIPSize(String key) {
        for(int i=0; i < mPIPSizeStringArray.length; i++){
            if(key.equals(mPIPSizeStringArray[i])){
                if(i != mCurrentPIPSizeIndex){
                    mCurrentPIPSizeIndex = i;
                    setOrgetPIPSizeIndex(true);
                    saveAndSetPIPBounds();
                }
                return;
            }
        }
    }

    private Intent getLauncherIntent(String pkgname){
        PackageManager pm = this.getPackageManager();
        Intent lintent = pm.getLeanbackLaunchIntentForPackage(pkgname);
        if (lintent == null) {
            lintent = pm.getLaunchIntentForPackage(pkgname);
        }
        return lintent;
    }

    private void updateActionFrament(){
        refreshActionList();
        Fragment fragment = getActionFragment();
        if ((fragment != null) && (fragment instanceof ActionFragment)) {
          ActionFragment actFrag = (ActionFragment) fragment;
          ((ActionAdapter) actFrag.getAdapter()).setActions(mActions);
        }
    }

    private String displayFormat2Name(int value){
       String retval;
       switch(value){
       case  DisplayOutputManager.DISPLAY_TVFORMAT_480I:
           retval = "HDMI 480I";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_576I:
           retval = "HDMI 576I";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_480P:
           retval = "HDMI 480P";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_576P:
           retval = "HDMI 576P";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_720P_50HZ:
           retval = "HDMI 720P 50HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_720P_60HZ:
           retval = "HDMI 720P 60HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_1080I_50HZ:
           retval = "HDMI 1080I 50HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_1080I_60HZ:
           retval = "HDMI 1080I 60HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_1080P_24HZ:
           retval = "HDMI 1080P 24HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_1080P_50HZ:
           retval = "HDMI 1080P 50HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_1080P_60HZ:
           retval = "HDMI 1080P 60HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_3840_2160P_30HZ:
           retval = "HDMI 3840x2160P 30HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_3840_2160P_60HZ:
           retval = "HDMI 3840x2160P 60HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_3840_2160P_25HZ:
           retval = "HDMI 3840x2160P 25HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_3840_2160P_24HZ:
           retval = "HDMI 3840x2160P 24HZ";
           break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_4096_2160P_24HZ:
	   retval = "HDMI 4096x2160P 24HZ";
	   break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_4096_2160P_25HZ:
	   retval = "HDMI 4096x2160P 25HZ";
	   break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_4096_2160P_30HZ:
	   retval = "HDMI 4096x2160P 30HZ";
	   break;
       case  DisplayOutputManager.DISPLAY_TVFORMAT_4096_2160P_60HZ:
	   retval = "HDMI 4096x2160P 60HZ";
	   break;
       default:
           retval = "Unknown";
       }
       return retval;
    }
    private void saveAndSetPIPBounds(){
        setOrgetPIPPositionIndex(true);
        SystemProperties.set(ALLWINNER_PIP_SETTINGS_BOUNDS_LEFT,String.valueOf(mPIPBoundsLeft));
        SystemProperties.set(ALLWINNER_PIP_SETTINGS_BOUNDS_RIGHT,String.valueOf(mPIPBoundsRight));
        SystemProperties.set(ALLWINNER_PIP_SETTINGS_BOUNDS_TOP,String.valueOf(mPIPBoundsTop));
        SystemProperties.set(ALLWINNER_PIP_SETTINGS_BOUNDS_BOTTOM,String.valueOf(mPIPBoundsBottom));
        SystemProperties.set(ALLWINNER_PIP_SETTINGS_BOUNDS_POSITION, String.valueOf(mCurrentPIPPositionIndex));
        try {
            StackInfo stackInfo = mActivityManager.getStackInfo(PINNED_STACK_ID);
            if (stackInfo == null) {  //暂时还没有进入PIP模式
                Log.w(TAG, "Cannot find pinned stack");
            }else{  //已经在PIP模式了，就直接设置进去
                try{
                    mActivityManager.resizeStack(PINNED_STACK_ID, new Rect(mPIPBoundsLeft,mPIPBoundsTop,mPIPBoundsRight,mPIPBoundsBottom),true, true, true, -1);//这个切换动作参考ActivityManagerNative
                }catch(RemoteException e){
                    Log.e(TAG, "setDisplayPIPPosition resizeStack failed", e);
                }
            }
        } catch (RemoteException e) {
            Log.e(TAG, "getStackInfo failed", e);
        }
    }

    private void setOrgetPIPSizeIndex(boolean isSet){
        if(isSet){
            switch(mCurrentPIPSizeIndex){
                case 0:
                    mPIPBoundsWidth = DEFAULT_PIP_BOUNDS_WIDTH;
                    mPIPBoundsHeight = DEFAULT_PIP_BOUNDS_HEIGHT;
                    break;
                case 1: //+10%
                    mPIPBoundsWidth = DEFAULT_PIP_BOUNDS_WIDTH +  DEFAULT_PIP_BOUNDS_WIDTH / 10; //用除法可以省去不用像乘法那样转换0.1这个浮点数
                    mPIPBoundsHeight = DEFAULT_PIP_BOUNDS_HEIGHT + DEFAULT_PIP_BOUNDS_HEIGHT / 10;
                    break;
                case 2: //+20%
                    mPIPBoundsWidth = DEFAULT_PIP_BOUNDS_WIDTH +  DEFAULT_PIP_BOUNDS_WIDTH / 5;
                    mPIPBoundsHeight = DEFAULT_PIP_BOUNDS_HEIGHT + DEFAULT_PIP_BOUNDS_HEIGHT / 5;
                    break;
                case 3: //+30%
                    mPIPBoundsWidth = DEFAULT_PIP_BOUNDS_WIDTH +  DEFAULT_PIP_BOUNDS_WIDTH / 10 * 3;
                    mPIPBoundsHeight = DEFAULT_PIP_BOUNDS_HEIGHT + DEFAULT_PIP_BOUNDS_HEIGHT / 10 * 3;
                    break;
                case 4: //+40%
                    mPIPBoundsWidth = DEFAULT_PIP_BOUNDS_WIDTH +  DEFAULT_PIP_BOUNDS_WIDTH / 5 * 2;
                    mPIPBoundsHeight = DEFAULT_PIP_BOUNDS_HEIGHT + DEFAULT_PIP_BOUNDS_HEIGHT / 5 * 2;
                    break;
            }
        }else{
            if(mPIPBoundsWidth == DEFAULT_PIP_BOUNDS_WIDTH)
                mCurrentPIPSizeIndex = 0;
            else if(mPIPBoundsWidth == DEFAULT_PIP_BOUNDS_WIDTH + DEFAULT_PIP_BOUNDS_WIDTH / 10)
                mCurrentPIPSizeIndex = 1;
            else if(mPIPBoundsWidth == DEFAULT_PIP_BOUNDS_WIDTH + DEFAULT_PIP_BOUNDS_WIDTH / 5)
                mCurrentPIPSizeIndex = 2;
            else if(mPIPBoundsWidth == DEFAULT_PIP_BOUNDS_WIDTH + DEFAULT_PIP_BOUNDS_WIDTH / 10 * 3)
                mCurrentPIPSizeIndex = 3;
            else if(mPIPBoundsWidth == DEFAULT_PIP_BOUNDS_WIDTH +  DEFAULT_PIP_BOUNDS_WIDTH / 5 * 2)
                mCurrentPIPSizeIndex = 4;
        }
    }
    private void setOrgetPIPPositionIndex(boolean isSet){
        if(isSet){ //设置新边界
            switch(mCurrentPIPPositionIndex){
                case 0: //左上角
                    mPIPBoundsLeft = mScreenWidth - DEFAULT_PIP_BOUNDS_RIGHT;
                    mPIPBoundsTop = DEFAULT_PIP_BOUNDS_TOP;
                    mPIPBoundsRight = mPIPBoundsLeft + mPIPBoundsWidth;
                    mPIPBoundsBottom = DEFAULT_PIP_BOUNDS_TOP + mPIPBoundsHeight;
                    break;
                case 1://右上角
                    mPIPBoundsTop = DEFAULT_PIP_BOUNDS_TOP;
                    mPIPBoundsRight = DEFAULT_PIP_BOUNDS_RIGHT;
                    mPIPBoundsLeft = DEFAULT_PIP_BOUNDS_RIGHT - mPIPBoundsWidth;
                    mPIPBoundsBottom = DEFAULT_PIP_BOUNDS_TOP + mPIPBoundsHeight;
                    break;
                case 2://中间
                    mPIPBoundsTop = mScreenHeight/2-mPIPBoundsHeight/2;
                    mPIPBoundsLeft = mScreenWidth/2-mPIPBoundsWidth/2;
                    mPIPBoundsBottom = mPIPBoundsTop + mPIPBoundsHeight;
                    mPIPBoundsRight = mPIPBoundsLeft + mPIPBoundsWidth;
                    break;
                case 3: //左下角
                    mPIPBoundsLeft = mScreenWidth - DEFAULT_PIP_BOUNDS_RIGHT;
                    mPIPBoundsBottom = mScreenHeight - DEFAULT_PIP_BOUNDS_TOP;
                    mPIPBoundsRight = mPIPBoundsLeft + mPIPBoundsWidth;
                    mPIPBoundsTop = mPIPBoundsBottom - mPIPBoundsHeight;
                    break;
                case 4: //右下角
                    mPIPBoundsRight = DEFAULT_PIP_BOUNDS_RIGHT;
                    mPIPBoundsBottom = mScreenHeight - DEFAULT_PIP_BOUNDS_TOP;
                    mPIPBoundsLeft = DEFAULT_PIP_BOUNDS_RIGHT - mPIPBoundsWidth;
                    mPIPBoundsTop = mPIPBoundsBottom - mPIPBoundsHeight;
                    break;
            }
        }
    }
}

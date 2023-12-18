package com.softwinner.tvfilemanager;

import android.app.Application;
import android.content.Context;
import android.os.storage.StorageManager;

import com.softwinner.tvfilemanager.entity.LocalMusic;
import com.softwinner.tvfilemanager.entity.LocalPicture;
import com.softwinner.tvfilemanager.entity.LocalVideo;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by ningxiangzhong on 15-3-2.
 */
public class MyApplication extends Application {
    public final static String TAG = "MyApplication";
    public static Context mContext;
    //挂载的设备路径
    public static List<String> pathMounted;
    public static List<String> pathRemoved;
    //本地视频条目，在MainFragment加载本地视频后给他复制作为全局变量保存供其他fragment或Activity使用
    public static List<LocalVideo> mLocalVideo;
    //本地图片条目，在MainFragment加载本地图片后给他复制作为全局变量保存供其他fragment或Activity使用
    public static List<LocalPicture> mLocalPicture;
    //本地音乐条目，在MainFragment加载本地音乐后给他复制作为全局变量保存供其他fragment或Activity使用
    public static List<LocalMusic> mLocalMusic;

    public static List<String> mFileList;

    @Override
    public void onCreate() {
        super.onCreate();
        //初始化，不然报nullPointer错误，切记
        mContext = getApplicationContext();
        pathMounted = new ArrayList<>();
        pathRemoved = new ArrayList<>();
        mLocalVideo = new ArrayList<>();
        mLocalPicture = new ArrayList<>();
        mLocalMusic = new ArrayList<>();

        mFileList = new ArrayList<>();

        //获取挂载的设备
        String[] volumPathList = null;
        StorageManager storageManager = (StorageManager) mContext.getSystemService(Context.STORAGE_SERVICE);
        try {
            Method getVolumePaths = storageManager.getClass().getDeclaredMethod("getVolumePaths");
            getVolumePaths.setAccessible(true);
            volumPathList = (String[]) getVolumePaths.invoke(storageManager);
        } catch (Exception e) {
            e.printStackTrace();
        }
        for (int i = 0; i < volumPathList.length; i++) {
            pathMounted.add(volumPathList[i]);
        }

    }


    public static Context getContext() {
        return mContext;
    }

    public static List<String> getPathMounted() {
        return pathMounted;
    }

    public static List<String> getPathRemoved() {
        return pathRemoved;
    }
}

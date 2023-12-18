package com.android.calculator2;

import android.content.Context;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by zengshuchuan on 2017/7/12
 */

public class StorageHelper {
    private final Context mContext;
    private final StorageManager mStorageManager;

    public StorageHelper(Context context) {
        mContext = context;
        mStorageManager = (StorageManager) mContext.getSystemService(Context.STORAGE_SERVICE);
    }

    public List<String> getMountPoints() {
        List<String> mountPoints = new ArrayList<>();
        try {
            List<StorageVolume> list = mStorageManager.getStorageVolumes();
            final Method getPath = StorageVolume.class.getMethod("getPath");
            for (StorageVolume sv : list) {
                String state = sv.getState();
                String path = (String) getPath.invoke(sv);
                if (Environment.MEDIA_MOUNTED.equals(state)) {
                    mountPoints.add(path);
                }
            }

        } catch (NoSuchMethodException | InvocationTargetException | IllegalAccessException e) {
            e.printStackTrace();
        }
        return mountPoints;
    }
}

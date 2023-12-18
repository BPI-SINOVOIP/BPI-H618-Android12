package com.softwinner.dragonbox.manager;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;
import android.util.Log;

import android.content.Context;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;

import com.softwinner.dragonbox.entity.StorageInfo;

public class StorageInfoManager {
    public List<StorageInfo> mStorageInfos;
    private Context mContext;

    private StorageManager mStorageManager;

    public StorageInfoManager(Context context) {
        mContext = context;
        mStorageManager = (StorageManager) mContext
                .getSystemService(Context.STORAGE_SERVICE);


    }

    public StorageInfo getExtsdInfo() {
        List<StorageInfo> storageInfo = updateStorageInfos(StorageInfo.PATH_EXDSD_PREFIX);
        return storageInfo.get(0);

    }

    public List<StorageInfo> getUsbdeviceInfos() {
        return updateStorageInfos(StorageInfo.PATH_USB_DEVICE_PREFIX);
    }

    private List<StorageInfo> updateStorageInfos(String path) {
        mStorageInfos = new ArrayList<StorageInfo>();
        StorageVolume[] volumes = mStorageManager.getVolumeList();
        for (StorageVolume volume : volumes) {
            StorageInfo storageinfo = new StorageInfo();
            storageinfo.setPath(volume.getPath());
            mStorageInfos.add(storageinfo);
        }

        List<StorageInfo> result = new ArrayList<StorageInfo>();
        for (StorageInfo storageInfo : mStorageInfos) {
            if (storageInfo.getPath().startsWith(path)) {
                result.add(storageInfo);
                String state = mStorageManager.getVolumeState(storageInfo
                        .getPath());
                if (Environment.MEDIA_MOUNTED.equals(state)) {
                    storageInfo.setMounted(true);
                    storageInfo.setRWable(isRWable(storageInfo.getPath()));
                } else {
                    storageInfo.setMounted(false);
                    storageInfo.setRWable(false);
                }
            }
        }
        if (result.size() == 0) {
            StorageInfo defaultInfo = new StorageInfo();
            defaultInfo.setPath(path);
            defaultInfo.setRWable(false);
            result.add(defaultInfo);
        }
        return result;
    }

    private boolean isRWable(String path) {
        File file = new File(path);
        return file.isDirectory() && file.canRead() && testRW(path);
    }

    private boolean testRW(String path) {
        String fileName = path + "/.DragonBoxTest";
        String readFileStr = null;
        try {
            File testFile = new File(fileName);
            if (!testFile.exists()) {
                testFile.createNewFile();
            }
            BufferedWriter bw = new BufferedWriter(new FileWriter(testFile));
            BufferedReader br = new BufferedReader(new FileReader(testFile));

            bw.write(fileName);
            bw.flush();
            bw.close();

            readFileStr = br.readLine();
            br.close();

        } catch (Exception e) {
            e.printStackTrace();
        } finally {

            File file = new File(fileName);
            if (file.exists()) {
                file.delete();
            }
        }

        return readFileStr != null && fileName.equals(readFileStr);
    }

}

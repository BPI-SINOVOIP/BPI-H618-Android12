/**
 * you can find the path of sdcard,flash and usbhost in here
 * @author chenjd
 * @email chenjd@allwinnertech.com
 * @data 2011-8-10
 */
package com.softwinner.TvdFileManager;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Environment;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.storage.IStorageManager;
import android.os.storage.StorageVolume;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import jcifs.util.LocalNetwork;

/**
 * manager of flash,sdcard,usbhost
 *
 * @author chenjd
 *
 */
public class DeviceManager {

    private static final String TAG = "DeviceManager";

    private List<StorageVolume> localDevicesList;
    private ArrayList<StorageVolume> sdDevicesList;
    private ArrayList<StorageVolume> usbDevicesList;
    private ArrayList<StorageVolume> sataDevicesList;
    private ArrayList<StorageVolume> internalDevicesList;
    private ArrayList<StorageVolume> mountedDevicesList;
    private Context mContext;
    private StorageManager manager;

    private IStorageManager mMountService;

    public DeviceManager(Context mContext1) {
        this.mContext = mContext1;
        manager = (StorageManager) mContext1.getSystemService(Context.STORAGE_SERVICE);
        mMountService = IStorageManager .Stub.asInterface(ServiceManager.getService("mount"));
        localDevicesList = new ArrayList<StorageVolume>();
        internalDevicesList = new ArrayList<StorageVolume>();
        sdDevicesList = new ArrayList<StorageVolume>();
        usbDevicesList = new ArrayList<StorageVolume>();
        sataDevicesList = new ArrayList<StorageVolume>();
    }

    private void devicesPathClear() {
        localDevicesList.clear();
        internalDevicesList.clear();
        sdDevicesList.clear();
        usbDevicesList.clear();
        sataDevicesList.clear();
    }

    private void devicesPathAdd() {
        if (manager == null) {
            Log.e(TAG, "devicesPathAdd manager is null return error!");
            return;
        }
        localDevicesList=manager.getStorageVolumes();
        internalDevicesList.add(manager.getPrimaryStorageVolume());
        StorageVolume storageVolume;
        for (int i = 0; i < localDevicesList.size(); i++) {
            storageVolume = localDevicesList.get(i);
            if (!storageVolume.getPath().equals(Environment.getExternalStorageDirectory().getPath())) {
                if (storageVolume.getId().startsWith("public:179")) {
                    /* 获取SD卡设备路径列表 */
                    sdDevicesList.add(storageVolume);
                } else if (storageVolume.getId().startsWith("public:8")) {
                    /* 获取USB设备路径列表 */
                    usbDevicesList.add(storageVolume);
                } else if (storageVolume.getPath().contains("sata")) {
                    /* 获取sata设备路径列表 */
                    sataDevicesList.add(storageVolume);
                }
            }
        }
    }

    public boolean isLocalDevicesRootPath(String path) {
        for (int i = 0; i < localDevicesList.size(); i++) {
            if (path.equals(localDevicesList.get(i).getPath())) {
                return true;
            }
        }
        return false;
    }

    /**
     * 获取总设备的列表
     *
     * @return
     */
    public ArrayList<StorageVolume> getLocalDevicesList() {
        return (ArrayList<StorageVolume>) localDevicesList;
    }

    /**
     * 获取当前被挂载的设备列表
     */
    public ArrayList<StorageVolume> getMountedDevicesList() {
        String state;
        ArrayList<StorageVolume> mountedDevices = new ArrayList<StorageVolume>();
        devicesPathClear();
        devicesPathAdd();
        try {
            for (int i = 0; i < localDevicesList.size(); i++) {
                state = localDevicesList.get(i).getState();
                if (state.equals(Environment.MEDIA_MOUNTED)) {
                    mountedDevices.add(localDevicesList.get(i));
                }
            }
        } catch (Exception rex) {
        }
        return mountedDevices;
    }

    public boolean isInterStoragePath(String path) {
        if (internalDevicesList !=null && internalDevicesList.size() > 0 ) {
            if (internalDevicesList.get(0).getPath().contains(path)) {
                return true;
            }
        }
        return false;
    }

    public boolean isSdStoragePath(String path) {
        if (path.startsWith("public:179")) {
            return true;
        }
        return false;
    }

    public boolean isUsbStoragePath(String path) {
        if (path.startsWith("public:8")) {
            return true;
        }
        return false;
    }

    public boolean isSataStoragePath(String path) {
        if (sataDevicesList.contains(path)) {
            return true;
        }
        return false;
    }

    public ArrayList<StorageVolume> getSdDevicesList() {
        return (ArrayList<StorageVolume>) sdDevicesList.clone();
    }

    public ArrayList<StorageVolume> getUsbDevicesList() {
        return (ArrayList<StorageVolume>) usbDevicesList.clone();
    }

    public ArrayList<StorageVolume> getInternalDevicesList() {
        return (ArrayList<StorageVolume>) internalDevicesList.clone();
    }

    public ArrayList<StorageVolume> getSataDevicesList() {
        return (ArrayList<StorageVolume>) sataDevicesList.clone();
    }

    public boolean hasMultiplePartition(String dPath) {
        try {
            File file = new File(dPath);
            String minor = null;
            String major = null;
            for (int i = 0; i < localDevicesList.size(); i++) {
                if (dPath.equals(localDevicesList.get(i).getPath())) {
                    String[] list = file.list();
                    /********
                     * add by hechuanlong 2013-08-20
                     * start{{---------------------------- fix a bug when
                     * list.length==0 the device be a multiple partition
                     ********/
                    if (0 == list.length) {
                        return false;
                    }
                    /********
                     * ------------------------}}end
                     *********/
                    for (int j = 0; j < list.length; j++) {
                        /* 如果目录命名规则不满足"主设备号:次设备号"(当前分区的命名规则),则返回false */
                        int lst = list[j].lastIndexOf("_");
                        if (lst != -1 && lst != (list[j].length() - 1)) {
                            major = list[j].substring(0, lst);
                            minor = list[j].substring(lst + 1, list[j].length());
                            try {

                                Integer.valueOf(major);
                                Integer.valueOf(minor);
                            } catch (NumberFormatException e) {
                                /* 如果该字符串不能被解析为数字,则退出 */
                                return false;
                            }
                        } else {
                            return false;
                        }
                    }
                    return true;
                }
            }
            return false;
        } catch (Exception e) {
            Log.e(TAG, "hasMultiplePartition() exception e");
            return false;
        }
    }

    public ArrayList<String> getNetDeviceList() {
        SharedPreferences pref = mContext.getSharedPreferences("Device", 0);
        String list = pref.getString("Net", null);
        if (list != null) {
            String[] split = list.split(",");
            if (split != null) {
                ArrayList<String> devList = new ArrayList<String>();
                for (int i = 0; i < split.length; i++) {
                    devList.add(split[i]);
                }
                return devList;
            }
        }
        return null;
    }

    public void saveNetDevice(String devPath) {
        if (devPath == null) {
            return;
        }
        SharedPreferences pref = mContext.getSharedPreferences("Device", 0);
        SharedPreferences.Editor editor = pref.edit();
        String list = pref.getString("Net", null);
        if (list == null) {
            editor.putString("Net", devPath);
        } else {
            list = list + "," + devPath;
            editor.putString("Net", list);
        }
        editor.commit();
    }

    public void delNetDevice(String devPath) {
        if (devPath == null) {
            return;
        }
        ArrayList<String> list = getNetDeviceList();
        list.remove(devPath);
        String st = null;
        for (int i = 0; i < list.size(); i++) {
            if (st == null) {
                st = list.get(i);
            } else {
                st = st + "," + list.get(i);
            }
        }
        SharedPreferences pref = mContext.getSharedPreferences("Device", 0);
        SharedPreferences.Editor editor = pref.edit();
        editor.putString("Net", st);
        editor.commit();
    }

    public boolean isNetStoragePath(String path) {
        if (path.startsWith("nfs|") || path.startsWith("smb|")) {
            Log.d(TAG, path + " is net storage");
            return true;
        }
        if (LocalNetwork.getIpType(path) != LocalNetwork.IP_TYPE_UNKNOWN) {
            return true;
        }
        return false;
    }

    public boolean doMount(String path) {
        try {
            mMountService.mount(path);
            return true;
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return false;
    }

    public void doUmount(String path) {
        String volId = null;
        try {
            VolumeInfo[] vols = mMountService.getVolumes(0);
            for(int i = 0; i < vols.length; i++){
                final VolumeInfo vol = vols[i];
                if (vol.path != null && path.startsWith(vol.path)) {
                    volId = vol.id;
                }
            }
            if(volId != null)
                mMountService.unmount(volId);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public boolean isDeviceMounted(String path) {
        if (path == null || "".equals(path)
                || !(path.startsWith("/mnt") || path.startsWith("/storage"))) {
            return false;
        }
        String state = "";
        try {
            state = manager.getVolumeState(path);
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
        return Environment.MEDIA_MOUNTED.equals(state);
    }
}

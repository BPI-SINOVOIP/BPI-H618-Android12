package com.softwinner.dragonbox.utils;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.nio.Buffer;
import java.util.Enumeration;

import android.content.Context;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Environment;
import android.os.StatFs;
import android.text.format.Formatter;
import android.util.Log;

public class SystemUtil {

    // the TYPE_CPU_*'s value is match sFILE_CPU_INFOS's index , so be careful
    // when change the value
    public static final String TAG = "DragonBox-SystemUtil";
    public static final int TYPE_CPU_MAX = 0;
    public static final int TYPE_CPU_MIN = 1;
    public static final int TYPE_CPU_CUR = 2;
    private static final String[] sFILE_CPU_INFOS = {
            "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq",
            "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq",
            "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq" };

    private static final String sFILE_CPU_GOVERNOR = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
    private static final String sFILE_CPU_SPEED = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed";
    private static final String sFIleCPU_SPEED_MAX = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq";
    private static final String sFIleCPU_SPEED_MIN = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq";

    public static String getCpuName() {
        String firstLine = getFileStrFirstLine("/proc/cpuinfo");
        String[] array = null;
        try {
            array = firstLine.split(":\\s+", 2);
            if (array.length != 2) {
                return "N/A";
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return array[1];
    }

    public static void setCpuFreq(int freq) {
        QwUtil.executeQw(new String[] { "echo " + freq + " > " + sFIleCPU_SPEED_MAX});
        QwUtil.executeQw(new String[] { "echo " + freq + " > " + sFIleCPU_SPEED_MIN});
        try {
            System.out.println("----------------" + getCpuFreq(TYPE_CPU_CUR)
                    + "----------" + freq);
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    public static void resetCPU(int freqMin, int freqMax) {
        QwUtil.executeQw(new String[] { "echo " + freqMax + " > " + sFIleCPU_SPEED_MAX });
        QwUtil.executeQw(new String[] { "echo " + freqMin + " > " + sFIleCPU_SPEED_MIN });
    }

    public static String getCpuFreq(int type) throws Exception {
        if (type >= sFILE_CPU_INFOS.length) {
            throw new Exception("Type not valid");
        }
        String freqStr = getFileStrFirstLine(sFILE_CPU_INFOS[type]);
        int freq;
        try {
            freq = Integer.parseInt(freqStr);// / 1000;
        } catch (NumberFormatException e) {
            e.printStackTrace();
            return "N/A";
        }
        return Integer.toString(freq);
    }

    private static String getFileStrFirstLine(String fileName) {
        String result = "";
        BufferedReader br = null;
        try {
            br = new BufferedReader(new FileReader(fileName));
            String text = br.readLine();
            result = text.trim();
        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            if (br != null) {
                try {
                    br.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return result;
    }

    public static String getEth0MAC() {
        String ifaceName = "eth0";
        Enumeration<NetworkInterface> el;
        String mac_s = "";
        try {
            el = NetworkInterface.getNetworkInterfaces();
            while (el.hasMoreElements()) {
                NetworkInterface iface = el.nextElement();
                if (iface.getName().compareToIgnoreCase(ifaceName) == 0) {
                    byte[] mac = iface.getHardwareAddress();
                    if (mac == null)
                        continue;
                    mac_s = hexByte(mac[0]) + ":" + hexByte(mac[1]) + ":"
                            + hexByte(mac[2]) + ":" + hexByte(mac[3]) + ":"
                            + hexByte(mac[4]) + ":" + hexByte(mac[5]);
                }
            }
        } catch (SocketException e1) {
            e1.printStackTrace();
        }
        return mac_s;
    }

    // sd卡容量
    public long[] getSDCardMemory() {
        long[] sdCardInfo = new long[2];
        String state = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(state)) {
            File sdcardDir = Environment.getExternalStorageDirectory();
            StatFs sf = new StatFs(sdcardDir.getPath());
            long bSize = sf.getBlockSize();
            long bCount = sf.getBlockCount();
            long availBlocks = sf.getAvailableBlocks();

            sdCardInfo[0] = bSize * bCount;// 总大小
            Log.i("SD卡总容量:", sdCardInfo[0] / (1024) + "KB");
            sdCardInfo[1] = bSize * availBlocks;// 可用大小
            Log.i("SD卡可用容量:", sdCardInfo[1] / (1024) + "KB");
        }
        return sdCardInfo;
    }

    // 获得系统总内存, 单位GB
    public static double getTotalMemory(Context context) {
        String str1 = "/proc/meminfo";// 系统内存信息文件
        String str2;
        String[] arrayOfString;
        long initial_memory = 0;
        try {
            FileReader localFileReader = new FileReader(str1);
            BufferedReader localBufferedReader = new BufferedReader(
                    localFileReader, 8192);
            str2 = localBufferedReader.readLine();// 读取meminfo第一行，系统总内存大小
            arrayOfString = str2.split("\\s+");
            initial_memory = (long)(Integer.valueOf(arrayOfString[1]).intValue()) * 1024;// 获得系统总内存，单位是KB，乘以1024转换为Byte
            localBufferedReader.close();
        } catch (IOException e) {
        }
        double gb = initial_memory/1024.0/1024.0/1024.0;
        return gb;//GB
    }
    public static double getFlashCapability() {
        String partitionPath = "/proc/partitions";//节点中的分区大小单位是kb
        double flashSize = 0.0;
        try {
            BufferedReader br = new BufferedReader(new FileReader(partitionPath));
            String line;
            String[] arr=null;
            boolean found = false;//mmcblk0/nand0
            while((line = br.readLine())!=null){
                Log.w(TAG,"line is "+line);
                arr = line.split("\\s+");
                for(int i = 0;i<arr.length;i++) {
                    //Log.w(TAG,"arr is "+arr[i]);
                    if(arr[i].equals("mmcblk0")||arr[i].equals("nand0")) {
                        found=true;
                    }
                }
                if(found) {
                    break;
                }
            }
            br.close();
            Log.w(TAG,"flash size is "+arr[3]);
            flashSize = Double.parseDouble(arr[3])/1024.0/1024.0;//GB
        } catch (Exception e) {
            e.printStackTrace();
        }
        return flashSize;
    }

    public static long[] getRomMemroy() {
        long[] romInfo = new long[2];
        romInfo[0] = getTotalInternalMemorySize();

        File path = Environment.getDataDirectory();
        StatFs stat = new StatFs(path.getPath());
        long blockSize = stat.getBlockSize();
        long availableBlocks = stat.getAvailableBlocks();
        romInfo[1] = blockSize * availableBlocks;
        return romInfo;
    }

    public static long getTotalInternalMemorySize() {
        File path = Environment.getDataDirectory();
        StatFs stat = new StatFs(path.getPath());
        long blockSize = stat.getBlockSize();
        long totalBlocks = stat.getBlockCount();
        return totalBlocks * blockSize;
    }

    private static String hexByte(byte b) {
        String s = "000000" + Integer.toHexString(b);
        return s.substring(s.length() - 2);
    }

}

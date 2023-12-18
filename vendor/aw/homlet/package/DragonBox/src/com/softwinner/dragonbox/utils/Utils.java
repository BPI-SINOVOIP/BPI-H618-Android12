package com.softwinner.dragonbox.utils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.security.MessageDigest;
import java.io.InputStream;
import java.util.Random;
import java.util.UUID;

import org.json.JSONObject;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.os.SystemProperties;
import android.util.Log;

import com.softwinner.SystemMix;

public class Utils {
    public static final String TAG = "DragonBox-Utils";
    public static final String RID_FILE_PATH = "/persist/factory/appProperties.xml";
    public static final String RID_DIR_PATH = "/persist/factory";
    public static final String RID_KEY = "Application.var.Rid=";
    public static boolean ridFileCheck;//检查rid文件写入情况
    public static boolean propertyCheck;//检查属性设置情况
    public static final int RID_FILE_LENGTH = 88;//检查rid文件的依据：rid文件长度88
    public static boolean md5CheckResult;//比较拷贝前后文件的md5结果

    public static boolean checkHasFile(Context context, String fileName) {
        String filePath = getFileAbsolutePath(context, fileName);
        return filePath != null && !"".equals(filePath);
    }

    public static String getFileAbsolutePath(Context context, String fileName) {
        StorageManager storageManager = (StorageManager) context
                .getSystemService(Context.STORAGE_SERVICE);
        StorageVolume[] volumes = storageManager.getVolumeList();
        for (StorageVolume volume : volumes) {
            String state = storageManager.getVolumeState(volume.getPath());
            if (Environment.MEDIA_MOUNTED.equals(state)) {
                String path = volume.getPath() + fileName;
                File file = new File(path);
                if (file.exists() && file.isFile()) {
                    return path;
                }
            }
        }
        return null;
    }

    public String formatSize(long size) {
        String suffix = null;
        float fSize = 0;
        if (size >= 1024) {
            suffix = "KB";
            fSize = size / 1024;
            if (fSize >= 1024) {
                suffix = "MB";
                fSize /= 1024;
            }
            if (fSize >= 1024) {
                suffix = "GB";
                fSize /= 1024;
            }
        } else {
            fSize = size;
        }
        java.text.DecimalFormat df = new java.text.DecimalFormat("#0.00");
        StringBuilder resultBuffer = new StringBuilder(df.format(fSize));
        if (suffix != null)
            resultBuffer.append(suffix);
        return resultBuffer.toString();
    }

    //args[0] : shell 命令 如"ls" 或"ls -1"; args[1] : 命令执行路径 如"/" ;
    public static String execute(String[] cmmand, String directory) throws IOException {
        String result = "";
        try {
            ProcessBuilder builder = new ProcessBuilder(cmmand);
            if (directory != null)
                builder.directory(new File(directory));
            builder.redirectErrorStream(true);
            Process process = builder.start();
            // 得到命令执行后的结果
            InputStream is = process.getInputStream();
            byte[] buffer = new byte[1024];
            while (is.read(buffer) != -1) {
                result = result + new String(buffer);
            }
            is.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return result;
    }

    //M20 RID记录烧入，路径：/persist/factory/appProperties.xml
    public static String buildRIDString() {
        String retValue = "{\"rid\":\"1ab567a628374de5be40742834f0af17\",\"ssec\":\"ZW9SOt==4YeZZT&?\"}";
        Log.i(TAG, "buildRIDString default retValue = " + retValue);
        try {
            String uuid = UUID.randomUUID().toString();
            String tmprid = uuid.substring(0,8)+uuid.substring(9,13)+uuid.substring(14,18)+uuid.substring(19,23)+uuid.substring(24,uuid.length());
            Log.i(TAG, "tmprid = " + tmprid);

            String ssecSeed = "0123456789abcefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@$%&?#-=";
            String tmpssec = new String();
            Random rand = new Random();
            for(int i = 0;i<16;i++) {
                int pos = rand.nextInt(ssecSeed.length());
                tmpssec+=ssecSeed.substring(pos, pos+1);
            }
            Log.i(TAG, "buildRIDString tmpssec = " + tmpssec);
            String JSON_KEY_SSEC = "ssec";
            String JSON_KEY_RID = "rid";
            JSONObject jsonObject = new JSONObject();
            jsonObject.put(JSON_KEY_SSEC,tmpssec);
            jsonObject.put(JSON_KEY_RID,tmprid);
            retValue = jsonObject.toString();
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.i(TAG, "buildRIDString retValue = " + retValue);
        return retValue;
    }

    /**
     *先根遍历序递归删除文件夹
     *
     *@param dirFile 要被删除的文件或者目录
     *@return 删除成功返回true, 否则返回false
     */
    public static boolean deleteFile(File dirFile) {
        // 如果dir对应的文件不存在，则退出
        if (!dirFile.exists()) {
            return false;
        }

        if (dirFile.isFile()) {
            return dirFile.delete();
        } else {

            for (File file : dirFile.listFiles()) {
                deleteFile(file);
            }
        }

        return dirFile.delete();
    }
    /**
     * 读取文件文本内容
     *
     *@param path 待读取文件的绝对路径
     *@return 文件文本内容
     */
    public static String readFile(String path){
        File file = new File(path);
        BufferedReader br=null;
        String content = "";
        if(!file.exists())
            return "文件"+path+"不存在!";
        if(file.length()==0)
            return "文件"+path+"长度为0!";
        try{
            br = new BufferedReader(new InputStreamReader(new FileInputStream(file)));
            String line= "";
            while((line = br.readLine())!=null){
                content = content+line;
            }
        }catch(Exception e){
            e.printStackTrace();
        }finally{
            if(br!=null){
                try{
                    br.close();
                    br = null;
                }catch(Exception e){
                    e.printStackTrace();
                }
            }
        }
        return content;
    }
    /**
     * 向Reserve0分区烧录rid号，用于手机远程遥控盒子
     */
    public static void writeRidtoPersist() {
        File ridFile = new File(RID_FILE_PATH);
        if (ridFile.exists()) {
            if (ridFile.length() == RID_FILE_LENGTH) {
                Log.e(TAG, "ridFile check pass! ridFile length is "+ridFile.length());
                ridFileCheck = true;
                return;
            } else {
                Log.e(TAG, "rid file check failed!");
                ridFile.delete();
            }
        }
        String ridXmlString = Utils.RID_KEY + Utils.buildRIDString();
        Log.e(TAG, "ridXmlString = " + ridXmlString);
        SystemMix.mkdir(Utils.RID_DIR_PATH);
        SystemMix.writeFile(Utils.RID_FILE_PATH, ridXmlString);
    }
    /**
     * 多次写入rid，保证rid写入到flash上。
     */
    public static void writeRidtoPersistSecure() {
        ridFileCheck = false;
        for(int i=0;i<5;i++) {
            Log.d(TAG, "invoke writeRidtoPersist method times:"+i);
            writeRidtoPersist();
            if(ridFileCheck) {
                Log.d(TAG, "rid file check pass,stop invoke writeRidtoPersist()");
                break;
            }
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }
    /**
     * 设置属性
     * @param name 属性名称
     * @param value 属性值
     */
    public static void setProperty(String name,String value) {
        String readValue = SystemProperties.get(name);
        if(readValue!=null&&readValue.equals(value)) {
            Log.d(TAG,"property: "+name+" check pass, read value is "+readValue);
            propertyCheck = true;
            return;
        }
        Log.d(TAG,"set property "+name+":"+value);
        SystemProperties.set(name,value);
        QwUtil.exec("sync");
    }
    /**
     * 多次设置属性，保证属性设置成功
     * @param name 属性名称
     * @param value 属性值
     */
    public static void setPropertySecure(String name,String value){
        propertyCheck = false;
        for(int i=0;i<5;i++){
            Log.d(TAG,"invoke setProperty method times:"+i);
            setProperty(name,value);
            if(propertyCheck){
                Log.d(TAG,"set property success,stop invoke setpropperty method");
                break;
            }
        }

    }
    /**
     *格式化DragonAging的老化时长
     *@param millionSeconds DragonAging当前的老化时长
     *@return 格式：xx小时xx分
     */
    public static String formatTime(int millionSeconds){
        int msRemained = millionSeconds%1000;//毫秒余数
        int secondsTotal = millionSeconds/1000;//总秒数
        int secondsRemained = secondsTotal%60;//秒余数
        int minutesTotal = secondsTotal/60;//总分钟数
        int minutesRemained = minutesTotal%60;//分钟余数
        int hours = minutesTotal/60;//总小时数
        if(msRemained!=0)
            return hours+"h"+minutesRemained+"m"+secondsRemained+"s"+msRemained;
        else if(secondsRemained!=0)
            return hours+"h"+minutesRemained+"m"+secondsRemained+"s";
        else return hours+"h"+minutesRemained+"m";
    }
    /**
     *write file to cache repeatedlly until md5 check pass
     */
    public static String writeFileToCacheSecure(String fileName,Context context) {
        md5CheckResult = false;
        String path=null;
        for(int i=0;i<5;i++){
            Log.d(TAG,"invoke writeFileToCache times:"+i);
            path = writeFileToCache(fileName,context);
            if(md5CheckResult){
                Log.d(TAG,"md5 check pass ,stop invoke writeFileToCache");
                break;
            }
        }
        return path;
    }
    public static String writeFileToCache(String fileName,Context context) {
        File file = new File(context.getCacheDir().getPath() + File.separator
                + fileName);
        AssetManager am = context.getAssets();
        try {
            if (file.exists()) {//md5 check
                InputStream fileInputStream = new FileInputStream(file);
                InputStream assetInputStream = am.open(fileName);
                String fileMD5 = MD5(fileInputStream);
                String assetMD5 = MD5(assetInputStream);
                Log.d(TAG,"file:"+fileName+" fileMD5 = "+fileMD5);
                Log.d(TAG,"assetMD5 = "+assetMD5);
                md5CheckResult = fileMD5.equals(assetMD5);
                fileInputStream.close();
                assetInputStream.close();
                if(md5CheckResult){
                    Log.d(TAG,"md5 check result pass!");
                    return file.getAbsolutePath();
                }else{
                    Log.e(TAG, "error: the copyed video file md5 check error!copy agagin!");
                    file.delete();
                }
            }
            Log.d(TAG,"start to write File to Cache");
            file.createNewFile();
            file.setReadable(true, false);
            InputStream is = am.open(fileName);
            BufferedInputStream bis = new BufferedInputStream(is);
            BufferedOutputStream bos = new BufferedOutputStream(
                    new FileOutputStream(file));
            byte[] buf = new byte[1024 * 1024];
            int length = 0;
            while ((length = bis.read(buf)) > 0) {
                bos.write(buf, 0, length);
            }
            bos.flush();
            bis.close();
            is.close();
            bos.close();
            return file.getAbsolutePath();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    private static String MD5(InputStream is) {
        try {
            byte[] buffer = new byte[8192];
            int len = 0;
            MessageDigest md = MessageDigest.getInstance("MD5");
            while ((len = is.read(buffer)) != -1) {
                md.update(buffer, 0, len);
            }
            is.close();
            byte[] bytes = md.digest();
            return toHex(bytes);
        }
        catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private static String toHex(byte[] bytes) {

        final char[] HEX_DIGITS = "0123456789ABCDEF".toCharArray();
        StringBuilder ret = new StringBuilder(bytes.length * 2);
        for (int i=0; i<bytes.length; i++) {
            ret.append(HEX_DIGITS[(bytes[i] >> 4) & 0x0f]);
            ret.append(HEX_DIGITS[bytes[i] & 0x0f]);
        }
        return ret.toString();
    }
}

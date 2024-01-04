package com.softwinner.agingdragonbox.engine;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.jar.JarEntry;
import java.util.jar.JarInputStream;
import java.security.MessageDigest;

import com.softwinner.agingdragonbox.R;
import com.softwinner.agingdragonbox.ThreadVar;
import com.softwinner.agingdragonbox.engine.testcase.CaseComprehensive;
import com.softwinner.agingdragonbox.engine.testcase.CaseGPUStress;
import com.softwinner.agingdragonbox.engine.testcase.CaseMemory;
import com.softwinner.agingdragonbox.engine.testcase.CaseThreeDimensional;
import com.softwinner.agingdragonbox.engine.testcase.CaseVideo;
import com.softwinner.agingdragonbox.service.TestFinishService;
import com.softwinner.agingdragonbox.QwUtil;

public class Utils {

    public static final String                   TAG            = "DragonAging-Uitls";
    public static final String                   VERSION            = "v1.1.2";
    static {
        Log.w(TAG, "DragonBox version:" + VERSION);
    }

    public static final String                   CONFIGURATION_NODE = "TestCase";
    public static final String                   CONFIG_FILE = "custom_aging_cases.xml";

    public static final String[]                 ALL_CASES          = {CaseComprehensive.class
                                                                            .getSimpleName() };
    public static final int LEDTIME_TESTING = 1000;//测试时，每一秒闪一次。
    public static final int LEDTIME_TEST_FINISHED = 100;//测试结束后，每0.1秒闪一次。
    public static final int LOG_TIME = 20;//每隔一段时间打印一次led log.
    public static final String PROPERTY_DRAGONAGING_TIME = "persist.sys.dragonaging_time";
    public static final String PROPERTY_DRAGONAGING_SET_TIME = "persist.sys.dragonaging_settime";
    public static final String PROPERTY_DRAGONAGING = "persist.sys.dragonaging";
    public static final String PROPERTY_DRAGON_USB0DEVICE = "persist.sys.usb0device";
    public static final int TEST_TIME = SystemProperties.getInt(PROPERTY_DRAGONAGING_SET_TIME,2*60*60*1000);//测试时长，单位：毫秒
    public static int alreadyTestTime = SystemProperties.getInt(PROPERTY_DRAGONAGING_TIME,0); 
    public static boolean md5CheckResult;//比较拷贝前后文件的md5结果
    public static boolean propertyCheck;//检查属性设置情况

    public static final HashMap<String, Integer> CASE_MAP
        = new HashMap<String, Integer>();
    static {
        // CASE_MAP.put(CaseComprehensive.class.getSimpleName(),
        // R.string.case_comprehensive);
        CASE_MAP.put(CaseMemory.class.getSimpleName(), R.string.case_memory_name);
        CASE_MAP.put(CaseVideo.class.getSimpleName(), R.string.case_video_name);
        CASE_MAP.put(CaseThreeDimensional.class.getSimpleName(),
                R.string.case_threedimensional_name);
        CASE_MAP.put(CaseGPUStress.class.getSimpleName(), R.string.case_threedimensional_name);

    }

    public static BaseCase createCase(String caseName) {
        BaseCase aCase = null;
        try {
            BaseCase clazz = (BaseCase) Class.forName(
                    "com.softwinner.agingdragonbox.engine.testcase." + caseName).newInstance();
            aCase = clazz;
        } catch (ClassNotFoundException e) {
            Log.w(TAG, e.getMessage());
        } catch (ClassCastException e) {
            Log.w(TAG, e.getMessage());
        } catch (InstantiationException e) {
            Log.w(TAG, e.getMessage());
        } catch (IllegalAccessException e) {
            Log.w(TAG, e.getMessage());
        }
        return aCase;
    }

    public static boolean search(ArrayList<File> result, File folder, String[] types,
            boolean returnFirst) {
        File[] fileList = folder.listFiles();
        boolean b = false;
        if (fileList == null) {
            return b;
        }
        for (File file : fileList) {

            if (file.isFile()) {
                boolean tag = false;
                for (String type : types) {
                    // Log.d(Utils.TAG,"file:" + file.getName() + type);
                    tag |= file.getName().endsWith(type);
                }
                if (tag) {
                    result.add(file);
                }
                b |= tag;
            }
            if (returnFirst && result.size() > 0) {
                return b;
            }
            if (file.isDirectory()) {
                b |= search(result, file, types, returnFirst);
            }
        }
        return b;
    }

    public static Object callMethod(Object obj, String methodName, Object[] args,
            Class<?>... aClassType) throws SecurityException, NoSuchMethodException,
            IllegalArgumentException, IllegalAccessException, InvocationTargetException,
            ClassNotFoundException {
        Class<?> aClass = obj.getClass();
        Method method = aClass.getDeclaredMethod(methodName, aClassType);
        Object object = method.invoke(obj, args);
        return object;
    }

    public static Object getFields(Class<?> aClass, String fieldsName) throws SecurityException,
            NoSuchMethodException, IllegalArgumentException, IllegalAccessException,
            InvocationTargetException, NoSuchFieldException {
        Field field = aClass.getField(fieldsName);
        // 原来是field.getInt(_class);改成get(_class)，不知是否会出现问题
        Object object = field.get(aClass);
        return object;
    }

    private static final String[]   CLASS_PATH_PROP  = {"java.class.path", "java.library.path" };

    private static List<File> classPathArray = getClassPath();

    private static List<File> getClassPath() {
        List<File> ret = new ArrayList<File>();
        String delim = ":";
        if (System.getProperty("os.name").indexOf("Windows") != -1) {
            delim = ";";
        }
        for (String pro : CLASS_PATH_PROP) {
            String str = System.getProperty(pro);
            Log.d(TAG, "getProperty:" + System.getProperty(pro));
            if (str != null) {
                String[] pathes = str.split(delim);
                for (String path : pathes) {
                    ret.add(new File(path));
                }
            }
        }
        return ret;
    }

    public static List<String> getClassInPackage(String pkgName) {
        List<String> ret = new ArrayList<String>();
        String rPath = pkgName.replace('.', '/') + "/";
        try {
            for (File classPath : classPathArray) {
                if (!classPath.exists()) {
                    continue;
                }
                if (classPath.isDirectory()) {
                    File dir = new File(classPath, rPath);
                    if (!dir.exists()) {
                        continue;
                    }
                    for (File file : dir.listFiles()) {
                        if (file.isFile()) {
                            String clsName = file.getName();
                            clsName = pkgName + "." + clsName.substring(0, clsName.length() - 6);
                            ret.add(clsName);
                        }
                    }
                } else {
                    FileInputStream fis = new FileInputStream(classPath);
                    JarInputStream jis = new JarInputStream(fis, false);
                    JarEntry e = null;
                    while ((e = jis.getNextJarEntry()) != null) {
                        String eName = e.getName();
                        if (eName.startsWith(rPath) && !eName.endsWith("/")) {
                            ret.add(eName.replace('/', '.').substring(0, eName.length() - 6));
                        }
                        jis.closeEntry();
                    }
                    jis.close();
                }
            }
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        return ret;
    }

    public static void getCaseList(String packageName) throws IOException {
        List<String> cls = getClassInPackage(packageName);
        for (String s : cls) {
            System.out.println(s);
        }
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

    public static void chanceLedStatus(int status) {
        if (status > 0) {
            com.softwinner.Gpio.setNormalLedOn(false);
            //com.softwinner.Gpio.setStandbyLedOn(true);
        } else {
            com.softwinner.Gpio.setNormalLedOn(true);
            //com.softwinner.Gpio.setStandbyLedOn(false);
        } 
    }

    public static void startCtrlLedThread() {
        new Thread() {
            public void run() {
                try {
                    int ledTime =1000;
                    Log.d(TAG,"begin blinking led!");
                    int logFrq = 0;//当logFrq==20时，打印调试日志
                    while (ThreadVar.threadExitDdr && ThreadVar.threadExit) {
                        if (ThreadVar.testFinished) {
                            ledTime = LEDTIME_TEST_FINISHED;
                            logFrq = logFrq+1;
                        }else {
                            ledTime = LEDTIME_TESTING;
                            logFrq = logFrq+10;
                        }
                        if(logFrq>=LOG_TIME){
                            logFrq=0;
                            Log.e(TAG,"blink led once!(print this log every four seconds)");
                        }
                        chanceLedStatus(0);
                        Thread.sleep(ledTime);
                        chanceLedStatus(1);
                        Thread.sleep(ledTime);
                    }
                    Log.d(TAG,"stop blinking led! memory occured some error");
                    chanceLedStatus(0);//memory/video异常时，灯常亮
                } catch (Exception e) {
                    e.printStackTrace();
                }

            }
        }.start();
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
}

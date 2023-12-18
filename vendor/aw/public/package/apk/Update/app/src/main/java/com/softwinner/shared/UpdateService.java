package com.softwinner.shared;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.provider.Settings;
import android.support.annotation.Nullable;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

import com.softwinner.update.PropertiesHelper;
import com.softwinner.update.R;
import com.softwinner.update.UpdateApplication;
import com.softwinner.update.ui.DialogActivity;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.*;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;


public class UpdateService extends Service{
    private final String TAG = "UpdateService";
    private  NotificationManager updateNotificationManager;
    private static final int EXECUTE_COMPLETE = 0;
    private static final int EXECUTE_UNZIP = 2;
    private static final int EXECUTE_UPDATE = 3;
    private static final int EXECUTE_FAIL = 4;
    private static final int EXECUTE_FINALCOMPLETE = 5;
    private NotificationCompat.Builder builder;
    private Notification notification; //更新通知进度提示
    float step = 100.0f / 250.0f;
    float process = 0;
    String packagePath="";

    private Handler updateHandler = new  Handler(){
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what){
                case EXECUTE_COMPLETE:
                    updateHandler.removeMessages(EXECUTE_UNZIP);
                    updateHandler.removeMessages(EXECUTE_UPDATE);
                    Log.e(TAG,"EXECUTE_COMPLETE");
                    PropertiesHelper.set(UpdateService.this, "ota.updateboot","1");
                    updateHandler.sendEmptyMessageDelayed(EXECUTE_FINALCOMPLETE, 1000);
                    break;
                case EXECUTE_FINALCOMPLETE:
                    builder.setContentText(getResources().getString(R.string.update_completed))
                            .setProgress(100 ,100,true);
                    notification = builder.build();
                    updateNotificationManager.notify(1, notification);
                    updateNotificationManager.cancel(1);
                    PropertiesHelper.set(UpdateService.this, "ota.updateboot","0");
                    PropertiesHelper.set(UpdateService.this, "ota.package.path","");
                   // showDialog();
                    Intent serviceIntent = new Intent();
                    serviceIntent.setAction("com.softwinner.shared.UpdateService");
                    sendBroadcast(serviceIntent);
                    Intent intent =new Intent(UpdateService.this, DialogActivity.class);
                    startActivity(intent);
                    break;
                case EXECUTE_UNZIP:
                    if (process < 100.0f) {
                        builder.setContentText(getResources().getString(R.string.unzip))
                                .setProgress(100,  (int)process, false);
                        notification = builder.build();
                        updateNotificationManager.notify(1, notification);
                        process = process + step;
                        updateHandler.removeMessages(EXECUTE_UNZIP);
                        updateHandler.sendEmptyMessageDelayed(EXECUTE_UNZIP, 1000);
                    }
                    break;
                case EXECUTE_UPDATE:
                    updateHandler.removeMessages(EXECUTE_UNZIP);
                    if (process < 100.0f) {
                        builder.setContentText(getResources().getString(R.string.update))
                                .setProgress(100, (int)process, false);
                        notification = builder.build();
                        updateNotificationManager.notify(1, notification);
                        process = process + step;
                        updateHandler.removeMessages(EXECUTE_UPDATE);
                        updateHandler.sendEmptyMessageDelayed(EXECUTE_UPDATE, 1000);
                    }

                    break;
                case EXECUTE_FAIL:
                    //升级失败
                    builder.setContentText(getResources().getString(R.string.update_fail));
                    notification = builder.build();
                    updateNotificationManager.notify(1, notification);
                    updateNotificationManager.cancel(1);
                    updateHandler.removeMessages(EXECUTE_UNZIP);
                    updateHandler.removeMessages(EXECUTE_UPDATE);
                    Log.e(TAG,"EXECUTE_FAIL");
                    Intent intent2= new Intent();
                    intent2.setAction("com.softwinner.shared.UpdateService");
                    sendBroadcast(intent2);
                    Handler handler = new Handler(Looper.getMainLooper());

                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(UpdateService.this,getResources().getString(R.string.update_fail_hint), Toast.LENGTH_LONG)
                                    .show();
                        }
                    });
                    break;
                default:
                    //stopSelf();
            }
        }
    };
    private DataOutputStream dos;
    private Process p;
    private Thread thread;


    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "===========onCreate============= ");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "===========onStartCommand============= ");
        //获取传值
        //titleId = intent.getIntExtra("titleId",0);
        updateNotificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        packagePath = intent.getStringExtra("path");
        //packageFile = new File(packagePath);
        builder = new NotificationCompat.Builder(this);
        builder.setContentTitle(getResources().getString(R.string.update_system_txt)) //设置通知标题
                .setSmallIcon(R.drawable.ic_upgrade_hl)
                .setAutoCancel(false)//设置通知被点击一次是否自动取消
                .setContentText(R.string.update_progress + "0%");
        notification = builder.build();//构建通知对象
        new Thread(new updateRunnable()).start();
        startForeground(1, notification);// 开始前台服务
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }
    class updateRunnable implements Runnable {
        List<String> commandHeadersList;
        String commandHeaderstr="";
        //Message messageUnzip = updateHandler.obtainMessage();
        Message messageFail = updateHandler.obtainMessage();
        Message messageUpdate = updateHandler.obtainMessage();
        Message messageComplete = updateHandler.obtainMessage();
        String commandUpdate="";
        String updateState="";
        //String updatePath="/sdcard/update/";
        String headOffset="";
        String size="";
        public void run() {
            try{
                Log.e(TAG,"packagePath:"+packagePath);
                if(packagePath.startsWith("/storage/emulated/")){
                    packagePath=packagePath.replaceFirst("/storage/emulated/0","/sdcard");
                }
                PropertiesHelper.set(UpdateService.this, "ota.package.path",packagePath);
                messageUpdate.what = EXECUTE_UPDATE;
                updateHandler.sendMessage(messageUpdate);
                commandHeadersList =readZipFile(packagePath);
                headOffset=commandHeadersList.get(0);
                size=commandHeadersList.get(1);
                for(int i=2;i<commandHeadersList.size();i++){
                    commandHeaderstr=commandHeaderstr+commandHeadersList.get(i)+"\n";
                }
                Log.e(TAG,"commandHeaderstr"+commandHeaderstr);
                commandUpdate="update_engine_client --update --follow --payload=file://"+packagePath+" --offset="+headOffset+" --size="+size+" --headers=\""+commandHeaderstr+"\"\n"+"exit";
                Log.e(TAG,"commandUpdate"+commandUpdate);
                Log.e(TAG,"start exec");
                int result =exec(commandUpdate);
                //int result = exec();
                Log.e(TAG,"end exec");
                updateState=PropertiesHelper.get(UpdateService.this, "ota.state");
                Log.e(TAG,"updateState:"+updateState);
                Log.e(TAG,"result:"+result);
                if (result == 0) {

                    //升级成功

                    if(updateState.equals("1")) {

                        messageComplete.what = EXECUTE_COMPLETE;
                        updateHandler.sendMessage(messageComplete);
                        PropertiesHelper.set(UpdateService.this, "ota.state","0");

                    }else{
                        //升级失败
                        messageFail.what = EXECUTE_FAIL;
                        updateHandler.sendMessage(messageFail);
                    }
                }else {
                    messageFail.what = EXECUTE_FAIL;
                    updateHandler.sendMessage(messageFail);
                }
            }catch(Exception ex){
                ex.printStackTrace();
                messageFail.what = EXECUTE_FAIL;
                //执行失败
                updateHandler.sendMessage(messageFail);
            }
        }
    }

    private int exec(String command) {
        try {
            Process process = Runtime.getRuntime().exec("sh");
            //Process process = Runtime.getRuntime().exec(command);
            dos = new DataOutputStream(process.getOutputStream());
            Log.i(TAG, command);
            dos.writeBytes(command + "\n");
            dos.flush();
            process.waitFor();
            return process.exitValue();
        } catch (IOException e) {
            throw new RuntimeException(e);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }
    public static List<String> readZipFile(String file) throws Exception {
        List<String> list = new ArrayList<String>();
        ZipFile zf = new ZipFile(file);
        InputStream in = new BufferedInputStream(new FileInputStream(file));
        ZipInputStream zin = new ZipInputStream(in);
        ZipEntry ze;
        Enumeration<? extends ZipEntry> zipEntries = zf.entries();
        long offset = 0;
        while (zipEntries.hasMoreElements()) {
            ZipEntry entry = (ZipEntry) zipEntries.nextElement();
            long fileSize = 0;
            long extra = entry.getExtra() == null ? 0 : entry.getExtra().length;
            offset += 30 + entry.getName().length() + extra;
            if (!entry.isDirectory()) {
                if (entry.getName().equals("payload.bin")) {
                    list.add(offset+"");
                    list.add(entry.getSize()+"");
                    System.err.println("file -" + entry.getName() + " : "
                            + entry.getSize() + " offset::" + offset);
                }
                fileSize = entry.getCompressedSize();
                if (entry.getName().equals("payload_properties.txt")) {
                    System.err.println("file - " + entry.getName() + " : "
                            + entry.getSize() + " bytes");
                    long size = entry.getSize();
                    if (size > 0) {
                        BufferedReader br = new BufferedReader(
                                new InputStreamReader(zf.getInputStream(entry)));
                        String line;
                        while ((line = br.readLine()) != null) {
                            list.add(line);
                            System.out.println(line);
                        }
                        br.close();
                    }
                    System.out.println();

                    // Do stuff here with fileSize & offset
                }
                offset += fileSize;
            }
        }
        zin.closeEntry();
        return list;
    }
}

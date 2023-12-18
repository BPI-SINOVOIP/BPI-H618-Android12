package com.android.abupdater.ui;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.lang.Process;
import android.os.RecoverySystem;
import com.android.abupdater.view.WaterWaveView;
import com.android.abupdater.ui.SimpleDialog;

import android.os.UpdateEngine;
import android.os.UpdateEngineCallback;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.List;
import java.util.ArrayList;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.OutputStream;
import android.os.Looper;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

import com.android.abupdater.R;

public class UpdateMainActivity extends Activity {

    private final static String TAG = "UpdateMainActivity";
    private static final boolean debug = true;

    private static final String FILE_PATH = "filePath";

    private Context mContext;
    private WaterWaveView mWaterWaveView;
    private TextView mShowProgress;
    private String msgRecord = "";
    private String updatePath = "";
    private String copyFilePath = "";
    private int progress = 0;

    // handler code
    private final static int UPDATE_COPY_FILE = 110;
    private final static int UPDATE_UNZIP_PACKAGE = 111;
    private final static int UPDATE_VERIFY_PACKAGE = 112;
    private final static int UPDATE_APPLY_PACKAGE = 113;
    private final static int UPDATE_PROGRESS_REFRESH = 114;
    private final static int UPDATE_APPLY_REBOOT = 115;
    private final static int UPDATE_APPLY_FAILED = 116;

    // failed code
    private final static int UPDATE_COPY_FILE_FAILED = 210;
    private final static int UPDATE_UNZIP_PACKAGE_FAILED = 211;
    private final static int UPDATE_VERIFY_PACKAGE_FAILED = 212;
    private final static int UPDATE_APPLY_PACKAGE_FAILED = 213;
    private final static int UPDATE_PROGRESS_REFRESH_FAILED = 214;
    private final static int UPDATE_APPLY_REBOOT_FAILED = 215;
    private final static int UPDATE_APPLY_FAILED_FAILED = 216;

    // file path
    private final static String OTA_UPDATE_ZIP = "update.zip";
    private final static String DATA_OTA_PATH = "/data/ota_package/";
    private final static String OTA_FILE_LINK = "file://";
    private final static String OTA_PAYLOAD_BIN = "payload.bin";
    private static final String DATA_OTA_METADATA = "/data/ota_package/META-INF/com/android/metadata";
    private static final String OTA_PAYLOAD_PROP = "payload_properties.txt";

    // set duration 5 * 1000 ms
    private final static int ANIMATOR_DURATION = 1 * 1000;

    private SimpleDialog mSimpleDialog;

    private Handler mHandler = new Handler(Looper.myLooper()) {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case UPDATE_COPY_FILE:
                    mShowProgress.setText(getString(R.string.local_update_progress_copy));
                    updateProgress(5);
                    copyPackage();
                    break;
                case UPDATE_UNZIP_PACKAGE:
                    mShowProgress.setText(getString(R.string.local_update_progress_unzip));
                    updateProgress(10);
                    new Thread(() -> {unzipPackage(copyFilePath, DATA_OTA_PATH);}).start();
                    break;
                case UPDATE_VERIFY_PACKAGE:
                    mShowProgress.setText(getString(R.string.local_update_progress_verify));
                    updateProgress(15);
                    new Thread(() -> {verifyPackage();}).start();
                    break;
                case UPDATE_APPLY_PACKAGE:
                    mShowProgress.setText(getString(R.string.local_update_progress_apply));
                    updateProgress(20);
                    new Thread(() -> {applyUpdate();}).start();
                    break;
                case UPDATE_PROGRESS_REFRESH:
                    progress = 20 + (msg.arg1 * 4 / 5);
                    mShowProgress.setText(getString(R.string.local_update_progress_update) + String.valueOf(msg.arg1) + "%");
                    updateProgress(progress);
                    break;
                case UPDATE_APPLY_REBOOT:
                    updateProgress(100);
                    mShowProgress.setText(getString(R.string.local_update_progress_reboot));
                    removeUpdateFile(DATA_OTA_PATH);
                    if (mSimpleDialog != null) {
                        mSimpleDialog.showUseDialog("",
                                    getString(R.string.local_update_progress_reboot_content),
                                    getString(R.string.local_update_progress_reboot_confirm),
                                    () -> {
                                        startReboot();
                                    },
                                    getString(R.string.local_update_progress_reboot_cancel),
                                    () -> {
                                        finish();
                                    }
                        );
                    }
                    break;
                case UPDATE_APPLY_FAILED:
                    msgRecord = "";
                    switch (msg.arg1) {
                        case UPDATE_COPY_FILE_FAILED:
                            msgRecord = "copy file";
                            break;
                        case UPDATE_UNZIP_PACKAGE_FAILED:
                            msgRecord = "unzip file";
                            break;
                        case UPDATE_VERIFY_PACKAGE_FAILED:
                            msgRecord = "verify file";
                            break;
                        case UPDATE_APPLY_PACKAGE_FAILED:
                            msgRecord = "apply update";
                            break;
                        case UPDATE_PROGRESS_REFRESH_FAILED:
                            msgRecord = "update";
                            break;
                        case UPDATE_APPLY_REBOOT_FAILED:
                            msgRecord = "reboot";
                            break;
                        default:
                            break;
                    }
                    if (!msgRecord.equals("")) {
                        msgRecord += " failed";
                        mShowProgress.setText(msgRecord);
                    }
                    removeUpdateFile(DATA_OTA_PATH);
                    break;
            }
            super.handleMessage(msg);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.update_main_activity);
        mContext = this;

        // get package path
        Intent intent = getIntent();
        if (intent == null) {
            finish();
        }
        Bundle bundle = intent.getExtras();
        if (bundle == null) {
            finish();
        }
        updatePath = bundle.getString(FILE_PATH);
        if (updatePath.equals("")) {
            if (debug) Log.w (TAG, FILE_PATH + " is null, finish it");
            finish();
        }

        if (updatePath.lastIndexOf(".zip") == -1 && updatePath.lastIndexOf(".bin") == -1) {
            if (debug) Log.w (TAG, FILE_PATH + " is invalid, finish it");
            finish();
        }

        initView();
        initData();
    }

    private void initView() {
        mWaterWaveView = (WaterWaveView) findViewById(R.id.wave_progress_view);
        mShowProgress = (TextView) findViewById(R.id.show_msg_progress);
    }

    private void initData() {
        mSimpleDialog = new SimpleDialog(mContext);
        mWaterWaveView.setWaveHeight(10);
        mWaterWaveView.setTranslateX(7);
        mWaterWaveView.setDelayTime(150);
        Message message = new Message();
        message.what = UPDATE_COPY_FILE;
        mHandler.sendMessage(message);
    }

    private void updateProgress(int progress) {
        mWaterWaveView.setProgress(progress, ANIMATOR_DURATION);
    }

    private void copyPackage() {
        new Thread(() -> {
            Message message = new Message();
            message.what = UPDATE_UNZIP_PACKAGE;
            File file = new File(DATA_OTA_PATH);
            if (file.exists() && file.canWrite()) {
                File targetFile;
                if (updatePath.lastIndexOf(".zip") != -1) {
                    targetFile = new File(DATA_OTA_PATH, OTA_UPDATE_ZIP);
                } else {
                    targetFile = new File(DATA_OTA_PATH, OTA_PAYLOAD_BIN);
                }
                if (!copyFile(new File(updatePath), targetFile)) {
                    if (debug) Log.w(TAG,"cope file failed");
                    message.what = UPDATE_APPLY_FAILED;
                    message.arg1 = UPDATE_COPY_FILE_FAILED;
                };
                copyFilePath = targetFile.getAbsolutePath();
            }
            mHandler.sendMessage(message);
        }).start();
    }

    private boolean copyFile(File sorceFile, File targetFile) {
        boolean copyStatus = true;
        FileInputStream inputStream = null;
        FileOutputStream outputStream = null;
        try {
            if (!targetFile.exists()) {
                targetFile.createNewFile();
                Process p = Runtime.getRuntime().exec("chmod 777 " + targetFile.getAbsolutePath());
                int status = p.waitFor();
                if (debug) Log.w(TAG,"the file Process status: " + status);
            }
            long outSizipEntry = 0;
            inputStream = new FileInputStream(sorceFile);
            outputStream = new FileOutputStream(targetFile);
            int length = -1;
            byte[] buf = new byte[1024];
            while ((length = inputStream.read(buf)) != -1) {
                outputStream.write(buf, 0, length);
                outSizipEntry += length;
            }
        } catch (IOException e) {
            copyStatus = false;
            e.printStackTrace();
        } catch (InterruptedException e) {
            copyStatus = false;
            e.printStackTrace();
        } finally {
            try {
                outputStream.flush();
                inputStream.close();
                outputStream.close();
            } catch(IOException e) {
                e.printStackTrace();
            }
        }
        return copyStatus;
    }

    private void unzipPackage(String zipFile, String unzipPath) {
        if (debug) Log.w(TAG, "zipFile: " + zipFile + " unzipPath: " + unzipPath);
        if (zipFile.lastIndexOf(".bin") != -1) {
            return;
        }
        boolean unzipStatus = true;
        Message message = new Message();
        message.what = UPDATE_VERIFY_PACKAGE;
        try {
            OutputStream outputStream = null;
            InputStream inputStream = null;
            ZipFile zfile = null;
            zfile = new ZipFile(new File(zipFile));
            Enumeration <? extends ZipEntry> zList = zfile.entries();

            ZipEntry zipEntry = null;
            byte[] buf = new byte[1024];
            while (zList != null && zList.hasMoreElements()) {
                if (debug) Log.d(TAG, "unzip start");
                zipEntry = (ZipEntry) zList.nextElement();
                if (zipEntry.isDirectory()) {
                    if (debug) Log.d(TAG, "file name: " + zipEntry.getName());
                    String dirstr = unzipPath + zipEntry.getName();
                    dirstr = new String(dirstr.getBytes("8859_1"), "GB2312");
                    File f = new File(dirstr);
                    f.mkdir();
                    continue;
                }
                if (debug) Log.d(TAG, "file name: " + zipEntry.getName());
                getRealFileName(unzipPath, zipEntry.getName());
                outputStream = new BufferedOutputStream(new FileOutputStream(unzipPath + zipEntry.getName()));
                inputStream = new BufferedInputStream(zfile.getInputStream(zipEntry));
                int readLen = 0;
                if (debug) Log.d(TAG, "copy file");
                while ((readLen = inputStream.read(buf, 0, 1024)) != -1) {
                    outputStream.write(buf, 0, readLen);
                }
                if (debug) Log.d(TAG, "unzip end, start chmod");
                Process process = Runtime.getRuntime().exec("chmod 777 " +  DATA_OTA_PATH + zipEntry.getName());
                int status = process.waitFor();
                if (debug) Log.d(TAG, "file grant status： " + status);
                if (inputStream != null) {
                    inputStream.close();
                    inputStream = null;
                }
                if (outputStream != null) {
                    outputStream.close();
                    outputStream = null;
                }
            }
            zfile.close();
        } catch (InterruptedException e) {
            unzipStatus = false;
            e.printStackTrace();
        } catch (IOException e){
            unzipStatus = false;
            e.printStackTrace();
        }
        if (!unzipStatus) {
            message.what = UPDATE_APPLY_FAILED;
            message.arg1 = UPDATE_UNZIP_PACKAGE_FAILED;
        }
        mHandler.sendMessage(message);
    }
    private void verifyPackage() {
        // do nothing
        mHandler.sendEmptyMessage(UPDATE_APPLY_PACKAGE);
    }

    private void applyUpdate() {
        UpdateEngine mUpdateEngine = new UpdateEngine();
        mUpdateEngine.bind(new UECallback());
        int offset = 0;
        int size = 1;
        int[] data = getMetadataInfo(DATA_OTA_METADATA);
        String fileLink = OTA_FILE_LINK + DATA_OTA_PATH + OTA_UPDATE_ZIP;
        try {
            mUpdateEngine.applyPayload(fileLink, data[offset], data[size], getPayloadProperties(DATA_OTA_PATH + OTA_PAYLOAD_PROP));// 进行升级
        } catch (Exception e) {
            String exceptionInfo = e.getLocalizedMessage();
            Log.w(TAG, "getLocalizedMessage: " + exceptionInfo);
            if (exceptionInfo.contains("reboot") && exceptionInfo.contains("applied")) {
                mHandler.sendEmptyMessage(UPDATE_APPLY_REBOOT);
            }
            e.printStackTrace();
        }
    }

    private class UECallback extends UpdateEngineCallback {
        public void onStatusUpdate(int status, float percent) {
            if (status == UpdateEngine.UpdateStatusConstants.DOWNLOADING) {// 回调状态，升级进度
                Message message = new Message();
                message.what = UPDATE_PROGRESS_REFRESH;
                message.arg1 = (int) (percent * 100);
                Log.d(TAG, "update progress: " + message.arg1);
                mHandler.sendMessage(message);
            }
        }

        public void onPayloadApplicationComplete(int errCode) {
            if (debug) Log.e(TAG, "Payload application complete, error: " + Integer.toString(errCode));
            if (errCode == 0) {
                if (debug) Log.e(TAG, "Installation succeeded!");
                mHandler.sendEmptyMessage(UPDATE_APPLY_REBOOT);
            }else {
                Message message = new Message();
                message.what = UPDATE_APPLY_FAILED;
                message.arg1 = UPDATE_PROGRESS_REFRESH_FAILED;
                mHandler.sendMessage(message);
            }
        }
    }

    private int[] getMetadataInfo(String metadataPath) {
        int[] lines = new int[2];
        InputStreamReader is = null;
        BufferedReader br = null;
        try {
            File file = new File(metadataPath);
            is = new InputStreamReader(new FileInputStream(file));
            br = new BufferedReader(is);
            String line = null;
            while ((line = br.readLine()) != null) {
                if(line.contains(OTA_PAYLOAD_BIN)){
                    String[]  lineInfo = line.split(",");
                    for(int i=0;i<lineInfo.length;i++){
                        if(lineInfo[i].contains(OTA_PAYLOAD_BIN)){
                            String[] data = lineInfo[i].split(":");
                            int offset = Integer.valueOf(data[1]);
                            int size = Integer.valueOf(data[2]);
                            System.out.print(offset);
                            System.out.print(size);
                            lines[0] = offset;
                            lines[1] = size;
                            break;
                        }
                    }
                    break;
                }
            }
            return lines;
        } catch (IOException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                br.close();
                is.close();
            } catch(IOException e) {
                e.printStackTrace();
            }
        }
        return lines;
    }

    public  String[] getPayloadProperties(String payloadPath) {
        try {
            File file = new File(payloadPath);
            InputStreamReader is = new InputStreamReader(new FileInputStream(file));
            BufferedReader br = new BufferedReader(is);
            List<String> lines = new ArrayList<String>();
            String line = null;
            while ((line = br.readLine()) != null) {
                Log.d(TAG, "getPayloadProperties line: " + line);
                lines.add(line);
            }
            br.close();
            is.close();
            return lines.toArray(new String[lines.size()]);
        } catch (IOException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public File getRealFileName(String baseDir, String absFileName) {
        String[] dirs = absFileName.split("/");
        File ret = new File(baseDir);
        String substr = null;
        if (dirs.length > 1) {
            for (int i = 0; i < dirs.length - 1; i++) {
                substr = dirs[i];
                try {
                    substr = new String(substr.getBytes("8859_1"), "GB2312");
                } catch (UnsupportedEncodingException e) {
                    e.printStackTrace();
                }
                ret = new File(ret, substr);

            }
            Log.d(TAG, "ret = " + ret);
            if (!ret.exists())
                ret.mkdirs();
                substr = dirs[dirs.length - 1];
                try {
                    substr = new String(substr.getBytes("8859_1"), "GB2312");
                    Log.d(TAG, "substr = " + substr);
                } catch (UnsupportedEncodingException e) {
                    e.printStackTrace();
                }
                ret = new File(ret, substr);
                Log.d(TAG, "ret = " + ret);
                return ret;
        }
        return ret;
    }

    private void removeUpdateFile(String deleteFile) {
        File file = new File(deleteFile);
        try {
            if (file.exists() && file.canWrite()) {
                File[] fileList = file.listFiles();
                for (File f : fileList) {
                    if(f.isFile()) {
                        f.delete();
                    }
                    if(f.isDirectory()) {
                        removeUpdateFile(f.getAbsolutePath());
                    }
                }
            }
        } catch(Exception e) {
            e.printStackTrace();
        }
    }

    private void startReboot() {
        if (debug) Log.w(TAG,"start reboot!");
        Intent intent = new Intent(Intent.ACTION_REBOOT);
        intent.putExtra("android.intent.extra.KEY_CONFIRM", false);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
    }

    @Override
    protected void onDestroy() {
        removeUpdateFile(DATA_OTA_PATH);
        super.onDestroy();
    }
}

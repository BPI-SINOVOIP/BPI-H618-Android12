package com.softwinner.update.ui;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.os.RecoverySystem;
import android.os.ServiceSpecificException;
import android.os.UpdateEngine;
import android.os.UpdateEngineCallback;
import android.view.View;
import android.widget.TextView;
import android.util.Log;

import com.softwinner.update.Configs;
import com.softwinner.update.view.WaterWaveView;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import com.softwinner.shared.FileInfo;
import com.softwinner.update.R;

public class AbUpdate extends Activity {

    private final static String TAG = "AbUpdate";
    private static final boolean debug = true;

    private static final String FILE_PATH = "path";

    private Context mContext;
    private WaterWaveView mWaterWaveView;
    private TextView mShowProgress;
    private String mMsgRecord = "";
    private FileInfo.FileUnit mFileUnit = null;
    private int mFileType = FileInfo.TYPE_URI;
    private int mProgress = 0;
    private String mLocalPath = "";

    private UpdateEngine mUpdateEngine = null;

    // handler code
    private final static int UPDATE_VERIFY_PACKAGE = 112;
    private final static int UPDATE_APPLY_PACKAGE = 113;
    private final static int UPDATE_PROGRESS_REFRESH = 114;
    private final static int UPDATE_APPLY_REBOOT = 115;
    private final static int UPDATE_APPLY_FAILED = 116;

    // failed code
    private final static int UPDATE_VERIFY_PACKAGE_FAILED = 212;
    private final static int UPDATE_APPLY_PACKAGE_FAILED = 213;
    private final static int UPDATE_PROGRESS_REFRESH_FAILED = 214;
    private final static int UPDATE_APPLY_REBOOT_FAILED = 215;
    private final static int UPDATE_APPLY_FAILED_FAILED = 216;

    // file path
    private final static String OTA_FILE_LINK = "file://";
    private final static String OTA_PAYLOAD_BIN = "payload.bin:";
    private static final String OTA_METADATA = "META-INF/com/android/metadata";
    private static final String OTA_PAYLOAD_PROP = "payload_properties.txt";


    // set duration 1 * 1000 ms
    private final static int ANIMATOR_DURATION = 1 * 1000;

    private SimpleDialog mSimpleDialog;

    private Handler mHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case UPDATE_VERIFY_PACKAGE:
                    mShowProgress.setText(getString(R.string.local_update_progress_verify));
                    updateProgress(15);
                    new Thread(() -> {verifyPackage();}).start();
                    break;
                case UPDATE_APPLY_PACKAGE:
                    mShowProgress.setText(getString(R.string.local_update_progress_apply));
                    updateProgress(20);
                    new Thread(() -> {applyUpdate(mFileUnit.getPath(), mFileUnit.getType());}).start();
                    break;
                case UPDATE_PROGRESS_REFRESH:
                    mProgress = 20 + (msg.arg1 * 4 / 5);
                    mShowProgress.setText(getString(R.string.local_update_progress_update) +
                            String.valueOf(msg.arg1) + "%");
                    updateProgress(mProgress);
                    break;
                case UPDATE_APPLY_REBOOT:
                    updateProgress(100);
                    mShowProgress.setText(
                        getString(R.string.local_update_progress_reboot));
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
                    mMsgRecord = "";
                    switch (msg.arg1) {
                        case UPDATE_VERIFY_PACKAGE_FAILED:
                            mMsgRecord = "verify file";
                            break;
                        case UPDATE_APPLY_PACKAGE_FAILED:
                            mMsgRecord = "apply update";
                            break;
                        case UPDATE_PROGRESS_REFRESH_FAILED:
                            mMsgRecord = "update";
                            break;
                        case UPDATE_APPLY_REBOOT_FAILED:
                            mMsgRecord = "reboot";
                            break;
                        default:
                            break;
                    }
                    if (!mMsgRecord.equals("")) {
                        mMsgRecord += " failed";
                        mShowProgress.setText(mMsgRecord);
                    }
                    break;
            }
            super.handleMessage(msg);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.ab_update);
        mContext = this;

        // get package path
        Intent intent = getIntent();
        if (intent == null) {
            finish();
        }
        mFileUnit = (FileInfo.FileUnit) intent.getSerializableExtra(Configs.UPDATEPATH);
        if (mFileUnit.getPath().equals("")) {
            if (debug) Log.w (TAG, FILE_PATH + " is null, finish it");
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

        // prepare update engine
        mLocalPath = getCacheDir().getAbsolutePath();
        mUpdateEngine = new UpdateEngine();
        mHandler.sendEmptyMessage(UPDATE_APPLY_PACKAGE);
    }

    private void updateProgress(int progress) {
        mWaterWaveView.setProgress(progress, ANIMATOR_DURATION);
    }

    private void verifyPackage() {
        // verify
        Message message = mHandler.obtainMessage(
                UPDATE_APPLY_FAILED,
                UPDATE_VERIFY_PACKAGE_FAILED,
                0,
                null);
        //mUpdateEngine.verifyPayloadMetadata(DATA_OTA_PATH + OTA_PAYLOAD_BIN)) {
        message.what = UPDATE_APPLY_PACKAGE;
        mHandler.sendMessage(message);
    }

    private void applyUpdate(String updateUri, int type) {
        mUpdateEngine.bind(new UECallback());
        int offset = 0;
        int fileSize = 0;
        String [] mHeaderKeyValuePairs = null;
        try {
            ParcelFileDescriptor pfd = null;
            switch(type) {
                case FileInfo.TYPE_URI:
                    pfd = getContentResolver().openFileDescriptor(Uri.parse(updateUri.toString()), "rw");
                    break;
                case FileInfo.TYPE_FILE:
                    pfd = ParcelFileDescriptor.open(new File(updateUri), ParcelFileDescriptor.MODE_READ_ONLY);
                    break;
            }
            if (pfd == null) {
                if (debug) Log.w(TAG, "pfd is invalid!");
                return;
            }
            Log.w(TAG, "size: " + pfd.getStatSize());
            AssetFileDescriptor afd = new AssetFileDescriptor(pfd, 0, pfd.getStatSize());
            InputStream is = afd.createInputStream();
            ZipInputStream zis = new ZipInputStream(is);
            ZipEntry ze;
            while((ze = zis.getNextEntry()) != null) {
                String name = ze.getName();
                Log.w(TAG, "name: " + name);
                if (OTA_METADATA.equals(name)) {
                    //parse metadata
                    String[] metadata = divideExtractText(zis, (int) ze.getSize());
                    for (String line : metadata) {
                        String[] items = line.split(",");
                        for (String item : items) {
                            if (item.contains(OTA_PAYLOAD_BIN)) {
                                String[] updateData = item.split(":");
                                offset = Integer.parseInt(updateData[1]);
                                fileSize =  Integer.parseInt(updateData[2]);
                            }
                        }
                    }
                }
                if (OTA_PAYLOAD_PROP.equals(name)) {
                    mHeaderKeyValuePairs = divideExtractText(zis, (int) ze.getSize());
                }
            }
            if (mHeaderKeyValuePairs == null) {
                Log.w(TAG, "metadata match failed!");
                mHandler.sendMessage(
                    mHandler.obtainMessage(
                        UPDATE_APPLY_FAILED,
                        UPDATE_APPLY_PACKAGE_FAILED,
                        0,
                        null));
                return;
            }
            mUpdateEngine.applyPayload(new AssetFileDescriptor(pfd, offset, fileSize),
                        mHeaderKeyValuePairs);
        } catch (IOException|ServiceSpecificException e) {
            e.printStackTrace();
            String exceptionInfo = e.getLocalizedMessage();
            if (exceptionInfo.contains("reboot") &&
                exceptionInfo.contains("applied")) {
                mHandler.sendEmptyMessage(UPDATE_APPLY_REBOOT);
            } else {
                mHandler.sendMessage(
                    mHandler.obtainMessage(
                        UPDATE_APPLY_FAILED,
                        UPDATE_APPLY_PACKAGE_FAILED,
                        0,
                        null));
            }
        }
    }

    private int[] divideExtractMetadata(ZipInputStream zis, int size) throws IOException {
        int[] data = new int[2];
        String[] metadata = divideExtractText(zis, size);
        String PAYLOAD_BIN = "payload.bin:";
        for (String line : metadata) {
            String[] items = line.split(",");
            for (String item : items) {
                if (item.contains(PAYLOAD_BIN)) {
                    String[] updateData = item.split(":");
                    data[0] = Integer.parseInt(updateData[1]);
                    data[1] =  Integer.parseInt(updateData[2]);
                }
            }
        }
        return data;
    }

    private String[] divideExtractText(ZipInputStream zis, int size) throws IOException {
        return extractText(zis, size).split("[\n\t ]");
    }

    private String extractText(ZipInputStream zis, int size) throws IOException {
        int count = 0;
        byte[] buffer = new byte[size];
        while (count < buffer.length) {
            count += zis.read(buffer, count, buffer.length - count);
        }
        return new String(buffer, StandardCharsets.UTF_8);
    }

    private class UECallback extends UpdateEngineCallback {
        public void onStatusUpdate(int status, float percent) {
            if (status == UpdateEngine.UpdateStatusConstants.DOWNLOADING) {
                // call back status and ui show percent
                Log.d(TAG, "update progress: " + percent);
                mHandler.sendMessage(mHandler.obtainMessage(
                        UPDATE_PROGRESS_REFRESH,
                        (int) (percent * 100),
                        0,
                        null));
            }
        }

        public void onPayloadApplicationComplete(int errCode) {
            if (debug) Log.e(TAG, "Payload application complete, error: " +
                Integer.toString(errCode));
            if (errCode == 0) {
                if (debug) Log.e(TAG, "Installation succeeded!");
                mHandler.sendEmptyMessage(UPDATE_APPLY_REBOOT);
            }else {
                mHandler.sendMessage(mHandler.obtainMessage(
                        UPDATE_APPLY_FAILED,
                        UPDATE_PROGRESS_REFRESH_FAILED,
                        0,
                        null));
            }
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
        mUpdateEngine.unbind();
        super.onDestroy();
    }
}

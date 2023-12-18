package com.android.abupdater.ui;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Button;
import android.widget.LinearLayout;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.lang.Process;
import android.os.RecoverySystem;
import android.content.IntentSender;

import com.android.abupdater.R;

public class IndexActivity extends Activity implements View.OnClickListener {

    public static final String TAG = "IndexActivity";
    private static final int JSONFILESELECT = 998;
    private static final String FILEPATH = "filePath";
    private String jsonPath = "";

    private ImageView backBtn;
    private TextView filePath;
    private TextView selectTitle;
    private Button enterBtn;
    private Button selectBtn;
    private Button userKnowBtn;
    private Button selectJson;
    private Button selectPackage;
    private LinearLayout runningBar;

    private Context mContext;
    private boolean isJsonSelect = true;
    private int isSelectSuccessful = 0;
    private String OTA_PACKAGE = "/data/ota_package/";
    private String OTA_PACKAGE_PATH = "/data/ota_package/update.zip";
    private String tmpPath = "";

    private static final int INDEX_SHOW_RUNNING = 100;
    private static final int INDEX_HIDE_RUNNING = 101;
    private static final int SELECTFILE = 102;
    private static final int INDEX_SELECT_JSON = 103;
    private static final int INDEX_SELECT_PACKAGE = 104;
    private static final int INDEX_SELECT_NONE = 105;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // init view
        setContentView(R.layout.activity_index);
        mContext = this;
        initView();
        initData();
    }

    private void initView() {
        backBtn = (ImageView) findViewById(R.id.back_icon);
        filePath = (TextView) findViewById(R.id.show_file_path);
        selectTitle = (TextView) findViewById(R.id.select_title);
        selectBtn = (Button) findViewById(R.id.select_btn);
        enterBtn = (Button) findViewById(R.id.enter_btn);
        userKnowBtn = (Button) findViewById(R.id.user_know_btn);
        // selectJson = (Button) findViewById(R.id.select_json_btn);
        selectPackage = (Button) findViewById(R.id.select_package_btn);
        runningBar = (LinearLayout) findViewById(R.id.running_operation);
        runningBar.bringToFront();
        
        backBtn.setOnClickListener(this);
        selectBtn.setOnClickListener(this);
        userKnowBtn.setOnClickListener(this);
        enterBtn.setOnClickListener(this);
        // selectJson.setOnClickListener(this);
        selectPackage.setOnClickListener(this);
    }

    // init data
    private void initData() {
        jsonPath = mContext.getFilesDir().getAbsolutePath() + "/configs/";
    }

    // handler to copy file
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case SELECTFILE:
                    selectFile();
                    break;
                case INDEX_HIDE_RUNNING:
                    runningBar.setVisibility(View.GONE);
                    filePath.setText(tmpPath);
                    switch (isSelectSuccessful) {
                        case INDEX_SELECT_JSON:
                            selectPackage.setVisibility(View.VISIBLE);
                            break;
                        case INDEX_SELECT_PACKAGE:
                            enterBtn.setVisibility(View.VISIBLE);
                            break;
                        default:
                            break;
                    }
                    isSelectSuccessful = INDEX_SELECT_NONE;
                    break;
                case INDEX_SHOW_RUNNING:
                    runningBar.setVisibility(View.VISIBLE);
                    filePath.setText("");
                    break;

            }
        }
    };

    private void selectFile() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                mHandler.sendEmptyMessage(INDEX_SHOW_RUNNING);
                if (isJsonSelect) {
                    if (!copyJson(tmpPath)) {
                        tmpPath = getString(R.string.index_select_error);
                    } else {
                        isJsonSelect = false;
                        isSelectSuccessful = INDEX_SELECT_JSON;
                    }
                } else {
                    if (!copyPackage(tmpPath)) {
                        tmpPath = getString(R.string.index_select_error);
                    } else {
                        isSelectSuccessful = INDEX_SELECT_PACKAGE;
                    }
                }
                mHandler.sendEmptyMessage(INDEX_HIDE_RUNNING);
            }
        }).start();
    }

    private void selectJsonFile() {
        //TODO: if user want to go to select Json file view.
    }

    private void selectPackageFile() {
        selectTitle.setText(R.string.index_update_package_title);
        selectPackage.setVisibility(View.GONE);
        // enterBtn.setVisibility(View.VISIBLE);
        filePath.setText("");
    }

    @Override
    public void onClick(View view) {
        int id = view.getId();
        Intent intent = null;
        switch (id) {
            case R.id.back_icon:
                finish();
                break;
            case R.id.select_btn:
                intent = new Intent(IndexActivity.this, FileSelector.class);
                startActivityForResult(intent, JSONFILESELECT);
                break;
            case R.id.enter_btn:
                intent = new Intent(IndexActivity.this, MainActivity.class);
                startActivity(intent);
                break;
            case R.id.user_know_btn:
                showUseDialog();
                break;
            case R.id.select_json_btn:
                selectJsonFile();
                break;
            case R.id.select_package_btn:
                selectPackageFile();
                break;
        }

    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (data == null || resultCode != RESULT_OK) {
            return;
        }
        switch(requestCode) {
            case JSONFILESELECT:
                String path = data.getStringExtra(FILEPATH);
                // Log.w(TAG, " path: " + path);
                tmpPath = path;
                Message msg = mHandler.obtainMessage();
                msg.what = SELECTFILE;
                mHandler.dispatchMessage(msg);
                break;
        }
    }

    // copy json
    private boolean copyJson(String path) {
        if (path.lastIndexOf(".json") != -1) {
            File toFileConfig = new File(jsonPath);
            if (!toFileConfig.exists()) {
                toFileConfig.mkdir();
            }
            String tmpFilePath = jsonPath + String.valueOf(System.currentTimeMillis()) + ".json";
            if (copyFileTo(path, tmpFilePath)) {
                return true;
            }
        }
        return false;
    }

    // copy update package
    private boolean copyPackage(String path) {
        if (path.lastIndexOf(".zip") != -1) {
            try {
                File file = new File(OTA_PACKAGE);
                if (!(file.exists() && file.canWrite())) {
                    return false;
                }
                File otaPackage = new File(OTA_PACKAGE_PATH);
                if (!otaPackage.exists()) {
                    otaPackage.createNewFile();
                    Process p = Runtime.getRuntime().exec("chmod 777 " +  OTA_PACKAGE_PATH);
                    int status = p.waitFor();
                    // Log.w(TAG,"the file Process status: " + status);
                }
                if (copyFileTo(path, OTA_PACKAGE_PATH)) {
                    return true;
                }
            } catch (IOException e) {
                e.printStackTrace();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        return false;
    }

    private boolean copyFileTo(String fromFilePath, String toFilePath) {
        // Log.w(TAG, " fromFilePath:" + fromFilePath + " toFilePath: " + toFilePath);
        FileInputStream inputStream = null;
        FileOutputStream outputStream = null;
        try {
            File fromFile = new File(fromFilePath);
            File toFile = new File(toFilePath);
            long outSize = 0;
            if (fromFile.exists() && fromFile.canRead()) {
                if (!toFile.exists()) {
                    toFile.createNewFile();
                }
                inputStream = new FileInputStream(fromFile);
                outputStream = new FileOutputStream(toFile);
                int length = -1;
                byte[] buf = new byte[1024];
                while ((length = inputStream.read(buf)) != -1) {
                    outputStream.write(buf, 0, length);
                    outSize += length;
                }
            } else {
                return false;
            }
            outputStream.flush();
            inputStream.close();
            outputStream.close();
        } catch(IOException e) {
            e.printStackTrace();
        }
        return true;
    }

    private void showUseDialog() {
        LayoutInflater inflater = LayoutInflater.from(this);
        View view = inflater.inflate(R.layout.activity_index_dialog, null);
        TextView userNotice = (TextView) view.findViewById(R.id.user_notice);
        userNotice.setText(getString(R.string.index_user_know_content));
        new AlertDialog.Builder(this)
            .setTitle(R.string.index_user_know_title)
            .setView(view)
            .setPositiveButton(getString(R.string.index_confirm), new DialogInterface.OnClickListener(){
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    dialog.dismiss();
                }
            })
            .create()
            .show();
    }

}

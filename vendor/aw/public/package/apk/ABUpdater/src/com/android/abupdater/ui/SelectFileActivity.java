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

public class SelectFileActivity extends Activity implements View.OnClickListener {

    private static final String TAG = "SelectFileActivity";
    private static final boolean debug = true;
    private static final String FILEPATH = "filePath";

    private ImageView backView;
    private TextView filePath;
    private ImageView selectTips;

    private Context mContext;
    private String tmpPath = "";

    private static final int INDEX_SHOW_PATH = 100;
    private static final int INDEX_SELECT_PACKAGE = 101;
    private static final int INDEX_FILE_SELECT = 102;
    private static final int INDEX_FILE_CHECK = 103;
    private static final int FILE_SELECT = 999;
    private static final int UPDATE_APPLY = 998;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // init view
        setContentView(R.layout.select_file_activity);
        mContext = this;
        initView();
        initData();
    }

    private void initView() {
        backView = (ImageView) findViewById(R.id.back);
        filePath = (TextView) findViewById(R.id.file_path);
        //selectTips = (ImageView) findViewById(R.id.select_tips);

        backView.setOnClickListener(this);
        filePath.setOnClickListener(this);
        //selectTips.setOnClickListener(this);
    }

    // init data
    private void initData() {
    }

    // handler to copy file
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case INDEX_FILE_SELECT:
                    //startActivity();
                    //selectFile();
                    break;
                case INDEX_FILE_CHECK:
                    Log.w(TAG, "INDEX_FILE_CHECK");
                    if (!checkPackage(tmpPath)) {
                        tmpPath = getString(R.string.local_update_package_select_error);
                    } else {
                        showUseDialog("",
                                    getString(R.string.local_update_content_info),
                                    getString(R.string.local_update_content_confirm),
                                    () -> {
                                            Intent intent = new Intent(mContext, UpdateMainActivity.class);
                                            Bundle bundle = new Bundle();
                                            bundle.putString(FILEPATH, tmpPath);
                                            intent.putExtras(bundle);
                                            startActivityForResult(intent, UPDATE_APPLY);
                                        },
                                    getString(R.string.local_update_content_cancel),
                                    null
                        );
                    }
                case INDEX_SHOW_PATH:
                    //select();
                    filePath.setText(tmpPath);
                    break;

            }
        }
    };

    private boolean checkPackage(String path) {
        return path.lastIndexOf(".zip") != -1;
    }

    private void clearPackage() {
        filePath.setText("");
    }

    @Override
    public void onClick(View view) {
        int id = view.getId();
        Intent intent = null;
        switch (id) {
            case R.id.back:
                finish();
                break;
            case R.id.file_path:
                intent = new Intent(mContext, FileSelector.class);
                startActivityForResult(intent, FILE_SELECT);
                break;
            /*case R.id.select_tips:
                showUseDialog();
                break;
            */
        }

    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK) {
            if (debug) Log.w(TAG, "onActivityResult error, resultCode is " + resultCode);
            return;
        }
        switch(requestCode) {
            case FILE_SELECT:
                if (data == null) return;
                String path = data.getStringExtra(FILEPATH);
                tmpPath = path;
                Message msg = mHandler.obtainMessage();
                msg.what = INDEX_FILE_CHECK;
                mHandler.dispatchMessage(msg);
                break;
            case UPDATE_APPLY:
                finish();
                break;
        }
    }

    private void showUseDialog(String title, String msg,
                String positiveMsg, PositiveInterface posInterface,
                String negativeMsg, NegativeInterface negInterface) {
        LayoutInflater inflater = LayoutInflater.from(this);
        View view = inflater.inflate(R.layout.dialog_layout, null);
        TextView showMsgTitle = (TextView) view.findViewById(R.id.show_msg_title);
        TextView showMsgContent = (TextView) view.findViewById(R.id.show_msg_content);
        Button showMsgConfirm = (Button) view.findViewById(R.id.show_msg_confirm);
        Button showMsgCancel = (Button) view.findViewById(R.id.show_msg_cancel);

        showMsgTitle.setText(title);
        showMsgContent.setText(msg);
        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this)
            .setView(view)
            .setCancelable(false);
        AlertDialog dialog = dialogBuilder.create();
        showMsgConfirm.setText(positiveMsg);
        showMsgCancel.setText(negativeMsg);
        showMsgConfirm.setOnClickListener(v -> {
            if (posInterface != null) {
                posInterface.positiveFunc();
            }
            dialog.dismiss();
        });
        showMsgCancel.setOnClickListener(v -> {
            if (negInterface != null) {
                negInterface.negativeFunc();
            }
            dialog.dismiss();
        });
        dialog.show();
    }

    @FunctionalInterface
    interface PositiveInterface {
        void positiveFunc();
    }

    @FunctionalInterface
    interface NegativeInterface {
        void negativeFunc();
    }

}

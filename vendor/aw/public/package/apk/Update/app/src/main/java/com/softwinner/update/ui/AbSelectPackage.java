package com.softwinner.update.ui;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.view.LayoutInflater;
import android.view.KeyEvent;
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

import com.softwinner.shared.FileInfo;
import com.softwinner.shared.FileSelector;
import com.softwinner.update.Configs;
import com.softwinner.update.PropertiesHelper;
import com.softwinner.update.R;

import java.io.File;

public class AbSelectPackage extends Activity implements View.OnClickListener {

    private static final String TAG = "AbSelectPackage";
    private static final boolean debug = true;
    public static final String FILEPATH = "filePath";

    private ImageView backView;
    private TextView filePath;
    private ImageView selectTips;

    private Context mContext;

    private boolean mUseSelfSelector = false;

    private static final int INDEX_SHOW_PATH = 100;
    private static final int INDEX_SELECT_PACKAGE = 101;
    private static final int INDEX_FILE_SELECT = 102;
    private static final int INDEX_FILE_CHECK = 103;
    public static final int FILE_SELECT = 999;
    private static final int UPDATE_APPLY = 998;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // init view
        setContentView(R.layout.select_package);
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
        mUseSelfSelector = !PropertiesHelper.get(this, "ro.build.characteristics").contains("tablet");
    }

    // handler to copy file
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case INDEX_FILE_CHECK:
                    Log.w(TAG, "INDEX_FILE_CHECK");
                    FileInfo.FileUnit fu = (FileInfo.FileUnit) msg.obj;
                    showUseDialog("",
                                getString(R.string.local_update_content_info),
                                getString(R.string.local_update_content_confirm),
                                () -> {
                                    Intent intent = new Intent(mContext, AbUpdate.class);
                                    intent.putExtra(Configs.UPDATEPATH, fu);
                                    startActivityForResult(intent, UPDATE_APPLY);
                                },
                                getString(R.string.local_update_content_cancel),
                                null
                    );
                    filePath.setText(fu.getPath());
                    break;

            }
        }
    };

    private void clearPackage() {
        filePath.setText("");
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.KEYCODE_DPAD_UP) return true;
            return super.dispatchKeyEvent(event);
    }

    @Override
    public void onClick(View view) {
        int id = view.getId();
        switch (id) {
            case R.id.back:
                finish();
                break;
            case R.id.file_path:
                Intent intent = null;
                if (!mUseSelfSelector) {
                    if (debug) Log.w(TAG, "File is null, Please retry!");
                    intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("application/zip");
                } else {
                    if (debug) Log.w(TAG, "File is null, Please retry! " + mUseSelfSelector);
                    intent = new Intent(mContext, FileSelector.class);
                    Bundle bundle = new Bundle();
                    bundle.putString(Configs.ABUPDATE, "true");
                    intent.putExtras(bundle);
                }
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
                FileInfo.FileUnit fu;
                if (mUseSelfSelector) {
                    fu = new FileInfo.FileUnit(data.getStringExtra(FILEPATH), FileInfo.TYPE_FILE);
                } else {
                    fu = new FileInfo.FileUnit(data.getData().toString(), FileInfo.TYPE_URI);
                }
                if (fu.getPath() == null) {
                    if (debug) Log.w(TAG, "File is null, Please retry!");
                    return;
                }

                if (debug) Log.w(TAG, "file path is: " + fu.getPath());
                mHandler.dispatchMessage(mHandler.obtainMessage(INDEX_FILE_CHECK, fu));
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

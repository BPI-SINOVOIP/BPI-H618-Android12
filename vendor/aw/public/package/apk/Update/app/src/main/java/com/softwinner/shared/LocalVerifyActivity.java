package com.softwinner.shared;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.softwinner.update.PropertiesHelper;
import com.softwinner.update.R;

import java.io.File;
import java.text.DecimalFormat;


public class LocalVerifyActivity extends Activity implements VerifyPackage.ProgressListener {

    private static final int VERIFY_SUCCESS = 0;
    private static final int VERIFY_FIALED = 1;

    public RelativeLayout backRL, startRL;
    private TextView titleTv, nameTv, sizeTv, startTv, verifyTv;
    private ImageView startImg;
    private VerifyPackage mVerifyPackage;
    private String packagePath;
    private File packageFile;
    private VerifyTask mVerifyTask;
    private MyReceiver receiver=null;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case VERIFY_SUCCESS:
                    startTv.setTextColor(getResources().getColor(R.color.start_btn_enabled_color));
                    startRL.setClickable(true);
                    //verifyTv.setText(R.string.package_verify_success);
                    verifyTv.setText("");
                    startImg.setBackgroundResource(R.drawable.ic_check_press);
                    break;
                case VERIFY_FIALED:
                    verifyTv.setText(R.string.package_verify_failed);
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.local_verify_layout);
        backRL = (RelativeLayout) findViewById(R.id.back_btn);
        startRL = (RelativeLayout) findViewById(R.id.start_install_btn);
        titleTv = (TextView) findViewById(R.id.title_text);
        nameTv = (TextView) findViewById(R.id.package_name);
        sizeTv = (TextView) findViewById(R.id.package_size);
        startTv = (TextView) findViewById(R.id.start_install_txt);
        verifyTv = (TextView) findViewById(R.id.verifying_txt);
        startImg = (ImageView) findViewById(R.id.start_install_img);

        Intent intent = getIntent();
        packagePath = intent.getStringExtra("path");
        packageFile = new File(packagePath);
        titleTv.setText(packageFile.getName());
        nameTv.setText(packageFile.getName());
        sizeTv.setText(sizeToMb(packageFile.length()));
        verifyTv.setText(R.string.verifying_local_txt);
        startTv.setTextColor(getResources().getColor(R.color.start_btn_disable_color));
        startImg.setBackgroundResource(R.drawable.ic_check_nor);
        receiver=new MyReceiver();
        IntentFilter filter=new IntentFilter();
        filter.addAction("com.softwinner.shared.UpdateService");
        registerReceiver(receiver,filter);
        backRL.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                mVerifyTask.cancel(false);
                LocalVerifyActivity.this.finish();
            }
        });

        startRL.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                if(!("tablet".equals(PropertiesHelper.get(LocalVerifyActivity.this, "ro.product.platform")))) {
                    if ("true".equals(PropertiesHelper.get(LocalVerifyActivity.this, "ro.build.ab_update"))) {
                        //执行后台升级
                        showUpdateDialog(LocalVerifyActivity.this, packagePath);
                    } else {
                        Intent intent = new Intent(LocalVerifyActivity.this, LocalRebootActivity.class);
                        intent.putExtra("path", packagePath);
                        LocalVerifyActivity.this.startActivity(intent);
                    }
                }else {
                    InstallPowerDialog dialog = new InstallPowerDialog.Builder(
                            LocalVerifyActivity.this).setClickListener(new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            if (which == 0) {
                                dialog.dismiss();
                            } else if (which == 1) {
                                dialog.dismiss();

                                //判断平台
                                if ("true".equals(PropertiesHelper.get(LocalVerifyActivity.this, "ro.build.ab_update"))) {
                                    //执行后台升级
                                    showUpdateDialog(LocalVerifyActivity.this, packagePath);
                                } else {
                                    Intent intent = new Intent(LocalVerifyActivity.this, LocalRebootActivity.class);
                                    intent.putExtra("path", packagePath);
                                    LocalVerifyActivity.this.startActivity(intent);
                                }
                            }
                        }
                    }).create();
                    dialog.setCanceledOnTouchOutside(false);
                    dialog.show();
                }
            }
        });
        startRL.setClickable(false);

        mVerifyTask = new VerifyTask(this);
        mVerifyTask.execute();
    }
    public void showUpdateDialog( final Context context, final String path){
        /* @setIcon 设置对话框图标
         * @setTitle 设置对话框标题
         * @setMessage 设置对话框消息提示
         * setXXX方法返回Dialog对象，因此可以链式设置属性
         */
        AlertDialog.Builder normalDialog =
                new AlertDialog.Builder(context);
        normalDialog.setTitle(R.string.update_system_txt);
        normalDialog.setMessage(R.string.update_confirm_txt);
        normalDialog.setPositiveButton(R.string.dialog_confirm,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        //...To-do
                        startRL.setClickable(false);
                        startImg.setBackgroundResource(R.drawable.ic_check_nor);
                        Intent updateIntent =new Intent(context, UpdateService.class);
                        updateIntent.putExtra("path", path);
                        context.startService(updateIntent);
                        //Activity activity=(Activity)context;
                       // activity.finish();
                    }
                });
        normalDialog.setNegativeButton(R.string.dialog_cancel,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        //...To-do
                    }
                });
        // 显示
        normalDialog.show();
    }
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    private String sizeToMb(long length) {
        String str = "";
        float size = (float) length / 1024 / 1024;
        str = new DecimalFormat("#.000").format(size) + "Mb";
        return str;
    }

    @Override
    public void onProgress(int progress) {
        if (progress == 100) {
            Message msg = new Message();
            msg.what = VERIFY_SUCCESS;
            mHandler.sendMessage(msg);
        }
    }

    @Override
    public void onVerifyFailed(int errorCode, Object object) {
        Message msg = new Message();
        msg.what = VERIFY_FIALED;
        mHandler.sendMessage(msg);
    }

    @Override
    public void onCopyProgress(int progress) {

    }

    @Override
    public void onCopyFailed(int errorCode, Object object) {

    }

    class VerifyTask extends AsyncTask<Void, Integer, Integer> {
        private Context context;

        VerifyTask(Context context) {
            this.context = context;
        }

        @Override
        protected void onPreExecute() {

        }

        @Override
        protected Integer doInBackground(Void... params) {
            mVerifyPackage = new VerifyPackage(LocalVerifyActivity.this);
            mVerifyPackage.verifyPackage(packageFile, LocalVerifyActivity.this);
            return 0;
        }

        @Override
        protected void onPostExecute(Integer integer) {

        }

        @Override
        protected void onProgressUpdate(Integer... values) {

        }
    }
    public class MyReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            startRL.setClickable(true);
            startImg.setBackgroundResource(R.drawable.ic_check_press);
        }
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(receiver != null){
            unregisterReceiver(receiver);
        }
    }
}

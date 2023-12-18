package com.softwinner.update;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.bigbigcloud.devicehive.download.DownloadManager;
import com.bigbigcloud.devicehive.download.DownloadStatusListener;
import com.bigbigcloud.devicehive.utils.Utils;
import com.softwinner.shared.InstallPowerDialog;
import com.softwinner.shared.OnlineRebootActivity;
import com.softwinner.shared.VerifyPackage;

import org.xutils.common.Callback;

import java.io.File;
import java.text.DecimalFormat;

public class DownloadPackageActivity extends Activity implements
        OnClickListener, VerifyPackage.ProgressListener {

    private static final int GET_SIZE = 0;
    private static final int VERIFY_SUCCESS = 1;
    private static final int DOWNLOAD_COMPLETE = 2;

    private RelativeLayout backRL, startRL, downloadPanel;
    private TextView nameTv, sizeTv, progreesTv, verifyTv, startBtnTv;
    private ImageButton cancelBtn, pauseBtn;
    private ProgressBar downloadPb;

    private Preferences mPrefs;
    private String mUrl;
    private boolean readyToInstall = false;
    private boolean isVerifying = false;
    private VerifyPackage mVerifyPackage;
    private PowerManager.WakeLock wakeLock;
    private VerifyTask mVerifyTask = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.download_layout);

        backRL = (RelativeLayout) findViewById(R.id.back_btn);
        startRL = (RelativeLayout) findViewById(R.id.start_install_btn);
        downloadPanel = (RelativeLayout) findViewById(R.id.download_panel);
        nameTv = (TextView) findViewById(R.id.package_name);
        sizeTv = (TextView) findViewById(R.id.package_size);
        verifyTv = (TextView) findViewById(R.id.verifying_txt);
        startBtnTv = (TextView) findViewById(R.id.pause_start_tv);
        cancelBtn = (ImageButton) findViewById(R.id.download_cancel_btn);
        pauseBtn = (ImageButton) findViewById(R.id.download_pause_start_btn);
        progreesTv = (TextView) findViewById(R.id.download_progress_tv);
        downloadPb = (ProgressBar) findViewById(R.id.download_pb);

        Intent intent = getIntent();
        nameTv.setText(intent.getStringExtra("name"));

        backRL.setOnClickListener(this);
        startRL.setOnClickListener(this);
        cancelBtn.setOnClickListener(this);
        pauseBtn.setOnClickListener(this);

        mPrefs = new Preferences(this);
        mUrl = mPrefs.getDownloadURL();
        sizeTv.setText(sizeToMb(mPrefs.getDownloadSize()));

        requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (wakeLock == null) {
            PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
            wakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, this
                    .getClass().getCanonicalName());
            wakeLock.acquire();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (wakeLock != null) {
            wakeLock.release();
            wakeLock = null;
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case GET_SIZE:
                    break;
                case VERIFY_SUCCESS:
                    readyToInstall = true;
                    isVerifying = false;
                    verifyTv.setText(R.string.package_verify_success);
                    startBtnTv.setText(R.string.install_txt);
                    break;
                case DOWNLOAD_COMPLETE:
                    progreesTv.setText(getString(R.string.Download_succeed));
                    downloadPb.setProgress(100);
                    startBtnTv.setText(R.string.downlaod_install_txt);
                    verifyTv.setText(R.string.verifying_local_txt);
                    isVerifying = true;
                    mVerifyTask = new VerifyTask(DownloadPackageActivity.this);
                    mVerifyTask.execute();
                    break;
                default:
                    break;
            }
        }

    };

    @Override
    public void onClick(View view) {
        if (view == backRL) {
            if (mVerifyTask != null) {
                mVerifyTask.cancel(false);
            }
            DownloadPackageActivity.this.finish();
        } else if (view == startRL) {
            if (mUrl != null) {
                try {
                    DownloadManager.getInstance().startDownload(mUrl, Configs.DOWNLOAD_PATH, true, false, mDownloadStatusListener);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            startRL.setVisibility(View.INVISIBLE);
            startRL.setClickable(false);
            downloadPanel.setVisibility(View.VISIBLE);
        } else if (view == cancelBtn) {
            if (mVerifyTask != null) {
                mVerifyTask.cancel(false);
            }
            DownloadPackageActivity.this.finish();
        } else if (view == pauseBtn) {
            if (isVerifying) {
                return;
            }
            if (readyToInstall) {
                InstallPowerDialog dialog = new InstallPowerDialog.Builder(
                        DownloadPackageActivity.this).setClickListener(new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (which == 0) {
                            dialog.dismiss();
                        } else if (which == 1) {
                            dialog.dismiss();
                            Intent intent = new Intent(DownloadPackageActivity.this, OnlineRebootActivity.class);
                            intent.putExtra("path", mPrefs.getDownloadTarget());
                            DownloadPackageActivity.this.startActivity(intent);
                        }
                    }
                }).create();
                dialog.setCanceledOnTouchOutside(false);
                dialog.show();
                return;
            }
            if (mUrl != null) {
                try {
                    DownloadManager.getInstance().autoPause(mUrl, Configs.DOWNLOAD_PATH, true, false, mDownloadStatusListener);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private String sizeToMb(long length) {
        String str = "";
        float size = (float) length / 1024 / 1024;
        str = new DecimalFormat("#.000").format(size) + "Mb";
        return str;
    }

    private DownloadStatusListener mDownloadStatusListener = new DownloadStatusListener() {

        @Override
        public void onWaiting() {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    startBtnTv.setText(R.string.download_resume_txt);
                }
            });

        }

        @Override
        public void onStarted() {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    startBtnTv.setText(R.string.download_pause_txt);
                }
            });
        }

        @Override
        public void onLoading(long total, long current) {
            final int percent = (int) (100 * current / total);
            mHandler.post(new Runnable() {

                @Override
                public void run() {
                    progreesTv.setText(setStr(mPrefs.getDownloadSize(), percent));
                    downloadPb.setProgress(percent);
                }
            });
        }

        @Override
        public void onSuccess(File result) {
            String md5 = Utils.getFileMD5(result.getAbsolutePath());
            if (md5 != null && mPrefs.getMd5().equalsIgnoreCase(md5)) {
                Message msg = new Message();
                msg.what = DOWNLOAD_COMPLETE;
                mHandler.sendMessage(msg);
            } else {
                progreesTv.setText(R.string.Download_failed);
            }
        }

        @Override
        public void onError(Throwable ex, boolean isOnCallback) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    progreesTv.setText(R.string.download_error);
                }
            });

        }

        @Override
        public void onCancelled(Callback.CancelledException cex) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    startBtnTv.setText(R.string.download_start_txt);
                }
            });

        }
    };

    private String setStr(long size, int status) {
        String str = null;
        long k = (size / 1024);
        if (k < 1024) {
            str = status * k / 100 + "K/" + k + "K";
        } else if (k >= 1024) {
            long m = k / 1024;
            k = k % 1024;
            str = m * status / 100 + "." + status * k / 100 + "Mb/" + m + "." + k + "Mb" + "   " + status + "%";
        }
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
        verifyTv.setText(R.string.package_verify_failed);
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
            mVerifyPackage = new VerifyPackage(DownloadPackageActivity.this);
            mVerifyPackage.verifyPackage(new File(mPrefs.getDownloadTarget()), DownloadPackageActivity.this);
            return 0;
        }

        @Override
        protected void onPostExecute(Integer integer) {

        }

        @Override
        protected void onProgressUpdate(Integer... values) {

        }
    }

}

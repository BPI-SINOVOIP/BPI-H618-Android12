package com.softwinner.shared;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.softwinner.update.R;

public class LocalRebootActivity extends Activity implements OnClickListener {

    private ImageButton laterBtn, rebootBtn;
    private RelativeLayout backRL;
    private TextView autoTv;
    private String packagePath;
    private CountDownTimer mCountDownTimer = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.local_install_layout);
        //mUpdateUtils = new OtaUpgradeUtils(this);

        Intent intent = getIntent();
        packagePath = intent.getStringExtra("path");

        backRL = (RelativeLayout) findViewById(R.id.back_btn);
        laterBtn = (ImageButton) findViewById(R.id.later_install);
        rebootBtn = (ImageButton) findViewById(R.id.reboot_btn);
        autoTv = (TextView) findViewById(R.id.auto_reboot_tv);

        backRL.setOnClickListener(this);
        laterBtn.setOnClickListener(this);
        rebootBtn.setOnClickListener(this);
        autoTv.setText(String.format(
                getResources().getString(R.string.auto_reboot_install_txt), 20));

        if (mCountDownTimer == null) {
            mCountDownTimer = new CountDownTimer(20000, 1000) {

                public void onTick(long millisUntilFinished) {
                    autoTv.setText(String.format(
                            getResources().getString(
                                    R.string.auto_reboot_install_txt),
                            millisUntilFinished / 1000));
                }

                public void onFinish() {
                    autoTv.setText(R.string.now_reboot_install_txt);
                    Intent intent = new Intent(LocalRebootActivity.this, CopyPackageActivity.class);
                    intent.putExtra("path", packagePath);
                    LocalRebootActivity.this.startActivity(intent);
                }
            };
            mCountDownTimer.start();
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    @Override
    public void onClick(View view) {
        if (view == backRL || view == laterBtn) {
            mCountDownTimer.cancel();
            LocalRebootActivity.this.finish();
        } else if (view == rebootBtn) {
            mCountDownTimer.cancel();
            Intent intent = new Intent(LocalRebootActivity.this, CopyPackageActivity.class);
            intent.putExtra("path", packagePath);
            LocalRebootActivity.this.startActivity(intent);
        }
    }

}

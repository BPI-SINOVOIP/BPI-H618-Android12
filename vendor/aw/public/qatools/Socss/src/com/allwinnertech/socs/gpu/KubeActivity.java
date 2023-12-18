package com.allwinnertech.socs.gpu;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;
import android.content.Intent;
import android.os.CountDownTimer;

public class KubeActivity extends Activity {
    private KubeView mKubeView;

    private CountDownTimer mShowTimer = new CountDownTimer(5 * 1000, 5 * 1000) {

        @Override
        public void onTick(long arg0) {

        }

        @Override
        public void onFinish() {
            finishTest();
        }
    };

    private void finishTest() {
        finish();
    }

    @Override
    public void finish() {
        mShowTimer.cancel();
        Intent returnIntend =new Intent();
        setResult(RESULT_OK,returnIntend);
        super.finish();
    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        mKubeView = new KubeView(this);
        setContentView(mKubeView);
        mShowTimer.start();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mKubeView.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mKubeView.onPause();
    }
}

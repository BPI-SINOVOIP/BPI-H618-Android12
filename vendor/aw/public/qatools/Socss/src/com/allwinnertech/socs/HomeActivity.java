package com.allwinnertech.socs;

import android.app.Activity;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Handler;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.util.Log;

public class HomeActivity extends Activity implements OnClickListener {

    private String TAG = "HomeActivity";
    Context mContext;
    private TestsConfig mTestConfig;
    private boolean mTesting = false;
    private boolean mAutoTest = false;
    private Handler mHandler = new Handler();
    private String mKillingPackage;
    private CountDownTimer mCountDownTimer;


    private boolean checkAutoTestConfiged(){
        return mTestConfig.getAutoTest();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_home);
        mContext = (Context) this;
        Button mStart = (Button)this.findViewById(R.id.start);
        Button mConfig = (Button)this.findViewById(R.id.config);
        Button mExit = (Button)this.findViewById(R.id.exit);
        Button mSetpTest = (Button)this.findViewById(R.id.step_test);
        mTestConfig = new TestsConfig();
        mTestConfig.loadAllConfigFromFile(this.getFilesDir() + "/config.json");
        mTestConfig.InitTestCases();
        mTestConfig.DumpAllValue();
        mStart.setOnClickListener(this);
        mConfig.setOnClickListener(this);
        mExit.setOnClickListener(this);
        mSetpTest.setOnClickListener(this);
        if (checkAutoTestConfiged())
            autoTestStart();
    }

    @Override
    public void onClick(View arg0) {
        switch(arg0.getId()){
        case R.id.start:
          autoTestStart();
          break;
        case R.id.config:
          startConfig();
          break;
        case R.id.step_test:
          stepTestStart();
          break;
        case R.id.exit:
          finish();
          break;
        }
    }

    private void startConfig() {
        Intent intent = new Intent();
        intent.setAction("com.allwinnertech.socs.ACTION_CONFIG");
        mContext.startActivity(intent);
    }

    private Runnable mNextTestRunnable = new Runnable() {

        @Override
        public void run() {
            stepTestStart();
        }
    };

    private void autoTestStart() {
        mTestConfig.gTestPosition = 0;
        mAutoTest = true;
        mHandler.post(mNextTestRunnable);
    }

    private void stepTestStart() {
        if (TestsConfig.gTestPosition < TestsConfig.TESTCASES.size()) {
            mTesting = true;
            int pos = TestsConfig.gTestPosition;
            Intent intent = TestsConfig.TESTCASES.get(pos);
            int countDown = intent.getIntExtra("countDown", -1);
            if (countDown > 0) {
                Log.d(TAG, "countDown = " + countDown);
                mKillingPackage = intent.getComponent().getPackageName();
                mCountDownTimer = new CountDownTimer(countDown * 1000, countDown * 1000) {
                    @Override
                    public void onTick(long millisUntilFinished) {
                        // TODO
                    }
                    @Override
                    public void onFinish() {
                        Log.d(TAG, "CountDown time out, stop " + mKillingPackage);
                        TestsConfig.gTestPosition++;
                        mCountDownTimer = null;
                        mTesting = false;
                        SysIntf.runRootCmd("am force-stop " + mKillingPackage);
                        SysIntf.runRootCmd("am start -n com.allwinnertech.socs/.HomeActivity");
                        mKillingPackage = null;
                    }
                };
                mCountDownTimer.start();
            }
            Log.d(TAG, "start intent:" + intent);
            startActivityForResult(intent, pos);
        } else {
            mTesting = false;
            Log.d(TAG, "all test is run");
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // for which our TestCase
        if (requestCode == mTestConfig.gTestPosition) {
            if (resultCode == RESULT_OK) {
                mTestConfig.gTestPosition++;
                if (mAutoTest)
                    mHandler.post(mNextTestRunnable);
                else
                    mTesting = false;
            }
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        // Maybe for which third party app test
        Log.d(TAG, "onNewIntent, maybe auto test now");
        if (!mTesting && mAutoTest) {
            stepTestStart();
        }
    }

}

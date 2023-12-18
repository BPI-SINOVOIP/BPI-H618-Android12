package com.softwinner.handwritten.demo.activity;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.CheckBox;

import com.softwinner.handwritten.demo.R;
import com.softwinner.handwritten.demo.ui.NormalHandWrittenView;
import com.softwinner.handwritten.demo.utils.Utils;

public class NormalHandWrittenActivity extends BaseActivity implements View.OnClickListener {

    private final static String TAG = Utils.TAG;

    private View mBtnExit;
    private View mBtnClear;
    private CheckBox mCbRubber;

    private NormalHandWrittenView mHandWrittenView;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_normal_hand_written);

        initData();
        initView();
    }

    private void initData() {

    }

    private void initView() {
        mBtnExit = findViewById(R.id.btn_exit);
        mBtnClear = findViewById(R.id.btn_clear);
        mCbRubber = (CheckBox) findViewById(R.id.cb_rubber);
        mHandWrittenView = (NormalHandWrittenView) findViewById(R.id.hand_written_view);

        mBtnExit.setOnClickListener(this);
        mBtnClear.setOnClickListener(this);
        mCbRubber.setOnClickListener(this);

        //mHandWrittenView.setLayerType(View.LAYER_TYPE_SOFTWARE, null);

        /*getWindow().peekDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
        );*/
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume ... ");
        super.onResume();
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause ... ");
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy ... ");
        super.onDestroy();

        mHandWrittenView.release();
    }

    @Override
    public void onClick(View view) {
        if (view.equals(mBtnExit)) {
            finish();
        } else if (view.equals(mBtnClear)) {
            mHandWrittenView.clearPanel();
        } else if (view.equals(mCbRubber)) {
            onBtnRubberClick();
        }
    }

    private void onBtnRubberClick() {
        mHandWrittenView.setRubberMode(mCbRubber.isChecked());
    }
}

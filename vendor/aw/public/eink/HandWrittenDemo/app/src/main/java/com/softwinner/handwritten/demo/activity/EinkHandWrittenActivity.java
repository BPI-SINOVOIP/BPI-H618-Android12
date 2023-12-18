package com.softwinner.handwritten.demo.activity;

import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Switch;

import com.softwinner.EinkManager;
import com.softwinner.handwritten.demo.R;
import com.softwinner.handwritten.demo.utils.Utils;
import com.softwinner.handwritten.demo.ui.HandWrittenView;

import java.io.IOException;
import java.io.InputStream;

public class EinkHandWrittenActivity extends BaseActivity implements View.OnClickListener {

    private final static String TAG = Utils.TAG;
    private final static String BACKGROUND_INDEX = "bg_index";
    private final static String SAVE_PATH = "/data/media/0/Pictures/";

    private View mBtnExit;
    private View mBtnClear;
    private View mBtnRefresh;
    private Switch mCbRubber;
    private View mBtnSwitchBg;
    private View mBtnSave;
    private Switch mCbAutoTest;

    private HandWrittenView mHandWrittenView;
    private String[] mAssets;
    private int mIndex;
    private SharedPreferences mSharedPreferences;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate ... ");
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_eink_hand_written);

        initData();
        initView();
    }

    private void initData() {
        try {
            mAssets = this.getAssets().list("");
            mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
            mIndex = mSharedPreferences.getInt(BACKGROUND_INDEX, 0) - 1;
            Log.d(TAG, "initData mIndex=" + mIndex);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void initView() {
        mBtnExit = findViewById(R.id.btn_exit);
        mBtnClear = findViewById(R.id.btn_clear);
        mBtnRefresh = findViewById(R.id.btn_refresh);
        mCbRubber = (Switch) findViewById(R.id.cb_rubber);
        mBtnSwitchBg = findViewById(R.id.btn_switch_bg);
        mBtnSave = findViewById(R.id.btn_save);
        mCbAutoTest = (Switch)findViewById(R.id.cb_autotest);
        mHandWrittenView = (HandWrittenView) findViewById(R.id.hand_written_view);
        mHandWrittenView.setBg(getNextBitmap());

        mBtnExit.setOnClickListener(this);
        mBtnClear.setOnClickListener(this);
        mBtnRefresh.setOnClickListener(this);
        mCbRubber.setOnClickListener(this);
        mCbRubber.setVisibility(View.GONE);
        mBtnSwitchBg.setOnClickListener(this);
        mBtnSave.setOnClickListener(this);
        mCbAutoTest.setOnClickListener(this);
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
    }

    @Override
    public void onClick(View view) {
        if (view.equals(mBtnExit)) {
            finish();
        } else if (view.equals(mBtnClear)) {
            onBtnClearClick();
        } else if (view.equals(mBtnRefresh)) {
            onBtnRefreshClick();
        } else if (view.equals(mCbRubber)) {
            onBtnRubberClick();
        } else if (view.equals(mBtnSwitchBg)) {
            onBtnSwitchBgClick();
        } else if (view.equals(mBtnSave)) {
            onBtnSaveClick();
        } else if (view.equals(mCbAutoTest)) {
            onSwitchAutoTestClick();
        }
    }

    private void onBtnClearClick() {
        EinkManager.clear();
    }

    private void onBtnRefreshClick() {
        EinkManager.forceGlobalRefresh(true);
        //In handwriting mode, after notifying force refresh, need to update once.
        EinkManager.refresh();
    }

    private void onBtnRubberClick() {
        //mHandWrittenView.setRubber(mCbRubber.isChecked());
    }

    private void onBtnSwitchBgClick() {
        mHandWrittenView.setBg(getNextBitmap());
    }

    private void onBtnSaveClick() {
        String name = "Handwritten" + Utils.timestamp2Date(System.currentTimeMillis(), "yyyy_MM_dd_HH_mm_ss");
        EinkManager.save(SAVE_PATH+name+".png", true);
    }

    private void onSwitchAutoTestClick() {
        Log.d(TAG, "onSwitchAutoTestClick " + mCbAutoTest.isChecked());
        if (mCbAutoTest.isChecked()) {
            EinkManager.autoTest(true, 120, 500, 4000);
        } else {
            EinkManager.autoTest(false, 0, 0, 0);
        }
    }

    private Bitmap getNextBitmap() {
        if (mAssets == null) return null;
        int len = mAssets.length;
        Bitmap bitmap=null;
        for (int i = 0; i < len; i++) {
            int index = (mIndex + i + 1) % len;
            Log.d(TAG, "search assets for png: " + mAssets[index]);
            if (mAssets[index].endsWith(".png")) {
                try {
                    InputStream is = this.getAssets().open(mAssets[index]);
                    bitmap = BitmapFactory.decodeStream(is);
                    is.close();
                    mIndex = index;
                    mSharedPreferences.edit().putInt(BACKGROUND_INDEX, mIndex).apply();
                    break;
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return bitmap;
    }
}

package com.softwinner.reader.demo.activity;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import com.softwinner.EinkManager;
import com.softwinner.reader.demo.R;
import com.softwinner.reader.demo.data.TxtReader;
import com.softwinner.reader.demo.ui.TxtPageView;
import com.softwinner.reader.demo.utils.Utils;

import java.util.ArrayList;

public class ReaderActivity extends BaseActivity implements View.OnClickListener,
    TxtPageView.OnPageTurnListener, View.OnAttachStateChangeListener {

    private final static String TAG = Utils.TAG;

    private View mBtnExit;
    private Button mBtnRefreshMode;
    private View mBtnForceRefresh;
    private TextView mTvPage;
    private TxtPageView mTxtPageView;

    private Handler mUIHandler;

    private Uri mDataSource;
    private ArrayList<String> mRefreshModeList;

    private WindowManager mWindowMgr;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        WindowManager.LayoutParams params = getWindow().getAttributes();
        params.privateFlags |= WindowManager.LayoutParams.PRIVATE_FLAG_AW_EINK_READING;
        getWindow().setAttributes(params);
        getWindow().setRefreshMode("GU16");
        setContentView(R.layout.activity_reader);

        initData();
        initView();
        mTvPage.setText("loading txt ... ");
        TxtReader.get().postLoadTxt(mDataSource);

        // TODO: test for DecorView not attach set refresh mode
        //getWindow().setRefreshMode(WindowManager.LayoutParams.EINK_DU_MODE);

        // for switch to back get current refresh mode here
        updateCurrentRefreshModeShow();
    }

    private void initData() {
        Intent intent = getIntent();
        if (null != intent && Intent.ACTION_VIEW.equals(intent.getAction())) {
            // pick from a external txt file
            mDataSource = intent.getData();
        } else {
            // use internal pkg txt file
            mDataSource = null;
        }

        // init change refresh mode
        mRefreshModeList = new ArrayList<String>();
        mRefreshModeList.add("GU16");
        mRefreshModeList.add("GC16");
        mRefreshModeList.add("DU");
        mRefreshModeList.add("A2");

        mWindowMgr = (WindowManager) getSystemService(WINDOW_SERVICE);

        initHandler();
    }

    private void initView() {
        mBtnExit = findViewById(R.id.btn_exit);
        mBtnRefreshMode = (Button) findViewById(R.id.btn_mode);
        mBtnForceRefresh = findViewById(R.id.btn_force_refresh);
        mTvPage = (TextView) findViewById(R.id.tv_page);
        mTxtPageView = (TxtPageView) findViewById(R.id.reader_view);

        mBtnExit.setOnClickListener(this);
        mBtnRefreshMode.setOnClickListener(this);
        mBtnForceRefresh.setOnClickListener(this);
        mTxtPageView.setOnPageTurnListener(this);
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume ... ");
        super.onResume();

        // after setContentView DecorView is already created
        getWindow().peekDecorView().addOnAttachStateChangeListener(this);
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause ... ");
        super.onPause();

        getWindow().peekDecorView().removeOnAttachStateChangeListener(this);
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy ... ");
        super.onDestroy();

        TxtReader.get().removeListener(mUIHandler);
        TxtReader.get().releaseLoadedTxt();

        mTxtPageView.setTxtData(null);
        mTxtPageView.setOnPageTurnListener(null);
    }

    @Override
    public void onViewAttachedToWindow(View view) {
        // TODO: now don't need wait DecorView attach to window
        // if you want config refresh mode you should wait DecorView attach to window
        //getWindow().setRefreshMode(WindowManager.LayoutParams.EINK_DU_MODE);
        //updateCurrentRefreshModeShow();
    }

    @Override
    public void onViewDetachedFromWindow(View view) {
    }

    @Override
    public void onClick(View view) {
        if (view.equals(mBtnExit)) {
            finish();
        } else if (view.equals(mBtnRefreshMode)) {
            switchRefreshMode();
        } else if (view.equals(mBtnForceRefresh)) {
            forceRefresh();
        }
    }

    @Override
    public void onPageTurn(boolean isNext) {
        String pageInfo = "" + (mTxtPageView.getCurrentPage() + 1) + "/" + mTxtPageView.getTotalPage();
        mTvPage.setText(pageInfo);
    }

    private void initHandler() {
        mUIHandler = new Handler(Looper.getMainLooper(), new Handler.Callback() {
            @Override
            public boolean handleMessage(Message msg) {
                switch(msg.what) {
                    case TxtReader.NOTIFY_LOAD_TXT_DONE:
                        onUILoadTxtDone(msg.arg1);
                        return true;
                    case TxtReader.NOTIFY_PARSE_TXT_DONE:
                        onUIParseTxtDone(msg.arg1);
                        return true;
                    default:
                        break;
                }
                return false;
            }
        });

        TxtReader.get().addListener(mUIHandler);
    }

    private void onUILoadTxtDone(int isLoadOk) {
        if (isLoadOk <= 0) {
            return;
        }
        mTxtPageView.setTxtData(TxtReader.get().getLoadedTxt());
        //TxtReader.get().postParseTxt(mTxtPageView);
    }

    private void onUIParseTxtDone(int isParseOk) {
        if (0 == isParseOk) {
            return;
        }
        String strPage = TxtReader.get().nextPage(false);
        mTxtPageView.setText(strPage);
        mTvPage.setText(TxtReader.get().getCurPage() + "/" + TxtReader.get().getTotalPage());
    }

    private void switchRefreshMode() {
        String curMode = getWindow().getRefreshMode();
        int index = mRefreshModeList.indexOf(curMode);
        if (index < 0) {
            index = 0;
        }
        index += 1;
        if (index >= mRefreshModeList.size()) {
            index = 0;
        }
        String nextMode = mRefreshModeList.get(index);
        getWindow().setRefreshMode(nextMode);
        Log.i(TAG, "curMode: " + curMode + " to " + nextMode);
        updateCurrentRefreshModeShow();
    }

    private void updateCurrentRefreshModeShow() {
        String strMode = null;
        try {
            // try-catch for can run in other platform
            strMode = getWindow().getRefreshMode();
        } catch (Error e) {
            Log.e(TAG, "error:", e);
        }
        if (TextUtils.isEmpty(strMode)) {
            strMode = "Unkonw";
        }
        mBtnRefreshMode.setText(strMode);
    }

    private void forceRefresh() {
        // true: for refresh all layer immediately
        EinkManager.forceGlobalRefresh(true);
        //getWindow().forceGlobalRefresh(true);
        //mWindowMgr.forceGlobalRefresh(true);
    }

}

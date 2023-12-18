package com.softwinner.reader.demo.data;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.text.Layout;
import android.text.StaticLayout;
import android.util.Log;
import android.widget.TextView;

import com.softwinner.reader.demo.ReaderApp;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

public class TxtReader {

    public final static String TAG = "txt-reader";

    // we just for test, so don't handle large txt file ... (1Mb)
    private final static int MAX_READ_SIZE = 1024 * 1024;
    private final static String INTERNAL_TXT_FILE = "reader_demo.txt";

    private static final int NOTIFY_BASE = 900;
    public static final int NOTIFY_LOAD_TXT_DONE = NOTIFY_BASE + 1;
    public static final int NOTIFY_PARSE_TXT_DONE = NOTIFY_BASE + 2;

    private static final int MSG_BK_LOAD_TXT = 100;
    private static final int MSG_BK_PARSE_TXT = 101;

    private HandlerThread mBkThread;
    private Handler mBkHandler;
    private List<Handler> mListenerList;

    private Context mCtx;

    private String mStrTxt;
    private int mCurPage;
    private int mPageSize;
    private int mTotalPage;
    private boolean mIsTxtParseDone;

    private StaticLayout mMeasureLayout;

    private static TxtReader sInstance = null;

    // singleton pattern with double lock check
    public static TxtReader get() {
        if (null == sInstance) {
            synchronized (TxtReader.class) {
                if (null == sInstance) {
                    sInstance = new TxtReader(ReaderApp.get());
                }
            }
        }
        return sInstance;
    }

    private TxtReader(Context ctx) {
        mCtx = ctx;
        init();
    }

    public void addListener(Handler listener) {
        if (!mListenerList.contains(listener)) {
            mListenerList.add(listener);
        }
    }

    public void removeListener(Handler listener) {
        mListenerList.remove(listener);
    }

    public void postLoadTxt(Uri txtSource) {
        mIsTxtParseDone = false;

        Message msg = mBkHandler.obtainMessage(MSG_BK_LOAD_TXT, txtSource);
        mBkHandler.sendMessage(msg);
    }

    public void postParseTxt(TextView displayTextView) {
        if (null == mStrTxt) {
            Log.i(TAG, "txt data not loaded, can't parse");
            Message msg = mBkHandler.obtainMessage(NOTIFY_PARSE_TXT_DONE);
            msg.arg1 = 0;
            notifyListener(msg);
            return;
        }

        mIsTxtParseDone = false;

        Message msg = mBkHandler.obtainMessage(MSG_BK_PARSE_TXT);
        msg.obj = displayTextView;
        mBkHandler.sendMessage(msg);
    }

    public void releaseLoadedTxt() {
        mStrTxt = null;
    }

    public String getLoadedTxt() {
        return mStrTxt;
    }

    public String nextPage(boolean isPrev) {
        if (isPrev) {
            mCurPage -= 1;
        } else {
            mCurPage += 1;
        }
        if (mCurPage > mTotalPage) {
            Log.i(TAG, "curPage: " + mCurPage + " is large than total: " + mTotalPage);
            return null;
        } else if (mCurPage < 0) {
            Log.i(TAG, "curPage: " + mCurPage + " is < 0");
            return null;
        }

        int start = mCurPage * mPageSize;
        int end = start + mPageSize;
        if (end > mStrTxt.length()) {
            end = mStrTxt.length();
        }
        return mStrTxt.substring(start, end);
    }

    public int getCurPage() {
        return mCurPage;
    }

    public int getTotalPage() {
        return mTotalPage;
    }

    public boolean isParseDone() {
        return mIsTxtParseDone;
    }

    private void init() {
        mStrTxt = null;
        mListenerList = new ArrayList<Handler>();

        mCurPage = 0;
        mPageSize = 0;
        mIsTxtParseDone = false;

        startWorkThread();
    }

    private void startWorkThread() {
        mBkThread = new HandlerThread("TxtReaderThread");
        mBkThread.start();
        mBkHandler = new Handler(mBkThread.getLooper(), new Handler.Callback() {
            @Override
            public boolean handleMessage(Message msg) {
                switch(msg.what) {
                    case MSG_BK_LOAD_TXT:
                        onBkLoadTxt((Uri)msg.obj);
                        return true;
                    case MSG_BK_PARSE_TXT:
                        onBkParseTxt((TextView)msg.obj);
                        return true;
                    default:
                        break;
                }
                return false;
            }
        });
    }

    private void stopWorkThread() {
        if (null != mBkThread) {
            mBkThread.quitSafely();
            try {
                mBkThread.join();
                mBkThread = null;
                mBkHandler = null;
            } catch (InterruptedException e) {
                Log.e(TAG, "error", e);
            }
        }
    }

    private void onBkLoadTxt(Uri txtSource) {
        boolean ret = false;
        if (null == txtSource) {
            ret = loadTxtFromPkg(mCtx);
        } else {
            ret = loadTxtFromExternal(txtSource, mCtx);
        }

        Message msg = mBkHandler.obtainMessage(NOTIFY_LOAD_TXT_DONE);
        if (ret) {
            msg.arg1 = 1;
        } else {
            msg.arg1 = 0;
        }
        notifyListener(msg);
    }

    private void onBkParseTxt(TextView displayTextView) {
        mCurPage = -1;

        mPageSize = calcPageCharNum(displayTextView);
        if (mPageSize > 0) {
            mTotalPage = mStrTxt.length() / mPageSize;
            mIsTxtParseDone = true;
        } else {
            mIsTxtParseDone = false;
        }

        Message msg = mBkHandler.obtainMessage(NOTIFY_PARSE_TXT_DONE);
        if (mIsTxtParseDone) {
            msg.arg1 = 1;
        } else {
            msg.arg1 = 0;
        }
        notifyListener(msg);
    }

    // TODO: now we just for test, so use one page size to layout whole pages
    private int calcPageCharNum(TextView displayTextView) {
        // TODO: off-screen parse measure has some bug now ... use on-screen measure parse
        String strMeasure = mStrTxt.substring((mStrTxt.length() / 2));
        StaticLayout measureLayout = StaticLayout.Builder.obtain(strMeasure, 0, strMeasure.length(),
            displayTextView.getPaint(), displayTextView.getWidth()).build();

        int topOfLastLine = displayTextView.getHeight() - displayTextView.getPaddingTop()
            - displayTextView.getPaddingBottom() - displayTextView.getLineHeight();
        int lineNum = measureLayout.getLineVisibleEnd(topOfLastLine);
        return measureLayout.getLineEnd(lineNum);
    }

    private void notifyListener(Message msg) {
        for (Handler listener : mListenerList) {
            listener.sendMessage(msg);
        }
    }

    private boolean loadTxtFromPkg(Context ctx) {
        AssetManager assetMgr = ctx.getAssets();
        InputStream is = null;
        try {
            is = assetMgr.open(INTERNAL_TXT_FILE);
        } catch (Exception e) {
            Log.e(TAG, "error:", e);
            return false;
        }
        return loadTxtFromInputStream(is);
    }

    private boolean loadTxtFromExternal(Uri dataSource, Context ctx) {
        ContentResolver cr = ctx.getContentResolver();
        InputStream is = null;
        try {
            is = cr.openInputStream(dataSource);
        } catch (Exception e) {
            Log.e(TAG, "error:", e);
            return false;
        }
        return loadTxtFromInputStream(is);
    }

    private boolean loadTxtFromInputStream(InputStream is) {
        if (null == is) {
            return false;
        }
        try {
            int size = is.available();
            if (size > MAX_READ_SIZE) {
                Log.i(TAG, "txt size: " + size + "bytes is too large, limit it to MAX_READ_SIZE !");
                size = MAX_READ_SIZE;
            }
            byte[] buff = new byte[size];
            is.read(buff, 0, size);
            mStrTxt = new String(buff);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "error:", e);
            return false;
        } finally {
            try {
                is.close();
            } catch (Exception e) {}
        }
    }

}

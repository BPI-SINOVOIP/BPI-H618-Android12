package com.softwinner.reader.demo.ui;

import android.content.Context;
import android.text.Layout;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.TextView;

public class TxtPageView extends TextView  {

    private final static String TAG = "txt-view";

    private int mPageCharSize;
    private int mCurPageIndex;
    private int mTotalPage;
    private int mTotalLength;
    private String mTxtData;
    //private int mTxtPosition;
    //private boolean mIsTxtParseDone;

    private OnPageTurnListener mOnPageTurnListener;

    public interface OnPageTurnListener {
        void onPageTurn(boolean isNext);
    }

    public TxtPageView(Context context) {
        this(context, null);
    }

    public TxtPageView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public TxtPageView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    private void init() {
        mTxtData = null;
        mCurPageIndex = 0;
        mPageCharSize = 0;
        mTotalPage = 0;

        mOnPageTurnListener = null;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            int halfW = getWidth() / 2;
            int x = (int)event.getX();
            // click left page prev, click right page next
            return nextPage((x >= halfW));
        }
        return false;
    }

    public void setTxtData(String txtData) {
        mTxtData = txtData;
        mCurPageIndex = 0;
        mPageCharSize = 0;
        mTotalPage = 0;

        if (null == txtData) {
            setText(null);
            return;
        }

        calcPageCharSize();

        // let nextPage to 'No.0' page
        mCurPageIndex = -1;
        nextPage(true);
    }

    public boolean nextPage(boolean isNext) {
        if (null == mTxtData) {
            return false;
        }

        if (isNext) {
            mCurPageIndex += 1;
        } else {
            mCurPageIndex -= 1;
        }
        if (mCurPageIndex >= mTotalPage) {
            Log.i(TAG, "nextPage: " + mCurPageIndex + " is end with total page: " + mTotalPage);
            mCurPageIndex = mTotalPage - 1;
            return false;
        } else if (mCurPageIndex < 0) {
            Log.i(TAG, "nextPage: " + mCurPageIndex + " is reach first page");
            mCurPageIndex = 0;
            return false;
        }

        int pageStartPos = mCurPageIndex * mPageCharSize;
        int pageEndPos = pageStartPos + mPageCharSize;
        if (pageEndPos >= mTotalLength) pageEndPos = mTotalLength;
        String strPage = mTxtData.substring(pageStartPos, pageEndPos);
        setText(strPage);
        if (null != mOnPageTurnListener) {
            mOnPageTurnListener.onPageTurn(isNext);
        }

        return true;
    }

    public int getCurrentPage() {
        return mCurPageIndex;
    }

    public int getTotalPage() {
        return mTotalPage;
    }

    public void setOnPageTurnListener(OnPageTurnListener pageTurnListener) {
        mOnPageTurnListener = pageTurnListener;
    }

    private int getPageCharSize() {
        Layout measureLayout = getLayout();
        int topOfLastLine = getHeight() - getPaddingTop() - getPaddingBottom() - getLineHeight();
        int pageLineSize = measureLayout.getLineForVertical(topOfLastLine);
        return measureLayout.getLineEnd(pageLineSize);
    }

    // TODO: now we just for test, so use one page size to layout whole pages
    private void calcPageCharSize() {
        int measureLen = 2000;
        int len = mTxtData.length();
        String strMeasure;
        if (len > measureLen) {
            if (len <= measureLen * 2) {
                strMeasure = mTxtData.substring(0, measureLen);
            } else {
                strMeasure = mTxtData.substring(len / 2, len);
            }
        } else {
            strMeasure = mTxtData;
        }

        setText(strMeasure);

        mPageCharSize = getPageCharSize();
        if (mPageCharSize > 680) { // don't know why some text miss
            mPageCharSize = 680;
        }
        mTotalLength = len;
        mTotalPage = len / mPageCharSize;
        if (0 != len % mPageCharSize) {
            mTotalPage += 1;
        }

        Log.d(TAG, "pageSize=" + mPageCharSize + ", total=" + mTotalPage);
    }

}

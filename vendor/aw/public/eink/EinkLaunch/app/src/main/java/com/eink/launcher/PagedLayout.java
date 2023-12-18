package com.eink.launcher;

import com.softwinner.einklaunch.R;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

public class PagedLayout extends ViewGroup implements Paged {

    private int mUnitWidth, mUnitHeight;
    private int mWidthGap, mHeightGap;
    private int mUnitCountX, mUnitCountY;

    private boolean mCenterContent;

    public PagedLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub

        mUnitCountX = mUnitCountY = 1;
        mWidthGap = mHeightGap = -1;

        mUnitWidth = (int) getResources().getDimension(R.dimen.apps_cell_width);
        mUnitHeight = (int) getResources().getDimension(R.dimen.apps_cell_height);
    }

    public PagedLayout(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
        // TODO Auto-generated constructor stub
    }

    public PagedLayout(Context context) {
        this(context, null);
        // TODO Auto-generated constructor stub
    }

    public void setUnitGap(int wGap, int hGap) {
        mWidthGap = wGap;
        mHeightGap = hGap;

        requestLayout();
    }

    public void setUnitDimen(int width, int height) {
        mUnitWidth = width;
        mUnitHeight = height;

        requestLayout();
    }

    public void setUnitCount(int countX, int countY) {
        mUnitCountX = countX;
        mUnitCountY = countY;

        requestLayout();
    }

    public int estimateCellHSpan(int width) {
        //Log.d("debug","padding left : right = " + mPaddingLeft + " " + mPaddingRight);
        int availWidth = width - (mPaddingLeft + mPaddingRight);
        int n = Math.max(1, (availWidth) / (mUnitWidth + mWidthGap));

        return n;
    }

    public int estimateCellVSpan(int height) {
        int availHeight = height - (mPaddingTop + mPaddingBottom);
        int n = Math.max(1, (availHeight) / (mUnitHeight + mHeightGap));

        return n;
    }

    public void calculateCellCount(int width, int height) {
        mUnitCountX = estimateCellHSpan(width);
        mUnitCountY = estimateCellVSpan(height);

        //Log.d("debug", "calculate x y = " + mUnitCountX + " " + mUnitCountY);

        requestLayout();
    }

    public boolean addViewToLayout(View child, int index, int childId,
                                   PagedLayout.LayoutParams params) {
        final PagedLayout.LayoutParams lp = params;
        if (lp.cellX >= 0 && lp.cellX <= (getUnitCountX() - 1) &&
                lp.cellY >= 0 && (lp.cellY <= getUnitCountY() - 1)) {

            if (lp.hSpan < 0) lp.hSpan = getUnitCountX();
            if (lp.vSpan < 0) lp.vSpan = getUnitCountY();

            child.setId(childId);
            addView(child, index, lp);

            return true;
        }

        return false;
    }

    public int getUnitCountX() {
        return mUnitCountX;
    }

    public int getUnitCountY() {
        return mUnitCountY;
    }

    public void enableCenteredContent(boolean enabled) {
        mCenterContent = enabled;
    }

    @Override
    public ViewGroup.LayoutParams generateLayoutParams(AttributeSet attrs) {
        return new PagedLayout.LayoutParams(getContext(), attrs);
    }

    @Override
    protected boolean checkLayoutParams(ViewGroup.LayoutParams p) {
        return p instanceof PagedLayout.LayoutParams;
    }

    @Override
    protected ViewGroup.LayoutParams generateLayoutParams(ViewGroup.LayoutParams p) {
        return new PagedLayout.LayoutParams(p);
    }

    @Override
    public int getPageChildCount() {
        // TODO Auto-generated method stub
        return getChildCount();
    }

    @Override
    public View getChildOnPageAt(int i) {
        // TODO Auto-generated method stub
        return getChildAt(i);
    }

    @Override
    public void removeAllViewsOnPage() {
        // TODO Auto-generated method stub
        removeAllViews();
    }

    @Override
    public void removeViewOnPageAt(int i) {
        // TODO Auto-generated method stub
        removeViewAt(i);
    }

    @Override
    public int indexOfChildOnPage(View v) {
        // TODO Auto-generated method stub
        return indexOfChild(v);
    }

    void destroyHardwareLayer() {
        setLayerType(LAYER_TYPE_NONE, null);
    }

    void createHardwareLayer() {
        setLayerType(LAYER_TYPE_HARDWARE, null);
    }

    @Override
    protected void setChildrenDrawingCacheEnabled(boolean enabled) {
        final int count = getChildCount();
        for (int i = 0; i < count; i++) {
            final View view = getChildAt(i);
            view.setDrawingCacheEnabled(enabled);
            // Update the drawing caches
            if (!view.isHardwareAccelerated()) {
                view.buildDrawingCache(true);
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        boolean result = super.onTouchEvent(event);
        int count = getPageChildCount();
        if (count > 0) {
            // We only intercept the touch if we are tapping in empty space after the final row
            View child = getChildOnPageAt(count - 1);
            int bottom = child.getBottom();
            int numRows = (int) Math.ceil((float) getPageChildCount() / getUnitCountX());
            if (numRows < getUnitCountY()) {
                // Add a little bit of buffer if there is room for another row
                bottom += mUnitHeight / 2;
            }
            result = result || (event.getY() < bottom);
        }
        return result;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // TODO Auto-generated method stub
        int widthSpecMode = MeasureSpec.getMode(widthMeasureSpec);
        int widthSpecSize = MeasureSpec.getSize(widthMeasureSpec);

        int heightSpecMode = MeasureSpec.getMode(heightMeasureSpec);
        int heightSpecSize = MeasureSpec.getSize(heightMeasureSpec);

        if (widthSpecMode == MeasureSpec.UNSPECIFIED || heightSpecMode == MeasureSpec.UNSPECIFIED) {
            throw new RuntimeException("CellLayout cannot have UNSPECIFIED dimensions");
        }

        final int count = getChildCount();
        for (int i = 0; i < count; i++) {
            View child = getChildAt(i);
            PagedLayout.LayoutParams lp = (LayoutParams) child.getLayoutParams();
            lp.setup(mUnitWidth, mUnitHeight, mWidthGap, mHeightGap,
                    getPaddingLeft(), getPaddingTop());

            int childWidthMeasureSpec = MeasureSpec.makeMeasureSpec(lp.width,
                    MeasureSpec.EXACTLY);
            int childheightMeasureSpec = MeasureSpec.makeMeasureSpec(lp.height,
                    MeasureSpec.EXACTLY);

            child.measure(childWidthMeasureSpec, childheightMeasureSpec);
        }

        setMeasuredDimension(widthSpecSize, heightSpecSize);
    }

    private int offsetX = 0, offsetY = 0;

    public int getOffsetX() {
        return offsetX;
    }

    public int getOffsetY() {
        return offsetY;
    }

    private void calculateCenterOffset() {
        int count = getChildCount();

        int minCount = (mUnitCountX * mUnitCountX) - mUnitCountX;
        boolean isCenterY = count > minCount;

        ViewGroup parent = (ViewGroup) getParent();
        int index = parent.indexOfChild(this);
        index -= 1;
        if (index >= 0) {
            PagedLayout prev = (PagedLayout) parent.getChildAt(index);
            if (prev != null) {
                offsetX = prev.getOffsetX();
                offsetY = prev.getOffsetY();
            }
        } else if (mCenterContent && count > 0) {
            // determine the max width of all the rows and center accordingly
            int maxRowX = 0;
            int minRowX = Integer.MAX_VALUE;

            int maxRowY = 0;
            int minRowY = Integer.MAX_VALUE;

            for (int i = 0; i < count; i++) {
                View child = getChildAt(i);
                if (child.getVisibility() != GONE) {
                    PagedLayout.LayoutParams lp =
                            (PagedLayout.LayoutParams) child.getLayoutParams();
                    minRowX = Math.min(minRowX, lp.x);
                    maxRowX = Math.max(maxRowX, lp.x + lp.width);

                    minRowY = Math.min(minRowY, lp.y);
                    maxRowY = Math.max(maxRowY, lp.y + lp.height);
                }
            }
            int maxRowWidth = maxRowX - minRowX;
            int maxRowHeight = maxRowY - minRowY;

            offsetX = (getMeasuredWidth() - maxRowWidth) / 2 - getPaddingLeft();
            offsetY = (getMeasuredHeight() - maxRowHeight) / 2 - getPaddingTop();
            if (!isCenterY) {
                offsetY = 0;
            }
        }
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        // TODO Auto-generated method stub
        int count = getChildCount();

        calculateCenterOffset();

        for (int i = 0; i < count; i++) {
            View child = getChildAt(i);
            if (child.getVisibility() != GONE) {
                PagedLayout.LayoutParams lp =
                        (PagedLayout.LayoutParams) child.getLayoutParams();

                int childLeft = offsetX + lp.x;
                int childTop = offsetY + lp.y;
                child.layout(childLeft, childTop, childLeft + lp.width, childTop + lp.height);
            }
        }
    }

    public static class LayoutParams extends ViewGroup.MarginLayoutParams {
        public int cellX;
        public int cellY;
        public int hSpan;
        public int vSpan;
        private Object mTag;

        int x, y;

        public LayoutParams() {
            super(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
            vSpan = hSpan = 1;
        }

        public LayoutParams(Context arg0, AttributeSet arg1) {
            super(arg0, arg1);
            // TODO Auto-generated constructor stub
            vSpan = hSpan = 1;
        }

        public LayoutParams(int arg0, int arg1) {
            super(arg0, arg1);
            // TODO Auto-generated constructor stub
            vSpan = hSpan = 1;
        }

        public LayoutParams(LayoutParams arg0) {
            super(arg0);
            // TODO Auto-generated constructor stub
            this.cellX = arg0.cellX;
            this.cellY = arg0.cellY;
            this.hSpan = arg0.hSpan;
            this.vSpan = arg0.vSpan;
        }

        public LayoutParams(ViewGroup.LayoutParams arg0) {
            super(arg0);
            this.hSpan = this.vSpan = 1;
        }

        public LayoutParams(int cellX, int cellY, int vSpan, int hSpan) {
            super(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);

            this.cellX = cellX;
            this.cellY = cellY;
            this.vSpan = vSpan;
            this.hSpan = hSpan;
        }

        public void setup(int cellWidth, int cellHeight, int widthGap, int heightGap,
                          int hSpadding, int vSpadding) {
            final int mCellHSpan = hSpan;
            final int mCellVSpan = vSpan;
            final int mCellX = cellX;
            final int mCellY = cellY;

            width = mCellHSpan * cellWidth + (mCellHSpan - 1) * widthGap
                    - leftMargin - rightMargin;
            height = mCellVSpan * cellHeight + (mCellVSpan - 1) * heightGap
                    - topMargin - bottomMargin;

            x = hSpadding + mCellX * (cellWidth + widthGap) + leftMargin;
            y = vSpadding + mCellY * (cellHeight + heightGap) + topMargin;
        }

        public Object getTag() {
            return mTag;
        }

        public void setTag(Object tag) {
            mTag = tag;
        }

        public String toString() {
            return "(" + this.cellX + ", " + this.cellY + ", " +
                    this.hSpan + ", " + this.vSpan + ")";
        }
    }
}

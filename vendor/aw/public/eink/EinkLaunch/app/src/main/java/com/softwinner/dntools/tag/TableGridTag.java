package com.softwinner.dntools.tag;

import android.graphics.Color;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.FrameLayout;
import android.widget.TableLayout;
import android.widget.TableLayout.LayoutParams;
import android.widget.TableRow;
import android.widget.TextView;

public class TableGridTag {

    protected int columnCount = 1;
    protected int rowCount = 1;
    protected int currentPageIndex = 1;

    private TableLayout mTableLayout;

    protected FrameLayout[] childViews;        //每个child view是表格的一个单元格

    protected TagCallBack callBack;


    public TableGridTag(TableLayout layout) {
        mTableLayout = layout;
    }

    public TableGridTag(TableLayout layout, int column, int row) {
        mTableLayout = layout;
        this.columnCount = column;
        this.rowCount = row;
        initLayout();
    }

    protected void initLayout() {
        childViews = new FrameLayout[columnCount * rowCount];
        for (int i = 0; i < rowCount; i++) {

            //新增一列
            TableRow tableRow = new TableRow(mTableLayout.getContext());
            LayoutParams params = new LayoutParams(LayoutParams.MATCH_PARENT, 200);
            mTableLayout.addView(tableRow, params);

            //添加分割线
            TextView halving = new TextView(mTableLayout.getContext());
            halving.setBackgroundColor(Color.BLACK);
            LayoutParams halvingParams = new LayoutParams(LayoutParams.MATCH_PARENT, 1);
            halvingParams.leftMargin = 10;
            halvingParams.rightMargin = 10;
            mTableLayout.addView(halving, halvingParams);

            //
            for (int j = 0; j < columnCount; j++) {
                //新增一个单元格
                final int position = i * columnCount + j;
                FrameLayout child = new FrameLayout(mTableLayout.getContext());
                TableRow.LayoutParams childParams = new TableRow.LayoutParams(TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);
                childParams.weight = 1;
                childViews[position] = child;
                tableRow.addView(child, childParams);
                child.setOnClickListener(new OnClickListener() {

                    public void onClick(View arg0) {
                        if (callBack != null) {
                            callBack.onItemClick(arg0, position);
                        }
                    }

                });
            }
        }
    }

    /**
     * 刷新表格显示的数据
     */
    public void refesh() {
        if (childViews == null || callBack == null) {
            return;
        }

        int availableSize = childViews.length;

        //如果是最后一页
        if (currentPageIndex == getPageCount() && (callBack.getCount() % getPageCount() != 0)) {
            availableSize = callBack.getCount() % getPageCount();
        }

        Log.i("table_grid", "refesh " + availableSize + " items");
        for (int i = 0; i < childViews.length; i++) {
            childViews[i].removeAllViews();
            int dataIndex = i + (currentPageIndex - 1) * this.columnCount * this.rowCount;
            if (dataIndex >= callBack.getCount()) {
                continue;
            }
            View v = callBack.getView(dataIndex);
            if (v != null) {
                Log.i("table_grid", "add view " + i);
                childViews[i].addView(v);
            }
        }
    }

    public void pre() {
        if (currentPageIndex <= 1) {
            return;
        }
        currentPageIndex--;
        refesh();
    }

    public void next() {
        if (currentPageIndex >= getPageCount()) {
            return;
        }
        currentPageIndex++;
        refesh();
    }

    public int getCurrentPage() {
        return this.currentPageIndex;
    }

    public int getColumn() {
        return columnCount;
    }

    public int getRow() {
        return rowCount;
    }

    public String getPageInfo() {
        return this.currentPageIndex + "/" + getPageCount();
    }

    public int getPageCount() {
        int itemCountOnPage = this.columnCount * this.rowCount;

        //如果最后一页正好填满
        if (callBack.getCount() % itemCountOnPage == 0) {
            return callBack.getCount() / itemCountOnPage;
        } else if (callBack.getCount() % itemCountOnPage < 1) {
            return 1;
        }

        //如果最后一页有空余
        return callBack.getCount() / itemCountOnPage + 1;
    }


    public void setCallBack(TagCallBack listener) {
        callBack = listener;
        refesh();
    }

    /**
     * CallBack， 将数据显示到view上， 并设置OnItem点击事件
     */
    public interface TagCallBack {
        public void onItemClick(View v, int position);

        public int getCount();

        public View getView(int position);
    }
}

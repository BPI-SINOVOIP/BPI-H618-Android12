package com.softwinner.TvdVideo;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

import java.util.ArrayList;
import java.util.List;

public class SinglechoiceListAdapter extends BaseAdapter {
    private int mSelectPosition = 0;
    private int mSelectIcon = 0;
    private Context mContext = null;
    // item list
    private List<String> mItems = new ArrayList<String>();

    public SinglechoiceListAdapter(Context context) {
        mContext = context;
    }

    // set the list items
    public void setListItems(List<String> lit) {
        mItems = lit;
    }

    // get item count
    public int getCount() {
        return mItems.size();
    }

    // get one item
    public Object getItem(int position) {
        return mItems.get(position);
    }

    // if can all selected or not
    public boolean areAllItemsSelectable() {
        return false;
    }

    // get item id
    public long getItemId(int position) {
        return position;
    }

    // set secected position
    // public void setSelectPosition(int position) { mSelectPosition = position;
    // }
    // get selected position
    public int getSelectPosition() {
        return mSelectPosition;
    }

    // rewrite getview
    public void setSelectIcon(int id) {
        mSelectIcon = id;
    }

    public void notifyDataSetChanged(int position) {
        // TODO Auto-generated method stub
        mSelectPosition = position;
        super.notifyDataSetChanged();
    }

    public View getView(int position, View convertView, ViewGroup parent) {

        IconifiedTextView btv;
        if (convertView == null) {
            if (position == mSelectPosition && mSelectIcon != 0) {
                btv = new IconifiedTextView(mContext, mSelectIcon, mItems.get(position));
            } else {
                btv = new IconifiedTextView(mContext, R.drawable.unselected, mItems.get(position));
            }
        } else {
            btv = (IconifiedTextView) convertView;
            btv.setText(mItems.get(position));
            if (position == mSelectPosition && mSelectIcon != 0) {
                btv.setIcon(mContext.getResources().getDrawable(mSelectIcon));
            } else {
                btv.setIcon(mContext.getResources().getDrawable(R.drawable.unselected));
            }
        }

        return btv;
    }
}

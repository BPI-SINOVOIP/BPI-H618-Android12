package com.allwinner.camera.views;

import android.annotation.SuppressLint;
import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.TextView;

import com.allwinner.camera.R;

import java.util.List;

public class CardRvAdapter extends RecyclerView.Adapter<CardRvAdapter.ItemViewHolder> implements View.OnClickListener{
    private List<Integer> mList;
    private Context mContext;
    private int[] mTextArray;
    private int[] mTextColorArray;


    private OnItemClickListener mOnItemClickListener = null;
    private ItemViewHolder mHolder;
    private int mPosition = -5;

    public int getSelectedPosition() {
        return mPosition;
    }

    public void setSelectedPosition(int position) {
        mPosition = position;
    }

    public static interface OnItemClickListener {
        void onItemClick(View view , int position);
    }

    public CardRvAdapter(Context context, List<Integer> list,int[] textArray,int[] textColorArray) {
        this.mContext = context;
        this.mList = list;
        this.mTextArray = textArray;
        this.mTextColorArray = textColorArray;

    }
    public void setOnItemClickListener(OnItemClickListener listener) {
        this.mOnItemClickListener = listener;
    }

    @Override
    public void onClick(View v) {
        if (mOnItemClickListener != null) {
            //注意这里使用getTag方法获取position

            mOnItemClickListener.onItemClick(v,(int)v.getTag());
        }
    }
    @Override
    public ItemViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View itemView = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_card_view, parent, false);
        itemView.setOnClickListener(this);
        return new ItemViewHolder(itemView);
    }

    @SuppressLint("ResourceAsColor")
    @Override
    public void onBindViewHolder(final ItemViewHolder holder, int position) {
       // holder.iv.setText(mDatas.get(position));
        holder.iv.setImageResource(mList.get(position));
        holder.tv.setBackgroundResource(mTextColorArray[position]);
        holder.tv.setText(mTextArray[position]);
        if(position == mPosition ) {
            holder.ivBorder.setVisibility(View.VISIBLE);
        }else{
            holder.ivBorder.setVisibility(View.GONE);
        }
        holder.itemView.setTag(position);
    }

    @Override
    public int getItemCount() {
        return mList.size();
    }

    class ItemViewHolder extends RecyclerView.ViewHolder {
        private ImageView iv;
        private ImageView ivBorder;
        private TextView tv;

        public ItemViewHolder(View itemView) {
            super(itemView);
            iv = (ImageView) itemView.findViewById(R.id.imageView);
            ivBorder = (ImageView) itemView.findViewById(R.id.iv_border);
            tv =(TextView) itemView.findViewById(R.id.tv_filter_type);
        }
    }
}

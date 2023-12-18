package com.allwinnertech.dragonsn.view;


import android.content.Context;
import android.graphics.Color;
import android.text.InputType;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.TextView;

import java.util.List;

import com.allwinnertech.dragonsn.BurnManager;
import com.allwinnertech.dragonsn.R;
import com.allwinnertech.dragonsn.entity.BindedColume;

public class VerifyListAdatper extends BaseAdapter {
    Context mContext;
    private List<BindedColume> mBindedColume;
    private BurnManager mBurnManager;

    public VerifyListAdatper(Context context, BurnManager burnManager) {
        mContext = context;
        mBurnManager = burnManager;
        mBindedColume = burnManager.getBindedColumes();
    }

    @Override
        public int getCount() {
            return mBindedColume.size();
        }

    @Override
        public Object getItem(int position) {
            return mBindedColume.get(position);
        }

    @Override
        public long getItemId(int position) {
            return position;
        }

    class ViewHolder {
        TextView columeTitle;
        EditText localData;
        TextView remoteData;
        TextView resultkeyTitle;
    }

    @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = View.inflate(mContext, R.layout.verify_list_item, null);
                ViewHolder holder = new ViewHolder();
                holder.columeTitle = (TextView) convertView.findViewById(R.id.colume_title);
                holder.localData = (EditText) convertView.findViewById(R.id.local_data);
                holder.remoteData = (TextView) convertView.findViewById(R.id.remote_data);
                holder.resultkeyTitle = (TextView) convertView.findViewById(R.id.result_key_title);
                holder.localData.setInputType(InputType.TYPE_NULL);
                convertView.setTag(holder);
            }
            ViewHolder holder = (ViewHolder) convertView.getTag();

            BindedColume bindedColume = mBindedColume.get(position);
            holder.columeTitle.setText(bindedColume.getColName());
            holder.localData.setText(bindedColume.getLocalData().trim());
            holder.remoteData.setText(bindedColume.getRemoteData().trim());

            if (bindedColume.isResultKey()) {
                holder.resultkeyTitle.setVisibility(View.VISIBLE);
            } else {
                holder.resultkeyTitle.setVisibility(View.GONE);
            }

            if (bindedColume.isAllValid()) {
                convertView.setBackgroundColor(Color.GREEN);
            } else {
                convertView.setBackgroundColor(Color.RED);
            }

            return convertView;
        }


}

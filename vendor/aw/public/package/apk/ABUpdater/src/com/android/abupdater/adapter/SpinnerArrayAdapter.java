package com.android.abupdater.adapter;

import java.util.List;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import android.widget.ImageView;

import com.android.abupdater.R;
import com.android.abupdater.UpdateConfig;
import com.android.abupdater.util.UpdateConfigs;



public class SpinnerArrayAdapter extends BaseAdapter {
    private List<String> mList;
    private Context mContext;
    private ConfigsController configsController;

    public SpinnerArrayAdapter(Context context, List<String> list) {
        mContext = context;
        mList = list;
    }

    @Override
    public int getCount() {
        return mList.size();
    }

    @Override
    public Object getItem(int position) {
        return mList.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View view, ViewGroup viewGroup) {
        LayoutInflater layoutInflater = LayoutInflater.from(mContext);
        view = layoutInflater.inflate(R.layout.spinner_item, null);
        if(view != null) {
            TextView itemName = (TextView) view.findViewById(R.id.item_name);
            ImageView itemDelete = (ImageView) view.findViewById(R.id.item_delete);
            itemName.setText(mList.get(position));
            itemDelete.setOnClickListener(new OnClickListener() {
                public void onClick(View arg0) {
                    configsController.deleteConfig(position);
                    mList.remove(position);
                    notifyDataSetChanged();
                }
            });
        }
        return view;
    }

    public interface ConfigsController {
        public void deleteConfig(int position);
    }

    public void setConfigsController(ConfigsController mConfigsController) {
        configsController = mConfigsController;
    }
}


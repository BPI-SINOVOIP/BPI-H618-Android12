package com.softwinner.dragonbox.view;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.WifiConnInfo;

public class WifiScanListAdapter extends BaseAdapter{


    List<WifiConnInfo> mWifiConnInfos = new ArrayList<WifiConnInfo>();
    private LayoutInflater mInflater;
    public WifiScanListAdapter(Context context) {
        mInflater = LayoutInflater.from(context);
    }
    public void setWifiConnInfos(List<WifiConnInfo> wifiConnInfos) {
        mWifiConnInfos = wifiConnInfos;
        notifyDataSetChanged();
    }

    @Override
    public int getCount() {
        return mWifiConnInfos.size();
    }

    @Override
    public WifiConnInfo getItem(int position) {
        return mWifiConnInfos.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convert, ViewGroup root) {
        if (convert == null) {
            convert = mInflater.inflate(R.layout.case_wifi_item, null);
            ViewHolder holder = new ViewHolder();
            holder.ssid = (TextView) convert.findViewById(R.id.case_wifi_name);
            holder.strength = (TextView) convert
                    .findViewById(R.id.case_wifi_strength);
            convert.setTag(holder);
        }
        ViewHolder holder = (ViewHolder) convert.getTag();
        holder.ssid.setText(getItem(position).ssid);
        holder.strength.setText(getItem(position).RSSI + "DB");
        return convert;
    }

    private class ViewHolder {
        TextView ssid;
        TextView strength;
    }

}

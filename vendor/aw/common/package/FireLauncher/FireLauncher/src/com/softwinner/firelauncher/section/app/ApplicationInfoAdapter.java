package com.softwinner.firelauncher.section.app;

import java.util.ArrayList;
import com.softwinner.firelauncher.R;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

public class ApplicationInfoAdapter extends BaseAdapter {

	private Context context;
	private ArrayList<AppBean> mAppBeans;
	public void setLauncher(ArrayList<AppBean> infos) {
		if (infos != null) {
			this.mAppBeans = infos;
		} else {
			this.mAppBeans = new ArrayList<AppBean>();
		}
	}

	public ApplicationInfoAdapter(Context context, ArrayList<AppBean> infos) {
		super();
		this.context = context;
		this.setLauncher(infos);
		context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	}

	public void changeData(ArrayList<AppBean> dateList) {
		this.mAppBeans = dateList;
		notifyDataSetChanged();
	}

	@Override
	public int getCount() {
		return 14;

	}

	@Override
	public Object getItem(int position) {
		return mAppBeans.get(position);
	}

	@Override
	public long getItemId(int position) {
		return position;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		ViewHolder holder = null;
		AppBean info = mAppBeans.get(position);
		if (convertView == null) {
			holder = new ViewHolder();
			convertView = LayoutInflater.from(context).inflate(R.layout.app_section_item, null);
			holder.appIcon = (ImageView) convertView.findViewById(R.id.app_icon);
			holder.appName = (TextView) convertView.findViewById(R.id.app_name);
			convertView.setTag(holder);			
		} else {
			holder = (ViewHolder) convertView.getTag();
		}
		if (info != null) {
			holder.appIcon.setImageDrawable(info.getIcon());
			holder.appName.setText(info.getName());
		} else {
			holder.appIcon.setImageResource(R.drawable.add_apps);
			holder.appName.setText("（未添加）");
		}

		return convertView;
	}

	private class ViewHolder {
		private ImageView appIcon;
		private TextView appName;
	}

}

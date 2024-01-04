package com.softwinner.firelauncher.section.app;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import com.softwinner.firelauncher.section.app.AppBean;

public class AppListBiz {
	private Context context;

	public AppListBiz(Context context) {
		super();
		this.context = context;
	}

	public ArrayList<AppBean> getLauncherApps() {
		PackageManager pm = context.getPackageManager();
		ArrayList<AppBean> launchers = null;
		Intent intent = new Intent(Intent.ACTION_MAIN);
		intent.addCategory(Intent.CATEGORY_LAUNCHER);
		List<ResolveInfo> infos = pm.queryIntentActivities(intent,
				PackageManager.GET_INTENT_FILTERS);
		if (infos != null) {
			launchers = new ArrayList<AppBean>();
			for (ResolveInfo info : infos) {
				AppBean launcher = new AppBean();
				launcher.setIcon(info.activityInfo.loadIcon(pm));
				launcher.setName(info.activityInfo.loadLabel(pm).toString());
				launcher.setPackageName(info.activityInfo.packageName);
				launcher.setDataDir(info.activityInfo.applicationInfo.publicSourceDir);
				System.out.println(launcher.toString());
				launchers.add(launcher);
			}  
		}
		return launchers;
	}

	public ArrayList<AppBean> getUserApps() {
		PackageManager pm = context.getPackageManager();
		ArrayList<AppBean> launchers = null;
		Intent intent = new Intent(Intent.ACTION_MAIN);
		intent.addCategory(Intent.CATEGORY_LAUNCHER);
		List<ResolveInfo> infos = pm.queryIntentActivities(intent,
				PackageManager.GET_INTENT_FILTERS);
		if (infos != null) {
			launchers = new ArrayList<AppBean>();
			for (ResolveInfo info : infos) {
				String pkg = info.activityInfo.packageName;
				if (!isSystem(pkg)) {
					AppBean launcher = new AppBean();
					launcher.setIcon(info.activityInfo.loadIcon(pm));
					launcher.setName(info.activityInfo.loadLabel(pm).toString());
					launcher.setPackageName(info.activityInfo.packageName);
					launcher.setDataDir(info.activityInfo.applicationInfo.publicSourceDir);
					launchers.add(launcher);
				}
			}
		}
		return launchers;
	}

	private boolean isSystem(String pkg) {
		if (pkg.contains("com.android.")) {
			return true;
		} else if (pkg.contains("com.google.")) {
			return true;
		} else {
			return false;
		}
	}
}

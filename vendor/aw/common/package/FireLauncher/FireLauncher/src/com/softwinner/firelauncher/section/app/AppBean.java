package com.softwinner.firelauncher.section.app;

import android.graphics.drawable.Drawable;

public class AppBean {

	private String packageName;
	private int position;
	private Drawable icon;
	private int pageIndex;
	private String name;
	private String id;
	private String dataDir;

	public String getDataDir() {
		return dataDir;
	}

	public void setDataDir(String dataDir) {
		this.dataDir = dataDir;
	}

	public Drawable getIcon() {
		return icon;
	}

	public void setIcon(Drawable icon) {
		this.icon = icon;
	}

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public String getId() {
		return id;
	}

	public void setId(String id) {
		this.id = id;
	}

	public String getPackageName() {
		return packageName;
	}

	public void setPackageName(String packageName) {
		this.packageName = packageName;
	}

	public int getPosition() {
		return position;
	}

	public void setPosition(int position) {
		this.position = position;
	}

	public int getPageIndex() {
		return pageIndex;
	}

	public void setPageIndex(int pageIndex) {
		this.pageIndex = pageIndex;
	}

	@Override
	public String toString() {
		return "AppBean [packageName=" + packageName + ", name=" + name
				+ ", dataDir=" + dataDir + "]";
	}

}

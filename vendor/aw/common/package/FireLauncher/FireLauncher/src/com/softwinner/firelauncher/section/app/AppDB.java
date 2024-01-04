package com.softwinner.firelauncher.section.app;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.HashMap;


import android.annotation.SuppressLint;
import android.content.ContentValues;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.Log;


public class AppDB extends SQLiteOpenHelper {
	private Context context;
	private static final String DBNAME = "app.db";
	private static final int VERSION = 2;
	private static final String TABLE_POSITION = "user_app";
	private static final String PACKAGENAME = "packageName";
	private static final String PAGEINDEX = "pageIndex";
	private static final String INDEX_IN_PAGE = "index_in_dex";
	private static final String APP_IMAGE = "app_image";
	private static final String _ID = "_id";
	private static final String NAME = "name";
	private static final String DATA_DIR = "data_dir";

	public AppDB(Context context) {
		super(context, DBNAME, null, VERSION);
		this.context = context;
	}

	@Override
	public void onCreate(SQLiteDatabase db) {
		Log.w("AppDB", "onCreate");
		String sql = "create table if not exists " + TABLE_POSITION + " ( "
				+ _ID + " INTEGER primary key autoincrement," + PACKAGENAME
				+ " text not null," + APP_IMAGE + " blob not null," + DATA_DIR
				+ " text not null," + NAME + " text not null," + PAGEINDEX
				+ " INTEGER not null," + INDEX_IN_PAGE + " INTEGER not null)";
		db.execSQL(sql);
		// initDataBase();
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		db.execSQL("drop table if exists " + TABLE_POSITION);
		onCreate(db);
	}

	public void initDataBase() {
//		ArrayList<AppBean> infos = new AppListBiz(context).getLauncherApps();
		ArrayList<AppBean> infos = new AppListBiz(context).getUserApps();
		ArrayList<ArrayList<AppBean>> lists = deliverList(infos);
		insert(lists);
		Log.i("APPDB", "应用程序数据初始化完成");
	}

	private ArrayList<ArrayList<AppBean>> deliverList(ArrayList<AppBean> infos) {
		int i = 1;
		Log.i("info", "总共安装APP个数" + infos.size());
		ArrayList<ArrayList<AppBean>> childLists = new ArrayList<ArrayList<AppBean>>();
		ArrayList<AppBean> list = new ArrayList<AppBean>();
		while (i <= infos.size()) {
			list.add(infos.get(i - 1));
			if (i % 12 == 0) {
				childLists.add(list);
				Log.i("info", "产生一个子集合");
				list = new ArrayList<AppBean>();
			} else if (i == infos.size()) {
				childLists.add(list);
				Log.i("info", "产生最后一个子集合");
			}
			i++;
		}
		return childLists;
	}

	/**
	 * 查询所有的应用程序
	 * 
	 * @return
	 */
	public ArrayList<ArrayList<AppBean>> queryAllAppInfos() {
		ArrayList<ArrayList<AppBean>> lists = new ArrayList<ArrayList<AppBean>>();
		ArrayList<Integer> pages = queryPageIndex();
		if (pages == null) {
			initDataBase();
			pages = queryPageIndex();
		}
		for (Integer page : pages) {
			ArrayList<AppBean> list = queryAppInfos(page);
			lists.add(list);
		}
		return lists;
	}

	public ArrayList<Integer> queryPageIndex() {
		SQLiteDatabase db = getReadableDatabase();

		ArrayList<Integer> list = null;
		Cursor c = db.query(TABLE_POSITION, new String[] { PAGEINDEX }, null,
				null, null, null, PAGEINDEX + " ASC", null);
		if (c != null && c.getCount() > 0) {
			list = new ArrayList<Integer>();
			while (c.moveToNext()) {
				int q = Integer.parseInt(c.getString(c
						.getColumnIndex(PAGEINDEX)));
				if (!list.contains(q)) {
					list.add(q);
				}
			}
		}
		c.close();
		db.close();
		return list;
	}

	/**
	 * 更新应用的位置
	 * 
	 * @param lists
	 */
	public void updateAppPosition(ArrayList<ArrayList<AppBean>> lists) {
		SQLiteDatabase db = getWritableDatabase();
		ContentValues values = new ContentValues();
		for (int i = 0; i < lists.size(); i++) {
			for (int j = 0; j < lists.get(i).size(); j++) {
				AppBean bean = lists.get(i).get(j);
				// System.out.println(i + "," + j);
				values.put(PAGEINDEX, i);
				values.put(INDEX_IN_PAGE, j);
				db.update(TABLE_POSITION, values, _ID + "=?",
						new String[] { bean.getId() });
			}
		}
		db.close();
	}

	/**
	 * 查询指定页码的应用程序
	 * 
	 * @param pageIndex
	 * @return
	 */
	@SuppressWarnings("deprecation")
	public ArrayList<AppBean> queryAppInfos(int pageIndex) {
		SQLiteDatabase db = getReadableDatabase();
		String index = String.valueOf(pageIndex);
		ArrayList<AppBean> list = new ArrayList<AppBean>();
		AppBean bean = null;
		Cursor c = db.query(TABLE_POSITION, null, PAGEINDEX + "=?",
				new String[] { index }, null, null, INDEX_IN_PAGE + " ASC",
				null);
		if (c != null && c.getCount() > 0) {
			
			while (c.moveToNext()) {
				bean = new AppBean();
				byte[] blob = c.getBlob(c.getColumnIndex(APP_IMAGE));
				Bitmap bmp = BitmapFactory
						.decodeByteArray(blob, 0, blob.length);
				bean.setIcon(new BitmapDrawable(bmp));
				bean.setPackageName(c.getString(c.getColumnIndex(PACKAGENAME)));
				bean.setPosition(c.getInt(c.getColumnIndex(INDEX_IN_PAGE)));
				bean.setPageIndex(c.getInt(c.getColumnIndex(PAGEINDEX)));
				bean.setDataDir(c.getString(c.getColumnIndex(DATA_DIR)));
				bean.setName(c.getString(c.getColumnIndex(NAME)));
				bean.setId(c.getString(c.getColumnIndex(_ID)));
				list.add(bean);
			}
		}
		c.close();
		db.close();
		return list;
	}

	/**
	 * 查询首页显示的APP（新版），支持不连续排序
	 * 
	 * @return
	 */
	@SuppressLint("UseSparseArrays")
	@SuppressWarnings("deprecation")
	public HashMap<Integer, AppBean> queryTopApps() {
		SQLiteDatabase db = getReadableDatabase();
		HashMap<Integer, AppBean> topApps = new HashMap<Integer, AppBean>();
		AppBean bean = null;
		Cursor c = db.query(TABLE_POSITION, null, PAGEINDEX + "=?",
				new String[] { "0" }, null, null, INDEX_IN_PAGE + " ASC", null);
		if (c != null && c.getCount() > 0) {

			while (c.moveToNext()) {
				bean = new AppBean();
				byte[] blob = c.getBlob(c.getColumnIndex(APP_IMAGE));
				Bitmap bmp = BitmapFactory
						.decodeByteArray(blob, 0, blob.length);
				bean.setIcon(new BitmapDrawable(bmp));
				bean.setPackageName(c.getString(c.getColumnIndex(PACKAGENAME)));
				bean.setPosition(c.getInt(c.getColumnIndex(INDEX_IN_PAGE)));
				bean.setPageIndex(c.getInt(c.getColumnIndex(PAGEINDEX)));
				bean.setDataDir(c.getString(c.getColumnIndex(DATA_DIR)));
				bean.setName(c.getString(c.getColumnIndex(NAME)));
				bean.setId(c.getString(c.getColumnIndex(_ID)));
				topApps.put(bean.getPosition(), bean);
			}
		}
		c.close();
		db.close();
		return topApps;
	}

	/**
	 * 新安装程序应用插入数据库，positon和updateTime可以在外面自定义
	 * 
	 * @param position
	 * @param pageIndex
	 */

	public boolean insertApp(String packageName) {
		int[] location = getNewAppLocation();
		AppBean bean = getTheApp(packageName);
		SQLiteDatabase db = getWritableDatabase();
		ContentValues values = new ContentValues();
		values.put(PACKAGENAME, bean.getPackageName());
		values.put(APP_IMAGE, drawableChange(bean.getIcon()));
		values.put(NAME, bean.getName());
		values.put(DATA_DIR, bean.getDataDir());
		values.put(PAGEINDEX, location[0]);
		values.put(INDEX_IN_PAGE, location[1]);
		long c = db.insert(TABLE_POSITION, null, values);
		db.close();
		if (c != -1) {
			return true;
		}
		return false;
	}

	/**
	 * 记录app位置（新版），此方法只用于将APP设置到首页显示
	 * 
	 * @param bean
	 * @param position
	 * @return
	 */
	public boolean recodeApp(AppBean bean, int position) {
		SQLiteDatabase db = getWritableDatabase();
		ContentValues values = new ContentValues();
		values.put(PACKAGENAME, bean.getPackageName());
		values.put(APP_IMAGE, drawableChange(bean.getIcon()));
		values.put(NAME, bean.getName());
		values.put(DATA_DIR, bean.getDataDir());
		values.put(PAGEINDEX, 0);// 0表示放到首页
		values.put(INDEX_IN_PAGE, position);
		long c = db.insert(TABLE_POSITION, null, values);
		if (c != -1) {
			return true;
		}
		db.close();
		return false;
	}

	/**
	 * 移除app位置记录（新版），此方法只用于将APP从首页显示上移除
	 * 
	 * @param bean
	 * @return
	 */
	public boolean removeRecode(AppBean bean) {
		SQLiteDatabase db = getWritableDatabase();
		int c = db.delete(TABLE_POSITION, PAGEINDEX + "=0 and " + INDEX_IN_PAGE
				+ "=?", new String[] { String.valueOf(bean.getPosition()) });
		if (c > 0) {
			return true;
		}
		db.close();
		return false;
	}

	/**
	 * 确定新安装引用的位置
	 * 
	 * @return
	 */
	private int[] getNewAppLocation() {
		int[] location = new int[2];
		ArrayList<Integer> pages = queryPageIndex();
		int pageSize = pages.size();
		ArrayList<ArrayList<AppBean>> lists = queryAllAppInfos();
		if (lists.get(pageSize - 1).size() < 12) {
			location[0] = pageSize - 1;
			location[1] = lists.get(pageSize - 1).size();
		} else {
			location[0] = pageSize;
			location[1] = 0;
		}
		pages = null;
		lists = null;
		return location;
	}

	private AppBean getTheApp(String packageName) {
		PackageManager pm = context.getPackageManager();
		ApplicationInfo appInfo = null;
		try {
			appInfo = pm.getApplicationInfo(packageName, 0);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		AppBean appBean = new AppBean();
		appBean.setName(appInfo.loadLabel(pm).toString());
		appBean.setPackageName(appInfo.packageName);
		appBean.setIcon(appInfo.loadIcon(pm));
		appBean.setDataDir(appInfo.publicSourceDir);
		return appBean;
	}

	public boolean deleteApp(String packageName) {
		SQLiteDatabase db = getWritableDatabase();
		int row = db.delete(TABLE_POSITION, PACKAGENAME + "=?",
				new String[] { packageName });
		db.close();
		return row > 0;
	}

	/**
	 * 删除整个表的数据
	 * 
	 * @return
	 */
	public boolean deletAppinfoss() {
		SQLiteDatabase db = getWritableDatabase();
		int c = db.delete(TABLE_POSITION, null, null);
		db.close();
		if (c != -1) {

			return true;
		}

		return false;
	}

	public void insert(ArrayList<ArrayList<AppBean>> lists) {
		SQLiteDatabase db = getWritableDatabase();
		db.beginTransaction();// 手动设置事务开始
		AppBean bean = null;
		ContentValues values = new ContentValues();
		try {
			int size = lists.size();
			for (int i = 0; i < size; i++) {
				for (int j = 0; j < lists.get(i).size(); j++) {
					bean = new AppBean();
					bean = lists.get(i).get(j);
					byte[] blog = drawableChange(bean.getIcon());

					values.put(INDEX_IN_PAGE, j);
					values.put(NAME, bean.getName());
					values.put(PACKAGENAME, bean.getPackageName());
					values.put(DATA_DIR, bean.getDataDir());
					values.put(PAGEINDEX, i);
					values.put(APP_IMAGE, blog);
					db.insert(TABLE_POSITION, null, values);
				}
			}
			db.setTransactionSuccessful();
		} finally {
			db.endTransaction();// 处理完成
			db.close();
		}
	}

	/**
	 * drawable转化成字节数组
	 * 
	 * @param drawable
	 * @return
	 */
	private byte[] drawableChange(Drawable drawable) {

		Bitmap bm = ((BitmapDrawable) drawable).getBitmap();
		ByteArrayOutputStream baos = new ByteArrayOutputStream();
		bm.compress(Bitmap.CompressFormat.PNG, 100, baos);
		byte[] date = baos.toByteArray();
		return date;
	}

}

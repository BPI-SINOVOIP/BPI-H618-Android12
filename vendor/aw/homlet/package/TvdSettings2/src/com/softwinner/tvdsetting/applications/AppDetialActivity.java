package com.softwinner.tvdsetting.applications;

import java.util.ArrayList;
import java.util.List;

import com.softwinner.tvdsetting.R;
import com.softwinner.tvdsetting.applications.AppDetailDialog.AppDetailDialogInterface;
import com.softwinner.tvdsetting.applications.ApplicationsState.AppEntry;
import com.softwinner.tvdsetting.widget.ExtImageButton;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.INotificationManager;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageDataObserver;
import android.content.pm.IPackageMoveObserver;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.RemoteException;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.UserHandle;
import android.os.ServiceManager;
import android.text.format.Formatter;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Button;

public class AppDetialActivity extends Activity implements
		View.OnClickListener, CheckBox.OnCheckedChangeListener, ApplicationsState.Callbacks{
	private static final String TAG = "AppDetialActivity";
	public static final String ARG_PACKAGE_NAME = "PACKAGENAME";
	ApplicationsState mState;
	private ApplicationsState.Session mSession;
	private TextView appName;
	private TextView appVersion;
	private TextView mTotalSize;
	private TextView mAppSize;
	private TextView mDataSize;
	private TextView mExternalCodeSize;
	private TextView mExternalDataSize;
	private TextView mCacheSize;
	private ImageView appIcon;
	private CheckBox mNotificationSwitch;
	private PackageManager mPm;
	private AppEntry mAppEntry;
	private PackageInfo mPkgInfo;
	private DevicePolicyManager mDpm;
	private Button mStopapp, mUninstallapp, mCleardata, mClearcache;
	private ClearCacheObserver mClearCacheObserver;
	private ClearUserDataObserver mClearDataObserver;

	private static final int SIZE_INVALID = -1;
	// Resource strings
	private CharSequence mInvalidSizeStr;
	private CharSequence mComputingStr;

	private boolean mHaveSizes = false;
	private long mLastCodeSize = -1;
	private long mLastDataSize = -1;
	private long mLastExternalCodeSize = -1;
	private long mLastExternalDataSize = -1;
	private long mLastCacheSize = -1;
	private long mLastTotalSize = -1;

	// Result code identifiers
	public static final int REQUEST_UNINSTALL = 1;
	public static final int REQUEST_MANAGE_SPACE = 2;

	// internal constants used in Handler
	private static final int OP_SUCCESSFUL = 1;
	private static final int OP_FAILED = 2;
	private static final int CLEAR_USER_DATA = 1;
	private static final int CLEAR_CACHE = 3;
	private static final int PACKAGE_MOVE = 4;

	private Handler mHandler = new Handler() {
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case CLEAR_USER_DATA:
				processClearMsg(msg);
				Log.d(TAG, "Clean DATA Finished");
				mAppEntry = mState.getEntry(mAppEntry.info.packageName);
				updateStorageData();
				mCacheSize.setText(getSizeStr(0));
				mDataSize.setText(getSizeStr(0));
				break;
			case CLEAR_CACHE:
				// Refresh size info
				mState.requestSize(mAppEntry.info.packageName);
				//mAppEntry = mState.getEntry(mAppEntry.info.packageName);
				//updateStorageData();
				//mCacheSize.setText(getSizeStr(0));
				break;
			case PACKAGE_MOVE:
				processMoveMsg(msg);
				;
				break;
			default:
				break;
			}
		}
	};

	private void processClearMsg(Message msg) {
		int result = msg.arg1;
		String packageName = mAppEntry.info.packageName;
		if (result == OP_SUCCESSFUL) {
			Log.i(TAG, "Cleared user data for package : " + packageName);
			mState.requestSize(mAppEntry.info.packageName);
		} else {
			mCleardata.setEnabled(true);
		}
		checkForceStop();
	}

	private void processMoveMsg(Message msg) {
		int result = msg.arg1;
		String packageName = mAppEntry.info.packageName;
		// Refresh the button attributes.
		if (result == PackageManager.MOVE_SUCCEEDED) {
			Log.i(TAG, "Moved resources for " + packageName);
			// Refresh size information again.
			mState.requestSize(mAppEntry.info.packageName);
		} else {
			// showDialogInner(DLG_MOVE_FAILED, result);
		}
		// refreshUi();
	}

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		this.setContentView(R.layout.appdetail);
		Intent intent = this.getIntent();
		final Bundle args = intent.getExtras();
		String packageName = (args != null) ? args.getString(ARG_PACKAGE_NAME)
				: null;
		mState = ApplicationsState.getInstance(getApplication());
		mSession = mState.newSession(this);
		mPm = this.getPackageManager();
		mDpm = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
		mAppEntry = mState.getEntry(packageName);
		if (mAppEntry != null) {
			try {
				mPkgInfo = mPm.getPackageInfo(mAppEntry.info.packageName, 0);
			} catch (NameNotFoundException e) {
				Log.e(TAG, "Exception when retrieving package:"
						+ mAppEntry.info.packageName, e);
			}
		} else {
			this.finish();
		}
		appName = (TextView) this.findViewById(R.id.appdetialname);
		appVersion = (TextView) this.findViewById(R.id.appdetialversion);
		appIcon = (ImageView) this.findViewById(R.id.appdetialicon);
		appName.setText(mAppEntry.label);
		appIcon.setImageDrawable(mAppEntry.icon);
		appVersion.setText("version:" + mPkgInfo.versionName);
		Log.d(TAG, "package name = " + packageName);

		mStopapp = (Button) this.findViewById(R.id.stopapp);
		mUninstallapp = (Button) this.findViewById(R.id.uninstallapp);
		mCleardata = (Button) this.findViewById(R.id.cleardata);
		mClearcache = (Button) this.findViewById(R.id.clearcache);
		mNotificationSwitch = (CheckBox) this
				.findViewById(R.id.allownotification);
		initNotificationButton();
		mStopapp.setOnClickListener(this);
		if(!isAppRuning(mAppEntry.info.packageName)){
			mStopapp.setFocusable(false);
		}
		mUninstallapp.setOnClickListener(this);
		if(isThisASystemPackage()){
			mUninstallapp.setFocusable(false);
		}
		mCleardata.setOnClickListener(this);
		if (isThisASystemPackage()) {
			mClearcache.setFocusable(false);
		}
		mClearcache.setOnClickListener(this);

		// Set default values on sizes
		mTotalSize = (TextView) this.findViewById(R.id.total_size_text);
		mAppSize = (TextView) this.findViewById(R.id.application_size_text);
		mDataSize = (TextView) this.findViewById(R.id.data_size_text);
		// mExternalCodeSize =
		// (TextView)this.findViewById(R.id.external_code_size_text);
		mExternalDataSize = (TextView) this
				.findViewById(R.id.external_data_size_text);
		mCacheSize = (TextView) this.findViewById(R.id.cache_size_text);
		updateStorageData();
		mSession.resume();

	}

	@Override
	protected void onResume() {
		initNotificationButton();
		updateStorageData();
		mSession.resume();
		super.onResume();
	}
	
	@Override
	protected void onPause() {
		// TODO Auto-generated method stub
		mSession.pause();
		super.onPause();
	}

	private void updateStorageData() {
		mHaveSizes = true;
		long codeSize = mAppEntry.codeSize;
		long dataSize = mAppEntry.dataSize;

		if (Environment.isExternalStorageEmulated()) {
			codeSize += mAppEntry.externalCodeSize;
			dataSize += mAppEntry.externalDataSize;
		} else {
			if (mLastExternalCodeSize != mAppEntry.externalCodeSize) {
				mLastExternalCodeSize = mAppEntry.externalCodeSize;
				// mExternalCodeSize.setText(getSizeStr(mAppEntry.externalCodeSize));
			}
			if (mLastExternalDataSize != mAppEntry.externalDataSize) {
				mLastExternalDataSize = mAppEntry.externalDataSize;
				mExternalDataSize
						.setText(getSizeStr(mAppEntry.externalDataSize));
			}
		}
		if (mLastCodeSize != codeSize) {
			mLastCodeSize = codeSize;
			mAppSize.setText(getSizeStr(codeSize));
		}
		if (mLastDataSize != dataSize) {
			mLastDataSize = dataSize;
			mDataSize.setText(getSizeStr(dataSize));
			if(mLastDataSize < 1){
				mCleardata.setFocusable(false);
			}
		}

		long cacheSize = mAppEntry.cacheSize + mAppEntry.externalCacheSize;
		Log.d(TAG, "cacheSize = " + cacheSize + " dataSize = " + dataSize);
		if (mLastCacheSize != cacheSize) {
			mLastCacheSize = cacheSize;
			mCacheSize.setText(getSizeStr(cacheSize));
			if(mLastCacheSize < 1){
				mClearcache.setFocusable(false);
			}
		}
		mCacheSize.setText(getSizeStr(cacheSize));
		if (mLastTotalSize != mAppEntry.size) {
			mLastTotalSize = mAppEntry.size;
			mTotalSize.setText(getSizeStr(mAppEntry.size));
		}

	}

	/*
	 * Private method to initiate clearing user data when the user clicks the
	 * clear data button for a system package
	 */
	private void initiateClearUserData() {
		mCleardata.setEnabled(false);
		// Invoke uninstall or clear user data based on sysPackage
		String packageName = mAppEntry.info.packageName;
		Log.i(TAG, "Clearing user data for package : " + packageName);
		if (mClearDataObserver == null) {
			mClearDataObserver = new ClearUserDataObserver();
		}
		ActivityManager am = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
		boolean res = am.clearApplicationUserData(packageName,
				mClearDataObserver);
		if (!res) {
			// Clearing data failed for some obscure reason. Just log error for
			// now
			Log.i(TAG, "Couldnt clear application user data for package:"
					+ packageName);
		} else {
		}
	}

	@Override
	public void onClick(View arg0) {
		// TODO Auto-generated method stub
		// statrtAppDetialDialog();
		switch (arg0.getId()) {
		case R.id.stopapp:
			onForceStopApp();
			break;
		case R.id.uninstallapp:
			onUninstallApp();
			break;
		case R.id.cleardata:
			onClearData();
			break;
		case R.id.clearcache:
			if (!isThisASystemPackage()) {
				onClearCache();
			}
			break;
		default:
			break;
		}
	}

	private void onClearCache() {
		// TODO Auto-generated method stub
		if (mClearCacheObserver == null) {
			mClearCacheObserver = new ClearCacheObserver();
		}
		mPm.deleteApplicationCacheFiles(mAppEntry.info.packageName,
				mClearCacheObserver);
		// updateStorageData();
	}

	private void onClearData() {
		// TODO Auto-generated method stub
		final AppDetailDialog dialog = new AppDetailDialog(
				AppDetialActivity.this, R.style.CommonDialog, mAppEntry.label,
				getString(R.string.touncleanappdata), mAppEntry.icon);
		AppDetailDialogInterface appi = new AppDetailDialogInterface() {

			@Override
			public void onButtonYesClick() {
				// TODO Auto-generated method stub
				if (mAppEntry.info.manageSpaceActivityName != null) {
					/*
					 * Intent intent = new Intent(Intent.ACTION_DEFAULT);
					 * intent.setClassName(mAppEntry.info.packageName,
					 * mAppEntry.info.manageSpaceActivityName);
					 * startActivityForResult(intent, 2);
					 */
					// Log.d(TAG,"start clean application data");
					// initiateClearUserData();
				}
				initiateClearUserData();
				dialog.dismiss();
			}

			@Override
			public void onButtonNoClick() {
				// TODO Auto-generated method stub
				dialog.dismiss();
			}

		};
		dialog.setAppDetailDialogInterface(appi);
		dialog.show();

	}

	private void onUninstallApp() {
		// TODO Auto-generated method stub
		if (isThisASystemPackage()) {
			mUninstallapp.setEnabled(false);
			Toast toast = Toast.makeText(AppDetialActivity.this, "系统应用无法卸载",
					Toast.LENGTH_SHORT);
			toast.show();
		} else {
			final AppDetailDialog dialog = new AppDetailDialog(
					AppDetialActivity.this, R.style.CommonDialog,
					mAppEntry.label, getString(R.string.touninstallapp),
					mAppEntry.icon);
			AppDetailDialogInterface appi = new AppDetailDialogInterface() {

				@Override
				public void onButtonYesClick() {
					// TODO Auto-generated method stub
					uninstallPkg(mAppEntry.info.packageName, true, false);
					dialog.dismiss();
				}

				@Override
				public void onButtonNoClick() {
					// TODO Auto-generated method stub
					dialog.dismiss();
				}

			};
			dialog.setAppDetailDialogInterface(appi);
			dialog.show();
		}

	}

	private void onForceStopApp() {
		// TODO Auto-generated method stub
		final AppDetailDialog dialog = new AppDetailDialog(
				AppDetialActivity.this, R.style.CommonDialog, mAppEntry.label,
				getString(R.string.toforcestopapp), mAppEntry.icon);
		AppDetailDialogInterface appi = new AppDetailDialogInterface() {

			@Override
			public void onButtonYesClick() {
				// TODO Auto-generated method stub
				forceStopPackage(mAppEntry.info.packageName);
				dialog.dismiss();
			}

			@Override
			public void onButtonNoClick() {
				// TODO Auto-generated method stub
				dialog.dismiss();
			}

		};
		dialog.setAppDetailDialogInterface(appi);
		dialog.show();

	}

	class ClearUserDataObserver extends IPackageDataObserver.Stub {
		public void onRemoveCompleted(final String packageName,
				final boolean succeeded) {
			final Message msg = mHandler.obtainMessage(CLEAR_USER_DATA);
			msg.arg1 = succeeded ? OP_SUCCESSFUL : OP_FAILED;
			mHandler.sendMessage(msg);
		}
	}

	class ClearCacheObserver extends IPackageDataObserver.Stub {
		public void onRemoveCompleted(final String packageName,
				final boolean succeeded) {
			final Message msg = mHandler.obtainMessage(CLEAR_CACHE);
			msg.arg1 = succeeded ? OP_SUCCESSFUL : OP_FAILED;
			mHandler.sendMessage(msg);
		}
	}

	class PackageMoveObserver extends IPackageMoveObserver.Stub {
		public void packageMoved(String packageName, int returnCode)
				throws RemoteException {
			final Message msg = mHandler.obtainMessage(PACKAGE_MOVE);
			msg.arg1 = returnCode;
			mHandler.sendMessage(msg);
		}
	}

	private String getSizeStr(long size) {
		if (size == SIZE_INVALID) {
			return mInvalidSizeStr.toString();
		}
		return Formatter.formatFileSize(this, size);
	}

	private final BroadcastReceiver mCheckKillProcessesReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			updateForceStopButton(getResultCode() != Activity.RESULT_CANCELED);
		}
	};

	private void statrtAppDetialDialog() {
		Dialog dialog = new AppDetailDialog(AppDetialActivity.this,
				R.style.CommonDialog);
		dialog.show();
	}

	private void startUninstallProgress(String packageName) {
		ApplicationInfo mAppInfo = null;
		try {
			mAppInfo = mPm.getApplicationInfo(packageName,
					PackageManager.GET_UNINSTALLED_PACKAGES);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		Intent newIntent = new Intent(Intent.ACTION_VIEW);
		newIntent.putExtra("com.android.packageinstaller.applicationInfo",
				mAppInfo);
		newIntent.putExtra(Intent.EXTRA_UNINSTALL_ALL_USERS, true);
		if (getIntent().getBooleanExtra(Intent.EXTRA_RETURN_RESULT, false)) {
			newIntent.putExtra(Intent.EXTRA_RETURN_RESULT, true);
			newIntent.addFlags(Intent.FLAG_ACTIVITY_FORWARD_RESULT);
		}
		ComponentName comp = new ComponentName("com.android.packageinstaller",
				"com.android.packageinstaller.UninstallAppProgress");
		newIntent.setComponent(comp);
		// newIntent.setClass(this, UninstallAppProgress.class);
		startActivity(newIntent);
		finish();
	}

	private void uninstallPkg(String packageName, boolean allUsers,
			boolean andDisable) {
		// Create new intent to launch Uninstaller activity
		/*
		 * Uri packageURI = Uri.parse("package:"+packageName); Intent
		 * uninstallIntent = new Intent(Intent.ACTION_UNINSTALL_PACKAGE,
		 * packageURI);
		 * uninstallIntent.putExtra(Intent.EXTRA_UNINSTALL_ALL_USERS, allUsers);
		 * startActivityForResult(uninstallIntent, 1);
		 */
		// mDisableAfterUninstall = andDisable;
		startUninstallProgress(packageName);
	}

	private boolean isThisASystemPackage() {
		// try {
		// PackageInfo sys = mPm.getPackageInfo("android",
		// PackageManager.GET_SIGNATURES);
		// return (mPkgInfo != null && mPkgInfo.signatures != null &&
		// sys.signatures[0].equals(mPkgInfo.signatures[0]));
		// } catch (PackageManager.NameNotFoundException e) {
		// return false;
		// }

		if ((mPkgInfo.applicationInfo.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0) {
			// 代表的是系统的应用,但是被用户升级了. 用户应用
			return false;
		} else if ((mPkgInfo.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) == 0) {
			// 代表的用户的应用
			return false;
		}
		return true;
	}

	private void initNotificationButton() {
		INotificationManager nm = INotificationManager.Stub
				.asInterface(ServiceManager
						.getService(Context.NOTIFICATION_SERVICE));
		boolean enabled = true; // default on
		try {
			enabled = nm.areNotificationsEnabledForPackage(
					mAppEntry.info.packageName, mAppEntry.info.uid);
		} catch (android.os.RemoteException ex) {
			// this does not bode well
		}
		mNotificationSwitch.setChecked(enabled);
		if (isThisASystemPackage()) {
			mNotificationSwitch.setEnabled(false);
		} else {
			mNotificationSwitch.setEnabled(true);
			mNotificationSwitch.setOnCheckedChangeListener(this);
		}
	}

	// ~~直接实现在onCheckedChanged里
	private void setNotificationsEnabled(boolean enabled) {
		String packageName = mAppEntry.info.packageName;
		INotificationManager nm = INotificationManager.Stub
				.asInterface(ServiceManager
						.getService(Context.NOTIFICATION_SERVICE));
		try {
			final boolean enable = mNotificationSwitch.isChecked();
			nm.setNotificationsEnabledForPackage(packageName,
					mAppEntry.info.uid, enabled);
		} catch (android.os.RemoteException ex) {
			mNotificationSwitch.setChecked(!enabled); // revert
		}
	}

	private void forceStopPackage(String pkgName) {
		ActivityManager am = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
		am.forceStopPackage(pkgName);
		mState.invalidatePackage(pkgName);
		ApplicationsState.AppEntry newEnt = mState.getEntry(pkgName);
		if (newEnt != null) {
			mAppEntry = newEnt;
		}
		checkForceStop();
	}
	
	private boolean isAppRuning(String pkgName){
		ActivityManager am = (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
		List<RunningTaskInfo> list = am.getRunningTasks(100);
		boolean isAppRunning = false;
		for (RunningTaskInfo info : list) {
			if (info.topActivity.getPackageName().equals(pkgName) 
				|| info.baseActivity.getPackageName().equals(pkgName)) {
				
				isAppRunning = true;
				Log.i(TAG,info.topActivity.getPackageName() + " info.baseActivity.getPackageName()="
				      +info.baseActivity.getPackageName());
				break;
			}
		}
		return isAppRunning;
	}

	private void updateForceStopButton(boolean enabled) {
		mStopapp.setEnabled(enabled);
		mStopapp.setOnClickListener(this); // ~~再设置有什么用？
	}

	private void checkForceStop() {
		if (mDpm.packageHasActiveAdmins(mPkgInfo.packageName)) {
			// User can't force stop device admin.
			updateForceStopButton(false);
		} else if ((mAppEntry.info.flags & ApplicationInfo.FLAG_STOPPED) == 0) {
			// If the app isn't explicitly stopped, then always show the
			// force stop button.
			updateForceStopButton(true);
		} else { // ~~这个else是什么情况，下面的intent是干嘛用的？
			Intent intent = new Intent(Intent.ACTION_QUERY_PACKAGE_RESTART,
					Uri.fromParts("package", mAppEntry.info.packageName, null));
			intent.putExtra(Intent.EXTRA_PACKAGES,
					new String[] { mAppEntry.info.packageName });
			intent.putExtra(Intent.EXTRA_UID, mAppEntry.info.uid);
			intent.putExtra(Intent.EXTRA_USER_HANDLE,
					UserHandle.getUserId(mAppEntry.info.uid));
			this.sendOrderedBroadcastAsUser(intent, UserHandle.CURRENT, null,
					mCheckKillProcessesReceiver, null,
					Activity.RESULT_CANCELED, null, null);
		}
	}

	@Override
	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
		// String packageName = mAppEntry.info.packageName;
		// if (buttonView == mNotificationSwitch) {
		// if (!isChecked) { //~~这里可以直接判断，而不需要else？
		// } else {
		// setNotificationsEnabled(true);
		// }
		// }
		String packageName = mAppEntry.info.packageName;
		INotificationManager nm = INotificationManager.Stub
				.asInterface(ServiceManager
						.getService(Context.NOTIFICATION_SERVICE));
		try {
			final boolean enable = mNotificationSwitch.isChecked();
			nm.setNotificationsEnabledForPackage(packageName,
					mAppEntry.info.uid, enable);
		} catch (android.os.RemoteException ex) {
			mNotificationSwitch.setChecked(!isChecked); // revert
		}
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		TextView title = (TextView) this.findViewById(R.id.title);
		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK:
			title.setText(R.string.setting_appinfo);
			break;
		}
		return false;
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		TextView title = (TextView) this.findViewById(R.id.title);
		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK:
			title.setText(R.string.setting_appinfo);
			finish();
			break;
		}
		return false;
	}

	@Override
	public void onRunningStateChanged(boolean running) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onPackageListChanged() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onRebuildComplete(ArrayList<AppEntry> apps) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onPackageIconChanged() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onPackageSizeChanged(String packageName) {
		if (packageName.equals(mAppEntry.info.packageName)) {
            updateStorageData();
        }
	}

	@Override
	public void onAllSizesComputed() {
		// TODO Auto-generated method stub
		
	}
}

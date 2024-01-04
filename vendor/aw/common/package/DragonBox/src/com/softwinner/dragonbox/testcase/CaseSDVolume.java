package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.StorageInfo;
import com.softwinner.dragonbox.manager.StorageInfoManager;

public class CaseSDVolume extends IBaseCase {
    private StorageInfoManager mStorageInfoManager;

    TextView mMaxMountStatus;
    TextView mMaxRWStatus;

    TextView mMinMountStatus;
    TextView mMinRWStatus;

    private BroadcastReceiver mBroadCastReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_MEDIA_MOUNTED.equals(action)) {
                updateStatus();
            }
            if (Intent.ACTION_MEDIA_UNMOUNTED.equals(action)) {
                updateStatus();
            }
        }

    };

    public CaseSDVolume(Context context) {
        super(context, R.string.case_sdvolume_name, R.layout.case_sdvolume_max,
                R.layout.case_sdvolume_min, TYPE_MODE_AUTO);
        mStorageInfoManager = new StorageInfoManager(mContext);
        mMaxMountStatus = (TextView) mMaxView
                .findViewById(R.id.case_sdvolume_mount_status);
        mMaxRWStatus = (TextView) mMaxView
                .findViewById(R.id.case_sdvolume_rw_status);
        mMinMountStatus = (TextView) mMinView
                .findViewById(R.id.case_sdvolume_mount_status);
        mMinRWStatus = (TextView) mMinView
                .findViewById(R.id.case_sdvolume_rw_status);
    }

    public CaseSDVolume(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    @Override
    public void onStartCase() {
        setDialogPositiveButtonEnable(false);
        IntentFilter filter = new IntentFilter();
        filter.addDataScheme("file");
        filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        mContext.registerReceiver(mBroadCastReceiver, filter);
        updateStatus();
    }

    private void updateStatus() {

        StorageInfo info = mStorageInfoManager.getExtsdInfo();
        mMaxMountStatus
                .setText(info.isMounted() ? R.string.case_sdvolume_mount_status_success
                        : R.string.case_SDvolume_mount_status_fail);
        mMaxRWStatus
                .setText(info.isRWable() ? R.string.case_sdvolume_status_success
                        : R.string.case_sdvolume_status_fail);
        mMinMountStatus
                .setText(info.isMounted() ? R.string.case_sdvolume_mount_status_success
                        : R.string.case_SDvolume_mount_status_fail);
        mMinRWStatus
                .setText(info.isRWable() ? R.string.case_sdvolume_status_success
                        : R.string.case_sdvolume_status_fail);
        boolean result = info.isMounted() && info.isRWable();
        setDialogPositiveButtonEnable(result);
        setCaseResult(result);
    }

    @Override
    public void onStopCase() {
        mContext.unregisterReceiver(mBroadCastReceiver);
    }

    @Override
    public void reset() {
        super.reset();

    }

}

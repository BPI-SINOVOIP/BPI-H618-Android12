package com.softwinner.dragonbox.testcase;

import java.util.List;

import org.xmlpull.v1.XmlPullParser;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import android.widget.TextView;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.StorageInfo;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.manager.StorageInfoManager;

public class CaseUsbVolume extends IBaseCase {
    public static final String TAG = "DragonBox-CaseUsbVolume";
    private StorageInfoManager mStorageInfoManager;

    private int testUSBNum = 1;

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
        }
    };

    public CaseUsbVolume(Context context) {
        super(context, R.string.case_usbvolume_name,
                R.layout.case_usbvolume_max, R.layout.case_usbvolume_min,
                TYPE_MODE_AUTO);
        mStorageInfoManager = new StorageInfoManager(mContext);
        mMaxMountStatus = (TextView) mMaxView
                .findViewById(R.id.case_usbvolume_mount_status);
        mMaxRWStatus = (TextView) mMaxView
                .findViewById(R.id.case_usbvolume_rw_status);
        mMinMountStatus = (TextView) mMinView
                .findViewById(R.id.case_usbvolume_mount_status);
        mMinRWStatus = (TextView) mMinView
                .findViewById(R.id.case_usbvolume_rw_status);
    }

    public CaseUsbVolume(Context context, XmlPullParser xmlParser) {
        this(context);
        String num = xmlParser.getAttributeValue(null, "num");
        try{
            testUSBNum = Integer.parseInt(num);
        } catch (Exception e) {
            e.printStackTrace();
            testUSBNum = 1;
        }
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseUsbVolume");
        setDialogPositiveButtonEnable(false);
        IntentFilter filter = new IntentFilter();
        filter.addDataScheme("file");
        filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        mContext.registerReceiver(mBroadCastReceiver, filter);
        updateStatus();
    }

    private void updateStatus() {

        List<StorageInfo> infos = mStorageInfoManager.getUsbdeviceInfos();
        //boolean haveMounted = false;
        //boolean allRWable = false;
        int mountedCount = 0;
        int rwSuccessCount = 0;
        for (StorageInfo info : infos) {
            Log.i("CaseUsbVolume", "UsbdeviceInfo =" + info.getPath());
            if (info.isMounted()) {
                mountedCount++;
                if (info.isRWable()) {
                    rwSuccessCount++;
                }
            }
        }

        int mountResultRes = (testUSBNum <= mountedCount) ? R.string.case_usbvolume_mount_status_success
                : R.string.case_usbvolume_mount_status_fail;
        int rwResultRes = (testUSBNum <= rwSuccessCount) ? R.string.case_usbvolume_status_success
                : R.string.case_usbvolume_status_fail;

        mMaxMountStatus
                .setText(mContext.getString(mountResultRes, testUSBNum, mountedCount));
        mMaxRWStatus.setText(mContext.getString(rwResultRes, testUSBNum, mountedCount));
        mMinMountStatus
                .setText(mContext.getString(mountResultRes, testUSBNum, mountedCount));
        mMinRWStatus.setText(mContext.getString(rwResultRes, testUSBNum, mountedCount));

        setCaseResult(testUSBNum <= mountedCount && testUSBNum <= rwSuccessCount);
        Log.w(TAG,"CaseUsbVolume test over, test result is "+getCaseResult());
        setDialogPositiveButtonEnable(testUSBNum <= mountedCount && testUSBNum <= rwSuccessCount);
    }

    @Override
    public void onStopCase() {
        mContext.unregisterReceiver(mBroadCastReceiver);
    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[14], ""+getCaseResult());//usb
    }

    @Override
    public void reset() {
        super.reset();
    }
}

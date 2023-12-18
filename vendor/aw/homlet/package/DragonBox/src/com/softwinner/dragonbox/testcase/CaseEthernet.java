package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.app.AlertDialog;
import android.content.Context;
import android.util.Log;
import android.widget.TextView;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.EthernetInfo;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.manager.EthernetCheckManager;
import com.softwinner.dragonbox.manager.EthernetCheckManager.OnEthernetCheckListener;

public class CaseEthernet extends IBaseCase implements OnEthernetCheckListener {
    public static final String TAG = "DragonBox-CaseEthernet";
    private EthernetCheckManager ethernetCheckManager;

    private TextView mMaxLineConnTV;
    private TextView mMaxIpAddrTV;
    private TextView mMinLineConnTV;
    private TextView mMinIpAddrTV;

    public CaseEthernet(Context context) {
        super(context, R.string.case_ethernet_name, R.layout.case_ethernet_max,
                R.layout.case_ethernet_min, IBaseCase.TYPE_MODE_AUTO);
        ethernetCheckManager = new EthernetCheckManager(context);
        ethernetCheckManager.setOnEthernetCheckListener(this);

        mMaxLineConnTV = (TextView) mMaxView
                .findViewById(R.id.case_ethernet_line_conn);
        mMaxIpAddrTV = (TextView) mMaxView
                .findViewById(R.id.case_ethernet_ip_addr);
        mMinLineConnTV = (TextView) mMinView
                .findViewById(R.id.case_ethernet_line_conn);
        mMinIpAddrTV = (TextView) mMinView
                .findViewById(R.id.case_ethernet_ip_addr);
    }

    public CaseEthernet(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase");
        setDialogPositiveButtonEnable(false);
        ethernetCheckManager.startEthernetCheck();
    }

    @Override
    public void onStopCase() {
        ethernetCheckManager.stopEthernetCheck();
        updateView(ethernetCheckManager.getEthernetInfo());
    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[16], getCaseResult()+"");//ethernet
    }

    private void updateView(EthernetInfo ethernetInfo) {
        mMaxLineConnTV
                .setText(ethernetInfo.isLineConn() ? R.string.case_ethernet_line_conn_success
                        : R.string.case_ethernet_line_conn_fail);
        mMinLineConnTV
                .setText(ethernetInfo.isLineConn() ? R.string.case_ethernet_line_conn_success
                        : R.string.case_ethernet_line_conn_fail);
        if (ethernetInfo.getIpAdd() != null) {
            mMaxIpAddrTV.setText(ethernetInfo.getIpAdd());
            mMinIpAddrTV.setText(ethernetInfo.getIpAdd());
        } else {
            mMaxIpAddrTV.setText(R.string.case_ethernet_ip_addr_fail);
            mMinIpAddrTV.setText(R.string.case_ethernet_ip_addr_fail);
        }
    }

    @Override
    public void onCheckStart() {
        mMaxLineConnTV.setText(R.string.case_ethernet_line_conn);
        mMaxIpAddrTV.setText(R.string.case_ethernet_ip_addr_getting);
        mMinIpAddrTV.setText(R.string.case_ethernet_ip_addr_getting);
        mMinLineConnTV.setText(R.string.case_ethernet_line_conn);
    }

    @Override
    public void onCheckEnd(EthernetInfo ethernetInfo) {
        Log.w(TAG,"onCheckEnd, ip is "+ethernetInfo.getIpAdd()+"  连接状态: "+ethernetInfo.isLineConn());
        updateView(ethernetInfo);
        boolean result = ethernetInfo.getIpAdd() != null
                && ethernetInfo.isLineConn();
        stopCase();
        setCaseResult(result);
        setDialogPositiveButtonEnable(result);
    }

    @Override
    public void onCheckUpdate(EthernetInfo ethernetInfo) {
        Log.w(TAG,"onCheckUpdate, ip is "+ethernetInfo.getIpAdd()+" 连接状态: "+ethernetInfo.isLineConn());
        setDialogPositiveButtonEnable(false);
    }

    @Override
    public void reset() {
        super.reset();
        mMaxLineConnTV.setText(R.string.case_ethernet_line_conn);
        mMaxIpAddrTV.setText(R.string.case_ethernet_ip_addr);
        mMinLineConnTV.setText(R.string.case_ethernet_line_conn);
        mMinIpAddrTV.setText(R.string.case_ethernet_ip_addr);
    }

}

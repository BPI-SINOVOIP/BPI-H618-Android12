package com.softwinner.dragonbox.testcase;

import java.util.ArrayList;
import java.util.List;

import org.xmlpull.v1.XmlPullParser;

import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.BluetoothDeviceInfo;
import com.softwinner.dragonbox.manager.BluetoothManager;
import com.softwinner.dragonbox.manager.BluetoothManager.OnBluetoothSearchListener;
import com.softwinner.dragonbox.jni.ReadPrivateJNI;
import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.DragonBoxApplication;

public class CaseBluetooth extends IBaseCase implements OnBluetoothSearchListener{
    private static final String TAG = "DragonBox-CaseBluetooth";
    private BluetoothViewAdapter bluetoothAdapter;

    private TextView mMaxBTSearchStatusTV;
    private ListView mMaxBTSearchResultLV;

    private TextView mMinBTAddr;
    private TextView mMinBTStatus;
    private TextView mMinBTFoundNum;
    private TextView mMinBTFoundName;

    public static String btpair = null;
    private ReadPrivateJNI mReadPrivateJNI = DragonBoxApplication.getReadPrivateJNI();

    private BluetoothManager mBluetoothManager;
    public CaseBluetooth(Context context) {
        super(context, R.string.case_bluetooth_name, R.layout.case_bluetooth_max,
                R.layout.case_bluetooth_min, TYPE_MODE_AUTO);
        mBluetoothManager = new BluetoothManager(context);

        mMaxBTSearchStatusTV = (TextView) mMaxView.findViewById(R.id.case_bluetooth_status);
        mMaxBTSearchResultLV = (ListView) mMaxView.findViewById(R.id.case_bluetooth_result_list);
        mMinBTAddr = (TextView) mMinView.findViewById(R.id.case_bt_addr);
        mMinBTStatus = (TextView) mMinView.findViewById(R.id.case_bt_status);
        mMinBTFoundNum = (TextView) mMinView.findViewById(R.id.case_bt_total_found);
        mMinBTFoundName = (TextView) mMinView.findViewById(R.id.case_bt_found_name);

        bluetoothAdapter = new BluetoothViewAdapter();
        mMaxBTSearchResultLV.setAdapter(bluetoothAdapter);
        mBluetoothManager.setOnBluetoothSearchListener(this);
    }

    public CaseBluetooth(Context context, XmlPullParser xmlParser) {
        this(context);
        btpair = xmlParser.getAttributeValue(null,"btpair");
        if(btpair == null){
            btpair = "false";
        } else if(btpair.equals("true")){
            mReadPrivateJNI.nativeSetParameter("btpair", "start");
        }
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase");
        initView();
        mBluetoothManager.statDiscovery();

    }

    @Override
    public void onStopCase() {
        Log.w(TAG,"onStopCase");
        mBluetoothManager.stopDiscovery();
    }

    @Override
    public void onSearchStart(List<BluetoothDeviceInfo> deviceList) {
        bluetoothAdapter.setDeviceList(deviceList);
        String status = mContext.getString(R.string.case_bt_status) + "[搜索中...]";
        mMaxBTSearchStatusTV.setText(status);
        mMinBTStatus.setText(status);
        updateView(deviceList);
        if(deviceList.size()>0)
            setDialogPositiveButtonEnable(true);
        else
            setDialogPositiveButtonEnable(false);
    }

    @Override
    public void onSearching(List<BluetoothDeviceInfo> deviceList) {
        bluetoothAdapter.setDeviceList(deviceList);
        String status = mContext.getString(R.string.case_bt_status) + "[搜索中...]";
        mMaxBTSearchStatusTV.setText(status);
        mMinBTStatus.setText(status);
        Log.i(TAG, "onSearching size=" + deviceList.size());
        updateView(deviceList);
        if(deviceList.size()>0)
            setDialogPositiveButtonEnable(true);
        else
            setDialogPositiveButtonEnable(false);
    }

    @Override
    public void onSearchEnd(List<BluetoothDeviceInfo> deviceList) {
        bluetoothAdapter.setDeviceList(deviceList);
        String status = mContext.getString(R.string.case_bt_status) + "[搜索结束]";
        mMaxBTSearchStatusTV.setText(status);
        mMinBTStatus.setText(status);
        if (deviceList.size() == 0) {
            mBluetoothManager.statDiscovery();
        } else {
            setCaseResult(true);
        }
        updateView(deviceList);
        Log.i(TAG, "onSearchEnd size=" + deviceList.size());
        if(deviceList.size()>0)
            setDialogPositiveButtonEnable(true);
        else
            setDialogPositiveButtonEnable(false);
    }


    private class BluetoothViewAdapter extends BaseAdapter {
        private List<BluetoothDeviceInfo> mDeviceList = new ArrayList<BluetoothDeviceInfo>();
        private class ViewHolder {
            TextView bluetoothName;
            TextView strength;
        }

        public void setDeviceList(List<BluetoothDeviceInfo> deviceList){
            this.mDeviceList = deviceList;
            notifyDataSetChanged();
        }

        @Override
        public int getCount() {
            return mDeviceList.size();
        }

        @Override
        public Object getItem(int position) {

            return mDeviceList.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convert, ViewGroup root) {
            if (convert == null) {
                convert = LayoutInflater.from(mContext).inflate(R.layout.case_bluetooth_item, null);
                ViewHolder holder = new ViewHolder();
                holder.bluetoothName = (TextView) convert.findViewById(R.id.case_bluetooth_name);
                holder.strength = (TextView) convert
                        .findViewById(R.id.case_bluetooth_strength);
                convert.setTag(holder);
            }
            ViewHolder holder = (ViewHolder) convert.getTag();
            holder.bluetoothName.setText(mDeviceList.get(position).getmBuletoothDevice().getName());
            short rssi = mDeviceList.get(position).getRSSI();
            if (rssi != 0) {
                holder.strength.setText(rssi + "");
            } else {
                holder.strength.setText("已配对");
            }

            return convert;
        }
    }

    @Override
    public void reset() {
        super.reset();
        initView();
    }

    private void initView(){
        String address = "null";
        if(BluetoothAdapter.getDefaultAdapter()!=null){
            address = BluetoothAdapter.getDefaultAdapter().getAddress();
        }
        mMinBTAddr.setText(mContext.getString(R.string.case_bt_addr) + "[" + address + "]");
        mMinBTStatus.setText(R.string.case_bt_status);
        mMinBTFoundNum.setText(R.string.case_bt_total_found);
        mMinBTFoundName.setText(R.string.case_bt_found_name);
    }

    private void updateView(List<BluetoothDeviceInfo> deviceList){
        mMinBTFoundNum.setText(mContext.getString(R.string.case_bt_total_found) + "["+ deviceList.size() +"]");
        String name = "";
        if (deviceList.size() > 0) {
            BluetoothDeviceInfo info = deviceList.get(0);
            if (info != null) {
                name = "[" + info.getmBuletoothDevice().getName() + "]";
            }
        }
        mMinBTFoundName.setText(mContext.getString(R.string.case_bt_found_name) + name);
    }
}

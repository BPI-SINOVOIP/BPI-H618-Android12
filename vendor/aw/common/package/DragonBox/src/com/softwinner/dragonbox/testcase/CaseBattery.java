package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.util.Log;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.widget.TextView;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.manager.LedManager;

public class CaseBattery extends IBaseCase {
    public static final String TAG = "DragonBox-CaseBattery";
    Context mContext;
    private TextView mMaxStatusBatteryInfo;
    private TextView mMaxPluggedBatteryInfo;
    private TextView mMaxLevelBatteryInfo;
    private TextView mMaxVoltageBatteryInfo;
    private TextView mMaxTemperatureBatteryInfo;
    private TextView mMaxTechnologyBatteryInfo;
    private TextView mMaxHealthBatteryInfo;
    private TextView mMaxPresentBatteryInfo;

    private TextView mMinStatusBatteryInfo;
    private TextView mMinPluggedBatteryInfo;
    private TextView mMinLevelBatteryInfo;
    private TextView mMinVoltageBatteryInfo;
    private TextView mMinTemperatureBatteryInfo;
    private TextView mMinTechnologyBatteryInfo;
    private TextView mMinHealthBatteryInfo;
    private TextView mMinPresentBatteryInfo;

    private int mLevel = 0;//电量低于mLevel，判断测试失败,单位%，例如30%
    private int mVoltage = 0;//电压低于mVoltage，判断测试失败，单位mV，例如3700mV


    public CaseBattery(Context context) {
        super(context, R.string.case_battery_name, R.layout.case_battery_max,
                R.layout.case_battery_min, TYPE_MODE_AUTO);
        mContext = context;
        initView();
    }

    public CaseBattery(Context context, XmlPullParser xmlParser) {
        this(context);
        String tempLevel = xmlParser.getAttributeValue(null, "level");
        Log.w(TAG, "set level: "+tempLevel);
        if(tempLevel!=null) {
            mLevel = Integer.parseInt(tempLevel);
        }
        String tempVoltage = xmlParser.getAttributeValue(null, "voltage");
        Log.w(TAG, "set voltage: "+tempVoltage);
        if(tempVoltage!=null) {
            mVoltage = Integer.parseInt(tempVoltage);
        }
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"CaseBattery onStartCase!");
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_BATTERY_CHANGED);
        mContext.registerReceiver(mBroadcastReceiver, intentFilter);
        setDialogPositiveButtonEnable(false);
    }

    public void initView() {
        mMaxStatusBatteryInfo=(TextView)mMaxView.findViewById(R.id.case_battery_status);
        mMinStatusBatteryInfo=(TextView)mMinView.findViewById(R.id.case_b_status);

        mMaxPluggedBatteryInfo=(TextView)mMaxView.findViewById(R.id.case_battery_plugged);
        mMinPluggedBatteryInfo=(TextView)mMinView.findViewById(R.id.case_b_plugged);

        mMaxLevelBatteryInfo=(TextView)mMaxView.findViewById(R.id.case_battery_level);
        mMinLevelBatteryInfo=(TextView)mMinView.findViewById(R.id.case_b_level);

        mMaxVoltageBatteryInfo=(TextView)mMaxView.findViewById(R.id.case_battery_voltage);
        mMinVoltageBatteryInfo=(TextView)mMinView.findViewById(R.id.case_b_voltage);

        mMaxTemperatureBatteryInfo=(TextView)mMaxView.findViewById(R.id.case_battery_temperature);
        mMinTemperatureBatteryInfo=(TextView)mMinView.findViewById(R.id.case_b_temperature);

        mMaxTechnologyBatteryInfo=(TextView)mMaxView.findViewById(R.id.case_battery_technology);
        mMinTechnologyBatteryInfo=(TextView)mMinView.findViewById(R.id.case_b_technology);

        mMaxHealthBatteryInfo=(TextView)mMaxView.findViewById(R.id.case_battery_health);
        mMinHealthBatteryInfo=(TextView)mMinView.findViewById(R.id.case_b_health);

        mMaxPresentBatteryInfo=(TextView)mMaxView.findViewById(R.id.case_battery_present);
        mMinPresentBatteryInfo=(TextView)mMinView.findViewById(R.id.case_b_present);
    }
    BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            boolean testResult = true;
            String action = intent.getAction();
            Log.e(TAG,"receive broadcast,action is "+action);
            if(action.equals(Intent.ACTION_BATTERY_CHANGED)) {
                int status = intent.getIntExtra("status", 0);
                int health = intent.getIntExtra("health", 0);
                int level = intent.getIntExtra("level", 0);
                int scale = intent.getIntExtra("scale", 0);
                int plugged = intent.getIntExtra("plugged", 0);
                int voltage = intent.getIntExtra("voltage", 0);
                int temperature = intent.getIntExtra("temperature", 0);
                boolean present = intent.getBooleanExtra("present", false);
                String technology = intent.getStringExtra("technology");
                Log.e(TAG, "status: "+status+"\n"+
                        "health: "+health+"\n"+
                        "level: "+level+"\n"+
                        "scale: "+scale+"\n"+
                        "plugged: "+plugged+"\n"+
                        "voltage: "+voltage+"\n"+
                        "temperature: "+temperature+"\n"+
                        "technology: "+technology+"\n"+
                        "present: "+present);
                mMaxVoltageBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_voltage_textview,voltage+"mV"));
                mMinVoltageBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_voltage_textview,voltage+"mV"));

                mMaxTemperatureBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_temperature_textview,((float)temperature/10)+"℃"));
                mMinTemperatureBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_temperature_textview,((float)temperature/10)+"℃"));

                mMaxTechnologyBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_technology_textview,technology));
                mMinTechnologyBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_technology_textview,technology));

                mMaxPresentBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_present_textview,present));
                mMinPresentBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_present_textview,present));

                mMaxLevelBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_level_textview,((float)level/scale*100+"%")));
                mMinLevelBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_level_textview,((float)level/scale*100+"%")));

                switch (status) {
                    case BatteryManager.BATTERY_STATUS_UNKNOWN:
                        mMaxStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_UNKNOWN)));
                        mMinStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_UNKNOWN)));
                        break;
                    case BatteryManager.BATTERY_STATUS_CHARGING:
                        mMaxStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_CHARGING)));
                        mMinStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_CHARGING)));
                        break;
                    case BatteryManager.BATTERY_STATUS_DISCHARGING:
                        mMaxStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_DISCHARGING)));
                        mMinStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_DISCHARGING)));
                        break;
                    case BatteryManager.BATTERY_STATUS_NOT_CHARGING:
                        mMaxStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_NOT_CHARGING)));
                        mMinStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_NOT_CHARGING)));
                        break;
                    case BatteryManager.BATTERY_STATUS_FULL:
                        mMaxStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_FULL)));
                        mMinStatusBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_status_textview, mContext.getString(R.string.BatteryInfo_status_FULL)));
                        break;

                    default:
                        break;
                }
                switch (health) {
                    case BatteryManager.BATTERY_HEALTH_UNKNOWN:
                        mMaxHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_UNKOWN)));
                        mMinHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_UNKOWN)));
                        break;
                    case BatteryManager.BATTERY_HEALTH_GOOD:
                        mMaxHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_GOOD)));
                        mMinHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_GOOD)));
                        break;
                    case BatteryManager.BATTERY_HEALTH_OVERHEAT:
                        mMaxHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_OVERHEAT)));
                        mMinHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_OVERHEAT)));
                        break;
                    case BatteryManager.BATTERY_HEALTH_DEAD:
                        mMaxHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_DEAD)));
                        mMinHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_DEAD)));
                        break;
                    case BatteryManager.BATTERY_HEALTH_OVER_VOLTAGE:
                        mMaxHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_VOLTAGE)));
                        mMinHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_VOLTAGE)));
                        break;
                    case BatteryManager.BATTERY_HEALTH_UNSPECIFIED_FAILURE:
                        mMaxHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_UNSPECIFIED_FAILURE)));
                        mMinHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_UNSPECIFIED_FAILURE)));
                        break;
                    case BatteryManager.BATTERY_HEALTH_COLD:
                        mMaxHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_OVERCOLD)));
                        mMinHealthBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_health_textview, mContext.getString(R.string.BatteryInfo_health_OVERCOLD)));
                        break;

                    default:
                        break;
                }
                switch (plugged) {
                    case BatteryManager.BATTERY_PLUGGED_AC:
                        mMaxPluggedBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_plugged_textview, mContext.getString(R.string.BatteryInfo_plugged_AC)));
                        mMinPluggedBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_plugged_textview, mContext.getString(R.string.BatteryInfo_plugged_AC)));
                        break;
                    case BatteryManager.BATTERY_PLUGGED_USB:
                        mMaxPluggedBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_plugged_textview, mContext.getString(R.string.BatteryInfo_plugged_USB)));
                        mMinPluggedBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_plugged_textview, mContext.getString(R.string.BatteryInfo_plugged_USB)));
                        break;
                    default:
                        mMaxPluggedBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_plugged_textview, mContext.getString(R.string.BatteryInfo_plugged_NULL)));
                        mMinPluggedBatteryInfo.setText(mContext.getString(R.string.BatteryInfo_plugged_textview, mContext.getString(R.string.BatteryInfo_plugged_NULL)));
                        break;
                }

                if(present==false) {//电池不存在，测试失败。
                    testResult = false;
                }
                if(status==BatteryManager.BATTERY_STATUS_UNKNOWN) {//电池状态未知，暂定测试失败
                    testResult = false;
                }
                if(health!=BatteryManager.BATTERY_HEALTH_GOOD) {//电池健康是good以外的其他状态，均认为测试失败
                    testResult = false;
                }
                if((float)level/scale*100 < (float)mLevel) {//默认电池电量<30%,认为测试失败。
                    testResult = false;
                }
                if(voltage<mVoltage) {//默认电压<3700mV,认为测试失败
                    testResult = false;
                }
                stopCase();
                setCaseResult(testResult);
                Log.w(TAG, "CaseBattery test over, test result is "+testResult);
                setDialogPositiveButtonEnable(testResult);
            }
        }
    };
    @Override
    public void onStopCase() {
        Log.w(TAG,"on stopcase,unregisterReceiver broadcastreceiver");
        mContext.unregisterReceiver(mBroadcastReceiver);
    }

    @Override
    public void reset() {
        super.reset();
    }

}

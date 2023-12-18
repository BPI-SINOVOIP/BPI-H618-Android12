package com.softwinner.dragonbox.testcase;

import java.io.File;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoWcdma;
import android.telephony.CellSignalStrengthCdma;
import android.telephony.CellSignalStrengthGsm;
import android.telephony.CellSignalStrengthLte;
import android.telephony.CellSignalStrengthWcdma;
import android.telephony.PhoneStateListener;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;
import android.widget.TextView;
import android.util.Log;

import com.softwinner.dragonbox.R;

public class CaseSIM extends IBaseCase {
    public static final String TAG = "DragonBox-CaseSIM";
    public static final int WHAT_SET_DBM = 0;
    public static final int WHAT_TEST_TIME_OVER = 1;
    public static final int WHAT_SIM_PRESENT = 2;
    public static final int TEST_TIME = 55;//在规定时间内如果信号强度不满足测试要求，则测试失败
    public static final int AVERAGE_NUMBER = 2;
    public static final int DBM_LEGAL_UP = -2147483647;//DBM合法性上限
    public static final int DBM_LEGAL_DOWN = -1;//DBM合法性下限
    private Context mContext;
    TextView mMaxTVSimState;
    TextView mMaxTVSignalStrength;
    TextView mMaxTVOperatorName;
    TextView mMaxTVPhoneNumber;
    TextView mMaxTVIMSI;
    TextView mMaxTVSerialNumber;

    TextView mMinTVSimState;
    TextView mMinTVSignalStrength;
    TextView mMinTVOperatorName;
    TextView mMinTVPhoneNumber;
    TextView mMinTVIMSI;
    TextView mMinTVSerialNumber;
    boolean mSimPresent = false;
    TelephonyManager mTelephonyManager;
    int mThresholdSignalStrength;//信号设定的临界值(建议-90dBm)，大于该值，信号良好，测试通过
    int mNowSignalStrength = -110;//当前信号大小,-1000代表无信号
    String mIMSI;
    boolean mTestResult = false;//测试结果默认false
    LinkedList<Integer> mListSignalStrength = new LinkedList<Integer>();

    public CaseSIM(Context context) {
        super(context, R.string.case_sim_name, R.layout.case_sim_max,
                R.layout.case_sim_min, TYPE_MODE_AUTO);
        mContext = context;
    }

    public CaseSIM(Context context, XmlPullParser xmlParser) {
        this(context);
        String tempLevel = xmlParser.getAttributeValue(null, "signalstrength");
        Log.w(TAG, "set level: "+tempLevel);
        if(tempLevel!=null) {
            mThresholdSignalStrength = Integer.parseInt(tempLevel);
        }
        mMaxTVSimState = (TextView)mMaxView.findViewById(R.id.case_sim_state);
        mMaxTVSignalStrength = (TextView)mMaxView.findViewById(R.id.case_sim_signal_strength);
        mMaxTVOperatorName = (TextView)mMaxView.findViewById(R.id.case_sim_operator_name);
        mMaxTVPhoneNumber = (TextView)mMaxView.findViewById(R.id.case_sim_phone_number);
        mMaxTVIMSI = (TextView)mMaxView.findViewById(R.id.case_sim_imsi);
        mMaxTVSerialNumber = (TextView)mMaxView.findViewById(R.id.case_sim_serial_number);

        mMinTVSimState = (TextView)mMinView.findViewById(R.id.case_s_state);
        mMinTVSignalStrength = (TextView)mMinView.findViewById(R.id.case_s_signal_strength);
        mMinTVOperatorName = (TextView)mMinView.findViewById(R.id.case_s_operator_name);
        mMinTVPhoneNumber = (TextView)mMinView.findViewById(R.id.case_s_phone_number);
        mMinTVIMSI = (TextView)mMinView.findViewById(R.id.case_s_imsi);
        mMinTVSerialNumber = (TextView)mMinView.findViewById(R.id.case_s_serial_number);
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseSim");
        setDialogPositiveButtonEnable(false);
        mTelephonyManager = (TelephonyManager)mContext.getSystemService(Context.TELEPHONY_SERVICE);
        mMaxTVSignalStrength.setText(mContext.getString(R.string.phone_signal_strength, -1000));
        mMinTVSignalStrength.setText(mContext.getString(R.string.phone_signal_strength, -1000));
        mHandler.sendEmptyMessage(WHAT_SIM_PRESENT);
        mHandler.sendEmptyMessageDelayed(WHAT_TEST_TIME_OVER, TEST_TIME*1000);
    }
    /**
     *获取信号强度有很多种方法，暂时不用这种方法，因为使用思东的电话卡测试，tm.getAllCellInfo总是返回null
     */
    public void getMobileDbm()
    {
        int dbm = -1;
        TelephonyManager tm = (TelephonyManager)mContext.getSystemService(Context.TELEPHONY_SERVICE);
        List<CellInfo> cellInfoList = tm.getAllCellInfo();
        Log.e(TAG, "1");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
        {
            Log.e(TAG, "2");
            if (null != cellInfoList)
            {
                Log.e(TAG, "4");
                for (CellInfo cellInfo : cellInfoList)
                {
                    Log.e(TAG, "6");
                    if (cellInfo instanceof CellInfoGsm)
                    {
                        CellSignalStrengthGsm cellSignalStrengthGsm = ((CellInfoGsm)cellInfo).getCellSignalStrength();
                        dbm = cellSignalStrengthGsm.getDbm();
                        Log.e(TAG, "cellSignalStrengthGsm" + cellSignalStrengthGsm.toString());
                    }
                    else if (cellInfo instanceof CellInfoCdma)
                    {
                        CellSignalStrengthCdma cellSignalStrengthCdma = ((CellInfoCdma)cellInfo).getCellSignalStrength();
                        dbm = cellSignalStrengthCdma.getDbm();
                        Log.e(TAG, "cellSignalStrengthCdma" + cellSignalStrengthCdma.toString() );
                    }
                    else if (cellInfo instanceof CellInfoWcdma)
                    {
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
                        {
                            CellSignalStrengthWcdma cellSignalStrengthWcdma = ((CellInfoWcdma)cellInfo).getCellSignalStrength();
                            dbm = cellSignalStrengthWcdma.getDbm();
                            Log.e(TAG, "cellSignalStrengthWcdma" + cellSignalStrengthWcdma.toString() );
                        }
                    }
                    else if (cellInfo instanceof CellInfoLte)
                    {
                        CellSignalStrengthLte cellSignalStrengthLte = ((CellInfoLte)cellInfo).getCellSignalStrength();
                        dbm = cellSignalStrengthLte.getDbm();
                        Log.e(TAG, "cellSignalStrengthLte.getAsuLevel()\t" + cellSignalStrengthLte.getAsuLevel() );
                        Log.e(TAG, "cellSignalStrengthLte.getDbm()\t " + cellSignalStrengthLte.getDbm() );
                        Log.e(TAG, "cellSignalStrengthLte.getLevel()\t " + cellSignalStrengthLte.getLevel() );
                        Log.e(TAG, "cellSignalStrengthLte.getTimingAdvance()\t " + cellSignalStrengthLte.getTimingAdvance() );
                    }
                }
            }else{
                Log.e(TAG, "5");
            }
        }else{
            Log.e(TAG, "3");
        }
    }


    PhoneStateListener mPhoneStateListener = new PhoneStateListener() {
        @Override
        public void onSignalStrengthsChanged(SignalStrength signalStrength) {
            super.onSignalStrengthsChanged(signalStrength);

            String simOperatorName = mTelephonyManager.getSimOperatorName();
            mMaxTVOperatorName.setText(mContext.getString(R.string.phone_sim_operator_name, simOperatorName));
            mMinTVOperatorName.setText(mContext.getString(R.string.phone_sim_operator_name, simOperatorName));
            String phoneNumber = mTelephonyManager.getLine1Number();
            mMaxTVPhoneNumber.setText(mContext.getString(R.string.phone_sim_phone_number, phoneNumber));
            mMinTVPhoneNumber.setText(mContext.getString(R.string.phone_sim_phone_number, phoneNumber));
            mIMSI = mTelephonyManager.getSubscriberId();
            mMaxTVIMSI.setText(mContext.getString(R.string.phone_sim_imsi, mIMSI));
            mMinTVIMSI.setText(mContext.getString(R.string.phone_sim_imsi, mIMSI));
            String serialNumber = mTelephonyManager.getSimSerialNumber();
            mMaxTVSerialNumber.setText(mContext.getString(R.string.phone_sim_serial_number, serialNumber));
            mMinTVSerialNumber.setText(mContext.getString(R.string.phone_sim_serial_number, serialNumber));

            String signalInfo = signalStrength.toString();
            Log.w(TAG,"signalInfo is "+signalInfo);
            String[] params = signalInfo.split(" ");

            Log.w(TAG,"net workType is "+mTelephonyManager.getNetworkType());
            Log.w(TAG,"signal is gsm:"+signalStrength.isGsm()+"\n level is "+signalStrength.getLevel()+"\n cdma level is "+signalStrength.getCdmaLevel());
            Log.w(TAG,"signal dbm is "+signalStrength.getDbm());
            int firstDbm =  signalStrength.getDbm();
            setDBM(""+firstDbm);

            if (mTelephonyManager.getNetworkType() == TelephonyManager.NETWORK_TYPE_LTE) {
                // 4G网络 最佳范围 >-90dBm 越大越好
                int Itedbm = Integer.parseInt(params[9]);
                Itedbm = -1*Math.abs(Itedbm);
                Log.w(TAG, "signal strength is 4G "+Itedbm +"dBm");
                setDBM(""+Itedbm);
            } else if (mTelephonyManager.getNetworkType() == TelephonyManager.NETWORK_TYPE_HSDPA
                    || mTelephonyManager.getNetworkType() == TelephonyManager.NETWORK_TYPE_HSPA
                    || mTelephonyManager.getNetworkType() == TelephonyManager.NETWORK_TYPE_HSUPA
                    || mTelephonyManager.getNetworkType() == TelephonyManager.NETWORK_TYPE_UMTS) {
                // 3G网络最佳范围 >-90dBm 越大越好 ps:中国移动3G获取不到 返回的无效dbm值是正数（85dbm）
                // 在这个范围的已经确定是3G，但不同运营商的3G有不同的获取方法，故在此需做判断 判断运营商与网络类型的工具类在最下方
                if (mIMSI == null) {
                    setDBM("-1000");
                } else if (mIMSI.startsWith("46000") || mIMSI.startsWith("46002") || mIMSI.startsWith("46007")) {
                    // 中国移动
                    setDBM(0 + "");// 中国移动3G不可获取，故在此返回0
                    Log.w(TAG, "signal strength is 3G 移动 "+0 +"dbm");
                } else if (mIMSI.startsWith("46001")) {
                    // 中国联通
                    int cdmaDbm = signalStrength.getCdmaDbm();
                    cdmaDbm = -1*Math.abs(cdmaDbm);
                    setDBM(""+cdmaDbm);
                    Log.w(TAG, "signal strength is 3G 联通 "+cdmaDbm +"dbm");
                } else if (mIMSI.startsWith("46003")) {
                    // 中国电信
                    int evdoDbm = signalStrength.getEvdoDbm();
                    evdoDbm = -1*Math.abs(evdoDbm);
                    setDBM(evdoDbm + "");
                    Log.w(TAG, "signal strength is 3G 电信 "+evdoDbm +"dbm");
                } else {
                    setDBM("-1000");
                }
            }else{
                // 2G网络最佳范围>-90dBm 越大越好
                int asu = signalStrength.getGsmSignalStrength();
                int dbm = -113 + 2 * asu;
                dbm = -1*Math.abs(dbm);
                setDBM(dbm + "");
                Log.w(TAG, "signal strength is 2G "+dbm +"dbm");
            }
        }
    };
    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case WHAT_SET_DBM:
                    mListSignalStrength.add(msg.arg1);
                    if (mListSignalStrength.size() > AVERAGE_NUMBER) {
                        mListSignalStrength.poll();
                    }

                    long sum = 0;
                    for (int i = 0; i < mListSignalStrength.size(); i++) {
                        Log.e(TAG, "mListSignalStrength 0 is "+mListSignalStrength.get(i));
                        sum += mListSignalStrength.get(i);
                    }
                    long average = sum / mListSignalStrength.size();
                    Log.e(TAG, "average signal strength is " + average + "dbm");
                    mMaxTVSignalStrength.setText(mContext.getString(R.string.phone_signal_strength, average));
                    mMinTVSignalStrength.setText(mContext.getString(R.string.phone_signal_strength, average));
                    if (average >= mThresholdSignalStrength) {
                        mTestResult = true;
                        setDialogPositiveButtonEnable(mTestResult);
                        if(mHandler.hasMessages(WHAT_TEST_TIME_OVER)) {
                            Log.w(TAG, "remove message WHAT_TEST_TIME_OVER");
                            mHandler.removeMessages(WHAT_TEST_TIME_OVER);
                        }else {
                            Log.w(TAG,"WHAT_TEST_TIME_OVER msg does not exists in mHandler");
                        }
                        stopCase();
                    } else {
                        mTestResult = false;
                    }

                    break;
                case WHAT_SIM_PRESENT:
                    Log.w(TAG, "sim state "+mTelephonyManager.getSimState());
                    Log.w(TAG, "simOperatorName is "+mTelephonyManager.getSimOperatorName());
                    Log.w(TAG, "line number is "+mTelephonyManager.getLine1Number());
                    Log.w(TAG,"imsi is "+mTelephonyManager.getSubscriberId());
                    Log.w(TAG,"sim serial number is "+mTelephonyManager.getSimSerialNumber());

                    int simState = mTelephonyManager.getSimState();
                    String simOperatorName = mTelephonyManager.getSimOperatorName();
                    String phoneNumber = mTelephonyManager.getLine1Number();
                    mIMSI = mTelephonyManager.getSubscriberId();
                    String serialNumber = mTelephonyManager.getSimSerialNumber();

                    switch (simState) {
                        case TelephonyManager.SIM_STATE_READY:
                            mMaxTVSimState.setText(mContext.getString(R.string.phone_sim_state, mContext.getString(R.string.phone_sim_ready)));
                            mMinTVSimState.setText(mContext.getString(R.string.phone_sim_state, mContext.getString(R.string.phone_sim_ready)));
                            mSimPresent = true;
                            break;
                        case TelephonyManager.SIM_STATE_ABSENT:
                        case TelephonyManager.SIM_STATE_UNKNOWN:
                            mMaxTVSimState.setText(mContext.getString(R.string.phone_sim_state, mContext.getString(R.string.phone_sim_not_found)));
                            mMinTVSimState.setText(mContext.getString(R.string.phone_sim_state, mContext.getString(R.string.phone_sim_not_found)));
                            break;
                        default:
                            mMaxTVSimState.setText(mContext.getString(R.string.phone_sim_state, mContext.getString(R.string.phone_sim_exception)));
                            mMinTVSimState.setText(mContext.getString(R.string.phone_sim_state, mContext.getString(R.string.phone_sim_exception)));
                            break;
                    }

                    if(mSimPresent&&phoneNumber != null) {
                        mTelephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_SIGNAL_STRENGTHS|
                                PhoneStateListener.LISTEN_SERVICE_STATE|PhoneStateListener.LISTEN_DATA_CONNECTION_STATE);
                    }else {
                        mHandler.sendEmptyMessageDelayed(WHAT_SIM_PRESENT, 1000);
                    }
                    break;
                case WHAT_TEST_TIME_OVER:
                    Log.w(TAG,"sim test time over,stop case");
                    if(mHandler.hasMessages(WHAT_SIM_PRESENT)) {
                        Log.w(TAG, "remove message WHAT_SIM_PRESENT");
                        mHandler.removeMessages(WHAT_SIM_PRESENT);
                    }
                    mTestResult = false;
                    stopCase();
                    break;

                default:
                    break;
            }
        }
    };
    private void setDBM(String dbm) {
        Log.w(TAG, "get signal strength: "+dbm+"dbm");
        mNowSignalStrength = Integer.parseInt(dbm);
        if(mNowSignalStrength>DBM_LEGAL_UP&&mNowSignalStrength<DBM_LEGAL_DOWN) {
            Log.w(TAG,"dbm is legal, mNowSignalStrength is "+mNowSignalStrength);
            Message msg = Message.obtain();
            msg.what = WHAT_SET_DBM;
            msg.arg1 = mNowSignalStrength;
            mHandler.sendMessage(msg);
        }
    }

    @Override
    public void onStopCase() {
        Log.w(TAG, "sim onStopCase,test result is "+mTestResult);
        if (mSimPresent) {
            //取消监听
            mTelephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_NONE);
        }
        setCaseResult(mTestResult);
    }

    @Override
    public void reset() {
        super.reset();
    }

}

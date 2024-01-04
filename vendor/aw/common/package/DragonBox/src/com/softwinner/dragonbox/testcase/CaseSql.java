package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.widget.TextView;
import android.util.Log;

import com.softwinner.dragonbox.ui.DragonBoxMain;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.entity.TestResultforSQLInfo;

public class CaseSql extends IBaseCase {
    public static final String TAG = "DragonBox-CaseSql";
    TextView mMinSqlStatus;
    TextView mMaxSqlStatus;
    Context mContext;
    //sql config
    String mDBServer = "192.168.1.11:3306";
    String mDBname = "m20c";
    String mDBtable = "b1_number_up";
    String mDBAccount = "test";
    String mDBpassword = "12345678";
    public static SQLConfig mSQLConfig = null;

    public CaseSql(Context context) {
        super(context, R.string.case_sql_name, R.layout.case_sql_max,
                R.layout.case_sql_min, TYPE_MODE_AUTO);
        mMinSqlStatus = (TextView) mMinView.findViewById(R.id.case_sql_min_tv);
        mMaxSqlStatus = (TextView) mMaxView.findViewById(R.id.case_sql_max_tv);
        mContext = context;
    }

    public CaseSql(Context context, XmlPullParser xmlParser) {
        this(context);
        String dbServer = xmlParser.getAttributeValue(null,"dbserver");
        if(dbServer!=null) {
            mDBServer = dbServer;
        }
        String dbName = xmlParser.getAttributeValue(null,"dbname");
        if(dbName!=null) {
            mDBname = dbName;
        }
        String dbTable = xmlParser.getAttributeValue(null,"dbtable");
        if(dbTable!=null) {
            mDBtable = dbTable;
        }
        String dbAccount = xmlParser.getAttributeValue(null,"dbaccount");
        if(dbAccount!=null) {
            mDBAccount = dbAccount;
        }
        String dbPassword = xmlParser.getAttributeValue(null,"dbpassword");
        if(dbPassword!=null) {
            mDBpassword = dbPassword;
        }
        Log.w(TAG,"sql config\n dbserver is "+dbServer+
                "\n dbname is "+dbName+
                "\n dbtable is "+dbTable+
                "\n dbaccount is "+dbAccount+
                "\n dbpassword is "+dbPassword);
    }

    public static final SQLConfig getSQLConfig() {
        if(mSQLConfig == null) {
            mSQLConfig = new SQLConfig();
        }
        return mSQLConfig;
    }

    @Override
    public void onStartCase() {
        Log.w(TAG,"onStartCase CaseSql");
        mSQLConfig = getSQLConfig();
        mSQLConfig.mDBAccount = mDBAccount;
        mSQLConfig.mDBname = mDBname;
        mSQLConfig.mDBpassword = mDBpassword;
        mSQLConfig.mDBServer = mDBServer;
        mSQLConfig.mDBtable = mDBtable;
        mMinSqlStatus.setText(mContext.getString(R.string.case_sql_info, mDBServer, mDBname, mDBtable, mDBAccount, mDBpassword));
        mMaxSqlStatus.setText(mContext.getString(R.string.case_sql_info, mDBServer, mDBname, mDBtable, mDBAccount, mDBpassword));
        setCaseResult(true);
    }

    @Override
    public void onStopCase() {

    }

    @Override
    public void onSetResult() {
        super.onSetResult();
        TestResultforSQLInfo testResultforSQLInfo = DragonBoxMain.getTestResultforSQLInfo();
        testResultforSQLInfo.putOneInfo(testResultforSQLInfo.mLegalKey[20], getCaseResult()+"");//sql
    }

    @Override
    public void reset() {
        super.reset();
    }

    public static class SQLConfig{
        public String mDBServer = "192.168.1.11:3306";
        public String mDBname = "m20c";
        public String mDBtable = "b1_number_up";
        public String mDBAccount = "test";
        public String mDBpassword = "12345678";
    }
}

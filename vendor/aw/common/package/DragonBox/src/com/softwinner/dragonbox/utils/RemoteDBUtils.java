package com.softwinner.dragonbox.utils;

import android.annotation.SuppressLint;
import android.util.ArrayMap;
import android.util.Log;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.sql.Statement;

import com.softwinner.dragonbox.entity.TestResultforSQLInfo;
import com.softwinner.dragonbox.testcase.CaseSql.SQLConfig;

public class RemoteDBUtils {

    private static final String TAG = "DragonBox-RemoteDBUtils";
    private static final String SQL_CLASS = "com.mysql.jdbc.Driver";
    private static final String DATABASE_URL = "jdbc:mysql://%s/%s";

    private SQLConfig mSqlConfig;
    private static final String QUERY_COMMAND = "SELECT * FROM %s WHERE %s='%s'";
    private static final String COMMIT_COMMAND = "UPDATE %s SET %s WHERE %s='%s'";

    private Connection mConnection;

    public RemoteDBUtils(SQLConfig sqlConfig) {
    	mSqlConfig = sqlConfig;
    }

    static {
        try {
            Class.forName(SQL_CLASS);// 注册 JDBC 驱动
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
    }

    private Connection getConnection() throws SQLException {
        if (mConnection == null) {
            String serverURL = String.format(DATABASE_URL, mSqlConfig.mDBServer, mSqlConfig.mDBname);
            Log.w(TAG,"serverURL is "+serverURL);
            mConnection = DriverManager.getConnection(serverURL,
                    mSqlConfig.mDBAccount, mSqlConfig.mDBpassword);
            Log.d(TAG, "DateBase connection Success!");
        }
        return mConnection;
    }

    @SuppressLint("NewApi")
    public boolean updateData(TestResultforSQLInfo testResultforSQLInfo) {
        String commitCommand = "";
        StringBuilder updataDB = new StringBuilder();
        String primaryColumeKey = testResultforSQLInfo.getPrimaryKey();
        String primaryColumeValue = testResultforSQLInfo.getPrimaryValue();
        ArrayMap<String, String> arrResultInfo = testResultforSQLInfo.getALLResultInfo();
        for (int i = 0;i<arrResultInfo.size();i++) {
            String key = arrResultInfo.keyAt(i);
            String value = arrResultInfo.valueAt(i);
            // just like `a`='b',`c`='d',
            updataDB.append("`").append(key).append("`='").append(value).append("',");
        }
        
        if (updataDB.length() == 0 || primaryColumeValue == null || primaryColumeValue.equals("")) {
        	Log.w(TAG,"update data failed!");
            return false;
        }
        //just for remove the last ','
        updataDB.deleteCharAt(updataDB.length() - 1);
        Log.w(TAG,"updateDB is "+updataDB);

        commitCommand = String.format(COMMIT_COMMAND, mSqlConfig.mDBtable,
        		updataDB, primaryColumeKey,primaryColumeValue);

        Statement stmt = null;
        try {
            stmt = getConnection().createStatement();
            int count = stmt.executeUpdate(commitCommand);
            if (count == 0) {
                return false;
            }
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        } finally {
            try {
                if (stmt != null) {
                    stmt.close();
                }
                if (mConnection != null) {
                    mConnection.close();
                }
            } catch (SQLException e) {
                e.printStackTrace();
                return false;
            }
        }
        return true;
    }
}

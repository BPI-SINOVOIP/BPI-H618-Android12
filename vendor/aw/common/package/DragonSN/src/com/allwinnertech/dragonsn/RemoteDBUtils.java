package com.allwinnertech.dragonsn;

import android.util.Log;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.List;

import com.allwinnertech.dragonsn.entity.BindedColume;

public class RemoteDBUtils {

    private static final String TAG = "RemoteDBUtils";

    private Config mConfig;
    private static final String QUERY_COMMAND = "SELECT * FROM %s WHERE %s='%s'";
    private static final String COMMIT_COMMAND = "UPDATE %s SET %s WHERE %s='%s'";

    private Connection mConnection;

    public RemoteDBUtils(Config config) {
        mConfig = config;
    }

    static {
        try {
            Class.forName("net.sourceforge.jtds.jdbc.Driver");
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
    }

    private Connection getConnection() throws SQLException {
        if (mConnection == null) {
            mConnection = DriverManager.getConnection(mConfig.serverUrl,
                    mConfig.account, mConfig.password);
            Log.d(TAG, "DateBase connection Success!");
        }
        return mConnection;
    }

    public boolean queryData(List<BindedColume> bindedColume) {
        String queryCommand = "";
        for (BindedColume colume : bindedColume) {
            if (colume.isPrimaryKey()) {
                queryCommand = String.format(QUERY_COMMAND, mConfig.tableName,
                        colume.getColName(), colume.getPrimValue());
                break;
            }
        }

        if (queryCommand == null || "".equals(queryCommand)) {
            return false;
        }

        Statement stmt = null;
        ResultSet rs = null;
        try {
            stmt = getConnection().createStatement();
            rs = stmt.executeQuery(queryCommand);

            while (rs.next()) {
                for (BindedColume colume : bindedColume) {
                    String value = rs.getString(colume.getColName());
                    colume.setRemoteData(value.trim());
                }
            }
        } catch (SQLException e) {
            e.printStackTrace();
            return false;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        } finally {
            try {
                if (rs != null) {
                    rs.close();
                }
                if (stmt != null) {
                    stmt.close();
                }
            } catch (SQLException e) {
                e.printStackTrace();
                return false;
            }
        }

        return true;
    }

    public boolean updateResult(List<BindedColume> bindedColume) {
        String commitCommand = "";
        StringBuilder updataSB = new StringBuilder();
        BindedColume primColume = null;
        BindedColume resultColume = null;
        for (BindedColume colume : bindedColume) {
            if (colume.isPrimaryKey()) {
                primColume = colume;
            }
            // just like a='b',c='d',
            if (colume.isResultKey()) {
                resultColume = colume;
                updataSB.append(colume.getColName()).append("='")
                    .append(colume.getLocalData()).append("',");
            }
        }
        if (updataSB.length() == 0 || primColume == null) {
            return false;
        }
        //just for remove the last ','
        updataSB.deleteCharAt(updataSB.length() - 1);

        commitCommand = String.format(COMMIT_COMMAND, mConfig.tableName,
                updataSB, primColume.getColName(),
                primColume.getLocalData());

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
            } catch (SQLException e) {
                e.printStackTrace();
                return false;
            }
        }
        resultColume.setRemoteData("1");
        return true;
    }
}

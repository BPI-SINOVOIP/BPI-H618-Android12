package com.softwinner.timerswitch.utils;

import android.net.Uri;

public class Constant {

    public final static int DB_VERSION = 1;
    public final static String DB_NAME = "alarms.db";
    public final static String TABLE_NAME = "alarms";
    public final static int COLUMN_COUNT = 7;
    public final static String COLUMN_ID = "id";
    public final static String COLUMN_CREATE_TIMESTAMP = "createTimeStamp";
    public final static String COLUMN_DAYS_OF_WEEK = "daysOfWeek";
    public final static String COLUMN_REPEAT_DAYS_OF_WEEK = "repeatDaysOfWeek";
    public final static String COLUMN_HOUR = "hour";
    public final static String COLUMN_MINUTES = "minutes";
    public final static String COLUMN_ENABLED = "enable";
    public final static String COLUMN_LABEL = "label";
    public final static String COLUMN_ALERT_TIME = "alertTime";

    public static final Uri CONTENT_URI_INSERT = Uri.parse("content://com.softwinner.timerswitch/alarm/insert");
    public static final Uri CONTENT_URI_DELETE = Uri.parse("content://com.softwinner.timerswitch/alarm/delete");
    public static final Uri CONTENT_URI_UPDATE = Uri.parse("content://com.softwinner.timerswitch/alarm/update");
    public static final Uri CONTENT_URI_QUERY = Uri.parse("content://com.softwinner.timerswitch/alarm/query");

    public static final int MSG_INSERT = 0x01;
    public static final int MSG_DELETE = 0x02;
    public static final int MSG_UPDATE = 0x03;

}

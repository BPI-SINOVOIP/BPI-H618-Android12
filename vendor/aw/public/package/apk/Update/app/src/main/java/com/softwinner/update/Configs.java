package com.softwinner.update;

import android.os.Environment;

public class Configs {
    public static final String GROBLE_TAG = "SoftwinnerUpdater";

    public static final String SERVER_URL_USE_DOMAIN = "http://www.china-ota.com/ota/service/request";
    public static final String SERVER_URL_USE_IP = "http://42.121.236.55/ota/service/request";

    public static final String DOWNLOAD_PATH = Environment.getExternalStorageDirectory() + "/ota.zip";
    public static final boolean DEBUG = false;  //false
    public static final int CHECK_CYCLE_DAY = 1;

    public static final String ABUPDATE = "ab_update";
    public static final String UPDATEPATH = "update_path";
}

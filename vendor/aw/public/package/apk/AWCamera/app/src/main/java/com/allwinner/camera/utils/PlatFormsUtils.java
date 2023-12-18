package com.allwinner.camera.utils;

import android.content.Context;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;

import com.allwinner.camera.platforms.a100;
import com.allwinner.camera.platforms.a50;

import java.lang.reflect.Array;

public class PlatFormsUtils {
    public static boolean SUPPROT_4_3_RATIO_BACK = true;
    public static boolean SUPPORT_16_9_RATIO_BACK = true;
    public static boolean SUPPROT_4_3_RATIO_FRONT = true;
    public static boolean SUPPORT_16_9_RATIO_FRONT = false;
    public static String[] ARRAY_PICTURE_RATIO_BACK = {"4:3","16:9"};
    public static String[] ARRAY_PICTRUE_RATIO_FRONT = {"4:3"};
    public static String[] ARRAY_VIDEO_RATIO_BACK = {"480P (4:3)","720P (16:9)"};
    public static String[] ARRAY_VIDEO_RATIO_FRONT = {"480P (4:3)"};
    public static boolean ISPLATFORM_A50 = false;
    public static boolean ISPLATFORM_A100 = false;
    public static void init(Context context){
        String platform = SystemPropertyUtils.get(context,"ro.board.platform");
        Log.i("PlatFormsUtils","init platform: " + platform);
        if (!TextUtils.isEmpty(platform)) {
            switch (platform){
                case "a50":
                    a50.setPram();
                    break;
                case "ceres":
                    a100.setPram();
                    break;
                default:
                    break;
            }
        }
    }
}

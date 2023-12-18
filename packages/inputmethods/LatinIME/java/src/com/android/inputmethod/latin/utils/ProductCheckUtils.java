package com.android.inputmethod.latin.utils;

import android.util.Log;
import java.io.BufferedReader;
import java.io.InputStreamReader;

public class ProductCheckUtils {
    private static boolean mHomletProduct = false;
    public static boolean isHomlet() {
        return mHomletProduct;
    }

    public static boolean isTable() {
        return !mHomletProduct;
    }

    public static void checkProductType() {
        String line = null;
        BufferedReader input = null;
        try {
            Process p = Runtime.getRuntime().exec("getprop ro.build.characteristics");
            input = new BufferedReader(new InputStreamReader(p.getInputStream()), 1024);
            line = input.readLine();
            input.close();
        } catch (Exception e) {
            Log.d("Latin", "getprop error, use for table");
        }
        if (line != null) {
            if (line.equals("homlet"))
                mHomletProduct = true;
        }
    }

}

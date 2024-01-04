package com.softwinner.agingdragonbox.engine;

import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * TODO<时间工具类>
 *
 * @author maizirong
 * @data: 2014-4-8
 * @version: V1.0
 */

public class MyDate {
    public static String getFileName() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd");
        String date = format.format(new Date(System.currentTimeMillis()));
        return date;
    }

    public static String getDateEN() {
        SimpleDateFormat format1 = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        String date1 = format1.format(new Date(System.currentTimeMillis()));
        return date1;
    }
}


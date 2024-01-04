package com.softwinner.TvdVideo;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

public class MangerDatabase extends SQLiteOpenHelper {

    public static final String NAME = "breakpoint";
    public static final String PATH = "path";
    public static final String BOOKMARK = "bookmark";
    public static final String TIME = "time";
    public static final String SUBSAVE = "subsave";
    public static final String TRACKSAVE = "tracksave";
    public static final String SUBCOLORSAVE = "subcolorsave";
    public static final String SUBSIZESAVE = "subsizesave";

    private static final int VERSION = 1;

    public MangerDatabase(Context context) {
        super(context, NAME, null, VERSION);
        // TODO Auto-generated constructor stub
    }

    @Override
    public void onCreate(SQLiteDatabase arg0) {
        // TODO Auto-generated method stub
        Log.w("MangerDatabase", "*********** create MangerDatabase !");

        arg0.execSQL("CREATE TABLE IF NOT EXISTS " + NAME + " (" + PATH + " varchar PRIMARY KEY, "
                + BOOKMARK + " INTEGR, " + SUBSAVE + " INTEGR, " + TRACKSAVE + " INTEGR, "
                + SUBCOLORSAVE + " INTEGR, " + SUBSIZESAVE + " INTEGR, " + TIME + " LONG)");
    }

    @Override
    public void onUpgrade(SQLiteDatabase arg0, int arg1, int arg2) {
        // TODO Auto-generated method stub
        arg0.execSQL("DROP TABLE IF EXISTS " + NAME);
        onCreate(arg0);
    }

}

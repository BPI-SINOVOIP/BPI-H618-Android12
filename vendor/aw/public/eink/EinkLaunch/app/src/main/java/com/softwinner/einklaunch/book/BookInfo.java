package com.softwinner.einklaunch.book;

import java.io.File;

import com.softwinner.einklaunch.R;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.net.Uri;
import android.util.Log;
import android.widget.Toast;

public class BookInfo {
    private String title;
    private String path;
    private int currentPageNo;

    private String author;
    private int totalPage = -1;


    public BookInfo() {
    }

    public BookInfo(String title, String _path) {
        this.title = title;
        setPath(_path);
    }

    public void setPath(String path) {
        this.path = path;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    public String getPath() {
        return this.path;
    }

    public void setCurrentPageNo(int no) {
        this.currentPageNo = no;
    }

    public String getTitle() {
        return this.title;
    }

    public String path() {
        return this.path;
    }

    public int currentPageNo() {
        return this.currentPageNo;
    }

    public Bitmap getCover(Context context) {
        if (this.path == null) {
            return null;
        }

        Bitmap coverimg = null;

        if (checkForWPSFormat()) {
            if (coverimg == null) {
                coverimg = BitmapFactory.decodeResource(context.getResources(),
                        R.drawable.book_coverdefault);
            }
            return null;
        }

        Cursor cursor = context.getContentResolver().query(
                BookManager.COVERING_CONTENT_URI,
                null,
                "path=?",
                new String[]{this.path},
                null);
        if (cursor == null || cursor.getCount() == 0) {
            if (cursor != null)
                cursor.close();
            return BitmapFactory.decodeResource(context.getResources(),
                    R.drawable.book_coverdefault);
        }

        cursor.moveToFirst();
        int flag = cursor.getInt(cursor.getColumnIndex("defaultflag"));
        if (flag == 1) {
            cursor.close();
            return BitmapFactory.decodeResource(context.getResources(),
                    R.drawable.book_coverdefault);
        }

        byte[] byteArray = cursor.getBlob(cursor.getColumnIndex("coverimg"));
        coverimg = BitmapFactory.decodeByteArray(byteArray, 0, byteArray.length);


        cursor.close();

        return coverimg;

        //return zoomBitmap(coverimg, COVER_IMG_RATI0_WIDTH_HEIGH);
    }

    private void initAuthortAndPageNumber(Context context) {
        if (path == null) {
            return;
        }

        Cursor cursor = context.getContentResolver().query(
                BookManager.BOOK_INFO_CONTENT_URI,
                BookManager.BOOK_INFO_PROJECTION,
                "path=?",
                new String[]{this.path},
                null);

        if (cursor == null) {
            return;
        }

        if (cursor.moveToFirst()) {
            author = cursor.getString(cursor.getColumnIndex("author"));
            totalPage = cursor.getInt(cursor.getColumnIndex("totalpage"));
        }
        cursor.close();
    }

    public String getAuthor(Context context) {
        if (author == null) {
            initAuthortAndPageNumber(context);
        }
        return this.author;
    }

    public int getTotalPage(Context context) {
        if (totalPage <= 0) {
            initAuthortAndPageNumber(context);
        }
        return totalPage;

    }

    public boolean checkForWPSFormat() {
        int index = this.path.lastIndexOf(".");
        String format = this.path.substring(index + 1);

        if (format.equalsIgnoreCase("DOC") || format.equalsIgnoreCase("DOCX")
                || format.equalsIgnoreCase("PPT") || format.equalsIgnoreCase("XLS")) {
            return true;
        }

        return false;
    }

    public void startReading(Context context) {
        if (checkForWPSFormat()) {
            Log.i("book_info", "start reading wps file " + path);
            try {
                Intent intent = new Intent();
                ComponentName componentName = new ComponentName("cn.wps.moffice",
                        "cn.wps.moffice.documentmanager.PreStartActivity");
                intent.setComponent(componentName);
                intent.setAction(Intent.ACTION_VIEW);
                intent.setDataAndType(Uri.fromFile(new File(path)), "txt/plain");
                com.eink.launcher.Launcher.startActivitySafely(context, intent, null);
            } catch (ActivityNotFoundException e) {
                Toast.makeText(context, "Can not open wps file, please intall wps office!", Toast.LENGTH_LONG).show();
            }

        } else {
            Log.i("book_info", "start reading book " + path);
            Intent intent = new Intent();
            intent.setAction("com.allwinnertech.bookread");
            intent.setDataAndType(Uri.fromFile(new File(path)), "txt/plain");
            com.eink.launcher.Launcher.startActivitySafely(context, intent, null);
        }
    }

    private Bitmap zoomBitmap(Bitmap old, float ratio) {
        if (old == null || ratio == 0) {
            return null;
        }

        int w = old.getWidth();
        int h = old.getHeight();

        Log.i("book_cover", "ratio = " + ratio + " w=" + w + ",  h=" + h);

        float oldRatio = w / (float) h;

        if (ratio == oldRatio) {
            return old;
        } else if (ratio > oldRatio) {

            return Bitmap.createScaledBitmap(old, (int) (h / ratio), h, true);
        } else {
            return Bitmap.createScaledBitmap(old, w, (int) (w / ratio), true);
        }
        //return Bitmap.createScaledBitmap(old, 132, 180, true);
    }
}

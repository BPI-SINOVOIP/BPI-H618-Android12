package com.softwinner.einklaunch.book;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

public class BookManager {

    private final static String TAG = "BookManager";

    //最多显示最近阅读的Book的数量
    private final static int READ_NEW_DISPLAY_COUNT = 4;

    private Context mContext;

    public BookManager(Context context) {
        mContext = context;
    }

    public List<BookInfo> getReadNewBooks() {
        ArrayList<BookInfo> books = new ArrayList<BookInfo>();
        if (mContext == null) {
            return books;
        }
        Cursor cursor = null;
        try {
            cursor = mContext.getContentResolver().query(NEW_READ_CONTENT_URI,
                    READ_NEW_PROJECTION,
                    null,
                    null,
                    READ_NEW_ORDER_BY);
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (cursor == null) {
            return books;
        }

        int sum = READ_NEW_DISPLAY_COUNT;

        if (cursor.getCount() < 5) {
            sum = cursor.getCount();
        }

        for (int i = 0; i < sum; i++) {
            if (!cursor.moveToNext()) {
                break;
            }

            String name = cursor.getString(0);
            String path = cursor.getString(1);

            if (TextUtils.isEmpty(path)) {
                i--;
                sum++;
                continue;
            }

            BookInfo book = new BookInfo();
            book.setPath(path);
            book.setTitle(name);
            try {
                book.setCurrentPageNo(cursor.getInt(cursor.getColumnIndex("curpageno")));
            } catch (NumberFormatException e) {
                Log.e(TAG, "book point current page no format error");
            }
            Log.i("book_manager", "get book list : path=" + path + ",   title=" + name + ",  currentNo=" + book.currentPageNo());
            books.add(book);
        }

        cursor.close();
        return books;
    }


    public List<BookInfo> getAllBooks() {
        ArrayList<BookInfo> books = new ArrayList<BookInfo>();
        if (mContext == null) {
            return books;
        }

        Cursor cursor = mContext.getContentResolver().query(BOOKS_QUERY_ALL_URI, null, null, null, "_id DESC");

        if (cursor == null) {
            return books;
        }

        while (cursor.moveToNext()) {
            BookInfo book = new BookInfo();
            book.setPath(cursor.getString(cursor.getColumnIndex("path")));
            book.setTitle(cursor.getString(cursor.getColumnIndex("bookname")));
            books.add(book);
        }

        cursor.close();
        return books;
    }

    //最近阅读记录的Content Provider
    private static final String NEW_READ_AUTHORITY = "com.allwinnertech.reader.breakpointprovider";

    private static final Uri NEW_READ_CONTENT_URI = Uri.parse("content://" + NEW_READ_AUTHORITY + "/books");

    private static final String READ_NEW_ORDER_BY = "_id DESC";

    public static final String[] READ_NEW_PROJECTION = new String[]{
            "bookname",
            "path",
            "curpageno"
    };

    //封面的Content Provider
    public static final String COVERING_AUTHORITY = "com.allwinnertech.reader.bookcoverprovider";
    public static final Uri COVERING_CONTENT_URI = Uri.parse("content://" + COVERING_AUTHORITY + "/books");

    public static final String[] BOOK_COVERING_PROJECTION = new String[]{
            "path",
            "defaultflag",
            "coverimg"
    };

    //Book基本信息
    public static final String BOOK_INFO_AUTHORITY = "com.allwinnertech.reader.booklistprovider";
    public static final Uri BOOK_INFO_CONTENT_URI = Uri.parse("content://" + BOOK_INFO_AUTHORITY + "/books");

    public static final String[] BOOK_INFO_PROJECTION = new String[]{
            "path",
            "author",
            "totalpage"
    };

    //查询所有Books
    private static final Uri BOOKS_QUERY_ALL_URI = Uri.parse("content://" + "com.allwinnertech.reader.booklistprovider" + "/books");
    public static final String[] ALL_BOOKS__PROJECTION = new String[]{
            "path",
            "bookname",
    };
}

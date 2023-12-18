package com.softwinner.einklaunch;

import java.util.List;
import com.eink.launcher.Launcher;

import com.softwinner.dntools.mvp.EventHandler;
import com.softwinner.einklaunch.R;
import com.softwinner.einklaunch.book.BookInfo;

import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;

// AW:Added for BOOTEVENT
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

public class LaunchActivity extends Activity implements ILaunchView, OnClickListener, OnItemClickListener {
    private static final String TAG = "MainActivity";

    private EventHandler resumeHandler;
    private EventHandler pauseHandler;
    List<BookInfo> newReadBooks;
    private LayoutInflater mInflate;
    private EventHandler startReadingHandler;
    ImageView bookCoverImg;

    private Handler mUpdateHandler;

    private final int UPDATE_READIND = 0x00001;

    // AW:Added for BOOTEVENT
    public static String getSystemProperty(String property, String defaultValue) {
        try {
            Class clazz = Class.forName("android.os.SystemProperties");
            java.lang.reflect.Method getter = clazz.getDeclaredMethod("get", String.class);
            String value = (String) getter.invoke(null, property);
            if (!TextUtils.isEmpty(value)) {
                return value;
            }
        } catch (Exception e) {
            Log.d("launcher", "Unable to read system properties");
        }
        return defaultValue;
    }
    private static boolean sBootEventenable = "true".equals(getSystemProperty("persist.sys.bootevent", "true"));
    static void logBootEvent(String bootevent) {
        if (!sBootEventenable) {
            return ;
        }
        FileOutputStream fos =null;
        try {
            fos = new FileOutputStream("/proc/bootevent");
            fos.write(bootevent.getBytes());
            fos.flush();
        } catch (FileNotFoundException e) {
            Log.e("BOOTEVENT","Failure open /proc/bootevent,not found!",e);
        } catch (java.io.IOException e) {
            Log.e("BOOTEVENT","Failure open /proc/bootevent entry",e);
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    Log.e ("BOOTEVENT","Failure close /proc/bootevent entry",e);
                }
            }
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        // AW:BOOTEVENT
        logBootEvent("Launcher:onCreate start");
    /*View.setEinkUpdateStrategy(
        View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
        View.EINK_DISPLAY_MODE_GC16_LOCAL,
        View.EINK_DISPLAY_MODE_GC16_FULL,
        View.EINK_DISPLAY_MODE_GC16_LOCAL);*/
        super.onCreate(savedInstanceState);
        Log.i("launcher","Launcher:onCreate start");
        setContentView(R.layout.launcher);
        new LaunchPresenter().setView(this);
        ListView lv = (ListView) findViewById(R.id.reading_new_list_view);
        lv.setAdapter(mListAdapter);
        lv.setOnItemClickListener(this);
        mInflate = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        findViewById(R.id.apps_table_libraries).setOnClickListener(this);
        findViewById(R.id.apps_table_browser).setOnClickListener(this);
        findViewById(R.id.apps_table_settings).setOnClickListener(this);
        findViewById(R.id.apps_table_files).setOnClickListener(this);
        findViewById(R.id.apps_table_music).setOnClickListener(this);
        findViewById(R.id.apps_table_applist).setOnClickListener(this);

        findViewById(R.id.reading_now_book_cover).setOnClickListener(this);

        mUpdateHandler = new UpdateHandler();
    }

    private class UpdateHandler extends Handler {
        public UpdateHandler() {

        }

        ;

        @Override
        public void handleMessage(Message msg) {

            int id = msg.what;
            switch (id) {
                case UPDATE_READIND:
                /*View.setEinkUpdateStrategy(
                View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
                View.EINK_DISPLAY_MODE_GC16_LOCAL,
                View.EINK_DISPLAY_MODE_GC16_FULL,
                View.EINK_DISPLAY_MODE_GC16_LOCAL);*/
                    resumeHandler.handler();
                    break;

            }
        }
    }

    public Context getContext() {
        return this;
    }

    public void displayReadingNowBook(String bookName, Bitmap bookCover,
                                      String path, int curPgae, int totalPage) {

        TextView bookNameText = (TextView) findViewById(R.id.reading_now_book_name);
        bookCoverImg = (ImageView) findViewById(R.id.reading_now_book_cover);
        TextView readingPageIndexText = (TextView) findViewById(R.id.reading_now_book_page_number);
        TextView readingPageTotalIndexText = (TextView) findViewById(R.id.reading_now_book_page_number_after);

        if (TextUtils.isEmpty(bookName)) {
            bookNameText.setText(R.string.reading_now_no_read);
        } else {
            bookNameText.setText(bookName);
        }

        if (bookCover != null) {
            bookCoverImg.setImageBitmap(bookCover);

        } else {
            bookCoverImg.setImageResource(R.drawable.book_coverdefault);
        }
        bookCoverImg.setContentDescription(path);

        if (totalPage <= 0) {
            readingPageIndexText.setText("");
            readingPageIndexText.setText("");
            findViewById(R.id.reading_now_book_page_number_pre).setVisibility(View.GONE);
            findViewById(R.id.reading_now_book_page_number_after).setVisibility(View.GONE);
        } else {
            readingPageTotalIndexText.setText(String.format(getString(R.string.reading_now_page_after), totalPage));
            readingPageIndexText.setText(String.valueOf(curPgae));
            findViewById(R.id.reading_now_book_page_number_pre).setVisibility(View.VISIBLE);
            findViewById(R.id.reading_now_book_page_number_after).setVisibility(View.VISIBLE);
        }

    }

    public void setOnResumeHandler(EventHandler handler) {
        resumeHandler = handler;
    }

    @Override
    protected void onResume() {
        // AW:BOOTEVENT
        logBootEvent("Launcher:onResume start");
        Log.i("launcher","Launcher:onResume start");
        /*View.setEinkUpdateStrategy(
        View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
        View.EINK_DISPLAY_MODE_GC16_FULL,
        View.EINK_DISPLAY_MODE_GC16_FULL,
        View.EINK_DISPLAY_MODE_GC16_FULL);*/
        //View.setEinkUpdateMode(View.EINK_DISPLAY_MODE_GC16_FULL);
        super.onResume();
        if (resumeHandler != null) {
            mUpdateHandler.sendEmptyMessageDelayed(UPDATE_READIND, 500);
        }
        //View.invalidate(View.EINK_DISPLAY_MODE_GC16_FULL);
        // AW:BOOTEVENT Launcher displayed end,turn off bootevent
        Log.i("launcher","Launcher:onResume end");
        logBootEvent("Launcher:onResume end");
        logBootEvent("0");
    }

    private BaseAdapter mListAdapter = new BaseAdapter() {

        public int getCount() {
            return newReadBooks == null ? 0 : newReadBooks.size();
        }

        public Object getItem(int arg0) {
            return null;
        }

        public long getItemId(int arg0) {
            return 0;
        }

        public View getView(int arg0, View convertView, ViewGroup arg2) {

            convertView = mInflate.inflate(R.layout.book_list_item, null);
            BookInfo book = newReadBooks.get(arg0);
            if (book == null) {
                return convertView;
            }
            TextView bookNameText = (TextView) convertView.findViewById(R.id.book_item_title);
            TextView bookAuthorText = (TextView) convertView.findViewById(R.id.book_item_author);
            bookNameText.setText(book.getTitle());
            String author = book.getAuthor(getBaseContext());
            if (!TextUtils.isEmpty(author)) {
                bookAuthorText.setText(author);
            } else {
                bookAuthorText.setText(getString(R.string.author_unknown));
            }
            convertView.setContentDescription(book.getPath());
            return convertView;
        }

    };

    public void displayNewRead(List<BookInfo> books) {
        this.newReadBooks = books;
        mListAdapter.notifyDataSetChanged();
    }

    public void onClick(View v) {
        /*View.setEinkUpdateStrategy(View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
        View.EINK_DISPLAY_MODE_GC16_LOCAL, View.EINK_DISPLAY_MODE_GC16_FULL, View.EINK_DISPLAY_MODE_GC16_LOCAL);*/
        switch (v.getId()) {
            case R.id.apps_table_libraries:
                startApp("com.softwinner.einklaunch", "com.softwinner.einklaunch.BookLibrariesActivity");
                break;
            case R.id.apps_table_browser:
                startApp("org.chromium.webview_shell", "org.chromium.webview_shell.WebViewBrowserActivity");
                break;
            case R.id.apps_table_settings:
                startApp("com.android.settings", "com.android.settings.Settings");
                break;
            case R.id.apps_table_files:
                // launch AOSP DocumentsUI file ui to browser:
                Intent intent = new Intent();
                intent.setClassName("com.android.documentsui", "com.android.documentsui.files.FilesActivity");
                intent.setAction(Intent.ACTION_VIEW);
                // MIME type and Uri maybe change, see the DocumentsUI self launch Intent for detail
                intent.setType("vnd.android.document/root");
                intent.setData(Uri.parse("content://com.android.externalstorage.documents/root/primary"));
                startApp(intent);
                break;
            case R.id.apps_table_music:
                startApp("com.android.music", "com.android.music.MusicBrowserActivity");
                break;
            case R.id.apps_table_applist:
                startApp("com.softwinner.einklaunch", "com.eink.launcher.Launcher");
                break;
            case R.id.reading_now_book_cover:
                CharSequence path = bookCoverImg.getContentDescription();
                if (!TextUtils.isEmpty(path)) {
                    Log.i("launcher", "start reading " + path);
                    startReadingHandler.handler(path);
                }
                break;
        }
    }

    private void startApp(String packageName, String className) {
        Intent intent = new Intent();
        intent.setClassName(packageName, className);
        startApp(intent);
    }

    private void startApp(Intent intent) {

        /*set eink updata strategy by A-gan*/
        /*View.setEinkUpdateStrategy(
        View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
        View.EINK_DISPLAY_MODE_GC16_LOCAL,
        View.EINK_DISPLAY_MODE_GC16_FULL,
        View.EINK_DISPLAY_MODE_GC16_LOCAL);*/

        try {
            Launcher.startActivitySafely(this, intent, null);
        } catch (Exception e) {
            Log.e("launcher", "error: ", e);
        }
    }

    public void setStartReadingHandler(EventHandler handler) {
        startReadingHandler = handler;

    }

    public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
        CharSequence path = arg1.getContentDescription();
        if (!TextUtils.isEmpty(path)) {
            startReadingHandler.handler(path);
        }
    }

    public void setOnPauseHandler(EventHandler handler) {
        pauseHandler = handler;

    }

    @Override
    protected void onPause() {
        super.onPause();
        if (pauseHandler != null) {
            pauseHandler.handler();
        }
    }

    @Override
    public boolean onKeyDown(int arg0, KeyEvent arg1) {
        if (arg0 == KeyEvent.KEYCODE_BACK) {
            return true;
        }
        return super.onKeyDown(arg0, arg1);
    }
}

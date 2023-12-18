package com.softwinner.einklaunch;

import java.util.List;

import com.softwinner.dntools.mvp.EventHandler;
import com.softwinner.dntools.tag.TableGridTag;
import com.softwinner.einklaunch.book.BookInfo;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.TableLayout;
import android.widget.TextView;


public class BookLibrariesActivity extends Activity implements IBookLibrariesView, TableGridTag.TagCallBack, OnClickListener {

    private EventHandler createHandler;
    private EventHandler destoryHandler;

    private List<BookInfo> books;
    TableGridTag librariesTag;

    LayoutInflater mInflater;

    private final static String TAG = "Book_Libraries";

    TextView pageInfoText;
    View preBtn;
    View nextBtn;

    @Override
    protected void onCreate(Bundle arg0) {
        /*add by A-gan set eink update strategy*/
        //View.setEinkUpdateStrategy(View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
        //		View.EINK_DISPLAY_MODE_GC16_LOCAL, View.EINK_DISPLAY_MODE_GC16_FULL, View.EINK_DISPLAY_MODE_GC16_LOCAL);
        /*add end*/
        super.onCreate(arg0);
        setContentView(R.layout.book_libraries);

        mInflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        TableLayout tableLayout = (TableLayout) findViewById(R.id.book_libraries_table);
        librariesTag = new TableGridTag(tableLayout, 3, 3);
        librariesTag.setCallBack(this);

        new BookLibrariesPresenter().setView(this);


        pageInfoText = (TextView) findViewById(R.id.page_number_info);
        preBtn = findViewById(R.id.page_pre_btn);
        preBtn.setOnClickListener(this);
        nextBtn = findViewById(R.id.page_next_btn);
        nextBtn.setOnClickListener(this);

        createHandler.handler();
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
        destoryHandler.handler();
    }


    public Context getContext() {
        return this;
    }

    public void updatePageInfo() {
        pageInfoText.setText(librariesTag.getPageInfo());
        if (librariesTag.getCurrentPage() == 1) {
            preBtn.setEnabled(false);
        } else {
            preBtn.setEnabled(true);
        }

        if (librariesTag.getCurrentPage() == librariesTag.getPageCount()) {
            nextBtn.setEnabled(false);
        } else {
            nextBtn.setEnabled(true);
        }
    }

    public void displayBookLibraries(List<BookInfo> books) {
        this.books = books;
        librariesTag.refesh();
        updatePageInfo();
    }

    public void setCreateHandler(EventHandler handler) {
        this.createHandler = handler;
    }

    public void setDestoryHandler(EventHandler handler) {
        this.destoryHandler = handler;
    }

    public void onItemClick(View v, int position) {
        int bookIndex = position + (librariesTag.getCurrentPage() - 1) * librariesTag.getColumn() * librariesTag.getRow();
        if (books == null || books.size() < bookIndex) {
            return;
        }
        BookInfo book = books.get(bookIndex);
        if (book != null && !TextUtils.isEmpty(book.getPath())) {
            book.startReading(this);
        }
    }

    public int getCount() {
        return books == null ? 0 : books.size();
    }

    public View getView(int position) {
        if (books == null) {
            return null;
        }
        BookInfo b = books.get(position);
        if (b == null) {
            return null;
        }
        View v = mInflater.inflate(R.layout.book_table_item, null, false);
        TextView nameText = (TextView) v.findViewById(R.id.book_table_item_name);
        ImageView img = (ImageView) v.findViewById(R.id.book_table_item_img);
        nameText.setText(b.getTitle());
        img.setImageBitmap(b.getCover(this));

        Log.i(TAG, "new book: " + b.getTitle() + "   path: " + b.getPath());

        return v;
    }


    public void onClick(View arg0) {
        switch (arg0.getId()) {
            case R.id.page_pre_btn:
                librariesTag.pre();

                updatePageInfo();
                break;
            case R.id.page_next_btn:
                librariesTag.next();
                updatePageInfo();
                break;
        }
    }


    @Override
    protected void onResume() {
        /*add by A-gan set eink update strategy*/
        //View.setEinkUpdateStrategy(View.EINK_DISPLAY_STRATEGY_ALL_FLIP_WITHOUT_LAST,
        //		View.EINK_DISPLAY_MODE_GC16_LOCAL, View.EINK_DISPLAY_MODE_GC16_FULL, View.EINK_DISPLAY_MODE_GC16_LOCAL);
        /*add end*/
        super.onResume();
    }


    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_PAGE_UP) {
            if (preBtn.isEnabled()) {
                librariesTag.pre();
                updatePageInfo();
            }
            return true;
        }

        if (keyCode == KeyEvent.KEYCODE_PAGE_DOWN) {
            if (nextBtn.isEnabled()) {
                librariesTag.next();
                updatePageInfo();
            }
            return true;
        }

        return super.onKeyUp(keyCode, event);
    }
}

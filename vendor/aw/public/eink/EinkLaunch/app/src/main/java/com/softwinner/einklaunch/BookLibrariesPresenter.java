package com.softwinner.einklaunch;

import java.util.List;

import android.content.Context;

import com.softwinner.dntools.mvp.EventHandler;
import com.softwinner.dntools.mvp.IPresenter;
import com.softwinner.einklaunch.book.BookInfo;
import com.softwinner.einklaunch.book.BookManager;

public class BookLibrariesPresenter implements IPresenter<IBookLibrariesView> {

    private BookManager bm;


    public void setView(final IBookLibrariesView view) {

        view.setDestoryHandler(new EventHandler() {

            public void handler(Object... args) {

            }

        });

        view.setCreateHandler(new EventHandler() {

            public void handler(Object... args) {
        //ArrayList<BookInfo> books = new ArrayList<BookInfo>();
        //for(int i = 0; i < 40; i++) {
        //  BookInfo b = new BookInfo();
        //  b.setTitle("中国新书");
        //  b.setPath("/mnt/sdcard/Coding2.pdf");
        //  books.add(b);
        //}
                view.displayBookLibraries(queryAllBooks(view.getContext()));
            }

        });
    }

    private List<BookInfo> queryAllBooks(Context context) {
        if (bm == null) {
            bm = new BookManager(context);
        }
        return bm.getAllBooks();
    }
}

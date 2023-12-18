package com.softwinner.einklaunch;

import java.io.File;
import java.util.List;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;

import com.softwinner.dntools.mvp.EventHandler;
import com.softwinner.dntools.mvp.IPresenter;
import com.softwinner.einklaunch.book.BookInfo;
import com.softwinner.einklaunch.book.BookManager;

public class LaunchPresenter implements IPresenter<ILaunchView> {

    private ILaunchView mView;

    private BookManager bm;

    public void setView(final ILaunchView view) {
        mView = view;
        view.setOnResumeHandler(new EventHandler() {

            public void handler(Object... args) {
                if (bm == null) {
                    bm = new BookManager(view.getContext());
                }

                List<BookInfo> nearReadBooks = bm.getReadNewBooks();
                mView.displayNewRead(nearReadBooks);

                if (nearReadBooks.size() > 0) {
                    BookInfo lastBook = nearReadBooks.get(0);
                    mView.displayReadingNowBook(lastBook.getTitle(), lastBook.getCover(view.getContext()), lastBook.getPath(), lastBook.currentPageNo(), lastBook.getTotalPage(view.getContext()));
                } else {
                    mView.displayReadingNowBook(view.getContext().getString(R.string.no_reading),
                            null,
                            null,
                            0,
                            0);
                }

                IntentFilter filter = new IntentFilter();
                filter.addAction("com.action.scanfinish");
                mView.getContext().registerReceiver(mReceiver, filter);
            }
        });

        view.setStartReadingHandler(new EventHandler() {

            public void handler(Object... args) {
                if (args == null || args[0] == null) {
                    return;
                }
                String path = args[0].toString();
                Intent i = new Intent("com.allwinnertech.bookread");
                i.setDataAndType(Uri.fromFile(new File(path)), "txt/plain");
                com.eink.launcher.Launcher.startActivitySafely(view.getContext(),i,null);
            }

        });

        view.setOnPauseHandler(new EventHandler() {

            public void handler(Object... args) {
                try {
                    mView.getContext().unregisterReceiver(mReceiver);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }

        });

    }


    BroadcastReceiver mReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context arg0, Intent arg1) {
            if (bm == null) {
                bm = new BookManager(mView.getContext());
            }

            List<BookInfo> nearReadBooks = bm.getReadNewBooks();
            mView.displayNewRead(nearReadBooks);

            if (nearReadBooks.size() > 0) {
                BookInfo lastBook = nearReadBooks.get(0);
                mView.displayReadingNowBook(lastBook.getTitle(), lastBook.getCover(arg0), lastBook.getPath(), lastBook.currentPageNo(), lastBook.getTotalPage(arg0));
            } else {
                mView.displayReadingNowBook(mView.getContext().getString(R.string.no_reading),
                        null,
                        null,
                        0,
                        0);
            }
        }

    };

}

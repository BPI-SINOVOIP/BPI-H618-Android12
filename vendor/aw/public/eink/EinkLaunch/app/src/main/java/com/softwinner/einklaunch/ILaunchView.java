package com.softwinner.einklaunch;

import java.util.List;

import android.graphics.Bitmap;

import com.softwinner.dntools.mvp.EventHandler;
import com.softwinner.dntools.mvp.IView;
import com.softwinner.einklaunch.book.BookInfo;

public interface ILaunchView extends IView {
    public void displayReadingNowBook(String bookName, Bitmap bookCover, String path, int curPgae, int totalPage);

    public void displayNewRead(List<BookInfo> books);

    public void setOnResumeHandler(EventHandler handler);

    public void setStartReadingHandler(EventHandler handler);

    public void setOnPauseHandler(EventHandler handler);
}

package com.softwinner.einklaunch;

import java.util.List;

import com.softwinner.dntools.mvp.EventHandler;
import com.softwinner.dntools.mvp.IView;
import com.softwinner.einklaunch.book.BookInfo;

public interface IBookLibrariesView extends IView {
    public void displayBookLibraries(List<BookInfo> books);

    public void setCreateHandler(EventHandler handler);

    public void setDestoryHandler(EventHandler handler);
}

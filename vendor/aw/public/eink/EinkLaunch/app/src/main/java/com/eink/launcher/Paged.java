package com.eink.launcher;

import android.view.View;

public interface Paged {
    public int getPageChildCount();

    public View getChildOnPageAt(int i);

    public void removeAllViewsOnPage();

    public void removeViewOnPageAt(int i);

    public int indexOfChildOnPage(View v);
}

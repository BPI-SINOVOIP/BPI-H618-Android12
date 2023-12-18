package com.eink.launcher;

import android.view.ContextMenu;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;

public interface ItemEventListener {
    public void onClick(View v);

    public void onLongClick(View v);

    public void onCreateContextMenu(ContextMenu menu, View view,
                                    ContextMenuInfo menuInfoIn);
}

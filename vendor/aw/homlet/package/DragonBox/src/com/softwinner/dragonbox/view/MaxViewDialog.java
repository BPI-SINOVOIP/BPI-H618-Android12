package com.softwinner.dragonbox.view;

import android.app.AlertDialog;
import android.content.Context;

public class MaxViewDialog extends AlertDialog{

    private boolean mShowButton = true;

    public MaxViewDialog(Context context, boolean cancelable,
                         OnCancelListener cancelListener) {
        super(context, cancelable, cancelListener);
        // TODO Auto-generated constructor stub
    }

    public MaxViewDialog(Context context, int theme) {
        super(context, theme);
        // TODO Auto-generated constructor stub
    }

    public MaxViewDialog(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }


}

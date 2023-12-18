package com.allwinnertech.socs;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.widget.Toast;

public class ToastInfo {

    Context mContext;

    public ToastInfo(Context context){
        mContext = context;
    }

    public void showInfo(String info){
        Toast.makeText(mContext, info, 2000);
    }

}

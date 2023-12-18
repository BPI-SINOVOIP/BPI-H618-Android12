package com.softwinner.timerswitch.observer;

import android.database.ContentObserver;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import com.softwinner.timerswitch.utils.Constant;
import com.softwinner.timerswitch.utils.Utils;

public class InsertContentObserver extends ContentObserver {

    private Handler handler;

    public InsertContentObserver(Handler handler) {
        super(handler);
        this.handler = handler;
    }

    @Override
    public void onChange(boolean selfChange,Uri uri) {
        if(handler!=null) {
            long id = Utils.getIdByUri(uri);
            Message msg = Message.obtain();
            msg.what = Constant.MSG_INSERT;
            msg.obj = id;
            handler.sendMessage(msg);
        }
    }

}

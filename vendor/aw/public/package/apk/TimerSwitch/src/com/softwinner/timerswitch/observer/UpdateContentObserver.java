package com.softwinner.timerswitch.observer;

import android.database.ContentObserver;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import com.softwinner.timerswitch.utils.Constant;
import com.softwinner.timerswitch.utils.Utils;

public class UpdateContentObserver extends ContentObserver {

    private Handler handler;

    public UpdateContentObserver(Handler handler) {
        super(handler);
        this.handler = handler;
    }

    @Override
    public void onChange(boolean selfChange, Uri uri) {
        if(handler!=null) {
            long id = Utils.getIdByUri(uri);
            Message msg = Message.obtain();
            msg.what = Constant.MSG_UPDATE;
            msg.obj = id;
            handler.sendMessage(msg);
        }
    }
}

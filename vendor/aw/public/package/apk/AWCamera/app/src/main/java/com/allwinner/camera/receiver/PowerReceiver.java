package com.allwinner.camera.receiver;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;
import com.allwinner.camera.R;

public class PowerReceiver extends BroadcastReceiver {
    private static final String TAG = "PowerReceiver";
    private Callback mCallback = null;
    private static final String BATTERY_REMAIN = "level";

    @Override
     public void onReceive(Context context, Intent intent) {
        if(intent.getAction() == Intent.ACTION_BATTERY_CHANGED && mCallback != null) {
            mCallback.remainLevel(intent.getIntExtra(BATTERY_REMAIN, 0));
        }
    }

    public void setCallback(Callback callback) {
        mCallback = callback;
    }

    public interface Callback {
        public void remainLevel(int level);
    }
}

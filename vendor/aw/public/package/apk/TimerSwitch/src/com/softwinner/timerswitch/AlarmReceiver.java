package com.softwinner.timerswitch;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.text.TextUtils;
import android.util.Log;
import com.softwinner.timerswitch.data.Alarm;
import com.softwinner.timerswitch.data.AlarmController;
import com.softwinner.timerswitch.data.AlarmModel;
import com.softwinner.timerswitch.utils.Utils;

import java.lang.reflect.Method;
import java.util.Calendar;
import java.util.List;

public class AlarmReceiver extends BroadcastReceiver {

    private final static String TAG = "TAG_AlarmReceiver";

    public final static String ACTION_BOOT = "action_boot";
    public final static String ACTION_SHUT_DOWN = "action_shutdown";
    public static final String ACTION_BOOT_COMPLETED = "android.intent.action.BOOT_COMPLETED";

    @Override
    public void onReceive(final Context context, Intent intent) {
        String action = intent.getAction();
        if(TextUtils.isEmpty(action)) return;
        Bundle bundle = intent.getBundleExtra(Alarm.KEY_BUNDLE);
        AlarmModel model = new AlarmModel(context.getContentResolver());
        AlarmController controller = new AlarmController(context,model);
        Calendar curCalendar = Calendar.getInstance();
        long curTime = curCalendar.getTimeInMillis();
        Log.d(TAG,"receive  broadcast, curTime : " + Utils.getTimeByFormat(curTime));
        boolean needKnowCurWeekdays = true;
        switch (action){
            case ACTION_BOOT_COMPLETED:
                Log.d(TAG,"********receive system bootUp broadcast*********");
                controller.resetAllDBAlarms(context);
                break;
            case ACTION_BOOT:
                Log.d(TAG,"*****receive boot broadcast*****");
                if(bundle != null) {
                    Alarm receiverAlarm = (Alarm) bundle.getSerializable(Alarm.ACTION_BOOT_ALARM);
                    if(curTime - receiverAlarm.alertTime >  60 * 1000){
                        Log.d(TAG, "******The time difference between current system time and  " +
                                "boot alarm time is greater than 1 minute and should be filtered out*****");
                        needKnowCurWeekdays = false;
                    }
                    controller.resetAlarm(context,receiverAlarm,needKnowCurWeekdays);
                }
                break;
            case ACTION_SHUT_DOWN:
                Log.d(TAG,"*****receive shutdown broadcast*****");
                boolean canShutdown = true;
                if(bundle != null) {
                    Alarm receiverAlarm = (Alarm) bundle.getSerializable(Alarm.ACTION_SHUTDOWN_ALARM);
                    if(curTime - receiverAlarm.alertTime >  60 * 1000){
                        Log.d(TAG, "******The time difference between current system time and  " +
                                "shutdown alarm time is greater than 1 minute and should be filtered out*****");
                        canShutdown = false;
                        needKnowCurWeekdays = false;
                    }
                    controller.resetAlarm(context,receiverAlarm,needKnowCurWeekdays);
                }
                if(canShutdown) {
                    delayedShutdown(context);
                }
                break;
        }

    }

    private void delayedShutdown(final Context context){
        if(context == null) return;
        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                Intent intent = new Intent(Intent.ACTION_REQUEST_SHUTDOWN);
                intent.putExtra("android.intent.extra.KEY_CONFIRM", false);
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            }
        }, 500);
    }

}

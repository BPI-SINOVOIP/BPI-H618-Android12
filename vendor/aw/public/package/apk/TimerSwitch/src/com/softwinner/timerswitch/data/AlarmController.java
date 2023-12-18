package com.softwinner.timerswitch.data;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import com.softwinner.timerswitch.AlarmReceiver;
import com.softwinner.timerswitch.WeekdaysLinkList;
import com.softwinner.timerswitch.utils.Utils;
import java.util.Calendar;
import java.util.List;

public class AlarmController {

    private final static String TAG = "TAG_AlarmController";
    public final static String ACTION_BOOT = "action_boot";
    public final static String ACTION_SHUT_DOWN = "action_shutdown";
    private final static long DELAY_WAKE_UP_TIME = 60 * 1000;

    private Context mContext;
    private AlarmModel model;

    public AlarmController(Context context, AlarmModel alarmModel){
        this.mContext = context;
        this.model = alarmModel;
    }

    private AlarmManager getAlarmManager(Context context) {
        return (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
    }

    private Intent createShutdownIntent(Alarm alarm, boolean newAlarm){
        Intent intent = new Intent(mContext, AlarmReceiver.class);
        intent.setAction(ACTION_SHUT_DOWN);
        if(newAlarm) {
            Bundle bundle = new Bundle();
            bundle.putSerializable(Alarm.ACTION_SHUTDOWN_ALARM, alarm);
            intent.putExtra(Alarm.KEY_BUNDLE, bundle);
        }
        return intent;
    }

    private Intent createBootIntent(Alarm alarm,boolean newAlarm){
        Intent intent = new Intent(mContext, AlarmReceiver.class);
        intent.setAction(ACTION_BOOT);
        if(newAlarm) {
            Bundle bundle = new Bundle();
            bundle.putSerializable(Alarm.ACTION_BOOT_ALARM, alarm);
            intent.putExtra(Alarm.KEY_BUNDLE, bundle);
        }
        return intent;
    }

    public void startTimingAlarm(Alarm alarm){
        Alarm dbAlarm = convertUserAlarmToDataBaseAlarm(alarm);
        startAlarmClock(mContext,dbAlarm);
    }

    public void closeTimingAlarm(Alarm alarm){
        if(alarm == null || !alarm.enable) return;
        AlarmManager alarmManager = getAlarmManager(mContext);
        alarm.enable = false;
        model.updateById(alarm);
        PendingIntent pendingIntent = null;
        if(alarm.label.equals(Alarm.LABEL_ALARM_SHUTDOWN)) {
            pendingIntent = PendingIntent.getBroadcast(mContext, alarm.hashCode(), createShutdownIntent(alarm, false),
                    PendingIntent.FLAG_IMMUTABLE);
        } else if(alarm.label.equals(Alarm.LABEL_ALARM_BOOT)){
            pendingIntent = PendingIntent.getBroadcast(mContext, alarm.hashCode(), createBootIntent(alarm, false),
                    PendingIntent.FLAG_IMMUTABLE);
        }
        if(pendingIntent != null){
            Log.d(TAG,"cancel alarm " + " alarm.hashCode() : " + alarm.hashCode());
            alarmManager.cancel(pendingIntent);
        }
    }

    public Alarm convertUserAlarmToDataBaseAlarm(Alarm userAlarm){
        if(userAlarm == null || !userAlarm.enable) return userAlarm;
        Log.d(TAG,"cur time : " + Utils.getTimeByFormat(Calendar.getInstance().getTimeInMillis()));
        long shutdownTime = userAlarm.getNextTime(Calendar.getInstance()).getTimeInMillis();
        userAlarm.alertTime = shutdownTime;
        Log.d(TAG,"start timing Alarm alarm : " + userAlarm.toString());

        if (userAlarm.getId() == Alarm.INVALID_ID) {
            Log.d(TAG,"insert,item : " +userAlarm.toString());
            model.insert(userAlarm);
        } else {
            Log.d(TAG,"update，item : " +userAlarm.toString());
            model.updateById(userAlarm);
        }
        // use alarm data in database
        Alarm dbAlarm;
        if(userAlarm.getId() == Alarm.INVALID_ID) {
            dbAlarm = model.findByCreateTimeStamp(userAlarm.createTimeStamp);
        } else {
            dbAlarm = model.findByAlarmId(userAlarm.getId());
        }
        return dbAlarm;
    }

    public void startAlarmClock(Context context, Alarm dbAlarm){
        if(context == null || dbAlarm == null) return;
        AlarmManager alarmManager = getAlarmManager(context);
        PendingIntent pendingIntent = null;
        AlarmManager.AlarmClockInfo info = null;
        if(dbAlarm.label.equals(Alarm.LABEL_ALARM_SHUTDOWN)){
            pendingIntent = PendingIntent.getBroadcast(context, dbAlarm.hashCode(), createShutdownIntent(dbAlarm,true),
                    PendingIntent.FLAG_IMMUTABLE);
            info = new AlarmManager.AlarmClockInfo(dbAlarm.alertTime, pendingIntent);
        } else if(dbAlarm.label.equals(Alarm.LABEL_ALARM_BOOT)){
            pendingIntent = PendingIntent.getBroadcast(context, dbAlarm.hashCode(), createBootIntent(dbAlarm,true),
                    PendingIntent.FLAG_IMMUTABLE);
            info = new AlarmManager.AlarmClockInfo(dbAlarm.alertTime + DELAY_WAKE_UP_TIME, pendingIntent);
        }
        alarmManager.setAlarmClock(info, pendingIntent);
    }

    public void resetAlarm(Context context,Alarm receiverAlarm,boolean needKnowCurWeekdays){
        if(context == null) return;
        if (receiverAlarm != null) {
            //for repeat alarm
            if(receiverAlarm.repeatDaysOfWeek != null && receiverAlarm.repeatDaysOfWeek.size() > 0) {
                Log.d(TAG,"resetAlarm for repeat alarm");
                WeekdaysLinkList<Weekdays> repeatDaysOfWeek = receiverAlarm.getResetRepeatDaysOfWeek();
                Weekdays weekdays = repeatDaysOfWeek.getFirst();
                Alarm newAlarm = null;
                if(needKnowCurWeekdays) {
                    newAlarm = new Alarm(receiverAlarm.getId(), System.currentTimeMillis(), weekdays, repeatDaysOfWeek,
                            receiverAlarm.hourOfDay, receiverAlarm.minutes, receiverAlarm.enable, receiverAlarm.label, receiverAlarm.alertTime);
                } else{
                    newAlarm = new Alarm(receiverAlarm.getId(), System.currentTimeMillis(),receiverAlarm.repeatDaysOfWeek,
                            receiverAlarm.hourOfDay, receiverAlarm.minutes,receiverAlarm.enable,receiverAlarm.label);
                }
                startTimingAlarm(newAlarm);
            } else{
                //update alarm enable state
                Log.d(TAG,"resetAlarm update  non-repeat alarm");
                receiverAlarm.enable = false;
                model.updateById(receiverAlarm);
            }
        }
    }

    /**
     * reset all database alarms
     * @param context
     */
    public void resetAllDBAlarms(Context context){
        model.updateAllTimeOutAlarm();
        List<Alarm> notTimeOtAlarmList = model.findAllNotTimeOutAlarms();
        for(Alarm alarm : notTimeOtAlarmList){
            startAlarmClock(context,alarm);
        }
    }


}

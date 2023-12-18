package com.softwinner.timerswitch.data;

import android.content.ContentValues;
import android.database.Cursor;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.Log;

import com.softwinner.timerswitch.utils.Constant;
import com.softwinner.timerswitch.utils.Utils;
import com.softwinner.timerswitch.WeekdaysLinkList;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class Alarm implements Serializable {

    private static final String TAG = "TAG_Alarm";

    public static final String ACTION_SHUTDOWN_ALARM = "action_shutdown_alarm";
    public static final String ACTION_BOOT_ALARM = "action_boot_alarm";
    public static final String KEY_BUNDLE = "key_bundle";

    public static final String LABEL_ALARM_BOOT = "boot";
    public static final String LABEL_ALARM_SHUTDOWN = "shutdown";

    private static final Map<Integer, Weekdays> mCalendarDayForWeekdays;
    static {
        final Map<Integer, Weekdays> map = new ArrayMap<>(7);
        map.put(Calendar.MONDAY,   Weekdays.MONDAY );
        map.put(Calendar.TUESDAY,   Weekdays.TUESDAY);
        map.put(Calendar.WEDNESDAY, Weekdays.WEDNESDAY);
        map.put(Calendar.THURSDAY,  Weekdays.THURSDAY);
        map.put(Calendar.FRIDAY,    Weekdays.FRIDAY);
        map.put(Calendar.SATURDAY,  Weekdays.SATURDAY);
        map.put(Calendar.SUNDAY,    Weekdays.SUNDAY);
        mCalendarDayForWeekdays = Collections.unmodifiableMap(map);
    }

    public static Map<Integer, Weekdays> getCalendarDayForWeekdays() {
        return mCalendarDayForWeekdays;
    }

    /**
     * Alarms start with an invalid id when it hasn't been saved to the database.
     */
    public static final long INVALID_ID = -1;
    private long id;
    /**
     * timestamp of create alarm
     */
    public long createTimeStamp;
    /**
     * what day is today
     */
    public Weekdays daysOfWeek;
    /**
     * repeat period,such as {'MONDAY','TUESDAY'}
     */
    public WeekdaysLinkList<Weekdays> repeatDaysOfWeek;
    public int hourOfDay;
    public int minutes;
    /**
     * alarm state : true means open ,false means close
     */
    public boolean enable;

    /**
     * determine type of alarm
     */
    public String label;

    /**
     * alarm clock time
     */
    public long alertTime;

    /**
     * for user,create a non-repeated alarm
     */
    public Alarm(int hourOfDay, int minutes, String label) {
        this.id = INVALID_ID;
        this.createTimeStamp = System.currentTimeMillis();
        this.repeatDaysOfWeek = null;
        this.hourOfDay = hourOfDay;
        this.minutes = minutes;
        this.daysOfWeek = getCurAlarmWeekdays();
        this.enable = true;
        this.label = label;
    }

    /**
     * for users,create a repeated alarm.
     */
    public Alarm(WeekdaysLinkList<Weekdays> repeatDaysOfWeek, int hourOfDay, int minutes,String label) {
        this.id = INVALID_ID;
        this.createTimeStamp = System.currentTimeMillis();
        this.daysOfWeek = getCurAlarmWeekdays(repeatDaysOfWeek, hourOfDay,minutes);
        this.repeatDaysOfWeek = getSortedRepeatDaysOfWeek(this.daysOfWeek,repeatDaysOfWeek);
        this.hourOfDay = hourOfDay;
        this.minutes = minutes;
        this.enable = true;
        this.label = label;
    }

    /**
     * for system
     */
    public Alarm(long id, long createTimeStamp,WeekdaysLinkList<Weekdays> repeatDaysOfWeek, int hourOfDay,
                 int minutes,boolean enable,String label) {
        this.id = id;
        this.createTimeStamp = createTimeStamp;
        this.daysOfWeek = getCurAlarmWeekdays(repeatDaysOfWeek, hourOfDay,minutes);
        this.repeatDaysOfWeek = getSortedRepeatDaysOfWeek(this.daysOfWeek,repeatDaysOfWeek);
        this.hourOfDay = hourOfDay;
        this.minutes = minutes;
        this.enable = enable;
        this.label = label;
    }

    /**
     * for system
     */
    public Alarm(long id, long createTimeStamp,Weekdays daysOfWeek, WeekdaysLinkList<Weekdays> repeatDaysOfWeek,
                 int hourOfDay, int minutes, boolean enable, String label,long alertTime) {
        this.id = id;
        this.createTimeStamp = createTimeStamp;
        this.daysOfWeek = daysOfWeek;
        this.repeatDaysOfWeek = repeatDaysOfWeek;
        this.hourOfDay = hourOfDay;
        this.minutes = minutes;
        this.enable = enable;
        this.label = label;
        this.alertTime = alertTime;
    }

    public long getId() {
        return id;
    }

    public static ContentValues createContentValues(Alarm alarm){
        if(alarm == null) return null;
        ContentValues values = new ContentValues(Constant.COLUMN_COUNT);
        if (alarm.id != INVALID_ID) {
            values.put(Constant.COLUMN_ID, alarm.id);
        }
        values.put(Constant.COLUMN_CREATE_TIMESTAMP, alarm.createTimeStamp);
        values.put(Constant.COLUMN_DAYS_OF_WEEK, alarm.daysOfWeek.toString());
        if(Utils.getLinkListSize(alarm.repeatDaysOfWeek) == 0) {
            values.put(Constant.COLUMN_REPEAT_DAYS_OF_WEEK, "");
        } else {
            values.put(Constant.COLUMN_REPEAT_DAYS_OF_WEEK, alarm.repeatDaysOfWeek.toString());
        }
        values.put(Constant.COLUMN_HOUR, alarm.hourOfDay);
        values.put(Constant.COLUMN_MINUTES, alarm.minutes);
        values.put(Constant.COLUMN_ENABLED, alarm.enable? 1 : 0);
        values.put(Constant.COLUMN_LABEL, alarm.label);
        values.put(Constant.COLUMN_ALERT_TIME,alarm.alertTime);
        return values;
    }

    public static Alarm getAlarmFromCursor(Cursor cursor){
        Alarm alarm = null;
        if (cursor != null && cursor.moveToFirst()) {
            long id = cursor.getLong(cursor.getColumnIndex(Constant.COLUMN_ID));
            long createTimeStamp = cursor.getLong(cursor.getColumnIndex(Constant.COLUMN_CREATE_TIMESTAMP));
            String daysOfWeek = cursor.getString(cursor.getColumnIndex(Constant.COLUMN_DAYS_OF_WEEK));
            String repeat = cursor.getString(cursor.getColumnIndex(Constant.COLUMN_REPEAT_DAYS_OF_WEEK));
            WeekdaysLinkList<String> str_repeatDaysOfWeek;
            if(TextUtils.isEmpty(repeat)) {
                str_repeatDaysOfWeek = new WeekdaysLinkList<>();
            } else {
                str_repeatDaysOfWeek = new WeekdaysLinkList<>(Arrays.asList(repeat.split(",")));
            }
            int hour = cursor.getInt(cursor.getColumnIndex(Constant.COLUMN_HOUR));
            int minutes = cursor.getInt(cursor.getColumnIndex(Constant.COLUMN_MINUTES));
            int enable = cursor.getInt(cursor.getColumnIndex(Constant.COLUMN_ENABLED));
            String label = cursor.getString(cursor.getColumnIndex(Constant.COLUMN_LABEL));
            long alertTime = cursor.getLong(cursor.getColumnIndex(Constant.COLUMN_ALERT_TIME));
            alarm = new Alarm(id,createTimeStamp,Weekdays.fromCalendarDay(daysOfWeek),
                    getRepeatDaysOfWeek(str_repeatDaysOfWeek), hour,minutes, enable == 1,label,alertTime);
            Log.d(TAG,"getAlarmFromCursor database alarm data : " +  alarm.toString());
            cursor.close();
        }
        return alarm;
    }

    public static List<Alarm> getAlarmListFromCursor(Cursor cursor){
        List<Alarm> ret = new ArrayList<>();
        if(cursor != null && cursor.getCount() > 0){
            cursor.moveToFirst();
            for(int i = 0; i < cursor.getCount();i++){
                long id = cursor.getLong(cursor.getColumnIndex(Constant.COLUMN_ID));
                long createTimeStamp = cursor.getLong(cursor.getColumnIndex(Constant.COLUMN_CREATE_TIMESTAMP));
                String daysOfWeek = cursor.getString(cursor.getColumnIndex(Constant.COLUMN_DAYS_OF_WEEK));
                String repeat = cursor.getString(cursor.getColumnIndex(Constant.COLUMN_REPEAT_DAYS_OF_WEEK));
                WeekdaysLinkList<String> str_repeatDaysOfWeek;
                if(TextUtils.isEmpty(repeat)) {
                    str_repeatDaysOfWeek = new WeekdaysLinkList<>();
                } else {
                    str_repeatDaysOfWeek = new WeekdaysLinkList<>(Arrays.asList(repeat.split(",")));
                }
                int hour = cursor.getInt(cursor.getColumnIndex(Constant.COLUMN_HOUR));
                int minutes = cursor.getInt(cursor.getColumnIndex(Constant.COLUMN_MINUTES));
                int enable = cursor.getInt(cursor.getColumnIndex(Constant.COLUMN_ENABLED));
                String label = cursor.getString(cursor.getColumnIndex(Constant.COLUMN_LABEL));
                long alertTime = cursor.getLong(cursor.getColumnIndex(Constant.COLUMN_ALERT_TIME));
                Alarm alarm = new Alarm(id,createTimeStamp,Weekdays.fromCalendarDay(daysOfWeek),
                        getRepeatDaysOfWeek(str_repeatDaysOfWeek),hour,minutes, enable == 1,label,alertTime);
                //Log.d(TAG,"getAlarmListFromCursor database alarm data : " +  alarm.toString());
                ret.add(alarm);
                cursor.moveToNext();
            }
            cursor.close();
        }
        return ret;
    }

    private static WeekdaysLinkList<Weekdays> getRepeatDaysOfWeek(WeekdaysLinkList<String> param){
        if(param == null || param.size() == 0) return null;
        WeekdaysLinkList<Weekdays> ret = new WeekdaysLinkList<>();
        for (String item : param) {
            Weekdays weekdays = Weekdays.fromCalendarDay(item);
            ret.add(weekdays);
        }
        return ret;
    }

    /**
     * get next time because need to update alarm
     */
    public Calendar getNextTime(Calendar currentTime){
        final Calendar nextInstanceTime = Calendar.getInstance(currentTime.getTimeZone());
        nextInstanceTime.set(Calendar.YEAR, currentTime.get(Calendar.YEAR));
        nextInstanceTime.set(Calendar.MONTH, currentTime.get(Calendar.MONTH));
        nextInstanceTime.set(Calendar.DAY_OF_YEAR, currentTime.get(Calendar.DAY_OF_YEAR));
        nextInstanceTime.set(Calendar.HOUR_OF_DAY, hourOfDay);
        nextInstanceTime.set(Calendar.MINUTE, minutes);
        nextInstanceTime.set(Calendar.SECOND, 0);

        // If we are still behind the passed in currentTime, then add a day
        if (nextInstanceTime.getTimeInMillis() <= currentTime.getTimeInMillis()) {
            nextInstanceTime.add(Calendar.DAY_OF_YEAR, 1);
        }

        Log.d(TAG,"getNextTime before : " + Utils.getTimeByFormat(nextInstanceTime.getTimeInMillis()));

        // The day of the week might be invalid, so find next valid one
        final int addDays = daysOfWeek.getDistanceToNextDay(nextInstanceTime);
        if (addDays > 0) {
            nextInstanceTime.add(Calendar.DAY_OF_WEEK, addDays);
        }
        Log.d(TAG,"daysOfWeek : " +daysOfWeek.toString() + "  addDays : " + addDays);

        // Daylight Savings Time can alter the hours and minutes when adjusting the day above.
        // Reset the desired hour and minute now that the correct day has been chosen.
        nextInstanceTime.set(Calendar.HOUR_OF_DAY, hourOfDay);
        nextInstanceTime.set(Calendar.MINUTE, minutes);

        Log.d(TAG,"getNextTime final : " + Utils.getTimeByFormat(nextInstanceTime.getTimeInMillis()));

        return nextInstanceTime;
    }

    private Weekdays getCurAlarmWeekdays(){
        final Calendar curCalendar = Calendar.getInstance();
        final Calendar nextInstanceTime = Calendar.getInstance();
        nextInstanceTime.set(Calendar.YEAR, curCalendar.get(Calendar.YEAR));
        nextInstanceTime.set(Calendar.MONTH, curCalendar.get(Calendar.MONTH));
        nextInstanceTime.set(Calendar.DAY_OF_YEAR, curCalendar.get(Calendar.DAY_OF_YEAR));
        nextInstanceTime.set(Calendar.HOUR_OF_DAY, hourOfDay);
        nextInstanceTime.set(Calendar.MINUTE, minutes);
        nextInstanceTime.set(Calendar.SECOND, 0);

        // If we are still behind the passed in currentTime, then add a day
        if (nextInstanceTime.getTimeInMillis() <= curCalendar.getTimeInMillis()) {
            nextInstanceTime.add(Calendar.DAY_OF_YEAR, 1);
        }

        int calendarDay = nextInstanceTime.get(Calendar.DAY_OF_WEEK);
        return mCalendarDayForWeekdays.get(calendarDay);
    }

    private Weekdays getNextWeekdays(Weekdays curWeekdays, WeekdaysLinkList<Weekdays> repeatDaysOfWeek){
        // for only_once alarm
        if(Utils.getLinkListSize(repeatDaysOfWeek) == 0){
            return Weekdays.getNextWeekDays(curWeekdays);
        } else if(Utils.getLinkListSize(repeatDaysOfWeek) == 1){
            return repeatDaysOfWeek.getFirst();
        } else {
            int size = repeatDaysOfWeek.size();
            int key = 0;
            for(int i = 0; i < size; i++){
                if(repeatDaysOfWeek.get(i).equals(curWeekdays)){
                    if(i != size - 1){
                        key = i + 1;
                        break;
                    } else{
                        key = 0;
                    }
                    break;
                }
            }
            return repeatDaysOfWeek.get(key);
        }
    }

    public boolean hasTimeOut(){
        Calendar calendar = Calendar.getInstance();
        long curTime = calendar.getTimeInMillis();
        boolean hasTimeOut =curTime > alertTime;
        Log.d(TAG,"alertTime : " + Utils.getTimeByFormat(alertTime) + "  curTime : " +
                Utils.getTimeByFormat(curTime) + " hasTimeOut ? : "+ hasTimeOut);
        return hasTimeOut;
    }

    public static Alarm createNewAlarmForSwitchEvent(Alarm data){
        if(data == null) return null;
        Alarm newAlarm = new Alarm(data.getId(),data.createTimeStamp,data.daysOfWeek,
                data.repeatDaysOfWeek,data.hourOfDay,data.minutes,true, data.label,data.alertTime);
        if(!newAlarm.hasTimeOut()){
            Log.d(TAG,"createNewAlarmForSwitchEvent non-TimeOut*****");
            newAlarm.daysOfWeek = data.daysOfWeek;
            newAlarm.repeatDaysOfWeek = getSortedRepeatDaysOfWeek(newAlarm.daysOfWeek,data.repeatDaysOfWeek);
        } else{
            Log.d(TAG,"createNewAlarmForSwitchEvent hasTimeOut*****");
            newAlarm.daysOfWeek = newAlarm.getNextWeekdays(data.daysOfWeek,data.repeatDaysOfWeek);
            newAlarm.repeatDaysOfWeek = newAlarm.getResetRepeatDaysOfWeek();
        }
        return newAlarm;
    }

    /**
     * get current alarm weekdays and reset repeatDaysOfWeek
     */
    private static WeekdaysLinkList<Weekdays> getSortedRepeatDaysOfWeek(Weekdays curWeekdays,WeekdaysLinkList<Weekdays> repeatDaysOfWeek){
        if(repeatDaysOfWeek == null || repeatDaysOfWeek.size() <= 1) return repeatDaysOfWeek;
        Collections.sort(repeatDaysOfWeek);
        WeekdaysLinkList<Weekdays> ret = new WeekdaysLinkList<>();
        int key = 0;
        Log.d(TAG,"getSortedRepeatDaysOfWeek curWeekdays : " + curWeekdays);

        int size = repeatDaysOfWeek.size();
        for (int i = 0; i < repeatDaysOfWeek.size(); i++) {
            if(repeatDaysOfWeek.get(i).compareTo(curWeekdays) >= 0){
                key = i;
                Log.d(TAG,"key : " + key + " size : " + size);

                break;
            }
        }
        for (int i = key; i < size ; i++){
            ret.add(repeatDaysOfWeek.get(i));
        }
        for (int i = 0; i < key ; i++){
            ret.add(repeatDaysOfWeek.get(i));
        }
        return ret;
    }

    /**
     * determine the weekday of the alarm based on the repeatDaysOfWeek、hour and minutes
     */
    private Weekdays getCurAlarmWeekdays(WeekdaysLinkList<Weekdays> repeatDaysOfWeek,int hour,int minutes){
        if(repeatDaysOfWeek == null) return Weekdays.NONE;
        if(repeatDaysOfWeek.size() == 1) return repeatDaysOfWeek.getFirst();
        Collections.sort(repeatDaysOfWeek);
        Iterator<Weekdays> it = repeatDaysOfWeek.iterator();
        Weekdays ret = Weekdays.NONE;
        while (it.hasNext()){
            Weekdays node = it.next();
            if(node.compareTo(Utils.getNowWeekdays()) > 0) {
                ret = node;
                Log.d(TAG," > node : " + node);
                break;
            } else if(node.compareTo(Utils.getNowWeekdays()) == 0) {
                Calendar currentTime = Calendar.getInstance();
                Log.d(TAG," == node : " + node + " currentTime : " + Utils.getTimeByFormat(currentTime.getTimeInMillis())
                + " compare : " + Utils.getTimeByFormat(Utils.getCompareCalendar(currentTime,hour,minutes).getTimeInMillis()));
                if(currentTime.getTimeInMillis() <= Utils.getCompareCalendar(currentTime,hour,minutes).getTimeInMillis()) {
                    ret = node;
                    break;
                }
            }
        }
        if(ret == Weekdays.NONE) return repeatDaysOfWeek.getFirst(); //filter
        return ret;
    }

    public WeekdaysLinkList<Weekdays> getResetRepeatDaysOfWeek(){
        if(repeatDaysOfWeek == null || repeatDaysOfWeek.size() <= 1) return repeatDaysOfWeek;
        Weekdays head = repeatDaysOfWeek.removeFirst();
        repeatDaysOfWeek.add(head);
        return repeatDaysOfWeek;
    }

    @Override
    public int hashCode() {
        return Long.valueOf(createTimeStamp).hashCode();
    }

    @Override
    public String toString() {
        return "Alarm{" +
                "id=" + id +
                ", createTimeStamp=" + createTimeStamp +
                ", daysOfWeek=" + daysOfWeek +
                ", repeatDaysOfWeek=" + repeatDaysOfWeek +
                ", hourOfDay=" + hourOfDay +
                ", minutes=" + minutes +
                ", enable=" + enable +
                ", label='" + label + '\'' +
                ", alertTime=" + Utils.getTimeByFormat(alertTime) +
                '}' + " hasCode : " + hashCode();
    }
}

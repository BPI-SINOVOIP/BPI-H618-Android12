package com.softwinner.timerswitch.data;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;

import com.softwinner.timerswitch.utils.Constant;
import com.softwinner.timerswitch.utils.Utils;

import java.util.ArrayList;
import java.util.List;

/**
 * alarm data layer,used to read and write date from database
 */
public class AlarmModel {

    private ContentResolver mContentResolver;

    public AlarmModel(ContentResolver contentResolver) {
        this.mContentResolver = contentResolver;
    }

    public void insert(Alarm alarm) {
        ContentValues contentValues = Alarm.createContentValues(alarm);
        mContentResolver.insert(Constant.CONTENT_URI_INSERT, contentValues);
    }

    public void deleteById(long alarm_id){
        mContentResolver.delete(Constant.CONTENT_URI_DELETE,Constant.COLUMN_ID + "=? ",
                new String[]{String.valueOf(alarm_id)});
    }

    public void updateById(Alarm alarm) {
        ContentValues contentValues = Alarm.createContentValues(alarm);
        mContentResolver.update(Constant.CONTENT_URI_UPDATE, contentValues, Constant.COLUMN_ID + "=? ",
                new String[]{String.valueOf(alarm.getId())});
    }

    public void updateAllTimeOutAlarm() {
        List<Alarm> alarmList =  findAll();
        if(alarmList != null) {
            for(Alarm alarm : alarmList){
                if(alarm.hasTimeOut() && Utils.getLinkListSize(alarm.repeatDaysOfWeek) == 0){
                    alarm.enable = false;
                    updateById(alarm);
                }
            }
        }
    }

    public List<Alarm> findAllNotTimeOutAlarms(){
        List<Alarm> alarmList =  findAll();
        List<Alarm> ret = new ArrayList<>();
        if(alarmList != null) {
            for(Alarm alarm : alarmList){
                if(!alarm.hasTimeOut()){
                    ret.add(alarm);
                }
            }
        }
        return ret;
    }

    public Alarm findByAlarmId(long alarm_id) {
        Cursor cursor = mContentResolver.query(Constant.CONTENT_URI_QUERY,
                new String[]{Constant.COLUMN_ID}, Constant.COLUMN_ID + "= " + alarm_id,
                null,null);
        return Alarm.getAlarmFromCursor(cursor);
    }

    public Alarm findByCreateTimeStamp(long createTimeStamp) {
        Cursor cursor = mContentResolver.query(Constant.CONTENT_URI_QUERY,
                new String[]{Constant.COLUMN_CREATE_TIMESTAMP}, Constant.COLUMN_CREATE_TIMESTAMP + "= " + createTimeStamp,
                null,null);
        return Alarm.getAlarmFromCursor(cursor);
    }

    public List<Alarm> findAll() {
        Cursor cursorAll = mContentResolver.query(Constant.CONTENT_URI_QUERY,null,null,null);
        return Alarm.getAlarmListFromCursor(cursorAll);
    }

    public List<Alarm> findBootAlarms(){
        Cursor cursorBoot = mContentResolver.query(Constant.CONTENT_URI_QUERY,
                new String[]{Constant.COLUMN_LABEL}, Constant.COLUMN_LABEL + "='boot'" ,
                null,null);
        return Alarm.getAlarmListFromCursor(cursorBoot);
    }

    public List<Alarm> findShutdownAlarms(){
        Cursor cursorShutdown = mContentResolver.query(Constant.CONTENT_URI_QUERY,
                new String[]{Constant.COLUMN_LABEL}, Constant.COLUMN_LABEL + "='shutdown'" ,
                null,null);
        return Alarm.getAlarmListFromCursor(cursorShutdown);
    }
}

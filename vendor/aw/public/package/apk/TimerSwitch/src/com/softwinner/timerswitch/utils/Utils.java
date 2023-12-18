package com.softwinner.timerswitch.utils;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Context;
import android.net.Uri;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TextView;
import com.softwinner.timerswitch.R;
import com.softwinner.timerswitch.WeekdaysLinkList;
import com.softwinner.timerswitch.callback.DialogCallBack;
import com.softwinner.timerswitch.data.Alarm;
import com.softwinner.timerswitch.data.Weekdays;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.Objects;

public class Utils {

    private static final String TAG = "TAG_Utils";

    public static String getTimeByFormat(long currentTime){
        @SuppressLint("SimpleDateFormat")
        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        return df.format(new Date(currentTime));
    }

    public static Calendar getCompareCalendar(Calendar currentTime,int hour,int minutes){
        final Calendar nextInstanceTime = Calendar.getInstance(currentTime.getTimeZone());
        nextInstanceTime.set(Calendar.YEAR, currentTime.get(Calendar.YEAR));
        nextInstanceTime.set(Calendar.MONTH, currentTime.get(Calendar.MONTH));
        nextInstanceTime.set(Calendar.DAY_OF_YEAR, currentTime.get(Calendar.DAY_OF_YEAR));
        nextInstanceTime.set(Calendar.HOUR_OF_DAY, hour);
        nextInstanceTime.set(Calendar.MINUTE, minutes);
        nextInstanceTime.set(Calendar.SECOND, currentTime.get(Calendar.SECOND));
        nextInstanceTime.set(Calendar.MILLISECOND, currentTime.get(Calendar.MILLISECOND));
        return nextInstanceTime;
    }

    public static Weekdays getNowWeekdays(){
        int calendarDay = Calendar.getInstance().get(Calendar.DAY_OF_WEEK);
        return Alarm.getCalendarDayForWeekdays().get(calendarDay);
    }

    public static int dp2px(Context context, float dpVal) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dpVal,
                context.getResources().getDisplayMetrics());
    }

    public static String getFormatHourOfDay(int hourOfDay){
        return hourOfDay < 10 ? "0" + hourOfDay :  "" + hourOfDay;
    }

    public static String getFormatMinutes(int minutes){
        return minutes < 10 ? "0" + minutes :  "" + minutes;
    }

    public static int getLinkListSize(WeekdaysLinkList<Weekdays> repeatDaysOfWeek){
        if(repeatDaysOfWeek == null || repeatDaysOfWeek.size() == 0) return 0;
        return repeatDaysOfWeek.size();
    }

    public static String getShowRepeatDaysOfWeek(Context context,WeekdaysLinkList<Weekdays> repeatDaysOfWeek){
        if(context == null || repeatDaysOfWeek == null) return "";
        StringBuilder ret = new StringBuilder();
        Collections.sort(repeatDaysOfWeek);
        for (Weekdays weekdays:repeatDaysOfWeek) {
            ret.append(getWeekdaysShowContent(context,weekdays)).append(" ");
        }
        return ret.toString();
    }

    public static long getIdByUri(Uri uri){
        if(uri == null) return -1;
        String id;
        String strUri = uri.toString();
        int lastIndex = strUri.lastIndexOf("/");
        id = strUri.substring(lastIndex + 1);
        Log.d(TAG,"getIdByUri uri: " +uri.toString() + " *** id: "+id);
        return Long.parseLong(id);
    }

    public static String getWeekdaysShowContent(Context context,Weekdays weekdays){
        if(context == null || weekdays == null) return "";
        String ret;
        switch (weekdays.getBits()){
            case 0x01 :
                ret = context.getResources().getString(R.string.MONDAY);
                break;
            case 0x02 :
                ret = context.getResources().getString(R.string.TUESDAY);
                break;
            case 0x04 :
                ret = context.getResources().getString(R.string.WEDNESDAY);
                break;
            case 0x08 :
                ret = context.getResources().getString(R.string.THURSDAY);
                break;
            case 0x10 :
                ret = context.getResources().getString(R.string.FRIDAY);
                break;
            case 0x20 :
                ret = context.getResources().getString(R.string.SATURDAY);
                break;
            case 0x40 :
                ret = context.getResources().getString(R.string.SUNDAY);
                break;
            default:
                ret = "";
        }
        return ret;
    }

    public static void showRepeatDialog(final Context context, final String repeatContent,final DialogCallBack callBack) {
        if(context == null) return;
        final Dialog bottomDialog = new Dialog(context, R.style.BottomDialog);
        @SuppressLint("InflateParams")
        View contentView = LayoutInflater.from(context).inflate(R.layout.dialog_repeat, null);
        final TextView tv_no_repeat = contentView.findViewById(R.id.tv_no_repeat);
        final TextView tv_everyday = contentView.findViewById(R.id.tv_everyday);
        final TextView tv_custom = contentView.findViewById(R.id.tv_custom);
        bottomDialog.setContentView(contentView);
        ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) contentView.getLayoutParams();
        params.width = context.getResources().getDisplayMetrics().widthPixels - Utils.dp2px(context, 20f);
        params.bottomMargin = Utils.dp2px(context, 20f);
        contentView.setLayoutParams(params);
        Objects.requireNonNull(bottomDialog.getWindow()).setGravity(Gravity.BOTTOM);
        bottomDialog.getWindow().setWindowAnimations(R.style.BottomDialog_Animation);
        tv_no_repeat.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(callBack!=null){
                    callBack.onClickRepeatEvent(tv_no_repeat.getText().toString());
                }
                bottomDialog.dismiss();
            }
        });
        tv_everyday.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(callBack!=null){
                    callBack.onClickRepeatEvent(tv_everyday.getText().toString());
                }
                bottomDialog.dismiss();
            }
        });
        tv_custom.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                bottomDialog.dismiss();
                showRepeatDaysOfWeekDialog(context,repeatContent,callBack);
            }
        });
        bottomDialog.show();
    }

    private static void showRepeatDaysOfWeekDialog(final Context context, final String repeatContent, final DialogCallBack callBack) {
        if(context == null) return;
        final Dialog bottomDialog = new Dialog(context, R.style.BottomDialog);
        @SuppressLint("InflateParams")
        View contentView = LayoutInflater.from(context).inflate(R.layout.dialog_repeat_daysofweek, null);
        final CheckBox cb_monday = contentView.findViewById(R.id.cb_monday);
        final CheckBox cb_tuesday = contentView.findViewById(R.id.cb_tuesday);
        final CheckBox cb_wednesday = contentView.findViewById(R.id.cb_wednesday);
        final CheckBox cb_thursday = contentView.findViewById(R.id.cb_thursday);
        final CheckBox cb_friday = contentView.findViewById(R.id.cb_friday);
        final CheckBox cb_saturday = contentView.findViewById(R.id.cb_saturday);
        final CheckBox cb_sunday = contentView.findViewById(R.id.cb_sunday);
        final Button bt_cancel = contentView.findViewById(R.id.bt_cancel);
        final Button bt_confirm = contentView.findViewById(R.id.bt_confirm);

        if(repeatContent != null){
            if(repeatContent.contains(context.getResources().getString(R.string.MONDAY))) {
                cb_monday.setChecked(true);
            }
            if(repeatContent.contains(context.getResources().getString(R.string.TUESDAY))) {
                cb_tuesday.setChecked(true);
            }
            if(repeatContent.contains(context.getResources().getString(R.string.WEDNESDAY))) {
                cb_wednesday.setChecked(true);
            }
            if(repeatContent.contains(context.getResources().getString(R.string.THURSDAY))) {
                cb_thursday.setChecked(true);
            }
            if(repeatContent.contains(context.getResources().getString(R.string.FRIDAY))) {
                cb_friday.setChecked(true);
            }
            if(repeatContent.contains(context.getResources().getString(R.string.SATURDAY))) {
                cb_saturday.setChecked(true);
            }
            if(repeatContent.contains(context.getResources().getString(R.string.SUNDAY))) {
                cb_sunday.setChecked(true);
            }
        }

        bottomDialog.setContentView(contentView);
        ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) contentView.getLayoutParams();
        params.width = context.getResources().getDisplayMetrics().widthPixels - Utils.dp2px(context, 20f);
        params.bottomMargin = Utils.dp2px(context, 20f);
        contentView.setLayoutParams(params);
        Objects.requireNonNull(bottomDialog.getWindow()).setGravity(Gravity.BOTTOM);
        bottomDialog.getWindow().setWindowAnimations(R.style.BottomDialog_Animation);
        final WeekdaysLinkList<Weekdays> repeatDaysOfWeek = new WeekdaysLinkList<>();
        if(cb_monday.isChecked()){
            repeatDaysOfWeek.add(Weekdays.MONDAY);
        }
        if(cb_tuesday.isChecked()){
            repeatDaysOfWeek.add(Weekdays.TUESDAY);
        }
        if(cb_wednesday.isChecked()){
            repeatDaysOfWeek.add(Weekdays.WEDNESDAY);
        }
        if(cb_thursday.isChecked()){
            repeatDaysOfWeek.add(Weekdays.THURSDAY);
        }
        if(cb_friday.isChecked()){
            repeatDaysOfWeek.add(Weekdays.FRIDAY);
        }
        if(cb_saturday.isChecked()){
            repeatDaysOfWeek.add(Weekdays.SATURDAY);
        }
        if(cb_sunday.isChecked()){
            repeatDaysOfWeek.add(Weekdays.SUNDAY);
        }
        cb_monday.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    repeatDaysOfWeek.add(Weekdays.MONDAY);
                } else {
                    repeatDaysOfWeek.remove(Weekdays.MONDAY);
                }
            }
        });
        cb_tuesday.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    repeatDaysOfWeek.add(Weekdays.TUESDAY);
                } else {
                    repeatDaysOfWeek.remove(Weekdays.TUESDAY);
                }
            }
        });
        cb_wednesday.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    repeatDaysOfWeek.add(Weekdays.WEDNESDAY);
                } else {
                    repeatDaysOfWeek.remove(Weekdays.WEDNESDAY);
                }
            }
        });
        cb_thursday.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    repeatDaysOfWeek.add(Weekdays.THURSDAY);
                } else {
                    repeatDaysOfWeek.remove(Weekdays.THURSDAY);
                }
            }
        });
        cb_friday.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    repeatDaysOfWeek.add(Weekdays.FRIDAY);
                } else {
                    repeatDaysOfWeek.remove(Weekdays.FRIDAY);
                }
            }
        });
        cb_saturday.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    repeatDaysOfWeek.add(Weekdays.SATURDAY);
                } else {
                    repeatDaysOfWeek.remove(Weekdays.SATURDAY);
                }
            }
        });
        cb_sunday.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    repeatDaysOfWeek.add(Weekdays.SUNDAY);
                } else {
                    repeatDaysOfWeek.remove(Weekdays.SUNDAY);
                }
            }
        });
        bt_cancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                bottomDialog.dismiss();
            }
        });
        bt_confirm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(callBack!=null){
                    String content = Utils.getShowRepeatDaysOfWeek(context,repeatDaysOfWeek);
                    callBack.onClickConfirmEvent(content);
                }
                bottomDialog.dismiss();
            }
        });
        bottomDialog.show();
    }

}

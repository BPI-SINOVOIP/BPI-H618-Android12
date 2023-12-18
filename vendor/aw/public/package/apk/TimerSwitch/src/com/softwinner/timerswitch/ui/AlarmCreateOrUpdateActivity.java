package com.softwinner.timerswitch.ui;

import android.app.TimePickerDialog;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.TimePicker;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import com.softwinner.timerswitch.WeekdaysLinkList;
import com.softwinner.timerswitch.callback.DialogCallBack;
import com.softwinner.timerswitch.R;
import com.softwinner.timerswitch.data.Alarm;
import com.softwinner.timerswitch.data.AlarmController;
import com.softwinner.timerswitch.data.AlarmModel;
import com.softwinner.timerswitch.data.Weekdays;
import com.softwinner.timerswitch.utils.Utils;
import java.util.Calendar;

public class AlarmCreateOrUpdateActivity extends AppCompatActivity implements View.OnClickListener{

    private static final String TAG = "TAG_AlarmCreateOrUpdateActivity";

    private LinearLayout ll_time;
    private LinearLayout ll_repeat;
    private TextView tv_time;
    private TextView tv_repeat_content;
    private Button bt_cancel;
    private Button bt_confirm;

    public static final String KEY_LABEL_TITLE = "key_label_title";
    public static final String KEY_USE_ALARM_TYPE = "key_use_alarm_type";
    public static final String KEY_TIME = "key_time";
    public static final String KEY_ALARM = "key_alarm";
    public static final String KEY_ALARM_LABEL = "key_alarm_label";
    public static final String KEY_REPEAT_CONTENT = "key_time_content";
    public static final int TYPE_CREATE_ALARM = 0x01;
    public static final int TYPE_UPDATE_ALARM = 0x02;

    private String time;
    private String repeat_content;
    private int type_alarm = -1;
    private String alarm_label;
    private Alarm editAlarm;

    private AlarmController controller;
    private AlarmModel model;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_alarm_create_or_update);

        String labelTitle = getIntent().getStringExtra(KEY_LABEL_TITLE);
        time = getIntent().getStringExtra(KEY_TIME);
        type_alarm = getIntent().getIntExtra(KEY_USE_ALARM_TYPE,-1);
        alarm_label = getIntent().getStringExtra(KEY_ALARM_LABEL);
        Bundle bundle = getIntent().getExtras();
        if(bundle != null){
            editAlarm = (Alarm) bundle.getSerializable(KEY_ALARM);
        }
        repeat_content = getIntent().getStringExtra(KEY_REPEAT_CONTENT);

        ActionBar actionBar = getSupportActionBar();
        if(actionBar != null){
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setTitle(labelTitle);
        }

        ll_time = findViewById(R.id.ll_time);
        ll_repeat = findViewById(R.id.ll_repeat);
        tv_time = findViewById(R.id.tv_time);
        tv_repeat_content = findViewById(R.id.tv_repeat_content);
        bt_cancel = findViewById(R.id.bt_cancel);
        bt_confirm = findViewById(R.id.bt_confirm);

        ll_time.setOnClickListener(this);
        ll_repeat.setOnClickListener(this);
        bt_cancel.setOnClickListener(this);
        bt_confirm.setOnClickListener(this);

        if (time == null) {
            Calendar cur = Calendar.getInstance();
            int hourOfDay = cur.get(Calendar.HOUR_OF_DAY);
            int minutes = cur.get(Calendar.MINUTE);
            String txt_hourOfDay = Utils.getFormatHourOfDay(hourOfDay);
            String txt_minutes = Utils.getFormatMinutes(minutes);
            time = txt_hourOfDay + ":" + txt_minutes;
        }
        tv_time.setText(time);
        if(TextUtils.isEmpty(repeat_content)){
            repeat_content = getResources().getString(R.string.only_once);
        }
        tv_repeat_content.setText(repeat_content);

        if(!TextUtils.isEmpty(labelTitle)){
            if(labelTitle.equals(getResources().getString(R.string.edit))){
                bt_cancel.setText(getResources().getString(R.string.delete));
            } else if(labelTitle.equals(getResources().getString(R.string.create))){
                bt_cancel.setText(getResources().getString(R.string.cancel));
            }
        }

        model = new AlarmModel(getContentResolver());
        controller = new AlarmController(this,model);
    }


    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onClick(View v) {
        if(v.getId() == R.id.ll_time){
            onClickTime();
        } else if(v.getId() == R.id.ll_repeat){
            onClickRepeat();
        } else if(v.getId() == R.id.bt_cancel){
            if(bt_cancel.getText().equals(getResources().getString(R.string.delete))){
                model.deleteById(editAlarm.getId());
                controller.closeTimingAlarm(editAlarm);
            }
            finish();
        } else if(v.getId() == R.id.bt_confirm){
            onClickConfirm();
        }
    }

    private void onClickTime(){
        String[] split = time.split(":");
        if(split.length == 2) {
            int pass_hourOfDay = Integer.parseInt(split[0]);
            int pass_minutes = Integer.parseInt(split[1]);
            showTimePickerDialog(pass_hourOfDay,pass_minutes);
        }
    }

    private void onClickRepeat(){
        Utils.showRepeatDialog(this,repeat_content, new DialogCallBack() {
            @Override
            public void onClickRepeatEvent(String msg) {
                repeat_content = msg;
                tv_repeat_content.setText(msg);
            }

            @Override
            public void onClickConfirmEvent(String msg) {
                repeat_content = msg;
                tv_repeat_content.setText(msg);
            }
        });
    }

    private void onClickConfirm(){
        String[] split = tv_time.getText().toString().split(":");
        if(split.length == 2) {
            int hourOfDay = Integer.parseInt(split[0]);
            int minutes = Integer.parseInt(split[1]);
            if(type_alarm == TYPE_CREATE_ALARM){
                startAlarmClock(hourOfDay,minutes);
                finish();
            } else if(type_alarm == TYPE_UPDATE_ALARM){
                if(checkNeedUpdateAlarm(hourOfDay,minutes,repeat_content)) {
                    Log.d(TAG,"need update alarm");
                    model.deleteById(editAlarm.getId());
                    startAlarmClock(hourOfDay,minutes);
                } else{
                    Log.d(TAG,"no need update alarm !!!");
                }
                finish();
            }
        }
    }

    private void showTimePickerDialog(int pass_hourOfDay,int pass_minutes){
        new TimePickerDialog(this, new TimePickerDialog.OnTimeSetListener() {
            @Override
            public void onTimeSet(TimePicker view, int hourOfDay, int minute) {
                String txt_hourOfDay = Utils.getFormatHourOfDay(hourOfDay);
                String txt_minutes = Utils.getFormatMinutes(minute);
                final String time_content = txt_hourOfDay + ":" + txt_minutes;
                tv_time.setText(time_content);
            }
        }, pass_hourOfDay, pass_minutes, true).show();
    }

    private boolean checkNeedUpdateAlarm(int hourOfDay,int minutes,String repeatContent){
        if(editAlarm == null || editAlarm.hourOfDay != hourOfDay || editAlarm.minutes != minutes){
            return true;
        }
        if(repeatContent.equals(getResources().getString(R.string.only_once))){
            return Utils.getLinkListSize(editAlarm.repeatDaysOfWeek) > 0;
        }
        if(repeatContent.equals(getResources().getString(R.string.everyday))) {
            return Utils.getLinkListSize(editAlarm.repeatDaysOfWeek) != 7;
        }
        return !Utils.getShowRepeatDaysOfWeek(this, editAlarm.repeatDaysOfWeek).equals(repeatContent);
    }


    private void startAlarmClock(int hourOfDay,int minutes){
        Alarm alarm;
        if (repeat_content.contains(getResources().getString(R.string.only_once))) {
            alarm = new Alarm(hourOfDay, minutes, alarm_label);
        } else if (repeat_content.contains(getResources().getString(R.string.everyday))) {
            WeekdaysLinkList<Weekdays> repeatDaysOfWeek = new WeekdaysLinkList<Weekdays>() {
                {
                    add(Weekdays.MONDAY);
                    add(Weekdays.TUESDAY);
                    add(Weekdays.WEDNESDAY);
                    add(Weekdays.THURSDAY);
                    add(Weekdays.FRIDAY);
                    add(Weekdays.SATURDAY);
                    add(Weekdays.SUNDAY);
                }
            };
            alarm = new Alarm(repeatDaysOfWeek, hourOfDay, minutes, alarm_label);
        } else {
            final WeekdaysLinkList<Weekdays> repeatDaysOfWeek = new WeekdaysLinkList<>();
            if (repeat_content.contains(getResources().getString(R.string.MONDAY))) {
                repeatDaysOfWeek.add(Weekdays.MONDAY);
            }
            if (repeat_content.contains(getResources().getString(R.string.TUESDAY))) {
                repeatDaysOfWeek.add(Weekdays.TUESDAY);
            }
            if (repeat_content.contains(getResources().getString(R.string.WEDNESDAY))) {
                repeatDaysOfWeek.add(Weekdays.WEDNESDAY);
            }
            if (repeat_content.contains(getResources().getString(R.string.THURSDAY))) {
                repeatDaysOfWeek.add(Weekdays.THURSDAY);
            }
            if (repeat_content.contains(getResources().getString(R.string.FRIDAY))) {
                repeatDaysOfWeek.add(Weekdays.FRIDAY);
            }
            if (repeat_content.contains(getResources().getString(R.string.SATURDAY))) {
                repeatDaysOfWeek.add(Weekdays.SATURDAY);
            }
            if (repeat_content.contains(getResources().getString(R.string.SUNDAY))) {
                repeatDaysOfWeek.add(Weekdays.SUNDAY);
            }
            alarm = new Alarm(repeatDaysOfWeek, hourOfDay, minutes, alarm_label);
        }
        controller.startTimingAlarm(alarm);
    }

}

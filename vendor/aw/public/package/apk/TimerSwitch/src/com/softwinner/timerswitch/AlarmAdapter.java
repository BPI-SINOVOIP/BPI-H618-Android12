package com.softwinner.timerswitch;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.Switch;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.softwinner.timerswitch.data.Alarm;
import com.softwinner.timerswitch.ui.AlarmCreateOrUpdateActivity;
import com.softwinner.timerswitch.utils.Constant;
import com.softwinner.timerswitch.utils.Utils;
import java.util.List;

public class AlarmAdapter extends RecyclerView.Adapter<AlarmAdapter.AlarmViewHolder> {

    private static final String TAG = "TAG_AlarmAdapter";

    private List<Alarm> alarmList;
    private Context mContext;
    private SwitchAlarmCallBack callBack;

    public AlarmAdapter(Context context,List<Alarm> alarmList,SwitchAlarmCallBack callBack){
        this.mContext = context;
        this.alarmList = alarmList;
        this.callBack = callBack;
    }

    @NonNull
    @Override
    public AlarmViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View item = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_alarm, parent, false);
        return new AlarmViewHolder(item);
    }

    @Override
    public void onBindViewHolder(@NonNull AlarmViewHolder viewHolder, int position) {
        final Alarm data = alarmList.get(position);
        String txt_hourOfDay;
        String txt_minutes;
        txt_hourOfDay = Utils.getFormatHourOfDay(data.hourOfDay);
        txt_minutes = Utils.getFormatMinutes(data.minutes);
        final String time = txt_hourOfDay + ":" + txt_minutes;
        viewHolder.tv_alarm_time.setText(time);
        viewHolder.tv_alarm_label.setText(data.label.equals(Alarm.LABEL_ALARM_BOOT) ?
                mContext.getResources().getString(R.string.boot):mContext.getResources().getString(R.string.shutdown));

        final String show_repeatDaysOfWeek;

        if(data.repeatDaysOfWeek!=null && data.repeatDaysOfWeek.size() == 7){
            show_repeatDaysOfWeek = mContext.getResources().getString(R.string.everyday);
        } else {
            show_repeatDaysOfWeek = Utils.getShowRepeatDaysOfWeek(mContext,data.repeatDaysOfWeek);
        }
        viewHolder.tv_alarm_repeatDaysOfWeek.setText(show_repeatDaysOfWeek);
        viewHolder.switch_alarm.setChecked(data.enable);
        viewHolder.ll_root.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(TAG,"adapter : dbAlarm : " + data.toString());
                Intent intent = new Intent(mContext, AlarmCreateOrUpdateActivity.class);
                intent.putExtra(AlarmCreateOrUpdateActivity.KEY_LABEL_TITLE,mContext.getResources().getString(R.string.edit));
                intent.putExtra(AlarmCreateOrUpdateActivity.KEY_TIME,time);
                intent.putExtra(AlarmCreateOrUpdateActivity.KEY_REPEAT_CONTENT,show_repeatDaysOfWeek);
                intent.putExtra(AlarmCreateOrUpdateActivity.KEY_USE_ALARM_TYPE, AlarmCreateOrUpdateActivity.TYPE_UPDATE_ALARM);
                intent.putExtra(AlarmCreateOrUpdateActivity.KEY_ALARM_LABEL,data.label);
                Bundle bundle = new Bundle();
                bundle.putSerializable(AlarmCreateOrUpdateActivity.KEY_ALARM,data);
                intent.putExtras(bundle);
                mContext.startActivity(intent);
            }
        });
        viewHolder.switch_alarm.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(callBack != null){
                    callBack.onChecked(data,isChecked);
                }
            }
        });
    }

    public void refreshUIForInsertOP(long id){
        Cursor cursor = mContext.getContentResolver().query(Constant.CONTENT_URI_QUERY,
                new String[]{Constant.COLUMN_ID}, Constant.COLUMN_ID + "= " + id,
                null,null);
        Alarm alarm = Alarm.getAlarmFromCursor(cursor);
        if(alarm == null) return;
        if (alarmList != null) {
            alarmList.add(alarm);
        }
        notifyDataSetChanged();
    }

    public void refreshUIForUpdateOP(long id){
        Cursor cursor = mContext.getContentResolver().query(Constant.CONTENT_URI_QUERY,
                new String[]{Constant.COLUMN_ID}, Constant.COLUMN_ID + "= " + id,
                null,null);
        Alarm alarm = Alarm.getAlarmFromCursor(cursor);
        if(alarm == null) return;
        if (alarmList != null) {
            for (int i = 0; i < alarmList.size(); i++) {
                Alarm data = alarmList.get(i);
                if (data.getId() == id) {
                    alarmList.set(i, alarm);
                    break;
                }
            }
        }
        notifyDataSetChanged();
    }

    public void refreshUIForDeleteOP(long id){
        if (alarmList != null) {
            for (int i = 0; i < alarmList.size(); i++) {
                Alarm data = alarmList.get(i);
                if (data.getId() == id) {
                    alarmList.remove(i);
                    break;
                }
            }
        }
        notifyDataSetChanged();
    }

    @Override
    public int getItemCount() {
        return alarmList == null ? 0 : alarmList.size();
    }

    public interface SwitchAlarmCallBack{
        void onChecked(Alarm data,boolean isCheck);
    }

    static class AlarmViewHolder extends RecyclerView.ViewHolder {

        private LinearLayout ll_root;
        private TextView tv_alarm_time;
        private TextView tv_alarm_label;
        private TextView tv_alarm_repeatDaysOfWeek;
        private Switch switch_alarm;

        public AlarmViewHolder(@NonNull View itemView) {
            super(itemView);
            ll_root = itemView.findViewById(R.id.ll_root);
            tv_alarm_time = itemView.findViewById(R.id.tv_alarm_time);
            tv_alarm_label = itemView.findViewById(R.id.tv_alarm_label);
            tv_alarm_repeatDaysOfWeek = itemView.findViewById(R.id.tv_alarm_repeatDaysOfWeek);
            switch_alarm = itemView.findViewById(R.id.switch_alarm);

        }
    }
}

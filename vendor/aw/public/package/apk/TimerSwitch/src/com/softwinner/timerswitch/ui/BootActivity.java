package com.softwinner.timerswitch.ui;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.softwinner.timerswitch.data.Alarm;
import com.softwinner.timerswitch.AlarmAdapter;
import com.softwinner.timerswitch.data.AlarmController;
import com.softwinner.timerswitch.data.AlarmModel;
import com.softwinner.timerswitch.R;
import com.softwinner.timerswitch.observer.DeleteContentObserver;
import com.softwinner.timerswitch.observer.InsertContentObserver;
import com.softwinner.timerswitch.observer.UpdateContentObserver;
import com.softwinner.timerswitch.utils.Constant;

import java.util.List;

public class BootActivity extends AppCompatActivity{

    private static final String TAG = "TAG_BootActivity";

    private LinearLayout ll_showAlarms;
    private LinearLayout ll_no_alarm;

    private RecyclerView rv_alarms;
    private AlarmAdapter adapter;
    private AlarmController controller;

    private List<Alarm> bootAlarms;

    private InsertContentObserver insertContentObserver;
    private DeleteContentObserver deleteContentObserver;
    private UpdateContentObserver updateContentObserver;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            long id = (long) msg.obj;
            switch (msg.what){
                case Constant.MSG_INSERT:
                    if(adapter != null) {
                        if(adapter.getItemCount() == 0){
                            rvSetAdapter();
                        }
                        adapter.refreshUIForInsertOP(id);
                    }
                    break;
                case Constant.MSG_DELETE:
                    if(adapter != null) {
                        adapter.refreshUIForDeleteOP(id);
                        if(adapter.getItemCount() == 0){
                            ll_no_alarm.setVisibility(View.VISIBLE);
                            ll_showAlarms.setVisibility(View.GONE);
                        }
                    }
                    break;
                case Constant.MSG_UPDATE:
                    if(adapter != null) {
                        adapter.refreshUIForUpdateOP(id);
                    }
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_boot);

        ActionBar actionBar = getSupportActionBar();
        if(actionBar != null){
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setTitle(getResources().getString(R.string.timing_boot));
        }
        rv_alarms = findViewById(R.id.rv_alarms);
        ll_showAlarms = findViewById(R.id.ll_showAlarms);
        ll_no_alarm = findViewById(R.id.ll_no_alarm);

        AlarmModel model = new AlarmModel(getContentResolver());
        controller = new AlarmController(this, model);

        bootAlarms = model.findBootAlarms();

        adapter = new AlarmAdapter(this, bootAlarms, new AlarmAdapter.SwitchAlarmCallBack() {
            @Override
            public void onChecked(Alarm data,boolean isCheck) {
                if(isCheck) {
                    Alarm newAlarm = Alarm.createNewAlarmForSwitchEvent(data);
                    controller.startTimingAlarm(newAlarm);
                } else{
                    controller.closeTimingAlarm(data);
                }
            }
        });

        initRecyclerView();

        insertContentObserver = new InsertContentObserver(mHandler);
        deleteContentObserver = new DeleteContentObserver(mHandler);
        updateContentObserver = new UpdateContentObserver(mHandler);
        getContentResolver().registerContentObserver(Constant.CONTENT_URI_INSERT,true,insertContentObserver);
        getContentResolver().registerContentObserver(Constant.CONTENT_URI_DELETE,true,deleteContentObserver);
        getContentResolver().registerContentObserver(Constant.CONTENT_URI_UPDATE,true,updateContentObserver);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        getContentResolver().unregisterContentObserver(insertContentObserver);
        getContentResolver().unregisterContentObserver(deleteContentObserver);
        getContentResolver().unregisterContentObserver(updateContentObserver);
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.features,menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
            case R.id.action_add:
                Intent intent = new Intent(this, AlarmCreateOrUpdateActivity.class);
                intent.putExtra(AlarmCreateOrUpdateActivity.KEY_LABEL_TITLE,getResources().getString(R.string.create));
                intent.putExtra(AlarmCreateOrUpdateActivity.KEY_USE_ALARM_TYPE, AlarmCreateOrUpdateActivity.TYPE_CREATE_ALARM);
                intent.putExtra(AlarmCreateOrUpdateActivity.KEY_ALARM_LABEL,Alarm.LABEL_ALARM_BOOT);
                startActivity(intent);
                return true;
        }
        return super.onOptionsItemSelected(item);
    }


    private void initRecyclerView(){
        if(bootAlarms == null || bootAlarms.size() == 0){
            ll_no_alarm.setVisibility(View.VISIBLE);
            ll_showAlarms.setVisibility(View.GONE);
        } else {
            rvSetAdapter();
        }
    }

    private void rvSetAdapter(){
        ll_no_alarm.setVisibility(View.GONE);
        ll_showAlarms.setVisibility(View.VISIBLE);
        LinearLayoutManager layoutManager = new LinearLayoutManager(this);
        layoutManager.setOrientation(LinearLayoutManager.VERTICAL);
        rv_alarms.setLayoutManager(layoutManager);
        rv_alarms.setAdapter(adapter);
    }
}
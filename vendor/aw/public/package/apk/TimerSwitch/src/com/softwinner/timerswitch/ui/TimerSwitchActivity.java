package com.softwinner.timerswitch.ui;

import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import com.softwinner.timerswitch.R;
import com.softwinner.timerswitch.data.Alarm;

public class TimerSwitchActivity extends AppCompatActivity implements View.OnClickListener {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_timer_switch);

        ActionBar actionBar = getSupportActionBar();
        if(actionBar != null){
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        LinearLayout ll_boot = findViewById(R.id.ll_boot);
        LinearLayout ll_shutdown = findViewById(R.id.ll_shutdown);

        ll_boot.setOnClickListener(this);
        ll_shutdown.setOnClickListener(this);

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
        if(v.getId() == R.id.ll_boot){
            Intent intent = new Intent(this, BootActivity.class);
            startActivity(intent);
        } else if(v.getId() == R.id.ll_shutdown){
            Intent intent = new Intent(this, ShutDownActivity.class);
            startActivity(intent);
        }

    }

}
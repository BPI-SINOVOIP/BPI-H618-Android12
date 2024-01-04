package com.softwinner.agingdragonbox;

import android.app.Activity;
import android.os.Bundle;
import android.widget.ListView;

public class CustomDefault extends Activity {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(new ListView(this));
    }
}

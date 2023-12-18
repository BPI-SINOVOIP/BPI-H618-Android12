package com.softwinner.tvfilemanager;

import android.app.Activity;
import android.os.Bundle;

/**
 * Created by ningxiangzhong on 15-3-9.
 */
public class PictureGridActivity extends Activity {

    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.picture_grid);
        getWindow().setBackgroundDrawableResource(R.drawable.grid_bg);
    }

    /*@Override
    public boolean onSearchRequested() {
        startActivity(new Intent(this, SearchActivity.class));
        return true;
    }*/
}

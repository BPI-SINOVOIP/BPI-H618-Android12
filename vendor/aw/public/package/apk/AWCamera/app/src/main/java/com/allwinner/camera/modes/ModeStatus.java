package com.allwinner.camera.modes;

import android.view.View;

public interface ModeStatus {
    void setCurrentMode();

    void onModeResume();

    void onModePause();

    void initView(View rootView);
}

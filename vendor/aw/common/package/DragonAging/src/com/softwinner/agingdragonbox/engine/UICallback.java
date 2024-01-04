package com.softwinner.agingdragonbox.engine;

import android.view.View;

public interface UICallback {

    int UI_COMPONENT_BTN_PASSED = 1;
    int UI_COMPONENT_BTN_FAILED = 2;

    void setCaseContent(View v);

    void onCaseCompleted();

    void setUiVisible(int component, int visible);
}

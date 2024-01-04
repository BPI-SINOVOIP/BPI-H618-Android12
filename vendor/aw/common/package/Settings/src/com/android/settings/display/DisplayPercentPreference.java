
package com.android.settings;

import android.content.Context;
import android.hardware.display.DisplayManager;
import android.view.Display;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import java.lang.Integer;
import android.os.SystemProperties;
import java.lang.Integer;
import java.lang.String;
import java.lang.Exception;
import com.android.settings.SeekBarDialogPreference;

import com.softwinner.display.DisplaydClient;
import softwinner.homlet.displayd.V1_0.ScreenMargin;

public class DisplayPercentPreference extends SeekBarDialogPreference implements
        SeekBar.OnSeekBarChangeListener {

    private final static String TAG = "DisplayPercentPreference";
    private final static int MAXIMUM_VALUE = 100;
    private final static int MINIMUM_VALUE =  90;

    private int mDisplayIndex = 0;
    private DisplaydClient mDispdClient;
    private SeekBar mSeekBar;
    private int mCurrentValue;

    public DisplayPercentPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mDispdClient = new DisplaydClient(null);
    }

    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);

        mSeekBar = getSeekBar(view);
        mSeekBar.setMax(MAXIMUM_VALUE - MINIMUM_VALUE);
        mCurrentValue = getDisplayPercent();
        mSeekBar.setProgress(mCurrentValue - MINIMUM_VALUE);
        mSeekBar.setOnSeekBarChangeListener(this);
    }

    public void onProgressChanged(SeekBar seekBar, int progress,
            boolean fromTouch) {
        setDisplayPercent(progress + MINIMUM_VALUE);
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        if (positiveResult) {
            setDisplayPercent(mSeekBar.getProgress() + MINIMUM_VALUE);
        } else {
            setDisplayPercent(mCurrentValue);
        }
        super.onDialogClosed(positiveResult);
    }


    private void setDisplayPercent(int value) {
        ScreenMargin margin = new ScreenMargin();
        margin.left   = value;
        margin.right  = value;
        margin.top    = value;
        margin.bottom = value;
        mDispdClient.setMargin(mDisplayIndex, margin);
    }

    private int getDisplayPercent() {
        ScreenMargin margin = mDispdClient.getMargin(mDisplayIndex);
        if (margin != null)
            return margin.left;
        return MAXIMUM_VALUE;
    }

    /* implements method in SeekBar.OnSeekBarChangeListener */
    @Override
    public void onStartTrackingTouch(SeekBar arg0) {
    }

    /* implements method in SeekBar.OnSeekBarChangeListener */
    @Override
    public void onStopTrackingTouch(SeekBar arg0) {
    }
}

/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.cts.verifier.audio;

import com.android.compatibility.common.util.ReportLog;
import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import android.graphics.Color;

import android.media.AudioDeviceCallback;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;

import android.os.Bundle;
import android.os.Handler;

import android.util.Log;

import android.view.KeyEvent;
import android.view.View;

import android.widget.Button;
import android.widget.TextView;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;  // needed to access resource in CTSVerifier project namespace.

// MegaPlayer
import org.hyphonate.megaaudio.player.AudioSourceProvider;
import org.hyphonate.megaaudio.player.JavaPlayer;
import org.hyphonate.megaaudio.player.PlayerBuilder;
import org.hyphonate.megaaudio.player.sources.SinAudioSourceProvider;

public class AnalogHeadsetAudioActivity
        extends PassFailButtons.Activity
        implements View.OnClickListener {
    private static final String TAG = AnalogHeadsetAudioActivity.class.getSimpleName();
    private static final boolean DEBUG = false;

    private AudioManager    mAudioManager;

    // UI
    private Button mHasAnalogPortYesBtn;
    private Button mHasAnalogPortNoBtn;

    private Button mPlayButton;
    private Button mStopButton;
    private Button mPlaybackSuccessBtn;
    private Button mPlaybackFailBtn;

    private TextView mHeadsetNameText;
    private TextView mHeadsetPlugMessage;

    private TextView mHeadsetHookText;
    private TextView mHeadsetVolUpText;
    private TextView mHeadsetVolDownText;

    // Devices
    private AudioDeviceInfo mHeadsetDeviceInfo;
    private boolean mHasHeadsetPort;
    private boolean mPlugIntentReceived;
    private boolean mPlaybackSuccess;

    // Intents
    private HeadsetPlugReceiver mHeadsetPlugReceiver;

    // Buttons
    private boolean mHasHeadsetHook;
    private boolean mHasPlayPause;
    private boolean mHasVolUp;
    private boolean mHasVolDown;

    // Player
    protected boolean mIsPlaying = false;

    // Mega Player
    static final int NUM_CHANNELS = 2;
    static final int SAMPLE_RATE = 48000;

    JavaPlayer mAudioPlayer;

    public AnalogHeadsetAudioActivity() {
        super();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.audio_headset_audio_activity);

        mHeadsetNameText = (TextView)findViewById(R.id.headset_analog_name);
        mHeadsetPlugMessage = (TextView)findViewById(R.id.headset_analog_plug_message);

        // Analog Port?
        mHasAnalogPortYesBtn = (Button)findViewById(R.id.headset_analog_port_yes);
        mHasAnalogPortYesBtn.setOnClickListener(this);
        mHasAnalogPortNoBtn = (Button)findViewById(R.id.headset_analog_port_no);
        mHasAnalogPortNoBtn.setOnClickListener(this);

        // Player Controls.
        mPlayButton = (Button)findViewById(R.id.headset_analog_play);
        mPlayButton.setOnClickListener(this);
        mStopButton = (Button)findViewById(R.id.headset_analog_stop);
        mStopButton.setOnClickListener(this);

        // Play Status
        mPlaybackSuccessBtn = (Button)findViewById(R.id.headset_analog_play_yes);
        mPlaybackSuccessBtn.setOnClickListener(this);
        mPlaybackFailBtn = (Button)findViewById(R.id.headset_analog_play_no);
        mPlaybackFailBtn.setOnClickListener(this);
        mPlaybackSuccessBtn.setEnabled(false);
        mPlaybackFailBtn.setEnabled(false);

        // Keycodes
        mHeadsetHookText = (TextView)findViewById(R.id.headset_keycode_headsethook);
        mHeadsetVolUpText = (TextView)findViewById(R.id.headset_keycode_volume_up);
        mHeadsetVolDownText = (TextView)findViewById(R.id.headset_keycode_volume_down);

        mAudioManager = (AudioManager)getSystemService(AUDIO_SERVICE);

        setupPlayer();

        mAudioManager.registerAudioDeviceCallback(new ConnectListener(), new Handler());

        mHeadsetPlugReceiver = new HeadsetPlugReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
        registerReceiver(mHeadsetPlugReceiver, filter);

        showKeyMessagesState();

        setInfoResources(R.string.analog_headset_test, R.string.analog_headset_test_info, -1);

        setPassFailButtonClickListeners();
        getPassButton().setEnabled(false);
    }

    //
    // Reporting
    //
    private boolean calculatePass() {
        if (!mHasHeadsetPort) {
            return true;
        } else {
            return mPlugIntentReceived &&
                    mHeadsetDeviceInfo != null &&
                    mPlaybackSuccess &&
                    (mHasHeadsetHook || mHasPlayPause) && mHasVolUp && mHasVolDown;
        }
    }

    private void reportHeadsetPort(boolean has) {
        mHasHeadsetPort = has;
        getReportLog().addValue(
                "User Reports Headset Port",
                has ? 1 : 0,
                ResultType.NEUTRAL,
                ResultUnit.NONE);
        if (has) {
            mHasAnalogPortNoBtn.setEnabled(false);
        } else {
            mHasAnalogPortYesBtn.setEnabled(false);
        }
        enablePlayerButtons(has && mHeadsetDeviceInfo != null);

        if (!has) {
            // no port, so can't test. Let them pass
            getPassButton().setEnabled(true);
        }
    }

    private void reportPlugIntent(Intent intent) {
        // [C-1-4] MUST trigger ACTION_HEADSET_PLUG upon a plug insert,
        // but only after all contacts on plug are touching their relevant segments on the jack.
        mPlugIntentReceived = true;

        // state - 0 for unplugged, 1 for plugged.
        // name - Headset type, human readable string
        // microphone - 1 if headset has a microphone, 0 otherwise

        int state = intent.getIntExtra("state", -1);
        if (state != -1) {

            StringBuilder sb = new StringBuilder();
            sb.append("ACTION_HEADSET_PLUG received - " + (state == 0 ? "Unplugged" : "Plugged"));

            String name = intent.getStringExtra("name");
            if (name != null) {
                sb.append(" - " + name);
            }

            int hasMic = intent.getIntExtra("microphone", 0);
            if (hasMic == 1) {
                sb.append(" [mic]");
            }

            mHeadsetPlugMessage.setText(sb.toString());
        }
        getReportLog().addValue(
                "ACTION_HEADSET_PLUG Intent Received. State: ",
                state,
                ResultType.NEUTRAL,
                ResultUnit.NONE);
    }

    private void reportPlaybackStatus(boolean success) {
        // [C-1-1] MUST support audio playback to stereo headphones
        // and stereo headsets with a microphone.
        mPlaybackSuccess = success;
        if (success) {
            mPlaybackFailBtn.setEnabled(false);
        } else {
            mPlaybackSuccessBtn.setEnabled(false);
        }

        mPlaybackSuccessBtn.setEnabled(success);
        mPlaybackFailBtn.setEnabled(success);

        getPassButton().setEnabled(calculatePass());

        getReportLog().addValue(
                "User reported headset/headphones playback",
                success ? 1 : 0,
                ResultType.NEUTRAL,
                ResultUnit.NONE);
    }

    //
    // UI
    //
    private void showConnectedDevice() {
        if (mHeadsetDeviceInfo != null) {
            mHeadsetNameText.setText(
                    mHeadsetDeviceInfo.getType() == AudioDeviceInfo.TYPE_WIRED_HEADSET
                    ? "Headset Connected"
                    : "Headphones Connected");
        } else {
            mHeadsetNameText.setText("No Headset/Headphones Connected");
        }
    }

    private void enablePlayerButtons(boolean enabled) {
        mPlayButton.setEnabled(enabled);
        mStopButton.setEnabled(enabled);
    }

    private void showKeyMessagesState() {
        mHeadsetHookText.setTextColor((mHasHeadsetHook || mHasPlayPause)
                ? Color.WHITE : Color.GRAY);
        mHeadsetVolUpText.setTextColor(mHasVolUp ? Color.WHITE : Color.GRAY);
        mHeadsetVolDownText.setTextColor(mHasVolDown ? Color.WHITE : Color.GRAY);
    }

    //
    // Player
    //
    protected void setupPlayer() {
        //
        // Allocate the source provider for the sort of signal we want to play
        //
        AudioSourceProvider sourceProvider = new SinAudioSourceProvider();
        try {
            PlayerBuilder builder = new PlayerBuilder();
            mAudioPlayer = (JavaPlayer)builder
                    // choose one or the other of these for a Java or an Oboe player
                    .setPlayerType(PlayerBuilder.TYPE_JAVA)
                    // .setPlayerType(PlayerBuilder.PLAYER_OBOE)
                    .setSourceProvider(sourceProvider)
                    .build();
        } catch (PlayerBuilder.BadStateException ex) {
            Log.e(TAG, "Failed MegaPlayer build.");
        }
    }

    protected void startPlay() {
        if (!mIsPlaying) {
            //TODO - explain the choice of 96 here.
            mAudioPlayer.setupStream(NUM_CHANNELS, SAMPLE_RATE, 96);
            mAudioPlayer.startStream();
            mIsPlaying = true;
        }
    }

    protected void stopPlay() {
        if (mIsPlaying) {
            mAudioPlayer.stopStream();
            mAudioPlayer.teardownStream();
            mIsPlaying = false;
        }
    }

    //
    // View.OnClickHandler
    //
    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.headset_analog_port_yes:
                reportHeadsetPort(true);
                break;

            case R.id.headset_analog_port_no:
                reportHeadsetPort(false);
                break;

            case R.id.headset_analog_play:
                startPlay();
                break;

            case R.id.headset_analog_stop:
                stopPlay();
                mPlaybackSuccessBtn.setEnabled(true);
                mPlaybackFailBtn.setEnabled(true);
                break;

            case R.id.headset_analog_play_yes:
                reportPlaybackStatus(true);
                break;

            case R.id.headset_analog_play_no:
                reportPlaybackStatus(false);
                break;
        }
    }

    //
    // Devices
    //
    private void scanPeripheralList(AudioDeviceInfo[] devices) {
        mHeadsetDeviceInfo = null;
        for(AudioDeviceInfo devInfo : devices) {
            if (devInfo.getType() == AudioDeviceInfo.TYPE_WIRED_HEADSET ||
                    devInfo.getType() == AudioDeviceInfo.TYPE_WIRED_HEADPHONES) {
                mHeadsetDeviceInfo = devInfo;

                getReportLog().addValue(
                        (devInfo.getType() == AudioDeviceInfo.TYPE_WIRED_HEADSET
                                ? "Headset" : "Headphones") + " connected",
                        0,
                        ResultType.NEUTRAL,
                        ResultUnit.NONE);
                break;
            }
        }

        showConnectedDevice();
        enablePlayerButtons(mHeadsetDeviceInfo != null);
    }

    private class ConnectListener extends AudioDeviceCallback {
        /*package*/ ConnectListener() {}

        //
        // AudioDevicesManager.OnDeviceConnectionListener
        //
        @Override
        public void onAudioDevicesAdded(AudioDeviceInfo[] addedDevices) {
            Log.i(TAG, "onAudioDevicesAdded() num:" + addedDevices.length);

            scanPeripheralList(mAudioManager.getDevices(AudioManager.GET_DEVICES_ALL));
        }

        @Override
        public void onAudioDevicesRemoved(AudioDeviceInfo[] removedDevices) {
            Log.i(TAG, "onAudioDevicesRemoved() num:" + removedDevices.length);

            scanPeripheralList(mAudioManager.getDevices(AudioManager.GET_DEVICES_ALL));
        }
    }

    private class HeadsetPlugReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            reportPlugIntent(intent);
        }
    }

    //
    // Keycodes
    //
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // Log.i(TAG, "onKeyDown(" + keyCode + ")");
        switch (keyCode) {
            case KeyEvent.KEYCODE_HEADSETHOOK:
                mHasHeadsetHook = true;
                showKeyMessagesState();
                getPassButton().setEnabled(calculatePass());
                getReportLog().addValue(
                        "KEYCODE_HEADSETHOOK", 1, ResultType.NEUTRAL, ResultUnit.NONE);
                break;

            case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                mHasPlayPause = true;
                showKeyMessagesState();
                getPassButton().setEnabled(calculatePass());
                getReportLog().addValue(
                        "KEYCODE_MEDIA_PLAY_PAUSE", 1, ResultType.NEUTRAL, ResultUnit.NONE);
                break;

            case KeyEvent.KEYCODE_VOLUME_UP:
                mHasVolUp = true;
                showKeyMessagesState();
                getPassButton().setEnabled(calculatePass());
                getReportLog().addValue(
                        "KEYCODE_VOLUME_UP", 1, ResultType.NEUTRAL, ResultUnit.NONE);
                break;

            case KeyEvent.KEYCODE_VOLUME_DOWN:
                mHasVolDown = true;
                showKeyMessagesState();
                getPassButton().setEnabled(calculatePass());
                getReportLog().addValue(
                        "KEYCODE_VOLUME_DOWN", 1, ResultType.NEUTRAL, ResultUnit.NONE);
                break;
        }
        return super.onKeyDown(keyCode, event);
    }
}

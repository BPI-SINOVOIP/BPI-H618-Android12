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

package com.android.car.audio.hal;

import static com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport.DUMP_INFO;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.hardware.automotive.audiocontrol.DuckingInfo;
import android.hardware.automotive.audiocontrol.IAudioControl;
import android.hardware.automotive.audiocontrol.IFocusListener;
import android.hardware.automotive.audiocontrol.MutingInfo;
import android.media.AudioAttributes;
import android.media.AudioAttributes.AttributeUsage;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.IndentingPrintWriter;
import android.util.Log;
import android.util.Slog;

import com.android.car.CarLog;
import com.android.car.audio.CarDuckingInfo;
import com.android.car.internal.ExcludeFromCodeCoverageGeneratedReport;
import com.android.internal.util.Preconditions;

import java.util.List;
import java.util.Objects;

/**
 * Wrapper for AIDL interface for AudioControl HAL
 */
public final class AudioControlWrapperAidl implements AudioControlWrapper {
    private static final String TAG = CarLog.tagFor(AudioControlWrapperAidl.class);
    private static final String AUDIO_CONTROL_SERVICE =
            "android.hardware.automotive.audiocontrol.IAudioControl/default";
    private IBinder mBinder;
    private IAudioControl mAudioControl;
    private boolean mListenerRegistered = false;

    private AudioControlDeathRecipient mDeathRecipient;

    static @Nullable IBinder getService() {
        return Binder.allowBlocking(ServiceManager.waitForDeclaredService(
                AUDIO_CONTROL_SERVICE));
    }

    AudioControlWrapperAidl(IBinder binder) {
        mBinder = Objects.requireNonNull(binder);
        mAudioControl = IAudioControl.Stub.asInterface(binder);
    }

    @Override
    public void unregisterFocusListener() {
        // Focus listener will be unregistered by HAL automatically
    }

    @Override
    public boolean supportsFeature(int feature) {
        switch (feature) {
            case AUDIOCONTROL_FEATURE_AUDIO_FOCUS:
            case AUDIOCONTROL_FEATURE_AUDIO_DUCKING:
            case AUDIOCONTROL_FEATURE_AUDIO_GROUP_MUTING:
                return true;
            default:
                return false;
        }
    }

    @Override
    public void registerFocusListener(HalFocusListener focusListener) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "Registering focus listener on AudioControl HAL");
        }
        IFocusListener listenerWrapper = new FocusListenerWrapper(focusListener);
        try {
            mAudioControl.registerFocusListener(listenerWrapper);
        } catch (RemoteException e) {
            Slog.e(TAG, "Failed to register focus listener");
            throw new IllegalStateException("IAudioControl#registerFocusListener failed", e);
        }
        mListenerRegistered = true;
    }

    @Override
    public void onAudioFocusChange(@AttributeUsage int usage, int zoneId, int focusChange) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Slog.d(TAG, "onAudioFocusChange: usage " + AudioAttributes.usageToString(usage)
                    + ", zoneId " + zoneId + ", focusChange " + focusChange);
        }
        try {
            String usageName = AudioAttributes.usageToXsdString(usage);
            mAudioControl.onAudioFocusChange(usageName, zoneId, focusChange);
        } catch (RemoteException e) {
            throw new IllegalStateException("Failed to query IAudioControl#onAudioFocusChange", e);
        }
    }

    @Override
    @ExcludeFromCodeCoverageGeneratedReport(reason = DUMP_INFO)
    public void dump(IndentingPrintWriter writer) {
        writer.println("*AudioControlWrapperAidl*");
        writer.increaseIndent();
        writer.printf("Focus listener registered on HAL? %b\n", mListenerRegistered);

        writer.println("Supported Features");
        writer.increaseIndent();
        writer.println("- AUDIOCONTROL_FEATURE_AUDIO_FOCUS");
        writer.println("- AUDIOCONTROL_FEATURE_AUDIO_DUCKING");
        writer.decreaseIndent();

        writer.decreaseIndent();
    }

    @Override
    public void setFadeTowardFront(float value) {
        try {
            mAudioControl.setFadeTowardFront(value);
        } catch (RemoteException e) {
            Slog.e(TAG, "setFadeTowardFront with " + value + " failed", e);
        }
    }

    @Override
    public void setBalanceTowardRight(float value) {
        try {
            mAudioControl.setBalanceTowardRight(value);
        } catch (RemoteException e) {
            Slog.e(TAG, "setBalanceTowardRight with " + value + " failed", e);
        }
    }

    @Override
    public void onDevicesToDuckChange(@NonNull List<CarDuckingInfo> carDuckingInfos) {
        Objects.requireNonNull(carDuckingInfos);
        DuckingInfo[] duckingInfos = new DuckingInfo[carDuckingInfos.size()];
        for (int i = 0; i < carDuckingInfos.size(); i++) {
            CarDuckingInfo info = Objects.requireNonNull(carDuckingInfos.get(i));
            duckingInfos[i] = info.generateDuckingInfo();
        }

        try {
            mAudioControl.onDevicesToDuckChange(duckingInfos);
        } catch (RemoteException e) {
            Slog.e(TAG, "onDevicesToDuckChange failed", e);
        }
    }

    @Override
    public void onDevicesToMuteChange(@NonNull List<MutingInfo> carZonesMutingInfo) {
        Objects.requireNonNull(carZonesMutingInfo, "Muting info can not be null");
        Preconditions.checkArgument(!carZonesMutingInfo.isEmpty(), "Muting info can not be empty");
        MutingInfo[] mutingInfoToHal = carZonesMutingInfo
                .toArray(new MutingInfo[carZonesMutingInfo.size()]);
        try {
            mAudioControl.onDevicesToMuteChange(mutingInfoToHal);
        } catch (RemoteException e) {
            Slog.e(TAG, "onDevicesToMuteChange failed", e);
        }
    }

    @Override
    public void linkToDeath(@Nullable AudioControlDeathRecipient deathRecipient) {
        try {
            mBinder.linkToDeath(this::binderDied, 0);
            mDeathRecipient = deathRecipient;
        } catch (RemoteException e) {
            throw new IllegalStateException("Call to IAudioControl#linkToDeath failed", e);
        }
    }

    @Override
    public void unlinkToDeath() {
        mBinder.unlinkToDeath(this::binderDied, 0);
        mDeathRecipient = null;
    }

    private void binderDied() {
        Slog.w(TAG, "AudioControl HAL died. Fetching new handle");
        mListenerRegistered = false;
        mBinder = AudioControlWrapperAidl.getService();
        mAudioControl = IAudioControl.Stub.asInterface(mBinder);
        linkToDeath(mDeathRecipient);
        if (mDeathRecipient != null) {
            mDeathRecipient.serviceDied();
        }
    }

    private static final class FocusListenerWrapper extends IFocusListener.Stub {
        private final HalFocusListener mListener;

        FocusListenerWrapper(HalFocusListener halFocusListener) {
            mListener = halFocusListener;
        }

        @Override
        public int getInterfaceVersion() {
            return this.VERSION;
        }

        @Override
        public String getInterfaceHash() {
            return this.HASH;
        }

        @Override
        public void requestAudioFocus(String usage, int zoneId, int focusGain) {
            @AttributeUsage int usageValue = AudioAttributes.xsdStringToUsage(usage);
            mListener.requestAudioFocus(usageValue, zoneId, focusGain);
        }

        @Override
        public void abandonAudioFocus(String usage, int zoneId) {
            @AttributeUsage int usageValue = AudioAttributes.xsdStringToUsage(usage);
            mListener.abandonAudioFocus(usageValue, zoneId);
        }
    }
}

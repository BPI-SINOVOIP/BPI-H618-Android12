/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.cts.verifier.audio.audiolib;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioDeviceInfo;
import android.util.Log;

public class AudioSystemFlags {
    static final String TAG = AudioSystemFlags.class.getName();

    public static boolean claimsOutput(Context context) {
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT);
    }

    public static boolean claimsInput(Context context) {
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MICROPHONE);
    }

    public static boolean claimsProAudio(Context context) {
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_PRO);
    }

    public static boolean claimsLowLatencyAudio(Context context) {
        // CDD Section C-1-1: android.hardware.audio.low_latency
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_LOW_LATENCY);
    }

    public static boolean claimsMIDI(Context context) {
        // CDD Section C-1-4: android.software.midi
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MIDI);
    }

    public static boolean claimsUSBHostMode(Context context) {
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_USB_HOST);
    }

    public static boolean claimsUSBPeripheralMode(Context context) {
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_USB_ACCESSORY);
    }
}

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

package android.media.cts;

import android.content.pm.PackageManager;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.util.Log;

import androidx.test.filters.SdkSuppress;

import com.android.compatibility.common.util.CtsAndroidTestCase;
import com.android.internal.annotations.GuardedBy;

import java.util.List;
import java.util.concurrent.Executors;


@SdkSuppress(minSdkVersion = 31, codeName = "S")
public class AudioCommunicationDeviceTest extends CtsAndroidTestCase {
    private final static String TAG = "AudioCommunicationDeviceTest";

    private AudioManager mAudioManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mAudioManager = getInstrumentation().getContext().getSystemService(AudioManager.class);
    }

    public void testSetValidCommunicationDevice() {
        if (!isValidPlatform("testSetValidCommunicationDevice")) return;

        AudioDeviceInfo commDevice = null;
        List<AudioDeviceInfo> devices = mAudioManager.getAvailableCommunicationDevices();
        for (AudioDeviceInfo device : devices) {
            try {
                mAudioManager.setCommunicationDevice(device);
                try {
                    commDevice = mAudioManager.getCommunicationDevice();
                } catch (Exception e) {
                    fail("getCommunicationDevice failed with exception: " + e);
                }
                if (commDevice == null || commDevice.getType() != device.getType()) {
                    fail("setCommunicationDevice failed, expected device: "
                            + device.getType() + " but got: "
                            + ((commDevice == null)
                                ? AudioDeviceInfo.TYPE_UNKNOWN : commDevice.getType()));
                }
            } catch (Exception e) {
                fail("setCommunicationDevice failed with exception: " + e);
            }
        }

        try {
            mAudioManager.clearCommunicationDevice();
        } catch (Exception e) {
            fail("clearCommunicationDevice failed with exception: " + e);
        }
        try {
            commDevice = mAudioManager.getCommunicationDevice();
        } catch (Exception e) {
            fail("getCommunicationDevice failed with exception: " + e);
        }
        if (commDevice == null) {
            fail("platform has no default communication device");
        }
    }

    public void testSetInvalidCommunicationDevice() {
        if (!isValidPlatform("testSetInvalidCommunicationDevice")) return;

        AudioDeviceInfo[] alldevices = mAudioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
        List<AudioDeviceInfo> validDevices = mAudioManager.getAvailableCommunicationDevices();

        for (AudioDeviceInfo device : alldevices) {
            if (validDevices.contains(device)) {
                continue;
            }
            try {
                mAudioManager.setCommunicationDevice(device);
                fail("setCommunicationDevice should fail for device: " + device.getType());
            } catch (Exception e) {
            }
        }
    }

    static class MyOnCommunicationDeviceChangedListener implements
            AudioManager.OnCommunicationDeviceChangedListener {

        private final Object mCbLock = new Object();
        @GuardedBy("mCbLock")
        private boolean mCalled;
        @GuardedBy("mCbLock")
        private AudioDeviceInfo mDevice;

        private static final int LISTENER_WAIT_TIMEOUT_MS = 3000;
        void reset() {
            synchronized (mCbLock) {
                mCalled = false;
                mDevice = null;
            }
        }

        AudioDeviceInfo waitForDeviceUpdate() {
            synchronized (mCbLock) {
                while (!mCalled) {
                    try {
                        mCbLock.wait(LISTENER_WAIT_TIMEOUT_MS);
                    } catch (InterruptedException e) {
                    }
                }
                return mDevice;
            }
        }

        AudioDeviceInfo getDevice() {
            synchronized (mCbLock) {
                return mDevice;
            }
        }

        MyOnCommunicationDeviceChangedListener() {
            reset();
        }

        @Override
        public void onCommunicationDeviceChanged(AudioDeviceInfo device) {
            synchronized (mCbLock) {
                mCalled = true;
                mDevice = device;
                mCbLock.notifyAll();
            }
        }
    }

    public void testCommunicationDeviceListener() {
        if (!isValidPlatform("testCommunicationDeviceListener")) return;

        MyOnCommunicationDeviceChangedListener listener =
                new MyOnCommunicationDeviceChangedListener();

        try {
            mAudioManager.addOnCommunicationDeviceChangedListener(null, listener);
            fail("addOnCommunicationDeviceChangedListener should fail with null executor");
        } catch (Exception e) {
        }

        try {
            mAudioManager.addOnCommunicationDeviceChangedListener(
                    Executors.newSingleThreadExecutor(), null);
            fail("addOnCommunicationDeviceChangedListener should fail with null listener");
        } catch (Exception e) {
        }

        try {
            mAudioManager.removeOnCommunicationDeviceChangedListener(null);
            fail("removeOnCommunicationDeviceChangedListener should fail with null listener");
        } catch (Exception e) {
        }

        try {
            mAudioManager.addOnCommunicationDeviceChangedListener(
                Executors.newSingleThreadExecutor(), listener);
        } catch (Exception e) {
            fail("addOnCommunicationDeviceChangedListener failed with exception: "
                    + e);
        }

        try {
            mAudioManager.addOnCommunicationDeviceChangedListener(
                Executors.newSingleThreadExecutor(), listener);
            fail("addOnCommunicationDeviceChangedListener succeeded for same listener");
        } catch (Exception e) {
        }

        AudioDeviceInfo originalDevice = mAudioManager.getCommunicationDevice();
        assertNotNull("Platform as no default communication device", originalDevice);

        AudioDeviceInfo requestedDevice = null;
        List<AudioDeviceInfo> devices = mAudioManager.getAvailableCommunicationDevices();

        for (AudioDeviceInfo device : devices) {
            if (device.getType() != originalDevice.getType()) {
                requestedDevice = device;
                break;
            }
        }
        if (requestedDevice == null) {
            Log.i(TAG,"Skipping end of testCommunicationDeviceListener test,"
                    +" no valid decice to test");
            return;
        }
        mAudioManager.setCommunicationDevice(requestedDevice);
        AudioDeviceInfo listenerDevice = listener.waitForDeviceUpdate();
        if (listenerDevice == null || listenerDevice.getType() != requestedDevice.getType()) {
            fail("listener and setter device mismatch, expected device: "
                    + requestedDevice.getType() + " but got: "
                    + ((listenerDevice == null)
                        ? AudioDeviceInfo.TYPE_UNKNOWN : listenerDevice.getType()));
        }
        AudioDeviceInfo getterDevice = mAudioManager.getCommunicationDevice();
        if (getterDevice == null || getterDevice.getType() != listenerDevice.getType()) {
            fail("listener and getter device mismatch, expected device: "
                    + listenerDevice.getType() + " but got: "
                    + ((getterDevice == null)
                        ? AudioDeviceInfo.TYPE_UNKNOWN : getterDevice.getType()));
        }

        listener.reset();

        mAudioManager.setCommunicationDevice(originalDevice);

        listenerDevice = listener.waitForDeviceUpdate();
        assertNotNull("Platform as no default communication device", listenerDevice);

        if (listenerDevice.getType() != originalDevice.getType()) {
            fail("communication device listener failed on clear, expected device: "
                    + originalDevice.getType() + " but got: " + listenerDevice.getType());
        }

        try {
            mAudioManager.removeOnCommunicationDeviceChangedListener(listener);
        } catch (Exception e) {
            fail("removeOnCommunicationDeviceChangedListener failed with exception: "
                    + e);
        }
    }

    private boolean isValidPlatform(String testName) {
        PackageManager pm = getInstrumentation().getContext().getPackageManager();
        if (!pm.hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT)
                ||  pm.hasSystemFeature(PackageManager.FEATURE_LEANBACK_ONLY)
                || !pm.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Log.i(TAG,"Skipping test " + testName
                    + " : device has no audio output or is a TV or does not support telephony");
            return false;
        }
        return true;
    }
}

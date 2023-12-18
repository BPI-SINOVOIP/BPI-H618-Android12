/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.media.tv.cts;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.media.AudioManager;
import android.media.tv.cts.TvViewTest.MockCallback;
import android.media.tv.TunedInfo;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputManager.Hardware;
import android.media.tv.TvInputManager.HardwareCallback;
import android.media.tv.TvInputService;
import android.media.tv.TvStreamConfig;
import android.media.tv.TvView;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.test.ActivityInstrumentationTestCase2;
import android.tv.cts.R;

import com.android.compatibility.common.util.PollingCheck;

import androidx.test.InstrumentationRegistry;

import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.Executor;
import java.util.List;

import org.xmlpull.v1.XmlPullParserException;

/**
 * Test for {@link android.media.tv.TvInputManager}.
 */
public class TvInputManagerTest extends ActivityInstrumentationTestCase2<TvViewStubActivity> {
    /** The maximum time to wait for an operation. */
    private static final long TIME_OUT_MS = 15000L;

    private static final String[] VALID_TV_INPUT_SERVICES = {
        StubTunerTvInputService.class.getName()
    };
    private static final String[] INVALID_TV_INPUT_SERVICES = {
        NoMetadataTvInputService.class.getName(), NoPermissionTvInputService.class.getName()
    };
    private static final TvContentRating DUMMY_RATING = TvContentRating.createRating(
            "com.android.tv", "US_TV", "US_TV_PG", "US_TV_D", "US_TV_L");

    private String mStubId;
    private TvInputManager mManager;
    private LoggingCallback mCallback = new LoggingCallback();
    private TvInputInfo mStubTvInputInfo;
    private TvView mTvView;
    private Activity mActivity;
    private Instrumentation mInstrumentation;
    private TvInputInfo mStubTunerTvInputInfo;
    private final MockCallback mMockCallback = new MockCallback();

    private static TvInputInfo getInfoForClassName(List<TvInputInfo> list, String name) {
        for (TvInputInfo info : list) {
            if (info.getServiceInfo().name.equals(name)) {
                return info;
            }
        }
        return null;
    }

    public TvInputManagerTest() {
        super(TvViewStubActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        if (!Utils.hasTvInputFramework(mActivity)) {
            return;
        }
        mInstrumentation = getInstrumentation();
        mTvView = findTvViewById(R.id.tvview);
        mManager = (TvInputManager) mActivity.getSystemService(Context.TV_INPUT_SERVICE);
        mStubId = getInfoForClassName(
                mManager.getTvInputList(), StubTvInputService2.class.getName()).getId();
        mStubTvInputInfo = getInfoForClassName(
                mManager.getTvInputList(), StubTvInputService2.class.getName());
        for (TvInputInfo info : mManager.getTvInputList()) {
            if (info.getServiceInfo().name.equals(StubTunerTvInputService.class.getName())) {
                mStubTunerTvInputInfo = info;
                break;
            }
        }
        assertNotNull(mStubTunerTvInputInfo);
        mTvView.setCallback(mMockCallback);

        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .adoptShellPermissionIdentity();
    }

    @Override
    protected void tearDown() throws Exception {
        if (!Utils.hasTvInputFramework(getActivity())) {
            super.tearDown();
            return;
        }
        StubTunerTvInputService.deleteChannels(
                mActivity.getContentResolver(), mStubTunerTvInputInfo);
        StubTunerTvInputService.clearTracks();
        try {
            runTestOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mTvView.reset();
                }
            });
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
        mInstrumentation.waitForIdleSync();

        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .dropShellPermissionIdentity();

        super.tearDown();
    }

    private TvView findTvViewById(int id) {
        return (TvView) mActivity.findViewById(id);
    }

    private void tryTuneAllChannels() throws Throwable {
        StubTunerTvInputService.insertChannels(
                mActivity.getContentResolver(), mStubTunerTvInputInfo);

        Uri uri = TvContract.buildChannelsUriForInput(mStubTunerTvInputInfo.getId());
        String[] projection = { TvContract.Channels._ID };
        try (Cursor cursor = mActivity.getContentResolver().query(
                uri, projection, null, null, null)) {
            while (cursor != null && cursor.moveToNext()) {
                long channelId = cursor.getLong(0);
                Uri channelUri = TvContract.buildChannelUri(channelId);
                mCallback.mTunedInfos = null;
                mTvView.tune(mStubTunerTvInputInfo.getId(), channelUri);
                mInstrumentation.waitForIdleSync();
                new PollingCheck(TIME_OUT_MS) {
                    @Override
                    protected boolean check() {
                        return mMockCallback.isVideoAvailable(mStubTunerTvInputInfo.getId());
                    }
                }.run();
                new PollingCheck(TIME_OUT_MS) {
                    @Override
                    protected boolean check() {
                        return mCallback.mTunedInfos != null;
                    }
                }.run();

                List<TunedInfo> returnedInfos = mManager.getCurrentTunedInfos();
                assertEquals(1, returnedInfos.size());
                TunedInfo returnedInfo = returnedInfos.get(0);
                TunedInfo expectedInfo = new TunedInfo(
                        "android.tv.cts/android.media.tv.cts.StubTunerTvInputService",
                        channelUri,
                        false,
                        false,
                        false,
                        TunedInfo.APP_TYPE_SELF,
                        TunedInfo.APP_TAG_SELF);
                assertEquals(expectedInfo, returnedInfo);

                assertEquals(expectedInfo.getAppTag(), returnedInfo.getAppTag());
                assertEquals(expectedInfo.getAppType(), returnedInfo.getAppType());
                assertEquals(expectedInfo.getChannelUri(), returnedInfo.getChannelUri());
                assertEquals(expectedInfo.getInputId(), returnedInfo.getInputId());
                assertEquals(expectedInfo.isMainSession(), returnedInfo.isMainSession());
                assertEquals(expectedInfo.isRecordingSession(), returnedInfo.isRecordingSession());
                assertEquals(expectedInfo.isVisible(), returnedInfo.isVisible());

                assertEquals(1, mCallback.mTunedInfos.size());
                TunedInfo callbackInfo = mCallback.mTunedInfos.get(0);
                assertEquals(expectedInfo, callbackInfo);
            }
        }
    }

    public void testGetCurrentTunedInfos() throws Throwable {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.registerCallback(mCallback, new Handler());
            }
        });
        tryTuneAllChannels();
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.unregisterCallback(mCallback);
            }
        });
    }

    public void testGetInputState() throws Exception {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        assertEquals(mManager.getInputState(mStubId), TvInputManager.INPUT_STATE_CONNECTED);
    }

    public void testGetTvInputInfo() throws Exception {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        TvInputInfo expected = mManager.getTvInputInfo(mStubId);
        TvInputInfo actual = getInfoForClassName(mManager.getTvInputList(),
                StubTvInputService2.class.getName());
        assertTrue("expected=" + expected + " actual=" + actual,
                TvInputInfoTest.compareTvInputInfos(getActivity(), expected, actual));
    }

    public void testGetTvInputList() throws Exception {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        List<TvInputInfo> list = mManager.getTvInputList();
        for (String name : VALID_TV_INPUT_SERVICES) {
            assertNotNull("getTvInputList() doesn't contain valid input: " + name,
                    getInfoForClassName(list, name));
        }
        for (String name : INVALID_TV_INPUT_SERVICES) {
            assertNull("getTvInputList() contains invalind input: " + name,
                    getInfoForClassName(list, name));
        }
    }

    public void testIsParentalControlsEnabled() {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        try {
            mManager.isParentalControlsEnabled();
        } catch (Exception e) {
            fail();
        }
    }

    public void testIsRatingBlocked() {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        try {
            mManager.isRatingBlocked(DUMMY_RATING);
        } catch (Exception e) {
            fail();
        }
    }

    public void testRegisterUnregisterCallback() {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    mManager.registerCallback(mCallback, new Handler());
                    mManager.unregisterCallback(mCallback);
                } catch (Exception e) {
                    fail();
                }
            }
        });
        getInstrumentation().waitForIdleSync();
    }

    public void testInputAddedAndRemoved() {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.registerCallback(mCallback, new Handler());
            }
        });
        getInstrumentation().waitForIdleSync();

        // Test if onInputRemoved() is called.
        mCallback.resetLogs();
        PackageManager pm = getActivity().getPackageManager();
        ComponentName component = new ComponentName(getActivity(), StubTvInputService2.class);
        assertTrue(PackageManager.COMPONENT_ENABLED_STATE_DISABLED != pm.getComponentEnabledSetting(
                component));
        pm.setComponentEnabledSetting(component, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                PackageManager.DONT_KILL_APP);
        new PollingCheck(TIME_OUT_MS) {
            @Override
            protected boolean check() {
                return mCallback.isInputRemoved(mStubId);
            }
        }.run();

        // Test if onInputAdded() is called.
        mCallback.resetLogs();
        assertEquals(PackageManager.COMPONENT_ENABLED_STATE_DISABLED, pm.getComponentEnabledSetting(
                component));
        pm.setComponentEnabledSetting(component, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP);
        new PollingCheck(TIME_OUT_MS) {
            @Override
            protected boolean check() {
                return mCallback.isInputAdded(mStubId);
            }
        }.run();

        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.unregisterCallback(mCallback);
            }
        });
        getInstrumentation().waitForIdleSync();
    }

    public void testTvInputInfoUpdated() throws IOException, XmlPullParserException {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.registerCallback(mCallback, new Handler());
            }
        });
        getInstrumentation().waitForIdleSync();

        mCallback.resetLogs();
        TvInputInfo defaultInfo = new TvInputInfo.Builder(getActivity(),
                new ComponentName(getActivity(), StubTunerTvInputService.class)).build();
        TvInputInfo updatedInfo = new TvInputInfo.Builder(getActivity(),
                new ComponentName(getActivity(), StubTunerTvInputService.class))
                        .setTunerCount(10).setCanRecord(true).setCanPauseRecording(false).build();

        mManager.updateTvInputInfo(updatedInfo);
        new PollingCheck(TIME_OUT_MS) {
            @Override
            protected boolean check() {
                TvInputInfo info = mCallback.getLastUpdatedTvInputInfo();
                return info !=  null && info.getTunerCount() == 10 && info.canRecord()
                        && !info.canPauseRecording();
            }
        }.run();

        mManager.updateTvInputInfo(defaultInfo);
        new PollingCheck(TIME_OUT_MS) {
            @Override
            protected boolean check() {
                TvInputInfo info = mCallback.getLastUpdatedTvInputInfo();
                return info !=  null && info.getTunerCount() == 1 && !info.canRecord()
                        && info.canPauseRecording();
            }
        }.run();

        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mManager.unregisterCallback(mCallback);
            }
        });
        getInstrumentation().waitForIdleSync();
    }

    public void testAcquireTvInputHardware() {
        if (mManager == null) {
            return;
        }
        // Update hardware device list
        int deviceId = 0;
        boolean hardwareDeviceAdded = false;
        List<TvInputHardwareInfo> hardwareList = mManager.getHardwareList();
        if (hardwareList == null || hardwareList.isEmpty()) {
            // Use the test api to add an HDMI hardware device
            mManager.addHardwareDevice(deviceId);
            hardwareDeviceAdded = true;
        } else {
            deviceId = hardwareList.get(0).getDeviceId();
        }

        // Acquire Hardware with a record client
        HardwareCallback callback = new HardwareCallback() {
            @Override
            public void onReleased() {}

            @Override
            public void onStreamConfigChanged(TvStreamConfig[] configs) {}
        };
        CallbackExecutor executor = new CallbackExecutor();
        Hardware hardware = mManager.acquireTvInputHardware(
                deviceId, mStubTvInputInfo, null /*tvInputSessionId*/,
                TvInputService.PRIORITY_HINT_USE_CASE_TYPE_PLAYBACK,
                executor, callback);
        assertNotNull(hardware);

        // Acquire the same device with a LIVE client
        Hardware hardwareAcquired = mManager.acquireTvInputHardware(
                deviceId, mStubTvInputInfo, null /*tvInputSessionId*/,
                TvInputService.PRIORITY_HINT_USE_CASE_TYPE_LIVE,
                executor, callback);

        assertNotNull(hardwareAcquired);

        // Clean up
        if (hardwareDeviceAdded) {
            mManager.removeHardwareDevice(deviceId);
        }
    }

    private static class LoggingCallback extends TvInputManager.TvInputCallback {
        private final List<String> mAddedInputs = new ArrayList<>();
        private final List<String> mRemovedInputs = new ArrayList<>();
        private TvInputInfo mLastUpdatedTvInputInfo;
        private List<TunedInfo> mTunedInfos;

        @Override
        public synchronized void onInputAdded(String inputId) {
            mAddedInputs.add(inputId);
        }

        @Override
        public synchronized void onInputRemoved(String inputId) {
            mRemovedInputs.add(inputId);
        }

        @Override
        public synchronized void onTvInputInfoUpdated(TvInputInfo info) {
            mLastUpdatedTvInputInfo = info;
        }

        @Override
        public synchronized void onCurrentTunedInfosUpdated(
                List<TunedInfo> tunedInfos) {
            super.onCurrentTunedInfosUpdated(tunedInfos);
            mTunedInfos = tunedInfos;
        }

        public synchronized void resetLogs() {
            mAddedInputs.clear();
            mRemovedInputs.clear();
            mLastUpdatedTvInputInfo = null;
        }

        public synchronized boolean isInputAdded(String inputId) {
            return mRemovedInputs.isEmpty() && mAddedInputs.size() == 1 && mAddedInputs.contains(
                    inputId);
        }

        public synchronized boolean isInputRemoved(String inputId) {
            return mAddedInputs.isEmpty() && mRemovedInputs.size() == 1 && mRemovedInputs.contains(
                    inputId);
        }

        public synchronized TvInputInfo getLastUpdatedTvInputInfo() {
            return mLastUpdatedTvInputInfo;
        }
    }

    public static class StubTvInputService2 extends StubTvInputService {
        @Override
        public Session onCreateSession(String inputId) {
            return null;
        }
    }

    public class CallbackExecutor implements Executor {
        @Override
        public void execute(Runnable r) {
            r.run();
        }
    }
}

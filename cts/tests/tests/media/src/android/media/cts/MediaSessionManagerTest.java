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
package android.media.cts;

import static android.Manifest.permission.MEDIA_CONTENT_CONTROL;

import android.media.AudioManager;
import android.platform.test.annotations.AppModeFull;
import com.android.compatibility.common.util.ApiLevelUtil;
import com.android.compatibility.common.util.MediaUtils;
import com.android.compatibility.common.util.SystemUtil;

import android.content.ComponentName;
import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.media.MediaSession2;
import android.media.Session2CommandGroup;
import android.media.Session2Token;
import android.media.session.MediaController;
import android.media.session.MediaSession;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Process;
import android.platform.test.annotations.AppModeFull;
import android.test.InstrumentationTestCase;
import android.test.UiThreadTest;
import android.util.Log;
import android.view.KeyEvent;

import com.android.compatibility.common.util.SystemUtil;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

@AppModeFull(reason = "TODO: evaluate and port to instant")
public class MediaSessionManagerTest extends InstrumentationTestCase {
    private static final String TAG = "MediaSessionManagerTest";
    private static final int TIMEOUT_MS = 3000;
    private static final int WAIT_MS = 500;

    private AudioManager mAudioManager;
    private MediaSessionManager mSessionManager;

    private static boolean sIsAtLeastS = ApiLevelUtil.isAtLeast(Build.VERSION_CODES.S);

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mAudioManager = (AudioManager) getInstrumentation().getTargetContext()
                .getSystemService(Context.AUDIO_SERVICE);
        mSessionManager = (MediaSessionManager) getInstrumentation().getTargetContext()
                .getSystemService(Context.MEDIA_SESSION_SERVICE);
    }

    @Override
    protected void tearDown() throws Exception {
        getInstrumentation().getUiAutomation().dropShellPermissionIdentity();
        super.tearDown();
    }

    public void testGetActiveSessions() throws Exception {
        try {
            List<MediaController> controllers = mSessionManager.getActiveSessions(null);
            fail("Expected security exception for unauthorized call to getActiveSessions");
        } catch (SecurityException e) {
            // Expected
        }
        // TODO enable a notification listener, test again, disable, test again
    }

    public void testGetMediaKeyEventSession_throwsSecurityException() throws Exception {
        if (!MediaUtils.check(sIsAtLeastS, "test invalid before Android 12")) return;
        try {
            mSessionManager.getMediaKeyEventSession();
            fail("Expected security exception for call to getMediaKeyEventSession");
        } catch (SecurityException ex) {
            // Expected
        }
    }

    public void testOnMediaKeyEventSessionChangedListener() throws Exception {
        // The permission can be held only on S+
        if (!MediaUtils.check(sIsAtLeastS, "test invalid before Android 12")) return;

        getInstrumentation().getUiAutomation()
                .adoptShellPermissionIdentity(Manifest.permission.MEDIA_CONTENT_CONTROL);

        MediaKeyEventSessionListener keyEventSessionListener = new MediaKeyEventSessionListener();
        mSessionManager.addOnMediaKeyEventSessionChangedListener(
                Executors.newSingleThreadExecutor(), keyEventSessionListener);

        MediaSession session = new MediaSession(getInstrumentation().getTargetContext(), TAG);
        session.setFlags(MediaSession.FLAG_HANDLES_MEDIA_BUTTONS
                | MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);
        PlaybackState state = new PlaybackState.Builder()
                .setState(PlaybackState.STATE_PLAYING, 0, 1.0f).build();
        // Fake the media session service so this session can take the media key events.
        session.setPlaybackState(state);
        session.setActive(true);
        Utils.assertMediaPlaybackStarted(getInstrumentation().getTargetContext());

        assertTrue(keyEventSessionListener.mCountDownLatch
                .await(TIMEOUT_MS, TimeUnit.MILLISECONDS));

        assertEquals(session.getSessionToken(), mSessionManager.getMediaKeyEventSession());

        mSessionManager.removeOnMediaKeyEventSessionChangedListener(keyEventSessionListener);
        keyEventSessionListener.resetCountDownLatch();

        session.release();
        // This shouldn't be called because the callback is removed
        assertFalse(keyEventSessionListener.mCountDownLatch.await(WAIT_MS, TimeUnit.MILLISECONDS));
    }

    public void testOnMediaKeyEventDispatchedListener() throws Exception {
        // The permission can be held only on S+
        if (!MediaUtils.check(sIsAtLeastS, "test invalid before Android 12")) return;

        getInstrumentation().getUiAutomation()
                .adoptShellPermissionIdentity(Manifest.permission.MEDIA_CONTENT_CONTROL);

        MediaKeyEventDispatchedListener keyEventDispatchedListener =
                new MediaKeyEventDispatchedListener();
        mSessionManager.addOnMediaKeyEventDispatchedListener(Executors.newSingleThreadExecutor(),
                keyEventDispatchedListener);

        MediaSession session = new MediaSession(getInstrumentation().getTargetContext(), TAG);
        session.setFlags(MediaSession.FLAG_HANDLES_MEDIA_BUTTONS
                | MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);
        PlaybackState state = new PlaybackState.Builder()
                .setState(PlaybackState.STATE_PLAYING, 0, 1.0f).build();
        // Fake the media session service so this session can take the media key events.
        session.setPlaybackState(state);
        session.setActive(true);
        Utils.assertMediaPlaybackStarted(getInstrumentation().getTargetContext());

        final int keyCode = KeyEvent.KEYCODE_MEDIA_PLAY;
        simulateMediaKeyInput(keyCode);
        assertTrue(keyEventDispatchedListener.mCountDownLatch
                .await(TIMEOUT_MS, TimeUnit.MILLISECONDS));

        assertEquals(keyCode, keyEventDispatchedListener.mKeyEvent.getKeyCode());
        assertEquals(getInstrumentation().getTargetContext().getPackageName(),
                keyEventDispatchedListener.mPackageName);
        assertEquals(session.getSessionToken(), keyEventDispatchedListener.mSessionToken);

        mSessionManager.removeOnMediaKeyEventDispatchedListener(keyEventDispatchedListener);
        keyEventDispatchedListener.resetCountDownLatch();

        simulateMediaKeyInput(keyCode);
        // This shouldn't be called because the callback is removed
        assertFalse(keyEventDispatchedListener.mCountDownLatch
                .await(WAIT_MS, TimeUnit.MILLISECONDS));

        session.release();
    }

    @UiThreadTest
    public void testAddOnActiveSessionsListener() throws Exception {
        if (!MediaUtils.check(sIsAtLeastS, "test invalid before Android 12")) return;
        try {
            mSessionManager.addOnActiveSessionsChangedListener(null, null);
            fail("Expected NPE for call to addOnActiveSessionsChangedListener");
        } catch (NullPointerException e) {
            // Expected
        }

        MediaSessionManager.OnActiveSessionsChangedListener listener
                = new MediaSessionManager.OnActiveSessionsChangedListener() {
            @Override
            public void onActiveSessionsChanged(List<MediaController> controllers) {

            }
        };
        try {
            mSessionManager.addOnActiveSessionsChangedListener(listener, null);
            fail("Expected security exception for call to addOnActiveSessionsChangedListener");
        } catch (SecurityException e) {
            // Expected
        }
    }

    private void assertKeyEventEquals(KeyEvent lhs, int keyCode, int action, int repeatCount) {
        assertTrue(lhs.getKeyCode() == keyCode
                && lhs.getAction() == action
                && lhs.getRepeatCount() == repeatCount);
    }

    private void injectInputEvent(int keyCode, boolean longPress) throws IOException {
        // Injecting key with instrumentation requires a window/view, but we don't have it.
        // Inject key event through the adb commend to workaround.
        final String command = "input keyevent " + (longPress ? "--longpress " : "") + keyCode;
        SystemUtil.runShellCommand(getInstrumentation(), command);
    }

    public void testSetOnVolumeKeyLongPressListener() throws Exception {
        Context context = getInstrumentation().getTargetContext();
        if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK)
                || context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH)
                || context.getResources().getBoolean(Resources.getSystem().getIdentifier(
                        "config_handleVolumeKeysInWindowManager", "bool", "android"))) {
            // Skip this test, because the PhoneWindowManager dispatches volume key
            // events directly to the audio service to change the system volume.
            return;
        }
        Handler handler = createHandler();

        // Ensure that the listener is called for long-press.
        VolumeKeyLongPressListener listener = new VolumeKeyLongPressListener(3, handler);
        mSessionManager.setOnVolumeKeyLongPressListener(listener, handler);
        injectInputEvent(KeyEvent.KEYCODE_VOLUME_DOWN, true);
        assertTrue(listener.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
        assertEquals(listener.mKeyEvents.size(), 3);
        assertKeyEventEquals(listener.mKeyEvents.get(0),
                KeyEvent.KEYCODE_VOLUME_DOWN, KeyEvent.ACTION_DOWN, 0);
        assertKeyEventEquals(listener.mKeyEvents.get(1),
                KeyEvent.KEYCODE_VOLUME_DOWN, KeyEvent.ACTION_DOWN, 1);
        assertKeyEventEquals(listener.mKeyEvents.get(2),
                KeyEvent.KEYCODE_VOLUME_DOWN, KeyEvent.ACTION_UP, 0);

        // Ensure the the listener isn't called for short-press.
        listener = new VolumeKeyLongPressListener(1, handler);
        mSessionManager.setOnVolumeKeyLongPressListener(listener, handler);
        injectInputEvent(KeyEvent.KEYCODE_VOLUME_DOWN, false);
        assertFalse(listener.mCountDownLatch.await(WAIT_MS, TimeUnit.MILLISECONDS));
        assertEquals(listener.mKeyEvents.size(), 0);

        // Ensure that the listener isn't called anymore.
        mSessionManager.setOnVolumeKeyLongPressListener(null, handler);
        injectInputEvent(KeyEvent.KEYCODE_VOLUME_DOWN, true);
        assertFalse(listener.mCountDownLatch.await(WAIT_MS, TimeUnit.MILLISECONDS));
        assertEquals(listener.mKeyEvents.size(), 0);

        removeHandler(handler);
    }

    public void testSetOnMediaKeyListener() throws Exception {
        Handler handler = createHandler();
        MediaSession session = null;
        try {
            session = new MediaSession(getInstrumentation().getTargetContext(), TAG);
            MediaSessionCallback callback = new MediaSessionCallback(2, session);
            session.setFlags(MediaSession.FLAG_HANDLES_MEDIA_BUTTONS
                    | MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);
            session.setCallback(callback, handler);
            PlaybackState state = new PlaybackState.Builder()
                    .setState(PlaybackState.STATE_PLAYING, 0, 1.0f).build();
            // Fake the media session service so this session can take the media key events.
            session.setPlaybackState(state);
            session.setActive(true);

            // A media playback is also needed to receive media key events.
            Utils.assertMediaPlaybackStarted(getInstrumentation().getTargetContext());

            // Ensure that the listener is called for media key event,
            // and any other media sessions don't get the key.
            MediaKeyListener listener = new MediaKeyListener(2, true, handler);
            mSessionManager.setOnMediaKeyListener(listener, handler);
            injectInputEvent(KeyEvent.KEYCODE_HEADSETHOOK, false);
            assertTrue(listener.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            assertEquals(listener.mKeyEvents.size(), 2);
            assertKeyEventEquals(listener.mKeyEvents.get(0),
                    KeyEvent.KEYCODE_HEADSETHOOK, KeyEvent.ACTION_DOWN, 0);
            assertKeyEventEquals(listener.mKeyEvents.get(1),
                    KeyEvent.KEYCODE_HEADSETHOOK, KeyEvent.ACTION_UP, 0);
            assertFalse(callback.mCountDownLatch.await(WAIT_MS, TimeUnit.MILLISECONDS));
            assertEquals(callback.mKeyEvents.size(), 0);

            // Ensure that the listener is called for media key event,
            // and another media session gets the key.
            listener = new MediaKeyListener(2, false, handler);
            mSessionManager.setOnMediaKeyListener(listener, handler);
            injectInputEvent(KeyEvent.KEYCODE_HEADSETHOOK, false);
            assertTrue(listener.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            assertEquals(listener.mKeyEvents.size(), 2);
            assertKeyEventEquals(listener.mKeyEvents.get(0),
                    KeyEvent.KEYCODE_HEADSETHOOK, KeyEvent.ACTION_DOWN, 0);
            assertKeyEventEquals(listener.mKeyEvents.get(1),
                    KeyEvent.KEYCODE_HEADSETHOOK, KeyEvent.ACTION_UP, 0);
            assertTrue(callback.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            assertEquals(callback.mKeyEvents.size(), 2);
            assertKeyEventEquals(callback.mKeyEvents.get(0),
                    KeyEvent.KEYCODE_HEADSETHOOK, KeyEvent.ACTION_DOWN, 0);
            assertKeyEventEquals(callback.mKeyEvents.get(1),
                    KeyEvent.KEYCODE_HEADSETHOOK, KeyEvent.ACTION_UP, 0);

            // Ensure that the listener isn't called anymore.
            listener = new MediaKeyListener(1, true, handler);
            mSessionManager.setOnMediaKeyListener(listener, handler);
            mSessionManager.setOnMediaKeyListener(null, handler);
            injectInputEvent(KeyEvent.KEYCODE_HEADSETHOOK, false);
            assertFalse(listener.mCountDownLatch.await(WAIT_MS, TimeUnit.MILLISECONDS));
            assertEquals(listener.mKeyEvents.size(), 0);
        } finally {
            if (session != null) {
                session.release();
            }
            removeHandler(handler);
        }
    }

    public void testRemoteUserInfo() throws Exception {
        final Context context = getInstrumentation().getTargetContext();
        Handler handler = createHandler();

        MediaSession session = null;
        try {
            session = new MediaSession(context , TAG);
            MediaSessionCallback callback = new MediaSessionCallback(5, session);
            session.setFlags(MediaSession.FLAG_HANDLES_MEDIA_BUTTONS
                    | MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);
            session.setCallback(callback, handler);
            PlaybackState state = new PlaybackState.Builder()
                    .setState(PlaybackState.STATE_PLAYING, 0, 1.0f).build();
            // Fake the media session service so this session can take the media key events.
            session.setPlaybackState(state);
            session.setActive(true);

            // A media playback is also needed to receive media key events.
            Utils.assertMediaPlaybackStarted(context);

            // Dispatch key events 5 times.
            KeyEvent event = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_MEDIA_PLAY);
            // (1), (2): dispatch through key -- this will trigger event twice for up & down.
            injectInputEvent(KeyEvent.KEYCODE_HEADSETHOOK, false);
            // (3): dispatch through controller
            session.getController().dispatchMediaButtonEvent(event);

            // Creating another controller.
            MediaController controller = new MediaController(context, session.getSessionToken());
            // (4): dispatch through different controller.
            controller.dispatchMediaButtonEvent(event);
            // (5): dispatch through the same controller
            controller.dispatchMediaButtonEvent(event);

            // Wait.
            assertTrue(callback.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));

            // Caller of (1) ~ (4) shouldn't be the same as any others.
            for (int i = 0; i < 4; i ++) {
                for (int j = 0; j < i; j++) {
                    assertNotSame(callback.mCallers.get(i), callback.mCallers.get(j));
                }
            }
            // Caller of (5) should be the same as (4), since they're called from the same
            assertEquals(callback.mCallers.get(3), callback.mCallers.get(4));
        } finally {
            if (session != null) {
                session.release();
            }
            removeHandler(handler);
        }
    }

    public void testGetSession2Tokens() throws Exception {
        final Context context = getInstrumentation().getTargetContext();
        Handler handler = createHandler();
        Executor handlerExecutor = new HandlerExecutor(handler);

        Session2TokenListener listener = new Session2TokenListener();
        mSessionManager.addOnSession2TokensChangedListener(listener, handler);

        Session2Callback sessionCallback = new Session2Callback();
        try (MediaSession2 session = new MediaSession2.Builder(context)
                .setSessionCallback(handlerExecutor, sessionCallback)
                .build()) {
            assertTrue(sessionCallback.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            assertTrue(listener.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));

            Session2Token currentToken = session.getToken();
            assertTrue(listContainsToken(listener.mTokens, currentToken));
            assertTrue(listContainsToken(mSessionManager.getSession2Tokens(), currentToken));
        }
    }

    public void testGetSession2TokensWithTwoSessions() throws Exception {
        final Context context = getInstrumentation().getTargetContext();
        Handler handler = createHandler();
        Executor handlerExecutor = new HandlerExecutor(handler);

        Session2TokenListener listener = new Session2TokenListener();
        mSessionManager.addOnSession2TokensChangedListener(listener, handler);

        try (MediaSession2 session1 = new MediaSession2.Builder(context)
                .setSessionCallback(handlerExecutor, new Session2Callback())
                .setId("testGetSession2TokensWithTwoSessions_session1")
                .build()) {

            assertTrue(listener.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            Session2Token session1Token = session1.getToken();
            assertTrue(listContainsToken(mSessionManager.getSession2Tokens(), session1Token));

            // Create another session and check the result of getSession2Token().
            listener.resetCountDownLatch();
            Session2Token session2Token = null;
            try (MediaSession2 session2 = new MediaSession2.Builder(context)
                    .setSessionCallback(handlerExecutor, new Session2Callback())
                    .setId("testGetSession2TokensWithTwoSessions_session2")
                    .build()) {

                assertTrue(listener.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
                session2Token = session2.getToken();
                assertNotNull(session2Token);
                assertTrue(listContainsToken(mSessionManager.getSession2Tokens(), session1Token));
                assertTrue(listContainsToken(mSessionManager.getSession2Tokens(), session2Token));

                listener.resetCountDownLatch();
            }

            // Since the session2 is closed, getSession2Tokens() shouldn't include session2's token.
            assertTrue(listener.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            assertTrue(listContainsToken(mSessionManager.getSession2Tokens(), session1Token));
            assertFalse(listContainsToken(mSessionManager.getSession2Tokens(), session2Token));
        }
    }

    public void testAddAndRemoveSession2TokensListener() throws Exception {
        final Context context = getInstrumentation().getTargetContext();
        Handler handler = createHandler();
        Executor handlerExecutor = new HandlerExecutor(handler);

        Session2TokenListener listener1 = new Session2TokenListener();
        mSessionManager.addOnSession2TokensChangedListener(listener1, handler);

        Session2Callback sessionCallback = new Session2Callback();
        try (MediaSession2 session = new MediaSession2.Builder(context)
                .setSessionCallback(handlerExecutor, sessionCallback)
                .build()) {
            assertTrue(listener1.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            Session2Token currentToken = session.getToken();
            assertTrue(listContainsToken(listener1.mTokens, currentToken));

            // Test removing listener
            listener1.resetCountDownLatch();
            Session2TokenListener listener2 = new Session2TokenListener();
            mSessionManager.addOnSession2TokensChangedListener(listener2, handler);
            mSessionManager.removeOnSession2TokensChangedListener(listener1);

            session.close();
            assertFalse(listener1.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            assertTrue(listener2.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
        }
    }

    public void testSession2TokensNotChangedBySession1() throws Exception {
        final Context context = getInstrumentation().getTargetContext();
        Handler handler = createHandler();

        Session2TokenListener listener = new Session2TokenListener();
        List<Session2Token> initialSession2Tokens = mSessionManager.getSession2Tokens();
        mSessionManager.addOnSession2TokensChangedListener(listener, handler);
        MediaSession session = null;
        try {
            session = new MediaSession(context, TAG);
            session.setActive(true);
            session.setFlags(MediaSession.FLAG_HANDLES_MEDIA_BUTTONS
                    | MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);
            assertFalse(listener.mCountDownLatch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
            List<Session2Token> laterSession2Tokens = mSessionManager.getSession2Tokens();

            assertEquals(initialSession2Tokens.size(), laterSession2Tokens.size());
        } finally {
            if (session != null) {
                session.release();
            }
        }
    }

    public void testCustomClassConfigValuesAreValid() throws Exception {
        if (!MediaUtils.check(sIsAtLeastS, "test invalid before Android 12")) return;
        final Context context = getInstrumentation().getTargetContext();
        String customMediaKeyDispatcher = context.getString(
                android.R.string.config_customMediaKeyDispatcher);
        String customMediaSessionPolicyProvider = context.getString(
                android.R.string.config_customMediaSessionPolicyProvider);
        // MediaSessionService will call Class.forName(String) with the existing config value.
        // If the config value is not valid (i.e. given class doesn't exist), the following
        // methods will return false.
        if (!customMediaKeyDispatcher.isEmpty()) {
            assertTrue(mSessionManager.hasCustomMediaKeyDispatcher(customMediaKeyDispatcher));
        }
        if (!customMediaSessionPolicyProvider.isEmpty()) {
            assertTrue(mSessionManager.hasCustomMediaSessionPolicyProvider(
                    customMediaSessionPolicyProvider));
        }
    }

    private boolean listContainsToken(List<Session2Token> tokens, Session2Token token) {
        for (int i = 0; i < tokens.size(); i++) {
            if (tokens.get(i).equals(token)) {
                return true;
            }
        }
        return false;
    }

    private Handler createHandler() {
        HandlerThread handlerThread = new HandlerThread("MediaSessionManagerTest");
        handlerThread.start();
        return new Handler(handlerThread.getLooper());
    }

    private void removeHandler(Handler handler) {
        if (handler == null) {
            return;
        }
        handler.getLooper().quitSafely();
    }

    // This uses public APIs to dispatch key events, so sessions would consider this as
    // 'media key event from this application'.
    private void simulateMediaKeyInput(int keyCode) {
        long downTime = System.currentTimeMillis();
        mAudioManager.dispatchMediaKeyEvent(
                new KeyEvent(downTime, downTime, KeyEvent.ACTION_DOWN, keyCode, 0));
        mAudioManager.dispatchMediaKeyEvent(
                new KeyEvent(downTime, System.currentTimeMillis(), KeyEvent.ACTION_UP, keyCode, 0));
    }

    private class VolumeKeyLongPressListener
            implements MediaSessionManager.OnVolumeKeyLongPressListener {
        private final List<KeyEvent> mKeyEvents = new ArrayList<>();
        private final CountDownLatch mCountDownLatch;
        private final Handler mHandler;

        public VolumeKeyLongPressListener(int count, Handler handler) {
            mCountDownLatch = new CountDownLatch(count);
            mHandler = handler;
        }

        @Override
        public void onVolumeKeyLongPress(KeyEvent event) {
            mKeyEvents.add(event);
            // Ensure the listener is called on the thread.
            assertEquals(mHandler.getLooper(), Looper.myLooper());
            mCountDownLatch.countDown();
        }
    }

    private class MediaKeyListener implements MediaSessionManager.OnMediaKeyListener {
        private final CountDownLatch mCountDownLatch;
        private final boolean mConsume;
        private final Handler mHandler;
        private final List<KeyEvent> mKeyEvents = new ArrayList<>();

        public MediaKeyListener(int count, boolean consume, Handler handler) {
            mCountDownLatch = new CountDownLatch(count);
            mConsume = consume;
            mHandler = handler;
        }

        @Override
        public boolean onMediaKey(KeyEvent event) {
            mKeyEvents.add(event);
            // Ensure the listener is called on the thread.
            assertEquals(mHandler.getLooper(), Looper.myLooper());
            mCountDownLatch.countDown();
            return mConsume;
        }
    }

    private class MediaSessionCallback extends MediaSession.Callback {
        private final CountDownLatch mCountDownLatch;
        private final MediaSession mSession;
        private final List<KeyEvent> mKeyEvents = new ArrayList<>();
        private final List<MediaSessionManager.RemoteUserInfo> mCallers = new ArrayList<>();

        private MediaSessionCallback(int count, MediaSession session) {
            mCountDownLatch = new CountDownLatch(count);
            mSession = session;
        }

        public boolean onMediaButtonEvent(Intent mediaButtonIntent) {
            KeyEvent event = (KeyEvent) mediaButtonIntent.getParcelableExtra(
                    Intent.EXTRA_KEY_EVENT);
            assertNotNull(event);
            mKeyEvents.add(event);
            mCallers.add(mSession.getCurrentControllerInfo());
            mCountDownLatch.countDown();
            return true;
        }
    }

    private class Session2Callback extends MediaSession2.SessionCallback {
        private CountDownLatch mCountDownLatch;

        private Session2Callback() {
            mCountDownLatch = new CountDownLatch(1);
        }

        @Override
        public Session2CommandGroup onConnect(MediaSession2 session,
                MediaSession2.ControllerInfo controller) {
            if (controller.getUid() == Process.SYSTEM_UID) {
                // System server will try to connect here for monitor session.
                mCountDownLatch.countDown();
            }
            return new Session2CommandGroup.Builder().build();
        }
    }

    private class Session2TokenListener implements
            MediaSessionManager.OnSession2TokensChangedListener {
        private CountDownLatch mCountDownLatch;
        private List<Session2Token> mTokens;

        private Session2TokenListener() {
            mCountDownLatch = new CountDownLatch(1);
        }

        @Override
        public void onSession2TokensChanged(List<Session2Token> tokens) {
            mTokens = tokens;
            mCountDownLatch.countDown();
        }

        public void resetCountDownLatch() {
            mCountDownLatch = new CountDownLatch(1);
        }
    }

    private class MediaKeyEventSessionListener
            implements MediaSessionManager.OnMediaKeyEventSessionChangedListener {
        CountDownLatch mCountDownLatch;
        MediaSession.Token mSessionToken;

        MediaKeyEventSessionListener() {
            mCountDownLatch = new CountDownLatch(1);
        }

        void resetCountDownLatch() {
            mCountDownLatch = new CountDownLatch(1);
        }

        @Override
        public void onMediaKeyEventSessionChanged(String packageName,
                MediaSession.Token sessionToken) {
            mSessionToken = sessionToken;
            mCountDownLatch.countDown();
        }
    }

    private class MediaKeyEventDispatchedListener
            implements MediaSessionManager.OnMediaKeyEventDispatchedListener {
        CountDownLatch mCountDownLatch;
        KeyEvent mKeyEvent;
        String mPackageName;
        MediaSession.Token mSessionToken;

        MediaKeyEventDispatchedListener() {
            mCountDownLatch = new CountDownLatch(1);
        }

        void resetCountDownLatch() {
            mCountDownLatch = new CountDownLatch(1);
        }

        @Override
        public void onMediaKeyEventDispatched(KeyEvent event, String packageName,
                MediaSession.Token sessionToken) {
            mKeyEvent = event;
            mPackageName = packageName;
            mSessionToken = sessionToken;

            mCountDownLatch.countDown();
        }
    }

    private static class HandlerExecutor implements Executor {
        private final Handler mHandler;

        HandlerExecutor(Handler handler) {
            mHandler = handler;
        }

        @Override
        public void execute(Runnable command) {
            mHandler.post(command);
        }
    }
}

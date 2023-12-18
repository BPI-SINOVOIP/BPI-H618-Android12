/*
 **
 ** Copyright 2018, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

package android.media.cts;

import android.annotation.Nullable;
import android.annotation.RawRes;
import android.content.res.AssetFileDescriptor;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.SystemClock;
import android.util.Log;

import com.android.compatibility.common.util.CtsAndroidTestCase;

import javax.annotation.concurrent.GuardedBy;
import java.io.InputStream;
import java.util.concurrent.Executor;

@NonMediaMainlineTest
public class AudioTrackOffloadTest extends CtsAndroidTestCase {
    private static final String TAG = "AudioTrackOffloadTest";


    private static final int BUFFER_SIZE_SEC = 3;
    private static final long DATA_REQUEST_TIMEOUT_MS = 6 * 1000; // 6s
    private static final long DATA_REQUEST_POLL_PERIOD_MS = 1 * 1000; // 1s
    private static final long PRESENTATION_END_TIMEOUT_MS = 8 * 1000; // 8s
    private static final int AUDIOTRACK_DEFAULT_SAMPLE_RATE = 44100;
    private static final int AUDIOTRACK_DEFAULT_CHANNEL_MASK = AudioFormat.CHANNEL_OUT_STEREO;

    private static final AudioAttributes DEFAULT_ATTR = new AudioAttributes.Builder().build();

    public void testIsOffloadSupportedNullFormat() throws Exception {
        try {
            final boolean offloadableFormat = AudioManager.isOffloadedPlaybackSupported(null,
                    DEFAULT_ATTR);
            fail("Shouldn't be able to use null AudioFormat in isOffloadedPlaybackSupported()");
        } catch (NullPointerException e) {
            // ok, NPE is expected here
        }
    }

    public void testIsOffloadSupportedNullAttributes() throws Exception {
        try {
            final boolean offloadableFormat = AudioManager.isOffloadedPlaybackSupported(
                    getAudioFormatWithEncoding(AudioFormat.ENCODING_MP3), null);
            fail("Shouldn't be able to use null AudioAttributes in isOffloadedPlaybackSupported()");
        } catch (NullPointerException e) {
            // ok, NPE is expected here
        }
    }

    public void testExerciseIsOffloadSupported() throws Exception {
        final boolean offloadableFormat = AudioManager.isOffloadedPlaybackSupported(
                getAudioFormatWithEncoding(AudioFormat.ENCODING_MP3), DEFAULT_ATTR);
    }

    public void testGetPlaybackOffloadSupportNullFormat() throws Exception {
        try {
            final int offloadMode = AudioManager.getPlaybackOffloadSupport(null,
                    DEFAULT_ATTR);
            fail("Shouldn't be able to use null AudioFormat in getPlaybackOffloadSupport()");
        } catch (NullPointerException e) {
            // ok, NPE is expected here
        }
    }

    public void testGetPlaybackOffloadSupportNullAttributes() throws Exception {
        try {
            final int offloadMode = AudioManager.getPlaybackOffloadSupport(
                    getAudioFormatWithEncoding(AudioFormat.ENCODING_MP3), null);
            fail("Shouldn't be able to use null AudioAttributes in getPlaybackOffloadSupport()");
        } catch (NullPointerException e) {
            // ok, NPE is expected here
        }
    }

    public void testExerciseGetPlaybackOffloadSupport() throws Exception {
        final int offloadMode = AudioManager.getPlaybackOffloadSupport(
                getAudioFormatWithEncoding(AudioFormat.ENCODING_MP3), DEFAULT_ATTR);
        assertTrue("getPlaybackOffloadSupport returned invalid mode: " + offloadMode,
            offloadMode == AudioManager.PLAYBACK_OFFLOAD_NOT_SUPPORTED
                || offloadMode == AudioManager.PLAYBACK_OFFLOAD_SUPPORTED
                || offloadMode == AudioManager.PLAYBACK_OFFLOAD_GAPLESS_SUPPORTED);
    }

    public void testMP3AudioTrackOffload() throws Exception {
        testAudioTrackOffload(R.raw.sine1khzs40dblong,
                /* bitRateInkbps= */ 192,
                getAudioFormatWithEncoding(AudioFormat.ENCODING_MP3));
    }

    public void testOpusAudioTrackOffload() throws Exception {
        testAudioTrackOffload(R.raw.testopus,
                /* bitRateInkbps= */ 118, // Average
                getAudioFormatWithEncoding(AudioFormat.ENCODING_OPUS));
    }

    private @Nullable AudioTrack getOffloadAudioTrack(@RawRes int audioRes, int bitRateInkbps,
                                            AudioFormat audioFormat) {
        if (!AudioManager.isOffloadedPlaybackSupported(audioFormat, DEFAULT_ATTR)) {
            Log.i(TAG, "skipping testAudioTrackOffload as offload encoding "
                    + audioFormat.getEncoding() + " is not supported");
            // cannot test if offloading is not supported
            return null;
        }

        int bufferSizeInBytes = bitRateInkbps * 1000 * BUFFER_SIZE_SEC / 8;
        // format is offloadable, test playback head is progressing
        AudioTrack track = new AudioTrack.Builder()
                .setAudioAttributes(DEFAULT_ATTR)
                .setAudioFormat(audioFormat)
                .setTransferMode(AudioTrack.MODE_STREAM)
                .setBufferSizeInBytes(bufferSizeInBytes)
                .setOffloadedPlayback(true)
                .build();
        assertNotNull("Couldn't create offloaded AudioTrack", track);
        assertEquals("Unexpected track sample rate", AUDIOTRACK_DEFAULT_SAMPLE_RATE,
                track.getSampleRate());
        assertEquals("Unexpected track channel mask", AUDIOTRACK_DEFAULT_CHANNEL_MASK,
                track.getChannelConfiguration());
        return track;
    }

    /**
     * Test offload of an audio resource that MUST be at least 3sec long.
     */
    private void testAudioTrackOffload(@RawRes int audioRes, int bitRateInkbps,
                                       AudioFormat audioFormat) throws Exception {
        AudioTrack track = null;
        try (AssetFileDescriptor audioToOffload = getContext().getResources()
                .openRawResourceFd(audioRes);
             InputStream audioInputStream = audioToOffload.createInputStream()) {

            track = getOffloadAudioTrack(audioRes, bitRateInkbps, audioFormat);
            if (track == null) {
                return;
            }

            try {
                track.registerStreamEventCallback(mExec, null);
                fail("Shouldn't be able to register null StreamEventCallback");
            } catch (Exception e) {
            }
            track.registerStreamEventCallback(mExec, mCallback);

            int bufferSizeInBytes3sec = bitRateInkbps * 1000 * BUFFER_SIZE_SEC / 8;
            final byte[] data = new byte[bufferSizeInBytes3sec];
            final int read = audioInputStream.read(data);
            assertEquals("Could not read enough audio from the resource file",
                    bufferSizeInBytes3sec, read);

            track.play();
            int written = 0;
            while (written < read) {
                int wrote = track.write(data, written, read - written,
                        AudioTrack.WRITE_BLOCKING);
                Log.i(TAG, String.format("wrote %d bytes (%d out of %d)", wrote, written, read));
                if (wrote < 0) {
                    fail("Unable to write all read data, wrote " + written + " bytes");
                }
                written += wrote;
            }

            try {
                final long elapsed = checkDataRequest(DATA_REQUEST_TIMEOUT_MS);
                synchronized (mPresEndLock) {
                    track.setOffloadEndOfStream();

                    track.stop();
                    mPresEndLock.safeWait(PRESENTATION_END_TIMEOUT_MS - elapsed);
                }
            } catch (InterruptedException e) {
                fail("Error while sleeping");
            }
            synchronized (mPresEndLock) {
                // we are at most PRESENTATION_END_TIMEOUT_MS + 1s after about 3s of data was
                // supplied, presentation should have ended
                assertEquals("onPresentationEnded not called one time",
                        1, mCallback.mPresentationEndedCount);
            }

        } finally {
            if (track != null) {
                Log.i(TAG, "pause");
                track.pause();
                track.unregisterStreamEventCallback(mCallback);
                track.release();
            }
        };
    }

    private long checkDataRequest(long timeout) throws Exception {
        long checkStart = SystemClock.uptimeMillis();
        boolean calledback = false;
        while (SystemClock.uptimeMillis() - checkStart < timeout) {
            synchronized (mEventCallbackLock) {
                if (mCallback.mDataRequestCount > 0) {
                    calledback = true;
                    break;
                }
            }
            Thread.sleep(DATA_REQUEST_POLL_PERIOD_MS);
        }
        assertTrue("onDataRequest not called", calledback);
        return (SystemClock.uptimeMillis() - checkStart);
    }

    private AudioTrack allocNonOffloadAudioTrack() {
        // Attrributes the AudioTrack are irrelevant in this case. We just need to provide
        // an AudioTrack that IS NOT offloaded so that we can demonstrate failure.
        AudioTrack track = new AudioTrack.Builder()
                .setBufferSizeInBytes(2048/*arbitrary*/)
                .build();

        assert(track != null);
        return track;
    }

     // Arbitrary values..
    private static final int TEST_DELAY = 50;
    private static final int TEST_PADDING = 100;
    public void testOffloadPadding() {
        AudioTrack track =
                getOffloadAudioTrack(R.raw.sine1khzs40dblong,
                /* bitRateInkbps= */ 192,
                getAudioFormatWithEncoding(AudioFormat.ENCODING_MP3));
        if (track == null) {
            return;
        }

        assertTrue(track.getOffloadPadding() >= 0);

        track.setOffloadDelayPadding(0 /*delayInFrames*/, 0 /*paddingInFrames*/);

        int offloadDelay;
        offloadDelay = track.getOffloadDelay();
        assertEquals(0, offloadDelay);

        int padding = track.getOffloadPadding();
        assertEquals(0, padding);

        track.setOffloadDelayPadding(
                TEST_DELAY /*delayInFrames*/,
                TEST_PADDING /*paddingInFrames*/);
        offloadDelay = track.getOffloadDelay();
        assertEquals(TEST_DELAY, offloadDelay);
        padding = track.getOffloadPadding();
        assertEquals(TEST_PADDING, padding);
    }

    public void testIsOffloadedPlayback() {
        // non-offloaded case
        AudioTrack nonOffloadTrack = allocNonOffloadAudioTrack();
        assertFalse(nonOffloadTrack.isOffloadedPlayback());

        // offloaded case
        AudioTrack offloadTrack =
                getOffloadAudioTrack(R.raw.sine1khzs40dblong,
                        /* bitRateInkbps= */ 192,
                        getAudioFormatWithEncoding(AudioFormat.ENCODING_MP3));
        if (offloadTrack == null) {
            return;
        }
        assertTrue(offloadTrack.isOffloadedPlayback());
    }

    public void testSetOffloadEndOfStreamWithNonOffloadedTrack() {
        // Non-offload case
        AudioTrack nonOffloadTrack = allocNonOffloadAudioTrack();
        assertFalse(nonOffloadTrack.isOffloadedPlayback());
        org.testng.Assert.assertThrows(IllegalStateException.class,
                () -> nonOffloadTrack.setOffloadEndOfStream());
    }

    private static AudioFormat getAudioFormatWithEncoding(int encoding) {
       return new AudioFormat.Builder()
            .setEncoding(encoding)
            .setSampleRate(AUDIOTRACK_DEFAULT_SAMPLE_RATE)
            .setChannelMask(AUDIOTRACK_DEFAULT_CHANNEL_MASK)
            .build();
    }

    private Executor mExec = new Executor() {
        @Override
        public void execute(Runnable command) {
            command.run();
        }
    };

    private final Object mEventCallbackLock = new Object();
    private final SafeWaitObject mPresEndLock = new SafeWaitObject();

    private EventCallback mCallback = new EventCallback();

    private class EventCallback extends AudioTrack.StreamEventCallback {
        @GuardedBy("mEventCallbackLock")
        int mTearDownCount;
        @GuardedBy("mPresEndLock")
        int mPresentationEndedCount;
        @GuardedBy("mEventCallbackLock")
        int mDataRequestCount;

        @Override
        public void onTearDown(AudioTrack track) {
            synchronized (mEventCallbackLock) {
                Log.i(TAG, "onTearDown");
                mTearDownCount++;
            }
        }

        @Override
        public void onPresentationEnded(AudioTrack track) {
            synchronized (mPresEndLock) {
                Log.i(TAG, "onPresentationEnded");
                mPresentationEndedCount++;
                mPresEndLock.safeNotify();
            }
        }

        @Override
        public void onDataRequest(AudioTrack track, int size) {
            synchronized (mEventCallbackLock) {
                Log.i(TAG, "onDataRequest size:"+size);
                mDataRequestCount++;
            }
        }
    }
}

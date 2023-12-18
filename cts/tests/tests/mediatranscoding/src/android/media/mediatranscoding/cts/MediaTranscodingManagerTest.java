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

package android.media.mediatranscoding.cts;

import static org.testng.Assert.assertThrows;


import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.media.ApplicationMediaCapabilities;
import android.media.MediaFormat;
import android.media.MediaTranscodingManager;
import android.media.MediaTranscodingManager.TranscodingRequest;
import android.media.MediaTranscodingManager.TranscodingSession;
import android.media.MediaTranscodingManager.VideoTranscodingRequest;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.FileUtils;
import android.os.ParcelFileDescriptor;
import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.Presubmit;
import android.platform.test.annotations.RequiresDevice;
import android.provider.MediaStore;
import android.test.AndroidTestCase;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;

import com.android.compatibility.common.util.MediaUtils;

import org.junit.Test;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

@Presubmit
@RequiresDevice
@AppModeFull(reason = "Instant apps cannot access the SD card")
public class MediaTranscodingManagerTest extends AndroidTestCase {
    private static final String TAG = "MediaTranscodingManagerTest";
    /** The time to wait for the transcode operation to complete before failing the test. */
    private static final int TRANSCODE_TIMEOUT_SECONDS = 10;
    /** Copy the transcoded video to /storage/emulated/0/Download/ */
    private static final boolean DEBUG_TRANSCODED_VIDEO = false;
    /** Dump both source yuv and transcode YUV to /storage/emulated/0/Download/ */
    private static final boolean DEBUG_YUV = false;

    private Context mContext;
    private ContentResolver mContentResolver;
    private MediaTranscodingManager mMediaTranscodingManager = null;
    private Uri mSourceHEVCVideoUri = null;
    private Uri mSourceAVCVideoUri = null;
    private Uri mDestinationUri = null;

    // Default setting for transcoding to H.264.
    private static final String MIME_TYPE = MediaFormat.MIMETYPE_VIDEO_AVC;
    private static final int BIT_RATE = 20000000;            // 20Mbps
    private static final int WIDTH = 1920;
    private static final int HEIGHT = 1080;

    // Threshold for the psnr to make sure the transcoded video is valid.
    private static final int PSNR_THRESHOLD = 20;

    // Copy the resource to cache.
    private Uri resourceToUri(Context context, int resId, String name) throws IOException {
        Uri resUri = new Uri.Builder()
                .scheme(ContentResolver.SCHEME_ANDROID_RESOURCE)
                .authority(context.getResources().getResourcePackageName(resId))
                .appendPath(context.getResources().getResourceTypeName(resId))
                .appendPath(context.getResources().getResourceEntryName(resId))
                .build();

        Uri cacheUri = Uri.parse(ContentResolver.SCHEME_FILE + "://"
                + mContext.getCacheDir().getAbsolutePath() + "/" + name);

        InputStream is = mContext.getResources().openRawResource(resId);
        OutputStream os = mContext.getContentResolver().openOutputStream(cacheUri);

        FileUtils.copy(is, os);
        return cacheUri;
    }

    private static Uri generateNewUri(Context context, String filename) {
        File outFile = new File(context.getExternalCacheDir(), filename);
        return Uri.fromFile(outFile);
    }

    // Generates an invalid uri which will let the service return transcoding failure.
    private static Uri generateInvalidTranscodingUri(Context context) {
        File outFile = new File(context.getExternalCacheDir(), "InvalidUri.mp4");
        return Uri.fromFile(outFile);
    }

    /**
     * Creates a MediaFormat with the default settings.
     */
    private static MediaFormat createMediaFormat() {
        MediaFormat format = MediaFormat.createVideoFormat(MIME_TYPE, WIDTH, HEIGHT);
        format.setInteger(MediaFormat.KEY_BIT_RATE, BIT_RATE);
        return format;
    }

    @Override
    public void setUp() throws Exception {
        Log.d(TAG, "setUp");
        super.setUp();
        mContext = InstrumentationRegistry.getInstrumentation().getContext();
        mContentResolver = mContext.getContentResolver();

        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .adoptShellPermissionIdentity("android.permission.WRITE_MEDIA_STORAGE");
        mMediaTranscodingManager = mContext.getSystemService(MediaTranscodingManager.class);
        assertNotNull(mMediaTranscodingManager);
        androidx.test.InstrumentationRegistry.registerInstance(
                InstrumentationRegistry.getInstrumentation(), new Bundle());

        // Setup source HEVC file uri.
        mSourceHEVCVideoUri = resourceToUri(mContext, R.raw.Video_HEVC_30Frames,
                "Video_HEVC_30Frames.mp4");

        // Setup source AVC file uri.
        mSourceAVCVideoUri = resourceToUri(mContext, R.raw.Video_AVC_30Frames,
                "Video_AVC_30Frames.mp4");

        // Setup destination file.
        mDestinationUri = generateNewUri(mContext, "transcoded.mp4");
    }

    @Override
    public void tearDown() throws Exception {
        InstrumentationRegistry
                .getInstrumentation().getUiAutomation().dropShellPermissionIdentity();
        super.tearDown();
    }

    // Skip the test for TV, Car and Watch devices.
    private boolean shouldSkip() {
        PackageManager pm =
                InstrumentationRegistry.getInstrumentation().getTargetContext().getPackageManager();
        return pm.hasSystemFeature(pm.FEATURE_LEANBACK) || pm.hasSystemFeature(pm.FEATURE_WATCH)
                || pm.hasSystemFeature(pm.FEATURE_AUTOMOTIVE);
    }

    /**
     * Verify that setting null destination uri will throw exception.
     */
    public void testCreateTranscodingRequestWithNullDestinationUri() throws Exception {
        if (shouldSkip()) {
            return;
        }
        assertThrows(IllegalArgumentException.class, () -> {
            VideoTranscodingRequest request =
                    new VideoTranscodingRequest.Builder(mSourceHEVCVideoUri, null,
                            createMediaFormat())
                            .build();
        });
    }

    /**
     * Verify that setting invalid pid will throw exception.
     */
    public void testCreateTranscodingWithInvalidClientPid() throws Exception {
        if (shouldSkip()) {
            return;
        }
        assertThrows(IllegalArgumentException.class, () -> {
            VideoTranscodingRequest request =
                    new VideoTranscodingRequest.Builder(mSourceHEVCVideoUri, mDestinationUri,
                            createMediaFormat())
                            .setClientPid(-1)
                            .build();
        });
    }

    /**
     * Verify that setting invalid uid will throw exception.
     */
    public void testCreateTranscodingWithInvalidClientUid() throws Exception {
        if (shouldSkip()) {
            return;
        }
        assertThrows(IllegalArgumentException.class, () -> {
            VideoTranscodingRequest request =
                    new VideoTranscodingRequest.Builder(mSourceHEVCVideoUri, mDestinationUri,
                            createMediaFormat())
                            .setClientUid(-1)
                            .build();
        });
    }

    /**
     * Verify that setting null source uri will throw exception.
     */
    public void testCreateTranscodingRequestWithNullSourceUri() throws Exception {
        if (shouldSkip()) {
            return;
        }
        assertThrows(IllegalArgumentException.class, () -> {
            VideoTranscodingRequest request =
                    new VideoTranscodingRequest.Builder(null, mDestinationUri, createMediaFormat())
                            .build();
        });
    }

    /**
     * Verify that not setting source uri will throw exception.
     */
    public void testCreateTranscodingRequestWithoutSourceUri() throws Exception {
        if (shouldSkip()) {
            return;
        }
        assertThrows(IllegalArgumentException.class, () -> {
            VideoTranscodingRequest request =
                    new VideoTranscodingRequest.Builder(null, mDestinationUri, createMediaFormat())
                            .build();
        });
    }

    /**
     * Verify that not setting destination uri will throw exception.
     */
    public void testCreateTranscodingRequestWithoutDestinationUri() throws Exception {
        if (shouldSkip()) {
            return;
        }
        assertThrows(IllegalArgumentException.class, () -> {
            VideoTranscodingRequest request =
                    new VideoTranscodingRequest.Builder(mSourceHEVCVideoUri, null,
                            createMediaFormat())
                            .build();
        });
    }


    /**
     * Verify that setting video transcoding without setting video format will throw exception.
     */
    public void testCreateTranscodingRequestWithoutVideoFormat() throws Exception {
        if (shouldSkip()) {
            return;
        }
        assertThrows(IllegalArgumentException.class, () -> {
            VideoTranscodingRequest request =
                    new VideoTranscodingRequest.Builder(mSourceHEVCVideoUri, mDestinationUri, null)
                            .build();
        });
    }

    private void testTranscodingWithExpectResult(Uri srcUri, Uri dstUri, int expectedResult)
            throws Exception {
        if (shouldSkip()) {
            return;
        }
        Semaphore transcodeCompleteSemaphore = new Semaphore(0);

        VideoTranscodingRequest request =
                new VideoTranscodingRequest.Builder(srcUri, dstUri, createMediaFormat())
                        .build();
        Executor listenerExecutor = Executors.newSingleThreadExecutor();

        TranscodingSession session = mMediaTranscodingManager.enqueueRequest(
                request,
                listenerExecutor,
                transcodingSession -> {
                    Log.d(TAG,
                            "Transcoding completed with result: " + transcodingSession.getResult());
                    transcodeCompleteSemaphore.release();
                    assertEquals(expectedResult, transcodingSession.getResult());
                });
        assertNotNull(session);

        if (session != null) {
            Log.d(TAG, "testMediaTranscodingManager - Waiting for transcode to complete.");
            boolean finishedOnTime = transcodeCompleteSemaphore.tryAcquire(
                    TRANSCODE_TIMEOUT_SECONDS, TimeUnit.SECONDS);
            assertTrue("Transcode failed to complete in time.", finishedOnTime);
        }

        File dstFile = new File(dstUri.getPath());;
        if (expectedResult == TranscodingSession.RESULT_SUCCESS) {
            // Checks the destination file get generated.
            assertTrue("Failed to create destination file", dstFile.exists());
        }

        if (dstFile.exists()) {
            dstFile.delete();
        }
    }

    // Tests transcoding from invalid file uri and expects failure.
    public void testTranscodingInvalidSrcUri() throws Exception {
        if (shouldSkip()) {
            return;
        }
        Uri invalidSrcUri = Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://"
                + mContext.getPackageName() + "/source.mp4");
        // Create a file Uri: android.resource://android.media.cts/temp.mp4
        Uri destinationUri = Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://"
                + mContext.getPackageName() + "/temp.mp4");
        Log.d(TAG, "Transcoding " + invalidSrcUri + "to destination: " + destinationUri);

        testTranscodingWithExpectResult(invalidSrcUri, destinationUri,
                TranscodingSession.RESULT_ERROR);
    }

    // Tests transcoding to a uri in res folder and expects failure as test could not write to res
    // folder.
    public void testTranscodingToResFolder() throws Exception {
        if (shouldSkip()) {
            return;
        }
        // Create a file Uri:  android.resource://android.media.cts/temp.mp4
        Uri destinationUri = Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://"
                + mContext.getPackageName() + "/temp.mp4");
        Log.d(TAG, "Transcoding to destination: " + destinationUri);

        testTranscodingWithExpectResult(mSourceHEVCVideoUri, destinationUri,
                TranscodingSession.RESULT_ERROR);
    }

    // Tests transcoding to a uri in internal cache folder and expects success.
    public void testTranscodingToCacheDir() throws Exception {
        if (shouldSkip()) {
            return;
        }
        // Create a file Uri: file:///data/user/0/android.media.cts/cache/temp.mp4
        Uri destinationUri = Uri.parse(ContentResolver.SCHEME_FILE + "://"
                + mContext.getCacheDir().getAbsolutePath() + "/temp.mp4");
        Log.d(TAG, "Transcoding to cache: " + destinationUri);

        testTranscodingWithExpectResult(mSourceHEVCVideoUri, destinationUri,
                TranscodingSession.RESULT_SUCCESS);
    }

    // Tests transcoding to a uri in internal files directory and expects success.
    public void testTranscodingToInternalFilesDir() throws Exception {
        if (shouldSkip()) {
            return;
        }
        // Create a file Uri: file:///data/user/0/android.media.cts/files/temp.mp4
        Uri destinationUri = Uri.fromFile(new File(mContext.getFilesDir(), "temp.mp4"));
        Log.i(TAG, "Transcoding to files dir: " + destinationUri);

        testTranscodingWithExpectResult(mSourceHEVCVideoUri, destinationUri,
                TranscodingSession.RESULT_SUCCESS);
    }

    public void testAvcTranscoding1080PVideo30FramesWithoutAudio() throws Exception {
        if (shouldSkip()) {
            return;
        }
        transcodeFile(resourceToUri(mContext, R.raw.Video_AVC_30Frames, "Video_AVC_30Frames.mp4"),
                false /* testFileDescriptor */);
    }

    public void testHevcTranscoding1080PVideo30FramesWithoutAudio() throws Exception {
        if (shouldSkip()) {
            return;
        }
        transcodeFile(
                resourceToUri(mContext, R.raw.Video_HEVC_30Frames, "Video_HEVC_30Frames.mp4"),
                false /* testFileDescriptor */);
    }

    // Enable this after fixing b/175641397
    public void testHevcTranscoding1080PVideo1FrameWithAudio() throws Exception {
        if (shouldSkip()) {
            return;
        }
        transcodeFile(resourceToUri(mContext, R.raw.Video_HEVC_1Frame_Audio,
                "Video_HEVC_1Frame_Audio.mp4"), false /* testFileDescriptor */);
    }

    public void testHevcTranscoding1080PVideo37FramesWithAudio() throws Exception {
        if (shouldSkip()) {
            return;
        }
        transcodeFile(resourceToUri(mContext, R.raw.Video_HEVC_37Frames_Audio,
                "Video_HEVC_37Frames_Audio.mp4"), false /* testFileDescriptor */);
    }

    public void testHevcTranscoding1080PVideo72FramesWithAudio() throws Exception {
        if (shouldSkip()) {
            return;
        }
        transcodeFile(resourceToUri(mContext, R.raw.Video_HEVC_72Frames_Audio,
                "Video_HEVC_72Frames_Audio.mp4"), false /* testFileDescriptor */);
    }

    // This test will only run when the device support decoding and encoding 4K video.
    public void testHevcTranscoding4KVideo64FramesWithAudio() throws Exception {
        if (shouldSkip()) {
            return;
        }
        MediaFormat format = new MediaFormat();
        format.setString(MediaFormat.KEY_MIME, MediaFormat.MIMETYPE_VIDEO_HEVC);
        format.setInteger(MediaFormat.KEY_WIDTH, 3840);
        format.setInteger(MediaFormat.KEY_HEIGHT, 2160);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, 30);
        if (!MediaUtils.canDecode(format) || !MediaUtils.canEncode(format) ) {
            return;
        }
        transcodeFile(resourceToUri(mContext, R.raw.Video_4K_HEVC_64Frames_Audio,
                "Video_4K_HEVC_64Frames_Audio.mp4"), false /* testFileDescriptor */);
    }

    public void testHevcTranscodingWithFileDescriptor() throws Exception {
        if (shouldSkip()) {
            return;
        }
        transcodeFile(resourceToUri(mContext, R.raw.Video_HEVC_37Frames_Audio,
                "Video_HEVC_37Frames_Audio.mp4"), true /* testFileDescriptor */);
    }

    private void transcodeFile(Uri fileUri, boolean testFileDescriptor) throws Exception {
        Semaphore transcodeCompleteSemaphore = new Semaphore(0);

        // Create a file Uri: file:///data/user/0/android.media.cts/cache/HevcTranscode.mp4
        Uri destinationUri = Uri.parse(ContentResolver.SCHEME_FILE + "://"
                + mContext.getCacheDir().getAbsolutePath() + "/HevcTranscode.mp4");

        ApplicationMediaCapabilities clientCaps =
                new ApplicationMediaCapabilities.Builder().build();

        TranscodingRequest.VideoFormatResolver
                resolver = new TranscodingRequest.VideoFormatResolver(clientCaps,
                MediaFormat.createVideoFormat(
                        MediaFormat.MIMETYPE_VIDEO_HEVC, WIDTH, HEIGHT));
        assertTrue(resolver.shouldTranscode());
        MediaFormat videoTrackFormat = resolver.resolveVideoFormat();
        assertNotNull(videoTrackFormat);

        int pid = android.os.Process.myPid();
        int uid = android.os.Process.myUid();

        VideoTranscodingRequest.Builder builder =
                new VideoTranscodingRequest.Builder(fileUri, destinationUri, videoTrackFormat)
                        .setClientPid(pid)
                        .setClientUid(uid);

        AssetFileDescriptor srcFd = null;
        AssetFileDescriptor dstFd = null;
        if (testFileDescriptor) {
            // Open source Uri.
            srcFd = mContentResolver.openAssetFileDescriptor(fileUri,
                    "r");
            builder.setSourceFileDescriptor(srcFd.getParcelFileDescriptor());
            // Open destination Uri
            dstFd = mContentResolver.openAssetFileDescriptor(destinationUri, "rw");
            builder.setDestinationFileDescriptor(dstFd.getParcelFileDescriptor());
        }
        VideoTranscodingRequest request = builder.build();
        Executor listenerExecutor = Executors.newSingleThreadExecutor();
        assertEquals(pid, request.getClientPid());
        assertEquals(uid, request.getClientUid());

        Log.d(TAG, "transcoding to format: " + videoTrackFormat);

        TranscodingSession session = mMediaTranscodingManager.enqueueRequest(
                request,
                listenerExecutor,
                transcodingSession -> {
                    Log.d(TAG,
                            "Transcoding completed with result: " + transcodingSession.getResult());
                    assertEquals(TranscodingSession.RESULT_SUCCESS, transcodingSession.getResult());
                    transcodeCompleteSemaphore.release();
                });
        assertNotNull(session);
        assertTrue(compareFormat(videoTrackFormat, request.getVideoTrackFormat()));
        assertEquals(fileUri, request.getSourceUri());
        assertEquals(destinationUri, request.getDestinationUri());
        if (testFileDescriptor) {
            assertEquals(srcFd.getParcelFileDescriptor(), request.getSourceFileDescriptor());
            assertEquals(dstFd.getParcelFileDescriptor(), request.getDestinationFileDescriptor());
        }

        if (session != null) {
            Log.d(TAG, "testMediaTranscodingManager - Waiting for transcode to cancel.");
            boolean finishedOnTime = transcodeCompleteSemaphore.tryAcquire(
                    TRANSCODE_TIMEOUT_SECONDS, TimeUnit.SECONDS);
            assertTrue("Transcode failed to complete in time.", finishedOnTime);
        }

        if (DEBUG_TRANSCODED_VIDEO) {
            try {
                // Add the system time to avoid duplicate that leads to write failure.
                String filename =
                        "transcoded_" + System.nanoTime() + "_" + fileUri.getLastPathSegment();
                String path = "/storage/emulated/0/Download/" + filename;
                final File file = new File(path);
                ParcelFileDescriptor pfd = mContext.getContentResolver().openFileDescriptor(
                        destinationUri, "r");
                FileInputStream fis = new FileInputStream(pfd.getFileDescriptor());
                FileOutputStream fos = new FileOutputStream(file);
                FileUtils.copy(fis, fos);
            } catch (IOException e) {
                Log.e(TAG, "Failed to copy file", e);
            }
        }

        assertEquals(TranscodingSession.STATUS_FINISHED, session.getStatus());
        assertEquals(TranscodingSession.RESULT_SUCCESS, session.getResult());
        assertEquals(TranscodingSession.ERROR_NONE, session.getErrorCode());

        // TODO(hkuang): Validate the transcoded video's width and height, framerate.

        // Validates the transcoded video's psnr.
        // Enable this after fixing b/175644377
        MediaTranscodingTestUtil.VideoTranscodingStatistics stats =
                MediaTranscodingTestUtil.computeStats(mContext, fileUri, destinationUri, DEBUG_YUV);
        assertTrue("PSNR: " + stats.mAveragePSNR + " is too low",
                stats.mAveragePSNR >= PSNR_THRESHOLD);
    }

    private boolean compareFormat(MediaFormat fmt1, MediaFormat fmt2) {
        if (fmt1 == fmt2) return true;
        if (fmt1 == null || fmt2 == null) return false;

        return (fmt1.getString(MediaFormat.KEY_MIME) == fmt2.getString(MediaFormat.KEY_MIME) &&
                fmt1.getInteger(MediaFormat.KEY_WIDTH) == fmt2.getInteger(MediaFormat.KEY_WIDTH) &&
                fmt1.getInteger(MediaFormat.KEY_HEIGHT) == fmt2.getInteger(MediaFormat.KEY_HEIGHT)
                && fmt1.getInteger(MediaFormat.KEY_BIT_RATE) == fmt2.getInteger(
                MediaFormat.KEY_BIT_RATE));
    }

    public void testCancelTranscoding() throws Exception {
        if (shouldSkip()) {
            return;
        }
        Log.d(TAG, "Starting: testCancelTranscoding");
        Semaphore transcodeCompleteSemaphore = new Semaphore(0);
        final CountDownLatch statusLatch = new CountDownLatch(1);

        Uri destinationUri = Uri.parse(ContentResolver.SCHEME_FILE + "://"
                + mContext.getCacheDir().getAbsolutePath() + "/HevcTranscode.mp4");

        VideoTranscodingRequest request =
                new VideoTranscodingRequest.Builder(mSourceHEVCVideoUri, destinationUri,
                        createMediaFormat())
                        .build();
        Executor listenerExecutor = Executors.newSingleThreadExecutor();

        TranscodingSession session = mMediaTranscodingManager.enqueueRequest(
                request,
                listenerExecutor,
                transcodingSession -> {
                    Log.d(TAG,
                            "Transcoding completed with result: " + transcodingSession.getResult());
                    assertEquals(TranscodingSession.RESULT_CANCELED,
                            transcodingSession.getResult());
                    transcodeCompleteSemaphore.release();
                });
        assertNotNull(session);

        assertTrue(session.getSessionId() != -1);

        // Wait for progress update before cancel the transcoding.
        session.setOnProgressUpdateListener(listenerExecutor,
                new TranscodingSession.OnProgressUpdateListener() {
                    @Override
                    public void onProgressUpdate(TranscodingSession session, int newProgress) {
                        if (newProgress > 0) {
                            statusLatch.countDown();
                        }
                        assertEquals(newProgress, session.getProgress());
                    }
                });

        statusLatch.await(2, TimeUnit.MILLISECONDS);
        session.cancel();

        Log.d(TAG, "testMediaTranscodingManager - Waiting for transcode to cancel.");
        boolean finishedOnTime = transcodeCompleteSemaphore.tryAcquire(
                30, TimeUnit.MILLISECONDS);

        assertEquals(TranscodingSession.STATUS_FINISHED, session.getStatus());
        assertEquals(TranscodingSession.RESULT_CANCELED, session.getResult());
        assertEquals(TranscodingSession.ERROR_NONE, session.getErrorCode());
        assertTrue("Fails to cancel transcoding", finishedOnTime);
    }

    // Transcoding video on behalf of init dameon and expect UnsupportedOperationException due to
    // CTS test is not a privilege caller.
    // Disable this test as Android S will only allow MediaProvider to access the API.
    /*public void testPidAndUidForwarding() throws Exception {
        if (shouldSkip()) {
            return;
        }
        assertThrows(UnsupportedOperationException.class, () -> {
            Semaphore transcodeCompleteSemaphore = new Semaphore(0);

            // Use init dameon's pid and uid.
            int pid = 1;
            int uid = 0;
            TranscodingRequest request =
                    new TranscodingRequest.Builder()
                            .setSourceUri(mSourceHEVCVideoUri)
                            .setDestinationUri(mDestinationUri)
                            .setType(MediaTranscodingManager.TRANSCODING_TYPE_VIDEO)
                            .setClientPid(pid)
                            .setClientUid(uid)
                            .setPriority(MediaTranscodingManager.PRIORITY_REALTIME)
                            .setVideoTrackFormat(createMediaFormat())
                            .build();
            Executor listenerExecutor = Executors.newSingleThreadExecutor();

            TranscodingSession session =
                    mMediaTranscodingManager.enqueueRequest(
                            request,
                            listenerExecutor,
                            transcodingSession -> {
                                transcodeCompleteSemaphore.release();
                            });
        });
    }*/

    public void testTranscodingProgressUpdate() throws Exception {
        if (shouldSkip()) {
            return;
        }
        Log.d(TAG, "Starting: testTranscodingProgressUpdate");

        Semaphore transcodeCompleteSemaphore = new Semaphore(0);

        // Create a file Uri: file:///data/user/0/android.media.mediatranscoding.cts/cache/HevcTranscode.mp4
        Uri destinationUri = Uri.parse(ContentResolver.SCHEME_FILE + "://"
                + mContext.getCacheDir().getAbsolutePath() + "/HevcTranscode.mp4");

        VideoTranscodingRequest request =
                new VideoTranscodingRequest.Builder(mSourceHEVCVideoUri, destinationUri,
                        createMediaFormat())
                        .build();
        Executor listenerExecutor = Executors.newSingleThreadExecutor();

        TranscodingSession session = mMediaTranscodingManager.enqueueRequest(request,
                listenerExecutor,
                TranscodingSession -> {
                    Log.d(TAG,
                            "Transcoding completed with result: " + TranscodingSession.getResult());
                    assertEquals(TranscodingSession.RESULT_SUCCESS, TranscodingSession.getResult());
                    transcodeCompleteSemaphore.release();
                });
        assertNotNull(session);

        AtomicInteger progressUpdateCount = new AtomicInteger(0);

        // Set progress update executor and use the same executor as result listener.
        session.setOnProgressUpdateListener(listenerExecutor,
                new TranscodingSession.OnProgressUpdateListener() {
                    int mPreviousProgress = 0;

                    @Override
                    public void onProgressUpdate(TranscodingSession session, int newProgress) {
                        assertTrue("Invalid proress update", newProgress > mPreviousProgress);
                        assertTrue("Invalid proress update", newProgress <= 100);
                        mPreviousProgress = newProgress;
                        progressUpdateCount.getAndIncrement();
                        Log.i(TAG, "Get progress update " + newProgress);
                    }
                });

        boolean finishedOnTime = transcodeCompleteSemaphore.tryAcquire(
                TRANSCODE_TIMEOUT_SECONDS, TimeUnit.SECONDS);
        assertTrue("Transcode failed to complete in time.", finishedOnTime);
        assertTrue("Failed to receive at least 10 progress updates",
                progressUpdateCount.get() > 10);
    }

    public void testAddingClientUids() throws Exception {
        if (shouldSkip()) {
            return;
        }
        Log.d(TAG, "Starting: testTranscodingProgressUpdate");

        Semaphore transcodeCompleteSemaphore = new Semaphore(0);

        // Create a file Uri: file:///data/user/0/android.media.mediatranscoding.cts/cache/HevcTranscode.mp4
        Uri destinationUri = Uri.parse(ContentResolver.SCHEME_FILE + "://"
                + mContext.getCacheDir().getAbsolutePath() + "/HevcTranscode.mp4");

        VideoTranscodingRequest request =
                new VideoTranscodingRequest.Builder(mSourceHEVCVideoUri, destinationUri,
                        createMediaFormat())
                        .build();
        Executor listenerExecutor = Executors.newSingleThreadExecutor();

        TranscodingSession session = mMediaTranscodingManager.enqueueRequest(request,
                listenerExecutor,
                TranscodingSession -> {
                    Log.d(TAG,
                            "Transcoding completed with result: " + TranscodingSession.getResult());
                    assertEquals(TranscodingSession.RESULT_SUCCESS, TranscodingSession.getResult());
                    transcodeCompleteSemaphore.release();
                });
        assertNotNull(session);

        session.addClientUid(1898 /* test_uid */);
        session.addClientUid(1899 /* test_uid */);
        session.addClientUid(1900 /* test_uid */);

        List<Integer> uids = session.getClientUids();
        assertTrue(uids.size() == 4);  // At least 4 uid included the original request uid.
        assertTrue(uids.contains(1898));
        assertTrue(uids.contains(1899));
        assertTrue(uids.contains(1900));

        AtomicInteger progressUpdateCount = new AtomicInteger(0);

        // Set progress update executor and use the same executor as result listener.
        session.setOnProgressUpdateListener(listenerExecutor,
                new TranscodingSession.OnProgressUpdateListener() {
                    int mPreviousProgress = 0;

                    @Override
                    public void onProgressUpdate(TranscodingSession session, int newProgress) {
                        assertTrue("Invalid proress update", newProgress > mPreviousProgress);
                        assertTrue("Invalid proress update", newProgress <= 100);
                        mPreviousProgress = newProgress;
                        progressUpdateCount.getAndIncrement();
                        Log.i(TAG, "Get progress update " + newProgress);
                    }
                });

        boolean finishedOnTime = transcodeCompleteSemaphore.tryAcquire(
                TRANSCODE_TIMEOUT_SECONDS, TimeUnit.SECONDS);
        assertTrue("Transcode failed to complete in time.", finishedOnTime);
        assertTrue("Failed to receive at least 10 progress updates",
                progressUpdateCount.get() > 10);
    }
}

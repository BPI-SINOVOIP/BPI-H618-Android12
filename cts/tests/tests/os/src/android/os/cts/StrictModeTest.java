/*
 * Copyright (C) 2015 The Android Open Source Project
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

package android.os.cts;

import static android.content.Context.WINDOW_SERVICE;
import static android.view.Display.DEFAULT_DISPLAY;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.Activity;
import android.app.Instrumentation;
import android.app.WallpaperManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.hardware.display.DisplayManager;
import android.net.TrafficStats;
import android.net.Uri;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.StrictMode;
import android.os.StrictMode.ThreadPolicy.Builder;
import android.os.StrictMode.ViolationInfo;
import android.os.strictmode.CleartextNetworkViolation;
import android.os.strictmode.CustomViolation;
import android.os.strictmode.DiskReadViolation;
import android.os.strictmode.DiskWriteViolation;
import android.os.strictmode.ExplicitGcViolation;
import android.os.strictmode.FileUriExposedViolation;
import android.os.strictmode.InstanceCountViolation;
import android.os.strictmode.LeakedClosableViolation;
import android.os.strictmode.NetworkViolation;
import android.os.strictmode.NonSdkApiUsedViolation;
import android.os.strictmode.UnsafeIntentLaunchViolation;
import android.os.strictmode.UntaggedSocketViolation;
import android.os.strictmode.Violation;
import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.AppModeInstant;
import android.platform.test.annotations.Presubmit;
import android.system.Os;
import android.system.OsConstants;
import android.util.Log;
import android.view.Display;
import android.view.GestureDetector;
import android.view.ViewConfiguration;
import android.view.WindowManager;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.Socket;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

/** Tests for {@link StrictMode} */
@RunWith(AndroidJUnit4.class)
public class StrictModeTest {
    private static final String TAG = "StrictModeTest";
    private static final String REMOTE_SERVICE_ACTION = "android.app.REMOTESERVICE";
    private static final String UNSAFE_INTENT_LAUNCH = "UnsafeIntentLaunch";

    private StrictMode.ThreadPolicy mThreadPolicy;
    private StrictMode.VmPolicy mVmPolicy;

    private Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
    private GestureDetector.OnGestureListener mGestureListener =
            new GestureDetector.SimpleOnGestureListener();
    private static final String WM_CLASS_NAME = WindowManager.class.getSimpleName();

    private Context getContext() {
        return ApplicationProvider.getApplicationContext();
    }

    @Before
    public void setUp() {
        mThreadPolicy = StrictMode.getThreadPolicy();
        mVmPolicy = StrictMode.getVmPolicy();
    }

    @After
    public void tearDown() {
        StrictMode.setThreadPolicy(mThreadPolicy);
        StrictMode.setVmPolicy(mVmPolicy);
    }

    public interface ThrowingRunnable {
        void run() throws Exception;
    }

    @Test
    public void testThreadBuilder() throws Exception {
        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().detectDiskReads().penaltyLog().build();
        StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder(policy).build());

        final File test = File.createTempFile("foo", "bar");
        inspectViolation(
                test::exists,
                info -> {
                    assertThat(info.getViolationDetails()).isNull();
                    assertThat(info.getStackTrace()).contains("DiskReadViolation");
                });
    }

    @Test
    public void testUnclosedCloseable() throws Exception {
        //clean before test
        System.gc();
        System.runFinalization();

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectLeakedClosableObjects().build());

        inspectViolation(
                () -> leakCloseable("leaked.txt"),
                info -> {
                    assertThat(info.getViolationDetails())
                            .isEqualTo(
                                    "A resource was acquired at attached stack trace but never released. See java.io.Closeable for information on avoiding resource leaks.");
                    assertThat(info.getStackTrace())
                            .contains("Explicit termination method 'close' not called");
                    assertThat(info.getStackTrace()).contains("leakCloseable");
                    assertThat(info.getViolationClass())
                            .isAssignableTo(LeakedClosableViolation.class);
                });
    }

    private void leakCloseable(String fileName) throws InterruptedException {
        final CountDownLatch finalizedSignal = new CountDownLatch(1);
        try {
            new FileOutputStream(new File(getContext().getFilesDir(), fileName)) {
                @Override
                protected void finalize() throws IOException {
                    super.finalize();
                    finalizedSignal.countDown();
                }
            };
        } catch (FileNotFoundException e) {
            throw new RuntimeException(e);
        }
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        // Sometimes it needs extra prodding.
        if (!finalizedSignal.await(5, TimeUnit.SECONDS)) {
            Runtime.getRuntime().gc();
            Runtime.getRuntime().runFinalization();
        }
    }

    @Test
    public void testClassInstanceLimit() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .setClassInstanceLimit(LimitedClass.class, 1)
                        .build());
        List<LimitedClass> references = new ArrayList<>();
        assertNoViolation(() -> references.add(new LimitedClass()));
        references.add(new LimitedClass());
        inspectViolation(
                StrictMode::conditionallyCheckInstanceCounts,
                info -> assertThat(info.getViolationClass())
                        .isAssignableTo(InstanceCountViolation.class));
    }

    private static final class LimitedClass {}

    /** Insecure connection should be detected */
    @AppModeFull
    @Test
    public void testCleartextNetwork() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testCleartextNetwork() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectCleartextNetwork().penaltyLog().build());

        inspectViolation(
                () ->
                        ((HttpURLConnection) new URL("http://example.com/").openConnection())
                                .getResponseCode(),
                info -> assertThat(info.getViolationClass())
                        .isAssignableTo(CleartextNetworkViolation.class));
    }

    /** Secure connection should be ignored */
    @Test
    public void testEncryptedNetwork() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testEncryptedNetwork() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectCleartextNetwork().penaltyLog().build());

        assertNoViolation(
                () ->
                        ((HttpURLConnection) new URL("https://example.com/").openConnection())
                                .getResponseCode());
    }

    @Test
    public void testFileUriExposure() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectFileUriExposure().penaltyLog().build());

        final Uri badUri = Uri.fromFile(new File("/sdcard/meow.jpg"));
        inspectViolation(
                () -> {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setDataAndType(badUri, "image/jpeg");
                    getContext().startActivity(intent);
                },
                info -> {
                    assertThat(info.getStackTrace()).contains(badUri + " exposed beyond app");
                });

        final Uri goodUri = Uri.parse("content://com.example/foobar");
        assertNoViolation(
                () -> {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setDataAndType(goodUri, "image/jpeg");
                    getContext().startActivity(intent);
                });
    }

    @Test
    public void testFileUriExposure_Chooser() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectFileUriExposure().penaltyLog().build());

        final Uri badUri = Uri.fromFile(new File("/sdcard/meow.jpg"));
        inspectViolation(
                () -> {
                    Intent intent = new Intent(Intent.ACTION_SEND);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setType("image/jpeg");
                    intent.putExtra(Intent.EXTRA_STREAM, badUri);

                    Intent chooser = Intent.createChooser(intent, "CTS");
                    chooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    getContext().startActivity(chooser);
                },
                info -> {
                    assertThat(info.getStackTrace()).contains(badUri + " exposed beyond app");
                });
    }

    @Test
    public void testContentUriWithoutPermission() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectContentUriWithoutPermission()
                        .penaltyLog()
                        .build());

        final Uri uri = Uri.parse("content://com.example/foobar");
        inspectViolation(
                () -> {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setDataAndType(uri, "image/jpeg");
                    getContext().startActivity(intent);
                },
                info ->
                        assertThat(info.getStackTrace())
                                .contains(uri + " exposed beyond app"));

        assertNoViolation(
                () -> {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setDataAndType(uri, "image/jpeg");
                    intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    getContext().startActivity(intent);
                });
    }

    @AppModeFull
    @Test
    public void testUntaggedSocketsHttp() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testUntaggedSockets() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectUntaggedSockets().penaltyLog().build());

        inspectViolation(
                () ->
                        ((HttpURLConnection) new URL("http://example.com/").openConnection())
                                .getResponseCode(),
                info -> assertThat(info.getViolationClass())
                        .isAssignableTo(UntaggedSocketViolation.class));

        assertNoViolation(
                () -> {
                    TrafficStats.setThreadStatsTag(0xDECAFBAD);
                    try {
                        ((HttpURLConnection) new URL("http://example.com/").openConnection())
                                .getResponseCode();
                    } finally {
                        TrafficStats.clearThreadStatsTag();
                    }
                });
    }

    @Test
    public void testUntaggedSocketsRaw() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testUntaggedSockets() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectUntaggedSockets().penaltyLog().build());

        assertNoViolation(
                () -> {
                    TrafficStats.setThreadStatsTag(0xDECAFBAD);
                    try (Socket socket = new Socket("example.com", 80)) {
                        socket.getOutputStream().close();
                    } finally {
                        TrafficStats.clearThreadStatsTag();
                    }
                });

        inspectViolation(
                () -> {
                    try (Socket socket = new Socket("example.com", 80)) {
                        socket.getOutputStream().close();
                    }
                },
                info -> assertThat(info.getViolationClass())
                        .isAssignableTo(UntaggedSocketViolation.class));
    }

    private static final int PERMISSION_USER_ONLY = 0600;

    @Test
    public void testRead() throws Exception {
        final File test = File.createTempFile("foo", "bar");
        final File dir = test.getParentFile();

        final FileInputStream is = new FileInputStream(test);
        final FileDescriptor fd =
                Os.open(test.getAbsolutePath(), OsConstants.O_RDONLY, PERMISSION_USER_ONLY);

        StrictMode.setThreadPolicy(
                new StrictMode.ThreadPolicy.Builder().detectDiskReads().penaltyLog().build());
        inspectViolation(
                test::exists,
                info -> {
                    assertThat(info.getViolationDetails()).isNull();
                    assertThat(info.getStackTrace()).contains("DiskReadViolation");
                });

        Consumer<ViolationInfo> assertDiskReadPolicy = info -> assertThat(
                info.getViolationClass()).isAssignableTo(DiskReadViolation.class);
        inspectViolation(test::exists, assertDiskReadPolicy);
        inspectViolation(test::length, assertDiskReadPolicy);
        inspectViolation(dir::list, assertDiskReadPolicy);
        inspectViolation(is::read, assertDiskReadPolicy);

        inspectViolation(() -> new FileInputStream(test), assertDiskReadPolicy);
        inspectViolation(
                () -> Os.open(test.getAbsolutePath(), OsConstants.O_RDONLY, PERMISSION_USER_ONLY),
                assertDiskReadPolicy);
        inspectViolation(() -> Os.read(fd, new byte[10], 0, 1), assertDiskReadPolicy);
    }

    @Test
    public void testWrite() throws Exception {
        File file = File.createTempFile("foo", "bar");

        final FileOutputStream os = new FileOutputStream(file);
        final FileDescriptor fd =
                Os.open(file.getAbsolutePath(), OsConstants.O_RDWR, PERMISSION_USER_ONLY);

        StrictMode.setThreadPolicy(
                new StrictMode.ThreadPolicy.Builder().detectDiskWrites().penaltyLog().build());

        inspectViolation(
                file::createNewFile,
                info -> {
                    assertThat(info.getViolationDetails()).isNull();
                    assertThat(info.getStackTrace()).contains("DiskWriteViolation");
                });

        Consumer<ViolationInfo> assertDiskWritePolicy = info -> assertThat(
                info.getViolationClass()).isAssignableTo(DiskWriteViolation.class);

        inspectViolation(() -> File.createTempFile("foo", "bar"), assertDiskWritePolicy);
        inspectViolation(() -> new FileOutputStream(file), assertDiskWritePolicy);
        inspectViolation(file::delete, assertDiskWritePolicy);
        inspectViolation(file::createNewFile, assertDiskWritePolicy);
        inspectViolation(() -> os.write(32), assertDiskWritePolicy);

        inspectViolation(
                () -> Os.open(file.getAbsolutePath(), OsConstants.O_RDWR, PERMISSION_USER_ONLY),
                assertDiskWritePolicy);
        inspectViolation(() -> Os.write(fd, new byte[10], 0, 1), assertDiskWritePolicy);
        inspectViolation(() -> Os.fsync(fd), assertDiskWritePolicy);
        inspectViolation(
                () -> file.renameTo(new File(file.getParent(), "foobar")), assertDiskWritePolicy);
    }

    @AppModeFull
    @Test
    public void testNetwork() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testUntaggedSockets() ignored on device without Internet");
            return;
        }

        StrictMode.setThreadPolicy(
                new StrictMode.ThreadPolicy.Builder().detectNetwork().penaltyLog().build());

        inspectViolation(
                () -> {
                    try (Socket socket = new Socket("example.com", 80)) {
                        socket.getOutputStream().close();
                    }
                },
                info -> assertThat(info.getViolationClass())
                        .isAssignableTo(NetworkViolation.class));
        inspectViolation(
                () ->
                        ((HttpURLConnection) new URL("http://example.com/").openConnection())
                                .getResponseCode(),
                info -> assertThat(info.getViolationClass())
                        .isAssignableTo(NetworkViolation.class));
    }

    @Test
    public void testExplicitGc() throws Exception {
        StrictMode.setThreadPolicy(
                new StrictMode.ThreadPolicy.Builder().detectExplicitGc().penaltyLog().build());

        inspectViolation(
                () -> { Runtime.getRuntime().gc(); },
                info -> assertThat(info.getViolationClass())
                        .isAssignableTo(ExplicitGcViolation.class));
    }

    @Test
    public void testViolationAcrossBinder() throws Exception {
        runWithRemoteServiceBound(
                getContext(),
                service -> {
                    StrictMode.setThreadPolicy(
                            new Builder().detectDiskWrites().penaltyLog().build());

                    try {
                        inspectViolation(
                                () -> service.performDiskWrite(),
                                (info) -> {
                                    assertThat(info.getViolationClass())
                                            .isAssignableTo(DiskWriteViolation.class);
                                    assertThat(info.getViolationDetails())
                                            .isNull(); // Disk write has no message.
                                    assertThat(info.getStackTrace())
                                            .contains("DiskWriteViolation");
                                    assertThat(info.getStackTrace())
                                            .contains(
                                                    "at android.os.StrictMode$AndroidBlockGuardPolicy.onWriteToDisk");
                                    assertThat(info.getStackTrace())
                                            .contains("# via Binder call with stack:");
                                    assertThat(info.getStackTrace())
                                            .contains(
                                                    "at android.os.cts.ISecondary$Stub$Proxy.performDiskWrite");
                                });
                        assertNoViolation(() -> service.getPid());
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                });
    }

    private void checkNonSdkApiUsageViolation(boolean blacklist, String className,
            String methodName, Class<?>... paramTypes) throws Exception {
        Class<?> clazz = Class.forName(className);
        inspectViolation(
            () -> {
                try {
                    java.lang.reflect.Method m = clazz.getDeclaredMethod(methodName, paramTypes);
                    if (blacklist) {
                        fail();
                    }
                } catch (NoSuchMethodException expected) {
                  if (!blacklist) {
                    fail();
                  }
                }
            },
            info -> {
                assertThat(info).isNotNull();
                assertThat(info.getViolationClass())
                        .isAssignableTo(NonSdkApiUsedViolation.class);
                assertThat(info.getViolationDetails()).contains(methodName);
                assertThat(info.getStackTrace()).contains("checkNonSdkApiUsageViolation");
            }
        );
    }

    @Test
    public void testNonSdkApiUsage() throws Exception {
        StrictMode.VmPolicy oldVmPolicy = StrictMode.getVmPolicy();
        StrictMode.ThreadPolicy oldThreadPolicy = StrictMode.getThreadPolicy();
        try {
            StrictMode.setVmPolicy(
                    new StrictMode.VmPolicy.Builder().detectNonSdkApiUsage().build());
            checkNonSdkApiUsageViolation(
                true, "dalvik.system.VMRuntime", "setHiddenApiExemptions", String[].class);
            // verify that mutliple uses of a light greylist API are detected.
            checkNonSdkApiUsageViolation(false, "dalvik.system.VMRuntime", "getRuntime");
            checkNonSdkApiUsageViolation(false, "dalvik.system.VMRuntime", "getRuntime");

            // Verify that the VM policy is turned off after a call to permitNonSdkApiUsage.
            StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().permitNonSdkApiUsage().build());
            assertNoViolation(() -> {
                  Class<?> clazz = Class.forName("dalvik.system.VMRuntime");
                  try {
                      clazz.getDeclaredMethod("getRuntime");
                  } catch (NoSuchMethodException maybe) {
                  }
            });
        } finally {
            StrictMode.setVmPolicy(oldVmPolicy);
            StrictMode.setThreadPolicy(oldThreadPolicy);
        }
    }

    @Test
    public void testThreadPenaltyListener() throws Exception {
        final BlockingQueue<Violation> violations = new ArrayBlockingQueue<>(1);
        StrictMode.setThreadPolicy(
                new StrictMode.ThreadPolicy.Builder().detectCustomSlowCalls()
                        .penaltyListener(getContext().getMainExecutor(), (v) -> {
                            violations.add(v);
                        }).build());

        StrictMode.noteSlowCall("foo");

        final Violation v = violations.poll(5, TimeUnit.SECONDS);
        assertTrue(v instanceof CustomViolation);
    }

    @Test
    public void testVmPenaltyListener() throws Exception {
        final BlockingQueue<Violation> violations = new ArrayBlockingQueue<>(1);
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectFileUriExposure()
                        .penaltyListener(getContext().getMainExecutor(), (v) -> {
                            violations.add(v);
                        }).build());

        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setDataAndType(Uri.fromFile(new File("/sdcard/meow.jpg")), "image/jpeg");
        getContext().startActivity(intent);

        final Violation v = violations.poll(5, TimeUnit.SECONDS);
        assertTrue(v instanceof FileUriExposedViolation);
    }

    @AppModeInstant
    @Test
    public void testNoCleartextHttpTrafficAllowed() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testNoCleartextHttpTrafficAllowed() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectCleartextNetwork().penaltyLog().build());

        try {
            inspectViolation(
                    () ->
                            ((HttpURLConnection) new URL("http://example.com/").openConnection())
                                    .getResponseCode(),
                    info -> assertThat(info.getViolationClass())
                            .isAssignableTo(CleartextNetworkViolation.class));
            fail("Instant app was able to send cleartext http traffic.");
        } catch (IOException ex) {
            // Expected
        }
    }

    @Presubmit
    @Test
    public void testIncorrectContextUse_Application_ThrowViolation() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectIncorrectContextUse()
                        .penaltyLog()
                        .build());

        final Context applicationContext = getContext();

        assertViolation("Tried to access visual service " + WM_CLASS_NAME,
                () -> applicationContext.getSystemService(WindowManager.class));

        assertViolation(
                "The API:ViewConfiguration needs a proper configuration.",
                () -> ViewConfiguration.get(applicationContext));

        mInstrumentation.runOnMainSync(() -> {
            try {
                assertViolation("The API:GestureDetector#init needs a proper configuration.",
                        () -> new GestureDetector(applicationContext, mGestureListener));
            } catch (Exception e) {
                fail("Failed because of " + e);
            }
        });

        if (isWallpaperSupported()) {
            assertViolation("Tried to access UI related API:", () ->
                    applicationContext.getSystemService(WallpaperManager.class)
                            .getDesiredMinimumWidth());
        }
    }

    @Presubmit
    @Test
    public void testIncorrectContextUse_DisplayContext_ThrowViolation() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectIncorrectContextUse()
                        .penaltyLog()
                        .build());

        final Display display = getContext().getSystemService(DisplayManager.class)
                .getDisplay(DEFAULT_DISPLAY);
        final Context displayContext = getContext().createDisplayContext(display);

        assertViolation("Tried to access visual service " + WM_CLASS_NAME,
                () -> displayContext.getSystemService(WindowManager.class));

        assertViolation(
                "The API:ViewConfiguration needs a proper configuration.",
                () -> ViewConfiguration.get(displayContext));

        mInstrumentation.runOnMainSync(() -> {
            try {
                assertViolation("The API:GestureDetector#init needs a proper configuration.",
                        () -> new GestureDetector(displayContext, mGestureListener));
            } catch (Exception e) {
                fail("Failed because of " + e);
            }
        });

        if (isWallpaperSupported()) {
            assertViolation("Tried to access UI related API:", () ->
                    displayContext.getSystemService(WallpaperManager.class)
                            .getDesiredMinimumWidth());
        }
    }

    @Presubmit
    @Test
    public void testIncorrectContextUse_WindowContext_NoViolation() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectIncorrectContextUse()
                        .penaltyLog()
                        .build());

        final Context windowContext = createWindowContext();

        assertNoViolation(() -> windowContext.getSystemService(WINDOW_SERVICE));

        assertNoViolation(() -> ViewConfiguration.get(windowContext));

        mInstrumentation.runOnMainSync(() -> {
            try {
                assertNoViolation(() -> new GestureDetector(windowContext, mGestureListener));
            } catch (Exception e) {
                fail("Failed because of " + e);
            }
        });

        if (isWallpaperSupported()) {
            assertNoViolation(() -> windowContext.getSystemService(WallpaperManager.class)
                    .getDesiredMinimumWidth());
        }
    }

    @Presubmit
    @Test
    public void testIncorrectContextUse_Activity_NoViolation() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectIncorrectContextUse()
                        .penaltyLog()
                        .build());

        Intent intent = new Intent(getContext(), SimpleTestActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Activity activity = mInstrumentation.startActivitySync(intent);

        assertNoViolation(() -> activity.getSystemService(WINDOW_SERVICE));

        assertNoViolation(() -> ViewConfiguration.get(activity));

        mInstrumentation.runOnMainSync(() -> {
            try {
                assertNoViolation(() -> new GestureDetector(activity, mGestureListener));
            } catch (Exception e) {
                fail("Failed because of " + e);
            }
        });

        if (isWallpaperSupported()) {
            assertNoViolation(() -> activity.getSystemService(WallpaperManager.class)
                    .getDesiredMinimumWidth());
        }
    }

    @Presubmit
    @Test
    public void testIncorrectContextUse_UiDerivedContext_NoViolation() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectIncorrectContextUse()
                        .penaltyLog()
                        .build());

        final Configuration config = new Configuration();
        config.setToDefaults();
        final Context uiDerivedConfigContext =
                createWindowContext().createConfigurationContext(config);

        assertNoViolation(() -> uiDerivedConfigContext.getSystemService(WINDOW_SERVICE));

        assertNoViolation(() -> ViewConfiguration.get(uiDerivedConfigContext));

        mInstrumentation.runOnMainSync(() -> {
            try {
                assertNoViolation(() ->
                        new GestureDetector(uiDerivedConfigContext, mGestureListener));
            } catch (Exception e) {
                fail("Failed because of " + e);
            }
        });

        if (isWallpaperSupported()) {
            assertNoViolation(() -> uiDerivedConfigContext.getSystemService(WallpaperManager.class)
                    .getDesiredMinimumWidth());
        }

        final Context uiDerivedAttrContext = createWindowContext()
                .createAttributionContext(null /* attributeTag */);

        assertNoViolation(() -> uiDerivedAttrContext.getSystemService(WINDOW_SERVICE));

        assertNoViolation(() -> ViewConfiguration.get(uiDerivedAttrContext));

        mInstrumentation.runOnMainSync(() -> {
            try {
                assertNoViolation(() ->
                        new GestureDetector(uiDerivedAttrContext, mGestureListener));
            } catch (Exception e) {
                fail("Failed because of " + e);
            }
        });

        if (isWallpaperSupported()) {
            assertNoViolation(() -> uiDerivedAttrContext.getSystemService(WallpaperManager.class)
                    .getDesiredMinimumWidth());
        }
    }

    @Presubmit
    @Test
    public void testIncorrectContextUse_UiDerivedDisplayContext_ThrowViolation() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectIncorrectContextUse()
                        .penaltyLog()
                        .build());

        final Display display = getContext().getSystemService(DisplayManager.class)
                .getDisplay(DEFAULT_DISPLAY);
        final Context uiDerivedDisplayContext = createWindowContext().createDisplayContext(display);

        assertViolation("Tried to access visual service " + WM_CLASS_NAME,
                () -> uiDerivedDisplayContext.getSystemService(WindowManager.class));

        assertViolation(
                "The API:ViewConfiguration needs a proper configuration.",
                () -> ViewConfiguration.get(uiDerivedDisplayContext));

        mInstrumentation.runOnMainSync(() -> {
            try {
                assertViolation("The API:GestureDetector#init needs a proper configuration.",
                        () -> new GestureDetector(uiDerivedDisplayContext, mGestureListener));
            } catch (Exception e) {
                fail("Failed because of " + e);
            }
        });

        if (isWallpaperSupported()) {
            assertViolation("Tried to access UI related API:", () ->
                    uiDerivedDisplayContext.getSystemService(WallpaperManager.class)
                            .getDesiredMinimumWidth());
        }
    }

    @Presubmit
    @Test
    public void testIncorrectContextUse_ConfigContext() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectIncorrectContextUse()
                        .penaltyLog()
                        .build());

        final Configuration configuration = new Configuration();
        configuration.setToDefaults();
        final Context configContext = getContext().createConfigurationContext(configuration);

        assertViolation("Tried to access visual service " + WM_CLASS_NAME,
                () -> configContext.getSystemService(WindowManager.class));

        assertNoViolation(() -> ViewConfiguration.get(configContext));

        mInstrumentation.runOnMainSync(() -> {
            try {
                assertNoViolation(() -> new GestureDetector(configContext, mGestureListener));
            } catch (Exception e) {
                fail("Failed because of " + e);
            }
        });

        if (isWallpaperSupported()) {
            assertViolation("Tried to access UI related API:", () ->
                    configContext.getSystemService(WallpaperManager.class)
                            .getDesiredMinimumWidth());
        }
    }

    @Presubmit
    @Test
    public void testIncorrectContextUse_ConfigDerivedDisplayContext() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectIncorrectContextUse()
                        .penaltyLog()
                        .build());

        final Display display = getContext().getSystemService(DisplayManager.class)
                .getDisplay(DEFAULT_DISPLAY);
        final Configuration configuration = new Configuration();
        configuration.setToDefaults();
        final Context configDerivedDisplayContext = getContext()
                .createConfigurationContext(configuration).createDisplayContext(display);

        assertViolation("Tried to access visual service " + WM_CLASS_NAME,
                () -> configDerivedDisplayContext.getSystemService(WindowManager.class));

        assertViolation(
                "The API:ViewConfiguration needs a proper configuration.",
                () -> ViewConfiguration.get(configDerivedDisplayContext));

        mInstrumentation.runOnMainSync(() -> {
            try {
                assertViolation("The API:GestureDetector#init needs a proper configuration.",
                        () -> new GestureDetector(configDerivedDisplayContext, mGestureListener));
            } catch (Exception e) {
                fail("Failed because of " + e);
            }
        });

        if (isWallpaperSupported()) {
            assertViolation("Tried to access UI related API:", () ->
                    configDerivedDisplayContext.getSystemService(WallpaperManager.class)
                            .getDesiredMinimumWidth());
        }
    }

    /**
     * Returns {@code true} to indicate that wallpaper is supported.
     * <p>
     * Note that we check the nullity of {@link WallpaperManager} because it may not be obtainable
     * if the test is targeting at least {@link android.os.Build.VERSION_CODES#P} and running in
     * instant mode.
     */
    private boolean isWallpaperSupported() {
        final WallpaperManager wallpaperManager = WallpaperManager.getInstance(getContext());
        return wallpaperManager != null && wallpaperManager.isWallpaperSupported();
    }

    @Test
    public void testUnsafeIntentLaunch_ParceledIntentToActivity_ThrowsViolation() throws Exception {
        // The UnsafeIntentLaunch StrictMode check is intended to detect and report unparceling and
        // launching of Intents from the delivered Intent. This test verifies a violation is
        // reported when an inner Intent is unparceled from the Intent delivered to an Activity and
        // used to start another Activity. This test also uses its own OnVmViolationListener to
        // obtain the actual StrictMode Violation to verify the getIntent method of the
        // UnsafeIntentLaunchViolation returns the Intent that triggered the Violation.
        final LinkedBlockingQueue<Violation> violations = new LinkedBlockingQueue<>();
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyListener(Executors.newSingleThreadExecutor(),
                                violation -> violations.add(violation))
                        .build());
        Context context = getContext();
        Intent intent = IntentLaunchActivity.getUnsafeIntentLaunchTestIntent(context);
        Intent innerIntent = intent.getParcelableExtra(IntentLaunchActivity.EXTRA_INNER_INTENT);

        context.startActivity(intent);
        Violation violation = violations.poll(5, TimeUnit.SECONDS);
        assertThat(violation).isInstanceOf(UnsafeIntentLaunchViolation.class);
        // The inner Intent will only have the target component set; since the Intent references
        // may not be the same compare the component of the Intent that triggered the violation
        // against the inner Intent obtained above.
        assertThat(((UnsafeIntentLaunchViolation) violation).getIntent().getComponent()).isEqualTo(
                innerIntent.getComponent());
    }

    @Test
    public void testUnsafeIntentLaunch_ParceledIntentToActivityCheckDisabled_NoViolation()
            throws Exception {
        // This test verifies the StrictMode violation is not reported when unsafe intent launching
        // is permitted through the VmPolicy Builder permit API.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .permitUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        Intent intent = IntentLaunchActivity.getUnsafeIntentLaunchTestIntent(context);

        assertNoViolation(() -> context.startActivity(intent));
    }

    @Test
    public void testUnsafeIntentLaunch_ParceledIntentToBoundService_ThrowsViolation()
            throws Exception {
        // This test verifies a violation is reported when an inner Intent is unparceled from the
        // Intent delivered to a bound Service and used to bind to another service.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        Intent intent = IntentLaunchService.getTestIntent(context);

        assertViolation(UNSAFE_INTENT_LAUNCH,
                () -> context.bindService(intent, IntentLaunchService.getServiceConnection(),
                        Context.BIND_AUTO_CREATE));
    }

    @Test
    public void testUnsafeIntentLaunch_ParceledIntentToStartedService_ThrowsViolation()
            throws Exception {
        // This test verifies a violation is reported when an inner Intent is unparceled from the
        // Intent delivered to a started Service and used to start another service.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        Intent intent = IntentLaunchService.getTestIntent(context);

        assertViolation(UNSAFE_INTENT_LAUNCH, () -> context.startService(intent));
    }

    @Test
    @AppModeFull(reason = "Instant apps can only declare runtime receivers")
    public void testUnsafeIntentLaunch_ParceledIntentToStaticReceiver_ThrowsViolation()
            throws Exception {
        // This test verifies a violation is reported when an inner Intent is unparceled from the
        // Intent delivered to a statically declared BroadcastReceiver and used to send another
        // broadcast.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        Intent intent = new Intent(context, IntentLaunchReceiver.class);
        Intent innerIntent = new Intent("android.os.cts.TEST_BROADCAST_ACTION");
        intent.putExtra(IntentLaunchReceiver.INNER_INTENT_KEY, innerIntent);

        assertViolation(UNSAFE_INTENT_LAUNCH, () -> context.sendBroadcast(intent));
    }

    @Test
    public void testUnsafeIntentLaunch_ParceledIntentToDynamicReceiver_ThrowsViolation()
            throws Exception {
        // This test verifies a violation is reported when an inner Intent is unparceled from the
        // Intent delivered to a dynamically registered BroadcastReceiver and used to send another
        // broadcast.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        String receiverAction = "android.os.cts.TEST_INTENT_LAUNCH_RECEIVER_ACTION";
        context.registerReceiver(new IntentLaunchReceiver(), new IntentFilter(receiverAction));
        Intent intent = new Intent(receiverAction);
        Intent innerIntent = new Intent("android.os.cts.TEST_BROADCAST_ACTION");
        intent.putExtra(IntentLaunchReceiver.INNER_INTENT_KEY, innerIntent);

        assertViolation(UNSAFE_INTENT_LAUNCH, () -> context.sendBroadcast(intent));
    }

    @Test
    public void testUnsafeIntentLaunch_ParceledIntentDataCopy_ThrowsViolation() throws Exception {
        // This test verifies a violation is reported when data is copied from a parceled Intent
        // without sanitation or validation to a new Intent that is being created to launch a new
        // component.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        Intent intent = IntentLaunchActivity.getUnsafeDataCopyFromIntentTestIntent(context);

        assertViolation(UNSAFE_INTENT_LAUNCH, () -> context.startActivity(intent));
    }

    @Test
    public void testUnsafeIntentLaunch_UnsafeDataCopy_ThrowsViolation() throws Exception {
        // This test verifies a violation is reported when data is copied from unparceled extras
        // without sanitation or validation to a new Intent that is being created to launch a new
        // component.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        Intent intent = IntentLaunchActivity.getUnsafeDataCopyFromExtrasTestIntent(context);

        assertViolation(UNSAFE_INTENT_LAUNCH, () -> context.startActivity(intent));
    }

    @Test
    public void testUnsafeIntentLaunch_DataCopyFromIntentDeliveredToProtectedComponent_NoViolation()
        throws Exception {
        // This test verifies a violation is not reported when data is copied from the Intent
        // delivered to a protected component.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        Intent intent =
                IntentLaunchActivity.getDataCopyFromDeliveredIntentWithUnparceledExtrasTestIntent(
                        context);

        assertNoViolation(() -> context.startActivity(intent));
    }

    @Test
    public void testUnsafeIntentLaunch_UnsafeIntentFromUriLaunch_ThrowsViolation()
            throws Exception {
        // Intents can also be delivered as URI strings and parsed with Intent#parseUri. This test
        // verifies if an Intent is parsed from a URI string and launched without any additional
        // sanitation / validation then a violation is reported.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        Intent intent =
                IntentLaunchActivity.getUnsafeIntentFromUriLaunchTestIntent(context);

        assertViolation(UNSAFE_INTENT_LAUNCH, () -> context.startActivity(intent));
    }

    @Test
    public void testUnsafeIntentLaunch_SafeIntentFromUriLaunch_NoViolation() throws Exception {
        // The documentation for Intent#URI_ALLOW_UNSAFE recommend using the CATEGORY_BROWSABLE
        // when launching an Intent parsed from a URI; while an explicit Intent will still be
        // delivered to the target component with this category set an implicit Intent will be
        // limited to components with Intent-filters that handle this category. This test verifies
        // an implicit Intent parsed from a URI with the browsable category set does not result in
        // an UnsafeIntentLaunch StrictMode violation.
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectUnsafeIntentLaunch()
                        .penaltyLog()
                        .build());
        Context context = getContext();
        Intent intent =
                IntentLaunchActivity.getSafeIntentFromUriLaunchTestIntent(context);

        assertNoViolation(() -> context.startActivity(intent));
    }

    private Context createWindowContext() {
        final Display display = getContext().getSystemService(DisplayManager.class)
                .getDisplay(DEFAULT_DISPLAY);
        return getContext().createDisplayContext(display)
                .createWindowContext(TYPE_APPLICATION_OVERLAY, null /* options */);
    }

    private static void runWithRemoteServiceBound(Context context, Consumer<ISecondary> consumer)
            throws ExecutionException, InterruptedException, RemoteException {
        BlockingQueue<IBinder> binderHolder = new ArrayBlockingQueue<>(1);
        ServiceConnection secondaryConnection =
                new ServiceConnection() {
                    public void onServiceConnected(ComponentName className, IBinder service) {
                        binderHolder.add(service);
                    }

                    public void onServiceDisconnected(ComponentName className) {
                        binderHolder.drainTo(new ArrayList<>());
                    }
                };
        Intent intent = new Intent(REMOTE_SERVICE_ACTION);
        intent.setPackage(context.getPackageName());

        Intent secondaryIntent = new Intent(ISecondary.class.getName());
        secondaryIntent.setPackage(context.getPackageName());
        assertThat(
                        context.bindService(
                                secondaryIntent, secondaryConnection, Context.BIND_AUTO_CREATE))
                .isTrue();
        IBinder binder = binderHolder.take();
        assertThat(binder.queryLocalInterface(binder.getInterfaceDescriptor())).isNull();
        consumer.accept(ISecondary.Stub.asInterface(binder));
        context.unbindService(secondaryConnection);
        context.stopService(intent);
    }

    private static void assertViolation(String expected, ThrowingRunnable r) throws Exception {
        inspectViolation(r, info -> assertThat(info.getStackTrace()).contains(expected));
    }

    private static void assertNoViolation(ThrowingRunnable r) throws Exception {
        inspectViolation(
                r, info -> assertWithMessage("Unexpected violation").that(info).isNull());
    }

    private static void inspectViolation(
            ThrowingRunnable violating, Consumer<ViolationInfo> consume) throws Exception {
        final LinkedBlockingQueue<ViolationInfo> violations = new LinkedBlockingQueue<>();
        StrictMode.setViolationLogger(violations::add);

        try {
            violating.run();
            consume.accept(violations.poll(5, TimeUnit.SECONDS));
        } finally {
            StrictMode.setViolationLogger(null);
        }
    }

    private boolean hasInternetConnection() {
        final PackageManager pm = getContext().getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)
                || pm.hasSystemFeature(PackageManager.FEATURE_WIFI)
                || pm.hasSystemFeature(PackageManager.FEATURE_ETHERNET);
    }
}

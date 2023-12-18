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

package com.google.android.car.kitchensink.watchdog;

import android.app.AlertDialog;
import android.car.watchdog.CarWatchdogManager;
import android.car.watchdog.IoOveruseStats;
import android.car.watchdog.ResourceOveruseStats;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.FileUtils;
import android.os.Handler;
import android.os.SystemClock;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.android.internal.annotations.GuardedBy;

import com.google.android.car.kitchensink.KitchenSinkActivity;
import com.google.android.car.kitchensink.R;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.nio.file.Files;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Fragment to test the I/O monitoring of Car Watchdog.
 *
 * <p>Before running the tests, start a custom performance collection, this enables the watchdog
 * daemon to read proc stats more frequently and reduces the test wait time. Then run the dumpsys
 * command to reset I/O overuse counters in the adb shell, which clears any previous stats saved by
 * watchdog. After the test is finished, stop the custom performance collection, this resets
 * watchdog's I/O stat collection to the default interval.
 *
 * <p>Commands:
 *
 * <p>adb shell dumpsys android.automotive.watchdog.ICarWatchdog/default --start_perf \
 * --max_duration 600 --interval 1
 *
 * <p>adb shell dumpsys android.automotive.watchdog.ICarWatchdog/default \
 * --reset_resource_overuse_stats shared:com.google.android.car.uid.kitchensink
 *
 * <p>adb shell dumpsys android.automotive.watchdog.ICarWatchdog/default --stop_perf /dev/null
 */
public class CarWatchdogTestFragment extends Fragment {
    private static final long TEN_MEGABYTES = 1024 * 1024 * 10;
    private static final int DISK_DELAY_MS = 3000;
    private static final String TAG = "CarWatchdogTestFragment";
    private static final double EXCEED_WARN_THRESHOLD_PERCENT = 0.9;

    private final ExecutorService mExecutor = Executors.newSingleThreadExecutor();
    private Context mContext;
    private CarWatchdogManager mCarWatchdogManager;
    private KitchenSinkActivity mActivity;
    private File mTestDir;
    private TextView mOveruseTextView;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        mContext = getContext();
        mActivity = (KitchenSinkActivity) getActivity();
        mActivity.requestRefreshManager(
                () -> {
                    mCarWatchdogManager = mActivity.getCarWatchdogManager();
                },
                new Handler(mContext.getMainLooper()));
        super.onCreate(savedInstanceState);
    }

    @Nullable
    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.car_watchdog_test, container, false);

        mOveruseTextView = view.findViewById(R.id.io_overuse_textview);
        Button overuseWarningBtn = view.findViewById(R.id.io_overuse_warning_btn);
        Button overuseKillingBtn = view.findViewById(R.id.io_overuse_killing_btn);

        try {
            mTestDir =
                    Files.createTempDirectory(mActivity.getFilesDir().toPath(), "testDir").toFile();
        } catch (IOException e) {
            e.printStackTrace();
            mActivity.finish();
        }

        overuseWarningBtn.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        mExecutor.execute(
                                () -> {
                                    IoOveruseListener listener = addResourceOveruseListener();

                                    if (!writeToDisk(TEN_MEGABYTES)) {
                                        mCarWatchdogManager.removeResourceOveruseListener(listener);
                                        return;
                                    }

                                    long remainingBytes = fetchRemainingBytes(TEN_MEGABYTES);
                                    if (remainingBytes == 0) {
                                        mCarWatchdogManager.removeResourceOveruseListener(listener);
                                        return;
                                    }

                                    long bytesToExceedWarnThreshold =
                                            (long) Math.ceil(remainingBytes
                                                    * EXCEED_WARN_THRESHOLD_PERCENT);

                                    listener.setExpectedMinWrittenBytes(
                                            TEN_MEGABYTES + bytesToExceedWarnThreshold);

                                    if (!writeToDisk(bytesToExceedWarnThreshold)) {
                                        mCarWatchdogManager.removeResourceOveruseListener(listener);
                                        return;
                                    }

                                    listener.checkIsNotified();

                                    mCarWatchdogManager.removeResourceOveruseListener(listener);
                                    Log.d(TAG, "Test finished.");
                                });
                    }
                });

        overuseKillingBtn.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        // TODO(b/185807690): Implement test
                    }
                });

        return view;
    }

    @Override
    public void onDestroyView() {
        FileUtils.deleteContentsAndDir(mTestDir);
        super.onDestroyView();
    }

    private long fetchRemainingBytes(long minWrittenBytes) {
        ResourceOveruseStats stats =
                mCarWatchdogManager.getResourceOveruseStats(
                        CarWatchdogManager.FLAG_RESOURCE_OVERUSE_IO,
                        CarWatchdogManager.STATS_PERIOD_CURRENT_DAY);

        IoOveruseStats ioOveruseStats = stats.getIoOveruseStats();
        if (ioOveruseStats == null) {
            showErrorAlert(
                    "No I/O overuse stats available for the application after writing "
                            + minWrittenBytes
                            + " bytes.");
            return 0;
        }
        if (ioOveruseStats.getTotalBytesWritten() < minWrittenBytes) {
            showErrorAlert(
                    "Actual written bytes to disk '"
                            + minWrittenBytes
                            + "' don't match written bytes '"
                            + ioOveruseStats.getTotalBytesWritten()
                            + "' returned by get request");
            return 0;
        }
        /*
         * Check for foreground mode bytes given kitchensink app is running in the foreground
         * during manual testing.
         */
        return ioOveruseStats.getRemainingWriteBytes().getForegroundModeBytes();
    }

    private IoOveruseListener addResourceOveruseListener() {
        IoOveruseListener listener = new IoOveruseListener();
        mCarWatchdogManager.addResourceOveruseListener(
                mActivity.getMainExecutor(), CarWatchdogManager.FLAG_RESOURCE_OVERUSE_IO, listener);
        return listener;
    }

    private void showErrorAlert(String message) {
        showAlert("Error", message, android.R.drawable.ic_dialog_alert);
    }

    private void showAlert(String title, String message, int iconDrawable) {
        mActivity.runOnUiThread(
                () -> {
                    new AlertDialog.Builder(mContext)
                            .setTitle(title)
                            .setMessage(message)
                            .setPositiveButton(
                                    android.R.string.yes,
                                    new DialogInterface.OnClickListener() {
                                        public void onClick(DialogInterface dialog, int which) {
                                            // do nothing
                                        }
                                    })
                            .setIcon(iconDrawable)
                            .show();
                });
    }

    private void onDiskWrite(long writtenBytes, long pendingBytes) {
        mActivity.runOnUiThread(() -> {
            if (pendingBytes != 0) {
                mOveruseTextView.setText("Writing to disk... (" + writtenBytes + " / "
                        + (writtenBytes + pendingBytes) + ") bytes");
            } else {
                mOveruseTextView.setText("Disk write completed!");
            }
        });
    }

    private boolean writeToDisk(long bytes) {
        long writtenBytes = 0;
        File uniqueFile = new File(mTestDir, Long.toString(System.nanoTime()));
        try (FileOutputStream fos = new FileOutputStream(uniqueFile)) {
            Log.d(TAG, "Attempting to write " + bytes + " bytes");
            writtenBytes = writeToFos(fos, bytes);
            if (writtenBytes < bytes) {
                showErrorAlert("Failed to write '" + bytes + "' bytes to disk. '" + writtenBytes
                                + "' bytes were successfully written, while '"
                                + (bytes - writtenBytes)
                                + "' bytes were pending at the moment the exception occurred.");
                return false;
            }
            fos.getFD().sync();
            // Wait for the IO event to propagate to the system
            Thread.sleep(DISK_DELAY_MS);
            return true;
        } catch (IOException | InterruptedException e) {
            String reason = e instanceof IOException ? "I/O exception" : "Thread interrupted";
            showErrorAlert(
                    reason + " after successfully writing to disk.\n\n" + e.getMessage());
            return false;
        }
    }

    private long writeToFos(FileOutputStream fos, long maxSize) {
        long writtenSize = 0;
        while (maxSize != 0) {
            int writeSize =
                    (int) Math.min(Integer.MAX_VALUE,
                                    Math.min(Runtime.getRuntime().freeMemory(), maxSize));
            try {
                fos.write(new byte[writeSize]);
            } catch (InterruptedIOException e) {
                Thread.currentThread().interrupt();
                continue;
            } catch (IOException e) {
                e.printStackTrace();
                return writtenSize;
            }
            writtenSize += writeSize;
            maxSize -= writeSize;
            if (writeSize > 0) {
                Log.i(TAG, "writeSize:" + writeSize);
                onDiskWrite(writtenSize, maxSize);
            }
        }
        Log.i(TAG, "Write completed.");
        return writtenSize;
    }

    private final class IoOveruseListener
            implements CarWatchdogManager.ResourceOveruseListener {
        private static final int NOTIFICATION_DELAY_MS = 10000;

        private final Object mLock = new Object();
        @GuardedBy("mLock")
        private boolean mNotificationReceived;

        private long mExpectedMinWrittenBytes;

        @Override
        public void onOveruse(@NonNull ResourceOveruseStats resourceOveruseStats) {
            synchronized (mLock) {
                mNotificationReceived = true;
                mLock.notifyAll();
            }
            Log.d(TAG, resourceOveruseStats.toString());
            if (resourceOveruseStats.getIoOveruseStats() == null) {
                showErrorAlert(
                        "No I/O overuse stats reported for the application in the overuse"
                                + " notification.");
                return;
            }
            long reportedWrittenBytes =
                    resourceOveruseStats.getIoOveruseStats().getTotalBytesWritten();
            if (reportedWrittenBytes < mExpectedMinWrittenBytes) {
                showErrorAlert(
                        "Actual written bytes to disk '"
                                + mExpectedMinWrittenBytes
                                + "' don't match written bytes '"
                                + reportedWrittenBytes
                                + "' reported in overuse notification");
                return;
            }
            showAlert("I/O Overuse", "Overuse notification received!", 0);
        }

        public void setExpectedMinWrittenBytes(long expectedMinWrittenBytes) {
            mExpectedMinWrittenBytes = expectedMinWrittenBytes;
        }

        private void checkIsNotified() {
            synchronized (mLock) {
                long now = SystemClock.uptimeMillis();
                long deadline = now + NOTIFICATION_DELAY_MS;
                while (!mNotificationReceived && now < deadline) {
                    try {
                        mLock.wait(deadline - now);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        continue;
                    } finally {
                        now = SystemClock.uptimeMillis();
                    }
                    break;
                }
                if (!mNotificationReceived) {
                    showErrorAlert("I/O Overuse notification not received.");
                }
            }
        }
    }
}

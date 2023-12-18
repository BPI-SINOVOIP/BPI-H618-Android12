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

package android.assist.service;

import android.app.assist.AssistContent;
import android.app.assist.AssistStructure;
import android.assist.common.Utils;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Point;
import android.os.Bundle;
import android.os.RemoteCallback;
import android.service.voice.VoiceInteractionSession;
import android.util.Log;
import android.view.Display;
import android.view.DisplayCutout;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewTreeObserver;

public class MainInteractionSession extends VoiceInteractionSession {
    static final String TAG = "MainInteractionSession";

    Context mContext;
    Bundle mAssistData = new Bundle();

    private boolean hasReceivedAssistData = false;
    private boolean hasReceivedScreenshot = false;
    private boolean mScreenshotNeeded = true;
    private int mCurColor;
    private int mDisplayHeight;
    private int mDisplayWidth;
    private BroadcastReceiver mReceiver;
    private String mTestName;
    private View mContentView;
    private RemoteCallback mRemoteCallback;

    MainInteractionSession(Context context) {
        super(context);
        mContext = context;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (action.equals(Utils.HIDE_SESSION)) {
                    hide();
                }

                Bundle bundle = new Bundle();
                bundle.putString(Utils.EXTRA_REMOTE_CALLBACK_ACTION, Utils.HIDE_SESSION_COMPLETE);
                mRemoteCallback.sendResult(bundle);
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(Utils.HIDE_SESSION);
        mContext.registerReceiver(mReceiver, filter, Context.RECEIVER_VISIBLE_TO_INSTANT_APPS);
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestroy()");
        super.onDestroy();
        if (mReceiver != null) {
            try {
                mContext.unregisterReceiver(mReceiver);
            } catch (IllegalArgumentException e) {
                // Ignore this exception when unregisterReceiver fails. Due to there will be timing
                // case to destroy VoiceInteractionSessionService before VoiceInteractionSession.
                Log.e(TAG, "Failed to unregister receiver in onDestroy.", e);
            }
        }
    }

    @Override
    public void onPrepareShow(Bundle args, int showFlags) {
        if (Utils.LIFECYCLE_NOUI.equals(args.getString(Utils.TESTCASE_TYPE, ""))) {
            setUiEnabled(false);
        } else  {
            setUiEnabled(true);
        }
    }

    @Override
    public void onShow(Bundle args, int showFlags) {
        if (args == null) {
            Log.e(TAG, "onshow() received null args");
            return;
        }
        mScreenshotNeeded = (showFlags & SHOW_WITH_SCREENSHOT) != 0;
        mTestName = args.getString(Utils.TESTCASE_TYPE, "");
        mCurColor = args.getInt(Utils.SCREENSHOT_COLOR_KEY);
        mDisplayHeight = args.getInt(Utils.DISPLAY_HEIGHT_KEY);
        mDisplayWidth = args.getInt(Utils.DISPLAY_WIDTH_KEY);
        mRemoteCallback = args.getParcelable(Utils.EXTRA_REMOTE_CALLBACK);
        super.onShow(args, showFlags);
        if (mContentView == null) return; // Happens when ui is not enabled.
        mContentView.getViewTreeObserver().addOnPreDrawListener(
                new ViewTreeObserver.OnPreDrawListener() {
                @Override
                public boolean onPreDraw() {
                    mContentView.getViewTreeObserver().removeOnPreDrawListener(this);
                    Display d = mContentView.getDisplay();
                    Point displayPoint = new Point();
                    d.getRealSize(displayPoint);
                    DisplayCutout dc = d.getCutout();
                    if (dc != null) {
                        // Means the device has a cutout area
                        android.graphics.Insets wi = d.getCutout().getWaterfallInsets();

                        if (wi != android.graphics.Insets.NONE) {
                            // Waterfall cutout. Considers only the display
                            // useful area discarding the cutout.
                            displayPoint.x -= (wi.left + wi.right);
                        }
                    }
                    Bundle bundle = new Bundle();
                    bundle.putString(Utils.EXTRA_REMOTE_CALLBACK_ACTION, Utils.BROADCAST_CONTENT_VIEW_HEIGHT);
                    bundle.putInt(Utils.EXTRA_CONTENT_VIEW_HEIGHT, mContentView.getHeight());
                    bundle.putInt(Utils.EXTRA_CONTENT_VIEW_WIDTH, mContentView.getWidth());
                    bundle.putParcelable(Utils.EXTRA_DISPLAY_POINT, displayPoint);
                    mRemoteCallback.sendResult(bundle);
                    return true;
                }
            });
    }

    @Override
    public void onHandleAssist(AssistState state) {
        super.onHandleAssist(state);
        Bundle data = state.getAssistData();
        AssistStructure structure = state.getAssistStructure();
        AssistContent content = state.getAssistContent();
        ComponentName activity = structure == null ? null : structure.getActivityComponent();
        Log.i(TAG, "onHandleAssist()");
        Log.i(TAG, String.format("Bundle: %s, Activity: %s, Structure: %s, Content: %s",
                data, activity, structure, content));

        if (activity != null && Utils.isAutomotive(mContext)
                && !activity.getPackageName().equals("android.assist.testapp")) {
            // TODO: automotive has multiple activities / displays, so the test might fail if it
            // receives one of them (like the cluster activity) instead of what's expecting. This is
            // a quick fix for the issue; a better solution would be refactoring the infra to
            // either send all events, or let the test specifify which activity it's waiting for
            Log.i(TAG, "Ignoring " + activity.flattenToShortString() + " on automotive");
            return;
        }

        // send to test to verify that this is accurate.
        mAssistData.putBoolean(Utils.ASSIST_IS_ACTIVITY_ID_NULL, state.getActivityId() == null);
        mAssistData.putParcelable(Utils.ASSIST_STRUCTURE_KEY, structure);
        mAssistData.putParcelable(Utils.ASSIST_CONTENT_KEY, content);
        mAssistData.putBundle(Utils.ASSIST_BUNDLE_KEY, data);
        hasReceivedAssistData = true;
        maybeBroadcastResults();
    }

    @Override
    public void onAssistStructureFailure(Throwable failure) {
        Log.e(TAG, "onAssistStructureFailure(): D'OH!!!", failure);
    }

    @Override
    public void onHandleScreenshot(/*@Nullable*/ Bitmap screenshot) {
        Log.i(TAG, String.format("onHandleScreenshot - Screenshot: %s", screenshot));
        super.onHandleScreenshot(screenshot);

        if (screenshot != null) {
            mAssistData.putBoolean(Utils.ASSIST_SCREENSHOT_KEY, true);

            if (mTestName.equals(Utils.SCREENSHOT)) {
                boolean screenshotMatches = compareScreenshot(screenshot, mCurColor);
                Log.i(TAG, "this is a screenshot test. Matches? " + screenshotMatches);
                mAssistData.putBoolean(
                    Utils.COMPARE_SCREENSHOT_KEY, screenshotMatches);
            }
        } else {
            mAssistData.putBoolean(Utils.ASSIST_SCREENSHOT_KEY, false);
        }
        hasReceivedScreenshot = true;
        maybeBroadcastResults();
    }

    private boolean compareScreenshot(Bitmap screenshot, int color) {
        Point size = new Point(mDisplayWidth, mDisplayHeight);

        if (screenshot.getWidth() != size.x || screenshot.getHeight() != size.y) {
            Log.i(TAG, "width  or height didn't match: " + size + " vs " + screenshot.getWidth()
                    + "," + screenshot.getHeight());
            return false;
        }
        int[] pixels = new int[size.x * size.y];
        screenshot.getPixels(pixels, 0, size.x, 0, 0, size.x, size.y);

        int expectedColor = 0;
        for (int pixel : pixels) {
            // Check for roughly the same because there are rounding errors converting from the
            // screenshot's color space to SRGB, which is what getPixels does.
            if ((Color.red(pixel) - Color.red(color) < 5)
                    && (Color.green(pixel) - Color.green(color) < 5)
                    && (Color.blue(pixel) - Color.blue(color) < 5)) {
                expectedColor += 1;
            }
        }

        int pixelCount = screenshot.getWidth() * screenshot.getHeight();
        double colorRatio = (double) expectedColor / pixelCount;
        Log.i(TAG, "the ratio is " + colorRatio);
        return colorRatio >= 0.6;
    }

    private void maybeBroadcastResults() {
        if (!hasReceivedAssistData) {
            Log.i(TAG, "waiting for assist data before broadcasting results");
        } else if (mScreenshotNeeded && !hasReceivedScreenshot) {
            Log.i(TAG, "waiting for screenshot before broadcasting results");
        } else {
            Bundle bundle = new Bundle();
            bundle.putString(Utils.EXTRA_REMOTE_CALLBACK_ACTION, Utils.BROADCAST_ASSIST_DATA_INTENT);
            bundle.putAll(mAssistData);
            mRemoteCallback.sendResult(bundle);

            hasReceivedAssistData = false;
            hasReceivedScreenshot = false;
        }
    }

    @Override
    public View onCreateContentView() {
        LayoutInflater f = getLayoutInflater();
        if (f == null) {
            Log.wtf(TAG, "layout inflater was null");
        }
        mContentView = f.inflate(R.layout.assist_layer,null);
        Log.i(TAG, "onCreateContentView");
        return mContentView;
    }
}

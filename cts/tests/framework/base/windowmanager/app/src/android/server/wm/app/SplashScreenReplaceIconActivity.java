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

package android.server.wm.app;

import static android.server.wm.app.Components.TestStartingWindowKeys.CANCEL_HANDLE_EXIT;
import static android.server.wm.app.Components.TestStartingWindowKeys.CENTER_VIEW_IS_SURFACE_VIEW;
import static android.server.wm.app.Components.TestStartingWindowKeys.CONTAINS_CENTER_VIEW;
import static android.server.wm.app.Components.TestStartingWindowKeys.DELAY_RESUME;
import static android.server.wm.app.Components.TestStartingWindowKeys.ICON_ANIMATION_DURATION;
import static android.server.wm.app.Components.TestStartingWindowKeys.ICON_ANIMATION_START;
import static android.server.wm.app.Components.TestStartingWindowKeys.RECEIVE_SPLASH_SCREEN_EXIT;
import static android.server.wm.app.Components.TestStartingWindowKeys.REPLACE_ICON_EXIT;
import static android.server.wm.app.Components.TestStartingWindowKeys.REQUEST_HANDLE_EXIT_ON_CREATE;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.SystemClock;
import android.server.wm.TestJournalProvider;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.window.SplashScreen;
import android.window.SplashScreenView;

public class SplashScreenReplaceIconActivity extends Activity {
    private SplashScreen mSSM;
    private final SplashScreen.OnExitAnimationListener mSplashScreenExitHandler=
            this::onSplashScreenExit;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mSSM = getSplashScreen();
        if (getIntent().getBooleanExtra(REQUEST_HANDLE_EXIT_ON_CREATE, false)) {
            mSSM.setOnExitAnimationListener(mSplashScreenExitHandler);
            SystemClock.sleep(500);
        }
        if (getIntent().getBooleanExtra(DELAY_RESUME, false)) {
            SystemClock.sleep(5000);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        if (getIntent().getBooleanExtra(CANCEL_HANDLE_EXIT, false)) {
            mSSM.clearOnExitAnimationListener();
        }
    }

    private void onSplashScreenExit(SplashScreenView view) {
        final Context baseContext = getBaseContext();
        final View centerView = view.getIconView();
        final boolean isSurfaceView = view.getIconView() instanceof SurfaceView;
        TestJournalProvider.putExtras(baseContext, REPLACE_ICON_EXIT, bundle -> {
            bundle.putBoolean(RECEIVE_SPLASH_SCREEN_EXIT, true);
            bundle.putBoolean(CONTAINS_CENTER_VIEW, centerView != null);
            bundle.putLong(ICON_ANIMATION_DURATION, view.getIconAnimationDuration() != null
                    ? view.getIconAnimationDuration().toMillis() : 0);
            bundle.putLong(ICON_ANIMATION_START, view.getIconAnimationStart() != null
                    ? view.getIconAnimationStart().toEpochMilli() : 0);
            bundle.putBoolean(CENTER_VIEW_IS_SURFACE_VIEW, isSurfaceView);
        });
        view.remove();
    }
}

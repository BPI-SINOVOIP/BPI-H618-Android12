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
 * limitations under the License
 */

package android.server.wm.app;

import static android.server.wm.app.Components.BlurActivity.EXTRA_NO_BLUR_BACKGROUND_COLOR;
import static android.server.wm.app.Components.BlurActivity.EXTRA_BACKGROUND_BLUR_RADIUS_PX;
import static android.server.wm.app.Components.BlurActivity.EXTRA_BLUR_BEHIND_RADIUS_PX;
import static android.view.WindowInsets.Type.systemBars;

import android.app.Activity;
import android.os.Bundle;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.server.wm.app.BroadcastReceiverActivity;
import android.view.WindowManager;
import android.view.Window;

import java.util.function.Consumer;

public class BlurActivity extends BroadcastReceiverActivity {
    private int mNoBlurBackgroundColor;
    private int mBackgroundBlurRadius;
    private int mBlurBehindRadius;

    private Consumer<Boolean> mCrossWindowBlurEnabledListener = enabled -> {
        final Window window = getWindow();
        if (enabled) {
            if (mBackgroundBlurRadius > 0) {
                window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
                window.setBackgroundBlurRadius(mBackgroundBlurRadius);
            } else {
                window.setBackgroundDrawable(new ColorDrawable(mNoBlurBackgroundColor));
            }
            window.getAttributes().setBlurBehindRadius(mBlurBehindRadius);
        } else {
            window.setBackgroundDrawable(new ColorDrawable(mNoBlurBackgroundColor));
            window.getAttributes().setBlurBehindRadius(0);
        }
        getWindowManager().updateViewLayout(window.getDecorView(), window.getAttributes());

    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.blur_activity);
        getWindow().setDecorFitsSystemWindows(false);
        getWindow().getInsetsController().hide(systemBars());

        mBlurBehindRadius = getIntent().getIntExtra(EXTRA_BLUR_BEHIND_RADIUS_PX, 0);
        mBackgroundBlurRadius = getIntent().getIntExtra(EXTRA_BACKGROUND_BLUR_RADIUS_PX, 0);
        mNoBlurBackgroundColor =
                getIntent().getIntExtra(EXTRA_NO_BLUR_BACKGROUND_COLOR, Color.GREEN);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        getWindowManager().addCrossWindowBlurEnabledListener(mCrossWindowBlurEnabledListener);
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        getWindowManager().removeCrossWindowBlurEnabledListener(mCrossWindowBlurEnabledListener);
    }
}

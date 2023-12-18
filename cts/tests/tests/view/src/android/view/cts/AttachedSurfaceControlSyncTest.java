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
package android.view.cts;

import static android.server.wm.WindowManagerState.getLogicalDisplaySize;

import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.Region;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.view.Gravity;
import android.view.SurfaceControl;
import android.view.SurfaceView;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.animation.LinearInterpolator;
import android.view.cts.surfacevalidator.AnimationFactory;
import android.view.cts.surfacevalidator.AnimationTestCase;
import android.view.cts.surfacevalidator.CapturedActivityWithResource;
import android.view.cts.surfacevalidator.PixelChecker;
import android.view.cts.surfacevalidator.ViewFactory;
import android.widget.FrameLayout;

import android.util.Log;

import androidx.test.filters.LargeTest;
import androidx.test.filters.RequiresDevice;
import androidx.test.rule.ActivityTestRule;
import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@LargeTest
@SuppressLint("RtlHardcoded")
@RequiresDevice
public class AttachedSurfaceControlSyncTest {
    private static final String TAG = "AttachedSurfaceControlSyncTests";

    @Rule
    public ActivityTestRule<CapturedActivityWithResource> mActivityRule =
            new ActivityTestRule<>(CapturedActivityWithResource.class);

    @Rule
    public TestName mName = new TestName();

    private CapturedActivityWithResource mActivity;

      private static ValueAnimator makeInfinite(ValueAnimator a) {
        a.setRepeatMode(ObjectAnimator.REVERSE);
        a.setRepeatCount(ObjectAnimator.INFINITE);
        a.setDuration(200);
        a.setInterpolator(new LinearInterpolator());
        return a;
    }

    class GreenSurfaceAnchorView extends View {
        SurfaceControl mSurfaceControl;
        final Surface mSurface;
        final int[] mLocation = new int[2];

        private final ViewTreeObserver.OnPreDrawListener mDrawListener = () -> {
            SurfaceControl.Transaction t = new SurfaceControl.Transaction();
            getLocationInWindow(mLocation);
            t.setGeometry(mSurfaceControl, null, new Rect(mLocation[0], mLocation[1],
                    mLocation[0]+100,
                    mLocation[1]+100), 0);
            getRootSurfaceControl().applyTransactionOnDraw(t);
            return true;
        };

        GreenSurfaceAnchorView(Context c) {
            super(c, null, 0, 0);
            mSurfaceControl = new SurfaceControl.Builder()
                              .setName("SurfaceAnchorView")
                              .setBufferSize(100, 100)
                              .build();
            mSurface = new Surface(mSurfaceControl);
            Canvas canvas = mSurface.lockHardwareCanvas();
            canvas.drawColor(Color.GREEN);
            mSurface.unlockCanvasAndPost(canvas);
        }

        @Override
        public boolean gatherTransparentRegion(Region region) {
            boolean opaque = true;
            int w = getWidth();
            int h = getHeight();
            if (w>0 && h>0) {
                getLocationInWindow(mLocation);
                int l = mLocation[0];
                int t = mLocation[1];
                region.op(l, t, l+w, t+h, Region.Op.UNION);
            }
            return false;
        }

        @Override
        protected void onAttachedToWindow() {
            super.onAttachedToWindow();
            SurfaceControl.Transaction t =
                getRootSurfaceControl().buildReparentTransaction(mSurfaceControl);
            t.setLayer(mSurfaceControl, -1)
                .setVisibility(mSurfaceControl, true)
                .apply();

            ViewTreeObserver observer = getViewTreeObserver();
            observer.addOnPreDrawListener(mDrawListener);

            getParent().requestTransparentRegion(this);
        }

        @Override
        protected void onDetachedFromWindow() {
            ViewTreeObserver observer = getViewTreeObserver();
            observer.removeOnPreDrawListener(mDrawListener);

            new SurfaceControl.Transaction().reparent(mSurfaceControl, null).apply();
            mSurfaceControl.release();
            mSurface.release();

            super.onDetachedFromWindow();
        }

        @Override
        public void onDraw(Canvas canvas) {
            canvas.drawColor(0, PorterDuff.Mode.CLEAR);
        }
    }

    private ViewFactory sGreenSurfaceControlAnchorFactory = context -> {
        return new GreenSurfaceAnchorView(context);
    };

    private AnimationFactory sTranslateAnimationFactory = view -> {
        PropertyValuesHolder pvhX = PropertyValuesHolder.ofFloat(View.TRANSLATION_X, 10f, 30f);
        PropertyValuesHolder pvhY = PropertyValuesHolder.ofFloat(View.TRANSLATION_Y, 10f, 30f);
        return makeInfinite(ObjectAnimator.ofPropertyValuesHolder(view, pvhX, pvhY));
    };

    @Before
    public void setup() {
        mActivity = mActivityRule.getActivity();
        mActivity.setLogicalDisplaySize(getLogicalDisplaySize());
    }

    /**
     * Want to be especially sure we don't leave up the permission dialog, so try and dismiss
     * after test.
     */
    @After
    public void tearDown() throws UiObjectNotFoundException {
        mActivity.dismissPermissionDialog();
    }

    /** Draws a moving 10x10 green rectangle with hole punch, make sure we don't get any sync errors */
    @Test
    public void testSync() throws Throwable {
        mActivity.verifyTest(new AnimationTestCase(
                sGreenSurfaceControlAnchorFactory,
                new FrameLayout.LayoutParams(100, 100, Gravity.LEFT | Gravity.TOP),
                sTranslateAnimationFactory,
                new PixelChecker() {
                    @Override
                    public boolean checkPixels(int blackishPixelCount, int width, int height) {
                        return blackishPixelCount == 0;
                    }
                }), mName);
    }
}

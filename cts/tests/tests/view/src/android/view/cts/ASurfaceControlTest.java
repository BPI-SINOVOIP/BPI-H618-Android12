/*
 * Copyright 2018 The Android Open Source Project
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

import static android.server.wm.ActivityManagerTestBase.createFullscreenActivityScenarioRule;
import static android.view.cts.surfacevalidator.ASurfaceControlTestActivity.MultiRectChecker;
import static android.view.cts.util.ASurfaceControlTestUtils.applyAndDeleteSurfaceTransaction;
import static android.view.cts.util.ASurfaceControlTestUtils.createSurfaceTransaction;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceControl_acquire;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceControl_create;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceControl_createFromWindow;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceControl_release;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_apply;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_checkOnComplete;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_create;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_delete;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_releaseBuffer;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setDamageRegion;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setDesiredPresentTime;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setOnCommitCallback;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setOnCommitCallbackWithoutContext;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setOnComplete;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setOnCompleteCallback;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setOnCompleteCallbackWithoutContext;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setPosition;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setQuadrantBuffer;
import static android.view.cts.util.ASurfaceControlTestUtils.nSurfaceTransaction_setSolidBuffer;
import static android.view.cts.util.ASurfaceControlTestUtils.reparent;
import static android.view.cts.util.ASurfaceControlTestUtils.setBufferAlpha;
import static android.view.cts.util.ASurfaceControlTestUtils.setBufferOpaque;
import static android.view.cts.util.ASurfaceControlTestUtils.setBufferTransform;
import static android.view.cts.util.ASurfaceControlTestUtils.setColor;
import static android.view.cts.util.ASurfaceControlTestUtils.setCrop;
import static android.view.cts.util.ASurfaceControlTestUtils.setGeometry;
import static android.view.cts.util.ASurfaceControlTestUtils.setPosition;
import static android.view.cts.util.ASurfaceControlTestUtils.setScale;
import static android.view.cts.util.ASurfaceControlTestUtils.setVisibility;
import static android.view.cts.util.ASurfaceControlTestUtils.setZOrder;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.SystemClock;
import android.platform.test.annotations.RequiresDevice;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.cts.surfacevalidator.ASurfaceControlTestActivity;
import android.view.cts.surfacevalidator.ASurfaceControlTestActivity.PixelChecker;
import android.view.cts.surfacevalidator.PixelColor;
import android.view.cts.util.ASurfaceControlTestUtils;

import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.junit.runner.RunWith;

import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@LargeTest
@RunWith(AndroidJUnit4.class)
public class ASurfaceControlTest {
    static {
        System.loadLibrary("ctsview_jni");
    }

    private static final String TAG = ASurfaceControlTest.class.getSimpleName();
    private static final boolean DEBUG = false;

    private static final int DEFAULT_LAYOUT_WIDTH = 100;
    private static final int DEFAULT_LAYOUT_HEIGHT = 100;

    private static final PixelColor RED = new PixelColor(PixelColor.RED);
    private static final PixelColor BLUE = new PixelColor(PixelColor.BLUE);
    private static final PixelColor MAGENTA = new PixelColor(PixelColor.MAGENTA);
    private static final PixelColor GREEN = new PixelColor(PixelColor.GREEN);
    private static final PixelColor YELLOW = new PixelColor(PixelColor.YELLOW);

    @Rule
    public ActivityScenarioRule<ASurfaceControlTestActivity> mActivityRule =
            createFullscreenActivityScenarioRule(ASurfaceControlTestActivity.class);

    @Rule
    public TestName mName = new TestName();

    private ASurfaceControlTestActivity mActivity;

    @Before
    public void setup() {
        mActivityRule.getScenario().onActivity(activity -> mActivity = activity);
    }

    ///////////////////////////////////////////////////////////////////////////
    // SurfaceHolder.Callbacks
    ///////////////////////////////////////////////////////////////////////////

    private abstract class BasicSurfaceHolderCallback implements SurfaceHolder.Callback {
        private final Set<Long> mSurfaceControls = new HashSet<>();
        private final Set<Long> mBuffers = new HashSet<>();

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            Canvas canvas = holder.lockCanvas();
            canvas.drawColor(Color.YELLOW);
            holder.unlockCanvasAndPost(canvas);
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            for (Long surfaceControl : mSurfaceControls) {
                reparent(surfaceControl, 0);
                nSurfaceControl_release(surfaceControl);
            }
            mSurfaceControls.clear();

            for (Long buffer : mBuffers) {
                nSurfaceTransaction_releaseBuffer(buffer);
            }
            mBuffers.clear();
        }

        public long createFromWindow(Surface surface) {
            long surfaceControl = nSurfaceControl_createFromWindow(surface);
            assertTrue("failed to create surface control", surfaceControl != 0);

            mSurfaceControls.add(surfaceControl);
            return surfaceControl;
        }

        public long create(long parentSurfaceControl) {
            long childSurfaceControl = nSurfaceControl_create(parentSurfaceControl);
            assertTrue("failed to create child surface control", childSurfaceControl != 0);

            mSurfaceControls.add(childSurfaceControl);
            return childSurfaceControl;
        }

        public long setSolidBuffer(
                long surfaceControl, long surfaceTransaction, int width, int height, int color) {
            long buffer = nSurfaceTransaction_setSolidBuffer(surfaceControl, surfaceTransaction,
                    width, height, color);
            assertTrue("failed to set buffer", buffer != 0);
            mBuffers.add(buffer);
            return buffer;
        }

        public long setSolidBuffer(long surfaceControl, int width, int height, int color) {
            long surfaceTransaction = createSurfaceTransaction();
            long buffer = setSolidBuffer(surfaceControl, surfaceTransaction, width, height, color);
            TimedTransactionListener onCommitCallback = new TimedTransactionListener();
            nSurfaceTransaction_setOnCommitCallback(surfaceTransaction, onCommitCallback);
            applyAndDeleteSurfaceTransaction(surfaceTransaction);
            try {
                onCommitCallback.mLatch.await(1, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
            }
            return buffer;
        }

        public void setQuadrantBuffer(long surfaceControl, long surfaceTransaction, int width,
                int height, int colorTopLeft, int colorTopRight, int colorBottomRight,
                int colorBottomLeft) {
            long buffer = nSurfaceTransaction_setQuadrantBuffer(surfaceControl, surfaceTransaction,
                    width, height, colorTopLeft, colorTopRight, colorBottomRight, colorBottomLeft);
            assertTrue("failed to set buffer", buffer != 0);
            mBuffers.add(buffer);
        }

        public void setQuadrantBuffer(long surfaceControl, int width, int height, int colorTopLeft,
                int colorTopRight, int colorBottomRight, int colorBottomLeft) {
            long surfaceTransaction = createSurfaceTransaction();
            setQuadrantBuffer(surfaceControl, surfaceTransaction, width, height, colorTopLeft,
                    colorTopRight, colorBottomRight, colorBottomLeft);
            TimedTransactionListener onCommitCallback = new TimedTransactionListener();
            nSurfaceTransaction_setOnCommitCallback(surfaceTransaction, onCommitCallback);
            applyAndDeleteSurfaceTransaction(surfaceTransaction);
            try {
                onCommitCallback.mLatch.await(1, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Tests
    ///////////////////////////////////////////////////////////////////////////

    private void verifyTest(SurfaceHolder.Callback callback, PixelChecker pixelChecker) {
        mActivity.verifyTest(callback, pixelChecker, 0 /* delayInMs */);
    }

    @Test
    public void testSurfaceTransaction_create() {
        long surfaceTransaction = nSurfaceTransaction_create();
        assertTrue("failed to create surface transaction", surfaceTransaction != 0);

        nSurfaceTransaction_delete(surfaceTransaction);
    }

    @Test
    public void testSurfaceTransaction_apply() {
        long surfaceTransaction = nSurfaceTransaction_create();
        assertTrue("failed to create surface transaction", surfaceTransaction != 0);

        Log.e("Transaction", "created: " + surfaceTransaction);

        nSurfaceTransaction_apply(surfaceTransaction);
        nSurfaceTransaction_delete(surfaceTransaction);
    }

    // INTRO: The following tests run a series of commands and verify the
    // output based on the number of pixels with a certain color on the display.
    //
    // The interface being tested is a NDK api but the only way to record the display
    // through public apis is in through the SDK. So the test logic and test verification
    // is in Java but the hooks that call into the NDK api are jni code.
    //
    // The set up is done during the surfaceCreated callback. In most cases, the
    // test uses the opportunity to create a child layer through createFromWindow and
    // performs operations on the child layer.
    //
    // When there is no visible buffer for the layer(s) the color defaults to black.
    // The test cases allow a +/- 10% error rate. This is based on the error
    // rate allowed in the SurfaceViewSyncTests

    @Test
    public void testSurfaceControl_createFromWindow() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());
                    }
                },
                new PixelChecker(PixelColor.YELLOW) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceControl_create() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        long childSurfaceControl = create(parentSurfaceControl);
                    }
                },
                new PixelChecker(PixelColor.YELLOW) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceControl_acquire() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());
                        // increment one refcount
                        nSurfaceControl_acquire(surfaceControl);
                        // decrement one refcount incremented from create call
                        nSurfaceControl_release(surfaceControl);
                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setBuffer() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());
                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setBuffer_parentAndChild() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        long childSurfaceControl = create(parentSurfaceControl);

                        setSolidBuffer(parentSurfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.BLUE);
                        setSolidBuffer(childSurfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setBuffer_childOnly() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        long childSurfaceControl = create(parentSurfaceControl);

                        setSolidBuffer(childSurfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setVisibility_show() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setVisibility(surfaceControl, true);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setVisibility_hide() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setVisibility(surfaceControl, false);
                    }
                },
                new PixelChecker(PixelColor.YELLOW) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setBufferOpaque_opaque() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setBufferOpaque(surfaceControl, true);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setBufferOpaque_transparent() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.TRANSPARENT_RED);
                        setBufferOpaque(surfaceControl, false);
                    }
                },
                // setBufferOpaque is an optimization that can be used by SurfaceFlinger.
                // It isn't required to affect SurfaceFlinger's behavior.
                //
                // Ideally we would check for a specific blending of red with a layer below
                // it. Unfortunately we don't know what blending the layer will use and
                // we don't know what variation the GPU/DPU/blitter might have. Although
                // we don't know what shade of red might be present, we can at least check
                // that the optimization doesn't cause the framework to drop the buffer entirely.
                new PixelChecker(PixelColor.YELLOW) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount == 0;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setDestinationRect() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setDestinationRect_small() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setGeometry(surfaceControl, 0, 0, 100, 100, 10, 10, 50, 50, 0);
                    }
                },
                new PixelChecker(PixelColor.RED) { //1600
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 1440 && pixelCount < 1760;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setDestinationRect_childSmall() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        long childSurfaceControl = create(parentSurfaceControl);

                        setSolidBuffer(childSurfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        setGeometry(childSurfaceControl, 0, 0, 100, 100, 10, 10, 50, 50, 0);
                    }
                },
                new PixelChecker(PixelColor.RED) { //1600
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 1440 && pixelCount < 1760;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setDestinationRect_extraLarge() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setGeometry(surfaceControl, 0, 0, 100, 100, -100, -100, 200, 200, 0);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setDestinationRect_childExtraLarge() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        long childSurfaceControl = create(parentSurfaceControl);

                        setSolidBuffer(childSurfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        setGeometry(childSurfaceControl, 0, 0, 100, 100, -100, -100, 200, 200, 0);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setDestinationRect_negativeOffset() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setGeometry(surfaceControl, 0, 0, 100, 100, -30, -20, 50, 50, 0);
                    }
                },
                new PixelChecker(PixelColor.RED) { //5600 (w = 80, h = 70)
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 5000 && pixelCount < 6000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setDestinationRect_outOfParentBounds() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setGeometry(surfaceControl, 0, 0, 100, 100, 50, 50, 110, 105, 0);
                    }
                },
                new PixelChecker(PixelColor.RED) { //2500
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 2250 && pixelCount < 2750;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setDestinationRect_twoLayers() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl1 = createFromWindow(holder.getSurface());
                        long surfaceControl2 = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl1, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setSolidBuffer(surfaceControl2, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.BLUE);
                        setGeometry(surfaceControl1, 0, 0, 100, 100, 10, 10, 30, 40, 0);
                        setGeometry(surfaceControl2, 0, 0, 100, 100, 70, 20, 90, 50, 0);
                    }
                },

                new MultiRectChecker(new Rect(0, 0, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT)) {
                    @Override
                    public PixelColor getExpectedColor(int x, int y) {
                        if (x >= 10 && x < 30 && y >= 10 && y < 40) {
                            return RED;
                        } else if (x >= 70 && x < 90 && y >= 20 && y < 50) {
                            return BLUE;
                        } else {
                            return YELLOW;
                        }
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setSourceRect() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                    }
                },

                new MultiRectChecker(new Rect(0, 0, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT)) {
                    @Override
                    public PixelColor getExpectedColor(int x, int y) {
                        int halfWidth = DEFAULT_LAYOUT_WIDTH / 2;
                        int halfHeight = DEFAULT_LAYOUT_HEIGHT / 2;
                        if (x < halfWidth && y < halfHeight) {
                            return RED;
                        } else if (x >= halfWidth && y < halfHeight) {
                            return BLUE;
                        } else if (x < halfWidth && y >= halfHeight) {
                            return GREEN;
                        } else {
                            return MAGENTA;
                        }
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setSourceRect_smallCentered() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setGeometry(surfaceControl, 10, 10, 90, 90, 0, 0, 100, 100, 0);
                    }
                },

                new MultiRectChecker(new Rect(0, 0, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT)) {
                    @Override
                    public PixelColor getExpectedColor(int x, int y) {
                        int halfWidth = DEFAULT_LAYOUT_WIDTH / 2;
                        int halfHeight = DEFAULT_LAYOUT_HEIGHT / 2;
                        if (x < halfWidth && y < halfHeight) {
                            return RED;
                        } else if (x >= halfWidth && y < halfHeight) {
                            return BLUE;
                        } else if (x < halfWidth && y >= halfHeight) {
                            return GREEN;
                        } else {
                            return MAGENTA;
                        }
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setSourceRect_small() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setGeometry(surfaceControl, 60, 10, 90, 90, 0, 0, 100, 100, 0);
                    }
                },
                new PixelChecker(PixelColor.MAGENTA) { //5000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 4500 && pixelCount < 5500;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setSourceRect_extraLarge() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setGeometry(surfaceControl, -50, -50, 150, 150, 0, 0, 100, 100, 0);
                    }
                },

                new MultiRectChecker(new Rect(0, 0, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT)) {
                    @Override
                    public PixelColor getExpectedColor(int x, int y) {
                        int halfWidth = DEFAULT_LAYOUT_WIDTH / 2;
                        int halfHeight = DEFAULT_LAYOUT_HEIGHT / 2;
                        if (x < halfWidth && y < halfHeight) {
                            return RED;
                        } else if (x >= halfWidth && y < halfHeight) {
                            return BLUE;
                        } else if (x < halfWidth && y >= halfHeight) {
                            return GREEN;
                        } else {
                            return MAGENTA;
                        }
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setSourceRect_badOffset() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setGeometry(surfaceControl, -50, -50, 50, 50, 0, 0, 100, 100, 0);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setTransform_flipH() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setGeometry(surfaceControl, 60, 10, 90, 90, 0, 0, 100, 100,
                                /*NATIVE_WINDOW_TRANSFORM_FLIP_H*/ 1);
                    }
                },
                new PixelChecker(PixelColor.BLUE) { //5000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 4500 && pixelCount < 5500;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setTransform_rotate180() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setGeometry(surfaceControl, 60, 10, 90, 90, 0, 0, 100, 100,
                                /*NATIVE_WINDOW_TRANSFORM_ROT_180*/ 3);
                    }
                },
                new PixelChecker(PixelColor.BLUE) { //5000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 4500 && pixelCount < 5500;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setDamageRegion_all() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());
                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);

                        long surfaceTransaction = createSurfaceTransaction();
                        setSolidBuffer(surfaceControl, surfaceTransaction, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.BLUE);
                        nSurfaceTransaction_setDamageRegion(surfaceControl, surfaceTransaction, 0,
                                0, 100, 100);
                        applyAndDeleteSurfaceTransaction(surfaceTransaction);
                    }
                },
                new PixelChecker(PixelColor.BLUE) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setZOrder_zero() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl1 = createFromWindow(holder.getSurface());
                        long surfaceControl2 = createFromWindow(holder.getSurface());
                        setSolidBuffer(surfaceControl1, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setSolidBuffer(surfaceControl2, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.MAGENTA);

                        setZOrder(surfaceControl1, 1);
                        setZOrder(surfaceControl2, 0);
                    }
                },
                new PixelChecker(PixelColor.YELLOW) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount == 0;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setZOrder_positive() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl1 = createFromWindow(holder.getSurface());
                        long surfaceControl2 = createFromWindow(holder.getSurface());
                        setSolidBuffer(surfaceControl1, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setSolidBuffer(surfaceControl2, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.MAGENTA);

                        setZOrder(surfaceControl1, 1);
                        setZOrder(surfaceControl2, 5);
                    }
                },
                new PixelChecker(PixelColor.RED) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount == 0;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setZOrder_negative() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl1 = createFromWindow(holder.getSurface());
                        long surfaceControl2 = createFromWindow(holder.getSurface());
                        setSolidBuffer(surfaceControl1, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setSolidBuffer(surfaceControl2, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.MAGENTA);

                        setZOrder(surfaceControl1, 1);
                        setZOrder(surfaceControl2, -15);
                    }
                },
                new PixelChecker(PixelColor.YELLOW) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount == 0;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setZOrder_max() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl1 = createFromWindow(holder.getSurface());
                        long surfaceControl2 = createFromWindow(holder.getSurface());
                        setSolidBuffer(surfaceControl1, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setSolidBuffer(surfaceControl2, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.MAGENTA);

                        setZOrder(surfaceControl1, 1);
                        setZOrder(surfaceControl2, Integer.MAX_VALUE);
                    }
                },
                new PixelChecker(PixelColor.RED) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount == 0;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setZOrder_min() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl1 = createFromWindow(holder.getSurface());
                        long surfaceControl2 = createFromWindow(holder.getSurface());
                        setSolidBuffer(surfaceControl1, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setSolidBuffer(surfaceControl2, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.MAGENTA);

                        setZOrder(surfaceControl1, 1);
                        setZOrder(surfaceControl2, Integer.MIN_VALUE);
                    }
                },
                new PixelChecker(PixelColor.YELLOW) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount == 0;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setOnComplete() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    private long mContext;

                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        long surfaceTransaction = createSurfaceTransaction();
                        setSolidBuffer(surfaceControl, surfaceTransaction, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        mContext = nSurfaceTransaction_setOnComplete(surfaceTransaction);
                        applyAndDeleteSurfaceTransaction(surfaceTransaction);
                    }

                    @Override
                    public void surfaceDestroyed(SurfaceHolder holder) {
                        super.surfaceDestroyed(holder);
                        nSurfaceTransaction_checkOnComplete(mContext, -1);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    @RequiresDevice // emulators can't support sync fences
    public void testSurfaceTransaction_setDesiredPresentTime_now() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    private long mContext;
                    private long mDesiredPresentTime;

                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        long surfaceTransaction = createSurfaceTransaction();
                        setSolidBuffer(surfaceControl, surfaceTransaction, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        mDesiredPresentTime = nSurfaceTransaction_setDesiredPresentTime(
                                surfaceTransaction, 0);
                        mContext = nSurfaceTransaction_setOnComplete(surfaceTransaction);
                        applyAndDeleteSurfaceTransaction(surfaceTransaction);
                    }

                    @Override
                    public void surfaceDestroyed(SurfaceHolder holder) {
                        super.surfaceDestroyed(holder);
                        nSurfaceTransaction_checkOnComplete(mContext, mDesiredPresentTime);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    @RequiresDevice // emulators can't support sync fences
    public void testSurfaceTransaction_setDesiredPresentTime_30ms() {
        mActivity.verifyTest(
                new BasicSurfaceHolderCallback() {
                    private long mContext;
                    private long mDesiredPresentTime;

                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        long surfaceTransaction = createSurfaceTransaction();
                        setSolidBuffer(surfaceControl, surfaceTransaction, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        mDesiredPresentTime = nSurfaceTransaction_setDesiredPresentTime(
                                surfaceTransaction, 30000000);
                        mContext = nSurfaceTransaction_setOnComplete(surfaceTransaction);
                        applyAndDeleteSurfaceTransaction(surfaceTransaction);
                    }

                    @Override
                    public void surfaceDestroyed(SurfaceHolder holder) {
                        super.surfaceDestroyed(holder);
                        nSurfaceTransaction_checkOnComplete(mContext, mDesiredPresentTime);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                }, 30 /* delayInMs */);
    }

    @Test
    @RequiresDevice // emulators can't support sync fences
    public void testSurfaceTransaction_setDesiredPresentTime_100ms() {
        mActivity.verifyTest(
                new BasicSurfaceHolderCallback() {
                    private long mContext;
                    private long mDesiredPresentTime;

                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        long surfaceTransaction = createSurfaceTransaction();
                        setSolidBuffer(surfaceControl, surfaceTransaction, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        mDesiredPresentTime = nSurfaceTransaction_setDesiredPresentTime(
                                surfaceTransaction, 100000000);
                        mContext = nSurfaceTransaction_setOnComplete(surfaceTransaction);
                        applyAndDeleteSurfaceTransaction(surfaceTransaction);
                    }

                    @Override
                    public void surfaceDestroyed(SurfaceHolder holder) {
                        super.surfaceDestroyed(holder);
                        nSurfaceTransaction_checkOnComplete(mContext, mDesiredPresentTime);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        Log.d(TAG, "pixelCount " + pixelCount);
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                }, 100 /* delayInMs */);
    }

    @Test
    public void testSurfaceTransaction_setBufferAlpha_1_0() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setBufferAlpha(surfaceControl, 1.0);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setBufferAlpha_0_5() {
        BasicSurfaceHolderCallback callback = new BasicSurfaceHolderCallback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                long surfaceControl = createFromWindow(holder.getSurface());

                setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                        PixelColor.RED);
                setBufferAlpha(surfaceControl, 0.5);
            }
        };
        verifyTest(callback,
                new PixelChecker(PixelColor.YELLOW) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount == 0;
                    }
                });
        verifyTest(callback,
                new PixelChecker(PixelColor.RED) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount == 0;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setBufferAlpha_0_0() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setBufferAlpha(surfaceControl, 0.0);
                    }
                },
                new PixelChecker(PixelColor.YELLOW) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_reparent() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl1 = createFromWindow(holder.getSurface());
                        long parentSurfaceControl2 = createFromWindow(holder.getSurface());
                        long childSurfaceControl = create(parentSurfaceControl1);

                        setGeometry(parentSurfaceControl1, 0, 0, 100, 100, 0, 0, 25, 100, 0);
                        setGeometry(parentSurfaceControl2, 0, 0, 100, 100, 25, 0, 100, 100, 0);

                        setSolidBuffer(childSurfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);

                        reparent(childSurfaceControl, parentSurfaceControl2);
                    }
                },
                new PixelChecker(PixelColor.RED) { //7500
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 6750 && pixelCount < 8250;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_reparent_null() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        long childSurfaceControl = create(parentSurfaceControl);

                        setSolidBuffer(childSurfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);

                        reparent(childSurfaceControl, 0);
                    }
                },
                new PixelChecker(PixelColor.YELLOW) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setColor() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setColor(surfaceControl, 0, 1.0f, 0, 1.0f);
                    }
                },
                new PixelChecker(PixelColor.GREEN) { // 10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_noColorNoBuffer() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        long childSurfaceControl = create(parentSurfaceControl);

                        setColor(parentSurfaceControl, 0, 1.0f, 0, 1.0f);
                    }
                },
                new PixelChecker(PixelColor.GREEN) { // 10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setColorAlpha() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        setColor(parentSurfaceControl, 0, 0, 1.0f, 0);
                    }
                },
                new PixelChecker(PixelColor.YELLOW) { // 10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setColorAndBuffer() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(
                                surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        setColor(surfaceControl, 0, 1.0f, 0, 1.0f);
                    }
                },
                new PixelChecker(PixelColor.RED) { // 10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setColorAndBuffer_bufferAlpha_0_5() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(
                                surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setBufferAlpha(surfaceControl, 0.5);
                        setColor(surfaceControl, 0, 0, 1.0f, 1.0f);
                    }
                },
                new PixelChecker(PixelColor.RED) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount == 0;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setBufferNoColor_bufferAlpha_0() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControlA = createFromWindow(holder.getSurface());
                        long surfaceControlB = createFromWindow(holder.getSurface());

                        setColor(surfaceControlA, 1.0f, 0, 0, 1.0f);
                        setSolidBuffer(surfaceControlB, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.TRANSPARENT);

                        setZOrder(surfaceControlA, 1);
                        setZOrder(surfaceControlB, 2);
                    }
                },
                new PixelChecker(PixelColor.RED) { // 10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setColorAndBuffer_hide() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        long childSurfaceControl = create(parentSurfaceControl);

                        setColor(parentSurfaceControl, 0, 1.0f, 0, 1.0f);

                        setSolidBuffer(
                                childSurfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        setColor(childSurfaceControl, 0, 0, 1.0f, 1.0f);
                        setVisibility(childSurfaceControl, false);
                    }
                },
                new PixelChecker(PixelColor.GREEN) { // 10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_zOrderMultipleSurfaces() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControlA = createFromWindow(holder.getSurface());
                        long surfaceControlB = createFromWindow(holder.getSurface());

                        // blue color layer of A is above the green buffer and red color layer
                        // of B
                        setColor(surfaceControlA, 0, 0, 1.0f, 1.0f);
                        setSolidBuffer(
                                surfaceControlB, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.GREEN);
                        setColor(surfaceControlB, 1.0f, 0, 0, 1.0f);
                        setZOrder(surfaceControlA, 5);
                        setZOrder(surfaceControlB, 4);
                    }
                },
                new PixelChecker(PixelColor.BLUE) { // 10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_zOrderMultipleSurfacesWithParent() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long parentSurfaceControl = createFromWindow(holder.getSurface());
                        long surfaceControlA = create(parentSurfaceControl);
                        long surfaceControlB = create(parentSurfaceControl);

                        setColor(surfaceControlA, 0, 1.0f, 0, 1.0f);
                        setSolidBuffer(
                                surfaceControlA, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.GREEN);
                        setColor(surfaceControlB, 1.0f, 0, 0, 1.0f);
                        setZOrder(surfaceControlA, 3);
                        setZOrder(surfaceControlB, 4);
                    }
                },
                new PixelChecker(PixelColor.RED) { // 10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setPosition() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setPosition(surfaceControl, 20, 10);
                    }
                },
                new PixelChecker(PixelColor.RED) { // 7200
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 7000 && pixelCount < 8000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setPositionNegative() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        // Offset -20, -10
                        setPosition(surfaceControl,  -20, -10);
                    }
                },
                new PixelChecker(PixelColor.RED) { // 7200
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 7000 && pixelCount < 8000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setScale() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setSolidBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT,
                                PixelColor.RED);
                        setScale(surfaceControl, .5f, .5f);
                    }
                },
                new PixelChecker(PixelColor.RED) { // 2500
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 2000 && pixelCount < 3000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setBufferTransform90() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setPosition(surfaceControl, -50, -50);
                        setBufferTransform(surfaceControl, /* NATIVE_WINDOW_TRANSFORM_ROT_90 */ 4);
                    }
                },
                new PixelChecker(PixelColor.BLUE) { // 2500
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 2000 && pixelCount < 3000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setCropSmall() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setCrop(surfaceControl, new Rect(0, 0, 50, 50));
                    }
                },

                new MultiRectChecker(new Rect(0, 0, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT)) {
                    @Override
                    public PixelColor getExpectedColor(int x, int y) {
                        int halfWidth = DEFAULT_LAYOUT_WIDTH / 2;
                        int halfHeight = DEFAULT_LAYOUT_HEIGHT / 2;
                        if (x < halfWidth && y < halfHeight) {
                            return RED;
                        } else {
                            return YELLOW;
                        }
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setCropLarge() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setCrop(surfaceControl, new Rect(0, 0, 150, 150));
                    }
                },

                new MultiRectChecker(new Rect(0, 0, DEFAULT_LAYOUT_WIDTH, DEFAULT_LAYOUT_HEIGHT)) {
                    @Override
                    public PixelColor getExpectedColor(int x, int y) {
                        int halfWidth = DEFAULT_LAYOUT_WIDTH / 2;
                        int halfHeight = DEFAULT_LAYOUT_HEIGHT / 2;
                        if (x < halfWidth && y < halfHeight) {
                            return RED;
                        } else if (x >= halfWidth && y < halfHeight) {
                            return BLUE;
                        } else if (x < halfWidth && y >= halfHeight) {
                            return GREEN;
                        } else {
                            return MAGENTA;
                        }
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setCropOffset() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setCrop(surfaceControl, new Rect(50, 50, 100, 100));
                    }
                }, new PixelChecker(PixelColor.MAGENTA) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 2000 && pixelCount < 3000;
                    }
                });
    }

    @Test
    public void testSurfaceTransaction_setCropNegative() {
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceControl = createFromWindow(holder.getSurface());

                        setQuadrantBuffer(surfaceControl, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED, PixelColor.BLUE,
                                PixelColor.MAGENTA, PixelColor.GREEN);
                        setCrop(surfaceControl, new Rect(-50, -50, 50, 50));
                    }
                }, new PixelChecker(PixelColor.RED) {
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 2000 && pixelCount < 3000;
                    }
                });
    }

    static class TimedTransactionListener implements
            ASurfaceControlTestUtils.TransactionCompleteListener {
        long mCallbackTime = -1;
        long mLatchTime = -1;
        CountDownLatch mLatch = new CountDownLatch(1);
        @Override
        public void onTransactionComplete(long inLatchTime) {
            mCallbackTime = SystemClock.elapsedRealtime();
            mLatchTime = inLatchTime;
            mLatch.countDown();
        }
    }

    @Test
    public void testSurfaceTransactionOnCommitCallback_emptyTransaction()
            throws InterruptedException {
        // Create and send an empty transaction with onCommit and onComplete callbacks.
        long surfaceTransaction = nSurfaceTransaction_create();
        TimedTransactionListener onCompleteCallback = new TimedTransactionListener();
        nSurfaceTransaction_setOnCompleteCallback(surfaceTransaction, onCompleteCallback);
        TimedTransactionListener onCommitCallback = new TimedTransactionListener();
        nSurfaceTransaction_setOnCommitCallback(surfaceTransaction, onCommitCallback);
        nSurfaceTransaction_apply(surfaceTransaction);
        nSurfaceTransaction_delete(surfaceTransaction);

        // Wait for callbacks to fire.
        onCommitCallback.mLatch.await(1, TimeUnit.SECONDS);
        onCompleteCallback.mLatch.await(1, TimeUnit.SECONDS);

        // Validate we got callbacks.
        assertEquals(0, onCommitCallback.mLatch.getCount());
        assertTrue(onCommitCallback.mCallbackTime > 0);
        assertEquals(0, onCompleteCallback.mLatch.getCount());
        assertTrue(onCompleteCallback.mCallbackTime > 0);

        // Validate we received the callbacks in expected order.
        assertTrue(onCommitCallback.mCallbackTime <= onCompleteCallback.mCallbackTime);
    }

    @Test
    public void testSurfaceTransactionOnCommitCallback_bufferTransaction()
            throws Throwable {
        // Create and send a transaction with a buffer update and with onCommit and onComplete
        // callbacks.
        TimedTransactionListener onCompleteCallback = new TimedTransactionListener();
        TimedTransactionListener onCommitCallback = new TimedTransactionListener();
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceTransaction = nSurfaceTransaction_create();
                        long surfaceControl = createFromWindow(holder.getSurface());
                        setSolidBuffer(surfaceControl, surfaceTransaction, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        nSurfaceTransaction_setOnCompleteCallback(surfaceTransaction,
                                onCompleteCallback);
                        nSurfaceTransaction_setOnCommitCallback(surfaceTransaction,
                                onCommitCallback);
                        nSurfaceTransaction_apply(surfaceTransaction);
                        nSurfaceTransaction_delete(surfaceTransaction);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });

        // Wait for callbacks to fire.
        onCommitCallback.mLatch.await(1, TimeUnit.SECONDS);
        onCompleteCallback.mLatch.await(1, TimeUnit.SECONDS);

        // Validate we got callbacks with a valid latch time.
        assertEquals(0, onCommitCallback.mLatch.getCount());
        assertTrue(onCommitCallback.mCallbackTime > 0);
        assertTrue(onCommitCallback.mLatchTime > 0);
        assertEquals(0, onCompleteCallback.mLatch.getCount());
        assertTrue(onCompleteCallback.mCallbackTime > 0);
        assertTrue(onCompleteCallback.mLatchTime > 0);

        // Validate we received the callbacks in expected order and the latch times reported
        // matches.
        assertTrue(onCommitCallback.mCallbackTime <= onCompleteCallback.mCallbackTime);
        assertEquals(onCommitCallback.mLatchTime, onCompleteCallback.mLatchTime);
    }

    @Test
    public void testSurfaceTransactionOnCommitCallback_geometryTransaction()
            throws Throwable {
        // Create and send a transaction with a buffer update and with onCommit and onComplete
        // callbacks.
        TimedTransactionListener onCompleteCallback = new TimedTransactionListener();
        TimedTransactionListener onCommitCallback = new TimedTransactionListener();
        verifyTest(
                new BasicSurfaceHolderCallback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        long surfaceTransaction = nSurfaceTransaction_create();
                        long surfaceControl = createFromWindow(holder.getSurface());
                        setSolidBuffer(surfaceControl, surfaceTransaction, DEFAULT_LAYOUT_WIDTH,
                                DEFAULT_LAYOUT_HEIGHT, PixelColor.RED);
                        nSurfaceTransaction_apply(surfaceTransaction);
                        nSurfaceTransaction_delete(surfaceTransaction);
                        surfaceTransaction = nSurfaceTransaction_create();
                        nSurfaceTransaction_setPosition(surfaceControl, surfaceTransaction, 1, 0);
                        nSurfaceTransaction_setOnCompleteCallback(surfaceTransaction,
                                onCompleteCallback);
                        nSurfaceTransaction_setOnCommitCallback(surfaceTransaction,
                                onCommitCallback);
                        nSurfaceTransaction_apply(surfaceTransaction);
                        nSurfaceTransaction_delete(surfaceTransaction);
                    }
                },
                new PixelChecker(PixelColor.RED) { //10000
                    @Override
                    public boolean checkPixels(int pixelCount, int width, int height) {
                        return pixelCount > 9000 && pixelCount < 11000;
                    }
                });

        // Wait for callbacks to fire.
        onCommitCallback.mLatch.await(1, TimeUnit.SECONDS);
        onCompleteCallback.mLatch.await(1, TimeUnit.SECONDS);

        // Validate we got callbacks with a valid latch time.
        assertTrue(onCommitCallback.mLatch.getCount() == 0);
        assertTrue(onCommitCallback.mCallbackTime > 0);
        assertTrue(onCommitCallback.mLatchTime > 0);
        assertTrue(onCompleteCallback.mLatch.getCount() == 0);
        assertTrue(onCompleteCallback.mCallbackTime > 0);
        assertTrue(onCompleteCallback.mLatchTime > 0);

        // Validate we received the callbacks in expected order and the latch times reported
        // matches.
        assertTrue(onCommitCallback.mCallbackTime <= onCompleteCallback.mCallbackTime);
        assertTrue(onCommitCallback.mLatchTime == onCompleteCallback.mLatchTime);
    }

    @Test
    public void testSurfaceTransactionOnCommitCallback_withoutContext()
            throws InterruptedException {
        // Create and send an empty transaction with onCommit callbacks without context.
        long surfaceTransaction = nSurfaceTransaction_create();
        TimedTransactionListener onCommitCallback = new TimedTransactionListener();
        nSurfaceTransaction_setOnCommitCallbackWithoutContext(surfaceTransaction, onCommitCallback);
        nSurfaceTransaction_apply(surfaceTransaction);
        nSurfaceTransaction_delete(surfaceTransaction);

        // Wait for callbacks to fire.
        onCommitCallback.mLatch.await(1, TimeUnit.SECONDS);

        // Validate we got callbacks.
        assertEquals(0, onCommitCallback.mLatch.getCount());
        assertTrue(onCommitCallback.mCallbackTime > 0);
    }

    @Test
    public void testSurfaceTransactionOnCompleteCallback_withoutContext()
            throws InterruptedException {
        // Create and send an empty transaction with onComplete callbacks without context.
        long surfaceTransaction = nSurfaceTransaction_create();
        TimedTransactionListener onCompleteCallback = new TimedTransactionListener();
        nSurfaceTransaction_setOnCompleteCallbackWithoutContext(surfaceTransaction,
                onCompleteCallback);
        nSurfaceTransaction_apply(surfaceTransaction);
        nSurfaceTransaction_delete(surfaceTransaction);

        // Wait for callbacks to fire.
        onCompleteCallback.mLatch.await(1, TimeUnit.SECONDS);

        // Validate we got callbacks.
        assertEquals(0, onCompleteCallback.mLatch.getCount());
        assertTrue(onCompleteCallback.mCallbackTime > 0);
    }
}

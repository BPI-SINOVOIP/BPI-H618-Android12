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
package com.android.car.ui.uxr;

import static org.junit.Assert.assertNotNull;

import android.content.Context;
import android.content.res.Resources;
import android.util.AttributeSet;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.ui.TestActivity;
import com.android.car.ui.test.R;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.function.Supplier;
import java.util.stream.Collectors;

@RunWith(Parameterized.class)
public class DrawableStateViewTest {

    private static final List<Class<? extends DrawableStateView>> ALL_DRAWABLE_STATE_VIEWS =
            Collections.unmodifiableList(Arrays.asList(
                    DrawableStateButton.class,
                    DrawableStateConstraintLayout.class,
                    DrawableStateFrameLayout.class,
                    DrawableStateImageView.class,
                    DrawableStateLinearLayout.class,
                    DrawableStateRelativeLayout.class,
                    DrawableStateSwitch.class,
                    DrawableStateTextView.class,
                    DrawableStateToggleButton.class));

    @Parameterized.Parameters
    public static Object[] data() {
        return ALL_DRAWABLE_STATE_VIEWS.toArray();
    }

    private final Class<? extends DrawableStateView> mClassToTest;

    public DrawableStateViewTest(Class<? extends DrawableStateView> classToTest) {
        mClassToTest = classToTest;
    }

    @Rule
    public final ActivityScenarioRule<TestActivity> mScenarioRule =
            new ActivityScenarioRule<>(TestActivity.class);

    @Test
    public void test_drawablestateview_methods_work() throws ReflectiveOperationException {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        DrawableStateView button = mClassToTest.getDeclaredConstructor(Context.class)
                .newInstance(context);

        int[] drawableState = runOnUiThread(((View) button)::getDrawableState);

        assertDrawableArrays(
                drawableState,
                new int[] { android.R.attr.state_enabled },
                null);

        drawableState = runOnUiThread(() -> {
            ((View) button).setEnabled(false);
            return ((View) button).getDrawableState();
        });

        assertDrawableArrays(
                drawableState,
                null,
                new int[] { android.R.attr.state_enabled });

        drawableState = runOnUiThread(() -> {
            ((View) button).setEnabled(true);
            button.setExtraDrawableState(new int[] { R.attr.state_ux_restricted }, null);
            return ((View) button).getDrawableState();
        });

        assertDrawableArrays(
                drawableState,
                new int[] {
                        android.R.attr.state_enabled,
                        R.attr.state_ux_restricted
                },
                null);


        drawableState = runOnUiThread(() -> {
            ((View) button).setEnabled(true);
            button.setExtraDrawableState(
                    new int[] { R.attr.state_ux_restricted },
                    new int[] { android.R.attr.state_enabled });
            return ((View) button).getDrawableState();
        });

        assertDrawableArrays(
                drawableState,
                new int[] { R.attr.state_ux_restricted },
                new int[] { android.R.attr.state_enabled });
    }

    @Test
    public void test_setExtraDrawableState_calledFirst() throws ReflectiveOperationException {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        DrawableStateView button = mClassToTest.getDeclaredConstructor(Context.class)
                .newInstance(context);

        int[] drawableState = runOnUiThread(() -> {
            button.setExtraDrawableState(new int[] { R.attr.state_ux_restricted }, null);
            return ((View) button).getDrawableState();
        });

        assertDrawableArrays(
                drawableState,
                new int[] { android.R.attr.state_enabled, R.attr.state_ux_restricted },
                null);
    }

    @Test
    public void test_emptyStateToRemove_works() throws ReflectiveOperationException {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        DrawableStateView button = mClassToTest.getDeclaredConstructor(Context.class)
                .newInstance(context);

        int[] drawableState = runOnUiThread(() -> {
            button.setExtraDrawableState(null, new int[] {});
            return ((View) button).getDrawableState();
        });

        assertDrawableArrays(
                drawableState,
                new int[] { android.R.attr.state_enabled },
                new int[] { R.attr.state_ux_restricted });
    }

    @Test
    public void test_constructors_work() throws ReflectiveOperationException {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();
        assertNotNull(mClassToTest
                .getDeclaredConstructor(Context.class)
                .newInstance(context));
        assertNotNull(mClassToTest
                .getDeclaredConstructor(Context.class, AttributeSet.class)
                .newInstance(context, null));
        assertNotNull(mClassToTest
                .getDeclaredConstructor(Context.class, AttributeSet.class, int.class)
                .newInstance(context, null, 0));
    }

    private int[] runOnUiThread(Supplier<int[]> runnable) {
        int[][] result = new int[][] {null};
        mScenarioRule.getScenario().onActivity(activity -> result[0] = runnable.get());
        return result[0];
    }

    /**
     * Checks that a drawable array has some attributes and doesn't have others.
     *
     * @param drawableArr The drawable array to check
     * @param contains An array of attributes that the drawable array must contain.
     * @param doesntContain An array of attributes the drawable array must not contain.
     */
    private static void assertDrawableArrays(
            int[] drawableArr, int[] contains, int[] doesntContain) {
        if (contains != null) {
            for (int contain : contains) {
                boolean found = false;
                for (int attr : drawableArr) {
                    if (attr == contain) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    Assert.fail("Drawable array " + drawableArrayToString(drawableArr)
                            + " did not contain required attribute "
                            + getResourceName(contain));
                }
            }
        }

        if (doesntContain != null) {
            for (int notContained : doesntContain) {
                for (int attr : drawableArr) {
                    if (attr == notContained) {
                        Assert.fail("Drawable array " + drawableArrayToString(drawableArr)
                                + " contained prohibited attribute "
                                + getResourceName(notContained));
                    }
                }
            }
        }
    }

    @NonNull
    private static String drawableArrayToString(@Nullable int[] arr) {
        if (arr == null) {
            return "null";
        }

        return "[" + Arrays.stream(arr)
                .mapToObj(DrawableStateViewTest::getResourceName)
                .collect(Collectors.joining(", ")) + "]";
    }

    private static String getResourceName(int resource) {
        Context context = InstrumentationRegistry.getInstrumentation().getContext();

        try {
            return context.getResources().getResourceName(resource);
        } catch (Resources.NotFoundException e) {
            return String.valueOf(resource);
        }
    }
}

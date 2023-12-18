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

package com.android.activitycontext;

import static android.content.Intent.FLAG_ACTIVITY_CLEAR_TASK;
import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;

import com.android.compatibility.common.util.ShellIdentityUtils.QuadFunction;
import com.android.compatibility.common.util.ShellIdentityUtils.TriFunction;

import java.util.concurrent.CountDownLatch;
import java.util.function.BiConsumer;
import java.util.function.BiFunction;
import java.util.function.Consumer;
import java.util.function.Function;

import javax.annotation.Nullable;

/**
 * Activity used for tests which need an actual {@link Context}.
 */
public class ActivityContext extends Activity {

    private static final String LOG_TAG = "ActivityContext";
    private static final Context sContext =
            InstrumentationRegistry.getInstrumentation().getContext();

    private static Function<Activity, ?> sRunnable;
    private static @Nullable Object sReturnValue;
    private static @Nullable Object sThrowValue;
    private static CountDownLatch sLatch;

    /**
     * Run some code using an Activity {@link Context}.
     *
     * <p>This method should only be called from an instrumented app.
     *
     * <p>The {@link Activity} will be valid within the {@code runnable} callback. Passing the
     * {@link Activity} outside of the callback is not recommended because it may become invalid
     * due to lifecycle changes.
     *
     * <p>This method will block until the callback has been executed. It will return the same value
     * as returned by the callback.
     */
    public static <E> E getWithContext(Function<Activity, E> runnable) throws InterruptedException {
        if (runnable == null) {
            throw new NullPointerException();
        }
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();

        if (!instrumentation.getContext().getPackageName().equals(
                instrumentation.getTargetContext().getPackageName())) {
            throw new IllegalStateException("ActivityContext can only be used in test apps which"
                    + " instrument themselves. Consider ActivityScenario for this case.");
        }

        synchronized (ActivityContext.class) {
            sRunnable = runnable;

            sLatch = new CountDownLatch(1);
            sReturnValue = null;
            sThrowValue = null;

            Intent intent = new Intent();
            intent.setClass(sContext, ActivityContext.class);
            intent.setFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_CLEAR_TASK);
            sContext.startActivity(intent);
        }

        sLatch.await();

        synchronized (ActivityContext.class) {
            sRunnable = null;

            if (sThrowValue != null) {
                if (sThrowValue instanceof RuntimeException) {
                    throw (RuntimeException) sThrowValue;
                }

                if (sThrowValue instanceof Error) {
                    throw (Error) sThrowValue;
                }

                throw new IllegalStateException("Invalid value for sThrowValue");
            }

            return (E) sReturnValue;
        }
    }

    /** {@link #getWithContext(Function)} which does not return a value. */
    public static void runWithContext(Consumer<Activity> runnable) throws InterruptedException {
        getWithContext((inContext) -> {runnable.accept(inContext); return null; });
    }

    /** {@link #getWithContext(Function)} with an additional argument. */
    public static <E, F> F getWithContext(E arg1,
            BiFunction<Activity, E, F> runnable) throws InterruptedException {
        return getWithContext((inContext) -> runnable.apply(inContext, arg1));
    }

    /**
     * {@link #getWithContext(Function)} which takes an additional argument and does not
     * return a value.
     */
    public static <E> void runWithContext(E arg1, BiConsumer<Activity, E> runnable)
            throws InterruptedException {
        getWithContext((inContext) -> {runnable.accept(inContext, arg1); return null; });
    }

    /** {@link #getWithContext(Function)} with two additional arguments. */
    public static <E, F, G> G getWithContext(E arg1, F arg2,
            TriFunction<Activity, E, F, G> runnable) throws InterruptedException {
        return getWithContext((inContext) -> runnable.apply(inContext, arg1, arg2));
    }

    /** {@link #getWithContext(Function)} with three additional arguments. */
    public static <E, F, G, H> H getWithContext(E arg1, F arg2, G arg3,
            QuadFunction<Activity, E, F, G, H> runnable) throws InterruptedException {
        return getWithContext((inContext) -> runnable.apply(inContext, arg1, arg2, arg3));
    }

    @Override
    protected void onResume() {
        super.onResume();
        synchronized (ActivityContext.class) {
            if (sRunnable == null) {
                Log.e(LOG_TAG, "Launched ActivityContext without runnable");
            } else {
                try {
                    sReturnValue = sRunnable.apply(this);
                } catch (RuntimeException | Error e) {
                    sThrowValue = e;
                }
                sLatch.countDown();
            }
        }
    }
}

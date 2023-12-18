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
package com.android.car.ui.core;

import android.app.Activity;
import android.app.Application;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatDelegate;

import com.android.car.ui.CarUiLayoutInflaterFactory;
import com.android.car.ui.baselayout.Insets;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Locale;

/**
 * {@link ContentProvider ContentProvider's} onCreate() methods are "called for all registered
 * content providers on the application main thread at application launch time." This means we
 * can use a content provider to register for Activity lifecycle callbacks before any activities
 * have started, for installing the CarUi base layout into all activities.
 *
 * Notice that in many of the methods in this class we're using reflection to make method calls.
 * As it's explained in (b/156532465), {@link CarUiInstaller} is loaded from
 * GMSCore's ContainerActivity classloader which is different than the classloader of the Activity
 * that's passed as an argument to these methods. This happens when the Activity's module is loaded
 * dynamically. That means {@link CarUiInstaller} will have a different classloader than the
 * Activity. Hence we will need to use the Activity's classloader to load
 * {@link BaseLayoutController} class otherwise the base layout will be loaded
 * by the wrong classloader. And then calls to {@see CarUi#getToolbar(Activity)} will return null.
 */
public class CarUiInstaller extends ContentProvider {

    private static final String TAG = "CarUiInstaller";
    private static final String CAR_UI_INSET_LEFT = "CAR_UI_INSET_LEFT";
    private static final String CAR_UI_INSET_RIGHT = "CAR_UI_INSET_RIGHT";
    private static final String CAR_UI_INSET_TOP = "CAR_UI_INSET_TOP";
    private static final String CAR_UI_INSET_BOTTOM = "CAR_UI_INSET_BOTTOM";

    private static final boolean IS_DEBUG_DEVICE =
            Build.TYPE.toLowerCase(Locale.ROOT).contains("debug")
                    || Build.TYPE.toLowerCase(Locale.ROOT).equals("eng");

    @Override
    public boolean onCreate() {
        Context context = getContext();
        if (context == null) {
            Log.e(TAG, "CarUiInstaller had a null context!");
            return false;
        }
        Log.i(TAG, "CarUiInstaller started for " + context.getPackageName());

        Application application = (Application) context.getApplicationContext();
        application.registerActivityLifecycleCallbacks(
                new Application.ActivityLifecycleCallbacks() {
                    private Insets mInsets = null;
                    private boolean mIsActivityStartedForFirstTime = false;

                    @Override
                    public void onActivityCreated(Activity activity, Bundle savedInstanceState) {
                        injectLayoutInflaterFactory(activity);

                        callMethodReflective(
                                activity.getClassLoader(),
                                BaseLayoutController.class,
                                "build",
                                null,
                                activity);

                        if (savedInstanceState != null) {
                            int inset_left = savedInstanceState.getInt(CAR_UI_INSET_LEFT);
                            int inset_top = savedInstanceState.getInt(CAR_UI_INSET_TOP);
                            int inset_right = savedInstanceState.getInt(CAR_UI_INSET_RIGHT);
                            int inset_bottom = savedInstanceState.getInt(CAR_UI_INSET_BOTTOM);
                            mInsets = new Insets(inset_left, inset_top, inset_right, inset_bottom);
                        }

                        mIsActivityStartedForFirstTime = true;
                    }

                    @Override
                    public void onActivityPostStarted(Activity activity) {
                        Object controller = callMethodReflective(
                                activity.getClassLoader(),
                                BaseLayoutController.class,
                                "getBaseLayoutController",
                                null,
                                activity);
                        if (mInsets != null && controller != null
                                && mIsActivityStartedForFirstTime) {
                            callMethodReflective(
                                    activity.getClassLoader(),
                                    BaseLayoutController.class,
                                    "dispatchNewInsets",
                                    controller,
                                    changeInsetsClassLoader(activity.getClassLoader(), mInsets));
                            mIsActivityStartedForFirstTime = false;
                        }
                    }

                    @Override
                    public void onActivityStarted(Activity activity) {
                    }

                    @Override
                    public void onActivityResumed(Activity activity) {
                    }

                    @Override
                    public void onActivityPaused(Activity activity) {
                    }

                    @Override
                    public void onActivityStopped(Activity activity) {
                    }

                    @Override
                    public void onActivitySaveInstanceState(Activity activity, Bundle outState) {
                        Object controller = callMethodReflective(
                                activity.getClassLoader(),
                                BaseLayoutController.class,
                                "getBaseLayoutController",
                                null,
                                activity);
                        if (controller != null) {
                            Object insets = callMethodReflective(
                                    activity.getClassLoader(),
                                    BaseLayoutController.class,
                                    "getInsets",
                                    controller);
                            outState.putInt(CAR_UI_INSET_LEFT,
                                    (int) callMethodReflective(
                                            activity.getClassLoader(),
                                            Insets.class,
                                            "getLeft",
                                            insets));
                            outState.putInt(CAR_UI_INSET_TOP,
                                    (int) callMethodReflective(
                                            activity.getClassLoader(),
                                            Insets.class,
                                            "getTop",
                                            insets));
                            outState.putInt(CAR_UI_INSET_RIGHT,
                                    (int) callMethodReflective(
                                            activity.getClassLoader(),
                                            Insets.class,
                                            "getRight",
                                            insets));
                            outState.putInt(CAR_UI_INSET_BOTTOM,
                                    (int) callMethodReflective(
                                            activity.getClassLoader(),
                                            Insets.class,
                                            "getBottom",
                                            insets));
                        }
                    }

                    @Override
                    public void onActivityDestroyed(Activity activity) {
                        callMethodReflective(
                                activity.getClassLoader(),
                                BaseLayoutController.class,
                                "destroy",
                                null,
                                activity);
                    }
                });

        return true;
    }

    @Nullable
    @Override
    public Cursor query(@NonNull Uri uri, @Nullable String[] projection, @Nullable String selection,
            @Nullable String[] selectionArgs, @Nullable String sortOrder) {
        return null;
    }

    @Nullable
    @Override
    public String getType(@NonNull Uri uri) {
        return null;
    }

    @Nullable
    @Override
    public Uri insert(@NonNull Uri uri, @Nullable ContentValues values) {
        return null;
    }

    @Override
    public int delete(@NonNull Uri uri, @Nullable String selection,
            @Nullable String[] selectionArgs) {
        return 0;
    }

    @Override
    public int update(@NonNull Uri uri, @Nullable ContentValues values, @Nullable String selection,
            @Nullable String[] selectionArgs) {
        return 0;
    }

    @SuppressWarnings("AndroidJdkLibsChecker")
    private static Object callMethodReflective(@NonNull ClassLoader cl, @NonNull Class<?> srcClass,
            @NonNull String methodName, @Nullable Object instance, @Nullable Object... args) {
        try {
            Class<?> clazz = cl.loadClass(srcClass.getName());
            Class<?>[] classArgs = args == null ? null
                    : Arrays.stream(args)
                            .map(arg -> arg instanceof Activity ? Activity.class : arg.getClass())
                            .toArray(Class<?>[]::new);
            Method method = clazz.getDeclaredMethod(methodName, classArgs);
            method.setAccessible(true);
            return method.invoke(instance, args);
        } catch (ReflectiveOperationException | SecurityException e) {
            throw new RuntimeException(e);
        }
    }

    private static Object changeInsetsClassLoader(@NonNull ClassLoader cl, @Nullable Insets src) {
        if (src == null) {
            return null;
        }
        try {
            Class<?> insetsClass = cl.loadClass(Insets.class.getName());
            Constructor<?> cnst = insetsClass.getDeclaredConstructor(
                    int.class,
                    int.class,
                    int.class,
                    int.class);
            cnst.setAccessible(true);
            return cnst.newInstance(
                    src.getLeft(),
                    src.getTop(),
                    src.getRight(),
                    src.getBottom());
        } catch (ReflectiveOperationException | SecurityException e) {
            throw new RuntimeException();
        }
    }

    private static void injectLayoutInflaterFactory(Context context) {
        // For {@link AppCompatActivity} activities our layout inflater
        // factory is instantiated via viewInflaterClass attribute.
        LayoutInflater layoutInflater = LayoutInflater.from(context);
        if (layoutInflater.getFactory2() == null) {
            layoutInflater.setFactory2(new CarUiLayoutInflaterFactory());
        } else if (!(layoutInflater.getFactory2()
                instanceof CarUiLayoutInflaterFactory)
                        && !(layoutInflater.getFactory2()
                                instanceof AppCompatDelegate)) {
            throw new AssertionError(layoutInflater.getFactory2()
                    + " must extend CarUiLayoutInflaterFactory");
        }
    }
}

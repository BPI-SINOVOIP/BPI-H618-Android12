/*
 * Copyright (C) 2019 The Android Open Source Project
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
package com.android.car.ui.utils;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.DimenRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.annotation.UiThread;

import com.android.car.ui.R;
import com.android.car.ui.uxr.DrawableStateView;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Function;

/**
 * Collection of utility methods
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public final class CarUiUtils {

    private static final String TAG = "CarUiUtils";
    private static final String READ_ONLY_SYSTEM_PROPERTY_PREFIX = "ro.";
    /** A map to cache read-only system properties. */
    private static final SparseArray<String> READ_ONLY_SYSTEM_PROPERTY_MAP = new SparseArray<>();

    private static int[] sRestrictedState;

    /** This is a utility class */
    private CarUiUtils() {
    }

    /**
     * Reads a float value from a dimens resource. This is necessary as {@link Resources#getFloat}
     * is not currently public.
     *
     * @param res   {@link Resources} to read values from
     * @param resId Id of the dimens resource to read
     */
    public static float getFloat(Resources res, @DimenRes int resId) {
        TypedValue outValue = new TypedValue();
        res.getValue(resId, outValue, true);
        return outValue.getFloat();
    }

    /** Returns the identifier of the resolved resource assigned to the given attribute. */
    public static int getAttrResourceId(Context context, int attr) {
        return getAttrResourceId(context, /*styleResId=*/ 0, attr);
    }

    /**
     * Returns the identifier of the resolved resource assigned to the given attribute defined in
     * the given style.
     */
    public static int getAttrResourceId(Context context, @StyleRes int styleResId, int attr) {
        TypedArray ta = context.obtainStyledAttributes(styleResId, new int[]{attr});
        int resId = ta.getResourceId(0, 0);
        ta.recycle();
        return resId;
    }

    /**
     * Gets the {@link Activity} for a certain {@link Context}.
     *
     * <p>It is possible the Context is not associated with an Activity, in which case
     * this method will return null.
     */
    @Nullable
    public static Activity getActivity(@Nullable Context context) {
        while (context instanceof ContextWrapper) {
            if (context instanceof Activity) {
                return (Activity) context;
            }
            context = ((ContextWrapper) context).getBaseContext();
        }
        return null;
    }

    /**
     * It behaves similarly to {@link View#findViewById(int)}, except that on Q and below,
     * it will first resolve the id to whatever it references.
     *
     * This is to support layout RROs before the new RRO features in R.
     *
     * @param id the ID to search for
     * @return a view with given ID if found, or {@code null} otherwise
     * @see View#requireViewById(int)
     */
    @Nullable
    @UiThread
    public static <T extends View> T findViewByRefId(@NonNull View root, @IdRes int id) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return root.findViewById(id);
        }

        if (id == View.NO_ID) {
            return null;
        }

        TypedValue value = new TypedValue();
        root.getResources().getValue(id, value, true);
        return root.findViewById(value.resourceId);
    }

    /**
     * It behaves similarly to {@link View#requireViewById(int)}, except that on Q and below,
     * it will first resolve the id to whatever it references.
     *
     * This is to support layout RROs before the new RRO features in R.
     *
     * @param id the ID to search for
     * @return a view with given ID
     * @see View#findViewById(int)
     */
    @NonNull
    @UiThread
    public static <T extends View> T requireViewByRefId(@NonNull View root, @IdRes int id) {
        T view = findViewByRefId(root, id);
        if (view == null) {
            throw new IllegalArgumentException("ID "
                    + root.getResources().getResourceName(id)
                    + " does not reference a View inside this View");
        }
        return view;
    }

    /**
     * Returns the system property of type boolean. This method converts the boolean value in string
     * returned by {@link #getSystemProperty(Resources, int)}
     */
    public static boolean getBooleanSystemProperty(
            @NonNull Resources resources, int propertyResId, boolean defaultValue) {
        String value = getSystemProperty(resources, propertyResId);

        if (!TextUtils.isEmpty(value)) {
            return Boolean.parseBoolean(value);
        }
        return defaultValue;
    }

    /**
     * Use reflection to interact with the hidden API <code>android.os.SystemProperties</code>.
     *
     * <p>This method caches read-only properties. CAVEAT: Please do not set read-only properties
     * by 'adb setprop' after app started. Read-only properties CAN BE SET ONCE if it is unset.
     * Thus, read-only properties MAY BE CHANGED from unset to set during application's lifetime if
     * you use 'adb setprop' command to set read-only properties after app started. For the sake of
     * performance, this method also caches the unset state. Otherwise, cache may not effective if
     * the system property is unset (which is most-likely).
     *
     * @param resources     resources object to fetch string
     * @param propertyResId the property resource id.
     * @return The value of the property if defined, else null. Does not return empty strings.
     */
    @Nullable
    public static String getSystemProperty(@NonNull Resources resources, int propertyResId) {
        String propertyName = resources.getString(propertyResId);
        boolean isReadOnly = propertyName.startsWith(READ_ONLY_SYSTEM_PROPERTY_PREFIX);
        if (!isReadOnly) {
            return readSystemProperty(propertyName);
        }
        synchronized (READ_ONLY_SYSTEM_PROPERTY_MAP) {
            // readOnlySystemPropertyMap may contain null values.
            if (READ_ONLY_SYSTEM_PROPERTY_MAP.indexOfKey(propertyResId) >= 0) {
                return READ_ONLY_SYSTEM_PROPERTY_MAP.get(propertyResId);
            }
            String value = readSystemProperty(propertyName);
            READ_ONLY_SYSTEM_PROPERTY_MAP.put(propertyResId, value);
            return value;
        }
    }

    @Nullable
    private static String readSystemProperty(String propertyName) {
        Class<?> systemPropertiesClass;
        try {
            systemPropertiesClass = Class.forName("android.os.SystemProperties");
        } catch (ClassNotFoundException e) {
            Log.w(TAG, "Cannot find android.os.SystemProperties: ", e);
            return null;
        }

        Method getMethod;
        try {
            getMethod = systemPropertiesClass.getMethod("get", String.class);
        } catch (NoSuchMethodException e) {
            Log.w(TAG, "Cannot find SystemProperties.get(): ", e);
            return null;
        }

        try {
            Object[] params = new Object[]{propertyName};
            String value = (String) getMethod.invoke(systemPropertiesClass, params);
            return TextUtils.isEmpty(value) ? null : value;
        } catch (Exception e) {
            Log.w(TAG, "Failed to invoke SystemProperties.get(): ", e);
            return null;
        }
    }

    /**
     * Converts a drawable to bitmap. This value should not be null.
     */
    public static Bitmap drawableToBitmap(@NonNull Drawable drawable) {
        Bitmap bitmap;

        if (drawable instanceof BitmapDrawable) {
            BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
            if (bitmapDrawable.getBitmap() != null) {
                return bitmapDrawable.getBitmap();
            }
        }

        if (drawable.getIntrinsicWidth() <= 0 || drawable.getIntrinsicHeight() <= 0) {
            bitmap = Bitmap.createBitmap(1, 1,
                    Bitmap.Config.ARGB_8888); // Single color bitmap will be created of 1x1 pixel
        } else {
            bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                    drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        }

        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        drawable.draw(canvas);
        return bitmap;
    }

    /**
     * Exact copy from Androidx.TypedArrayUtils class
     * @return The resource ID value in the {@code context} specified by {@code attr}. If it does
     * not exist, {@code fallbackAttr}.
     */
    public static int getAttr(@NonNull Context context, int attr, int fallbackAttr) {
        TypedValue value = new TypedValue();
        context.getTheme().resolveAttribute(attr, value, true);
        if (value.resourceId != 0) {
            return attr;
        }
        return fallbackAttr;
    }

    /**
     * Converts a {@link CharSequence} to a {@link String}.
     *
     * This is the same as calling {@link CharSequence#toString()}, except it will handle
     * null CharSequences, returning a null string.
     */
    public static String charSequenceToString(@Nullable CharSequence charSequence) {
        return charSequence == null ? null : charSequence.toString();
    }

    /**
     * Given a list of T and a function to convert from T to U, return a list of U.
     *
     * This will create a new list.
     */
    public static <T, U> List<U> convertList(List<T> list, Function<T, U> f) {
        if (list == null) {
            return null;
        }

        List<U> result = new ArrayList<>();
        for (T item : list) {
            result.add(f.apply(item));
        }
        return result;
    }

    /**
     * Traverses the view hierarchy, and whenever it sees a {@link DrawableStateView}, adds
     * state_ux_restricted to it.
     *
     * Note that this will remove any other drawable states added by other calls to
     * {@link DrawableStateView#setExtraDrawableState(int[], int[])}
     */
    public static void makeAllViewsUxRestricted(@Nullable View view, boolean restricted) {
        if (view instanceof DrawableStateView) {
            if (sRestrictedState == null) {
                int androidStateUxRestricted = view.getResources()
                        .getIdentifier("state_ux_restricted", "attr", "android");

                if (androidStateUxRestricted == 0) {
                    sRestrictedState = new int[] { R.attr.state_ux_restricted };
                } else {
                    sRestrictedState = new int[] {
                            R.attr.state_ux_restricted,
                            androidStateUxRestricted
                    };
                }
            }

            ((DrawableStateView) view).setExtraDrawableState(
                    restricted ? sRestrictedState : null, null);
        }

        if (view instanceof ViewGroup) {
            ViewGroup vg = (ViewGroup) view;
            for (int i = 0; i < vg.getChildCount(); i++) {
                makeAllViewsUxRestricted(vg.getChildAt(i), restricted);
            }
        }
    }
}

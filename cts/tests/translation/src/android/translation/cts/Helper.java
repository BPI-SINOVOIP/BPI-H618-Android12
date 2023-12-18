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

package android.translation.cts;

import static com.android.compatibility.common.util.ShellUtils.runShellCommand;

import android.content.ContentCaptureOptions;
import android.content.Context;
import android.os.UserHandle;
import android.util.Log;
import android.view.contentcapture.ContentCaptureContext;

import androidx.test.InstrumentationRegistry;
import androidx.test.uiautomator.By;
import androidx.test.uiautomator.UiDevice;
import androidx.test.uiautomator.UiObject2;
import androidx.test.uiautomator.Until;

/**
 * Helper for common funcionalities.
 */
public final class Helper {

    private static final String TAG = "Helper";

    public static final String ACTIVITY_PACKAGE = "android.translation.cts";

    public static final String ACTION_REGISTER_UI_TRANSLATION_CALLBACK =
            "android.translation.cts.action.REGISTER_UI_TRANSLATION_CALLBACK";
    public static final String ACTION_UNREGISTER_UI_TRANSLATION_CALLBACK =
            "android.translation.cts.action.UNREGISTER_UI_TRANSLATION_CALLBACK";
    public static final String ACTION_ASSERT_UI_TRANSLATION_CALLBACK_ON_START =
            "android.translation.cts.action.ASSERT_UI_TRANSLATION_CALLBACK_ON_START";
    public static final String ACTION_ASSERT_UI_TRANSLATION_CALLBACK_ON_FINISH =
            "android.translation.cts.action.ASSERT_UI_TRANSLATION_CALLBACK_ON_FINISH";
    public static final String ACTION_ASSERT_UI_TRANSLATION_CALLBACK_ON_RESUME =
            "android.translation.cts.action.ASSERT_UI_TRANSLATION_CALLBACK_ON_RESUME";
    public static final String ACTION_ASSERT_UI_TRANSLATION_CALLBACK_ON_PAUSE =
            "android.translation.cts.action.ASSERT_UI_TRANSLATION_CALLBACK_ON_PAUSE";

    public static final String EXTRA_FINISH_COMMAND = "finish_command";
    public static final String EXTRA_SOURCE_LOCALE = "source_locale";
    public static final String EXTRA_TARGET_LOCALE = "target_locale";
    public static final String EXTRA_VERIFY_RESULT = "verify_result";

    public static final String CUSTOM_TRANSLATION_ID_MY_TAG = "myTag";
    private static final String LOG_TAG = "log.tag.UiTranslation";

    /**
     * Sets the translation service temporarily.
     *
     * @param service name of temporary translation service.
     */
    public static void setTemporaryTranslationService(String service) {
        Log.d(TAG, "Setting translation service to " + service);
        final int userId = UserHandle.myUserId();
        runShellCommand("cmd translation set temporary-service %d %s 12000", userId, service);
    }

    /**
     * Resets the translation service.
     */
    public static void resetTemporaryTranslationService() {
        final int userId = UserHandle.myUserId();
        Log.d(TAG, "Resetting back user " + userId + " to default translation service");
        runShellCommand("cmd translation set temporary-service %d", userId);
    }

    /**
     * Sets the content capture service temporarily.
     *
     * @param service name of temporary translation service.
     */
    public static void setTemporaryContentCaptureService(String service) {
        Log.d(TAG, "Setting content capture service to " + service);
        final int userId = UserHandle.myUserId();
        runShellCommand("cmd content_capture set temporary-service %d %s 12000", userId, service);
    }

    /**
     * Resets the content capture service.
     */
    public static void resetTemporaryContentCaptureService() {
        final int userId = UserHandle.myUserId();
        Log.d(TAG, "Resetting back user " + userId + " to default service");
        runShellCommand("cmd content_capture set temporary-service %d", userId);
    }

    /**
     * Enable or disable the default content capture service.
     *
     * @param enabled {@code true} to enable default content capture service.
     */
    public static void setDefaultContentCaptureServiceEnabled(boolean enabled) {
        final int userId = UserHandle.myUserId();
        Log.d(TAG, "setDefaultServiceEnabled(user=" + userId + ", enabled= " + enabled + ")");
        runShellCommand("cmd content_capture set default-service-enabled %d %s", userId,
                Boolean.toString(enabled));
    }

    /**
     * Add the cts itself into content capture allow list.
     *
     * @param context Context of the app.
     */
    public static void allowSelfForContentCapture(Context context) {
        final ContentCaptureOptions options = ContentCaptureOptions.forWhitelistingItself();
        Log.v(TAG, "allowSelfForContentCapture(): options=" + options);
        context.getApplicationContext().setContentCaptureOptions(options);
    }

    /**
     * Reset the cts itself from content capture allow list.
     *
     * @param context Context of the app.
     */
    public static void unAllowSelfForContentCapture(Context context) {
        Log.v(TAG, "unAllowSelfForContentCapture()");
        context.getApplicationContext().setContentCaptureOptions(null);
    }

    /**
     * Return a ui object for resource id.
     *
     * @param resourcePackage  package of the object
     * @param resourceId the resource id of the object
     */
    public static UiObject2 findObjectByResId(String resourcePackage, String resourceId) {
        final UiDevice uiDevice =
                UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        final UiObject2 foundObj = uiDevice.wait(
                        Until.findObject(By.res(resourcePackage, resourceId)), 5_000L);
        return foundObj;
    }

    /**
     * Enable DEBUG log and returns the original log level value.
     */
    public static String enableDebugLog() {
        String originalValue = System.getProperty(LOG_TAG, "");
        System.setProperty(LOG_TAG, "DEBUG");
        Log.d(TAG, "enableDebugLog(), original value = " + originalValue);
        return originalValue;
    }

    /**
     * Disable debug log.
     *
     * @param level the log level. The value can be DEBUG, INFO, VERBOSE or empty if not set.
     */
    public static void disableDebugLog(String level) {
        Log.d(TAG, "disableDebugLog(), set level  " + level);
        System.setProperty(LOG_TAG, level);
    }
}
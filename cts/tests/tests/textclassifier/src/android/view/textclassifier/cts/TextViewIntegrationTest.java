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

package android.view.textclassifier.cts;

import static android.content.pm.PackageManager.FEATURE_TOUCHSCREEN;
import static android.provider.Settings.Global.ANIMATOR_DURATION_SCALE;
import static android.provider.Settings.Global.TRANSITION_ANIMATION_SCALE;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.RootMatchers.isPlatformPopup;
import static androidx.test.espresso.matcher.ViewMatchers.hasDescendant;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withTagValue;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static com.google.common.truth.Truth.assertThat;

import static org.hamcrest.CoreMatchers.allOf;
import static org.hamcrest.CoreMatchers.is;

import android.app.PendingIntent;
import android.app.RemoteAction;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.RemoteException;
import android.provider.Settings;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextClassifier;
import android.view.textclassifier.TextLinks;
import android.view.textclassifier.TextSelection;
import android.widget.TextView;

import androidx.core.os.BuildCompat;
import androidx.test.core.app.ActivityScenario;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.espresso.ViewInteraction;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.uiautomator.UiDevice;

import com.android.compatibility.common.util.ShellUtils;
import com.android.compatibility.common.util.SystemUtil;

import org.junit.AfterClass;
import org.junit.Assume;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;

import java.io.IOException;
import java.util.Collections;
import java.util.concurrent.atomic.AtomicInteger;

public class TextViewIntegrationTest {
    private final static String LOG_TAG = "TextViewIntegrationTest";
    private final static String TOOLBAR_TAG = "floating_toolbar";

    private SimpleTextClassifier mSimpleTextClassifier;

    @Rule
    public ActivityScenarioRule<TextViewActivity> rule = new ActivityScenarioRule<>(
            TextViewActivity.class);

    private static float sOriginalAnimationDurationScale;
    private static float sOriginalTransitionAnimationDurationScale;

    @Before
    public void setup() throws Exception {
        Assume.assumeTrue(
                ApplicationProvider.getApplicationContext().getPackageManager()
                        .hasSystemFeature(FEATURE_TOUCHSCREEN));
        workAroundNotificationShadeWindowIssue();
        mSimpleTextClassifier = new SimpleTextClassifier();
        UiDevice.getInstance(InstrumentationRegistry.getInstrumentation()).wakeUp();
        dismissKeyguard();
        closeSystemDialog();
    }

    // Somehow there is a stale "NotificationShade" window from SysUI stealing the inputs.
    // The window is in the "exiting" state and seems never finish exiting.
    // The workaround here is to (hopefully) reset its state by expanding the notification panel
    // and collapsing it again.
    private void workAroundNotificationShadeWindowIssue() throws InterruptedException {
        ShellUtils.runShellCommand("cmd statusbar expand-notifications");
        Thread.sleep(1000);
        ShellUtils.runShellCommand("cmd statusbar collapse");
        Thread.sleep(1000);
    }

    private void dismissKeyguard() {
        ShellUtils.runShellCommand("wm dismiss-keyguard");
    }

    private static void closeSystemDialog() {
        ShellUtils.runShellCommand("am broadcast -a android.intent.action.CLOSE_SYSTEM_DIALOGS");
    }

    @BeforeClass
    public static void disableAnimation() {
        SystemUtil.runWithShellPermissionIdentity(() -> {
            ContentResolver resolver =
                    ApplicationProvider.getApplicationContext().getContentResolver();
            sOriginalAnimationDurationScale =
                    Settings.Global.getFloat(resolver, ANIMATOR_DURATION_SCALE, 1f);
            Settings.Global.putFloat(resolver, ANIMATOR_DURATION_SCALE, 0);

            sOriginalTransitionAnimationDurationScale =
                    Settings.Global.getFloat(resolver, TRANSITION_ANIMATION_SCALE, 1f);
            Settings.Global.putFloat(resolver, TRANSITION_ANIMATION_SCALE, 0);
        });
    }

    @AfterClass
    public static void restoreAnimation() {
        SystemUtil.runWithShellPermissionIdentity(() -> {
            Settings.Global.putFloat(
                    ApplicationProvider.getApplicationContext().getContentResolver(),
                    ANIMATOR_DURATION_SCALE, sOriginalAnimationDurationScale);

            Settings.Global.putFloat(
                    ApplicationProvider.getApplicationContext().getContentResolver(),
                    TRANSITION_ANIMATION_SCALE, sOriginalTransitionAnimationDurationScale);
        });
    }

    @Test
    public void smartLinkify() throws Exception {
        ActivityScenario<TextViewActivity> scenario = rule.getScenario();
        // Linkify the text.
        final String TEXT = "Link: https://www.android.com";
        AtomicInteger clickIndex = new AtomicInteger();
        Spannable linkifiedText = createLinkifiedText(TEXT);
        scenario.onActivity(activity -> {
            TextView textView = activity.findViewById(R.id.textview);
            textView.setText(linkifiedText);
            textView.setTextClassifier(mSimpleTextClassifier);
            textView.setMovementMethod(LinkMovementMethod.getInstance());
            TextLinks.TextLinkSpan[] spans = linkifiedText.getSpans(0, TEXT.length(),
                    TextLinks.TextLinkSpan.class);
            assertThat(spans).hasLength(1);
            TextLinks.TextLinkSpan span = spans[0];
            clickIndex.set(
                    (span.getTextLink().getStart() + span.getTextLink().getEnd()) / 2);
        });
        // To wait for the rendering of the activity to be completed, so that the upcoming click
        // action will work.
        Thread.sleep(2000);
        onView(allOf(withId(R.id.textview), withText(TEXT))).check(matches(isDisplayed()));
        // Click on the span.
        Log.d(LOG_TAG, "clickIndex = " + clickIndex.get());
        onView(withId(R.id.textview)).perform(TextViewActions.tapOnTextAtIndex(clickIndex.get()));

        assertFloatingToolbarIsDisplayed();
        assertFloatingToolbarContainsItem("Test");
    }

    @Test
    public void smartSelection_suggestSelectionNotIncludeTextClassification() throws Exception {
        Assume.assumeTrue(BuildCompat.isAtLeastS());
        smartSelectionInternal();

        assertThat(mSimpleTextClassifier.getClassifyTextInvocationCount()).isEqualTo(1);
    }

    @Test
    public void smartSelection_suggestSelectionIncludeTextClassification() throws Exception {
        Assume.assumeTrue(BuildCompat.isAtLeastS());
        mSimpleTextClassifier.setIncludeTextClassification(true);
        smartSelectionInternal();

        assertThat(mSimpleTextClassifier.getClassifyTextInvocationCount()).isEqualTo(0);
    }

    @Test
    @Ignore  // Enable the test once b/187862341 is fixed.
    public void smartSelection_cancelSelectionDoesNotInvokeClassifyText() throws Exception {
        Assume.assumeTrue(BuildCompat.isAtLeastS());
        smartSelectionInternal();
        onView(withId(R.id.textview)).perform(TextViewActions.tapOnTextAtIndex(0));
        Thread.sleep(1000);

        assertThat(mSimpleTextClassifier.getClassifyTextInvocationCount()).isEqualTo(1);
    }

    private void smartSelectionInternal() {
        ActivityScenario<TextViewActivity> scenario = rule.getScenario();
        AtomicInteger clickIndex = new AtomicInteger();
        //                   0123456789
        final String TEXT = "Link: https://www.android.com";
        scenario.onActivity(activity -> {
            TextView textView = activity.findViewById(R.id.textview);
            textView.setTextIsSelectable(true);
            textView.setText(TEXT);
            textView.setTextClassifier(mSimpleTextClassifier);
            clickIndex.set(9);
        });
        onView(allOf(withId(R.id.textview), withText(TEXT))).check(matches(isDisplayed()));

        // Long press the url to perform smart selection.
        Log.d(LOG_TAG, "clickIndex = " + clickIndex.get());
        onView(withId(R.id.textview)).perform(
                TextViewActions.longTapOnTextAtIndex(clickIndex.get()));

        assertFloatingToolbarIsDisplayed();
        assertFloatingToolbarContainsItem("Test");
    }

    private Spannable createLinkifiedText(CharSequence text) {
        TextLinks.Request request = new TextLinks.Request.Builder(text)
                .setEntityConfig(
                        new TextClassifier.EntityConfig.Builder()
                                .setIncludedTypes(Collections.singleton(TextClassifier.TYPE_URL))
                                .build())
                .build();
        TextLinks textLinks = mSimpleTextClassifier.generateLinks(request);
        Spannable linkifiedText = new SpannableString(text);
        int resultCode = textLinks.apply(
                linkifiedText,
                TextLinks.APPLY_STRATEGY_REPLACE,
                /* spanFactory= */null);
        assertThat(resultCode).isEqualTo(TextLinks.STATUS_LINKS_APPLIED);
        return linkifiedText;
    }

    private static ViewInteraction onFloatingToolBar() {
        return onView(withTagValue(is(TOOLBAR_TAG))).inRoot(isPlatformPopup());
    }

    private static void assertFloatingToolbarIsDisplayed() {
        onFloatingToolBar().check(matches(isDisplayed()));
    }

    private static void assertFloatingToolbarContainsItem(String itemLabel) {
        onFloatingToolBar().check(matches(hasDescendant(withText(itemLabel))));
    }

    /**
     * A {@link TextClassifier} that can only annotate the android.com url. Do not reuse the same
     * instance across tests.
     */
    private static class SimpleTextClassifier implements TextClassifier {
        private static final String ANDROID_URL = "https://www.android.com";
        private static final Icon NO_ICON = Icon.createWithData(new byte[0], 0, 0);
        private boolean mSetIncludeTextClassification = false;
        private int mClassifyTextInvocationCount = 0;

        public void setIncludeTextClassification(boolean setIncludeTextClassification) {
            mSetIncludeTextClassification = setIncludeTextClassification;
        }

        public int getClassifyTextInvocationCount() {
            return mClassifyTextInvocationCount;
        }

        @Override
        public TextSelection suggestSelection(TextSelection.Request request) {
            int start = request.getText().toString().indexOf(ANDROID_URL);
            if (start == -1) {
                return new TextSelection.Builder(
                        request.getStartIndex(), request.getEndIndex())
                        .build();
            }
            TextSelection.Builder builder =
                    new TextSelection.Builder(start, start + ANDROID_URL.length())
                            .setEntityType(TextClassifier.TYPE_URL, 1.0f);
            if (mSetIncludeTextClassification) {
                builder.setTextClassification(createAndroidUrlTextClassification());
            }
            return builder.build();
        }

        @Override
        public TextClassification classifyText(TextClassification.Request request) {
            mClassifyTextInvocationCount += 1;
            String spanText = request.getText().toString()
                    .substring(request.getStartIndex(), request.getEndIndex());
            if (TextUtils.equals(ANDROID_URL, spanText)) {
                return createAndroidUrlTextClassification();
            }
            return new TextClassification.Builder().build();
        }

        private TextClassification createAndroidUrlTextClassification() {
            TextClassification.Builder builder =
                    new TextClassification.Builder().setText(ANDROID_URL);
            builder.setEntityType(TextClassifier.TYPE_URL, 1.0f);

            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setData(Uri.parse(ANDROID_URL));
            PendingIntent pendingIntent = PendingIntent.getActivity(
                    ApplicationProvider.getApplicationContext(),
                    /* requestCode= */ 0,
                    intent,
                    PendingIntent.FLAG_IMMUTABLE);

            RemoteAction remoteAction =
                    new RemoteAction(NO_ICON, "Test", "content description", pendingIntent);
            remoteAction.setShouldShowIcon(false);
            builder.addAction(remoteAction);
            return builder.build();
        }

        @Override
        public TextLinks generateLinks(TextLinks.Request request) {
            TextLinks.Builder builder = new TextLinks.Builder(request.getText().toString());
            int index = request.getText().toString().indexOf(ANDROID_URL);
            if (index == -1) {
                return builder.build();
            }
            builder.addLink(index,
                    index + ANDROID_URL.length(),
                    Collections.singletonMap(TextClassifier.TYPE_URL, 1.0f));
            return builder.build();
        }
    }
}

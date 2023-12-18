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

package com.android.car.ui.imewidescreen;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.assertThat;
import static androidx.test.espresso.matcher.ViewMatchers.hasDescendant;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withEffectiveVisibility;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static com.android.car.ui.core.SearchResultsProvider.ITEM_ID;
import static com.android.car.ui.core.SearchResultsProvider.SECONDARY_IMAGE_ID;
import static com.android.car.ui.core.SearchResultsProvider.SUBTITLE;
import static com.android.car.ui.core.SearchResultsProvider.TITLE;
import static com.android.car.ui.core.SearchResultsProviderTest.AUTHORITY;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.ADD_DESC_TITLE_TO_CONTENT_AREA;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.ADD_DESC_TO_CONTENT_AREA;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.ADD_ERROR_DESC_TO_INPUT_AREA;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.CONTENT_AREA_SURFACE_PACKAGE;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.REQUEST_RENDER_CONTENT_AREA;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_ACTION;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_EXTRACTED_TEXT_ICON;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_EXTRACTED_TEXT_ICON_RES_ID;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenController.WIDE_SCREEN_SEARCH_RESULTS;
import static com.android.car.ui.imewidescreen.CarUiImeWideScreenTestActivity.sCarUiImeWideScreenController;

import static com.google.common.base.Preconditions.checkNotNull;

import static org.hamcrest.Matchers.containsString;
import static org.hamcrest.Matchers.is;
import static org.hamcrest.Matchers.not;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Dialog;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.inputmethodservice.ExtractEditText;
import android.inputmethodservice.InputMethodService;
import android.inputmethodservice.InputMethodService.Insets;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.SurfaceControlViewHost;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import androidx.test.InstrumentationRegistry;
import androidx.test.core.app.ActivityScenario;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.espresso.contrib.RecyclerViewActions;
import androidx.test.espresso.matcher.BoundedMatcher;
import androidx.test.espresso.matcher.ViewMatchers.Visibility;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.ui.core.SearchResultsProvider;
import com.android.car.ui.test.R;
import com.android.car.ui.utils.CarUiUtils;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.mockito.ArgumentMatchers;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

/**
 * Unit tests for {@link CarUiImeWideScreenController}.
 */
@RunWith(AndroidJUnit4.class)
public class CarUiImeWideScreenControllerTest {

    private final Context mContext = ApplicationProvider.getApplicationContext();
    private static final String TEST = "test";

    @Mock
    private EditorInfo mEditorInfoMock;

    @Mock
    private Display mDisplayMock;

    @Mock
    private InputConnection mInputConnectionMock;

    @Mock
    private SurfaceView mContentAreaSurfaceViewMock;

    @Mock
    private IBinder mHasTokenMock;

    @Mock
    private InputMethodService mInputMethodServiceMock;

    @Mock
    private Dialog mDialogMock;

    @Mock
    private Window mWindowMock;

    @Rule
    public ActivityScenarioRule<CarUiImeWideScreenTestActivity> mActivityRule =
            new ActivityScenarioRule<>(CarUiImeWideScreenTestActivity.class);

    @Rule
    public ExpectedException mThrownRule = ExpectedException.none();

    private CarUiImeWideScreenTestActivity mActivity;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        ActivityScenario<CarUiImeWideScreenTestActivity> mScenario = mActivityRule.getScenario();
        mScenario.onActivity(activity -> {
            mActivity = activity;
        });
    }

    @Test
    public void createWideScreenImeView_shouldWrapTheViewInTemplate() {
        // make sure view is wrapped in the template.
        assertNotNull(mActivity.findViewById(R.id.test_ime_input_view_id));

        // check all views in template default visibility.
        onView(withId(R.id.car_ui_wideScreenDescriptionTitle)).check(matches(not(isDisplayed())));
        onView(withId(R.id.car_ui_wideScreenDescription)).check(matches(not(isDisplayed())));
        onView(withId(R.id.car_ui_inputExtractActionAutomotive)).check(matches(not(isDisplayed())));
        onView(withId(R.id.car_ui_wideScreenSearchResultList)).check(matches(not(isDisplayed())));
        onView(withId(R.id.car_ui_wideScreenErrorMessage)).check(matches(not(isDisplayed())));
        onView(withId(R.id.car_ui_wideScreenError)).check(matches(not(isDisplayed())));
        onView(withId(R.id.car_ui_contentAreaAutomotive)).check(matches(not(isDisplayed())));
        onView(withId(R.id.car_ui_ime_surface)).check(matches(not(isDisplayed())));

        onView(withId(R.id.car_ui_wideScreenExtractedTextIcon)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_wideScreenClearData)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_fullscreenArea)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_inputExtractEditTextContainer)).check(matches(isDisplayed()));

        // check if the click listener is installed on the image to clear data.
        View clearDataIcon = mActivity.findViewById(R.id.car_ui_wideScreenClearData);
        assertTrue(clearDataIcon.hasOnClickListeners());
    }

    @Test
    public void onComputeInsets_showContentArea_shouldUpdateEntireAreaAsTouchable() {
        when(mInputMethodServiceMock.getWindow()).thenReturn(mDialogMock);
        when(mDialogMock.getWindow()).thenReturn(mWindowMock);
        View view = new FrameLayout(mContext);
        view.setTop(0);
        view.setBottom(200);
        when(mWindowMock.getDecorView()).thenReturn(view);

        InputMethodService.Insets outInsets = new Insets();
        CarUiImeWideScreenController carUiImeWideScreenController = getController();
        carUiImeWideScreenController.onComputeInsets(outInsets);

        assertThat(outInsets.touchableInsets, is(InputMethodService.Insets.TOUCHABLE_INSETS_FRAME));
        assertThat(outInsets.contentTopInsets, is(200));
        assertThat(outInsets.visibleTopInsets, is(200));
    }

    @Test
    public void onComputeInsets_hideContentArea_shouldUpdateRegionAsTouchable() {
        when(mInputMethodServiceMock.getWindow()).thenReturn(mDialogMock);
        when(mDialogMock.getWindow()).thenReturn(mWindowMock);
        View view = new FrameLayout(mActivity);
        view.setTop(0);
        view.setBottom(200);
        when(mWindowMock.getDecorView()).thenReturn(view);

        View imeInputView = LayoutInflater.from(mActivity)
                .inflate(R.layout.test_ime_input_view, null, false);
        CarUiImeWideScreenController carUiImeWideScreenController = getController();
        carUiImeWideScreenController.createWideScreenImeView(imeInputView);

        when(mContentAreaSurfaceViewMock.getDisplay()).thenReturn(mDisplayMock);
        when(mDisplayMock.getDisplayId()).thenReturn(0);
        carUiImeWideScreenController.setContentAreaSurfaceView(mContentAreaSurfaceViewMock);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            carUiImeWideScreenController
                    .onStartInputView(mEditorInfoMock, mInputConnectionMock, TEST);

            Bundle bundle = new Bundle();
            bundle.putBoolean(REQUEST_RENDER_CONTENT_AREA, false);
            carUiImeWideScreenController.onAppPrivateCommand(WIDE_SCREEN_ACTION, bundle);
        });

        InputMethodService.Insets outInsets = new Insets();
        carUiImeWideScreenController.onComputeInsets(outInsets);

        assertThat(outInsets.touchableInsets,
                is(InputMethodService.Insets.TOUCHABLE_INSETS_REGION));
        assertThat(outInsets.contentTopInsets, is(200));
        assertThat(outInsets.visibleTopInsets, is(200));
    }

    @Test
    public void onAppPrivateCommand_shouldShowTitleAndDesc() {
        when(mInputMethodServiceMock.getWindow()).thenReturn(mDialogMock);
        when(mDialogMock.getWindow()).thenReturn(mWindowMock);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.onStartInputView(mEditorInfoMock, mInputConnectionMock,
                    TEST);

            Bundle bundle = new Bundle();
            bundle.putString(ADD_DESC_TITLE_TO_CONTENT_AREA, "Title");
            bundle.putString(ADD_DESC_TO_CONTENT_AREA, "Description");
            sCarUiImeWideScreenController.onAppPrivateCommand(WIDE_SCREEN_ACTION, bundle);
        });

        onView(withId(R.id.car_ui_wideScreenDescriptionTitle)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_wideScreenDescriptionTitle)).check(
                matches(withText(containsString("Title"))));

        onView(withId(R.id.car_ui_wideScreenDescription)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_wideScreenDescription)).check(
                matches(withText(containsString("Description"))));
    }

    @Test
    public void onAppPrivateCommand_shouldShowErrorMessage() {
        when(mInputMethodServiceMock.getWindow()).thenReturn(mDialogMock);
        when(mDialogMock.getWindow()).thenReturn(mWindowMock);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.onStartInputView(mEditorInfoMock, mInputConnectionMock,
                    TEST);

            Bundle bundle = new Bundle();
            bundle.putString(ADD_ERROR_DESC_TO_INPUT_AREA, "Error Message");
            sCarUiImeWideScreenController.onAppPrivateCommand(WIDE_SCREEN_ACTION, bundle);
        });

        onView(withId(R.id.car_ui_wideScreenErrorMessage)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_wideScreenErrorMessage)).check(
                matches(withText(containsString("Error Message"))));
        onView(withId(R.id.car_ui_wideScreenError)).check(matches(isDisplayed()));
    }

    @Test
    public void isPackageAuthorized_shouldReturnFalse() {
        CarUiImeWideScreenController carUiImeWideScreenController = getController();
        boolean res = carUiImeWideScreenController.isPackageAuthorized("xyz");

        assertThat(res, is(false));
    }

    @Test
    public void setWideScreenExtractedIcon_shouldSetIcon() {

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.onStartInputView(mEditorInfoMock, mInputConnectionMock,
                    TEST);

            Bundle bundle = new Bundle();
            Drawable drawable = mContext.getResources().getDrawable(R.drawable.ic_launcher);
            Bitmap bitmap = CarUiUtils.drawableToBitmap(drawable);
            byte[] byteArray = bitmapToByteArray(bitmap);

            bundle.putByteArray(WIDE_SCREEN_EXTRACTED_TEXT_ICON, byteArray);
            sCarUiImeWideScreenController.onAppPrivateCommand(WIDE_SCREEN_SEARCH_RESULTS, bundle);
        });

        onView(withId(R.id.car_ui_wideScreenExtractedTextIcon)).check(matches(isDisplayed()));
    }

    @Test
    public void setWideScreenExtractedIconResId0_shouldSetIcon() {

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.onStartInputView(mEditorInfoMock, mInputConnectionMock,
                    TEST);

            Bundle bundle = new Bundle();
            bundle.putInt(WIDE_SCREEN_EXTRACTED_TEXT_ICON_RES_ID, 0);
            sCarUiImeWideScreenController.onAppPrivateCommand(WIDE_SCREEN_SEARCH_RESULTS, bundle);
        });

        onView(withId(R.id.car_ui_wideScreenExtractedTextIcon)).check(matches(isDisplayed()));
    }

    @Test
    public void setWideScreenExtractedIconResId_shouldSetIcon() {

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.onStartInputView(mEditorInfoMock, mInputConnectionMock,
                    TEST);

            Bundle bundle = new Bundle();
            bundle.putInt(WIDE_SCREEN_EXTRACTED_TEXT_ICON_RES_ID, R.drawable.ic_launcher);
            sCarUiImeWideScreenController.onAppPrivateCommand(WIDE_SCREEN_SEARCH_RESULTS, bundle);
        });

        onView(withId(R.id.car_ui_wideScreenExtractedTextIcon)).check(matches(isDisplayed()));
    }

    @Test
    public void setWideScreenExtractedIconResId_shouldTHideIcon() {
        when(mContentAreaSurfaceViewMock.getDisplay()).thenReturn(mDisplayMock);
        when(mDisplayMock.getDisplayId()).thenReturn(0);
        sCarUiImeWideScreenController.setContentAreaSurfaceView(mContentAreaSurfaceViewMock);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.onStartInputView(mEditorInfoMock, mInputConnectionMock,
                    TEST);

            Bundle bundle = new Bundle();
            bundle.putInt(WIDE_SCREEN_EXTRACTED_TEXT_ICON_RES_ID, 123);

            sCarUiImeWideScreenController.onAppPrivateCommand(WIDE_SCREEN_SEARCH_RESULTS, bundle);
        });

        onView(withId(R.id.car_ui_wideScreenExtractedTextIcon)).check(
                matches(withEffectiveVisibility(
                        Visibility.GONE)));
    }

    @Test
    public void setExtractViewShown_shouldHideExtractView() {
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.setExtractViewShown(false);
        });
        onView(withId(R.id.car_ui_fullscreenArea)).check(matches(not(isDisplayed())));

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.setExtractViewShown(true);
        });
    }

    @Test
    public void setExtractViewShown_shouldShowExtractView() {
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.setExtractViewShown(true);
        });

        onView(withId(R.id.car_ui_fullscreenArea)).check(matches(isDisplayed()));
    }

    @Test
    public void onAppPrivateCommand_shouldShowSearchResults() {
        ContentResolver cr = ApplicationProvider.getApplicationContext().getContentResolver();
        cr.insert(Uri.parse(AUTHORITY), getRecord());

        when(mInputMethodServiceMock.getWindow()).thenReturn(mDialogMock);
        when(mDialogMock.getWindow()).thenReturn(mWindowMock);

        CharSequence charSequence = "test";
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            doReturn(mHasTokenMock).when(mContentAreaSurfaceViewMock).getHostToken();
            sCarUiImeWideScreenController.setContentAreaSurfaceView(mContentAreaSurfaceViewMock);
            sCarUiImeWideScreenController
                    .onStartInputView(mEditorInfoMock, mInputConnectionMock, charSequence);
        });

        CarUiImeWideScreenController spy = Mockito.spy(sCarUiImeWideScreenController);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            doReturn("com.android.car.ui.test").when(spy).getPackageName(ArgumentMatchers.any());
            doNothing().when(spy).onItemClicked(ArgumentMatchers.any());
            Bundle bundle = new Bundle();
            spy.onAppPrivateCommand(WIDE_SCREEN_SEARCH_RESULTS, bundle);
        });

        onView(withId(R.id.car_ui_wideScreenSearchResultList)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_wideScreenSearchResultList))
                .check(matches(atPosition(0, hasDescendant(withText("Title")))));
        onView(withId(R.id.car_ui_wideScreenSearchResultList))
                .check(matches(atPosition(0, hasDescendant(withText("SubTitle")))));
        onView(withId(R.id.car_ui_wideScreenSearchResultList))
                .perform(RecyclerViewActions.actionOnItem(hasDescendant(withText("Title")),
                        click()));

        verify(spy, times(1)).onItemClicked("1");
    }

    @Test
    public void onAppPrivateCommand_shouldShowSurfaceView() {
        when(mInputMethodServiceMock.getWindow()).thenReturn(mDialogMock);
        when(mDialogMock.getWindow()).thenReturn(mWindowMock);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            sCarUiImeWideScreenController.onStartInputView(mEditorInfoMock, mInputConnectionMock,
                    TEST);

            Bundle bundle = new Bundle();
            // can't be inline to avoid ambiguity of constructors in SurfaceControlViewHost.
            IBinder hostToken = null;
            SurfaceControlViewHost surfaceControlViewHost = new SurfaceControlViewHost(mContext,
                    null, hostToken);
            bundle.putParcelable(CONTENT_AREA_SURFACE_PACKAGE,
                    surfaceControlViewHost.getSurfacePackage());
            sCarUiImeWideScreenController.onAppPrivateCommand(WIDE_SCREEN_ACTION, bundle);
        });

        onView(withId(R.id.car_ui_ime_surface)).check(matches(isDisplayed()));
    }

    @Test
    public void onEvaluateFullscreenMode_shouldReturnTrue() {
        boolean result = sCarUiImeWideScreenController.onEvaluateFullscreenMode(false);
        assertThat(result, is(true));
    }

    @Test
    public void onEvaluateFullscreenMode_inWidescreenMode_shouldReturnTrue() {
        boolean result = sCarUiImeWideScreenController.onEvaluateFullscreenMode(true);
        assertThat(result, is(true));
    }

    private ContentValues getRecord() {
        ContentValues values = new ContentValues();
        int id = 1;
        values.put(ITEM_ID, id);
        values.put(SECONDARY_IMAGE_ID, id);
        values.put(TITLE, "Title");
        values.put(SUBTITLE, "SubTitle");

        String idString = String.valueOf(id);
        values.put(SearchResultsProvider.ITEM_ID, id);
        values.put(SearchResultsProvider.SECONDARY_IMAGE_ID, id);
        BitmapDrawable icon = (BitmapDrawable) mContext.getResources()
                .getDrawable(R.drawable.ic_launcher);
        values.put(SearchResultsProvider.PRIMARY_IMAGE_BLOB,
                icon != null ? bitmapToByteArray(icon.getBitmap()) : null);
        BitmapDrawable supplementalIcon = (BitmapDrawable) mContext.getResources()
                .getDrawable(R.drawable.ic_launcher);
        values.put(SearchResultsProvider.SECONDARY_IMAGE_BLOB,
                supplementalIcon != null ? bitmapToByteArray(supplementalIcon.getBitmap())
                        : null);
        return values;
    }

    private byte[] bitmapToByteArray(Bitmap bitmap) {
        Parcel parcel = Parcel.obtain();
        bitmap.writeToParcel(parcel, 0);
        byte[] bytes = parcel.marshall();
        parcel.recycle();
        return bytes;
    }

    private static Matcher<View> atPosition(final int position,
            @NonNull final Matcher<View> itemMatcher) {
        checkNotNull(itemMatcher);
        return new BoundedMatcher<View, RecyclerView>(RecyclerView.class) {
            @Override
            public void describeTo(Description description) {
                description.appendText("has item at position " + position + ": ");
                itemMatcher.describeTo(description);
            }

            @Override
            protected boolean matchesSafely(final RecyclerView view) {
                RecyclerView.ViewHolder viewHolder = view.findViewHolderForAdapterPosition(
                        position);
                if (viewHolder == null) {
                    // has no item on such position
                    return false;
                }
                return itemMatcher.matches(viewHolder.itemView);
            }
        };
    }

    private CarUiImeWideScreenController getController() {
        return new CarUiImeWideScreenController(mActivity, mInputMethodServiceMock) {
            @Override
            public boolean isWideScreenMode() {
                return true;
            }

            @Override
            ExtractEditText getExtractEditText() {
                FrameLayout parent = new FrameLayout(mContext);
                ExtractEditText extractEditText = new ExtractEditText(mContext);
                parent.addView(extractEditText);
                return extractEditText;
            }
        };
    }
}

/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.managedprovisioning.common;

import static android.graphics.PorterDuff.Mode.SRC_ATOP;

import static androidx.test.espresso.matcher.ViewMatchers.assertThat;

import static org.hamcrest.CoreMatchers.equalTo;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.PorterDuffColorFilter;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.widget.ImageView;

import com.android.managedprovisioning.R;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;

public class CustomizationVerifier {
    private final Activity mActivity;

    public CustomizationVerifier(Activity activity) {
        mActivity = activity;
    }

    public void assertDefaultLogoCorrect(int targetColor) {
        Drawable actualLogo = extractLogo();
        Drawable expectedLogo = makeDefaultLogo(targetColor);
        assertThat(actualLogo.getConstantState(), equalTo(expectedLogo.getConstantState()));
        assertThat(actualLogo.getColorFilter(), equalTo(expectedLogo.getColorFilter()));
    }

    public void assertCustomLogoCorrect(Bitmap targetLogo) {
        Bitmap actualLogo = ((BitmapDrawable) extractLogo()).getBitmap();
        assertThat(targetLogo, bitmapEqualTo(actualLogo));
    }

    private Matcher<Bitmap> bitmapEqualTo(Bitmap expected) {
        return new BaseMatcher<Bitmap>() {
            @Override
            public void describeTo(Description description) {
                description.appendText("Bitmap different from expected.");
            }

            @Override
            public boolean matches(Object actual) {
                return expected.sameAs((Bitmap) actual);
            }
        };
    }

    private Drawable makeDefaultLogo(int color) {
        Drawable logo = mActivity.getDrawable(R.drawable.ic_enterprise_blue_24dp);
        logo.setColorFilter(new PorterDuffColorFilter(color, SRC_ATOP));
        return logo;
    }

    private Drawable extractLogo() {
        return ((ImageView) mActivity.findViewById(R.id.sud_layout_icon)).getDrawable();
    }
}

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

import static androidx.test.espresso.action.ViewActions.actionWithAssertions;

import android.text.Layout;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import androidx.test.espresso.UiController;
import androidx.test.espresso.ViewAction;
import androidx.test.espresso.action.CoordinatesProvider;
import androidx.test.espresso.action.GeneralClickAction;
import androidx.test.espresso.action.PrecisionDescriber;
import androidx.test.espresso.action.Press;
import androidx.test.espresso.action.Tap;
import androidx.test.espresso.action.Tapper;

import org.hamcrest.Matcher;

import java.util.Arrays;

/**
 * Espresso utils to perform actions on a TextView.
 */
public final class TextViewActions {
    private static final String TAG = "TextViewActions";

    /**
     * Tap on the text at the given character index.
     */
    public static ViewAction tapOnTextAtIndex(int index) {
        return actionWithAssertions(
                new ViewClickAction(Tap.SINGLE, new TextCoordinates(index), Press.FINGER));
    }

    public static ViewAction longTapOnTextAtIndex(int index) {
        return actionWithAssertions(
                new ViewClickAction(Tap.LONG, new TextCoordinates(index), Press.FINGER));
    }

    private static final class ViewClickAction implements ViewAction {
        private final GeneralClickAction mGeneralClickAction;

        public ViewClickAction(
                Tapper tapper,
                CoordinatesProvider coordinatesProvider,
                PrecisionDescriber precisionDescriber) {
            mGeneralClickAction = new GeneralClickAction(tapper, coordinatesProvider,
                    precisionDescriber);
        }

        @Override
        public Matcher<View> getConstraints() {
            return mGeneralClickAction.getConstraints();
        }

        @Override
        public String getDescription() {
            return mGeneralClickAction.getDescription();
        }

        @Override
        public void perform(UiController uiController, View view) {
            mGeneralClickAction.perform(uiController, view);
        }
    }

    private static final class TextCoordinates implements CoordinatesProvider {

        private final int mIndex;

        public TextCoordinates(int index) {
            mIndex = index;
        }

        @Override
        public float[] calculateCoordinates(View view) {
            TextView textView = (TextView) view;
            final Layout layout = textView.getLayout();
            final int line = layout.getLineForOffset(mIndex);
            final int[] xy = new int[2];
            textView.getLocationOnScreen(xy);
            float[] coordinates = new float[]{
                    layout.getPrimaryHorizontal(mIndex)
                            + textView.getTotalPaddingLeft()
                            - textView.getScrollX()
                            + xy[0],
                    layout.getLineTop(line) + textView.getTotalPaddingTop() - textView.getScrollY()
                            + xy[1]};
            Log.d(TAG, "calculateCoordinates: " + Arrays.toString(coordinates));
            return coordinates;
        }
    }

    private TextViewActions() {
    }
}
/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.view.inputmethod.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.os.Bundle;
import android.os.LocaleList;
import android.os.Parcel;
import android.test.MoreAsserts;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.util.StringBuilderPrinter;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.SurroundingText;

import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class EditorInfoTest {
    private static final int TEST_TEXT_LENGTH = 2048;
    /** A text with 1 million chars! This is way too long. */
    private static final int OVER_SIZED_TEXT_LENGTH = 1 * 1024 * 1024;
    /** To get the longest available text from getInitialText methods. */
    private static final int REQUEST_LONGEST_AVAILABLE_TEXT = OVER_SIZED_TEXT_LENGTH; //

    @Test
    public void testEditorInfo() {
        EditorInfo info = new EditorInfo();
        CharSequence testInitialText = createTestText(TEST_TEXT_LENGTH);

        info.actionId = 1;
        info.actionLabel = "actionLabel";
        info.fieldId = 2;
        info.fieldName = "fieldName";
        info.hintText = "hintText";
        info.imeOptions = EditorInfo.IME_FLAG_NO_ENTER_ACTION;
        info.initialCapsMode = TextUtils.CAP_MODE_CHARACTERS;
        info.initialSelEnd = 10;
        info.initialSelStart = 0;
        info.inputType = EditorInfo.TYPE_MASK_CLASS;
        info.label = "label";
        info.packageName = "android.view.cts";
        info.privateImeOptions = "privateIme";
        Bundle b = new Bundle();
        info.setInitialSurroundingText(testInitialText);
        String key = "bundleKey";
        String value = "bundleValue";
        b.putString(key, value);
        info.extras = b;
        info.hintLocales = LocaleList.forLanguageTags("en-PH,en-US");
        info.contentMimeTypes = new String[]{"image/gif", "image/png"};

        assertEquals(0, info.describeContents());

        Parcel p = Parcel.obtain();
        info.writeToParcel(p, 0);
        p.setDataPosition(0);
        EditorInfo targetInfo = EditorInfo.CREATOR.createFromParcel(p);
        p.recycle();
        assertEquals(info.actionId, targetInfo.actionId);
        assertEquals(info.fieldId, targetInfo.fieldId);
        assertEquals(info.fieldName, targetInfo.fieldName);
        assertEquals(info.imeOptions, targetInfo.imeOptions);
        assertEquals(info.initialCapsMode, targetInfo.initialCapsMode);
        assertEquals(info.initialSelEnd, targetInfo.initialSelEnd);
        assertEquals(info.initialSelStart, targetInfo.initialSelStart);
        assertEquals(info.inputType, targetInfo.inputType);
        assertEquals(info.packageName, targetInfo.packageName);
        assertEquals(info.privateImeOptions, targetInfo.privateImeOptions);
        assertTrue(TextUtils.equals(testInitialText, concatInitialSurroundingText(targetInfo)));
        assertEquals(info.hintText.toString(), targetInfo.hintText.toString());
        assertEquals(info.actionLabel.toString(), targetInfo.actionLabel.toString());
        assertEquals(info.label.toString(), targetInfo.label.toString());
        assertEquals(info.extras.getString(key), targetInfo.extras.getString(key));
        assertEquals(info.hintLocales, targetInfo.hintLocales);
        MoreAsserts.assertEquals(info.contentMimeTypes, targetInfo.contentMimeTypes);

        StringBuilder sb = new StringBuilder();
        StringBuilderPrinter sbPrinter = new StringBuilderPrinter(sb);
        String prefix = "TestEditorInfo";
        info.dump(sbPrinter, prefix);

        assertFalse(TextUtils.isEmpty(sb.toString()));
        assertFalse(sb.toString().contains(testInitialText));
    }

    @Test
    public void testNullHintLocals() {
        EditorInfo info = new EditorInfo();
        info.hintLocales = null;
        Parcel p = Parcel.obtain();
        info.writeToParcel(p, 0);
        p.setDataPosition(0);
        EditorInfo targetInfo = EditorInfo.CREATOR.createFromParcel(p);
        p.recycle();
        assertNull(targetInfo.hintLocales);
    }

    @Test
    public void testInitialSurroundingText_nullInput_throwsException() {
        final EditorInfo info = new EditorInfo();

        try {
            info.setInitialSurroundingText(null);
            fail("Shall not take null input");
        } catch (NullPointerException expected) {
            // Expected behavior, nothing to do.
        }
    }

    @Test
    public void testInitialSurroundingText_passwordTypes_notObtain() {
        final EditorInfo info = new EditorInfo();
        final CharSequence testInitialText = createTestText(/* size= */ 10);
        info.initialSelStart = 1;
        info.initialSelEnd = 2;

        // Text password type
        info.inputType = (EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_PASSWORD);

        info.setInitialSurroundingText(testInitialText);

        assertExpectedTextLength(info,
                /* expectBeforeCursorLength= */null,
                /* expectSelectionLength= */null,
                /* expectAfterCursorLength= */null,
                /* expectSurroundingText= */null);

        // Web password type
        info.inputType = (EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_WEB_PASSWORD);

        info.setInitialSurroundingText(testInitialText);

        assertExpectedTextLength(info,
                /* expectBeforeCursorLength= */null,
                /* expectSelectionLength= */null,
                /* expectAfterCursorLength= */null,
                /* expectSurroundingText= */null);

        // Number password type
        info.inputType = (EditorInfo.TYPE_CLASS_NUMBER | EditorInfo.TYPE_NUMBER_VARIATION_PASSWORD);

        info.setInitialSurroundingText(testInitialText);

        assertExpectedTextLength(info,
                /* expectBeforeCursorLength= */null,
                /* expectSelectionLength= */null,
                /* expectAfterCursorLength= */null,
                /* expectSurroundingText= */null);
    }

    @Test
    public void testInitialSurroundingText_cursorAtHead_emptyBeforeCursorText() {
        final EditorInfo info = new EditorInfo();
        final CharSequence testText = createTestText(TEST_TEXT_LENGTH);
        final int selLength = 10;
        info.initialSelStart = 0;
        info.initialSelEnd = info.initialSelStart + selLength;
        final int expectedTextBeforeCursorLength = 0;
        final int expectedTextAfterCursorLength = testText.length() - selLength;
        final SurroundingText expectedSurroundingText =
                new SurroundingText(testText, info.initialSelStart, info.initialSelEnd, 0);

        info.setInitialSurroundingText(testText);

        assertExpectedTextLength(info, expectedTextBeforeCursorLength, selLength,
                expectedTextAfterCursorLength, expectedSurroundingText);
    }

    @Test
    public void testInitialSurroundingText_cursorAtTail_emptyAfterCursorText() {
        final EditorInfo info = new EditorInfo();
        final CharSequence testText = createTestText(TEST_TEXT_LENGTH);
        final int selLength = 10;
        info.initialSelStart = testText.length() - selLength;
        info.initialSelEnd = testText.length();
        final int expectedTextBeforeCursorLength = testText.length() - selLength;
        final int expectedTextAfterCursorLength = 0;
        final SurroundingText expectedSurroundingText =
                new SurroundingText(testText, info.initialSelStart, info.initialSelEnd, 0);

        info.setInitialSurroundingText(testText);

        assertExpectedTextLength(info, expectedTextBeforeCursorLength, selLength,
                expectedTextAfterCursorLength, expectedSurroundingText);
    }

    @Test
    public void testInitialSurroundingText_noSelection_emptySelectionText() {
        final EditorInfo info = new EditorInfo();
        final CharSequence testText = createTestText(TEST_TEXT_LENGTH);
        final int selLength = 0;
        info.initialSelStart = 0;
        info.initialSelEnd = info.initialSelStart + selLength;
        final int expectedTextBeforeCursorLength = 0;
        final int expectedTextAfterCursorLength = testText.length();
        final SurroundingText expectedSurroundingText =
                new SurroundingText(testText, info.initialSelStart, info.initialSelEnd, 0);

        info.setInitialSurroundingText(testText);

        assertExpectedTextLength(info, expectedTextBeforeCursorLength, selLength,
                expectedTextAfterCursorLength, expectedSurroundingText);
    }

    @Test
    public void testInitialSurroundingText_overSizedSelection_keepsBeforeAfterTextValid() {
        final EditorInfo info = new EditorInfo();
        final CharSequence testText = createTestText(OVER_SIZED_TEXT_LENGTH);
        final int selLength = OVER_SIZED_TEXT_LENGTH - 2;
        info.initialSelStart = 1;
        info.initialSelEnd = info.initialSelStart + selLength;
        final int expectedTextBeforeCursorLength = 1;
        final int expectedTextAfterCursorLength = 1;
        final int offset = info.initialSelStart - expectedTextBeforeCursorLength;
        final CharSequence beforeCursor = testText.subSequence(offset,
                offset + expectedTextBeforeCursorLength);
        final CharSequence afterCursor = testText.subSequence(info.initialSelEnd,
                testText.length());
        final CharSequence surroundingText = TextUtils.concat(beforeCursor, afterCursor);
        final SurroundingText expectedSurroundingText =
                new SurroundingText(surroundingText, info.initialSelStart, info.initialSelStart, 0);

        info.setInitialSurroundingText(testText);

        assertExpectedTextLength(info, expectedTextBeforeCursorLength,
                /* expectSelectionLength= */null, expectedTextAfterCursorLength,
                expectedSurroundingText);

    }

    @Test
    public void testInitialSurroundingSubText_keepsOriginalCursorPosition() {
        final EditorInfo info = new EditorInfo();
        final String prefixString = "prefix";
        final CharSequence subText = createTestText(TEST_TEXT_LENGTH);
        final CharSequence originalText = TextUtils.concat(prefixString, subText);
        final int selLength = 2;
        info.initialSelStart = originalText.length() / 2;
        info.initialSelEnd = info.initialSelStart + selLength;
        final CharSequence expectedTextBeforeCursor = createExpectedText(/* startNumber= */0,
                info.initialSelStart - prefixString.length());
        final CharSequence expectedSelectedText = createExpectedText(
                info.initialSelStart - prefixString.length(), selLength);
        final CharSequence expectedTextAfterCursor = createExpectedText(
                info.initialSelEnd - prefixString.length(),
                originalText.length() - info.initialSelEnd);
        final SurroundingText expectedSurroundingText = new SurroundingText(
                TextUtils.concat(expectedTextBeforeCursor, expectedSelectedText,
                        expectedTextAfterCursor),
                info.initialSelStart - prefixString.length(),
                info.initialSelStart - prefixString.length() + selLength,
                prefixString.length());

        info.setInitialSurroundingSubText(subText, prefixString.length());

        assertTrue(TextUtils.equals(expectedTextBeforeCursor,
                info.getInitialTextBeforeCursor(REQUEST_LONGEST_AVAILABLE_TEXT,
                        InputConnection.GET_TEXT_WITH_STYLES)));
        assertTrue(TextUtils.equals(expectedSelectedText,
                info.getInitialSelectedText(InputConnection.GET_TEXT_WITH_STYLES)));
        assertTrue(TextUtils.equals(expectedTextAfterCursor,
                info.getInitialTextAfterCursor(REQUEST_LONGEST_AVAILABLE_TEXT,
                        InputConnection.GET_TEXT_WITH_STYLES)));
        SurroundingText surroundingText = info.getInitialSurroundingText(
                REQUEST_LONGEST_AVAILABLE_TEXT,
                REQUEST_LONGEST_AVAILABLE_TEXT,
                InputConnection.GET_TEXT_WITH_STYLES);
        assertNotNull(surroundingText);
        assertTrue(TextUtils.equals(expectedSurroundingText.getText(), surroundingText.getText()));
        assertEquals(expectedSurroundingText.getSelectionStart(),
                surroundingText.getSelectionStart());
        assertEquals(expectedSurroundingText.getSelectionEnd(), surroundingText.getSelectionEnd());
    }

    private static void assertExpectedTextLength(EditorInfo editorInfo,
            Integer expectBeforeCursorLength, Integer expectSelectionLength,
            Integer expectAfterCursorLength,
            SurroundingText expectSurroundingText) {
        final CharSequence textBeforeCursor =
                editorInfo.getInitialTextBeforeCursor(REQUEST_LONGEST_AVAILABLE_TEXT,
                        InputConnection.GET_TEXT_WITH_STYLES);
        final CharSequence selectedText =
                editorInfo.getInitialSelectedText(InputConnection.GET_TEXT_WITH_STYLES);
        final CharSequence textAfterCursor =
                editorInfo.getInitialTextAfterCursor(REQUEST_LONGEST_AVAILABLE_TEXT,
                        InputConnection.GET_TEXT_WITH_STYLES);
        final SurroundingText surroundingText = editorInfo.getInitialSurroundingText(
                REQUEST_LONGEST_AVAILABLE_TEXT,
                REQUEST_LONGEST_AVAILABLE_TEXT,
                InputConnection.GET_TEXT_WITH_STYLES);

        if (expectBeforeCursorLength == null) {
            assertNull(textBeforeCursor);
        } else {
            assertEquals(expectBeforeCursorLength.intValue(), textBeforeCursor.length());
        }

        if (expectSelectionLength == null) {
            assertNull(selectedText);
        } else {
            assertEquals(expectSelectionLength.intValue(), selectedText.length());
        }

        if (expectAfterCursorLength == null) {
            assertNull(textAfterCursor);
        } else {
            assertEquals(expectAfterCursorLength.intValue(), textAfterCursor.length());
        }

        if (expectSurroundingText == null) {
            assertNull(surroundingText);
        } else {
            assertTrue(TextUtils.equals(
                    expectSurroundingText.getText(), surroundingText.getText()));
            assertEquals(expectSurroundingText.getSelectionStart(),
                    surroundingText.getSelectionStart());
            assertEquals(expectSurroundingText.getSelectionEnd(),
                    surroundingText.getSelectionEnd());
            assertEquals(expectSurroundingText.getOffset(), surroundingText.getOffset());
        }
    }

    private static CharSequence createTestText(int size) {
        final SpannableStringBuilder builder = new SpannableStringBuilder();
        for (int i = 0; i < size; i++) {
            builder.append(Integer.toString(i % 10));
        }
        return builder;
    }

    private static CharSequence createExpectedText(int startNumber, int length) {
        final SpannableStringBuilder builder = new SpannableStringBuilder();
        for (int i = startNumber; i < startNumber + length; i++) {
            builder.append(Integer.toString(i % 10));
        }
        return builder;
    }

    private static CharSequence concatInitialSurroundingText(EditorInfo info) {
        final CharSequence textBeforeCursor =
                nullToEmpty(info.getInitialTextBeforeCursor(REQUEST_LONGEST_AVAILABLE_TEXT,
                        InputConnection.GET_TEXT_WITH_STYLES));
        final CharSequence selectedText =
                nullToEmpty(info.getInitialSelectedText(InputConnection.GET_TEXT_WITH_STYLES));
        final CharSequence textAfterCursor =
                nullToEmpty(info.getInitialTextAfterCursor(REQUEST_LONGEST_AVAILABLE_TEXT,
                        InputConnection.GET_TEXT_WITH_STYLES));

        return TextUtils.concat(textBeforeCursor, selectedText, textAfterCursor);
    }

    private static CharSequence nullToEmpty(CharSequence source) {
        return (source == null) ? new SpannableStringBuilder("") : source;
    }
}

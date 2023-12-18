/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.telephony.common;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.database.Cursor;
import android.provider.ContactsContract;
import android.telephony.TelephonyManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RobolectricTestRunner;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

@RunWith(RobolectricTestRunner.class)
public class ContactTest {

    private static final int DISPLAY_NAME_COLUMN = 1;
    private static final int MIMETYPE_COLUMN = 2;
    private static final int LOOK_UP_KEY_COLUMN = 3;
    private static final int PRIMARY_KEY_COLUMN = 4;
    private static final String PHONEBOOK_LABEL = "phonebook_label";
    private static final int PHONEBOOK_LABEL_COLUMN = 5;

    private static final String EMPTY_MIME = "";
    private static final String DEFAULT_LOOK_UP_KEY = "PRIMARY";
    private static final String NULL_NAME = null;
    private static final String EMPTY_NAME = "";
    private static final String LETTER_NAME_1 = "test";
    private static final String LETTER_NAME_2 = "ta";
    private static final String DIGIT_NAME_1 = "123";
    private static final String DIGIT_NAME_2 = "321";
    private static final String SPEC_CHAR_NAME = "-";

    private static final int COMPARE_RESULT_EQUAL = 0;
    private static final int COMPARE_RESULT_SMALLER = -1;
    private static final int COMPARE_RESULT_LARGER = 1;

    @Mock
    private Context mMockContext;
    @Mock
    private Cursor mMockCursor;
    @Mock
    private TelephonyManager mMockTelephonyManager;

    private Contact mNullName;
    private Contact mEmptyName;
    private Contact mLetterName1;
    private Contact mLetterName2;
    private Contact mDigitName1;
    private Contact mDigitName2;
    private Contact mSpecCharName;

    private int mDisplayNameColumn;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);

        when(mMockCursor.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME)).thenReturn(
                DISPLAY_NAME_COLUMN);
        when(mMockCursor.getColumnIndex(ContactsContract.RawContacts.SORT_KEY_PRIMARY)).thenReturn(
                PRIMARY_KEY_COLUMN);
        when(mMockCursor.getColumnIndex(ContactsContract.Data.MIMETYPE)).thenReturn(
                MIMETYPE_COLUMN);
        when(mMockCursor.getString(MIMETYPE_COLUMN)).thenReturn(EMPTY_MIME);
        when(mMockCursor.getColumnIndex(ContactsContract.Data.LOOKUP_KEY)).thenReturn(
                LOOK_UP_KEY_COLUMN);
        when(mMockCursor.getString(LOOK_UP_KEY_COLUMN)).thenReturn(DEFAULT_LOOK_UP_KEY);
        when(mMockCursor.getColumnIndex(PHONEBOOK_LABEL)).thenReturn(
                PHONEBOOK_LABEL_COLUMN);

        when(mMockContext.getSystemService(Context.TELEPHONY_SERVICE)).thenReturn(
                mMockTelephonyManager);
        when(mMockTelephonyManager.getSimCountryIso()).thenReturn("");

        when(mMockCursor.getString(PHONEBOOK_LABEL_COLUMN)).thenReturn("");
        when(mMockCursor.getString(PRIMARY_KEY_COLUMN)).thenReturn(NULL_NAME);
        mNullName = Contact.fromCursor(mMockContext, mMockCursor);
        when(mMockCursor.getString(PRIMARY_KEY_COLUMN)).thenReturn(EMPTY_NAME);
        mEmptyName = Contact.fromCursor(mMockContext, mMockCursor);

        when(mMockCursor.getString(PHONEBOOK_LABEL_COLUMN)).thenReturn("T");
        when(mMockCursor.getString(PRIMARY_KEY_COLUMN)).thenReturn(LETTER_NAME_1);
        mLetterName1 = Contact.fromCursor(mMockContext, mMockCursor);
        when(mMockCursor.getString(PRIMARY_KEY_COLUMN)).thenReturn(LETTER_NAME_2);
        mLetterName2 = Contact.fromCursor(mMockContext, mMockCursor);

        when(mMockCursor.getString(PHONEBOOK_LABEL_COLUMN)).thenReturn("#");
        when(mMockCursor.getString(PRIMARY_KEY_COLUMN)).thenReturn(DIGIT_NAME_1);
        mDigitName1 = Contact.fromCursor(mMockContext, mMockCursor);
        when(mMockCursor.getString(PRIMARY_KEY_COLUMN)).thenReturn(DIGIT_NAME_2);
        mDigitName2 = Contact.fromCursor(mMockContext, mMockCursor);

        when(mMockCursor.getString(PHONEBOOK_LABEL_COLUMN)).thenReturn("");
        when(mMockCursor.getString(PRIMARY_KEY_COLUMN)).thenReturn(SPEC_CHAR_NAME);
        mSpecCharName = Contact.fromCursor(mMockContext, mMockCursor);
    }

    @Test
    public void compareTo_TwoNullStrings_Equal() {
        // NULL == NULL
        int compareResult = mNullName.compareTo(mNullName);
        assertEquals(COMPARE_RESULT_EQUAL, compareResult);
    }

    @Test
    public void compareTo_TwoEmptyStrings_Equal() {
        // "" == NULL
        int compareResult = mEmptyName.compareTo(mEmptyName);
        assertEquals(COMPARE_RESULT_EQUAL, compareResult);
    }

    @Test
    public void compareTo_TwoLetterStrings_Larger() {
        // "test" > "ta"
        int compareResult = mLetterName1.compareTo(mLetterName2);
        assertEquals(COMPARE_RESULT_LARGER, compareResult);
    }

    @Test
    public void compareTo_TwoDigitStrings_Smaller() {
        // "123" < "321"
        int compareResult = mDigitName1.compareTo(mDigitName2);
        assertEquals(COMPARE_RESULT_SMALLER, compareResult);
    }

    @Test
    public void compareTo_LetterAndDigitStrings_Smaller() {
        // "test" < "123", because of Phonebook_label
        int compareResult = mLetterName1.compareTo(mDigitName1);
        assertEquals(COMPARE_RESULT_SMALLER, compareResult);
    }

    @Test
    public void compareTo_LetterAndSpecialCharStrings_Smaller() {
        // "test" <"-", because of Phonebook_label
        int compareResult = mLetterName1.compareTo(mSpecCharName);
        assertEquals(COMPARE_RESULT_SMALLER, compareResult);
    }

    @Test
    public void compareTo_LetterAndEmptyStrings_Smaller() {
        // "test" < "", because of Phonebook_label
        int compareResult = mLetterName1.compareTo(mEmptyName);
        assertEquals(COMPARE_RESULT_SMALLER, compareResult);
    }

    @Test
    public void compareTo_LetterAndNullStrings_Smaller() {
        // "test < NULL, because of Phonebook_label
        int compareResult = mLetterName1.compareTo(mNullName);
        assertEquals(COMPARE_RESULT_SMALLER, compareResult);
    }

    @Test
    public void compareTo_DigitAndSpecialCharStrings_Smaller() {
        // "123" < "-", because of Phonebook_label
        int compareResult = mDigitName1.compareTo(mSpecCharName);
        assertEquals(COMPARE_RESULT_SMALLER, compareResult);
    }

    @Test
    public void compareTo_DigitAndEmptyStrings_Smaller() {
        // "123" > "", because of Phonebook_label
        int compareResult = mDigitName1.compareTo(mEmptyName);
        assertEquals(COMPARE_RESULT_SMALLER, compareResult);
    }

    @Test
    public void compareTo_DigitAndNullStrings_Smaller() {
        // "123" < NULL, because of Phonebook_label
        int compareResult = mDigitName1.compareTo(mNullName);
        assertEquals(COMPARE_RESULT_SMALLER, compareResult);
    }

    @Test
    public void compareTo_SpecialCharAndEmptyStrings_Any() {
        // "-" > ""
        int compareResult = mSpecCharName.compareTo(mEmptyName);
        assertThat(compareResult).isAnyOf(COMPARE_RESULT_SMALLER, COMPARE_RESULT_EQUAL,
                COMPARE_RESULT_LARGER);
    }

    @Test
    public void compareTo_SpecialCharAndNullStrings_Any() {
        // "-" > NULL
        int compareResult = mSpecCharName.compareTo(mNullName);
        assertThat(compareResult).isAnyOf(COMPARE_RESULT_SMALLER, COMPARE_RESULT_EQUAL,
                COMPARE_RESULT_LARGER);
    }

    @Test
    public void compareTo_EmptyAndNullStrings_Equal() {
        // "" == NULL
        int compareResult = mEmptyName.compareTo(mNullName);
        assertEquals(COMPARE_RESULT_EQUAL, compareResult);
    }

    @Test
    public void sortContactTest() {
        List<Contact> expectedList = new ArrayList<>();
        expectedList.add(mLetterName2);
        expectedList.add(mLetterName1);
        expectedList.add(mDigitName1);
        expectedList.add(mDigitName2);
        expectedList.add(mEmptyName);
        expectedList.add(mSpecCharName);

        List<Contact> contactList = new ArrayList<>();
        contactList.add(mDigitName1);
        contactList.add(mSpecCharName);
        contactList.add(mLetterName1);
        contactList.add(mEmptyName);
        contactList.add(mLetterName2);
        contactList.add(mDigitName2);

        Collections.sort(contactList);
        assertArrayEquals(expectedList.toArray(), contactList.toArray());
    }

    @Test
    public void sortContactPrimaryTest() {
        when(mMockCursor.getString(PRIMARY_KEY_COLUMN)).thenReturn("tä");
        Contact letterName3 = Contact.fromCursor(mMockContext, mMockCursor);

        when(mMockCursor.getString(PRIMARY_KEY_COLUMN)).thenReturn("tb");
        Contact letterName4 = Contact.fromCursor(mMockContext, mMockCursor);

        List<Contact> expectedList = new ArrayList<>();
        expectedList.add(mLetterName2);
        expectedList.add(letterName3);
        expectedList.add(letterName4);
        expectedList.add(mLetterName1);
        expectedList.add(mDigitName1);
        expectedList.add(mDigitName2);
        expectedList.add(mEmptyName);
        expectedList.add(mSpecCharName);

        List<Contact> contactList = new ArrayList<>();
        contactList.add(mDigitName1);
        contactList.add(letterName4);
        contactList.add(mSpecCharName);
        contactList.add(mLetterName1);
        contactList.add(mEmptyName);
        contactList.add(mLetterName2);
        contactList.add(letterName3);
        contactList.add(mDigitName2);

        Collections.sort(contactList, (o1, o2) -> o1.compareBySortKeyPrimary(o2));
        assertArrayEquals(expectedList.toArray(), contactList.toArray());
    }

    @Test
    public void sortContactAlternativeTest() {
        int alternativeColumnIndex = 20;
        when(mMockCursor.getColumnIndex(ContactsContract.RawContacts.SORT_KEY_ALTERNATIVE))
                .thenReturn(alternativeColumnIndex);
        when(mMockCursor.getString(alternativeColumnIndex)).thenReturn("tä");
        Contact letterName3 = Contact.fromCursor(mMockContext, mMockCursor);

        when(mMockCursor.getString(alternativeColumnIndex)).thenReturn("tb");
        Contact letterName4 = Contact.fromCursor(mMockContext, mMockCursor);

        List<Contact> expectedList = new ArrayList<>();
        expectedList.add(mLetterName2);
        expectedList.add(letterName3);
        expectedList.add(letterName4);
        expectedList.add(mLetterName1);
        expectedList.add(mDigitName1);
        expectedList.add(mDigitName2);
        expectedList.add(mEmptyName);
        expectedList.add(mSpecCharName);

        List<Contact> contactList = new ArrayList<>();
        contactList.add(mDigitName1);
        contactList.add(letterName4);
        contactList.add(mSpecCharName);
        contactList.add(mLetterName1);
        contactList.add(mEmptyName);
        contactList.add(mLetterName2);
        contactList.add(letterName3);
        contactList.add(mDigitName2);

        Collections.sort(contactList, (o1, o2) -> o1.compareBySortKeyAlt(o2));
        assertArrayEquals(expectedList.toArray(), contactList.toArray());
    }
}

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

package com.android.car.telephony.common;

import static com.android.car.telephony.common.QueryParam.QueryBuilder.AND;
import static com.android.car.telephony.common.QueryParam.QueryBuilder.OR;
import static com.android.car.telephony.common.QueryParam.QueryBuilder.ORDER_ASC;
import static com.android.car.telephony.common.QueryParam.QueryBuilder.ORDER_DESC;

import static com.google.common.truth.Truth.assertThat;

import android.Manifest;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.CommonDataKinds.StructuredPostal;
import android.provider.ContactsContract.Data;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.telephony.common.QueryParam.QueryBuilder.Condition;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;

@RunWith(AndroidJUnit4.class)
public class QueryParamTest {
    private static final Uri CONTENT_URI = Data.CONTENT_URI;
    private static final String COLUMN_A = "COLUMN_A";
    private static final String COLUMN_B = "COLUMN_B";

    private static final String OP_EQUAL = "=";

    private static final String STRING_VALUE = "STRING_VALUE";
    private static final int INTEGER_VALUE = 100;

    @Test
    public void testContentUri() {
        QueryParam.QueryBuilder builder = new QueryParam.QueryBuilder(CONTENT_URI);

        QueryParam queryParam = builder.toQueryParam();

        assertThat(queryParam.mUri).isEqualTo(CONTENT_URI);
        assertThat(queryParam.mProjection).isNull();
        assertThat(queryParam.mSelection).isNull();
        assertThat(queryParam.mSelectionArgs).isEmpty();
        assertThat(queryParam.mOrderBy).isEmpty();
    }

    @Test
    public void testEmptyQueryParam() {
        QueryParam.QueryBuilder builder = new QueryParam.QueryBuilder(CONTENT_URI);
        builder.project(COLUMN_A);
        builder.project(COLUMN_B);

        QueryParam queryParam = builder.toQueryParam();

        assertThat(queryParam.mProjection).hasLength(2);
        assertThat(Arrays.stream(queryParam.mProjection).anyMatch(COLUMN_A::equals)).isTrue();
        assertThat(Arrays.stream(queryParam.mProjection).anyMatch(COLUMN_B::equals)).isTrue();
    }

    @Test
    public void testBuildQueryParam() {
        QueryParam expectedParams = new QueryParam(
                Data.CONTENT_URI,
                null,
                Data.MIMETYPE + " = ? OR "
                        + Data.MIMETYPE + " = ? OR "
                        + Data.MIMETYPE + " = ?",
                new String[]{
                        Phone.CONTENT_ITEM_TYPE,
                        StructuredName.CONTENT_ITEM_TYPE,
                        StructuredPostal.CONTENT_ITEM_TYPE},
                ContactsContract.Contacts.DISPLAY_NAME + " ASC",
                Manifest.permission.READ_CONTACTS);

        QueryParam builtQueryParam = new QueryParam.QueryBuilder(Data.CONTENT_URI)
                .projectAll()
                .where(Condition
                        .is(Data.MIMETYPE, "=", Phone.CONTENT_ITEM_TYPE)
                        .or(Data.MIMETYPE, "=", StructuredName.CONTENT_ITEM_TYPE)
                        .or(Data.MIMETYPE, "=", StructuredPostal.CONTENT_ITEM_TYPE))
                .orderAscBy(ContactsContract.Contacts.DISPLAY_NAME)
                .checkPermission(Manifest.permission.READ_CONTACTS)
                .toQueryParam();

        assertThat(builtQueryParam.mUri).isEqualTo(expectedParams.mUri);
        assertThat(builtQueryParam.mSelection).isEqualTo(expectedParams.mSelection);
        assertThat(builtQueryParam.mProjection).isEqualTo(expectedParams.mProjection);
        assertThat(Arrays.equals(builtQueryParam.mSelectionArgs, expectedParams.mSelectionArgs))
                .isTrue();
        assertThat(builtQueryParam.mOrderBy).isEqualTo(expectedParams.mOrderBy);
        assertThat(builtQueryParam.mPermission).isEqualTo(expectedParams.mPermission);
    }

    @Test
    public void testBuildProjection_selectAll() {
        QueryParam.QueryBuilder builder = new QueryParam.QueryBuilder(CONTENT_URI);
        builder.projectAll();

        QueryParam queryParam = builder.toQueryParam();

        assertThat(queryParam.mProjection).isNull();
    }

    @Test
    public void testBuildSelection() {
        QueryParam.QueryBuilder builder = new QueryParam.QueryBuilder(CONTENT_URI);
        builder.where(Condition.is(COLUMN_A, OP_EQUAL, STRING_VALUE));

        QueryParam queryParam = builder.toQueryParam();

        assertThat(queryParam.mSelection)
                .isEqualTo(Condition.toSelectionString(COLUMN_A, OP_EQUAL));
        assertThat(queryParam.mSelectionArgs[0]).isEqualTo(STRING_VALUE);
    }

    @Test
    public void testBuildSelection_andCondition() {
        QueryParam.QueryBuilder builder = new QueryParam.QueryBuilder(CONTENT_URI);
        builder.where(Condition
                .is(COLUMN_A, OP_EQUAL, STRING_VALUE)
                .and(COLUMN_B, OP_EQUAL, INTEGER_VALUE));

        QueryParam queryParam = builder.toQueryParam();

        String expectedSelection = Condition.toSelectionString(COLUMN_A, OP_EQUAL);
        expectedSelection += Condition.wrapWithSpaces(AND)
                + Condition.toSelectionString(COLUMN_B, OP_EQUAL);
        assertThat(queryParam.mSelection).isEqualTo(expectedSelection);
        assertThat(queryParam.mSelectionArgs).hasLength(2);
        assertThat(queryParam.mSelectionArgs[0]).isEqualTo(STRING_VALUE);
        assertThat(queryParam.mSelectionArgs[1]).isEqualTo(Integer.toString(INTEGER_VALUE));
    }

    @Test
    public void testBuildSelection_orCondition() {
        QueryParam.QueryBuilder builder = new QueryParam.QueryBuilder(CONTENT_URI);
        builder.where(Condition
                .is(COLUMN_A, OP_EQUAL, STRING_VALUE)
                .or(COLUMN_B, OP_EQUAL, INTEGER_VALUE));

        QueryParam queryParam = builder.toQueryParam();

        String expectedSelection = Condition.toSelectionString(COLUMN_A, OP_EQUAL);
        expectedSelection += Condition.wrapWithSpaces(OR)
                + Condition.toSelectionString(COLUMN_B, OP_EQUAL);
        assertThat(queryParam.mSelection).isEqualTo(expectedSelection);
        assertThat(queryParam.mSelectionArgs).hasLength(2);
        assertThat(queryParam.mSelectionArgs[0]).isEqualTo(STRING_VALUE);
        assertThat(queryParam.mSelectionArgs[1]).isEqualTo(Integer.toString(INTEGER_VALUE));
    }

    @Test
    public void testOrderAscBy() {
        QueryParam.QueryBuilder builder = new QueryParam.QueryBuilder(CONTENT_URI);
        builder.orderAscBy(COLUMN_A);

        QueryParam queryParam = builder.toQueryParam();

        String expectedOrder = COLUMN_A + " " + ORDER_ASC;
        assertThat(queryParam.mOrderBy).isEqualTo(expectedOrder);
    }

    @Test
    public void testOrderBy_twoOrder() {
        QueryParam.QueryBuilder builder = new QueryParam.QueryBuilder(CONTENT_URI);
        builder.orderAscBy(COLUMN_A);
        builder.orderDescBy(COLUMN_B);

        QueryParam queryParam = builder.toQueryParam();

        String expectedOrder = COLUMN_A + " " + ORDER_ASC;
        expectedOrder += "," + COLUMN_B + " " + ORDER_DESC;
        assertThat(queryParam.mOrderBy).isEqualTo(expectedOrder);
    }
}


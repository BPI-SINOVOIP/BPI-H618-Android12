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

package android.autofillservice.cts.unittests;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;
import static org.testng.Assert.assertThrows;

import static java.util.Arrays.asList;
import static java.util.Collections.singletonList;

import android.app.slice.Slice;
import android.app.slice.SliceSpec;
import android.content.ClipData;
import android.content.Intent;
import android.content.IntentSender;
import android.net.Uri;
import android.os.Parcel;
import android.platform.test.annotations.AppModeFull;
import android.service.autofill.Dataset;
import android.service.autofill.InlinePresentation;
import android.util.Size;
import android.view.autofill.AutofillId;
import android.view.autofill.AutofillValue;
import android.widget.RemoteViews;
import android.widget.inline.InlinePresentationSpec;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.regex.Pattern;

@RunWith(AndroidJUnit4.class)
@AppModeFull(reason = "Unit test")
public class DatasetTest {

    private final AutofillId mId = new AutofillId(42);
    private final AutofillId mId2 = new AutofillId(43);
    private final AutofillValue mValue = AutofillValue.forText("ValuableLikeGold");
    private final Pattern mFilter = Pattern.compile("whatever");
    private final InlinePresentation mInlinePresentation = new InlinePresentation(
            new Slice.Builder(new Uri.Builder().appendPath("DatasetTest").build(),
                    new SliceSpec("DatasetTest", 1)).build(),
            new InlinePresentationSpec.Builder(new Size(10, 10),
                    new Size(50, 50)).build(), /* pinned= */ false);
    private final ClipData mContent = new ClipData("sample label", new String[] {"image/png"},
            new ClipData.Item("content://example/0"));
    private final IntentSender mAuth = mock(IntentSender.class);

    private final RemoteViews mPresentation = mock(RemoteViews.class);

    @Test
    public void testBuilder_nullPresentation() {
        assertThrows(NullPointerException.class, () -> new Dataset.Builder((RemoteViews) null));
    }

    @Test
    public void testBuilder_nullInlinePresentation() {
        assertThrows(NullPointerException.class,
                () -> new Dataset.Builder((InlinePresentation) null));
    }

    @Test
    public void testBuilder_validPresentations() {
        assertThat(new Dataset.Builder(mPresentation)).isNotNull();
        assertThat(new Dataset.Builder(mInlinePresentation)).isNotNull();
    }

    @Test
    public void testBuilder_setNullInlinePresentation() {
        final Dataset.Builder builder = new Dataset.Builder(mPresentation);
        assertThrows(NullPointerException.class, () -> builder.setInlinePresentation(null));
    }

    @Test
    public void testBuilder_setInlinePresentation() {
        assertThat(new Dataset.Builder().setInlinePresentation(mInlinePresentation)).isNotNull();
    }

    @Test
    public void testBuilder_setValueNullId() {
        final Dataset.Builder builder = new Dataset.Builder(mPresentation);
        assertThrows(NullPointerException.class, () -> builder.setValue(null, mValue));
    }

    @Test
    public void testBuilder_setValueWithoutPresentation() {
        // Just assert that it builds without throwing an exception.
        assertThat(new Dataset.Builder().setValue(mId, mValue).build()).isNotNull();
    }

    @Test
    public void testBuilder_setValueWithNullPresentation() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(NullPointerException.class, () -> builder.setValue(mId, mValue,
                (RemoteViews) null));
    }

    @Test
    public void testBuilder_setValueWithBothPresentation_nullPresentation() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(NullPointerException.class, () -> builder.setValue(mId, mValue,
                null, mInlinePresentation));
    }

    @Test
    public void testBuilder_setValueWithBothPresentation_nullInlinePresentation() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(NullPointerException.class, () -> builder.setValue(mId, mValue,
                mPresentation, null));
    }

    @Test
    public void testBuilder_setValueWithBothPresentation_bothNull() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(NullPointerException.class, () -> builder.setValue(mId, mValue,
                (RemoteViews) null, null));
    }

    @Test
    public void testBuilder_setFilteredValueWithNullFilter() {
        assertThat(new Dataset.Builder(mPresentation).setValue(mId, mValue, (Pattern) null).build())
                .isNotNull();
    }

    @Test
    public void testBuilder_setFilteredValueWithPresentation_nullFilter() {
        assertThat(new Dataset.Builder().setValue(mId, mValue, null, mPresentation).build())
                .isNotNull();
    }

    @Test
    public void testBuilder_setFilteredValueWithPresentation_nullPresentation() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(NullPointerException.class, () -> builder.setValue(mId, mValue, mFilter,
                null));
    }

    @Test
    public void testBuilder_setFilteredValueWithoutPresentation() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(IllegalStateException.class, () -> builder.setValue(mId, mValue, mFilter));
    }

    @Test
    public void testBuilder_setFilteredValueWithBothPresentation_nullPresentation() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(NullPointerException.class, () -> builder.setValue(mId, mValue, mFilter,
                null, mInlinePresentation));
    }

    @Test
    public void testBuilder_setFilteredValueWithBothPresentation_nullInlinePresentation() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(NullPointerException.class, () -> builder.setValue(mId, mValue, mFilter,
                mPresentation, null));
    }

    @Test
    public void testBuilder_setFilteredValueWithBothPresentation_bothNull() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(NullPointerException.class, () -> builder.setValue(mId, mValue, mFilter,
                null, null));
    }

    @Test
    public void testBuilder_setFieldInlinePresentations() {
        assertThat(new Dataset.Builder().setFieldInlinePresentation(mId, mValue, mFilter,
                mInlinePresentation)).isNotNull();
    }

    @Test
    public void testBuilder_setValue() {
        Dataset.Builder builder = new Dataset.Builder().setValue(mId, mValue);
        Dataset dataset = builder.build();
        assertThat(dataset.getFieldIds()).isEqualTo(singletonList(mId));
        assertThat(dataset.getFieldValues()).isEqualTo(singletonList(mValue));
    }

    @Test
    public void testBuilder_setValueForMultipleFields() {
        Dataset.Builder builder = new Dataset.Builder()
                .setValue(mId, mValue)
                .setValue(mId2, mValue);
        Dataset dataset = builder.build();
        assertThat(dataset.getFieldIds()).isEqualTo(asList(mId, mId2));
        assertThat(dataset.getFieldValues()).isEqualTo(asList(mValue, mValue));
    }

    @Test
    public void testBuilder_setValueAcceptsNullValue() {
        // It's valid to pass null value, e.g. when wanting to trigger the auth flow.
        Dataset.Builder builder = new Dataset.Builder().setValue(mId, null);
        Dataset dataset = builder.build();
        assertThat(dataset.getFieldIds()).isEqualTo(singletonList(mId));
        assertThat(dataset.getFieldValues()).isEqualTo(singletonList(null));
    }

    @Test
    public void testBuilder_setValueWithAuthentication() {
        Dataset.Builder builder = new Dataset.Builder()
                .setValue(mId, mValue)
                .setAuthentication(mAuth);
        Dataset dataset = builder.build();
        assertThat(dataset.getFieldIds()).isEqualTo(singletonList(mId));
        assertThat(dataset.getFieldValues()).isEqualTo(singletonList(mValue));
        assertThat(dataset.getAuthentication()).isEqualTo(mAuth);
    }

    @Test
    public void testBuilder_setContent() {
        Dataset.Builder builder = new Dataset.Builder().setContent(mId, mContent);
        Dataset dataset = builder.build();
        assertThat(dataset.getFieldIds()).isEqualTo(singletonList(mId));
        assertThat(dataset.getFieldContent()).isEqualTo(mContent);
        assertThat(dataset.getFieldValues()).isEqualTo(singletonList(null));
    }

    @Test
    public void testBuilder_setContentWithIntentIsNotAllowed() {
        Dataset.Builder builder = new Dataset.Builder();
        ClipData clip = ClipData.newIntent("", new Intent());
        assertThrows(IllegalArgumentException.class, () -> builder.setContent(mId, clip));
    }

    @Test
    public void testBuilder_setContentAcceptsNullContent() {
        // It's valid to pass null content, e.g. when wanting to trigger the auth flow.
        Dataset.Builder builder = new Dataset.Builder().setContent(mId, null);
        Dataset dataset = builder.build();
        assertThat(dataset.getFieldIds()).isEqualTo(singletonList(mId));
        assertThat(dataset.getFieldContent()).isNull();
        assertThat(dataset.getFieldValues()).isEqualTo(singletonList(null));
    }

    @Test
    public void testBuilder_setContentWithAuthentication() {
        Dataset.Builder builder = new Dataset.Builder()
                .setContent(mId, mContent)
                .setAuthentication(mAuth);
        Dataset dataset = builder.build();
        assertThat(dataset.getFieldIds()).isEqualTo(singletonList(mId));
        assertThat(dataset.getFieldContent()).isEqualTo(mContent);
        assertThat(dataset.getAuthentication()).isEqualTo(mAuth);
        assertThat(dataset.getFieldValues()).isEqualTo(singletonList(null));
    }

    @Test
    public void testBuilder_settingBothContentAndValuesIsNotAllowed() {
        // Setting both content and value for the same field is not allowed.
        Dataset.Builder builder = new Dataset.Builder();
        builder.setContent(mId, mContent);
        builder.setValue(mId, mValue);
        assertThrows(IllegalStateException.class, builder::build);

        // Setting both content and value, even if for different fields, is not allowed.
        builder = new Dataset.Builder();
        builder.setContent(mId, mContent);
        builder.setValue(mId2, mValue);
        assertThrows(IllegalStateException.class, builder::build);
    }

    @Test
    public void testBuilder_settingContentForMultipleFieldsIsNotAllowed() {
        Dataset.Builder builder = new Dataset.Builder();
        builder.setContent(mId, mContent);
        builder.setContent(mId2, mContent);
        assertThrows(IllegalStateException.class, builder::build);
    }

    @Test
    public void testBuild_noValues() {
        final Dataset.Builder builder = new Dataset.Builder();
        assertThrows(IllegalStateException.class, () -> builder.build());
    }

    @Test
    public void testNoMoreInteractionsAfterBuild() {
        final Dataset.Builder builder = new Dataset.Builder();
        builder.setValue(mId, mValue, mPresentation);
        assertThat(builder.build()).isNotNull();
        assertThrows(IllegalStateException.class, () -> builder.build());
        assertThrows(IllegalStateException.class,
                () -> builder.setInlinePresentation(mInlinePresentation));
        assertThrows(IllegalStateException.class, () -> builder.setValue(mId, mValue));
        assertThrows(IllegalStateException.class,
                () -> builder.setValue(mId, mValue, mPresentation));
        assertThrows(IllegalStateException.class,
                () -> builder.setValue(mId, mValue, mFilter));
        assertThrows(IllegalStateException.class,
                () -> builder.setValue(mId, mValue, mFilter, mPresentation));
        assertThrows(IllegalStateException.class,
                () -> builder.setValue(mId, mValue, mPresentation, mInlinePresentation));
        assertThrows(IllegalStateException.class,
                () -> builder.setValue(mId, mValue, mFilter, mPresentation, mInlinePresentation));
        assertThrows(IllegalStateException.class,
                () -> builder.setFieldInlinePresentation(mId, mValue, mFilter,
                        mInlinePresentation));
        assertThrows(IllegalStateException.class, () -> builder.setContent(mId, mContent));
    }

    @Test
    public void testWriteToParcel_values() throws Exception {
        Dataset dataset = new Dataset.Builder(mInlinePresentation)
                .setValue(mId, mValue)
                .setValue(mId2, mValue)
                .setId("test-dataset-id")
                .build();
        Parcel parcel = Parcel.obtain();
        dataset.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);

        Dataset result = Dataset.CREATOR.createFromParcel(parcel);
        assertThat(result.getId()).isEqualTo(dataset.getId());
        assertThat(result.getFieldIds()).isEqualTo(asList(mId, mId2));
        assertThat(result.getFieldValues()).isEqualTo(asList(mValue, mValue));
        assertThat(result.getFieldContent()).isNull();
    }

    @Test
    public void testWriteToParcel_content() throws Exception {
        Dataset dataset = new Dataset.Builder(mInlinePresentation)
                .setContent(mId, mContent)
                .setId("test-dataset-id")
                .build();
        Parcel parcel = Parcel.obtain();
        dataset.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);

        Dataset result = Dataset.CREATOR.createFromParcel(parcel);
        assertThat(result.getId()).isEqualTo(dataset.getId());
        assertThat(result.getFieldIds()).isEqualTo(singletonList(mId));
        assertThat(result.getFieldContent().getItemCount()).isEqualTo(mContent.getItemCount());
        assertThat(dataset.getFieldValues()).isEqualTo(singletonList(null));
    }
}

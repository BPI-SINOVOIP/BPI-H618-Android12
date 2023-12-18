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

package android.content.cts;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.mock;

import android.content.ClipData;
import android.content.ClipDescription;
import android.content.Intent;
import android.net.Uri;
import android.os.PersistableBundle;
import android.util.Log;

import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class ClipDataTest {
    private static final String LOG_TAG = "ClipDataTest";

    @Test
    public void testToString_text() {
        ClipData clip = ClipData.newPlainText(null, "secret-text");
        String clipStr = clip.toString();
        Log.i(LOG_TAG, clipStr);
        assertThat(clipStr).doesNotContain("secret");
    }

    @Test
    public void testToString_html() {
        ClipData clip = ClipData.newHtmlText(null, "secret-text", "secret-html");
        String clipStr = clip.toString();
        Log.i(LOG_TAG, clipStr);
        assertThat(clipStr).doesNotContain("secret");
    }

    @Test
    public void testToString_uri() {
        ClipData clip = ClipData.newRawUri(null, Uri.parse("content://secret"));
        String clipStr = clip.toString();
        Log.i(LOG_TAG, clipStr);
        assertThat(clipStr).doesNotContain("secret");
    }

    @Test
    public void testToString_metadata() {
        ClipDescription description = new ClipDescription("secret-label",
                new String[]{"text/plain"});
        PersistableBundle extras = new PersistableBundle();
        extras.putString("secret-key", "secret-value");
        description.setExtras(extras);
        description.setTimestamp(42);
        ClipData clip = new ClipData(description, new ClipData.Item("secret-text"));
        String clipStr = clip.toString();
        Log.i(LOG_TAG, clipStr);
        assertThat(clipStr).doesNotContain("secret");
    }

    @Test
    public void testToString_multipleItems() {
        ClipData clip = ClipData.newPlainText(null, "secret-one");
        clip.addItem(new ClipData.Item("secret-two"));
        clip.addItem(new ClipData.Item("secret-three"));
        String clipStr = clip.toString();
        Log.i(LOG_TAG, clipStr);
        assertThat(clipStr).doesNotContain("secret");
    }

    @Test
    public void testToString_complexItem() {
        ClipData.Item item = new ClipData.Item(
                "secret-text",
                "secret-html",
                mock(Intent.class),
                Uri.parse("content://secret"));
        String[] mimeTypes = {
                ClipDescription.MIMETYPE_TEXT_PLAIN,
                ClipDescription.MIMETYPE_TEXT_HTML,
                ClipDescription.MIMETYPE_TEXT_INTENT,
                ClipDescription.MIMETYPE_TEXT_URILIST
        };
        ClipData clip = new ClipData("secret-label", mimeTypes, item);
        String clipStr = clip.toString();
        Log.i(LOG_TAG, clipStr);
        assertThat(clipStr).doesNotContain("secret");
    }
}

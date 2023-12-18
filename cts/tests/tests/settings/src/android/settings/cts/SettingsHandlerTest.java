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

package android.settings.cts;

import static com.google.common.truth.Truth.assertThat;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.storage.StorageManager;
import android.provider.DocumentsContract;

import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

/** Test to make there is only default handler for settings intent. */
@RunWith(AndroidJUnit4.class)
public class SettingsHandlerTest {

    private static final String MEDIA_PROVIDER_AUTHORITY = "com.android.providers.media.documents";
    private static final String RESOLVER_ACTIVITY_PACKAGE_NAME = "android";
    private static final String RESOLVER_ACTIVITY_NAME =
            "com.android.internal.app.ResolverActivity";

    PackageManager mPackageManager =
            InstrumentationRegistry.getInstrumentation().getContext().getPackageManager();

    @Test
    public void oneDefaultHandlerForVideoRoot() throws Exception {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        Uri uri = DocumentsContract.buildRootUri(MEDIA_PROVIDER_AUTHORITY, "videos_root");
        intent.setDataAndType(uri, DocumentsContract.Root.MIME_TYPE_ITEM);

        assertThat(hasAtMostOneDefaultHandlerForIntent(intent)).isTrue();
    }

    @Test
    public void oneDefaultHandlerForImageRoot() throws Exception {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        Uri uri = DocumentsContract.buildRootUri(MEDIA_PROVIDER_AUTHORITY, "images_root");
        intent.setDataAndType(uri, DocumentsContract.Root.MIME_TYPE_ITEM);

        assertThat(hasAtMostOneDefaultHandlerForIntent(intent)).isTrue();
    }

    @Test
    public void oneDefaultHandlerForAudioRoot() throws Exception {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        Uri uri = DocumentsContract.buildRootUri(MEDIA_PROVIDER_AUTHORITY, "audio_root");
        intent.setDataAndType(uri, DocumentsContract.Root.MIME_TYPE_ITEM);

        assertThat(hasAtMostOneDefaultHandlerForIntent(intent)).isTrue();
    }

    @Test
    public void oneDefaultHandlerForDocumentRoot() throws Exception {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        Uri uri = DocumentsContract.buildRootUri(MEDIA_PROVIDER_AUTHORITY, "documents_root");
        intent.setDataAndType(uri, DocumentsContract.Root.MIME_TYPE_ITEM);

        assertThat(hasAtMostOneDefaultHandlerForIntent(intent)).isTrue();
    }

    @Test
    public void oneDefaultHandlerForManageStorage() throws Exception {
        Intent intent = new Intent(StorageManager.ACTION_MANAGE_STORAGE);

        assertThat(hasAtMostOneDefaultHandlerForIntent(intent)).isTrue();
    }

    private boolean hasAtMostOneDefaultHandlerForIntent(Intent intent) {
        ResolveInfo resolveInfo = mPackageManager.resolveActivity(intent, /* flags= */ 0);

        if (resolveInfo == null) {
            // Return true if no handlers can handle the intent, meaning that the specific platform
            // does need to handle the intent.
            return true;
        }

        String packageName = resolveInfo.activityInfo.packageName;
        String activityName = resolveInfo.activityInfo.name;
        // If there are more than one handlers with no preferences set, the intent will resolve
        // to com.android.internal.app.ResolverActivity.
        return !packageName.equals(RESOLVER_ACTIVITY_PACKAGE_NAME) || !activityName.equals(
                RESOLVER_ACTIVITY_NAME);
    }
}

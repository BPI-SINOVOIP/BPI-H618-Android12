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

package android.app.appsearch.cts.app;

import static android.os.storage.StorageManager.UUID_DEFAULT;

import static com.google.common.truth.Truth.assertThat;

import android.app.appsearch.AppSearchManager;
import android.app.appsearch.AppSearchSessionShim;
import android.app.appsearch.PutDocumentsRequest;
import android.app.appsearch.SetSchemaRequest;
import android.app.usage.StorageStats;
import android.app.usage.StorageStatsManager;
import android.content.Context;
import android.os.UserHandle;

import androidx.test.core.app.ApplicationProvider;

import com.android.server.appsearch.testing.AppSearchEmail;
import com.android.server.appsearch.testing.AppSearchSessionShimImpl;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

public class AppSearchSessionPlatformCtsTest {
    private static final String DB_NAME = "testDb";

    private AppSearchSessionShim mDb;

    @Before
    public void setUp() throws Exception {
        mDb =
                AppSearchSessionShimImpl.createSearchSession(
                                new AppSearchManager.SearchContext.Builder(DB_NAME).build())
                        .get();

        // Cleanup whatever documents may still exist in these databases. This is needed in
        // addition to tearDown in case a test exited without completing properly.
        cleanup();
    }

    @After
    public void tearDown() throws Exception {
        // Cleanup whatever documents may still exist in these databases.
        cleanup();
    }

    private void cleanup() throws Exception {
        mDb.setSchema(new SetSchemaRequest.Builder().setForceOverride(true).build()).get();
    }

    @Test
    public void testStorageAttributedToSelf_withDocument() throws Exception {
        Context context = ApplicationProvider.getApplicationContext();
        StorageStatsManager storageStatsManager =
                context.getSystemService(StorageStatsManager.class);
        String packageName = context.getPackageName();
        UserHandle user = context.getUser();
        int uid = android.os.Process.myUid();
        StorageStats beforeStatsForPkg =
                storageStatsManager.queryStatsForPackage(UUID_DEFAULT, packageName, user);
        StorageStats beforeStatsForUid = storageStatsManager.queryStatsForUid(UUID_DEFAULT, uid);

        // Schema registration.
        mDb.setSchema(new SetSchemaRequest.Builder().addSchemas(AppSearchEmail.SCHEMA).build())
                .get();
        AppSearchEmail email =
                new AppSearchEmail.Builder("namespace", "uri1")
                        .setFrom("from@example.com")
                        .setTo("to1@example.com", "to2@example.com")
                        .setSubject("testPut example")
                        .setBody("This is the body of the testPut email")
                        .build();
        // Put 30 document. Needed to spot the storage size increase.
        List<AppSearchEmail> emails = new ArrayList<>();
        for (int i = 0; i < 30; i++) {
            emails.add(email);
        }
        mDb.put(new PutDocumentsRequest.Builder().addGenericDocuments(emails).build()).get();

        StorageStats afterStatsForPkg =
                storageStatsManager.queryStatsForPackage(UUID_DEFAULT, packageName, user);
        StorageStats afterStatsForUid = storageStatsManager.queryStatsForUid(UUID_DEFAULT, uid);

        // Verify the storage size increase.
        assertThat(afterStatsForPkg.getDataBytes()).isGreaterThan(beforeStatsForPkg.getDataBytes());
        assertThat(afterStatsForUid.getDataBytes()).isGreaterThan(beforeStatsForUid.getDataBytes());
    }

    @Test
    public void testStorageAttributedToSelf_withoutDocument() throws Exception {
        Context context = ApplicationProvider.getApplicationContext();
        StorageStatsManager storageStatsManager =
                context.getSystemService(StorageStatsManager.class);
        String packageName = context.getPackageName();
        UserHandle user = context.getUser();
        int uid = android.os.Process.myUid();
        StorageStats beforeStatsForPkg =
                storageStatsManager.queryStatsForPackage(UUID_DEFAULT, packageName, user);
        StorageStats beforeStatsForUid = storageStatsManager.queryStatsForUid(UUID_DEFAULT, uid);

        // Schema registration.
        mDb.setSchema(new SetSchemaRequest.Builder().addSchemas(AppSearchEmail.SCHEMA).build())
                .get();

        StorageStats afterStatsForPkg =
                storageStatsManager.queryStatsForPackage(UUID_DEFAULT, packageName, user);
        StorageStats afterStatsForUid = storageStatsManager.queryStatsForUid(UUID_DEFAULT, uid);

        // Verify that storage size doesn't change.
        assertThat(afterStatsForPkg.getDataBytes()).isEqualTo(beforeStatsForPkg.getDataBytes());
        assertThat(afterStatsForUid.getDataBytes()).isEqualTo(beforeStatsForUid.getDataBytes());
    }
}

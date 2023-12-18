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

import static android.Manifest.permission.READ_GLOBAL_APP_SEARCH_DATA;

import static com.android.server.appsearch.testing.AppSearchTestUtils.checkIsBatchResultSuccess;

import static com.google.common.truth.Truth.assertThat;

import android.app.appsearch.AppSearchManager;
import android.app.appsearch.AppSearchSessionShim;
import android.app.appsearch.GenericDocument;
import android.app.appsearch.GlobalSearchSessionShim;
import android.app.appsearch.PackageIdentifier;
import android.app.appsearch.PutDocumentsRequest;
import android.app.appsearch.ReportSystemUsageRequest;
import android.app.appsearch.ReportUsageRequest;
import android.app.appsearch.SearchResult;
import android.app.appsearch.SearchResultsShim;
import android.app.appsearch.SearchSpec;
import android.app.appsearch.SetSchemaRequest;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.platform.test.annotations.AppModeFull;
import android.util.Log;

import androidx.test.core.app.ApplicationProvider;

import com.android.compatibility.common.util.SystemUtil;
import com.android.cts.appsearch.ICommandReceiver;
import com.android.server.appsearch.testing.AppSearchEmail;
import com.android.server.appsearch.testing.AppSearchSessionShimImpl;
import com.android.server.appsearch.testing.GlobalSearchSessionShimImpl;

import com.google.common.collect.ImmutableSet;
import com.google.common.io.BaseEncoding;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.List;
import java.util.Set;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * This doesn't extend {@link android.app.appsearch.cts.app.GlobalSearchSessionCtsTestBase} since
 * these test cases can't be run in a non-platform environment.
 */
@AppModeFull(reason = "Can't bind to helper apps from instant mode")
public class GlobalSearchSessionPlatformCtsTest {

    private static final long TIMEOUT_BIND_SERVICE_SEC = 2;

    private static final String TAG = "GlobalSearchSessionPlatformCtsTest";

    private static final String PKG_A = "com.android.cts.appsearch.helper.a";

    // To generate, run `apksigner` on the build APK. e.g.
    //   ./apksigner verify --print-certs \
    //   ~/sc-dev/out/soong/.intermediates/cts/tests/appsearch/CtsAppSearchTestHelperA/\
    //   android_common/CtsAppSearchTestHelperA.apk`
    // to get the SHA-256 digest. All characters need to be uppercase.
    //
    // Note: May need to switch the "sdk_version" of the test app from "test_current" to "30" before
    // building the apk and running apksigner
    private static final byte[] PKG_A_CERT_SHA256 =
            BaseEncoding.base16()
                    .decode("A90B80BD307B71BB4029674C5C4FE18066994E352EAC933B7B68266210CAFB53");

    private static final String PKG_B = "com.android.cts.appsearch.helper.b";

    // To generate, run `apksigner` on the build APK. e.g.
    //   ./apksigner verify --print-certs \
    //   ~/sc-dev/out/soong/.intermediates/cts/tests/appsearch/CtsAppSearchTestHelperB/\
    //   android_common/CtsAppSearchTestHelperB.apk`
    // to get the SHA-256 digest. All characters need to be uppercase.
    //
    // Note: May need to switch the "sdk_version" of the test app from "test_current" to "30" before
    // building the apk and running apksigner
    private static final byte[] PKG_B_CERT_SHA256 =
            BaseEncoding.base16()
                    .decode("88C0B41A31943D13226C3F22A86A6B4F300315575A6BC533CBF16C4EF3CFAA37");

    private static final String HELPER_SERVICE =
            "com.android.cts.appsearch.helper.AppSearchTestService";

    private static final String TEXT = "foo";

    private static final AppSearchEmail EMAIL_DOCUMENT =
            new AppSearchEmail.Builder("namespace", "id1")
                    .setFrom("from@example.com")
                    .setTo("to1@example.com", "to2@example.com")
                    .setSubject(TEXT)
                    .setBody("this is the body of the email")
                    .build();

    private static final String DB_NAME = "";

    private GlobalSearchSessionShim mGlobalSearchSession;
    private AppSearchSessionShim mDb;

    private Context mContext;

    @Before
    public void setUp() throws Exception {
        mContext = ApplicationProvider.getApplicationContext();
        mDb =
                AppSearchSessionShimImpl.createSearchSession(
                                new AppSearchManager.SearchContext.Builder(DB_NAME).build())
                        .get();
        cleanup();
    }

    @After
    public void tearDown() throws Exception {
        // Cleanup whatever documents may still exist in these databases.
        cleanup();
    }

    private void cleanup() throws Exception {
        mDb.setSchema(new SetSchemaRequest.Builder().setForceOverride(true).build()).get();

        clearData(PKG_A);
        clearData(PKG_B);
    }

    @Test
    public void testNoPackageAccess_default() throws Exception {
        mDb.setSchema(new SetSchemaRequest.Builder().addSchemas(AppSearchEmail.SCHEMA).build())
                .get();
        checkIsBatchResultSuccess(
                mDb.put(
                        new PutDocumentsRequest.Builder()
                                .addGenericDocuments(EMAIL_DOCUMENT)
                                .build()));

        // No package has access by default
        assertPackageCannotAccess(PKG_A);
        assertPackageCannotAccess(PKG_B);
    }

    @Test
    public void testNoPackageAccess_wrongPackageName() throws Exception {
        mDb.setSchema(
                        new SetSchemaRequest.Builder()
                                .addSchemas(AppSearchEmail.SCHEMA)
                                .setSchemaTypeVisibilityForPackage(
                                        AppSearchEmail.SCHEMA_TYPE,
                                        /*visible=*/ true,
                                        new PackageIdentifier(
                                                "some.other.package", PKG_A_CERT_SHA256))
                                .build())
                .get();
        checkIsBatchResultSuccess(
                mDb.put(
                        new PutDocumentsRequest.Builder()
                                .addGenericDocuments(EMAIL_DOCUMENT)
                                .build()));

        assertPackageCannotAccess(PKG_A);
    }

    @Test
    public void testNoPackageAccess_wrongCertificate() throws Exception {
        mDb.setSchema(
                        new SetSchemaRequest.Builder()
                                .addSchemas(AppSearchEmail.SCHEMA)
                                .setSchemaTypeVisibilityForPackage(
                                        AppSearchEmail.SCHEMA_TYPE,
                                        /*visible=*/ true,
                                        new PackageIdentifier(PKG_A, new byte[] {10}))
                                .build())
                .get();
        checkIsBatchResultSuccess(
                mDb.put(
                        new PutDocumentsRequest.Builder()
                                .addGenericDocuments(EMAIL_DOCUMENT)
                                .build()));

        assertPackageCannotAccess(PKG_A);
    }

    @Test
    public void testAllowPackageAccess() throws Exception {
        mDb.setSchema(
                        new SetSchemaRequest.Builder()
                                .addSchemas(AppSearchEmail.SCHEMA)
                                .setSchemaTypeVisibilityForPackage(
                                        AppSearchEmail.SCHEMA_TYPE,
                                        /*visible=*/ true,
                                        new PackageIdentifier(PKG_A, PKG_A_CERT_SHA256))
                                .build())
                .get();
        checkIsBatchResultSuccess(
                mDb.put(
                        new PutDocumentsRequest.Builder()
                                .addGenericDocuments(EMAIL_DOCUMENT)
                                .build()));

        assertPackageCanAccess(EMAIL_DOCUMENT, PKG_A);
        assertPackageCannotAccess(PKG_B);
    }

    @Test
    public void testAllowMultiplePackageAccess() throws Exception {
        mDb.setSchema(
                        new SetSchemaRequest.Builder()
                                .addSchemas(AppSearchEmail.SCHEMA)
                                .setSchemaTypeVisibilityForPackage(
                                        AppSearchEmail.SCHEMA_TYPE,
                                        /*visible=*/ true,
                                        new PackageIdentifier(PKG_A, PKG_A_CERT_SHA256))
                                .setSchemaTypeVisibilityForPackage(
                                        AppSearchEmail.SCHEMA_TYPE,
                                        /*visible=*/ true,
                                        new PackageIdentifier(PKG_B, PKG_B_CERT_SHA256))
                                .build())
                .get();
        checkIsBatchResultSuccess(
                mDb.put(
                        new PutDocumentsRequest.Builder()
                                .addGenericDocuments(EMAIL_DOCUMENT)
                                .build()));

        assertPackageCanAccess(EMAIL_DOCUMENT, PKG_A);
        assertPackageCanAccess(EMAIL_DOCUMENT, PKG_B);
    }

    @Test
    public void testNoPackageAccess_revoked() throws Exception {
        mDb.setSchema(
                        new SetSchemaRequest.Builder()
                                .addSchemas(AppSearchEmail.SCHEMA)
                                .setSchemaTypeVisibilityForPackage(
                                        AppSearchEmail.SCHEMA_TYPE,
                                        /*visible=*/ true,
                                        new PackageIdentifier(PKG_A, PKG_A_CERT_SHA256))
                                .build())
                .get();
        checkIsBatchResultSuccess(
                mDb.put(
                        new PutDocumentsRequest.Builder()
                                .addGenericDocuments(EMAIL_DOCUMENT)
                                .build()));
        assertPackageCanAccess(EMAIL_DOCUMENT, PKG_A);

        // Set the schema again, but package access as false.
        mDb.setSchema(
                        new SetSchemaRequest.Builder()
                                .addSchemas(AppSearchEmail.SCHEMA)
                                .setSchemaTypeVisibilityForPackage(
                                        AppSearchEmail.SCHEMA_TYPE,
                                        /*visible=*/ false,
                                        new PackageIdentifier(PKG_A, PKG_A_CERT_SHA256))
                                .build())
                .get();
        checkIsBatchResultSuccess(
                mDb.put(
                        new PutDocumentsRequest.Builder()
                                .addGenericDocuments(EMAIL_DOCUMENT)
                                .build()));
        assertPackageCannotAccess(PKG_A);

        // Set the schema again, but with default (i.e. no) access
        mDb.setSchema(
                        new SetSchemaRequest.Builder()
                                .addSchemas(AppSearchEmail.SCHEMA)
                                .setSchemaTypeVisibilityForPackage(
                                        AppSearchEmail.SCHEMA_TYPE,
                                        /*visible=*/ false,
                                        new PackageIdentifier(PKG_A, PKG_A_CERT_SHA256))
                                .build())
                .get();
        checkIsBatchResultSuccess(
                mDb.put(
                        new PutDocumentsRequest.Builder()
                                .addGenericDocuments(EMAIL_DOCUMENT)
                                .build()));
        assertPackageCannotAccess(PKG_A);
    }

    @Test
    public void testGlobalSearch_withAccess() throws Exception {
        indexGloballySearchableDocument(PKG_A);
        indexGloballySearchableDocument(PKG_B);

        SystemUtil.runWithShellPermissionIdentity(
                () -> {
                    mGlobalSearchSession =
                            GlobalSearchSessionShimImpl.createGlobalSearchSession(mContext).get();

                    SearchResultsShim searchResults =
                            mGlobalSearchSession.search(
                                    /*queryExpression=*/ "",
                                    new SearchSpec.Builder()
                                            .setTermMatch(SearchSpec.TERM_MATCH_EXACT_ONLY)
                                            .addFilterPackageNames(PKG_A, PKG_B)
                                            .build());
                    List<SearchResult> page = searchResults.getNextPage().get();
                    assertThat(page).hasSize(2);

                    Set<String> actualPackageNames =
                            ImmutableSet.of(
                                    page.get(0).getPackageName(), page.get(1).getPackageName());
                    assertThat(actualPackageNames).containsExactly(PKG_A, PKG_B);
                },
                READ_GLOBAL_APP_SEARCH_DATA);
    }

    @Test
    public void testGlobalSearch_withPartialAccess() throws Exception {
        indexGloballySearchableDocument(PKG_A);
        indexNotGloballySearchableDocument(PKG_B);

        SystemUtil.runWithShellPermissionIdentity(
                () -> {
                    mGlobalSearchSession =
                            GlobalSearchSessionShimImpl.createGlobalSearchSession(mContext).get();

                    SearchResultsShim searchResults =
                            mGlobalSearchSession.search(
                                    /*queryExpression=*/ "",
                                    new SearchSpec.Builder()
                                            .setTermMatch(SearchSpec.TERM_MATCH_EXACT_ONLY)
                                            .addFilterPackageNames(PKG_A, PKG_B)
                                            .build());
                    List<SearchResult> page = searchResults.getNextPage().get();
                    assertThat(page).hasSize(1);

                    assertThat(page.get(0).getPackageName()).isEqualTo(PKG_A);
                },
                READ_GLOBAL_APP_SEARCH_DATA);
    }

    @Test
    public void testGlobalSearch_withPackageFilters() throws Exception {
        indexGloballySearchableDocument(PKG_A);
        indexGloballySearchableDocument(PKG_B);

        SystemUtil.runWithShellPermissionIdentity(
                () -> {
                    mGlobalSearchSession =
                            GlobalSearchSessionShimImpl.createGlobalSearchSession(mContext).get();

                    SearchResultsShim searchResults =
                            mGlobalSearchSession.search(
                                    /*queryExpression=*/ "",
                                    new SearchSpec.Builder()
                                            .setTermMatch(SearchSpec.TERM_MATCH_EXACT_ONLY)
                                            .addFilterPackageNames(PKG_B)
                                            .build());
                    List<SearchResult> page = searchResults.getNextPage().get();
                    assertThat(page).hasSize(1);

                    assertThat(page.get(0).getPackageName()).isEqualTo(PKG_B);
                },
                READ_GLOBAL_APP_SEARCH_DATA);
    }

    @Test
    public void testGlobalSearch_withoutAccess() throws Exception {
        indexGloballySearchableDocument(PKG_A);
        indexGloballySearchableDocument(PKG_B);

        mGlobalSearchSession =
                GlobalSearchSessionShimImpl.createGlobalSearchSession(mContext).get();

        SearchResultsShim searchResults =
                mGlobalSearchSession.search(
                        /*queryExpression=*/ "",
                        new SearchSpec.Builder()
                                .setTermMatch(SearchSpec.TERM_MATCH_EXACT_ONLY)
                                .addFilterPackageNames(PKG_A, PKG_B)
                                .build());
        List<SearchResult> page = searchResults.getNextPage().get();
        assertThat(page).isEmpty();
    }

    @Test
    public void testReportSystemUsage() throws Exception {
        // Insert schema
        mDb.setSchema(new SetSchemaRequest.Builder().addSchemas(AppSearchEmail.SCHEMA).build())
                .get();

        // Insert two docs
        GenericDocument document1 =
                new GenericDocument.Builder<>("namespace", "id1", AppSearchEmail.SCHEMA_TYPE)
                        .build();
        GenericDocument document2 =
                new GenericDocument.Builder<>("namespace", "id2", AppSearchEmail.SCHEMA_TYPE)
                        .build();
        mDb.put(new PutDocumentsRequest.Builder().addGenericDocuments(document1, document2).build())
                .get();

        // Report some usages. id1 has 2 app and 1 system usage, id2 has 1 app and 2 system usage.
        mDb.reportUsage(
                new ReportUsageRequest.Builder("namespace", "id1")
                        .setUsageTimestampMillis(10)
                        .build())
                .get();
        mDb.reportUsage(
                new ReportUsageRequest.Builder("namespace", "id1")
                        .setUsageTimestampMillis(20)
                        .build())
                .get();
        mDb.reportUsage(
                new ReportUsageRequest.Builder("namespace", "id2")
                        .setUsageTimestampMillis(100)
                        .build())
                .get();

        SystemUtil.runWithShellPermissionIdentity(() -> {
            mGlobalSearchSession =
                    GlobalSearchSessionShimImpl.createGlobalSearchSession(mContext).get();
            mGlobalSearchSession
                    .reportSystemUsage(
                            new ReportSystemUsageRequest.Builder(
                                    mContext.getPackageName(), DB_NAME, "namespace", "id1")
                                    .setUsageTimestampMillis(1000)
                                    .build())
                    .get();
            mGlobalSearchSession
                    .reportSystemUsage(
                            new ReportSystemUsageRequest.Builder(
                                    mContext.getPackageName(), DB_NAME, "namespace", "id2")
                                    .setUsageTimestampMillis(200)
                                    .build())
                    .get();
            mGlobalSearchSession
                    .reportSystemUsage(
                            new ReportSystemUsageRequest.Builder(
                                    mContext.getPackageName(), DB_NAME, "namespace", "id2")
                                    .setUsageTimestampMillis(150)
                                    .build())
                    .get();
        }, READ_GLOBAL_APP_SEARCH_DATA);

        // Query the data
        mGlobalSearchSession =
                GlobalSearchSessionShimImpl.createGlobalSearchSession(mContext).get();

        // Sort by app usage count: id1 should win
        try (SearchResultsShim results = mDb.search(
                "",
                new SearchSpec.Builder()
                        .setTermMatch(SearchSpec.TERM_MATCH_EXACT_ONLY)
                        .setRankingStrategy(SearchSpec.RANKING_STRATEGY_USAGE_COUNT)
                        .build())) {
            List<SearchResult> page = results.getNextPage().get();
            assertThat(page).hasSize(2);
            assertThat(page.get(0).getGenericDocument().getId()).isEqualTo("id1");
            assertThat(page.get(1).getGenericDocument().getId()).isEqualTo("id2");
        }

        // Sort by app usage timestamp: id2 should win
        try (SearchResultsShim results = mDb.search(
                "",
                new SearchSpec.Builder()
                        .setTermMatch(SearchSpec.TERM_MATCH_EXACT_ONLY)
                        .setRankingStrategy(SearchSpec.RANKING_STRATEGY_USAGE_LAST_USED_TIMESTAMP)
                        .build())) {
            List<SearchResult> page = results.getNextPage().get();
            assertThat(page).hasSize(2);
            assertThat(page.get(0).getGenericDocument().getId()).isEqualTo("id2");
            assertThat(page.get(1).getGenericDocument().getId()).isEqualTo("id1");
        }

        // Sort by system usage count: id2 should win
        try (SearchResultsShim results = mDb.search(
                "",
                new SearchSpec.Builder()
                        .setTermMatch(SearchSpec.TERM_MATCH_EXACT_ONLY)
                        .setRankingStrategy(
                                SearchSpec.RANKING_STRATEGY_SYSTEM_USAGE_COUNT)
                        .build())) {
            List<SearchResult> page = results.getNextPage().get();
            assertThat(page).hasSize(2);
            assertThat(page.get(0).getGenericDocument().getId()).isEqualTo("id2");
            assertThat(page.get(1).getGenericDocument().getId()).isEqualTo("id1");
        }

        // Sort by system usage timestamp: id1 should win
        SearchSpec searchSpec = new SearchSpec.Builder()
                .setTermMatch(SearchSpec.TERM_MATCH_EXACT_ONLY)
                .setRankingStrategy(SearchSpec.RANKING_STRATEGY_SYSTEM_USAGE_LAST_USED_TIMESTAMP)
                .build();
        try (SearchResultsShim results = mDb.search("", searchSpec)) {
            List<SearchResult> page = results.getNextPage().get();
            assertThat(page).hasSize(2);
            assertThat(page.get(0).getGenericDocument().getId()).isEqualTo("id1");
            assertThat(page.get(1).getGenericDocument().getId()).isEqualTo("id2");
        }
    }

    private void assertPackageCannotAccess(String pkg) throws Exception {
        GlobalSearchSessionPlatformCtsTest.TestServiceConnection serviceConnection =
                bindToHelperService(pkg);
        try {
            ICommandReceiver commandReceiver = serviceConnection.getCommandReceiver();
            List<String> results = commandReceiver.globalSearch(TEXT);
            assertThat(results).isEmpty();
        } finally {
            serviceConnection.unbind();
        }
    }

    private void assertPackageCanAccess(GenericDocument expectedDocument, String pkg)
            throws Exception {
        GlobalSearchSessionPlatformCtsTest.TestServiceConnection serviceConnection =
                bindToHelperService(pkg);
        try {
            ICommandReceiver commandReceiver = serviceConnection.getCommandReceiver();
            List<String> results = commandReceiver.globalSearch(TEXT);
            assertThat(results).containsExactly(expectedDocument.toString());
        } finally {
            serviceConnection.unbind();
        }
    }

    private void indexGloballySearchableDocument(String pkg) throws Exception {
        GlobalSearchSessionPlatformCtsTest.TestServiceConnection serviceConnection =
                bindToHelperService(pkg);
        try {
            ICommandReceiver commandReceiver = serviceConnection.getCommandReceiver();
            assertThat(commandReceiver.indexGloballySearchableDocument()).isTrue();
        } finally {
            serviceConnection.unbind();
        }
    }

    private void indexNotGloballySearchableDocument(String pkg) throws Exception {
        GlobalSearchSessionPlatformCtsTest.TestServiceConnection serviceConnection =
                bindToHelperService(pkg);
        try {
            ICommandReceiver commandReceiver = serviceConnection.getCommandReceiver();
            assertThat(commandReceiver.indexNotGloballySearchableDocument()).isTrue();
        } finally {
            serviceConnection.unbind();
        }
    }

    private void clearData(String pkg) throws Exception {
        GlobalSearchSessionPlatformCtsTest.TestServiceConnection serviceConnection =
                bindToHelperService(pkg);
        try {
            ICommandReceiver commandReceiver = serviceConnection.getCommandReceiver();
            assertThat(commandReceiver.clearData()).isTrue();
        } finally {
            serviceConnection.unbind();
        }
    }

    private GlobalSearchSessionPlatformCtsTest.TestServiceConnection bindToHelperService(
            String pkg) {
        GlobalSearchSessionPlatformCtsTest.TestServiceConnection serviceConnection =
                new GlobalSearchSessionPlatformCtsTest.TestServiceConnection(mContext);
        Intent intent = new Intent().setComponent(new ComponentName(pkg, HELPER_SERVICE));
        mContext.bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
        return serviceConnection;
    }

    private class TestServiceConnection implements ServiceConnection {
        private final Context mContext;
        private final BlockingQueue<IBinder> mBlockingQueue = new LinkedBlockingQueue<>();
        private ICommandReceiver mCommandReceiver;

        TestServiceConnection(Context context) {
            mContext = context;
        }

        public void onServiceConnected(ComponentName componentName, IBinder service) {
            Log.i(TAG, "Service got connected: " + componentName);
            mBlockingQueue.offer(service);
        }

        public void onServiceDisconnected(ComponentName componentName) {
            Log.e(TAG, "Service got disconnected: " + componentName);
        }

        private IBinder getService() throws Exception {
            IBinder service = mBlockingQueue.poll(TIMEOUT_BIND_SERVICE_SEC, TimeUnit.SECONDS);
            return service;
        }

        public ICommandReceiver getCommandReceiver() throws Exception {
            if (mCommandReceiver == null) {
                mCommandReceiver = ICommandReceiver.Stub.asInterface(getService());
            }
            return mCommandReceiver;
        }

        public void unbind() {
            mCommandReceiver = null;
            mContext.unbindService(this);
        }
    }
}

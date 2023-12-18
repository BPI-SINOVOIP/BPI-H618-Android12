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
package com.android.cts.appsearch.helper;

import static com.android.server.appsearch.testing.AppSearchTestUtils.checkIsBatchResultSuccess;
import static com.android.server.appsearch.testing.AppSearchTestUtils.convertSearchResultsToDocuments;

import android.app.Service;
import android.app.appsearch.AppSearchManager;
import android.app.appsearch.AppSearchSessionShim;
import android.app.appsearch.GenericDocument;
import android.app.appsearch.GlobalSearchSessionShim;
import android.app.appsearch.PutDocumentsRequest;
import android.app.appsearch.SearchResultsShim;
import android.app.appsearch.SearchSpec;
import android.app.appsearch.SetSchemaRequest;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import com.android.cts.appsearch.ICommandReceiver;
import com.android.server.appsearch.testing.AppSearchEmail;
import com.android.server.appsearch.testing.AppSearchSessionShimImpl;
import com.android.server.appsearch.testing.GlobalSearchSessionShimImpl;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.Executors;

public class AppSearchTestService extends Service {

    private static final String TAG = "AppSearchTestService";
    private GlobalSearchSessionShim mGlobalSearchSessionShim;
    private AppSearchSessionShim mAppSearchSessionShim;

    @Override
    public void onCreate() {
        try {
            // We call this here so we can pass in a context. If we try to create the session in the
            // stub, it'll try to grab the context from ApplicationProvider. But that will fail
            // since this isn't instrumented.
            mGlobalSearchSessionShim =
                    GlobalSearchSessionShimImpl.createGlobalSearchSession(this).get();

            mAppSearchSessionShim =
                    AppSearchSessionShimImpl.createSearchSession(
                                    this,
                                    new AppSearchManager.SearchContext.Builder("database").build(),
                                    Executors.newCachedThreadPool())
                            .get();
        } catch (Exception e) {
            Log.e(TAG, "Error starting service.", e);
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return new CommandReceiver();
    }

    private class CommandReceiver extends ICommandReceiver.Stub {

        @Override
        public List<String> globalSearch(String queryExpression) {
            try {
                final SearchSpec searchSpec =
                        new SearchSpec.Builder()
                                .setTermMatch(SearchSpec.TERM_MATCH_EXACT_ONLY)
                                .build();
                SearchResultsShim searchResults =
                        mGlobalSearchSessionShim.search(queryExpression, searchSpec);
                List<GenericDocument> results = convertSearchResultsToDocuments(searchResults);

                List<String> resultStrings = new ArrayList<>();
                for (GenericDocument doc : results) {
                    resultStrings.add(doc.toString());
                }

                return resultStrings;
            } catch (Exception e) {
                Log.e(TAG, "Error issuing global search.", e);
                return Collections.emptyList();
            }
        }

        @Override
        public boolean indexGloballySearchableDocument() {
            try {
                // By default, schemas/documents are globally searchable. We don't purposely set
                // setSchemaTypeDisplayedBySystem(false) for this schema.
                mAppSearchSessionShim
                        .setSchema(
                                new SetSchemaRequest.Builder()
                                        .addSchemas(AppSearchEmail.SCHEMA)
                                        .build())
                        .get();

                AppSearchEmail emailDocument =
                        new AppSearchEmail.Builder("namespace", "id1")
                                .setFrom("from@example.com")
                                .setTo("to1@example.com", "to2@example.com")
                                .setSubject("subject")
                                .setBody("this is the body of the email")
                                .build();
                checkIsBatchResultSuccess(
                        mAppSearchSessionShim.put(
                                new PutDocumentsRequest.Builder()
                                        .addGenericDocuments(emailDocument)
                                        .build()));
                return true;
            } catch (Exception e) {
                Log.e(TAG, "Failed to index globally searchable document.", e);
            }
            return false;
        }

        @Override
        public boolean indexNotGloballySearchableDocument() {
            try {
                mAppSearchSessionShim
                        .setSchema(
                                new SetSchemaRequest.Builder()
                                        .addSchemas(AppSearchEmail.SCHEMA)
                                        .setSchemaTypeDisplayedBySystem(
                                                AppSearchEmail.SCHEMA_TYPE, /*displayed=*/ false)
                                        .build())
                        .get();

                AppSearchEmail emailDocument =
                        new AppSearchEmail.Builder("namespace", "id1")
                                .setFrom("from@example.com")
                                .setTo("to1@example.com", "to2@example.com")
                                .setSubject("subject")
                                .setBody("this is the body of the email")
                                .build();
                checkIsBatchResultSuccess(
                        mAppSearchSessionShim.put(
                                new PutDocumentsRequest.Builder()
                                        .addGenericDocuments(emailDocument)
                                        .build()));
                return true;
            } catch (Exception e) {
                Log.e(TAG, "Failed to index not-globally searchable document.", e);
            }
            return false;
        }

        public boolean clearData() {
            try {
                // Force override with empty schema will clear all previous schemas and their
                // documents.
                mAppSearchSessionShim
                        .setSchema(new SetSchemaRequest.Builder().setForceOverride(true).build())
                        .get();
                return true;
            } catch (Exception e) {
                Log.e(TAG, "Failed to clear data.", e);
            }
            return false;
        }
    }
}

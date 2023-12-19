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

package androidx.appsearch.smoketest;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertEquals;

import androidx.appsearch.app.AppSearchSchema;
import androidx.appsearch.app.AppSearchSchema.PropertyConfig;
import androidx.appsearch.app.AppSearchSchema.StringPropertyConfig;
import androidx.appsearch.app.AppSearchSession;
import androidx.appsearch.app.PutDocumentsRequest;
import androidx.appsearch.app.SearchResult;
import androidx.appsearch.app.SearchResults;
import androidx.appsearch.app.SearchSpec;
import androidx.appsearch.app.SetSchemaRequest;
import androidx.appsearch.localstorage.LocalStorage;
import androidx.appsearch.localstorage.LocalStorage.SearchContext;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;

import java.util.List;

@SmallTest
public class AppSearchSmokeTest {
    private AppSearchSession appSearch;

    @Before
    public void setUp() throws Exception {
        appSearch =
                LocalStorage.createSearchSession(
                                new SearchContext.Builder(
                                                ApplicationProvider.getApplicationContext())
                                        .build())
                        .get();
        // Remove all data before test
        appSearch.setSchema(new SetSchemaRequest.Builder().setForceOverride(true).build()).get();
    }

    @Test
    public void smokeTest() throws Exception {
        AppSearchSchema schema =
                new AppSearchSchema.Builder("testType")
                        .addProperty(
                                new StringPropertyConfig.Builder("prop")
                                        .setCardinality(PropertyConfig.CARDINALITY_OPTIONAL)
                                        .setIndexingType(
                                                StringPropertyConfig.INDEXING_TYPE_PREFIXES)
                                        .setTokenizerType(StringPropertyConfig.TOKENIZER_TYPE_PLAIN)
                                        .build())
                        .build();
        appSearch.setSchema(new SetSchemaRequest.Builder().addSchemas(schema).build()).get();
    }

    @Test
    public void smokeTestAnnotationProcessor() throws Exception {
        appSearch
                .setSchema(
                        new SetSchemaRequest.Builder()
                                .addDocumentClasses(TestDocument.class)
                                .build())
                .get();

        TestDocument input = new TestDocument("uri1", "avocado");
        appSearch
                .put(new PutDocumentsRequest.Builder().addDocuments(input).build())
                .get()
                .checkSuccess();
        SearchResults results =
                appSearch.search(
                        "av",
                        new SearchSpec.Builder()
                                .setTermMatch(SearchSpec.TERM_MATCH_PREFIX)
                                .build());
        List<SearchResult> page = results.getNextPage().get();
        assertThat(page).hasSize(1);
        SearchResult result = page.get(0);
        assertThat(results.getNextPage().get()).isEmpty();

        assertEquals("uri1", result.getDocument().getUri());
        assertEquals("avocado", result.getDocument().getPropertyString("body"));
        TestDocument output = result.getDocument().toDocumentClass(TestDocument.class);
        assertEquals("uri1", output.getUri());
        assertEquals("avocado", output.getBody());
    }
}

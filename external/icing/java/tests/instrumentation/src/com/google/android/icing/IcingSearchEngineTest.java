// Copyright (C) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.icing;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import com.google.android.icing.proto.DeleteByNamespaceResultProto;
import com.google.android.icing.proto.DeleteByQueryResultProto;
import com.google.android.icing.proto.DeleteBySchemaTypeResultProto;
import com.google.android.icing.proto.DeleteResultProto;
import com.google.android.icing.proto.DocumentProto;
import com.google.android.icing.proto.GetAllNamespacesResultProto;
import com.google.android.icing.proto.GetOptimizeInfoResultProto;
import com.google.android.icing.proto.GetResultProto;
import com.google.android.icing.proto.GetResultSpecProto;
import com.google.android.icing.proto.GetSchemaResultProto;
import com.google.android.icing.proto.GetSchemaTypeResultProto;
import com.google.android.icing.proto.IcingSearchEngineOptions;
import com.google.android.icing.proto.InitializeResultProto;
import com.google.android.icing.proto.OptimizeResultProto;
import com.google.android.icing.proto.PersistToDiskResultProto;
import com.google.android.icing.proto.PersistType;
import com.google.android.icing.proto.PropertyConfigProto;
import com.google.android.icing.proto.PropertyProto;
import com.google.android.icing.proto.PutResultProto;
import com.google.android.icing.proto.ReportUsageResultProto;
import com.google.android.icing.proto.ResetResultProto;
import com.google.android.icing.proto.ResultSpecProto;
import com.google.android.icing.proto.SchemaProto;
import com.google.android.icing.proto.SchemaTypeConfigProto;
import com.google.android.icing.proto.ScoringSpecProto;
import com.google.android.icing.proto.SearchResultProto;
import com.google.android.icing.proto.SearchSpecProto;
import com.google.android.icing.proto.SetSchemaResultProto;
import com.google.android.icing.proto.SnippetMatchProto;
import com.google.android.icing.proto.SnippetProto;
import com.google.android.icing.proto.StatusProto;
import com.google.android.icing.proto.StorageInfoResultProto;
import com.google.android.icing.proto.StringIndexingConfig;
import com.google.android.icing.proto.StringIndexingConfig.TokenizerType;
import com.google.android.icing.proto.TermMatchType;
import com.google.android.icing.proto.UsageReport;
import com.google.android.icing.IcingSearchEngine;
import java.io.File;
import java.util.HashMap;
import java.util.Map;
import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * This test is not intended to fully test the functionality of each API. But rather to test the JNI
 * wrapper and Java interfaces of Icing library {@link IcingSearchEngine}.
 */
@RunWith(JUnit4.class)
public final class IcingSearchEngineTest {
  @Rule public TemporaryFolder temporaryFolder = new TemporaryFolder();

  private static final String EMAIL_TYPE = "Email";

  private File tempDir;

  private IcingSearchEngine icingSearchEngine;

  private static SchemaTypeConfigProto createEmailTypeConfig() {
    return SchemaTypeConfigProto.newBuilder()
        .setSchemaType(EMAIL_TYPE)
        .addProperties(
            PropertyConfigProto.newBuilder()
                .setPropertyName("subject")
                .setDataType(PropertyConfigProto.DataType.Code.STRING)
                .setCardinality(PropertyConfigProto.Cardinality.Code.OPTIONAL)
                .setStringIndexingConfig(
                    StringIndexingConfig.newBuilder()
                        .setTokenizerType(TokenizerType.Code.PLAIN)
                        .setTermMatchType(TermMatchType.Code.PREFIX)))
        .addProperties(
            PropertyConfigProto.newBuilder()
                .setPropertyName("body")
                .setDataType(PropertyConfigProto.DataType.Code.STRING)
                .setCardinality(PropertyConfigProto.Cardinality.Code.OPTIONAL)
                .setStringIndexingConfig(
                    StringIndexingConfig.newBuilder()
                        .setTokenizerType(TokenizerType.Code.PLAIN)
                        .setTermMatchType(TermMatchType.Code.PREFIX)))
        .build();
  }

  private static DocumentProto createEmailDocument(String namespace, String uri) {
    return DocumentProto.newBuilder()
        .setNamespace(namespace)
        .setUri(uri)
        .setSchema(EMAIL_TYPE)
        .setCreationTimestampMs(1) // Arbitrary non-zero number so Icing doesn't override it
        .build();
  }

  @Before
  public void setUp() throws Exception {
    tempDir = temporaryFolder.newFolder();
    IcingSearchEngineOptions options =
        IcingSearchEngineOptions.newBuilder().setBaseDir(tempDir.getCanonicalPath()).build();
    icingSearchEngine = new IcingSearchEngine(options);
  }

  @After
  public void tearDown() throws Exception {
    icingSearchEngine.close();
  }

  @Test
  public void testInitialize() throws Exception {
    InitializeResultProto initializeResultProto = icingSearchEngine.initialize();
    assertStatusOk(initializeResultProto.getStatus());
  }

  @Test
  public void testSetAndGetSchema() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    SetSchemaResultProto setSchemaResultProto =
        icingSearchEngine.setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false);
    assertStatusOk(setSchemaResultProto.getStatus());

    GetSchemaResultProto getSchemaResultProto = icingSearchEngine.getSchema();
    assertStatusOk(getSchemaResultProto.getStatus());
    assertThat(getSchemaResultProto.getSchema()).isEqualTo(schema);

    GetSchemaTypeResultProto getSchemaTypeResultProto =
        icingSearchEngine.getSchemaType(emailTypeConfig.getSchemaType());
    assertStatusOk(getSchemaTypeResultProto.getStatus());
    assertThat(getSchemaTypeResultProto.getSchemaTypeConfig()).isEqualTo(emailTypeConfig);
  }

  @Test
  public void testPutAndGetDocuments() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    assertThat(
            icingSearchEngine
                .setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false)
                .getStatus()
                .getCode())
        .isEqualTo(StatusProto.Code.OK);

    DocumentProto emailDocument = createEmailDocument("namespace", "uri");
    PutResultProto putResultProto = icingSearchEngine.put(emailDocument);
    assertStatusOk(putResultProto.getStatus());

    GetResultProto getResultProto =
        icingSearchEngine.get("namespace", "uri", GetResultSpecProto.getDefaultInstance());
    assertStatusOk(getResultProto.getStatus());
    assertThat(getResultProto.getDocument()).isEqualTo(emailDocument);
  }

  @Test
  public void testSearch() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    assertThat(
            icingSearchEngine
                .setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false)
                .getStatus()
                .getCode())
        .isEqualTo(StatusProto.Code.OK);

    DocumentProto emailDocument =
        createEmailDocument("namespace", "uri").toBuilder()
            .addProperties(PropertyProto.newBuilder().setName("subject").addStringValues("foo"))
            .build();
    assertStatusOk(icingSearchEngine.put(emailDocument).getStatus());

    SearchSpecProto searchSpec =
        SearchSpecProto.newBuilder()
            .setQuery("foo")
            .setTermMatchType(TermMatchType.Code.PREFIX)
            .build();

    SearchResultProto searchResultProto =
        icingSearchEngine.search(
            searchSpec,
            ScoringSpecProto.getDefaultInstance(),
            ResultSpecProto.getDefaultInstance());
    assertStatusOk(searchResultProto.getStatus());
    assertThat(searchResultProto.getResultsCount()).isEqualTo(1);
    assertThat(searchResultProto.getResults(0).getDocument()).isEqualTo(emailDocument);
  }

  @Test
  public void testGetNextPage() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    assertThat(
            icingSearchEngine
                .setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false)
                .getStatus()
                .getCode())
        .isEqualTo(StatusProto.Code.OK);

    Map<String, DocumentProto> documents = new HashMap<>();
    for (int i = 0; i < 10; i++) {
      DocumentProto emailDocument =
          createEmailDocument("namespace", "uri:" + i).toBuilder()
              .addProperties(PropertyProto.newBuilder().setName("subject").addStringValues("foo"))
              .build();
      documents.put("uri:" + i, emailDocument);
      assertWithMessage(icingSearchEngine.put(emailDocument).getStatus().getMessage())
          .that(icingSearchEngine.put(emailDocument).getStatus().getCode())
          .isEqualTo(StatusProto.Code.OK);
    }

    SearchSpecProto searchSpec =
        SearchSpecProto.newBuilder()
            .setQuery("foo")
            .setTermMatchType(TermMatchType.Code.PREFIX)
            .build();
    ResultSpecProto resultSpecProto = ResultSpecProto.newBuilder().setNumPerPage(1).build();

    SearchResultProto searchResultProto =
        icingSearchEngine.search(
            searchSpec, ScoringSpecProto.getDefaultInstance(), resultSpecProto);
    assertStatusOk(searchResultProto.getStatus());
    assertThat(searchResultProto.getResultsCount()).isEqualTo(1);
    DocumentProto resultDocument = searchResultProto.getResults(0).getDocument();
    assertThat(resultDocument).isEqualTo(documents.remove(resultDocument.getUri()));

    // fetch rest pages
    for (int i = 1; i < 5; i++) {
      searchResultProto = icingSearchEngine.getNextPage(searchResultProto.getNextPageToken());
      assertWithMessage(searchResultProto.getStatus().getMessage())
          .that(searchResultProto.getStatus().getCode())
          .isEqualTo(StatusProto.Code.OK);
      assertThat(searchResultProto.getResultsCount()).isEqualTo(1);
      resultDocument = searchResultProto.getResults(0).getDocument();
      assertThat(resultDocument).isEqualTo(documents.remove(resultDocument.getUri()));
    }

    // invalidate rest result
    icingSearchEngine.invalidateNextPageToken(searchResultProto.getNextPageToken());

    searchResultProto = icingSearchEngine.getNextPage(searchResultProto.getNextPageToken());
    assertStatusOk(searchResultProto.getStatus());
    assertThat(searchResultProto.getResultsCount()).isEqualTo(0);
  }

  @Test
  public void testDelete() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    assertThat(
            icingSearchEngine
                .setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false)
                .getStatus()
                .getCode())
        .isEqualTo(StatusProto.Code.OK);

    DocumentProto emailDocument = createEmailDocument("namespace", "uri");
    assertStatusOk(icingSearchEngine.put(emailDocument).getStatus());

    DeleteResultProto deleteResultProto = icingSearchEngine.delete("namespace", "uri");
    assertStatusOk(deleteResultProto.getStatus());

    GetResultProto getResultProto =
        icingSearchEngine.get("namespace", "uri", GetResultSpecProto.getDefaultInstance());
    assertThat(getResultProto.getStatus().getCode()).isEqualTo(StatusProto.Code.NOT_FOUND);
  }

  @Test
  public void testDeleteByNamespace() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    assertThat(
            icingSearchEngine
                .setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false)
                .getStatus()
                .getCode())
        .isEqualTo(StatusProto.Code.OK);

    DocumentProto emailDocument = createEmailDocument("namespace", "uri");
    assertStatusOk(icingSearchEngine.put(emailDocument).getStatus());

    DeleteByNamespaceResultProto deleteByNamespaceResultProto =
        icingSearchEngine.deleteByNamespace("namespace");
    assertStatusOk(deleteByNamespaceResultProto.getStatus());

    GetResultProto getResultProto =
        icingSearchEngine.get("namespace", "uri", GetResultSpecProto.getDefaultInstance());
    assertThat(getResultProto.getStatus().getCode()).isEqualTo(StatusProto.Code.NOT_FOUND);
  }

  @Test
  public void testDeleteBySchemaType() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    assertThat(
            icingSearchEngine
                .setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false)
                .getStatus()
                .getCode())
        .isEqualTo(StatusProto.Code.OK);

    DocumentProto emailDocument = createEmailDocument("namespace", "uri");
    assertStatusOk(icingSearchEngine.put(emailDocument).getStatus());

    DeleteBySchemaTypeResultProto deleteBySchemaTypeResultProto =
        icingSearchEngine.deleteBySchemaType(EMAIL_TYPE);
    assertStatusOk(deleteBySchemaTypeResultProto.getStatus());

    GetResultProto getResultProto =
        icingSearchEngine.get("namespace", "uri", GetResultSpecProto.getDefaultInstance());
    assertThat(getResultProto.getStatus().getCode()).isEqualTo(StatusProto.Code.NOT_FOUND);
  }

  @Test
  public void testDeleteByQuery() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    assertThat(
            icingSearchEngine
                .setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false)
                .getStatus()
                .getCode())
        .isEqualTo(StatusProto.Code.OK);

    DocumentProto emailDocument1 =
        createEmailDocument("namespace", "uri1").toBuilder()
            .addProperties(PropertyProto.newBuilder().setName("subject").addStringValues("foo"))
            .build();

    assertStatusOk(icingSearchEngine.put(emailDocument1).getStatus());
    DocumentProto emailDocument2 =
        createEmailDocument("namespace", "uri2").toBuilder()
            .addProperties(PropertyProto.newBuilder().setName("subject").addStringValues("bar"))
            .build();

    assertStatusOk(icingSearchEngine.put(emailDocument2).getStatus());

    SearchSpecProto searchSpec =
        SearchSpecProto.newBuilder()
            .setQuery("foo")
            .setTermMatchType(TermMatchType.Code.PREFIX)
            .build();

    SearchResultProto searchResultProto =
        icingSearchEngine.search(
            searchSpec,
            ScoringSpecProto.getDefaultInstance(),
            ResultSpecProto.getDefaultInstance());
    assertStatusOk(searchResultProto.getStatus());
    assertThat(searchResultProto.getResultsCount()).isEqualTo(1);
    assertThat(searchResultProto.getResults(0).getDocument()).isEqualTo(emailDocument1);

    DeleteByQueryResultProto deleteResultProto = icingSearchEngine.deleteByQuery(searchSpec);
    assertStatusOk(deleteResultProto.getStatus());

    GetResultProto getResultProto =
        icingSearchEngine.get("namespace", "uri1", GetResultSpecProto.getDefaultInstance());
    assertThat(getResultProto.getStatus().getCode()).isEqualTo(StatusProto.Code.NOT_FOUND);
    getResultProto =
        icingSearchEngine.get("namespace", "uri2", GetResultSpecProto.getDefaultInstance());
    assertStatusOk(getResultProto.getStatus());
  }

  @Test
  public void testPersistToDisk() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    PersistToDiskResultProto persistToDiskResultProto =
        icingSearchEngine.persistToDisk(PersistType.Code.LITE);
    assertStatusOk(persistToDiskResultProto.getStatus());
  }

  @Test
  public void testOptimize() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    OptimizeResultProto optimizeResultProto = icingSearchEngine.optimize();
    assertStatusOk(optimizeResultProto.getStatus());
  }

  @Test
  public void testGetOptimizeInfo() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    GetOptimizeInfoResultProto getOptimizeInfoResultProto = icingSearchEngine.getOptimizeInfo();
    assertStatusOk(getOptimizeInfoResultProto.getStatus());
    assertThat(getOptimizeInfoResultProto.getOptimizableDocs()).isEqualTo(0);
    assertThat(getOptimizeInfoResultProto.getEstimatedOptimizableBytes()).isEqualTo(0);
  }

  @Test
  public void testGetStorageInfo() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    StorageInfoResultProto storageInfoResultProto = icingSearchEngine.getStorageInfo();
    assertStatusOk(storageInfoResultProto.getStatus());
  }

  @Test
  public void testGetAllNamespaces() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    assertThat(
            icingSearchEngine
                .setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false)
                .getStatus()
                .getCode())
        .isEqualTo(StatusProto.Code.OK);

    DocumentProto emailDocument = createEmailDocument("namespace", "uri");
    assertStatusOk(icingSearchEngine.put(emailDocument).getStatus());

    GetAllNamespacesResultProto getAllNamespacesResultProto = icingSearchEngine.getAllNamespaces();
    assertStatusOk(getAllNamespacesResultProto.getStatus());
    assertThat(getAllNamespacesResultProto.getNamespacesList()).containsExactly("namespace");
  }

  @Test
  public void testReset() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    ResetResultProto resetResultProto = icingSearchEngine.reset();
    assertStatusOk(resetResultProto.getStatus());
  }

  @Test
  public void testReportUsage() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    // Set schema and put a document.
    SchemaTypeConfigProto emailTypeConfig = createEmailTypeConfig();
    SchemaProto schema = SchemaProto.newBuilder().addTypes(emailTypeConfig).build();
    assertThat(
            icingSearchEngine
                .setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false)
                .getStatus()
                .getCode())
        .isEqualTo(StatusProto.Code.OK);

    DocumentProto emailDocument = createEmailDocument("namespace", "uri");
    PutResultProto putResultProto = icingSearchEngine.put(emailDocument);
    assertStatusOk(putResultProto.getStatus());

    // Report usage
    UsageReport usageReport =
        UsageReport.newBuilder()
            .setDocumentNamespace("namespace")
            .setDocumentUri("uri")
            .setUsageTimestampMs(1)
            .setUsageType(UsageReport.UsageType.USAGE_TYPE1)
            .build();
    ReportUsageResultProto reportUsageResultProto = icingSearchEngine.reportUsage(usageReport);
    assertStatusOk(reportUsageResultProto.getStatus());
  }

  @Test
  @Ignore("b/190845688")
  public void testCJKTSnippets() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaProto schema = SchemaProto.newBuilder().addTypes(createEmailTypeConfig()).build();
    assertStatusOk(
        icingSearchEngine.setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false).getStatus());

    // String:     "我每天走路去上班。"
    //              ^ ^  ^   ^^
    // UTF16 idx:   0 1  3   5 6
    // Breaks into segments: "我", "每天", "走路", "去", "上班"
    String chinese = "我每天走路去上班。";
    assertThat(chinese.length()).isEqualTo(9);
    DocumentProto emailDocument1 =
        createEmailDocument("namespace", "uri1").toBuilder()
            .addProperties(PropertyProto.newBuilder().setName("subject").addStringValues(chinese))
            .build();
    assertStatusOk(icingSearchEngine.put(emailDocument1).getStatus());

    // Search and request snippet matching but no windowing.
    SearchSpecProto searchSpec =
        SearchSpecProto.newBuilder()
            .setQuery("每")
            .setTermMatchType(TermMatchType.Code.PREFIX)
            .build();
    ResultSpecProto resultSpecProto =
        ResultSpecProto.newBuilder()
            .setSnippetSpec(
                ResultSpecProto.SnippetSpecProto.newBuilder()
                    .setNumToSnippet(Integer.MAX_VALUE)
                    .setNumMatchesPerProperty(Integer.MAX_VALUE))
            .build();

    // Search and make sure that we got a single successful results
    SearchResultProto searchResultProto =
        icingSearchEngine.search(
            searchSpec, ScoringSpecProto.getDefaultInstance(), resultSpecProto);
    assertStatusOk(searchResultProto.getStatus());
    assertThat(searchResultProto.getResultsCount()).isEqualTo(1);

    // Ensure that one and only one property was matched and it was "subject"
    SnippetProto snippetProto = searchResultProto.getResults(0).getSnippet();
    assertThat(snippetProto.getEntriesList()).hasSize(1);
    SnippetProto.EntryProto entryProto = snippetProto.getEntries(0);
    assertThat(entryProto.getPropertyName()).isEqualTo("subject");

    // Get the content for "subject" and see what the match is.
    DocumentProto resultDocument = searchResultProto.getResults(0).getDocument();
    assertThat(resultDocument.getPropertiesList()).hasSize(1);
    PropertyProto subjectProperty = resultDocument.getProperties(0);
    assertThat(subjectProperty.getName()).isEqualTo("subject");
    assertThat(subjectProperty.getStringValuesList()).hasSize(1);
    String content = subjectProperty.getStringValues(0);

    // Ensure that there is one and only one match within "subject"
    assertThat(entryProto.getSnippetMatchesList()).hasSize(1);
    SnippetMatchProto matchProto = entryProto.getSnippetMatches(0);

    int matchStart = matchProto.getExactMatchUtf16Position();
    int matchEnd = matchStart + matchProto.getExactMatchUtf16Length();
    assertThat(matchStart).isEqualTo(1);
    assertThat(matchEnd).isEqualTo(3);
    String match = content.substring(matchStart, matchEnd);
    assertThat(match).isEqualTo("每天");
  }

  @Test
  public void testUtf16MultiByteSnippets() throws Exception {
    assertStatusOk(icingSearchEngine.initialize().getStatus());

    SchemaProto schema = SchemaProto.newBuilder().addTypes(createEmailTypeConfig()).build();
    assertStatusOk(
        icingSearchEngine.setSchema(schema, /*ignoreErrorsAndDeleteDocuments=*/ false).getStatus());

    // String:    "𐀀𐀁 𐀂𐀃 𐀄"
    //             ^  ^  ^
    // UTF16 idx:  0  5  10
    // Breaks into segments: "𐀀𐀁", "𐀂𐀃", "𐀄"
    String text = "𐀀𐀁 𐀂𐀃 𐀄";
    assertThat(text.length()).isEqualTo(12);
    DocumentProto emailDocument1 =
        createEmailDocument("namespace", "uri1").toBuilder()
            .addProperties(PropertyProto.newBuilder().setName("subject").addStringValues(text))
            .build();
    assertStatusOk(icingSearchEngine.put(emailDocument1).getStatus());

    // Search and request snippet matching but no windowing.
    SearchSpecProto searchSpec =
        SearchSpecProto.newBuilder()
            .setQuery("𐀂")
            .setTermMatchType(TermMatchType.Code.PREFIX)
            .build();
    ResultSpecProto resultSpecProto =
        ResultSpecProto.newBuilder()
            .setSnippetSpec(
                ResultSpecProto.SnippetSpecProto.newBuilder()
                    .setNumToSnippet(Integer.MAX_VALUE)
                    .setNumMatchesPerProperty(Integer.MAX_VALUE))
            .build();

    // Search and make sure that we got a single successful results
    SearchResultProto searchResultProto =
        icingSearchEngine.search(
            searchSpec, ScoringSpecProto.getDefaultInstance(), resultSpecProto);
    assertStatusOk(searchResultProto.getStatus());
    assertThat(searchResultProto.getResultsCount()).isEqualTo(1);

    // Ensure that one and only one property was matched and it was "subject"
    SnippetProto snippetProto = searchResultProto.getResults(0).getSnippet();
    assertThat(snippetProto.getEntriesList()).hasSize(1);
    SnippetProto.EntryProto entryProto = snippetProto.getEntries(0);
    assertThat(entryProto.getPropertyName()).isEqualTo("subject");

    // Get the content for "subject" and see what the match is.
    DocumentProto resultDocument = searchResultProto.getResults(0).getDocument();
    assertThat(resultDocument.getPropertiesList()).hasSize(1);
    PropertyProto subjectProperty = resultDocument.getProperties(0);
    assertThat(subjectProperty.getName()).isEqualTo("subject");
    assertThat(subjectProperty.getStringValuesList()).hasSize(1);
    String content = subjectProperty.getStringValues(0);

    // Ensure that there is one and only one match within "subject"
    assertThat(entryProto.getSnippetMatchesList()).hasSize(1);
    SnippetMatchProto matchProto = entryProto.getSnippetMatches(0);

    int matchStart = matchProto.getExactMatchUtf16Position();
    int matchEnd = matchStart + matchProto.getExactMatchUtf16Length();
    assertThat(matchStart).isEqualTo(5);
    assertThat(matchEnd).isEqualTo(9);
    String match = content.substring(matchStart, matchEnd);
    assertThat(match).isEqualTo("𐀂𐀃");
  }

  private static void assertStatusOk(StatusProto status) {
    assertWithMessage(status.getMessage()).that(status.getCode()).isEqualTo(StatusProto.Code.OK);
  }
}

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

#include <unistd.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "testing/base/public/benchmark.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/document-builder.h"
#include "icing/file/filesystem.h"
#include "icing/icing-search-engine.h"
#include "icing/proto/document.pb.h"
#include "icing/proto/initialize.pb.h"
#include "icing/proto/schema.pb.h"
#include "icing/proto/scoring.pb.h"
#include "icing/proto/search.pb.h"
#include "icing/proto/status.pb.h"
#include "icing/proto/term.pb.h"
#include "icing/schema-builder.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/document-generator.h"
#include "icing/testing/random-string.h"
#include "icing/testing/recorder-test-utils.h"
#include "icing/testing/schema-generator.h"
#include "icing/testing/tmp-directory.h"

// Run on a Linux workstation:
//    $ blaze build -c opt --dynamic_mode=off --copt=-gmlt
//    //icing:icing-search-engine_benchmark
//
//    $ blaze-bin/icing/icing-search-engine_benchmark
//    --benchmarks=all --benchmark_memory_usage
//
// Run on an Android device:
//    $ blaze build --copt="-DGOOGLE_COMMANDLINEFLAGS_FULL_API=1"
//    --config=android_arm64 -c opt --dynamic_mode=off --copt=-gmlt
//    //icing:icing-search-engine_benchmark
//
//    $ adb push blaze-bin/icing/icing-search-engine_benchmark
//    /data/local/tmp/
//
//    $ adb shell /data/local/tmp/icing-search-engine_benchmark --benchmarks=all

namespace icing {
namespace lib {

namespace {

using ::testing::Eq;
using ::testing::HasSubstr;

// Icing GMSCore has, on average, 17 corpora on a device and 30 corpora at the
// 95th pct. Most clients use a single type. This is a function of Icing's
// constrained type offering. Assume that each package will use 3 types on
// average.
constexpr int kAvgNumNamespaces = 10;
constexpr int kAvgNumTypes = 3;

// ASSUME: Properties will have at most ten properties. Types will be created
// with [1, 10] properties.
constexpr int kMaxNumProperties = 10;

// Based on logs from Icing GMSCore.
constexpr int kAvgDocumentSize = 300;

// ASSUME: ~75% of the document's size comes from it's content.
constexpr float kContentSizePct = 0.7;

// Average length of word in English is 4.7 characters.
constexpr int kAvgTokenLen = 5;
// Made up value. This results in a fairly reasonable language - the majority of
// generated words are 3-9 characters, ~3% of words are >=20 chars, and the
// longest ones are 27 chars, (roughly consistent with the longest,
// non-contrived English words
// https://en.wikipedia.org/wiki/Longest_word_in_English)
constexpr int kTokenStdDev = 7;
constexpr int kLanguageSize = 1000;

// Lite Index size required to fit 128k docs, each doc requires ~64 bytes of
// space in the lite index.
constexpr int kIcingFullIndexSize = 1024 * 1024 * 8;

// Query params
constexpr int kNumPerPage = 10;
constexpr int kNumToSnippet = 10000;
constexpr int kMatchesPerProperty = 1;

std::vector<std::string> CreateNamespaces(int num_namespaces) {
  std::vector<std::string> namespaces;
  while (--num_namespaces >= 0) {
    namespaces.push_back("comgooglepackage" + std::to_string(num_namespaces));
  }
  return namespaces;
}

// Creates a vector containing num_words randomly-generated words for use by
// documents.
template <typename Rand>
std::vector<std::string> CreateLanguages(int num_words, Rand* r) {
  std::vector<std::string> language;
  std::normal_distribution<> norm_dist(kAvgTokenLen, kTokenStdDev);
  while (--num_words >= 0) {
    int word_length = 0;
    while (word_length < 1) {
      word_length = std::round(norm_dist(*r));
    }
    language.push_back(RandomString(kAlNumAlphabet, word_length, r));
  }
  return language;
}

SearchSpecProto CreateSearchSpec(const std::string& query,
                                 const std::vector<std::string>& namespaces,
                                 TermMatchType::Code match_type) {
  SearchSpecProto search_spec;
  search_spec.set_query(query);
  for (const std::string& name_space : namespaces) {
    search_spec.add_namespace_filters(name_space);
  }
  search_spec.set_term_match_type(match_type);
  return search_spec;
}

ResultSpecProto CreateResultSpec(int num_per_page, int num_to_snippet,
                                 int matches_per_property) {
  ResultSpecProto result_spec;
  result_spec.set_num_per_page(num_per_page);
  result_spec.mutable_snippet_spec()->set_num_to_snippet(num_to_snippet);
  result_spec.mutable_snippet_spec()->set_num_matches_per_property(
      matches_per_property);
  return result_spec;
}

ScoringSpecProto CreateScoringSpec(
    ScoringSpecProto::RankingStrategy::Code ranking_strategy) {
  ScoringSpecProto scoring_spec;
  scoring_spec.set_rank_by(ranking_strategy);
  return scoring_spec;
}

class DestructibleDirectory {
 public:
  explicit DestructibleDirectory(const Filesystem& filesystem,
                                 const std::string& dir)
      : filesystem_(filesystem), dir_(dir) {
    filesystem_.CreateDirectoryRecursively(dir_.c_str());
  }
  ~DestructibleDirectory() {
    filesystem_.DeleteDirectoryRecursively(dir_.c_str());
  }

 private:
  Filesystem filesystem_;
  std::string dir_;
};

std::vector<DocumentProto> GenerateRandomDocuments(
    EvenDistributionTypeSelector* type_selector, int num_docs) {
  std::vector<std::string> namespaces = CreateNamespaces(kAvgNumNamespaces);
  EvenDistributionNamespaceSelector namespace_selector(namespaces);

  std::default_random_engine random;
  std::vector<std::string> language = CreateLanguages(kLanguageSize, &random);
  UniformDistributionLanguageTokenGenerator<std::default_random_engine>
      token_generator(language, &random);

  DocumentGenerator<
      EvenDistributionNamespaceSelector, EvenDistributionTypeSelector,
      UniformDistributionLanguageTokenGenerator<std::default_random_engine>>
      generator(&namespace_selector, type_selector, &token_generator,
                kAvgDocumentSize * kContentSizePct);

  std::vector<DocumentProto> random_docs;
  random_docs.reserve(num_docs);
  for (int i = 0; i < num_docs; i++) {
    random_docs.push_back(generator.generateDoc());
  }
  return random_docs;
}

void BM_IndexLatency(benchmark::State& state) {
  // Initialize the filesystem
  std::string test_dir = GetTestTempDir() + "/icing/benchmark";
  Filesystem filesystem;
  DestructibleDirectory ddir(filesystem, test_dir);

  // Create the schema.
  std::default_random_engine random;
  int num_types = kAvgNumNamespaces * kAvgNumTypes;
  ExactStringPropertyGenerator property_generator;
  SchemaGenerator<ExactStringPropertyGenerator> schema_generator(
      /*num_properties=*/state.range(1), &property_generator);
  SchemaProto schema = schema_generator.GenerateSchema(num_types);
  EvenDistributionTypeSelector type_selector(schema);

  // Create the index.
  IcingSearchEngineOptions options;
  options.set_base_dir(test_dir);
  options.set_index_merge_size(kIcingFullIndexSize);
  std::unique_ptr<IcingSearchEngine> icing =
      std::make_unique<IcingSearchEngine>(options);

  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing->SetSchema(schema).status(), ProtoIsOk());

  int num_docs = state.range(0);
  const std::vector<DocumentProto> random_docs =
      GenerateRandomDocuments(&type_selector, num_docs);
  Timer timer;
  for (const DocumentProto& doc : random_docs) {
    ASSERT_THAT(icing->Put(doc).status(), ProtoIsOk());
  }
  int64_t time_taken_ns = timer.GetElapsedNanoseconds();
  int64_t time_per_doc_ns = time_taken_ns / num_docs;
  std::cout << "Number of indexed documents:\t" << num_docs
            << "\t\tNumber of indexed sections:\t" << state.range(1)
            << "\t\tTime taken (ms):\t" << time_taken_ns / 1000000
            << "\t\tTime taken per doc (us):\t" << time_per_doc_ns / 1000
            << std::endl;
}
BENCHMARK(BM_IndexLatency)
    // Arguments: num_indexed_documents, num_sections
    ->ArgPair(1, 1)
    ->ArgPair(2, 1)
    ->ArgPair(8, 1)
    ->ArgPair(32, 1)
    ->ArgPair(128, 1)
    ->ArgPair(1 << 10, 1)
    ->ArgPair(1 << 13, 1)
    ->ArgPair(1 << 15, 1)
    ->ArgPair(1 << 17, 1)
    ->ArgPair(1, 5)
    ->ArgPair(2, 5)
    ->ArgPair(8, 5)
    ->ArgPair(32, 5)
    ->ArgPair(128, 5)
    ->ArgPair(1 << 10, 5)
    ->ArgPair(1 << 13, 5)
    ->ArgPair(1 << 15, 5)
    ->ArgPair(1 << 17, 5)
    ->ArgPair(1, 10)
    ->ArgPair(2, 10)
    ->ArgPair(8, 10)
    ->ArgPair(32, 10)
    ->ArgPair(128, 10)
    ->ArgPair(1 << 10, 10)
    ->ArgPair(1 << 13, 10)
    ->ArgPair(1 << 15, 10)
    ->ArgPair(1 << 17, 10);

void BM_IndexThroughput(benchmark::State& state) {
  // Initialize the filesystem
  std::string test_dir = GetTestTempDir() + "/icing/benchmark";
  Filesystem filesystem;
  DestructibleDirectory ddir(filesystem, test_dir);

  // Create the schema.
  std::default_random_engine random;
  int num_types = kAvgNumNamespaces * kAvgNumTypes;
  ExactStringPropertyGenerator property_generator;
  SchemaGenerator<ExactStringPropertyGenerator> schema_generator(
      /*num_properties=*/state.range(1), &property_generator);
  SchemaProto schema = schema_generator.GenerateSchema(num_types);
  EvenDistributionTypeSelector type_selector(schema);

  // Create the index.
  IcingSearchEngineOptions options;
  options.set_base_dir(test_dir);
  options.set_index_merge_size(kIcingFullIndexSize);
  std::unique_ptr<IcingSearchEngine> icing =
      std::make_unique<IcingSearchEngine>(options);

  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing->SetSchema(schema).status(), ProtoIsOk());

  int num_docs = state.range(0);
  const std::vector<DocumentProto> random_docs =
      GenerateRandomDocuments(&type_selector, num_docs);
  for (auto s : state) {
    for (const DocumentProto& doc : random_docs) {
      ASSERT_THAT(icing->Put(doc).status(), ProtoIsOk());
    }
  }
  state.SetItemsProcessed(state.iterations() * num_docs);
}
BENCHMARK(BM_IndexThroughput)
    // Arguments: num_indexed_documents, num_sections
    ->ArgPair(1, 1)
    ->ArgPair(2, 1)
    ->ArgPair(8, 1)
    ->ArgPair(32, 1)
    ->ArgPair(128, 1)
    ->ArgPair(1 << 10, 1)
    ->ArgPair(1 << 13, 1)
    ->ArgPair(1 << 15, 1)
    ->ArgPair(1 << 17, 1)
    ->ArgPair(1, 5)
    ->ArgPair(2, 5)
    ->ArgPair(8, 5)
    ->ArgPair(32, 5)
    ->ArgPair(128, 5)
    ->ArgPair(1 << 10, 5)
    ->ArgPair(1 << 13, 5)
    ->ArgPair(1 << 15, 5)
    ->ArgPair(1 << 17, 5)
    ->ArgPair(1, 10)
    ->ArgPair(2, 10)
    ->ArgPair(8, 10)
    ->ArgPair(32, 10)
    ->ArgPair(128, 10)
    ->ArgPair(1 << 10, 10)
    ->ArgPair(1 << 13, 10)
    ->ArgPair(1 << 15, 10)
    ->ArgPair(1 << 17, 10);

void BM_MutlipleIndices(benchmark::State& state) {
  // Initialize the filesystem
  std::string test_dir = GetTestTempDir() + "/icing/benchmark";
  Filesystem filesystem;
  DestructibleDirectory ddir(filesystem, test_dir);

  // Create the schema.
  std::default_random_engine random;
  int num_types = kAvgNumNamespaces * kAvgNumTypes;
  ExactStringPropertyGenerator property_generator;
  RandomSchemaGenerator<std::default_random_engine,
                        ExactStringPropertyGenerator>
      schema_generator(&random, &property_generator);
  SchemaProto schema =
      schema_generator.GenerateSchema(num_types, kMaxNumProperties);
  EvenDistributionTypeSelector type_selector(schema);

  // Create the indices.
  std::vector<std::unique_ptr<IcingSearchEngine>> icings;
  int num_indices = state.range(0);
  for (int i = 0; i < num_indices; ++i) {
    IcingSearchEngineOptions options;
    std::string base_dir = test_dir + "/" + std::to_string(i);
    options.set_base_dir(base_dir);
    options.set_index_merge_size(kIcingFullIndexSize / num_indices);
    auto icing = std::make_unique<IcingSearchEngine>(options);

    ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
    ASSERT_THAT(icing->SetSchema(schema).status(), ProtoIsOk());
    icings.push_back(std::move(icing));
  }

  // Setup namespace info and language
  std::vector<std::string> namespaces = CreateNamespaces(kAvgNumNamespaces);
  EvenDistributionNamespaceSelector namespace_selector(namespaces);

  std::vector<std::string> language = CreateLanguages(kLanguageSize, &random);
  UniformDistributionLanguageTokenGenerator<std::default_random_engine>
      token_generator(language, &random);

  // Fill the index.
  DocumentGenerator<
      EvenDistributionNamespaceSelector, EvenDistributionTypeSelector,
      UniformDistributionLanguageTokenGenerator<std::default_random_engine>>
      generator(&namespace_selector, &type_selector, &token_generator,
                kAvgDocumentSize * kContentSizePct);
  for (int i = 0; i < state.range(1); ++i) {
    DocumentProto doc = generator.generateDoc();
    PutResultProto put_result;
    if (icings.empty()) {
      ASSERT_THAT(put_result.status().code(), Eq(StatusProto::UNKNOWN));
      continue;
    }
    ASSERT_THAT(icings.at(i % icings.size())->Put(doc).status(), ProtoIsOk());
  }

  // QUERY!
  // Every document has its own namespace as a token. This query that should
  // match 1/kAvgNumNamespace% of all documents.
  const std::string& name_space = namespaces.at(0);
  SearchSpecProto search_spec = CreateSearchSpec(
      /*query=*/name_space, {name_space}, TermMatchType::EXACT_ONLY);
  ResultSpecProto result_spec =
      CreateResultSpec(kNumPerPage, kNumToSnippet, kMatchesPerProperty);
  ScoringSpecProto scoring_spec =
      CreateScoringSpec(ScoringSpecProto::RankingStrategy::CREATION_TIMESTAMP);

  int num_results = 0;
  for (auto _ : state) {
    num_results = 0;
    SearchResultProto result;
    if (icings.empty()) {
      ASSERT_THAT(result.status().code(), Eq(StatusProto::UNKNOWN));
      continue;
    }
    result = icings.at(0)->Search(search_spec, scoring_spec, result_spec);
    ASSERT_THAT(result.status(), ProtoIsOk());
    while (!result.results().empty()) {
      num_results += result.results_size();
      if (!icings.empty()) {
        result = icings.at(0)->GetNextPage(result.next_page_token());
      }
      ASSERT_THAT(result.status(), ProtoIsOk());
    }
  }

  // Measure size.
  int64_t disk_usage = filesystem.GetDiskUsage(test_dir.c_str());
  std::cout << "Num results:\t" << num_results << "\t\tDisk Use:\t"
            << disk_usage / 1024.0 << std::endl;
}
BENCHMARK(BM_MutlipleIndices)
    // First argument: num_indices, Second argument: num_total_documents
    // So each index will contain (num_total_documents / num_indices) documents.
    ->ArgPair(0, 0)
    ->ArgPair(0, 1024)
    ->ArgPair(0, 131072)
    ->ArgPair(1, 0)
    ->ArgPair(1, 1)
    ->ArgPair(1, 2)
    ->ArgPair(1, 8)
    ->ArgPair(1, 32)
    ->ArgPair(1, 128)
    ->ArgPair(1, 1024)
    ->ArgPair(1, 8192)
    ->ArgPair(1, 32768)
    ->ArgPair(1, 131072)
    ->ArgPair(2, 0)
    ->ArgPair(2, 1)
    ->ArgPair(2, 2)
    ->ArgPair(2, 8)
    ->ArgPair(2, 32)
    ->ArgPair(2, 128)
    ->ArgPair(2, 1024)
    ->ArgPair(2, 8192)
    ->ArgPair(2, 32768)
    ->ArgPair(2, 131072)
    ->ArgPair(10, 0)
    ->ArgPair(10, 1)
    ->ArgPair(10, 2)
    ->ArgPair(10, 8)
    ->ArgPair(10, 32)
    ->ArgPair(10, 128)
    ->ArgPair(10, 1024)
    ->ArgPair(10, 8192)
    ->ArgPair(10, 32768)
    ->ArgPair(10, 131072);

void BM_SearchNoStackOverflow(benchmark::State& state) {
  // Initialize the filesystem
  std::string test_dir = GetTestTempDir() + "/icing/benchmark";
  Filesystem filesystem;
  DestructibleDirectory ddir(filesystem, test_dir);

  // Create the schema.
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("Message").AddProperty(
              PropertyConfigBuilder()
                  .SetName("body")
                  .SetDataTypeString(TermMatchType::PREFIX,
                                     StringIndexingConfig::TokenizerType::PLAIN)
                  .SetCardinality(PropertyConfigProto::Cardinality::OPTIONAL)))
          .Build();

  // Create the index.
  IcingSearchEngineOptions options;
  options.set_base_dir(test_dir);
  options.set_index_merge_size(kIcingFullIndexSize);
  std::unique_ptr<IcingSearchEngine> icing =
      std::make_unique<IcingSearchEngine>(options);

  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing->SetSchema(schema).status(), ProtoIsOk());

  // Create a document that has the term "foo"
  DocumentProto base_document = DocumentBuilder()
                                    .SetSchema("Message")
                                    .SetNamespace("namespace")
                                    .AddStringProperty("body", "foo")
                                    .Build();

  // Insert a lot of documents with the term "foo"
  int64_t num_docs = state.range(0);
  for (int64_t i = 0; i < num_docs; ++i) {
    DocumentProto document =
        DocumentBuilder(base_document).SetUri(std::to_string(i)).Build();
    ASSERT_THAT(icing->Put(document).status(), ProtoIsOk());
  }

  // Do a query and exclude documents with the term "foo". The way this is
  // currently implemented is that we'll iterate over all the documents in the
  // index, then apply the exclusion check. Since all our documents have "foo",
  // we'll consider it a "miss". Previously with recursion, we would have
  // recursed until we got a success, which would never happen causing us to
  // recurse through all the documents and trigger a stack overflow. With
  // the iterative implementation, we should avoid this.
  SearchSpecProto search_spec;
  search_spec.set_query("-foo");
  search_spec.set_term_match_type(TermMatchType::PREFIX);

  ResultSpecProto result_spec;
  ScoringSpecProto scoring_spec;
  for (auto s : state) {
    icing->Search(search_spec, scoring_spec, result_spec);
  }
}
// For other reasons, we hit a limit when inserting the ~350,000th document. So
// cap the limit to 1 << 18.
BENCHMARK(BM_SearchNoStackOverflow)
    ->Range(/*start=*/1 << 10, /*limit=*/1 << 18);

// Added for b/184373205. Ensure that we can repeatedly put documents even if
// the underlying mmapped areas grow past a few page sizes.
void BM_RepeatedPut(benchmark::State& state) {
  // Initialize the filesystem
  std::string test_dir = GetTestTempDir() + "/icing/benchmark";
  Filesystem filesystem;
  DestructibleDirectory ddir(filesystem, test_dir);

  // Create the schema.
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("Message").AddProperty(
              PropertyConfigBuilder()
                  .SetName("body")
                  .SetDataTypeString(TermMatchType::PREFIX,
                                     StringIndexingConfig::TokenizerType::PLAIN)
                  .SetCardinality(PropertyConfigProto::Cardinality::OPTIONAL)))
          .Build();

  // Create the index.
  IcingSearchEngineOptions options;
  options.set_base_dir(test_dir);
  options.set_index_merge_size(kIcingFullIndexSize);
  std::unique_ptr<IcingSearchEngine> icing =
      std::make_unique<IcingSearchEngine>(options);

  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing->SetSchema(schema).status(), ProtoIsOk());

  // Create a document that has the term "foo"
  DocumentProto base_document = DocumentBuilder()
                                    .SetSchema("Message")
                                    .SetNamespace("namespace")
                                    .AddStringProperty("body", "foo")
                                    .Build();

  // Insert a lot of documents with the term "foo"
  int64_t num_docs = state.range(0);
  for (auto s : state) {
    for (int64_t i = 0; i < num_docs; ++i) {
      DocumentProto document =
          DocumentBuilder(base_document).SetUri("uri").Build();
      ASSERT_THAT(icing->Put(document).status(), ProtoIsOk());
    }
  }
}
// For other reasons, we hit a limit when inserting the ~350,000th document. So
// cap the limit to 1 << 18.
BENCHMARK(BM_RepeatedPut)->Range(/*start=*/100, /*limit=*/1 << 18);

// This is different from BM_RepeatedPut since we're just trying to benchmark
// one Put call, not thousands of them at once.
void BM_Put(benchmark::State& state) {
  // Initialize the filesystem
  std::string test_dir = GetTestTempDir() + "/icing/benchmark";
  Filesystem filesystem;
  DestructibleDirectory ddir(filesystem, test_dir);

  // Create the schema.
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("Message"))
          .Build();

  // Create the index.
  IcingSearchEngineOptions options;
  options.set_base_dir(test_dir);
  options.set_index_merge_size(kIcingFullIndexSize);
  std::unique_ptr<IcingSearchEngine> icing =
      std::make_unique<IcingSearchEngine>(options);

  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing->SetSchema(schema).status(), ProtoIsOk());

  // Create a document
  DocumentProto document = DocumentBuilder()
                               .SetSchema("Message")
                               .SetNamespace("namespace")
                               .SetUri("uri")
                               .Build();

  for (auto s : state) {
    benchmark::DoNotOptimize(icing->Put(document));
  }
}
BENCHMARK(BM_Put);

void BM_Get(benchmark::State& state) {
  // Initialize the filesystem
  std::string test_dir = GetTestTempDir() + "/icing/benchmark";
  Filesystem filesystem;
  DestructibleDirectory ddir(filesystem, test_dir);

  // Create the schema.
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("Message"))
          .Build();

  // Create the index.
  IcingSearchEngineOptions options;
  options.set_base_dir(test_dir);
  options.set_index_merge_size(kIcingFullIndexSize);
  std::unique_ptr<IcingSearchEngine> icing =
      std::make_unique<IcingSearchEngine>(options);

  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing->SetSchema(schema).status(), ProtoIsOk());

  // Create a document
  DocumentProto document = DocumentBuilder()
                               .SetSchema("Message")
                               .SetNamespace("namespace")
                               .SetUri("uri")
                               .Build();

  ASSERT_THAT(icing->Put(document).status(), ProtoIsOk());
  for (auto s : state) {
    benchmark::DoNotOptimize(
        icing->Get("namespace", "uri", GetResultSpecProto::default_instance()));
  }
}
BENCHMARK(BM_Get);

void BM_Delete(benchmark::State& state) {
  // Initialize the filesystem
  std::string test_dir = GetTestTempDir() + "/icing/benchmark";
  Filesystem filesystem;
  DestructibleDirectory ddir(filesystem, test_dir);

  // Create the schema.
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("Message"))
          .Build();

  // Create the index.
  IcingSearchEngineOptions options;
  options.set_base_dir(test_dir);
  options.set_index_merge_size(kIcingFullIndexSize);
  std::unique_ptr<IcingSearchEngine> icing =
      std::make_unique<IcingSearchEngine>(options);

  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing->SetSchema(schema).status(), ProtoIsOk());

  // Create a document
  DocumentProto document = DocumentBuilder()
                               .SetSchema("Message")
                               .SetNamespace("namespace")
                               .SetUri("uri")
                               .Build();

  ASSERT_THAT(icing->Put(document).status(), ProtoIsOk());
  for (auto s : state) {
    state.PauseTiming();
    icing->Put(document);
    state.ResumeTiming();

    benchmark::DoNotOptimize(icing->Delete("namespace", "uri"));
  }
}
BENCHMARK(BM_Delete);

void BM_PutMaxAllowedDocuments(benchmark::State& state) {
  // Initialize the filesystem
  std::string test_dir = GetTestTempDir() + "/icing/benchmark";
  Filesystem filesystem;
  DestructibleDirectory ddir(filesystem, test_dir);

  // Create the schema.
  SchemaProto schema =
      SchemaBuilder()
          .AddType(SchemaTypeConfigBuilder().SetType("Message").AddProperty(
              PropertyConfigBuilder()
                  .SetName("body")
                  .SetDataTypeString(TermMatchType::PREFIX,
                                     StringIndexingConfig::TokenizerType::PLAIN)
                  .SetCardinality(PropertyConfigProto::Cardinality::OPTIONAL)))
          .Build();

  // Create the index.
  IcingSearchEngineOptions options;
  options.set_base_dir(test_dir);
  options.set_index_merge_size(kIcingFullIndexSize);
  std::unique_ptr<IcingSearchEngine> icing =
      std::make_unique<IcingSearchEngine>(options);

  ASSERT_THAT(icing->Initialize().status(), ProtoIsOk());
  ASSERT_THAT(icing->SetSchema(schema).status(), ProtoIsOk());

  // Create a document that has the term "foo"
  DocumentProto base_document = DocumentBuilder()
                                    .SetSchema("Message")
                                    .SetNamespace("namespace")
                                    .AddStringProperty("body", "foo")
                                    .Build();

  // Insert a lot of documents with the term "foo"
  for (auto s : state) {
    for (int64_t i = 0; i <= kMaxDocumentId; ++i) {
      DocumentProto document =
          DocumentBuilder(base_document).SetUri(std::to_string(i)).Build();
      EXPECT_THAT(icing->Put(document).status(), ProtoIsOk());
    }
  }

  DocumentProto document =
      DocumentBuilder(base_document).SetUri("out_of_space_uri").Build();
  PutResultProto put_result_proto = icing->Put(document);
  EXPECT_THAT(put_result_proto.status(),
              ProtoStatusIs(StatusProto::OUT_OF_SPACE));
  EXPECT_THAT(put_result_proto.status().message(),
              HasSubstr("Exceeded maximum number of documents"));
}
BENCHMARK(BM_PutMaxAllowedDocuments);

}  // namespace

}  // namespace lib
}  // namespace icing

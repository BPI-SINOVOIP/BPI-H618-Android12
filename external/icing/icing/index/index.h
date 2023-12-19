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

#ifndef ICING_INDEX_INDEX_H_
#define ICING_INDEX_INDEX_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "icing/text_classifier/lib3/utils/base/status.h"
#include "icing/text_classifier/lib3/utils/base/statusor.h"
#include "icing/file/filesystem.h"
#include "icing/index/hit/hit.h"
#include "icing/index/iterator/doc-hit-info-iterator.h"
#include "icing/index/lite/lite-index.h"
#include "icing/index/main/main-index-merger.h"
#include "icing/index/main/main-index.h"
#include "icing/index/term-id-codec.h"
#include "icing/index/term-metadata.h"
#include "icing/legacy/index/icing-filesystem.h"
#include "icing/proto/storage.pb.h"
#include "icing/proto/term.pb.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"
#include "icing/store/namespace-id.h"
#include "icing/util/crc32.h"

namespace icing {
namespace lib {

// The class representing the Icing search index. This index maps terms to hits
// (document_ids, section_ids).
// Content is added to the index through the Editor class - which also dedupes
// hits (calling Editor::AddHit with the same arguments will only result in the
// creation of a single hit).
// Ex.
// ICING_ASSIGN_OR_RETURN(std::unique_ptr<Index> index,
// .                Index::Create(MakeIndexOptions()));
// Index::Editor editor = index->Edit(document_id, section_id,
//     TermMatchType::EXACT_ONLY); ICING_RETURN_IF_ERROR(editor.AddHit("foo"));
// ICING_RETURN_IF_ERROR(editor.AddHit("baz"));
//
// Content is retrieved from the index through the Iterator class.
// Ex.
// ICING_ASSIGN_OR_RETURN(std::unique_ptr<Index> index,
// .                Index::Create(MakeIndexOptions()));
// ICING_ASSIGN_OR_RETURN(Index::Iterator iterator =
//     index->GetIterator("foo", kSectionIdMaskAll, TermMatchType::EXACT_ONLY));
// while(iterator->Advance().ok())
//   ProcessResult(iterator->value());
class Index {
 public:
  struct Options {
    explicit Options(const std::string& base_dir, uint32_t index_merge_size)
        : base_dir(base_dir), index_merge_size(index_merge_size) {}

    std::string base_dir;
    int32_t index_merge_size;
  };

  // Creates an instance of Index in the directory pointed by file_dir.
  //
  // Returns:
  //   Valid Index on success
  //   DATA_LOSS if the index was corrupt and had to be cleared
  //   INVALID_ARGUMENT if options have invalid values
  //   INTERNAL on I/O error
  static libtextclassifier3::StatusOr<std::unique_ptr<Index>> Create(
      const Options& options, const Filesystem* filesystem,
      const IcingFilesystem* icing_filesystem);

  // Clears all files created by the index. Returns OK if all files were
  // cleared.
  libtextclassifier3::Status Reset() {
    ICING_RETURN_IF_ERROR(lite_index_->Reset());
    return main_index_->Reset();
  }

  // Brings components of the index into memory in anticipation of a query in
  // order to reduce latency.
  void Warm() {
    lite_index_->Warm();
    main_index_->Warm();
  }

  // Syncs all the data and metadata changes to disk.
  //
  // Returns:
  //   OK on success
  //   INTERNAL on I/O errors
  libtextclassifier3::Status PersistToDisk() {
    ICING_RETURN_IF_ERROR(lite_index_->PersistToDisk());
    return main_index_->PersistToDisk();
  }

  // Discard parts of the index if they contain data for document ids greater
  // than document_id.
  //
  // NOTE: This means that TruncateTo(kInvalidDocumentId) will have no effect.
  //
  // Returns:
  //   OK on success
  //   INTERNAL on I/O errors
  libtextclassifier3::Status TruncateTo(DocumentId document_id);

  // DocumentIds are always inserted in increasing order. Returns the largest
  // document_id added to the index.
  DocumentId last_added_document_id() const {
    DocumentId lite_document_id = lite_index_->last_added_document_id();
    if (lite_document_id != kInvalidDocumentId) {
      return lite_document_id;
    }
    return main_index_->last_added_document_id();
  }

  // Sets last_added_document_id to document_id so long as document_id >
  // last_added_document_id()
  void set_last_added_document_id(DocumentId document_id) {
    DocumentId lite_document_id = lite_index_->last_added_document_id();
    if (lite_document_id == kInvalidDocumentId ||
        document_id >= lite_document_id) {
      lite_index_->set_last_added_document_id(document_id);
    }
  }

  // Returns debug information for the index in out.
  // verbosity <= 0, simplest debug information - just the lexicons and lite
  //                 index.
  // verbosity > 0, more detailed debug information including raw postings
  //                lists.
  void GetDebugInfo(int verbosity, std::string* out) const {
    lite_index_->GetDebugInfo(verbosity, out);
    main_index_->GetDebugInfo(verbosity, out);
  }

  // Returns the byte size of the all the elements held in the index. This
  // excludes the size of any internal metadata of the index, e.g. the index's
  // header.
  //
  // Returns:
  //   Byte size on success
  //   INTERNAL_ERROR on IO error
  libtextclassifier3::StatusOr<int64_t> GetElementsSize() const {
    ICING_ASSIGN_OR_RETURN(int64_t lite_index_size,
                           lite_index_->GetElementsSize());
    ICING_ASSIGN_OR_RETURN(int64_t main_index_size,
                           main_index_->GetElementsSize());
    return lite_index_size + main_index_size;
  }

  // Calculates the StorageInfo for the Index.
  //
  // If an IO error occurs while trying to calculate the value for a field, then
  // that field will be set to -1.
  IndexStorageInfoProto GetStorageInfo() const;

  // Create an iterator to iterate through all doc hit infos in the index that
  // match the term. section_id_mask can be set to ignore hits from sections not
  // listed in the mask. Eg. section_id_mask = 1U << 3; would only return hits
  // that occur in section 3.
  //
  // Returns:
  //   unique ptr to a valid DocHitInfoIterator that matches the term
  //   INVALID_ARGUMENT if given an invalid term_match_type
  libtextclassifier3::StatusOr<std::unique_ptr<DocHitInfoIterator>> GetIterator(
      const std::string& term, SectionIdMask section_id_mask,
      TermMatchType::Code term_match_type);

  // Finds terms with the given prefix in the given namespaces. If
  // 'namespace_ids' is empty, returns results from all the namespaces. The
  // input prefix must be normalized, otherwise inaccurate results may be
  // returned. Results are not sorted specifically and are in their original
  // order. Number of results are no more than 'num_to_return'.
  //
  // Returns:
  //   A list of TermMetadata on success
  //   INTERNAL_ERROR if failed to access term data.
  libtextclassifier3::StatusOr<std::vector<TermMetadata>> FindTermsByPrefix(
      const std::string& prefix, const std::vector<NamespaceId>& namespace_ids,
      int num_to_return);

  // A class that can be used to add hits to the index.
  //
  // An editor groups hits from a particular section within a document together
  // and dedupes hits for the same term within a section. This removes the
  // burden of deduping from the caller and direct access to the index
  // implementation allows for more efficient deduping.
  class Editor {
   public:
    // Does not take any ownership, and all pointers must refer to valid objects
    // that outlive the one constructed.
    // TODO(b/141180665): Add nullptr checks for the raw pointers
    Editor(const TermIdCodec* term_id_codec, LiteIndex* lite_index,
           DocumentId document_id, SectionId section_id,
           TermMatchType::Code term_match_type, NamespaceId namespace_id)
        : term_id_codec_(term_id_codec),
          lite_index_(lite_index),
          document_id_(document_id),
          term_match_type_(term_match_type),
          namespace_id_(namespace_id),
          section_id_(section_id) {}

    // Buffer the term in seen_tokens_.
    libtextclassifier3::Status BufferTerm(const char* term);
    // Index all the terms stored in seen_tokens_.
    libtextclassifier3::Status IndexAllBufferedTerms();

   private:
    // The Editor is able to store previously seen terms as TermIds. This is
    // is more efficient than a client doing this externally because TermIds are
    // not exposed to clients.
    std::unordered_map<uint32_t, Hit::TermFrequency> seen_tokens_;
    const TermIdCodec* term_id_codec_;
    LiteIndex* lite_index_;
    DocumentId document_id_;
    TermMatchType::Code term_match_type_;
    NamespaceId namespace_id_;
    SectionId section_id_;
  };
  Editor Edit(DocumentId document_id, SectionId section_id,
              TermMatchType::Code term_match_type, NamespaceId namespace_id) {
    return Editor(term_id_codec_.get(), lite_index_.get(), document_id,
                  section_id, term_match_type, namespace_id);
  }

  bool WantsMerge() const { return lite_index_->WantsMerge(); }

  // Merges newly-added hits in the LiteIndex into the MainIndex.
  //
  // RETURNS:
  //  - INTERNAL on IO error while writing to the MainIndex.
  //  - RESOURCE_EXHAUSTED error if unable to grow the index.
  libtextclassifier3::Status Merge() {
    ICING_ASSIGN_OR_RETURN(MainIndex::LexiconMergeOutputs outputs,
                           main_index_->MergeLexicon(lite_index_->lexicon()));
    ICING_ASSIGN_OR_RETURN(std::vector<TermIdHitPair> term_id_hit_pairs,
                           MainIndexMerger::TranslateAndExpandLiteHits(
                               *lite_index_, *term_id_codec_, outputs));
    ICING_RETURN_IF_ERROR(main_index_->AddHits(
        *term_id_codec_, std::move(outputs.backfill_map),
        std::move(term_id_hit_pairs), lite_index_->last_added_document_id()));
    return lite_index_->Reset();
  }

 private:
  Index(const Options& options, std::unique_ptr<TermIdCodec> term_id_codec,
        std::unique_ptr<LiteIndex> lite_index,
        std::unique_ptr<MainIndex> main_index, const Filesystem* filesystem)
      : lite_index_(std::move(lite_index)),
        main_index_(std::move(main_index)),
        options_(options),
        term_id_codec_(std::move(term_id_codec)),
        filesystem_(filesystem) {}

  libtextclassifier3::StatusOr<std::vector<TermMetadata>> FindLiteTermsByPrefix(
      const std::string& prefix, const std::vector<NamespaceId>& namespace_ids,
      int num_to_return);

  std::unique_ptr<LiteIndex> lite_index_;
  std::unique_ptr<MainIndex> main_index_;
  const Options options_;
  std::unique_ptr<TermIdCodec> term_id_codec_;
  const Filesystem* filesystem_;
};

}  // namespace lib
}  // namespace icing

#endif  // ICING_INDEX_INDEX_H_

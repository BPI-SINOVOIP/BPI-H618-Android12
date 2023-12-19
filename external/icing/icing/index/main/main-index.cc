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
#include "icing/index/main/main-index.h"

#include <cstdint>
#include <cstring>
#include <memory>

#include "icing/absl_ports/canonical_errors.h"
#include "icing/absl_ports/str_cat.h"
#include "icing/index/main/index-block.h"
#include "icing/index/term-id-codec.h"
#include "icing/index/term-property-id.h"
#include "icing/legacy/index/icing-dynamic-trie.h"
#include "icing/util/status-macros.h"

namespace icing {
namespace lib {

namespace {

// Finds the shortest,valid prefix term with prefix hits in lexicon for which
// "prefix" is a prefix.
// Returns a valid FindTermResult with found=true if either:
//   1. prefix exists as a term in lexicon.
//   2. the shortest, valid prefix in the lexicon exists and contains prefix
//      hits.
// Returns a FindTermResult with found=false and undefined values of tvi and
// exact if no term was found.
struct FindTermResult {
  // TVI of the term that was found. Undefined if found=false.
  uint32_t tvi;
  // Whether or not a valid term with prefix hits was found.
  bool found;
  // Whether or not that term is equal to 'prefix'
  bool exact;
};
FindTermResult FindShortestValidTermWithPrefixHits(
    const IcingDynamicTrie* lexicon, const std::string& prefix) {
  // For prefix indexing: when we are doing a prefix match for "prefix", find
  // the tvi to the equivalent posting list. prefix's own posting list might not
  // exist but one of its children acts as a proxy.
  IcingDynamicTrie::PropertyReader hits_in_prefix_section(
      *lexicon, GetHasHitsInPrefixSectionPropertyId());
  uint32_t tvi = 0;
  bool found = false;
  bool exact = false;
  for (IcingDynamicTrie::Iterator it(*lexicon, prefix.c_str()); it.IsValid();
       it.Advance()) {
    PostingListIdentifier posting_list_id = PostingListIdentifier::kInvalid;
    memcpy(&posting_list_id, it.GetValue(), sizeof(posting_list_id));

    // Posting list id might be invalid if this is also a backfill term.
    // Suppose that the main index has two pre-existing prefix hits "foot" and
    // "fool" - it will have a branch point posting list for "foo". Then, let's
    // suppose that the other index adds hits for "foul", "four" and "far". This
    // will result in branch points for "fo" and "f".
    // If "fo" was added before "f", then the iterator would first give us "fo".
    // "fo" will have an invalid posting_list_id because it hasn't been
    // backfilled yet, so we need to continue iterating to "foo".
    if (posting_list_id.is_valid()) {
      exact = (prefix.size() == strlen(it.GetKey()));
      tvi = it.GetValueIndex();
      // Found it. Does it have prefix hits?
      found = exact || hits_in_prefix_section.HasProperty(tvi);
      break;
    }
  }
  FindTermResult result = {tvi, found, exact};
  return result;
}

}  // namespace

libtextclassifier3::StatusOr<std::unique_ptr<MainIndex>> MainIndex::Create(
    const std::string& index_directory, const Filesystem* filesystem,
    const IcingFilesystem* icing_filesystem) {
  ICING_RETURN_ERROR_IF_NULL(filesystem);
  ICING_RETURN_ERROR_IF_NULL(icing_filesystem);
  auto main_index = std::make_unique<MainIndex>();
  ICING_RETURN_IF_ERROR(
      main_index->Init(index_directory, filesystem, icing_filesystem));
  return main_index;
}

// TODO(b/139087650) : Migrate off of IcingFilesystem.
libtextclassifier3::Status MainIndex::Init(
    const std::string& index_directory, const Filesystem* filesystem,
    const IcingFilesystem* icing_filesystem) {
  if (!filesystem->CreateDirectoryRecursively(index_directory.c_str())) {
    return absl_ports::InternalError("Unable to create main index directory.");
  }
  std::string flash_index_file = index_directory + "/main_index";
  ICING_ASSIGN_OR_RETURN(
      FlashIndexStorage flash_index,
      FlashIndexStorage::Create(flash_index_file, filesystem));
  flash_index_storage_ =
      std::make_unique<FlashIndexStorage>(std::move(flash_index));

  std::string lexicon_file = index_directory + "/main-lexicon";
  IcingDynamicTrie::RuntimeOptions runtime_options;
  main_lexicon_ = std::make_unique<IcingDynamicTrie>(
      lexicon_file, runtime_options, icing_filesystem);
  IcingDynamicTrie::Options lexicon_options;
  if (!main_lexicon_->CreateIfNotExist(lexicon_options) ||
      !main_lexicon_->Init()) {
    return absl_ports::InternalError("Failed to initialize lexicon trie");
  }
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::StatusOr<int64_t> MainIndex::GetElementsSize() const {
  IndexStorageInfoProto storage_info = GetStorageInfo(IndexStorageInfoProto());
  if (storage_info.main_index_storage_size() == -1 ||
      storage_info.main_index_lexicon_size() == -1) {
    return absl_ports::AbortedError(
        "Failed to get size of MainIndex's members.");
  }
  return storage_info.main_index_storage_size() +
         storage_info.main_index_lexicon_size();
}

IndexStorageInfoProto MainIndex::GetStorageInfo(
    IndexStorageInfoProto storage_info) const {
  int64_t lexicon_elt_size = main_lexicon_->GetElementsSize();
  if (lexicon_elt_size != IcingFilesystem::kBadFileSize) {
    storage_info.set_main_index_lexicon_size(lexicon_elt_size);
  } else {
    storage_info.set_main_index_lexicon_size(-1);
  }
  int64_t index_elt_size = flash_index_storage_->GetElementsSize();
  if (lexicon_elt_size != IcingFilesystem::kBadFileSize) {
    storage_info.set_main_index_storage_size(index_elt_size);
  } else {
    storage_info.set_main_index_storage_size(-1);
  }
  storage_info.set_main_index_block_size(flash_index_storage_->block_size());
  storage_info.set_num_blocks(flash_index_storage_->num_blocks());
  storage_info.set_min_free_fraction(flash_index_storage_->min_free_fraction());
  return storage_info;
}

libtextclassifier3::StatusOr<std::unique_ptr<PostingListAccessor>>
MainIndex::GetAccessorForExactTerm(const std::string& term) {
  PostingListIdentifier posting_list_id = PostingListIdentifier::kInvalid;
  if (!main_lexicon_->Find(term.c_str(), &posting_list_id)) {
    return absl_ports::NotFoundError(IcingStringUtil::StringPrintf(
        "Term %s is not present in main lexicon.", term.c_str()));
  }
  ICING_ASSIGN_OR_RETURN(PostingListAccessor accessor,
                         PostingListAccessor::CreateFromExisting(
                             flash_index_storage_.get(), posting_list_id));
  return std::make_unique<PostingListAccessor>(std::move(accessor));
}

libtextclassifier3::StatusOr<MainIndex::GetPrefixAccessorResult>
MainIndex::GetAccessorForPrefixTerm(const std::string& prefix) {
  bool exact = false;
  // For prefix indexing: when we are doing a prefix match for
  // "prefix", find the tvi to the equivalent posting list. prefix's
  // own posting list might not exist but its shortest child acts as a proxy.
  //
  // For example, if there are only two hits in the index are prefix hits for
  // "bar" and "bat", then both will appear on a posting list for "ba". "b"
  // won't have a posting list, but "ba" will suffice.
  IcingDynamicTrie::PropertyReader hits_in_prefix_section(
      *main_lexicon_, GetHasHitsInPrefixSectionPropertyId());
  IcingDynamicTrie::Iterator main_itr(*main_lexicon_, prefix.c_str());
  if (!main_itr.IsValid()) {
    return absl_ports::NotFoundError(IcingStringUtil::StringPrintf(
        "Term: %s is not present in the main lexicon.", prefix.c_str()));
  }
  exact = (prefix.length() == strlen(main_itr.GetKey()));

  if (!exact && !hits_in_prefix_section.HasProperty(main_itr.GetValueIndex())) {
    // Found it, but it doesn't have prefix hits. Exit early. No need to
    // retrieve the posting list because there's nothing there for us.
    return libtextclassifier3::Status::OK;
  }
  PostingListIdentifier posting_list_id = PostingListIdentifier::kInvalid;
  memcpy(&posting_list_id, main_itr.GetValue(), sizeof(posting_list_id));
  ICING_ASSIGN_OR_RETURN(PostingListAccessor pl_accessor,
                         PostingListAccessor::CreateFromExisting(
                             flash_index_storage_.get(), posting_list_id));
  GetPrefixAccessorResult result = {
      std::make_unique<PostingListAccessor>(std::move(pl_accessor)), exact};
  return result;
}

// TODO(tjbarron): Implement a method PropertyReadersAll.HasAnyProperty().
bool IsTermInNamespaces(
    const IcingDynamicTrie::PropertyReadersAll& property_reader,
    uint32_t value_index, const std::vector<NamespaceId>& namespace_ids) {
  if (namespace_ids.empty()) {
    return true;
  }
  for (NamespaceId namespace_id : namespace_ids) {
    if (property_reader.HasProperty(GetNamespacePropertyId(namespace_id),
                                    value_index)) {
      return true;
    }
  }

  return false;
}

libtextclassifier3::StatusOr<std::vector<TermMetadata>>
MainIndex::FindTermsByPrefix(const std::string& prefix,
                             const std::vector<NamespaceId>& namespace_ids,
                             int num_to_return) {
  // Finds all the terms that start with the given prefix in the lexicon.
  IcingDynamicTrie::Iterator term_iterator(*main_lexicon_, prefix.c_str());

  // A property reader to help check if a term has some property.
  IcingDynamicTrie::PropertyReadersAll property_reader(*main_lexicon_);

  std::vector<TermMetadata> term_metadata_list;
  while (term_iterator.IsValid() && term_metadata_list.size() < num_to_return) {
    uint32_t term_value_index = term_iterator.GetValueIndex();

    // Skips the terms that don't exist in the given namespaces. We won't skip
    // any terms if namespace_ids is empty.
    if (!IsTermInNamespaces(property_reader, term_value_index, namespace_ids)) {
      term_iterator.Advance();
      continue;
    }
    PostingListIdentifier posting_list_id = PostingListIdentifier::kInvalid;
    memcpy(&posting_list_id, term_iterator.GetValue(), sizeof(posting_list_id));
    // Getting the actual hit count would require reading the entire posting
    // list chain. We take an approximation to avoid all of those IO ops.
    // Because we are not reading the posting lists, it is impossible to
    // differentiate between single max-size posting lists and chains of
    // max-size posting lists. We assume that the impact on scoring is not
    // significant.
    int approx_hit_count = IndexBlock::ApproximateFullPostingListHitsForBlock(
        flash_index_storage_->block_size(),
        posting_list_id.posting_list_index_bits());
    term_metadata_list.emplace_back(term_iterator.GetKey(), approx_hit_count);

    term_iterator.Advance();
  }
  if (term_iterator.IsValid()) {
    // We exited the loop above because we hit the num_to_return limit.
    ICING_LOG(WARNING) << "Ran into limit of " << num_to_return
                       << " retrieving suggestions for " << prefix
                       << ". Some suggestions may not be returned and others "
                          "may be misranked.";
  }
  return term_metadata_list;
}

libtextclassifier3::StatusOr<MainIndex::LexiconMergeOutputs>
MainIndex::AddBackfillBranchPoints(const IcingDynamicTrie& other_lexicon) {
  // Maps new branching points in main lexicon to the term such that
  // branching_point_term is a prefix of term and there are no terms smaller
  // than term and greater than branching_point_term.
  std::string prefix;
  LexiconMergeOutputs outputs;
  for (IcingDynamicTrie::Iterator other_term_itr(other_lexicon, /*prefix=*/"");
       other_term_itr.IsValid(); other_term_itr.Advance()) {
    // If term were inserted in the main lexicon, what new branching would it
    // create? (It always creates at most one.)
    int prefix_len = main_lexicon_->FindNewBranchingPrefixLength(
        other_term_itr.GetKey(), /*utf8=*/true);
    if (prefix_len <= 0) {
      continue;
    }
    prefix.assign(other_term_itr.GetKey(), prefix_len);

    // Figure out backfill tvi. Might not exist since all children terms could
    // only contain hits from non-prefix sections.
    //
    // Ex. Suppose that the main lexicon contains "foot" and "fool" and that
    // we're adding "foul". The new branching prefix will be "fo". The backfill
    // prefix will be "foo" - all hits in prefix section on "foo" will need to
    // be added to the new "fo" posting list later.
    FindTermResult result =
        FindShortestValidTermWithPrefixHits(main_lexicon_.get(), prefix);
    if (!result.found || result.exact) {
      continue;
    }

    // This is a new prefix that will need backfilling from its next-in-line
    // posting list. This new prefix will have to have a posting list eventually
    // so insert a default PostingListIdentifier as a placeholder.
    uint32_t branching_prefix_tvi;
    bool new_key;
    PostingListIdentifier posting_list_id = PostingListIdentifier::kInvalid;
    if (!main_lexicon_->Insert(prefix.c_str(), &posting_list_id,
                               &branching_prefix_tvi, /*replace=*/false,
                               &new_key)) {
      return absl_ports::InternalError("Could not insert branching prefix");
    }

    // Backfills only contain prefix hits by default. So set these here but
    // could be overridden when adding hits from the other index later.
    if (!main_lexicon_->SetProperty(branching_prefix_tvi,
                                    GetHasNoExactHitsPropertyId()) ||
        !main_lexicon_->SetProperty(branching_prefix_tvi,
                                    GetHasHitsInPrefixSectionPropertyId())) {
      return absl_ports::InternalError("Setting prefix prop failed");
    }

    outputs.backfill_map[branching_prefix_tvi] = result.tvi;
  }
  return outputs;
}

libtextclassifier3::StatusOr<MainIndex::LexiconMergeOutputs>
MainIndex::AddTerms(const IcingDynamicTrie& other_lexicon,
                    LexiconMergeOutputs&& outputs) {
  IcingDynamicTrie::PropertyReadersAll new_term_prop_readers(other_lexicon);
  for (IcingDynamicTrie::Iterator other_term_itr(other_lexicon, /*prefix=*/"");
       other_term_itr.IsValid(); other_term_itr.Advance()) {
    uint32_t new_main_tvi;
    PostingListIdentifier posting_list_id = PostingListIdentifier::kInvalid;
    if (!main_lexicon_->Insert(other_term_itr.GetKey(), &posting_list_id,
                               &new_main_tvi,
                               /*replace=*/false)) {
      return absl_ports::InternalError(absl_ports::StrCat(
          "Could not insert term: ", other_term_itr.GetKey()));
    }

    // Copy the properties from the other lexicon over to the main lexicon.
    uint32_t other_tvi = other_term_itr.GetValueIndex();
    if (!CopyProperties(new_term_prop_readers, other_lexicon, other_tvi,
                        new_main_tvi)) {
      return absl_ports::InternalError(absl_ports::StrCat(
          "Could not insert term: ", other_term_itr.GetKey()));
    }

    // Add other to main mapping.
    outputs.other_tvi_to_main_tvi.emplace(other_tvi, new_main_tvi);

    memcpy(&posting_list_id, main_lexicon_->GetValueAtIndex(new_main_tvi),
           sizeof(posting_list_id));
    if (posting_list_id.block_index() != kInvalidBlockIndex) {
      outputs.main_tvi_to_block_index[new_main_tvi] =
          posting_list_id.block_index();
    }
  }
  return std::move(outputs);
}

libtextclassifier3::StatusOr<MainIndex::LexiconMergeOutputs>
MainIndex::AddBranchPoints(const IcingDynamicTrie& other_lexicon,
                           LexiconMergeOutputs&& outputs) {
  IcingDynamicTrie::PropertyReader has_prefix_prop_reader(
      other_lexicon, GetHasHitsInPrefixSectionPropertyId());
  if (!has_prefix_prop_reader.Exists()) {
    return std::move(outputs);
  }
  std::string prefix;
  for (IcingDynamicTrie::Iterator other_term_itr(other_lexicon, /*prefix=*/"");
       other_term_itr.IsValid(); other_term_itr.Advance()) {
    // Only expand terms that have hits in prefix sections.
    if (!has_prefix_prop_reader.HasProperty(other_term_itr.GetValueIndex())) {
      continue;
    }

    // Get prefixes where there is already a branching point in the main
    // lexicon. We skip prefixes which don't already have a branching point.
    std::vector<int> prefix_lengths = main_lexicon_->FindBranchingPrefixLengths(
        other_term_itr.GetKey(), /*utf8=*/true);

    int buf_start = outputs.prefix_tvis_buf.size();
    // Add prefixes.
    for (int prefix_length : prefix_lengths) {
      if (prefix_length <= 0) {
        continue;
      }

      prefix.assign(other_term_itr.GetKey(), prefix_length);
      uint32_t prefix_tvi;
      bool new_key;
      PostingListIdentifier posting_list_id = PostingListIdentifier::kInvalid;
      if (!main_lexicon_->Insert(prefix.c_str(), &posting_list_id, &prefix_tvi,
                                 /*replace=*/false, &new_key)) {
        return absl_ports::InternalError(
            absl_ports::StrCat("Could not insert prefix: ", prefix));
      }

      // Prefix tvi will have hits in prefix section.
      if (!main_lexicon_->SetProperty(prefix_tvi,
                                      GetHasHitsInPrefixSectionPropertyId())) {
        return absl_ports::InternalError(
            "Setting has hits in prefix section prop failed");
      }

      // If it hasn't been added by non-prefix term insertions in
      // AddBackfillBranchPoints and AddTerms, it is a prefix-only term.
      if (new_key && !main_lexicon_->SetProperty(
                         prefix_tvi, GetHasNoExactHitsPropertyId())) {
        return absl_ports::InternalError("Setting no exact hits prop failed");
      }

      outputs.prefix_tvis_buf.push_back(prefix_tvi);

      memcpy(&posting_list_id, main_lexicon_->GetValueAtIndex(prefix_tvi),
             sizeof(posting_list_id));
      if (posting_list_id.block_index() != kInvalidBlockIndex) {
        outputs.main_tvi_to_block_index[prefix_tvi] =
            posting_list_id.block_index();
      }
    }

    // Any prefixes added? Then add to map.
    if (buf_start < outputs.prefix_tvis_buf.size()) {
      outputs.other_tvi_to_prefix_main_tvis[other_term_itr.GetValueIndex()] = {
          buf_start, outputs.prefix_tvis_buf.size() - buf_start};
    }
  }
  return std::move(outputs);
}

bool MainIndex::CopyProperties(
    const IcingDynamicTrie::PropertyReadersAll& prop_reader,
    const IcingDynamicTrie& other_lexicon, uint32_t other_tvi,
    uint32_t new_main_tvi) {
  for (uint32_t property_id = 0; property_id < prop_reader.size();
       ++property_id) {
    if (property_id == GetHasNoExactHitsPropertyId()) {
      // HasNoExactHitsProperty is an inverse. If other_lexicon has exact hits
      // for this term, then HasNoExactHits needs to be set to false in
      // main_lexicon. If other_lexicon has no exact hits for this term, then
      // HasNoExactHits in the main_lexicon should not be modified.
      if (!prop_reader.HasProperty(property_id, other_tvi) &&
          !main_lexicon_->ClearProperty(new_main_tvi, property_id)) {
        ICING_LOG(ERROR) << "Clearing HasNoExactHitsProperty failed";
        return false;
      }
    } else {
      // If other_lexicon has this property set for this term, then that
      // property needs to be set for the main_lexicon. If other_lexicon
      // doesn't have this property set, then the property in the main lexicon
      // should not be modified.
      if (prop_reader.HasProperty(property_id, other_tvi) &&
          !main_lexicon_->SetProperty(new_main_tvi, property_id)) {
        return false;
      }
    }
  }
  return true;
}

libtextclassifier3::Status MainIndex::AddHits(
    const TermIdCodec& term_id_codec,
    std::unordered_map<uint32_t, uint32_t>&& backfill_map,
    std::vector<TermIdHitPair>&& hits, DocumentId last_added_document_id) {
  if (hits.empty()) {
    flash_index_storage_->set_last_indexed_docid(last_added_document_id);
    return libtextclassifier3::Status::OK;
  }
  uint32_t cur_term_id = hits[0].term_id();
  ICING_ASSIGN_OR_RETURN(TermIdCodec::DecodedTermInfo cur_decoded_term,
                         term_id_codec.DecodeTermInfo(cur_term_id));
  // Iterate through all hits. If these hits are for a term that also needs
  // backfill, then backfill first and then add the new hits.
  size_t k_start = 0;
  size_t k_end = 0;
  while (k_start < hits.size()) {
    uint32_t term_id = hits[k_end].term_id();
    while (term_id == cur_term_id && ++k_end < hits.size()) {
      term_id = hits[k_end].term_id();
    }

    // Look for backfill.
    PostingListIdentifier backfill_posting_list_id =
        PostingListIdentifier::kInvalid;
    auto itr = backfill_map.find(cur_decoded_term.tvi);
    if (itr != backfill_map.end()) {
      const void* value = main_lexicon_->GetValueAtIndex(itr->second);
      memcpy(&backfill_posting_list_id, value,
             sizeof(backfill_posting_list_id));
      backfill_map.erase(itr);
    }
    ICING_RETURN_IF_ERROR(AddHitsForTerm(cur_decoded_term.tvi,
                                         backfill_posting_list_id,
                                         &hits[k_start], k_end - k_start));
    cur_term_id = term_id;
    ICING_ASSIGN_OR_RETURN(cur_decoded_term,
                           term_id_codec.DecodeTermInfo(cur_term_id));
    k_start = k_end;
  }

  // Now copy remaining backfills.
  ICING_VLOG(1) << IcingStringUtil::StringPrintf("Remaining backfills %zu",
                                                 backfill_map.size());
  for (auto other_tvi_main_tvi_pair : backfill_map) {
    PostingListIdentifier backfill_posting_list_id =
        PostingListIdentifier::kInvalid;
    memcpy(&backfill_posting_list_id,
           main_lexicon_->GetValueAtIndex(other_tvi_main_tvi_pair.second),
           sizeof(backfill_posting_list_id));
    ICING_ASSIGN_OR_RETURN(
        PostingListAccessor hit_accum,
        PostingListAccessor::Create(flash_index_storage_.get()));
    ICING_RETURN_IF_ERROR(
        AddPrefixBackfillHits(backfill_posting_list_id, &hit_accum));
    PostingListAccessor::FinalizeResult result =
        PostingListAccessor::Finalize(std::move(hit_accum));
    if (result.id.is_valid()) {
      main_lexicon_->SetValueAtIndex(other_tvi_main_tvi_pair.first, &result.id);
    }
  }
  flash_index_storage_->set_last_indexed_docid(last_added_document_id);
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status MainIndex::AddHitsForTerm(
    uint32_t tvi, PostingListIdentifier backfill_posting_list_id,
    const TermIdHitPair* hit_elements, size_t len) {
  // 1. Create a PostingListAccessor - either from the pre-existing block, if
  // one exists, or from scratch.
  PostingListIdentifier posting_list_id = PostingListIdentifier::kInvalid;
  memcpy(&posting_list_id, main_lexicon_->GetValueAtIndex(tvi),
         sizeof(posting_list_id));
  std::unique_ptr<PostingListAccessor> pl_accessor;
  if (posting_list_id.is_valid()) {
    if (posting_list_id.block_index() >= flash_index_storage_->num_blocks()) {
      ICING_LOG(ERROR) << IcingStringUtil::StringPrintf(
          "Index dropped hits. Invalid block index %u >= %u",
          posting_list_id.block_index(), flash_index_storage_->num_blocks());
      // TODO(b/159918304) : Consider revising the checksumming strategy in the
      // main index. Providing some mechanism to check for corruption - either
      // during initialization or some later time would allow us to avoid
      // whack-a-mole with odd corruption issues like this one (b/62820689).
      return absl_ports::InternalError(
          "Valid posting list has an invalid block index!");
    }
    ICING_ASSIGN_OR_RETURN(PostingListAccessor tmp,
                           PostingListAccessor::CreateFromExisting(
                               flash_index_storage_.get(), posting_list_id));
    pl_accessor = std::make_unique<PostingListAccessor>(std::move(tmp));
  } else {
    // New posting list.
    ICING_ASSIGN_OR_RETURN(
        PostingListAccessor tmp,
        PostingListAccessor::Create(flash_index_storage_.get()));
    pl_accessor = std::make_unique<PostingListAccessor>(std::move(tmp));
  }

  // 2. Backfill any hits if necessary.
  if (backfill_posting_list_id.is_valid()) {
    ICING_RETURN_IF_ERROR(
        AddPrefixBackfillHits(backfill_posting_list_id, pl_accessor.get()));
  }

  // 3. Add all the new hits.
  for (int i = len - 1; i >= 0; --i) {
    Hit hit = hit_elements[i].hit();
    ICING_RETURN_IF_ERROR(pl_accessor->PrependHit(hit));
  }

  // 4. Finalize this posting list and put its identifier in the lexicon.
  PostingListAccessor::FinalizeResult result =
      PostingListAccessor::Finalize(std::move(*pl_accessor));
  if (result.id.is_valid()) {
    main_lexicon_->SetValueAtIndex(tvi, &result.id);
  }
  return libtextclassifier3::Status::OK;
}

libtextclassifier3::Status MainIndex::AddPrefixBackfillHits(
    PostingListIdentifier backfill_posting_list_id,
    PostingListAccessor* hit_accum) {
  ICING_ASSIGN_OR_RETURN(
      PostingListAccessor backfill_accessor,
      PostingListAccessor::CreateFromExisting(flash_index_storage_.get(),
                                              backfill_posting_list_id));
  std::vector<Hit> backfill_hits;
  ICING_ASSIGN_OR_RETURN(std::vector<Hit> tmp,
                         backfill_accessor.GetNextHitsBatch());
  while (!tmp.empty()) {
    std::copy(tmp.begin(), tmp.end(), std::back_inserter(backfill_hits));
    ICING_ASSIGN_OR_RETURN(tmp, backfill_accessor.GetNextHitsBatch());
  }

  Hit last_added_hit;
  // The hits in backfill_hits are in the reverse order of how they were added.
  // Iterate in reverse to add them to this new posting list in the correct
  // order.
  for (auto itr = backfill_hits.rbegin(); itr != backfill_hits.rend(); ++itr) {
    const Hit& hit = *itr;
    // Skip hits from non-prefix-enabled sections.
    if (!hit.is_in_prefix_section()) {
      continue;
    }

    // A backfill hit is a prefix hit in a prefix section.
    const Hit backfill_hit(hit.section_id(), hit.document_id(),
                           hit.term_frequency(),
                           /*is_in_prefix_section=*/true,
                           /*is_prefix_hit=*/true);
    if (backfill_hit == last_added_hit) {
      // Skip duplicate values due to overriding of the is_prefix flag.
      continue;
    }
    last_added_hit = backfill_hit;
    ICING_RETURN_IF_ERROR(hit_accum->PrependHit(backfill_hit));
  }
  return libtextclassifier3::Status::OK;
}

void MainIndex::GetDebugInfo(int verbosity, std::string* out) const {
  // Lexicon.
  out->append("Main Lexicon stats:\n");
  main_lexicon_->GetDebugInfo(verbosity, out);

  if (verbosity <= 0) {
    return;
  }

  flash_index_storage_->GetDebugInfo(verbosity, out);
}

}  // namespace lib
}  // namespace icing

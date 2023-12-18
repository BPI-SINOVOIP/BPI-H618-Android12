/*
 * Copyright (C) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "comments.h"

#include <android-base/result.h>
#include <android-base/strings.h>

#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "logging.h"

using android::base::EndsWith;
using android::base::Error;
using android::base::Join;
using android::base::Result;
using android::base::Split;
using android::base::StartsWith;
using android::base::Trim;

namespace android {
namespace aidl {

namespace {

static const std::string_view kLineCommentBegin = "//";
static const std::string_view kBlockCommentBegin = "/*";
static const std::string_view kBlockCommentEnd = "*/";
static const std::string_view kDocCommentBegin = "/**";
static const std::string kTagDeprecated = "@deprecated";
static const std::regex kTagHideRegex{"@hide\\b"};

std::string ConsumePrefix(const std::string& s, std::string_view prefix) {
  AIDL_FATAL_IF(!StartsWith(s, prefix), AIDL_LOCATION_HERE)
      << "'" << s << "' has no prefix '" << prefix << "'";
  return s.substr(prefix.size());
}

std::string ConsumeSuffix(const std::string& s, std::string_view suffix) {
  AIDL_FATAL_IF(!EndsWith(s, suffix), AIDL_LOCATION_HERE);
  return s.substr(0, s.size() - suffix.size());
}

struct BlockTag {
  std::string name;
  std::string description;
};

// Removes comment markers: //, /*, */, optional leading "*" in block comments
// - keeps leading spaces, but trims trailing spaces
// - keeps empty lines
std::vector<std::string> TrimmedLines(const Comment& c) {
  if (c.type == Comment::Type::LINE) {
    return std::vector{ConsumePrefix(c.body, kLineCommentBegin)};
  }

  std::string stripped = ConsumeSuffix(ConsumePrefix(c.body, kBlockCommentBegin), kBlockCommentEnd);

  std::vector<std::string> lines;
  bool found_first_line = false;

  for (auto& line : Split(stripped, "\n")) {
    // Delete prefixes like "    * ", "   *", or "    ".
    size_t idx = 0;
    for (; idx < line.size() && isspace(line[idx]); idx++)
      ;
    if (idx < line.size() && line[idx] == '*') idx++;
    if (idx < line.size() && line[idx] == ' ') idx++;

    const std::string& sanitized_line = line.substr(idx);
    size_t i = sanitized_line.size();
    for (; i > 0 && isspace(sanitized_line[i - 1]); i--)
      ;

    // Either the size is 0 or everything was whitespace.
    bool is_empty_line = i == 0;

    found_first_line = found_first_line || !is_empty_line;
    if (!found_first_line) continue;

    // if is_empty_line, i == 0 so substr == ""
    lines.push_back(sanitized_line.substr(0, i));
  }
  // remove trailing empty lines
  while (!lines.empty() && Trim(lines.back()).empty()) {
    lines.pop_back();
  }
  return lines;
}

// Parses a block comment and returns block tags in the comment.
std::vector<BlockTag> BlockTags(const Comment& c) {
  AIDL_FATAL_IF(c.type != Comment::Type::BLOCK, AIDL_LOCATION_HERE);

  std::vector<BlockTag> tags;

  // current tag and paragraph
  std::string tag;
  std::vector<std::string> paragraph;

  auto end_paragraph = [&]() {
    if (tag.empty()) {
      paragraph.clear();
      return;
    }
    // paragraph lines are trimed at both ends
    tags.push_back({tag, Join(paragraph, " ")});
    tag.clear();
    paragraph.clear();
  };

  for (const auto& line : TrimmedLines(c)) {
    size_t idx = 0;
    // skip leading spaces
    for (; idx < line.size() && isspace(line[idx]); idx++)
      ;

    if (idx == line.size()) {
      // skip empty lines
    } else if (line[idx] == '@') {
      // end the current paragraph before reading a new block tag (+ description paragraph)
      end_paragraph();

      size_t end_idx = idx + 1;
      for (; end_idx < line.size() && isalpha(line[end_idx]); end_idx++)
        ;

      tag = line.substr(idx, end_idx - idx);

      if (end_idx < line.size() && line[end_idx] == ' ') end_idx++;
      // skip empty line
      if (end_idx < line.size()) {
        paragraph.push_back(line.substr(end_idx));
      }
    } else {
      // gather paragraph lines with leading spaces trimmed
      paragraph.push_back(line.substr(idx));
    }
  }

  end_paragraph();

  return tags;
}

}  // namespace

Comment::Comment(const std::string& body) : body(body) {
  if (StartsWith(body, kLineCommentBegin)) {
    type = Type::LINE;
  } else if (StartsWith(body, kBlockCommentBegin) && EndsWith(body, kBlockCommentEnd)) {
    type = Type::BLOCK;
  } else {
    AIDL_FATAL(AIDL_LOCATION_HERE) << "invalid comments body:" << body;
  }
}

// Returns the immediate block comment from the list of comments.
// Only the last/block comment can have the tag.
//
//   /* @hide */
//   int x;
//
// But tags in line or distant comments don't count. In the following,
// the variable 'x' is not hidden.
//
//    // @hide
//    int x;
//
//    /* @hide */
//    /* this is the immemediate comment to 'x' */
//    int x;
//
static std::optional<Comment> GetValidComment(const Comments& comments) {
  if (!comments.empty() && comments.back().type == Comment::Type::BLOCK) {
    return comments.back();
  }
  return std::nullopt;
}

// Sees if comments have the @hide tag.
// Example: /** @hide */
bool HasHideInComments(const Comments& comments) {
  const auto valid_comment = GetValidComment(comments);
  return valid_comment && std::regex_search(valid_comment->body, kTagHideRegex);
}

// Finds the @deprecated tag in comments and returns it with optional note which
// follows the tag.
// Example: /** @deprecated reason */
std::optional<Deprecated> FindDeprecated(const Comments& comments) {
  if (const auto valid_comment = GetValidComment(comments); valid_comment) {
    for (const auto& [name, description] : BlockTags(comments.back())) {
      // take the first @deprecated
      if (kTagDeprecated == name) {
        return Deprecated{description};
      }
    }
  }
  return std::nullopt;
}

// Formats comments for the Java backend.
// The last/block comment is transformed into javadoc(/** */)
// and others are used as they are.
std::string FormatCommentsForJava(const Comments& comments) {
  std::stringstream out;
  for (auto it = begin(comments); it != end(comments); it++) {
    const bool last = next(it) == end(comments);
    // We only re-format the last/block comment which is not already a doc-style comment.
    if (last && it->type == Comment::Type::BLOCK && !StartsWith(it->body, kDocCommentBegin)) {
      out << kDocCommentBegin << ConsumePrefix(it->body, kBlockCommentBegin);
    } else {
      out << it->body;
    }
  }
  return out.str();
}

}  // namespace aidl
}  // namespace android
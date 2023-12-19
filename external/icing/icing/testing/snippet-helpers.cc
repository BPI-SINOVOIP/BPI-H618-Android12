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

#include "icing/testing/snippet-helpers.h"

#include <algorithm>
#include <string_view>

#include "icing/absl_ports/str_join.h"
#include "icing/proto/search.pb.h"
#include "icing/schema/section-manager.h"

namespace icing {
namespace lib {

namespace {

// Returns the property index and the property name with the index removed.
// Examples:
//   GetPropertyIndex("foo") will return ["foo", 0]
//   GetPropertyIndex("foo[5]") will return ["foo", 5]
std::pair<std::string_view, int> GetPropertyIndex(std::string_view property) {
  size_t l_bracket = property.find(kLBracket);
  if (l_bracket == std::string_view::npos || l_bracket >= property.length()) {
    return {property, 0};
  }
  size_t r_bracket = property.find(kRBracket, l_bracket);
  if (r_bracket == std::string_view::npos || r_bracket - l_bracket < 2) {
    return {property, 0};
  }
  std::string index_string =
      std::string(property.substr(l_bracket + 1, r_bracket - l_bracket - 1));
  return {property.substr(0, l_bracket), std::stoi(index_string)};
}

}  // namespace

const PropertyProto* GetProperty(const DocumentProto& document,
                                 std::string_view property_name) {
  const PropertyProto* property = nullptr;
  for (const PropertyProto& prop : document.properties()) {
    if (prop.name() == property_name) {
      property = &prop;
    }
  }
  return property;
}

std::vector<std::string_view> GetWindows(
    std::string_view content, const SnippetProto::EntryProto& snippet_proto) {
  std::vector<std::string_view> windows;
  for (const SnippetMatchProto& match : snippet_proto.snippet_matches()) {
    windows.push_back(content.substr(match.window_byte_position(),
                                     match.window_byte_length()));
  }
  return windows;
}

std::vector<std::string_view> GetMatches(
    std::string_view content, const SnippetProto::EntryProto& snippet_proto) {
  std::vector<std::string_view> matches;
  for (const SnippetMatchProto& match : snippet_proto.snippet_matches()) {
    matches.push_back(content.substr(match.exact_match_byte_position(),
                                     match.exact_match_byte_length()));
  }
  return matches;
}

std::string_view GetString(const DocumentProto* document,
                           std::string_view property_path) {
  std::vector<std::string_view> properties =
      absl_ports::StrSplit(property_path, kPropertySeparator);
  for (int i = 0; i < properties.size(); ++i) {
    std::string_view property = properties.at(i);
    int property_index;
    std::tie(property, property_index) = GetPropertyIndex(property);
    const PropertyProto* prop = GetProperty(*document, property);
    if (prop == nullptr) {
      // requested property doesn't exist in the document. Return empty string.
      return "";
    }
    if (i == properties.size() - 1) {
      // The last property. Get the string_value
      if (prop->string_values_size() - 1 < property_index) {
        // The requested string doesn't exist. Return empty string.
        return "";
      }
      return prop->string_values(property_index);
    } else if (prop->document_values_size() - 1 < property_index) {
      // The requested subproperty doesn't exist. return an empty string.
      return "";
    } else {
      // Go to the next subproperty.
      document = &prop->document_values(property_index);
    }
  }
  return "";
}

}  // namespace lib
}  // namespace icing

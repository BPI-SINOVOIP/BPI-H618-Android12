/*
 * Copyright (C) 2018, The Android Open Source Project
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

#include "aidl_typenames.h"
#include "aidl_language.h"
#include "logging.h"

#include <android-base/strings.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

using android::base::Split;

using std::make_pair;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

namespace android {
namespace aidl {

// The built-in AIDL types..
static const set<string> kBuiltinTypes = {"void",
                                          "boolean",
                                          "byte",
                                          "char",
                                          "int",
                                          "long",
                                          "float",
                                          "double",
                                          "String",
                                          "List",
                                          "Map",
                                          "IBinder",
                                          "FileDescriptor",
                                          "CharSequence",
                                          "ParcelFileDescriptor",
                                          "ParcelableHolder"};

static const set<string> kPrimitiveTypes = {"void", "boolean", "byte",  "char",
                                            "int",  "long",    "float", "double"};

// Note: these types may look wrong because they look like Java
// types, but they have long been supported from the time when Java
// was the only target language of this compiler. They are added here for
// backwards compatibility, but we internally treat them as List and Map,
// respectively.
static const map<string, string> kJavaLikeTypeToAidlType = {
    {"java.util.List", "List"},
    {"java.util.Map", "Map"},
    {"android.os.ParcelFileDescriptor", "ParcelFileDescriptor"},
};

// Package name and type name can't be one of these as they are keywords
// in Java and C++. Using these names will eventually cause compilation error,
// so checking this here is not a must have, but early detection of errors
// is always better.
static const set<string> kCppOrJavaReservedWord = {
    "break",  "case",   "catch", "char",     "class",  "continue", "default",
    "do",     "double", "else",  "enum",     "false",  "float",    "for",
    "goto",   "if",     "int",   "long",     "new",    "private",  "protected",
    "public", "return", "short", "static",   "switch", "this",     "throw",
    "true",   "try",    "void",  "volatile", "while"};

static bool HasValidNameComponents(const AidlDefinedType& defined) {
  bool success = true;
  vector<string> pieces = Split(defined.GetCanonicalName(), ".");
  for (const string& piece : pieces) {
    if (kCppOrJavaReservedWord.find(piece) != kCppOrJavaReservedWord.end()) {
      AIDL_ERROR(defined) << defined.GetCanonicalName() << " is an invalid name because '" << piece
                          << "' is a Java or C++ identifier.";
      success = false;
    }
    // not checking kJavaLikeTypeToAidl, since that wouldn't make sense here
    if (kBuiltinTypes.find(piece) != kBuiltinTypes.end()) {
      AIDL_ERROR(defined) << defined.GetCanonicalName() << " is an invalid name because '" << piece
                          << "' is a built-in AIDL type.";
      success = false;
    }
  }
  return success;
}

bool AidlTypenames::IsIgnorableImport(const string& import) const {
  static set<string> ignore_import = {
      "android.os.IInterface",   "android.os.IBinder", "android.os.Parcelable", "android.os.Parcel",
      "android.content.Context", "java.lang.String",   "java.lang.CharSequence"};
  // these known built-in types don't need to be imported
  const bool in_ignore_import = ignore_import.find(import) != ignore_import.end();
  // an already defined type doesn't need to be imported again unless it is from
  // the preprocessed file
  auto ret = TryGetDefinedTypeImpl(import);
  const bool defined_type_not_from_preprocessed = ret.type != nullptr && !ret.from_preprocessed;
  return in_ignore_import || defined_type_not_from_preprocessed;
}

bool AidlTypenames::AddDocument(std::unique_ptr<AidlDocument> doc) {
  for (const auto& type : doc->DefinedTypes()) {
    if (defined_types_.find(type->GetCanonicalName()) != defined_types_.end()) {
      return false;
    }
    if (!HasValidNameComponents(*type)) {
      return false;
    }
  }
  documents_.push_back(std::move(doc));
  for (const auto& type : documents_.back()->DefinedTypes()) {
    defined_types_.emplace(type->GetCanonicalName(), type.get());
  }
  return true;
}

const AidlDocument* AidlTypenames::GetDocumentFor(const AidlDefinedType* type) const {
  for (const auto& doc : AllDocuments()) {
    for (const auto& defined_type : doc->DefinedTypes()) {
      if (defined_type.get() == type) {
        return doc.get();
      }
    }
  }
  return nullptr;
}

const AidlDocument& AidlTypenames::MainDocument() const {
  AIDL_FATAL_IF(documents_.size() == 0, AIDL_LOCATION_HERE) << "Main document doesn't exist";
  return *(documents_[0]);
}

bool AidlTypenames::AddPreprocessedType(unique_ptr<AidlDefinedType> type) {
  const string name = type->GetCanonicalName();
  if (preprocessed_types_.find(name) != preprocessed_types_.end()) {
    return false;
  }
  if (!HasValidNameComponents(*type)) {
    return false;
  }
  preprocessed_types_.insert(make_pair(name, std::move(type)));
  return true;
}

bool AidlTypenames::IsBuiltinTypename(const string& type_name) {
  return kBuiltinTypes.find(type_name) != kBuiltinTypes.end() ||
      kJavaLikeTypeToAidlType.find(type_name) != kJavaLikeTypeToAidlType.end();
}

bool AidlTypenames::IsPrimitiveTypename(const string& type_name) {
  return kPrimitiveTypes.find(type_name) != kPrimitiveTypes.end();
}

bool AidlTypenames::IsParcelable(const string& type_name) const {
  if (IsBuiltinTypename(type_name)) {
    return type_name == "ParcelableHolder" || type_name == "ParcelFileDescriptor";
  }
  if (auto defined_type = TryGetDefinedType(type_name); defined_type) {
    return defined_type->AsParcelable() != nullptr;
  }
  return false;
}

const AidlDefinedType* AidlTypenames::TryGetDefinedType(const string& type_name) const {
  return TryGetDefinedTypeImpl(type_name).type;
}

AidlTypenames::DefinedImplResult AidlTypenames::TryGetDefinedTypeImpl(
    const string& type_name) const {
  // Do the exact match first.
  auto found_def = defined_types_.find(type_name);
  if (found_def != defined_types_.end()) {
    return DefinedImplResult(found_def->second, false);
  }

  auto found_prep = preprocessed_types_.find(type_name);
  if (found_prep != preprocessed_types_.end()) {
    return DefinedImplResult(found_prep->second.get(), true);
  }

  // Then match with the class name. Defined types has higher priority than
  // types from the preprocessed file.
  for (auto it = defined_types_.begin(); it != defined_types_.end(); it++) {
    if (it->second->GetName() == type_name) {
      return DefinedImplResult(it->second, false);
    }
  }

  for (auto it = preprocessed_types_.begin(); it != preprocessed_types_.end(); it++) {
    if (it->second->GetName() == type_name) {
      return DefinedImplResult(it->second.get(), true);
    }
  }

  return DefinedImplResult(nullptr, false);
}

std::vector<AidlDefinedType*> AidlTypenames::AllDefinedTypes() const {
  std::vector<AidlDefinedType*> res;
  for (const auto& d : AllDocuments()) {
    for (const auto& t : d->DefinedTypes()) {
      res.push_back(t.get());
    }
  }
  return res;
}

AidlTypenames::ResolvedTypename AidlTypenames::ResolveTypename(const string& type_name) const {
  if (IsBuiltinTypename(type_name)) {
    auto found = kJavaLikeTypeToAidlType.find(type_name);
    if (found != kJavaLikeTypeToAidlType.end()) {
      return {found->second, true, nullptr};
    }
    return {type_name, true, nullptr};
  }
  const AidlDefinedType* defined_type = TryGetDefinedType(type_name);
  if (defined_type != nullptr) {
    return {defined_type->GetCanonicalName(), true, defined_type};
  } else {
    return {type_name, false, nullptr};
  }
}

// Only immutable Parcelable, primitive type, and String, and List, Map, array of the types can be
// immutable.
bool AidlTypenames::CanBeJavaOnlyImmutable(const AidlTypeSpecifier& type) const {
  const string& name = type.GetName();
  if (type.IsGeneric()) {
    if (type.GetName() == "List" || type.GetName() == "Map") {
      const auto& types = type.GetTypeParameters();
      return std::all_of(types.begin(), types.end(),
                         [this](const auto& t) { return CanBeJavaOnlyImmutable(*t); });
    }
    AIDL_ERROR(type) << "For a generic type, an immutable parcelable can contain only List or Map.";
    return false;
  }
  if (IsPrimitiveTypename(name) || name == "String") {
    return true;
  }
  const AidlDefinedType* t = TryGetDefinedType(type.GetName());
  if (t == nullptr) {
    AIDL_ERROR(type) << "An immutable parcelable can contain only immutable Parcelable, primitive "
                        "type, and String.";
    return false;
  }
  if (t->AsEnumDeclaration()) {
    return true;
  }
  return t->IsJavaOnlyImmutable();
}

// Only FixedSize Parcelable, primitive types, and enum types can be FixedSize.
bool AidlTypenames::CanBeFixedSize(const AidlTypeSpecifier& type) const {
  const string& name = type.GetName();
  if (type.IsGeneric() || type.IsArray()) {
    return false;
  }
  if (IsPrimitiveTypename(name)) {
    return true;
  }
  if (IsBuiltinTypename(name)) {
    return false;
  }
  const AidlDefinedType* t = TryGetDefinedType(type.GetName());
  AIDL_FATAL_IF(t == nullptr, type)
      << "Failed to look up type. Cannot determine if it can be fixed size: " << type.GetName();

  if (t->AsEnumDeclaration()) {
    return true;
  }
  return t->IsFixedSize();
}

bool AidlTypenames::IsList(const AidlTypeSpecifier& type) {
  return type.GetName() == "List";
}

ArgumentAspect AidlTypenames::GetArgumentAspect(const AidlTypeSpecifier& type) const {
  if (type.IsArray()) {
    return {"array",
            {AidlArgument::Direction::IN_DIR, AidlArgument::Direction::OUT_DIR,
             AidlArgument::Direction::INOUT_DIR}};
  }
  const string& name = type.GetName();
  if (IsBuiltinTypename(name)) {
    if (name == "List" || name == "Map") {
      return {name,
              {AidlArgument::Direction::IN_DIR, AidlArgument::Direction::OUT_DIR,
               AidlArgument::Direction::INOUT_DIR}};
    } else if (name == "ParcelFileDescriptor") {
      // "out ParcelFileDescriptor" is not allowed because ParcelFileDescriptor is not
      // default-constructible.
      return {name, {AidlArgument::Direction::IN_DIR, AidlArgument::Direction::INOUT_DIR}};
    } else if (name == "ParcelableHolder") {
      // TODO(b/156872582): Support it when ParcelableHolder supports every backend.
      return {name, {}};
    } else {
      return {name, {AidlArgument::Direction::IN_DIR}};
    }
  }

  const AidlDefinedType* t = TryGetDefinedType(name);
  AIDL_FATAL_IF(t == nullptr, type) << "Unrecognized type: '" << name << "'";

  // An 'out' field is passed as an argument, so it doesn't make sense if it is immutable.
  if (t->AsParcelable() != nullptr) {
    if (t->IsJavaOnlyImmutable()) {
      return {"@JavaOnlyImmutable", {AidlArgument::Direction::IN_DIR}};
    }
    return {"parcelable/union",
            {AidlArgument::Direction::IN_DIR, AidlArgument::Direction::OUT_DIR,
             AidlArgument::Direction::INOUT_DIR}};
  }

  return {t->GetPreprocessDeclarationName(), {AidlArgument::Direction::IN_DIR}};
}

const AidlEnumDeclaration* AidlTypenames::GetEnumDeclaration(const AidlTypeSpecifier& type) const {
  if (auto defined_type = TryGetDefinedType(type.GetName()); defined_type != nullptr) {
    if (auto enum_decl = defined_type->AsEnumDeclaration(); enum_decl != nullptr) {
      return enum_decl;
    }
  }
  return nullptr;
}

const AidlInterface* AidlTypenames::GetInterface(const AidlTypeSpecifier& type) const {
  if (auto defined_type = TryGetDefinedType(type.GetName()); defined_type != nullptr) {
    if (auto intf = defined_type->AsInterface(); intf != nullptr) {
      return intf;
    }
  }
  return nullptr;
}

const AidlParcelable* AidlTypenames::GetParcelable(const AidlTypeSpecifier& type) const {
  if (auto defined_type = TryGetDefinedType(type.GetName()); defined_type != nullptr) {
    if (auto parcelable = defined_type->AsParcelable(); parcelable != nullptr) {
      return parcelable;
    }
  }
  return nullptr;
}

void AidlTypenames::IterateTypes(const std::function<void(const AidlDefinedType&)>& body) const {
  for (const auto& kv : defined_types_) {
    body(*kv.second);
  }
  for (const auto& kv : preprocessed_types_) {
    body(*kv.second);
  }
}

bool AidlTypenames::Autofill() const {
  bool success = true;
  IterateTypes([&](const AidlDefinedType& type) {
    // BackingType is filled in for all known enums, including imported enums,
    // because other types that may use enums, such as Interface or
    // StructuredParcelable, need to know the enum BackingType when
    // generating code.
    if (auto enum_decl = const_cast<AidlDefinedType&>(type).AsEnumDeclaration(); enum_decl) {
      if (!enum_decl->Autofill(*this)) {
        success = false;
      }
    }
  });
  return success;
}

}  // namespace aidl
}  // namespace android

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

#include "aidl_to_cpp.h"
#include "aidl_to_cpp_common.h"
#include "aidl_language.h"
#include "logging.h"

#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include <functional>
#include <unordered_map>

using android::base::Join;
using android::base::Split;
using android::base::StringPrintf;
using std::ostringstream;

namespace android {
namespace aidl {
namespace cpp {

namespace {
std::string RawParcelMethod(const AidlTypeSpecifier& type, const AidlTypenames& typenames,
                            bool readMethod) {
  static map<string, string> kBuiltin = {
      {"byte", "Byte"},
      {"boolean", "Bool"},
      {"char", "Char"},
      {"double", "Double"},
      {"FileDescriptor", "UniqueFileDescriptor"},
      {"float", "Float"},
      {"IBinder", "StrongBinder"},
      {"int", "Int32"},
      {"long", "Int64"},
      {"ParcelFileDescriptor", "Parcelable"},
      {"String", "String16"},
      {"ParcelableHolder", "Parcelable"},
  };

  static map<string, string> kBuiltinVector = {
      {"FileDescriptor", "UniqueFileDescriptorVector"},
      {"double", "DoubleVector"},
      {"char", "CharVector"},
      {"boolean", "BoolVector"},
      {"byte", "ByteVector"},
      {"float", "FloatVector"},
      {"IBinder", "StrongBinderVector"},
      {"String", "String16Vector"},
      {"int", "Int32Vector"},
      {"long", "Int64Vector"},
      {"ParcelFileDescriptor", "ParcelableVector"},
  };

  const bool nullable = type.IsNullable();
  const bool isVector = type.IsArray() || typenames.IsList(type);
  const bool utf8 = type.IsUtf8InCpp();

  if (auto enum_decl = typenames.GetEnumDeclaration(type); enum_decl != nullptr) {
    if (isVector) {
      return "EnumVector";
    } else {
      return RawParcelMethod(enum_decl->GetBackingType(), typenames, readMethod);
    }
  }

  if (isVector) {
    string element_name;
    if (typenames.IsList(type)) {
      AIDL_FATAL_IF(type.GetTypeParameters().size() != 1, type);
      element_name = type.GetTypeParameters().at(0)->GetName();
    } else {
      element_name = type.GetName();
    }
    if (kBuiltinVector.find(element_name) != kBuiltinVector.end()) {
      AIDL_FATAL_IF(!AidlTypenames::IsBuiltinTypename(element_name), type);
      if (utf8) {
        AIDL_FATAL_IF(element_name != "String", type);
        return readMethod ? "Utf8VectorFromUtf16Vector" : "Utf8VectorAsUtf16Vector";
      }
      return kBuiltinVector[element_name];
    }
    auto definedType = typenames.TryGetDefinedType(element_name);
    if (definedType != nullptr && definedType->AsInterface() != nullptr) {
      return "StrongBinderVector";
    }
    return "ParcelableVector";
  }

  const string& type_name = type.GetName();
  if (kBuiltin.find(type_name) != kBuiltin.end()) {
    AIDL_FATAL_IF(!AidlTypenames::IsBuiltinTypename(type_name), type);
    if (type_name == "IBinder" && nullable && readMethod) {
      return "NullableStrongBinder";
    }
    if (type_name == "ParcelFileDescriptor" && nullable && !readMethod) {
      return "NullableParcelable";
    }
    if (utf8) {
      AIDL_FATAL_IF(type_name != "String", type);
      return readMethod ? "Utf8FromUtf16" : "Utf8AsUtf16";
    }
    return kBuiltin[type_name];
  }

  AIDL_FATAL_IF(AidlTypenames::IsBuiltinTypename(type.GetName()), type);
  auto definedType = typenames.TryGetDefinedType(type.GetName());
  // The type must be either primitive or interface or parcelable,
  // so it cannot be nullptr.
  AIDL_FATAL_IF(definedType == nullptr, type) << type.GetName() << " is not found.";

  if (definedType->AsInterface() != nullptr) {
    if (nullable && readMethod) {
      return "NullableStrongBinder";
    }
    return "StrongBinder";
  }

  // Parcelable
  if (nullable && !readMethod) {
    return "NullableParcelable";
  }
  return "Parcelable";
}

std::string GetRawCppName(const AidlTypeSpecifier& type) {
  return "::" + Join(type.GetSplitName(), "::");
}

std::string WrapIfNullable(const std::string type_str, const AidlTypeSpecifier& raw_type,
                           const AidlTypenames& typenames) {
  const auto& type = typenames.IsList(raw_type) ? (*raw_type.GetTypeParameters().at(0)) : raw_type;

  if (raw_type.IsNullable() && !AidlTypenames::IsPrimitiveTypename(type.GetName()) &&
      type.GetName() != "IBinder" && typenames.GetEnumDeclaration(type) == nullptr) {
    return "::std::optional<" + type_str + ">";
  }
  return type_str;
}

std::string GetCppName(const AidlTypeSpecifier& raw_type, const AidlTypenames& typenames) {
  // map from AIDL built-in type name to the corresponding Cpp type name
  static map<string, string> m = {
      {"boolean", "bool"},
      {"byte", "int8_t"},
      {"char", "char16_t"},
      {"double", "double"},
      {"FileDescriptor", "::android::base::unique_fd"},
      {"float", "float"},
      {"IBinder", "::android::sp<::android::IBinder>"},
      {"int", "int32_t"},
      {"long", "int64_t"},
      {"ParcelFileDescriptor", "::android::os::ParcelFileDescriptor"},
      {"String", "::android::String16"},
      {"void", "void"},
      {"ParcelableHolder", "::android::os::ParcelableHolder"},
  };
  AIDL_FATAL_IF(typenames.IsList(raw_type) && raw_type.GetTypeParameters().size() != 1, raw_type);
  const auto& type = typenames.IsList(raw_type) ? (*raw_type.GetTypeParameters().at(0)) : raw_type;
  const string& aidl_name = type.GetName();
  if (m.find(aidl_name) != m.end()) {
    AIDL_FATAL_IF(!AidlTypenames::IsBuiltinTypename(aidl_name), raw_type);
    if (aidl_name == "byte" && type.IsArray()) {
      return "uint8_t";
    } else if (raw_type.IsUtf8InCpp()) {
      AIDL_FATAL_IF(aidl_name != "String", type);
      return WrapIfNullable("::std::string", raw_type, typenames);
    }
    return WrapIfNullable(m[aidl_name], raw_type, typenames);
  }
  auto definedType = typenames.TryGetDefinedType(type.GetName());
  if (definedType != nullptr && definedType->AsInterface() != nullptr) {
    return "::android::sp<" + GetRawCppName(type) + ">";
  }

  return WrapIfNullable(GetRawCppName(type), raw_type, typenames);
}
}  // namespace
std::string ConstantValueDecorator(const AidlTypeSpecifier& type, const std::string& raw_value) {
  if (type.IsArray()) {
    return raw_value;
  }

  if (type.GetName() == "long") {
    return raw_value + "L";
  }

  if (type.GetName() == "String" && !type.IsUtf8InCpp()) {
    return "::android::String16(" + raw_value + ")";
  }

  if (auto defined_type = type.GetDefinedType(); defined_type) {
    auto enum_type = defined_type->AsEnumDeclaration();
    AIDL_FATAL_IF(!enum_type, type) << "Invalid type for \"" << raw_value << "\"";
    return GetRawCppName(type) + "::" + raw_value.substr(raw_value.find_last_of('.') + 1);
  }

  return raw_value;
};

std::string GetTransactionIdFor(const AidlInterface& iface, const AidlMethod& method) {
  return ClassName(iface, ClassNames::SERVER) + "::TRANSACTION_" + method.GetName();
}

std::string CppNameOf(const AidlTypeSpecifier& type, const AidlTypenames& typenames) {
  if (type.IsArray() || typenames.IsList(type)) {
    std::string cpp_name = GetCppName(type, typenames);
    if (type.IsNullable()) {
      return "::std::optional<::std::vector<" + cpp_name + ">>";
    }
    return "::std::vector<" + cpp_name + ">";
  } else if (type.IsGeneric()) {
    std::vector<std::string> type_params;
    for (const auto& parameter : type.GetTypeParameters()) {
      type_params.push_back(CppNameOf(*parameter, typenames));
    }
    return StringPrintf("%s<%s>", GetCppName(type, typenames).c_str(),
                        base::Join(type_params, ", ").c_str());
  }
  return GetCppName(type, typenames);
}

bool IsNonCopyableType(const AidlTypeSpecifier& type, const AidlTypenames& typenames) {
  if (type.IsArray() || typenames.IsList(type)) {
    return false;
  }

  const std::string cpp_name = GetCppName(type, typenames);
  if (cpp_name == "::android::base::unique_fd") {
    return true;
  }
  return false;
}

std::string ParcelReadMethodOf(const AidlTypeSpecifier& type, const AidlTypenames& typenames) {
  return "read" + RawParcelMethod(type, typenames, true /* readMethod */);
}

std::string ParcelReadCastOf(const AidlTypeSpecifier& type, const AidlTypenames& typenames,
                             const std::string& variable_name) {
  if (auto enum_decl = typenames.GetEnumDeclaration(type);
      enum_decl != nullptr && !type.IsArray()) {
    return StringPrintf("reinterpret_cast<%s *>(%s)",
                        CppNameOf(enum_decl->GetBackingType(), typenames).c_str(),
                        variable_name.c_str());
  }

  return variable_name;
}

std::string ParcelWriteMethodOf(const AidlTypeSpecifier& type, const AidlTypenames& typenames) {
  return "write" + RawParcelMethod(type, typenames, false /* readMethod */);
}

std::string ParcelWriteCastOf(const AidlTypeSpecifier& type, const AidlTypenames& typenames,
                              const std::string& variable_name) {
  if (auto enum_decl = typenames.GetEnumDeclaration(type);
      enum_decl != nullptr && !type.IsArray()) {
    return StringPrintf("static_cast<%s>(%s)",
                        CppNameOf(enum_decl->GetBackingType(), typenames).c_str(),
                        variable_name.c_str());
  }

  if (typenames.GetInterface(type) != nullptr) {
    return GetRawCppName(type) + "::asBinder(" + variable_name + ")";
  }

  return variable_name;
}

void AddHeaders(const AidlTypeSpecifier& type, const AidlTypenames& typenames,
                std::set<std::string>* headers) {
  AIDL_FATAL_IF(typenames.IsList(type) && type.GetTypeParameters().size() != 1, type);
  bool isVector = type.IsArray() || typenames.IsList(type);
  bool isNullable = type.IsNullable();
  bool utf8 = type.IsUtf8InCpp();

  if (isVector) {
    headers->insert("vector");
  }
  if (type.IsGeneric()) {
    for (const auto& parameter : type.GetTypeParameters()) {
      AddHeaders(*parameter, typenames, headers);
    }
  }
  if (isNullable) {
    if (type.GetName() != "IBinder") {
      headers->insert("optional");
    }
  }
  if (typenames.IsList(type)) {
    // Nothing else to do for List.
    return;
  }
  if (type.GetName() == "String") {
    headers->insert(utf8 ? "string" : "utils/String16.h");
    return;
  }
  if (type.GetName() == "IBinder") {
    headers->insert("binder/IBinder.h");
    return;
  }
  if (type.GetName() == "FileDescriptor") {
    headers->insert("android-base/unique_fd.h");
    return;
  }
  if (type.GetName() == "ParcelFileDescriptor") {
    headers->insert("binder/ParcelFileDescriptor.h");
    return;
  }
  if (type.GetName() == "ParcelableHolder") {
    headers->insert("binder/ParcelableHolder.h");
    return;
  }

  static const std::set<string> need_cstdint{"byte", "int", "long"};
  if (need_cstdint.find(type.GetName()) != need_cstdint.end()) {
    headers->insert("cstdint");
    return;
  }

  if (AidlTypenames::IsPrimitiveTypename(type.GetName())) {
    return;
  }

  auto definedType = typenames.TryGetDefinedType(type.GetName());
  AIDL_FATAL_IF(definedType == nullptr, type) << "Unexpected type: " << type.GetName();

  if (definedType->AsInterface() != nullptr || definedType->AsStructuredParcelable() != nullptr ||
      definedType->AsEnumDeclaration() != nullptr || definedType->AsUnionDeclaration() != nullptr) {
    AddHeaders(*definedType, headers);
  } else if (definedType->AsParcelable() != nullptr) {
    const std::string cpp_header = definedType->AsParcelable()->GetCppHeader();
    AIDL_FATAL_IF(cpp_header.empty(), definedType->AsParcelable())
        << "Parcelable " << definedType->AsParcelable()->GetCanonicalName()
        << " has no C++ header defined.";
    headers->insert(cpp_header);
  }
}

void AddHeaders(const AidlDefinedType& definedType, std::set<std::string>* headers) {
  vector<string> name = definedType.GetSplitPackage();
  name.push_back(definedType.GetName());
  const std::string cpp_header = Join(name, '/') + ".h";
  headers->insert(cpp_header);
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android

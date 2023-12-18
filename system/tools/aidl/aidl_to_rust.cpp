/*
 * Copyright (C) 2020, The Android Open Source Project
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

#include "aidl_to_rust.h"
#include "aidl_language.h"
#include "aidl_typenames.h"
#include "logging.h"

#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using android::base::Join;
using android::base::Split;
using android::base::StringPrintf;

namespace android {
namespace aidl {
namespace rust {

namespace {
std::string GetRawRustName(const AidlTypeSpecifier& type);

std::string ConstantValueDecoratorInternal(const AidlTypeSpecifier& type,
                                           const std::string& raw_value, bool by_ref) {
  if (type.IsArray()) {
    // Convert `{ ... }` to `vec!{ ... }`
    return "vec!" + raw_value;
  }

  const auto& aidl_name = type.GetName();
  if (aidl_name == "char") {
    return raw_value + " as u16";
  }

  if (aidl_name == "float") {
    // raw_value already ends in `f`, so just add `32`
    return raw_value + "32";
  }

  if (aidl_name == "double") {
    return raw_value + "f64";
  }

  if (aidl_name == "String" && !by_ref) {
    // The actual type might be String or &str,
    // and .into() transparently converts into either one
    return raw_value + ".into()";
  }

  if (auto defined_type = type.GetDefinedType(); defined_type) {
    auto enum_type = defined_type->AsEnumDeclaration();
    AIDL_FATAL_IF(!enum_type, type) << "Invalid type for \"" << raw_value << "\"";
    return GetRawRustName(type) + "::" + raw_value.substr(raw_value.find_last_of('.') + 1);
  }

  return raw_value;
}

std::string GetRawRustName(const AidlTypeSpecifier& type) {
  // Each Rust type is defined in a file with the same name,
  // e.g., IFoo is in IFoo.rs
  auto split_name = type.GetSplitName();
  std::string rust_name{"crate::mangled::"};
  for (const auto& component : split_name) {
    rust_name += StringPrintf("_%zd_%s", component.size(), component.c_str());
  }
  return rust_name;
}

std::string GetRustName(const AidlTypeSpecifier& type, const AidlTypenames& typenames,
                        StorageMode mode) {
  // map from AIDL built-in type name to the corresponding Rust type name
  static map<string, string> m = {
      {"void", "()"},
      {"boolean", "bool"},
      {"byte", "i8"},
      {"char", "u16"},
      {"int", "i32"},
      {"long", "i64"},
      {"float", "f32"},
      {"double", "f64"},
      {"String", "String"},
      {"IBinder", "binder::SpIBinder"},
      {"ParcelFileDescriptor", "binder::parcel::ParcelFileDescriptor"},
  };

  // If the type is an array/List<T>, get the inner element type
  AIDL_FATAL_IF(typenames.IsList(type) && type.GetTypeParameters().size() != 1, type);
  const auto& element_type = type.IsGeneric() ? (*type.GetTypeParameters().at(0)) : type;
  const string& element_type_name = element_type.GetName();
  if (m.find(element_type_name) != m.end()) {
    AIDL_FATAL_IF(!AidlTypenames::IsBuiltinTypename(element_type_name), type);
    if (element_type_name == "byte" && type.IsArray()) {
      return "u8";
    } else if (element_type_name == "String" && mode == StorageMode::UNSIZED_ARGUMENT) {
      return "str";
    } else if (element_type_name == "ParcelFileDescriptor") {
      if (type.IsArray() && mode == StorageMode::DEFAULT_VALUE) {
        // Out-arguments of ParcelFileDescriptors arrays need to
        // be Vec<Option<ParcelFileDescriptor>> so resize_out_vec
        // can initialize all elements to None (it requires Default
        // and ParcelFileDescriptor doesn't implement that)
        return "Option<" + m[element_type_name] + ">";
      } else {
        return m[element_type_name];
      }
    }
    return m[element_type_name];
  }
  if (TypeIsInterface(element_type, typenames)) {
    return "binder::Strong<dyn " + GetRawRustName(element_type) + ">";
  }

  return GetRawRustName(element_type);
}
}  // namespace

std::string ConstantValueDecorator(const AidlTypeSpecifier& type, const std::string& raw_value) {
  auto rust_value = ConstantValueDecoratorInternal(type, raw_value, false);
  if (type.IsNullable()) {
    return "Some(" + rust_value + ")";
  }
  return rust_value;
}

std::string ConstantValueDecoratorRef(const AidlTypeSpecifier& type, const std::string& raw_value) {
  auto rust_value = ConstantValueDecoratorInternal(type, raw_value, true);
  if (type.IsNullable()) {
    return "Some(" + rust_value + ")";
  }
  return rust_value;
}

std::string RustNameOf(const AidlTypeSpecifier& type, const AidlTypenames& typenames,
                       StorageMode mode) {
  std::string rust_name;
  if (type.IsArray() || typenames.IsList(type)) {
    StorageMode element_mode;
    if (mode == StorageMode::OUT_ARGUMENT || mode == StorageMode::DEFAULT_VALUE) {
      // Elements need to have Default for resize_out_vec()
      element_mode = StorageMode::DEFAULT_VALUE;
    } else {
      element_mode = StorageMode::VALUE;
    }
    rust_name = GetRustName(type, typenames, element_mode);
    if (type.IsNullable() && rust_name == "String") {
      // The mapping for nullable string arrays is
      // optional<vector<optional<string>>> in the NDK,
      // so we do the same
      rust_name = "Option<" + rust_name + ">";
    }
    if (mode == StorageMode::UNSIZED_ARGUMENT) {
      rust_name = "[" + rust_name + "]";
    } else {
      rust_name = "Vec<" + rust_name + ">";
    }
  } else {
    rust_name = GetRustName(type, typenames, mode);
  }

  if (mode == StorageMode::IN_ARGUMENT || mode == StorageMode::UNSIZED_ARGUMENT) {
    // If this is a nullable input argument, put the reference inside the option,
    // e.g., `Option<&str>` instead of `&Option<str>`
    rust_name = "&" + rust_name;
  }

  if (type.IsNullable() ||
      // Some types don't implement Default, so we wrap them
      // in Option, which defaults to None
      (!TypeHasDefault(type, typenames) &&
       (mode == StorageMode::DEFAULT_VALUE || mode == StorageMode::OUT_ARGUMENT ||
        mode == StorageMode::PARCELABLE_FIELD))) {
    rust_name = "Option<" + rust_name + ">";
  }

  if (mode == StorageMode::OUT_ARGUMENT || mode == StorageMode::INOUT_ARGUMENT) {
    rust_name = "&mut " + rust_name;
  }

  return rust_name;
}

StorageMode ArgumentStorageMode(const AidlArgument& arg, const AidlTypenames& typenames) {
  if (arg.IsOut()) {
    return arg.IsIn() ? StorageMode::INOUT_ARGUMENT : StorageMode::OUT_ARGUMENT;
  }

  const auto typeName = arg.GetType().GetName();
  const auto definedType = typenames.TryGetDefinedType(typeName);

  const bool isEnum = definedType && definedType->AsEnumDeclaration() != nullptr;
  const bool isPrimitive = AidlTypenames::IsPrimitiveTypename(typeName);
  if (typeName == "String" || arg.GetType().IsArray() || typenames.IsList(arg.GetType())) {
    return StorageMode::UNSIZED_ARGUMENT;
  } else if (!(isPrimitive || isEnum)) {
    return StorageMode::IN_ARGUMENT;
  } else {
    return StorageMode::VALUE;
  }
}

ReferenceMode ArgumentReferenceMode(const AidlArgument& arg, const AidlTypenames& typenames) {
  auto arg_mode = ArgumentStorageMode(arg, typenames);
  switch (arg_mode) {
    case StorageMode::IN_ARGUMENT:
      if (arg.GetType().IsNullable()) {
        // &Option<T> => Option<&T>
        return ReferenceMode::AS_REF;
      } else {
        return ReferenceMode::REF;
      }

    case StorageMode::OUT_ARGUMENT:
    case StorageMode::INOUT_ARGUMENT:
      return ReferenceMode::MUT_REF;

    case StorageMode::UNSIZED_ARGUMENT:
      if (arg.GetType().IsNullable()) {
        // &Option<String> => Option<&str>
        // &Option<Vec<T>> => Option<&[T]>
        return ReferenceMode::AS_DEREF;
      } else {
        return ReferenceMode::REF;
      }

    default:
      return ReferenceMode::VALUE;
  }
}

std::string TakeReference(ReferenceMode ref_mode, const std::string& name) {
  switch (ref_mode) {
    case ReferenceMode::REF:
      return "&" + name;

    case ReferenceMode::MUT_REF:
      return "&mut " + name;

    case ReferenceMode::AS_REF:
      return name + ".as_ref()";

    case ReferenceMode::AS_DEREF:
      return name + ".as_deref()";

    default:
      return name;
  }
}

bool TypeIsInterface(const AidlTypeSpecifier& type, const AidlTypenames& typenames) {
  const auto definedType = typenames.TryGetDefinedType(type.GetName());
  return definedType != nullptr && definedType->AsInterface() != nullptr;
}

bool TypeHasDefault(const AidlTypeSpecifier& type, const AidlTypenames& typenames) {
  if (type.IsArray() || typenames.IsList(type)) {
    return true;
  }

  // Already an Option<T>
  if (type.IsNullable()) {
    return true;
  }

  const string& aidl_name = type.GetName();
  if (aidl_name == "IBinder") {
    return false;
  }
  if (aidl_name == "ParcelFileDescriptor") {
    return false;
  }

  // Strong<dyn IFoo> values don't implement Default
  if (TypeIsInterface(type, typenames)) {
    return false;
  }

  return true;
}

}  // namespace rust
}  // namespace aidl
}  // namespace android

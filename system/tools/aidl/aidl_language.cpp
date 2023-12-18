/*
 * Copyright (C) 2015, The Android Open Source Project
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

#include "aidl_language.h"
#include "aidl_typenames.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>

#include <android-base/parsedouble.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>

#include "aidl_language_y.h"
#include "comments.h"
#include "logging.h"

#include "aidl.h"

#ifdef _WIN32
int isatty(int  fd)
{
    return (fd == 0);
}
#endif

using android::aidl::IoDelegate;
using android::base::Join;
using android::base::Split;
using std::cerr;
using std::pair;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

namespace {
bool IsJavaKeyword(const char* str) {
  static const std::vector<std::string> kJavaKeywords{
      "abstract", "assert", "boolean",    "break",     "byte",       "case",      "catch",
      "char",     "class",  "const",      "continue",  "default",    "do",        "double",
      "else",     "enum",   "extends",    "final",     "finally",    "float",     "for",
      "goto",     "if",     "implements", "import",    "instanceof", "int",       "interface",
      "long",     "native", "new",        "package",   "private",    "protected", "public",
      "return",   "short",  "static",     "strictfp",  "super",      "switch",    "synchronized",
      "this",     "throw",  "throws",     "transient", "try",        "void",      "volatile",
      "while",    "true",   "false",      "null",
  };
  return std::find(kJavaKeywords.begin(), kJavaKeywords.end(), str) != kJavaKeywords.end();
}
}  // namespace

AidlNode::AidlNode(const AidlLocation& location, const Comments& comments)
    : location_(location), comments_(comments) {}

std::string AidlNode::PrintLine() const {
  std::stringstream ss;
  ss << location_.file_ << ":" << location_.begin_.line;
  return ss.str();
}

std::string AidlNode::PrintLocation() const {
  std::stringstream ss;
  ss << location_.file_ << ":" << location_.begin_.line << ":" << location_.begin_.column << ":"
     << location_.end_.line << ":" << location_.end_.column;
  return ss.str();
}

static const AidlTypeSpecifier kStringType{AIDL_LOCATION_HERE, "String", false, nullptr,
                                           Comments{}};
static const AidlTypeSpecifier kStringArrayType{AIDL_LOCATION_HERE, "String", true, nullptr,
                                                Comments{}};
static const AidlTypeSpecifier kIntType{AIDL_LOCATION_HERE, "int", false, nullptr, Comments{}};
static const AidlTypeSpecifier kLongType{AIDL_LOCATION_HERE, "long", false, nullptr, Comments{}};
static const AidlTypeSpecifier kBooleanType{AIDL_LOCATION_HERE, "boolean", false, nullptr,
                                            Comments{}};

const std::vector<AidlAnnotation::Schema>& AidlAnnotation::AllSchemas() {
  static const std::vector<Schema> kSchemas{
      {AidlAnnotation::Type::NULLABLE, "nullable", CONTEXT_TYPE_SPECIFIER, {}},
      {AidlAnnotation::Type::UTF8_IN_CPP, "utf8InCpp", CONTEXT_TYPE_SPECIFIER, {}},
      {AidlAnnotation::Type::SENSITIVE_DATA, "SensitiveData", CONTEXT_TYPE_INTERFACE, {}},
      {AidlAnnotation::Type::VINTF_STABILITY, "VintfStability", CONTEXT_TYPE, {}},
      {AidlAnnotation::Type::UNSUPPORTED_APP_USAGE,
       "UnsupportedAppUsage",
       CONTEXT_TYPE | CONTEXT_MEMBER,
       {{"expectedSignature", kStringType},
        {"implicitMember", kStringType},
        {"maxTargetSdk", kIntType},
        {"publicAlternatives", kStringType},
        {"trackingBug", kLongType}}},
      {AidlAnnotation::Type::JAVA_STABLE_PARCELABLE,
       "JavaOnlyStableParcelable",
       CONTEXT_TYPE_UNSTRUCTURED_PARCELABLE,
       {}},
      {AidlAnnotation::Type::HIDE, "Hide", CONTEXT_TYPE | CONTEXT_MEMBER, {}},
      {AidlAnnotation::Type::BACKING,
       "Backing",
       CONTEXT_TYPE_ENUM,
       {{"type", kStringType, /* required= */ true}}},
      {AidlAnnotation::Type::JAVA_PASSTHROUGH,
       "JavaPassthrough",
       CONTEXT_ALL,
       {{"annotation", kStringType, /* required= */ true}},
       /* repeatable= */ true},
      {AidlAnnotation::Type::JAVA_DERIVE,
       "JavaDerive",
       CONTEXT_TYPE_STRUCTURED_PARCELABLE | CONTEXT_TYPE_UNION,
       {{"toString", kBooleanType}, {"equals", kBooleanType}}},
      {AidlAnnotation::Type::JAVA_ONLY_IMMUTABLE,
       "JavaOnlyImmutable",
       CONTEXT_TYPE_STRUCTURED_PARCELABLE | CONTEXT_TYPE_UNION |
           CONTEXT_TYPE_UNSTRUCTURED_PARCELABLE,
       {}},
      {AidlAnnotation::Type::FIXED_SIZE, "FixedSize", CONTEXT_TYPE_STRUCTURED_PARCELABLE, {}},
      {AidlAnnotation::Type::DESCRIPTOR,
       "Descriptor",
       CONTEXT_TYPE_INTERFACE,
       {{"value", kStringType, /* required= */ true}}},
      {AidlAnnotation::Type::RUST_DERIVE,
       "RustDerive",
       CONTEXT_TYPE_STRUCTURED_PARCELABLE | CONTEXT_TYPE_UNION,
       {{"Copy", kBooleanType},
        {"Clone", kBooleanType},
        {"PartialOrd", kBooleanType},
        {"Ord", kBooleanType},
        {"PartialEq", kBooleanType},
        {"Eq", kBooleanType},
        {"Hash", kBooleanType}}},
      {AidlAnnotation::Type::SUPPRESS_WARNINGS,
       "SuppressWarnings",
       CONTEXT_TYPE | CONTEXT_MEMBER,
       {{"value", kStringArrayType, /* required= */ true}}},
  };
  return kSchemas;
}

std::string AidlAnnotation::TypeToString(Type type) {
  for (const Schema& schema : AllSchemas()) {
    if (type == schema.type) return schema.name;
  }
  AIDL_FATAL(AIDL_LOCATION_HERE) << "Unrecognized type: " << static_cast<size_t>(type);
  __builtin_unreachable();
}

AidlAnnotation* AidlAnnotation::Parse(
    const AidlLocation& location, const string& name,
    std::map<std::string, std::shared_ptr<AidlConstantValue>>* parameter_list,
    const Comments& comments) {
  const Schema* schema = nullptr;
  for (const Schema& a_schema : AllSchemas()) {
    if (a_schema.name == name) {
      schema = &a_schema;
    }
  }

  if (schema == nullptr) {
    std::ostringstream stream;
    stream << "'" << name << "' is not a recognized annotation. ";
    stream << "It must be one of:";
    for (const Schema& s : AllSchemas()) {
      stream << " " << s.name;
    }
    stream << ".";
    AIDL_ERROR(location) << stream.str();
    return nullptr;
  }
  if (parameter_list == nullptr) {
    return new AidlAnnotation(location, *schema, {}, comments);
  }

  return new AidlAnnotation(location, *schema, std::move(*parameter_list), comments);
}

AidlAnnotation::AidlAnnotation(
    const AidlLocation& location, const Schema& schema,
    std::map<std::string, std::shared_ptr<AidlConstantValue>>&& parameters,
    const Comments& comments)
    : AidlNode(location, comments), schema_(schema), parameters_(std::move(parameters)) {}

struct ConstReferenceFinder : AidlVisitor {
  const AidlConstantReference* found;
  void Visit(const AidlConstantReference& ref) override {
    if (!found) found = &ref;
  }
  static const AidlConstantReference* Find(const AidlConstantValue& c) {
    ConstReferenceFinder finder;
    VisitTopDown(finder, c);
    return finder.found;
  }
};

// Checks if annotation complies with the schema
// - every parameter is known and has well-typed value.
// - every required parameter is present.
bool AidlAnnotation::CheckValid() const {
  for (const auto& name_and_param : parameters_) {
    const std::string& param_name = name_and_param.first;
    const std::shared_ptr<AidlConstantValue>& param = name_and_param.second;

    const ParamType* param_type = schema_.ParamType(param_name);
    if (!param_type) {
      std::ostringstream stream;
      stream << "Parameter " << param_name << " not supported ";
      stream << "for annotation " << GetName() << ". ";
      stream << "It must be one of:";
      for (const auto& param : schema_.parameters) {
        stream << " " << param.name;
      }
      AIDL_ERROR(this) << stream.str();
      return false;
    }

    const auto& found = ConstReferenceFinder::Find(*param);
    if (found) {
      AIDL_ERROR(found) << "Value must be a constant expression but contains reference to "
                        << found->GetFieldName() << ".";
      return false;
    }

    if (!param->CheckValid()) {
      AIDL_ERROR(this) << "Invalid value for parameter " << param_name << " on annotation "
                       << GetName() << ".";
      return false;
    }

    const std::string param_value =
        param->ValueString(param_type->type, AidlConstantValueDecorator);
    // Assume error on empty string.
    if (param_value == "") {
      AIDL_ERROR(this) << "Invalid value for parameter " << param_name << " on annotation "
                       << GetName() << ".";
      return false;
    }
  }
  bool success = true;
  for (const auto& param : schema_.parameters) {
    if (param.required && parameters_.count(param.name) == 0) {
      AIDL_ERROR(this) << "Missing '" << param.name << "' on @" << GetName() << ".";
      success = false;
    }
  }
  return success;
}

// Checks if the annotation is applicable to the current context.
// For example, annotations like @VintfStability, @FixedSize is not applicable to AidlTypeSpecifier
// nodes.
bool AidlAnnotation::CheckContext(TargetContext context) const {
  if (schema_.target_context & static_cast<uint32_t>(context)) {
    return true;
  }
  const static map<TargetContext, string> context_name_map{
      {CONTEXT_TYPE_INTERFACE, "interface"},
      {CONTEXT_TYPE_ENUM, "enum"},
      {CONTEXT_TYPE_STRUCTURED_PARCELABLE, "structured parcelable"},
      {CONTEXT_TYPE_UNION, "union"},
      {CONTEXT_TYPE_UNSTRUCTURED_PARCELABLE, "parcelable"},
      {CONTEXT_CONST, "constant"},
      {CONTEXT_FIELD, "field"},
      {CONTEXT_METHOD, "method"},
      {CONTEXT_TYPE_SPECIFIER, "type"},
  };
  vector<string> available;
  for (const auto& [context, name] : context_name_map) {
    if (schema_.target_context & context) {
      available.push_back(name);
    }
  }
  AIDL_ERROR(this) << "@" << GetName() << " is not available. It can annotate {"
                   << Join(available, ", ") << "}.";
  return false;
}

std::map<std::string, std::string> AidlAnnotation::AnnotationParams(
    const ConstantValueDecorator& decorator) const {
  std::map<std::string, std::string> raw_params;
  for (const auto& name_and_param : parameters_) {
    const std::string& param_name = name_and_param.first;
    const std::shared_ptr<AidlConstantValue>& param = name_and_param.second;
    const ParamType* param_type = schema_.ParamType(param_name);
    AIDL_FATAL_IF(!param_type, this);
    raw_params.emplace(param_name, param->ValueString(param_type->type, decorator));
  }
  return raw_params;
}

std::string AidlAnnotation::ToString() const {
  if (parameters_.empty()) {
    return "@" + GetName();
  } else {
    vector<string> param_strings;
    for (const auto& [name, value] : AnnotationParams(AidlConstantValueDecorator)) {
      param_strings.emplace_back(name + "=" + value);
    }
    return "@" + GetName() + "(" + Join(param_strings, ", ") + ")";
  }
}

void AidlAnnotation::TraverseChildren(std::function<void(const AidlNode&)> traverse) const {
  for (const auto& [name, value] : parameters_) {
    (void)name;
    traverse(*value);
  }
}

static const AidlAnnotation* GetAnnotation(const vector<AidlAnnotation>& annotations,
                                           AidlAnnotation::Type type) {
  for (const auto& a : annotations) {
    if (a.GetType() == type) {
      AIDL_FATAL_IF(a.Repeatable(), a)
          << "Trying to get a single annotation when it is repeatable.";
      return &a;
    }
  }
  return nullptr;
}

AidlAnnotatable::AidlAnnotatable(const AidlLocation& location, const Comments& comments)
    : AidlCommentable(location, comments) {}

bool AidlAnnotatable::IsNullable() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::NULLABLE);
}

bool AidlAnnotatable::IsUtf8InCpp() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::UTF8_IN_CPP);
}

bool AidlAnnotatable::IsSensitiveData() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::SENSITIVE_DATA);
}

bool AidlAnnotatable::IsVintfStability() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::VINTF_STABILITY);
}

bool AidlAnnotatable::IsJavaOnlyImmutable() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::JAVA_ONLY_IMMUTABLE);
}

bool AidlAnnotatable::IsFixedSize() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::FIXED_SIZE);
}

const AidlAnnotation* AidlAnnotatable::UnsupportedAppUsage() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::UNSUPPORTED_APP_USAGE);
}

const AidlAnnotation* AidlAnnotatable::RustDerive() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::RUST_DERIVE);
}

const AidlAnnotation* AidlAnnotatable::BackingType() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::BACKING);
}

std::vector<std::string> AidlAnnotatable::SuppressWarnings() const {
  auto annot = GetAnnotation(annotations_, AidlAnnotation::Type::SUPPRESS_WARNINGS);
  if (annot) {
    auto names = annot->ParamValue<std::vector<std::string>>("value");
    AIDL_FATAL_IF(!names.has_value(), this);
    return std::move(names.value());
  }
  return {};
}

bool AidlAnnotatable::IsStableApiParcelable(Options::Language lang) const {
  return lang == Options::Language::JAVA &&
         GetAnnotation(annotations_, AidlAnnotation::Type::JAVA_STABLE_PARCELABLE);
}

bool AidlAnnotatable::IsHide() const {
  return GetAnnotation(annotations_, AidlAnnotation::Type::HIDE);
}

bool AidlAnnotatable::JavaDerive(const std::string& method) const {
  auto annotation = GetAnnotation(annotations_, AidlAnnotation::Type::JAVA_DERIVE);
  if (annotation != nullptr) {
    return annotation->ParamValue<bool>(method).value_or(false);
  }
  return false;
}

std::string AidlAnnotatable::GetDescriptor() const {
  auto annotation = GetAnnotation(annotations_, AidlAnnotation::Type::DESCRIPTOR);
  if (annotation != nullptr) {
    return annotation->ParamValue<std::string>("value").value();
  }
  return "";
}

bool AidlAnnotatable::CheckValid(const AidlTypenames&) const {
  for (const auto& annotation : GetAnnotations()) {
    if (!annotation.CheckValid()) {
      return false;
    }
  }

  std::map<AidlAnnotation::Type, AidlLocation> declared;
  for (const auto& annotation : GetAnnotations()) {
    const auto& [iter, inserted] = declared.emplace(annotation.GetType(), annotation.GetLocation());
    if (!inserted && !annotation.Repeatable()) {
      AIDL_ERROR(this) << "'" << annotation.GetName()
                       << "' is repeated, but not allowed. Previous location: " << iter->second;
      return false;
    }
  }

  return true;
}

string AidlAnnotatable::ToString() const {
  vector<string> ret;
  for (const auto& a : annotations_) {
    ret.emplace_back(a.ToString());
  }
  std::sort(ret.begin(), ret.end());
  return Join(ret, " ");
}

AidlTypeSpecifier::AidlTypeSpecifier(const AidlLocation& location, const string& unresolved_name,
                                     bool is_array,
                                     vector<unique_ptr<AidlTypeSpecifier>>* type_params,
                                     const Comments& comments)
    : AidlAnnotatable(location, comments),
      AidlParameterizable<unique_ptr<AidlTypeSpecifier>>(type_params),
      unresolved_name_(unresolved_name),
      is_array_(is_array),
      split_name_(Split(unresolved_name, ".")) {}

const AidlTypeSpecifier& AidlTypeSpecifier::ArrayBase() const {
  AIDL_FATAL_IF(!is_array_, this);
  // Declaring array of generic type cannot happen, it is grammar error.
  AIDL_FATAL_IF(IsGeneric(), this);

  if (!array_base_) {
    array_base_.reset(new AidlTypeSpecifier(*this));
    array_base_->is_array_ = false;
  }
  return *array_base_;
}

string AidlTypeSpecifier::Signature() const {
  string ret = GetName();
  if (IsGeneric()) {
    vector<string> arg_names;
    for (const auto& ta : GetTypeParameters()) {
      arg_names.emplace_back(ta->Signature());
    }
    ret += "<" + Join(arg_names, ",") + ">";
  }
  if (IsArray()) {
    ret += "[]";
  }
  return ret;
}

string AidlTypeSpecifier::ToString() const {
  string ret = Signature();
  string annotations = AidlAnnotatable::ToString();
  if (annotations != "") {
    ret = annotations + " " + ret;
  }
  return ret;
}

bool AidlTypeSpecifier::Resolve(const AidlTypenames& typenames) {
  AIDL_FATAL_IF(IsResolved(), this);
  AidlTypenames::ResolvedTypename result = typenames.ResolveTypename(unresolved_name_);
  if (result.is_resolved) {
    fully_qualified_name_ = result.canonical_name;
    split_name_ = Split(fully_qualified_name_, ".");
    defined_type_ = result.defined_type;
  }
  return result.is_resolved;
}

const AidlDefinedType* AidlTypeSpecifier::GetDefinedType() const {
  return defined_type_;
}

bool AidlTypeSpecifier::CheckValid(const AidlTypenames& typenames) const {
  if (!AidlAnnotatable::CheckValid(typenames)) {
    return false;
  }
  if (IsGeneric()) {
    const auto& types = GetTypeParameters();
    for (const auto& arg : types) {
      if (!arg->CheckValid(typenames)) {
        return false;
      }
    }

    const string& type_name = GetName();
    // TODO(b/136048684) Disallow to use primitive types only if it is List or Map.
    if (type_name == "List" || type_name == "Map") {
      if (std::any_of(types.begin(), types.end(), [&](auto& type_ptr) {
            return !type_ptr->IsArray() &&
                   (typenames.GetEnumDeclaration(*type_ptr) ||
                    AidlTypenames::IsPrimitiveTypename(type_ptr->GetName()));
          })) {
        AIDL_ERROR(this) << "A generic type cannot have any primitive type parameters.";
        return false;
      }
    }
    const auto defined_type = typenames.TryGetDefinedType(type_name);
    const auto parameterizable =
        defined_type != nullptr ? defined_type->AsParameterizable() : nullptr;
    const bool is_user_defined_generic_type =
        parameterizable != nullptr && parameterizable->IsGeneric();
    const size_t num_params = GetTypeParameters().size();
    if (type_name == "List") {
      if (num_params > 1) {
        AIDL_ERROR(this) << "List can only have one type parameter, but got: '" << Signature()
                         << "'";
        return false;
      }
      const AidlTypeSpecifier& contained_type = *GetTypeParameters()[0];
      if (contained_type.IsArray()) {
        AIDL_ERROR(this)
            << "List of arrays is not supported. List<T> supports parcelable/union, String, "
               "IBinder, and ParcelFileDescriptor.";
        return false;
      }
      const string& contained_type_name = contained_type.GetName();
      if (AidlTypenames::IsBuiltinTypename(contained_type_name)) {
        if (contained_type_name != "String" && contained_type_name != "IBinder" &&
            contained_type_name != "ParcelFileDescriptor") {
          AIDL_ERROR(this) << "List<" << contained_type_name
                           << "> is not supported. List<T> supports parcelable/union, String, "
                              "IBinder, and ParcelFileDescriptor.";
          return false;
        }
      } else {  // Defined types
        if (typenames.GetInterface(contained_type)) {
          AIDL_ERROR(this) << "List<" << contained_type_name
                           << "> is not supported. List<T> supports parcelable/union, String, "
                              "IBinder, and ParcelFileDescriptor.";
          return false;
        }
      }
    } else if (type_name == "Map") {
      if (num_params != 0 && num_params != 2) {
        AIDL_ERROR(this) << "Map must have 0 or 2 type parameters, but got "
                         << "'" << Signature() << "'";
        return false;
      }
      if (num_params == 2) {
        const string& key_type = GetTypeParameters()[0]->Signature();
        if (key_type != "String") {
          AIDL_ERROR(this) << "The type of key in map must be String, but it is "
                           << "'" << key_type << "'";
          return false;
        }
      }
    } else if (is_user_defined_generic_type) {
      const size_t allowed = parameterizable->GetTypeParameters().size();
      if (num_params != allowed) {
        AIDL_ERROR(this) << type_name << " must have " << allowed << " type parameters, but got "
                         << num_params;
        return false;
      }
    } else {
      AIDL_ERROR(this) << type_name << " is not a generic type.";
      return false;
    }
  }

  const bool is_generic_string_list = GetName() == "List" && IsGeneric() &&
                                      GetTypeParameters().size() == 1 &&
                                      GetTypeParameters()[0]->GetName() == "String";
  if (IsUtf8InCpp() && (GetName() != "String" && !is_generic_string_list)) {
    AIDL_ERROR(this) << "@utf8InCpp can only be used on String, String[], and List<String>.";
    return false;
  }

  if (GetName() == "void") {
    if (IsArray() || IsNullable() || IsUtf8InCpp()) {
      AIDL_ERROR(this) << "void type cannot be an array or nullable or utf8 string";
      return false;
    }
  }

  if (IsArray()) {
    const auto defined_type = typenames.TryGetDefinedType(GetName());
    if (defined_type != nullptr && defined_type->AsInterface() != nullptr) {
      AIDL_ERROR(this) << "Binder type cannot be an array";
      return false;
    }
    if (GetName() == "ParcelableHolder") {
      AIDL_ERROR(this) << "Arrays of ParcelableHolder are not supported.";
      return false;
    }
  }

  if (IsNullable()) {
    if (AidlTypenames::IsPrimitiveTypename(GetName()) && !IsArray()) {
      AIDL_ERROR(this) << "Primitive type cannot get nullable annotation";
      return false;
    }
    const auto defined_type = typenames.TryGetDefinedType(GetName());
    if (defined_type != nullptr && defined_type->AsEnumDeclaration() != nullptr && !IsArray()) {
      AIDL_ERROR(this) << "Enum type cannot get nullable annotation";
      return false;
    }
    if (GetName() == "ParcelableHolder") {
      AIDL_ERROR(this) << "ParcelableHolder cannot be nullable.";
      return false;
    }
  }
  return true;
}

std::string AidlConstantValueDecorator(const AidlTypeSpecifier& type,
                                       const std::string& raw_value) {
  if (type.IsArray()) {
    return raw_value;
  }

  if (auto defined_type = type.GetDefinedType(); defined_type) {
    auto enum_type = defined_type->AsEnumDeclaration();
    AIDL_FATAL_IF(!enum_type, type) << "Invalid type for \"" << raw_value << "\"";
    return type.GetName() + "." + raw_value.substr(raw_value.find_last_of('.') + 1);
  }
  return raw_value;
}

AidlVariableDeclaration::AidlVariableDeclaration(const AidlLocation& location,
                                                 AidlTypeSpecifier* type, const std::string& name)
    : AidlVariableDeclaration(location, type, name, AidlConstantValue::Default(*type)) {
  default_user_specified_ = false;
}

AidlVariableDeclaration::AidlVariableDeclaration(const AidlLocation& location,
                                                 AidlTypeSpecifier* type, const std::string& name,
                                                 AidlConstantValue* default_value)
    : AidlMember(location, type->GetComments()),
      type_(type),
      name_(name),
      default_user_specified_(true),
      default_value_(default_value) {}

bool AidlVariableDeclaration::HasUsefulDefaultValue() const {
  if (GetDefaultValue()) {
    return true;
  }
  // null is accepted as a valid default value in all backends
  if (GetType().IsNullable()) {
    return true;
  }
  return false;
}

bool AidlVariableDeclaration::CheckValid(const AidlTypenames& typenames) const {
  bool valid = true;
  valid &= type_->CheckValid(typenames);

  if (type_->GetName() == "void") {
    AIDL_ERROR(this) << "Declaration " << name_
                     << " is void, but declarations cannot be of void type.";
    valid = false;
  }

  if (default_value_ == nullptr) return valid;
  valid &= default_value_->CheckValid();

  if (!valid) return false;

  return !ValueString(AidlConstantValueDecorator).empty();
}

string AidlVariableDeclaration::GetCapitalizedName() const {
  AIDL_FATAL_IF(name_.size() <= 0, *this) << "Name can't be empty.";
  string str = name_;
  str[0] = static_cast<char>(toupper(str[0]));
  return str;
}

string AidlVariableDeclaration::ToString() const {
  string ret = type_->ToString() + " " + name_;
  if (default_value_ != nullptr && default_user_specified_) {
    ret += " = " + ValueString(AidlConstantValueDecorator);
  }
  return ret;
}

string AidlVariableDeclaration::Signature() const {
  return type_->Signature() + " " + name_;
}

std::string AidlVariableDeclaration::ValueString(const ConstantValueDecorator& decorator) const {
  if (default_value_ != nullptr) {
    return default_value_->ValueString(GetType(), decorator);
  } else {
    return "";
  }
}

void AidlVariableDeclaration::TraverseChildren(
    std::function<void(const AidlNode&)> traverse) const {
  traverse(GetType());
  if (IsDefaultUserSpecified()) {
    traverse(*GetDefaultValue());
  }
}

AidlArgument::AidlArgument(const AidlLocation& location, AidlArgument::Direction direction,
                           AidlTypeSpecifier* type, const std::string& name)
    : AidlVariableDeclaration(location, type, name),
      direction_(direction),
      direction_specified_(true) {}

AidlArgument::AidlArgument(const AidlLocation& location, AidlTypeSpecifier* type,
                           const std::string& name)
    : AidlVariableDeclaration(location, type, name),
      direction_(AidlArgument::IN_DIR),
      direction_specified_(false) {}

static std::string to_string(AidlArgument::Direction direction) {
  switch (direction) {
    case AidlArgument::IN_DIR:
      return "in";
    case AidlArgument::OUT_DIR:
      return "out";
    case AidlArgument::INOUT_DIR:
      return "inout";
  }
}

string AidlArgument::GetDirectionSpecifier() const {
  string ret;
  if (direction_specified_) {
    ret = to_string(direction_);
  }
  return ret;
}

string AidlArgument::ToString() const {
  if (direction_specified_) {
    return GetDirectionSpecifier() + " " + AidlVariableDeclaration::ToString();
  } else {
    return AidlVariableDeclaration::ToString();
  }
}

static std::string FormatDirections(const std::set<AidlArgument::Direction>& directions) {
  std::vector<std::string> out;
  for (const auto& d : directions) {
    out.push_back(to_string(d));
  }

  if (out.size() <= 1) {  // [] => "" or [A] => "A"
    return Join(out, "");
  } else if (out.size() == 2) {  // [A,B] => "A or B"
    return Join(out, " or ");
  } else {  // [A,B,C] => "A, B, or C"
    out.back() = "or " + out.back();
    return Join(out, ", ");
  }
}

bool AidlArgument::CheckValid(const AidlTypenames& typenames) const {
  if (!GetType().CheckValid(typenames)) {
    return false;
  }

  const auto& aspect = typenames.GetArgumentAspect(GetType());

  if (aspect.possible_directions.size() == 0) {
    AIDL_ERROR(this) << aspect.name << " cannot be an argument type";
    return false;
  }

  // when direction is not specified, "in" is assumed and should be the only possible direction
  if (!DirectionWasSpecified() && aspect.possible_directions != std::set{AidlArgument::IN_DIR}) {
    AIDL_ERROR(this) << "The direction of '" << GetName() << "' is not specified. " << aspect.name
                     << " can be an " << FormatDirections(aspect.possible_directions)
                     << " parameter.";
    return false;
  }

  if (aspect.possible_directions.count(GetDirection()) == 0) {
    AIDL_ERROR(this) << "'" << GetName() << "' can't be an " << GetDirectionSpecifier()
                     << " parameter because " << aspect.name << " can only be an "
                     << FormatDirections(aspect.possible_directions) << " parameter.";
    return false;
  }

  return true;
}

bool AidlCommentable::IsHidden() const {
  return android::aidl::HasHideInComments(GetComments());
}

bool AidlCommentable::IsDeprecated() const {
  return android::aidl::FindDeprecated(GetComments()).has_value();
}

AidlMember::AidlMember(const AidlLocation& location, const Comments& comments)
    : AidlCommentable(location, comments) {}

AidlConstantDeclaration::AidlConstantDeclaration(const AidlLocation& location,
                                                 AidlTypeSpecifier* type, const std::string& name,
                                                 AidlConstantValue* value)
    : AidlMember(location, type->GetComments()), type_(type), name_(name), value_(value) {}

bool AidlConstantDeclaration::CheckValid(const AidlTypenames& typenames) const {
  bool valid = true;
  valid &= type_->CheckValid(typenames);
  valid &= value_->CheckValid();
  if (!valid) return false;

  const static set<string> kSupportedConstTypes = {"String", "byte", "int", "long"};
  if (kSupportedConstTypes.find(type_->Signature()) == kSupportedConstTypes.end()) {
    AIDL_ERROR(this) << "Constant of type " << type_->Signature() << " is not supported.";
    return false;
  }

  return true;
}

string AidlConstantDeclaration::ToString() const {
  return "const " + type_->ToString() + " " + name_ + " = " +
         ValueString(AidlConstantValueDecorator);
}

string AidlConstantDeclaration::Signature() const {
  return type_->Signature() + " " + name_;
}

AidlMethod::AidlMethod(const AidlLocation& location, bool oneway, AidlTypeSpecifier* type,
                       const std::string& name, std::vector<std::unique_ptr<AidlArgument>>* args,
                       const Comments& comments)
    : AidlMethod(location, oneway, type, name, args, comments, 0, true) {
  has_id_ = false;
}

AidlMethod::AidlMethod(const AidlLocation& location, bool oneway, AidlTypeSpecifier* type,
                       const std::string& name, std::vector<std::unique_ptr<AidlArgument>>* args,
                       const Comments& comments, int id, bool is_user_defined)
    : AidlMember(location, comments),
      oneway_(oneway),
      type_(type),
      name_(name),
      arguments_(std::move(*args)),
      id_(id),
      is_user_defined_(is_user_defined) {
  has_id_ = true;
  delete args;
  for (const unique_ptr<AidlArgument>& a : arguments_) {
    if (a->IsIn()) { in_arguments_.push_back(a.get()); }
    if (a->IsOut()) { out_arguments_.push_back(a.get()); }
  }
}

string AidlMethod::Signature() const {
  vector<string> arg_signatures;
  for (const auto& arg : GetArguments()) {
    arg_signatures.emplace_back(arg->GetType().Signature());
  }
  return GetName() + "(" + Join(arg_signatures, ", ") + ")";
}

string AidlMethod::ToString() const {
  vector<string> arg_strings;
  for (const auto& arg : GetArguments()) {
    arg_strings.emplace_back(arg->ToString());
  }
  string ret = (IsOneway() ? "oneway " : "") + GetType().ToString() + " " + GetName() + "(" +
               Join(arg_strings, ", ") + ")";
  if (HasId()) {
    ret += " = " + std::to_string(GetId());
  }
  return ret;
}

AidlDefinedType::AidlDefinedType(const AidlLocation& location, const std::string& name,
                                 const Comments& comments, const std::string& package,
                                 std::vector<std::unique_ptr<AidlMember>>* members)
    : AidlAnnotatable(location, comments),
      name_(name),
      package_(package),
      split_package_(package.empty() ? std::vector<std::string>()
                                     : android::base::Split(package, ".")) {
  if (members) {
    for (auto& m : *members) {
      if (auto constant = m->AsConstantDeclaration(); constant) {
        constants_.emplace_back(constant);
      } else if (auto variable = m->AsVariableDeclaration(); variable) {
        variables_.emplace_back(variable);
      } else if (auto method = m->AsMethod(); method) {
        methods_.emplace_back(method);
      } else {
        AIDL_FATAL(*m);
      }
      members_.push_back(m.release());
    }
    delete members;
  }
}

bool AidlDefinedType::CheckValid(const AidlTypenames& typenames) const {
  if (!AidlAnnotatable::CheckValid(typenames)) {
    return false;
  }
  if (!CheckValidWithMembers(typenames)) {
    return false;
  }
  return true;
}

std::string AidlDefinedType::GetCanonicalName() const {
  if (package_.empty()) {
    return GetName();
  }
  return GetPackage() + "." + GetName();
}

bool AidlDefinedType::CheckValidWithMembers(const AidlTypenames& typenames) const {
  bool success = true;

  for (const auto& v : GetFields()) {
    const bool field_valid = v->CheckValid(typenames);
    success = success && field_valid;
  }

  // field names should be unique
  std::set<std::string> fieldnames;
  for (const auto& v : GetFields()) {
    bool duplicated = !fieldnames.emplace(v->GetName()).second;
    if (duplicated) {
      AIDL_ERROR(v) << "'" << GetName() << "' has duplicate field name '" << v->GetName() << "'";
      success = false;
    }
  }

  // immutable parcelables should have immutable fields.
  if (IsJavaOnlyImmutable()) {
    for (const auto& v : GetFields()) {
      if (!typenames.CanBeJavaOnlyImmutable(v->GetType())) {
        AIDL_ERROR(v) << "The @JavaOnlyImmutable '" << GetName() << "' has a "
                      << "non-immutable field named '" << v->GetName() << "'.";
        success = false;
      }
    }
  }

  set<string> constant_names;
  for (const auto& constant : GetConstantDeclarations()) {
    if (constant_names.count(constant->GetName()) > 0) {
      AIDL_ERROR(constant) << "Found duplicate constant name '" << constant->GetName() << "'";
      success = false;
    }
    constant_names.insert(constant->GetName());
    success = success && constant->CheckValid(typenames);
  }

  return success;
}

bool AidlDefinedType::CheckValidForGetterNames() const {
  bool success = true;
  std::set<std::string> getters;
  for (const auto& v : GetFields()) {
    bool duplicated = !getters.emplace(v->GetCapitalizedName()).second;
    if (duplicated) {
      AIDL_ERROR(v) << "'" << GetName() << "' has duplicate field name '" << v->GetName()
                    << "' after capitalizing the first letter";
      success = false;
    }
  }
  return success;
}

AidlParcelable::AidlParcelable(const AidlLocation& location, const std::string& name,
                               const std::string& package, const Comments& comments,
                               const std::string& cpp_header, std::vector<std::string>* type_params,
                               std::vector<std::unique_ptr<AidlMember>>* members)
    : AidlDefinedType(location, name, comments, package, members),
      AidlParameterizable<std::string>(type_params),
      cpp_header_(cpp_header) {
  // Strip off quotation marks if we actually have a cpp header.
  if (cpp_header_.length() >= 2) {
    cpp_header_ = cpp_header_.substr(1, cpp_header_.length() - 2);
  }
}
template <typename T>
AidlParameterizable<T>::AidlParameterizable(const AidlParameterizable& other) {
  // Copying is not supported if it has type parameters.
  // It doesn't make a problem because only ArrayBase() makes a copy,
  // and it can be called only if a type is not generic.
  AIDL_FATAL_IF(other.IsGeneric(), AIDL_LOCATION_HERE);
}

template <typename T>
bool AidlParameterizable<T>::CheckValid() const {
  return true;
};

template <>
bool AidlParameterizable<std::string>::CheckValid() const {
  if (!IsGeneric()) {
    return true;
  }
  std::unordered_set<std::string> set(GetTypeParameters().begin(), GetTypeParameters().end());
  if (set.size() != GetTypeParameters().size()) {
    AIDL_ERROR(this->AsAidlNode()) << "Every type parameter should be unique.";
    return false;
  }
  return true;
}

bool AidlParcelable::CheckValid(const AidlTypenames& typenames) const {
  if (!AidlDefinedType::CheckValid(typenames)) {
    return false;
  }
  if (!AidlParameterizable<std::string>::CheckValid()) {
    return false;
  }

  return true;
}

AidlStructuredParcelable::AidlStructuredParcelable(
    const AidlLocation& location, const std::string& name, const std::string& package,
    const Comments& comments, std::vector<std::string>* type_params,
    std::vector<std::unique_ptr<AidlMember>>* members)
    : AidlParcelable(location, name, package, comments, "" /*cpp_header*/, type_params, members) {}

bool AidlStructuredParcelable::CheckValid(const AidlTypenames& typenames) const {
  if (!AidlParcelable::CheckValid(typenames)) {
    return false;
  }

  bool success = true;

  if (IsFixedSize()) {
    for (const auto& v : GetFields()) {
      if (!typenames.CanBeFixedSize(v->GetType())) {
        AIDL_ERROR(v) << "The @FixedSize parcelable '" << this->GetName() << "' has a "
                      << "non-fixed size field named " << v->GetName() << ".";
        success = false;
      }
    }
  }

  if (IsJavaOnlyImmutable()) {
    // Immutable parcelables provide getters
    if (!CheckValidForGetterNames()) {
      success = false;
    }
  }

  return success;
}

// TODO: we should treat every backend all the same in future.
bool AidlTypeSpecifier::LanguageSpecificCheckValid(const AidlTypenames& typenames,
                                                   Options::Language lang) const {
  if (IsGeneric()) {
    const auto& types = GetTypeParameters();
    for (const auto& arg : types) {
      if (!arg->LanguageSpecificCheckValid(typenames, lang)) {
        return false;
      }
    }
  }

  if ((lang == Options::Language::NDK || lang == Options::Language::RUST) && IsArray() &&
      GetName() == "IBinder") {
    AIDL_ERROR(this) << "The " << to_string(lang) << " backend does not support array of IBinder";
    return false;
  }
  if (lang == Options::Language::RUST && GetName() == "ParcelableHolder") {
    // TODO(b/146611855): Remove it when Rust backend supports ParcelableHolder
    AIDL_ERROR(this) << "The Rust backend does not support ParcelableHolder yet.";
    return false;
  }
  if ((lang == Options::Language::NDK || lang == Options::Language::RUST) && IsArray() &&
      IsNullable()) {
    if (GetName() == "ParcelFileDescriptor") {
      AIDL_ERROR(this) << "The " << to_string(lang)
                       << " backend does not support nullable array of ParcelFileDescriptor";
      return false;
    }

    const auto defined_type = typenames.TryGetDefinedType(GetName());
    if (defined_type != nullptr && defined_type->AsParcelable() != nullptr) {
      AIDL_ERROR(this) << "The " << to_string(lang)
                       << " backend does not support nullable array of parcelable";
      return false;
    }
  }
  if (this->GetName() == "FileDescriptor" &&
      (lang == Options::Language::NDK || lang == Options::Language::RUST)) {
    AIDL_ERROR(this) << "FileDescriptor isn't supported by the " << to_string(lang) << " backend.";
    return false;
  }
  if (this->IsGeneric()) {
    if (this->GetName() == "List") {
      if (lang == Options::Language::NDK) {
        const AidlTypeSpecifier& contained_type = *GetTypeParameters()[0];
        const string& contained_type_name = contained_type.GetName();
        if (typenames.GetInterface(contained_type)) {
          AIDL_ERROR(this) << "List<" << contained_type_name
                           << "> is not supported. List in NDK doesn't support interface.";
          return false;
        }
        if (contained_type_name == "IBinder") {
          AIDL_ERROR(this) << "List<" << contained_type_name
                           << "> is not supported. List in NDK doesn't support IBinder.";
          return false;
        }
      }
    }
  }

  if (this->IsArray()) {
    if (this->GetName() == "List" || this->GetName() == "Map" ||
        this->GetName() == "CharSequence") {
      AIDL_ERROR(this) << this->GetName() << "[] is not supported.";
      return false;
    }
  }

  if (lang != Options::Language::JAVA) {
    if (this->GetName() == "List" && !this->IsGeneric()) {
      AIDL_ERROR(this) << "Currently, only the Java backend supports non-generic List.";
      return false;
    }
    if (this->GetName() == "Map" || this->GetName() == "CharSequence") {
      AIDL_ERROR(this) << "Currently, only Java backend supports " << this->GetName() << ".";
      return false;
    }
  }

  return true;
}

// TODO: we should treat every backend all the same in future.
bool AidlParcelable::LanguageSpecificCheckValid(const AidlTypenames& /*typenames*/,
                                                Options::Language lang) const {
  if (lang == Options::Language::CPP || lang == Options::Language::NDK) {
    const AidlParcelable* unstructured_parcelable = this->AsUnstructuredParcelable();
    if (unstructured_parcelable != nullptr) {
      if (unstructured_parcelable->GetCppHeader().empty()) {
        AIDL_ERROR(unstructured_parcelable)
            << "Unstructured parcelable must have C++ header defined.";
        return false;
      }
    }
  }
  return true;
}

// TODO: we should treat every backend all the same in future.
bool AidlStructuredParcelable::LanguageSpecificCheckValid(const AidlTypenames& typenames,
                                                          Options::Language lang) const {
  if (!AidlParcelable::LanguageSpecificCheckValid(typenames, lang)) {
    return false;
  }
  for (const auto& v : this->GetFields()) {
    if (!v->GetType().LanguageSpecificCheckValid(typenames, lang)) {
      return false;
    }
  }
  return true;
}

AidlEnumerator::AidlEnumerator(const AidlLocation& location, const std::string& name,
                               AidlConstantValue* value, const Comments& comments)
    : AidlCommentable(location, comments),
      name_(name),
      value_(value),
      value_user_specified_(value != nullptr) {}

bool AidlEnumerator::CheckValid(const AidlTypeSpecifier& enum_backing_type) const {
  if (GetValue() == nullptr) {
    return false;
  }
  if (!GetValue()->CheckValid()) {
    return false;
  }
  if (GetValue()->ValueString(enum_backing_type, AidlConstantValueDecorator).empty()) {
    AIDL_ERROR(this) << "Enumerator type differs from enum backing type.";
    return false;
  }
  return true;
}

string AidlEnumerator::ValueString(const AidlTypeSpecifier& backing_type,
                                   const ConstantValueDecorator& decorator) const {
  return GetValue()->ValueString(backing_type, decorator);
}

AidlEnumDeclaration::AidlEnumDeclaration(const AidlLocation& location, const std::string& name,
                                         std::vector<std::unique_ptr<AidlEnumerator>>* enumerators,
                                         const std::string& package, const Comments& comments)
    : AidlDefinedType(location, name, comments, package, nullptr),
      enumerators_(std::move(*enumerators)) {
  // Fill missing enumerator values with <prev + 1>
  // This can't be done in Autofill() because type/ref resolution depends on this.
  // For example, with enum E { A, B = A }, B's value 'A' is a reference which can't be
  // resolved if A has no value set.
  const AidlEnumerator* previous = nullptr;
  for (const auto& enumerator : enumerators_) {
    if (enumerator->GetValue() == nullptr) {
      auto loc = enumerator->GetLocation();
      if (previous == nullptr) {
        enumerator->SetValue(
            std::unique_ptr<AidlConstantValue>(AidlConstantValue::Integral(loc, "0")));
      } else {
        auto prev_value = std::make_unique<AidlConstantReference>(loc, previous->GetName());
        enumerator->SetValue(std::make_unique<AidlBinaryConstExpression>(
            loc, std::move(prev_value), "+",
            std::unique_ptr<AidlConstantValue>(AidlConstantValue::Integral(loc, "1"))));
      }
    }
    previous = enumerator.get();
  }
}

bool AidlEnumDeclaration::Autofill(const AidlTypenames& typenames) {
  if (auto annot = BackingType(); annot != nullptr) {
    // Autofill() is called before the grand CheckValid(). But AidlAnnotation::ParamValue()
    // calls AidlConstantValue::evaluate() which requires CheckValid() to be called before. So we
    // need to call CheckValid().
    if (!annot->CheckValid()) {
      return false;
    }
    auto type = annot->ParamValue<std::string>("type").value();
    backing_type_ =
        std::make_unique<AidlTypeSpecifier>(annot->GetLocation(), type, false, nullptr, Comments{});
  } else {
    // Default to byte type for enums.
    backing_type_ =
        std::make_unique<AidlTypeSpecifier>(AIDL_LOCATION_HERE, "byte", false, nullptr, Comments{});
  }
  // Autofill() is called after type resolution, we resolve the backing type manually.
  if (!backing_type_->Resolve(typenames)) {
    AIDL_ERROR(this) << "Invalid backing type: " << backing_type_->GetName();
  }
  return true;
}

bool AidlEnumDeclaration::CheckValid(const AidlTypenames& typenames) const {
  if (!AidlDefinedType::CheckValid(typenames)) {
    return false;
  }
  if (!GetMembers().empty()) {
    AIDL_ERROR(this) << "Enum doesn't support fields/constants/methods.";
    return false;
  }
  if (backing_type_ == nullptr) {
    AIDL_ERROR(this) << "Enum declaration missing backing type.";
    return false;
  }
  bool success = true;
  for (const auto& enumerator : enumerators_) {
    success = success && enumerator->CheckValid(GetBackingType());
  }

  return success;
}

AidlUnionDecl::AidlUnionDecl(const AidlLocation& location, const std::string& name,
                             const std::string& package, const Comments& comments,
                             std::vector<std::string>* type_params,
                             std::vector<std::unique_ptr<AidlMember>>* members)
    : AidlParcelable(location, name, package, comments, "" /*cpp_header*/, type_params, members) {}

bool AidlUnionDecl::CheckValid(const AidlTypenames& typenames) const {
  // visit parents
  if (!AidlParcelable::CheckValid(typenames)) {
    return false;
  }

  // unions provide getters always
  if (!CheckValidForGetterNames()) {
    return false;
  }

  // now, visit self!
  bool success = true;

  // TODO(b/170807936) do we need to allow ParcelableHolder in union?
  for (const auto& v : GetFields()) {
    if (v->GetType().GetName() == "ParcelableHolder") {
      AIDL_ERROR(*v) << "A union can't have a member of ParcelableHolder '" << v->GetName() << "'";
      success = false;
    }
  }

  if (GetFields().empty()) {
    AIDL_ERROR(*this) << "The union '" << this->GetName() << "' has no fields.";
    return false;
  }

  // first member should have useful default value (implicit or explicit)
  const auto& first = GetFields()[0];
  if (!first->HasUsefulDefaultValue()) {
    // Most types can be initialized without a default value. For example,
    // interface types are inherently nullable. But, enum types should have
    // an explicit default value.
    if (!first->GetType().IsArray() && typenames.GetEnumDeclaration(first->GetType())) {
      AIDL_ERROR(first)
          << "The union's first member should have a useful default value. Enum types can be "
             "initialized with a reference. (e.g. ... = MyEnum.FOO;)";
      return false;
    }
    // In Java, array types are initialized as null without a default value. To be sure that default
    // initialized unions are accepted by other backends we require arrays also have a default
    // value.
    if (first->GetType().IsArray()) {
      AIDL_ERROR(first)
          << "The union's first member should have a useful default value. Arrays can be "
             "initialized with values(e.g. ... = { values... };) or marked as @nullable.";
      return false;
    }
  }

  return success;
}

// TODO: we should treat every backend all the same in future.
bool AidlUnionDecl::LanguageSpecificCheckValid(const AidlTypenames& typenames,
                                               Options::Language lang) const {
  if (!AidlParcelable::LanguageSpecificCheckValid(typenames, lang)) {
    return false;
  }
  for (const auto& v : this->GetFields()) {
    if (!v->GetType().LanguageSpecificCheckValid(typenames, lang)) {
      return false;
    }
  }
  return true;
}

// TODO: we should treat every backend all the same in future.
bool AidlInterface::LanguageSpecificCheckValid(const AidlTypenames& typenames,
                                               Options::Language lang) const {
  for (const auto& m : this->GetMethods()) {
    if (!m->GetType().LanguageSpecificCheckValid(typenames, lang)) {
      return false;
    }
    for (const auto& arg : m->GetArguments()) {
      if (!arg->GetType().LanguageSpecificCheckValid(typenames, lang)) {
        return false;
      }
    }
  }
  return true;
}

AidlInterface::AidlInterface(const AidlLocation& location, const std::string& name,
                             const Comments& comments, bool oneway, const std::string& package,
                             std::vector<std::unique_ptr<AidlMember>>* members)
    : AidlDefinedType(location, name, comments, package, members) {
  for (auto& m : GetMethods()) {
    m.get()->ApplyInterfaceOneway(oneway);
  }
}

bool AidlInterface::CheckValid(const AidlTypenames& typenames) const {
  if (!AidlDefinedType::CheckValid(typenames)) {
    return false;
  }
  // Has to be a pointer due to deleting copy constructor. No idea why.
  map<string, const AidlMethod*> method_names;
  for (const auto& m : GetMethods()) {
    if (!m->GetType().CheckValid(typenames)) {
      return false;
    }

    // TODO(b/156872582): Support it when ParcelableHolder supports every backend.
    if (m->GetType().GetName() == "ParcelableHolder") {
      AIDL_ERROR(m) << "ParcelableHolder cannot be a return type";
      return false;
    }
    if (m->IsOneway() && m->GetType().GetName() != "void") {
      AIDL_ERROR(m) << "oneway method '" << m->GetName() << "' cannot return a value";
      return false;
    }

    set<string> argument_names;
    for (const auto& arg : m->GetArguments()) {
      auto it = argument_names.find(arg->GetName());
      if (it != argument_names.end()) {
        AIDL_ERROR(m) << "method '" << m->GetName() << "' has duplicate argument name '"
                      << arg->GetName() << "'";
        return false;
      }
      argument_names.insert(arg->GetName());

      if (!arg->CheckValid(typenames)) {
        return false;
      }

      if (m->IsOneway() && arg->IsOut()) {
        AIDL_ERROR(m) << "oneway method '" << m->GetName() << "' cannot have out parameters";
        return false;
      }

      // check that the name doesn't match a keyword
      if (IsJavaKeyword(arg->GetName().c_str())) {
        AIDL_ERROR(arg) << "Argument name is a Java or aidl keyword";
        return false;
      }

      // Reserve a namespace for internal use
      if (android::base::StartsWith(arg->GetName(), "_aidl")) {
        AIDL_ERROR(arg) << "Argument name cannot begin with '_aidl'";
        return false;
      }
    }

    auto it = method_names.find(m->GetName());
    // prevent duplicate methods
    if (it == method_names.end()) {
      method_names[m->GetName()] = m.get();
    } else {
      AIDL_ERROR(m) << "attempt to redefine method " << m->GetName() << ":";
      AIDL_ERROR(it->second) << "previously defined here.";
      return false;
    }

    static set<string> reserved_methods{"asBinder()", "getInterfaceHash()", "getInterfaceVersion()",
                                        "getTransactionName(int)"};

    if (reserved_methods.find(m->Signature()) != reserved_methods.end()) {
      AIDL_ERROR(m) << " method " << m->Signature() << " is reserved for internal use.";
      return false;
    }
  }

  bool success = true;
  set<string> constant_names;
  for (const auto& constant : GetConstantDeclarations()) {
    if (constant_names.count(constant->GetName()) > 0) {
      AIDL_ERROR(constant) << "Found duplicate constant name '" << constant->GetName() << "'";
      success = false;
    }
    constant_names.insert(constant->GetName());
    success = success && constant->CheckValid(typenames);
  }
  return success;
}

std::string AidlInterface::GetDescriptor() const {
  std::string annotatedDescriptor = AidlAnnotatable::GetDescriptor();
  if (annotatedDescriptor != "") {
    return annotatedDescriptor;
  }
  return GetCanonicalName();
}

AidlImport::AidlImport(const AidlLocation& location, const std::string& needed_class,
                       const Comments& comments)
    : AidlNode(location, comments), needed_class_(needed_class) {}

// Resolves unresolved type name to fully qualified typename to import
// case #1: SimpleName --> import p.SimpleName
// case #2: Outer.Inner --> import p.Outer
// case #3: p.SimpleName --> (as is)
std::optional<std::string> AidlDocument::ResolveName(const std::string& unresolved_name) const {
  std::string canonical_name;
  const auto first_dot = unresolved_name.find_first_of('.');
  const std::string class_name =
      (first_dot == std::string::npos) ? unresolved_name : unresolved_name.substr(0, first_dot);
  for (const auto& import : Imports()) {
    const auto& fq_name = import->GetNeededClass();
    const auto last_dot = fq_name.find_last_of('.');
    const std::string imported_type_name =
        (last_dot == std::string::npos) ? fq_name : fq_name.substr(last_dot + 1);
    if (imported_type_name == class_name) {
      if (canonical_name != "" && canonical_name != fq_name) {
        AIDL_ERROR(import) << "Ambiguous type: " << canonical_name << " vs. " << fq_name;
        return {};
      }
      canonical_name = fq_name;
    }
  }
  // if not found, use unresolved_name as it is
  if (canonical_name == "") {
    return unresolved_name;
  }
  return canonical_name;
}

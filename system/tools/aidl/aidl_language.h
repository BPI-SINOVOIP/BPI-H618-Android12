/*
 * Copyright (C) 2019, The Android Open Source Project
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

#pragma once

#include <memory>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

#include <android-base/strings.h>

#include "aidl_typenames.h"
#include "code_writer.h"
#include "comments.h"
#include "diagnostics.h"
#include "io_delegate.h"
#include "location.h"
#include "logging.h"
#include "options.h"

using android::aidl::AidlTypenames;
using android::aidl::CodeWriter;
using android::aidl::Comments;
using android::aidl::Options;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;
class AidlNode;

// helper to see if T is the same to one of Args types.
template <typename T, typename... Args>
struct is_one_of : std::false_type {};

template <typename T, typename S, typename... Args>
struct is_one_of<T, S, Args...> {
  enum { value = std::is_same_v<T, S> || is_one_of<T, Args...>::value };
};

// helper to see if T is std::vector of something.
template <typename>
struct is_vector : std::false_type {};

template <typename T>
struct is_vector<std::vector<T>> : std::true_type {};

// helper for static_assert(false)
template <typename T>
struct unsupported_type : std::false_type {};

namespace android {
namespace aidl {
namespace mappings {
std::string dump_location(const AidlNode& method);
}  // namespace mappings
namespace java {
std::string dump_location(const AidlNode& method);
}  // namespace java
}  // namespace aidl
}  // namespace android

bool ParseFloating(std::string_view sv, double* parsed);
bool ParseFloating(std::string_view sv, float* parsed);

class AidlDocument;
class AidlPackage;
class AidlImport;
class AidlInterface;
class AidlParcelable;
class AidlStructuredParcelable;
class AidlEnumDeclaration;
class AidlUnionDecl;
class AidlVariableDeclaration;
class AidlConstantDeclaration;
class AidlEnumerator;
class AidlMethod;
class AidlArgument;
class AidlConstantValue;
class AidlConstantReference;
class AidlUnaryConstExpression;
class AidlBinaryConstExpression;
class AidlAnnotation;

// Interface for visitors that can traverse AidlTraversable nodes.
class AidlVisitor {
 public:
  virtual ~AidlVisitor() = default;
  virtual void Visit(const AidlDocument&) {}
  virtual void Visit(const AidlInterface&) {}
  virtual void Visit(const AidlParcelable&) {}
  virtual void Visit(const AidlStructuredParcelable&) {}
  virtual void Visit(const AidlUnionDecl&) {}
  virtual void Visit(const AidlEnumDeclaration&) {}
  virtual void Visit(const AidlEnumerator&) {}
  virtual void Visit(const AidlMethod&) {}
  virtual void Visit(const AidlVariableDeclaration&) {}
  virtual void Visit(const AidlConstantDeclaration&) {}
  virtual void Visit(const AidlArgument&) {}
  virtual void Visit(const AidlTypeSpecifier&) {}
  virtual void Visit(const AidlConstantValue&) {}
  virtual void Visit(const AidlConstantReference&) {}
  virtual void Visit(const AidlUnaryConstExpression&) {}
  virtual void Visit(const AidlBinaryConstExpression&) {}
  virtual void Visit(const AidlAnnotation&) {}
  virtual void Visit(const AidlImport&) {}
  virtual void Visit(const AidlPackage&) {}
};

// Anything that is locatable in a .aidl file.
class AidlNode {
 public:
  AidlNode(const AidlLocation& location, const Comments& comments = {});

  AidlNode(const AidlNode&) = default;
  virtual ~AidlNode() = default;

  AidlNode(AidlNode&&) = delete;
  AidlNode& operator=(AidlNode&&) = delete;

  // To be able to print AidlLocation
  friend class AidlErrorLog;
  friend std::string android::aidl::mappings::dump_location(const AidlNode&);
  friend std::string android::aidl::java::dump_location(const AidlNode&);

  const AidlLocation& GetLocation() const { return location_; }
  virtual void TraverseChildren(std::function<void(const AidlNode&)> traverse) const = 0;
  virtual void DispatchVisit(AidlVisitor&) const = 0;

  const Comments& GetComments() const { return comments_; }
  void SetComments(const Comments& comments) { comments_ = comments; }

 private:
  std::string PrintLine() const;
  std::string PrintLocation() const;
  const AidlLocation location_;
  Comments comments_;
};

// unique_ptr<AidlTypeSpecifier> for type arugment,
// std::string for type parameter(T, U, and so on).
template <typename T>
class AidlParameterizable {
 public:
  AidlParameterizable(std::vector<T>* type_params) : type_params_(type_params) {}
  virtual ~AidlParameterizable() = default;
  bool IsGeneric() const { return type_params_ != nullptr; }
  const std::vector<T>& GetTypeParameters() const { return *type_params_; }
  bool CheckValid() const;

  __attribute__((warn_unused_result)) bool SetTypeParameters(std::vector<T>* type_params) {
    if (type_params_) return false;
    type_params_.reset(type_params);
    return true;
  }

  virtual const AidlNode& AsAidlNode() const = 0;

 protected:
  AidlParameterizable(const AidlParameterizable&);

 private:
  unique_ptr<std::vector<T>> type_params_;
  static_assert(std::is_same<T, unique_ptr<AidlTypeSpecifier>>::value ||
                std::is_same<T, std::string>::value);
};
template <>
bool AidlParameterizable<std::string>::CheckValid() const;

class AidlCommentable : public AidlNode {
 public:
  AidlCommentable(const AidlLocation& location, const Comments& comments)
      : AidlNode(location, comments) {}
  virtual ~AidlCommentable() = default;

  bool IsHidden() const;
  bool IsDeprecated() const;
};

// Transforms a value string into a language specific form. Raw value as produced by
// AidlConstantValue.
using ConstantValueDecorator =
    std::function<std::string(const AidlTypeSpecifier& type, const std::string& raw_value)>;

class AidlAnnotation : public AidlNode {
 public:
  enum class Type {
    BACKING = 1,
    HIDE,
    JAVA_STABLE_PARCELABLE,
    UNSUPPORTED_APP_USAGE,
    VINTF_STABILITY,
    NULLABLE,
    UTF8_IN_CPP,
    SENSITIVE_DATA,
    JAVA_PASSTHROUGH,
    JAVA_DERIVE,
    JAVA_ONLY_IMMUTABLE,
    FIXED_SIZE,
    DESCRIPTOR,
    RUST_DERIVE,
    SUPPRESS_WARNINGS,
  };

  using TargetContext = uint16_t;
  static constexpr TargetContext CONTEXT_TYPE_INTERFACE = 0x1 << 0;
  static constexpr TargetContext CONTEXT_TYPE_ENUM = 0x1 << 1;
  static constexpr TargetContext CONTEXT_TYPE_STRUCTURED_PARCELABLE = 0x1 << 2;
  static constexpr TargetContext CONTEXT_TYPE_UNION = 0x1 << 3;
  static constexpr TargetContext CONTEXT_TYPE_UNSTRUCTURED_PARCELABLE = 0x1 << 4;
  static constexpr TargetContext CONTEXT_TYPE =
      CONTEXT_TYPE_INTERFACE | CONTEXT_TYPE_ENUM | CONTEXT_TYPE_STRUCTURED_PARCELABLE |
      CONTEXT_TYPE_UNION | CONTEXT_TYPE_UNSTRUCTURED_PARCELABLE;
  static constexpr TargetContext CONTEXT_CONST = 0x1 << 5;
  static constexpr TargetContext CONTEXT_FIELD = 0x1 << 6;
  static constexpr TargetContext CONTEXT_METHOD = 0x1 << 7;
  static constexpr TargetContext CONTEXT_MEMBER = CONTEXT_CONST | CONTEXT_FIELD | CONTEXT_METHOD;
  static constexpr TargetContext CONTEXT_TYPE_SPECIFIER = 0x1 << 8;
  static constexpr TargetContext CONTEXT_ALL =
      CONTEXT_TYPE | CONTEXT_MEMBER | CONTEXT_TYPE_SPECIFIER;

  static std::string TypeToString(Type type);

  static AidlAnnotation* Parse(
      const AidlLocation& location, const string& name,
      std::map<std::string, std::shared_ptr<AidlConstantValue>>* parameter_list,
      const Comments& comments);

  AidlAnnotation(const AidlAnnotation&) = default;
  AidlAnnotation(AidlAnnotation&&) = default;
  virtual ~AidlAnnotation() = default;
  bool CheckValid() const;
  bool CheckContext(TargetContext context) const;
  const string& GetName() const { return schema_.name; }
  const Type& GetType() const { return schema_.type; }
  bool Repeatable() const { return schema_.repeatable; }

  // ToString is for dumping AIDL.
  // Returns string representation of this annotation.
  // e.g) "@RustDerive(Clone=true, Copy=true)"
  string ToString() const;

  template <typename T>
  std::optional<T> ParamValue(const std::string& param_name) const;

  std::map<std::string, std::string> AnnotationParams(
      const ConstantValueDecorator& decorator) const;
  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override;
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  struct ParamType {
    std::string name;
    const AidlTypeSpecifier& type;
    bool required = false;
  };

  struct Schema {
    AidlAnnotation::Type type;
    std::string name;
    TargetContext target_context;
    std::vector<ParamType> parameters;
    bool repeatable = false;

    const ParamType* ParamType(const std::string& name) const {
      for (const auto& param : parameters) {
        if (param.name == name) {
          return &param;
        }
      }
      return nullptr;
    }
  };

  static const std::vector<Schema>& AllSchemas();

  AidlAnnotation(const AidlLocation& location, const Schema& schema,
                 std::map<std::string, std::shared_ptr<AidlConstantValue>>&& parameters,
                 const Comments& comments);

  const Schema& schema_;
  std::map<std::string, std::shared_ptr<AidlConstantValue>> parameters_;
};

static inline bool operator<(const AidlAnnotation& lhs, const AidlAnnotation& rhs) {
  return lhs.GetName() < rhs.GetName();
}
static inline bool operator==(const AidlAnnotation& lhs, const AidlAnnotation& rhs) {
  return lhs.GetName() == rhs.GetName();
}

class AidlAnnotatable : public AidlCommentable {
 public:
  AidlAnnotatable(const AidlLocation& location, const Comments& comments);

  AidlAnnotatable(const AidlAnnotatable&) = default;
  AidlAnnotatable(AidlAnnotatable&&) = default;
  virtual ~AidlAnnotatable() = default;

  void Annotate(vector<AidlAnnotation>&& annotations) {
    for (auto& annotation : annotations) {
      annotations_.emplace_back(std::move(annotation));
    }
  }
  bool IsNullable() const;
  bool IsUtf8InCpp() const;
  bool IsSensitiveData() const;
  bool IsVintfStability() const;
  bool IsJavaOnlyImmutable() const;
  bool IsFixedSize() const;
  bool IsStableApiParcelable(Options::Language lang) const;
  bool IsHide() const;
  bool JavaDerive(const std::string& method) const;
  std::string GetDescriptor() const;

  const AidlAnnotation* UnsupportedAppUsage() const;
  const AidlAnnotation* RustDerive() const;
  const AidlAnnotation* BackingType() const;
  std::vector<std::string> SuppressWarnings() const;

  // ToString is for dumping AIDL.
  // Returns string representation of annotations.
  // e.g) "@JavaDerive(toString=true) @RustDerive(Clone=true, Copy=true)"
  std::string ToString() const;

  const vector<AidlAnnotation>& GetAnnotations() const { return annotations_; }
  bool CheckValid(const AidlTypenames&) const;
  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    for (const auto& annot : GetAnnotations()) {
      traverse(annot);
    }
  }

 private:
  vector<AidlAnnotation> annotations_;
};

// AidlTypeSpecifier represents a reference to either a built-in type,
// a defined type, or a variant (e.g., array of generic) of a type.
class AidlTypeSpecifier final : public AidlAnnotatable,
                                public AidlParameterizable<unique_ptr<AidlTypeSpecifier>> {
 public:
  AidlTypeSpecifier(const AidlLocation& location, const string& unresolved_name, bool is_array,
                    vector<unique_ptr<AidlTypeSpecifier>>* type_params, const Comments& comments);
  virtual ~AidlTypeSpecifier() = default;

  // Copy of this type which is not an array.
  const AidlTypeSpecifier& ArrayBase() const;

  // Returns the full-qualified name of the base type.
  // int -> int
  // int[] -> int
  // List<String> -> List
  // IFoo -> foo.bar.IFoo (if IFoo is in package foo.bar)
  const string& GetName() const {
    if (IsResolved()) {
      return fully_qualified_name_;
    } else {
      return GetUnresolvedName();
    }
  }

  // ToString is for dumping AIDL.
  // Returns string representation of this type specifier including annotations.
  // This is "annotations type_name type_params? array_marker?".
  // e.g) "@utf8InCpp String[]";
  std::string ToString() const;

  // Signature is for comparing AIDL types.
  // Returns string representation of this type specifier.
  // This is "type_name type_params? array_marker?".
  // e.g.) "String[]" (even if it is annotated with @utf8InCpp)
  std::string Signature() const;

  const string& GetUnresolvedName() const { return unresolved_name_; }

  const std::vector<std::string> GetSplitName() const { return split_name_; }

  bool IsResolved() const { return fully_qualified_name_ != ""; }

  bool IsArray() const { return is_array_; }

  __attribute__((warn_unused_result)) bool SetArray() {
    if (is_array_) return false;
    is_array_ = true;
    return true;
  }

  // Resolve the base type name to a fully-qualified name. Return false if the
  // resolution fails.
  bool Resolve(const AidlTypenames& typenames);

  bool CheckValid(const AidlTypenames& typenames) const;
  bool LanguageSpecificCheckValid(const AidlTypenames& typenames, Options::Language lang) const;
  const AidlNode& AsAidlNode() const override { return *this; }

  const AidlDefinedType* GetDefinedType() const;
  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    AidlAnnotatable::TraverseChildren(traverse);
    if (IsGeneric()) {
      for (const auto& tp : GetTypeParameters()) {
        traverse(*tp);
      }
    }
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  AidlTypeSpecifier(const AidlTypeSpecifier&) = default;

  const string unresolved_name_;
  string fully_qualified_name_;
  bool is_array_;
  vector<string> split_name_;
  const AidlDefinedType* defined_type_ = nullptr;  // set when Resolve() for defined types
  mutable shared_ptr<AidlTypeSpecifier> array_base_;
};

// Returns the universal value unaltered.
std::string AidlConstantValueDecorator(const AidlTypeSpecifier& type, const std::string& raw_value);

class AidlMember : public AidlCommentable {
 public:
  AidlMember(const AidlLocation& location, const Comments& comments);
  virtual ~AidlMember() = default;

  // non-copyable, non-movable
  AidlMember(const AidlMember&) = delete;
  AidlMember(AidlMember&&) = delete;
  AidlMember& operator=(const AidlMember&) = delete;
  AidlMember& operator=(AidlMember&&) = delete;

  virtual const AidlMethod* AsMethod() const { return nullptr; }
  virtual const AidlConstantDeclaration* AsConstantDeclaration() const { return nullptr; }
  virtual const AidlVariableDeclaration* AsVariableDeclaration() const { return nullptr; }

  AidlMethod* AsMethod() {
    return const_cast<AidlMethod*>(const_cast<const AidlMember*>(this)->AsMethod());
  }
  AidlConstantDeclaration* AsConstantDeclaration() {
    return const_cast<AidlConstantDeclaration*>(
        const_cast<const AidlMember*>(this)->AsConstantDeclaration());
  }
  AidlVariableDeclaration* AsVariableDeclaration() {
    return const_cast<AidlVariableDeclaration*>(
        const_cast<const AidlMember*>(this)->AsVariableDeclaration());
  }
};

// TODO: This class is used for method arguments and also parcelable fields,
// and it should be split up since default values don't apply to method
// arguments
class AidlVariableDeclaration : public AidlMember {
 public:
  AidlVariableDeclaration(const AidlLocation& location, AidlTypeSpecifier* type,
                          const std::string& name);
  AidlVariableDeclaration(const AidlLocation& location, AidlTypeSpecifier* type,
                          const std::string& name, AidlConstantValue* default_value);
  virtual ~AidlVariableDeclaration() = default;

  // non-copyable, non-movable
  AidlVariableDeclaration(const AidlVariableDeclaration&) = delete;
  AidlVariableDeclaration(AidlVariableDeclaration&&) = delete;
  AidlVariableDeclaration& operator=(const AidlVariableDeclaration&) = delete;
  AidlVariableDeclaration& operator=(AidlVariableDeclaration&&) = delete;

  const AidlVariableDeclaration* AsVariableDeclaration() const override { return this; }

  std::string GetName() const { return name_; }
  std::string GetCapitalizedName() const;
  const AidlTypeSpecifier& GetType() const { return *type_; }
  // if this was constructed explicitly with a default value
  bool IsDefaultUserSpecified() const { return default_user_specified_; }
  // will return the default value this is constructed with or a default value
  // if one is available
  const AidlConstantValue* GetDefaultValue() const { return default_value_.get(); }
  bool HasUsefulDefaultValue() const;

  AidlTypeSpecifier* GetMutableType() { return type_.get(); }

  bool CheckValid(const AidlTypenames& typenames) const;

  // ToString is for dumping AIDL.
  // Returns string representation of this variable decl including default value.
  // This is "annotations type name default_value?".
  // e.g) "@utf8InCpp String[] names = {"hello"}"
  std::string ToString() const;

  // Signature is for comparing AIDL types.
  // Returns string representation of this variable decl.
  // This is "type name".
  // e.g) "String[] name" (even if it is annotated with @utf8InCpp and has a default value.)
  std::string Signature() const;

  std::string ValueString(const ConstantValueDecorator& decorator) const;

  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override;
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  std::unique_ptr<AidlTypeSpecifier> type_;
  std::string name_;
  bool default_user_specified_;
  std::unique_ptr<AidlConstantValue> default_value_;
};

class AidlArgument : public AidlVariableDeclaration {
 public:
  enum Direction { IN_DIR = 1, OUT_DIR = 2, INOUT_DIR = 3 };

  AidlArgument(const AidlLocation& location, AidlArgument::Direction direction,
               AidlTypeSpecifier* type, const std::string& name);
  AidlArgument(const AidlLocation& location, AidlTypeSpecifier* type, const std::string& name);
  virtual ~AidlArgument() = default;

  // non-copyable, non-movable
  AidlArgument(const AidlArgument&) = delete;
  AidlArgument(AidlArgument&&) = delete;
  AidlArgument& operator=(const AidlArgument&) = delete;
  AidlArgument& operator=(AidlArgument&&) = delete;

  Direction GetDirection() const { return direction_; }
  bool IsOut() const { return direction_ & OUT_DIR; }
  bool IsIn() const { return direction_ & IN_DIR; }
  bool DirectionWasSpecified() const { return direction_specified_; }
  string GetDirectionSpecifier() const;
  bool CheckValid(const AidlTypenames& typenames) const;

  // ToString is for dumping AIDL.
  // Returns string representation of this argument including direction
  // This is "direction annotations type name".
  // e.g) "in @utf8InCpp String[] names"
  std::string ToString() const;

  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    traverse(GetType());
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  Direction direction_;
  bool direction_specified_;
};

struct ArgumentAspect {
  std::string name;
  std::set<AidlArgument::Direction> possible_directions;
};

class AidlUnaryConstExpression;
class AidlBinaryConstExpression;
class AidlConstantReference;

class AidlConstantValue : public AidlNode {
 public:
  enum class Type {
    // WARNING: Don't change this order! The order is used to determine type
    // promotion during a binary expression.
    BOOLEAN,
    INT8,
    INT32,
    INT64,
    ARRAY,
    CHARACTER,
    STRING,
    REF,
    FLOATING,
    UNARY,
    BINARY,
    ERROR,
  };

  // Returns the evaluated value. T> should match to the actual type.
  template <typename T>
  T EvaluatedValue() const {
    is_evaluated_ || (CheckValid() && evaluate());
    AIDL_FATAL_IF(!is_valid_, this);

    if constexpr (is_vector<T>::value) {
      AIDL_FATAL_IF(final_type_ != Type::ARRAY, this);
      T result;
      for (const auto& v : values_) {
        result.push_back(v->EvaluatedValue<typename T::value_type>());
      }
      return result;
    } else if constexpr (is_one_of<T, float, double>::value) {
      AIDL_FATAL_IF(final_type_ != Type::FLOATING, this);
      T result;
      AIDL_FATAL_IF(!ParseFloating(value_, &result), this);
      return result;
    } else if constexpr (std::is_same<T, std::string>::value) {
      AIDL_FATAL_IF(final_type_ != Type::STRING, this);
      return final_string_value_.substr(1, final_string_value_.size() - 2);  // unquote "
    } else if constexpr (is_one_of<T, int8_t, int32_t, int64_t>::value) {
      AIDL_FATAL_IF(final_type_ < Type::INT8 && final_type_ > Type::INT64, this);
      return static_cast<T>(final_value_);
    } else if constexpr (std::is_same<T, char16_t>::value) {
      AIDL_FATAL_IF(final_type_ != Type::CHARACTER, this);
      return final_string_value_.at(1);  // unquote '
    } else if constexpr (std::is_same<T, bool>::value) {
      static_assert(std::is_same<T, bool>::value, "..");
      AIDL_FATAL_IF(final_type_ != Type::BOOLEAN, this);
      return final_value_ != 0;
    } else {
      static_assert(unsupported_type<T>::value);
    }
  }

  virtual ~AidlConstantValue() = default;

  // non-copyable, non-movable
  AidlConstantValue(const AidlConstantValue&) = delete;
  AidlConstantValue(AidlConstantValue&&) = delete;
  AidlConstantValue& operator=(const AidlConstantValue&) = delete;
  AidlConstantValue& operator=(AidlConstantValue&&) = delete;

  // creates default value, when one isn't specified
  // nullptr if no default available
  static AidlConstantValue* Default(const AidlTypeSpecifier& specifier);

  static AidlConstantValue* Boolean(const AidlLocation& location, bool value);
  static AidlConstantValue* Character(const AidlLocation& location, const std::string& value);
  // example: 123, -5498, maybe any size
  static AidlConstantValue* Integral(const AidlLocation& location, const std::string& value);
  static AidlConstantValue* Floating(const AidlLocation& location, const std::string& value);
  static AidlConstantValue* Array(const AidlLocation& location,
                                  std::unique_ptr<vector<unique_ptr<AidlConstantValue>>> values);
  // example: "\"asdf\""
  static AidlConstantValue* String(const AidlLocation& location, const string& value);

  Type GetType() const { return final_type_; }
  const std::string& Literal() const { return value_; }

  virtual bool CheckValid() const;

  // Raw value of type (currently valid in C++ and Java). Empty string on error.
  string ValueString(const AidlTypeSpecifier& type, const ConstantValueDecorator& decorator) const;

  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const {
    if (type_ == Type::ARRAY) {
      for (const auto& v : values_) {
        traverse(*v);
      }
    }
  }
  void DispatchVisit(AidlVisitor& visitor) const override { visitor.Visit(*this); }

 private:
  AidlConstantValue(const AidlLocation& location, Type parsed_type, int64_t parsed_value,
                    const string& checked_value);
  AidlConstantValue(const AidlLocation& location, Type type, const string& checked_value);
  AidlConstantValue(const AidlLocation& location, Type type,
                    std::unique_ptr<vector<unique_ptr<AidlConstantValue>>> values,
                    const std::string& value);
  static string ToString(Type type);
  static bool ParseIntegral(const string& value, int64_t* parsed_value, Type* parsed_type);
  static bool IsHex(const string& value);

  virtual bool evaluate() const;

  const Type type_ = Type::ERROR;
  const vector<unique_ptr<AidlConstantValue>> values_;  // if type_ == ARRAY
  const string value_;                                  // otherwise

  // State for tracking evaluation of expressions
  mutable bool is_valid_ = false;      // cache of CheckValid, but may be marked false in evaluate
  mutable bool is_evaluated_ = false;  // whether evaluate has been called
  mutable Type final_type_;
  mutable int64_t final_value_;
  mutable string final_string_value_ = "";

  friend AidlUnaryConstExpression;
  friend AidlBinaryConstExpression;
  friend AidlConstantReference;
};

// Represents "<type>.<field>" which resolves to a constant which is one of
// - constant declartion
// - enumerator
// When a <type> is missing, <field> is of the enclosing type.
class AidlConstantReference : public AidlConstantValue {
 public:
  AidlConstantReference(const AidlLocation& location, const std::string& value);

  const std::unique_ptr<AidlTypeSpecifier>& GetRefType() const { return ref_type_; }
  const std::string& GetFieldName() const { return field_name_; }

  bool CheckValid() const override;
  void TraverseChildren(std::function<void(const AidlNode&)>) const override {
    // resolved_ is not my child.
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }
  const AidlConstantValue* Resolve(const AidlDefinedType* scope) const;

 private:
  bool evaluate() const override;

  std::unique_ptr<AidlTypeSpecifier> ref_type_;
  std::string field_name_;
  mutable const AidlConstantValue* resolved_ = nullptr;
};

class AidlUnaryConstExpression : public AidlConstantValue {
 public:
  AidlUnaryConstExpression(const AidlLocation& location, const string& op,
                           std::unique_ptr<AidlConstantValue> rval);

  static bool IsCompatibleType(Type type, const string& op);
  bool CheckValid() const override;
  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    traverse(*unary_);
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  bool evaluate() const override;

  std::unique_ptr<AidlConstantValue> unary_;
  const string op_;
};

class AidlBinaryConstExpression : public AidlConstantValue {
 public:
  AidlBinaryConstExpression(const AidlLocation& location, std::unique_ptr<AidlConstantValue> lval,
                            const string& op, std::unique_ptr<AidlConstantValue> rval);

  bool CheckValid() const override;

  static bool AreCompatibleTypes(Type t1, Type t2);
  // Returns the promoted kind for both operands
  static Type UsualArithmeticConversion(Type left, Type right);
  // Returns the promoted integral type where INT32 is the smallest type
  static Type IntegralPromotion(Type in);
  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    traverse(*left_val_);
    traverse(*right_val_);
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  bool evaluate() const override;

  std::unique_ptr<AidlConstantValue> left_val_;
  std::unique_ptr<AidlConstantValue> right_val_;
  const string op_;
};

struct AidlAnnotationParameter {
  std::string name;
  std::unique_ptr<AidlConstantValue> value;
};

class AidlConstantDeclaration : public AidlMember {
 public:
  AidlConstantDeclaration(const AidlLocation& location, AidlTypeSpecifier* specifier,
                          const string& name, AidlConstantValue* value);
  virtual ~AidlConstantDeclaration() = default;

  // non-copyable, non-movable
  AidlConstantDeclaration(const AidlConstantDeclaration&) = delete;
  AidlConstantDeclaration(AidlConstantDeclaration&&) = delete;
  AidlConstantDeclaration& operator=(const AidlConstantDeclaration&) = delete;
  AidlConstantDeclaration& operator=(AidlConstantDeclaration&&) = delete;

  const AidlTypeSpecifier& GetType() const { return *type_; }
  AidlTypeSpecifier* GetMutableType() { return type_.get(); }
  const string& GetName() const { return name_; }
  const AidlConstantValue& GetValue() const { return *value_; }
  bool CheckValid(const AidlTypenames& typenames) const;

  // ToString is for dumping AIDL.
  // Returns string representation of this const decl including a const value.
  // This is "`const` annotations type name value".
  // e.g) "const @utf8InCpp String[] names = { "hello" }"
  string ToString() const;

  // Signature is for comparing types.
  // Returns string representation of this const decl.
  // This is "direction annotations type name".
  // e.g) "String[] names"
  string Signature() const;

  string ValueString(const ConstantValueDecorator& decorator) const {
    return value_->ValueString(GetType(), decorator);
  }

  const AidlConstantDeclaration* AsConstantDeclaration() const override { return this; }

  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    traverse(GetType());
    traverse(GetValue());
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  const unique_ptr<AidlTypeSpecifier> type_;
  const string name_;
  unique_ptr<AidlConstantValue> value_;
};

class AidlMethod : public AidlMember {
 public:
  AidlMethod(const AidlLocation& location, bool oneway, AidlTypeSpecifier* type, const string& name,
             vector<unique_ptr<AidlArgument>>* args, const Comments& comments);
  AidlMethod(const AidlLocation& location, bool oneway, AidlTypeSpecifier* type, const string& name,
             vector<unique_ptr<AidlArgument>>* args, const Comments& comments, int id,
             bool is_user_defined = true);
  virtual ~AidlMethod() = default;

  // non-copyable, non-movable
  AidlMethod(const AidlMethod&) = delete;
  AidlMethod(AidlMethod&&) = delete;
  AidlMethod& operator=(const AidlMethod&) = delete;
  AidlMethod& operator=(AidlMethod&&) = delete;

  const AidlMethod* AsMethod() const override { return this; }
  const AidlTypeSpecifier& GetType() const { return *type_; }
  AidlTypeSpecifier* GetMutableType() { return type_.get(); }

  // set if this method is part of an interface that is marked oneway
  void ApplyInterfaceOneway(bool oneway) { oneway_ = oneway_ || oneway; }
  bool IsOneway() const { return oneway_; }

  const std::string& GetName() const { return name_; }
  bool HasId() const { return has_id_; }
  int GetId() const { return id_; }
  void SetId(unsigned id) { id_ = id; }

  bool IsUserDefined() const { return is_user_defined_; }

  const std::vector<std::unique_ptr<AidlArgument>>& GetArguments() const {
    return arguments_;
  }
  // An inout parameter will appear in both GetInArguments()
  // and GetOutArguments().  AidlMethod retains ownership of the argument
  // pointers returned in this way.
  const std::vector<const AidlArgument*>& GetInArguments() const {
    return in_arguments_;
  }
  const std::vector<const AidlArgument*>& GetOutArguments() const {
    return out_arguments_;
  }

  // ToString is for dumping AIDL.
  // Returns string representation of this method including everything.
  // This is "ret_type name ( arg_list ) = id".
  // e.g) "boolean foo(int, @Nullable String) = 1"
  std::string ToString() const;

  // Signature is for comparing AIDL types.
  // Returns string representation of this method's name & type.
  // e.g) "foo(int, String)"
  std::string Signature() const;

  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    traverse(GetType());
    for (const auto& a : GetArguments()) {
      traverse(*a);
    }
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  bool oneway_;
  std::unique_ptr<AidlTypeSpecifier> type_;
  std::string name_;
  const std::vector<std::unique_ptr<AidlArgument>> arguments_;
  std::vector<const AidlArgument*> in_arguments_;
  std::vector<const AidlArgument*> out_arguments_;
  bool has_id_;
  int id_;
  bool is_user_defined_ = true;
};

// AidlDefinedType represents either an interface, parcelable, or enum that is
// defined in the source file.
class AidlDefinedType : public AidlAnnotatable {
 public:
  AidlDefinedType(const AidlLocation& location, const std::string& name, const Comments& comments,
                  const std::string& package, std::vector<std::unique_ptr<AidlMember>>* members);
  virtual ~AidlDefinedType() = default;

  // non-copyable, non-movable
  AidlDefinedType(const AidlDefinedType&) = delete;
  AidlDefinedType(AidlDefinedType&&) = delete;
  AidlDefinedType& operator=(const AidlDefinedType&) = delete;
  AidlDefinedType& operator=(AidlDefinedType&&) = delete;

  const std::string& GetName() const { return name_; };

  /* dot joined package, example: "android.package.foo" */
  std::string GetPackage() const { return package_; }
  /* dot joined package and name, example: "android.package.foo.IBar" */
  std::string GetCanonicalName() const;
  const std::vector<std::string>& GetSplitPackage() const { return split_package_; }

  virtual std::string GetPreprocessDeclarationName() const = 0;

  virtual const AidlStructuredParcelable* AsStructuredParcelable() const { return nullptr; }
  virtual const AidlParcelable* AsParcelable() const { return nullptr; }
  virtual const AidlEnumDeclaration* AsEnumDeclaration() const { return nullptr; }
  virtual const AidlUnionDecl* AsUnionDeclaration() const { return nullptr; }
  virtual const AidlInterface* AsInterface() const { return nullptr; }
  virtual const AidlParameterizable<std::string>* AsParameterizable() const { return nullptr; }
  virtual bool CheckValid(const AidlTypenames& typenames) const;
  virtual bool LanguageSpecificCheckValid(const AidlTypenames& typenames,
                                          Options::Language lang) const = 0;
  AidlStructuredParcelable* AsStructuredParcelable() {
    return const_cast<AidlStructuredParcelable*>(
        const_cast<const AidlDefinedType*>(this)->AsStructuredParcelable());
  }
  AidlParcelable* AsParcelable() {
    return const_cast<AidlParcelable*>(const_cast<const AidlDefinedType*>(this)->AsParcelable());
  }
  AidlEnumDeclaration* AsEnumDeclaration() {
    return const_cast<AidlEnumDeclaration*>(
        const_cast<const AidlDefinedType*>(this)->AsEnumDeclaration());
  }
  AidlUnionDecl* AsUnionDeclaration() {
    return const_cast<AidlUnionDecl*>(
        const_cast<const AidlDefinedType*>(this)->AsUnionDeclaration());
  }
  AidlInterface* AsInterface() {
    return const_cast<AidlInterface*>(const_cast<const AidlDefinedType*>(this)->AsInterface());
  }

  AidlParameterizable<std::string>* AsParameterizable() {
    return const_cast<AidlParameterizable<std::string>*>(
        const_cast<const AidlDefinedType*>(this)->AsParameterizable());
  }

  const AidlParcelable* AsUnstructuredParcelable() const {
    if (this->AsStructuredParcelable() != nullptr) return nullptr;
    if (this->AsUnionDeclaration() != nullptr) return nullptr;
    return this->AsParcelable();
  }
  AidlParcelable* AsUnstructuredParcelable() {
    return const_cast<AidlParcelable*>(
        const_cast<const AidlDefinedType*>(this)->AsUnstructuredParcelable());
  }

  const std::vector<std::unique_ptr<AidlVariableDeclaration>>& GetFields() const {
    return variables_;
  }
  const std::vector<std::unique_ptr<AidlConstantDeclaration>>& GetConstantDeclarations() const {
    return constants_;
  }
  const std::vector<std::unique_ptr<AidlMethod>>& GetMethods() const { return methods_; }
  void AddMethod(std::unique_ptr<AidlMethod> method) { methods_.push_back(std::move(method)); }
  const std::vector<const AidlMember*>& GetMembers() const { return members_; }
  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    AidlAnnotatable::TraverseChildren(traverse);
    for (const auto c : GetMembers()) {
      traverse(*c);
    }
  }

 protected:
  // utility for subclasses with getter names
  bool CheckValidForGetterNames() const;

 private:
  bool CheckValidWithMembers(const AidlTypenames& typenames) const;

  std::string name_;
  const std::string package_;
  const std::vector<std::string> split_package_;
  std::vector<std::unique_ptr<AidlVariableDeclaration>> variables_;
  std::vector<std::unique_ptr<AidlConstantDeclaration>> constants_;
  std::vector<std::unique_ptr<AidlMethod>> methods_;
  std::vector<const AidlMember*> members_;  // keep members in order of appearance.
};

class AidlParcelable : public AidlDefinedType, public AidlParameterizable<std::string> {
 public:
  AidlParcelable(const AidlLocation& location, const std::string& name, const std::string& package,
                 const Comments& comments, const std::string& cpp_header = "",
                 std::vector<std::string>* type_params = nullptr,
                 std::vector<std::unique_ptr<AidlMember>>* members = nullptr);
  virtual ~AidlParcelable() = default;

  // non-copyable, non-movable
  AidlParcelable(const AidlParcelable&) = delete;
  AidlParcelable(AidlParcelable&&) = delete;
  AidlParcelable& operator=(const AidlParcelable&) = delete;
  AidlParcelable& operator=(AidlParcelable&&) = delete;

  std::string GetCppHeader() const { return cpp_header_; }

  bool CheckValid(const AidlTypenames& typenames) const override;
  bool LanguageSpecificCheckValid(const AidlTypenames& typenames,
                                  Options::Language lang) const override;
  const AidlParcelable* AsParcelable() const override { return this; }
  const AidlParameterizable<std::string>* AsParameterizable() const override { return this; }
  const AidlNode& AsAidlNode() const override { return *this; }
  std::string GetPreprocessDeclarationName() const override { return "parcelable"; }

  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  std::string cpp_header_;
};

class AidlStructuredParcelable : public AidlParcelable {
 public:
  AidlStructuredParcelable(const AidlLocation& location, const std::string& name,
                           const std::string& package, const Comments& comments,
                           std::vector<std::string>* type_params,
                           std::vector<std::unique_ptr<AidlMember>>* members);
  virtual ~AidlStructuredParcelable() = default;

  // non-copyable, non-movable
  AidlStructuredParcelable(const AidlStructuredParcelable&) = delete;
  AidlStructuredParcelable(AidlStructuredParcelable&&) = delete;
  AidlStructuredParcelable& operator=(const AidlStructuredParcelable&) = delete;
  AidlStructuredParcelable& operator=(AidlStructuredParcelable&&) = delete;

  const AidlStructuredParcelable* AsStructuredParcelable() const override { return this; }
  std::string GetPreprocessDeclarationName() const override { return "structured_parcelable"; }

  bool CheckValid(const AidlTypenames& typenames) const override;
  bool LanguageSpecificCheckValid(const AidlTypenames& typenames,
                                  Options::Language lang) const override;

  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }
};

class AidlEnumerator : public AidlCommentable {
 public:
  AidlEnumerator(const AidlLocation& location, const std::string& name, AidlConstantValue* value,
                 const Comments& comments);
  virtual ~AidlEnumerator() = default;

  // non-copyable, non-movable
  AidlEnumerator(const AidlEnumerator&) = delete;
  AidlEnumerator(AidlEnumerator&&) = delete;
  AidlEnumerator& operator=(const AidlEnumerator&) = delete;
  AidlEnumerator& operator=(AidlEnumerator&&) = delete;

  const std::string& GetName() const { return name_; }
  AidlConstantValue* GetValue() const { return value_.get(); }
  bool CheckValid(const AidlTypeSpecifier& enum_backing_type) const;

  string ValueString(const AidlTypeSpecifier& backing_type,
                     const ConstantValueDecorator& decorator) const;

  void SetValue(std::unique_ptr<AidlConstantValue> value) { value_ = std::move(value); }
  bool IsValueUserSpecified() const { return value_user_specified_; }

  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    traverse(*value_);
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  const std::string name_;
  unique_ptr<AidlConstantValue> value_;
  const bool value_user_specified_;
};

class AidlEnumDeclaration : public AidlDefinedType {
 public:
  AidlEnumDeclaration(const AidlLocation& location, const string& name,
                      std::vector<std::unique_ptr<AidlEnumerator>>* enumerators,
                      const std::string& package, const Comments& comments);
  virtual ~AidlEnumDeclaration() = default;

  // non-copyable, non-movable
  AidlEnumDeclaration(const AidlEnumDeclaration&) = delete;
  AidlEnumDeclaration(AidlEnumDeclaration&&) = delete;
  AidlEnumDeclaration& operator=(const AidlEnumDeclaration&) = delete;
  AidlEnumDeclaration& operator=(AidlEnumDeclaration&&) = delete;

  bool Autofill(const AidlTypenames&);
  const AidlTypeSpecifier& GetBackingType() const { return *backing_type_; }
  const std::vector<std::unique_ptr<AidlEnumerator>>& GetEnumerators() const {
    return enumerators_;
  }
  bool CheckValid(const AidlTypenames& typenames) const override;
  bool LanguageSpecificCheckValid(const AidlTypenames& /*typenames*/,
                                  Options::Language) const override {
    return true;
  }
  std::string GetPreprocessDeclarationName() const override { return "enum"; }

  const AidlEnumDeclaration* AsEnumDeclaration() const override { return this; }

  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    AidlDefinedType::TraverseChildren(traverse);
    for (const auto& c : GetEnumerators()) {
      traverse(*c);
    }
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:

  const std::string name_;
  const std::vector<std::unique_ptr<AidlEnumerator>> enumerators_;
  std::unique_ptr<AidlTypeSpecifier> backing_type_;
};

class AidlUnionDecl : public AidlParcelable {
 public:
  AidlUnionDecl(const AidlLocation& location, const std::string& name, const std::string& package,
                const Comments& comments, std::vector<std::string>* type_params,
                std::vector<std::unique_ptr<AidlMember>>* members);
  virtual ~AidlUnionDecl() = default;

  // non-copyable, non-movable
  AidlUnionDecl(const AidlUnionDecl&) = delete;
  AidlUnionDecl(AidlUnionDecl&&) = delete;
  AidlUnionDecl& operator=(const AidlUnionDecl&) = delete;
  AidlUnionDecl& operator=(AidlUnionDecl&&) = delete;


  const AidlNode& AsAidlNode() const override { return *this; }
  bool CheckValid(const AidlTypenames& typenames) const override;
  bool LanguageSpecificCheckValid(const AidlTypenames& typenames,
                                  Options::Language lang) const override;
  std::string GetPreprocessDeclarationName() const override { return "union"; }

  const AidlUnionDecl* AsUnionDeclaration() const override { return this; }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }
};

class AidlInterface final : public AidlDefinedType {
 public:
  AidlInterface(const AidlLocation& location, const std::string& name, const Comments& comments,
                bool oneway_, const std::string& package,
                std::vector<std::unique_ptr<AidlMember>>* members);
  virtual ~AidlInterface() = default;

  // non-copyable, non-movable
  AidlInterface(const AidlInterface&) = delete;
  AidlInterface(AidlInterface&&) = delete;
  AidlInterface& operator=(const AidlInterface&) = delete;
  AidlInterface& operator=(AidlInterface&&) = delete;

  const AidlInterface* AsInterface() const override { return this; }
  std::string GetPreprocessDeclarationName() const override { return "interface"; }

  bool CheckValid(const AidlTypenames& typenames) const override;
  bool LanguageSpecificCheckValid(const AidlTypenames& typenames,
                                  Options::Language lang) const override;

  std::string GetDescriptor() const;
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }
};

class AidlPackage : public AidlNode {
 public:
  AidlPackage(const AidlLocation& location, const Comments& comments)
      : AidlNode(location, comments) {}
  virtual ~AidlPackage() = default;
  void TraverseChildren(std::function<void(const AidlNode&)>) const {}
  void DispatchVisit(AidlVisitor& v) const { v.Visit(*this); }
};

class AidlImport : public AidlNode {
 public:
  AidlImport(const AidlLocation& location, const std::string& needed_class,
             const Comments& comments);
  virtual ~AidlImport() = default;

  // non-copyable, non-movable
  AidlImport(const AidlImport&) = delete;
  AidlImport(AidlImport&&) = delete;
  AidlImport& operator=(const AidlImport&) = delete;
  AidlImport& operator=(AidlImport&&) = delete;

  const std::string& GetNeededClass() const { return needed_class_; }
  void TraverseChildren(std::function<void(const AidlNode&)>) const {}
  void DispatchVisit(AidlVisitor& v) const { v.Visit(*this); }

 private:
  std::string needed_class_;
};

// AidlDocument models an AIDL file
class AidlDocument : public AidlCommentable {
 public:
  AidlDocument(const AidlLocation& location, const Comments& comments,
               std::vector<std::unique_ptr<AidlImport>> imports,
               std::vector<std::unique_ptr<AidlDefinedType>> defined_types)
      : AidlCommentable(location, comments),
        imports_(std::move(imports)),
        defined_types_(std::move(defined_types)) {}
  ~AidlDocument() = default;

  // non-copyable, non-movable
  AidlDocument(const AidlDocument&) = delete;
  AidlDocument(AidlDocument&&) = delete;
  AidlDocument& operator=(const AidlDocument&) = delete;
  AidlDocument& operator=(AidlDocument&&) = delete;

  std::optional<std::string> ResolveName(const std::string& unresolved_type) const;
  const std::vector<std::unique_ptr<AidlImport>>& Imports() const { return imports_; }
  const std::vector<std::unique_ptr<AidlDefinedType>>& DefinedTypes() const {
    return defined_types_;
  }

  void TraverseChildren(std::function<void(const AidlNode&)> traverse) const override {
    for (const auto& i : Imports()) {
      traverse(*i);
    }
    for (const auto& t : DefinedTypes()) {
      traverse(*t);
    }
  }
  void DispatchVisit(AidlVisitor& v) const override { v.Visit(*this); }

 private:
  const std::vector<std::unique_ptr<AidlImport>> imports_;
  const std::vector<std::unique_ptr<AidlDefinedType>> defined_types_;
};

template <typename T>
std::optional<T> AidlAnnotation::ParamValue(const std::string& param_name) const {
  auto it = parameters_.find(param_name);
  if (it == parameters_.end()) {
    return std::nullopt;
  }
  return it->second->EvaluatedValue<T>();
}

// Utility to make a visitor to visit AST tree in top-down order
// Given:       foo
//              / \
//            bar baz
// VisitTopDown(v, foo) makes v visit foo -> bar -> baz.
inline void VisitTopDown(AidlVisitor& v, const AidlNode& node) {
  std::function<void(const AidlNode&)> top_down = [&](const AidlNode& n) {
    n.DispatchVisit(v);
    n.TraverseChildren(top_down);
  };
  top_down(node);
}
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
#include "aidl_to_cpp_common.h"

#include <android-base/format.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include <set>
#include <unordered_map>

#include "ast_cpp.h"
#include "comments.h"
#include "logging.h"
#include "os.h"

using ::android::base::Join;

namespace android {
namespace aidl {
namespace cpp {

char kTransactionLogStruct[] = R"(struct TransactionLog {
  double duration_ms;
  std::string interface_name;
  std::string method_name;
  const void* proxy_address;
  const void* stub_address;
  std::vector<std::pair<std::string, std::string>> input_args;
  std::vector<std::pair<std::string, std::string>> output_args;
  std::string result;
  std::string exception_message;
  int32_t exception_code;
  int32_t transaction_error;
  int32_t service_specific_error_code;
};
)";

string ClassName(const AidlDefinedType& defined_type, ClassNames type) {
  string base_name = defined_type.GetName();
  if (base_name.length() >= 2 && base_name[0] == 'I' && isupper(base_name[1])) {
    base_name = base_name.substr(1);
  }

  switch (type) {
    case ClassNames::CLIENT:
      return "Bp" + base_name;
    case ClassNames::SERVER:
      return "Bn" + base_name;
    case ClassNames::INTERFACE:
      return "I" + base_name;
    case ClassNames::DEFAULT_IMPL:
      return "I" + base_name + "Default";
    case ClassNames::BASE:
      return base_name;
    case ClassNames::RAW:
      [[fallthrough]];
    default:
      return defined_type.GetName();
  }
}

std::string HeaderFile(const AidlDefinedType& defined_type, ClassNames class_type,
                       bool use_os_sep) {
  std::string file_path = defined_type.GetPackage();
  for (char& c : file_path) {
    if (c == '.') {
      c = (use_os_sep) ? OS_PATH_SEPARATOR : '/';
    }
  }
  if (!file_path.empty()) {
    file_path += (use_os_sep) ? OS_PATH_SEPARATOR : '/';
  }
  file_path += ClassName(defined_type, class_type);
  file_path += ".h";

  return file_path;
}

void EnterNamespace(CodeWriter& out, const AidlDefinedType& defined_type) {
  const std::vector<std::string> packages = defined_type.GetSplitPackage();
  for (const std::string& package : packages) {
    out << "namespace " << package << " {\n";
  }
}
void LeaveNamespace(CodeWriter& out, const AidlDefinedType& defined_type) {
  const std::vector<std::string> packages = defined_type.GetSplitPackage();
  for (auto it = packages.rbegin(); it != packages.rend(); ++it) {
    out << "}  // namespace " << *it << "\n";
  }
}

string BuildVarName(const AidlArgument& a) {
  string prefix = "out_";
  if (a.GetDirection() & AidlArgument::IN_DIR) {
    prefix = "in_";
  }
  return prefix + a.GetName();
}

void WriteLogForArgument(CodeWriter& w, const AidlArgument& a, bool is_server,
                         const string& log_var, bool is_ndk) {
  const string var_name = is_server || is_ndk ? BuildVarName(a) : a.GetName();
  const bool is_pointer = a.IsOut() && !is_server;
  const string value_expr = (is_pointer ? "*" : "") + var_name;
  w << log_var
    << ".emplace_back(\"" + var_name + "\", ::android::internal::ToString(" + value_expr + "));\n";
}

const string GenLogBeforeExecute(const string className, const AidlMethod& method, bool isServer,
                                 bool isNdk) {
  string code;
  CodeWriterPtr writer = CodeWriter::ForString(&code);
  (*writer) << className << "::TransactionLog _transaction_log;\n";

  (*writer) << "if (" << className << "::logFunc != nullptr) {\n";
  (*writer).Indent();

  for (const auto& a : method.GetInArguments()) {
    WriteLogForArgument(*writer, *a, isServer, "_transaction_log.input_args", isNdk);
  }

  (*writer).Dedent();
  (*writer) << "}\n";

  (*writer) << "auto _log_start = std::chrono::steady_clock::now();\n";
  writer->Close();
  return code;
}

const string GenLogAfterExecute(const string className, const AidlInterface& interface,
                                const AidlMethod& method, const string& statusVarName,
                                const string& returnVarName, bool isServer, bool isNdk) {
  string code;
  CodeWriterPtr writer = CodeWriter::ForString(&code);

  (*writer) << "if (" << className << "::logFunc != nullptr) {\n";
  (*writer).Indent();
  const auto address = (isNdk && isServer) ? "_aidl_impl.get()" : "static_cast<const void*>(this)";
  (*writer) << "auto _log_end = std::chrono::steady_clock::now();\n";
  (*writer) << "_transaction_log.duration_ms = std::chrono::duration<double, std::milli>(_log_end "
               "- _log_start).count();\n";
  (*writer) << "_transaction_log.interface_name = \"" << interface.GetCanonicalName() << "\";\n";
  (*writer) << "_transaction_log.method_name = \"" << method.GetName() << "\";\n";
  (*writer) << "_transaction_log.stub_address = " << (isServer ? address : "nullptr") << ";\n";
  (*writer) << "_transaction_log.proxy_address = " << (isServer ? "nullptr" : address) << ";\n";

  if (isNdk) {
    (*writer) << "_transaction_log.exception_code = AStatus_getExceptionCode(" << statusVarName
              << ".get());\n";
    (*writer) << "_transaction_log.exception_message = AStatus_getMessage(" << statusVarName
              << ".get());\n";
    (*writer) << "_transaction_log.transaction_error = AStatus_getStatus(" << statusVarName
              << ".get());\n";
    (*writer) << "_transaction_log.service_specific_error_code = AStatus_getServiceSpecificError("
              << statusVarName << ".get());\n";
  } else {
    (*writer) << "_transaction_log.exception_code = " << statusVarName << ".exceptionCode();\n";
    (*writer) << "_transaction_log.exception_message = " << statusVarName
              << ".exceptionMessage();\n";
    (*writer) << "_transaction_log.transaction_error = " << statusVarName
              << ".transactionError();\n";
    (*writer) << "_transaction_log.service_specific_error_code = " << statusVarName
              << ".serviceSpecificErrorCode();\n";
  }

  for (const auto& a : method.GetOutArguments()) {
    WriteLogForArgument(*writer, *a, isServer, "_transaction_log.output_args", isNdk);
  }

  if (method.GetType().GetName() != "void") {
    const string expr = (isServer ? "" : "*") + returnVarName;
    (*writer) << "_transaction_log.result = ::android::internal::ToString(" << expr << ");\n";
  }

  // call the user-provided function with the transaction log object
  (*writer) << className << "::logFunc(_transaction_log);\n";

  (*writer).Dedent();
  (*writer) << "}\n";

  writer->Close();
  return code;
}

std::string GenerateEnumValues(const AidlEnumDeclaration& enum_decl,
                               const std::vector<std::string>& enclosing_namespaces_of_enum_decl) {
  const auto fq_name =
      Join(Append(enclosing_namespaces_of_enum_decl, enum_decl.GetSplitPackage()), "::") +
      "::" + enum_decl.GetName();
  const auto size = enum_decl.GetEnumerators().size();
  std::ostringstream code;
  code << "#pragma clang diagnostic push\n";
  code << "#pragma clang diagnostic ignored \"-Wc++17-extensions\"\n";
  code << "template <>\n";
  code << "constexpr inline std::array<" << fq_name << ", " << size << ">";
  GenerateDeprecated(code, enum_decl);
  code << " enum_values<" << fq_name << "> = {\n";
  for (const auto& enumerator : enum_decl.GetEnumerators()) {
    code << "  " << fq_name << "::" << enumerator->GetName() << ",\n";
  }
  code << "};\n";
  code << "#pragma clang diagnostic pop\n";
  return code.str();
}

std::string TemplateDecl(const AidlParcelable& defined_type) {
  std::string decl = "";
  if (defined_type.IsGeneric()) {
    std::vector<std::string> template_params;
    for (const auto& parameter : defined_type.GetTypeParameters()) {
      template_params.push_back(parameter);
    }
    decl = base::StringPrintf("template <typename %s>\n",
                              base::Join(template_params, ", typename ").c_str());
  }
  return decl;
}

void GenerateParcelableComparisonOperators(CodeWriter& out, const AidlParcelable& parcelable) {
  std::set<string> operators{"<", ">", "==", ">=", "<=", "!="};
  bool is_empty = false;

  auto comparable = [&](const string& prefix) {
    vector<string> fields;
    if (auto p = parcelable.AsStructuredParcelable(); p != nullptr) {
      is_empty = p->GetFields().empty();
      for (const auto& f : p->GetFields()) {
        fields.push_back(prefix + f->GetName());
      }
      return "std::tie(" + Join(fields, ", ") + ")";
    } else if (auto p = parcelable.AsUnionDeclaration(); p != nullptr) {
      return prefix + "_value";
    } else {
      AIDL_FATAL(parcelable) << "Unknown paracelable type";
    }
  };

  string lhs = comparable("");
  string rhs = comparable("rhs.");
  for (const auto& op : operators) {
    out << "inline bool operator" << op << "(const " << parcelable.GetName() << "&"
        << (is_empty ? "" : " rhs") << ") const {\n"
        << "  return " << lhs << " " << op << " " << rhs << ";\n"
        << "}\n";
  }
  out << "\n";
}

// Output may look like:
// inline std::string toString() const {
//   std::ostringstream os;
//   os << "MyData{";
//   os << "field1: " << field1;
//   os << ", field2: " << v.field2;
//   ...
//   os << "}";
//   return os.str();
// }
void GenerateToString(CodeWriter& out, const AidlStructuredParcelable& parcelable) {
  out << "inline std::string toString() const {\n";
  out.Indent();
  out << "std::ostringstream os;\n";
  out << "os << \"" << parcelable.GetName() << "{\";\n";
  bool first = true;
  for (const auto& f : parcelable.GetFields()) {
    if (first) {
      out << "os << \"";
      first = false;
    } else {
      out << "os << \", ";
    }
    out << f->GetName() << ": \" << ::android::internal::ToString(" << f->GetName() << ");\n";
  }
  out << "os << \"}\";\n";
  out << "return os.str();\n";
  out.Dedent();
  out << "}\n";
}

// Output may look like:
// inline std::string toString() const {
//   std::ostringstream os;
//   os << "MyData{";
//   switch (v.getTag()) {
//   case MyData::field: os << "field: " << v.get<MyData::field>(); break;
//   ...
//   }
//   os << "}";
//   return os.str();
// }
void GenerateToString(CodeWriter& out, const AidlUnionDecl& parcelable) {
  out << "inline std::string toString() const {\n";
  out.Indent();
  out << "std::ostringstream os;\n";
  out << "os << \"" + parcelable.GetName() + "{\";\n";
  out << "switch (getTag()) {\n";
  for (const auto& f : parcelable.GetFields()) {
    const string tag = f->GetName();
    out << "case " << tag << ": os << \"" << tag << ": \" << "
        << "::android::internal::ToString(get<" + tag + ">()); break;\n";
  }
  out << "}\n";
  out << "os << \"}\";\n";
  out << "return os.str();\n";
  out.Dedent();
  out << "}\n";
}

std::string GetDeprecatedAttribute(const AidlCommentable& type) {
  if (auto deprecated = FindDeprecated(type.GetComments()); deprecated.has_value()) {
    if (deprecated->note.empty()) {
      return "__attribute__((deprecated))";
    }
    return "__attribute__((deprecated(" + QuotedEscape(deprecated->note) + ")))";
  }
  return "";
}

const vector<string> UnionWriter::headers{
    "cassert",      // __assert for logging
    "type_traits",  // std::is_same_v
    "utility",      // std::mode/forward for value
    "variant",      // std::variant for value
};

void UnionWriter::PrivateFields(CodeWriter& out) const {
  vector<string> field_types;
  for (const auto& f : decl.GetFields()) {
    field_types.push_back(name_of(f->GetType(), typenames));
  }
  out << "std::variant<" + Join(field_types, ", ") + "> _value;\n";
}

void UnionWriter::PublicFields(CodeWriter& out) const {
  AidlTypeSpecifier tag_type(AIDL_LOCATION_HERE, "int", /* is_array= */ false,
                             /* type_params= */ nullptr, Comments{});
  tag_type.Resolve(typenames);

  out << "enum Tag : " << name_of(tag_type, typenames) << " {\n";
  bool is_first = true;
  for (const auto& f : decl.GetFields()) {
    out << "  " << f->GetName();
    GenerateDeprecated(out, *f);
    if (is_first) out << " = 0";
    out << ",  // " << f->Signature() << ";\n";
    is_first = false;
  }
  out << "};\n";

  const auto& name = decl.GetName();

  AIDL_FATAL_IF(decl.GetFields().empty(), decl) << "Union '" << name << "' is empty.";
  const auto& first_field = decl.GetFields()[0];
  const auto& default_name = first_field->GetName();
  const auto& default_value =
      name_of(first_field->GetType(), typenames) + "(" + first_field->ValueString(decorator) + ")";

  auto tmpl = R"--(
template<typename _Tp>
static constexpr bool _not_self = !std::is_same_v<std::remove_cv_t<std::remove_reference_t<_Tp>>, {name}>;

{name}() : _value(std::in_place_index<{default_name}>, {default_value}) {{ }}
{name}(const {name}&) = default;
{name}({name}&&) = default;
{name}& operator=(const {name}&) = default;
{name}& operator=({name}&&) = default;

template <typename _Tp, typename = std::enable_if_t<_not_self<_Tp>>>
// NOLINTNEXTLINE(google-explicit-constructor)
constexpr {name}(_Tp&& _arg)
    : _value(std::forward<_Tp>(_arg)) {{}}

template <size_t _Np, typename... _Tp>
constexpr explicit {name}(std::in_place_index_t<_Np>, _Tp&&... _args)
    : _value(std::in_place_index<_Np>, std::forward<_Tp>(_args)...) {{}}

template <Tag _tag, typename... _Tp>
static {name} make(_Tp&&... _args) {{
  return {name}(std::in_place_index<_tag>, std::forward<_Tp>(_args)...);
}}

template <Tag _tag, typename _Tp, typename... _Up>
static {name} make(std::initializer_list<_Tp> _il, _Up&&... _args) {{
  return {name}(std::in_place_index<_tag>, std::move(_il), std::forward<_Up>(_args)...);
}}

Tag getTag() const {{
  return static_cast<Tag>(_value.index());
}}

template <Tag _tag>
const auto& get() const {{
  if (getTag() != _tag) {{ __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "bad access: a wrong tag"); }}
  return std::get<_tag>(_value);
}}

template <Tag _tag>
auto& get() {{
  if (getTag() != _tag) {{ __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "bad access: a wrong tag"); }}
  return std::get<_tag>(_value);
}}

template <Tag _tag, typename... _Tp>
void set(_Tp&&... _args) {{
  _value.emplace<_tag>(std::forward<_Tp>(_args)...);
}}

)--";
  out << fmt::format(tmpl, fmt::arg("name", name), fmt::arg("default_name", default_name),
                     fmt::arg("default_value", default_value));
}

void UnionWriter::ReadFromParcel(CodeWriter& out, const ParcelWriterContext& ctx) const {
  AidlTypeSpecifier tag_type(AIDL_LOCATION_HERE, "int", /* is_array= */ false,
                             /* type_params= */ nullptr, Comments{});
  tag_type.Resolve(typenames);

  const string tag = "_aidl_tag";
  const string value = "_aidl_value";
  const string status = "_aidl_ret_status";

  auto read_var = [&](const string& var, const AidlTypeSpecifier& type) {
    out << fmt::format("{} {};\n", name_of(type, typenames), var);
    out << fmt::format("if (({} = ", status);
    ctx.read_func(out, var, type);
    out << fmt::format(") != {}) return {};\n", ctx.status_ok, status);
  };

  out << fmt::format("{} {};\n", ctx.status_type, status);
  read_var(tag, tag_type);
  out << fmt::format("switch ({}) {{\n", tag);
  for (const auto& variable : decl.GetFields()) {
    out << fmt::format("case {}: {{\n", variable->GetName());
    out.Indent();
    const auto& type = variable->GetType();
    read_var(value, type);
    out << fmt::format("if constexpr (std::is_trivially_copyable_v<{}>) {{\n",
                       name_of(type, typenames));
    out.Indent();
    out << fmt::format("set<{}>({});\n", variable->GetName(), value);
    out.Dedent();
    out << "} else {\n";
    out.Indent();
    // Even when the `if constexpr` is false, the compiler runs the tidy check for the
    // next line, which doesn't make sense. Silence the check for the unreachable code.
    out << "// NOLINTNEXTLINE(performance-move-const-arg)\n";
    out << fmt::format("set<{}>(std::move({}));\n", variable->GetName(), value);
    out.Dedent();
    out << "}\n";
    out << fmt::format("return {}; }}\n", ctx.status_ok);
    out.Dedent();
  }
  out << "}\n";
  out << fmt::format("return {};\n", ctx.status_bad);
}

void UnionWriter::WriteToParcel(CodeWriter& out, const ParcelWriterContext& ctx) const {
  AidlTypeSpecifier tag_type(AIDL_LOCATION_HERE, "int", /* is_array= */ false,
                             /* type_params= */ nullptr, Comments{});
  tag_type.Resolve(typenames);

  const string tag = "_aidl_tag";
  const string value = "_aidl_value";
  const string status = "_aidl_ret_status";

  out << fmt::format("{} {} = ", ctx.status_type, status);
  ctx.write_func(out, "getTag()", tag_type);
  out << ";\n";
  out << fmt::format("if ({} != {}) return {};\n", status, ctx.status_ok, status);
  out << "switch (getTag()) {\n";
  for (const auto& variable : decl.GetFields()) {
    out << fmt::format("case {}: return ", variable->GetName());
    ctx.write_func(out, "get<" + variable->GetName() + ">()", variable->GetType());
    out << ";\n";
  }
  out << "}\n";
  out << "__assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, \"can't reach here\");\n";
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android

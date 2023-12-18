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

#include "generate_ndk.h"

#include "aidl.h"
#include "aidl_language.h"
#include "aidl_to_cpp_common.h"
#include "aidl_to_ndk.h"
#include "logging.h"

#include <android-base/stringprintf.h>

namespace android {
namespace aidl {
namespace ndk {

static constexpr const char* kClazz = "_g_aidl_clazz";
static constexpr const char* kDescriptor = "descriptor";
static constexpr const char* kVersion = "version";
static constexpr const char* kHash = "hash";
static constexpr const char* kCachedVersion = "_aidl_cached_version";
static constexpr const char* kCachedHash = "_aidl_cached_hash";
static constexpr const char* kCachedHashMutex = "_aidl_cached_hash_mutex";

using namespace internals;
namespace internals {
void GenerateParcelHeader(CodeWriter& out, const AidlTypenames& types,
                          const AidlStructuredParcelable& defined_type, const Options& options);
void GenerateParcelSource(CodeWriter& out, const AidlTypenames& types,
                          const AidlStructuredParcelable& defined_type, const Options& options);
void GenerateParcelHeader(CodeWriter& out, const AidlTypenames& types,
                          const AidlUnionDecl& defined_type, const Options& options);
void GenerateParcelSource(CodeWriter& out, const AidlTypenames& types,
                          const AidlUnionDecl& defined_type, const Options& options);
}  // namespace internals

using cpp::ClassNames;

void GenerateNdkInterface(const string& output_file, const Options& options,
                          const AidlTypenames& types, const AidlInterface& defined_type,
                          const IoDelegate& io_delegate) {
  const string i_header = options.OutputHeaderDir() + NdkHeaderFile(defined_type, ClassNames::RAW);
  unique_ptr<CodeWriter> i_writer(io_delegate.GetCodeWriter(i_header));
  GenerateInterfaceHeader(*i_writer, types, defined_type, options);
  AIDL_FATAL_IF(!i_writer->Close(), i_header);

  const string bp_header =
      options.OutputHeaderDir() + NdkHeaderFile(defined_type, ClassNames::CLIENT);
  unique_ptr<CodeWriter> bp_writer(io_delegate.GetCodeWriter(bp_header));
  GenerateClientHeader(*bp_writer, types, defined_type, options);
  AIDL_FATAL_IF(!bp_writer->Close(), bp_header);

  const string bn_header =
      options.OutputHeaderDir() + NdkHeaderFile(defined_type, ClassNames::SERVER);
  unique_ptr<CodeWriter> bn_writer(io_delegate.GetCodeWriter(bn_header));
  GenerateServerHeader(*bn_writer, types, defined_type, options);
  AIDL_FATAL_IF(!bn_writer->Close(), bn_header);

  unique_ptr<CodeWriter> source_writer = io_delegate.GetCodeWriter(output_file);
  GenerateSource(*source_writer, types, defined_type, options);
  AIDL_FATAL_IF(!source_writer->Close(), output_file);
}

template <typename ParcelableType>
void GenerateNdkParcel(const string& output_file, const Options& options,
                       const AidlTypenames& types, const ParcelableType& defined_type,
                       const IoDelegate& io_delegate) {
  const string header_path =
      options.OutputHeaderDir() + NdkHeaderFile(defined_type, ClassNames::RAW);
  unique_ptr<CodeWriter> header_writer(io_delegate.GetCodeWriter(header_path));
  GenerateParcelHeader(*header_writer, types, defined_type, options);

  unique_ptr<CodeWriter> source_writer(io_delegate.GetCodeWriter(output_file));
  if (defined_type.IsGeneric()) {
    // Need to write source to header if this is a template
    GenerateParcelSource(*header_writer, types, defined_type, options);
  } else {
    GenerateParcelSource(*source_writer, types, defined_type, options);
  }
  (source_writer->Close());
  AIDL_FATAL_IF(!header_writer->Close(), header_path);

  const string bp_header =
      options.OutputHeaderDir() + NdkHeaderFile(defined_type, ClassNames::CLIENT);
  unique_ptr<CodeWriter> bp_writer(io_delegate.GetCodeWriter(bp_header));
  *bp_writer << "#error TODO(b/111362593) defined_types do not have bp classes\n";
  AIDL_FATAL_IF(!bp_writer->Close(), bp_header);

  const string bn_header =
      options.OutputHeaderDir() + NdkHeaderFile(defined_type, ClassNames::SERVER);
  unique_ptr<CodeWriter> bn_writer(io_delegate.GetCodeWriter(bn_header));
  *bn_writer << "#error TODO(b/111362593) defined_types do not have bn classes\n";
  AIDL_FATAL_IF(!bn_writer->Close(), bn_header);
}

void GenerateNdkParcelDeclaration(const std::string& filename, const IoDelegate& io_delegate) {
  CodeWriterPtr code_writer = io_delegate.GetCodeWriter(filename);
  *code_writer
      << "// This file is intentionally left blank as placeholder for parcel declaration.\n";
  AIDL_FATAL_IF(!code_writer->Close(), filename);
}

void GenerateNdkEnumDeclaration(const string& output_file, const Options& options,
                                const AidlTypenames& types, const AidlEnumDeclaration& defined_type,
                                const IoDelegate& io_delegate) {
  const string header_path =
      options.OutputHeaderDir() + NdkHeaderFile(defined_type, ClassNames::RAW);
  unique_ptr<CodeWriter> header_writer(io_delegate.GetCodeWriter(header_path));
  GenerateEnumHeader(*header_writer, types, defined_type, options);
  AIDL_FATAL_IF(!header_writer->Close(), header_path);

  const string bp_header =
      options.OutputHeaderDir() + NdkHeaderFile(defined_type, ClassNames::CLIENT);
  unique_ptr<CodeWriter> bp_writer(io_delegate.GetCodeWriter(bp_header));
  *bp_writer << "#error TODO(b/111362593) enums do not have bp classes\n";
  AIDL_FATAL_IF(!bp_writer->Close(), bp_header);

  const string bn_header =
      options.OutputHeaderDir() + NdkHeaderFile(defined_type, ClassNames::SERVER);
  unique_ptr<CodeWriter> bn_writer(io_delegate.GetCodeWriter(bn_header));
  *bn_writer << "#error TODO(b/111362593) enums do not have bn classes\n";
  AIDL_FATAL_IF(!bn_writer->Close(), bn_header);

  unique_ptr<CodeWriter> source_writer = io_delegate.GetCodeWriter(output_file);
  *source_writer
      << "// This file is intentionally left blank as placeholder for enum declaration.\n";
  AIDL_FATAL_IF(!source_writer->Close(), output_file);
}

void GenerateNdk(const string& output_file, const Options& options, const AidlTypenames& types,
                 const AidlDefinedType& defined_type, const IoDelegate& io_delegate) {
  if (const AidlStructuredParcelable* parcelable = defined_type.AsStructuredParcelable();
      parcelable != nullptr) {
    GenerateNdkParcel<AidlStructuredParcelable>(output_file, options, types, *parcelable,
                                                io_delegate);
    return;
  }

  if (const AidlUnionDecl* union_decl = defined_type.AsUnionDeclaration(); union_decl != nullptr) {
    GenerateNdkParcel<AidlUnionDecl>(output_file, options, types, *union_decl, io_delegate);
    return;
  }

  if (const AidlParcelable* parcelable_decl = defined_type.AsParcelable();
      parcelable_decl != nullptr) {
    GenerateNdkParcelDeclaration(output_file, io_delegate);
    return;
  }

  if (const AidlEnumDeclaration* enum_decl = defined_type.AsEnumDeclaration();
      enum_decl != nullptr) {
    GenerateNdkEnumDeclaration(output_file, options, types, *enum_decl, io_delegate);
    return;
  }

  if (const AidlInterface* interface = defined_type.AsInterface(); interface != nullptr) {
    GenerateNdkInterface(output_file, options, types, *interface, io_delegate);
    return;
  }

  AIDL_FATAL(defined_type) << "Unrecognized type sent for NDK cpp generation.";
}
namespace internals {

void EnterNdkNamespace(CodeWriter& out, const AidlDefinedType& defined_type) {
  out << "namespace aidl {\n";
  cpp::EnterNamespace(out, defined_type);
}
void LeaveNdkNamespace(CodeWriter& out, const AidlDefinedType& defined_type) {
  cpp::LeaveNamespace(out, defined_type);
  out << "}  // namespace aidl\n";
}

static void StatusCheckGoto(CodeWriter& out) {
  out << "if (_aidl_ret_status != STATUS_OK) goto _aidl_error;\n\n";
}
static void StatusCheckBreak(CodeWriter& out) {
  out << "if (_aidl_ret_status != STATUS_OK) break;\n\n";
}
static void StatusCheckReturn(CodeWriter& out) {
  out << "if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;\n\n";
}

static void GenerateHeaderIncludes(CodeWriter& out, const AidlTypenames& types,
                                   const AidlDefinedType& defined_type) {
  out << "#include <cstdint>\n";
  out << "#include <memory>\n";
  out << "#include <optional>\n";
  out << "#include <string>\n";
  out << "#include <vector>\n";
  out << "#ifdef BINDER_STABILITY_SUPPORT\n";
  out << "#include <android/binder_stability.h>\n";
  out << "#endif  // BINDER_STABILITY_SUPPORT\n";

  if (defined_type.IsSensitiveData()) {
    out << "#include <android/binder_parcel_platform.h>\n";
    out << "#include <android/binder_ibinder_platform.h>\n";
  }

  auto headerFilePath = [&types](const AidlTypeSpecifier& typespec) -> std::string {
    const AidlDefinedType* type = types.TryGetDefinedType(typespec.GetName());
    if (type == nullptr) {
      // could be a primitive type.
      return "";
    }

    if (type->AsInterface() != nullptr) {
      return NdkHeaderFile(*type, ClassNames::RAW, false /*use_os_sep*/);
    } else if (type->AsStructuredParcelable() != nullptr) {
      return NdkHeaderFile(*type, ClassNames::RAW, false /*use_os_sep*/);
    } else if (type->AsUnionDeclaration() != nullptr) {
      return NdkHeaderFile(*type, ClassNames::RAW, false /*use_os_sep*/);
    } else if (type->AsParcelable() != nullptr) {
      return type->AsParcelable()->GetCppHeader();
    } else if (type->AsEnumDeclaration() != nullptr) {
      return NdkHeaderFile(*type, ClassNames::RAW, false /*use_os_sep*/);
    } else {
      AIDL_FATAL(*type) << "Unrecognized type.";
      return "";
    }
  };

  std::set<std::string> includes;

  // visit a type and collect all reference types' headers
  std::function<void(const AidlTypeSpecifier& type)> visit = [&](const AidlTypeSpecifier& type) {
    includes.insert(headerFilePath(type));
    if (type.IsGeneric()) {
      for (const auto& param : type.GetTypeParameters()) {
        visit(*param);
      }
    }
  };

  const AidlInterface* interface = defined_type.AsInterface();
  if (interface != nullptr) {
    for (const auto& method : interface->GetMethods()) {
      visit(method->GetType());
      for (const auto& argument : method->GetArguments()) {
        visit(argument->GetType());
      }
    }
  }

  auto visit_parcelable = [&](const auto& parcelable) {
    for (const auto& field : parcelable.GetFields()) {
      visit(field->GetType());
      // Check the fields for generic type arguments
      if (field->GetType().IsGeneric()) {
        for (const auto& type_argument : field->GetType().GetTypeParameters()) {
          visit(*type_argument);
        }
      }
    }
  };

  const AidlStructuredParcelable* parcelable = defined_type.AsStructuredParcelable();
  if (parcelable != nullptr) {
    visit_parcelable(*parcelable);
  }

  const AidlUnionDecl* union_decl = defined_type.AsUnionDeclaration();
  if (union_decl != nullptr) {
    visit_parcelable(*union_decl);
  }

  const AidlEnumDeclaration* enum_decl = defined_type.AsEnumDeclaration();
  if (enum_decl != nullptr) {
    visit(enum_decl->GetBackingType());
  }

  for (const auto& path : includes) {
    if (path == "") {
      continue;
    }
    out << "#include <" << path << ">\n";
  }
}

static void GenerateSourceIncludes(CodeWriter& out, const AidlTypenames& types,
                                   const AidlDefinedType& /*defined_type*/) {
  out << "#include <android/binder_parcel_utils.h>\n";

  types.IterateTypes([&](const AidlDefinedType& a_defined_type) {
    if (a_defined_type.AsInterface() != nullptr) {
      out << "#include <" << NdkHeaderFile(a_defined_type, ClassNames::CLIENT, false /*use_os_sep*/)
          << ">\n";
      out << "#include <" << NdkHeaderFile(a_defined_type, ClassNames::SERVER, false /*use_os_sep*/)
          << ">\n";
      out << "#include <" << NdkHeaderFile(a_defined_type, ClassNames::RAW, false /*use_os_sep*/)
          << ">\n";
    }
  });
}

template <typename TypeWithConstants>
static void GenerateConstantDeclarations(CodeWriter& out, const AidlTypenames& types,
                                         const TypeWithConstants& type) {
  for (const auto& constant : type.GetConstantDeclarations()) {
    const AidlTypeSpecifier& type = constant->GetType();

    if (type.Signature() == "String") {
      out << "static const char*";
      cpp::GenerateDeprecated(out, *constant);
      out << " " << constant->GetName() << ";\n";
    } else {
      out << "enum : " << NdkNameOf(types, type, StorageMode::STACK) << " { ";
      out << constant->GetName();
      cpp::GenerateDeprecated(out, *constant);
      out << " = " << constant->ValueString(ConstantValueDecorator) << " };\n";
    }
  }
}

template <typename TypeWithConstants>
static void GenerateConstantDefinitions(CodeWriter& out, const TypeWithConstants& interface,
                                        const std::string& clazz,
                                        const std::string& tmpl_decl = "") {
  for (const auto& constant : interface.GetConstantDeclarations()) {
    const AidlConstantValue& value = constant->GetValue();
    AIDL_FATAL_IF(value.GetType() == AidlConstantValue::Type::UNARY ||
                      value.GetType() == AidlConstantValue::Type::BINARY,
                  value);
    if (value.GetType() == AidlConstantValue::Type::STRING) {
      out << tmpl_decl;
      out << "const char* " << clazz << "::" << constant->GetName() << " = "
          << constant->ValueString(ConstantValueDecorator) << ";\n";
    }
  }
}

void GenerateSource(CodeWriter& out, const AidlTypenames& types, const AidlInterface& defined_type,
                    const Options& options) {
  GenerateSourceIncludes(out, types, defined_type);
  if (options.GenLog()) {
    out << "#include <android/binder_to_string.h>\n";
  }
  out << "\n";

  EnterNdkNamespace(out, defined_type);
  GenerateClassSource(out, types, defined_type, options);
  GenerateClientSource(out, types, defined_type, options);
  GenerateServerSource(out, types, defined_type, options);
  GenerateInterfaceSource(out, types, defined_type, options);
  LeaveNdkNamespace(out, defined_type);
}

static std::string MethodId(const AidlMethod& m) {
  return "(FIRST_CALL_TRANSACTION + " + std::to_string(m.GetId()) + " /*" + m.GetName() + "*/)";
}

static void GenerateClientMethodDefinition(CodeWriter& out, const AidlTypenames& types,
                                           const AidlInterface& defined_type,
                                           const AidlMethod& method,
                                           const Options& options) {
  const std::string clazz = ClassName(defined_type, ClassNames::CLIENT);

  out << NdkMethodDecl(types, method, clazz) << " {\n";
  out.Indent();
  out << "binder_status_t _aidl_ret_status = STATUS_OK;\n";
  out << "::ndk::ScopedAStatus _aidl_status;\n";

  if (method.GetName() == kGetInterfaceHash && !options.Hash().empty()) {
    out << "const std::lock_guard<std::mutex> lock(" << kCachedHashMutex << ");\n";
    out << "if (" << kCachedHash << " != \"-1\") {\n";
    out.Indent();
    out << "*_aidl_return = " << kCachedHash << ";\n"
        << "_aidl_status.set(AStatus_fromStatus(_aidl_ret_status));\n"
        << "return _aidl_status;\n";
    out.Dedent();
    out << "}\n";
  } else if (method.GetName() == kGetInterfaceVersion && options.Version() > 0) {
    out << "if (" << kCachedVersion << " != -1) {\n";
    out.Indent();
    out << "*_aidl_return = " << kCachedVersion << ";\n"
        << "_aidl_status.set(AStatus_fromStatus(_aidl_ret_status));\n"
        << "return _aidl_status;\n";
    out.Dedent();
    out << "}\n";
  }
  out << "::ndk::ScopedAParcel _aidl_in;\n";
  out << "::ndk::ScopedAParcel _aidl_out;\n";
  out << "\n";

  if (options.GenLog()) {
    out << cpp::GenLogBeforeExecute(ClassName(defined_type, ClassNames::CLIENT), method,
                                    false /* isServer */, true /* isNdk */);
  }
  if (options.GenTraces()) {
    out << "ScopedTrace _aidl_trace(\"AIDL::" << to_string(options.TargetLanguage())
        << "::" << ClassName(defined_type, ClassNames::INTERFACE) << "::" << method.GetName()
        << "::client\");\n";
  }

  out << "_aidl_ret_status = AIBinder_prepareTransaction(asBinder().get(), _aidl_in.getR());\n";
  if (defined_type.IsSensitiveData()) {
    out << "AParcel_markSensitive(_aidl_in.get());\n";
  }
  StatusCheckGoto(out);

  for (const auto& arg : method.GetArguments()) {
    const std::string var_name = cpp::BuildVarName(*arg);

    if (arg->IsIn()) {
      out << "_aidl_ret_status = ";
      const std::string prefix = (arg->IsOut() ? "*" : "");
      WriteToParcelFor({out, types, arg->GetType(), "_aidl_in.get()", prefix + var_name});
      out << ";\n";
      StatusCheckGoto(out);
    } else if (arg->IsOut() && arg->GetType().IsArray()) {
      out << "_aidl_ret_status = ::ndk::AParcel_writeVectorSize(_aidl_in.get(), *" << var_name
          << ");\n";
      StatusCheckGoto(out);
    }
  }
  out << "_aidl_ret_status = AIBinder_transact(\n";
  out.Indent();
  out << "asBinder().get(),\n";
  out << MethodId(method) << ",\n";
  out << "_aidl_in.getR(),\n";
  out << "_aidl_out.getR(),\n";

  std::vector<std::string> flags;
  if (method.IsOneway()) flags.push_back("FLAG_ONEWAY");
  if (defined_type.IsSensitiveData()) flags.push_back("FLAG_CLEAR_BUF");
  out << (flags.empty() ? "0" : base::Join(flags, " | ")) << "\n";

  out << "#ifdef BINDER_STABILITY_SUPPORT\n";
  out << "| FLAG_PRIVATE_LOCAL\n";
  out << "#endif  // BINDER_STABILITY_SUPPORT\n";
  out << ");\n";
  out.Dedent();

  // If the method is not implmented in the server side but the client has
  // provided the default implementation, call it instead of failing hard.
  const std::string iface = ClassName(defined_type, ClassNames::INTERFACE);
  out << "if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && ";
  out << iface << "::getDefaultImpl()) {\n";
  out.Indent();
  out << "_aidl_status = " << iface << "::getDefaultImpl()->" << method.GetName() << "(";
  out << NdkArgList(types, method, FormatArgNameOnly) << ");\n";
  out << "goto _aidl_status_return;\n";
  out.Dedent();
  out << "}\n";

  StatusCheckGoto(out);

  if (!method.IsOneway()) {
    out << "_aidl_ret_status = AParcel_readStatusHeader(_aidl_out.get(), _aidl_status.getR());\n";
    StatusCheckGoto(out);

    out << "if (!AStatus_isOk(_aidl_status.get())) goto _aidl_status_return;\n";
  }

  if (method.GetType().GetName() != "void") {
    out << "_aidl_ret_status = ";
    ReadFromParcelFor({out, types, method.GetType(), "_aidl_out.get()", "_aidl_return"});
    out << ";\n";
    StatusCheckGoto(out);
    if (method.GetName() == kGetInterfaceHash && !options.Hash().empty()) {
      out << kCachedHash << " = *_aidl_return;\n";
    } else if (method.GetName() == kGetInterfaceVersion && options.Version() > 0) {
      out << kCachedVersion << " = *_aidl_return;\n";
    }
  }
  for (const AidlArgument* arg : method.GetOutArguments()) {
    out << "_aidl_ret_status = ";
    ReadFromParcelFor({out, types, arg->GetType(), "_aidl_out.get()", cpp::BuildVarName(*arg)});
    out << ";\n";
    StatusCheckGoto(out);
  }

  out << "_aidl_error:\n";
  out << "_aidl_status.set(AStatus_fromStatus(_aidl_ret_status));\n";
  out << "_aidl_status_return:\n";
  if (options.GenLog()) {
    out << cpp::GenLogAfterExecute(ClassName(defined_type, ClassNames::CLIENT), defined_type,
                                   method, "_aidl_status", "_aidl_return", false /* isServer */,
                                   true /* isNdk */);
  }

  out << "return _aidl_status;\n";
  out.Dedent();
  out << "}\n";
}

static void GenerateServerCaseDefinition(CodeWriter& out, const AidlTypenames& types,
                                         const AidlInterface& defined_type,
                                         const AidlMethod& method, const Options& options) {
  out << "case " << MethodId(method) << ": {\n";
  out.Indent();
  for (const auto& arg : method.GetArguments()) {
    out << NdkNameOf(types, arg->GetType(), StorageMode::STACK) << " " << cpp::BuildVarName(*arg)
        << ";\n";
  }
  if (method.GetType().GetName() != "void") {
    out << NdkNameOf(types, method.GetType(), StorageMode::STACK) << " _aidl_return;\n";
  }
  out << "\n";
  if (options.GenTraces()) {
    out << "ScopedTrace _aidl_trace(\"AIDL::" << to_string(options.TargetLanguage())
        << "::" << ClassName(defined_type, ClassNames::INTERFACE) << "::" << method.GetName()
        << "::server\");\n";
  }

  for (const auto& arg : method.GetArguments()) {
    const std::string var_name = cpp::BuildVarName(*arg);

    if (arg->IsIn()) {
      out << "_aidl_ret_status = ";
      ReadFromParcelFor({out, types, arg->GetType(), "_aidl_in", "&" + var_name});
      out << ";\n";
      StatusCheckBreak(out);
    } else if (arg->IsOut() && arg->GetType().IsArray()) {
      out << "_aidl_ret_status = ::ndk::AParcel_resizeVector(_aidl_in, &" << var_name << ");\n";
      StatusCheckBreak(out);
    }
  }
  if (options.GenLog()) {
    out << cpp::GenLogBeforeExecute(ClassName(defined_type, ClassNames::SERVER), method,
                                    true /* isServer */, true /* isNdk */);
  }
  out << "::ndk::ScopedAStatus _aidl_status = _aidl_impl->" << method.GetName() << "("
      << NdkArgList(types, method, FormatArgForCall) << ");\n";

  if (options.GenLog()) {
    out << cpp::GenLogAfterExecute(ClassName(defined_type, ClassNames::SERVER), defined_type,
                                   method, "_aidl_status", "_aidl_return", true /* isServer */,
                                   true /* isNdk */);
  }
  if (method.IsOneway()) {
    // For a oneway transaction, the kernel will have already returned a result. This is for the
    // in-process case when a oneway transaction is parceled/unparceled in the same process.
    out << "_aidl_ret_status = STATUS_OK;\n";
  } else {
    out << "_aidl_ret_status = AParcel_writeStatusHeader(_aidl_out, _aidl_status.get());\n";
    StatusCheckBreak(out);

    out << "if (!AStatus_isOk(_aidl_status.get())) break;\n\n";

    if (method.GetType().GetName() != "void") {
      out << "_aidl_ret_status = ";
      WriteToParcelFor({out, types, method.GetType(), "_aidl_out", "_aidl_return"});
      out << ";\n";
      StatusCheckBreak(out);
    }
    for (const AidlArgument* arg : method.GetOutArguments()) {
      out << "_aidl_ret_status = ";
      WriteToParcelFor({out, types, arg->GetType(), "_aidl_out", cpp::BuildVarName(*arg)});
      out << ";\n";
      StatusCheckBreak(out);
    }
  }
  out << "break;\n";
  out.Dedent();
  out << "}\n";
}

void GenerateClassSource(CodeWriter& out, const AidlTypenames& types,
                         const AidlInterface& defined_type, const Options& options) {
  const std::string clazz = ClassName(defined_type, ClassNames::INTERFACE);
  const std::string bn_clazz = ClassName(defined_type, ClassNames::SERVER);
  if (options.GenTraces()) {
    out << "class ScopedTrace {\n";
    out.Indent();
    out << "public:\n"
        << "inline explicit ScopedTrace(const char* name) {\n"
        << "ATrace_beginSection(name);\n"
        << "}\n"
        << "inline ~ScopedTrace() {\n"
        << "ATrace_endSection();\n"
        << "}\n";
    out.Dedent();
    out << "};\n";
  }
  bool deprecated = defined_type.IsDeprecated() ||
                    std::any_of(defined_type.GetMethods().begin(), defined_type.GetMethods().end(),
                                [](const auto& m) { return m->IsDeprecated(); });
  if (deprecated) {
    out << "#pragma clang diagnostic push\n";
    out << "#pragma clang diagnostic ignored \"-Wdeprecated\"\n";
  }
  out << "static binder_status_t "
      << "_aidl_onTransact"
      << "(AIBinder* _aidl_binder, transaction_code_t _aidl_code, const AParcel* _aidl_in, "
         "AParcel* _aidl_out) {\n";
  out.Indent();
  out << "(void)_aidl_in;\n";
  out << "(void)_aidl_out;\n";
  out << "binder_status_t _aidl_ret_status = STATUS_UNKNOWN_TRANSACTION;\n";
  if (!defined_type.GetMethods().empty()) {
    // we know this cast is valid because this method is only called by the ICInterface
    // AIBinder_Class object which is associated with this class.
    out << "std::shared_ptr<" << bn_clazz << "> _aidl_impl = std::static_pointer_cast<" << bn_clazz
        << ">(::ndk::ICInterface::asInterface(_aidl_binder));\n";
    out << "switch (_aidl_code) {\n";
    out.Indent();
    for (const auto& method : defined_type.GetMethods()) {
      GenerateServerCaseDefinition(out, types, defined_type, *method, options);
    }
    out.Dedent();
    out << "}\n";
  } else {
    out << "(void)_aidl_binder;\n";
    out << "(void)_aidl_code;\n";
  }
  out << "return _aidl_ret_status;\n";
  out.Dedent();
  out << "}\n\n";

  out << "static AIBinder_Class* " << kClazz << " = ::ndk::ICInterface::defineClass(" << clazz
      << "::" << kDescriptor << ", _aidl_onTransact);\n\n";
  if (deprecated) {
    out << "#pragma clang diagnostic pop\n";
  }
}

void GenerateClientSource(CodeWriter& out, const AidlTypenames& types,
                          const AidlInterface& defined_type, const Options& options) {
  const std::string clazz = ClassName(defined_type, ClassNames::CLIENT);

  out << clazz << "::" << clazz << "(const ::ndk::SpAIBinder& binder) : BpCInterface(binder) {}\n";
  out << clazz << "::~" << clazz << "() {}\n";
  if (options.GenLog()) {
    out << "std::function<void(const " + clazz + "::TransactionLog&)> " << clazz << "::logFunc;\n";
  }
  out << "\n";
  for (const auto& method : defined_type.GetMethods()) {
    GenerateClientMethodDefinition(out, types, defined_type, *method, options);
  }
}
void GenerateServerSource(CodeWriter& out, const AidlTypenames& types,
                          const AidlInterface& defined_type, const Options& options) {
  const std::string clazz = ClassName(defined_type, ClassNames::SERVER);
  const std::string iface = ClassName(defined_type, ClassNames::INTERFACE);

  out << "// Source for " << clazz << "\n";
  out << clazz << "::" << clazz << "() {}\n";
  out << clazz << "::~" << clazz << "() {}\n";
  if (options.GenLog()) {
    out << "std::function<void(const " + clazz + "::TransactionLog&)> " << clazz << "::logFunc;\n";
  }
  out << "::ndk::SpAIBinder " << clazz << "::createBinder() {\n";
  out.Indent();
  out << "AIBinder* binder = AIBinder_new(" << kClazz << ", static_cast<void*>(this));\n";

  out << "#ifdef BINDER_STABILITY_SUPPORT\n";
  if (defined_type.IsVintfStability()) {
    out << "AIBinder_markVintfStability(binder);\n";
  } else {
    out << "AIBinder_markCompilationUnitStability(binder);\n";
  }
  out << "#endif  // BINDER_STABILITY_SUPPORT\n";

  out << "return ::ndk::SpAIBinder(binder);\n";
  out.Dedent();
  out << "}\n";

  // Implement the meta methods
  for (const auto& method : defined_type.GetMethods()) {
    if (method->IsUserDefined()) {
      continue;
    }
    if (method->GetName() == kGetInterfaceVersion && options.Version() > 0) {
      out << NdkMethodDecl(types, *method, clazz) << " {\n";
      out.Indent();
      out << "*_aidl_return = " << iface << "::" << kVersion << ";\n";
      out << "return ::ndk::ScopedAStatus(AStatus_newOk());\n";
      out.Dedent();
      out << "}\n";
    }
    if (method->GetName() == kGetInterfaceHash && !options.Hash().empty()) {
      out << NdkMethodDecl(types, *method, clazz) << " {\n";
      out.Indent();
      out << "*_aidl_return = " << iface << "::" << kHash << ";\n";
      out << "return ::ndk::ScopedAStatus(AStatus_newOk());\n";
      out.Dedent();
      out << "}\n";
    }
  }
}
void GenerateInterfaceSource(CodeWriter& out, const AidlTypenames& types,
                             const AidlInterface& defined_type, const Options& options) {
  const std::string clazz = ClassName(defined_type, ClassNames::INTERFACE);
  const std::string bp_clazz = ClassName(defined_type, ClassNames::CLIENT);

  out << "// Source for " << clazz << "\n";
  out << "const char* " << clazz << "::" << kDescriptor << " = \"" << defined_type.GetDescriptor()
      << "\";\n";
  out << clazz << "::" << clazz << "() {}\n";
  out << clazz << "::~" << clazz << "() {}\n";
  out << "\n";
  GenerateConstantDefinitions(out, defined_type, clazz);
  out << "\n";

  out << "std::shared_ptr<" << clazz << "> " << clazz
      << "::fromBinder(const ::ndk::SpAIBinder& binder) {\n";
  out.Indent();
  out << "if (!AIBinder_associateClass(binder.get(), " << kClazz << ")) { return nullptr; }\n";
  out << "std::shared_ptr<::ndk::ICInterface> interface = "
         "::ndk::ICInterface::asInterface(binder.get());\n";
  out << "if (interface) {\n";
  out.Indent();
  out << "return std::static_pointer_cast<" << clazz << ">(interface);\n";
  out.Dedent();
  out << "}\n";
  out << "return ::ndk::SharedRefBase::make<" << bp_clazz << ">(binder);\n";
  out.Dedent();
  out << "}\n\n";

  out << "binder_status_t " << clazz << "::writeToParcel(AParcel* parcel, const std::shared_ptr<"
      << clazz << ">& instance) {\n";
  out.Indent();
  out << "return AParcel_writeStrongBinder(parcel, instance ? instance->asBinder().get() : "
         "nullptr);\n";
  out.Dedent();
  out << "}\n";

  out << "binder_status_t " << clazz << "::readFromParcel(const AParcel* parcel, std::shared_ptr<"
      << clazz << ">* instance) {\n";
  out.Indent();
  out << "::ndk::SpAIBinder binder;\n";
  out << "binder_status_t status = AParcel_readStrongBinder(parcel, binder.getR());\n";
  out << "if (status != STATUS_OK) return status;\n";
  out << "*instance = " << clazz << "::fromBinder(binder);\n";
  out << "return STATUS_OK;\n";
  out.Dedent();
  out << "}\n";

  // defintion for static member setDefaultImpl
  out << "bool " << clazz << "::setDefaultImpl(const std::shared_ptr<" << clazz << ">& impl) {\n";
  out.Indent();
  out << "// Only one user of this interface can use this function\n";
  out << "// at a time. This is a heuristic to detect if two different\n";
  out << "// users in the same process use this function.\n";
  out << "assert(!" << clazz << "::default_impl);\n";
  out << "if (impl) {\n";
  out.Indent();
  out << clazz << "::default_impl = impl;\n";
  out << "return true;\n";
  out.Dedent();
  out << "}\n";
  out << "return false;\n";
  out.Dedent();
  out << "}\n";

  // definition for static member getDefaultImpl
  out << "const std::shared_ptr<" << clazz << ">& " << clazz << "::getDefaultImpl() {\n";
  out.Indent();
  out << "return " << clazz << "::default_impl;\n";
  out.Dedent();
  out << "}\n";

  // definition for the static field default_impl
  out << "std::shared_ptr<" << clazz << "> " << clazz << "::default_impl = nullptr;\n";

  // default implementation for the <Name>Default class members
  const std::string defaultClazz = clazz + "Default";
  for (const auto& method : defined_type.GetMethods()) {
    if (method->IsUserDefined()) {
      out << "::ndk::ScopedAStatus " << defaultClazz << "::" << method->GetName() << "("
          << NdkArgList(types, *method, FormatArgNameUnused) << ") {\n";
      out.Indent();
      out << "::ndk::ScopedAStatus _aidl_status;\n";
      out << "_aidl_status.set(AStatus_fromStatus(STATUS_UNKNOWN_TRANSACTION));\n";
      out << "return _aidl_status;\n";
      out.Dedent();
      out << "}\n";
    } else {
      if (method->GetName() == kGetInterfaceVersion && options.Version() > 0) {
        out << "::ndk::ScopedAStatus " << defaultClazz << "::" << method->GetName() << "("
            << "int32_t* _aidl_return) {\n";
        out.Indent();
        out << "*_aidl_return = 0;\n";
        out << "return ::ndk::ScopedAStatus(AStatus_newOk());\n";
        out.Dedent();
        out << "}\n";
      }
      if (method->GetName() == kGetInterfaceHash && !options.Hash().empty()) {
        out << "::ndk::ScopedAStatus " << defaultClazz << "::" << method->GetName() << "("
            << "std::string* _aidl_return) {\n";
        out.Indent();
        out << "*_aidl_return = \"\";\n";
        out << "return ::ndk::ScopedAStatus(AStatus_newOk());\n";
        out.Dedent();
        out << "}\n";
      }
    }
  }

  out << "::ndk::SpAIBinder " << defaultClazz << "::asBinder() {\n";
  out.Indent();
  out << "return ::ndk::SpAIBinder();\n";
  out.Dedent();
  out << "}\n";

  out << "bool " << defaultClazz << "::isRemote() {\n";
  out.Indent();
  out << "return false;\n";
  out.Dedent();
  out << "}\n";
}

void GenerateClientHeader(CodeWriter& out, const AidlTypenames& types,
                          const AidlInterface& defined_type, const Options& options) {
  const std::string clazz = ClassName(defined_type, ClassNames::CLIENT);

  out << "#pragma once\n\n";
  out << "#include \"" << NdkHeaderFile(defined_type, ClassNames::RAW, false /*use_os_sep*/)
      << "\"\n";
  out << "\n";
  out << "#include <android/binder_ibinder.h>\n";
  if (options.GenLog()) {
    out << "#include <functional>\n";
    out << "#include <chrono>\n";
    out << "#include <sstream>\n";
  }
  if (options.GenTraces()) {
    out << "#include <android/trace.h>\n";
  }
  out << "\n";
  EnterNdkNamespace(out, defined_type);
  out << "class";
  cpp::GenerateDeprecated(out, defined_type);
  out << " " << clazz << " : public ::ndk::BpCInterface<"
      << ClassName(defined_type, ClassNames::INTERFACE) << "> {\n";
  out << "public:\n";
  out.Indent();
  out << "explicit " << clazz << "(const ::ndk::SpAIBinder& binder);\n";
  out << "virtual ~" << clazz << "();\n";
  out << "\n";
  for (const auto& method : defined_type.GetMethods()) {
    out << NdkMethodDecl(types, *method) << " override";
    cpp::GenerateDeprecated(out, *method);
    out << ";\n";
  }

  if (options.Version() > 0) {
    out << "int32_t " << kCachedVersion << " = -1;\n";
  }

  if (!options.Hash().empty()) {
    out << "std::string " << kCachedHash << " = \"-1\";\n";
    out << "std::mutex " << kCachedHashMutex << ";\n";
  }
  if (options.GenLog()) {
    out << cpp::kTransactionLogStruct;
    out << "static std::function<void(const TransactionLog&)> logFunc;\n";
  }
  out.Dedent();
  out << "};\n";
  LeaveNdkNamespace(out, defined_type);
}
void GenerateServerHeader(CodeWriter& out, const AidlTypenames& types,
                          const AidlInterface& defined_type, const Options& options) {
  const std::string clazz = ClassName(defined_type, ClassNames::SERVER);
  const std::string iface = ClassName(defined_type, ClassNames::INTERFACE);

  out << "#pragma once\n\n";
  out << "#include \"" << NdkHeaderFile(defined_type, ClassNames::RAW, false /*use_os_sep*/)
      << "\"\n";
  out << "\n";
  out << "#include <android/binder_ibinder.h>\n";
  out << "\n";
  EnterNdkNamespace(out, defined_type);
  out << "class";
  cpp::GenerateDeprecated(out, defined_type);
  out << " " << clazz << " : public ::ndk::BnCInterface<" << iface << "> {\n";
  out << "public:\n";
  out.Indent();
  out << clazz << "();\n";
  out << "virtual ~" << clazz << "();\n";

  // Declare the meta methods
  for (const auto& method : defined_type.GetMethods()) {
    if (method->IsUserDefined()) {
      continue;
    }
    if (method->GetName() == kGetInterfaceVersion && options.Version() > 0) {
      out << NdkMethodDecl(types, *method) << " final;\n";
    } else if (method->GetName() == kGetInterfaceHash && !options.Hash().empty()) {
      out << NdkMethodDecl(types, *method) << " final;\n";
    } else {
      AIDL_FATAL(defined_type) << "Meta method '" << method->GetName() << "' is unimplemented.";
    }
  }
  if (options.GenLog()) {
    out << cpp::kTransactionLogStruct;
    out << "static std::function<void(const TransactionLog&)> logFunc;\n";
  }
  out.Dedent();
  out << "protected:\n";
  out.Indent();
  out << "::ndk::SpAIBinder createBinder() override;\n";
  out.Dedent();
  out << "private:\n";
  out.Indent();
  out.Dedent();
  out << "};\n";
  LeaveNdkNamespace(out, defined_type);
}
void GenerateInterfaceHeader(CodeWriter& out, const AidlTypenames& types,
                             const AidlInterface& defined_type, const Options& options) {
  const std::string clazz = ClassName(defined_type, ClassNames::INTERFACE);

  out << "#pragma once\n\n";
  out << "#include <android/binder_interface_utils.h>\n";
  if (options.GenLog()) {
    out << "#include <functional>\n";
    out << "#include <chrono>\n";
    out << "#include <sstream>\n";
  }
  out << "\n";

  GenerateHeaderIncludes(out, types, defined_type);
  out << "\n";

  EnterNdkNamespace(out, defined_type);
  out << "class";
  cpp::GenerateDeprecated(out, defined_type);
  out << " " << clazz << " : public ::ndk::ICInterface {\n";
  out << "public:\n";
  out.Indent();
  out << "static const char* " << kDescriptor << ";\n";
  out << clazz << "();\n";
  out << "virtual ~" << clazz << "();\n";
  out << "\n";
  GenerateConstantDeclarations(out, types, defined_type);
  if (options.Version() > 0) {
    out << "static const int32_t " << kVersion << " = " << std::to_string(options.Version())
        << ";\n";
  }
  if (!options.Hash().empty()) {
    out << "static inline const std::string " << kHash << " = \"" << options.Hash() << "\";\n";
  }
  for (const auto& method : defined_type.GetMethods()) {
    if (!method->IsUserDefined()) {
      continue;
    }
    out << "static constexpr uint32_t TRANSACTION_" << method->GetName() << " = "
        << "FIRST_CALL_TRANSACTION + " << std::to_string(method->GetId()) << ";\n";
  }
  out << "\n";
  out << "static std::shared_ptr<" << clazz << "> fromBinder(const ::ndk::SpAIBinder& binder);\n";
  out << "static binder_status_t writeToParcel(AParcel* parcel, const std::shared_ptr<" << clazz
      << ">& instance);";
  out << "\n";
  out << "static binder_status_t readFromParcel(const AParcel* parcel, std::shared_ptr<" << clazz
      << ">* instance);";
  out << "\n";
  out << "static bool setDefaultImpl(const std::shared_ptr<" << clazz << ">& impl);";
  out << "\n";
  out << "static const std::shared_ptr<" << clazz << ">& getDefaultImpl();";
  out << "\n";
  for (const auto& method : defined_type.GetMethods()) {
    out << "virtual " << NdkMethodDecl(types, *method);
    cpp::GenerateDeprecated(out, *method);
    out << " = 0;\n";
  }
  out.Dedent();
  out << "private:\n";
  out.Indent();
  out << "static std::shared_ptr<" << clazz << "> default_impl;\n";
  out.Dedent();
  out << "};\n";

  const std::string defaultClazz = clazz + "Default";
  out << "class";
  cpp::GenerateDeprecated(out, defined_type);
  out << " " << defaultClazz << " : public " << clazz << " {\n";
  out << "public:\n";
  out.Indent();
  for (const auto& method : defined_type.GetMethods()) {
    if (method->IsUserDefined()) {
      out << NdkMethodDecl(types, *method) << " override";
      cpp::GenerateDeprecated(out, *method);
      out << ";\n";
    } else if (method->GetName() == kGetInterfaceVersion && options.Version() > 0) {
      out << NdkMethodDecl(types, *method) << " override;\n";
    } else if (method->GetName() == kGetInterfaceHash && !options.Hash().empty()) {
      out << NdkMethodDecl(types, *method) << " override;\n";
    }
  }
  out << "::ndk::SpAIBinder asBinder() override;\n";
  out << "bool isRemote() override;\n";
  out.Dedent();
  out << "};\n";

  LeaveNdkNamespace(out, defined_type);
}
void GenerateParcelHeader(CodeWriter& out, const AidlTypenames& types,
                          const AidlStructuredParcelable& defined_type,
                          const Options& /*options*/) {
  const std::string clazz = ClassName(defined_type, ClassNames::RAW);

  out << "#pragma once\n";
  out << "#include <android/binder_interface_utils.h>\n";
  out << "#include <android/binder_parcelable_utils.h>\n";

  // used by toString()
  out << "#include <android/binder_to_string.h>\n";

  GenerateHeaderIncludes(out, types, defined_type);

  EnterNdkNamespace(out, defined_type);
  out << cpp::TemplateDecl(defined_type);
  out << "class";
  cpp::GenerateDeprecated(out, defined_type);
  out << " " << clazz << " {\n";
  out << "public:\n";
  out.Indent();
  if (defined_type.IsFixedSize()) {
    out << "typedef std::true_type fixed_size;\n";
  } else {
    out << "typedef std::false_type fixed_size;\n";
  }
  out << "static const char* descriptor;\n";
  out << "\n";
  for (const auto& variable : defined_type.GetFields()) {
    out << NdkNameOf(types, variable->GetType(), StorageMode::STACK);
    cpp::GenerateDeprecated(out, *variable);
    out << " " << variable->GetName();
    if (variable->GetType().GetName() == "ParcelableHolder") {
      out << "{::ndk::" << (defined_type.IsVintfStability() ? "STABILITY_VINTF" : "STABILITY_LOCAL")
          << "}";
    }
    if (defined_type.IsFixedSize()) {
      int alignment = NdkAlignmentOf(types, variable->GetType());
      if (alignment > 0) {
        out << " __attribute__((aligned (" << std::to_string(alignment) << ")))";
      }
    }
    if (variable->GetDefaultValue()) {
      out << " = " << variable->ValueString(ConstantValueDecorator);
    } else if (auto type = variable->GetType().GetDefinedType(); type) {
      if (auto enum_type = type->AsEnumDeclaration(); enum_type) {
        if (!variable->GetType().IsArray()) {
          // if an enum doesn't have explicit default value, do zero-initialization
          out << " = " << NdkNameOf(types, variable->GetType(), StorageMode::STACK) << "(0)";
        }
      }
    }
    out << ";\n";
  }
  out << "\n";
  out << "binder_status_t readFromParcel(const AParcel* parcel);\n";
  out << "binder_status_t writeToParcel(AParcel* parcel) const;\n";
  out << "\n";

  cpp::GenerateParcelableComparisonOperators(out, defined_type);

  out << "static const ::ndk::parcelable_stability_t _aidl_stability = ::ndk::"
      << (defined_type.IsVintfStability() ? "STABILITY_VINTF" : "STABILITY_LOCAL") << ";\n";

  GenerateConstantDeclarations(out, types, defined_type);
  cpp::GenerateToString(out, defined_type);

  out.Dedent();
  out << "};\n";
  LeaveNdkNamespace(out, defined_type);
}
void GenerateParcelSource(CodeWriter& out, const AidlTypenames& types,
                          const AidlStructuredParcelable& defined_type,
                          const Options& /*options*/) {
  std::string clazz = ClassName(defined_type, ClassNames::RAW);
  if (defined_type.IsGeneric()) {
    std::vector<std::string> template_params;
    for (const auto& parameter : defined_type.GetTypeParameters()) {
      template_params.push_back(parameter);
    }
    clazz += base::StringPrintf("<%s>", base::Join(template_params, ", ").c_str());
  }

  out << "#include \"" << NdkHeaderFile(defined_type, ClassNames::RAW, false /*use_os_sep*/)
      << "\"\n";
  out << "\n";
  GenerateSourceIncludes(out, types, defined_type);
  out << "\n";
  EnterNdkNamespace(out, defined_type);
  out << cpp::TemplateDecl(defined_type);
  out << "const char* " << clazz << "::" << kDescriptor << " = \""
      << defined_type.GetCanonicalName() << "\";\n";
  out << "\n";

  GenerateConstantDefinitions(out, defined_type, clazz, cpp::TemplateDecl(defined_type));

  out << cpp::TemplateDecl(defined_type);
  out << "binder_status_t " << clazz << "::readFromParcel(const AParcel* parcel) {\n";
  out.Indent();
  out << "int32_t _aidl_parcelable_size;\n";
  out << "int32_t _aidl_start_pos = AParcel_getDataPosition(parcel);\n";
  out << "binder_status_t _aidl_ret_status = AParcel_readInt32(parcel, &_aidl_parcelable_size);\n";
  out << "if (_aidl_start_pos > INT32_MAX - _aidl_parcelable_size) return STATUS_BAD_VALUE;\n";
  out << "if (_aidl_parcelable_size < 0) return STATUS_BAD_VALUE;\n";
  StatusCheckReturn(out);

  for (const auto& variable : defined_type.GetFields()) {
    out << "if (AParcel_getDataPosition(parcel) - _aidl_start_pos >= _aidl_parcelable_size) {\n"
        << "  AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);\n"
        << "  return _aidl_ret_status;\n"
        << "}\n";
    out << "_aidl_ret_status = ";
    ReadFromParcelFor({out, types, variable->GetType(), "parcel", "&" + variable->GetName()});
    out << ";\n";
    StatusCheckReturn(out);
  }
  out << "AParcel_setDataPosition(parcel, _aidl_start_pos + _aidl_parcelable_size);\n"
      << "return _aidl_ret_status;\n";
  out.Dedent();
  out << "}\n";

  out << cpp::TemplateDecl(defined_type);
  out << "binder_status_t " << clazz << "::writeToParcel(AParcel* parcel) const {\n";
  out.Indent();
  out << "binder_status_t _aidl_ret_status;\n";

  out << "size_t _aidl_start_pos = AParcel_getDataPosition(parcel);\n";
  out << "_aidl_ret_status = AParcel_writeInt32(parcel, 0);\n";
  StatusCheckReturn(out);

  for (const auto& variable : defined_type.GetFields()) {
    out << "_aidl_ret_status = ";
    WriteToParcelFor({out, types, variable->GetType(), "parcel", variable->GetName()});
    out << ";\n";
    StatusCheckReturn(out);
  }
  out << "size_t _aidl_end_pos = AParcel_getDataPosition(parcel);\n";
  out << "AParcel_setDataPosition(parcel, _aidl_start_pos);\n";
  out << "AParcel_writeInt32(parcel, _aidl_end_pos - _aidl_start_pos);\n";
  out << "AParcel_setDataPosition(parcel, _aidl_end_pos);\n";

  out << "return _aidl_ret_status;\n";
  out.Dedent();
  out << "}\n";
  out << "\n";
  LeaveNdkNamespace(out, defined_type);
}

void GenerateParcelHeader(CodeWriter& out, const AidlTypenames& types,
                          const AidlUnionDecl& defined_type, const Options& /*options*/) {
  const std::string clazz = ClassName(defined_type, ClassNames::RAW);
  cpp::UnionWriter uw{defined_type, types,
                      [&](const AidlTypeSpecifier& type, const AidlTypenames& types) {
                        return NdkNameOf(types, type, StorageMode::STACK);
                      },
                      &ConstantValueDecorator};

  out << "#pragma once\n";
  out << "#include <android/binder_interface_utils.h>\n";
  out << "#include <android/binder_parcelable_utils.h>\n";

  // used by toString()
  out << "#include <android/binder_to_string.h>\n";

  out << "\n";

  for (const auto& header : cpp::UnionWriter::headers) {
    out << "#include <" << header << ">\n";
  }
  GenerateHeaderIncludes(out, types, defined_type);

  // TODO(b/31559095) bionic on host should define this
  out << "\n";
  out << "#ifndef __BIONIC__\n";
  out << "#define __assert2(a,b,c,d) ((void)0)\n";
  out << "#endif\n";
  out << "\n";

  EnterNdkNamespace(out, defined_type);
  out << cpp::TemplateDecl(defined_type);
  out << "class";
  cpp::GenerateDeprecated(out, defined_type);
  out << " " << clazz << " {\n";
  out << "public:\n";
  out.Indent();
  if (defined_type.IsFixedSize()) {
    out << "typedef std::true_type fixed_size;\n";
  } else {
    out << "typedef std::false_type fixed_size;\n";
  }
  out << "static const char* descriptor;\n";
  out << "\n";
  uw.PublicFields(out);

  out << "binder_status_t readFromParcel(const AParcel* _parcel);\n";
  out << "binder_status_t writeToParcel(AParcel* _parcel) const;\n";
  out << "\n";

  cpp::GenerateParcelableComparisonOperators(out, defined_type);

  out << "static const ::ndk::parcelable_stability_t _aidl_stability = ::ndk::"
      << (defined_type.IsVintfStability() ? "STABILITY_VINTF" : "STABILITY_LOCAL") << ";\n";
  GenerateConstantDeclarations(out, types, defined_type);
  cpp::GenerateToString(out, defined_type);
  out.Dedent();
  out << "private:\n";
  out.Indent();
  uw.PrivateFields(out);
  out.Dedent();
  out << "};\n";
  LeaveNdkNamespace(out, defined_type);
}
void GenerateParcelSource(CodeWriter& out, const AidlTypenames& types,
                          const AidlUnionDecl& defined_type, const Options& /*options*/) {
  std::string clazz = ClassName(defined_type, ClassNames::RAW);
  if (defined_type.IsGeneric()) {
    std::vector<std::string> template_params;
    for (const auto& parameter : defined_type.GetTypeParameters()) {
      template_params.push_back(parameter);
    }
    clazz += base::StringPrintf("<%s>", base::Join(template_params, ", ").c_str());
  }

  cpp::UnionWriter uw{defined_type, types,
                      [&](const AidlTypeSpecifier& type, const AidlTypenames& types) {
                        return NdkNameOf(types, type, StorageMode::STACK);
                      },
                      &ConstantValueDecorator};
  cpp::ParcelWriterContext ctx{
      .status_type = "binder_status_t",
      .status_ok = "STATUS_OK",
      .status_bad = "STATUS_BAD_VALUE",
      .read_func =
          [&](CodeWriter& out, const std::string& var, const AidlTypeSpecifier& type) {
            ReadFromParcelFor({out, types, type, "_parcel", "&" + var});
          },
      .write_func =
          [&](CodeWriter& out, const std::string& value, const AidlTypeSpecifier& type) {
            WriteToParcelFor({out, types, type, "_parcel", value});
          },
  };

  out << "#include \"" << NdkHeaderFile(defined_type, ClassNames::RAW, false /*use_os_sep*/)
      << "\"\n";
  out << "\n";
  GenerateSourceIncludes(out, types, defined_type);
  out << "\n";
  EnterNdkNamespace(out, defined_type);
  out << cpp::TemplateDecl(defined_type);
  out << "const char* " << clazz << "::" << kDescriptor << " = \""
      << defined_type.GetCanonicalName() << "\";\n";
  out << "\n";

  GenerateConstantDefinitions(out, defined_type, clazz, cpp::TemplateDecl(defined_type));

  out << cpp::TemplateDecl(defined_type);
  out << "binder_status_t " << clazz << "::readFromParcel(const AParcel* _parcel) {\n";
  out.Indent();
  uw.ReadFromParcel(out, ctx);
  out.Dedent();
  out << "}\n";

  out << cpp::TemplateDecl(defined_type);
  out << "binder_status_t " << clazz << "::writeToParcel(AParcel* _parcel) const {\n";
  out.Indent();
  uw.WriteToParcel(out, ctx);
  out.Dedent();
  out << "}\n";
  out << "\n";
  LeaveNdkNamespace(out, defined_type);
}

std::string GenerateEnumToString(const AidlTypenames& typenames,
                                 const AidlEnumDeclaration& enum_decl) {
  std::ostringstream code;
  const std::string signature =
      "[[nodiscard]] static inline std::string toString(" + enum_decl.GetName() + " val)";
  if (enum_decl.IsDeprecated()) {
    code << signature;
    cpp::GenerateDeprecated(code, enum_decl);
    code << ";\n";
  }
  code << signature << " {\n";
  code << "  switch(val) {\n";
  std::set<std::string> unique_cases;
  for (const auto& enumerator : enum_decl.GetEnumerators()) {
    std::string c = enumerator->ValueString(enum_decl.GetBackingType(), ConstantValueDecorator);
    // Only add a case if its value has not yet been used in the switch
    // statement. C++ does not allow multiple cases with the same value, but
    // enums does allow this. In this scenario, the first declared
    // enumerator with the given value is printed.
    if (unique_cases.count(c) == 0) {
      unique_cases.insert(c);
      code << "  case " << enum_decl.GetName() << "::" << enumerator->GetName() << ":\n";
      code << "    return \"" << enumerator->GetName() << "\";\n";
    }
  }
  code << "  default:\n";
  code << "    return std::to_string(static_cast<"
       << NdkNameOf(typenames, enum_decl.GetBackingType(), StorageMode::STACK) << ">(val));\n";
  code << "  }\n";
  code << "}\n";
  return code.str();
}

void GenerateEnumHeader(CodeWriter& out, const AidlTypenames& types,
                        const AidlEnumDeclaration& enum_decl, const Options& /*options*/) {
  out << "#pragma once\n";
  out << "\n";

  GenerateHeaderIncludes(out, types, enum_decl);
  // enum specific headers
  out << "#include <array>\n";
  out << "#include <android/binder_enums.h>\n";

  EnterNdkNamespace(out, enum_decl);
  out << "enum class";
  cpp::GenerateDeprecated(out, enum_decl);
  out << " " << enum_decl.GetName() << " : "
      << NdkNameOf(types, enum_decl.GetBackingType(), StorageMode::STACK) << " {\n";
  out.Indent();
  for (const auto& enumerator : enum_decl.GetEnumerators()) {
    out << enumerator->GetName() << " = "
        << enumerator->ValueString(enum_decl.GetBackingType(), ConstantValueDecorator) << ",\n";
  }
  out.Dedent();
  out << "};\n";
  out << "\n";
  out << GenerateEnumToString(types, enum_decl);
  LeaveNdkNamespace(out, enum_decl);

  out << "namespace ndk {\n";
  out << "namespace internal {\n";
  out << cpp::GenerateEnumValues(enum_decl, {"aidl"});
  out << "}  // namespace internal\n";
  out << "}  // namespace android\n";
}

}  // namespace internals
}  // namespace ndk
}  // namespace aidl
}  // namespace android

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

#include "aidl.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#endif

#include <android-base/strings.h>

#include "aidl_checkapi.h"
#include "aidl_dumpapi.h"
#include "aidl_language.h"
#include "aidl_typenames.h"
#include "generate_aidl_mappings.h"
#include "generate_cpp.h"
#include "generate_java.h"
#include "generate_ndk.h"
#include "generate_rust.h"
#include "import_resolver.h"
#include "logging.h"
#include "options.h"
#include "os.h"
#include "parser.h"

#ifndef O_BINARY
#  define O_BINARY  0
#endif

using android::base::Join;
using android::base::Split;
using std::set;
using std::string;
using std::unique_ptr;
using std::unordered_set;
using std::vector;

namespace android {
namespace aidl {
namespace {

// Copied from android.is.IBinder.[FIRST|LAST]_CALL_TRANSACTION
const int kFirstCallTransaction = 1;
const int kLastCallTransaction = 0x00ffffff;

// Following IDs are all offsets from  kFirstCallTransaction

// IDs for meta transactions. Most of the meta transactions are implemented in
// the framework side (Binder.java or Binder.cpp). But these are the ones that
// are auto-implemented by the AIDL compiler.
const int kFirstMetaMethodId = kLastCallTransaction - kFirstCallTransaction;
const int kGetInterfaceVersionId = kFirstMetaMethodId;
const int kGetInterfaceHashId = kFirstMetaMethodId - 1;
// Additional meta transactions implemented by AIDL should use
// kFirstMetaMethodId -1, -2, ...and so on.

// Reserve 100 IDs for meta methods, which is more than enough. If we don't reserve,
// in the future, a newly added meta transaction ID will have a chance to
// collide with the user-defined methods that were added in the past. So,
// let's prevent users from using IDs in this range from the beginning.
const int kLastMetaMethodId = kFirstMetaMethodId - 99;

// Range of IDs that is allowed for user-defined methods.
const int kMinUserSetMethodId = 0;
const int kMaxUserSetMethodId = kLastMetaMethodId - 1;

bool check_filename(const std::string& filename, const AidlDefinedType& defined_type) {
    const char* p;
    string expected;
    string fn;
    size_t len;
    bool valid = false;

    if (!IoDelegate::GetAbsolutePath(filename, &fn)) {
      return false;
    }

    const std::string package = defined_type.GetPackage();
    if (!package.empty()) {
        expected = package;
        expected += '.';
    }

    len = expected.length();
    for (size_t i=0; i<len; i++) {
        if (expected[i] == '.') {
            expected[i] = OS_PATH_SEPARATOR;
        }
    }

    const std::string name = defined_type.GetName();
    expected.append(name, 0, name.find('.'));

    expected += ".aidl";

    len = fn.length();
    valid = (len >= expected.length());

    if (valid) {
        p = fn.c_str() + (len - expected.length());

#ifdef _WIN32
        if (OS_PATH_SEPARATOR != '/') {
            // Input filename under cygwin most likely has / separators
            // whereas the expected string uses \\ separators. Adjust
            // them accordingly.
          for (char *c = const_cast<char *>(p); *c; ++c) {
                if (*c == '/') *c = OS_PATH_SEPARATOR;
            }
        }
#endif

        // aidl assumes case-insensitivity on Mac Os and Windows.
#if defined(__linux__)
        valid = (expected == p);
#else
        valid = !strcasecmp(expected.c_str(), p);
#endif
    }

    if (!valid) {
      AIDL_ERROR(defined_type) << name << " should be declared in a file called " << expected;
    }

    return valid;
}

bool write_dep_file(const Options& options, const AidlDefinedType& defined_type,
                    const vector<string>& imports, const IoDelegate& io_delegate,
                    const string& input_file, const string& output_file) {
  string dep_file_name = options.DependencyFile();
  if (dep_file_name.empty() && options.AutoDepFile()) {
    dep_file_name = output_file + ".d";
  }

  if (dep_file_name.empty()) {
    return true;  // nothing to do
  }

  CodeWriterPtr writer = io_delegate.GetCodeWriter(dep_file_name);
  if (!writer) {
    AIDL_ERROR(dep_file_name) << "Could not open dependency file.";
    return false;
  }

  vector<string> source_aidl = {input_file};
  for (const auto& import : imports) {
    source_aidl.push_back(import);
  }

  // Encode that the output file depends on aidl input files.
  if (defined_type.AsUnstructuredParcelable() != nullptr &&
      options.TargetLanguage() == Options::Language::JAVA) {
    // Legacy behavior. For parcelable declarations in Java, don't emit output file as
    // the dependency target. b/141372861
    writer->Write(" : \\\n");
  } else {
    writer->Write("%s : \\\n", output_file.c_str());
  }
  writer->Write("  %s", Join(source_aidl, " \\\n  ").c_str());
  writer->Write("\n");

  if (!options.DependencyFileNinja()) {
    writer->Write("\n");
    // Output "<input_aidl_file>: " so make won't fail if the input .aidl file
    // has been deleted, moved or renamed in incremental build.
    for (const auto& src : source_aidl) {
      writer->Write("%s :\n", src.c_str());
    }
  }

  if (options.IsCppOutput()) {
    if (!options.DependencyFileNinja()) {
      using ::android::aidl::cpp::ClassNames;
      using ::android::aidl::cpp::HeaderFile;
      vector<string> headers;
      for (ClassNames c : {ClassNames::CLIENT, ClassNames::SERVER, ClassNames::RAW}) {
        headers.push_back(options.OutputHeaderDir() +
                          HeaderFile(defined_type, c, false /* use_os_sep */));
      }

      writer->Write("\n");

      // Generated headers also depend on the source aidl files.
      writer->Write("%s : \\\n    %s\n", Join(headers, " \\\n    ").c_str(),
                    Join(source_aidl, " \\\n    ").c_str());
    }
  }

  return true;
}

// Returns the path to the destination file of `defined_type`.
string GetOutputFilePath(const Options& options, const AidlDefinedType& defined_type) {
  string result = options.OutputDir();

  // add the package
  string package = defined_type.GetPackage();
  if (!package.empty()) {
    for (auto& c : package) {
      if (c == '.') {
        c = OS_PATH_SEPARATOR;
      }
    }
    result += package;
    result += OS_PATH_SEPARATOR;
  }

  // add the filename
  result += defined_type.GetName();
  if (options.TargetLanguage() == Options::Language::JAVA) {
    result += ".java";
  } else if (options.IsCppOutput()) {
    result += ".cpp";
  } else if (options.TargetLanguage() == Options::Language::RUST) {
    result += ".rs";
  } else {
    AIDL_FATAL("Unknown target language");
    return "";
  }

  return result;
}

bool check_and_assign_method_ids(const std::vector<std::unique_ptr<AidlMethod>>& items) {
  // Check whether there are any methods with manually assigned id's and any
  // that are not. Either all method id's must be manually assigned or all of
  // them must not. Also, check for uplicates of user set ID's and that the
  // ID's are within the proper bounds.
  set<int> usedIds;
  bool hasUnassignedIds = false;
  bool hasAssignedIds = false;
  int newId = kMinUserSetMethodId;
  for (const auto& item : items) {
    // However, meta transactions that are added by the AIDL compiler are
    // exceptions. They have fixed IDs but allowed to be with user-defined
    // methods having auto-assigned IDs. This is because the Ids of the meta
    // transactions must be stable during the entire lifetime of an interface.
    // In other words, their IDs must be the same even when new user-defined
    // methods are added.
    if (!item->IsUserDefined()) {
      continue;
    }
    if (item->HasId()) {
      hasAssignedIds = true;
    } else {
      item->SetId(newId++);
      hasUnassignedIds = true;
    }

    if (hasAssignedIds && hasUnassignedIds) {
      AIDL_ERROR(item) << "You must either assign id's to all methods or to none of them.";
      return false;
    }

    // Ensure that the user set id is not duplicated.
    if (usedIds.find(item->GetId()) != usedIds.end()) {
      // We found a duplicate id, so throw an error.
      AIDL_ERROR(item) << "Found duplicate method id (" << item->GetId() << ") for method "
                       << item->GetName();
      return false;
    }
    usedIds.insert(item->GetId());

    // Ensure that the user set id is within the appropriate limits
    if (item->GetId() < kMinUserSetMethodId || item->GetId() > kMaxUserSetMethodId) {
      AIDL_ERROR(item) << "Found out of bounds id (" << item->GetId() << ") for method "
                       << item->GetName() << ". Value for id must be between "
                       << kMinUserSetMethodId << " and " << kMaxUserSetMethodId << " inclusive.";
      return false;
    }
  }

  return true;
}

// TODO: Remove this in favor of using the YACC parser b/25479378
bool ParsePreprocessedLine(const string& line, string* decl, std::string* package,
                           string* class_name) {
  // erase all trailing whitespace and semicolons
  const size_t end = line.find_last_not_of(" ;\t");
  if (end == string::npos) {
    return false;
  }
  if (line.rfind(';', end) != string::npos) {
    return false;
  }

  decl->clear();
  string type;
  vector<string> pieces = Split(line.substr(0, end + 1), " \t");
  for (const string& piece : pieces) {
    if (piece.empty()) {
      continue;
    }
    if (decl->empty()) {
      *decl = std::move(piece);
    } else if (type.empty()) {
      type = std::move(piece);
    } else {
      return false;
    }
  }

  // Note that this logic is absolutely wrong.  Given a parcelable
  // org.some.Foo.Bar, the class name is Foo.Bar, but this code will claim that
  // the class is just Bar.  However, this was the way it was done in the past.
  //
  // See b/17415692
  size_t dot_pos = type.rfind('.');
  if (dot_pos != string::npos) {
    *class_name = type.substr(dot_pos + 1);
    *package = type.substr(0, dot_pos);
  } else {
    *class_name = type;
    package->clear();
  }

  return true;
}

bool ValidateAnnotationContext(const AidlDocument& doc) {
  struct AnnotationValidator : AidlVisitor {
    bool success = true;

    void Check(const AidlAnnotatable& annotatable, AidlAnnotation::TargetContext context) {
      for (const auto& annot : annotatable.GetAnnotations()) {
        if (!annot.CheckContext(context)) {
          success = false;
        }
      }
    }
    void Visit(const AidlInterface& m) override {
      Check(m, AidlAnnotation::CONTEXT_TYPE_INTERFACE);
    }
    void Visit(const AidlParcelable& m) override {
      Check(m, AidlAnnotation::CONTEXT_TYPE_UNSTRUCTURED_PARCELABLE);
    }
    void Visit(const AidlStructuredParcelable& m) override {
      Check(m, AidlAnnotation::CONTEXT_TYPE_STRUCTURED_PARCELABLE);
    }
    void Visit(const AidlEnumDeclaration& m) override {
      Check(m, AidlAnnotation::CONTEXT_TYPE_ENUM);
    }
    void Visit(const AidlUnionDecl& m) override { Check(m, AidlAnnotation::CONTEXT_TYPE_UNION); }
    void Visit(const AidlMethod& m) override {
      Check(m.GetType(), AidlAnnotation::CONTEXT_TYPE_SPECIFIER | AidlAnnotation::CONTEXT_METHOD);
      for (const auto& arg : m.GetArguments()) {
        Check(arg->GetType(), AidlAnnotation::CONTEXT_TYPE_SPECIFIER);
      }
    }
    void Visit(const AidlConstantDeclaration& m) override {
      Check(m.GetType(), AidlAnnotation::CONTEXT_TYPE_SPECIFIER | AidlAnnotation::CONTEXT_CONST);
    }
    void Visit(const AidlVariableDeclaration& m) override {
      Check(m.GetType(), AidlAnnotation::CONTEXT_TYPE_SPECIFIER | AidlAnnotation::CONTEXT_FIELD);
    }
    void Visit(const AidlTypeSpecifier& m) override {
      // nested generic type parameters are checked as well
      if (m.IsGeneric()) {
        for (const auto& tp : m.GetTypeParameters()) {
          Check(*tp, AidlAnnotation::CONTEXT_TYPE_SPECIFIER);
        }
      }
    }
  };

  AnnotationValidator validator;
  VisitTopDown(validator, doc);
  return validator.success;
}

}  // namespace

namespace internals {

bool parse_preprocessed_file(const IoDelegate& io_delegate, const string& filename,
                             AidlTypenames* typenames) {
  bool success = true;
  unique_ptr<LineReader> line_reader = io_delegate.GetLineReader(filename);
  if (!line_reader) {
    AIDL_ERROR(filename) << "cannot open preprocessed file";
    success = false;
    return success;
  }

  string line;
  int lineno = 1;
  for ( ; line_reader->ReadLine(&line); ++lineno) {
    if (line.empty() || line.compare(0, 2, "//") == 0) {
      // skip comments and empty lines
      continue;
    }

    string decl;
    std::string package;
    string class_name;
    if (!ParsePreprocessedLine(line, &decl, &package, &class_name)) {
      success = false;
      break;
    }

    AidlLocation::Point point = {.line = lineno, .column = 0 /*column*/};
    AidlLocation location = AidlLocation(filename, point, point, AidlLocation::Source::EXTERNAL);

    if (decl == "parcelable") {
      // ParcelFileDescriptor is treated as a built-in type, but it's also in the framework.aidl.
      // So aidl should ignore built-in types in framework.aidl to prevent duplication.
      // (b/130899491)
      if (AidlTypenames::IsBuiltinTypename(class_name)) {
        continue;
      }
      AidlParcelable* doc = new AidlParcelable(location, class_name, package, Comments{});
      typenames->AddPreprocessedType(unique_ptr<AidlParcelable>(doc));
    } else if (decl == "structured_parcelable") {
      AidlStructuredParcelable* doc =
          new AidlStructuredParcelable(location, class_name, package, Comments{}, nullptr, nullptr);
      typenames->AddPreprocessedType(unique_ptr<AidlStructuredParcelable>(doc));
    } else if (decl == "interface") {
      AidlInterface* doc =
          new AidlInterface(location, class_name, Comments{}, false, package, nullptr);
      typenames->AddPreprocessedType(unique_ptr<AidlInterface>(doc));
    } else {
      success = false;
      break;
    }
  }
  if (!success) {
    AIDL_ERROR(filename) << " on line " << lineno << " malformed preprocessed file line: '" << line
                         << "'";
  }

  return success;
}

AidlError load_and_validate_aidl(const std::string& input_file_name, const Options& options,
                                 const IoDelegate& io_delegate, AidlTypenames* typenames,
                                 vector<string>* imported_files) {
  AidlError err = AidlError::OK;

  //////////////////////////////////////////////////////////////////////////
  // Loading phase
  //////////////////////////////////////////////////////////////////////////

  // Parse the main input file
  std::unique_ptr<Parser> main_parser = Parser::Parse(input_file_name, io_delegate, *typenames);
  if (main_parser == nullptr) {
    return AidlError::PARSE_ERROR;
  }
  int num_top_level_decls = 0;
  for (const auto& type : main_parser->ParsedDocument().DefinedTypes()) {
    if (type->AsUnstructuredParcelable() == nullptr) {
      num_top_level_decls++;
      if (num_top_level_decls > 1) {
        AIDL_ERROR(*type) << "You must declare only one type per file.";
        return AidlError::BAD_TYPE;
      }
    }
  }

  // Import the preprocessed file
  for (const string& s : options.PreprocessedFiles()) {
    if (!parse_preprocessed_file(io_delegate, s, typenames)) {
      err = AidlError::BAD_PRE_PROCESSED_FILE;
    }
  }
  if (err != AidlError::OK) {
    return err;
  }

  // Find files to import and parse them
  vector<string> import_paths;
  ImportResolver import_resolver{io_delegate, input_file_name, options.ImportDirs(),
                                 options.InputFiles()};
  for (const auto& import : main_parser->ParsedDocument().Imports()) {
    if (AidlTypenames::IsBuiltinTypename(import->GetNeededClass())) {
      continue;
    }
    if (typenames->IsIgnorableImport(import->GetNeededClass())) {
      // There are places in the Android tree where an import doesn't resolve,
      // but we'll pick the type up through the preprocessed types.
      // This seems like an error, but legacy support demands we support it...
      continue;
    }
    string import_path = import_resolver.FindImportFile(import->GetNeededClass());
    if (import_path.empty()) {
      if (typenames->ResolveTypename(import->GetNeededClass()).is_resolved) {
        // This could happen when the type is from the preprocessed aidl file.
        // In that case, use the type from preprocessed aidl file
        continue;
      }
      AIDL_ERROR(input_file_name) << "Couldn't find import for class " << import->GetNeededClass();
      err = AidlError::BAD_IMPORT;
      continue;
    }

    import_paths.emplace_back(import_path);

    std::unique_ptr<Parser> import_parser = Parser::Parse(import_path, io_delegate, *typenames);
    if (import_parser == nullptr) {
      AIDL_ERROR(import_path) << "error while importing " << import_path << " for " << import;
      err = AidlError::BAD_IMPORT;
      continue;
    }
  }
  if (err != AidlError::OK) {
    return err;
  }

  for (const auto& imported_file : options.ImportFiles()) {
    import_paths.emplace_back(imported_file);

    std::unique_ptr<Parser> import_parser = Parser::Parse(imported_file, io_delegate, *typenames);
    if (import_parser == nullptr) {
      AIDL_ERROR(imported_file) << "error while importing " << imported_file;
      err = AidlError::BAD_IMPORT;
      continue;
    }
  }
  if (err != AidlError::OK) {
    return err;
  }

  TypeResolver resolver = [&](const AidlDocument* doc, AidlTypeSpecifier* type) {
    if (type->Resolve(*typenames)) return true;

    const string unresolved_name = type->GetUnresolvedName();
    const std::optional<string> canonical_name = doc->ResolveName(unresolved_name);
    if (!canonical_name) {
      return false;
    }
    const string import_path = import_resolver.FindImportFile(*canonical_name);
    if (import_path.empty()) {
      return false;
    }
    import_paths.push_back(import_path);

    std::unique_ptr<Parser> import_parser = Parser::Parse(import_path, io_delegate, *typenames);
    if (import_parser == nullptr) {
      AIDL_ERROR(import_path) << "error while importing " << import_path << " for " << import_path;
      return false;
    }
    if (!type->Resolve(*typenames)) {
      AIDL_ERROR(type) << "Can't resolve " << type->GetName();
      return false;
    }
    return true;
  };
  const bool is_check_api = options.GetTask() == Options::Task::CHECK_API;
  const bool is_dump_api = options.GetTask() == Options::Task::DUMP_API;

  // Resolve the unresolved type references found from the input file
  if (!is_check_api && !main_parser->Resolve(resolver)) {
    // Resolution is not need for check api because all typespecs are
    // using fully qualified names.
    return AidlError::BAD_TYPE;
  }

  if (!typenames->Autofill()) {
    return AidlError::BAD_TYPE;
  }

  //////////////////////////////////////////////////////////////////////////
  // Validation phase
  //////////////////////////////////////////////////////////////////////////

  // For legacy reasons, by default, compiling an unstructured parcelable (which contains no output)
  // is allowed. This must not be returned as an error until the very end of this procedure since
  // this may be considered a success, and we should first check that there are not other, more
  // serious failures.
  bool contains_unstructured_parcelable = false;

  const auto& types = main_parser->ParsedDocument().DefinedTypes();
  const int num_defined_types = types.size();
  for (const auto& defined_type : types) {
    AIDL_FATAL_IF(defined_type == nullptr, main_parser->FileName());

    // Ensure type is exactly one of the following:
    AidlInterface* interface = defined_type->AsInterface();
    AidlStructuredParcelable* parcelable = defined_type->AsStructuredParcelable();
    AidlParcelable* unstructured_parcelable = defined_type->AsUnstructuredParcelable();
    AidlEnumDeclaration* enum_decl = defined_type->AsEnumDeclaration();
    AidlUnionDecl* union_decl = defined_type->AsUnionDeclaration();
    AIDL_FATAL_IF(
        !!interface + !!parcelable + !!unstructured_parcelable + !!enum_decl + !!union_decl != 1,
        defined_type);

    // Ensure that foo.bar.IFoo is defined in <some_path>/foo/bar/IFoo.aidl
    if (num_defined_types == 1 && !check_filename(input_file_name, *defined_type)) {
      return AidlError::BAD_PACKAGE;
    }

    {
      bool valid_type = true;

      if (!is_check_api) {
        // Ideally, we could do this for check api, but we can't resolve imports
        if (!defined_type->CheckValid(*typenames)) {
          valid_type = false;
        }
      }

      if (!is_dump_api && !is_check_api) {
        if (!defined_type->LanguageSpecificCheckValid(*typenames, options.TargetLanguage())) {
          valid_type = false;
        }
      }

      if (!valid_type) {
        return AidlError::BAD_TYPE;
      }
    }

    if (unstructured_parcelable != nullptr) {
      bool isStable = unstructured_parcelable->IsStableApiParcelable(options.TargetLanguage());
      if (options.IsStructured() && !isStable) {
        AIDL_ERROR(unstructured_parcelable)
            << "Cannot declared parcelable in a --structured interface. Parcelable must be defined "
               "in AIDL directly.";
        return AidlError::NOT_STRUCTURED;
      }
      if (options.FailOnParcelable()) {
        AIDL_ERROR(unstructured_parcelable)
            << "Refusing to generate code with unstructured parcelables. Declared parcelables "
               "should be in their own file and/or cannot be used with --structured interfaces.";
        // Continue parsing for more errors
      }

      contains_unstructured_parcelable = true;
    }

    if (defined_type->IsVintfStability()) {
      bool success = true;
      if (options.GetStability() != Options::Stability::VINTF) {
        AIDL_ERROR(defined_type)
            << "Must compile @VintfStability type w/ aidl_interface 'stability: \"vintf\"'";
        success = false;
      }
      if (!options.IsStructured()) {
        AIDL_ERROR(defined_type)
            << "Must compile @VintfStability type w/ aidl_interface --structured";
        success = false;
      }
      if (!success) return AidlError::NOT_STRUCTURED;
    }

    if (interface != nullptr) {
      // add the meta-method 'int getInterfaceVersion()' if version is specified.
      if (options.Version() > 0) {
        AidlTypeSpecifier* ret =
            new AidlTypeSpecifier(AIDL_LOCATION_HERE, "int", false, nullptr, Comments{});
        ret->Resolve(*typenames);
        vector<unique_ptr<AidlArgument>>* args = new vector<unique_ptr<AidlArgument>>();
        auto method = std::make_unique<AidlMethod>(
            AIDL_LOCATION_HERE, false, ret, "getInterfaceVersion", args, Comments{},
            kGetInterfaceVersionId, false /* is_user_defined */);
        interface->AddMethod(std::move(method));
      }
      // add the meta-method 'string getInterfaceHash()' if hash is specified.
      if (!options.Hash().empty()) {
        AidlTypeSpecifier* ret =
            new AidlTypeSpecifier(AIDL_LOCATION_HERE, "String", false, nullptr, Comments{});
        ret->Resolve(*typenames);
        vector<unique_ptr<AidlArgument>>* args = new vector<unique_ptr<AidlArgument>>();
        auto method = std::make_unique<AidlMethod>(
            AIDL_LOCATION_HERE, false, ret, kGetInterfaceHash, args, Comments{},
            kGetInterfaceHashId, false /* is_user_defined */);
        interface->AddMethod(std::move(method));
      }
      if (!check_and_assign_method_ids(interface->GetMethods())) {
        return AidlError::BAD_METHOD_ID;
      }
    }
    // Verify the var/const declarations.
    // const expressions should be non-empty when evaluated with the var/const type.
    if (!is_check_api) {
      for (const auto& constant : defined_type->GetConstantDeclarations()) {
        if (constant->ValueString(AidlConstantValueDecorator).empty()) {
          return AidlError::BAD_TYPE;
        }
      }
      for (const auto& var : defined_type->GetFields()) {
        if (var->GetDefaultValue() && var->ValueString(AidlConstantValueDecorator).empty()) {
          return AidlError::BAD_TYPE;
        }
      }
    }
  }

  if (!ValidateAnnotationContext(main_parser->ParsedDocument())) {
    return AidlError::BAD_TYPE;
  }

  if (!is_check_api && !Diagnose(main_parser->ParsedDocument(), options.GetDiagnosticMapping())) {
    return AidlError::BAD_TYPE;
  }

  typenames->IterateTypes([&](const AidlDefinedType& type) {
    if (options.IsStructured() && type.AsUnstructuredParcelable() != nullptr &&
        !type.AsUnstructuredParcelable()->IsStableApiParcelable(options.TargetLanguage())) {
      err = AidlError::NOT_STRUCTURED;
      AIDL_ERROR(type) << type.GetCanonicalName()
                       << " is not structured, but this is a structured interface.";
    }
    if (options.GetStability() == Options::Stability::VINTF && !type.IsVintfStability()) {
      err = AidlError::NOT_STRUCTURED;
      AIDL_ERROR(type) << type.GetCanonicalName()
                       << " does not have VINTF level stability, but this interface requires it.";
    }

    // Ensure that untyped List/Map is not used in a parcelable, a union and a stable interface.

    std::function<void(const AidlTypeSpecifier&, const AidlNode*)> check_untyped_container =
        [&err, &check_untyped_container](const AidlTypeSpecifier& type, const AidlNode* node) {
          if (type.IsGeneric()) {
            std::for_each(type.GetTypeParameters().begin(), type.GetTypeParameters().end(),
                          [&node, &check_untyped_container](auto& nested) {
                            check_untyped_container(*nested, node);
                          });
            return;
          }
          if (type.GetName() == "List" || type.GetName() == "Map") {
            err = AidlError::BAD_TYPE;
            AIDL_ERROR(node)
                << "Encountered an untyped List or Map. The use of untyped List/Map is prohibited "
                << "because it is not guaranteed that the objects in the list are recognizable in "
                << "the receiving side. Consider switching to an array or a generic List/Map.";
          }
        };

    if (type.AsInterface() && options.IsStructured()) {
      for (const auto& method : type.GetMethods()) {
        check_untyped_container(method->GetType(), method.get());
        for (const auto& arg : method->GetArguments()) {
          check_untyped_container(arg->GetType(), method.get());
        }
      }
    }
    for (const auto& field : type.GetFields()) {
      check_untyped_container(field->GetType(), field.get());
    }
  });

  if (err != AidlError::OK) {
    return err;
  }

  if (imported_files != nullptr) {
    *imported_files = import_paths;
  }

  if (contains_unstructured_parcelable) {
    // Considered a success for the legacy case, so this must be returned last.
    return AidlError::FOUND_PARCELABLE;
  }

  return AidlError::OK;
}

} // namespace internals

int compile_aidl(const Options& options, const IoDelegate& io_delegate) {
  const Options::Language lang = options.TargetLanguage();
  for (const string& input_file : options.InputFiles()) {
    AidlTypenames typenames;

    vector<string> imported_files;

    AidlError aidl_err = internals::load_and_validate_aidl(input_file, options, io_delegate,
                                                           &typenames, &imported_files);
    bool allowError = aidl_err == AidlError::FOUND_PARCELABLE && !options.FailOnParcelable();
    if (aidl_err != AidlError::OK && !allowError) {
      return 1;
    }

    for (const auto& defined_type : typenames.MainDocument().DefinedTypes()) {
      AIDL_FATAL_IF(defined_type == nullptr, input_file);

      string output_file_name = options.OutputFile();
      // if needed, generate the output file name from the base folder
      if (output_file_name.empty() && !options.OutputDir().empty()) {
        output_file_name = GetOutputFilePath(options, *defined_type);
        if (output_file_name.empty()) {
          return 1;
        }
      }

      if (!write_dep_file(options, *defined_type, imported_files, io_delegate, input_file,
                          output_file_name)) {
        return 1;
      }

      bool success = false;
      if (lang == Options::Language::CPP) {
        success =
            cpp::GenerateCpp(output_file_name, options, typenames, *defined_type, io_delegate);
      } else if (lang == Options::Language::NDK) {
        ndk::GenerateNdk(output_file_name, options, typenames, *defined_type, io_delegate);
        success = true;
      } else if (lang == Options::Language::JAVA) {
        if (defined_type->AsUnstructuredParcelable() != nullptr) {
          // Legacy behavior. For parcelable declarations in Java, don't generate output file.
          success = true;
        } else {
          success = java::generate_java(output_file_name, defined_type.get(), typenames,
                                        io_delegate, options);
        }
      } else if (lang == Options::Language::RUST) {
        success = rust::GenerateRust(output_file_name, defined_type.get(), typenames, io_delegate,
                                     options);
      } else {
        AIDL_FATAL(input_file) << "Should not reach here.";
      }
      if (!success) {
        return 1;
      }
    }
  }
  return 0;
}

bool dump_mappings(const Options& options, const IoDelegate& io_delegate) {
  android::aidl::mappings::SignatureMap all_mappings;
  for (const string& input_file : options.InputFiles()) {
    AidlTypenames typenames;
    vector<string> imported_files;

    AidlError aidl_err = internals::load_and_validate_aidl(input_file, options, io_delegate,
                                                           &typenames, &imported_files);
    if (aidl_err != AidlError::OK) {
      return false;
    }
    for (const auto& defined_type : typenames.MainDocument().DefinedTypes()) {
      auto mappings = mappings::generate_mappings(defined_type.get(), typenames);
      all_mappings.insert(mappings.begin(), mappings.end());
    }
  }
  std::stringstream mappings_str;
  for (const auto& mapping : all_mappings) {
    mappings_str << mapping.first << "\n" << mapping.second << "\n";
  }
  auto code_writer = io_delegate.GetCodeWriter(options.OutputFile());
  code_writer->Write("%s", mappings_str.str().c_str());
  return true;
}

bool preprocess_aidl(const Options& options, const IoDelegate& io_delegate) {
  unique_ptr<CodeWriter> writer = io_delegate.GetCodeWriter(options.OutputFile());

  for (const auto& file : options.InputFiles()) {
    AidlTypenames typenames;
    std::unique_ptr<Parser> p = Parser::Parse(file, io_delegate, typenames);
    if (p == nullptr) return false;

    for (const auto& defined_type : p->ParsedDocument().DefinedTypes()) {
      if (!writer->Write("%s %s;\n", defined_type->GetPreprocessDeclarationName().c_str(),
                         defined_type->GetCanonicalName().c_str())) {
        return false;
      }
    }
  }

  return writer->Close();
}

int aidl_entry(const Options& options, const IoDelegate& io_delegate) {
  AidlErrorLog::clearError();

  int ret = 1;
  switch (options.GetTask()) {
    case Options::Task::COMPILE:
      ret = android::aidl::compile_aidl(options, io_delegate);
      break;
    case Options::Task::PREPROCESS:
      ret = android::aidl::preprocess_aidl(options, io_delegate) ? 0 : 1;
      break;
    case Options::Task::DUMP_API:
      ret = android::aidl::dump_api(options, io_delegate) ? 0 : 1;
      break;
    case Options::Task::CHECK_API:
      ret = android::aidl::check_api(options, io_delegate) ? 0 : 1;
      break;
    case Options::Task::DUMP_MAPPINGS:
      ret = android::aidl::dump_mappings(options, io_delegate) ? 0 : 1;
      break;
    default:
      AIDL_FATAL(AIDL_LOCATION_HERE)
          << "Unrecognized task: " << static_cast<size_t>(options.GetTask());
  }

  // compiler invariants
  const bool shouldReportError = ret != 0;
  const bool reportedError = AidlErrorLog::hadError();
  AIDL_FATAL_IF(shouldReportError != reportedError, AIDL_LOCATION_HERE)
      << "Compiler returned error " << ret << " but did" << (reportedError ? "" : " not")
      << " emit error logs";

  return ret;
}

}  // namespace aidl
}  // namespace android

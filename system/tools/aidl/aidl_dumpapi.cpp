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

#include "aidl_dumpapi.h"

#include <android-base/strings.h>

#include "aidl.h"
#include "logging.h"
#include "os.h"

using android::base::EndsWith;
using android::base::Join;
using android::base::Split;
using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {

void DumpVisitor::DumpType(const AidlDefinedType& dt, const string& type) {
  DumpComments(dt);
  DumpAnnotations(dt);
  out << type << " " << dt.GetName();
  if (auto generic_type = dt.AsParameterizable(); generic_type && generic_type->IsGeneric()) {
    out << "<" << Join(generic_type->GetTypeParameters(), ", ") << ">";
  }
  out << " {\n";
  out.Indent();
  DumpMembers(dt);
  out.Dedent();
  out << "}\n";
}

void DumpVisitor::DumpMembers(const AidlDefinedType& dt) {
  for (const auto& method : dt.GetMethods()) {
    if (!method->IsUserDefined()) continue;
    method->DispatchVisit(*this);
  }
  for (const auto& field : dt.GetFields()) {
    field->DispatchVisit(*this);
  }
  for (const auto& constdecl : dt.GetConstantDeclarations()) {
    constdecl->DispatchVisit(*this);
  }
}

// Dumps comment only if its has meaningful tags.
void DumpVisitor::DumpComments(const AidlCommentable& c) {
  const auto hidden = c.IsHidden();
  const auto deprecated = FindDeprecated(c.GetComments());
  if (hidden && !deprecated) {
    // to pass --checkapi between the current and the tot in the mainline-prod branch
    // emit @hide in a legacy dump style
    out << "/* @hide */\n";
  } else if (hidden || deprecated) {
    out << "/**\n";
    if (hidden) {
      out << " * @hide\n";
    }
    if (deprecated) {
      out << " * @deprecated " << deprecated->note << "\n";
    }
    out << " */\n";
  }
}

void DumpVisitor::DumpAnnotations(const AidlAnnotatable& a) {
  auto annotations = a.ToString();
  if (!annotations.empty()) {
    out << annotations << "\n";
  }
}

void DumpVisitor::DumpConstantValue(const AidlTypeSpecifier& type, const AidlConstantValue& c) {
  out << c.ValueString(type, AidlConstantValueDecorator);
}

void DumpVisitor::Visit(const AidlInterface& t) {
  DumpType(t, "interface");
}
void DumpVisitor::Visit(const AidlParcelable& t) {
  DumpType(t, "parcelable");
}
void DumpVisitor::Visit(const AidlStructuredParcelable& t) {
  DumpType(t, "parcelable");
}
void DumpVisitor::Visit(const AidlUnionDecl& t) {
  DumpType(t, "union");
}
void DumpVisitor::Visit(const AidlEnumDeclaration& t) {
  DumpComments(t);
  DumpAnnotations(t);
  out << "enum " << t.GetName() << " {\n";
  out.Indent();
  for (const auto& e : t.GetEnumerators()) {
    out << e->GetName() << " = ";
    DumpConstantValue(t.GetBackingType(), *e->GetValue());
    out << ",\n";
  }
  out.Dedent();
  out << "}\n";
}

void DumpVisitor::Visit(const AidlMethod& m) {
  DumpComments(m);
  out << m.ToString() << ";\n";
}
void DumpVisitor::Visit(const AidlVariableDeclaration& v) {
  DumpComments(v);
  Visit(v.GetType());
  if (v.IsDefaultUserSpecified()) {
    out << " " << v.GetName() << " = ";
    DumpConstantValue(v.GetType(), *v.GetDefaultValue());
    out << ";\n";
  } else {
    out << " " << v.GetName() << ";\n";
  }
}
void DumpVisitor::Visit(const AidlConstantDeclaration& c) {
  DumpComments(c);
  out << "const ";
  Visit(c.GetType());
  out << " " << c.GetName() << " = ";
  DumpConstantValue(c.GetType(), c.GetValue());
  out << ";\n";
}

void DumpVisitor::Visit(const AidlEnumerator& e) {
  out << e.GetName() << " = ";

  e.GetValue()->DispatchVisit(*this);
  out << ",\n";
}

void DumpVisitor::Visit(const AidlTypeSpecifier& t) {
  out << t.ToString();
}

static string GetApiDumpPathFor(const AidlDefinedType& defined_type, const Options& options) {
  string package_as_path = Join(Split(defined_type.GetPackage(), "."), OS_PATH_SEPARATOR);
  AIDL_FATAL_IF(options.OutputDir().empty() || options.OutputDir().back() != '/', defined_type);
  return options.OutputDir() + package_as_path + OS_PATH_SEPARATOR + defined_type.GetName() +
         ".aidl";
}

static void DumpComments(CodeWriter& out, const Comments& comments) {
  bool needs_newline = false;
  for (const auto& c : comments) {
    out << c.body;
    needs_newline = !EndsWith(c.body, "\n");
  }
  if (needs_newline) {
    out << "\n";
  }
}

bool dump_api(const Options& options, const IoDelegate& io_delegate) {
  for (const auto& file : options.InputFiles()) {
    AidlTypenames typenames;
    if (internals::load_and_validate_aidl(file, options, io_delegate, &typenames, nullptr) ==
        AidlError::OK) {
      const auto& doc = typenames.MainDocument();

      for (const auto& type : doc.DefinedTypes()) {
        unique_ptr<CodeWriter> writer =
            io_delegate.GetCodeWriter(GetApiDumpPathFor(*type, options));
        if (!options.DumpNoLicense()) {
          // dump doc comments (license) as well for each type
          DumpComments(*writer, doc.GetComments());
        }
        (*writer) << kPreamble;
        if (!type->GetPackage().empty()) {
          (*writer) << "package " << type->GetPackage() << ";\n";
        }
        DumpVisitor visitor(*writer);
        type->DispatchVisit(visitor);
      }
    } else {
      return false;
    }
  }
  return true;
}

}  // namespace aidl
}  // namespace android
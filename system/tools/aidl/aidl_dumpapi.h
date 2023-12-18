/*
 * Copyright (C) 2021, The Android Open Source Project *
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

#include "aidl_language.h"
#include "code_writer.h"

namespace android {
namespace aidl {

struct DumpVisitor : AidlVisitor {
  CodeWriter& out;
  DumpVisitor(CodeWriter& out) : out(out) {}

  void DumpType(const AidlDefinedType& dt, const string& type);
  void DumpMembers(const AidlDefinedType& dt);
  void DumpComments(const AidlCommentable& c);
  void DumpAnnotations(const AidlAnnotatable& a);
  virtual void DumpConstantValue(const AidlTypeSpecifier& type, const AidlConstantValue& c);

  void Visit(const AidlInterface& t) override;
  void Visit(const AidlParcelable& t) override;
  void Visit(const AidlStructuredParcelable& t) override;
  void Visit(const AidlUnionDecl& t) override;
  void Visit(const AidlEnumDeclaration& t) override;
  void Visit(const AidlMethod& m) override;
  void Visit(const AidlVariableDeclaration& v) override;
  void Visit(const AidlConstantDeclaration& c) override;
  void Visit(const AidlEnumerator& e) override;
  void Visit(const AidlTypeSpecifier& t) override;
};

bool dump_api(const Options& options, const IoDelegate& io_delegate);

}  // namespace aidl
}  // namespace android

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
#include "diagnostics.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "aidl.h"
#include "parser.h"
#include "tests/fake_io_delegate.h"

using android::aidl::AidlError;
using android::aidl::AidlTypenames;
using android::aidl::DiagnosticID;
using android::aidl::Options;
using android::aidl::internals::load_and_validate_aidl;
using android::aidl::test::FakeIoDelegate;
using testing::internal::CaptureStderr;
using testing::internal::GetCapturedStderr;

struct DiagnosticsTest : testing::Test {
  void ParseFiles(std::vector<std::pair<std::string, std::string>>&& files) {
    ASSERT_TRUE(files.size() > 0);
    const std::string main = files.begin()->first;
    for (const auto& [file, contents] : files) {
      io.SetFileContents(file, contents);
    }
    // emit diagnostics as warnings.
    // "java" has no specific meaning here because we're testing CheckValid()
    const Options options =
        Options::From("aidl " + optional_args + " -I . --lang java -o out -Weverything " + main);
    CaptureStderr();
    load_and_validate_aidl(main, options, io, &typenames, nullptr);
    const std::string err = GetCapturedStderr();
    if (expect_diagnostics.empty()) {
      EXPECT_EQ("", err);
    } else {
      for (const auto id : expect_diagnostics) {
        EXPECT_THAT(err, testing::HasSubstr("-W" + to_string(id)));
      }
    }
  }

  AidlTypenames typenames;
  FakeIoDelegate io;
  std::string optional_args;
  std::vector<DiagnosticID> expect_diagnostics;
};

TEST_F(DiagnosticsTest, const_name_ForEnumerator) {
  expect_diagnostics = {DiagnosticID::const_name};
  ParseFiles({{"Foo.aidl", "enum Foo { foo }"}});
}

TEST_F(DiagnosticsTest, const_name_ForConstants) {
  expect_diagnostics = {DiagnosticID::const_name};
  ParseFiles({{"IFoo.aidl", "interface IFoo { const int foo = 1; }"}});
}

TEST_F(DiagnosticsTest, interface_name) {
  expect_diagnostics = {DiagnosticID::interface_name};
  ParseFiles({{"Foo.aidl", "interface Foo { }"}});
}

TEST_F(DiagnosticsTest, enum_explicit_default) {
  expect_diagnostics = {DiagnosticID::enum_explicit_default};
  ParseFiles({{"Foo.aidl", "parcelable Foo { E e; }"}, {"E.aidl", "enum E { A }"}});
}

TEST_F(DiagnosticsTest, inout_parameter) {
  expect_diagnostics = {DiagnosticID::inout_parameter};
  ParseFiles({{"IFoo.aidl", "interface IFoo { void foo(inout Bar bar); }"},
              {"Bar.aidl", "parcelable Bar {}"}});
}

TEST_F(DiagnosticsTest, inout_parameter_SuppressAtMethodLevel) {
  expect_diagnostics = {};
  ParseFiles({
      {"IFoo.aidl",
       "interface IFoo { @SuppressWarnings(value={\"inout-parameter\"}) void foo(inout Bar b); }"},
      {"Bar.aidl", "parcelable Bar {}"},
  });
}

TEST_F(DiagnosticsTest, inout_parameter_SuppressAtDeclLevel) {
  expect_diagnostics = {};
  ParseFiles({
      {"IFoo.aidl",
       "@SuppressWarnings(value={\"inout-parameter\"}) interface IFoo { void foo(inout Bar b); }"},
      {"Bar.aidl", "parcelable Bar {}"},
  });
}

TEST_F(DiagnosticsTest, UnknownWarning) {
  expect_diagnostics = {DiagnosticID::unknown_warning};
  ParseFiles({
      {"IFoo.aidl", "@SuppressWarnings(value={\"blahblah\"}) interface IFoo { void foo(); }"},
  });
}

TEST_F(DiagnosticsTest, CantSuppressUnknownWarning) {
  expect_diagnostics = {DiagnosticID::unknown_warning};
  ParseFiles({
      {"IFoo.aidl",
       "@SuppressWarnings(value={\"unknown-warning\"})\n"
       "interface IFoo { @SuppressWarnings(value={\"blah-blah\"}) void foo(); }"},
  });
}

TEST_F(DiagnosticsTest, DontMixOnewayWithTwowayMethods) {
  expect_diagnostics = {DiagnosticID::mixed_oneway};
  ParseFiles({
      {"IFoo.aidl", "interface IFoo { void foo(); oneway void bar(); }"},
  });
}

TEST_F(DiagnosticsTest, OnewayInterfaceIsOkayWithSyntheticMethods) {
  optional_args = "--version 2";  // will add getInterfaceVersion() synthetic method
  expect_diagnostics = {};
  ParseFiles({
      {"IFoo.aidl", "oneway interface IFoo { void foo(); }"},
  });
}

TEST_F(DiagnosticsTest, ArraysAsOutputParametersConsideredHarmful) {
  expect_diagnostics = {DiagnosticID::out_array};
  ParseFiles({
      {"IFoo.aidl", "interface IFoo { void foo(out String[] ret); }"},
  });
}

TEST_F(DiagnosticsTest, file_descriptor) {
  expect_diagnostics = {DiagnosticID::file_descriptor};
  ParseFiles({{"IFoo.aidl",
               "interface IFoo {\n"
               "  void foo(in FileDescriptor fd);\n"
               "}"}});
}

TEST_F(DiagnosticsTest, out_nullable) {
  expect_diagnostics = {DiagnosticID::out_nullable};
  ParseFiles({{"IFoo.aidl",
               "interface IFoo {\n"
               "  void foo(out @nullable Bar bar);\n"
               "}"},
              {"Bar.aidl", "parcelable Bar {}"}});
}

TEST_F(DiagnosticsTest, inout_nullable) {
  expect_diagnostics = {DiagnosticID::out_nullable};
  ParseFiles({{"IFoo.aidl",
               "interface IFoo {\n"
               "  void foo(inout @nullable Bar bar);\n"
               "}"},
              {"Bar.aidl", "parcelable Bar {}"}});
}

TEST_F(DiagnosticsTest, out_nullable_OkayForArrays) {
  expect_diagnostics = {DiagnosticID::out_array};  // not triggering out_nullable
  ParseFiles({{"IFoo.aidl",
               "interface IFoo {\n"
               "  void foo(inout @nullable Bar[] bar1, out @nullable Bar[] bar2);\n"
               "}"},
              {"Bar.aidl", "parcelable Bar {}"}});
}

/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "aidl.h"
#include "fake_io_delegate.h"
#include "options.h"

#include <fuzzer/FuzzedDataProvider.h>
#include <iostream>

#ifdef FUZZ_LOG
constexpr bool kFuzzLog = true;
#else
constexpr bool kFuzzLog = false;
#endif

using android::aidl::test::FakeIoDelegate;

void fuzz(const FakeIoDelegate& io, const std::vector<std::string>& args) {
  if (kFuzzLog) {
    std::cout << "cmd: ";
    for (const std::string& arg : args) {
      std::cout << arg << " ";
    }
    std::cout << std::endl;

    for (const auto& [f, input] : io.InputFiles()) {
      std::cout << "INPUT " << f << ": " << input << std::endl;
    }
  }

  int ret = android::aidl::aidl_entry(Options::From(args), io);
  if (ret != 0) return;

  if (kFuzzLog) {
    for (const auto& [f, output] : io.OutputFiles()) {
      std::cout << "OUTPUT " << f << ": " << std::endl;
      std::cout << output << std::endl;
    }
  }
}

void fuzzLang(const std::string& langOpt, const std::string& content) {
  FakeIoDelegate io;
  io.SetFileContents("a/path/Foo.aidl", content);

  std::vector<std::string> args;
  args.emplace_back("aidl");
  args.emplace_back("--lang=" + langOpt);
  args.emplace_back("-b");
  args.emplace_back("-I .");
  args.emplace_back("-o out");
  // corresponding package also in aidl_parser_fuzzer.dict
  args.emplace_back("a/path/Foo.aidl");

  fuzz(io, args);
}

void fuzzCheckApi(const std::string& a, const std::string& b) {
  FakeIoDelegate io;
  io.SetFileContents("a/path/Foo.aidl", a);
  io.SetFileContents("b/path/Foo.aidl", b);

  std::vector<std::string> args;
  args.emplace_back("aidl");
  args.emplace_back("--checkapi");
  // corresponding package also in aidl_parser_fuzzer.dict
  args.emplace_back("a/path/");
  args.emplace_back("b/path/");

  fuzz(io, args);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size <= 1) return 0;  // no use

  // b/145447540, large nested expressions sometimes hit the stack depth limit.
  // Fuzzing things of this size don't provide any additional meaningful
  // coverage. This is an approximate value which should allow us to explore all
  // of the language w/o hitting a stack overflow.
  if (size > 2000) return 0;

  FuzzedDataProvider provider = FuzzedDataProvider(data, size);

  if (provider.ConsumeBool()) {
    std::string content = provider.ConsumeRemainingBytesAsString();

    fuzzLang("ndk", content);
    fuzzLang("cpp", content);
    fuzzLang("java", content);
    fuzzLang("rust", content);
  } else {
    std::string contentA = provider.ConsumeRandomLengthString();
    std::string contentB = provider.ConsumeRemainingBytesAsString();

    fuzzCheckApi(contentA, contentB);
  }

  return 0;
}

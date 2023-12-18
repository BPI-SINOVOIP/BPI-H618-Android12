/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "derive_classpath.h"
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android-modules-utils/sdk_level.h>
#include <glob.h>
#include <regex>
#include <sstream>

#include "packages/modules/common/proto/classpaths.pb.h"

namespace android {
namespace derive_classpath {

using Filepaths = std::vector<std::string>;
using Classpaths = std::unordered_map<Classpath, Filepaths>;

static const std::regex kBindMountedApex("^/apex/[^/]+@[0-9]+/");
static const std::regex kApexPathRegex("(/apex/[^/]+)/");

// Defines the order of individual fragments to be merged for BOOTCLASSPATH:
// 1. Jars in ART module always come first;
// 2. Jars defined as part of /system/etc/classpaths;
// 3. Jars defined in all non-ART apexes that expose /apex/*/etc/classpaths fragments.
//
// Notes:
// - Relative order in the individual fragment files is not changed when merging.
// - If a fragment file is matched by multiple globs, the first one is used; i.e. ART module
//   fragment is only parsed once, even if there is a "/apex/*/" pattern later.
// - If there are multiple files matched for a glob pattern with wildcards, the results are sorted
//   by pathname (default glob behaviour); i.e. all fragment files are sorted within a single
//   "pattern block".
static const std::vector<std::string> kBootclasspathFragmentGlobPatterns = {
    // ART module is a special case and must come first before any other classpath entries.
    "/apex/com.android.art/etc/classpaths/bootclasspath.pb",
    "/system/etc/classpaths/bootclasspath.pb",
    "/apex/*/etc/classpaths/bootclasspath.pb",
};

// Defines the order of individual fragments to be merged for SYSTEMSERVERCLASSPATH.
//
// ART system server jars are not special in this case, and are considered to be part of all the
// other apexes that may expose system server jars.
//
// All notes from kBootclasspathFragmentGlobPatterns apply here.
static const std::vector<std::string> kSystemserverclasspathFragmentGlobPatterns = {
    "/system/etc/classpaths/systemserverclasspath.pb",
    "/apex/*/etc/classpaths/systemserverclasspath.pb",
};

// Finds all classpath fragment files that match the glob pattern and appends them to `fragments`.
//
// If a newly found fragment is already present in `fragments`, it is skipped to avoid duplicates.
// Note that appended fragment files are sorted by pathnames, which is a default behaviour for
// glob().
bool GlobClasspathFragments(Filepaths* fragments, const std::string& pattern) {
  glob_t glob_result;
  const int ret = glob(pattern.c_str(), GLOB_MARK, nullptr, &glob_result);
  if (ret != 0 && ret != GLOB_NOMATCH) {
    globfree(&glob_result);
    LOG(ERROR) << "Failed to glob " << pattern;
    return false;
  }

  for (size_t i = 0; i < glob_result.gl_pathc; i++) {
    std::string path = glob_result.gl_pathv[i];
    // Skip <name>@<ver> dirs, as they are bind-mounted to <name>
    if (std::regex_search(path, kBindMountedApex)) {
      continue;
    }
    // Make sure we don't push duplicate fragments from previously processed patterns
    if (std::find(fragments->begin(), fragments->end(), path) == fragments->end()) {
      fragments->push_back(path);
    }
  }
  globfree(&glob_result);
  return true;
}

// Writes the contents of *CLASSPATH variables to /data in the format expected by `load_exports`
// action from init.rc. See platform/system/core/init/README.md.
bool WriteClasspathExports(Classpaths classpaths, std::string_view output_path) {
  LOG(INFO) << "WriteClasspathExports " << output_path;

  std::stringstream out;
  out << "export BOOTCLASSPATH " << android::base::Join(classpaths[BOOTCLASSPATH], ':') << '\n';
  out << "export DEX2OATBOOTCLASSPATH "
      << android::base::Join(classpaths[DEX2OATBOOTCLASSPATH], ':') << '\n';
  out << "export SYSTEMSERVERCLASSPATH "
      << android::base::Join(classpaths[SYSTEMSERVERCLASSPATH], ':') << '\n';

  const std::string& content = out.str();
  LOG(INFO) << "WriteClasspathExports content\n" << content;

  const std::string path_str(output_path);
  if (android::base::StartsWith(path_str, "/data/")) {
    // When writing to /data, write to a temp file first to make sure the partition is not full.
    const std::string temp_str(path_str + ".tmp");
    if (!android::base::WriteStringToFile(content, temp_str, /*follow_symlinks=*/true)) {
      return false;
    }
    return rename(temp_str.c_str(), path_str.c_str()) == 0;
  } else {
    return android::base::WriteStringToFile(content, path_str, /*follow_symlinks=*/true);
  }
}

bool ReadClasspathFragment(ExportedClasspathsJars* fragment, const std::string& filepath) {
  LOG(INFO) << "ReadClasspathFragment " << filepath;
  std::string contents;
  if (!android::base::ReadFileToString(filepath, &contents)) {
    PLOG(ERROR) << "Failed to read " << filepath;
    return false;
  }
  if (!fragment->ParseFromString(contents)) {
    LOG(ERROR) << "Failed to parse " << filepath;
    return false;
  }
  return true;
}

// Returns an allowed prefix for a jar filepaths declared in a given fragment.
// For a given apex fragment, it returns the apex path - "/apex/com.android.foo" - as an allowed
// prefix for jars. This can be used to enforce that an apex fragment only exports jars located in
// that apex. For system fragment, it returns an empty string to allow any jars to be exported by
// the platform.
std::string GetAllowedJarPathPrefix(const std::string& fragment_path) {
  std::smatch match;
  if (std::regex_search(fragment_path, match, kApexPathRegex)) {
    return match[1];
  }
  return "";
}

// Finds and parses all classpath fragments on device matching given glob patterns.
bool ParseFragments(const std::string& globPatternPrefix, Classpaths& classpaths, bool boot_jars) {
  LOG(INFO) << "ParseFragments for " << (boot_jars ? "bootclasspath" : "systemserverclasspath");

  auto glob_patterns =
      boot_jars ? kBootclasspathFragmentGlobPatterns : kSystemserverclasspathFragmentGlobPatterns;

  Filepaths fragments;
  for (const auto& pattern : glob_patterns) {
    if (!GlobClasspathFragments(&fragments, globPatternPrefix + pattern)) {
      return false;
    }
  }

  for (const auto& fragment_path : fragments) {
    ExportedClasspathsJars exportedJars;
    if (!ReadClasspathFragment(&exportedJars, fragment_path)) {
      return false;
    }

    // Either a path to the apex, or an empty string
    const std::string& allowed_jar_prefix = GetAllowedJarPathPrefix(fragment_path);

    for (const Jar& jar : exportedJars.jars()) {
      // TODO(b/180105615): check for SdkVersion ranges;
      const std::string& jar_path = jar.path();
      CHECK(android::base::StartsWith(jar_path, allowed_jar_prefix))
          << fragment_path << " must not export a jar from outside of the apex: " << jar_path;
      const Classpath classpath = jar.classpath();
      CHECK(boot_jars ^ (classpath == SYSTEMSERVERCLASSPATH))
          << fragment_path << " must not export a jar for " << Classpath_Name(classpath);
      classpaths[classpath].push_back(jar_path);
    }
  }
  return true;
}

// Generates /data/system/environ/classpath exports file by globing and merging individual
// classpaths.proto config fragments. The exports file is read by init.rc to setenv *CLASSPATH
// environ variables at runtime.
bool GenerateClasspathExports(std::string_view output_path) {
  // Outside of tests use actual config fragments.
  return GenerateClasspathExports("", output_path);
}

// Internal implementation of GenerateClasspathExports that allows putting config fragments in
// temporary directories. `globPatternPrefix` is appended to each glob pattern from
// kBootclasspathFragmentGlobPatterns and kSystemserverclasspathFragmentGlobPatterns, which allows
// adding mock configs in /data/local/tmp for example.
bool GenerateClasspathExports(const std::string& globPatternPrefix, std::string_view output_path) {
  // Parse all known classpath fragments
  CHECK(android::modules::sdklevel::IsAtLeastS())
      << "derive_classpath must only be run on Android 12 or above";

  Classpaths classpaths;
  if (!ParseFragments(globPatternPrefix, classpaths, /*boot_jars=*/true)) {
    LOG(ERROR) << "Failed to parse BOOTCLASSPATH fragments";
    return false;
  }
  if (!ParseFragments(globPatternPrefix, classpaths, /*boot_jars=*/false)) {
    LOG(ERROR) << "Failed to parse SYSTEMSERVERCLASSPATH fragments";
    return false;
  }

  // Write export actions for init.rc
  if (!WriteClasspathExports(classpaths, output_path)) {
    PLOG(ERROR) << "Failed to write " << output_path;
    return false;
  }
  return true;
}

}  // namespace derive_classpath
}  // namespace android

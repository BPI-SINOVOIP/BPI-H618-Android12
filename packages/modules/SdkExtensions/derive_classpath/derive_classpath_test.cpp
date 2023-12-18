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
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-modules-utils/sdk_level.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <cstdlib>
#include <string_view>

#include "android-base/unique_fd.h"
#include "packages/modules/common/proto/classpaths.pb.h"

namespace android {
namespace derive_classpath {
namespace {

static const std::string kFrameworkJarFilepath = "/system/framework/framework.jar";
static const std::string kLibartJarFilepath = "/apex/com.android.art/javalib/core-libart.jar";
static const std::string kSdkExtensionsJarFilepath =
    "/apex/com.android.sdkext/javalib/framework-sdkextensions.jar";
static const std::string kServicesJarFilepath = "/system/framework/services.jar";

// The fixture for testing derive_classpath.
class DeriveClasspathTest : public ::testing::Test {
 protected:
  ~DeriveClasspathTest() override {
    // Not really needed, as a test device will re-generate a proper classpath on reboot,
    // but it's better to leave it in a clean state after a test.
    GenerateClasspathExports();
  }

  const std::string working_dir() { return std::string(temp_dir_.path); }

  // Parses the generated classpath exports file and returns each line individually.
  std::vector<std::string> ParseExportsFile(const char* file = "/data/system/environ/classpath") {
    std::string contents;
    EXPECT_TRUE(android::base::ReadFileToString(file, &contents, /*follow_symlinks=*/true));
    return android::base::Split(contents, "\n");
  }

  std::vector<std::string> SplitClasspathExportLine(const std::string& line) {
    std::vector<std::string> contents = android::base::Split(line, " ");
    // Export lines are expected to be structured as `export <name> <value>`.
    EXPECT_EQ(3, contents.size());
    EXPECT_EQ("export", contents[0]);
    return contents;
  }

  // Checks the order of the jars in a given classpath.
  // Instead of doing a full order check, it assumes the jars are grouped by partition and checks
  // that partitions come in order of the `prefixes` that is given.
  void CheckClasspathGroupOrder(const std::string classpath,
                                const std::vector<std::string> prefixes) {
    ASSERT_NE(0, prefixes.size());
    ASSERT_NE(0, classpath.size());

    auto jars = android::base::Split(classpath, ":");

    auto prefix = prefixes.begin();
    auto jar = jars.begin();
    for (; jar != jars.end() && prefix != prefixes.end(); ++jar) {
      if (*jar == "/apex/com.android.i18n/javalib/core-icu4j.jar") {
        // core-icu4j.jar is special and is out of order in BOOTCLASSPATH;
        // ignore it when checking for general order
        continue;
      }
      if (!android::base::StartsWith(*jar, *prefix)) {
        ++prefix;
      }
    }
    EXPECT_NE(prefix, prefixes.end());
    // All jars have been iterated over, thus they all have valid prefixes
    EXPECT_EQ(jar, jars.end());
  }

  void AddJarToClasspath(const std::string& partition, const std::string& jar_filepath,
                         Classpath classpath) {
    ExportedClasspathsJars exported_jars;
    Jar* jar = exported_jars.add_jars();
    jar->set_path(jar_filepath);
    jar->set_classpath(classpath);

    std::string basename = Classpath_Name(classpath) + ".pb";
    std::transform(basename.begin(), basename.end(), basename.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::string fragment_path = working_dir() + partition + "/etc/classpaths/" + basename;
    std::string buf;
    exported_jars.SerializeToString(&buf);
    std::string cmd("mkdir -p " + android::base::Dirname(fragment_path));
    ASSERT_EQ(0, system(cmd.c_str()));
    ASSERT_TRUE(android::base::WriteStringToFile(buf, fragment_path, true));
  }

  TemporaryDir temp_dir_;
};

using DeriveClasspathDeathTest = DeriveClasspathTest;

// Check only known *CLASSPATH variables are exported.
TEST_F(DeriveClasspathTest, DefaultNoUnknownClasspaths) {
  // Re-generate default on device classpaths
  GenerateClasspathExports();

  const std::vector<std::string> exportLines = ParseExportsFile();
  // The first three lines are tested above.
  for (int i = 3; i < exportLines.size(); i++) {
    EXPECT_EQ(exportLines[i], "");
  }
}

// Test that temp directory does not pick up actual jars.
TEST_F(DeriveClasspathTest, TempConfig) {
  AddJarToClasspath("/apex/com.android.foo", "/apex/com.android.foo/javalib/foo", BOOTCLASSPATH);
  AddJarToClasspath("/apex/com.android.baz", "/apex/com.android.baz/javalib/baz",
                    SYSTEMSERVERCLASSPATH);

  ASSERT_TRUE(GenerateClasspathExports(working_dir()));

  const std::vector<std::string> exportLines = ParseExportsFile();

  std::vector<std::string> splitExportLine;

  splitExportLine = SplitClasspathExportLine(exportLines[0]);
  EXPECT_EQ("BOOTCLASSPATH", splitExportLine[1]);
  EXPECT_EQ("/apex/com.android.foo/javalib/foo", splitExportLine[2]);
  splitExportLine = SplitClasspathExportLine(exportLines[2]);
  EXPECT_EQ("SYSTEMSERVERCLASSPATH", splitExportLine[1]);
  EXPECT_EQ("/apex/com.android.baz/javalib/baz", splitExportLine[2]);
}

// Test individual modules are sorted by pathnames.
TEST_F(DeriveClasspathTest, ModulesAreSorted) {
  AddJarToClasspath("/apex/com.android.art", "/apex/com.android.art/javalib/art", BOOTCLASSPATH);
  AddJarToClasspath("/system", "/system/framework/jar", BOOTCLASSPATH);
  AddJarToClasspath("/apex/com.android.foo", "/apex/com.android.foo/javalib/foo", BOOTCLASSPATH);
  AddJarToClasspath("/apex/com.android.bar", "/apex/com.android.bar/javalib/bar", BOOTCLASSPATH);
  AddJarToClasspath("/apex/com.android.baz", "/apex/com.android.baz/javalib/baz", BOOTCLASSPATH);

  ASSERT_TRUE(GenerateClasspathExports(working_dir()));

  const std::vector<std::string> exportLines = ParseExportsFile();
  const std::vector<std::string> splitExportLine = SplitClasspathExportLine(exportLines[0]);
  const std::string exportValue = splitExportLine[2];

  const std::string expectedJars(
      "/apex/com.android.art/javalib/art"
      ":/system/framework/jar"
      ":/apex/com.android.bar/javalib/bar"
      ":/apex/com.android.baz/javalib/baz"
      ":/apex/com.android.foo/javalib/foo");

  EXPECT_EQ(expectedJars, exportValue);
}

// Test we can output to custom files.
TEST_F(DeriveClasspathTest, CustomOutputLocation) {
  AddJarToClasspath("/apex/com.android.art", "/apex/com.android.art/javalib/art", BOOTCLASSPATH);
  AddJarToClasspath("/system", "/system/framework/jar", BOOTCLASSPATH);
  AddJarToClasspath("/apex/com.android.foo", "/apex/com.android.foo/javalib/foo", BOOTCLASSPATH);
  AddJarToClasspath("/apex/com.android.bar", "/apex/com.android.bar/javalib/bar", BOOTCLASSPATH);
  AddJarToClasspath("/apex/com.android.baz", "/apex/com.android.baz/javalib/baz", BOOTCLASSPATH);

  android::base::unique_fd fd(memfd_create("temp_file", MFD_CLOEXEC));
  ASSERT_TRUE(fd.ok()) << "Unable to open temp-file";
  const std::string file_name = android::base::StringPrintf("/proc/self/fd/%d", fd.get());
  ASSERT_TRUE(GenerateClasspathExports(working_dir(), file_name));

  const std::vector<std::string> exportLines = ParseExportsFile(file_name.c_str());
  const std::vector<std::string> splitExportLine = SplitClasspathExportLine(exportLines[0]);
  const std::string exportValue = splitExportLine[2];

  const std::string expectedJars(
      "/apex/com.android.art/javalib/art"
      ":/system/framework/jar"
      ":/apex/com.android.bar/javalib/bar"
      ":/apex/com.android.baz/javalib/baz"
      ":/apex/com.android.foo/javalib/foo");

  EXPECT_EQ(expectedJars, exportValue);
}

// Test output location that can't be written to.
TEST_F(DeriveClasspathTest, NonWriteableOutputLocation) {
  AddJarToClasspath("/apex/com.android.art", "/apex/com.android.art/javalib/art", BOOTCLASSPATH);
  AddJarToClasspath("/system", "/system/framework/jar", BOOTCLASSPATH);

  ASSERT_FALSE(GenerateClasspathExports(working_dir(), "/system/non_writable_path"));
}

// Test apexes only export their own jars.
TEST_F(DeriveClasspathDeathTest, ApexJarsBelongToApex) {
  // EXPECT_DEATH expects error messages in stderr, log there
  android::base::SetLogger(android::base::StderrLogger);

  AddJarToClasspath("/system", "/system/framework/jar", BOOTCLASSPATH);
  AddJarToClasspath("/apex/com.android.foo", "/apex/com.android.foo/javalib/foo", BOOTCLASSPATH);
  AddJarToClasspath("/apex/com.android.bar", "/apex/wrong/path/bar", BOOTCLASSPATH);

  EXPECT_DEATH(GenerateClasspathExports(working_dir()), "must not export a jar.*wrong/path/bar");
}

// Test classpath fragments export jars for themselves.
TEST_F(DeriveClasspathDeathTest, WrongClasspathInFragments) {
  // Valid configs
  AddJarToClasspath("/system", "/system/framework/framework-jar", BOOTCLASSPATH);
  AddJarToClasspath("/system", "/system/framework/service-jar", SYSTEMSERVERCLASSPATH);

  // Manually create an invalid config with both BCP and SSCP jars...
  ExportedClasspathsJars exported_jars;
  Jar* jar = exported_jars.add_jars();
  jar->set_path("/apex/com.android.foo/javalib/foo");
  jar->set_classpath(BOOTCLASSPATH);
  // note that DEX2OATBOOTCLASSPATH and BOOTCLASSPATH jars are expected to be in the same config
  jar = exported_jars.add_jars();
  jar->set_path("/apex/com.android.foo/javalib/foo");
  jar->set_classpath(DEX2OATBOOTCLASSPATH);
  jar = exported_jars.add_jars();
  jar->set_path("/apex/com.android.foo/javalib/service-foo");
  jar->set_classpath(SYSTEMSERVERCLASSPATH);

  // ...and write this config to bootclasspath.pb
  std::string fragment_path =
      working_dir() + "/apex/com.android.foo/etc/classpaths/bootclasspath.pb";
  std::string buf;
  exported_jars.SerializeToString(&buf);
  std::string cmd("mkdir -p " + android::base::Dirname(fragment_path));
  ASSERT_EQ(0, system(cmd.c_str()));
  ASSERT_TRUE(android::base::WriteStringToFile(buf, fragment_path, true));

  EXPECT_DEATH(GenerateClasspathExports(working_dir()),
               "must not export a jar for SYSTEMSERVERCLASSPATH");
}

}  // namespace
}  // namespace derive_classpath
}  // namespace android

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

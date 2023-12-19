// Copyright (C) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "icing/file/memory-mapped-file.h"

#include "perftools/profiles/collector/heap/alloc_recorder.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/file/filesystem.h"
#include "icing/testing/common-matchers.h"
#include "icing/testing/recorder-test-utils.h"
#include "icing/testing/tmp-directory.h"

namespace icing {
namespace lib {
namespace {

namespace heap_profile = ::perftools::profiles::collector::heap;

using testing::Le;

TEST(MemoryMappedFileTest, MMapMemoryLeak) {
  std::string test_dir = GetTestTempDir();
  std::string recorder_dir = test_dir + "/recorder";
  Filesystem filesystem;
  ASSERT_TRUE(filesystem.CreateDirectoryRecursively(recorder_dir.c_str()));

  ASSERT_TRUE(heap_profile::AllocRecorderStartWithMmapTracking(recorder_dir));
  {
    std::string mmfile_dir = test_dir + "/file";
    ASSERT_TRUE(filesystem.CreateDirectoryRecursively(mmfile_dir.c_str()));
    MemoryMappedFile mmfile(filesystem, mmfile_dir + "/mmfile",
                            MemoryMappedFile::READ_WRITE_AUTO_SYNC);
    // How this works:
    // We request a 500-byte mapping starting at the 101st byte of the file.
    // But(!), mmap only accepts offsets that are multiples of page size. So
    // instead mmfile will create a 600-byte mapping starting at the 1st byte of
    // file and then return the address of the 101st byte within that mapping.
    // For this reason, total bytes and peak bytes will be 600 bytes.
    //
    // When mmfile goes out of scope it needs to munmap the mapping that it
    // created. But, remember that the mapping is larger (600 bytes) than what
    // we requested (500 bytes)! So mmfile needs to remember the actual size of
    // the mapping, NOT the requested size. Calling munmap with the correct size
    // will ensure that total_inuse_bytes is 0 after mmfile goes out of scope.
    // Calling munmap with the requested size would still keep 100 bytes of the
    // mapping around!
    mmfile.Remap(100, 500);
  }
  heap_profile::AllocRecorderStop();

  // Mmap only affects bytes measurements.
  ProfileInfo profile_info = SummarizeProfileProto(recorder_dir + ".0.pb.gz");
  EXPECT_THAT(profile_info.total_alloc_bytes, Le(600));
  EXPECT_THAT(profile_info.peak_bytes, Le(600));
  EXPECT_THAT(profile_info.inuse_bytes, Le(0));
}

}  // namespace
}  // namespace lib
}  // namespace icing

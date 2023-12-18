#!/usr/bin/env python3
#
# Copyright (C) 2021 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import filecmp
import os
from pathlib import Path
import shutil
import tempfile

from binary_cache_builder import BinaryCacheBuilder
from simpleperf_utils import ReadElf, remove, ToolFinder
from . test_utils import TestBase, TestHelper


class TestBinaryCacheBuilder(TestBase):
    def test_copy_binaries_from_symfs_dirs(self):
        readelf = ReadElf(TestHelper.ndk_path)
        strip = ToolFinder.find_tool_path('strip', arch='arm')
        self.assertIsNotNone(strip)
        symfs_dir = os.path.join(self.test_dir, 'symfs_dir')
        remove(symfs_dir)
        os.mkdir(symfs_dir)
        filename = 'simpleperf_runtest_two_functions_arm'
        origin_file = TestHelper.testdata_path(filename)
        source_file = os.path.join(symfs_dir, filename)
        target_file = os.path.join('binary_cache', filename)
        expected_build_id = readelf.get_build_id(origin_file)
        binary_cache_builder = BinaryCacheBuilder(TestHelper.ndk_path, False)
        binary_cache_builder.binaries['simpleperf_runtest_two_functions_arm'] = expected_build_id

        # Copy binary if target file doesn't exist.
        remove(target_file)
        self.run_cmd([strip, '--strip-all', '-o', source_file, origin_file])
        binary_cache_builder.copy_binaries_from_symfs_dirs([symfs_dir])
        self.assertTrue(filecmp.cmp(target_file, source_file))

        # Copy binary if target file doesn't have .symtab and source file has .symtab.
        self.run_cmd([strip, '--strip-debug', '-o', source_file, origin_file])
        binary_cache_builder.copy_binaries_from_symfs_dirs([symfs_dir])
        self.assertTrue(filecmp.cmp(target_file, source_file))

        # Copy binary if target file doesn't have .debug_line and source_files has .debug_line.
        shutil.copy(origin_file, source_file)
        binary_cache_builder.copy_binaries_from_symfs_dirs([symfs_dir])
        self.assertTrue(filecmp.cmp(target_file, source_file))

    def test_copy_elf_without_build_id_from_symfs_dir(self):
        binary_cache_builder = BinaryCacheBuilder(TestHelper.ndk_path, False)
        binary_cache_builder.binaries['elf'] = ''
        symfs_dir = TestHelper.testdata_path('data/symfs_without_build_id')
        source_file = os.path.join(symfs_dir, 'elf')
        target_file = os.path.join('binary_cache', 'elf')
        binary_cache_builder.copy_binaries_from_symfs_dirs([symfs_dir])
        self.assertTrue(filecmp.cmp(target_file, source_file))
        binary_cache_builder.pull_binaries_from_device()
        self.assertTrue(filecmp.cmp(target_file, source_file))

    def test_prefer_binary_with_debug_info(self):
        binary_cache_builder = BinaryCacheBuilder(TestHelper.ndk_path, False)
        binary_cache_builder.collect_used_binaries(
            TestHelper.testdata_path('runtest_two_functions_arm64_perf.data'))

        # Create a symfs_dir, which contains elf file with and without debug info.
        with tempfile.TemporaryDirectory() as tmp_dir:
            shutil.copy(
                TestHelper.testdata_path(
                    'simpleperf_runtest_two_functions_arm64_without_debug_info'),
                Path(tmp_dir) / 'simpleperf_runtest_two_functions_arm64')

            debug_dir = Path(tmp_dir) / 'debug'
            debug_dir.mkdir()
            shutil.copy(TestHelper.testdata_path(
                'simpleperf_runtest_two_functions_arm64'), debug_dir)
            # Check if the elf file with debug info is chosen.
            binary_cache_builder.copy_binaries_from_symfs_dirs([tmp_dir])
            elf_path = (Path(binary_cache_builder.binary_cache_dir) / 'data' /
                        'local' / 'tmp' / 'simpleperf_runtest_two_functions_arm64')
            self.assertTrue(elf_path.is_file())
            self.assertIn('.debug_info', binary_cache_builder.readelf.get_sections(elf_path))

    def test_create_build_id_list(self):
        symfs_dir = TestHelper.testdata_dir
        binary_cache_builder = BinaryCacheBuilder(TestHelper.ndk_path, False)
        binary_cache_builder.collect_used_binaries(
            TestHelper.testdata_path('runtest_two_functions_arm64_perf.data'))
        binary_cache_builder.copy_binaries_from_symfs_dirs([symfs_dir])
        elf_path = (Path(binary_cache_builder.binary_cache_dir) / 'data' /
                    'local' / 'tmp' / 'simpleperf_runtest_two_functions_arm64')
        self.assertTrue(elf_path.is_file())

        binary_cache_builder.create_build_id_list()
        build_id_list_path = Path(binary_cache_builder.binary_cache_dir) / 'build_id_list'
        self.assertTrue(build_id_list_path.is_file())
        with open(build_id_list_path, 'r') as fh:
            self.assertIn('simpleperf_runtest_two_functions_arm64', fh.read())

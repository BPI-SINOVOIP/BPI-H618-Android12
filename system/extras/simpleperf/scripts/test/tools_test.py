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

import os
from pathlib import Path

from binary_cache_builder import BinaryCacheBuilder
from simpleperf_utils import (Addr2Nearestline, BinaryFinder, Objdump, ReadElf,
                              SourceFileSearcher, is_windows, remove)
from . test_utils import TestBase, TestHelper


class TestTools(TestBase):
    def test_addr2nearestline(self):
        self.run_addr2nearestline_test(True)
        self.run_addr2nearestline_test(False)

    def run_addr2nearestline_test(self, with_function_name):
        test_map = {
            '/simpleperf_runtest_two_functions_arm64': [
                {
                    'func_addr': 0x112c,
                    'addr': 0x112c,
                    'source': 'system/extras/simpleperf/runtest/two_functions.cpp:20',
                    'function': 'main',
                },
                {
                    'func_addr': 0x104c,
                    'addr': 0x105c,
                    'source': "system/extras/simpleperf/runtest/two_functions.cpp:7",
                    'function': "Function1()",
                },
            ],
            '/simpleperf_runtest_two_functions_arm': [
                {
                    'func_addr': 0x784,
                    'addr': 0x7b0,
                    'source': """system/extras/simpleperf/runtest/two_functions.cpp:14
                                 system/extras/simpleperf/runtest/two_functions.cpp:23""",
                    'function': """Function2()
                                   main""",
                },
                {
                    'func_addr': 0x784,
                    'addr': 0x7d0,
                    'source': """system/extras/simpleperf/runtest/two_functions.cpp:15
                                 system/extras/simpleperf/runtest/two_functions.cpp:23""",
                    'function': """Function2()
                                   main""",
                }
            ],
            '/simpleperf_runtest_two_functions_x86_64': [
                {
                    'func_addr': 0x840,
                    'addr': 0x840,
                    'source': 'system/extras/simpleperf/runtest/two_functions.cpp:7',
                    'function': 'Function1()',
                },
                {
                    'func_addr': 0x920,
                    'addr': 0x94a,
                    'source': """system/extras/simpleperf/runtest/two_functions.cpp:7
                                 system/extras/simpleperf/runtest/two_functions.cpp:22""",
                    'function': """Function1()
                                   main""",
                }
            ],
            '/simpleperf_runtest_two_functions_x86': [
                {
                    'func_addr': 0x6d0,
                    'addr': 0x6da,
                    'source': 'system/extras/simpleperf/runtest/two_functions.cpp:14',
                    'function': 'Function2()',
                },
                {
                    'func_addr': 0x710,
                    'addr': 0x749,
                    'source': """system/extras/simpleperf/runtest/two_functions.cpp:8
                                 system/extras/simpleperf/runtest/two_functions.cpp:22""",
                    'function': """Function1()
                                   main""",
                }
            ],
        }
        binary_finder = BinaryFinder(TestHelper.testdata_dir, ReadElf(TestHelper.ndk_path))
        addr2line = Addr2Nearestline(TestHelper.ndk_path, binary_finder, with_function_name)
        for dso_path in test_map:
            test_addrs = test_map[dso_path]
            for test_addr in test_addrs:
                addr2line.add_addr(dso_path, None, test_addr['func_addr'], test_addr['addr'])
        addr2line.convert_addrs_to_lines()
        for dso_path in test_map:
            dso = addr2line.get_dso(dso_path)
            self.assertIsNotNone(dso, dso_path)
            test_addrs = test_map[dso_path]
            for test_addr in test_addrs:
                expected_files = []
                expected_lines = []
                expected_functions = []
                for line in test_addr['source'].split('\n'):
                    items = line.split(':')
                    expected_files.append(items[0].strip())
                    expected_lines.append(int(items[1]))
                for line in test_addr['function'].split('\n'):
                    expected_functions.append(line.strip())
                self.assertEqual(len(expected_files), len(expected_functions))

                if with_function_name:
                    expected_source = list(zip(expected_files, expected_lines, expected_functions))
                else:
                    expected_source = list(zip(expected_files, expected_lines))

                actual_source = addr2line.get_addr_source(dso, test_addr['addr'])
                if is_windows():
                    self.assertIsNotNone(actual_source, 'for %s:0x%x' %
                                         (dso_path, test_addr['addr']))
                    for i, source in enumerate(actual_source):
                        new_source = list(source)
                        new_source[0] = new_source[0].replace('\\', '/')
                        actual_source[i] = tuple(new_source)

                self.assertEqual(actual_source, expected_source,
                                 'for %s:0x%x, expected source %s, actual source %s' %
                                 (dso_path, test_addr['addr'], expected_source, actual_source))

    def test_objdump(self):
        test_map = {
            '/simpleperf_runtest_two_functions_arm64': {
                'start_addr': 0x112c,
                'len': 28,
                'expected_items': [
                    ('main():', 0),
                    ('system/extras/simpleperf/runtest/two_functions.cpp:20', 0),
                    ('1134:      	add	x29, sp, #16', 0x1134),
                ],
            },
            '/simpleperf_runtest_two_functions_arm': {
                'start_addr': 0x784,
                'len': 80,
                'expected_items': [
                    ('main():', 0),
                    ('system/extras/simpleperf/runtest/two_functions.cpp:20', 0),
                    ('7ae:	bne.n	7a6 <main+0x22>', 0x7ae),
                ],
            },
            '/simpleperf_runtest_two_functions_x86_64': {
                'start_addr': 0x920,
                'len': 201,
                'expected_items': [
                    ('main():', 0),
                    ('system/extras/simpleperf/runtest/two_functions.cpp:20', 0),
                    ('96e:      	movl	%edx, (%rbx,%rax,4)', 0x96e),
                ],
            },
            '/simpleperf_runtest_two_functions_x86': {
                'start_addr': 0x710,
                'len': 98,
                'expected_items': [
                    ('main():', 0),
                    ('system/extras/simpleperf/runtest/two_functions.cpp:20', 0),
                    ('748:      	cmpl	$100000000, %ebp', 0x748),
                ],
            },
        }
        binary_finder = BinaryFinder(TestHelper.testdata_dir, ReadElf(TestHelper.ndk_path))
        objdump = Objdump(TestHelper.ndk_path, binary_finder)
        for dso_path in test_map:
            dso = test_map[dso_path]
            dso_info = objdump.get_dso_info(dso_path, None)
            self.assertIsNotNone(dso_info, dso_path)
            disassemble_code = objdump.disassemble_code(dso_info, dso['start_addr'], dso['len'])
            self.assertTrue(disassemble_code, dso_path)
            i = 0
            for expected_line, expected_addr in dso['expected_items']:
                found = False
                while i < len(disassemble_code):
                    line, addr = disassemble_code[i]
                    if addr == expected_addr and expected_line in line:
                        found = True
                        i += 1
                        break
                    i += 1
                if not found:
                    s = '\n'.join('%s:0x%x' % item for item in disassemble_code)
                    self.fail('for %s, %s:0x%x not found in disassemble code:\n%s' %
                              (dso_path, expected_line, expected_addr, s))

    def test_readelf(self):
        test_map = {
            'simpleperf_runtest_two_functions_arm64': {
                'arch': 'arm64',
                'build_id': '0xb4f1b49b0fe9e34e78fb14e5374c930c00000000',
                'sections': ['.note.gnu.build-id', '.dynsym', '.text', '.rodata', '.eh_frame',
                             '.eh_frame_hdr', '.debug_info',  '.debug_line', '.symtab'],
            },
            'simpleperf_runtest_two_functions_arm': {
                'arch': 'arm',
                'build_id': '0x718f5b36c4148ee1bd3f51af89ed2be600000000',
            },
            'simpleperf_runtest_two_functions_x86_64': {
                'arch': 'x86_64',
            },
            'simpleperf_runtest_two_functions_x86': {
                'arch': 'x86',
            }
        }
        readelf = ReadElf(TestHelper.ndk_path)
        for dso_path in test_map:
            dso_info = test_map[dso_path]
            path = os.path.join(TestHelper.testdata_dir, dso_path)
            self.assertEqual(dso_info['arch'], readelf.get_arch(path))
            if 'build_id' in dso_info:
                self.assertEqual(dso_info['build_id'], readelf.get_build_id(path), dso_path)
            if 'sections' in dso_info:
                sections = readelf.get_sections(path)
                for section in dso_info['sections']:
                    self.assertIn(section, sections)
        self.assertEqual(readelf.get_arch('not_exist_file'), 'unknown')
        self.assertEqual(readelf.get_build_id('not_exist_file'), '')
        self.assertEqual(readelf.get_sections('not_exist_file'), [])

    def test_source_file_searcher(self):
        searcher = SourceFileSearcher(
            [TestHelper.testdata_path('SimpleperfExampleWithNative'),
             TestHelper.testdata_path('SimpleperfExampleOfKotlin')])

        def format_path(path):
            return os.path.join(TestHelper.testdata_dir, path.replace('/', os.sep))
        # Find a C++ file with pure file name.
        self.assertEqual(
            format_path('SimpleperfExampleWithNative/app/src/main/cpp/native-lib.cpp'),
            searcher.get_real_path('native-lib.cpp'))
        # Find a C++ file with an absolute file path.
        self.assertEqual(
            format_path('SimpleperfExampleWithNative/app/src/main/cpp/native-lib.cpp'),
            searcher.get_real_path('/data/native-lib.cpp'))
        # Find a Java file.
        self.assertEqual(
            format_path('SimpleperfExampleWithNative/app/src/main/java/com/example/' +
                        'simpleperf/simpleperfexamplewithnative/MainActivity.java'),
            searcher.get_real_path('simpleperfexamplewithnative/MainActivity.java'))
        # Find a Kotlin file.
        self.assertEqual(
            format_path('SimpleperfExampleOfKotlin/app/src/main/java/com/example/' +
                        'simpleperf/simpleperfexampleofkotlin/MainActivity.kt'),
            searcher.get_real_path('MainActivity.kt'))

    def test_is_elf_file(self):
        self.assertTrue(ReadElf.is_elf_file(TestHelper.testdata_path(
            'simpleperf_runtest_two_functions_arm')))
        with open('not_elf', 'wb') as fh:
            fh.write(b'\x90123')
        try:
            self.assertFalse(ReadElf.is_elf_file('not_elf'))
        finally:
            remove('not_elf')

    def test_binary_finder(self):
        # Create binary_cache.
        binary_cache_builder = BinaryCacheBuilder(TestHelper.ndk_path, False)
        elf_name = 'simpleperf_runtest_two_functions_arm'
        elf_path = TestHelper.testdata_path(elf_name)
        readelf = ReadElf(TestHelper.ndk_path)
        build_id = readelf.get_build_id(elf_path)
        self.assertGreater(len(build_id), 0)
        binary_cache_builder.binaries[elf_name] = build_id
        binary_cache_builder.copy_binaries_from_symfs_dirs([TestHelper.testdata_dir])
        binary_cache_builder.create_build_id_list()

        # Test BinaryFinder.
        path_in_binary_cache = Path(binary_cache_builder.binary_cache_dir, elf_name)
        binary_finder = BinaryFinder(binary_cache_builder.binary_cache_dir, readelf)
        # Find binary using build id.
        path = binary_finder.find_binary('[not_exist_file]', build_id)
        self.assertEqual(path, path_in_binary_cache)
        # Find binary using path.
        path = binary_finder.find_binary('/' + elf_name, None)
        self.assertEqual(path, path_in_binary_cache)
        # Find binary using absolute path.
        path = binary_finder.find_binary(str(path_in_binary_cache), None)
        self.assertEqual(path, path_in_binary_cache)

        # The binary should has a matched build id.
        path = binary_finder.find_binary('/' + elf_name, 'wrong_build_id')
        self.assertIsNone(path)

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

from simpleperf_utils import remove
from . app_test import TestExampleBase
from . test_utils import INFERNO_SCRIPT, TestHelper


class TestExampleWithNative(TestExampleBase):
    @classmethod
    def setUpClass(cls):
        cls.prepare("SimpleperfExampleWithNative",
                    "com.example.simpleperf.simpleperfexamplewithnative",
                    ".MainActivity")

    def test_app_profiler(self):
        self.common_test_app_profiler()

    def test_app_profiler_profile_from_launch(self):
        self.run_app_profiler(start_activity=True, build_binary_cache=False)
        self.run_cmd(["report.py", "-g", "-o", "report.txt"])
        self.check_strings_in_file("report.txt", ["BusyLoopThread", "__start_thread"])

    def test_report(self):
        self.common_test_report()
        self.run_cmd(["report.py", "-g", "-o", "report.txt"])
        self.check_strings_in_file("report.txt", ["BusyLoopThread", "__start_thread"])

    def test_annotate(self):
        self.common_test_annotate()
        self.check_file_under_dir("annotated_files", "native-lib.cpp")
        summary_file = os.path.join("annotated_files", "summary")
        self.check_annotation_summary(summary_file, [
            ("native-lib.cpp", 20, 0),
            ("BusyLoopThread", 20, 0),
            ("line 46", 20, 0)])

    def test_report_sample(self):
        self.common_test_report_sample(
            ["BusyLoopThread",
             "__start_thread"])

    def test_pprof_proto_generator(self):
        check_strings_with_lines = [
            "native-lib.cpp",
            "BusyLoopThread",
            # Check if dso name in perf.data is replaced by binary path in binary_cache.
            'filename: binary_cache']
        self.common_test_pprof_proto_generator(
            check_strings_with_lines,
            check_strings_without_lines=["BusyLoopThread"])

    def test_inferno(self):
        self.common_test_inferno()
        self.run_app_profiler()
        self.run_cmd([INFERNO_SCRIPT, "-sc"])
        self.check_inferno_report_html([('BusyLoopThread', 20)])

    def test_report_html(self):
        self.common_test_report_html()
        self.run_cmd(['report_html.py', '--add_source_code', '--source_dirs', 'testdata',
                      '--add_disassembly', '--binary_filter', "libnative-lib.so"])


class TestExampleWithNativeRoot(TestExampleBase):
    @classmethod
    def setUpClass(cls):
        cls.prepare("SimpleperfExampleWithNative",
                    "com.example.simpleperf.simpleperfexamplewithnative",
                    ".MainActivity",
                    adb_root=True)

    def test_app_profiler(self):
        self.common_test_app_profiler()


class TestExampleWithNativeTraceOffCpu(TestExampleBase):
    @classmethod
    def setUpClass(cls):
        cls.prepare("SimpleperfExampleWithNative",
                    "com.example.simpleperf.simpleperfexamplewithnative",
                    ".SleepActivity")

    def test_smoke(self):
        self.run_app_profiler(record_arg="-g -f 1000 --duration 10 -e cpu-cycles:u --trace-offcpu")
        self.run_cmd(["report.py", "-g", "--comms", "SleepThread", "-o", "report.txt"])
        self.check_strings_in_file("report.txt", [
            "SleepThread(void*)",
            "RunFunction()",
            "SleepFunction(unsigned long long)"])
        remove("annotated_files")
        self.run_cmd(["annotate.py", "-s", self.example_path, "--comm", "SleepThread"])
        self.check_exist(dirname="annotated_files")
        self.check_file_under_dir("annotated_files", "native-lib.cpp")
        summary_file = os.path.join("annotated_files", "summary")
        self.check_annotation_summary(summary_file, [
            ("native-lib.cpp", 80, 20),
            ("SleepThread", 80, 0),
            ("RunFunction", 20, 20),
            ("SleepFunction", 20, 0),
            ("line 73", 20, 0),
            ("line 83", 20, 0)])
        self.run_cmd([INFERNO_SCRIPT, "-sc"])
        self.check_inferno_report_html([('SleepThread', 80),
                                        ('RunFunction', 20),
                                        ('SleepFunction', 20)])


class TestExampleWithNativeJniCall(TestExampleBase):
    @classmethod
    def setUpClass(cls):
        cls.prepare("SimpleperfExampleWithNative",
                    "com.example.simpleperf.simpleperfexamplewithnative",
                    ".MixActivity")

    def test_smoke(self):
        self.run_app_profiler()
        self.run_cmd(["report.py", "-g", "--comms", "BusyThread", "-o", "report.txt"])
        self.check_strings_in_file("report.txt", [
            "com.example.simpleperf.simpleperfexamplewithnative.MixActivity$1.run",
            "Java_com_example_simpleperf_simpleperfexamplewithnative_MixActivity_callFunction"])
        remove("annotated_files")
        self.run_cmd(["annotate.py", "-s", self.example_path, "--comm", "BusyThread"])
        self.check_exist(dirname="annotated_files")
        self.check_file_under_dir("annotated_files", "native-lib.cpp")
        summary_file = os.path.join("annotated_files", "summary")
        self.check_annotation_summary(summary_file, [("native-lib.cpp", 5, 0), ("line 40", 5, 0)])
        if self.use_compiled_java_code:
            self.check_file_under_dir("annotated_files", "MixActivity.java")
            self.check_annotation_summary(summary_file, [
                ("MixActivity.java", 80, 0),
                ("run", 80, 0),
                ("line 26", 20, 0),
                ("native-lib.cpp", 5, 0),
                ("line 40", 5, 0)])

        self.run_cmd([INFERNO_SCRIPT, "-sc"])


class TestExampleWithNativeForce32Bit(TestExampleWithNative):
    @classmethod
    def setUpClass(cls):
        cls.prepare("SimpleperfExampleWithNative",
                    "com.example.simpleperf.simpleperfexamplewithnative",
                    ".MainActivity",
                    abi=TestHelper.get_32bit_abi())


class TestExampleWithNativeRootForce32Bit(TestExampleWithNativeRoot):
    @classmethod
    def setUpClass(cls):
        cls.prepare("SimpleperfExampleWithNative",
                    "com.example.simpleperf.simpleperfexamplewithnative",
                    ".MainActivity",
                    abi=TestHelper.get_32bit_abi(),
                    adb_root=False)


class TestExampleWithNativeTraceOffCpuForce32Bit(TestExampleWithNativeTraceOffCpu):
    @classmethod
    def setUpClass(cls):
        cls.prepare("SimpleperfExampleWithNative",
                    "com.example.simpleperf.simpleperfexamplewithnative",
                    ".SleepActivity",
                    abi=TestHelper.get_32bit_abi())

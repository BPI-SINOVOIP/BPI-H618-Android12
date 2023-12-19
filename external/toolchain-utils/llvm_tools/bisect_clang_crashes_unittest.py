#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for bisect_clang_crashes."""

# pylint: disable=cros-logging-import
import glob
import logging
import os.path
import subprocess
import unittest
import unittest.mock as mock

import bisect_clang_crashes


class Test(unittest.TestCase):
  """Tests for bisect_clang_crashes."""

  class _SilencingFilter(object):
    """Silences all log messages.

    Also collects info about log messages that would've been emitted.
    """

    def __init__(self):
      self.messages = []

    def filter(self, record):
      self.messages.append(record.getMessage())
      return 0

  @mock.patch.object(subprocess, 'check_output')
  def test_get_artifacts(self, mock_gsutil_ls):
    pattern = 'gs://chromeos-toolchain-artifacts/clang-crash-diagnoses/' \
              '**/*clang_crash_diagnoses.tar.xz'
    mock_gsutil_ls.return_value = 'artifact1\nartifact2\nartifact3'
    results = bisect_clang_crashes.get_artifacts(pattern)
    self.assertEqual(results, ['artifact1', 'artifact2', 'artifact3'])
    mock_gsutil_ls.assert_called_once_with(['gsutil.py', 'ls', pattern],
                                           stderr=subprocess.STDOUT,
                                           encoding='utf-8')

  @mock.patch.object(os.path, 'exists')
  @mock.patch.object(glob, 'glob')
  def test_get_crash_reproducers_succeed(self, mock_file_search,
                                         mock_file_check):
    working_dir = 'SomeDirectory'
    mock_file_search.return_value = ['a.c', 'b.cpp', 'c.cc']
    mock_file_check.side_effect = [True, True, True]
    results = bisect_clang_crashes.get_crash_reproducers(working_dir)
    mock_file_search.assert_called_once_with('%s/*.c*' % working_dir)
    self.assertEqual(mock_file_check.call_count, 3)
    self.assertEqual(mock_file_check.call_args_list[0], mock.call('a.sh'))
    self.assertEqual(mock_file_check.call_args_list[1], mock.call('b.sh'))
    self.assertEqual(mock_file_check.call_args_list[2], mock.call('c.sh'))
    self.assertEqual(results, [('a.c', 'a.sh'), ('b.cpp', 'b.sh'),
                               ('c.cc', 'c.sh')])

  @mock.patch.object(os.path, 'exists')
  @mock.patch.object(glob, 'glob')
  def test_get_crash_reproducers_no_matching_script(self, mock_file_search,
                                                    mock_file_check):

    def silence_logging():
      root = logging.getLogger()
      filt = self._SilencingFilter()
      root.addFilter(filt)
      self.addCleanup(root.removeFilter, filt)
      return filt

    log_filter = silence_logging()
    working_dir = 'SomeDirectory'
    mock_file_search.return_value = ['a.c', 'b.cpp', 'c.cc']
    mock_file_check.side_effect = [True, False, True]
    results = bisect_clang_crashes.get_crash_reproducers(working_dir)
    mock_file_search.assert_called_once_with('%s/*.c*' % working_dir)
    self.assertEqual(mock_file_check.call_count, 3)
    self.assertEqual(mock_file_check.call_args_list[0], mock.call('a.sh'))
    self.assertEqual(mock_file_check.call_args_list[1], mock.call('b.sh'))
    self.assertEqual(mock_file_check.call_args_list[2], mock.call('c.sh'))
    self.assertEqual(results, [('a.c', 'a.sh'), ('c.cc', 'c.sh')])
    self.assertTrue(
        any('could not find the matching script of b.cpp' in x
            for x in log_filter.messages), log_filter.messages)


if __name__ == '__main__':
  unittest.main()

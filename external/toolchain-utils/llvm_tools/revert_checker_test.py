#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for revert_checker."""

from __future__ import print_function

# pylint: disable=cros-logging-import
import logging
import unittest

import llvm_project
import revert_checker

# pylint: disable=protected-access


class _SilencingFilter(object):
  """Silences all log messages.

  Also collects info about log messages that would've been emitted.
  """

  def __init__(self):
    self.messages = []

  def filter(self, record):
    self.messages.append(record.getMessage())
    return 0


class Test(unittest.TestCase):
  """Tests for revert_checker."""

  def silence_logging(self):
    root = logging.getLogger()
    filt = _SilencingFilter()
    root.addFilter(filt)
    self.addCleanup(root.removeFilter, filt)
    return filt

  def test_known_log_stream(self):
    start_sha = 'e241573d5972d34a323fa5c64774c4207340beb3'
    end_sha = 'a7a37517751ffb0f5529011b4ba96e67fcb27510'
    commits = [
        revert_checker._LogEntry(
            'e241573d5972d34a323fa5c64774c4207340beb3', '\n'.join((
                '[mlir] NFC: remove IntegerValueSet / MutableIntegerSet',
                '',
                'Summary:',
                '- these are unused and really not needed now given flat '
                'affine',
                '  constraints',
                '',
                'Differential Revision: https://reviews.llvm.org/D75792',
            ))),
        revert_checker._LogEntry(
            '97572fa6e9daecd648873496fd11f7d1e25a55f0',
            '[NFC] use hasAnyOperatorName and hasAnyOverloadedOperatorName '
            'functions in clang-tidy matchers',
        ),
    ]

    logs = list(
        revert_checker._log_stream(
            llvm_project.get_location(),
            root_sha=start_sha,
            end_at_sha=end_sha,
        ))
    self.assertEqual(commits, logs)

  def test_reverted_noncommit_object_is_a_nop(self):
    log_filter = self.silence_logging()
    # c9944df916e41b1014dff5f6f75d52297b48ecdc mentions reverting a non-commit
    # object. It sits between the given base_ref and root.
    reverts = revert_checker.find_reverts(
        git_dir=llvm_project.get_location(),
        across_ref='c9944df916e41b1014dff5f6f75d52297b48ecdc~',
        root='c9944df916e41b1014dff5f6f75d52297b48ecdc')
    self.assertEqual(reverts, [])

    complaint = ('Failed to resolve reverted object '
                 'edd18355be574122aaa9abf58c15d8c50fb085a1')
    self.assertTrue(
        any(x.startswith(complaint) for x in log_filter.messages),
        log_filter.messages)

  def test_known_reverts_across_previous_llvm_next_rev(self):
    # c9944df916e41b1014dff5f6f75d52297b48ecdc mentions reverting a non-commit
    # object. It sits between the given base_ref and root.
    reverts = revert_checker.find_reverts(
        git_dir=llvm_project.get_location(),
        across_ref='c47f971694be0159ffddfee8a75ae515eba91439',
        root='9f981e9adf9c8d29bb80306daf08d2770263ade6')
    self.assertEqual(reverts, [
        revert_checker.Revert(
            sha='9f981e9adf9c8d29bb80306daf08d2770263ade6',
            reverted_sha='4060016fce3e6a0b926ee9fc59e440a612d3a2ec'),
        revert_checker.Revert(
            sha='4e0fe038f438ae1679eae9e156e1f248595b2373',
            reverted_sha='65b21282c710afe9c275778820c6e3c1cf46734b'),
    ])


if __name__ == '__main__':
  llvm_project.ensure_up_to_date()
  unittest.main()

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for upload_lexan_crashes_to_forcey."""

import datetime
import unittest
import unittest.mock

import upload_lexan_crashes_to_forcey


class Test(unittest.TestCase):
  """Tests for upload_lexan_crashes_to_forcey."""

  def test_date_parsing_functions(self):
    self.assertEqual(
        datetime.date(2020, 2, 1),
        upload_lexan_crashes_to_forcey.from_ymd('2020-02-01'))

  @unittest.mock.patch(
      'upload_lexan_crashes_to_forcey.test_cases_on_or_after',
      return_value=(
          (
              datetime.date(2020, 1, 1),
              ('gs://test-case-1', 'gs://test-case-1.1'),
          ),
          (datetime.date(2020, 1, 2), ('gs://test-case-2',)),
          (datetime.date(2020, 1, 1), ('gs://test-case-3',)),
          (datetime.date(2020, 1, 4), ('gs://test-case-4',)),
      ))
  @unittest.mock.patch('upload_lexan_crashes_to_forcey.submit_test_case')
  @unittest.mock.patch('upload_lexan_crashes_to_forcey.persist_state')
  def test_new_test_case_submission_functions(self, persist_state_mock,
                                              submit_test_case_mock,
                                              test_cases_on_or_after_mock):
    forcey_path = '/path/to/4c'
    real_state_file_path = '/path/to/state/file'
    earliest_date = datetime.date(2020, 1, 1)

    persist_state_calls = []

    # Since the set this gets is mutated, we need to copy it somehow.
    def persist_state_side_effect(test_cases_to_persist, state_file_path,
                                  most_recent_date):
      self.assertEqual(state_file_path, real_state_file_path)
      persist_state_calls.append(
          (sorted(test_cases_to_persist), most_recent_date))

    persist_state_mock.side_effect = persist_state_side_effect

    upload_lexan_crashes_to_forcey.submit_new_test_cases(
        last_seen_test_cases=(
            'gs://test-case-0',
            'gs://test-case-1',
        ),
        earliest_date_to_check=earliest_date,
        forcey=forcey_path,
        state_file_path=real_state_file_path,
    )

    test_cases_on_or_after_mock.assert_called_once_with(earliest_date)
    self.assertEqual(submit_test_case_mock.call_args_list, [
        unittest.mock.call('gs://test-case-1.1', forcey_path),
        unittest.mock.call('gs://test-case-2', forcey_path),
        unittest.mock.call('gs://test-case-3', forcey_path),
        unittest.mock.call('gs://test-case-4', forcey_path),
    ])

    self.assertEqual(persist_state_calls, [
        (
            ['gs://test-case-0', 'gs://test-case-1', 'gs://test-case-1.1'],
            datetime.date(2020, 1, 1),
        ),
        (
            [
                'gs://test-case-0',
                'gs://test-case-1',
                'gs://test-case-1.1',
                'gs://test-case-2',
            ],
            datetime.date(2020, 1, 2),
        ),
        (
            [
                'gs://test-case-0',
                'gs://test-case-1',
                'gs://test-case-1.1',
                'gs://test-case-2',
                'gs://test-case-3',
            ],
            datetime.date(2020, 1, 2),
        ),
        (
            [
                'gs://test-case-0',
                'gs://test-case-1',
                'gs://test-case-1.1',
                'gs://test-case-2',
                'gs://test-case-3',
                'gs://test-case-4',
            ],
            datetime.date(2020, 1, 4),
        ),
        (
            [
                'gs://test-case-1',
                'gs://test-case-1.1',
                'gs://test-case-2',
                'gs://test-case-3',
                'gs://test-case-4',
            ],
            datetime.date(2020, 1, 4),
        ),
    ])


if __name__ == '__main__':
  unittest.main()

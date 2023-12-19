#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Fetches and submits the latest test-cases from Lexan's crash bucket."""

# pylint: disable=cros-logging-import

import argparse
import contextlib
import datetime
import json
import logging
import os
import shutil
import subprocess
import sys
import tempfile
from typing import Generator, List, Iterable

gsurl_base = 'gs://chrome-clang-crash-reports/v1'


def gsutil_ls(loc: str) -> List[str]:
  results = subprocess.run(['gsutil.py', 'ls', loc],
                           stdout=subprocess.PIPE,
                           check=True,
                           encoding='utf-8')
  return [l.strip() for l in results.stdout.splitlines()]


def gsurl_ls_last_numbers(url: str) -> List[int]:
  return sorted(int(x.rstrip('/').split('/')[-1]) for x in gsutil_ls(url))


def get_available_year_numbers() -> List[int]:
  return gsurl_ls_last_numbers(gsurl_base)


def get_available_month_numbers(year: int) -> List[int]:
  return gsurl_ls_last_numbers(f'{gsurl_base}/{year}')


def get_available_day_numbers(year: int, month: int) -> List[int]:
  return gsurl_ls_last_numbers(f'{gsurl_base}/{year}/{month:02d}')


def get_available_test_case_urls(year: int, month: int, day: int) -> List[str]:
  return gsutil_ls(f'{gsurl_base}/{year}/{month:02d}/{day:02d}')


def test_cases_on_or_after(date: datetime.datetime
                          ) -> Generator[str, None, None]:
  """Yields all test-cases submitted on or after the given date."""
  for year in get_available_year_numbers():
    if year < date.year:
      continue

    for month in get_available_month_numbers(year):
      if year == date.year and month < date.month:
        continue

      for day in get_available_day_numbers(year, month):
        when = datetime.date(year, month, day)
        if when < date:
          continue

        yield when, get_available_test_case_urls(year, month, day)


def to_ymd(date: datetime.date) -> str:
  return date.strftime('%Y-%m-%d')


def from_ymd(date_str: str) -> datetime.date:
  return datetime.datetime.strptime(date_str, '%Y-%m-%d').date()


def persist_state(seen_urls: Iterable[str], state_file: str,
                  current_date: datetime.date):
  tmp_state_file = state_file + '.tmp'
  with open(tmp_state_file, 'w', encoding='utf-8') as f:
    json.dump(
        {
            'already_seen': sorted(seen_urls),
            'most_recent_date': to_ymd(current_date),
        },
        f,
    )
  os.rename(tmp_state_file, state_file)


@contextlib.contextmanager
def temp_dir() -> Generator[str, None, None]:
  loc = tempfile.mkdtemp('lexan-autosubmit')
  try:
    yield loc
  finally:
    shutil.rmtree(loc)


def submit_test_case(gs_url: str, cr_tool: str) -> None:
  logging.info('Submitting %s', gs_url)
  suffix = os.path.splitext(gs_url)[1]
  with temp_dir() as tempdir:
    target_name = 'test_case' + suffix
    target = os.path.join(tempdir, target_name)
    subprocess.run(['gsutil.py', 'cp', gs_url, target], check=True)
    subprocess.run(['tar', 'xaf', target_name], check=True, cwd=tempdir)
    os.unlink(target)

    # Sometimes (e.g., in
    # gs://chrome-clang-crash-reports/v1/2020/03/27/
    # chromium.clang-ToTiOS-12754-GTXToolKit-2bfcde.tgz)
    # we'll get `.crash` files. Unclear why, but let's filter them out anyway.
    repro_files = [x for x in os.listdir(tempdir) if not x.endswith('.crash')]
    assert len(repro_files) == 2, repro_files
    if repro_files[0].endswith('.sh'):
      sh_file, src_file = repro_files
      assert not src_file.endswith('.sh'), repro_files
    else:
      src_file, sh_file = repro_files
      assert sh_file.endswith('.sh'), repro_files

    subprocess.run(
        [
            cr_tool,
            'reduce',
            '-stream=false',
            '-wait=false',
            '-note',
            gs_url,
            '-sh_file',
            os.path.join(tempdir, sh_file),
            '-src_file',
            os.path.join(tempdir, src_file),
        ],
        check=True,
    )


def submit_new_test_cases(
    last_seen_test_cases: Iterable[str],
    earliest_date_to_check: datetime.date,
    forcey: str,
    state_file_path: str,
) -> None:
  """Submits new test-cases to forcey.

  This will persist state after each test-case is submitted.

  Args:
    last_seen_test_cases: test-cases which have been submitted already, and
      should be skipped if seen again.
    earliest_date_to_check: the earliest date we should consider test-cases
      from.
    forcey: path to the forcey binary.
    state_file_path: path to our state file.
  """
  # `all_test_cases_seen` is the union of all test-cases seen on this and prior
  # invocations. It guarantees, in all cases we care about, that we won't
  # submit the same test-case twice. `test_cases_seen_this_invocation` is
  # persisted as "all of the test-cases we've seen on this and prior
  # invocations" if we successfully submit _all_ test-cases.
  #
  # Since you can visualize the test-cases this script considers as a sliding
  # window that only moves forward, if we saw a test-case on a prior iteration
  # but no longer see it, we'll never see it again (since it fell out of our
  # sliding window by being too old). Hence, keeping it around is
  # pointless.
  #
  # We only persist this minimized set of test-cases if _everything_ succeeds,
  # since if something fails below, there's a chance that we haven't revisited
  # test-cases that we've already seen.
  all_test_cases_seen = set(last_seen_test_cases)
  test_cases_seen_this_invocation = []
  most_recent_date = earliest_date_to_check
  for date, candidates in test_cases_on_or_after(earliest_date_to_check):
    most_recent_date = max(most_recent_date, date)

    for url in candidates:
      test_cases_seen_this_invocation.append(url)
      if url in all_test_cases_seen:
        continue

      all_test_cases_seen.add(url)
      submit_test_case(url, forcey)

      # Persisting on each iteration of this loop isn't free, but it's the
      # easiest way to not resubmit test-cases, and it's good to keep in mind
      # that:
      # - the state file will be small (<12KB, since it only keeps a few days
      #   worth of test-cases after the first run)
      # - in addition to this, we're downloading+unzipping+reuploading multiple
      #   MB of test-case bytes.
      #
      # So comparatively, the overhead here probably isn't an issue.
      persist_state(all_test_cases_seen, state_file_path, most_recent_date)

  persist_state(test_cases_seen_this_invocation, state_file_path,
                most_recent_date)


def main(argv: List[str]):
  logging.basicConfig(
      format='>> %(asctime)s: %(levelname)s: %(filename)s:%(lineno)d: '
      '%(message)s',
      level=logging.INFO,
  )

  my_dir = os.path.dirname(os.path.abspath(__file__))

  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument(
      '--state_file', default=os.path.join(my_dir, 'lexan-state.json'))
  parser.add_argument(
      '--last_date',
      help='The earliest date that we care about. All test cases from here '
      'on will be picked up. Format is YYYY-MM-DD.')
  parser.add_argument(
      '--4c', dest='forcey', required=True, help='Path to a 4c client binary')
  opts = parser.parse_args(argv)

  forcey = opts.forcey
  state_file = opts.state_file
  last_date_str = opts.last_date

  os.makedirs(os.path.dirname(state_file), 0o755, exist_ok=True)

  if last_date_str is None:
    with open(state_file, encoding='utf-8') as f:
      data = json.load(f)
    most_recent_date = from_ymd(data['most_recent_date'])
    submit_new_test_cases(
        last_seen_test_cases=data['already_seen'],
        # Note that we always subtract one day from this to avoid a race:
        # uploads may appear slightly out-of-order (or builders may lag, or
        # ...), so the last test-case uploaded for 2020/01/01 might appear
        # _after_ the first test-case for 2020/01/02. Assuming that builders
        # won't lag behind for over a day, the easiest way to handle this is to
        # always check the previous and current days.
        earliest_date_to_check=most_recent_date - datetime.timedelta(days=1),
        forcey=forcey,
        state_file_path=state_file,
    )
  else:
    submit_new_test_cases(
        last_seen_test_cases=(),
        earliest_date_to_check=from_ymd(last_date_str),
        forcey=forcey,
        state_file_path=state_file,
    )


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))

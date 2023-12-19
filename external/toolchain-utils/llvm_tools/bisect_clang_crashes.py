#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Fetches and submits the artifacts from Chrome OS toolchain's crash bucket.
"""

# pylint: disable=cros-logging-import

import argparse
import glob
import json
import logging
import os
import os.path
import shutil
import subprocess
import sys

import chroot


def get_artifacts(pattern):
  results = subprocess.check_output(['gsutil.py', 'ls', pattern],
                                    stderr=subprocess.STDOUT,
                                    encoding='utf-8')
  return sorted(l.strip() for l in results.splitlines())


def get_crash_reproducers(working_dir):
  results = []
  for src in [
      f for f in glob.glob('%s/*.c*' % working_dir)
      if f.split('.')[-1] in ['c', 'cc', 'cpp']
  ]:
    script = '.'.join(src.split('.')[:-1]) + '.sh'
    if not os.path.exists(script):
      logging.warning('could not find the matching script of %s', src)
    else:
      results.append((src, script))
  return results


def submit_crash_to_forcey(forcey: str, temporary_directory: str,
                           buildbucket_id: str, url: str) -> None:
  dest_dir = os.path.join(temporary_directory, buildbucket_id)
  dest_file = os.path.join(dest_dir, os.path.basename(url))
  logging.info('Downloading and submitting %r...', url)
  subprocess.check_output(['gsutil.py', 'cp', url, dest_file],
                          stderr=subprocess.STDOUT)
  subprocess.check_output(['tar', '-xJf', dest_file], cwd=dest_dir)
  for src, script in get_crash_reproducers(dest_dir):
    subprocess.check_output([
        forcey, 'reduce', '-wait=false', '-note',
        '%s:%s' % (url, src), '-sh_file', script, '-src_file', src
    ])


def main(argv):
  chroot.VerifyOutsideChroot()
  logging.basicConfig(
      format='%(asctime)s: %(levelname)s: %(filename)s:%(lineno)d: %(message)s',
      level=logging.INFO,
  )
  cur_dir = os.path.dirname(os.path.abspath(__file__))
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument(
      '--4c', dest='forcey', required=True, help='Path to a 4c client binary')
  parser.add_argument(
      '--state_file',
      default=os.path.join(cur_dir, 'chromeos-state.json'),
      help='The path to the state file.')
  parser.add_argument(
      '--nocleanup',
      action='store_false',
      dest='cleanup',
      help='Keep temporary files created after the script finishes.')
  opts = parser.parse_args(argv)

  state_file = os.path.abspath(opts.state_file)
  os.makedirs(os.path.dirname(state_file), exist_ok=True)
  temporary_directory = '/tmp/bisect_clang_crashes'
  os.makedirs(temporary_directory, exist_ok=True)
  urls = get_artifacts('gs://chromeos-toolchain-artifacts/clang-crash-diagnoses'
                       '/**/*clang_crash_diagnoses.tar.xz')
  logging.info('%d crash URLs found', len(urls))

  visited = {}
  if os.path.exists(state_file):
    buildbucket_ids = {url.split('/')[-2] for url in urls}
    with open(state_file, encoding='utf-8') as f:
      data = json.load(f)
      visited = {k: v for k, v in data.items() if k in buildbucket_ids}
    logging.info('Successfully loaded %d previously-submitted crashes',
                 len(visited))

  try:
    for url in urls:
      splits = url.split('/')
      buildbucket_id = splits[-2]
      # Skip the builds that has been processed
      if buildbucket_id in visited:
        continue
      submit_crash_to_forcey(
          forcey=opts.forcey,
          temporary_directory=temporary_directory,
          buildbucket_id=buildbucket_id,
          url=url,
      )
      visited[buildbucket_id] = url

    exception_in_flight = False
  except:
    exception_in_flight = True
    raise
  finally:
    if exception_in_flight:
      # This is best-effort. If the machine powers off or similar, we'll just
      # resubmit the same crashes, which is suboptimal, but otherwise
      # acceptable.
      logging.error('Something went wrong; attempting to save our work...')
    else:
      logging.info('Persisting state...')

    tmp_state_file = state_file + '.tmp'
    with open(tmp_state_file, 'w', encoding='utf-8') as f:
      json.dump(visited, f, indent=2)
    os.rename(tmp_state_file, state_file)

    logging.info('State successfully persisted')

  if opts.cleanup:
    shutil.rmtree(temporary_directory)


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))

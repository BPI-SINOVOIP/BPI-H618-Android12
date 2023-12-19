#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A crontab script to delete night test data."""

from __future__ import print_function

__author__ = 'shenhan@google.com (Han Shen)'

import argparse
import datetime
import os
import re
import shutil
import shlex
import sys
import time

from cros_utils import command_executer
from cros_utils import constants
from cros_utils import misc

DIR_BY_WEEKDAY = ('Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun')
NIGHTLY_TESTS_WORKSPACE = os.path.join(constants.CROSTC_WORKSPACE,
                                       'nightly-tests')


def CleanNumberedDir(s, dry_run=False):
  """Deleted directories under each dated_dir."""
  chromeos_dirs = [
      os.path.join(s, x)
      for x in os.listdir(s)
      if misc.IsChromeOsTree(os.path.join(s, x))
  ]
  ce = command_executer.GetCommandExecuter(log_level='none')
  all_succeeded = True
  for cd in chromeos_dirs:
    if misc.DeleteChromeOsTree(cd, dry_run=dry_run):
      print('Successfully removed chromeos tree "{0}".'.format(cd))
    else:
      all_succeeded = False
      print('Failed to remove chromeos tree "{0}", please check.'.format(cd))

  if not all_succeeded:
    print('Failed to delete at least one chromeos tree, please check.')
    return False

  ## Now delete the numbered dir Before forcibly removing the directory, just
  ## check 's' to make sure it matches the expected pattern.  A valid dir to be
  ## removed must be '/usr/local/google/crostc/(SUN|MON|TUE...|SAT)'.
  valid_dir_pattern = ('^' + NIGHTLY_TESTS_WORKSPACE + '/(' +
                       '|'.join(DIR_BY_WEEKDAY) + ')')
  if not re.search(valid_dir_pattern, s):
    print('Trying to delete an invalid dir "{0}" (must match "{1}"), '
          'please check.'.format(s, valid_dir_pattern))
    return False

  cmd = 'rm -fr {0}'.format(s)
  if dry_run:
    print(cmd)
  else:
    if ce.RunCommand(cmd, print_to_console=False, terminated_timeout=480) == 0:
      print('Successfully removed "{0}".'.format(s))
    else:
      all_succeeded = False
      print('Failed to remove "{0}", please check.'.format(s))
  return all_succeeded


def CleanDatedDir(dated_dir, dry_run=False):
  # List subdirs under dir
  subdirs = [
      os.path.join(dated_dir, x)
      for x in os.listdir(dated_dir)
      if os.path.isdir(os.path.join(dated_dir, x))
  ]
  all_succeeded = True
  for s in subdirs:
    if not CleanNumberedDir(s, dry_run):
      all_succeeded = False
  return all_succeeded


def ProcessArguments(argv):
  """Process arguments."""
  parser = argparse.ArgumentParser(
      description='Automatically delete nightly test data directories.',
      usage='auto_delete_nightly_test_data.py options')
  parser.add_argument(
      '-d',
      '--dry_run',
      dest='dry_run',
      default=False,
      action='store_true',
      help='Only print command line, do not execute anything.')
  parser.add_argument(
      '--days_to_preserve',
      dest='days_to_preserve',
      default=3,
      help=('Specify the number of days (not including today),'
            ' test data generated on these days will *NOT* be '
            'deleted. Defaults to 3.'))
  options = parser.parse_args(argv)
  return options


def CleanChromeOsTmpFiles(chroot_tmp, days_to_preserve, dry_run):
  rv = 0
  ce = command_executer.GetCommandExecuter()
  # Clean chroot/tmp/test_that_* and chroot/tmp/tmpxxxxxx, that were last
  # accessed more than specified time.
  minutes = 1440 * days_to_preserve
  cmd = (r'find {0} -maxdepth 1 -type d '
         r'\( -name "test_that_*"  -amin +{1} -o '
         r'   -name "cros-update*" -amin +{1} -o '
         r'   -name "CrAU_temp_data*" -amin +{1} -o '
         r' -regex "{0}/tmp......" -amin +{1} \) '
         r'-exec bash -c "echo rm -fr {{}}" \; '
         r'-exec bash -c "rm -fr {{}}" \;').format(chroot_tmp, minutes)
  if dry_run:
    print('Going to execute:\n%s' % cmd)
  else:
    rv = ce.RunCommand(cmd, print_to_console=False)
    if rv == 0:
      print('Successfully cleaned chromeos tree tmp directory '
            '"{0}".'.format(chroot_tmp))
    else:
      print('Some directories were not removed under chromeos tree '
            'tmp directory -"{0}".'.format(chroot_tmp))

  return rv


def CleanChromeOsImageFiles(chroot_tmp, subdir_suffix, days_to_preserve,
                            dry_run):
  # Clean files that were last accessed more than the specified time.
  seconds_delta = days_to_preserve * 24 * 3600
  now = time.time()
  errors = 0

  for tmp_dir in os.listdir(chroot_tmp):
    # Directory under /tmp
    tmp_dir = os.path.join(chroot_tmp, tmp_dir)
    if tmp_dir.endswith(subdir_suffix):
      # Tmp directory which ends with subdir_suffix.
      for subdir in os.listdir(tmp_dir):
        # Subdirectories targeted for deletion.
        subdir_path = os.path.join(tmp_dir, subdir)
        if now - os.path.getatime(subdir_path) > seconds_delta:
          if dry_run:
            print('Will run:\nshutil.rmtree({})'.format(subdir_path))
          else:
            try:
              shutil.rmtree(subdir_path)
              print('Successfully cleaned chromeos image autotest directories '
                    'from "{}".'.format(subdir_path))
            except OSError:
              print('Some image autotest directories were not removed from '
                    '"{}".'.format(subdir_path))
              errors += 1

  return errors


def CleanChromeOsTmpAndImages(days_to_preserve=1, dry_run=False):
  """Delete temporaries, images under crostc/chromeos."""
  chromeos_chroot_tmp = os.path.join(constants.CROSTC_WORKSPACE, 'chromeos',
                                     'chroot', 'tmp')
  # Clean files in tmp directory
  rv = CleanChromeOsTmpFiles(chromeos_chroot_tmp, days_to_preserve, dry_run)
  # Clean image files in *-tryjob directories
  rv += CleanChromeOsImageFiles(chromeos_chroot_tmp, '-tryjob',
                                days_to_preserve, dry_run)
  # Clean image files in *-release directories
  rv += CleanChromeOsImageFiles(chromeos_chroot_tmp, '-release',
                                days_to_preserve, dry_run)
  # Clean image files in *-pfq directories
  rv += CleanChromeOsImageFiles(chromeos_chroot_tmp, '-pfq', days_to_preserve,
                                dry_run)
  # Clean image files in *-llvm-next-nightly directories
  rv += CleanChromeOsImageFiles(chromeos_chroot_tmp, '-llvm-next-nightly',
                                days_to_preserve, dry_run)

  return rv


def CleanOldCLs(days_to_preserve='1', dry_run=False):
  """Abandon old CLs created by automation tooling."""
  ce = command_executer.GetCommandExecuter()
  chromeos_root = os.path.join(constants.CROSTC_WORKSPACE, 'chromeos')
  # Find Old CLs.
  old_cls_cmd = ('gerrit --raw search "owner:me status:open age:%sd"' %
                 days_to_preserve)
  _, cls, _ = ce.ChrootRunCommandWOutput(
      chromeos_root, old_cls_cmd, print_to_console=False)
  # Convert any whitespaces to spaces.
  cls = ' '.join(cls.split())
  if not cls:
    return 0

  abandon_cls_cmd = ('gerrit abandon %s' % cls)
  if dry_run:
    print('Going to execute: %s' % abandon_cls_cmd)
    return 0

  return ce.ChrootRunCommand(
      chromeos_root, abandon_cls_cmd, print_to_console=False)


def CleanChromeTelemetryTmpFiles(dry_run):
  rv = 0
  ce = command_executer.GetCommandExecuter()
  tmp_dir = os.path.join(constants.CROSTC_WORKSPACE, 'chromeos', '.cache',
                         'distfiles', 'chrome-src-internal', 'src', 'tmp')
  cmd = f'rm -fr {shlex.quote(tmp_dir)}/tmp*telemetry_Crosperf'
  if dry_run:
    print(f'Going to execute:\n{cmd}')
  else:
    rv = ce.RunCommand(cmd, print_to_console=False)
    if rv == 0:
      print(f'Successfully cleaned chrome tree tmp directory ' f'{tmp_dir!r} .')
    else:
      print(f'Some directories were not removed under chrome tree '
            f'tmp directory {tmp_dir!r}.')
  return rv


def Main(argv):
  """Delete nightly test data directories, tmps and test images."""
  options = ProcessArguments(argv)
  # Function 'isoweekday' returns 1(Monday) - 7 (Sunday).
  d = datetime.datetime.today().isoweekday()
  # We go back 1 week, delete from that day till we are
  # options.days_to_preserve away from today.
  s = d - 7
  e = d - int(options.days_to_preserve)
  rv = 0
  for i in range(s + 1, e):
    if i <= 0:
      ## Wrap around if index is negative.  6 is from i + 7 - 1, because
      ## DIR_BY_WEEKDAY starts from 0, while isoweekday is from 1-7.
      dated_dir = DIR_BY_WEEKDAY[i + 6]
    else:
      dated_dir = DIR_BY_WEEKDAY[i - 1]

    rv += 0 if CleanDatedDir(
        os.path.join(NIGHTLY_TESTS_WORKSPACE, dated_dir),
        options.dry_run) else 1

  ## Clean temporaries, images under crostc/chromeos
  rv2 = CleanChromeOsTmpAndImages(
      int(options.days_to_preserve), options.dry_run)

  # Clean CLs that are not updated in last 2 weeks.
  rv3 = CleanOldCLs('14', options.dry_run)

  # Clean telemetry temporaries from chrome source tree inside chroot.
  rv4 = CleanChromeTelemetryTmpFiles(options.dry_run)

  return rv + rv2 + rv3 + rv4


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)

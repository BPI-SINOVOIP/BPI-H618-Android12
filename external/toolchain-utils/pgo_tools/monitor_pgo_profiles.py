#!/usr/bin/env python3
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Emails the mage if PGO profile generation hasn't succeeded recently."""

# pylint: disable=cros-logging-import

import argparse
import datetime
import sys
import subprocess
import logging
from typing import List, NamedTuple, Optional, Tuple

from cros_utils import email_sender
from cros_utils import tiny_render

PGO_BUILDBOT_LINK = ('https://ci.chromium.org/p/chromeos/builders/toolchain/'
                     'pgo-generate-llvm-next-orchestrator')


class ProfdataInfo(NamedTuple):
  """Data about an llvm profdata in our gs:// bucket."""
  date: datetime.datetime
  location: str


def parse_date(date: str) -> datetime.datetime:
  time_format = '%Y-%m-%dT%H:%M:%SZ'
  if not date.endswith('Z'):
    time_format += '%z'
  return datetime.datetime.strptime(date, time_format)


def fetch_most_recent_profdata(arch: str) -> ProfdataInfo:
  result = subprocess.run(
      [
          'gsutil.py',
          'ls',
          '-l',
          f'gs://chromeos-toolchain-artifacts/llvm-pgo/{arch}/'
          '*.profdata.tar.xz',
      ],
      check=True,
      stdout=subprocess.PIPE,
      encoding='utf-8',
  )

  # Each line will be a profdata; the last one is a summary, so drop it.
  infos = []
  for rec in result.stdout.strip().splitlines()[:-1]:
    _size, date, url = rec.strip().split()
    infos.append(ProfdataInfo(date=parse_date(date), location=url))
  return max(infos)


def compose_complaint_email(
    out_of_date_profiles: List[Tuple[datetime.datetime, ProfdataInfo]]
) -> Optional[Tuple[str, tiny_render.Piece]]:
  if not out_of_date_profiles:
    return None

  if len(out_of_date_profiles) == 1:
    subject = '1 llvm profile is out of date'
    body = ['out-of-date profile:']
  else:
    subject = f'{len(out_of_date_profiles)} llvm profiles are out of date'
    body = ['out-of-date profiles:']

  out_of_date_items = []
  for arch, profdata_info in out_of_date_profiles:
    out_of_date_items.append(
        f'{arch} (most recent profile was from {profdata_info.date} at '
        f'{profdata_info.location!r})')

  body += [
      tiny_render.UnorderedList(out_of_date_items),
      tiny_render.line_break,
      tiny_render.line_break,
      'PTAL to see if the llvm-pgo-generate bots are functioning normally. '
      'Their status can be found at ',
      tiny_render.Link(href=PGO_BUILDBOT_LINK, inner=PGO_BUILDBOT_LINK),
      '.',
  ]
  return subject, body


def main() -> None:
  logging.basicConfig(level=logging.INFO)

  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument(
      '--dry_run',
      action='store_true',
      help="Don't actually send an email",
  )
  parser.add_argument(
      '--max_age_days',
      # These builders run ~weekly. If we fail to generate two in a row,
      # something's probably wrong.
      default=15,
      type=int,
      help='How old to let profiles get before complaining, in days',
  )
  args = parser.parse_args()

  now = datetime.datetime.now()
  logging.info('Start time is %r', now)

  max_age = datetime.timedelta(days=args.max_age_days)
  out_of_date_profiles = []
  for arch in ('arm', 'arm64', 'amd64'):
    logging.info('Fetching most recent profdata for %r', arch)
    most_recent = fetch_most_recent_profdata(arch)
    logging.info('Most recent profdata for %r is %r', arch, most_recent)

    age = now - most_recent.date
    if age >= max_age:
      out_of_date_profiles.append((arch, most_recent))

  email = compose_complaint_email(out_of_date_profiles)
  if not email:
    logging.info('No email to send; quit')
    return

  subject, body = email

  identifier = 'llvm-pgo-monitor'
  subject = f'[{identifier}] {subject}'

  logging.info('Sending email with title %r', subject)
  if args.dry_run:
    logging.info('Dry run specified\nSubject: %s\nBody:\n%s', subject,
                 tiny_render.render_text_pieces(body))
  else:
    email_sender.EmailSender().SendX20Email(
        subject=subject,
        identifier=identifier,
        well_known_recipients=['mage'],
        direct_recipients=['gbiv@google.com'],
        text_body=tiny_render.render_text_pieces(body),
        html_body=tiny_render.render_html_pieces(body),
    )


if __name__ == '__main__':
  sys.exit(main())

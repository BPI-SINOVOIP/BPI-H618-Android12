#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Checks for various upstream events with the Rust toolchain.

Sends an email if something interesting (probably) happened.
"""

# pylint: disable=cros-logging-import

import argparse
import itertools
import json
import logging
import pathlib
import re
import shutil
import subprocess
import sys
import time
from typing import Any, Dict, Iterable, List, Optional, Tuple, NamedTuple

from cros_utils import email_sender
from cros_utils import tiny_render


def gentoo_sha_to_link(sha: str) -> str:
  """Gets a URL to a webpage that shows the Gentoo commit at `sha`."""
  return f'https://gitweb.gentoo.org/repo/gentoo.git/commit?id={sha}'


def send_email(subject: str, body: List[tiny_render.Piece]) -> None:
  """Sends an email with the given title and body to... whoever cares."""
  email_sender.EmailSender().SendX20Email(
      subject=subject,
      identifier='rust-watch',
      well_known_recipients=['cros-team'],
      text_body=tiny_render.render_text_pieces(body),
      html_body=tiny_render.render_html_pieces(body),
  )


class RustReleaseVersion(NamedTuple):
  """Represents a version of Rust's stable compiler."""
  major: int
  minor: int
  patch: int

  @staticmethod
  def from_string(version_string: str) -> 'RustReleaseVersion':
    m = re.match(r'(\d+)\.(\d+)\.(\d+)', version_string)
    if not m:
      raise ValueError(f"{version_string!r} isn't a valid version string")
    return RustReleaseVersion(*[int(x) for x in m.groups()])

  def __str__(self) -> str:
    return f'{self.major}.{self.minor}.{self.patch}'

  def to_json(self) -> str:
    return str(self)

  @staticmethod
  def from_json(s: str) -> 'RustReleaseVersion':
    return RustReleaseVersion.from_string(s)


class State(NamedTuple):
  """State that we keep around from run to run."""
  # The last Rust release tag that we've seen.
  last_seen_release: RustReleaseVersion

  # We track Gentoo's upstream Rust ebuild. This is the last SHA we've seen
  # that updates it.
  last_gentoo_sha: str

  def to_json(self) -> Dict[str, Any]:
    return {
        'last_seen_release': self.last_seen_release.to_json(),
        'last_gentoo_sha': self.last_gentoo_sha,
    }

  @staticmethod
  def from_json(s: Dict[str, Any]) -> 'State':
    return State(
        last_seen_release=RustReleaseVersion.from_json(s['last_seen_release']),
        last_gentoo_sha=s['last_gentoo_sha'],
    )


def parse_release_tags(lines: Iterable[str]) -> Iterable[RustReleaseVersion]:
  """Parses `git ls-remote --tags` output into Rust stable release versions."""
  refs_tags = 'refs/tags/'
  for line in lines:
    _sha, tag = line.split(None, 1)
    tag = tag.strip()
    # Each tag has an associated 'refs/tags/name^{}', which is the actual
    # object that the tag points to. That's irrelevant to us.
    if tag.endswith('^{}'):
      continue

    if not tag.startswith(refs_tags):
      continue

    short_tag = tag[len(refs_tags):]
    # There are a few old versioning schemes. Ignore them.
    if short_tag.startswith('0.') or short_tag.startswith('release-'):
      continue
    yield RustReleaseVersion.from_string(short_tag)


def fetch_most_recent_release() -> RustReleaseVersion:
  """Fetches the most recent stable `rustc` version."""
  result = subprocess.run(
      ['git', 'ls-remote', '--tags', 'https://github.com/rust-lang/rust'],
      check=True,
      stdin=None,
      capture_output=True,
      encoding='utf-8',
  )
  tag_lines = result.stdout.strip().splitlines()
  return max(parse_release_tags(tag_lines))


class GitCommit(NamedTuple):
  """Represents a single git commit."""
  sha: str
  subject: str


def update_git_repo(git_dir: pathlib.Path) -> None:
  """Updates the repo at `git_dir`, retrying a few times on failure."""
  for i in itertools.count(start=1):
    result = subprocess.run(
        ['git', 'fetch', 'origin'],
        check=False,
        cwd=str(git_dir),
        stdin=None,
    )

    if not result.returncode:
      break

    if i == 5:
      # 5 attempts is too many. Something else may be wrong.
      result.check_returncode()

    sleep_time = 60 * i
    logging.error("Failed updating gentoo's repo; will try again in %ds...",
                  sleep_time)
    time.sleep(sleep_time)


def get_new_gentoo_commits(git_dir: pathlib.Path,
                           most_recent_sha: str) -> List[GitCommit]:
  """Gets commits to dev-lang/rust since `most_recent_sha`.

  Older commits come earlier in the returned list.
  """
  commits = subprocess.run(
      [
          'git',
          'log',
          '--format=%H %s',
          f'{most_recent_sha}..origin/master',
          '--',
          'dev-lang/rust',
      ],
      capture_output=True,
      check=False,
      cwd=str(git_dir),
      encoding='utf-8',
  )

  if commits.returncode:
    logging.error('Error getting new gentoo commits; stderr:\n%s',
                  commits.stderr)
    commits.check_returncode()

  results = []
  for line in commits.stdout.strip().splitlines():
    sha, subject = line.strip().split(None, 1)
    results.append(GitCommit(sha=sha, subject=subject))

  # `git log` outputs things in newest -> oldest order.
  results.reverse()
  return results


def setup_gentoo_git_repo(git_dir: pathlib.Path) -> str:
  """Sets up a gentoo git repo at the given directory. Returns HEAD."""
  subprocess.run(
      [
          'git', 'clone', 'https://anongit.gentoo.org/git/repo/gentoo.git',
          str(git_dir)
      ],
      stdin=None,
      check=True,
  )

  head_rev = subprocess.run(
      ['git', 'rev-parse', 'HEAD'],
      cwd=str(git_dir),
      check=True,
      stdin=None,
      capture_output=True,
      encoding='utf-8',
  )
  return head_rev.stdout.strip()


def read_state(state_file: pathlib.Path) -> State:
  """Reads state from the given file."""
  with state_file.open(encoding='utf-8') as f:
    return State.from_json(json.load(f))


def atomically_write_state(state_file: pathlib.Path, state: State) -> None:
  """Writes state to the given file."""
  temp_file = pathlib.Path(str(state_file) + '.new')
  with temp_file.open('w', encoding='utf-8') as f:
    json.dump(state.to_json(), f)
  temp_file.rename(state_file)


def maybe_compose_email(old_state: State, newest_release: RustReleaseVersion,
                        new_gentoo_commits: List[GitCommit]
                       ) -> Optional[Tuple[str, List[tiny_render.Piece]]]:
  """Creates an email given our new state, if doing so is appropriate."""
  subject_pieces = []
  body_pieces = []

  if newest_release > old_state.last_seen_release:
    subject_pieces.append('new rustc release detected')
    body_pieces.append(f'Rustc tag for v{newest_release} was found.')

  if new_gentoo_commits:
    # Separate the sections a bit for prettier output.
    if body_pieces:
      body_pieces += [tiny_render.line_break, tiny_render.line_break]

    if len(new_gentoo_commits) == 1:
      subject_pieces.append('new rust ebuild commit detected')
      body_pieces.append('commit:')
    else:
      subject_pieces.append('new rust ebuild commits detected')
      body_pieces.append('commits (newest first):')

    commit_lines = []
    for commit in new_gentoo_commits:
      commit_lines.append([
          tiny_render.Link(
              gentoo_sha_to_link(commit.sha),
              commit.sha[:12],
          ),
          f': {commit.subject}',
      ])

    body_pieces.append(tiny_render.UnorderedList(commit_lines))

  if not subject_pieces:
    return None

  subject = '[rust-watch] ' + '; '.join(subject_pieces)
  return subject, body_pieces


def main(argv: List[str]) -> None:
  logging.basicConfig(level=logging.INFO)

  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument(
      '--state_dir', required=True, help='Directory to store state in.')
  parser.add_argument(
      '--skip_email', action='store_true', help="Don't send an email.")
  parser.add_argument(
      '--skip_state_update',
      action='store_true',
      help="Don't update the state file. Doesn't apply to initial setup.")
  opts = parser.parse_args(argv)

  state_dir = pathlib.Path(opts.state_dir)
  state_file = state_dir / 'state.json'
  gentoo_subdir = state_dir / 'upstream-gentoo'
  if not state_file.exists():
    logging.info("state_dir isn't fully set up; doing that now.")

    # Could be in a partially set-up state.
    if state_dir.exists():
      logging.info('incomplete state_dir detected; removing.')
      shutil.rmtree(str(state_dir))

    state_dir.mkdir(parents=True)
    most_recent_release = fetch_most_recent_release()
    most_recent_gentoo_commit = setup_gentoo_git_repo(gentoo_subdir)
    atomically_write_state(
        state_file,
        State(
            last_seen_release=most_recent_release,
            last_gentoo_sha=most_recent_gentoo_commit,
        ),
    )
    # Running through this _should_ be a nop, but do it anyway. Should make any
    # bugs more obvious on the first run of the script.

  prior_state = read_state(state_file)
  logging.info('Last state was %r', prior_state)

  most_recent_release = fetch_most_recent_release()
  logging.info('Most recent Rust release is %s', most_recent_release)

  logging.info('Fetching new commits from Gentoo')
  update_git_repo(gentoo_subdir)
  new_commits = get_new_gentoo_commits(gentoo_subdir,
                                       prior_state.last_gentoo_sha)
  logging.info('New commits: %r', new_commits)

  maybe_email = maybe_compose_email(prior_state, most_recent_release,
                                    new_commits)

  if maybe_email is None:
    logging.info('No updates to send')
  else:
    title, body = maybe_email
    if opts.skip_email:
      logging.info('Skipping sending email with title %r and contents\n%s',
                   title, tiny_render.render_html_pieces(body))
    else:
      logging.info('Sending email')
      send_email(title, body)

  if opts.skip_state_update:
    logging.info('Skipping state update, as requested')
    return

  newest_sha = (
      new_commits[-1].sha if new_commits else prior_state.last_gentoo_sha)
  atomically_write_state(
      state_file,
      State(
          last_seen_release=most_recent_release,
          last_gentoo_sha=newest_sha,
      ),
  )


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Checks for reverts of commits across a given git commit.

To clarify the meaning of 'across' with an example, if we had the following
commit history (where `a -> b` notes that `b` is a direct child of `a`):

123abc -> 223abc -> 323abc -> 423abc -> 523abc

And where 423abc is a revert of 223abc, this revert is considered to be 'across'
323abc. More generally, a revert A of a parent commit B is considered to be
'across' a commit C if C is a parent of A and B is a parent of C.

Please note that revert detection in general is really difficult, since merge
conflicts/etc always introduce _some_ amount of fuzziness. This script just
uses a bundle of heuristics, and is bound to ignore / incorrectly flag some
reverts. The hope is that it'll easily catch the vast majority (>90%) of them,
though.
"""

# pylint: disable=cros-logging-import

from __future__ import print_function

import argparse
import collections
import logging
import re
import subprocess
import sys
import typing as t

# People are creative with their reverts, and heuristics are a bit difficult.
# Like 90% of of reverts have "This reverts commit ${full_sha}".
# Some lack that entirely, while others have many of them specified in ad-hoc
# ways, while others use short SHAs and whatever.
#
# The 90% case is trivial to handle (and 100% free + automatic). The extra 10%
# starts involving human intervention, which is probably not worth it for now.


def _try_parse_reverts_from_commit_message(commit_message: str) -> t.List[str]:
  if not commit_message:
    return []

  results = re.findall(r'This reverts commit ([a-f0-9]{40})\b', commit_message)

  first_line = commit_message.splitlines()[0]
  initial_revert = re.match(r'Revert ([a-f0-9]{6,}) "', first_line)
  if initial_revert:
    results.append(initial_revert.group(1))
  return results


def _stream_stdout(command: t.List[str]) -> t.Generator[str, None, None]:
  with subprocess.Popen(
      command, stdout=subprocess.PIPE, encoding='utf-8', errors='replace') as p:
    yield from p.stdout


def _resolve_sha(git_dir: str, sha: str) -> str:
  if len(sha) == 40:
    return sha

  return subprocess.check_output(
      ['git', '-C', git_dir, 'rev-parse', sha],
      encoding='utf-8',
      stderr=subprocess.DEVNULL,
  ).strip()


_LogEntry = t.NamedTuple('_LogEntry', [
    ('sha', str),
    ('commit_message', t.List[str]),
])


def _log_stream(git_dir: str, root_sha: str,
                end_at_sha: str) -> t.Iterable[_LogEntry]:
  sep = 50 * '<>'
  log_command = [
      'git',
      '-C',
      git_dir,
      'log',
      '^' + end_at_sha,
      root_sha,
      '--format=' + sep + '%n%H%n%B%n',
  ]

  stdout_stream = iter(_stream_stdout(log_command))

  # Find the next separator line. If there's nothing to log, it may not exist.
  # It might not be the first line if git feels complainy.
  found_commit_header = False
  for line in stdout_stream:
    if line.rstrip() == sep:
      found_commit_header = True
      break

  while found_commit_header:
    # crbug.com/1041148
    # pylint: disable=stop-iteration-return
    sha = next(stdout_stream, None)
    assert sha is not None, 'git died?'
    sha = sha.rstrip()

    commit_message = []

    found_commit_header = False
    for line in stdout_stream:
      line = line.rstrip()
      if line.rstrip() == sep:
        found_commit_header = True
        break
      commit_message.append(line)

    yield _LogEntry(sha, '\n'.join(commit_message).rstrip())


def _shas_between(git_dir: str, base_ref: str,
                  head_ref: str) -> t.Iterable[str]:
  rev_list = [
      'git',
      '-C',
      git_dir,
      'rev-list',
      '--first-parent',
      '%s..%s' % (base_ref, head_ref),
  ]
  return (x.strip() for x in _stream_stdout(rev_list))


def _rev_parse(git_dir: str, ref: str) -> str:
  result = subprocess.check_output(
      ['git', '-C', git_dir, 'rev-parse', ref],
      encoding='utf-8',
  ).strip()
  return t.cast(str, result)


Revert = t.NamedTuple('Revert', [
    ('sha', str),
    ('reverted_sha', str),
])


def find_common_parent_commit(git_dir: str, ref_a: str, ref_b: str) -> str:
  return subprocess.check_output(
      ['git', '-C', git_dir, 'merge-base', ref_a, ref_b],
      encoding='utf-8',
  ).strip()


def find_reverts(git_dir: str, across_ref: str, root: str) -> t.List[Revert]:
  """Finds reverts across `across_ref` in `git_dir`, starting from `root`."""
  across_sha = _rev_parse(git_dir, across_ref)
  root_sha = _rev_parse(git_dir, root)

  common_ancestor = find_common_parent_commit(git_dir, across_sha, root_sha)
  if common_ancestor != across_sha:
    raise ValueError("%s isn't an ancestor of %s (common ancestor: %s)" %
                     (across_sha, root_sha, common_ancestor))

  intermediate_commits = set(_shas_between(git_dir, across_sha, root_sha))
  assert across_ref not in intermediate_commits

  logging.debug('%d commits appear between %s and %s',
                len(intermediate_commits), across_sha, root_sha)

  all_reverts = []
  for sha, commit_message in _log_stream(git_dir, root_sha, across_sha):
    reverts = _try_parse_reverts_from_commit_message(commit_message)
    if not reverts:
      continue

    resolved_reverts = sorted(set(_resolve_sha(git_dir, x) for x in reverts))
    for reverted_sha in resolved_reverts:
      if reverted_sha in intermediate_commits:
        logging.debug('Commit %s reverts %s, which happened after %s', sha,
                      reverted_sha, across_sha)
        continue

      try:
        object_type = subprocess.check_output(
            ['git', '-C', git_dir, 'cat-file', '-t', reverted_sha],
            encoding='utf-8',
            stderr=subprocess.DEVNULL,
        ).strip()
      except subprocess.CalledProcessError:
        logging.warning(
            'Failed to resolve reverted object %s (claimed to be reverted '
            'by sha %s)', reverted_sha, sha)
        continue

      if object_type == 'commit':
        all_reverts.append(Revert(sha, reverted_sha))
        continue

      logging.error("%s claims to revert %s -- which isn't a commit -- %s", sha,
                    object_type, reverted_sha)

  return all_reverts


def main(args: t.List[str]) -> int:
  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument(
      'base_ref', help='Git ref or sha to check for reverts around.')
  parser.add_argument(
      '-C', '--git_dir', default='.', help='Git directory to use.')
  parser.add_argument(
      'root', nargs='+', help='Root(s) to search for commits from.')
  parser.add_argument('--debug', action='store_true')
  opts = parser.parse_args(args)

  logging.basicConfig(
      format='%(asctime)s: %(levelname)s: %(filename)s:%(lineno)d: %(message)s',
      level=logging.DEBUG if opts.debug else logging.INFO,
  )

  # `root`s can have related history, so we want to filter duplicate commits
  # out. The overwhelmingly common case is also to have one root, and it's way
  # easier to reason about output that comes in an order that's meaningful to
  # git.
  all_reverts = collections.OrderedDict()
  for root in opts.root:
    for revert in find_reverts(opts.git_dir, opts.base_ref, root):
      all_reverts[revert] = None

  for revert in all_reverts.keys():
    print('%s claims to revert %s' % (revert.sha, revert.reverted_sha))


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))

# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Helper functions to communicate with Git."""

import datetime
import re
import subprocess
from pathlib import Path
from typing import Dict, List, Tuple

import hashtags
import reviewers

def _run(cmd: List[str], cwd: Path) -> str:
    """Runs a command and returns its output."""
    return subprocess.check_output(cmd, text=True, cwd=cwd)


def fetch(proj_path: Path, remote_names: List[str]) -> None:
    """Runs git fetch.

    Args:
        proj_path: Path to Git repository.
        remote_names: Array of string to specify remote names.
    """
    _run(['git', 'fetch', '--tags', '--multiple'] + remote_names, cwd=proj_path)


def add_remote(proj_path: Path, name: str, url: str) -> None:
    """Adds a git remote.

    Args:
        proj_path: Path to Git repository.
        name: Name of the new remote.
        url: Url of the new remote.
    """
    _run(['git', 'remote', 'add', name, url], cwd=proj_path)


def remove_remote(proj_path: Path, name: str) -> None:
    """Removes a git remote."""
    _run(['git', 'remote', 'remove', name], cwd=proj_path)


def list_remotes(proj_path: Path) -> Dict[str, str]:
    """Lists all Git remotes.

    Args:
        proj_path: Path to Git repository.

    Returns:
        A dict from remote name to remote url.
    """
    def parse_remote(line: str) -> Tuple[str, str]:
        split = line.split()
        return (split[0], split[1])

    out = _run(['git', 'remote', '-v'], proj_path)
    lines = out.splitlines()
    return dict([parse_remote(line) for line in lines])


def get_sha_for_branch(proj_path: Path, branch: str):
    """Gets the hash SHA for a branch."""
    return _run(['git', 'rev-parse', branch], proj_path).strip()


def get_commits_ahead(proj_path: Path, branch: str,
                      base_branch: str) -> List[str]:
    """Lists commits in `branch` but not `base_branch`."""
    out = _run([
        'git', 'rev-list', '--left-only', '--ancestry-path', '{}...{}'.format(
            branch, base_branch)
    ], proj_path)
    return out.splitlines()


# pylint: disable=redefined-outer-name
def get_commit_time(proj_path: Path, commit: str) -> datetime.datetime:
    """Gets commit time of one commit."""
    out = _run(['git', 'show', '-s', '--format=%ct', commit], cwd=proj_path)
    return datetime.datetime.fromtimestamp(int(out.strip()))


def list_remote_branches(proj_path: Path, remote_name: str) -> List[str]:
    """Lists all branches for a remote."""
    lines = _run(['git', 'branch', '-r'], cwd=proj_path).splitlines()
    stripped = [line.strip() for line in lines]
    remote_path = remote_name + '/'
    return [
        line[len(remote_path):] for line in stripped
        if line.startswith(remote_path)
    ]


def list_remote_tags(proj_path: Path, remote_name: str) -> List[str]:
    """Lists all tags for a remote."""
    regex = re.compile(r".*refs/tags/(?P<tag>[^\^]*).*")
    def parse_remote_tag(line: str) -> str:
        return regex.match(line).group("tag")

    lines = _run(['git', "ls-remote", "--tags", remote_name],
                 cwd=proj_path).splitlines()
    tags = [parse_remote_tag(line) for line in lines]
    return list(set(tags))


def get_default_branch(proj_path: Path, remote_name: str) -> str:
    """Gets the name of the upstream branch to use."""
    branches_to_try = ['master', 'main']
    remote_branches = list_remote_branches(proj_path, remote_name)
    for branch in branches_to_try:
        if branch in remote_branches:
            return branch
    # We couldn't find any of the branches we expected.
    # Default to 'master', although nothing will work well.
    return 'master'


COMMIT_PATTERN = r'^[a-f0-9]{40}$'
COMMIT_RE = re.compile(COMMIT_PATTERN)


# pylint: disable=redefined-outer-name
def is_commit(commit: str) -> bool:
    """Whether a string looks like a SHA1 hash."""
    return bool(COMMIT_RE.match(commit))


def merge(proj_path: Path, branch: str) -> None:
    """Merges a branch."""
    try:
        _run(['git', 'merge', branch, '--no-commit'], cwd=proj_path)
    except subprocess.CalledProcessError:
        # Merge failed. Error is already written to console.
        _run(['git', 'merge', '--abort'], cwd=proj_path)
        raise


def add_file(proj_path: Path, file_name: str) -> None:
    """Stages a file."""
    _run(['git', 'add', file_name], cwd=proj_path)


def delete_branch(proj_path: Path, branch_name: str) -> None:
    """Force delete a branch."""
    _run(['git', 'branch', '-D', branch_name], cwd=proj_path)


def start_branch(proj_path: Path, branch_name: str) -> None:
    """Starts a new repo branch."""
    _run(['repo', 'start', branch_name], cwd=proj_path)


def commit(proj_path: Path, message: str) -> None:
    """Commits changes."""
    _run(['git', 'commit', '-m', message], cwd=proj_path)


def checkout(proj_path: Path, branch_name: str) -> None:
    """Checkouts a branch."""
    _run(['git', 'checkout', branch_name], cwd=proj_path)


def push(proj_path: Path, remote_name: str, has_errors: bool) -> None:
    """Pushes change to remote."""
    cmd = ['git', 'push', remote_name, 'HEAD:refs/for/master']
    if revs := reviewers.find_reviewers(str(proj_path)):
        cmd.extend(['-o', revs])
    if tag := hashtags.find_hashtag(proj_path):
        cmd.extend(['-o', 't=' + tag])
    if has_errors:
        cmd.extend(['-o', 'l=Verified-1'])
    _run(cmd, cwd=proj_path)

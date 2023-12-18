#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""A commandline tool to check and update packages in external/

Example usage:
updater.sh checkall
updater.sh update kotlinc
updater.sh update --refresh --keep_date rust/crates/libc
"""

import argparse
import enum
import json
import os
import sys
import subprocess
import time
from typing import Dict, Iterator, List, Union, Tuple, Type
from pathlib import Path

from base_updater import Updater
from crates_updater import CratesUpdater
from git_updater import GitUpdater
from github_archive_updater import GithubArchiveUpdater
import fileutils
import git_utils
# pylint: disable=import-error
import metadata_pb2  # type: ignore
import updater_utils

UPDATERS: List[Type[Updater]] = [
    CratesUpdater,
    GithubArchiveUpdater,
    GitUpdater,
]

TMP_BRANCH_NAME = 'tmp_auto_upgrade'
USE_COLOR = sys.stdout.isatty()


@enum.unique
class Color(enum.Enum):
    """Colors for output to console."""
    FRESH = '\x1b[32m'
    STALE = '\x1b[31;1m'
    ERROR = '\x1b[31m'


END_COLOR = '\033[0m'


def color_string(string: str, color: Color) -> str:
    """Changes the color of a string when print to terminal."""
    if not USE_COLOR:
        return string
    return color.value + string + END_COLOR


def build_updater(proj_path: Path) -> Tuple[Updater, metadata_pb2.MetaData]:
    """Build updater for a project specified by proj_path.

    Reads and parses METADATA file. And builds updater based on the information.

    Args:
      proj_path: Absolute or relative path to the project.

    Returns:
      The updater object built. None if there's any error.
    """

    proj_path = fileutils.get_absolute_project_path(proj_path)
    metadata = fileutils.read_metadata(proj_path)
    updater = updater_utils.create_updater(metadata, proj_path, UPDATERS)
    return (updater, metadata)


def _do_update(args: argparse.Namespace, updater: Updater,
               metadata: metadata_pb2.MetaData) -> None:
    full_path = updater.project_path

    if args.branch_and_commit:
        git_utils.checkout(full_path, args.remote_name + '/master')
        git_utils.start_branch(full_path, TMP_BRANCH_NAME)

    updater.update()

    updated_metadata = metadata_pb2.MetaData()
    updated_metadata.CopyFrom(metadata)
    updated_metadata.third_party.version = updater.latest_version
    for metadata_url in updated_metadata.third_party.url:
        if metadata_url == updater.current_url:
            metadata_url.CopyFrom(updater.latest_url)
    # For Rust crates, replace GIT url with ARCHIVE url
    if isinstance(updater, CratesUpdater):
        updater.update_metadata(updated_metadata, full_path)
    fileutils.write_metadata(full_path, updated_metadata, args.keep_date)
    git_utils.add_file(full_path, 'METADATA')

    if args.branch_and_commit:
        msg = 'Upgrade {} to {}\n\nTest: make\n'.format(
            args.path, updater.latest_version)
        git_utils.add_file(full_path, '*')
        git_utils.commit(full_path, msg)

    if args.push_change:
        git_utils.push(full_path, args.remote_name, updater.has_errors)

    if args.branch_and_commit:
        git_utils.checkout(full_path, args.remote_name + '/master')


def check_and_update(args: argparse.Namespace,
                     proj_path: Path,
                     update_lib=False) -> Union[Updater, str]:
    """Checks updates for a project. Prints result on console.

    Args:
      args: commandline arguments
      proj_path: Absolute or relative path to the project.
      update: If false, will only check for new version, but not update.
    """

    try:
        rel_proj_path = fileutils.get_relative_project_path(proj_path)
        print(f'Checking {rel_proj_path}. ', end='')
        updater, metadata = build_updater(proj_path)
        updater.check()

        current_ver = updater.current_version
        latest_ver = updater.latest_version
        print('Current version: {}. Latest version: {}'.format(
            current_ver, latest_ver),
              end='')

        has_new_version = current_ver != latest_ver
        if has_new_version:
            print(color_string(' Out of date!', Color.STALE))
        else:
            print(color_string(' Up to date.', Color.FRESH))

        if update_lib and args.refresh:
            print('Refreshing the current version')
            updater.use_current_as_latest()
        if update_lib and (has_new_version or args.force or args.refresh):
            _do_update(args, updater, metadata)
        return updater
    # pylint: disable=broad-except
    except Exception as err:
        print('{} {}.'.format(color_string('Failed.', Color.ERROR), err))
        return str(err)


def _check_path(args: argparse.Namespace, paths: Iterator[str],
                delay: int) -> Dict[str, Dict[str, str]]:
    results = {}
    for path in paths:
        res = {}
        updater = check_and_update(args, Path(path))
        if isinstance(updater, str):
            res['error'] = updater
        else:
            res['current'] = updater.current_version
            res['latest'] = updater.latest_version
        relative_path = fileutils.get_relative_project_path(Path(path))
        results[str(relative_path)] = res
        time.sleep(delay)
    return results


def _list_all_metadata() -> Iterator[str]:
    for path, dirs, files in os.walk(fileutils.EXTERNAL_PATH):
        if fileutils.METADATA_FILENAME in files:
            # Skip sub directories.
            dirs[:] = []
            yield path
        dirs.sort(key=lambda d: d.lower())


def check(args: argparse.Namespace) -> None:
    """Handler for check command."""
    paths = _list_all_metadata() if args.all else args.paths
    results = _check_path(args, paths, args.delay)

    if args.json_output is not None:
        with Path(args.json_output).open('w') as res_file:
            json.dump(results, res_file, sort_keys=True, indent=4)


def update(args: argparse.Namespace) -> None:
    """Handler for update command."""
    check_and_update(args, args.path, update_lib=True)


def parse_args() -> argparse.Namespace:
    """Parses commandline arguments."""

    parser = argparse.ArgumentParser(
        description='Check updates for third party projects in external/.')
    subparsers = parser.add_subparsers(dest='cmd')
    subparsers.required = True

    # Creates parser for check command.
    check_parser = subparsers.add_parser('check',
                                         help='Check update for one project.')
    check_parser.add_argument(
        'paths',
        nargs='*',
        help='Paths of the project. '
        'Relative paths will be resolved from external/.')
    check_parser.add_argument('--json_output',
                              help='Path of a json file to write result to.')
    check_parser.add_argument(
        '--all',
        action='store_true',
        help='If set, check updates for all supported projects.')
    check_parser.add_argument(
        '--delay',
        default=0,
        type=int,
        help='Time in seconds to wait between checking two projects.')
    check_parser.set_defaults(func=check)

    # Creates parser for update command.
    update_parser = subparsers.add_parser('update', help='Update one project.')
    update_parser.add_argument(
        'path',
        help='Path of the project. '
        'Relative paths will be resolved from external/.')
    update_parser.add_argument(
        '--force',
        help='Run update even if there\'s no new version.',
        action='store_true')
    update_parser.add_argument(
        '--refresh',
        help='Run update and refresh to the current version.',
        action='store_true')
    update_parser.add_argument(
        '--keep_date',
        help='Run update and do not change date in METADATA.',
        action='store_true')
    update_parser.add_argument('--branch_and_commit',
                               action='store_true',
                               help='Starts a new branch and commit changes.')
    update_parser.add_argument('--push_change',
                               action='store_true',
                               help='Pushes change to Gerrit.')
    update_parser.add_argument('--remote_name',
                               default='aosp',
                               required=False,
                               help='Upstream remote name.')
    update_parser.set_defaults(func=update)

    return parser.parse_args()


def main() -> None:
    """The main entry."""

    args = parse_args()
    args.func(args)


if __name__ == '__main__':
    main()

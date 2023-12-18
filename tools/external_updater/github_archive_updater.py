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
"""Module to update packages from GitHub archive."""

import json
import re
import time
import urllib.request
import urllib.error
from typing import List, Optional, Tuple

import archive_utils
from base_updater import Updater
import git_utils
# pylint: disable=import-error
import metadata_pb2  # type: ignore
import updater_utils

GITHUB_URL_PATTERN: str = (r'^https:\/\/github.com\/([-\w]+)\/([-\w]+)\/' +
                           r'(releases\/download\/|archive\/)')
GITHUB_URL_RE: re.Pattern = re.compile(GITHUB_URL_PATTERN)


def _edit_distance(str1: str, str2: str) -> int:
    prev = list(range(0, len(str2) + 1))
    for i, chr1 in enumerate(str1):
        cur = [i + 1]
        for j, chr2 in enumerate(str2):
            if chr1 == chr2:
                cur.append(prev[j])
            else:
                cur.append(min(prev[j + 1], prev[j], cur[j]) + 1)
        prev = cur
    return prev[len(str2)]


def choose_best_url(urls: List[str], previous_url: str) -> str:
    """Returns the best url to download from a list of candidate urls.

    This function calculates similarity between previous url and each of new
    urls. And returns the one best matches previous url.

    Similarity is measured by editing distance.

    Args:
        urls: Array of candidate urls.
        previous_url: String of the url used previously.

    Returns:
        One url from `urls`.
    """
    return min(urls,
               default="",
               key=lambda url: _edit_distance(url, previous_url))


class GithubArchiveUpdater(Updater):
    """Updater for archives from GitHub.

    This updater supports release archives in GitHub. Version is determined by
    release name in GitHub.
    """

    VERSION_FIELD: str = 'tag_name'
    owner: str
    repo: str

    def is_supported_url(self) -> bool:
        if self._old_url.type != metadata_pb2.URL.ARCHIVE:
            return False
        match = GITHUB_URL_RE.match(self._old_url.value)
        if match is None:
            return False
        try:
            self.owner, self.repo = match.group(1, 2)
        except IndexError:
            return False
        return True

    def _fetch_latest_release(self) -> Optional[Tuple[str, List[str]]]:
        # pylint: disable=line-too-long
        url = f'https://api.github.com/repos/{self.owner}/{self.repo}/releases/latest'
        try:
            with urllib.request.urlopen(url) as request:
                data = json.loads(request.read().decode())
        except urllib.error.HTTPError as err:
            if err.code == 404:
                return None
            raise
        supported_assets = [
            a['browser_download_url'] for a in data['assets']
            if archive_utils.is_supported_archive(a['browser_download_url'])
        ]
        return (data[self.VERSION_FIELD], supported_assets)

    def _fetch_latest_tag(self) -> Tuple[str, List[str]]:
        page = 1
        tags: List[str] = []
        # fetches at most 20 pages.
        for page in range(1, 21):
            # Sleeps 10s to avoid rate limit.
            time.sleep(10)
            # pylint: disable=line-too-long
            url = f'https://api.github.com/repos/{self.owner}/{self.repo}/tags?page={page}'
            with urllib.request.urlopen(url) as request:
                data = json.loads(request.read().decode())
            if len(data) == 0:
                break
            tags.extend(d['name'] for d in data)
        return (updater_utils.get_latest_version(self._old_ver, tags), [])

    def _fetch_latest_version(self) -> None:
        """Checks upstream and gets the latest release tag."""
        self._new_ver, urls = (self._fetch_latest_release()
                               or self._fetch_latest_tag())

        # Adds source code urls.
        urls.append('https://github.com/{}/{}/archive/{}.tar.gz'.format(
            self.owner, self.repo, self._new_ver))
        urls.append('https://github.com/{}/{}/archive/{}.zip'.format(
            self.owner, self.repo, self._new_ver))

        self._new_url.value = choose_best_url(urls, self._old_url.value)

    def _fetch_latest_commit(self) -> None:
        """Checks upstream and gets the latest commit to master."""

        # pylint: disable=line-too-long
        url = f'https://api.github.com/repos/{self.owner}/{self.repo}/commits/master'
        with urllib.request.urlopen(url) as request:
            data = json.loads(request.read().decode())
        self._new_ver = data['sha']
        self._new_url.value = (
            # pylint: disable=line-too-long
            f'https://github.com/{self.owner}/{self.repo}/archive/{self._new_ver}.zip'
        )

    def check(self) -> None:
        """Checks update for package.

        Returns True if a new version is available.
        """
        if git_utils.is_commit(self._old_ver):
            self._fetch_latest_commit()
        else:
            self._fetch_latest_version()

    def update(self) -> None:
        """Updates the package.

        Has to call check() before this function.
        """
        temporary_dir = None
        try:
            temporary_dir = archive_utils.download_and_extract(
                self._new_url.value)
            package_dir = archive_utils.find_archive_root(temporary_dir)
            updater_utils.replace_package(package_dir, self._proj_path)
        finally:
            # Don't remove the temporary directory, or it'll be impossible
            # to debug the failure...
            # shutil.rmtree(temporary_dir, ignore_errors=True)
            urllib.request.urlcleanup()

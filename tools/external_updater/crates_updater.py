# Copyright (C) 2020 The Android Open Source Project
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
"""Module to check updates from crates.io."""

import json
import os
# pylint: disable=g-importing-member
from pathlib import Path
import re
import urllib.request

import archive_utils
from base_updater import Updater
# pylint: disable=import-error
import metadata_pb2  # type: ignore
import updater_utils

CRATES_IO_URL_PATTERN: str = (r"^https:\/\/crates.io\/crates\/([-\w]+)")

CRATES_IO_URL_RE: re.Pattern = re.compile(CRATES_IO_URL_PATTERN)

ALPHA_BETA_PATTERN: str = (r"^.*[0-9]+\.[0-9]+\.[0-9]+-(alpha|beta).*")

ALPHA_BETA_RE: re.Pattern = re.compile(ALPHA_BETA_PATTERN)

VERSION_PATTERN: str = (r"([0-9]+)\.([0-9]+)\.([0-9]+)")

VERSION_MATCHER: re.Pattern = re.compile(VERSION_PATTERN)

DESCRIPTION_PATTERN: str = (r"^description *= *(\".+\")")

DESCRIPTION_MATCHER: re.Pattern = re.compile(DESCRIPTION_PATTERN)


class CratesUpdater(Updater):
    """Updater for crates.io packages."""

    download_url: str
    package: str

    def is_supported_url(self) -> bool:
        if self._old_url.type != metadata_pb2.URL.HOMEPAGE:
            return False
        match = CRATES_IO_URL_RE.match(self._old_url.value)
        if match is None:
            return False
        self.package = match.group(1)
        return True

    def _get_version_numbers(self, version: str) -> (int, int, int):
        match = VERSION_MATCHER.match(version)
        if match is not None:
            return tuple(int(match.group(i)) for i in range(1, 4))
        return (0, 0, 0)

    def _is_newer_version(self, prev_version: str, prev_id: int,
                          check_version: str, check_id: int):
        """Return true if check_version+id is newer than prev_version+id."""
        return ((self._get_version_numbers(check_version), check_id) >
                (self._get_version_numbers(prev_version), prev_id))

    def _find_latest_non_test_version(self) -> None:
        url = "https://crates.io/api/v1/crates/{}/versions".format(self.package)
        with urllib.request.urlopen(url) as request:
            data = json.loads(request.read().decode())
        last_id = 0
        self._new_ver = ""
        for v in data["versions"]:
            version = v["num"]
            if (not v["yanked"] and not ALPHA_BETA_RE.match(version) and
                self._is_newer_version(
                    self._new_ver, last_id, version, int(v["id"]))):
                last_id = int(v["id"])
                self._new_ver = version
                self.download_url = "https://crates.io" + v["dl_path"]

    def check(self) -> None:
        """Checks crates.io and returns whether a new version is available."""
        url = "https://crates.io/api/v1/crates/" + self.package
        with urllib.request.urlopen(url) as request:
            data = json.loads(request.read().decode())
            self._new_ver = data["crate"]["max_version"]
        # Skip d.d.d-{alpha,beta}* versions
        if ALPHA_BETA_RE.match(self._new_ver):
            print("Ignore alpha or beta release: {}-{}."
                  .format(self.package, self._new_ver))
            self._find_latest_non_test_version()
        else:
            url = url + "/" + self._new_ver
            with urllib.request.urlopen(url) as request:
                data = json.loads(request.read().decode())
                self.download_url = "https://crates.io" + data["version"]["dl_path"]

    def use_current_as_latest(self):
        Updater.use_current_as_latest(self)
        # A shortcut to use the static download path.
        self.download_url = "https://static.crates.io/crates/{}/{}-{}.crate".format(
            self.package, self.package, self._new_ver)

    def update(self) -> None:
        """Updates the package.

        Has to call check() before this function.
        """
        try:
            temporary_dir = archive_utils.download_and_extract(self.download_url)
            package_dir = archive_utils.find_archive_root(temporary_dir)
            updater_utils.replace_package(package_dir, self._proj_path)
            self.check_for_errors()
        finally:
            urllib.request.urlcleanup()

    # pylint: disable=no-self-use
    def update_metadata(self, metadata: metadata_pb2.MetaData,
                        full_path: Path) -> None:
        """Updates METADATA content."""
        # copy only HOMEPAGE url, and then add new ARCHIVE url.
        new_url_list = []
        for url in metadata.third_party.url:
            if url.type == metadata_pb2.URL.HOMEPAGE:
                new_url_list.append(url)
        new_url = metadata_pb2.URL()
        new_url.type = metadata_pb2.URL.ARCHIVE
        new_url.value = "https://static.crates.io/crates/{}/{}-{}.crate".format(
            metadata.name, metadata.name, metadata.third_party.version)
        new_url_list.append(new_url)
        del metadata.third_party.url[:]
        metadata.third_party.url.extend(new_url_list)
        # copy description from Cargo.toml to METADATA
        cargo_toml = os.path.join(full_path, "Cargo.toml")
        description = self._get_cargo_description(cargo_toml)
        if description and description != metadata.description:
            print("New METADATA description:", description)
            metadata.description = description

    def check_for_errors(self) -> None:
        # Check for .rej patches from failing to apply patches.
        # If this has too many false positives, we could either
        # check if the files are modified by patches or somehow
        # track which files existed before the patching.
        rejects = list(self._proj_path.glob('**/*.rej'))
        if len(rejects) > 0:
            print("Error: Found patch reject files: %s" % str(rejects))
            self._has_errors = True
        # Check for Cargo errors embedded in Android.bp.
        # Note that this should stay in sync with cargo2android.py.
        with open('%s/Android.bp' % self._proj_path, 'r') as bp_file:
            for line in bp_file:
                if line.strip() == "Errors in cargo.out:":
                    print("Error: Found Cargo errors in Android.bp")
                    self._has_errors = True
                    return

    def _toml2str(self, line: str) -> str:
        """Convert a quoted toml string to a Python str without quotes."""
        if line.startswith("\"\"\""):
            return ""  # cannot handle broken multi-line description
        # TOML string escapes: \b \t \n \f \r \" \\ (no unicode escape)
        line = line[1:-1].replace("\\\\", "\n").replace("\\b", "")
        line = line.replace("\\t", " ").replace("\\n", " ").replace("\\f", " ")
        line = line.replace("\\r", "").replace("\\\"", "\"").replace("\n", "\\")
        # replace a unicode quotation mark, used in the libloading crate
        return line.replace("’", "'").strip()

    def _get_cargo_description(self, cargo_toml: str) -> str:
        """Return the description in Cargo.toml or empty string."""
        if os.path.isfile(cargo_toml) and os.access(cargo_toml, os.R_OK):
            with open(cargo_toml, "r") as toml_file:
                for line in toml_file:
                    match = DESCRIPTION_MATCHER.match(line)
                    if match:
                        return self._toml2str(match.group(1))
        return ""

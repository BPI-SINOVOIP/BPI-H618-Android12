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
"""Base class for all updaters."""

from pathlib import Path

import fileutils
# pylint: disable=import-error
import metadata_pb2  # type: ignore


class Updater:
    """Base Updater that defines methods common for all updaters."""
    def __init__(self, proj_path: Path, old_url: metadata_pb2.URL,
                 old_ver: str) -> None:
        self._proj_path = fileutils.get_absolute_project_path(proj_path)
        self._old_url = old_url
        self._old_ver = old_ver

        self._new_url = metadata_pb2.URL()
        self._new_url.CopyFrom(old_url)
        self._new_ver = old_ver

        self._has_errors = False

    def is_supported_url(self) -> bool:
        """Returns whether the url is supported."""
        raise NotImplementedError()

    def check(self) -> None:
        """Checks whether a new version is available."""
        raise NotImplementedError()

    def update(self) -> None:
        """Updates the package.

        Has to call check() before this function.
        """
        raise NotImplementedError()

    @property
    def project_path(self) -> Path:
        """Gets absolute path to the project."""
        return self._proj_path

    @property
    def current_version(self) -> str:
        """Gets the current version."""
        return self._old_ver

    @property
    def current_url(self) -> metadata_pb2.URL:
        """Gets the current url."""
        return self._old_url

    @property
    def latest_version(self) -> str:
        """Gets latest version."""
        return self._new_ver

    @property
    def latest_url(self) -> metadata_pb2.URL:
        """Gets URL for latest version."""
        return self._new_url

    @property
    def has_errors(self) -> bool:
        """Gets whether this update had an error."""
        return self._has_errors

    def use_current_as_latest(self):
        """Uses current version/url as the latest to refresh project."""
        self._new_ver = self._old_ver
        self._new_url = self._old_url

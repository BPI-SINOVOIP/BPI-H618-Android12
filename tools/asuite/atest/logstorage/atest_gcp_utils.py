#  Copyright (C) 2020 The Android Open Source Project
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
Utility functions for atest.
"""
from __future__ import print_function

import os
import logging
try:
    import httplib2
except ModuleNotFoundError as e:
    logging.debug('Import error due to %s', e)
import constants

try:
    # pylint: disable=import-error
    from oauth2client import client as oauth2_client
    from oauth2client.contrib import multistore_file
    from oauth2client import tools as oauth2_tools
except ModuleNotFoundError as e:
    logging.debug('Import error due to %s', e)


class RunFlowFlags():
    """Flags for oauth2client.tools.run_flow."""
    def __init__(self, browser_auth):
        self.auth_host_port = [8080, 8090]
        self.auth_host_name = "localhost"
        self.logging_level = "ERROR"
        self.noauth_local_webserver = not browser_auth


class GCPHelper():
    """GCP bucket helper class."""
    def __init__(self, client_id=None, client_secret=None,
                 user_agent=None, scope=constants.SCOPE_BUILD_API_SCOPE):
        """Init stuff for GCPHelper class.
        Args:
            client_id: String, client id from the cloud project.
            client_secret: String, client secret for the client_id.
            user_agent: The user agent for the credential.
            scope: String, scopes separated by space.
        """
        self.client_id = client_id
        self.client_secret = client_secret
        self.user_agent = user_agent
        self.scope = scope

    def get_refreshed_credential_from_file(self, creds_file_path):
        """Get refreshed credential from file.
        Args:
            creds_file_path: Credential file path.
        Returns:
            An oauth2client.OAuth2Credentials instance.
        """
        credential = self.get_credential_from_file(creds_file_path)
        if credential:
            try:
                credential.refresh(httplib2.Http())
            except oauth2_client.AccessTokenRefreshError as e:
                logging.debug('Token refresh error: %s', e)
            if not credential.invalid:
                return credential
        logging.debug('Cannot get credential.')
        return None

    def get_credential_from_file(self, creds_file_path):
        """Get credential from file.
        Args:
            creds_file_path: Credential file path.
        Returns:
            An oauth2client.OAuth2Credentials instance.
        """
        storage = multistore_file.get_credential_storage(
            filename=os.path.abspath(creds_file_path),
            client_id=self.client_id,
            user_agent=self.user_agent,
            scope=self.scope)
        return storage.get()

    def get_credential_with_auth_flow(self, creds_file_path):
        """Get Credential object from file.
        Get credential object from file. Run oauth flow if haven't authorized
        before.

        Args:
            creds_file_path: Credential file path.
        Returns:
            An oauth2client.OAuth2Credentials instance.
        """
        credentials = self.get_refreshed_credential_from_file(creds_file_path)
        if not credentials:
            storage = multistore_file.get_credential_storage(
                filename=os.path.abspath(creds_file_path),
                client_id=self.client_id,
                user_agent=self.user_agent,
                scope=self.scope)
            return self._run_auth_flow(storage)
        return credentials

    def _run_auth_flow(self, storage):
        """Get user oauth2 credentials.

        Args:
            storage: GCP storage object.
        Returns:
            An oauth2client.OAuth2Credentials instance.
        """
        flags = RunFlowFlags(browser_auth=False)
        flow = oauth2_client.OAuth2WebServerFlow(
            client_id=self.client_id,
            client_secret=self.client_secret,
            scope=self.scope,
            user_agent=self.user_agent)
        credentials = oauth2_tools.run_flow(
            flow=flow, storage=storage, flags=flags)
        return credentials

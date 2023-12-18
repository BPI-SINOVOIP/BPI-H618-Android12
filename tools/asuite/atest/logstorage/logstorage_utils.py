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


""" Utility functions for logstorage. """
from __future__ import print_function

import logging
import constants

# pylint: disable=import-error
try:
    import httplib2
    from googleapiclient.discovery import build
except ImportError as e:
    logging.debug('Import error due to: %s', e)


class BuildClient:
    """Build api helper class."""

    def __init__(self, creds):
        """Init BuildClient class.
        Args:
            creds: An oauth2client.OAuth2Credentials instance.
        """
        http_auth = creds.authorize(httplib2.Http())
        self.client = build(
            serviceName=constants.STORAGE_SERVICE_NAME,
            version=constants.STORAGE_API_VERSION,
            cache_discovery=False,
            http=http_auth)

    def list_branch(self):
        """List all branch."""
        return self.client.branch().list(maxResults=10000).execute()

    def list_target(self, branch):
        """List all target in the branch."""
        return self.client.target().list(branch=branch,
                                         maxResults=10000).execute()

    def insert_local_build(self, external_id, target, branch):
        """Insert a build record.
        Args:
            external_id: unique id of build record.
            target: build target.
            branch: build branch.

        Returns:
            An build record object.
        """
        body = {
            "buildId": "",
            "externalId": external_id,
            "branch": branch,
            "target": {
                "name": target,
                "target": target
            },
            "buildAttemptStatus": "complete",
        }
        return self.client.build().insert(buildType="local",
                                          body=body).execute()

    def insert_build_attempts(self, build_record):
        """Insert a build attempt record.
        Args:
            build_record: build record.

        Returns:
            An build attempt object.
        """
        build_attempt = {
            "id": 0,
            "status": "complete",
            "successful": True
        }
        return self.client.buildattempt().insert(
            buildId=build_record['buildId'],
            target=build_record['target']['name'],
            body=build_attempt).execute()

    def insert_invocation(self, build_record):
        """Insert a build invocation record.
        Args:
            build_record: build record.

        Returns:
            A build invocation object.
        """
        invocation = {
            "primaryBuild": {
                "buildId": build_record['buildId'],
                "buildTarget": build_record['target']['name'],
                "branch": build_record['branch'],
            },
            "schedulerState": "running"
        }
        return self.client.invocation().insert(body=invocation).execute()

    def update_invocation(self, invocation):
        """Insert a build invocation record.
        Args:
            invocation: invocation record.

        Returns:
            A invocation object.
        """
        return self.client.invocation().update(
            resourceId=invocation['invocationId'],
            body=invocation).execute()

    def insert_work_unit(self, invocation_record):
        """Insert a workunit record.
          Args:
              invocation_record: invocation record.

          Returns:
              the workunit object.
          """
        workunit = {
            'invocationId': invocation_record['invocationId']
        }
        return self.client.workunit().insert(body=workunit).execute()

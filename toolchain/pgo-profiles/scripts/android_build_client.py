#!/usr/bin/env python3
#
# Copyright (C) 2019 The Android Open Source Project
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
#

try:
    import apiclient.discovery
    import apiclient.http
    from oauth2client import client as oauth2_client
except ImportError:
    missingImportString = """
  Missing necessary libraries. Try doing the following:
  $ sudo apt-get install python3-pip
  $ pip install --user --upgrade google-api-python-client
  $ pip install --user --upgrade oauth2client
"""
    raise ImportError(missingImportString)

import io
import getpass
import os

import utils

ANDROID_BUILD_API_SCOPE = (
    'https://www.googleapis.com/auth/androidbuild.internal')
ANDROID_BUILD_API_NAME = 'androidbuildinternal'
ANDROID_BUILD_API_VERSION = 'v2beta1'
CHUNK_SIZE = 10 * 1024 * 1024  # 10M

ANDROID_PGO_BUILD = 'pgo-coral-config1'

STUBBY_COMMAND_PATH = '/google/data/ro/teams/android-llvm/tests/sso_stubby_cmd.sh'
STUBBY_REQUEST = """
target: {{
  scope: GAIA_USER
  name: "{user}@google.com"
}}
target_credential: {{
  type: OAUTH2_TOKEN
  oauth2_attributes: {{
    scope: '{scope}'
  }}
}}
"""


def _get_oauth2_token():
    request = STUBBY_REQUEST.format(
        user=getpass.getuser(), scope=ANDROID_BUILD_API_SCOPE)
    with open(STUBBY_COMMAND_PATH) as stubby_command_file:
        stubby_command = stubby_command_file.read().strip().split()
    output = utils.check_output(stubby_command, input=request)
    # output is of the format:
    # oauth2_token: "<TOKEN>"
    return output.split('"')[1]


class AndroidBuildClient(object):

    def __init__(self):
        creds = oauth2_client.AccessTokenCredentials(
            access_token=_get_oauth2_token(), user_agent='unused/1.0')

        self.client = apiclient.discovery.build(
            ANDROID_BUILD_API_NAME,
            ANDROID_BUILD_API_VERSION,
            credentials=creds,
            discoveryServiceUrl=apiclient.discovery.DISCOVERY_URI)

    # Get the latest test invocation for a given test_tag for a given build.
    def get_invocation_id(self, build, test_tag):
        request = self.client.testresult().list(
            buildId=build, target=ANDROID_PGO_BUILD, attemptId='latest')

        response = request.execute()
        testResultWithTag = [
            r for r in response['testResults'] if r['testTag'] == test_tag
        ]
        if len(testResultWithTag) != 1:
            raise RuntimeError(
                'Expected one test with tag {} for build {}.  Found {}.  Full response is {}'
                .format(test_tag, build, len(testResultWithTag), response))
        return testResultWithTag[0]['id']

    # Get the full artifact name for the zipped PGO profiles
    # (_data_local_tmp_pgo_<hash>.zip) for a given <build, test_tag,
    # invocation_id>.
    def get_test_artifact_name(self, build, test_tag, invocation_id):
        request = self.client.testartifact().list(
            buildType='submitted',
            buildId=build,
            target=ANDROID_PGO_BUILD,
            attemptId='latest',
            testResultId=invocation_id,
            maxResults=100)

        response = request.execute()
        profile_zip = [
            f for f in response['test_artifacts']
            if f['name'].endswith('zip') and '_data_local_tmp_pgo_' in f['name']
        ]
        if len(profile_zip) != 1:
            raise RuntimeError(
                'Expected one matching zipfile for invocation {} of {} for build {}.  Found {} ({})'
                .format(invocation_id, test_tag, build, len(profile_zip),
                        ', '.join(profile_zip)))
        return profile_zip[0]['name']

    # Download the zipped PGO profiles for a given <build, test_tag,
    # invocation_id, artifact_name> into <output_zip>.
    def download_test_artifact(self, build, invocation_id, artifact_name,
                               output_zip):
        request = self.client.testartifact().get_media(
            buildType='submitted',
            buildId=build,
            target=ANDROID_PGO_BUILD,
            attemptId='latest',
            testResultId=invocation_id,
            resourceId=artifact_name)

        f = io.FileIO(output_zip, 'wb')
        try:
            downloader = apiclient.http.MediaIoBaseDownload(
                f, request, chunksize=CHUNK_SIZE)
            done = False
            while not done:
                status, done = downloader.next_chunk()
        except apiclient.errors.HttpError as e:
            if e.resp.status == 404:
                raise RuntimeError(
                    'Artifact {} does not exist for invocation {} for build {}.'
                    .format(artifact_name, invocation_id, build))

    # For a <build, test_tag>, find the invocation_id, artifact_name and
    # download the artifact into <output_dir>/pgo_profiles.zip.
    def download_pgo_zip(self, build, test_tag, output_dir):
        output_zip = os.path.join(output_dir, 'pgo_profiles.zip')

        invocation_id = self.get_invocation_id(build, test_tag)
        artifact_name = self.get_test_artifact_name(build, test_tag,
                                                    invocation_id)
        self.download_test_artifact(build, invocation_id, artifact_name,
                                    output_zip)
        return output_zip

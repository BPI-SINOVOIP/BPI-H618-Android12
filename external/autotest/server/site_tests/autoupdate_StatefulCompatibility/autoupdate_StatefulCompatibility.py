# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import re

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils as cutils
from autotest_lib.client.common_lib.cros import kernel_utils
from autotest_lib.client.cros import constants
from autotest_lib.server import utils
from autotest_lib.server.cros import provisioner
from autotest_lib.server.cros.update_engine import update_engine_test


class autoupdate_StatefulCompatibility(update_engine_test.UpdateEngineTest):
    """Tests autoupdating to/from kernel-next images."""
    version = 1

    _LOGIN_TEST = 'login_LoginSuccess'


    def cleanup(self):
        """Save the logs from stateful_partition's preserved/log dir."""
        stateful_preserved_logs = os.path.join(self.resultsdir,
                                               '~stateful_preserved_logs')
        os.makedirs(stateful_preserved_logs)
        self._host.get_file(
                constants.AUTOUPDATE_PRESERVE_LOG,
                stateful_preserved_logs,
                safe_symlinks=True,
                preserve_perm=False)
        super(autoupdate_StatefulCompatibility, self).cleanup()


    def _get_target_uri(self, target_board, version_regex, max_image_checks):
        """Checks through all valid builds for the latest green build

        @param target_board: the name of the board to test against
        @param version_regex: the version regex to test against
        @param max_image_checks: the number of images to check for stability

        @return the URI for the most recent passing build to test against

        """
        candidate_uris = self._get_candidate_uris(target_board, version_regex)
        candidate_uris = candidate_uris[:max_image_checks]

        metadata_uri = None
        most_recent_version = None
        most_recent_channel = None

        for uri in candidate_uris:
            uri = self._to_real_path(uri)
            metadata = self._get_metadata_dict(uri)
            chan = self._get_image_channel(metadata)
            version = cutils.parse_gs_uri_version(uri)

            if not self._stateful_payload_exists(chan, target_board, version):
                continue

            # Keep track of the first image found that has an available payload
            if most_recent_version is None:
                most_recent_version = version
                most_recent_channel = chan

            if self._is_build_green(metadata):
                metadata_uri = uri
                break

        if most_recent_version is None:
            raise error.TestError('Could not find an acceptable image for %s.' %
                                  target_board)

        if metadata_uri is None:
            logging.warning('No image met quality criteria. Checked %d images',
                            len(candidate_uris))
            # At this point we've checked as many images as possible up to the
            # specified maximum, and none of them have qualified with our pass/
            # fail criteria. Any image is as good as any other, so we might as
            # well continue with the most recent image. The only other option is
            # to fail this test
            version = most_recent_version
            chan = most_recent_channel

        payload = self._get_payload_uri(chan, target_board, version)
        if payload is not None:
            return payload

        raise error.TestError('Could not find an acceptable payload for %s.' %
                              target_board)


    def _get_candidate_uris(self, target_board, version_regex):
        """Retrieves a list of GS URIs that match the target board and version

        @param target_board: the name of the board to get image URIs for
        @param version_regex: a regex passed to 'gsutil ls' to match GS URIs

        @return: a list of boards that match the target_board and version_regex

        """
        logging.info('Going to find candidate image for %s.', target_board)

        payload_uri = 'gs://chromeos-image-archive/%s-release/%s/' % (
            target_board, version_regex)

        candidate_uris = utils.system_output('gsutil ls -d %s' %
                                             payload_uri).splitlines()
        candidate_uris.sort(cutils.compare_gs_uri_build_versions, reverse=True)
        return candidate_uris


    @staticmethod
    def _to_real_path(uri):
        """Converts a target image URI from the form LATEST-* to R##-*

        Target images can be referenced by matching against LATEST-* rather than
        the actual milestone. The LATEST-* files are actually text files that
        contain the name of the actual GS bucket that contains the image data.

        @param uri: the GS bucket URI of the LATEST-* bucket path

        @return the URI of the dereferenced GS bucket

        """
        latest_pos = uri.find('LATEST')
        if latest_pos < 0:
            # Path is not in the form 'gs://../../LATEST-*'
            return uri

        relative_path = utils.system_output('gsutil cat %s' % uri).strip()
        return uri[:latest_pos] + relative_path


    @staticmethod
    def _stateful_payload_exists(channel, target_board, version):
        """Checks that stateful.tgz exists for the given board and version

        @param channel: The release channel (canary, dev, beta, or stable)
        @param target_board: The name of the target board
        @param version: A string containing the build version ('12345.6.7')

        @return True if stateful.gz exists for this image, otherwise False

        """

        if channel is None:
            return False

        channel_payload_uri = 'gs://chromeos-releases/%s-channel/%s/%s' % (
                channel, target_board, version)
        exists = not utils.system('gsutil -q stat %s/stateful.tgz' %
                                  channel_payload_uri, ignore_status=True)
        return exists


    @staticmethod
    def _get_payload_uri(channel, board, version):
        """Gets the location of the update payload for staging on the dev server

        For a given release channel, board, and release version this will return
        the location for the full signed payload (as opposed to delta payloads).

        @param channel: The release channel (canary, dev, beta, or stable)
        @param board: The name of the target board
        @param version: A string containing the build version ('12345.6.7')

        @return The GS URI for the full payload to be staged on the devserver

        """
        payload_uri = 'gs://chromeos-releases/%s-channel/%s/%s/payloads' % (
            channel, board, version)

        payloads = utils.system_output('gsutil ls -d %s/*%s*full_test*' % (
            payload_uri, version)).splitlines()
        logging.debug('Payloads: %s', str(payloads))

        for payload in payloads:
            if re.match('.*-[a-z|0-9]{32}$', payload) is not None:
                return payload
        return None


    @staticmethod
    def _get_metadata_dict(payload_uri):
        """Fetches the build metadata from the associated GS bucket

        @param payload_uri: the URI for the GS bucket the image is from.

        @return a dictionary of values representing the metadata json values

        """
        metadata_uri = payload_uri.strip('/') + '/metadata.json'
        logging.info('Going to fetch image metadata (%s)', metadata_uri)
        cat_result = utils.run('gsutil cat %s' % metadata_uri,
                               ignore_status=True)

        if cat_result.exit_status != 0:
            logging.info('''Couldn't find metadata at %s.''', metadata_uri)
            return None

        metadata = json.loads(cat_result.stdout)
        return metadata


    @staticmethod
    def _get_image_channel(metadata):
        """Returns the release channel from the image metadata

        @param metadata: A dict of values representing the image metadata

        @return the release channel for the image (canary, dev, beta, stable)

        """

        all_channels = ['Stable', 'Beta', 'Dev', 'Canary']

        if 'tags' not in metadata:
            return None

        # The metadata tags contains the status for paygen stages on all
        # channels paygen was run for. This should tell us what channels the
        # payload is available under.
        # These tags use the form 'stage_status:PaygenBuild<Channel>'
        paygen_tags = [t for t in metadata['tags'] if 'PaygenBuild' in t]

        # Find all the channels paygen was run for on this image
        channels = [c for c in all_channels for t in paygen_tags if c in t]

        if not channels:
            return None

        # The channels list contains some subset of the elements in the
        # all_channels list, presented in the same order. If both the Beta and
        # Stable channels are available, this will return "stable", for example.
        return channels[0].lower()


    @staticmethod
    def _is_build_green(metadata):
        """Inspects the image metadata to see if the build is "green"

        @param metadata A dict of values representing the image metadata

        @return True if the image appears to be good enough to test against.

        """
        if metadata is None:
            return False

        if not ('tags' in metadata and 'status' in metadata['tags']):
            return False

        return metadata['tags']['status'] == 'pass'


    def run_once(self, test_conf, max_image_checks):
        """Main entry point of the test."""
        logging.debug("Using test_conf: %s", test_conf)

        self._source_payload_uri = test_conf['source_payload_uri']
        self._target_payload_uri = test_conf['target_payload_uri']

        if self._target_payload_uri is None:
            target_board = test_conf['target_board']
            target_version_regex = test_conf['target_version_regex']

            self._target_payload_uri = self._get_target_uri(
                target_board, target_version_regex, max_image_checks)

        logging.debug('Using source image %s', self._source_payload_uri)
        logging.debug('Using target image %s', self._target_payload_uri)

        self._autotest_devserver = self._get_devserver_for_test(
            {'target_payload_uri': self._target_payload_uri})

        self._stage_payloads(self._source_payload_uri, None)
        self._stage_payloads(self._target_payload_uri, None)

        if self._source_payload_uri is not None:
            build_name, _ = self._get_update_parameters_from_uri(
                    self._source_payload_uri)
            update_url = self._autotest_devserver.get_update_url(build_name)
            logging.info('Installing source image with update url: %s',
                         update_url)

            provisioner.ChromiumOSProvisioner(
                    update_url, host=self._host,
                    is_release_bucket=True).run_provision()

            self._run_client_test_and_check_result(self._LOGIN_TEST,
                                                   tag='source')

        # Record the active root partition.
        active, inactive = kernel_utils.get_kernel_state(self._host)
        logging.info('Source active slot: %s', active)

        # Get the source and target versions for verifying hostlog update events.
        source_release = self._host.get_release_version()
        target_release, _ = self._get_update_parameters_from_uri(
                self._target_payload_uri)
        target_release = target_release.split('/')[-1]

        logging.debug('Going to install target image on DUT.')
        self.update_device(
                self._target_payload_uri, tag='target', ignore_appid=True)

        # Compare hostlog events from the update to the expected ones.
        rootfs, reboot = self._create_hostlog_files()
        self.verify_update_events(source_release, rootfs)
        self.verify_update_events(source_release, reboot, target_release)
        kernel_utils.verify_boot_expectations(inactive, host=self._host)

        self._run_client_test_and_check_result(self._LOGIN_TEST, tag='target')

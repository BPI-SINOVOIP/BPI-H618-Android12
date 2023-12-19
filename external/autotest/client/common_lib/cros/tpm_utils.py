# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants


_RM_FILES = ['/home/chronos/.oobe_completed',
             '/home/chronos/Local\ State',
             '/var/cache/shill/default.profile']
_RM_DIRS = ['/home/.shadow/*',
            os.path.join(constants.ALLOWLIST_DIR, '*'),
            '/var/cache/app_pack',
            '/var/lib/tpm']


class NoTPMPasswordException(Exception):
    """No TPM Password could be found."""
    pass


def TPMStatus(client):
    """Returns a dictionary with TPM status.

    @param client: client object to run commands on.
    """
    out = client.run('tpm_manager_client status --nonsensitive').stdout.strip()
    lines = out.split('\n')[1:-1]
    status = {}
    for item in lines:
        item = item.split(':')
        if not item[0]:
            continue
        if len(item) == 1:
            item.append('')
        item = map(lambda x : x.strip(), item)
        item[1] = True if item[1] == 'true' else item[1]
        item[1] = False if item[1] == 'false' else item[1]
        status[item[0]] = item[1]
    return status


def IsTPMAvailable(client):
    """Returns True if the TPM is unowned and enabled.

    @param client: client object to run commands on.
    """
    status = TPMStatus(client)
    return status['is_enabled'] and not status['is_owned']


def ClearTPMServer(client, out_dir):
    """Clears the TPM and reboots from a server-side autotest.

    @param client: client object to run commands on.
    @param out_dir: temporary directory.
    """
    if IsTPMAvailable(client):
        logging.debug('TPM is not owned')
        return

    client.run('stop ui')
    client.run('crossystem clear_tpm_owner_request=1')
    CleanupAndReboot(client)

def ClearTPMOwnerRequest(client, wait_for_ready=False, timeout=60):
    """Clears the TPM using crossystem command.

    @param client: client object to run commands on.
    @param wait_for_ready: wait until the TPM status is ready
    @param timeout: number of seconds to wait for the TPM to become ready.
    """
    if not client.run('crossystem clear_tpm_owner_request=1',
                      ignore_status=True).exit_status == 0:
        raise error.TestFail('Unable to clear TPM.')

    CleanupAndReboot(client)

    if wait_for_ready:
        status = ''
        end_time = time.time() + timeout
        # Wait for tpm_manager to send a successful reply.
        while 'STATUS_SUCCESS' not in status and time.time() < end_time:
            status = client.run('tpm_manager_client status --nonsensitive',
                                ignore_status=True).stdout.strip()
            logging.debug(status)
            time.sleep(1)
        # Verify if the TPM is unowned.
        tpm_status = TPMStatus(client)
        logging.info('TPM status: %s', tpm_status)
        if tpm_status['is_owned']:
            raise error.TestFail('Failed to clear TPM.')


def ClearTPMIfOwned(client):
    """Clear the TPM only if device is already owned.

    @param client: client object to run commands on."""
    tpm_status = TPMStatus(client)
    logging.info('TPM status: %s', tpm_status)
    if tpm_status['is_owned']:
        logging.info('Clearing TPM because this device is owned.')
        ClearTPMOwnerRequest(client)


def CleanupAndReboot(client):
    """Cleanup and reboot the device.

    @param client: client object to run commands on.
    """
    full_rm = 'sudo rm -rf ' + ' '.join(_RM_FILES + _RM_DIRS)
    client.run(full_rm, ignore_status=True)
    client.run('sync', ignore_status=True)
    client.reboot()

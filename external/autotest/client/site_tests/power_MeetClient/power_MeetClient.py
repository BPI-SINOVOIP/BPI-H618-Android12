# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import collections
import enum
import json
import os
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib.cros import power_load_util
from autotest_lib.client.cros.input_playback import keyboard
from autotest_lib.client.cros.power import power_dashboard
from autotest_lib.client.cros.power import power_status
from autotest_lib.client.cros.power import power_test


class power_MeetClient(power_test.power_Test):
    """class for power_MeetClient test.

    This test should be call from power_MeetCall server test only.
    """
    version = 1

    video_url = 'http://meet.google.com'
    doc_url = 'http://doc.new'

    def initialize(self,
                   seconds_period=5.,
                   pdash_note='',
                   force_discharge=False):
        """initialize method."""
        super(power_MeetClient, self).initialize(
                seconds_period=seconds_period,
                pdash_note=pdash_note,
                force_discharge=force_discharge)

    def run_once(self,
                 meet_code,
                 duration=180,
                 layout='Tiled',
                 username=None,
                 password=None):
        """run_once method.

        @param meet_code: Meet code generated in power_MeetCall.
        @param duration: duration in seconds.
        @param layout: string of meet layout to use.
        @param username: Google account to use.
        @param password: password for Google account.
        """
        if not username and not password:
            username = power_load_util.get_meet_username()
            password = power_load_util.get_meet_password()
        if not username or not password:
            raise error.TestFail('Need to supply both username and password.')
        extra_browser_args = self.get_extra_browser_args_for_camera_test()
        with keyboard.Keyboard() as keys,\
             chrome.Chrome(init_network_controller=True,
                           gaia_login=True,
                           username=username,
                           password=password,
                           extra_browser_args=extra_browser_args,
                           autotest_ext=True) as cr:

            # Move existing window to left half and open video page
            tab = cr.browser.tabs[0]
            tab.Activate()

            # Run in full-screen.
            fullscreen = tab.EvaluateJavaScript('document.webkitIsFullScreen')
            if not fullscreen:
                keys.press_key('f4')

            url = self.video_url + '/' + meet_code
            logging.info('Navigating left window to %s', url)
            tab.Navigate(url)

            # Workaround when camera isn't init for some unknown reason.
            time.sleep(10)
            tab.EvaluateJavaScript('location.reload()')

            tab.WaitForDocumentReadyStateToBeComplete()
            logging.info(meet_code)
            self.keyvals['meet_code'] = meet_code

            def wait_until(cond, error_msg):
                """Helper for javascript polling wait."""
                for _ in range(60):
                    time.sleep(1)
                    if tab.EvaluateJavaScript(cond):
                        return
                raise error.TestFail(error_msg)

            wait_until('window.hasOwnProperty("hrTelemetryApi")',
                       'Meet API does not existed.')
            wait_until('hrTelemetryApi.isInMeeting()',
                       'Can not join meeting.')
            wait_until('hrTelemetryApi.getParticipantCount() > 1',
                       'Meeting has no other participant.')

            # Make sure camera and mic are on.
            tab.EvaluateJavaScript('hrTelemetryApi.setCameraMuted(false)')
            tab.EvaluateJavaScript('hrTelemetryApi.setMicMuted(false)')

            if layout == 'Tiled':
                tab.EvaluateJavaScript('hrTelemetryApi.setTiledLayout()')
            elif layout == 'Auto':
                tab.EvaluateJavaScript('hrTelemetryApi.setAutoLayout()')
            elif layout == 'Sidebar':
                tab.EvaluateJavaScript('hrTelemetryApi.setSidebarLayout()')
            elif layout == 'Spotlight':
                tab.EvaluateJavaScript('hrTelemetryApi.setSpotlightLayout()')
            else:
                raise error.TestError('Unknown layout %s' % layout)

            self.keyvals['layout'] = layout

            self.start_measurements()
            time.sleep(duration)
            end_time = self._start_time + duration

            # Collect stat
            if not tab.EvaluateJavaScript('window.hasOwnProperty("realtime")'):
                logging.info('Account %s is not in allowlist for MediaInfoAPI',
                             username)
                return

            meet_data = tab.EvaluateJavaScript(
                'realtime.media.getMediaInfoDataPoints()')

            power_dashboard.get_dashboard_factory().registerDataType(
                MeetStatLogger, MeetStatDashboard)

            self._meas_logs.append(
                    MeetStatLogger(self._start_time, end_time, meet_data))


class MeetStatLogger(power_status.MeasurementLogger):
    """Class for logging meet data point to power dashboard.

    Format of meet_data http://google3/logs/proto/buzz/callstats.proto
    """

    def __init__(self, start_ts, end_ts, meet_data):
        # Do not call parent constructor to avoid making a new thread.
        self.times = [start_ts]

        # Meet epoch timestamp uses millisec unit.
        self.meet_data = [data_point for data_point in meet_data
            if start_ts * 1000 <= data_point['timestamp'] <= end_ts * 1000]

    def calc(self, mtype=None):
        return {}

    def save_results(self, resultsdir, fname_prefix=None):
        # Save raw dict from meet to file. Ignore fname_prefix.
        with open(os.path.join(resultsdir, 'meet_powerlog.json'), 'w') as f:
            json.dump(self.meet_data , f, indent=4, separators=(',', ': '),
                      ensure_ascii=False)


class MeetStatDashboard(power_dashboard.MeasurementLoggerDashboard):
    """Dashboard class for MeetStatLogger class."""

    # Direction and type numbers map to constants in the proto
    class Direction(enum.IntEnum):
        """Possible directions for media entries of a data point."""
        SENDER = 0
        RECEIVER = 1

    class MediaType(enum.IntEnum):
        """Possible media types for media entries of a data point."""
        VIDEO = 2

    # Important metrics to collect.
    MEET_KEYS = [
        'encodeUsagePercent',
        'fps',
        'height',
        'width',
    ]

    def _get_ssrc_dict(self, meet_data):
        """ Extract http://what/ssrc for all video stream and map to string.

        The format of the string would be sender_# / receiver_# where # denotes
        index for the video counting from 0.

        Returns:
            dict from ssrc to video stream string.
        """
        ret = {}
        count = [0, 0]

        # We only care about video streams.
        for media in meet_data[-1]['media']:
            if media['mediatype'] != self.MediaType.VIDEO:
                continue
            if (media['direction'] != self.Direction.SENDER and
                media['direction'] != self.Direction.RECEIVER):
                continue
            name = [media['directionStr'], str(count[media['direction']])]
            if media['direction'] == self.Direction.SENDER:
                name.append(media['sendercodecname'])
            else:
                name.append(media['receiverCodecName'])
            count[media['direction']] += 1
            ret[media['ssrc']] = '_'.join(name)

        return ret

    def _get_meet_unit(self, key):
        """Return unit from name of the key."""
        if key.endswith('fps'):
            return 'fps'
        if key.endswith('Percent'):
            return 'percent'
        if key.endswith('width') or key.endswith('height') :
            return 'point'
        raise error.TestError('Unexpected key: %s' % key)

    def _get_meet_type(self, key):
        """Return type from name of the key."""
        if key.endswith('fps'):
            return 'meet_fps'
        if key.endswith('Percent'):
            return 'meet_encoder_load'
        if key.endswith('width'):
            return 'meet_width'
        if key.endswith('height'):
            return 'meet_height'
        raise error.TestError('Unexpected key: %s' % key)

    def _convert(self):
        """Convert meet raw dict to data to power dict."""

        meet_data = self._logger.meet_data
        ssrc_dict = self._get_ssrc_dict(meet_data)

        # Dict from timestamp to dict of meet_key to value
        parse_dict = collections.defaultdict(
                     lambda: collections.defaultdict(int))

        key_set = set()
        testname='power_MeetCall'

        for data_point in meet_data:
            timestamp = data_point['timestamp']
            for media in data_point['media']:
                ssrc = media.get('ssrc', 0)
                if ssrc not in ssrc_dict:
                    continue
                name = ssrc_dict[media['ssrc']]
                for meet_key in self.MEET_KEYS:
                    if meet_key not in media:
                        continue
                    key = '%s_%s' % (name, meet_key)
                    key_set.add(key)
                    parse_dict[timestamp][key] = media[meet_key]

        timestamps = sorted(parse_dict.keys())
        sample_count = len(timestamps)

        powerlog_data = collections.defaultdict(list)
        for ts in sorted(parse_dict.keys()):
            for key in key_set:
                powerlog_data[key].append(parse_dict[ts][key])

        powerlog_dict =  {
            'sample_count': sample_count,
            'sample_duration': 1,
            'average': {k: 1.0 * sum(v) / sample_count
                        for k, v in powerlog_data.iteritems()},
            'data': powerlog_data,
            'unit': {k: self._get_meet_unit(k) for k in key_set},
            'type': {k: self._get_meet_type(k) for k in key_set},
            'checkpoint': [[testname]] * sample_count,
        }

        return powerlog_dict

"""Module managing the required definitions for using the bits power monitor"""

from datetime import datetime
import logging
import os
import time

from acts import context
from acts.controllers import power_metrics
from acts.controllers import power_monitor
from acts.controllers.bits_lib import bits_client
from acts.controllers.bits_lib import bits_service
from acts.controllers.bits_lib import bits_service_config as bsc

MOBLY_CONTROLLER_CONFIG_NAME = 'Bits'
ACTS_CONTROLLER_REFERENCE_NAME = 'bitses'


def create(configs):
    return [Bits(index, config) for (index, config) in enumerate(configs)]


def destroy(bitses):
    for bits in bitses:
        bits.teardown()


def get_info(bitses):
    return [bits.config for bits in bitses]


def _transform_name(bits_metric_name):
    """Transform bits metrics names to a more succinct version.

    Examples of bits_metrics_name as provided by the client:
    - default_device.slider.C1_30__PP0750_L1S_VDD_G3D_M_P:mA,
    - default_device.slider.C1_30__PP0750_L1S_VDD_G3D_M_P:mW,
    - default_device.Monsoon.Monsoon:mA,
    - default_device.Monsoon.Monsoon:mW,
    - <device>.<collector>.<rail>:<unit>

    Args:
        bits_metric_name: A bits metric name.

    Returns:
        For monsoon metrics, and for backwards compatibility:
          Monsoon:mA -> avg_current,
          Monsoon:mW -> avg_power,

        For everything else:
          <rail>:mW -> <rail/rail>_avg_current
          <rail>:mW -> <rail/rail>_avg_power
          ...
    """
    prefix, unit = bits_metric_name.split(':')
    rail = prefix.split('.')[-1]

    if 'mW' == unit:
        suffix = 'avg_power'
    elif 'mA' == unit:
        suffix = 'avg_current'
    elif 'mV' == unit:
        suffix = 'avg_voltage'
    else:
        logging.getLogger().warning('unknown unit type for unit %s' % unit)
        suffix = ''

    if 'Monsoon' == rail:
        return suffix
    elif suffix == '':
        return rail
    else:
        return '%s_%s' % (rail, suffix)


def _raw_data_to_metrics(raw_data_obj):
    data = raw_data_obj['data']
    metrics = []
    for sample in data:
        unit = sample['unit']
        if 'Msg' == unit:
            continue
        elif 'mW' == unit:
            unit_type = 'power'
        elif 'mA' == unit:
            unit_type = 'current'
        elif 'mV' == unit:
            unit_type = 'voltage'
        else:
            logging.getLogger().warning('unknown unit type for unit %s' % unit)
            continue

        name = _transform_name(sample['name'])
        avg = sample['avg']
        metrics.append(power_metrics.Metric(avg, unit_type, unit, name=name))

    return metrics


def _get_single_file(registry, key):
    if key not in registry:
        return None
    entry = registry[key]
    if isinstance(entry, str):
        return entry
    if isinstance(entry, list):
        return None if len(entry) == 0 else entry[0]
    raise ValueError('registry["%s"] is of unsupported type %s for this '
                     'operation. Supported types are str and list.' % (
                         key, type(entry)))


class Bits(object):
    def __init__(self, index, config):
        """Creates an instance of a bits controller.

        Args:
            index: An integer identifier for this instance, this allows to
                tell apart different instances in the case where multiple
                bits controllers are being used concurrently.
            config: The config as defined in the ACTS  BiTS controller config.
                Expected format is:
                {
                    // optional
                    'Monsoon':   {
                        'serial_num': <serial number:int>,
                        'monsoon_voltage': <voltage:double>
                    }
                    // optional
                    'Kibble': [
                        {
                            'board': 'BoardName1',
                            'connector': 'A',
                            'serial': 'serial_1'
                        },
                        {
                            'board': 'BoardName2',
                            'connector': 'D',
                            'serial': 'serial_2'
                        }
                    ]
                }
        """
        self.index = index
        self.config = config
        self._service = None
        self._client = None

    def setup(self, *_, registry=None, **__):
        """Starts a bits_service in the background.

        This function needs to be
        Args:
            registry: A dictionary with files used by bits. Format:
                {
                    // required, string or list of strings
                    bits_service: ['/path/to/bits_service']

                    // required, string or list of strings
                    bits_client: ['/path/to/bits.par']

                    // needed for monsoon, string or list of strings
                    lvpm_monsoon: ['/path/to/lvpm_monsoon.par']

                    // needed for monsoon, string or list of strings
                    hvpm_monsoon: ['/path/to/hvpm_monsoon.par']

                    // needed for kibble, string or list of strings
                    kibble_bin: ['/path/to/kibble.par']

                    // needed for kibble, string or list of strings
                    kibble_board_file: ['/path/to/phone_s.board']

                    // optional, string or list of strings
                    vm_file: ['/path/to/file.vm']
                }

                All fields in this dictionary can be either a string or a list
                of strings. If lists are passed, only their first element is
                taken into account. The reason for supporting lists but only
                acting on their first element is for easier integration with
                harnesses that handle resources as lists.
        """
        if registry is None:
            registry = power_monitor.get_registry()
        if 'bits_service' not in registry:
            raise ValueError('No bits_service binary has been defined in the '
                             'global registry.')
        if 'bits_client' not in registry:
            raise ValueError('No bits_client binary has been defined in the '
                             'global registry.')

        bits_service_binary = _get_single_file(registry, 'bits_service')
        bits_client_binary = _get_single_file(registry, 'bits_client')
        lvpm_monsoon_bin = _get_single_file(registry, 'lvpm_monsoon')
        hvpm_monsoon_bin = _get_single_file(registry, 'hvpm_monsoon')
        kibble_bin = _get_single_file(registry, 'kibble_bin')
        kibble_board_file = _get_single_file(registry, 'kibble_board_file')
        vm_file = _get_single_file(registry, 'vm_file')
        config = bsc.BitsServiceConfig(self.config,
                                       lvpm_monsoon_bin=lvpm_monsoon_bin,
                                       hvpm_monsoon_bin=hvpm_monsoon_bin,
                                       kibble_bin=kibble_bin,
                                       kibble_board_file=kibble_board_file,
                                       virtual_metrics_file=vm_file)
        output_log = os.path.join(
            context.get_current_context().get_full_output_path(),
            'bits_service_out_%s.txt' % self.index)
        service_name = 'bits_config_%s' % self.index

        self._service = bits_service.BitsService(config,
                                                 bits_service_binary,
                                                 output_log,
                                                 name=service_name,
                                                 timeout=3600 * 24)
        self._service.start()
        self._client = bits_client.BitsClient(bits_client_binary,
                                              self._service,
                                              config)
        # this call makes sure that the client can interact with the server.
        devices = self._client.list_devices()
        logging.getLogger().debug(devices)

    def disconnect_usb(self, *_, **__):
        self._client.disconnect_usb()

    def connect_usb(self, *_, **__):
        self._client.connect_usb()

    def measure(self, *_, measurement_args=None, **__):
        """Blocking function that measures power through bits for the specified
        duration. Results need to be consulted through other methods such as
        get_metrics or export_to_csv.

        Args:
            measurement_args: A dictionary with the following structure:
                {
                   'duration': <seconds to measure for>
                }
        """
        if measurement_args is None:
            raise ValueError('measurement_args can not be left undefined')

        duration = measurement_args.get('duration')
        if duration is None:
            raise ValueError(
                'duration can not be left undefined within measurement_args')
        self._client.start_collection()
        time.sleep(duration)

    def get_metrics(self, *_, timestamps=None, **__):
        """Gets metrics for the segments delimited by the timestamps dictionary.

        Args:
            timestamps: A dictionary of the shape:
                {
                    'segment_name': {
                        'start' : <milliseconds_since_epoch> or <datetime>
                        'end': <milliseconds_since_epoch> or <datetime>
                    }
                    'another_segment': {
                        'start' : <milliseconds_since_epoch> or <datetime>
                        'end': <milliseconds_since_epoch> or <datetime>
                    }
                }
        Returns:
            A dictionary of the shape:
                {
                    'segment_name': <list of power_metrics.Metric>
                    'another_segment': <list of power_metrics.Metric>
                }
        """
        if timestamps is None:
            raise ValueError('timestamps dictionary can not be left undefined')

        metrics = {}

        for segment_name, times in timestamps.items():
            start = times['start']
            end = times['end']

            # bits accepts nanoseconds only, but since this interface needs to
            # backwards compatible with monsoon which works with milliseconds we
            # require to do a conversion from milliseconds to nanoseconds.
            # The preferred way for new calls to this function should be using
            # datetime instead which is unambiguous
            if isinstance(start, (int, float)):
                start = times['start'] * 1e6
            if isinstance(end, (int, float)):
                end = times['end'] * 1e6

            self._client.add_marker(start, 'start - %s' % segment_name)
            self._client.add_marker(end, 'end - %s' % segment_name)
            raw_metrics = self._client.get_metrics(start, end)
            metrics[segment_name] = _raw_data_to_metrics(raw_metrics)
        return metrics

    def release_resources(self):
        self._client.stop_collection()

    def teardown(self):
        if self._service is None:
            return

        if self._service.service_state == bits_service.BitsServiceStates.STARTED:
            self._service.stop()

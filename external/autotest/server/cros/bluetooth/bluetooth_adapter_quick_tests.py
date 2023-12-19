# Lint as: python2, python3
# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This class provides wrapper functions for Bluetooth quick health test
batches or packages
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import functools
import logging
import tempfile
import threading
import time

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import site_utils
from autotest_lib.server.cros.bluetooth import bluetooth_adapter_tests
from autotest_lib.server.cros.multimedia import remote_facade_factory
from autotest_lib.client.bin import utils
from six.moves import range

class BluetoothAdapterQuickTests(bluetooth_adapter_tests.BluetoothAdapterTests):
    """This class provide wrapper function for Bluetooth quick health test
    batches or packages.
    The Bluetooth quick test infrastructure provides a way to quickly run a set
    of tests. As for today, auto-test ramp up time per test is about 90-120
    seconds, where a typical Bluetooth test may take ~30-60 seconds to run.

    The quick test infra, implemented in this class, saves this huge overhead
    by running only the minimal reset and cleanup operations required between
    each set of tests (takes a few seconds).

    This class provides wrapper functions to start and end a test, a batch or a
    package. A batch is defined as a set of tests, preferably with a common
    subject. A package is a set of batches.
    This class takes care of tests, batches, and packages test results, and
    prints out summaries to results. The class also resets and cleans up
    required states between tests, batches and packages.

    A batch can also run as a separate auto-test. There is a place holder to
    add a way to run a specific test of a batch autonomously.

    A batch can be implemented by inheriting from this class, and using its
    wrapper functions. A package can be implemented by inheriting from a set of
    batches.

    Adding a test to one of the batches is as easy as adding a method to the
    class of the batch.
    """

    # Some delay is needed between tests. TODO(yshavit): investigate and remove
    TEST_SLEEP_SECS = 3
    GCS_MTBF_BUCKET = 'gs://chromeos-mtbf-bt-results/'


    def restart_peers(self):
        """Restart and clear peer devices"""
        # Restart the link to device
        logging.info('Restarting peer devices...')

        # Grab current device list for initialization
        connected_devices = self.devices
        self.cleanup(test_state='MID')

        for device_type, device_list in connected_devices.items():
            for device in device_list:
                if device is not None:
                    logging.info('Restarting %s', device_type)
                    self.get_device(device_type, on_start=False)


    def start_peers(self, devices):
        """Start peer devices"""
        # Start the link to devices
        if self.use_btpeer:
            logging.info('Starting peer devices...')
            self.get_device_rasp(devices)

            # Grab all the addresses to verify RSSI
            addresses = []
            for device_type, device_list in self.devices.items():
                # Skip bluetooth_tester since it won't be discoverable
                if 'TESTER' in device_type:
                    continue

                for device in device_list:
                    addresses.append(device.address)
                    self.start_agent(device)

            # Make sure device RSSI is sufficient
            self.verify_device_rssi(addresses)

    def _print_delimiter(self):
        logging.info('=======================================================')

    @staticmethod
    def _get_update_btpeers_arguments(args_dict=None):
        """Parse the update_btpeers argument"""
        key = 'update_btpeers'
        if args_dict is not None and key in args_dict:
            return args_dict[key].lower() != 'false'
        return True

    def quick_test_init(self,
                        host,
                        use_btpeer=True,
                        flag='Quick Health',
                        args_dict=None,
                        start_browser=False):
        """Inits the test batch"""
        self.host = host
        self.start_browser = start_browser
        self.use_btpeer = use_btpeer
        update_btpeers = self._get_update_btpeers_arguments(args_dict)
        btpeer_args = []
        if args_dict is not None:
            btpeer_args = self.host.get_btpeer_arguments(args_dict)
        #factory can not be declared as local variable, otherwise
        #factory._proxy.__del__ will be invoked, which shutdown the xmlrpc
        # server, which log out the user.

        self.factory = remote_facade_factory.RemoteFacadeFactory(
                host, no_chrome=not self.start_browser, disable_arc=True)
        try:
            self.bluetooth_facade = self.factory.create_bluetooth_facade()
        except Exception as e:
            logging.error('Exception %s while creating bluetooth_facade',
                          str(e))
            raise error.TestFail('Unable to create bluetooth_facade')


        if self.use_btpeer:
            self.input_facade = self.factory.create_input_facade()

            self.host.initialize_btpeer(btpeer_args=btpeer_args)
            logging.info('%s Bluetooth peers found',
                         len(self.host.btpeer_list))
            logging.info('labels: %s', self.host.get_labels())

            if len(self.host.btpeer_list) == 0:
                raise error.TestFail('Unable to find a Bluetooth peer')

            # Check the chameleond version on the peer and update if necessary
            if update_btpeers:
                if not self.update_btpeer():
                    logging.error('Updating btpeers failed. Ignored')
            else:
                logging.info('No attempting peer update.')

            # Query connected devices on our btpeer at init time
            self.available_devices = self.list_devices_available()

            for btpeer in self.host.btpeer_list:
                btpeer.register_raspPi_log(self.outputdir)

            self.btpeer_group = dict()
            # Create copy of btpeer_group
            self.btpeer_group_copy = dict()
            self.group_btpeers_type()


        # Clear the active devices for this test
        self.active_test_devices = {}

        self.enable_disable_debug_log(enable=True)

        # Disable cellular services, as they can sometimes interfere with
        # suspend/resume, i.e. b/161920740
        self.enable_disable_cellular(enable=False)

        self.enable_disable_ui(enable=False)

        # Delete files created in previous run
        self.host.run('[ ! -d {0} ] || rm -rf {0} || true'.format(
                                                    self.BTMON_DIR_LOG_PATH))
        self.host.run('[ ! -d {0} ] || rm -rf {0} || true'.format(
                                                    self.USBMON_DIR_LOG_PATH))

        self.start_new_btmon()
        self.start_new_usbmon()

        self.flag = flag
        self.test_iter = None

        self.bat_tests_results = []
        self.bat_pass_count = 0
        self.bat_fail_count = 0
        self.bat_testna_count = 0
        self.bat_warn_count = 0
        self.bat_name = None
        self.bat_iter = None

        self.pkg_tests_results = []
        self.pkg_pass_count = 0
        self.pkg_fail_count = 0
        self.pkg_testna_count = 0
        self.pkg_warn_count = 0
        self.pkg_name = None
        self.pkg_iter = None
        self.pkg_is_running = False
        self.mtbf_end = False
        self.mtbf_end_lock = threading.Lock()


    @staticmethod
    def quick_test_test_decorator(test_name, devices={}, flags=['All'],
                                  model_testNA=[],
                                  model_testWarn=[],
                                  skip_models=[],
                                  skip_chipsets=[],
                                  shared_devices_count=0):
        """A decorator providing a wrapper to a quick test.
           Using the decorator a test method can implement only the core
           test and let the decorator handle the quick test wrapper methods
           (test_start and test_end).

           @param test_name: the name of the test to log.
           @param devices:   list of device names which are going to be used
                             in the following test.
           @param flags: list of string to describe who should run the
                         test. The string could be one of the following:
                         ['AVL', 'Quick Health', 'All'].
           @param model_testNA: If the current platform is in this list,
                                failures are emitted as TestNAError.
           @param model_testWarn: If the current platform is in this list,
                                  failures are emitted as TestWarn.
           @param skip_models: Raises TestNA on these models and doesn't attempt
                               to run the tests.
           @param skip_chipsets: Raises TestNA on these chipset and doesn't
                                 attempt to run the tests.

        """

        def decorator(test_method):
            """A decorator wrapper of the decorated test_method.
               @param test_method: the test method being decorated.
               @returns the wrapper of the test method.
            """

            def _check_runnable(self):
                """Check if the test could be run"""

                # Check that the test is runnable in current setting
                if not(self.flag in flags or 'All' in flags):
                    logging.info('SKIPPING TEST %s', test_name)
                    logging.info('flag %s not in %s', self.flag, flags)
                    self._print_delimiter()
                    return False
                return True


            def _is_enough_peers_present(self):
                """Check if enough peer devices are available."""

                # Check that btpeer has all required devices before running
                for device_type, number in devices.items():
                    if self.available_devices.get(device_type, 0) < number:
                        logging.info('SKIPPING TEST %s', test_name)
                        logging.info('%s not available', device_type)
                        self._print_delimiter()
                        return False

                # Check if there are enough peers
                total_num_devices = sum(devices.values()) + shared_devices_count
                if total_num_devices > len(self.host.btpeer_list):
                    logging.info('SKIPPING TEST %s', test_name)
                    logging.info(
                            'Number of devices required %s is greater'
                            'than number of peers available %d',
                            total_num_devices, len(self.host.btpeer_list))
                    self._print_delimiter()
                    return False
                return True

            @functools.wraps(test_method)
            def wrapper(self):
                """A wrapper of the decorated method."""
                # Set test name before exiting so batches correctly identify
                # failing tests
                self.test_name = test_name

                if not _check_runnable(self):
                    return

                try:
                    if not _is_enough_peers_present(self):
                        logging.info('Not enough peer available')
                        raise error.TestNAError('Not enough peer available')

                    model = self.get_base_platform_name()
                    if model in skip_models:
                        logging.info('SKIPPING TEST %s', test_name)
                        raise error.TestNAError(
                                'Test not supported on this model')

                    chipset = self.get_chipset_name()
                    logging.debug('Bluetooth module name is %s', chipset)
                    if chipset in skip_chipsets:
                        logging.info('SKIPPING TEST %s on chipset %s',
                                     test_name, chipset)
                        raise error.TestNAError(
                                'Test not supported on this chipset')

                    self.quick_test_test_start(test_name, devices,
                                               shared_devices_count)

                    test_method(self)
                except error.TestError as e:
                    self.fails.append('[--- error {} ({})]'.format(
                            test_method.__name__, str(e)))
                except error.TestFail as e:
                    if not bool(self.fails):
                        self.fails.append('[--- failed {} ({})]'.format(
                                test_method.__name__, str(e)))
                except error.TestNAError as e:
                    self.fails.append('[--- SKIPPED: {}]'.format(str(e)))
                except Exception as e:
                    self.fails.append('[--- unknown error {} ({})]'.format(
                            test_method.__name__, str(e)))

                self.quick_test_test_end(model_testNA=model_testNA,
                                         model_testWarn=model_testWarn)
            return wrapper

        return decorator


    def quick_test_test_start(
            self, test_name=None, devices={}, shared_devices_count=0):
        """Start a quick test. The method clears and restarts adapter on DUT
           as well as peer devices. In addition the methods prints test start
           traces.
        """
        # Bluetoothd could have crashed behind the scenes; check to see if
        # everything is still ok and recover if needed.
        self.test_is_facade_valid()
        self.test_is_adapter_valid()

        # Reset the adapter
        self.test_reset_on_adapter()
        # Initialize bluetooth_adapter_tests class (also clears self.fails)
        self.initialize()
        # Start and peer HID devices
        self.start_peers(devices)
        self.shared_peers = self.host.btpeer_list[-shared_devices_count:]

        if test_name is not None:
            time.sleep(self.TEST_SLEEP_SECS)
            self._print_delimiter()
            logging.info('Starting test: %s', test_name)
            self.log_message('Starting test: %s'% test_name)

    def quick_test_test_end(self, model_testNA=[], model_testWarn=[]):
        """Log and track the test results"""
        result_msgs = []
        model = self.get_base_platform_name()

        if self.test_iter is not None:
            result_msgs += ['Test Iter: ' + str(self.test_iter)]

        if self.bat_iter is not None:
            result_msgs += ['Batch Iter: ' + str(self.bat_iter)]

        if self.pkg_is_running is True:
            result_msgs += ['Package iter: ' + str(self.pkg_iter)]

        if self.bat_name is not None:
            result_msgs += ['Batch Name: ' + self.bat_name]

        if self.test_name is not None:
            result_msgs += ['Test Name: ' + self.test_name]

        result_msg = ", ".join(result_msgs)

        if not bool(self.fails):
            result_msg = 'PASSED | ' + result_msg
            self.bat_pass_count += 1
            self.pkg_pass_count += 1
        # The test should be marked as TESTNA if any of the test expressions
        # were SKIPPED (they threw their own TESTNA error) or the model is in
        # the list of NA models (so any failure is considered NA instead)
        elif model in model_testNA or any(['SKIPPED' in x
                                           for x in self.fails]):
            result_msg = 'TESTNA | ' + result_msg
            self.bat_testna_count += 1
            self.pkg_testna_count += 1
        elif model in model_testWarn:
            result_msg = 'WARN   | ' + result_msg
            self.bat_warn_count += 1
            self.pkg_warn_count += 1
        else:
            result_msg = 'FAIL   | ' + result_msg
            self.bat_fail_count += 1
            self.pkg_fail_count += 1

        logging.info(result_msg)
        self.log_message(result_msg)
        self._print_delimiter()
        self.bat_tests_results.append(result_msg)
        self.pkg_tests_results.append(result_msg)

        if self.test_name is not None:
            logging.info('Cleanning up and restarting towards next test...')


        # Bluetoothd could have crashed behind the scenes; check if everything
        # is ok and recover if needed. This is done as part of clean-up as well
        # to make sure we can fully remove pairing info between tests
        self.test_is_facade_valid()

        self.bluetooth_facade.stop_discovery()

        # Store a copy of active devices for raspi reset in the final step
        self.active_test_devices = self.devices

        # Disconnect devices used in the test, and remove the pairing.
        for device_list in self.devices.values():
            for device in device_list:
                if device is not None:
                    self.stop_agent(device)
                    logging.info('Clear device %s from DUT', device.name)
                    self.bluetooth_facade.disconnect_device(device.address)
                    device_is_paired = self.bluetooth_facade.device_is_paired(
                            device.address)
                    if device_is_paired:
                        self.bluetooth_facade.remove_device_object(
                                device.address)

                    # Also remove pairing on Peer
                    logging.info('Clearing DUT from %s', device.name)
                    try:
                        device.RemoveDevice(self.bluetooth_facade.address)
                    except Exception as e:
                        # If peer doesn't expose RemoveDevice, ignore failure
                        if not (e.__class__.__name__ == 'Fault' and
                                'is not supported' in str(e)):
                            logging.info('Couldn\'t Remove: {}'.format(e))
                            raise


        # Repopulate btpeer_group for next tests
        # Clear previous tets's leftover entries. Don't delete the
        # btpeer_group dictionary though, it'll be used as it is.
        if self.use_btpeer:
            for device_type in self.btpeer_group:
                if len(self.btpeer_group[device_type]) > 0:
                    del self.btpeer_group[device_type][:]

            # Repopulate
            self.group_btpeers_type()

        # Close the connection between peers
        self.cleanup(test_state='NEW')

    @staticmethod
    def quick_test_batch_decorator(batch_name):
        """A decorator providing a wrapper to a batch.
           Using the decorator a test batch method can implement only its
           core tests invocations and let the decorator handle the wrapper,
           which is taking care for whether to run a specific test or the
           batch as a whole and and running the batch in iterations

           @param batch_name: the name of the batch to log
        """

        def decorator(batch_method):
            """A decorator wrapper of the decorated test_method.
               @param test_method: the test method being decorated.
               @returns the wrapper of the test method.
            """

            @functools.wraps(batch_method)
            def wrapper(self, num_iterations=1, test_name=None):
                """A wrapper of the decorated method.
                  @param num_iterations: how many interations to run
                  @param test_name: specifc test to run otherwise None to run
                                    the whole batch
                """
                if test_name is not None:
                    single_test_method = getattr(self,  test_name)
                    for iter in range(1,num_iterations+1):
                        self.test_iter = iter
                        single_test_method()

                    if self.fails:
                        # If failure is marked as TESTNA, prioritize that over
                        # a failure. Same with WARN.
                        if self.bat_testna_count > 0:
                            raise error.TestNAError(self.fails)
                        elif self.bat_warn_count > 0:
                            raise error.TestWarn(self.fails)
                        else:
                            raise error.TestFail(self.fails)
                else:
                    for iter in range(1,num_iterations+1):
                        self.quick_test_batch_start(batch_name, iter)
                        batch_method(self, num_iterations, test_name)
                        self.quick_test_batch_end()
            return wrapper

        return decorator


    def quick_test_batch_start(self, bat_name, iteration=1):
        """Start a test batch. The method clears and set batch variables"""
        self.bat_tests_results = []
        self.bat_pass_count = 0
        self.bat_fail_count = 0
        self.bat_testna_count = 0
        self.bat_warn_count = 0
        self.bat_name = bat_name
        self.bat_iter = iteration


    def quick_test_batch_end(self):
        """Print results summary of a test batch """
        logging.info(
                '%s Test Batch Summary: total pass %d, total fail %d, '
                'warn %d, NA %d', self.bat_name, self.bat_pass_count,
                self.bat_fail_count, self.bat_warn_count,
                self.bat_testna_count)
        for result in self.bat_tests_results:
            logging.info(result)
        self._print_delimiter();
        if self.bat_fail_count > 0:
            logging.error('===> Test Batch Failed! More than one failure')
            self._print_delimiter();
            if self.pkg_is_running is False:
                raise error.TestFail(self.bat_tests_results)
        elif self.bat_testna_count > 0:
            logging.error('===> Test Batch Passed! Some TestNA results')
            self._print_delimiter();
            if self.pkg_is_running is False:
                raise error.TestNAError(self.bat_tests_results)
        elif self.bat_warn_count > 0:
            logging.error('===> Test Batch Passed! Some WARN results')
            self._print_delimiter();
            if self.pkg_is_running is False:
                raise error.TestWarn(self.bat_tests_results)
        else:
            logging.info('===> Test Batch Passed! zero failures')
            self._print_delimiter();


    def quick_test_package_start(self, pkg_name):
        """Start a test package. The method clears and set batch variables"""
        self.pkg_tests_results = []
        self.pkg_pass_count = 0
        self.pkg_fail_count = 0
        self.pkg_name = pkg_name
        self.pkg_is_running = True


    def quick_test_print_summary(self):
        """Print results summary of a test package"""
        logging.info(
                '%s Test Package Summary: total pass %d, total fail %d, '
                'Warn %d, NA %d', self.pkg_name, self.pkg_pass_count,
                self.pkg_fail_count, self.pkg_warn_count,
                self.pkg_testna_count)
        for result in self.pkg_tests_results:
            logging.info(result)
        self._print_delimiter();


    def quick_test_package_update_iteration(self, iteration):
        """Update state and print log per package iteration.
           Must be called to have a proper package test result tracking.
        """
        self.pkg_iter = iteration
        if self.pkg_name is None:
            logging.error('Error: no quick package is running')
            raise error.TestFail('Error: no quick package is running')
        logging.info('Starting %s Test Package iteration %d',
                     self.pkg_name, iteration)


    def quick_test_package_end(self):
        """Print final result of a test package"""
        if self.pkg_fail_count > 0:
            logging.error('===> Test Package Failed! More than one failure')
            self._print_delimiter();
            raise error.TestFail(self.bat_tests_results)
        elif self.pkg_testna_count > 0:
            logging.error('===> Test Package Passed! Some TestNA results')
            self._print_delimiter();
            raise error.TestNAError(self.bat_tests_results)
        elif self.pkg_warn_count > 0:
            logging.error('===> Test Package Passed! Some WARN results')
            self._print_delimiter();
            raise error.TestWarn(self.bat_tests_results)
        else:
            logging.info('===> Test Package Passed! zero failures')
            self._print_delimiter();
        self.pkg_is_running = False


    def quick_test_cleanup(self):
        """ Cleanup any state test server and all device"""

        # Clear any raspi devices at very end of test
        for device_list in self.active_test_devices.values():
            for device in device_list:
                if device is not None:
                    self.clear_raspi_device(device)
                    self.device_set_powered(device, False)

        # Reset the adapter
        self.test_reset_on_adapter()
        # Initialize bluetooth_adapter_tests class (also clears self.fails)
        self.initialize()


    @staticmethod
    def quick_test_mtbf_decorator(timeout_mins, test_name):
        """A decorator enabling a test to be run as a MTBF test, it will run
           the underlying test in a infinite loop until it fails or timeout is
           reached, in both cases the time elapsed time will be reported.

           @param timeout_mins: the max execution time of the test, once the
                                time is up the test will report success and exit
           @param test_name: the MTBF test name to be output to the dashboard
        """

        def decorator(batch_method):
            """A decorator wrapper of the decorated batch_method.
               @param batch_method: the batch method being decorated.
               @returns the wrapper of the batch method.
            """

            @functools.wraps(batch_method)
            def wrapper(self, *args, **kwargs):
                """A wrapper of the decorated method"""
                self.mtbf_end = False
                mtbf_timer = threading.Timer(
                    timeout_mins * 60, self.mtbf_timeout)
                mtbf_timer.start()
                start_time = time.time()
                board = self.host.get_board().split(':')[1]
                model = self.host.get_model_from_cros_config()
                build = self.host.get_release_version()
                milestone = 'M' + self.host.get_chromeos_release_milestone()
                in_lab = site_utils.host_in_lab(self.host.hostname)
                while True:
                    with self.mtbf_end_lock:
                        # The test ran the full duration without failure
                        if self.mtbf_end:
                            self.report_mtbf_result(
                                True, start_time, test_name, model, build,
                                milestone, board, in_lab)
                            break
                    try:
                        batch_method(self, *args, **kwargs)
                    except Exception as e:
                        logging.info("Caught a failure: %r", e)
                        self.report_mtbf_result(
                            False, start_time, test_name, model, build,
                            milestone, board, in_lab)
                        # Don't report the test run as failed for MTBF
                        self.fails = []
                        break

                mtbf_timer.cancel()

            return wrapper

        return decorator


    def mtbf_timeout(self):
        """Handle time out event of a MTBF test"""
        with self.mtbf_end_lock:
            self.mtbf_end = True


    def report_mtbf_result(self, success, start_time, test_name, model, build,
        milestone, board, in_lab):
        """Report MTBF result by uploading it to GCS"""
        duration_secs = int(time.time() - start_time)
        start_time = int(start_time)
        gm_time_struct = time.localtime(start_time)
        output_file_name = self.GCS_MTBF_BUCKET + \
                           time.strftime('%Y-%m-%d/', gm_time_struct) + \
                           time.strftime('%H-%M-%S.csv', gm_time_struct)

        mtbf_result = '{0},{1},{2},{3},{4},{5},{6},{7}'.format(
            model, build, milestone, start_time * 1000000, duration_secs,
            success, test_name, board)
        with tempfile.NamedTemporaryFile() as tmp_file:
            tmp_file.write(mtbf_result.encode('utf-8'))
            tmp_file.flush()
            cmd = 'gsutil cp {0} {1}'.format(tmp_file.name, output_file_name)
            logging.info('Result to upload %s %s', mtbf_result, cmd)
            # Only upload the result when running in the lab.
            if in_lab:
                logging.info('Uploading result')
                utils.run(cmd)

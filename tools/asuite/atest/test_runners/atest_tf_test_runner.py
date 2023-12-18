# Copyright 2017, The Android Open Source Project
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

"""Atest Tradefed test runner class."""

# pylint: disable=line-too-long

from __future__ import print_function

import json
import logging
import os
import re
import select
import shutil
import socket
import uuid

from functools import partial
from pathlib import Path

import atest_utils
import constants
import result_reporter

from logstorage import atest_gcp_utils
from logstorage import logstorage_utils
from metrics import metrics
from test_finders import test_finder_utils
from test_finders import test_info
from test_runners import test_runner_base
from .event_handler import EventHandler

POLL_FREQ_SECS = 10
SOCKET_HOST = '127.0.0.1'
SOCKET_QUEUE_MAX = 1
SOCKET_BUFFER = 4096
SELECT_TIMEOUT = 0.5

# Socket Events of form FIRST_EVENT {JSON_DATA}\nSECOND_EVENT {JSON_DATA}
# EVENT_RE has groups for the name and the data. "." does not match \n.
EVENT_RE = re.compile(r'\n*(?P<event_name>[A-Z_]+) (?P<json_data>{.*})(?=\n|.)*')

EXEC_DEPENDENCIES = ('adb', 'aapt', 'fastboot')

TRADEFED_EXIT_MSG = 'TradeFed subprocess exited early with exit code=%s.'

LOG_FOLDER_NAME = 'log'

_INTEGRATION_FINDERS = frozenset(['', 'INTEGRATION', 'INTEGRATION_FILE_PATH'])

class TradeFedExitError(Exception):
    """Raised when TradeFed exists before test run has finished."""


class AtestTradefedTestRunner(test_runner_base.TestRunnerBase):
    """TradeFed Test Runner class."""
    NAME = 'AtestTradefedTestRunner'
    EXECUTABLE = 'atest_tradefed.sh'
    _TF_TEMPLATE = 'template/atest_local_min'
    # Use --no-enable-granular-attempts to control reporter replay behavior.
    # TODO(b/142630648): Enable option enable-granular-attempts
    # in sharding mode.
    _LOG_ARGS = ('--logcat-on-failure --atest-log-file-path={log_path} '
                 '--no-enable-granular-attempts '
                 '--proto-output-file={proto_path}')
    _RUN_CMD = ('{env} {exe} {template} --template:map '
                'test=atest {tf_customize_template} {log_args} {args}')
    _BUILD_REQ = {'tradefed-core'}
    _RERUN_OPTION_GROUP = [constants.ITERATIONS,
                           constants.RERUN_UNTIL_FAILURE,
                           constants.RETRY_ANY_FAILURE]

    def __init__(self, results_dir, module_info=None, **kwargs):
        """Init stuff for base class."""
        super().__init__(results_dir, **kwargs)
        self.module_info = module_info
        self.log_path = os.path.join(results_dir, LOG_FOLDER_NAME)
        if not os.path.exists(self.log_path):
            os.makedirs(self.log_path)
        log_args = {'log_path': self.log_path,
                    'proto_path': os.path.join(self.results_dir, constants.ATEST_TEST_RECORD_PROTO)}
        self.run_cmd_dict = {'env': self._get_ld_library_path(),
                             'exe': self.EXECUTABLE,
                             'template': self._TF_TEMPLATE,
                             'tf_customize_template': '',
                             'args': '',
                             'log_args': self._LOG_ARGS.format(**log_args)}
        self.is_verbose = logging.getLogger().isEnabledFor(logging.DEBUG)
        self.root_dir = os.environ.get(constants.ANDROID_BUILD_TOP)

    def _get_ld_library_path(self):
        """Get the extra environment setup string for running TF.

        Returns:
            Strings for the environment passed to TF. Currently only
            LD_LIBRARY_PATH for TF to load the correct local shared libraries.
        """
        out_dir = os.environ.get(constants.ANDROID_HOST_OUT, '')
        lib_dirs = ['lib', 'lib64']
        path = ''
        for lib in lib_dirs:
            lib_dir = os.path.join(out_dir, lib)
            path = path + lib_dir + ':'
        return 'LD_LIBRARY_PATH=%s' % path

    def _try_set_gts_authentication_key(self):
        """Set GTS authentication key if it is available or exists.

        Strategy:
            Get APE_API_KEY from os.environ:
                - If APE_API_KEY is already set by user -> do nothing.
            Get the APE_API_KEY from constants:
                - If the key file exists -> set to env var.
            If APE_API_KEY isn't set and the key file doesn't exist:
                - Warn user some GTS tests may fail without authentication.
        """
        if os.environ.get('APE_API_KEY'):
            logging.debug('APE_API_KEY is set by developer.')
            return
        ape_api_key = constants.GTS_GOOGLE_SERVICE_ACCOUNT
        key_path = os.path.join(self.root_dir, ape_api_key)
        if ape_api_key and os.path.exists(key_path):
            logging.debug('Set APE_API_KEY: %s', ape_api_key)
            os.environ['APE_API_KEY'] = key_path
        else:
            logging.debug('APE_API_KEY not set, some GTS tests may fail'
                          ' without authentication.')

    def run_tests(self, test_infos, extra_args, reporter):
        """Run the list of test_infos. See base class for more.

        Args:
            test_infos: A list of TestInfos.
            extra_args: Dict of extra args to add to test run.
            reporter: An instance of result_report.ResultReporter.

        Returns:
            0 if tests succeed, non-zero otherwise.
        """
        reporter.log_path = self.log_path
        reporter.rerun_options = self._extract_rerun_options(extra_args)
        # Set google service key if it's available or found before
        # running tests.
        self._try_set_gts_authentication_key()
        result = 0
        creds, inv = self._do_upload_flow(extra_args)
        try:
            if os.getenv(test_runner_base.OLD_OUTPUT_ENV_VAR):
                result = self.run_tests_raw(test_infos, extra_args, reporter)
            result = self.run_tests_pretty(test_infos, extra_args, reporter)
        finally:
            if inv:
                try:
                    logging.disable(logging.INFO)
                    # Always set invocation status to completed due to the ATest
                    # handle whole process by its own.
                    inv['schedulerState'] = 'completed'
                    logstorage_utils.BuildClient(creds).update_invocation(inv)
                    reporter.test_result_link = (constants.RESULT_LINK
                                                 % inv['invocationId'])
                finally:
                    logging.disable(logging.NOTSET)
        return result

    def _do_upload_flow(self, extra_args):
        """Run upload flow.

        Asking user's decision and do the related steps.

        Args:
            extra_args: Dict of extra args to add to test run.
        Return:
            tuple(invocation, workunit)
        """
        config_folder = os.path.join(os.path.expanduser('~'), '.atest')
        creds = self._request_consent_of_upload_test_result(
            config_folder,
            extra_args.get(constants.REQUEST_UPLOAD_RESULT, None))
        if creds:
            inv, workunit = self._prepare_data(creds)
            extra_args[constants.INVOCATION_ID] = inv['invocationId']
            extra_args[constants.WORKUNIT_ID] = workunit['id']
            if not os.path.exists(os.path.dirname(constants.TOKEN_FILE_PATH)):
                os.makedirs(os.path.dirname(constants.TOKEN_FILE_PATH))
            with open(constants.TOKEN_FILE_PATH, 'w') as token_file:
                token_file.write(creds.token_response['access_token'])
            return creds, inv
        return None, None

    def _prepare_data(self, creds):
        """Prepare data for build api using.

        Args:
            creds: The credential object.
        Return:
            invocation and workunit object.
        """
        try:
            logging.disable(logging.INFO)
            external_id = str(uuid.uuid4())
            client = logstorage_utils.BuildClient(creds)
            branch = self._get_branch(client)
            target = self._get_target(branch, client)
            build_record = client.insert_local_build(external_id,
                                                     target,
                                                     branch)
            client.insert_build_attempts(build_record)
            invocation = client.insert_invocation(build_record)
            workunit = client.insert_work_unit(invocation)
            return invocation, workunit
        finally:
            logging.disable(logging.NOTSET)

    def _get_branch(self, build_client):
        """Get source code tree branch.

        Args:
            build_client: The build client object.
        Return:
            "git_master" in internal git, "aosp-master" otherwise.
        """
        default_branch = ('git_master'
                          if constants.CREDENTIAL_FILE_NAME else 'aosp-master')
        local_branch = atest_utils.get_manifest_branch()
        branches = [b['name'] for b in build_client.list_branch()['branches']]
        return local_branch if local_branch in branches else default_branch

    def _get_target(self, branch, build_client):
        """Get local build selected target.

        Args:
            branch: The branch want to check.
            build_client: The build client object.
        Return:
            The matched build target, "aosp_x86-userdebug" otherwise.
        """
        default_target = 'aosp_x86-userdebug'
        local_target = atest_utils.get_build_target()
        targets = [t['target']
                   for t in build_client.list_target(branch)['targets']]
        return local_target if local_target in targets else default_target

    def _request_consent_of_upload_test_result(self, config_folder,
                                               request_to_upload_result):
        """Request the consent of upload test results at the first time.

        Args:
            config_folder: The directory path to put config file.
            request_to_upload_result: Prompt message for user determine.
        Return:
            The credential object.
        """
        if not os.path.exists(config_folder):
            os.makedirs(config_folder)
        not_upload_file = os.path.join(config_folder,
                                       constants.DO_NOT_UPLOAD)
        # Do nothing if there are no related config or DO_NOT_UPLOAD exists.
        if (not constants.CREDENTIAL_FILE_NAME or
                not constants.TOKEN_FILE_PATH):
            return None

        creds_f = os.path.join(config_folder, constants.CREDENTIAL_FILE_NAME)
        if request_to_upload_result:
            if os.path.exists(not_upload_file):
                os.remove(not_upload_file)
            if os.path.exists(creds_f):
                os.remove(creds_f)

        # If the credential file exists or the user says “Yes”, ATest will
        # try to get the credential from the file, else will create a
        # DO_NOT_UPLOAD to keep the user's decision.
        if not os.path.exists(not_upload_file):
            if (os.path.exists(creds_f) or
                    (request_to_upload_result and
                     atest_utils.prompt_with_yn_result(
                         constants.UPLOAD_TEST_RESULT_MSG, False))):
                return atest_gcp_utils.GCPHelper(
                    client_id=constants.CLIENT_ID,
                    client_secret=constants.CLIENT_SECRET,
                    user_agent='atest').get_credential_with_auth_flow(creds_f)

        Path(not_upload_file).touch()
        return None

    def run_tests_raw(self, test_infos, extra_args, reporter):
        """Run the list of test_infos. See base class for more.

        Args:
            test_infos: A list of TestInfos.
            extra_args: Dict of extra args to add to test run.
            reporter: An instance of result_report.ResultReporter.

        Returns:
            0 if tests succeed, non-zero otherwise.
        """
        iterations = self._generate_iterations(extra_args)
        reporter.register_unsupported_runner(self.NAME)

        ret_code = constants.EXIT_CODE_SUCCESS
        for _ in range(iterations):
            run_cmds = self.generate_run_commands(test_infos, extra_args)
            subproc = self.run(run_cmds[0], output_to_stdout=True,
                               env_vars=self.generate_env_vars(extra_args))
            ret_code |= self.wait_for_subprocess(subproc)
        return ret_code

    def run_tests_pretty(self, test_infos, extra_args, reporter):
        """Run the list of test_infos. See base class for more.

        Args:
            test_infos: A list of TestInfos.
            extra_args: Dict of extra args to add to test run.
            reporter: An instance of result_report.ResultReporter.

        Returns:
            0 if tests succeed, non-zero otherwise.
        """
        iterations = self._generate_iterations(extra_args)
        ret_code = constants.EXIT_CODE_SUCCESS
        for _ in range(iterations):
            server = self._start_socket_server()
            run_cmds = self.generate_run_commands(test_infos, extra_args,
                                                  server.getsockname()[1])
            subproc = self.run(run_cmds[0], output_to_stdout=self.is_verbose,
                               env_vars=self.generate_env_vars(extra_args))
            self.handle_subprocess(subproc, partial(self._start_monitor,
                                                    server,
                                                    subproc,
                                                    reporter,
                                                    extra_args))
            server.close()
            ret_code |= self.wait_for_subprocess(subproc)
        return ret_code

    # pylint: disable=too-many-branches
    # pylint: disable=too-many-locals
    def _start_monitor(self, server, tf_subproc, reporter, extra_args):
        """Polling and process event.

        Args:
            server: Socket server object.
            tf_subproc: The tradefed subprocess to poll.
            reporter: Result_Reporter object.
            extra_args: Dict of extra args to add to test run.
        """
        inputs = [server]
        event_handlers = {}
        data_map = {}
        inv_socket = None
        while inputs:
            try:
                readable, _, _ = select.select(inputs, [], [], SELECT_TIMEOUT)
                for socket_object in readable:
                    if socket_object is server:
                        conn, addr = socket_object.accept()
                        logging.debug('Accepted connection from %s', addr)
                        conn.setblocking(False)
                        inputs.append(conn)
                        data_map[conn] = ''
                        # The First connection should be invocation
                        # level reporter.
                        if not inv_socket:
                            inv_socket = conn
                    else:
                        # Count invocation level reporter events
                        # without showing real-time information.
                        if inv_socket == socket_object:
                            reporter.silent = True
                            event_handler = event_handlers.setdefault(
                                socket_object, EventHandler(reporter,
                                                            self.NAME))
                        else:
                            event_handler = event_handlers.setdefault(
                                socket_object, EventHandler(
                                    result_reporter.ResultReporter(
                                        collect_only=extra_args.get(
                                            constants.COLLECT_TESTS_ONLY),
                                        flakes_info=extra_args.get(
                                            constants.FLAKES_INFO)),

                                    self.NAME))
                        recv_data = self._process_connection(data_map,
                                                             socket_object,
                                                             event_handler)
                        if not recv_data:
                            inputs.remove(socket_object)
                            socket_object.close()
            finally:
                # Subprocess ended and all socket clients were closed.
                if tf_subproc.poll() is not None and len(inputs) == 1:
                    inputs.pop().close()
                    if not reporter.all_test_results:
                        atest_utils.colorful_print(
                            r'No test to run. Please check: '
                            r'{} for detail.'.format(reporter.log_path),
                            constants.RED, highlight=True)
                    if not data_map:
                        raise TradeFedExitError(TRADEFED_EXIT_MSG
                                                % tf_subproc.returncode)
                    self._handle_log_associations(event_handlers)

    def _process_connection(self, data_map, conn, event_handler):
        """Process a socket connection betwen TF and ATest.

        Expect data of form EVENT_NAME {JSON_DATA}.  Multiple events will be
        \n deliminated.  Need to buffer data in case data exceeds socket
        buffer.
        E.q.
            TEST_RUN_STARTED {runName":"hello_world_test","runAttempt":0}\n
            TEST_STARTED {"start_time":2172917, "testName":"PrintHelloWorld"}\n
        Args:
            data_map: The data map of all connections.
            conn: Socket connection.
            event_handler: EventHandler object.

        Returns:
            True if conn.recv() has data , False otherwise.
        """
        # Set connection into blocking mode.
        conn.settimeout(None)
        data = conn.recv(SOCKET_BUFFER)
        if isinstance(data, bytes):
            data = data.decode()
        logging.debug('received: %s', data)
        if data:
            data_map[conn] += data
            while True:
                match = EVENT_RE.match(data_map[conn])
                if not match:
                    break
                try:
                    event_data = json.loads(match.group('json_data'))
                except ValueError:
                    logging.debug('Json incomplete, wait for more data')
                    break
                event_name = match.group('event_name')
                event_handler.process_event(event_name, event_data)
                data_map[conn] = data_map[conn][match.end():]
        return bool(data)

    def _start_socket_server(self):
        """Start a TCP server."""
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # Port 0 lets the OS pick an open port between 1024 and 65535.
        server.bind((SOCKET_HOST, 0))
        server.listen(SOCKET_QUEUE_MAX)
        server.settimeout(POLL_FREQ_SECS)
        logging.debug('Socket server started on port %s',
                      server.getsockname()[1])
        return server

    def generate_env_vars(self, extra_args):
        """Convert extra args into env vars."""
        env_vars = os.environ.copy()
        if constants.TF_GLOBAL_CONFIG:
            env_vars["TF_GLOBAL_CONFIG"] = constants.TF_GLOBAL_CONFIG
        debug_port = extra_args.get(constants.TF_DEBUG, '')
        if debug_port:
            env_vars['TF_DEBUG'] = 'true'
            env_vars['TF_DEBUG_PORT'] = str(debug_port)
        filtered_paths = []
        for path in str(env_vars['PYTHONPATH']).split(':'):
            # TODO (b/166216843) Remove the hacky PYTHON path workaround.
            if (str(path).startswith('/tmp/Soong.python_') and
                    str(path).find('googleapiclient') > 0):
                continue
            filtered_paths.append(path)
        env_vars['PYTHONPATH'] = ':'.join(filtered_paths)
        return env_vars

    # pylint: disable=unnecessary-pass
    # Please keep above disable flag to ensure host_env_check is overriden.
    def host_env_check(self):
        """Check that host env has everything we need.

        We actually can assume the host env is fine because we have the same
        requirements that atest has. Update this to check for android env vars
        if that changes.
        """
        pass

    @staticmethod
    def _is_missing_exec(executable):
        """Check if system build executable is available.

        Args:
            executable: Executable we are checking for.

        Returns:
            True if executable is missing, False otherwise.
        """
        output = shutil.which(executable)
        if not output:
            return True
        # TODO: Check if there is a clever way to determine if system adb is
        # good enough.
        root_dir = os.environ.get(constants.ANDROID_BUILD_TOP)
        return os.path.commonprefix([output, root_dir]) != root_dir

    def get_test_runner_build_reqs(self):
        """Return the build requirements.

        Returns:
            Set of build targets.
        """
        build_req = self._BUILD_REQ
        # Use different base build requirements if google-tf is around.
        if self.module_info.is_module(constants.GTF_MODULE):
            build_req = {constants.GTF_TARGET}
        # Always add ATest's own TF target.
        build_req.add(constants.ATEST_TF_MODULE)
        # Add adb if we can't find it.
        for executable in EXEC_DEPENDENCIES:
            if self._is_missing_exec(executable):
                build_req.add(executable)
        return build_req

    # pylint: disable=too-many-branches
    # pylint: disable=too-many-statements
    def _parse_extra_args(self, test_infos, extra_args):
        """Convert the extra args into something tf can understand.

        Args:
            extra_args: Dict of args

        Returns:
            Tuple of args to append and args not supported.
        """
        args_to_append = []
        args_not_supported = []
        for arg in extra_args:
            if constants.WAIT_FOR_DEBUGGER == arg:
                args_to_append.append('--wait-for-debugger')
                continue
            if constants.DISABLE_INSTALL == arg:
                args_to_append.append('--disable-target-preparers')
                continue
            if constants.SERIAL == arg:
                args_to_append.append('--serial')
                args_to_append.append(extra_args[arg])
                continue
            if constants.SHARDING == arg:
                args_to_append.append('--shard-count')
                args_to_append.append(str(extra_args[arg]))
                continue
            if constants.DISABLE_TEARDOWN == arg:
                args_to_append.append('--disable-teardown')
                continue
            if constants.HOST == arg:
                args_to_append.append('-n')
                args_to_append.append('--prioritize-host-config')
                args_to_append.append('--skip-host-arch-check')
                continue
            if constants.CUSTOM_ARGS == arg:
                # We might need to sanitize it prior to appending but for now
                # let's just treat it like a simple arg to pass on through.
                args_to_append.extend(extra_args[arg])
                continue
            if constants.ALL_ABI == arg:
                args_to_append.append('--all-abi')
                continue
            if constants.DRY_RUN == arg:
                continue
            if constants.FLAKES_INFO == arg:
                continue
            if constants.INSTANT == arg:
                args_to_append.append('--enable-parameterized-modules')
                args_to_append.append('--module-parameter')
                args_to_append.append('instant_app')
                continue
            if constants.USER_TYPE == arg:
                args_to_append.append('--enable-parameterized-modules')
                args_to_append.append('--enable-optional-parameterization')
                args_to_append.append('--module-parameter')
                args_to_append.append(extra_args[arg])
                continue
            if constants.ITERATIONS == arg:
                args_to_append.append('--retry-strategy')
                args_to_append.append(constants.ITERATIONS)
                args_to_append.append('--max-testcase-run-count')
                args_to_append.append(str(extra_args[arg]))
                continue
            if constants.RERUN_UNTIL_FAILURE == arg:
                args_to_append.append('--retry-strategy')
                args_to_append.append(constants.RERUN_UNTIL_FAILURE)
                args_to_append.append('--max-testcase-run-count')
                args_to_append.append(str(extra_args[arg]))
                continue
            if constants.RETRY_ANY_FAILURE == arg:
                args_to_append.append('--retry-strategy')
                args_to_append.append(constants.RETRY_ANY_FAILURE)
                args_to_append.append('--max-testcase-run-count')
                args_to_append.append(str(extra_args[arg]))
                continue
            if constants.COLLECT_TESTS_ONLY == arg:
                args_to_append.append('--collect-tests-only')
                continue
            if constants.TF_DEBUG == arg:
                print("Please attach process to your IDE...")
                continue
            if arg in (constants.TF_TEMPLATE,
                       constants.TF_EARLY_DEVICE_RELEASE,
                       constants.INVOCATION_ID,
                       constants.WORKUNIT_ID):
                continue
            args_not_supported.append(arg)
        # Set exclude instant app annotation for non-instant mode run.
        if (constants.INSTANT not in extra_args and
            self._has_instant_app_config(test_infos, self.module_info)):
            args_to_append.append(constants.TF_TEST_ARG)
            args_to_append.append(
                '{tf_class}:{option_name}:{option_value}'.format(
                    tf_class=constants.TF_AND_JUNIT_CLASS,
                    option_name=constants.TF_EXCLUDE_ANNOTATE,
                    option_value=constants.INSTANT_MODE_ANNOTATE))
        # If test config has config with auto enable parameter, force exclude
        # those default parameters(ex: instant_app, secondary_user)
        if '--enable-parameterized-modules' not in args_to_append:
            for tinfo in test_infos:
                if self._is_parameter_auto_enabled_cfg(tinfo, self.module_info):
                    args_to_append.append('--enable-parameterized-modules')
                    for exclude_parameter in constants.DEFAULT_EXCLUDE_PARAS:
                        args_to_append.append('--exclude-module-parameters')
                        args_to_append.append(exclude_parameter)
                    break
        return args_to_append, args_not_supported

    def _generate_metrics_folder(self, extra_args):
        """Generate metrics folder."""
        metrics_folder = ''
        if extra_args.get(constants.PRE_PATCH_ITERATIONS):
            metrics_folder = os.path.join(self.results_dir, 'baseline-metrics')
        elif extra_args.get(constants.POST_PATCH_ITERATIONS):
            metrics_folder = os.path.join(self.results_dir, 'new-metrics')
        return metrics_folder

    def _generate_iterations(self, extra_args):
        """Generate iterations."""
        iterations = 1
        if extra_args.get(constants.PRE_PATCH_ITERATIONS):
            iterations = extra_args.pop(constants.PRE_PATCH_ITERATIONS)
        elif extra_args.get(constants.POST_PATCH_ITERATIONS):
            iterations = extra_args.pop(constants.POST_PATCH_ITERATIONS)
        return iterations

    def generate_run_commands(self, test_infos, extra_args, port=None):
        """Generate a single run command from TestInfos.

        Args:
            test_infos: A set of TestInfo instances.
            extra_args: A Dict of extra args to append.
            port: Optional. An int of the port number to send events to. If
                  None, then subprocess reporter in TF won't try to connect.

        Returns:
            A list that contains the string of atest tradefed run command.
            Only one command is returned.
        """
        args = self._create_test_args(test_infos)
        metrics_folder = self._generate_metrics_folder(extra_args)

        # Create a copy of args as more args could be added to the list.
        test_args = list(args)
        if port:
            test_args.extend(['--subprocess-report-port', str(port)])
        if metrics_folder:
            test_args.extend(['--metrics-folder', metrics_folder])
            logging.info('Saved metrics in: %s', metrics_folder)
        if extra_args.get(constants.INVOCATION_ID, None):
            test_args.append('--invocation-data invocation_id=%s'
                             % extra_args[constants.INVOCATION_ID])
        if extra_args.get(constants.WORKUNIT_ID, None):
            test_args.append('--invocation-data work_unit_id=%s'
                             % extra_args[constants.WORKUNIT_ID])
        # For detailed logs, set TF options log-level/log-level-display as
        # 'VERBOSE' by default.
        log_level = 'VERBOSE'
        test_args.extend(['--log-level-display', log_level])
        test_args.extend(['--log-level', log_level])
        # Set no-early-device-release by default to speed up TF teardown time.
        if not constants.TF_EARLY_DEVICE_RELEASE in extra_args:
            test_args.extend(['--no-early-device-release'])

        args_to_add, args_not_supported = self._parse_extra_args(test_infos, extra_args)

        # TODO(b/122889707) Remove this after finding the root cause.
        env_serial = os.environ.get(constants.ANDROID_SERIAL)
        # Use the env variable ANDROID_SERIAL if it's set by user but only when
        # the target tests are not deviceless tests.
        if env_serial and '--serial' not in args_to_add and '-n' not in args_to_add:
            args_to_add.append("--serial")
            args_to_add.append(env_serial)

        test_args.extend(args_to_add)
        if args_not_supported:
            logging.info('%s does not support the following args %s',
                         self.EXECUTABLE, args_not_supported)

        # Only need to check one TestInfo to determine if the tests are
        # configured in TEST_MAPPING.
        for_test_mapping = test_infos and test_infos[0].from_test_mapping
        test_args.extend(atest_utils.get_result_server_args(for_test_mapping))
        self.run_cmd_dict['args'] = ' '.join(test_args)
        self.run_cmd_dict['tf_customize_template'] = (
            self._extract_customize_tf_templates(extra_args))
        return [self._RUN_CMD.format(**self.run_cmd_dict)]

    def _flatten_test_infos(self, test_infos):
        """Sort and group test_infos by module_name and sort and group filters
        by class name.

            Example of three test_infos in a set:
                Module1, {(classA, {})}
                Module1, {(classB, {Method1})}
                Module1, {(classB, {Method2}}
            Becomes a set with one element:
                Module1, {(ClassA, {}), (ClassB, {Method1, Method2})}
            Where:
                  Each line is a test_info namedtuple
                  {} = Frozenset
                  () = TestFilter namedtuple

        Args:
            test_infos: A set of TestInfo namedtuples.

        Returns:
            A set of TestInfos flattened.
        """
        results = set()
        key = lambda x: x.test_name
        for module, group in atest_utils.sort_and_group(test_infos, key):
            # module is a string, group is a generator of grouped TestInfos.
            # Module Test, so flatten test_infos:
            no_filters = False
            filters = set()
            test_runner = None
            test_finder = None
            build_targets = set()
            data = {}
            module_args = []
            for test_info_i in group:
                data.update(test_info_i.data)
                # Extend data with constants.TI_MODULE_ARG instead of
                # overwriting.
                module_args.extend(test_info_i.data.get(
                    constants.TI_MODULE_ARG, []))
                test_runner = test_info_i.test_runner
                test_finder = test_info_i.test_finder
                build_targets |= test_info_i.build_targets
                test_filters = test_info_i.data.get(constants.TI_FILTER)
                if not test_filters or no_filters:
                    # test_info wants whole module run, so hardcode no filters.
                    no_filters = True
                    filters = set()
                    continue
                filters |= test_filters
            if module_args:
                data[constants.TI_MODULE_ARG] = module_args
            data[constants.TI_FILTER] = self._flatten_test_filters(filters)
            results.add(
                test_info.TestInfo(test_name=module,
                                   test_runner=test_runner,
                                   test_finder=test_finder,
                                   build_targets=build_targets,
                                   data=data))
        return results

    @staticmethod
    def _flatten_test_filters(filters):
        """Sort and group test_filters by class_name.

            Example of three test_filters in a frozenset:
                classA, {}
                classB, {Method1}
                classB, {Method2}
            Becomes a frozenset with these elements:
                classA, {}
                classB, {Method1, Method2}
            Where:
                Each line is a TestFilter namedtuple
                {} = Frozenset

        Args:
            filters: A frozenset of test_filters.

        Returns:
            A frozenset of test_filters flattened.
        """
        results = set()
        key = lambda x: x.class_name
        for class_name, group in atest_utils.sort_and_group(filters, key):
            # class_name is a string, group is a generator of TestFilters
            assert class_name is not None
            methods = set()
            for test_filter in group:
                if not test_filter.methods:
                    # Whole class should be run
                    methods = set()
                    break
                methods |= test_filter.methods
            results.add(test_info.TestFilter(class_name, frozenset(methods)))
        return frozenset(results)

    def _create_test_args(self, test_infos):
        """Compile TF command line args based on the given test infos.

        Args:
            test_infos: A set of TestInfo instances.

        Returns: A list of TF arguments to run the tests.
        """
        args = []
        if not test_infos:
            return []

        test_infos = self._flatten_test_infos(test_infos)
        has_integration_test = False
        for info in test_infos:
            # Integration test exists in TF's jar, so it must have the option
            # if it's integration finder.
            if info.test_finder in _INTEGRATION_FINDERS:
                has_integration_test = True
            # For non-paramertize test module, use --include-filter, but for
            # tests which have auto enable paramertize config use --module
            # instead.
            if self._is_parameter_auto_enabled_cfg(info, self.module_info):
                args.extend([constants.TF_MODULE_FILTER, info.test_name])
            else:
                args.extend([constants.TF_INCLUDE_FILTER, info.test_name])
            filters = set()
            for test_filter in info.data.get(constants.TI_FILTER, []):
                filters.update(test_filter.to_set_of_tf_strings())
            for test_filter in filters:
                filter_arg = constants.TF_ATEST_INCLUDE_FILTER_VALUE_FMT.format(
                    test_name=info.test_name, test_filter=test_filter)
                args.extend([constants.TF_ATEST_INCLUDE_FILTER, filter_arg])
            for option in info.data.get(constants.TI_MODULE_ARG, []):
                if constants.TF_INCLUDE_FILTER_OPTION == option[0]:
                    suite_filter = (
                        constants.TF_SUITE_FILTER_ARG_VALUE_FMT.format(
                            test_name=info.test_name, option_value=option[1]))
                    args.extend([constants.TF_INCLUDE_FILTER, suite_filter])
                elif constants.TF_EXCLUDE_FILTER_OPTION == option[0]:
                    suite_filter = (
                        constants.TF_SUITE_FILTER_ARG_VALUE_FMT.format(
                            test_name=info.test_name, option_value=option[1]))
                    args.extend([constants.TF_EXCLUDE_FILTER, suite_filter])
                else:
                    module_arg = (
                        constants.TF_MODULE_ARG_VALUE_FMT.format(
                            test_name=info.test_name, option_name=option[0],
                            option_value=option[1]))
                    args.extend([constants.TF_MODULE_ARG, module_arg])
        # TODO (b/141090547) Pass the config path to TF to load configs.
        # Compile option in TF if finder is not INTEGRATION or not set.
        if not has_integration_test:
            args.append(constants.TF_SKIP_LOADING_CONFIG_JAR)
        return args

    def _extract_rerun_options(self, extra_args):
        """Extract rerun options to a string for output.

        Args:
            extra_args: Dict of extra args for test runners to use.

        Returns: A string of rerun options.
        """
        extracted_options = ['{} {}'.format(arg, extra_args[arg])
                             for arg in extra_args
                             if arg in self._RERUN_OPTION_GROUP]
        return ' '.join(extracted_options)

    def _extract_customize_tf_templates(self, extra_args):
        """Extract tradefed template options to a string for output.

        Args:
            extra_args: Dict of extra args for test runners to use.

        Returns: A string of tradefed template options.
        """
        return ' '.join(['--template:map %s'
                         % x for x in extra_args.get(constants.TF_TEMPLATE, [])])

    def _handle_log_associations(self, event_handlers):
        """Handle TF's log associations information data.

        log_association dict:
        {'loggedFile': '/tmp/serial-util11375755456514097276.ser',
         'dataName': 'device_logcat_setup_127.0.0.1:58331',
         'time': 1602038599.856113},

        Args:
            event_handlers: Dict of {socket_object:EventHandler}.

        """
        log_associations = []
        for _, event_handler in event_handlers.items():
            if event_handler.log_associations:
                log_associations += event_handler.log_associations
        device_test_end_log_time = ''
        device_teardown_log_time = ''
        for log_association in log_associations:
            if 'device_logcat_test' in log_association.get('dataName', ''):
                device_test_end_log_time = log_association.get('time')
            if 'device_logcat_teardown' in log_association.get('dataName', ''):
                device_teardown_log_time = log_association.get('time')
        if device_test_end_log_time and device_teardown_log_time:
            teardowntime = (float(device_teardown_log_time) -
                            float(device_test_end_log_time))
            logging.debug('TF logcat teardown time=%s seconds.', teardowntime)
            metrics.LocalDetectEvent(
                detect_type=constants.DETECT_TYPE_TF_TEARDOWN_LOGCAT,
                result=int(teardowntime))

    @staticmethod
    def _has_instant_app_config(test_infos, mod_info):
        """Check if one of the input tests defined instant app mode in config.

        Args:
            test_infos: A set of TestInfo instances.
            mod_info: ModuleInfo object.

        Returns: True if one of the tests set up instant app mode.
        """
        for tinfo in test_infos:
            test_config, _ = test_finder_utils.get_test_config_and_srcs(
                tinfo, mod_info)
            if test_config:
                parameters = atest_utils.get_config_parameter(test_config)
                if constants.TF_PARA_INSTANT_APP in parameters:
                    return True
        return False

    @staticmethod
    def _is_parameter_auto_enabled_cfg(tinfo, mod_info):
        """Check if input tests contains auto enable support parameters.

        Args:
            test_infos: A set of TestInfo instances.
            mod_info: ModuleInfo object.

        Returns: True if input test has parameter setting which is not in the
                 exclude list.
        """
        test_config, _ = test_finder_utils.get_test_config_and_srcs(
            tinfo, mod_info)
        if test_config:
            parameters = atest_utils.get_config_parameter(test_config)
            if (parameters - constants.DEFAULT_EXCLUDE_PARAS
                - constants.DEFAULT_EXCLUDE_NOT_PARAS):
                return True
        return False

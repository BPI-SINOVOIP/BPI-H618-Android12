#!/usr/bin/env python3

import pipes
import re
import subprocess
import sys
import unittest

BITNESS_32 = ("", "32")
BITNESS_64 = ("64", "64")

APP_PROCESS_FOR_PRETTY_BITNESS = 'app_process%s'
NATIVE_TEST_SERVICE_FOR_BITNESS = ' /data/nativetest%s/aidl_test_service/aidl_test_service%s'
CPP_TEST_CLIENT_FOR_BITNESS = ' /data/nativetest%s/aidl_test_client/aidl_test_client%s'
NDK_TEST_CLIENT_FOR_BITNESS = ' /data/nativetest%s/aidl_test_client_ndk/aidl_test_client_ndk%s'
RUST_TEST_CLIENT_FOR_BITNESS = ' /data/nativetest%s/aidl_test_rust_client/aidl_test_rust_client%s'
RUST_TEST_SERVICE_FOR_BITNESS = ' /data/nativetest%s/aidl_test_rust_service/aidl_test_rust_service%s'

# From AidlTestsJava.java
INSTRUMENTATION_SUCCESS_PATTERN = r'TEST SUCCESS\n$'

class TestFail(Exception):
    """Raised on test failures."""
    pass

def pretty_bitness(bitness):
    """Returns a human readable version of bitness, corresponding to BITNESS_* variable"""
    return bitness[-1]

class ShellResult(object):
    """Represents the result of running a shell command."""

    def __init__(self, exit_status, stdout, stderr):
        """Construct an instance.

        Args:
            exit_status: integer exit code of shell command
            stdout: string stdout of shell command
            stderr: string stderr of shell command
        """
        self.stdout = stdout
        self.stderr = stderr
        self.exit_status = exit_status

    def printable_string(self):
        """Get a string we could print to the logs and understand."""
        output = []
        output.append('stdout:')
        for line in self.stdout.splitlines():
            output.append('  > %s' % line)
        output.append('stderr:')
        for line in self.stderr.splitlines():
            output.append('  > %s' % line)
        return '\n'.join(output)


class AdbHost(object):
    """Represents a device connected via ADB."""

    def run(self, command, background=False, ignore_status=False):
        """Run a command on the device via adb shell.

        Args:
            command: string containing a shell command to run.
            background: True iff we should run this command in the background.
            ignore_status: True iff we should ignore the command's exit code.

        Returns:
            instance of ShellResult.

        Raises:
            subprocess.CalledProcessError on command exit != 0.
        """
        if background:
            command = '( %s ) </dev/null >/dev/null 2>&1 &' % command
        return self.adb('shell %s' % pipes.quote(command),
                        ignore_status=ignore_status)

    def adb(self, command, ignore_status=False):
        """Run an ADB command (e.g. `adb sync`).

        Args:
            command: string containing command to run
            ignore_status: True iff we should ignore the command's exit code.

        Returns:
            instance of ShellResult.

        Raises:
            subprocess.CalledProcessError on command exit != 0.
        """
        command = 'adb %s' % command
        p = subprocess.Popen(command, shell=True, close_fds=True,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             universal_newlines=True)
        stdout, stderr = p.communicate()
        if not ignore_status and p.returncode:
            raise subprocess.CalledProcessError(p.returncode, command)
        return ShellResult(p.returncode, stdout, stderr)

class NativeServer:
    def __init__(self, host, bitness):
        self.name = "%s_bit_native_server" % pretty_bitness(bitness)
        self.host = host
        self.binary = NATIVE_TEST_SERVICE_FOR_BITNESS % bitness
    def cleanup(self):
        self.host.run('killall %s' % self.binary, ignore_status=True)
    def run(self):
        return self.host.run(self.binary, background=True)

class NativeClient:
    def cleanup(self):
        self.host.run('killall %s' % self.binary, ignore_status=True)
    def run(self):
        result = self.host.run(self.binary + ' --gtest_color=yes', ignore_status=True)
        print(result.printable_string())
        if result.exit_status:
            raise TestFail(result.stdout)

class CppClient(NativeClient):
    def __init__(self, host, bitness):
        self.name = "%s_bit_cpp_client" % pretty_bitness(bitness)
        self.host = host
        self.binary = CPP_TEST_CLIENT_FOR_BITNESS % bitness

class NdkClient(NativeClient):
    def __init__(self, host, bitness):
        self.name = "%s_bit_ndk_client" % pretty_bitness(bitness)
        self.host = host
        self.binary = NDK_TEST_CLIENT_FOR_BITNESS % bitness

class JavaServer:
    def __init__(self, host, bitness):
        self.name = "java_server_%s" % pretty_bitness(bitness)
        self.host = host
        self.bitness = bitness
    def cleanup(self):
        self.host.run('killall ' + APP_PROCESS_FOR_PRETTY_BITNESS % pretty_bitness(self.bitness),
                      ignore_status=True)
    def run(self):
        return self.host.run('CLASSPATH=/data/framework/aidl_test_java_service.jar '
                             + APP_PROCESS_FOR_PRETTY_BITNESS % pretty_bitness(self.bitness) +
                             ' /data/framework android.aidl.service.TestServiceServer',
                             background=True)

class JavaClient:
    def __init__(self, host, bitness):
        self.name = "java_client_%s" % pretty_bitness(bitness)
        self.host = host
        self.bitness = bitness
    def cleanup(self):
        self.host.run('killall ' + APP_PROCESS_FOR_PRETTY_BITNESS % pretty_bitness(self.bitness),
                      ignore_status=True)
    def run(self):
        result = self.host.run('CLASSPATH=/data/framework/aidl_test_java_client.jar '
                               + APP_PROCESS_FOR_PRETTY_BITNESS % pretty_bitness(self.bitness) +
                               ' /data/framework android.aidl.tests.AidlJavaTests')
        print(result.printable_string())
        if re.search(INSTRUMENTATION_SUCCESS_PATTERN, result.stdout) is None:
            raise TestFail(result.stdout)

def getprop(host, prop):
    return host.run('getprop "%s"' % prop).stdout.strip()

class RustClient:
    def __init__(self, host, bitness):
        self.name = "%s_bit_rust_client" % pretty_bitness(bitness)
        self.host = host
        self.binary = RUST_TEST_CLIENT_FOR_BITNESS % bitness
    def cleanup(self):
        self.host.run('killall %s' % self.binary, ignore_status=True)
    def run(self):
        result = self.host.run(self.binary, ignore_status=True)
        print(result.printable_string())
        if result.exit_status:
            raise TestFail(result.stdout)

class RustServer:
    def __init__(self, host, bitness):
        self.name = "%s_bit_rust_server" % pretty_bitness(bitness)
        self.host = host
        self.binary = RUST_TEST_SERVICE_FOR_BITNESS % bitness
    def cleanup(self):
        self.host.run('killall %s' % self.binary, ignore_status=True)
    def run(self):
        return self.host.run(self.binary, background=True)

def supported_bitnesses(host):
    bitnesses = []
    if getprop(host, "ro.product.cpu.abilist32") != "":
        bitnesses += [BITNESS_32]
    if getprop(host, "ro.product.cpu.abilist64") != "":
        bitnesses += [BITNESS_64]
    return bitnesses

# tests added dynamically below
class TestAidl(unittest.TestCase):
    pass

def make_test(client, server):
    def test(self):
        try:
            client.cleanup()
            server.cleanup()
            server.run()
            client.run()
        finally:
            client.cleanup()
            server.cleanup()
    return test

if __name__ == '__main__':
    host = AdbHost()
    bitnesses = supported_bitnesses(host)
    if len(bitnesses) == 0:
        print("No clients installed")
        exit(1)

    clients = []
    servers = []

    for bitness in bitnesses:
        clients += [NdkClient(host, bitness)]

        clients += [CppClient(host, bitness)]
        servers += [NativeServer(host, bitness)]

        clients += [JavaClient(host, bitness)]
        servers += [JavaServer(host, bitness)]

        clients += [RustClient(host, bitness)]
        servers += [RustServer(host, bitness)]

    for client in clients:
        for server in servers:
            test_name = 'test_%s_to_%s' % (client.name, server.name)
            test = make_test(client, server)
            setattr(TestAidl, test_name, test)

    suite = unittest.TestLoader().loadTestsFromTestCase(TestAidl)
    sys.exit(not unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful())

#!/usr/bin/env python3
#
#   Copyright 2020 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import time
import re
import os
import math
import shutil
import fnmatch
import posixpath
import tempfile
from collections import namedtuple

from acts import utils
from acts import asserts
from acts import signals
from acts.libs.proc import job
from acts.controllers.android_device import list_adb_devices
from acts.controllers.android_device import list_fastboot_devices
from acts.controllers.android_device import DEFAULT_QXDM_LOG_PATH
from acts.controllers.android_device import SL4A_APK_NAME
from acts_contrib.test_utils.wifi import wifi_test_utils as wutils
from acts_contrib.test_utils.tel import tel_test_utils as tutils
from acts_contrib.test_utils.instrumentation.device.command.instrumentation_command_builder import InstrumentationCommandBuilder
from acts_contrib.test_utils.instrumentation.device.command.instrumentation_command_builder import InstrumentationTestCommandBuilder
from acts.utils import get_current_epoch_time
from acts.utils import epoch_to_human_time

WifiEnums = wutils.WifiEnums
PULL_TIMEOUT = 300
GNSSSTATUS_LOG_PATH = (
    "/storage/emulated/0/Android/data/com.android.gpstool/files/")
QXDM_MASKS = ["GPS.cfg", "GPS-general.cfg", "default.cfg"]
TTFF_REPORT = namedtuple(
    "TTFF_REPORT", "utc_time ttff_loop ttff_sec ttff_pe ttff_ant_cn "
                   "ttff_base_cn")
TRACK_REPORT = namedtuple(
    "TRACK_REPORT", "l5flag pe ant_top4cn ant_cn base_top4cn base_cn")
LOCAL_PROP_FILE_CONTENTS = """\
log.tag.LocationManagerService=VERBOSE
log.tag.GnssLocationProvider=VERBOSE
log.tag.GnssMeasurementsProvider=VERBOSE
log.tag.GpsNetInitiatedHandler=VERBOSE
log.tag.GnssNetInitiatedHandler=VERBOSE
log.tag.GnssNetworkConnectivityHandler=VERBOSE
log.tag.ConnectivityService=VERBOSE
log.tag.ConnectivityManager=VERBOSE
log.tag.GnssVisibilityControl=VERBOSE
log.tag.NtpTimeHelper=VERBOSE
log.tag.NtpTrustedTime=VERBOSE
log.tag.GnssPsdsDownloader=VERBOSE
log.tag.Gnss=VERBOSE
log.tag.GnssConfiguration=VERBOSE"""
TEST_PACKAGE_NAME = "com.google.android.apps.maps"
LOCATION_PERMISSIONS = [
    "android.permission.ACCESS_FINE_LOCATION",
    "android.permission.ACCESS_COARSE_LOCATION"
]
GNSSTOOL_PACKAGE_NAME = "com.android.gpstool"
GNSSTOOL_PERMISSIONS = [
    "android.permission.ACCESS_FINE_LOCATION",
    "android.permission.READ_EXTERNAL_STORAGE",
    "android.permission.ACCESS_COARSE_LOCATION",
    "android.permission.CALL_PHONE",
    "android.permission.WRITE_CONTACTS",
    "android.permission.CAMERA",
    "android.permission.WRITE_EXTERNAL_STORAGE",
    "android.permission.READ_CONTACTS",
    "android.permission.ACCESS_BACKGROUND_LOCATION"
]
DISABLE_LTO_FILE_CONTENTS = """\
LONGTERM_PSDS_SERVER_1="http://"
LONGTERM_PSDS_SERVER_2="http://"
LONGTERM_PSDS_SERVER_3="http://"
NORMAL_PSDS_SERVER="http://"
REALTIME_PSDS_SERVER="http://"
"""


class GnssTestUtilsError(Exception):
    pass


def remount_device(ad):
    """Remount device file system to read and write.

    Args:
        ad: An AndroidDevice object.
    """
    for retries in range(5):
        ad.root_adb()
        if ad.adb.getprop("ro.boot.veritymode") == "enforcing":
            ad.adb.disable_verity()
            reboot(ad)
        remount_result = ad.adb.remount()
        ad.log.info("Attempt %d - %s" % (retries + 1, remount_result))
        if "remount succeeded" in remount_result:
            break


def reboot(ad):
    """Reboot device and check if mobile data is available.

    Args:
        ad: An AndroidDevice object.
    """
    ad.log.info("Reboot device to make changes take effect.")
    ad.reboot()
    ad.unlock_screen(password=None)
    if not int(ad.adb.shell("settings get global mobile_data")) == 1:
        set_mobile_data(ad, True)
    utils.sync_device_time(ad)


def enable_gnss_verbose_logging(ad):
    """Enable GNSS VERBOSE Logging and persistent logcat.

    Args:
        ad: An AndroidDevice object.
    """
    remount_device(ad)
    ad.log.info("Enable GNSS VERBOSE Logging and persistent logcat.")
    if check_chipset_vendor_by_qualcomm(ad):
        ad.adb.shell("echo -e '\nDEBUG_LEVEL = 5' >> /vendor/etc/gps.conf")
    else:
        ad.adb.shell("echo LogEnabled=true >> /data/vendor/gps/libgps.conf")
        ad.adb.shell("chown gps.system /data/vendor/gps/libgps.conf")
    ad.adb.shell("echo %r >> /data/local.prop" % LOCAL_PROP_FILE_CONTENTS)
    ad.adb.shell("chmod 644 /data/local.prop")
    ad.adb.shell("setprop persist.logd.logpersistd.size 20000")
    ad.adb.shell("setprop persist.logd.size 16777216")
    ad.adb.shell("setprop persist.vendor.radio.adb_log_on 1")
    ad.adb.shell("setprop persist.logd.logpersistd logcatd")
    ad.adb.shell("setprop log.tag.copresGcore VERBOSE")
    ad.adb.shell("sync")


def get_am_flags(value):
    """Returns the (value, type) flags for a given python value."""
    if type(value) is bool:
        return str(value).lower(), 'boolean'
    elif type(value) is str:
        return value, 'string'
    raise ValueError("%s should be either 'boolean' or 'string'" % value)


def enable_compact_and_particle_fusion_log(ad):
    """Enable CompactLog, FLP particle fusion log and disable gms
    location-based quake monitoring.

    Args:
        ad: An AndroidDevice object.
    """
    ad.root_adb()
    ad.log.info("Enable FLP flags and Disable GMS location-based quake "
                "monitoring.")
    overrides = {
        'compact_log_enabled': True,
        'flp_use_particle_fusion': True,
        'flp_particle_fusion_extended_bug_report': True,
        'flp_event_log_size': '86400',
        'proks_config': '28',
        'flp_particle_fusion_bug_report_window_sec': '86400',
        'flp_particle_fusion_bug_report_max_buffer_size': '86400',
        'seismic_data_collection': False,
        'Ealert__enable': False,
    }
    for flag, python_value in overrides.items():
        value, type = get_am_flags(python_value)
        cmd = ("am broadcast -a com.google.android.gms.phenotype.FLAG_OVERRIDE "
               "--es package com.google.android.location --es user \* "
               "--esa flags %s --esa values %s --esa types %s "
               "com.google.android.gms" % (flag, value, type))
        ad.adb.shell(cmd)
    ad.adb.shell("am force-stop com.google.android.gms")
    ad.adb.shell("am broadcast -a com.google.android.gms.INITIALIZE")


def disable_xtra_throttle(ad):
    """Disable XTRA throttle will have no limit to download XTRA data.

    Args:
        ad: An AndroidDevice object.
    """
    remount_device(ad)
    ad.log.info("Disable XTRA Throttle.")
    ad.adb.shell("echo -e '\nXTRA_TEST_ENABLED=1' >> /vendor/etc/gps.conf")
    ad.adb.shell("echo -e '\nXTRA_THROTTLE_ENABLED=0' >> /vendor/etc/gps.conf")


def enable_supl_mode(ad):
    """Enable SUPL back on for next test item.

    Args:
        ad: An AndroidDevice object.
    """
    remount_device(ad)
    ad.log.info("Enable SUPL mode.")
    ad.adb.shell("echo -e '\nSUPL_MODE=1' >> /etc/gps_debug.conf")
    if not check_chipset_vendor_by_qualcomm(ad):
        lto_mode(ad, True)
    else:
        reboot(ad)


def disable_supl_mode(ad):
    """Kill SUPL to test XTRA/LTO only test item.

    Args:
        ad: An AndroidDevice object.
    """
    remount_device(ad)
    ad.log.info("Disable SUPL mode.")
    ad.adb.shell("echo -e '\nSUPL_MODE=0' >> /etc/gps_debug.conf")
    if not check_chipset_vendor_by_qualcomm(ad):
        lto_mode(ad, True)
    else:
        reboot(ad)


def kill_xtra_daemon(ad):
    """Kill XTRA daemon to test SUPL only test item.

    Args:
        ad: An AndroidDevice object.
    """
    ad.root_adb()
    if check_chipset_vendor_by_qualcomm(ad):
        ad.log.info("Disable XTRA-daemon until next reboot.")
        ad.adb.shell("killall xtra-daemon", ignore_status=True)
    else:
        lto_mode(ad, False)


def disable_private_dns_mode(ad):
    """Due to b/118365122, it's better to disable private DNS mode while
       testing. 8.8.8.8 private dns sever is unstable now, sometimes server
       will not response dns query suddenly.

    Args:
        ad: An AndroidDevice object.
    """
    tutils.get_operator_name(ad.log, ad, subId=None)
    if ad.adb.shell("settings get global private_dns_mode") != "off":
        ad.log.info("Disable Private DNS mode.")
        ad.adb.shell("settings put global private_dns_mode off")


def _init_device(ad):
    """Init GNSS test devices.

    Args:
        ad: An AndroidDevice object.
    """
    enable_gnss_verbose_logging(ad)
    enable_compact_and_particle_fusion_log(ad)
    if check_chipset_vendor_by_qualcomm(ad):
        disable_xtra_throttle(ad)
    enable_supl_mode(ad)
    ad.adb.shell("settings put system screen_off_timeout 1800000")
    wutils.wifi_toggle_state(ad, False)
    ad.log.info("Setting Bluetooth state to False")
    ad.droid.bluetoothToggleState(False)
    check_location_service(ad)
    set_wifi_and_bt_scanning(ad, True)
    disable_private_dns_mode(ad)
    reboot(ad)
    init_gtw_gpstool(ad)


def connect_to_wifi_network(ad, network):
    """Connection logic for open and psk wifi networks.

    Args:
        ad: An AndroidDevice object.
        network: Dictionary with network info.
    """
    SSID = network[WifiEnums.SSID_KEY]
    ad.ed.clear_all_events()
    wutils.reset_wifi(ad)
    wutils.start_wifi_connection_scan_and_ensure_network_found(ad, SSID)
    wutils.wifi_connect(ad, network, num_of_tries=5)


def set_wifi_and_bt_scanning(ad, state=True):
    """Set Wi-Fi and Bluetooth scanning on/off in Settings -> Location

    Args:
        ad: An AndroidDevice object.
        state: True to turn on "Wi-Fi and Bluetooth scanning".
            False to turn off "Wi-Fi and Bluetooth scanning".
    """
    ad.root_adb()
    if state:
        ad.adb.shell("settings put global wifi_scan_always_enabled 1")
        ad.adb.shell("settings put global ble_scan_always_enabled 1")
        ad.log.info("Wi-Fi and Bluetooth scanning are enabled")
    else:
        ad.adb.shell("settings put global wifi_scan_always_enabled 0")
        ad.adb.shell("settings put global ble_scan_always_enabled 0")
        ad.log.info("Wi-Fi and Bluetooth scanning are disabled")


def check_location_service(ad):
    """Set location service on.
       Verify if location service is available.

    Args:
        ad: An AndroidDevice object.
    """
    remount_device(ad)
    utils.set_location_service(ad, True)
    location_mode = int(ad.adb.shell("settings get secure location_mode"))
    ad.log.info("Current Location Mode >> %d" % location_mode)
    if location_mode != 3:
        raise signals.TestError("Failed to turn Location on")


def clear_logd_gnss_qxdm_log(ad):
    """Clear /data/misc/logd,
    /storage/emulated/0/Android/data/com.android.gpstool/files and
    /data/vendor/radio/diag_logs/logs from previous test item then reboot.

    Args:
        ad: An AndroidDevice object.
    """
    remount_device(ad)
    ad.log.info("Clear Logd, GNSS and PixelLogger Log from previous test item.")
    ad.adb.shell("rm -rf /data/misc/logd", ignore_status=True)
    ad.adb.shell(
        'find %s -name "*.txt" -type f -delete' % GNSSSTATUS_LOG_PATH,
        ignore_status=True)
    if check_chipset_vendor_by_qualcomm(ad):
        diag_logs = (
            "/sdcard/Android/data/com.android.pixellogger/files/logs/diag_logs")
        ad.adb.shell("rm -rf %s" % diag_logs, ignore_status=True)
        output_path = posixpath.join(DEFAULT_QXDM_LOG_PATH, "logs")
    else:
        output_path = ("/sdcard/Android/data/com.android.pixellogger/files"
                       "/logs/gps/")
    ad.adb.shell("rm -rf %s" % output_path, ignore_status=True)
    reboot(ad)


def get_gnss_qxdm_log(ad, qdb_path):
    """Get /storage/emulated/0/Android/data/com.android.gpstool/files and
    /data/vendor/radio/diag_logs/logs for test item.

    Args:
        ad: An AndroidDevice object.
        qdb_path: The path of qdsp6m.qdb on different projects.
    """
    log_path = ad.device_log_path
    os.makedirs(log_path, exist_ok=True)
    gnss_log_name = "gnssstatus_log_%s_%s" % (ad.model, ad.serial)
    gnss_log_path = posixpath.join(log_path, gnss_log_name)
    os.makedirs(gnss_log_path, exist_ok=True)
    ad.log.info("Pull GnssStatus Log to %s" % gnss_log_path)
    ad.adb.pull("%s %s" % (GNSSSTATUS_LOG_PATH+".", gnss_log_path),
                timeout=PULL_TIMEOUT, ignore_status=True)
    shutil.make_archive(gnss_log_path, "zip", gnss_log_path)
    shutil.rmtree(gnss_log_path)
    if check_chipset_vendor_by_qualcomm(ad):
        output_path = (
            "/sdcard/Android/data/com.android.pixellogger/files/logs/diag_logs")
    else:
        output_path = (
            "/sdcard/Android/data/com.android.pixellogger/files/logs/gps/")
    qxdm_log_name = "PixelLogger_%s_%s" % (ad.model, ad.serial)
    qxdm_log_path = posixpath.join(log_path, qxdm_log_name)
    os.makedirs(qxdm_log_path, exist_ok=True)
    ad.log.info("Pull PixelLogger Log %s to %s" % (output_path,
                                                   qxdm_log_path))
    ad.adb.pull("%s %s" % (output_path, qxdm_log_path),
                timeout=PULL_TIMEOUT, ignore_status=True)
    if check_chipset_vendor_by_qualcomm(ad):
        for path in qdb_path:
            output = ad.adb.pull("%s %s" % (path, qxdm_log_path),
                                 timeout=PULL_TIMEOUT, ignore_status=True)
            if "No such file or directory" in output:
                continue
            break
    shutil.make_archive(qxdm_log_path, "zip", qxdm_log_path)
    shutil.rmtree(qxdm_log_path)


def set_mobile_data(ad, state):
    """Set mobile data on or off and check mobile data state.

    Args:
        ad: An AndroidDevice object.
        state: True to enable mobile data. False to disable mobile data.
    """
    ad.root_adb()
    if state:
        ad.log.info("Enable mobile data.")
        ad.adb.shell("svc data enable")
    else:
        ad.log.info("Disable mobile data.")
        ad.adb.shell("svc data disable")
    time.sleep(5)
    out = int(ad.adb.shell("settings get global mobile_data"))
    if state and out == 1:
        ad.log.info("Mobile data is enabled and set to %d" % out)
    elif not state and out == 0:
        ad.log.info("Mobile data is disabled and set to %d" % out)
    else:
        ad.log.error("Mobile data is at unknown state and set to %d" % out)


def gnss_trigger_modem_ssr_by_adb(ad, dwelltime=60):
    """Trigger modem SSR crash by adb and verify if modem crash and recover
    successfully.

    Args:
        ad: An AndroidDevice object.
        dwelltime: Waiting time for modem reset. Default is 60 seconds.

    Returns:
        True if success.
        False if failed.
    """
    begin_time = get_current_epoch_time()
    ad.root_adb()
    cmds = ("echo restart > /sys/kernel/debug/msm_subsys/modem",
            r"echo 'at+cfun=1,1\r' > /dev/at_mdm0")
    for cmd in cmds:
        ad.log.info("Triggering modem SSR crash by %s" % cmd)
        output = ad.adb.shell(cmd, ignore_status=True)
        if "No such file or directory" in output:
            continue
        break
    time.sleep(dwelltime)
    ad.send_keycode("HOME")
    logcat_results = ad.search_logcat("SSRObserver", begin_time)
    if logcat_results:
        for ssr in logcat_results:
            if "mSubsystem='modem', mCrashReason" in ssr["log_message"]:
                ad.log.debug(ssr["log_message"])
                ad.log.info("Triggering modem SSR crash successfully.")
                return True
        raise signals.TestError("Failed to trigger modem SSR crash")
    raise signals.TestError("No SSRObserver found in logcat")


def gnss_trigger_modem_ssr_by_mds(ad, dwelltime=60):
    """Trigger modem SSR crash by mds tool and verify if modem crash and recover
    successfully.

    Args:
        ad: An AndroidDevice object.
        dwelltime: Waiting time for modem reset. Default is 60 seconds.
    """
    mds_check = ad.adb.shell("pm path com.google.mdstest")
    if not mds_check:
        raise signals.TestError("MDS Tool is not properly installed.")
    ad.root_adb()
    cmd = ('am instrument -w -e request "4b 25 03 00" '
           '"com.google.mdstest/com.google.mdstest.instrument'
           '.ModemCommandInstrumentation"')
    ad.log.info("Triggering modem SSR crash by MDS")
    output = ad.adb.shell(cmd, ignore_status=True)
    ad.log.debug(output)
    time.sleep(dwelltime)
    ad.send_keycode("HOME")
    if "SUCCESS" in output:
        ad.log.info("Triggering modem SSR crash by MDS successfully.")
    else:
        raise signals.TestError(
            "Failed to trigger modem SSR crash by MDS. \n%s" % output)


def check_xtra_download(ad, begin_time):
    """Verify XTRA download success log message in logcat.

    Args:
        ad: An AndroidDevice object.
        begin_time: test begin time

    Returns:
        True: xtra_download if XTRA downloaded and injected successfully
        otherwise return False.
    """
    ad.send_keycode("HOME")
    if check_chipset_vendor_by_qualcomm(ad):
        xtra_results = ad.search_logcat("XTRA download success. "
                                        "inject data into modem", begin_time)
        if xtra_results:
            ad.log.debug("%s" % xtra_results[-1]["log_message"])
            ad.log.info("XTRA downloaded and injected successfully.")
            return True
        ad.log.error("XTRA downloaded FAIL.")
    else:
        lto_results = ad.search_logcat("GnssPsdsAidl: injectPsdsData: "
                                       "psdsType: 1", begin_time)
        if lto_results:
            ad.log.debug("%s" % lto_results[-1]["log_message"])
            ad.log.info("LTO downloaded and injected successfully.")
            return True
        ad.log.error("LTO downloaded and inject FAIL.")
    return False


def pull_package_apk(ad, package_name):
    """Pull apk of given package_name from device.

    Args:
        ad: An AndroidDevice object.
        package_name: Package name of apk to pull.

    Returns:
        The temp path of pulled apk.
    """
    apk_path = None
    out = ad.adb.shell("pm path %s" % package_name)
    result = re.search(r"package:(.*)", out)
    if not result:
        tutils.abort_all_tests(ad.log, "Couldn't find apk of %s" % package_name)
    else:
        apk_source = result.group(1)
        ad.log.info("Get apk of %s from %s" % (package_name, apk_source))
        apk_path = tempfile.mkdtemp()
        ad.pull_files([apk_source], apk_path)
    return apk_path


def pull_gnss_cfg_file(ad, file):
    """Pull given gnss cfg file from device.

    Args:
        ad: An AndroidDevice object.
        file: CFG file in device to pull.

    Returns:
        The temp path of pulled gnss cfg file in host.
    """
    ad.root_adb()
    host_dest = tempfile.mkdtemp()
    ad.pull_files(file, host_dest)
    for path_key in os.listdir(host_dest):
        if fnmatch.fnmatch(path_key, "*.cfg"):
            gnss_cfg_file = os.path.join(host_dest, path_key)
            break
    else:
        raise signals.TestError("No cfg file is found in %s" % host_dest)
    return gnss_cfg_file


def reinstall_package_apk(ad, package_name, apk_path):
    """Reinstall apk of given package_name.

    Args:
        ad: An AndroidDevice object.
        package_name: Package name of apk.
        apk_path: The temp path of pulled apk.
    """
    for path_key in os.listdir(apk_path):
        if fnmatch.fnmatch(path_key, "*.apk"):
            apk_path = os.path.join(apk_path, path_key)
            break
    else:
        raise signals.TestError("No apk is found in %s" % apk_path)
    ad.log.info("Re-install %s with path: %s" % (package_name, apk_path))
    ad.adb.shell("settings put global verifier_verify_adb_installs 0")
    ad.adb.install("-r -d -g --user 0 %s" % apk_path)
    package_check = ad.adb.shell("pm path %s" % package_name)
    if not package_check:
        tutils.abort_all_tests(
            ad.log, "%s is not properly re-installed." % package_name)
    ad.log.info("%s is re-installed successfully." % package_name)


def init_gtw_gpstool(ad):
    """Init GTW_GPSTool apk.

    Args:
        ad: An AndroidDevice object.
    """
    remount_device(ad)
    gpstool_path = pull_package_apk(ad, "com.android.gpstool")
    reinstall_package_apk(ad, "com.android.gpstool", gpstool_path)


def fastboot_factory_reset(ad):
    """Factory reset the device in fastboot mode.
       Pull sl4a apk from device. Terminate all sl4a sessions,
       Reboot the device to bootloader,
       factory reset the device by fastboot.
       Reboot the device. wait for device to complete booting
       Re-install and start an sl4a session.

    Args:
        ad: An AndroidDevice object.

    Returns:
        True if factory reset process complete.
    """
    status = True
    skip_setup_wizard = True
    gnss_cfg_path = "/vendor/etc/mdlog"
    default_gnss_cfg = "/vendor/etc/mdlog/DEFAULT+SECURITY+FULLDPL+GPS.cfg"
    sl4a_path = pull_package_apk(ad, SL4A_APK_NAME)
    gpstool_path = pull_package_apk(ad, "com.android.gpstool")
    mds_path = pull_package_apk(ad, "com.google.mdstest")
    if check_chipset_vendor_by_qualcomm(ad):
        gnss_cfg_file = pull_gnss_cfg_file(ad, default_gnss_cfg)
    stop_pixel_logger(ad)
    ad.stop_services()
    attempts = 3
    for i in range(1, attempts + 1):
        try:
            if ad.serial in list_adb_devices():
                ad.log.info("Reboot to bootloader")
                ad.adb.reboot("bootloader", ignore_status=True)
                time.sleep(10)
            if ad.serial in list_fastboot_devices():
                ad.log.info("Factory reset in fastboot")
                ad.fastboot._w(timeout=300, ignore_status=True)
                time.sleep(30)
                ad.log.info("Reboot in fastboot")
                ad.fastboot.reboot()
            ad.wait_for_boot_completion()
            ad.root_adb()
            if ad.skip_sl4a:
                break
            if ad.is_sl4a_installed():
                break
            reinstall_package_apk(ad, SL4A_APK_NAME, sl4a_path)
            reinstall_package_apk(ad, "com.android.gpstool", gpstool_path)
            reinstall_package_apk(ad, "com.google.mdstest", mds_path)
            if check_chipset_vendor_by_qualcomm(ad):
                ad.push_system_file(gnss_cfg_file, gnss_cfg_path)
            time.sleep(10)
            break
        except Exception as e:
            ad.log.error(e)
            if i == attempts:
                tutils.abort_all_tests(ad.log, str(e))
            time.sleep(5)
    try:
        ad.start_adb_logcat()
    except Exception as e:
        ad.log.error(e)
    if skip_setup_wizard:
        ad.exit_setup_wizard()
    if ad.skip_sl4a:
        return status
    tutils.bring_up_sl4a(ad)
    return status


def clear_aiding_data_by_gtw_gpstool(ad):
    """Launch GTW GPSTool and Clear all GNSS aiding data.
       Wait 5 seconds for GTW GPStool to clear all GNSS aiding
       data properly.

    Args:
        ad: An AndroidDevice object.
    """
    if not check_chipset_vendor_by_qualcomm(ad):
        delete_lto_file(ad)
    ad.log.info("Launch GTW GPSTool and Clear all GNSS aiding data")
    ad.adb.shell("am start -S -n com.android.gpstool/.GPSTool --es mode clear")
    time.sleep(10)


def start_gnss_by_gtw_gpstool(ad,
                              state,
                              type="gnss",
                              bgdisplay=False,
                              freq=0,
                              lowpower=False,
                              meas=False):
    """Start or stop GNSS on GTW_GPSTool.

    Args:
        ad: An AndroidDevice object.
        state: True to start GNSS. False to Stop GNSS.
        type: Different API for location fix. Use gnss/flp/nmea
        bgdisplay: true to run GTW when Display off. false to not run GTW when
          Display off.
        freq: An integer to set location update frequency.
        meas: A Boolean to set GNSS measurement registration.
        lowpower: A boolean to set GNSS LowPowerMode.
    """
    cmd = "am start -S -n com.android.gpstool/.GPSTool --es mode gps"
    if not state:
        ad.log.info("Stop %s on GTW_GPSTool." % type)
        cmd = "am broadcast -a com.android.gpstool.stop_gps_action"
    else:
        options = ("--es type {} --ei freq {} --ez BG {} --ez meas {} --ez "
                   "lowpower {}").format(type, freq, bgdisplay, meas, lowpower)
        cmd = cmd + " " + options
    ad.adb.shell(cmd)
    time.sleep(3)


def process_gnss_by_gtw_gpstool(ad,
                                criteria,
                                type="gnss",
                                clear_data=True,
                                meas_flag=False):
    """Launch GTW GPSTool and Clear all GNSS aiding data
       Start GNSS tracking on GTW_GPSTool.

    Args:
        ad: An AndroidDevice object.
        criteria: Criteria for current test item.
        type: Different API for location fix. Use gnss/flp/nmea
        clear_data: True to clear GNSS aiding data. False is not to. Default
        set to True.
        meas_flag: True to enable GnssMeasurement. False is not to. Default
        set to False.

    Returns:
        True: First fix TTFF are within criteria.
        False: First fix TTFF exceed criteria.
    """
    retries = 3
    for i in range(retries):
        if not ad.is_adb_logcat_on:
            ad.start_adb_logcat()
        check_adblog_functionality(ad)
        check_location_runtime_permissions(
            ad, GNSSTOOL_PACKAGE_NAME, GNSSTOOL_PERMISSIONS)
        begin_time = get_current_epoch_time()
        if clear_data:
            clear_aiding_data_by_gtw_gpstool(ad)
        ad.log.info("Start %s on GTW_GPSTool - attempt %d" % (type.upper(),
                                                              i+1))
        start_gnss_by_gtw_gpstool(ad, state=True, type=type, meas=meas_flag)
        for _ in range(10 + criteria):
            logcat_results = ad.search_logcat("First fixed", begin_time)
            if logcat_results:
                ad.log.debug(logcat_results[-1]["log_message"])
                first_fixed = int(logcat_results[-1]["log_message"].split()[-1])
                ad.log.info("%s First fixed = %.3f seconds" %
                            (type.upper(), first_fixed/1000))
                if (first_fixed/1000) <= criteria:
                    return True
                start_gnss_by_gtw_gpstool(ad, state=False, type=type)
                raise signals.TestFailure("Fail to get %s location fixed "
                                          "within %d seconds criteria."
                                          % (type.upper(), criteria))
            time.sleep(1)
        check_current_focus_app(ad)
        start_gnss_by_gtw_gpstool(ad, state=False, type=type)
    raise signals.TestFailure("Fail to get %s location fixed within %d "
                              "attempts." % (type.upper(), retries))

def start_ttff_by_gtw_gpstool(ad, ttff_mode, iteration, aid_data=False):
    """Identify which TTFF mode for different test items.

    Args:
        ad: An AndroidDevice object.
        ttff_mode: TTFF Test mode for current test item.
        iteration: Iteration of TTFF cycles.
        aid_data: Boolean for identify aid_data existed or not
    """
    begin_time = get_current_epoch_time()
    if (ttff_mode == "hs" or ttff_mode == "ws") and not aid_data:
        ad.log.info("Wait 5 minutes to start TTFF %s..." % ttff_mode.upper())
        time.sleep(300)
    if ttff_mode == "cs":
        ad.log.info("Start TTFF Cold Start...")
        time.sleep(3)
    for i in range(1, 4):
        ad.adb.shell("am broadcast -a com.android.gpstool.ttff_action "
                     "--es ttff %s --es cycle %d" % (ttff_mode, iteration))
        time.sleep(1)
        if ad.search_logcat("act=com.android.gpstool.start_test_action",
                            begin_time):
            ad.log.info("Send TTFF start_test_action successfully.")
            break
    else:
        check_current_focus_app(ad)
        raise signals.TestError("Fail to send TTFF start_test_action.")


def gnss_tracking_via_gtw_gpstool(ad,
                                  criteria,
                                  type="gnss",
                                  testtime=60,
                                  meas_flag=False):
    """Start GNSS/FLP tracking tests for input testtime on GTW_GPSTool.

    Args:
        ad: An AndroidDevice object.
        criteria: Criteria for current TTFF.
        type: Different API for location fix. Use gnss/flp/nmea
        testtime: Tracking test time for minutes. Default set to 60 minutes.
        meas_flag: True to enable GnssMeasurement. False is not to. Default
        set to False.
    """
    gnss_crash_list = [".*Fatal signal.*gnss",
                       ".*Fatal signal.*xtra",
                       ".*F DEBUG.*gnss"]
    process_gnss_by_gtw_gpstool(
        ad, criteria=criteria, type=type, meas_flag=meas_flag)
    ad.log.info("Start %s tracking test for %d minutes" % (type.upper(),
                                                           testtime))
    begin_time = get_current_epoch_time()
    while get_current_epoch_time() - begin_time < testtime * 60 * 1000:
        if not ad.is_adb_logcat_on:
            ad.start_adb_logcat()
        for attr in gnss_crash_list:
            gnss_crash_result = ad.adb.shell(
                "logcat -d | grep -E -i '%s'" % attr)
            if gnss_crash_result:
                start_gnss_by_gtw_gpstool(ad, state=False, type=type)
                raise signals.TestFailure(
                    "Test failed due to GNSS HAL crashed. \n%s" %
                    gnss_crash_result)
        gpstool_crash_result = ad.search_logcat("Force finishing activity "
                                                "com.android.gpstool/.GPSTool",
                                                begin_time)
        if gpstool_crash_result:
            raise signals.TestError("GPSTool crashed. Abort test.")
    ad.log.info("Successfully tested for %d minutes" % testtime)
    start_gnss_by_gtw_gpstool(ad, state=False, type=type)


def parse_gtw_gpstool_log(ad, true_position, type="gnss"):
    """Process GNSS/FLP API logs from GTW GPSTool and output track_data to
    test_run_info for ACTS plugin to parse and display on MobileHarness as
    Property.

    Args:
        ad: An AndroidDevice object.
        true_position: Coordinate as [latitude, longitude] to calculate
        position error.
        type: Different API for location fix. Use gnss/flp/nmea
    """
    test_logfile = {}
    track_data = {}
    ant_top4_cn = 0
    ant_cn = 0
    base_top4_cn = 0
    base_cn = 0
    track_lat = 0
    track_long = 0
    l5flag = "false"
    file_count = int(ad.adb.shell("find %s -type f -iname *.txt | wc -l"
                                  % GNSSSTATUS_LOG_PATH))
    if file_count != 1:
        ad.log.error("%d API logs exist." % file_count)
    dir_file = ad.adb.shell("ls %s" % GNSSSTATUS_LOG_PATH).split()
    for path_key in dir_file:
        if fnmatch.fnmatch(path_key, "*.txt"):
            logpath = posixpath.join(GNSSSTATUS_LOG_PATH, path_key)
            out = ad.adb.shell("wc -c %s" % logpath)
            file_size = int(out.split(" ")[0])
            if file_size < 2000:
                ad.log.info("Skip log %s due to log size %d bytes" %
                            (path_key, file_size))
                continue
            test_logfile = logpath
    if not test_logfile:
        raise signals.TestError("Failed to get test log file in device.")
    lines = ad.adb.shell("cat %s" % test_logfile).split("\n")
    for line in lines:
        if "Antenna_History Avg Top4" in line:
            ant_top4_cn = float(line.split(":")[-1].strip())
        elif "Antenna_History Avg" in line:
            ant_cn = float(line.split(":")[-1].strip())
        elif "Baseband_History Avg Top4" in line:
            base_top4_cn = float(line.split(":")[-1].strip())
        elif "Baseband_History Avg" in line:
            base_cn = float(line.split(":")[-1].strip())
        elif "L5 used in fix" in line:
            l5flag = line.split(":")[-1].strip()
        elif "Latitude" in line:
            track_lat = float(line.split(":")[-1].strip())
        elif "Longitude" in line:
            track_long = float(line.split(":")[-1].strip())
        elif "Time" in line:
            track_utc = line.split("Time:")[-1].strip()
            if track_utc in track_data.keys():
                continue
            pe = calculate_position_error(track_lat, track_long, true_position)
            track_data[track_utc] = TRACK_REPORT(l5flag=l5flag,
                                                 pe=pe,
                                                 ant_top4cn=ant_top4_cn,
                                                 ant_cn=ant_cn,
                                                 base_top4cn=base_top4_cn,
                                                 base_cn=base_cn)
    ad.log.debug(track_data)
    prop_basename = "TestResult %s_tracking_" % type.upper()
    time_list = sorted(track_data.keys())
    l5flag_list = [track_data[key].l5flag for key in time_list]
    pe_list = [float(track_data[key].pe) for key in time_list]
    ant_top4cn_list = [float(track_data[key].ant_top4cn) for key in time_list]
    ant_cn_list = [float(track_data[key].ant_cn) for key in time_list]
    base_top4cn_list = [float(track_data[key].base_top4cn) for key in time_list]
    base_cn_list = [float(track_data[key].base_cn) for key in time_list]
    ad.log.info(prop_basename+"StartTime %s" % time_list[0].replace(" ", "-"))
    ad.log.info(prop_basename+"EndTime %s" % time_list[-1].replace(" ", "-"))
    ad.log.info(prop_basename+"TotalFixPoints %d" % len(time_list))
    ad.log.info(prop_basename+"L5FixRate "+'{percent:.2%}'.format(
        percent=l5flag_list.count("true")/len(l5flag_list)))
    ad.log.info(prop_basename+"AvgDis %.1f" % (sum(pe_list)/len(pe_list)))
    ad.log.info(prop_basename+"MaxDis %.1f" % max(pe_list))
    ad.log.info(prop_basename+"Ant_AvgTop4Signal %.1f" % ant_top4cn_list[-1])
    ad.log.info(prop_basename+"Ant_AvgSignal %.1f" % ant_cn_list[-1])
    ad.log.info(prop_basename+"Base_AvgTop4Signal %.1f" % base_top4cn_list[-1])
    ad.log.info(prop_basename+"Base_AvgSignal %.1f" % base_cn_list[-1])


def process_ttff_by_gtw_gpstool(ad, begin_time, true_position, type="gnss"):
    """Process TTFF and record results in ttff_data.

    Args:
        ad: An AndroidDevice object.
        begin_time: test begin time.
        true_position: Coordinate as [latitude, longitude] to calculate
        position error.
        type: Different API for location fix. Use gnss/flp/nmea

    Returns:
        ttff_data: A dict of all TTFF data.
    """
    ttff_lat = 0
    ttff_lon = 0
    utc_time = epoch_to_human_time(get_current_epoch_time())
    ttff_data = {}
    ttff_loop_time = get_current_epoch_time()
    while True:
        if get_current_epoch_time() - ttff_loop_time >= 120000:
            raise signals.TestError("Fail to search specific GPSService "
                                    "message in logcat. Abort test.")
        if not ad.is_adb_logcat_on:
            ad.start_adb_logcat()
        logcat_results = ad.search_logcat("write TTFF log", ttff_loop_time)
        if logcat_results:
            ttff_loop_time = get_current_epoch_time()
            ttff_log = logcat_results[-1]["log_message"].split()
            ttff_loop = int(ttff_log[8].split(":")[-1])
            ttff_sec = float(ttff_log[11])
            if ttff_sec != 0.0:
                ttff_ant_cn = float(ttff_log[18].strip("]"))
                ttff_base_cn = float(ttff_log[25].strip("]"))
                if type == "gnss":
                    gnss_results = ad.search_logcat("GPSService: Check item",
                                                    begin_time)
                    if gnss_results:
                        ad.log.debug(gnss_results[-1]["log_message"])
                        gnss_location_log = \
                            gnss_results[-1]["log_message"].split()
                        ttff_lat = float(
                            gnss_location_log[8].split("=")[-1].strip(","))
                        ttff_lon = float(
                            gnss_location_log[9].split("=")[-1].strip(","))
                        loc_time = int(
                            gnss_location_log[10].split("=")[-1].strip(","))
                        utc_time = epoch_to_human_time(loc_time)
                elif type == "flp":
                    flp_results = ad.search_logcat("GPSService: FLP Location",
                                                   begin_time)
                    if flp_results:
                        ad.log.debug(flp_results[-1]["log_message"])
                        flp_location_log = flp_results[-1][
                            "log_message"].split()
                        ttff_lat = float(flp_location_log[8].split(",")[0])
                        ttff_lon = float(flp_location_log[8].split(",")[1])
                        utc_time = epoch_to_human_time(get_current_epoch_time())
            else:
                ttff_ant_cn = float(ttff_log[19].strip("]"))
                ttff_base_cn = float(ttff_log[26].strip("]"))
                ttff_lat = 0
                ttff_lon = 0
                utc_time = epoch_to_human_time(get_current_epoch_time())
            ad.log.debug("TTFF Loop %d - (Lat, Lon) = (%s, %s)" % (ttff_loop,
                                                                   ttff_lat,
                                                                   ttff_lon))
            ttff_pe = calculate_position_error(
                ttff_lat, ttff_lon, true_position)
            ttff_data[ttff_loop] = TTFF_REPORT(utc_time=utc_time,
                                               ttff_loop=ttff_loop,
                                               ttff_sec=ttff_sec,
                                               ttff_pe=ttff_pe,
                                               ttff_ant_cn=ttff_ant_cn,
                                               ttff_base_cn=ttff_base_cn)
            ad.log.info("UTC Time = %s, Loop %d = %.1f seconds, "
                        "Position Error = %.1f meters, "
                        "Antenna Average Signal = %.1f dbHz, "
                        "Baseband Average Signal = %.1f dbHz" % (utc_time,
                                                                 ttff_loop,
                                                                 ttff_sec,
                                                                 ttff_pe,
                                                                 ttff_ant_cn,
                                                                 ttff_base_cn))
        stop_gps_results = ad.search_logcat("stop gps test", begin_time)
        if stop_gps_results:
            ad.send_keycode("HOME")
            break
        crash_result = ad.search_logcat("Force finishing activity "
                                        "com.android.gpstool/.GPSTool",
                                        begin_time)
        if crash_result:
            raise signals.TestError("GPSTool crashed. Abort test.")
        # wait 5 seconds to avoid logs not writing into logcat yet
        time.sleep(5)
    return ttff_data


def check_ttff_data(ad, ttff_data, ttff_mode, criteria):
    """Verify all TTFF results from ttff_data.

    Args:
        ad: An AndroidDevice object.
        ttff_data: TTFF data of secs, position error and signal strength.
        ttff_mode: TTFF Test mode for current test item.
        criteria: Criteria for current test item.

    Returns:
        True: All TTFF results are within criteria.
        False: One or more TTFF results exceed criteria or Timeout.
    """
    ad.log.info("%d iterations of TTFF %s tests finished."
                % (len(ttff_data.keys()), ttff_mode))
    ad.log.info("%s PASS criteria is %d seconds" % (ttff_mode, criteria))
    ad.log.debug("%s TTFF data: %s" % (ttff_mode, ttff_data))
    ttff_property_key_and_value(ad, ttff_data, ttff_mode)
    if len(ttff_data.keys()) == 0:
        ad.log.error("GTW_GPSTool didn't process TTFF properly.")
        return False
    elif any(float(ttff_data[key].ttff_sec) == 0.0 for key in ttff_data.keys()):
        ad.log.error("One or more TTFF %s Timeout" % ttff_mode)
        return False
    elif any(float(ttff_data[key].ttff_sec) >= criteria for key in
             ttff_data.keys()):
        ad.log.error("One or more TTFF %s are over test criteria %d seconds"
                     % (ttff_mode, criteria))
        return False
    ad.log.info("All TTFF %s are within test criteria %d seconds."
                % (ttff_mode, criteria))
    return True


def ttff_property_key_and_value(ad, ttff_data, ttff_mode):
    """Output ttff_data to test_run_info for ACTS plugin to parse and display
    on MobileHarness as Property.

    Args:
        ad: An AndroidDevice object.
        ttff_data: TTFF data of secs, position error and signal strength.
        ttff_mode: TTFF Test mode for current test item.
    """
    prop_basename = "TestResult "+ttff_mode.replace(" ", "_")+"_TTFF_"
    sec_list = [float(ttff_data[key].ttff_sec) for key in ttff_data.keys()]
    pe_list = [float(ttff_data[key].ttff_pe) for key in ttff_data.keys()]
    ant_cn_list = [float(ttff_data[key].ttff_ant_cn) for key in
                   ttff_data.keys()]
    base_cn_list = [float(ttff_data[key].ttff_base_cn) for key in
                    ttff_data.keys()]
    timeoutcount = sec_list.count(0.0)
    if len(sec_list) == timeoutcount:
        avgttff = 9527
    else:
        avgttff = sum(sec_list)/(len(sec_list) - timeoutcount)
    if timeoutcount != 0:
        maxttff = 9527
    else:
        maxttff = max(sec_list)
    avgdis = sum(pe_list)/len(pe_list)
    maxdis = max(pe_list)
    ant_avgcn = sum(ant_cn_list)/len(ant_cn_list)
    base_avgcn = sum(base_cn_list)/len(base_cn_list)
    ad.log.info(prop_basename+"AvgTime %.1f" % avgttff)
    ad.log.info(prop_basename+"MaxTime %.1f" % maxttff)
    ad.log.info(prop_basename+"TimeoutCount %d" % timeoutcount)
    ad.log.info(prop_basename+"AvgDis %.1f" % avgdis)
    ad.log.info(prop_basename+"MaxDis %.1f" % maxdis)
    ad.log.info(prop_basename+"Ant_AvgSignal %.1f" % ant_avgcn)
    ad.log.info(prop_basename+"Base_AvgSignal %.1f" % base_avgcn)


def calculate_position_error(latitude, longitude, true_position):
    """Use haversine formula to calculate position error base on true location
    coordinate.

    Args:
        latitude: latitude of location fixed in the present.
        longitude: longitude of location fixed in the present.
        true_position: [latitude, longitude] of true location coordinate.

    Returns:
        position_error of location fixed in the present.
    """
    radius = 6371009
    dlat = math.radians(latitude - true_position[0])
    dlon = math.radians(longitude - true_position[1])
    a = math.sin(dlat/2) * math.sin(dlat/2) + \
        math.cos(math.radians(true_position[0])) * \
        math.cos(math.radians(latitude)) * math.sin(dlon/2) * math.sin(dlon/2)
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return radius * c


def launch_google_map(ad):
    """Launch Google Map via intent.

    Args:
        ad: An AndroidDevice object.
    """
    ad.log.info("Launch Google Map.")
    try:
        ad.adb.shell("am start -S -n com.google.android.apps.maps/"
                     "com.google.android.maps.MapsActivity")
        ad.send_keycode("BACK")
        ad.force_stop_apk("com.google.android.apps.maps")
        ad.adb.shell("am start -S -n com.google.android.apps.maps/"
                     "com.google.android.maps.MapsActivity")
    except Exception as e:
        ad.log.error(e)
        raise signals.TestError("Failed to launch google map.")
    check_current_focus_app(ad)


def check_current_focus_app(ad):
    """Check to see current focused window and app.

    Args:
        ad: An AndroidDevice object.
    """
    time.sleep(1)
    current = ad.adb.shell(
        "dumpsys window | grep -E 'mCurrentFocus|mFocusedApp'")
    ad.log.debug("\n"+current)


def check_location_api(ad, retries):
    """Verify if GnssLocationProvider API reports location.

    Args:
        ad: An AndroidDevice object.
        retries: Retry time.

    Returns:
        True: GnssLocationProvider API reports location.
        otherwise return False.
    """
    for i in range(retries):
        begin_time = get_current_epoch_time()
        ad.log.info("Try to get location report from GnssLocationProvider API "
                    "- attempt %d" % (i+1))
        while get_current_epoch_time() - begin_time <= 30000:
            logcat_results = ad.search_logcat("REPORT_LOCATION", begin_time)
            if logcat_results:
                ad.log.info("%s" % logcat_results[-1]["log_message"])
                ad.log.info("GnssLocationProvider reports location "
                            "successfully.")
                return True
        if not ad.is_adb_logcat_on:
            ad.start_adb_logcat()
    ad.log.error("GnssLocationProvider is unable to report location.")
    return False

def check_network_location(ad, retries, location_type, criteria=30):
    """Verify if NLP reports location after requesting via GPSTool.

    Args:
        ad: An AndroidDevice object.
        retries: Retry time.
        location_type: cell or wifi.
        criteria: expected nlp return time, default 30 seconds

    Returns:
        True: NLP reports location.
        otherwise return False.
    """
    criteria = criteria * 1000
    search_pattern = ("GPSTool : networkLocationType = %s" % location_type)
    for i in range(retries):
        begin_time = get_current_epoch_time()
        ad.log.info("Try to get NLP status - attempt %d" % (i+1))
        ad.adb.shell(
            "am start -S -n com.android.gpstool/.GPSTool --es mode nlp")
        while get_current_epoch_time() - begin_time <= criteria:
            # Search pattern in 1 second interval
            time.sleep(1)
            result = ad.search_logcat(search_pattern, begin_time)
            if result:
                ad.log.info("Pattern Found: %s." % result[-1]["log_message"])
                ad.send_keycode("BACK")
                return True
        if not ad.is_adb_logcat_on:
            ad.start_adb_logcat()
        ad.send_keycode("BACK")
    ad.log.error("Unable to report network location \"%s\"." % location_type)
    return False


def set_attenuator_gnss_signal(ad, attenuator, atten_value):
    """Set attenuation value for different GNSS signal.

    Args:
        ad: An AndroidDevice object.
        attenuator: The attenuator object.
        atten_value: attenuation value
    """
    try:
        ad.log.info(
            "Set attenuation value to \"%d\" for GNSS signal." % atten_value)
        attenuator[0].set_atten(atten_value)
    except Exception as e:
        ad.log.error(e)


def set_battery_saver_mode(ad, state):
    """Enable or disable battery saver mode via adb.

    Args:
        ad: An AndroidDevice object.
        state: True is enable Battery Saver mode. False is disable.
    """
    ad.root_adb()
    if state:
        ad.log.info("Enable Battery Saver mode.")
        ad.adb.shell("cmd battery unplug")
        ad.adb.shell("settings put global low_power 1")
    else:
        ad.log.info("Disable Battery Saver mode.")
        ad.adb.shell("settings put global low_power 0")
        ad.adb.shell("cmd battery reset")


def set_gnss_qxdm_mask(ad, masks):
    """Find defined gnss qxdm mask and set as default logging mask.

    Args:
        ad: An AndroidDevice object.
        masks: Defined gnss qxdm mask.
    """
    try:
        for mask in masks:
            if not tutils.find_qxdm_log_mask(ad, mask):
                continue
            tutils.set_qxdm_logger_command(ad, mask)
            break
    except Exception as e:
        ad.log.error(e)
        raise signals.TestError("Failed to set any QXDM masks.")


def start_youtube_video(ad, url=None, retries=0):
    """Start youtube video and verify if audio is in music state.

    Args:
        ad: An AndroidDevice object.
        url: Youtube video url.
        retries: Retry times if audio is not in music state.

    Returns:
        True if youtube video is playing normally.
        False if youtube video is not playing properly.
    """
    for i in range(retries):
        ad.log.info("Open an youtube video - attempt %d" % (i+1))
        cmd = ("am start -n com.google.android.youtube/"
               "com.google.android.youtube.UrlActivity -d \"%s\"" % url)
        ad.adb.shell(cmd)
        time.sleep(2)
        out = ad.adb.shell(
            "dumpsys activity | grep NewVersionAvailableActivity")
        if out:
            ad.log.info("Skip Youtube New Version Update.")
            ad.send_keycode("BACK")
        if tutils.wait_for_state(ad.droid.audioIsMusicActive, True, 15, 1):
            ad.log.info("Started a video in youtube, audio is in MUSIC state")
            return True
        ad.log.info("Force-Stop youtube and reopen youtube again.")
        ad.force_stop_apk("com.google.android.youtube")
    check_current_focus_app(ad)
    raise signals.TestError("Started a video in youtube, "
                            "but audio is not in MUSIC state")


def get_baseband_and_gms_version(ad, extra_msg=""):
    """Get current radio baseband and GMSCore version of AndroidDevice object.

    Args:
        ad: An AndroidDevice object.
        extra_msg: Extra message before or after the change.
    """
    try:
        build_version = ad.adb.getprop("ro.build.id")
        baseband_version = ad.adb.getprop("gsm.version.baseband")
        gms_version = ad.adb.shell(
            "dumpsys package com.google.android.gms | grep versionName"
        ).split("\n")[0].split("=")[1]
        mpss_version = ad.adb.shell("cat /sys/devices/soc0/images | grep MPSS "
                                    "| cut -d ':' -f 3")
        if not extra_msg:
            ad.log.info("TestResult Build_Version %s" % build_version)
            ad.log.info("TestResult Baseband_Version %s" % baseband_version)
            ad.log.info(
                "TestResult GMS_Version %s" % gms_version.replace(" ", ""))
            ad.log.info("TestResult MPSS_Version %s" % mpss_version)
        else:
            ad.log.info(
                "%s, Baseband_Version = %s" % (extra_msg, baseband_version))
    except Exception as e:
        ad.log.error(e)


def start_toggle_gnss_by_gtw_gpstool(ad, iteration):
    """Send toggle gnss off/on start_test_action

    Args:
        ad: An AndroidDevice object.
        iteration: Iteration of toggle gnss off/on cycles.
    """
    msg_list = []
    begin_time = get_current_epoch_time()
    try:
        for i in range(1, 4):
            ad.adb.shell("am start -S -n com.android.gpstool/.GPSTool "
                         "--es mode toggle --es cycle %d" % iteration)
            time.sleep(1)
            if ad.search_logcat("cmp=com.android.gpstool/.ToggleGPS",
                                begin_time):
                ad.log.info("Send ToggleGPS start_test_action successfully.")
                break
        else:
            check_current_focus_app(ad)
            raise signals.TestError("Fail to send ToggleGPS "
                                    "start_test_action within 3 attempts.")
        time.sleep(2)
        test_start = ad.search_logcat("GPSTool_ToggleGPS: startService",
                                      begin_time)
        if test_start:
            ad.log.info(test_start[-1]["log_message"].split(":")[-1].strip())
        else:
            raise signals.TestError("Fail to start toggle GPS off/on test.")
        # Every iteration is expected to finish within 4 minutes.
        while get_current_epoch_time() - begin_time <= iteration * 240000:
            crash_end = ad.search_logcat("Force finishing activity "
                                         "com.android.gpstool/.GPSTool",
                                         begin_time)
            if crash_end:
                raise signals.TestError("GPSTool crashed. Abort test.")
            toggle_results = ad.search_logcat("GPSTool : msg", begin_time)
            if toggle_results:
                for toggle_result in toggle_results:
                    msg = toggle_result["log_message"]
                    if not msg in msg_list:
                        ad.log.info(msg.split(":")[-1].strip())
                        msg_list.append(msg)
                    if "timeout" in msg:
                        raise signals.TestFailure("Fail to get location fixed "
                                                  "within 60 seconds.")
                    if "Test end" in msg:
                        raise signals.TestPass("Completed quick toggle GNSS "
                                               "off/on test.")
        raise signals.TestFailure("Fail to finish toggle GPS off/on test "
                                  "within %d minutes" % (iteration * 4))
    finally:
        ad.send_keycode("HOME")


def grant_location_permission(ad, option):
    """Grant or revoke location related permission.

    Args:
        ad: An AndroidDevice object.
        option: Boolean to grant or revoke location related permissions.
    """
    action = "grant" if option else "revoke"
    for permission in LOCATION_PERMISSIONS:
        ad.log.info(
            "%s permission:%s on %s" % (action, permission, TEST_PACKAGE_NAME))
        ad.adb.shell("pm %s %s %s" % (action, TEST_PACKAGE_NAME, permission))


def check_location_runtime_permissions(ad, package, permissions):
    """Check if runtime permissions are granted on selected package.

    Args:
        ad: An AndroidDevice object.
        package: Apk package name to check.
        permissions: A list of permissions to be granted.
    """
    for _ in range(3):
        location_runtime_permission = ad.adb.shell(
            "dumpsys package %s | grep ACCESS_FINE_LOCATION" % package)
        if "true" not in location_runtime_permission:
            ad.log.info("ACCESS_FINE_LOCATION is NOT granted on %s" % package)
            for permission in permissions:
                ad.log.debug("Grant %s on %s" % (permission, package))
                ad.adb.shell("pm grant %s %s" % (package, permission))
        else:
            ad.log.info("ACCESS_FINE_LOCATION is granted on %s" % package)
            break
    else:
        raise signals.TestError(
            "Fail to grant ACCESS_FINE_LOCATION on %s" % package)


def install_mdstest_app(ad, mdsapp):
    """
        Install MDS test app in DUT

        Args:
            ad: An Android Device Object
            mdsapp: Installation path of MDSTest app
    """
    if not ad.is_apk_installed("com.google.mdstest"):
        ad.adb.install("-r %s" % mdsapp, timeout=300, ignore_status=True)


def write_modemconfig(ad, mdsapp, nvitem_dict, modemparfile):
    """
        Modify the NV items using modem_tool.par
        Note: modem_tool.par

        Args:
            ad:  An Android Device Object
            mdsapp: Installation path of MDSTest app
            nvitem_dict: dictionary of NV items and values.
            modemparfile: modem_tool.par path.
    """
    ad.log.info("Verify MDSTest app installed in DUT")
    install_mdstest_app(ad, mdsapp)
    os.system("chmod 777 %s" % modemparfile)
    for key, value in nvitem_dict.items():
        if key.isdigit():
            op_name = "WriteEFS"
        else:
            op_name = "WriteNV"
        ad.log.info("Modifying the NV{!r} using {}".format(key, op_name))
        job.run("{} --op {} --item {} --data '{}'".
                format(modemparfile, op_name, key, value))
        time.sleep(2)


def verify_modemconfig(ad, nvitem_dict, modemparfile):
    """
        Verify the NV items using modem_tool.par
        Note: modem_tool.par

        Args:
            ad:  An Android Device Object
            nvitem_dict: dictionary of NV items and values
            modemparfile: modem_tool.par path.
    """
    os.system("chmod 777 %s" % modemparfile)
    for key, value in nvitem_dict.items():
        if key.isdigit():
            op_name = "ReadEFS"
        else:
            op_name = "ReadNV"
        # Sleeptime to avoid Modem communication error
        time.sleep(5)
        result = job.run(
            "{} --op {} --item {}".format(modemparfile, op_name, key))
        output = str(result.stdout)
        ad.log.info("Actual Value for NV{!r} is {!r}".format(key, output))
        if not value.casefold() in output:
            ad.log.error("NV Value is wrong {!r} in {!r}".format(value, result))
            raise ValueError(
                "could not find {!r} in {!r}".format(value, result))


def check_ttff_pe(ad, ttff_data, ttff_mode, pe_criteria):
    """Verify all TTFF results from ttff_data.

    Args:
        ad: An AndroidDevice object.
        ttff_data: TTFF data of secs, position error and signal strength.
        ttff_mode: TTFF Test mode for current test item.
        pe_criteria: Criteria for current test item.

    """
    ad.log.info("%d iterations of TTFF %s tests finished.",
                (len(ttff_data.keys()), ttff_mode))
    ad.log.info("%s PASS criteria is %f meters", (ttff_mode, pe_criteria))
    ad.log.debug("%s TTFF data: %s", (ttff_mode, ttff_data))

    if len(ttff_data.keys()) == 0:
        ad.log.error("GTW_GPSTool didn't process TTFF properly.")
        raise signals.TestFailure("GTW_GPSTool didn't process TTFF properly.")

    elif any(float(ttff_data[key].ttff_pe) >= pe_criteria for key in
             ttff_data.keys()):
        ad.log.error("One or more TTFF %s are over test criteria %f meters",
                     (ttff_mode, pe_criteria))
        raise signals.TestFailure("GTW_GPSTool didn't process TTFF properly.")
    ad.log.info("All TTFF %s are within test criteria %f meters.",
                (ttff_mode, pe_criteria))


def check_adblog_functionality(ad):
    """Restart adb logcat if system can't write logs into file after checking
    adblog file size.

    Args:
        ad: An AndroidDevice object.
    """
    logcat_path = os.path.join(ad.device_log_path, "adblog_%s_debug.txt" %
                               ad.serial)
    if not os.path.exists(logcat_path):
        raise signals.TestError("Logcat file %s does not exist." % logcat_path)
    original_log_size = os.path.getsize(logcat_path)
    ad.log.debug("Original adblog size is %d" % original_log_size)
    time.sleep(.5)
    current_log_size = os.path.getsize(logcat_path)
    ad.log.debug("Current adblog size is %d" % current_log_size)
    if current_log_size == original_log_size:
        ad.log.warn("System can't write logs into file. Restart adb "
                    "logcat process now.")
        ad.stop_adb_logcat()
        ad.start_adb_logcat()


def build_instrumentation_call(package,
                               runner,
                               test_methods=None,
                               options=None):
    """Build an instrumentation call for the tests

    Args:
        package: A string to identify test package.
        runner: A string to identify test runner.
        test_methods: A dictionary contains {class_name, test_method}.
        options: A dictionary constant {key, value} param for test.

    Returns:
        An instrumentation call command.
    """
    if test_methods is None:
        test_methods = {}
        cmd_builder = InstrumentationCommandBuilder()
    else:
        cmd_builder = InstrumentationTestCommandBuilder()
    if options is None:
        options = {}
    cmd_builder.set_manifest_package(package)
    cmd_builder.set_runner(runner)
    cmd_builder.add_flag("-w")
    for class_name, test_method in test_methods.items():
        cmd_builder.add_test_method(class_name, test_method)
    for option_key, option_value in options.items():
        cmd_builder.add_key_value_param(option_key, option_value)
    return cmd_builder.build()


def check_chipset_vendor_by_qualcomm(ad):
    """Check if cipset vendor is by Qualcomm.

    Args:
        ad: An AndroidDevice object.

    Returns:
        True if it's by Qualcomm. False irf not.
    """
    ad.root_adb()
    soc = str(ad.adb.shell("getprop gsm.version.ril-impl"))
    ad.log.debug("SOC = %s" % soc)
    return "Qualcomm" in soc


def delete_lto_file(ad):
    """Delete downloaded LTO files.

    Args:
        ad: An AndroidDevice object.
    """
    remount_device(ad)
    status = ad.adb.shell("rm -rf /data/vendor/gps/lto*")
    ad.log.info("Delete downloaded LTO files.\n%s" % status)


def lto_mode(ad, state):
    """Enable or Disable LTO mode.

    Args:
        ad: An AndroidDevice object.
        state: True to enable. False to disable.
    """
    server_list = ["LONGTERM_PSDS_SERVER_1",
                   "LONGTERM_PSDS_SERVER_2",
                   "LONGTERM_PSDS_SERVER_3",
                   "NORMAL_PSDS_SERVER",
                   "REALTIME_PSDS_SERVER"]
    delete_lto_file(ad)
    tmp_path = tempfile.mkdtemp()
    ad.pull_files("/etc/gps_debug.conf", tmp_path)
    gps_conf_path = os.path.join(tmp_path, "gps_debug.conf")
    gps_conf_file = open(gps_conf_path, "r")
    lines = gps_conf_file.readlines()
    gps_conf_file.close()
    fout = open(gps_conf_path, "w")
    if state:
        for line in lines:
            for server in server_list:
                if server in line:
                    line = line.replace(line, "")
            fout.write(line)
        fout.close()
        ad.push_system_file(gps_conf_path, "/etc/gps_debug.conf")
        ad.log.info("Push back modified gps_debug.conf")
        ad.log.info("LTO/RTO/RTI enabled")
    else:
        ad.adb.shell("echo %r >> /etc/gps_debug.conf" %
                     DISABLE_LTO_FILE_CONTENTS)
        ad.log.info("LTO/RTO/RTI disabled")
    reboot(ad)


def start_pixel_logger(ad, max_log_size_mb=100, max_number_of_files=500):
    """adb to start pixel logger for GNSS logging.

    Args:
        ad: An AndroidDevice object.
        max_log_size_mb: Determines when to create a new log file if current
            one reaches the size limit.
        max_number_of_files: Determines how many log files can be saved on DUT.
    """
    retries = 3
    start_timeout_sec = 60
    default_gnss_cfg = "/vendor/etc/mdlog/DEFAULT+SECURITY+FULLDPL+GPS.cfg"
    if check_chipset_vendor_by_qualcomm(ad):
        start_cmd = ("am start-foreground-service -a com.android.pixellogger"
                     ".service.logging.LoggingService.ACTION_START_LOGGING "
                     "-e intent_key_cfg_path '%s' "
                     "--ei intent_key_max_log_size_mb %d "
                     "--ei intent_key_max_number_of_files %d" % (
            default_gnss_cfg, max_log_size_mb, max_number_of_files))
    else:
        start_cmd = ("am startservice -a com.android.pixellogger."
                     "service.logging.LoggingService.ACTION_START_LOGGING "
                     "-e intent_logger brcm_gps")
    for attempt in range(retries):
        begin_time = get_current_epoch_time()
        ad.log.info("Start Pixel Logger. - Attempt %d" % (attempt + 1))
        ad.adb.shell(start_cmd)
        while get_current_epoch_time() - begin_time <= start_timeout_sec * 1000:
            if not ad.is_adb_logcat_on:
                ad.start_adb_logcat()
            if check_chipset_vendor_by_qualcomm(ad):
                start_result = ad.search_logcat("Start logging", begin_time)
            else:
                start_result = ad.search_logcat("startRecording", begin_time)
            if start_result:
                ad.log.info("Pixel Logger starts recording successfully.")
                return True
        ad.force_stop_apk("com.android.pixellogger")
    else:
        ad.log.warn("Pixel Logger fails to start recording in %d seconds "
                    "within %d attempts." % (start_timeout_sec, retries))


def stop_pixel_logger(ad):
    """adb to stop pixel logger for GNSS logging.

    Args:
        ad: An AndroidDevice object.
    """
    retries = 3
    stop_timeout_sec = 300
    if check_chipset_vendor_by_qualcomm(ad):
        stop_cmd = ("am start-foreground-service -a com.android.pixellogger"
                    ".service.logging.LoggingService.ACTION_STOP_LOGGING")
    else:
        stop_cmd = ("am startservice -a com.android.pixellogger."
                    "service.logging.LoggingService.ACTION_STOP_LOGGING "
                    "-e intent_logger brcm_gps")
    for attempt in range(retries):
        begin_time = get_current_epoch_time()
        ad.log.info("Stop Pixel Logger. - Attempt %d" % (attempt + 1))
        ad.adb.shell(stop_cmd)
        while get_current_epoch_time() - begin_time <= stop_timeout_sec * 1000:
            if not ad.is_adb_logcat_on:
                ad.start_adb_logcat()
            stop_result = ad.search_logcat(
                "LoggingService: Stopping service", begin_time)
            if stop_result:
                ad.log.info("Pixel Logger stops successfully.")
                return True
        ad.force_stop_apk("com.android.pixellogger")
    else:
        ad.log.warn("Pixel Logger fails to stop in %d seconds within %d "
                    "attempts." % (stop_timeout_sec, retries))


def launch_eecoexer(ad):
    """adb to stop pixel logger for GNSS logging.

    Args:
        ad: An AndroidDevice object.
    """
    launch_cmd = ("am start -a android.intent.action.MAIN -n"
                  "com.google.eecoexer"
                  "/.MainActivity")
    ad.adb.shell(launch_cmd)
    try:
        ad.log.info("Launch EEcoexer.")
    except Exception as e:
        ad.log.error(e)
        raise signals.TestError("Failed to launch EEcoexer.")


def excute_eecoexer_function(ad, eecoexer_args):
    """adb to stop pixel logger for GNSS logging.

    Args:
        ad: An AndroidDevice object.
        eecoexer_args: EEcoexer function arguments
    """
    enqueue_cmd = ("am broadcast -a com.google.eecoexer.action.LISTENER"
                   " --es sms_body ENQUEUE,{}".format(eecoexer_args))
    exe_cmd = ("am broadcast -a com.google.eecoexer.action.LISTENER"
               " --es sms_body EXECUTE")
    ad.log.info("EEcoexer Add Enqueue: {}".format(eecoexer_args))
    ad.adb.shell(enqueue_cmd)
    ad.log.info("EEcoexer Excute.")
    ad.adb.shell(exe_cmd)


def restart_gps_daemons(ad):
    """Restart GPS daemons by killing services of gpsd, lhd and scd.

    Args:
        ad: An AndroidDevice object.
    """
    gps_daemons_list = ["gpsd", "lhd", "scd"]
    ad.root_adb()
    for service in gps_daemons_list:
        begin_time = get_current_epoch_time()
        time.sleep(3)
        ad.log.info("Kill GPS daemon \"%s\"" % service)
        ad.adb.shell("killall %s" % service)
        # Wait 3 seconds for daemons and services to start.
        time.sleep(3)
        restart_services = ad.search_logcat("starting service", begin_time)
        for restart_service in restart_services:
            if service in restart_service["log_message"]:
                ad.log.info(restart_service["log_message"])
                ad.log.info(
                    "GPS daemon \"%s\" restarts successfully." % service)
                break
        else:
            raise signals.TestError("Unable to restart \"%s\"" % service)

# Copyright 2018 - The Android Open Source Project
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
r"""Create args.

Defines the create arg parser that holds create specific args.
"""

import argparse
import logging
import os

from acloud import errors
from acloud.create import create_common
from acloud.internal import constants
from acloud.internal.lib import utils

logger = logging.getLogger(__name__)
_DEFAULT_GPU = "default"
CMD_CREATE = "create"


# TODO: Add this into main create args once create_cf/gf is deprecated.
def AddCommonCreateArgs(parser):
    """Adds arguments common to create parsers.

    Args:
        parser: ArgumentParser object, used to parse flags.
    """
    parser.add_argument(
        "--num",
        type=int,
        dest="num",
        required=False,
        default=1,
        help="Number of instances to create.")
    parser.add_argument(
        "--serial-log-file",
        type=str,
        dest="serial_log_file",
        required=False,
        help="Path to a *tar.gz file where serial logs will be saved "
             "when a device fails on boot.")
    parser.add_argument(
        "--autoconnect",
        type=str,
        nargs="?",
        const=constants.INS_KEY_VNC,
        dest="autoconnect",
        required=False,
        choices=[constants.INS_KEY_VNC, constants.INS_KEY_ADB,
                 constants.INS_KEY_WEBRTC],
        help="Determines to establish a tunnel forwarding adb/vnc and "
             "launch VNC/webrtc. Establish a tunnel forwarding adb and vnc "
             "then launch vnc if --autoconnect vnc is provided. Establish a "
             "tunnel forwarding adb if --autoconnect adb is provided. "
             "Establish a tunnel forwarding adb and auto-launch on the browser "
             "if --autoconnect webrtc is provided. For local goldfish "
             "instance, create a window.")
    parser.add_argument(
        "--no-autoconnect",
        action="store_false",
        dest="autoconnect",
        required=False,
        help="Will not automatically create ssh tunnels forwarding adb & vnc "
             "when instance created.")
    parser.set_defaults(autoconnect=constants.INS_KEY_VNC)
    parser.add_argument(
        "--unlock",
        action="store_true",
        dest="unlock_screen",
        required=False,
        default=False,
        help="This can unlock screen after invoke vnc client.")
    parser.add_argument(
        "--report-internal-ip",
        action="store_true",
        dest="report_internal_ip",
        required=False,
        help="Report internal ip of the created instance instead of external "
             "ip. Using the internal ip is used when connecting from another "
             "GCE instance.")
    parser.add_argument(
        "--network",
        type=str,
        dest="network",
        required=False,
        help="Set the network the GCE instance will utilize.")
    parser.add_argument(
        "--skip-pre-run-check",
        action="store_true",
        dest="skip_pre_run_check",
        required=False,
        help="Skip the pre-run check.")
    parser.add_argument(
        "--boot-timeout",
        dest="boot_timeout_secs",
        type=int,
        required=False,
        help="The maximum time in seconds used to wait for the AVD to boot.")
    parser.add_argument(
        "--wait-for-ins-stable",
        dest="ins_timeout_secs",
        type=int,
        required=False,
        help="The maximum time in seconds used to wait for the instance boot "
             "up. The default value to wait for instance up time is 300 secs.")
    parser.add_argument(
        "--build-target",
        type=str,
        dest="build_target",
        help="Android build target, e.g. aosp_cf_x86_phone-userdebug, "
             "or short names: phone, tablet, or tablet_mobile.")
    parser.add_argument(
        "--branch",
        type=str,
        dest="branch",
        help="Android branch, e.g. mnc-dev or git_mnc-dev")
    parser.add_argument(
        "--build-id",
        type=str,
        dest="build_id",
        help="Android build id, e.g. 2145099, P2804227")
    parser.add_argument(
        "--bootloader-branch",
        type=str,
        dest="bootloader_branch",
        help="'cuttlefish only' Branch to consume the bootloader from.",
        required=False)
    parser.add_argument(
        "--bootloader-build-id",
        type=str,
        dest="bootloader_build_id",
        help="'cuttlefish only' Bootloader build id, e.g. P2804227",
        required=False)
    parser.add_argument(
        "--bootloader-build-target",
        type=str,
        dest="bootloader_build_target",
        help="'cuttlefish only' Bootloader build target.",
        required=False)
    parser.add_argument(
        "--kernel-build-id",
        type=str,
        dest="kernel_build_id",
        required=False,
        help="Android kernel build id, e.g. 4586590. This is to test a new"
        " kernel build with a particular Android build (--build-id). If neither"
        " kernel-branch nor kernel-build-id are specified, the kernel that's"
        " bundled with the Android build would be used.")
    parser.add_argument(
        "--kernel-branch",
        type=str,
        dest="kernel_branch",
        required=False,
        help="Android kernel build branch name, e.g."
        " kernel-common-android-4.14. This is to test a new kernel build with a"
        " particular Android build (--build-id). If specified without"
        " specifying kernel-build-id, the last green build in the branch will"
        " be used. If neither kernel-branch nor kernel-build-id are specified,"
        " the kernel that's bundled with the Android build would be used.")
    parser.add_argument(
        "--kernel-build-target",
        type=str,
        dest="kernel_build_target",
        default="kernel",
        help="Kernel build target, specify if different from 'kernel'")
    parser.add_argument(
        "--system-branch",
        type=str,
        dest="system_branch",
        help="'cuttlefish only' Branch to consume the system image (system.img) "
        "from, will default to what is defined by --branch. "
        "That feature allows to (automatically) test various combinations "
        "of vendor.img (CF, e.g.) and system images (GSI, e.g.). ",
        required=False)
    parser.add_argument(
        "--system-build-id",
        type=str,
        dest="system_build_id",
        help="'cuttlefish only' System image build id, e.g. 2145099, P2804227",
        required=False)
    parser.add_argument(
        "--system-build-target",
        type=str,
        dest="system_build_target",
        help="'cuttlefish only' System image build target, specify if different "
        "from --build-target",
        required=False)
    parser.add_argument(
        "--launch-args",
        type=str,
        dest="launch_args",
        help="'cuttlefish only' Add extra args to launch_cvd command.",
        required=False)
    # TODO(146314062): Remove --multi-stage-launch after infra don't use this
    # args.
    parser.add_argument(
        "--multi-stage-launch",
        dest="multi_stage_launch",
        action="store_true",
        required=False,
        default=True,
        help="Enable the multi-stage cuttlefish launch.")
    parser.add_argument(
        "--no-multi-stage-launch",
        dest="multi_stage_launch",
        action="store_false",
        required=False,
        default=None,
        help="Disable the multi-stage cuttlefish launch.")
    parser.add_argument(
        "--no-pull-log",
        dest="no_pull_log",
        action="store_true",
        required=False,
        default=None,
        help="Disable auto download logs when AVD booting up failed.")
    # TODO(147335651): Add gpu in user config.
    # TODO(147335651): Support "--gpu" without giving any value.
    parser.add_argument(
        "--gpu",
        type=str,
        const=_DEFAULT_GPU,
        nargs="?",
        dest="gpu",
        required=False,
        default=None,
        help="GPU accelerator to use if any. e.g. nvidia-tesla-k80. For local "
             "instances, this arg without assigning any value is to enable "
             "local gpu support.")
    # Hide following args for users, it is only used in infra.
    parser.add_argument(
        "--local-instance-dir",
        dest="local_instance_dir",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--num-avds-per-instance",
        type=int,
        dest="num_avds_per_instance",
        required=False,
        default=1,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--oxygen",
        action="store_true",
        dest="oxygen",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--zone",
        type=str,
        dest="zone",
        required=False,
        help=argparse.SUPPRESS)

    # TODO(b/118439885): Old arg formats to support transition, delete when
    # transistion is done.
    parser.add_argument(
        "--serial_log_file",
        type=str,
        dest="serial_log_file",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--build_id",
        type=str,
        dest="build_id",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--build_target",
        type=str,
        dest="build_target",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--system_branch",
        type=str,
        dest="system_branch",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--system_build_id",
        type=str,
        dest="system_build_id",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--system_build_target",
        type=str,
        dest="system_build_target",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--kernel_build_id",
        type=str,
        dest="kernel_build_id",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--kernel_branch",
        type=str,
        dest="kernel_branch",
        required=False,
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--kernel_build_target",
        type=str,
        dest="kernel_build_target",
        default="kernel",
        help=argparse.SUPPRESS)
    parser.add_argument(
        "--bootloader_branch",
        type=str,
        dest="bootloader_branch",
        help=argparse.SUPPRESS,
        required=False)
    parser.add_argument(
        "--bootloader_build_id",
        type=str,
        dest="bootloader_build_id",
        help=argparse.SUPPRESS,
        required=False)
    parser.add_argument(
        "--bootloader_build_target",
        type=str,
        dest="bootloader_build_target",
        help=argparse.SUPPRESS,
        required=False)


def GetCreateArgParser(subparser):
    """Return the create arg parser.

    Args:
       subparser: argparse.ArgumentParser that is attached to main acloud cmd.

    Returns:
        argparse.ArgumentParser with create options defined.
    """
    create_parser = subparser.add_parser(CMD_CREATE)
    create_parser.required = False
    create_parser.set_defaults(which=CMD_CREATE)
    # Use default=None to distinguish remote instance or local. The instance
    # type will be remote if the arg is not provided.
    create_parser.add_argument(
        "--local-instance",
        type=_PositiveInteger,
        const=0,
        metavar="ID",
        nargs="?",
        dest="local_instance",
        required=False,
        help="Create a local AVD instance using the resources associated with "
             "the ID. Choose an unused ID automatically if the value is "
             "not specified (primarily for infra usage).")
    create_parser.add_argument(
        "--adb-port", "-p",
        type=int,
        default=None,
        dest="adb_port",
        required=False,
        help="Specify port for adb forwarding.")
    create_parser.add_argument(
        "--avd-type",
        type=str,
        dest="avd_type",
        default=constants.TYPE_CF,
        choices=[constants.TYPE_GCE, constants.TYPE_CF, constants.TYPE_GF, constants.TYPE_CHEEPS,
                 constants.TYPE_FVP],
        help="Android Virtual Device type (default %s)." % constants.TYPE_CF)
    create_parser.add_argument(
        "--config", "--flavor",
        type=str,
        dest="flavor",
        help="The device flavor of the AVD (default %s). e.g. phone, tv, foldable."
        % constants.FLAVOR_PHONE)
    create_parser.add_argument(
        "--local-image",
        const=constants.FIND_IN_BUILD_ENV,
        type=str,
        dest="local_image",
        nargs="?",
        required=False,
        help="Use the locally built image for the AVD. Look for the image "
        "artifact in $ANDROID_PRODUCT_OUT if no args value is provided."
        "e.g --local-image or --local-image /path/to/dir or --local-image "
        "/path/to/file")
    create_parser.add_argument(
        "--local-kernel-image",
        const=constants.FIND_IN_BUILD_ENV,
        type=str,
        dest="local_kernel_image",
        nargs="?",
        required=False,
        help="Use the locally built kernel image for the AVD. Look for "
        "boot.img or boot-*.img if the argument is a directory. Look for the "
        "image in $ANDROID_PRODUCT_OUT if no argument is provided. e.g., "
        "--local-kernel-image, --local-kernel-image /path/to/dir, or "
        "--local-kernel-image /path/to/img")
    create_parser.add_argument(
        "--local-system-image",
        const=constants.FIND_IN_BUILD_ENV,
        type=str,
        dest="local_system_image",
        nargs="?",
        required=False,
        help="Use the locally built system images for the AVD. Look for the "
        "images in $ANDROID_PRODUCT_OUT if no args value is provided. "
        "e.g., --local-system-image, --local-system-image /path/to/dir, or "
        "--local-system-image /path/to/img")
    create_parser.add_argument(
        "--local-tool",
        type=str,
        dest="local_tool",
        action="append",
        default=[],
        required=False,
        help="Use the tools in the specified directory to create local "
        "instances. The directory structure follows $ANDROID_HOST_OUT or "
        "$ANDROID_EMULATOR_PREBUILTS.")
    create_parser.add_argument(
        "--image-download-dir",
        type=str,
        dest="image_download_dir",
        required=False,
        help="Define remote image download directory, e.g. /usr/local/dl.")
    create_parser.add_argument(
        "--yes", "-y",
        action="store_true",
        dest="no_prompt",
        required=False,
        help=("Automatic yes to prompts. Assume 'yes' as answer to all prompts "
              "and run non-interactively."))
    create_parser.add_argument(
        "--reuse-gce",
        type=str,
        const=constants.SELECT_ONE_GCE_INSTANCE,
        nargs="?",
        dest="reuse_gce",
        required=False,
        help="'cuttlefish only' This can help users use their own instance. "
        "Reusing specific gce instance if --reuse-gce [instance_name] is "
        "provided. Select one gce instance to reuse if --reuse-gce is "
        "provided.")
    create_parser.add_argument(
        "--gce-metadata",
        type=str,
        dest="gce_metadata",
        default=None,
        help="'GCE instance only' Record data into GCE instance metadata with "
        "key-value pair format. e.g. id:12,name:unknown.")
    create_parser.add_argument(
        "--host",
        type=str,
        dest="remote_host",
        default=None,
        help="'cuttlefish only' Provide host name to clean up the remote host. "
        "For example: '--host 1.1.1.1'")
    create_parser.add_argument(
        "--host-user",
        type=str,
        dest="host_user",
        default=constants.GCE_USER,
        help="'remote host only' Provide host user for logging in to the host. "
        "The default value is vsoc-01. For example: '--host 1.1.1.1 --host-user "
        "vsoc-02'")
    create_parser.add_argument(
        "--host-ssh-private-key-path",
        type=str,
        dest="host_ssh_private_key_path",
        default=None,
        help="'remote host only' Provide host key for login on on this host.")
    # User should not specify --spec and --hw_property at the same time.
    hw_spec_group = create_parser.add_mutually_exclusive_group()
    hw_spec_group.add_argument(
        "--hw-property",
        type=str,
        dest="hw_property",
        required=False,
        help="Supported HW properties and example values: %s" %
        constants.HW_PROPERTIES_CMD_EXAMPLE)
    hw_spec_group.add_argument(
        "--spec",
        type=str,
        dest="spec",
        required=False,
        choices=constants.SPEC_NAMES,
        help="The name of a pre-configured device spec that we are "
        "going to use.")
    # Arguments for goldfish type.
    # TODO(b/118439885): Verify args that are used in wrong avd_type.
    # e.g. $acloud create --avd-type cuttlefish --emulator-build-id
    create_parser.add_argument(
        "--emulator-build-id",
        type=int,
        dest="emulator_build_id",
        required=False,
        help="'goldfish only' Emulator build used to run the images. "
        "e.g. 4669466.")

    # Arguments for cheeps type.
    create_parser.add_argument(
        "--stable-cheeps-host-image-name",
        type=str,
        dest="stable_cheeps_host_image_name",
        required=False,
        default=None,
        help=("'cheeps only' The Cheeps host image from which instances are "
              "launched. If specified here, the value set in Acloud config "
              "file will be overridden."))
    create_parser.add_argument(
        "--stable-cheeps-host-image-project",
        type=str,
        dest="stable_cheeps_host_image_project",
        required=False,
        default=None,
        help=("'cheeps only' The project hosting the specified Cheeps host "
              "image. If specified here, the value set in Acloud config file "
              "will be overridden."))
    create_parser.add_argument(
        "--user",
        type=str,
        dest="username",
        required=False,
        default=None,
        help="'cheeps only' username to log in to Chrome OS as.")
    create_parser.add_argument(
        "--password",
        type=str,
        dest="password",
        required=False,
        default=None,
        help="'cheeps only' password to log in to Chrome OS with.")
    create_parser.add_argument(
        "--betty-image",
        type=str,
        dest="cheeps_betty_image",
        required=False,
        default=None,
        help=("'cheeps only' The L1 betty version to use. Only makes sense "
              "when launching a controller image with "
              "stable-cheeps-host-image"))

    AddCommonCreateArgs(create_parser)
    return create_parser


def _PositiveInteger(arg):
    """Convert an argument from a string to a positive integer."""
    try:
        value = int(arg)
    except ValueError as e:
        raise argparse.ArgumentTypeError(arg + " is not an integer.") from e
    if value <= 0:
        raise argparse.ArgumentTypeError(arg + " is not positive.")
    return value


def _VerifyLocalArgs(args):
    """Verify args starting with --local.

    Args:
        args: Namespace object from argparse.parse_args.

    Raises:
        errors.CheckPathError: Image path doesn't exist.
        errors.UnsupportedCreateArgs: The specified avd type does not support
                                      --local-system-image.
        errors.UnsupportedLocalInstanceId: Local instance ID is invalid.
    """
    if args.local_image and not os.path.exists(args.local_image):
        raise errors.CheckPathError(
            "Specified path doesn't exist: %s" % args.local_image)

    if args.local_instance_dir and not os.path.exists(args.local_instance_dir):
        raise errors.CheckPathError(
            "Specified path doesn't exist: %s" % args.local_instance_dir)

    if not (args.local_system_image is None or
            args.avd_type in (constants.TYPE_CF, constants.TYPE_GF)):
        raise errors.UnsupportedCreateArgs("%s instance does not support "
                                           "--local-system-image" %
                                           args.avd_type)

    if (args.local_system_image and
            not os.path.exists(args.local_system_image)):
        raise errors.CheckPathError(
            "Specified path doesn't exist: %s" % args.local_system_image)

    for tool_dir in args.local_tool:
        if not os.path.exists(tool_dir):
            raise errors.CheckPathError(
                "Specified path doesn't exist: %s" % tool_dir)

    if args.autoconnect == constants.INS_KEY_WEBRTC:
        if args.avd_type != constants.TYPE_CF:
            raise errors.UnsupportedCreateArgs(
                "'--autoconnect webrtc' only support cuttlefish.")


def _VerifyHostArgs(args):
    """Verify args starting with --host.

    Args:
        args: Namespace object from argparse.parse_args.

    Raises:
        errors.UnsupportedCreateArgs: When a create arg is specified but
                                      unsupported for remote host mode.
    """
    if args.remote_host and args.local_instance is not None:
        raise errors.UnsupportedCreateArgs(
            "--host is not supported for local instance.")

    if args.remote_host and args.num > 1:
        raise errors.UnsupportedCreateArgs(
            "--num is not supported for remote host.")

    if args.host_user != constants.GCE_USER and args.remote_host is None:
        raise errors.UnsupportedCreateArgs(
            "--host-user only support for remote host.")

    if args.host_ssh_private_key_path and args.remote_host is None:
        raise errors.UnsupportedCreateArgs(
            "--host-ssh-private-key-path only support for remote host.")


def VerifyArgs(args):
    """Verify args.

    Args:
        args: Namespace object from argparse.parse_args.

    Raises:
        errors.UnsupportedMultiAdbPort: multi adb port doesn't support.
        errors.UnsupportedCreateArgs: When a create arg is specified but
                                      unsupported for a particular avd type.
                                      (e.g. --system-build-id for gf)
    """
    # Verify that user specified flavor name is in support list.
    # We don't use argparse's builtin validation because we need to be able to
    # tell when a user doesn't specify a flavor.
    if args.flavor and args.flavor not in constants.ALL_FLAVORS:
        logger.debug("Flavor[%s] isn't in default support list: %s",
                     args.flavor, constants.ALL_FLAVORS)

    if args.avd_type != constants.TYPE_CF:
        if args.system_branch or args.system_build_id or args.system_build_target:
            raise errors.UnsupportedCreateArgs(
                "--system-* args are not supported for AVD type: %s"
                % args.avd_type)

    if args.num > 1 and args.adb_port:
        raise errors.UnsupportedMultiAdbPort(
            "--adb-port is not supported for multi-devices.")

    if args.num > 1 and args.local_instance is not None:
        raise errors.UnsupportedCreateArgs(
            "--num is not supported for local instance.")

    if args.local_instance is None and args.gpu == _DEFAULT_GPU:
        raise errors.UnsupportedCreateArgs(
            "Please assign one gpu model for GCE instance. Reference: "
            "https://cloud.google.com/compute/docs/gpus")

    if args.adb_port:
        utils.CheckPortFree(args.adb_port)

    hw_properties = create_common.ParseKeyValuePairArgs(args.hw_property)
    for key in hw_properties:
        if key not in constants.HW_PROPERTIES:
            raise errors.InvalidHWPropertyError(
                "[%s] is an invalid hw property, supported values are:%s. "
                % (key, constants.HW_PROPERTIES))

    cheeps_only_flags = [args.stable_cheeps_host_image_name,
                         args.stable_cheeps_host_image_project,
                         args.username,
                         args.password,
                         args.cheeps_betty_image]
    if args.avd_type != constants.TYPE_CHEEPS and any(cheeps_only_flags):
        raise errors.UnsupportedCreateArgs(
            "--stable-cheeps-*, --betty-image, --username and --password are "
            "only valid with avd_type == %s" % constants.TYPE_CHEEPS)
    if (args.username or args.password) and not (args.username and args.password):
        raise ValueError("--username and --password must both be set")
    if not args.autoconnect and args.unlock_screen:
        raise ValueError("--no-autoconnect and --unlock couldn't be "
                         "passed in together.")

    _VerifyLocalArgs(args)
    _VerifyHostArgs(args)

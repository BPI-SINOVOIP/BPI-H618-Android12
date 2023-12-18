DESCRIPTION = "Android Automotive OS Virtualization - AGL Services"

TOOLCHAIN = "clang"

DEPENDS += "\
    google-trout-grpc-utils-native \
    systemd \
"

TROUT_target_install = "\
    vehicle_hal_grpc_server \
    dumpstate_grpc_server \
    garage_mode_helper \
    watchdog_test_service \
"

COMMON_OPTIMIZATION = ""

inherit perlnative python3native

require common.inc

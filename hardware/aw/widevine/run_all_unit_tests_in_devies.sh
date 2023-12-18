#!/bin/sh

final_result=0
failed_tests=()

# Below, we will append filters to the exclusion portion of GTEST_FILTER, so we
# need to guarantee it has one.
if [ -z "$GTEST_FILTER" ]; then
  # If it wasn't set, make it add all tests, and remove none.
  GTEST_FILTER="*-"
# if GTEST_FILTER already has a negative sign, we leave it alone.
elif [ 0 -eq `expr index "$GTEST_FILTER" "-"` ]; then
  # If GTEST_FILTER was set, but does not have a negative sign, add one.  This
  # gives gtest an empty list of tests to skip.
  GTEST_FILTER="$GTEST_FILTER-"
fi

# The Android supplement allows for installation in these paths:
OEC_PATHS=/vendor/lib64:/vendor/lib:/system/lib64/vndk-R:/system/lib/vndk-R

# Execute a command in "adb shell" and capture the result.
adb_shell_run() {
  local test_file=$1
  shift
  if ls /vendor/bin/nativetest/$test_file &> /dev/null; then
    test_file=/vendor/bin/nativetest/$test_file
  else
    echo "Please install the test on the device in /vendor/bin/nativetest, "
    exit 1
  fi
  echo "------ Starting: $test_file"
  local tmp_log="/sdcard/mediadrmtest.log"
  local adb_error="[ADB SHELL] $@ $test_file failed"
  # The liboemcrypto.so looks for other shared libraries using the
  # LD_LIBRARY_PATH. It is possible that even though the 64-bit liboemcrypto.so
  # does not exist, there are 64-bit versions of the shared libraries it tries
  # to load. We must reverse the library path in this case so we don't attempt
  # to load 64-bit libraries with the 32-bit liboemcrypto.so.
  if ! ls /vendor/lib64/liboemcrypto.so &> /dev/null; then
    OEC_PATHS=/vendor/lib:/vendor/lib64
  fi
  LD_LIBRARY_PATH=$OEC_PATHS GTEST_FILTER=$GTEST_FILTER $@ $test_file \|\| echo "$adb_error" | tee "$tmp_log"
  ! grep -Fq "$adb_error" "$tmp_log"
  local result=$?
  if [ $result -ne 0 ]; then
    final_result=$result
    failed_tests+=("$adb_error")
  fi
}

# Run oemcrypto tests first due to historical test order issues
adb_shell_run oemcrypto_test \
  GTEST_FILTER="$GTEST_FILTER:*Level1Required" FORCE_LEVEL3_OEMCRYPTO=yes
adb_shell_run oemcrypto_test

# Run request_license_test next to ensure device is provisioned
# adb_shell_run request_license_test $PROVISIONING_ARG

# cdm_extended_duration_test takes >30 minutes to run.
# adb_shell_run cdm_extended_duration_test

# duration_use_case_test takes a very long time to run.
# adb_shell_run duration_use_case_test

# cdm_feature_test to be run with modified/mock oemcrypto
# adb_shell_run cdm_feature_test

# Additional tests
adb_shell_run base64_test
adb_shell_run buffer_reader_test
adb_shell_run cdm_engine_metrics_decorator_unittest
adb_shell_run cdm_engine_test
adb_shell_run cdm_session_unittest
adb_shell_run certificate_provisioning_unittest
adb_shell_run counter_metric_unittest
adb_shell_run crypto_session_unittest
adb_shell_run device_files_unittest
adb_shell_run distribution_unittest
adb_shell_run event_metric_unittest
adb_shell_run file_store_unittest
adb_shell_run file_utils_unittest
adb_shell_run generic_crypto_unittest
adb_shell_run hidl_metrics_adapter_unittest
adb_shell_run http_socket_test
adb_shell_run initialization_data_unittest
adb_shell_run libwvdrmdrmplugin_hidl_test
adb_shell_run libwvdrmdrmplugin_test
adb_shell_run libwvdrmmediacrypto_hidl_test
adb_shell_run libwvdrmmediacrypto_test
adb_shell_run license_keys_unittest
adb_shell_run license_unittest
adb_shell_run odk_test
adb_shell_run policy_engine_constraints_unittest
adb_shell_run policy_engine_unittest
adb_shell_run policy_integration_test
adb_shell_run rw_lock_test
adb_shell_run service_certificate_unittest
adb_shell_run timer_unittest
adb_shell_run usage_table_header_unittest
adb_shell_run value_metric_unittest
adb_shell_run wv_cdm_metrics_test

# Run the non-Treble test on non-Treble devices
if ls /vendor/lib/mediadrm/libwvdrmengine.so &> /dev/null ||
    ls /vendor/lib64/mediadrm/libwvdrmengine.so &> /dev/null; then
  library_path="/vendor/lib/mediadrm/:/vendor/lib64/mediadrm/"
  adb_shell_run libwvdrmengine_test LD_LIBRARY_PATH=$library_path
fi

# Run the Treble test on Treble devices
if ls /vendor/lib/libwvhidl.so &> /dev/null ||
    ls /vendor/lib64/libwvhidl.so &> /dev/null; then
  adb_shell_run libwvdrmengine_hidl_test
fi


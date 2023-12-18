#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH:= $(call my-dir)
# Build CTS verifier framework as a libary.

include $(CLEAR_VARS)

define java-files-in
$(sort $(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find -L $(1) -maxdepth 1 -name *.java -and -not -name ".*") \
 ))
endef

LOCAL_MODULE := cts-verifier-framework
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-BSD SPDX-license-identifier-CC-BY
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_AAPT_FLAGS := --auto-add-overlay --extra-packages android.support.v4
LOCAL_SDK_VERSION := current
LOCAL_MIN_SDK_VERSION := 19
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res
LOCAL_SRC_FILES := \
    $(call java-files-in, src/com/android/cts/verifier) \
    $(call all-Iaidl-files-under, src)

LOCAL_STATIC_JAVA_LIBRARIES := androidx.legacy_legacy-support-v4 \
                               compatibility-common-util-devicesidelib \
                               compatibility-device-util-axt

# Disable dexpreopt and <uses-library> check for test.
LOCAL_ENFORCE_USES_LIBRARIES := false
LOCAL_DEX_PREOPT := false

include $(BUILD_STATIC_JAVA_LIBRARY)

pre-installed-apps := \
    CtsEmptyDeviceAdmin \
    CtsEmptyDeviceOwner \
    CtsPermissionApp \
    CtsForceStopHelper \
    NotificationBot \
    CrossProfileTestApp \
    CtsTtsEngineSelectorTestHelper \
    CtsTtsEngineSelectorTestHelper2

# Apps to be installed as Instant App using adb install --instant
pre-installed-instant-app := CtsVerifierInstantApp

other-required-apps := \
    CtsVerifierUSBCompanion \
    CtsVpnFirewallAppApi23 \
    CtsVpnFirewallAppApi24 \
    CtsVpnFirewallAppNotAlwaysOn

apps-to-include := \
    $(pre-installed-apps) \
    $(pre-installed-instant-app) \
    $(other-required-apps)

define apk-location-for
    $(call intermediates-dir-for,APPS,$(1))/package.apk
endef

# Builds and launches CTS Verifier on a device.
.PHONY: cts-verifier
cts-verifier: CtsVerifier adb $(pre-installed-apps) $(pre-installed-instant-app)
	adb install -r $(PRODUCT_OUT)/data/app/CtsVerifier/CtsVerifier.apk \
		$(foreach app,$(pre-installed-apps), \
		    && adb install -r -t $(call apk-location-for,$(app))) \
		&& adb install -r --instant $(call apk-location-for,$(pre-installed-instant-app)) \
		&& adb shell "am start -n com.android.cts.verifier/.CtsVerifierActivity"

#
# Creates a "cts-verifier" directory that will contain:
#
# 1. Out directory with a "android-cts-verifier" containing the CTS Verifier
#    and other binaries it needs.
#
# 2. Zipped version of the android-cts-verifier directory to be included with
#    the build distribution.
#
cts-dir := $(HOST_OUT)/cts-verifier
verifier-dir-name := android-cts-verifier
verifier-dir := $(cts-dir)/$(verifier-dir-name)
verifier-zip-name := $(verifier-dir-name).zip
verifier-zip := $(cts-dir)/$(verifier-zip-name)

# turned off sensor power tests in initial L release
#$(PRODUCT_OUT)/data/app/CtsVerifier.apk $(verifier-zip): $(verifier-dir)/power/execute_power_tests.py
#$(PRODUCT_OUT)/data/app/CtsVerifier.apk $(verifier-zip): $(verifier-dir)/power/power_monitors/monsoon.py

# Copy the necessary host-side scripts to include in the zip file:
#$(verifier-dir)/power/power_monitors/monsoon.py: cts/apps/CtsVerifier/assets/scripts/power_monitors/monsoon.py | $(ACP)
#	$(hide) mkdir -p $(verifier-dir)/power/power_monitors
#	$(hide) $(ACP) -fp cts/apps/CtsVerifier/assets/scripts/power_monitors/*.py $(verifier-dir)/power/power_monitors/.
#
#$(verifier-dir)/power/execute_power_tests.py: cts/apps/CtsVerifier/assets/scripts/execute_power_tests.py | $(ACP)
#	$(hide) mkdir -p $(verifier-dir)/power
#	$(hide) $(ACP) -fp cts/apps/CtsVerifier/assets/scripts/execute_power_tests.py $@

$(verifier-dir)/NOTICE.txt: cts/apps/CtsVerifier/NOTICE.txt | $(ACP)
	$(hide) $(ACP) -fp cts/apps/CtsVerifier/NOTICE.txt $@

cts : $(verifier-zip)
CtsVerifier : $(verifier-zip)
$(verifier-zip) : $(HOST_OUT)/CameraITS/build_stamp
$(verifier-zip) : $(verifier-dir)/NOTICE.txt
$(verifier-zip) : $(foreach app,$(apps-to-include),$(call apk-location-for,$(app)))
$(verifier-zip) : $(call intermediates-dir-for,APPS,CtsVerifier)/package.apk | $(ACP)
		$(hide) mkdir -p $(verifier-dir)
		$(hide) $(ACP) -fp $< $(verifier-dir)/CtsVerifier.apk
		$(foreach app,$(apps-to-include), \
		    $(ACP) -fp $(call apk-location-for,$(app)) $(verifier-dir)/$(app).apk;)
		$(hide) $(ACP) -fpr $(HOST_OUT)/CameraITS $(verifier-dir)
		$(hide) cd $(cts-dir) && zip -rq $(verifier-dir-name) $(verifier-dir-name)

$(call dist-for-goals, cts, $(verifier-zip):$(verifier-zip-name))

include $(call all-makefiles-under,$(LOCAL_PATH))

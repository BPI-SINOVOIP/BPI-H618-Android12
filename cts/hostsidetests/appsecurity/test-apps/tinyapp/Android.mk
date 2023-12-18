#
# Copyright (C) 2020 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

cert_dir := cts/hostsidetests/appsecurity/certs/pkgsigverify

# This is the default test package signed with the default key.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := CtsPkgInstallTinyApp
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the test package v2 signed with the default key.
include $(LOCAL_PATH)/base.mk
LOCAL_MANIFEST_FILE := AndroidManifest-v2.xml
LOCAL_PACKAGE_NAME := CtsPkgInstallTinyAppV2
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the test package signed using the V1/V2 signature schemes with
# two signers targeting SDK version 30 with sandbox version 1. From this
# package the v1-ec-p256-two-signers-targetSdk-30.apk is created with the
# following command:
# apksigner sign --in v1v2-ec-p256-two-signers-targetSdk-30.apk --out
# v1-ec-p256-two-signers-targetSdk-30.apk --cert ec-p256.x509.pem --key
# ec-p256.pk8 --next-signer --cert ec-p256_2.x509.pem --key ec-p256_2.pk8
# --v2-signing-enabled false --v3-signing-enabled false --v4-signing-enabled false
include $(LOCAL_PATH)/base.mk
LOCAL_SDK_VERSION := 30
LOCAL_MANIFEST_FILE := AndroidManifest-sandbox-v1.xml
LOCAL_PACKAGE_NAME := v1v2-ec-p256-two-signers-targetSdk-30
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/ec-p256_2
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the test package signed using the V3 signature scheme
# with the previous key in the lineage and part of a sharedUid.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-1-sharedUid
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-shareduid.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the test package signed using the V3 signature scheme with
# a rotated key and one signer in the lineage with default capabilities.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-with-por_1_2-default-caps
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_2
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/ec-p256-por_1_2-default-caps
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the test package signed using the V3 signature scheme with
# a rotated key and part of a shareduid. The capabilities of this lineage
# grant access to the previous key in the lineage to join the sharedUid.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-with-por_1_2-default-caps-sharedUid
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-shareduid.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_2
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/ec-p256-por_1_2-default-caps
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the test package signed using the V3 signature scheme with
# a rotated key and part of a shareduid. The signing lineage begins
# with a key that is not in any of the other lineages and is intended
# to verify that two packages signed with lineages that have diverged
# ancestors are not allowed to be installed in the same sharedUserId.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-por_Y_1_2-default-caps-sharedUid
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-shareduid.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_2
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/rsa-2048 $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/por_Y_1_2-default-caps
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the test package signed using the V3 signature scheme with
# a rotated key and part of a shareduid. The capabilities of this lineage
# prevent the previous key in the lineage from joining the sharedUid.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-with-por_1_2-no-shUid-cap-sharedUid
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-shareduid.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_2
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/ec-p256-por_1_2-no-shUid-cap
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the first companion package signed using the V3 signature scheme
# with a rotated key and part of a sharedUid. The capabilities of this lineage
# grant access to the previous key in the lineage to join the sharedUid.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-with-por_1_2-default-caps-sharedUid-companion
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-companion-shareduid.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_2
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/ec-p256-por_1_2-default-caps
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the companion package signed using the V3 signature scheme with
# a rotated key and part of a shareduid. The signing lineage begins
# with a key that is not in any of the other lineages and is intended
# to verify that two packages signed with lineages that have diverged
# ancestors are not allowed to be installed in the same sharedUserId.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-por_Z_1_2-default-caps-sharedUid-companion
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-shareduid.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_2
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/dsa-2048 $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/por_Z_1_2-default-caps
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the first companion package signed using the V3 signature scheme
# with a rotated key and part of a sharedUid but without the signing lineage.
# This app is intended to test lineage scenarios where an app is only signed
# with the latest key in the lineage.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-2-sharedUid-companion
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-companion-shareduid.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_2
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is the second companion package signed using the V3 signature scheme
# with the previous key in the lineage and part of a sharedUid.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-1-sharedUid-companion2
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-companion2-shareduid.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the test package that declares a signature permission.
# The lineage used to sign this test package does not trust the first signing
# key but grants default capabilities to the second signing key.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-with-por_1_2_3-1-no-caps-2-default-declperm
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-declperm.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_3
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/ec-p256-por-1_2_3-1-no-caps-2-default
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the test package that declares a signature permission.
# The lineage used to sign this test package does not trust either of the signing
# keys so an app with only common signers in the lineage should not be granted the
# permission.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-with-por_1_2_3-no-caps-declperm
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-declperm.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_3
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/ec-p256-por-1_2_3-no-caps
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the companion package that requests the signature permission
# declared by the test package above. This package is signed with a signing key that
# diverges from the package above and is intended to verify that a common signing
# key in the lineage that is still granted the permission capability is sufficient
# to be granted a signature permission.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-with-por_1_2_4-companion-usesperm
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-companion-usesperm.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_4
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/ec-p256-por-1_2_4-default-caps
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the test package that declares a signature permission
# with the knownSigner protection flag. This app is signed with the rsa-2048
# signing key with the trusted certificates being ec-p256 and ec-p256_3.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-rsa-2048-decl-knownSigner-ec-p256-1-3
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-decl-knownSigner.xml
LOCAL_CERTIFICATE := $(cert_dir)/rsa-2048
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the test package that declares a signature permission
# without the knownSigner protection flag. This app is signed with the same
# rsa-2048 signing key to allow updates from the package above. This app can
# be used to verify behavior when an app initially uses the knownSigner flag
# and subsequently removes the flag from the permission declaration.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-rsa-2048-declperm
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-declperm.xml
LOCAL_CERTIFICATE := $(cert_dir)/rsa-2048
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the test package that declares a signature permission
# with the knownSigner protection flag using a string resource instead of a
# string-array resource for the trusted certs.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-rsa-2048-decl-knownSigner-str-res-ec-p256-1
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-decl-knownSigner-str-res.xml
LOCAL_CERTIFICATE := $(cert_dir)/rsa-2048
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the test package that declares a signature permission
# with the knownSigner protection flag using a string constant as the value
# of the knownCerts attribute.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-rsa-2048-decl-knownSigner-str-const-ec-p256-1
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-decl-knownSigner-str-const.xml
LOCAL_CERTIFICATE := $(cert_dir)/rsa-2048
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the companion package that uses the permission
# declared with the knownSigner flag. This app's current signer is in
# the array of certificate digests as declared by the test package
# above.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256_3-companion-uses-knownSigner
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-uses-knownSigner.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_3
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the companion package that uses the permission
# declared with the knownSigner flag. This app's current signer is not
# in the array of certificate digests as declared by the test package
# above.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256_2-companion-uses-knownSigner
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-uses-knownSigner.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_2
include $(BUILD_CTS_SUPPORT_PACKAGE)

# This is a version of the companion package that uses the permission
# declared with the knownSigner flag. This app is signed with a rotated
# signing key with the current signer not in the array of certificate
# digests as declared by the test package, but the previous signer in
# the lineage is. This app can be used to verify that knownSigner
# permissions are also granted if the app was previously signed with
# one of the declared digests.
include $(LOCAL_PATH)/base.mk
LOCAL_PACKAGE_NAME := v3-ec-p256-with-por_1_2-companion-uses-knownSigner
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_MANIFEST_FILE := AndroidManifest-uses-knownSigner.xml
LOCAL_CERTIFICATE := $(cert_dir)/ec-p256_2
LOCAL_ADDITIONAL_CERTIFICATES := $(cert_dir)/ec-p256
LOCAL_CERTIFICATE_LINEAGE := $(cert_dir)/ec-p256-por_1_2-default-caps
include $(BUILD_CTS_SUPPORT_PACKAGE)

cert_dir :=

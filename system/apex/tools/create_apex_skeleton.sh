#!/bin/sh

# Creates an apex stub in a subdirectory named after the package name. Edit the APEX_NAME variable
# before running.

APEX_NAME=com.android.yourpackagenamehere

mkdir ${APEX_NAME}
cd ${APEX_NAME}

cat > Android.bp <<EOF
// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
apex_key {
    name: "${APEX_NAME}.key",
    public_key: "${APEX_NAME}.avbpubkey",
    private_key: "${APEX_NAME}.pem",
}

android_app_certificate {
    name: "${APEX_NAME}.certificate",
    certificate: "${APEX_NAME}",
}

apex {
    name: "${APEX_NAME}",
    manifest: "manifest.json",
    file_contexts: ":apex.test-file_contexts",  // Default, please edit, see go/android-apex-howto
    key: "${APEX_NAME}.key",
}
EOF

openssl genrsa -out ${APEX_NAME}.pem 4096
avbtool extract_public_key --key ${APEX_NAME}.pem --output ${APEX_NAME}.avbpubkey

cat > csr.conf <<EOF
[req]
default_bits = 4096
distinguished_name = dn
prompt             = no

[dn]
C="US"
ST="California"
L="Mountain View"
O="Android"
OU="Android"
emailAddress="android@android.com"
CN="${APEX_NAME}"
EOF

openssl req -x509 -config csr.conf -newkey rsa:4096 -nodes -days 999999 -keyout key.pem -out ${APEX_NAME}.x509.pem
rm csr.conf
openssl pkcs8 -topk8 -inform PEM -outform DER -in key.pem -out ${APEX_NAME}.pk8 -nocrypt
rm key.pem

cat > manifest.json << EOF
{
  "name": "${APEX_NAME}",
  "version": 1
}
EOF

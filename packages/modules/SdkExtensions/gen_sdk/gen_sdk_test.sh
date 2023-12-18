#!/bin/bash -e
#
# Copyright (C) 2021 The Android Open Source Project
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

# To access gen_sdk
PATH=.:$PATH

# Verifies the tool correctly prints the binary representation of a db.
function test_print_binary() {
  # Golden binary rep generated with:
  # $ gqui from textproto:testdata/test_extensions_db.textpb \
  #        proto extensions_db.proto:ExtensionDatabase \
  #        --outfile rawproto:- | xxd -p
  cat > golden_binary << EOF
0a0a080112060803120208010a1a08021206080312020801120608021202
080212060801120208020a1a080312060803120208031206080212020802
12060801120208020a220804120608031202080312060802120208021206
0801120208041206080512020804
EOF

  diff golden_binary <(gen_sdk --action print_binary --database testdata/test_extensions_db.textpb | xxd -p)
}
test_print_binary

# Verifies the tool is able to re-create the test DB correctly.
function test_new_sdk() {
  rm -f extensions_db.textpb && touch extensions_db.textpb
  gen_sdk --action new_sdk --sdk 1 --modules MEDIA_PROVIDER
  gen_sdk --action new_sdk --sdk 2 --modules MEDIA,IPSEC
  gen_sdk --action new_sdk --sdk 3 --modules MEDIA_PROVIDER
  gen_sdk --action new_sdk --sdk 4 --modules SDK_EXTENSIONS,IPSEC

  diff -u0 testdata/test_extensions_db.textpb extensions_db.textpb
}
test_new_sdk

# Verifies the tool won't allow bogus SDK updates
function test_validate() {
  set +e

  rm -f extensions_db.textpb && echo bogus > extensions_db.textpb
  if gen_sdk --action validate; then
    echo "expected validate to fail on bogus db"
    exit 1
  fi

  rm -f extensions_db.textpb && touch extensions_db.textpb
  gen_sdk --action new_sdk --sdk 1 --modules MEDIA_PROVIDER
  if gen_sdk --action new_sdk --sdk 1 --modules SDK_EXTENSIONS; then
    echo "FAILED: expected duplicate sdk numbers to fail"
    echo "DB:"
    cat extensions_db.textpb
    exit 1
  fi

  if gen_sdk --action validate --database testdata/dupe_req.textpb; then
    echo "FAILED: expected duplicate module in one sdk level to fail"
    exit 1
  fi

  if gen_sdk --action validate --database testdata/backward_req.textpb; then
    echo "FAILED: expect version requirement going backward to fail"
    exit 1
  fi

  set -e
}
test_validate

function test_checked_in_db() {
  gen_sdk --action validate --database extensions_db.textpb
}
test_checked_in_db

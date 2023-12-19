#!/bin/bash -eux

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Builds fuzzers from within a container into ${OUT} directory.
# Expects "${SRC}/adhd" to contain an adhd checkout.

cd "${SRC}/adhd/cras"
./git_prepare.sh

FUZZER_LDFLAGS="${FUZZER_LDFLAGS} ${LIB_FUZZING_ENGINE}"
./configure --enable-fuzzer

# Compile fuzzers
make -j$(nproc)

# Copy fuzzers and dependencies to "${OUT}" directory
cp "${SRC}/adhd/cras/src/cras_rclient_message_fuzzer" "${OUT}/rclient_message"
zip -j "${OUT}/rclient_message_corpus.zip" ./src/fuzz/corpus/*

cp "${SRC}/adhd/cras/src/cras_hfp_slc_fuzzer" "${OUT}/cras_hfp_slc"
cp "${SRC}/adhd/cras/src/fuzz/cras_hfp_slc.dict" "${OUT}/cras_hfp_slc.dict"

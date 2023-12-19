# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Defines a docker image that can build cras fuzzers.
#
FROM gcr.io/oss-fuzz-base/base-builder
LABEL maintainer="dgreid@chromium.org"

COPY . "${SRC}/adhd"
COPY cras/src/fuzz/build.sh "${SRC}/build.sh"
RUN "${SRC}/adhd/cras/install_deps.sh"
RUN mkdir -p /etc/cras && cp "${SRC}/adhd/cras-config/dsp.ini.sample" /etc/cras

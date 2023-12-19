# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
AUTOMAKE_OPTIONS = foreign
SUBDIRS = src
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcras.pc

compile_commands.json:
	which bear || (echo "Please install 'bear' first." && exit 1)
	bear make check -j$(nproc)

clean-local:
	rm -f compile_commands.json

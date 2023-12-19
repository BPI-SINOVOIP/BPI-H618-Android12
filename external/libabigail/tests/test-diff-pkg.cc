// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Sinny Kumari

/// @file
///
/// This test harness program computes the ABI changes between ELF
/// binaries present inside input packages.  Some of the input
/// packages have debuginfo, some don't.  The resulting ABI change
/// report is then compared with a reference one.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution, which means they must be
/// referenced in tests/data/Makefile.am by the EXTRA_DIST variable.

// For package configuration macros.
#include "config.h"
#include <sys/wait.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "abg-workers.h"
#include "test-utils.h"
#include "abg-tools-utils.h"

using std::string;
using std::vector;
using std::cerr;
using abigail::tests::get_src_dir;
using abigail::tools_utils::split_string;

struct InOutSpec
{
  const char* first_in_package_path;
  const char* second_in_package_path;
  const char* prog_options;
  const char* suppression_path;
  const char* first_in_debug_package_path;
  const char* second_in_debug_package_path;
  const char* first_in_devel_package_path;
  const char* second_in_devel_package_path;
  const char* ref_report_path;
  const char* out_report_path;
};// end struct InOutSpec

static InOutSpec in_out_specs[] =
{
  // dir1 contains a suppr spec - it should be ignored.
  {
    "data/test-diff-pkg/dirpkg-0-dir1",
    "data/test-diff-pkg/dirpkg-0-dir2",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-0-report-0.txt",
    "output/test-diff-pkg/dirpkg-0-report-0.txt"
  },
  // dir2 contains a suppr spec - it should be recognized.
  {
    "data/test-diff-pkg/dirpkg-1-dir1",
    "data/test-diff-pkg/dirpkg-1-dir2",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-1-report-0.txt",
    "output/test-diff-pkg/dirpkg-1-report-0.txt"
  },
  // dir2 contains a suppr spec but --no-abignore is specified,
  // the file should be ignored.
  {
    "data/test-diff-pkg/dirpkg-1-dir1",
    "data/test-diff-pkg/dirpkg-1-dir2",
    "--no-default-suppression --no-abignore --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-1-report-1.txt",
    "output/test-diff-pkg/dirpkg-1-report-1.txt"
  },
  // dir2 contains several suppr spec files, ".abignore" and
  // "dir.abignore", so the specs should be merged.
  {
    "data/test-diff-pkg/dirpkg-2-dir1",
    "data/test-diff-pkg/dirpkg-2-dir2",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-2-report-0.txt",
    "output/test-diff-pkg/dirpkg-2-report-0.txt"
  },
  // dir2 contains a suppr spec file, ".abignore" and
  // an additional suppr file is specified on the command line,
  // so the specs should be merged.
  {
    "data/test-diff-pkg/dirpkg-3-dir1",
    "data/test-diff-pkg/dirpkg-3-dir2",
    "--no-default-suppression --no-show-locs",
    "data/test-diff-pkg/dirpkg-3.suppr",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-3-report-0.txt",
    "output/test-diff-pkg/dirpkg-3-report-0.txt"
  },
  // dir2 contains a suppr spec file, ".abignore", which should
  // be ignored because of the program options  and
  // an additional suppr file is specified on the command line,
  // which should be recognized.
  {
    "data/test-diff-pkg/dirpkg-3-dir1",
    "data/test-diff-pkg/dirpkg-3-dir2",
    "--no-default-suppression --no-show-locs --no-abignore",
    "data/test-diff-pkg/dirpkg-3.suppr",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-3-report-1.txt",
    "output/test-diff-pkg/dirpkg-3-report-1.txt"
  },
  { // Just like the previous tests, but loc info is emitted.
    "data/test-diff-pkg/dirpkg-3-dir1",
    "data/test-diff-pkg/dirpkg-3-dir2",
    "--no-default-suppression --no-abignore",
    "data/test-diff-pkg/dirpkg-3.suppr",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-3-report-2.txt",
    "output/test-diff-pkg/dirpkg-3-report-2.txt"
  },
  {
    "data/test-diff-pkg/symlink-dir-test1/dir1/symlinks",
    "data/test-diff-pkg/symlink-dir-test1/dir2/symlinks",
    "--no-default-suppression ",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/symlink-dir-test1-report0.txt ",
    "output/test-diff-pkg/symlink-dir-test1-report0.txt "
  },
#if WITH_TAR
  {
    "data/test-diff-pkg/tarpkg-0-dir1.tar",
    "data/test-diff-pkg/tarpkg-0-dir2.tar",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-0-report-0.txt",
    "output/test-diff-pkg/tarpkg-0-report-0.txt"
  },
  {
    "data/test-diff-pkg/tarpkg-0-dir1.ta",
    "data/test-diff-pkg/tarpkg-0-dir2.ta",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-0-report-0.txt",
    "output/test-diff-pkg/tarpkg-0-report-01.txt"
  },
  {
    "data/test-diff-pkg/tarpkg-0-dir1.tar.gz",
    "data/test-diff-pkg/tarpkg-0-dir2.tar.gz",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-0-report-0.txt",
    "output/test-diff-pkg/tarpkg-0-report-02.txt"
  },
  {
    "data/test-diff-pkg/tarpkg-0-dir1.tar.bz2",
    "data/test-diff-pkg/tarpkg-0-dir2.tar.bz2",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-0-report-0.txt",
    "output/test-diff-pkg/tarpkg-0-report-03.txt"
  },
  {
    "data/test-diff-pkg/tarpkg-1-dir1.tar.gz",
    "data/test-diff-pkg/tarpkg-1-dir2.tar.gz",
    "--no-default-suppression --dso-only",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-1-report-0.txt",
    "output/test-diff-pkg/tarpkg-1-report-0.txt"
  },
#endif //WITH_TAR

#ifdef WITH_RPM
  // Two RPM packages with debuginfo available and have ABI changes
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --private-dso --no-show-locs",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.104-3.fc23.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-0.txt",
    "output/test-diff-pkg/test-rpm-report-0.txt"
  },
  // Two RPM packages with 2nd package debuginfo missing
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --private-dso --no-show-locs",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-1.txt",
    "output/test-diff-pkg/test-rpm-report-1.txt"
  },

  // Two RPM packages with first package debuginfo missing
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --private-dso --no-show-locs",
    "",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.104-3.fc23.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-2.txt",
    "output/test-diff-pkg/test-rpm-report-2.txt"
  },

  // Two RPM packages with missing debuginfo
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-3.txt",
    "output/test-diff-pkg/test-rpm-report-3.txt"
  },

  // Two RPM packages with no ABI change
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "--no-default-suppression --no-show-locs",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-4.txt",
    "output/test-diff-pkg/test-rpm-report-4.txt"
  },
  // Two RPM packages with debuginfo available and we don't want to
  // see added symbols.
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --no-show-locs --no-added-syms",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.104-3.fc23.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-5.txt",
    "output/test-diff-pkg/test-rpm-report-5.txt"
  },
  {
    "data/test-diff-pkg/qemu-img-rhev-2.3.0-7.el7.ppc64.rpm",
    "data/test-diff-pkg/qemu-img-rhev-2.3.0-20.el7.ppc64.rpm",
    "--no-default-suppression --no-show-locs --no-added-syms",
    "",
    "data/test-diff-pkg/qemu-kvm-rhev-debuginfo-2.3.0-7.el7.ppc64.rpm",
    "data/test-diff-pkg/qemu-kvm-rhev-debuginfo-2.3.0-20.el7.ppc64.rpm",
    "",
    "",
    "data/test-diff-pkg/qemu-img-rhev-2.3.0-7.el7.ppc64--qemu-img-rhev-2.3.0-20.el7.ppc64-report-0.txt",
    "output/test-diff-pkg/qemu-img-rhev-2.3.0-7.el7.ppc64--qemu-img-rhev-2.3.0-20.el7.ppc64-report-0.txt"
  },
  {
    "data/test-diff-pkg/empty-pkg-libvirt-0.9.11.3-1.el7.ppc64.rpm",
    "data/test-diff-pkg/empty-pkg-libvirt-1.2.17-13.el7_2.2.ppc64.rpm",
    "",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/empty-pkg-report-0.txt",
    "output/test-diff-pkg/empty-pkg-report-0.txt"
  },
  {
    "data/test-diff-pkg/gmp-4.3.1-7.el6_2.2.ppc64.rpm",
    "data/test-diff-pkg/gmp-4.3.1-10.el6.ppc64.rpm",
    "",
    "",
    "data/test-diff-pkg/gmp-debuginfo-4.3.1-7.el6_2.2.ppc64.rpm",
    "data/test-diff-pkg/gmp-debuginfo-4.3.1-10.el6.ppc64.rpm",
    "",
    "",
    "data/test-diff-pkg/gmp-4.3.1-7.el6_2.2.ppc64--gmp-4.3.1-10.el6.ppc64-report-0.txt",
    "output/test-diff-pkg/gmp-4.3.1-7.el6_2.2.ppc64--gmp-4.3.1-10.el6.ppc64-report-0.txt"
  },
  {
    "data/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-4.3-3.20141204.fc23.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/tbb-debuginfo-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-debuginfo-4.3-3.20141204.fc23.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64--tbb-4.3-3.20141204.fc23.x86_64-report-0.txt",
    "output/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64--tbb-4.3-3.20141204.fc23.x86_64-report-0.txt"
  },
  {
    "data/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-4.3-3.20141204.fc23.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/tbb-debuginfo-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-debuginfo-4.3-3.20141204.fc23.x86_64.rpm",
    "data/test-diff-pkg/tbb-devel-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-devel-4.3-3.20141204.fc23.x86_64.rpm",
    "data/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64--tbb-4.3-3.20141204.fc23.x86_64-report-1.txt",
    "output/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64--tbb-4.3-3.20141204.fc23.x86_64-report-1.txt"
  },
  {
    "data/test-diff-pkg/tbb-2017-8.20161128.fc26.x86_64.rpm",
    "data/test-diff-pkg/tbb-2017-9.20170118.fc27.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/tbb-debuginfo-2017-8.20161128.fc26.x86_64.rpm",
    "data/test-diff-pkg/tbb-debuginfo-2017-9.20170118.fc27.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/tbb-2017-8.20161128.fc26.x86_64--tbb-2017-9.20170118.fc27.x86_64.txt",
    "output/test-diff-pkg/tbb-2017-8.20161128.fc26.x86_64--tbb-2017-9.20170118.fc27.x86_64.txt"
  },
  {
    "data/test-diff-pkg/libICE-1.0.6-1.el6.x86_64.rpm",
    "data/test-diff-pkg/libICE-1.0.9-2.el7.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/libICE-debuginfo-1.0.6-1.el6.x86_64.rpm",
    "data/test-diff-pkg/libICE-debuginfo-1.0.9-2.el7.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/libICE-1.0.6-1.el6.x86_64.rpm--libICE-1.0.9-2.el7.x86_64.rpm-report-0.txt",
    "output/test-diff-pkg/libICE-1.0.6-1.el6.x86_64.rpm--libICE-1.0.9-2.el7.x86_64.rpm-report-0.txt"
  },
  {
    "data/test-diff-pkg/gtk2-immodule-xim-2.24.22-5.el7.i686.rpm",
    "data/test-diff-pkg/gtk2-immodule-xim-2.24.28-8.el7.i686.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/gtk2-debuginfo-2.24.22-5.el7.i686.rpm",
    "data/test-diff-pkg/gtk2-debuginfo-2.24.28-8.el7.i686.rpm",
    "",
    "",
    "data/test-diff-pkg/gtk2-immodule-xim-2.24.22-5.el7.i686--gtk2-immodule-xim-2.24.28-8.el7.i686-report-0.txt",
    "output/test-diff-pkg/gtk2-immodule-xim-2.24.22-5.el7.i686--gtk2-immodule-xim-2.24.28-8.el7.i686-report-0.txt"
  },
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "--no-default-suppression --show-identical-binaries",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-dbus-glib-0.80-3.fc12.x86_64-report-0.txt",
    "output/test-diff-pkg/test-dbus-glib-0.80-3.fc12.x86_64-report-0.txt"
  },
  {
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.armv7hl.rpm",
    "--no-default-suppression",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64--dbus-glib-0.104-3.fc23.armv7hl-report-0.txt",
    "output/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64--dbus-glib-0.104-3.fc23.armv7hl-report-0.txt"
  },
  {
    "data/test-diff-pkg/nonexistent-0.rpm",
    "data/test-diff-pkg/nonexistent-1.rpm",
    "--no-default-suppression --private-dso",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/test-nonexistent-report-0.txt",
    "output/test-diff-pkg/test-nonexistent-report-0.txt"
  },
  {
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.8-1.el7.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/spice-debuginfo-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-debuginfo-0.12.8-1.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.8-1.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-0.txt",
    "output/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-0.txt"
  },
  {
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.8-1.el7.x86_64.rpm",
    "--no-default-suppression --redundant",
    "",
    "data/test-diff-pkg/spice-debuginfo-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-debuginfo-0.12.8-1.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.8-1.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-1.txt",
    "output/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-1.txt"
  },
  {
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.8-1.el7.x86_64.rpm",
    "--no-default-suppression --redundant",
    "",
    "data/test-diff-pkg/spice-debuginfo-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-debuginfo-0.12.8-1.el7.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-2.txt",
    "output/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-2.txt"
  },
  {
    "data/test-diff-pkg/libcdio-0.94-1.fc26.x86_64.rpm",
    "data/test-diff-pkg/libcdio-0.94-2.fc26.x86_64.rpm",
    "--no-default-suppression --redundant",
    "",
    "data/test-diff-pkg/libcdio-debuginfo-0.94-1.fc26.x86_64.rpm",
    "data/test-diff-pkg/libcdio-debuginfo-0.94-2.fc26.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/libcdio-0.94-1.fc26.x86_64--libcdio-0.94-2.fc26.x86_64-report.1.txt",
    "output/test-diff-pkg/libcdio-0.94-1.fc26.x86_64--libcdio-0.94-2.fc26.x86_64-report.1.txt"
  },
  {
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.8-1.el7.x86_64.rpm",
    "--no-default-suppression --leaf-changes-only --impacted-interfaces",
    "",
    "data/test-diff-pkg/spice-debuginfo-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-debuginfo-0.12.8-1.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-3.txt",
    "output/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-3.txt"
  },
  {
    "data/test-diff-pkg/libxfce4ui-devel-4.12.1-8.fc27.ppc64.rpm",
    "data/test-diff-pkg/libxfce4ui-devel-4.12.1-8.fc27.ppc64.rpm",
    "--fail-no-dbg",
    "",
    "data/test-diff-pkg/libxfce4ui-devel-debuginfo-4.12.1-8.fc27.ppc64.rpm",
    "data/test-diff-pkg/libxfce4ui-devel-debuginfo-4.12.1-8.fc27.ppc64.rpm",
    "",
    "",
    "data/test-diff-pkg/libxfce4ui-devel-4.12.1-8.fc27.ppc64-self-report-0.txt",
    "output/test-diff-pkg/libxfce4ui-devel-4.12.1-8.fc27.ppc64-self-report-0.txt"
  },
  {
    "data/test-diff-pkg/libxfce4ui-devel-4.12.1-8.fc27.ppc64.rpm",
    "data/test-diff-pkg/libxfce4ui-devel-4.12.1-8.fc27.ppc64.rpm",
    "--fail-no-dbg",
    "",
    "data/test-diff-pkg/libxfce4ui-debuginfo-4.12.1-8.fc27.ppc64.rpm "
    "data/test-diff-pkg/libxfce4ui-devel-debuginfo-4.12.1-8.fc27.ppc64.rpm",
    "data/test-diff-pkg/libxfce4ui-debuginfo-4.12.1-8.fc27.ppc64.rpm "
    "data/test-diff-pkg/libxfce4ui-devel-debuginfo-4.12.1-8.fc27.ppc64.rpm",
    "",
    "",
    "data/test-diff-pkg/libxfce4ui-devel-4.12.1-8.fc27.ppc64-self-report-ok-0.txt",
    "output/test-diff-pkg/libxfce4ui-devel-4.12.1-8.fc27.ppc64-self-report-ok-0.txt"
  },
  {
    "data/test-diff-pkg/elfutils-libs-0.170-4.el7.x86_64.rpm",
    "data/test-diff-pkg/elfutils-libs-0.171-1.el7.x86_64.rpm",
    "--fail-no-dbg",
    "",
    "data/test-diff-pkg/elfutils-debuginfo-0.170-4.el7.x86_64.rpm",
    "data/test-diff-pkg/elfutils-debuginfo-0.171-1.el7.x86_64.rpm",
    "data/test-diff-pkg/elfutils-devel-0.170-4.el7.x86_64.rpm",
    "data/test-diff-pkg/elfutils-devel-0.171-1.el7.x86_64.rpm",
    "data/test-diff-pkg/elfutils-libs-0.170-4.el7.x86_64-multiple-sym-vers-report-0.txt",
    "output/test-diff-pkg/elfutils-libs-0.170-4.el7.x86_64-multiple-sym-vers-report-0.txt"
  },
  {
    "data/test-diff-pkg/nss-3.23.0-1.0.fc23.x86_64.rpm",
    "data/test-diff-pkg/nss-3.24.0-1.0.fc23.x86_64.rpm",
    "--fail-no-dbg",
    "",
    "data/test-diff-pkg/nss-debuginfo-3.23.0-1.0.fc23.x86_64.rpm",
    "data/test-diff-pkg/nss-debuginfo-3.24.0-1.0.fc23.x86_64.rpm",
    "data/test-diff-pkg/nss-devel-3.23.0-1.0.fc23.x86_64.rpm",
    "data/test-diff-pkg/nss-devel-3.24.0-1.0.fc23.x86_64.rpm",
    "data/test-diff-pkg/nss-3.23.0-1.0.fc23.x86_64-report-0.txt",
    "output/test-diff-pkg/nss-3.23.0-1.0.fc23.x86_64-report-0.txt"
  },
  {
    "data/test-diff-pkg/GtkAda-gl-2.24.2-29.fc29.x86_64.rpm",
    "data/test-diff-pkg/GtkAda-gl-2.24.2-30.fc30.x86_64.rpm",
    "--fail-no-dbg",
    "",
    "data/test-diff-pkg/GtkAda-debuginfo-2.24.2-29.fc29.x86_64.rpm, "
    "data/test-diff-pkg/GtkAda-gl-debuginfo-2.24.2-29.fc29.x86_64.rpm",
    "data/test-diff-pkg/GtkAda-debuginfo-2.24.2-30.fc30.x86_64.rpm, "
    "data/test-diff-pkg/GtkAda-gl-debuginfo-2.24.2-30.fc30.x86_64.rpm",
    "data/test-diff-pkg/GtkAda-devel-2.24.2-29.fc29.x86_64.rpm",
    "data/test-diff-pkg/GtkAda-devel-2.24.2-30.fc30.x86_64.rpm",
    "data/test-diff-pkg/GtkAda-gl-2.24.2-29.fc29.x86_64--2.24.2-30.fc30.x86_64-report-0.txt",
    "output/test-diff-pkg/GtkAda-gl-2.24.2-29.fc29.x86_64--2.24.2-30.fc30.x86_64-report-0.txt"
  },
  {
    "data/test-diff-pkg/netcdf-fortran-mpich-4.4.4-10.fc29.x86_64.rpm",
    "data/test-diff-pkg/netcdf-fortran-mpich-4.4.4-11.fc30.x86_64.rpm",
    "--fail-no-dbg",
    "",
    "data/test-diff-pkg/netcdf-fortran-mpich-debuginfo-4.4.4-10.fc29.x86_64.rpm,"
    "data/test-diff-pkg/netcdf-fortran-debuginfo-4.4.4-10.fc29.x86_64.rpm",
    "data/test-diff-pkg/netcdf-fortran-mpich-debuginfo-4.4.4-11.fc30.x86_64.rpm,"
    "data/test-diff-pkg/netcdf-fortran-debuginfo-4.4.4-11.fc30.x86_64.rpm",
    "data/test-diff-pkg/netcdf-fortran-mpich-devel-4.4.4-10.fc29.x86_64.rpm",
    "data/test-diff-pkg/netcdf-fortran-mpich-devel-4.4.4-11.fc30.x86_64.rpm",
    "data/test-diff-pkg/netcdf-fortran-mpich-4.4.4-10.fc29.x86_64-4.4.4-11.fc30.x86_64-report-0.txt",
    "output/test-diff-pkg/netcdf-fortran-mpich-4.4.4-10.fc29.x86_64-4.4.4-11.fc30.x86_64-report-0.txt"
  },
  {
    "data/test-diff-pkg/PR24410-old/poppler-qt5-0.73.0-4.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24410-new/poppler-qt5-0.73.0-8.fc30.x86_64.rpm",
    "--fail-no-dbg",
    "",
    "data/test-diff-pkg/PR24410-old/poppler-debuginfo-0.73.0-4.fc30.x86_64.rpm, "
    "data/test-diff-pkg/PR24410-old/poppler-qt5-debuginfo-0.73.0-4.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24410-new/poppler-debuginfo-0.73.0-8.fc30.x86_64.rpm, "
    "data/test-diff-pkg/PR24410-new/poppler-qt5-debuginfo-0.73.0-8.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24410-old/poppler-qt5-devel-0.73.0-4.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24410-new/poppler-qt5-devel-0.73.0-8.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24410-report-0.txt",
    "output/test-diff-pkg/PR24410-report-0.txt"
  },
  {
    "data/test-diff-pkg/PR24690/flatpak-libs-1.2.4-3.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24690/flatpak-libs-1.4.0-1.fc30.x86_64.rpm",
    "--non-reachable-types",
    "",
    "data/test-diff-pkg/PR24690/flatpak-debuginfo-1.2.4-3.fc30.x86_64.rpm, "
    "data/test-diff-pkg/PR24690/flatpak-libs-debuginfo-1.2.4-3.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24690/flatpak-debuginfo-1.4.0-1.fc30.x86_64.rpm, "
    "data/test-diff-pkg/PR24690/flatpak-libs-debuginfo-1.4.0-1.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24690/flatpak-devel-1.2.4-3.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24690/flatpak-devel-1.4.0-1.fc30.x86_64.rpm",
    "data/test-diff-pkg/PR24690/PR24690-report-0.txt",
    "output/test-diff-pkg/PR24690/PR24690-report-0.txt"
  },
#if WITH_RPM_4_15
  // This RPM version supports packages compressed with zstd.
  // RPMs from Fedora 31 onwards are compressed with zstd.
  {
    "data/test-diff-pkg/mesa-libGLU-9.0.1-3.fc33.x86_64.rpm",
    "data/test-diff-pkg/mesa-libGLU-9.0.1-3.fc33.x86_64.rpm",
    "--self-check",
    "",
    "data/test-diff-pkg/mesa-libGLU-debuginfo-9.0.1-3.fc33.x86_64.rpm",
    "data/test-diff-pkg/mesa-libGLU-debuginfo-9.0.1-3.fc33.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/mesa-libGLU-9.0.1-3.fc33.x86_64.self-check-report-0.txt",
    "output/test-diff-pkg/mesa-libGLU-9.0.1-3.fc33.x86_64.self-check-report-0.txt"
  },
  {
    "data/test-diff-pkg/hdf5-1.10.6-2.fc33.x86_64.rpm",
    "data/test-diff-pkg/hdf5-1.10.6-2.fc33.x86_64.rpm",
    "--self-check",
    "",
    "data/test-diff-pkg/hdf5-debuginfo-1.10.6-2.fc33.x86_64.rpm",
    "data/test-diff-pkg/hdf5-debuginfo-1.10.6-2.fc33.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/hdf5-1.10.6-2.fc33.x86_64.self-check-report-0.txt",
    "output/test-diff-pkg/hdf5-1.10.6-2.fc33.x86_64.self-check-report-0.txt"
  },
  {
    "data/test-diff-pkg/cogl-1.22.8-2.fc33.x86_64.rpm",
    "data/test-diff-pkg/cogl-1.22.8-2.fc33.x86_64.rpm",
    "--self-check",
    "",
    "data/test-diff-pkg/cogl-debuginfo-1.22.8-2.fc33.x86_64.rpm",
    "data/test-diff-pkg/cogl-debuginfo-1.22.8-2.fc33.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/cogl-1.22.8-2.fc33.x86_64.self-check-report-0.txt",
    "output/test-diff-pkg/cogl-1.22.8-2.fc33.x86_64.self-check-report-0.txt"
  }
  ,
  {
    "data/test-diff-pkg/glibc-2.32-3.fc33.aarch64.rpm",
    "data/test-diff-pkg/glibc-2.32-3.fc33.aarch64.rpm",
    "--self-check",
    "",
    "data/test-diff-pkg/glibc-debuginfo-2.32-3.fc33.aarch64.rpm",
    "data/test-diff-pkg/glibc-debuginfo-2.32-3.fc33.aarch64.rpm",
    "",
    "",
    "data/test-diff-pkg/glibc-2.32-3.fc33.aarch64-self-check-report-0.txt",
    "output/test-diff-pkg/glibc-2.32-3.fc33.aarch64-self-check-report-0.txt"
  }
  ,
  {
    "data/test-diff-pkg/sshpass-1.07-1.fc34.x86_64.rpm",
    "data/test-diff-pkg/sshpass-1.07-1.fc34.x86_64.rpm",
    "--self-check",
    "",
    "data/test-diff-pkg/sshpass-debuginfo-1.07-1.fc34.x86_64.rpm",
    "data/test-diff-pkg/sshpass-debuginfo-1.07-1.fc34.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/sshpass-1.07-1.fc34.x86_64-self-check-report-0.txt",
    "output/test-diff-pkg/sshpass-1.07-1.fc34.x86_64-self-check-report-0.txt"
  },
  {
    "data/test-diff-pkg/nmap-7.70-5.el8_testjcc.x86_64.rpm",
    "data/test-diff-pkg/nmap-7.70-5.el8_testjcc.x86_64.rpm",
    "--self-check",
    "",
    "data/test-diff-pkg/nmap-debuginfo-7.70-5.el8_testjcc.x86_64.rpm",
    "data/test-diff-pkg/nmap-debuginfo-7.70-5.el8_testjcc.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/nmap-7.70-5.el8_testjcc.x86_64-self-check-report-0.txt",
    "output/test-diff-pkg/nmap-7.70-5.el8_testjcc.x86_64-self-check-report-0.txt"
  } ,
  {
    "data/test-diff-pkg/elfutils-libs-0.183-1.el9.x86_64.rpm",
    "data/test-diff-pkg/elfutils-libs-0.183-1.el9.x86_64.rpm",
    "--self-check",
    "",
    "data/test-diff-pkg/elfutils-debuginfo-0.183-1.el9.x86_64.rpm,"
    "data/test-diff-pkg/elfutils-libs-debuginfo-0.183-1.el9.x86_64.rpm",
    "data/test-diff-pkg/elfutils-debuginfo-0.183-1.el9.x86_64.rpm,"
    "data/test-diff-pkg/elfutils-libs-debuginfo-0.183-1.el9.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/elfutils-libs-debuginfo-0.183-1.el9.x86_64-self-check-report-0.txt",
    "output/test-diff-pkg/elfutils-libs-debuginfo-0.183-1.el9.x86_64-self-check-report-0.txt"
  } ,
#endif // WITH_RPM_4_15
#endif //WITH_RPM

#ifdef WITH_DEB
  // Two debian packages.
  {
    "data/test-diff-pkg/libsigc++-2.0-0c2a_2.4.0-1_amd64.deb",
    "data/test-diff-pkg/libsigc++-2.0-0v5_2.4.1-1ubuntu2_amd64.deb",
    "--no-default-suppression --no-show-locs --fail-no-dbg",
    "",
    "data/test-diff-pkg/libsigc++-2.0-0c2a-dbgsym_2.4.0-1_amd64.ddeb",
    "data/test-diff-pkg/libsigc++-2.0-0v5-dbgsym_2.4.1-1ubuntu2_amd64.ddeb",
    "",
    "",
    "data/test-diff-pkg/libsigc++-2.0-0c2a_2.4.0-1_amd64--libsigc++-2.0-0v5_2.4.1-1ubuntu2_amd64-report-0.txt",
    "output/test-diff-pkg/libsigc++-2.0-0c2a_2.4.0-1_amd64--libsigc++-2.0-0v5_2.4.1-1ubuntu2_amd64-report-0.txt"
  },
#endif // WITH_DEB
  // This should be the last entry.
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/// A task which launches abipkgdiff on the binaries passed to the
/// constructor of the task.  The test also launches gnu diff on the
/// result of abipkdiff to compare it against a reference abipkgdiff
/// result.
struct test_task : public abigail::workers::task
{
  InOutSpec spec;
  bool is_ok;
  string diff_cmd;
  string error_message;

  test_task(const InOutSpec& s)
    : spec(s),
      is_ok(true)
  {}

  /// This virtual function overload actually performs the job of the
  /// task.
  ///
  /// It actually launches abipkgdiff on the binaries passed to the
  /// constructor of the task.  It also launches gnu diff on the
  /// result of the abidiff to compare it against a reference abidiff
  /// result.
  virtual void
  perform()
  {
    using abigail::tests::get_build_dir;
    using abigail::tools_utils::ensure_parent_dir_created;

    string first_in_package_path, second_in_package_path,
      prog_options,
      ref_abi_diff_report_path, out_abi_diff_report_path, cmd, abipkgdiff,
      first_in_devel_package_path, second_in_devel_package_path,
      suppression_path;
    vector<string> first_in_debug_package_paths, second_in_debug_package_paths;

    first_in_package_path =
      string(get_src_dir()) + "/tests/" + spec.first_in_package_path;
    second_in_package_path =
      string(get_src_dir()) + "/tests/" + spec.second_in_package_path;

    prog_options = spec.prog_options;

    if (spec.first_in_debug_package_path
	&& strcmp(spec.first_in_debug_package_path, ""))
      {
	vector<string> debug_info_pkg_paths;
	split_string(spec.first_in_debug_package_path, ", ",
		     debug_info_pkg_paths);
	ABG_ASSERT(!debug_info_pkg_paths.empty());
	for (vector<string>::const_iterator s = debug_info_pkg_paths.begin();
	     s != debug_info_pkg_paths.end();
	     ++s)
	  first_in_debug_package_paths.push_back(string(get_src_dir()) +
						 "/tests/" + *s);
      }
    else
      first_in_debug_package_paths.clear();

    if (spec.second_in_debug_package_path
	&& strcmp(spec.second_in_debug_package_path, ""))
      {
	vector<string> debug_info_pkg_paths;
	split_string(spec.second_in_debug_package_path,
		     ", ",
		     debug_info_pkg_paths);
	ABG_ASSERT(!debug_info_pkg_paths.empty());
	for (vector<string>::const_iterator s = debug_info_pkg_paths.begin();
	     s != debug_info_pkg_paths.end();
	     ++s)
	  second_in_debug_package_paths.push_back(string(get_src_dir()) +
						  "/tests/" + *s);
      }
    else
      second_in_debug_package_paths.clear();

    if (spec.first_in_devel_package_path
	&& strcmp(spec.first_in_devel_package_path, ""))
      first_in_devel_package_path =
	string(get_src_dir()) + "/tests/" + spec.first_in_devel_package_path;

    if (spec.second_in_devel_package_path
	&& strcmp(spec.second_in_devel_package_path, ""))
      second_in_devel_package_path =
	string(get_src_dir()) + "/tests/" + spec.second_in_devel_package_path;

    if (spec.suppression_path
	&& strcmp(spec.suppression_path, ""))
      suppression_path =
	string(get_src_dir()) + "/tests/" + spec.suppression_path;
    else
      suppression_path.clear();

    ref_abi_diff_report_path =
      string(get_src_dir()) + "/tests/" + spec.ref_report_path;
    out_abi_diff_report_path =
      string(get_build_dir()) + "/tests/" + spec.out_report_path;

    if (!ensure_parent_dir_created(out_abi_diff_report_path))
      {
	cerr << "could not create parent directory for "
	     << out_abi_diff_report_path;
	is_ok = false;
	return;
      }

    abipkgdiff = string(get_build_dir()) + "/tools/abipkgdiff";

    if (!prog_options.empty())
      abipkgdiff +=  " " + prog_options;

    for (vector<string>::const_iterator p =
	   first_in_debug_package_paths.begin();
	 p != first_in_debug_package_paths.end();
	 ++p)
      abipkgdiff += " --d1 " + *p;

    for (vector<string>::const_iterator p =
	   second_in_debug_package_paths.begin();
	 p != second_in_debug_package_paths.end();
	 ++p)
      abipkgdiff += " --d2 " + *p;

    if (!first_in_devel_package_path.empty())
      abipkgdiff += " --devel1 " + first_in_devel_package_path;

    if (!second_in_devel_package_path.empty())
      abipkgdiff += " --devel2 " + second_in_devel_package_path;

    if (!suppression_path.empty())
      abipkgdiff += " --suppressions " + suppression_path;

    cmd =
      abipkgdiff + " " + first_in_package_path + " " + second_in_package_path;
    cmd += " > " + out_abi_diff_report_path + " 2>&1";

    bool abipkgdiff_ok = true;
    int code = system(cmd.c_str());
    if (!WIFEXITED(code))
      abipkgdiff_ok = false;

    if (abipkgdiff_ok)
      {
	cmd = "diff -u " + ref_abi_diff_report_path + " "
	  + out_abi_diff_report_path;
	if (system(cmd.c_str()))
	  is_ok = false;
      }
    else
      is_ok = false;
  }
}; // end struct test_task

/// A convenience typedef for shared
typedef shared_ptr<test_task> test_task_sptr;

int
main()
{
  using std::vector;
  using std::dynamic_pointer_cast;
  using abigail::workers::queue;
  using abigail::workers::task;
  using abigail::workers::task_sptr;
  using abigail::workers::get_number_of_threads;

  /// Create a task queue.  The max number of worker threads of the
  /// queue is the number of the concurrent threads supported by the
  /// processor of the machine this code runs on.
  const size_t num_tests = sizeof(in_out_specs) / sizeof (InOutSpec) - 1;
  size_t num_workers = std::min(get_number_of_threads(), num_tests);
  queue task_queue(num_workers);

  bool is_ok = true;

  for (InOutSpec *s = in_out_specs; s->first_in_package_path; ++s)
    {
      test_task_sptr t(new test_task(*s));
      ABG_ASSERT(task_queue.schedule_task(t));
    }

  /// Wait for all worker threads to finish their job, and wind down.
  task_queue.wait_for_workers_to_complete();

  // Now walk the results and print whatever error messages need to be
  // printed.

  const vector<task_sptr>& completed_tasks =
    task_queue.get_completed_tasks();
  ABG_ASSERT(completed_tasks.size() == num_tests);

  for (vector<task_sptr>::const_iterator ti = completed_tasks.begin();
       ti != completed_tasks.end();
       ++ti)
    {
      test_task_sptr t = dynamic_pointer_cast<test_task>(*ti);
      if (!t->is_ok)
	{
	  is_ok = false;
	  if (!t->diff_cmd.empty())
	    if (system(t->diff_cmd.c_str()) == -1)
	      cerr << "execution of '" << t->diff_cmd << "' failed\n";
	  if (!t->error_message.empty())
	    cerr << t->error_message << '\n';
	}
    }

    return !is_ok;
}

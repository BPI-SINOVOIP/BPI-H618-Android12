// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program runs a diff between input ELF files containing DWARF
/// debugging information and compares the resulting report with a
/// reference report.  If the resulting report is different from the
/// reference report, the test has failed.  Note that the comparison
/// is done using the abidiff command line comparison tool.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

#include <sys/wait.h>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include "abg-workers.h"
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::string;
using std::cerr;

/// This is an aggregate that specifies where a test shall get its
/// input from and where it shall write its ouput to.
struct InOutSpec
{
  const char* in_elfv0_path;
  const char* in_elfv1_path;
  const char* abidiff_options;
  const char* in_report_path;
  const char* out_report_path;
}; // end struct InOutSpec;

InOutSpec in_out_specs[] =
{
  {
    "data/test-diff-filter/test0-v0.o",
    "data/test-diff-filter/test0-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test0-report.txt",
    "output/test-diff-filter/test0-report.txt",
  },
  {
    "data/test-diff-filter/test0-v0.o",
    "data/test-diff-filter/test0-v1.o",
    "--no-default-suppression --harmless --no-linkage-name "
    "--no-show-locs --no-redundant",
    "data/test-diff-filter/test01-report.txt",
    "output/test-diff-filter/test01-report.txt",
  },
  {
    "data/test-diff-filter/test1-v0.o",
    "data/test-diff-filter/test1-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test1-report.txt",
    "output/test-diff-filter/test1-report.txt",
  },
  {
    "data/test-diff-filter/test2-v0.o",
    "data/test-diff-filter/test2-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test2-report.txt",
    "output/test-diff-filter/test2-report.txt",
  },
  {
    "data/test-diff-filter/test3-v0.o",
    "data/test-diff-filter/test3-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test3-report.txt",
    "output/test-diff-filter/test3-report.txt",
  },
  {
    "data/test-diff-filter/test4-v0.o",
    "data/test-diff-filter/test4-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test4-report.txt",
    "output/test-diff-filter/test4-report.txt",
  },
  {
    "data/test-diff-filter/test5-v0.o",
    "data/test-diff-filter/test5-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test5-report.txt",
    "output/test-diff-filter/test5-report.txt",
  },
  {
    "data/test-diff-filter/test6-v0.o",
    "data/test-diff-filter/test6-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test6-report.txt",
    "output/test-diff-filter/test6-report.txt",
  },
  {
    "data/test-diff-filter/test7-v0.o",
    "data/test-diff-filter/test7-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test7-report.txt",
    "output/test-diff-filter/test7-report.txt",
  },
  {
    "data/test-diff-filter/test8-v0.o",
    "data/test-diff-filter/test8-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test8-report.txt",
    "output/test-diff-filter/test8-report.txt",
  },
  {
    "data/test-diff-filter/test9-v0.o",
    "data/test-diff-filter/test9-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test9-report.txt",
    "output/test-diff-filter/test9-report.txt",
  },
  {
    "data/test-diff-filter/test10-v0.o",
    "data/test-diff-filter/test10-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test10-report.txt",
    "output/test-diff-filter/test10-report.txt",
  },
  {
    "data/test-diff-filter/test11-v0.o",
    "data/test-diff-filter/test11-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test11-report.txt",
    "output/test-diff-filter/test11-report.txt",
  },
  {
    "data/test-diff-filter/test12-v0.o",
    "data/test-diff-filter/test12-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test12-report.txt",
    "output/test-diff-filter/test12-report.txt",
  },
  {
    "data/test-diff-filter/test13-v0.o",
    "data/test-diff-filter/test13-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test13-report.txt",
    "output/test-diff-filter/test13-report.txt",
  },
  {
    "data/test-diff-filter/test14-v0.o",
    "data/test-diff-filter/test14-v1.o",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-filter/test14-0-report.txt",
    "output/test-diff-filter/test14-0-report.txt",
  },
  {
    "data/test-diff-filter/test14-v0.o",
    "data/test-diff-filter/test14-v1.o",
    "--no-default-suppression --no-show-locs --redundant",
    "data/test-diff-filter/test14-1-report.txt",
    "output/test-diff-filter/test14-1-report.txt",
  },
  {
    "data/test-diff-filter/test15-v0.o",
    "data/test-diff-filter/test15-v1.o",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-filter/test15-0-report.txt",
    "output/test-diff-filter/test15-0-report.txt",
  },
  {
    "data/test-diff-filter/test15-v0.o",
    "data/test-diff-filter/test15-v1.o",
    "--no-default-suppression --no-show-locs --redundant",
    "data/test-diff-filter/test15-1-report.txt",
    "output/test-diff-filter/test15-1-report.txt",
  },
  {
    "data/test-diff-filter/test16-v0.o",
    "data/test-diff-filter/test16-v1.o",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-filter/test16-report.txt",
    "output/test-diff-filter/test16-report.txt",
  },
  {
    "data/test-diff-filter/test16-v0.o",
    "data/test-diff-filter/test16-v1.o",
    "--no-default-suppression --no-show-locs --redundant",
    "data/test-diff-filter/test16-report-2.txt",
    "output/test-diff-filter/test16-report-2.txt",
  },
  {
    "data/test-diff-filter/test17-v0.o",
    "data/test-diff-filter/test17-v1.o",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-filter/test17-0-report.txt",
    "output/test-diff-filter/test17-0-report.txt",
  },
  {
    "data/test-diff-filter/test17-v0.o",
    "data/test-diff-filter/test17-v1.o",
    "--no-default-suppression --no-show-locs --redundant",
    "data/test-diff-filter/test17-1-report.txt",
    "output/test-diff-filter/test17-1-report.txt",
  },
  {
    "data/test-diff-filter/test18-v0.o",
    "data/test-diff-filter/test18-v1.o",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-filter/test18-report.txt",
    "output/test-diff-filter/test18-report.txt",
  },
  {
    "data/test-diff-filter/test19-enum-v0.o",
    "data/test-diff-filter/test19-enum-v1.o",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-filter/test19-enum-report-0.txt",
    "output/test-diff-filter/test19-enum-report-0.txt",
  },
  {
    "data/test-diff-filter/test19-enum-v0.o",
    "data/test-diff-filter/test19-enum-v1.o",
    "--no-default-suppression --no-show-locs --harmless",
    "data/test-diff-filter/test19-enum-report-1.txt",
    "output/test-diff-filter/test19-enum-report-1.txt",
  },
  {
    "data/test-diff-filter/test20-inline-v0.o",
    "data/test-diff-filter/test20-inline-v1.o",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-filter/test20-inline-report-0.txt",
    "output/test-diff-filter/test20-inline-report-0.txt",
  },
  {
    "data/test-diff-filter/test20-inline-v0.o",
    "data/test-diff-filter/test20-inline-v1.o",
    "--no-default-suppression --no-show-locs --harmless",
    "data/test-diff-filter/test20-inline-report-1.txt",
    "output/test-diff-filter/test20-inline-report-1.txt",
  },
  {
    "data/test-diff-filter/libtest21-compatible-vars-v0.so",
    "data/test-diff-filter/libtest21-compatible-vars-v1.so",
    "--no-default-suppression --no-show-locs --harmless",
    "data/test-diff-filter/test21-compatible-vars-report-0.txt",
    "output/test-diff-filter/test21-compatible-vars-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest21-compatible-vars-v0.so",
    "data/test-diff-filter/libtest21-compatible-vars-v1.so",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-filter/test21-compatible-vars-report-1.txt",
    "output/test-diff-filter/test21-compatible-vars-report-1.txt",
  },
  {
    "data/test-diff-filter/libtest22-compatible-fns-v0.so",
    "data/test-diff-filter/libtest22-compatible-fns-v1.so",
    "--no-default-suppression --no-show-locs --harmless",
    "data/test-diff-filter/test22-compatible-fns-report-0.txt",
    "output/test-diff-filter/test22-compatible-fns-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest22-compatible-fns-v0.so",
    "data/test-diff-filter/libtest22-compatible-fns-v1.so",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-filter/test22-compatible-fns-report-1.txt",
    "output/test-diff-filter/test22-compatible-fns-report-1.txt",
  },
  {
    "data/test-diff-filter/libtest23-redundant-fn-parm-change-v0.so",
    "data/test-diff-filter/libtest23-redundant-fn-parm-change-v1.so",
    "--no-default-suppression --no-show-locs",
    "data/test-diff-filter/test23-redundant-fn-parm-change-report-0.txt ",
    "output/test-diff-filter/test23-redundant-fn-parm-change-report-0.txt ",
  },
  {
    "data/test-diff-filter/libtest24-compatible-vars-v0.so",
    "data/test-diff-filter/libtest24-compatible-vars-v1.so",
    "--no-default-suppression --no-show-locs",
    "data/test-diff-filter/test24-compatible-vars-report-0.txt ",
    "output/test-diff-filter/test24-compatible-vars-report-0.txt ",
  },
  {
    "data/test-diff-filter/libtest24-compatible-vars-v0.so",
    "data/test-diff-filter/libtest24-compatible-vars-v1.so",
    "--no-default-suppression --no-show-locs --harmless",
    "data/test-diff-filter/test24-compatible-vars-report-1.txt ",
    "output/test-diff-filter/test24-compatible-vars-report-1.txt ",
  },
  {
    "data/test-diff-filter/libtest25-cyclic-type-v0.so",
    "data/test-diff-filter/libtest25-cyclic-type-v1.so",
    "--no-default-suppression --no-show-locs",
    "data/test-diff-filter/test25-cyclic-type-report-0.txt ",
    "output/test-diff-filter/test25-cyclic-type-report-0.txt "
  },
  {
    "data/test-diff-filter/libtest25-cyclic-type-v0.so",
    "data/test-diff-filter/libtest25-cyclic-type-v1.so",
    "--no-default-suppression --no-show-locs --redundant",
    "data/test-diff-filter/test25-cyclic-type-report-1.txt ",
    "output/test-diff-filter/test25-cyclic-type-report-1.txt "
  },
  {
    "data/test-diff-filter/libtest26-qualified-redundant-node-v0.so",
    "data/test-diff-filter/libtest26-qualified-redundant-node-v1.so",
    "--no-default-suppression --no-show-locs",
    "data/test-diff-filter/test26-qualified-redundant-node-report-0.txt",
    "output/test-diff-filter/test26-qualified-redundant-node-report-0.txt"
  },
  {
    "data/test-diff-filter/libtest26-qualified-redundant-node-v0.so",
    "data/test-diff-filter/libtest26-qualified-redundant-node-v1.so",
    "--no-default-suppression --no-show-locs --redundant",
    "data/test-diff-filter/test26-qualified-redundant-node-report-1.txt",
    "output/test-diff-filter/test26-qualified-redundant-node-report-1.txt"
  },
  {
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v1.so",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-0.txt",
    "output/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-0.txt"
  },
  {
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v1.so",
    "--no-default-suppression --no-linkage-name --no-show-locs --redundant",
    "data/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-1.txt",
    "output/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-1.txt"
  },
  {
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v1.so",
    "--no-default-suppression --no-linkage-name --redundant --no-show-locs --harmless",
    "data/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-2.txt",
    "output/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-2.txt"
  },
  {
    "data/test-diff-filter/libtest28-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest28-redundant-and-filtered-children-nodes-v1.so",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
   "data/test-diff-filter/test28-redundant-and-filtered-children-nodes-report-0.txt",
    "output/test-diff-filter/test28-redundant-and-filtered-children-nodes-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest28-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest28-redundant-and-filtered-children-nodes-v1.so",
    "--no-default-suppression --no-linkage-name --redundant --no-show-locs --harmless",
   "data/test-diff-filter/test28-redundant-and-filtered-children-nodes-report-1.txt",
    "output/test-diff-filter/test28-redundant-and-filtered-children-nodes-report-1.txt",
  },
  {
    "data/test-diff-filter/test29-finer-redundancy-marking-v0.o",
    "data/test-diff-filter/test29-finer-redundancy-marking-v1.o",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test29-finer-redundancy-marking-report-0.txt",
    "output/test-diff-filter/test29-finer-redundancy-marking-report-0.txt",
  },
  {
    "data/test-diff-filter/test30-pr18904-rvalueref-liba.so",
    "data/test-diff-filter/test30-pr18904-rvalueref-libb.so",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test30-pr18904-rvalueref-report0.txt",
    "output/test-diff-filter/test30-pr18904-rvalueref-report0.txt",
  },
  { // Just like the previous test, but emit loc info.
    "data/test-diff-filter/test30-pr18904-rvalueref-liba.so",
    "data/test-diff-filter/test30-pr18904-rvalueref-libb.so",
    "--no-default-suppression --no-linkage-name --no-redundant",
    "data/test-diff-filter/test30-pr18904-rvalueref-report1.txt",
    "output/test-diff-filter/test30-pr18904-rvalueref-report1.txt",
  },
  { // Just like the previous test, but emit sizes in hex and bytes
    "data/test-diff-filter/test30-pr18904-rvalueref-liba.so",
    "data/test-diff-filter/test30-pr18904-rvalueref-libb.so",
    "--no-default-suppression --no-linkage-name --no-redundant "
    "--show-hex --show-bytes",
    "data/test-diff-filter/test30-pr18904-rvalueref-report2.txt",
    "output/test-diff-filter/test30-pr18904-rvalueref-report2.txt",
  },
  {
    "data/test-diff-filter/test31-pr18535-libstdc++-4.8.3.so",
    "data/test-diff-filter/test31-pr18535-libstdc++-4.9.2.so",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test31-pr18535-libstdc++-report-0.txt",
    "output/test-diff-filter/test31-pr18535-libstdc++-report-0.txt",
  },
  { // Just like the previous test, but emit loc info.
    "data/test-diff-filter/test31-pr18535-libstdc++-4.8.3.so",
    "data/test-diff-filter/test31-pr18535-libstdc++-4.9.2.so",
    "--no-default-suppression --no-linkage-name --no-redundant",
    "data/test-diff-filter/test31-pr18535-libstdc++-report-1.txt",
    "output/test-diff-filter/test31-pr18535-libstdc++-report-1.txt",
  },
  {
    "data/test-diff-filter/libtest32-struct-change-v0.so",
    "data/test-diff-filter/libtest32-struct-change-v1.so",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test32-ppc64le-struct-change-report0.txt",
    "output/test-diff-filter/test32-ppc64le-struct-change-report0.txt",
  },
  {
    "data/test-diff-filter/test33-libelf.so.0.8.13-gcc",
    "data/test-diff-filter/test33-libelf.so.0.8.13-intel16.0.3",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test33-report-0.txt",
    "output/test-diff-filter/test33-report-0.txt",
  },
  {
    "data/test-diff-filter/test34-libjemalloc.so.2-gcc-6.1.0",
    "data/test-diff-filter/test34-libjemalloc.so.2-intel-16.0.3",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant",
    "data/test-diff-filter/test34-report-0.txt",
    "output/test-diff-filter/test34-report-0.txt",
  },
  {
    "data/test-diff-filter/test30-pr18904-rvalueref-liba.so",
    "data/test-diff-filter/test30-pr18904-rvalueref-libb.so",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant --no-added-syms",
    "data/test-diff-filter/test35-pr18754-no-added-syms-report-0.txt",
    "output/test-diff-filter/test35-pr18754-no-added-syms-report-0.txt",
  },
  {
    "data/test-diff-filter/test30-pr18904-rvalueref-liba.so",
    "data/test-diff-filter/test30-pr18904-rvalueref-libb.so",
    "--no-default-suppression --no-linkage-name --no-show-locs --no-redundant --deleted-fns --changed-vars --no-added-syms",
    "data/test-diff-filter/test35-pr18754-no-added-syms-report-1.txt",
    "output/test-diff-filter/test35-pr18754-no-added-syms-report-1.txt",
  },
  {
    "data/test-diff-filter/libtest36-v0.so",
    "data/test-diff-filter/libtest36-v1.so",
    "--no-default-suppression --no-linkage-name",
    "data/test-diff-filter/test36-report-0.txt",
    "output/test-diff-filter/test36-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest37-v0.so",
    "data/test-diff-filter/libtest37-v1.so",
    "--no-default-suppression --no-linkage-name",
    "data/test-diff-filter/test37-report-0.txt",
    "output/test-diff-filter/test37-report-0.txt",
  },
  {
    "data/test-diff-filter/test38/test38-v0",
    "data/test-diff-filter/test38/test38-v1",
    "--no-default-suppression --no-linkage-name",
    "data/test-diff-filter/test38/test38-report-0.txt",
    "output/test-diff-filter/test38/test38-report-0.txt",
  },
  {
    "data/test-diff-filter/test39/test39-v0",
    "data/test-diff-filter/test39/test39-v1",
    "--no-default-suppression --no-linkage-name",
    "data/test-diff-filter/test39/test39-report-0.txt",
    "output/test-diff-filter/test39/test39-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest40-v0.so",
    "data/test-diff-filter/libtest40-v1.so",
    "--no-default-suppression --no-linkage-name",
    "data/test-diff-filter/test40-report-0.txt",
    "output/test-diff-filter/test40-report-0.txt",
  },
  {
    "data/test-diff-filter/test41-PR21486-abg-writer.gcc.o",
    "data/test-diff-filter/test41-PR21486-abg-writer.llvm.o",
    "--no-default-suppression",
    "data/test-diff-filter/test41-report-0.txt",
    "output/test-diff-filter/test41-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest42-leaf-report-v0.so",
    "data/test-diff-filter/libtest42-leaf-report-v1.so",
    "--no-default-suppression --leaf-changes-only --impacted-interfaces",
    "data/test-diff-filter/test42-leaf-report-output-0.txt",
    "output/test-diff-filter/test42-leaf-report-output-0.txt",
  },
  {
    "data/test-diff-filter/libtest43-decl-only-def-change-leaf-report-v0.so",
    "data/test-diff-filter/libtest43-decl-only-def-change-leaf-report-v1.so",
    "--no-default-suppression --leaf-changes-only",
    "data/test-diff-filter/test43-decl-only-def-change-leaf-report-0.txt",
    "output/test-diff-filter/test43-decl-only-def-change-leaf-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest44-anonymous-data-member-v0.so",
    "data/test-diff-filter/libtest44-anonymous-data-member-v1.so",
    "--no-default-suppression",
    "data/test-diff-filter/test44-anonymous-data-member-report-0.txt",
    "output/test-diff-filter/test44-anonymous-data-member-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest44-anonymous-data-member-v0.so",
    "data/test-diff-filter/libtest44-anonymous-data-member-v1.so",
    "--no-default-suppression --leaf-changes-only",
    "data/test-diff-filter/test44-anonymous-data-member-report-1.txt",
    "output/test-diff-filter/test44-anonymous-data-member-report-1.txt",
  },
  {
    "data/test-diff-filter/libtest45-basic-type-change-v0.so",
    "data/test-diff-filter/libtest45-basic-type-change-v1.so",
    "--no-default-suppression",
    "data/test-diff-filter/libtest45-basic-type-change-report-0.txt",
    "output/test-diff-filter/libtest45-basic-type-change-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest45-basic-type-change-v0.so",
    "data/test-diff-filter/libtest45-basic-type-change-v1.so",
    "--no-default-suppression --leaf-changes-only",
    "data/test-diff-filter/libtest45-basic-type-change-report-1.txt",
    "output/test-diff-filter/libtest45-basic-type-change-report-1.txt",
  },
  {
    "data/test-diff-filter/test46-fn-return-qual-change-v0.o",
    "data/test-diff-filter/test46-fn-return-qual-change-v1.o",
    "--no-default-suppression",
    "data/test-diff-filter/test46-fn-return-qual-change-report-0.txt",
    "output/test-diff-filter/test46-fn-return-qual-change-report-0.txt",
  },
  {
    "data/test-diff-filter/test47-filter-void-ptr-change-v0.o",
    "data/test-diff-filter/test47-filter-void-ptr-change-v1.o",
    "--no-default-suppression",
    "data/test-diff-filter/test47-filter-void-ptr-change-report-0.txt",
    "output/test-diff-filter/test47-filter-void-ptr-change-report-0.txt",
  },
  {
    "data/test-diff-filter/PR24430-fold-qualified-array-clang",
    "data/test-diff-filter/PR24430-fold-qualified-array-gcc",
    "--no-default-suppression",
    "data/test-diff-filter/PR24430-fold-qualified-array-report-0.txt",
    "output/test-diff-filter/PR24430-fold-qualified-array-report-0.txt",
  },
  {
    "data/test-diff-filter/test-PR24731-v0.o ",
    "data/test-diff-filter/test-PR24731-v1.o ",
    "--no-default-suppression",
    "data/test-diff-filter/test-PR24731-report-0.txt",
    "output/test-diff-filter/test-PR24731-report-0.txt",
  },
  {
    "data/test-diff-filter/test-PR24731-v0.o ",
    "data/test-diff-filter/test-PR24731-v1.o ",
    "--no-default-suppression --harmless",
    "data/test-diff-filter/test-PR24731-report-1.txt",
    "output/test-diff-filter/test-PR24731-report-1.txt",
  },
  {
    "data/test-diff-filter/PR24787-libone.so",
    "data/test-diff-filter/PR24787-libtwo.so",
    "--no-default-suppression",
    "data/test-diff-filter/PR24787-report-0.txt",
    "output/test-diff-filter/PR24787-report-0.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-1-v0.o",
   "data/test-diff-filter/test-PR25661-1-v1.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR25661-1-report-1.txt",
   "output/test-diff-filter/test-PR25661-1-report-1.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-1-v0.o",
   "data/test-diff-filter/test-PR25661-1-v1.o",
   "--no-default-suppression --harmless",
   "data/test-diff-filter/test-PR25661-1-report-2.txt",
   "output/test-diff-filter/test-PR25661-1-report-2.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-1-v0.o",
   "data/test-diff-filter/test-PR25661-1-v1.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-1-report-3.txt",
   "output/test-diff-filter/test-PR25661-1-report-3.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-1-v0.o",
   "data/test-diff-filter/test-PR25661-1-v1.o",
   "--no-default-suppression --harmless --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-1-report-4.txt",
   "output/test-diff-filter/test-PR25661-1-report-4.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-2-v0.o",
   "data/test-diff-filter/test-PR25661-2-v1.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR25661-2-report-1.txt",
   "output/test-diff-filter/test-PR25661-2-report-1.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-2-v0.o",
   "data/test-diff-filter/test-PR25661-2-v1.o",
   "--no-default-suppression --harmless",
   "data/test-diff-filter/test-PR25661-2-report-2.txt",
   "output/test-diff-filter/test-PR25661-2-report-2.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-2-v0.o",
   "data/test-diff-filter/test-PR25661-2-v1.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-2-report-3.txt",
   "output/test-diff-filter/test-PR25661-2-report-3.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-2-v0.o",
   "data/test-diff-filter/test-PR25661-2-v1.o",
   "--no-default-suppression --harmless --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-2-report-4.txt",
   "output/test-diff-filter/test-PR25661-2-report-4.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-3-v0.o",
   "data/test-diff-filter/test-PR25661-3-v1.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR25661-3-report-1.txt",
   "output/test-diff-filter/test-PR25661-3-report-1.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-3-v0.o",
   "data/test-diff-filter/test-PR25661-3-v1.o",
   "--no-default-suppression --harmless",
   "data/test-diff-filter/test-PR25661-3-report-2.txt",
   "output/test-diff-filter/test-PR25661-3-report-2.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-3-v0.o",
   "data/test-diff-filter/test-PR25661-3-v1.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-3-report-3.txt",
   "output/test-diff-filter/test-PR25661-3-report-3.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-3-v0.o",
   "data/test-diff-filter/test-PR25661-3-v1.o",
   "--no-default-suppression --harmless --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-3-report-4.txt",
   "output/test-diff-filter/test-PR25661-3-report-4.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-4-v0.o",
   "data/test-diff-filter/test-PR25661-4-v1.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR25661-4-report-1.txt",
   "output/test-diff-filter/test-PR25661-4-report-1.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-4-v0.o",
   "data/test-diff-filter/test-PR25661-4-v1.o",
   "--no-default-suppression --harmless",
   "data/test-diff-filter/test-PR25661-4-report-2.txt",
   "output/test-diff-filter/test-PR25661-4-report-2.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-4-v0.o",
   "data/test-diff-filter/test-PR25661-4-v1.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-4-report-3.txt",
   "output/test-diff-filter/test-PR25661-4-report-3.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-4-v0.o",
   "data/test-diff-filter/test-PR25661-4-v1.o",
   "--no-default-suppression --harmless --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-4-report-4.txt",
   "output/test-diff-filter/test-PR25661-4-report-4.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-5-v0.o",
   "data/test-diff-filter/test-PR25661-5-v1.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR25661-5-report-1.txt",
   "output/test-diff-filter/test-PR25661-5-report-1.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-5-v0.o",
   "data/test-diff-filter/test-PR25661-5-v1.o",
   "--no-default-suppression --harmless",
   "data/test-diff-filter/test-PR25661-5-report-2.txt",
   "output/test-diff-filter/test-PR25661-5-report-2.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-5-v0.o",
   "data/test-diff-filter/test-PR25661-5-v1.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-5-report-3.txt",
   "output/test-diff-filter/test-PR25661-5-report-3.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-5-v0.o",
   "data/test-diff-filter/test-PR25661-5-v1.o",
   "--no-default-suppression --harmless --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-5-report-4.txt",
   "output/test-diff-filter/test-PR25661-5-report-4.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-6-v0.o",
   "data/test-diff-filter/test-PR25661-6-v1.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR25661-6-report-1.txt",
   "output/test-diff-filter/test-PR25661-6-report-1.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-6-v0.o",
   "data/test-diff-filter/test-PR25661-6-v1.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-6-report-2.txt",
   "output/test-diff-filter/test-PR25661-6-report-2.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-6-v0.o",
   "data/test-diff-filter/test-PR25661-6-v1.o",
   "--no-default-suppression --harmless",
   "data/test-diff-filter/test-PR25661-6-report-3.txt",
   "output/test-diff-filter/test-PR25661-6-report-3.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-6-v0.o",
   "data/test-diff-filter/test-PR25661-6-v1.o",
   "--no-default-suppression --harmless --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-6-report-4.txt",
   "output/test-diff-filter/test-PR25661-6-report-4.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-7-v0.o",
   "data/test-diff-filter/test-PR25661-7-v1.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR25661-7-report-1.txt",
   "output/test-diff-filter/test-PR25661-7-report-1.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-7-v0.o",
   "data/test-diff-filter/test-PR25661-7-v1.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-7-report-2.txt",
   "output/test-diff-filter/test-PR25661-7-report-2.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-7-v0.o",
   "data/test-diff-filter/test-PR25661-7-v1.o",
   "--no-default-suppression --harmless",
   "data/test-diff-filter/test-PR25661-7-report-3.txt",
   "output/test-diff-filter/test-PR25661-7-report-3.txt",
  },
  {
   "data/test-diff-filter/test-PR25661-7-v0.o",
   "data/test-diff-filter/test-PR25661-7-v1.o",
   "--no-default-suppression --harmless --leaf-changes-only",
   "data/test-diff-filter/test-PR25661-7-report-4.txt",
   "output/test-diff-filter/test-PR25661-7-report-4.txt",
  },
  {
   "data/test-diff-filter/test-PR26309-v0.o",
   "data/test-diff-filter/test-PR26309-v1.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR26309-report-0.txt",
   "output/test-diff-filter/test-PR26309-report-0.txt",
  },
  {
   "data/test-diff-filter/test-PR26739-v0.o",
   "data/test-diff-filter/test-PR26739-v1.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR26739-report-0.txt",
   "output/test-diff-filter/test-PR26739-report-0.txt",
  },
  {
   "data/test-diff-filter/test-PR26739-2-v0.o",
   "data/test-diff-filter/test-PR26739-2-v1.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR26739-2-report-0.txt",
   "output/test-diff-filter/test-PR26739-2-report-0.txt",
  },
  {
   "data/test-diff-filter/test-PR26684-dwarf4.o",
   "data/test-diff-filter/test-PR26684-dwarf5.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR26684-report-0.txt",
   "output/test-diff-filter/test-PR26684-report-0.txt",
  },
  {
   "data/test-diff-filter/test-PR27331-v0.o",
   "data/test-diff-filter/test-PR27331-v1.o",
   "--no-default-suppression --leaf-changes-only",
   "data/test-diff-filter/test-PR27331-report-0.txt",
   "output/test-diff-filter/test-PR27331-report-0.txt",
  },
  {
   "data/test-diff-filter/test-PR27569-v0.abi",
   "data/test-diff-filter/test-PR27569-v1.abi",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR27569-report-0.txt",
   "output/test-diff-filter/test-PR27569-report-0.txt",
  },
    {
   "data/test-diff-filter/test-PR27598-v0.o",
   "data/test-diff-filter/test-PR27598-v0.o",
   "--no-default-suppression",
   "data/test-diff-filter/test-PR27598-report-0.txt",
   "output/test-diff-filter/test-PR27598-report-0.txt",
  },
  // This should be the last entry
  {NULL, NULL, NULL, NULL, NULL}
};

/// A task which launches abidiff on the binaries passed to the
/// constructor of the task.  The test also launches gnu diff on the
/// result of the abidiff to compare it against a reference abidiff
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
  /// It actually launches abidiff on the binaries passed to the
  /// constructor of the task.  It also launches gnu diff on the
  /// result of the abidiff to compare it against a reference abidiff
  /// result.
  virtual void
  perform()
  {
    using abigail::tests::get_src_dir;
    using abigail::tests::get_build_dir;
    using abigail::tools_utils::ensure_parent_dir_created;
    using abigail::tools_utils::abidiff_status;

    string in_elfv0_path, in_elfv1_path,
      abidiff_options, abidiff, cmd,
      ref_diff_report_path, out_diff_report_path;

    in_elfv0_path = string(get_src_dir()) + "/tests/" + spec.in_elfv0_path;
    in_elfv1_path = string(get_src_dir()) + "/tests/" + spec.in_elfv1_path;
    abidiff_options = spec.abidiff_options;
    ref_diff_report_path =
      string(get_src_dir()) + "/tests/" + spec.in_report_path;
    out_diff_report_path =
      string(get_build_dir()) + "/tests/" + spec.out_report_path;

    if (!ensure_parent_dir_created(out_diff_report_path))
      {
	error_message = string("could not create parent directory for ")
	  + out_diff_report_path;
	is_ok = false;
	return;
      }

    abidiff = string(get_build_dir()) + "/tools/abidiff";
    abidiff += " " + abidiff_options;

    cmd = abidiff + " " + in_elfv0_path + " " + in_elfv1_path;
    cmd += " > " + out_diff_report_path;

    bool abidiff_ok = true;
    int code = system(cmd.c_str());
    if (!WIFEXITED(code))
      abidiff_ok = false;
    else
      {
	abidiff_status status =
	  static_cast<abidiff_status>(WEXITSTATUS(code));
	if (abigail::tools_utils::abidiff_status_has_error(status))
	  abidiff_ok = false;
      }

    if (abidiff_ok)
      {
	cmd = "diff -u " + ref_diff_report_path
	  + " " + out_diff_report_path;

	string cmd_no_out = cmd + " > /dev/null";
	if (system(cmd_no_out.c_str()))
	  {
	    is_ok = false;
	    diff_cmd = cmd;
	  }
      }
    else
      is_ok = false;
  }
}; //end struct test_task.

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

  for (InOutSpec* s = in_out_specs; s->in_elfv0_path; ++s)
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

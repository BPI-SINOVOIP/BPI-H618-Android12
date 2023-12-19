// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program runs a diff between input dwarf files and compares
/// the resulting report with a reference report.  If the resulting
/// report is different from the reference report, the test has
/// failed.  Note that the comparison is done using the libabigail
/// library directly.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"
#include "abg-dwarf-reader.h"
#include "abg-comparison.h"

using std::string;
using std::ofstream;
using std::cerr;

/// This is an aggregate that specifies where a test shall get its
/// input from and where it shall write its ouput to.
struct InOutSpec
{
  const char* in_elfv0_path;
  const char* in_elfv1_path;
  const char* in_report_path;
  const char* out_report_path;
};// end struct InOutSpec

InOutSpec in_out_specs[] =
{
  {
    "data/test-diff-dwarf/test0-v0.o",
    "data/test-diff-dwarf/test0-v1.o",
    "data/test-diff-dwarf/test0-report.txt",
    "output/test-diff-dwarf/test0-report.txt"
  },
  {
    "data/test-diff-dwarf/test1-v0.o",
    "data/test-diff-dwarf/test1-v1.o",
    "data/test-diff-dwarf/test1-report.txt",
    "output/test-diff-dwarf/test1-report.txt"
  },
  {
    "data/test-diff-dwarf/test2-v0.o",
    "data/test-diff-dwarf/test2-v1.o",
    "data/test-diff-dwarf/test2-report.txt",
    "output/test-diff-dwarf/test2-report.txt"
  },
  {
    "data/test-diff-dwarf/test3-v0.o",
    "data/test-diff-dwarf/test3-v1.o",
    "data/test-diff-dwarf/test3-report.txt",
    "output/test-diff-dwarf/test3-report.txt"
  },
  {
    "data/test-diff-dwarf/test3-v0.o",
    "data/test-diff-dwarf/test3-v1.o",
    "data/test-diff-dwarf/test3-report.txt",
    "output/test-diff-dwarf/test3-report.txt"
  },
  {
    "data/test-diff-dwarf/test4-v0.o",
    "data/test-diff-dwarf/test4-v1.o",
    "data/test-diff-dwarf/test4-report.txt",
    "output/test-diff-dwarf/test4-report.txt"
  },
  {
    "data/test-diff-dwarf/test5-v0.o",
    "data/test-diff-dwarf/test5-v1.o",
    "data/test-diff-dwarf/test5-report.txt",
    "output/test-diff-dwarf/test5-report.txt"
  },
  {
    "data/test-diff-dwarf/test6-v0.o",
    "data/test-diff-dwarf/test6-v1.o",
    "data/test-diff-dwarf/test6-report.txt",
    "output/test-diff-dwarf/test6-report.txt"
  },
  {
    "data/test-diff-dwarf/test7-v0.o",
    "data/test-diff-dwarf/test7-v1.o",
    "data/test-diff-dwarf/test7-report.txt",
    "output/test-diff-dwarf/test7-report.txt"
  },
  {
    "data/test-diff-dwarf/test8-v0.o",
    "data/test-diff-dwarf/test8-v1.o",
    "data/test-diff-dwarf/test8-report.txt",
    "output/test-diff-dwarf/test8-report.txt"
  },
  {
    "data/test-diff-dwarf/libtest9-v0.so",
    "data/test-diff-dwarf/libtest9-v1.so",
    "data/test-diff-dwarf/test9-report.txt",
    "output/test-diff-dwarf/test9-report.txt"
  },
  {
    "data/test-diff-dwarf/test10-v0.o",
    "data/test-diff-dwarf/test10-v1.o",
    "data/test-diff-dwarf/test10-report.txt",
    "output/test-diff-dwarf/test10-report.txt"
  },
  {
    "data/test-diff-dwarf/test11-v0.o",
    "data/test-diff-dwarf/test11-v1.o",
    "data/test-diff-dwarf/test11-report.txt",
    "output/test-diff-dwarf/test11-report.txt"
  },
  {
    "data/test-diff-dwarf/libtest12-v0.so",
    "data/test-diff-dwarf/libtest12-v1.so",
    "data/test-diff-dwarf/test12-report.txt",
    "output/test-diff-dwarf/test12-report.txt"
  },
  {
    "data/test-diff-dwarf/test13-v0.o",
    "data/test-diff-dwarf/test13-v1.o",
    "data/test-diff-dwarf/test13-report.txt",
    "output/test-diff-dwarf/test13-report.txt"
  },
  {
    "data/test-diff-dwarf/test14-inline-v0.o",
    "data/test-diff-dwarf/test14-inline-v1.o",
    "data/test-diff-dwarf/test14-inline-report.txt",
    "output/test-diff-dwarf/test14-inline-report.txt"
  },
  {
    "data/test-diff-dwarf/test15-enum-v0.o",
    "data/test-diff-dwarf/test15-enum-v1.o",
    "data/test-diff-dwarf/test15-enum-report.txt",
    "output/test-diff-dwarf/test15-enum-report.txt"
  },
  {
    "data/test-diff-dwarf/test16-syms-only-v0.o",
    "data/test-diff-dwarf/test16-syms-only-v1.o",
    "data/test-diff-dwarf/test16-syms-only-report.txt",
    "output/test-diff-dwarf/test16-syms-only-report.txt"
  },
  {
    "data/test-diff-dwarf/test17-non-refed-syms-v0.o",
    "data/test-diff-dwarf/test17-non-refed-syms-v1.o",
    "data/test-diff-dwarf/test17-non-refed-syms-report-0.txt",
    "output/test-diff-dwarf/test17-non-refed-syms-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest18-alias-sym-v0.so",
    "data/test-diff-dwarf/libtest18-alias-sym-v1.so",
    "data/test-diff-dwarf/test18-alias-sym-report-0.txt",
    "output/test-diff-dwarf/test18-alias-sym-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest19-soname-v0.so",
    "data/test-diff-dwarf/libtest19-soname-v1.so",
    "data/test-diff-dwarf/test19-soname-report-0.txt",
    "output/test-diff-dwarf/test19-soname-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest20-add-fn-parm-v0.so",
    "data/test-diff-dwarf/libtest20-add-fn-parm-v1.so",
    "data/test-diff-dwarf/test20-add-fn-parm-report-0.txt",
    "output/test-diff-dwarf/test20-add-fn-parm-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest21-redundant-fn-v0.so",
    "data/test-diff-dwarf/libtest21-redundant-fn-v1.so",
    "data/test-diff-dwarf/test21-redundant-fn-report-0.txt",
    "output/test-diff-dwarf/test21-redundant-fn-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest22-changed-parm-c-v0.so",
    "data/test-diff-dwarf/libtest22-changed-parm-c-v1.so",
    "data/test-diff-dwarf/test22-changed-parm-c-report-0.txt",
    "output/test-diff-dwarf/test22-changed-parm-c-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest-23-diff-arch-v0-32.so",
    "data/test-diff-dwarf/libtest-23-diff-arch-v0-64.so",
    "data/test-diff-dwarf/test-23-diff-arch-report-0.txt",
    "output/test-diff-dwarf/test-23-diff-arch-report-0.txt"
  },
    {
    "data/test-diff-dwarf/libtest24-added-fn-parms-v0.so",
    "data/test-diff-dwarf/libtest24-added-fn-parms-v1.so",
    "data/test-diff-dwarf/test24-added-fn-parms-report-0.txt",
    "output/test-diff-dwarf/test24-added-fn-parms-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest25-removed-fn-parms-v0.so",
    "data/test-diff-dwarf/libtest25-removed-fn-parms-v1.so",
    "data/test-diff-dwarf/test25-removed-fn-parms-report-0.txt",
    "output/test-diff-dwarf/test25-removed-fn-parms-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest26-added-parms-before-variadic-v0.so",
    "data/test-diff-dwarf/libtest26-added-parms-before-variadic-v1.so",
    "data/test-diff-dwarf/test26-added-parms-before-variadic-report.txt",
    "output/test-diff-dwarf/test26-added-parms-before-variadic-report.txt"
  },
  {
    "data/test-diff-dwarf/test27-local-base-diff-v0.o",
    "data/test-diff-dwarf/test27-local-base-diff-v1.o",
    "data/test-diff-dwarf/test27-local-base-diff-report.txt",
    "output/test-diff-dwarf/test27-local-base-diff-report.txt"
  },
  {
    "data/test-diff-dwarf/test28-vtable-changes-v0.o",
    "data/test-diff-dwarf/test28-vtable-changes-v1.o",
    "data/test-diff-dwarf/test28-vtable-changes-report-0.txt",
    "output/test-diff-dwarf/test28-vtable-changes-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test29-vtable-changes-v0.o",
    "data/test-diff-dwarf/test29-vtable-changes-v1.o",
    "data/test-diff-dwarf/test29-vtable-changes-report-0.txt",
    "output/test-diff-dwarf/test29-vtable-changes-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test30-vtable-changes-v0.o",
    "data/test-diff-dwarf/test30-vtable-changes-v1.o",
    "data/test-diff-dwarf/test30-vtable-changes-report-0.txt",
    "output/test-diff-dwarf/test30-vtable-changes-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test31-vtable-changes-v0.o",
    "data/test-diff-dwarf/test31-vtable-changes-v1.o",
    "data/test-diff-dwarf/test31-vtable-changes-report-0.txt",
    "output/test-diff-dwarf/test31-vtable-changes-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test32-fnptr-changes-v0.o",
    "data/test-diff-dwarf/test32-fnptr-changes-v1.o",
    "data/test-diff-dwarf/test32-fnptr-changes-report-0.txt",
    "output/test-diff-dwarf/test32-fnptr-changes-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test33-fnref-changes-v0.o",
    "data/test-diff-dwarf/test33-fnref-changes-v1.o",
    "data/test-diff-dwarf/test33-fnref-changes-report-0.txt",
    "output/test-diff-dwarf/test33-fnref-changes-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test34-pr19173-libfoo.so",
    "data/test-diff-dwarf/test34-pr19173-libfoo2.so",
    "data/test-diff-dwarf/test34-pr19173-libfoo-report-0.txt",
    "output/test-diff-dwarf/test34-pr19173-libfoo-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test35-pr19173-libfoo-long-gcc.so",
    "data/test-diff-dwarf/test35-pr19173-libfoo-long-gcc2.so",
    "data/test-diff-dwarf/test35-pr19173-libfoo-long-gcc-report-0.txt",
    "output/test-diff-dwarf/test35-pr19173-libfoo-long-gcc-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test35-pr19173-libfoo-long-clang.so",
    "data/test-diff-dwarf/test35-pr19173-libfoo-long-clang2.so",
    "data/test-diff-dwarf/test35-pr19173-libfoo-long-clang-report-0.txt",
    "output/test-diff-dwarf/test35-pr19173-libfoo-long-clang-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest36-ppc64-aliases-v0.so",
    "data/test-diff-dwarf/libtest36-ppc64-aliases-v1.so",
    "data/test-diff-dwarf/test36-ppc64-aliases-report-0.txt",
    "output/test-diff-dwarf/test36-ppc64-aliases-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest37-union-v0.so",
    "data/test-diff-dwarf/libtest37-union-v1.so",
    "data/test-diff-dwarf/test37-union-report-0.txt",
    "output/test-diff-dwarf/test37-union-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest38-union-v0.so",
    "data/test-diff-dwarf/libtest38-union-v1.so",
    "data/test-diff-dwarf/test38-union-report-0.txt",
    "output/test-diff-dwarf/test38-union-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest39-union-v0.so",
    "data/test-diff-dwarf/libtest39-union-v1.so",
    "data/test-diff-dwarf/test39-union-report-0.txt",
    "output/test-diff-dwarf/test39-union-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest40-v0.so",
    "data/test-diff-dwarf/libtest40-v1.so",
    "data/test-diff-dwarf/test40-report-0.txt",
    "output/test-diff-dwarf/test40-report-0.txt"
  },
  {
    "data/test-diff-dwarf/libtest41-PR20476-hidden-old.so",
    "data/test-diff-dwarf/libtest41-PR20476-hidden-new.so",
    "data/test-diff-dwarf/test41-PR20476-hidden-report-0.txt",
    "output/test-diff-dwarf/test41-PR20476-hidden-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test42-PR21296-libgcc.so",
    "data/test-diff-dwarf/test42-PR21296-libclang.so",
    "data/test-diff-dwarf/test42-PR21296-clanggcc-report0.txt",
    "output/test-diff-dwarf/test42-PR21296-clanggcc-report0.txt"
  },
  {
    "data/test-diff-dwarf/libtest43-PR22913-v0.so",
    "data/test-diff-dwarf/libtest43-PR22913-v1.so",
    "data/test-diff-dwarf/test43-PR22913-report-0.txt",
    "output/test-diff-dwarf/test43-PR22913-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test44-anon-struct-union-v0.o",
    "data/test-diff-dwarf/test44-anon-struct-union-v1.o",
    "data/test-diff-dwarf/test44-anon-struct-union-report-0.txt",
    "output/test-diff-dwarf/test44-anon-struct-union-report-0.txt"
  },
  {
    "data/test-diff-dwarf/test45-anon-dm-change-v0.o",
    "data/test-diff-dwarf/test45-anon-dm-change-v1.o",
    "data/test-diff-dwarf/test45-anon-dm-change-report-0.txt",
    "output/test-diff-dwarf/test45-anon-dm-change-report-0.txt"
  },
#ifdef HAVE_DW_LANG_Rust_enumerator
  {
    "data/test-diff-dwarf/test46-rust-libone.so",
    "data/test-diff-dwarf/test46-rust-libtwo.so",
    "data/test-diff-dwarf/test46-rust-report-0.txt",
    "output/test-diff-dwarf/test46-rust-report-0.txt"
  },
#endif
  {
    "data/test-diff-dwarf/PR25058-liblttng-ctl2.10.so",
    "data/test-diff-dwarf/PR25058-liblttng-ctl.so",
    "data/test-diff-dwarf/PR25058-liblttng-ctl-report-1.txt",
    "output/test-diff-dwarf/PR25058-liblttng-ctl-report-1.txt"
  },
  // This should be the last entry
  {NULL, NULL, NULL, NULL}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;
  using abigail::dwarf_reader::read_corpus_from_elf;
  using abigail::comparison::compute_diff;
  using abigail::comparison::corpus_diff_sptr;
  using abigail::ir::environment;
  using abigail::ir::environment_sptr;
  using abigail::comparison::diff_context_sptr;
  using abigail::comparison::diff_context;

  bool is_ok = true;
  string in_elfv0_path, in_elfv1_path,
    ref_diff_report_path, out_diff_report_path;

  for (InOutSpec* s = in_out_specs; s->in_elfv0_path; ++s)
    {
      in_elfv0_path = string(get_src_dir()) + "/tests/" + s->in_elfv0_path;
      in_elfv1_path = string(get_src_dir()) + "/tests/" + s->in_elfv1_path;
      out_diff_report_path =
	string(get_build_dir()) + "/tests/" + s->out_report_path;

      if (!ensure_parent_dir_created(out_diff_report_path))
	{
	  cerr << "could not create parent directory for "
	       << out_diff_report_path;
	  is_ok = false;
	  continue;
	}

      abigail::dwarf_reader::status status =
	abigail::dwarf_reader::STATUS_UNKNOWN;

      environment_sptr env(new environment);
      std::vector<char**> di_roots;
      abigail::corpus_sptr corp0 =
	read_corpus_from_elf(in_elfv0_path,
			     /*debug_info_root_path=*/di_roots,
			     env.get(),
			     /*load_all_types=*/false,
			     status);

      abigail::corpus_sptr corp1 =
	read_corpus_from_elf(in_elfv1_path,
			     /*debug_info_root_path=*/di_roots,
			     env.get(),
			     /*load_all_types=*/false,
			     status);

      if (!corp0)
	{
	  cerr << "failed to read " << in_elfv0_path << "\n";
	  is_ok = false;
	  continue;
	}

      if (!corp1)
	{
	  cerr << "failed to read " << in_elfv1_path << "\n";
	  is_ok = false;
	  continue;
	}

      corp0->set_path(s->in_elfv0_path);
      corp1->set_path(s->in_elfv1_path);

      diff_context_sptr ctxt(new diff_context);
      ctxt->show_locs(false);

      corpus_diff_sptr d = compute_diff(corp0, corp1, ctxt);
      if (!d)
	{
	  cerr << "failed to compute diff\n";
	  is_ok = false;
	  continue;
	}

      ref_diff_report_path =
	string(get_src_dir()) + "/tests/" + s->in_report_path;
      out_diff_report_path =
	string(get_build_dir()) + "/tests/" + s->out_report_path;

      ofstream of(out_diff_report_path.c_str(), std::ios_base::trunc);
      if (!of.is_open())
	{
	  cerr << "failed to read " << out_diff_report_path << "\n";
	  is_ok = false;
	  continue;
	}

      if (d->has_changes())
	d->report(of);
      of.close();

      string cmd =
	"diff -u " + ref_diff_report_path + " " + out_diff_report_path;
      if (system(cmd.c_str()))
	is_ok = false;
    }

  return !is_ok;
}

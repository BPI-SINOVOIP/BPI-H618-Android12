// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program tests the ELF symbols lookup APIs from
/// abigail::dwarf_reader.  It uses the lookupsym tool from the
/// libabigail distribution.

#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::cerr;
using std::string;

struct InOutSpec
{
  const char* in_elf_path;
  const char* symbol;
  const char* abisym_options;
  const char* in_report_path;
  const char* out_report_path;
}; // end struct InOutSpec

InOutSpec in_out_specs[] =
{
  {
    "data/test-lookup-syms/test0.o",
    "main",
    "",
    "data/test-lookup-syms/test0-report.txt",
    "output/test-lookup-syms/test0-report.txt"
  },
  {
    "data/test-lookup-syms/test0.o",
    "foo",
    "",
    "data/test-lookup-syms/test01-report.txt",
    "output/test-lookup-syms/test01-report.txt"
  },
  {
    "data/test-lookup-syms/test0.o",
    "\"bar(char)\"",
    "--demangle",
    "data/test-lookup-syms/test02-report.txt",
    "output/test-lookup-syms/test02-report.txt"
  },
  {
    "data/test-lookup-syms/test1.so",
    "foo",
    "",
    "data/test-lookup-syms/test1-1-report.txt",
    "output/test-lookup-syms/test1-1-report.txt"
  },
  {
    "data/test-lookup-syms/test1-32bits.so",
    "foo",
    "",
    "data/test-lookup-syms/test1-1-report.txt",
    "output/test-lookup-syms/test1-1-report.txt"
  },
  {
    "data/test-lookup-syms/test1.so",
    "_foo1",
    "--no-absolute-path",
    "data/test-lookup-syms/test1-2-report.txt",
    "output/test-lookup-syms/test-2-report.txt"
  },
  {
    "data/test-lookup-syms/test1.so",
    "_foo2",
    "--no-absolute-path",
    "data/test-lookup-syms/test1-3-report.txt",
    "output/test-lookup-syms/test-3-report.txt"
  },
  // This should always be the last entry.
  {NULL, NULL, NULL, NULL, NULL}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;

  bool is_ok = true;
  string in_elf_path, symbol, abisym, abisym_options,
    ref_report_path, out_report_path;

  for (InOutSpec* s = in_out_specs; s->in_elf_path; ++s)
    {
      in_elf_path = string(get_src_dir()) + "/tests/" + s->in_elf_path;
      symbol = s->symbol;
      abisym_options = s->abisym_options;
      ref_report_path = string(get_src_dir()) + "/tests/" + s->in_report_path;
      out_report_path =
	string(get_build_dir()) + "/tests/" + s->out_report_path;

      if (!ensure_parent_dir_created(out_report_path))
	{
	  cerr << "could not create parent directory for "
	       << out_report_path;
	  is_ok = false;
	  continue;
	}

      abisym = string(get_build_dir()) + "/tools/abisym";
      abisym += " " + abisym_options;

      string cmd = abisym + " " + in_elf_path + " " + symbol;
      cmd += " > " + out_report_path;

      bool abisym_ok = true;
      if (system(cmd.c_str()))
	abisym_ok = false;

      if (abisym_ok)
	{
	  cmd = "diff -u " + ref_report_path + " "+  out_report_path;
	  if (system(cmd.c_str()))
	    is_ok = false;
	}
      else
	is_ok = false;
    }

  return !is_ok;
}

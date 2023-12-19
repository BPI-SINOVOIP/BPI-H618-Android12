// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Author: Ondrej Oprala

/// @file
///
/// This program tests the annotation capabilities of the library.

#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::cerr;
using std::string;

struct InOutSpec
{
  const char* in_elf_path;
  const char* in_report_path;
  const char* out_report_path;
};

InOutSpec in_out_specs[] =
{
  {
    "data/test-read-dwarf/test0",
    "data/test-annotate/test0.abi",
    "output/test-annotate/test0.abi"
  },
  {
    "data/test-read-dwarf/test1",
    "data/test-annotate/test1.abi",
    "output/test-annotate/test1.abi"
  },
  {
    "data/test-read-dwarf/test2.so",
    "data/test-annotate/test2.so.abi",
    "output/test-annotate/test2.so.abi"
  },
  {
    "data/test-read-dwarf/test3.so",
    "data/test-annotate/test3.so.abi",
    "output/test-annotate/test3.so.abi"
  },
  {
    "data/test-read-dwarf/test4.so",
    "data/test-annotate/test4.so.abi",
    "output/test-annotate/test4.so.abi"
  },
  {
    "data/test-read-dwarf/test5.o",
    "data/test-annotate/test5.o.abi",
    "output/test-annotate/test5.o.abi"
  },
  {
    "data/test-read-dwarf/test6.so",
    "data/test-annotate/test6.so.abi",
    "output/test-annotate/test6.so.abi"
  },
  {
    "data/test-read-dwarf/test7.so",
    "data/test-annotate/test7.so.abi",
    "output/test-annotate/test7.so.abi"
  },
  {
    "data/test-read-dwarf/test8-qualified-this-pointer.so",
    "data/test-annotate/test8-qualified-this-pointer.so.abi",
    "output/test-annotate/test8-qualified-this-pointer.so.abi"
  },
  {
    "data/test-read-dwarf/test13-pr18894.so",
    "data/test-annotate/test13-pr18894.so.abi",
    "output/test-annotate/test13-pr18894.so.abi",
  },
  {
    "data/test-read-dwarf/test14-pr18893.so",
    "data/test-annotate/test14-pr18893.so.abi",
    "output/test-annotate/test14-pr18893.so.abi",
  },
  {
    "data/test-read-dwarf/test15-pr18892.so",
    "data/test-annotate/test15-pr18892.so.abi",
    "output/test-annotate/test15-pr18892.so.abi",
  },
  {
    "data/test-read-dwarf/test17-pr19027.so",
    "data/test-annotate/test17-pr19027.so.abi",
    "output/test-annotate/test17-pr19027.so.abi",
  },
  {
    "data/test-read-dwarf/test18-pr19037-libvtkRenderingLIC-6.1.so",
    "data/test-annotate/test18-pr19037-libvtkRenderingLIC-6.1.so.abi",
    "output/test-annotate/test18-pr19037-libvtkRenderingLIC-6.1.so.abi",
  },
  {
    "data/test-read-dwarf/test19-pr19023-libtcmalloc_and_profiler.so",
    "data/test-annotate/test19-pr19023-libtcmalloc_and_profiler.so.abi",
    "output/test-annotate/test19-pr19023-libtcmalloc_and_profiler.so.abi",
  },
  {
    "data/test-read-dwarf/test20-pr19025-libvtkParallelCore-6.1.so",
    "data/test-annotate/test20-pr19025-libvtkParallelCore-6.1.so.abi",
    "output/test-annotate/test20-pr19025-libvtkParallelCore-6.1.so.abi",
  },
  {
    "data/test-read-dwarf/test21-pr19092.so",
    "data/test-annotate/test21-pr19092.so.abi",
    "output/test-annotate/test21-pr19092.so.abi",
  },
  {
    "data/test-read-dwarf/libtest23.so",
    "data/test-annotate/libtest23.so.abi",
    "output/test-annotate/libtest23.so.abi",
  },
  {
    "data/test-read-dwarf/libtest24-drop-fns.so",
    "data/test-annotate/libtest24-drop-fns.so.abi",
    "output/test-annotate/libtest24-drop-fns.so.abi",
  },
  {
    "data/test-read-dwarf/libtest24-drop-fns.so",
    "data/test-annotate/libtest24-drop-fns-2.so.abi",
    "output/test-annotate/libtest24-drop-fns-2.so.abi",
  },
  {
    "data/test-annotate/test-anonymous-members-0.o",
    "data/test-annotate/test-anonymous-members-0.o.abi",
    "output/test-annotate/test-anonymous-members-0.o.abi",
  },
  // This should be the last entry.
  {NULL, NULL, NULL}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;

  bool is_ok = true;
  string in_elf_path, ref_report_path, out_report_path;
  string abidw;

  abidw = string(get_build_dir()) + "/tools/abidw "
    "--annotate --no-corpus-path";
  for (InOutSpec* s = in_out_specs; s->in_elf_path; ++s)
    {
      in_elf_path = string(get_src_dir()) + "/tests/" + s->in_elf_path;
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

      string cmd = abidw + " " + in_elf_path + " > " + out_report_path;

      bool abidw_ok = true;
      if (system(cmd.c_str()))
	abidw_ok = false;

      if (abidw_ok)
	{
	  cmd = "diff -u " + ref_report_path + " " + out_report_path;
	  if (system(cmd.c_str()))
	    is_ok &=false;
	}
      else
	{
	  cerr << "command failed: " << cmd << "\n";
	  is_ok &= false;
	}
    }

  return !is_ok;
}

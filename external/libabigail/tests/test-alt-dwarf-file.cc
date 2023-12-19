// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program tests that libabigail can handle alternate debug info
/// files as specified by http://www.dwarfstd.org/ShowIssue.php?issue=120604.1.

#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::cerr;
using std::string;

struct InOutSpec
{
  const char* in_elf_path;
  const char* debug_info_dir_path;
  const char* abidw_options;
  const char* in_report_path;
  const char* out_report_path;
};


InOutSpec in_out_specs[] =
{
  {
    "data/test-alt-dwarf-file/libtest0.so",
    "data/test-alt-dwarf-file/test0-debug-dir",
    "--check-alternate-debug-info-base-name",
    "data/test-alt-dwarf-file/test0-report.txt",
    "output/test-alt-dwarf-file/test0-report.txt"
  },
  {
    "data/test-alt-dwarf-file/libtest0-common.so",
    "data/test-alt-dwarf-file/test0-debug-dir",
    "--check-alternate-debug-info-base-name",
    "data/test-alt-dwarf-file/test0-report.txt",
    "output/test-alt-dwarf-file/test0-report.txt"
  },
  {
    "data/test-alt-dwarf-file/test1-libgromacs_d.so.0.0.0",
    "data/test-alt-dwarf-file/test1-libgromacs-debug-dir",
    "--noout --check-alternate-debug-info",
    "data/test-alt-dwarf-file/test1-report-0.txt",
    "output/test-alt-dwarf-file/test1-report-0.txt"
  },

  // This should always be the last entry
  {NULL, NULL, NULL, NULL, NULL}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;

  bool is_ok = true;
  string in_elf_path, ref_report_path, out_report_path, debug_info_dir;
  string abidw, abidw_options;

  abidw = string(get_build_dir()) + "/tools/abidw";
  for (InOutSpec* s = in_out_specs; s->in_elf_path; ++s)
    {
      abidw_options = s->abidw_options;
      in_elf_path = string(get_src_dir()) + "/tests/" + s->in_elf_path;
      debug_info_dir =
	string(get_src_dir()) + "/tests/" + s->debug_info_dir_path;
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

      string cmd = abidw + " --debug-info-dir " + debug_info_dir
	+ " " + abidw_options + " " + in_elf_path + " > " + out_report_path;

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

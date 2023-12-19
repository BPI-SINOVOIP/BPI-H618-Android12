// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file compare the ABI of an an elf binary and an abixml file.

#include <sys/wait.h>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"


using std::string;
using std::ofstream;
using std::cerr;
using abigail::tests::get_build_dir;

/// Specifies where a test should get its inputs from, and where it
/// should write its output to.
struct InOutSpec
{
  const char* in_elf_path;
  const char* in_abi_path;
  const char* in_report_path;
  const char* out_report_path;
};

InOutSpec in_out_specs[] =
{
  {
    "data/test-diff-dwarf-abixml/test0-pr19026-libvtkIOSQL-6.1.so.1",
    "data/test-diff-dwarf-abixml/test0-pr19026-libvtkIOSQL-6.1.so.1.abi",
    "data/test-diff-dwarf-abixml/test0-pr19026-libvtkIOSQL-6.1.so.1-report-0.txt",
    "output/test-diff-dwarf-abixml/test0-pr19026-libvtkIOSQL-6.1.so.1-report-0.txt",
  },
  {
    "data/test-diff-dwarf-abixml/PR25409-librte_bus_dpaa.so.20.0",
    "data/test-diff-dwarf-abixml/PR25409-librte_bus_dpaa.so.20.0.abi",
    "data/test-diff-dwarf-abixml/PR25409-librte_bus_dpaa.so.20.0-report-0.txt",
    "output/test-diff-dwarf-abixml/PR25409-librte_bus_dpaa.so.20.0-report-0.txt"
  },
  // This should be the last entry
  {0, 0, 0, 0}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;
  using abigail::tools_utils::abidiff_status;

  bool is_ok = true;
  string in_elf_path, in_abi_path,
    abidiff, cmd, ref_diff_report_path, out_diff_report_path;

  for (InOutSpec* s = in_out_specs; s->in_elf_path; ++s)
    {
      in_elf_path = string(get_src_dir()) + "/tests/" + s->in_elf_path;
      in_abi_path = string(get_src_dir()) + "/tests/"+ s->in_abi_path;
      ref_diff_report_path =
	string(get_src_dir()) + "/tests/" + s->in_report_path;
      out_diff_report_path =
	string(get_build_dir()) + "/tests/" + s->out_report_path;

      if (!ensure_parent_dir_created(out_diff_report_path))
	{
	  cerr << "could not create parent directory for "
	       << out_diff_report_path;
	  is_ok = false;
	  continue;
	}

      abidiff = string(get_build_dir()) + "/tools/abidiff";
      cmd = abidiff + " --no-default-suppression --no-architecture "
	+ in_elf_path + " " + in_abi_path;
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
	  if (system(cmd.c_str()))
	    is_ok = false;
	}
      else
	is_ok = false;
    }

  return !is_ok;
}

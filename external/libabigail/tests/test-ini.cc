// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This test harness program runs a diff between the output of
/// abinilint on an ini file and a reference expected output.

#include <sys/wait.h>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::string;
using std::cerr;

/// This is an aggregate that specifies where a test shall get its
/// input from and where it shall write its ouput to.
struct InOutSpec
{
  // Where to read the ini file from, with abinilint.
  const char* in_ini_path;
  // Where to get the expected output from abinilint.
  const char* in_reference_output_path;
  // Where to emit the output of abinilint to.
  const char* out_init_path;
  // The options to use with abinilint.
  const char* abinilint_options;
}; // end struct InOutSpec;

// An array of test specifications listing the ini files to read and
// their expected output.
InOutSpec in_out_specs[] =
{
  {
    "data/test-ini/test01-equal-in-property-string.abignore",
    "data/test-ini/test01-equal-in-property-string.abignore.expected",
    "output/test-ini/test01-equal-in-property-string.abignore",
    ""
  }
  ,
  // This one must always remain the last one.
  {0, 0, 0, 0}
};

/// @return the test source directory.
static string
get_test_src_dir()
{
  using abigail::tests::get_src_dir;
  return string(get_src_dir()) + "/tests";
}

/// @return the test build directory.
static string
get_test_build_dir()
{
  using abigail::tests::get_build_dir;
  return string(get_build_dir()) + "/tests";
}

/// @return the tools directory under the build directory;
static string
get_tools_build_dir()
{
  using abigail::tests::get_build_dir;
  return string(get_build_dir()) + "/tools";
}

int
main()
{
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;
  using abigail::tools_utils::abidiff_status;

  bool is_ok = true;
  string in_ini_path, in_reference_output_path, out_ini_path, cmd;

  for (InOutSpec *s = in_out_specs; s->in_ini_path; ++s)
    {
      in_ini_path = get_test_src_dir() + "/" + s->in_ini_path;
      in_reference_output_path =
	get_test_src_dir() + "/" + s->in_reference_output_path;
      out_ini_path = get_test_build_dir() + "/" + s->out_init_path;

      if (!ensure_parent_dir_created(out_ini_path))
	{
	  cerr << "could not create parent directory for "
	       << out_ini_path;
	  is_ok = false;
	  continue;
	}

      cmd = get_tools_build_dir() + "/abinilint";
      if (s->abinilint_options && strcmp(s->abinilint_options, ""))
	cmd += " " + string(s->abinilint_options);

      cmd += " " + in_ini_path + " > " + out_ini_path;

      bool cmd_is_ok = true;
      int code = system(cmd.c_str());
      if (!WIFEXITED(code))
	 cmd_is_ok = false;
      else
	{
	  abigail::tools_utils::abidiff_status status =
	    static_cast<abidiff_status>(WEXITSTATUS(code));
	  if (abigail::tools_utils::abidiff_status_has_error(status))
	    cmd_is_ok = false;
	}

      if (cmd_is_ok)
	{
	  cmd = "diff -u " + in_reference_output_path + " " + out_ini_path;
	  if (system(cmd.c_str()))
	    is_ok = false;
	}
      else
	is_ok = false;
    }

  return !is_ok;
}

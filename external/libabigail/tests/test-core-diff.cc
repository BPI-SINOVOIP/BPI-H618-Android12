// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "abg-diff-utils.h"
#include "test-utils.h"

using std::string;
using std::ofstream;
using std::cerr;

/// This structure describes the set of strings we want to diff
/// against each other as well as the reports we expect from these
/// diffs.
struct SESInOutSpec
{
  // This is the path to the report we expect from the diff algorithm
  // applied to second_string (below) diffed against first_string.
  const char* in_path;
  // This is the path where to store the report of the diff algorithm
  // applied to second_string (below) diffed against first_string.
  const char* out_path;
  // This is the first string to feed the diff algorithm with.
  const char* first_string;
  // This is the second string to feed the diff algorithm with.
  const char* second_string;
};// end struct SESInOutSpec

SESInOutSpec in_out_specs[] =
{
  {
    "data/test-core-diff/report0.txt",
    "output/test-core-diff/report0.txt",
    "abcabba",
    "cbabac"
  },
  {
    "data/test-core-diff/report1.txt",
    "output/test-core-diff/report1.txt",
    "xxabxx",
    "xbx"
  },
  {
    "data/test-core-diff/report2.txt",
    "output/test-core-diff/report2.txt",
    "xxabxx",
    "xbcx"
  },
  {
    "data/test-core-diff/report3.txt",
    "output/test-core-diff/report3.txt",
    "abc",
    "abdecfgc"
  },
  {
    "data/test-core-diff/report4.txt",
    "output/test-core-diff/report4.txt",
    "xxx",
    "xxx"
  },
  {
    "data/test-core-diff/report5.txt",
    "output/test-core-diff/report5.txt",
    "xabx",
    "xbx"
  },
  {
    "data/test-core-diff/report6.txt",
    "output/test-core-diff/report6.txt",
    "fou",
    "fubar"
  },
  {
    "data/test-core-diff/report7.txt",
    "output/test-core-diff/report7.txt",
    "sqkdjfjdsql",
    "sqdmlkjfmljdsqf"
  },
  {
    "data/test-core-diff/report8.txt",
    "output/test-core-diff/report8.txt",
    "abcdef",
    "bcghai"
  },
  {
    "data/test-core-diff/report9.txt",
    "output/test-core-diff/report9.txt",
    "abcdef",
    "bjcghai"
  },
  {
    "data/test-core-diff/report10.txt",
    "output/test-core-diff/report10.txt",
    "a",
    "ab"
  },
  {
    "data/test-core-diff/report11.txt",
    "output/test-core-diff/report11.txt",
    "a",
    "abd"
  },
  {
    "data/test-core-diff/report12.txt",
    "output/test-core-diff/report12.txt",
    "a",
    "cba"
  },
  {
    "data/test-core-diff/report13.txt",
    "output/test-core-diff/report13.txt",
    "abcdefghi",
    "jklmnopqrstadubvfwxgh"
  },
  // This should be the last entry.
  {NULL, NULL, NULL, NULL}
};

using abigail::diff_utils::edit_script;
using abigail::diff_utils::compute_ses;
using abigail::diff_utils::display_edit_script;

int
main()
{
  unsigned result = 1;

  bool problem = false;
  string in_path, out_path;

  for (SESInOutSpec *s = in_out_specs; s->in_path; ++s)
    {
      string input_suffix(s->in_path);
      in_path =
	string(abigail::tests::get_src_dir()) + "/tests/" + input_suffix;

      string output_suffix(s->out_path);
      out_path =
	string(abigail::tests::get_build_dir()) + "/tests/" + output_suffix;

      if (!abigail::tools_utils::ensure_parent_dir_created(out_path))
	{
	  cerr << "Could not create parent director for " << out_path;
	  problem = true;
	  return result;
	}

      ofstream of(out_path.c_str(), std::ios_base::trunc);
      if (!of.is_open())
	{
	  cerr << "failed to read " << out_path << "\n";
	  problem = true;
	  continue;
	}

      edit_script ses;
      // Compute the Shortest Edit Script (aka diff) that changes
      // s->first_string into s->second_string.
      compute_ses(s->first_string, s->second_string, ses);
      // Emit a report about that edit script
      display_edit_script(ses, s->first_string, s->second_string, of);
      of.close();
      // Diff that report against what we expect.
      string cmd = "diff -u " + in_path + " " + out_path;
      if (system(cmd.c_str()))
	problem= true;
    }

  return problem;
}

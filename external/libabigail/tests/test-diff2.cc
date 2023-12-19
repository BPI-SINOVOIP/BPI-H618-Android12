// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file implements a simple command line utility for
/// interactively testing the diff2 algorithms declared and defined in
/// abg-diff-utils.{h,cc}
///
/// The resulting binary name is testdiff2.  Run it to see a help message
/// showing you how to use it.

#include <cstring>
#include <iostream>
#include "abg-diff-utils.h"

using std::cout;
using std::string;

using abigail::diff_utils::ses_len;
using abigail::diff_utils::point;
using abigail::diff_utils::snake;
using abigail::diff_utils::compute_middle_snake;
using abigail::diff_utils::print_snake;
using abigail::diff_utils::compute_lcs;
using abigail::diff_utils::edit_script;
using abigail::diff_utils::compute_ses;
using abigail::diff_utils::display_edit_script;

struct options
{
  bool show_help;
  bool ses_len;
  bool reverse;
  bool middle_snake;
  bool lcs;
  bool ses;
  const char* str1;
  const char* str2;

  options()
    : show_help(false),
      ses_len(false),
      reverse(false),
      middle_snake(false),
      lcs(false),
      ses(false),
      str1(0),
      str2(0)
  {}
};// end struct options

static void
show_help(const string& progname)
{
   cout << "usage: " << progname << " [options] str1 str2\n"
	<< "where [options] can be:\n"
	<< "--seslen	print the length of the SES of the two strings\n"
	<< "--reverse	compute the d-paths in reverse order when applicable\n"
	<< "--middle-snake	display middle snake & length of SES\n"
	<< "--lcs	display the longest common subsequence\n"
	<< "--ses	display the shortest edit script transforming str1 into str2\n";
}

static void
parse_command_line(int argc, char* argv[], options &opts)
{
  if (argc < 3)
    {
      opts.show_help = true;
      return;
    }

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (!opts.str1)
	    opts.str1 = argv[i];
	  else if (!opts.str2)
	    opts.str2 = argv[i];
	  else
	    {
	      opts.show_help = true;
	      return;
	    }
	}
      else if (strcmp(argv[i], "--seslen") == 0)
	opts.ses_len = true;
      else if (strcmp(argv[i], "--reverse") == 0)
	opts.reverse = true;
      else if (strcmp(argv[i], "--middle-snake") == 0)
	opts.middle_snake = true;
      else if (strcmp(argv[i], "--lcs") == 0)
	opts.lcs = true;
      else if (strcmp(argv[i], "--ses") == 0)
	opts.ses = true;
      else
	opts.show_help = true;
    }
}

int
main(int argc, char*argv[])
{
  options opts;
  parse_command_line(argc, argv, opts);

  if (opts.show_help)
    {
      show_help(argv[0]);
      return -1;
    }

  if (opts.ses_len)
    {
      int len = ses_len(opts.str1, opts.str2, opts.reverse);
      cout << len << "\n";
      return 0;
    }

  if (opts.middle_snake)
    {
      int ses_len = 0;
      snake s;
      if (compute_middle_snake(opts.str1, opts.str2,
			       s, ses_len))
	{
	  print_snake(opts.str1, opts.str2, s, cout);
	  cout << "ses len: " << ses_len << "\n";
	}
      return 0;
    }

  if (opts.lcs)
    {
      string lcs;
      int ses_len = 0;
      compute_lcs(opts.str1, opts.str2, ses_len, lcs);
      cout << "lcs: " << lcs << "\n"
	   << "ses len: " << ses_len << "\n";
      return 0;
    }

  if (opts.ses)
    {
      edit_script ses;
      compute_ses(opts.str1, opts.str2, ses);
      display_edit_script(ses, opts.str1, opts.str2, cout);
      return 0;
    }

  return 0;
}

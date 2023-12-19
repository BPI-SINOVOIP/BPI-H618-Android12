// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This is a tool that reads an ini file and, if it could read it OK,
/// prints it on its standard output.  It's mainly meant to test the
/// abigail::ini::* functions, but one could also use it to make sure
/// that an ini file can be handled by the abigail::ini::* facilities

#include <cstring>
#include <iostream>
#include "abg-ini.h"

using std::cout;
using std::cerr;
using std::cin;
using std::string;
using std::ostream;
using abigail::ini::config;
using abigail::ini::config_sptr;
using abigail::ini::read_config;
using abigail::ini::write_config;

struct options
{
  bool display_usage;
  bool read_from_stdin;
  bool no_out;
  bool wrong_command_line_usage;
  string path;

  options ()
    : display_usage(false),
      read_from_stdin(false),
      no_out(false),
      wrong_command_line_usage(false)
  {}
};

static void
display_usage(const string& prog_name, ostream& out)
{
  out << "usage: " << prog_name << " [options] <ini file>\n"
      << "where options can be:\n"
      << "--help  display this help\n"
      << "--from-stdin  read the input ini file from stdin\n"
      << "--noout  do not output anything on stdout\n"
    ;
}

static bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc == 1)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (opts.path.empty())
	    opts.path = argv[i];
	  else
	    opts.wrong_command_line_usage = true;
	}
      else if (!strcmp(argv[i], "--help"))
	opts.display_usage = true;
      else if (!strcmp(argv[i], "--from-stdin"))
	opts.read_from_stdin = true;
      else if (!strcmp(argv[i], "--noout"))
	opts.no_out = true;
      else
	return false;
    }
  return true;
}

int
main(int argc, char* argv[])
{
  // First, parse the command line.

  options opts;

  if (argc == 1 || !parse_command_line(argc, argv, opts))
    {
      cerr << argv[0] << ": bad command usage\n"
	   << "Try " << argv[0] << " --help for more information\n";
      return 1;
    }

  // Then if we need to display the help, display it and get out.

  if (opts.display_usage)
    {
      display_usage(argv[0], cout);
      return 0;
    }

  // Otherwise, do the real work we are supposed to do after all.
  // That real work is driven by the options the user set; these
  // options are recorded in the opts variable.

  config_sptr conf;

  if (opts.read_from_stdin)
    conf = read_config(cin);
  else if (!opts.path.empty())
    conf = read_config(opts.path);

  if (conf && !opts.no_out)
    write_config(*conf, std::cout);

  return !conf;
}

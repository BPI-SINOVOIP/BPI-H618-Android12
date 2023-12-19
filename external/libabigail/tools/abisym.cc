// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program takes parameters to open an elf file, lookup a symbol
/// in its symbol tables and report what it sees.

#include <elf.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include "abg-config.h"
#include "abg-dwarf-reader.h"
#include "abg-ir.h"
#include "abg-tools-utils.h"

using std::cout;
using std::cerr;
using std::string;
using std::ostream;
using std::ostringstream;
using std::vector;

using abigail::ir::environment;
using abigail::ir::environment_sptr;
using abigail::dwarf_reader::lookup_symbol_from_elf;
using abigail::elf_symbol;
using abigail::elf_symbol_sptr;

struct options
{
  bool	show_help;
  bool	display_version;
  char* elf_path;
  char* symbol_name;
  bool	demangle;
  bool absolute_path;

  options()
    : show_help(false),
      display_version(false),
      elf_path(0),
      symbol_name(0),
      demangle(false),
      absolute_path(true)
  {}
};

static void
display_usage(const string& prog_name, ostream &out)
{
  out << "usage: " << prog_name << " [options] <elf file> <symbol-name>\n"
      << "where [options] can be:\n"
      << "  --help  display this help string\n"
      << "  --version|-v  display program version information and exit\n"
      << "  --demangle  demangle the symbols from the symbol table\n"
      << "  --no-absolute-path  do not show absolute paths in messages\n";
}

static void
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    {
      opts.show_help = true;
      return;
    }

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (!opts.elf_path)
	    opts.elf_path = argv[i];
	  else if (!opts.symbol_name)
	    opts.symbol_name = argv[i] ;
	  else
	    {
	      opts.show_help = true;
	      return;
	    }
	}
      else if (!strcmp(argv[i], "--help")
	       || !strcmp(argv[i], "-h"))
	{
	  opts.show_help = true;
	  return;
	}
      else if (!strcmp(argv[i], "--version")
	       || !strcmp(argv[i], "-v"))
	{
	  opts.display_version = true;
	  return;
	}
      else if (!strcmp(argv[i], "--demangle"))
	opts.demangle = true;
      else if (!strcmp(argv[i], "--no-absolute-path"))
	opts.absolute_path = false;
      else
	opts.show_help = true;
    }
}

int
main(int argc, char* argv[])
{
  options opts;
  parse_command_line(argc, argv, opts);

  if (opts.show_help)
    {
      display_usage(argv[0], cout);
      return 1;
    }

  if (opts.display_version)
    {
      abigail::tools_utils::emit_prefix(argv[0], cout)
	<< abigail::tools_utils::get_library_version_string();
      return 0;
    }

  assert(opts.elf_path != 0
	 && opts.symbol_name != 0);

  string p = opts.elf_path, n = opts.symbol_name;
  environment_sptr env(new environment);
  vector<elf_symbol_sptr> syms;
  if (!lookup_symbol_from_elf(env.get(), p, n, opts.demangle, syms))
    {
      cout << "could not find symbol '"
	   << opts.symbol_name
	   << "' in file '";
      if (opts.absolute_path)
	cout << opts.elf_path << "'\n";
      else
	cout << basename(opts.elf_path);
      return 0;
    }

  elf_symbol_sptr sym = syms[0];
  cout << "found symbol '" << n << "'";
  if (n != sym->get_name())
    cout << " (" << sym->get_name() << ")";
  cout << ", an instance of "
       << (elf_symbol::type) sym->get_type()
       << " of " << sym->get_binding();
  if (syms.size() > 1 || !sym->get_version().is_empty())
    {
      cout << ", of version";
      if (syms.size () > 1)
	cout << "s";
      cout << " ";
      for (vector<elf_symbol_sptr>::const_iterator i = syms.begin();
	   i != syms.end();
	   ++i)
	{
	  if (i != syms.begin())
	    cout << ", ";
	  cout << "'" << (*i)->get_version().str() << "'";
	}
    }
  cout << '\n';

  return 0;
}

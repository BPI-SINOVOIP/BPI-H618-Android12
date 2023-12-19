// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2021 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This is a program aimed at checking that a binary instrumentation
/// (bi) file is well formed and valid enough.  It acts by loading an
/// input bi file and saving it back to a temporary file.  It then
/// runs a diff on the two files and expects the result of the diff to
/// be empty.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "abg-config.h"
#include "abg-tools-utils.h"
#include "abg-ir.h"
#include "abg-corpus.h"
#include "abg-reader.h"
#include "abg-dwarf-reader.h"
#include "abg-writer.h"
#include "abg-suppression.h"

using std::string;
using std::cerr;
using std::cin;
using std::cout;
using std::ostream;
using std::ofstream;
using std::vector;
using abigail::tools_utils::emit_prefix;
using abigail::tools_utils::check_file;
using abigail::tools_utils::file_type;
using abigail::tools_utils::guess_file_type;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::read_suppressions;
using abigail::corpus;
using abigail::corpus_sptr;
using abigail::xml_reader::read_translation_unit_from_file;
using abigail::xml_reader::read_translation_unit_from_istream;
using abigail::xml_reader::read_corpus_from_native_xml;
using abigail::xml_reader::read_corpus_from_native_xml_file;
using abigail::xml_reader::read_corpus_group_from_input;
using abigail::dwarf_reader::read_corpus_from_elf;
using abigail::xml_writer::write_translation_unit;
using abigail::xml_writer::write_context_sptr;
using abigail::xml_writer::create_write_context;
using abigail::xml_writer::write_corpus;
using abigail::xml_writer::write_corpus_to_archive;

struct options
{
  string			wrong_option;
  string			file_path;
  bool				display_version;
  bool				read_from_stdin;
  bool				read_tu;
  bool				diff;
  bool				noout;
  std::shared_ptr<char>	di_root_path;
  vector<string>		suppression_paths;
  string			headers_dir;
  vector<string>		header_files;

  options()
    : display_version(false),
      read_from_stdin(false),
      read_tu(false),
      diff(false),
      noout(false)
  {}
};//end struct options;

static void
display_usage(const string& prog_name, ostream& out)
{
  emit_prefix(prog_name, out)
    << "usage: " << prog_name << " [options] [<abi-file1>]\n"
    << " where options can be:\n"
    << "  --help  display this message\n"
    << "  --version|-v  display program version information and exit\n"
    << "  --debug-info-dir <path> the path under which to look for "
    << "  --headers-dir|--hd <patch> the path to headers of the elf file\n"
    "debug info for the elf <abi-file>\n"
    << "  --header-file|--hf <path> the path to one header of the elf file\n"
    "debug info for the elf <abi-file>\n"
    << "  --suppressions|--suppr <path> specify a suppression file\n"
    << "  --diff  for xml inputs, perform a text diff between "
    "the input and the memory model saved back to disk\n"
    << "  --noout  do not display anything on stdout\n"
    << "  --stdin|--  read abi-file content from stdin\n"
    << "  --tu  expect a single translation unit file\n";
}

bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    {
      opts.read_from_stdin = true;
      return true;
    }

    for (int i = 1; i < argc; ++i)
      {
	if (argv[i][0] != '-')
	  {
	    if (opts.file_path.empty())
	      opts.file_path = argv[i];
	    else
	      return false;
	  }
	else if (!strcmp(argv[i], "--help"))
	  return false;
	else if (!strcmp(argv[i], "--version")
		 || !strcmp(argv[i], "-v"))
	  {
	    opts.display_version = true;
	    return true;
	  }
	else if (!strcmp(argv[i], "--debug-info-dir"))
	  {
	    if (argc <= i + 1
		|| argv[i + 1][0] == '-')
	      return false;
	    // elfutils wants the root path to the debug info to be
	    // absolute.
	    opts.di_root_path =
	      abigail::tools_utils::make_path_absolute(argv[i + 1]);
	    ++i;
	  }
      else if (!strcmp(argv[i], "--headers-dir")
	       || !strcmp(argv[i], "--hd"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.headers_dir = argv[j];
	  ++i;
	}
      else if (!strcmp(argv[i], "--header-file")
	       || !strcmp(argv[i], "--hf"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.header_files.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--suppressions")
	       || !strcmp(argv[i], "--suppr"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.suppression_paths.push_back(argv[j]);
	  ++i;
	}
	else if (!strcmp(argv[i], "--stdin"))
	  opts.read_from_stdin = true;
	else if (!strcmp(argv[i], "--tu"))
	  opts.read_tu = true;
	else if (!strcmp(argv[i], "--diff"))
	  opts.diff = true;
	else if (!strcmp(argv[i], "--noout"))
	  opts.noout = true;
	else
	  {
	    if (strlen(argv[i]) >= 2 && argv[i][0] == '-' && argv[i][1] == '-')
	      opts.wrong_option = argv[i];
	    return false;
	  }
      }

    if (opts.file_path.empty())
      opts.read_from_stdin = true;
    return true;
}

/// Check that the suppression specification files supplied are
/// present.  If not, emit an error on stderr.
///
/// @param opts the options instance to use.
///
/// @return true if all suppression specification files are present,
/// false otherwise.
static bool
maybe_check_suppression_files(const options& opts)
{
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    if (!check_file(*i, cerr, "abidiff"))
      return false;

  return true;
}

/// Set suppression specifications to the @p read_context used to load
/// the ABI corpus from the ELF/DWARF file.
///
/// These suppression specifications are going to be applied to drop
/// some ABI artifacts on the floor (while reading the ELF/DWARF file
/// or the native XML ABI file) and thus minimize the size of the
/// resulting ABI corpus.
///
/// @param read_ctxt the read context to apply the suppression
/// specifications to.  Note that the type of this parameter is
/// generic (class template) because in practise, it can be either an
/// abigail::dwarf_reader::read_context type or an
/// abigail::xml_reader::read_context type.
///
/// @param opts the options where to get the suppression
/// specifications from.
template<class ReadContextType>
static void
set_suppressions(ReadContextType& read_ctxt, const options& opts)
{
  suppressions_type supprs;
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    read_suppressions(*i, supprs);

  suppression_sptr suppr =
    abigail::tools_utils::gen_suppr_spec_from_headers(opts.headers_dir,
						      opts.header_files);
  if (suppr)
    supprs.push_back(suppr);

  add_read_context_suppressions(read_ctxt, supprs);
}

/// Reads a bi (binary instrumentation) file, saves it back to a
/// temporary file and run a diff on the two versions.
int
main(int argc, char* argv[])
{
  options opts;
  if (!parse_command_line(argc, argv, opts))
    {
      if (!opts.wrong_option.empty())
	emit_prefix(argv[0], cerr)
	  << "unrecognized option: " << opts.wrong_option << "\n";
      display_usage(argv[0], cerr);
      return 1;
    }

  if (opts.display_version)
    {
      emit_prefix(argv[0], cout)
	<< abigail::tools_utils::get_library_version_string();
      return 0;
    }

  if (!maybe_check_suppression_files(opts))
    return 1;

  abigail::ir::environment_sptr env(new abigail::ir::environment);
  if (opts.read_from_stdin)
    {
      if (!cin.good())
	return 1;

      if (opts.read_tu)
	{
	  abigail::translation_unit_sptr tu =
	    read_translation_unit_from_istream(&cin, env.get());

	  if (!tu)
	    {
	      emit_prefix(argv[0], cerr)
		<< "failed to read the ABI instrumentation from stdin\n";
	      return 1;
	    }

	  if (!opts.noout)
	    {
	      const write_context_sptr& ctxt
		  = create_write_context(tu->get_environment(), cout);
	      write_translation_unit(*ctxt, *tu, 0);
	    }
	  return 0;
	}
      else
	{
	  abigail::xml_reader::read_context_sptr ctxt =
	    abigail::xml_reader::create_native_xml_read_context(&cin,
								env.get());
	  assert(ctxt);
	  set_suppressions(*ctxt, opts);
	  corpus_sptr corp = abigail::xml_reader::read_corpus_from_input(*ctxt);
	  if (!opts.noout)
	    {
	      const write_context_sptr& ctxt
		  = create_write_context(corp->get_environment(), cout);
	      write_corpus(*ctxt, corp, /*indent=*/0);
	    }
	  return 0;
	}
    }
  else if (!opts.file_path.empty())
    {
      if (!check_file(opts.file_path, cerr, argv[0]))
	return 1;
      abigail::translation_unit_sptr tu;
      abigail::corpus_sptr corp;
      abigail::corpus_group_sptr group;
      abigail::dwarf_reader::status s = abigail::dwarf_reader::STATUS_OK;
      char* di_root_path = 0;
      file_type type = guess_file_type(opts.file_path);

      switch (type)
	{
	case abigail::tools_utils::FILE_TYPE_UNKNOWN:
	  emit_prefix(argv[0], cerr)
	    << "Unknown file type given in input: " << opts.file_path;
	  return 1;
	case abigail::tools_utils::FILE_TYPE_NATIVE_BI:
	  tu = read_translation_unit_from_file(opts.file_path, env.get());
	  break;
	case abigail::tools_utils::FILE_TYPE_ELF:
	case abigail::tools_utils::FILE_TYPE_AR:
	  {
	    di_root_path = opts.di_root_path.get();
	    vector<char**> di_roots;
	    di_roots.push_back(&di_root_path);
	    abigail::dwarf_reader::read_context_sptr ctxt =
	      abigail::dwarf_reader::create_read_context(opts.file_path,
							 di_roots, env.get(),
							 /*load_all_types=*/false);
	    assert(ctxt);
	    set_suppressions(*ctxt, opts);
	    corp = read_corpus_from_elf(*ctxt, s);
	  }
	  break;
	case abigail::tools_utils::FILE_TYPE_XML_CORPUS:
	  {
	    abigail::xml_reader::read_context_sptr ctxt =
	      abigail::xml_reader::create_native_xml_read_context(opts.file_path,
								  env.get());
	    assert(ctxt);
	    set_suppressions(*ctxt, opts);
	    corp = read_corpus_from_input(*ctxt);
	    break;
	  }
	case abigail::tools_utils::FILE_TYPE_XML_CORPUS_GROUP:
	  {
	    abigail::xml_reader::read_context_sptr ctxt =
	      abigail::xml_reader::create_native_xml_read_context(opts.file_path,
								  env.get());
	    assert(ctxt);
	    set_suppressions(*ctxt, opts);
	    group = read_corpus_group_from_input(*ctxt);
	  }
	  break;
	case abigail::tools_utils::FILE_TYPE_RPM:
	  break;
	case abigail::tools_utils::FILE_TYPE_SRPM:
	  break;
	case abigail::tools_utils::FILE_TYPE_DEB:
	  break;
	case abigail::tools_utils::FILE_TYPE_DIR:
	  break;
	case abigail::tools_utils::FILE_TYPE_TAR:
	  break;
	}

      if (!tu && !corp && !group)
	{
	  emit_prefix(argv[0], cerr)
	    << "failed to read " << opts.file_path << "\n";
	  if (!(s & abigail::dwarf_reader::STATUS_OK))
	    {
	      if (s & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
		{
		  cerr << "could not find the debug info";
		  if(di_root_path == 0)
		    emit_prefix(argv[0], cerr)
		      << " Maybe you should consider using the "
		      "--debug-info-dir1 option to tell me about the "
		      "root directory of the debuginfo? "
		      "(e.g, --debug-info-dir1 /usr/lib/debug)\n";
		  else
		    emit_prefix(argv[0], cerr)
		      << "Maybe the root path to the debug "
		      "information is wrong?\n";
		}
	      if (s & abigail::dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
		emit_prefix(argv[0], cerr)
		  << "could not find the ELF symbols in the file "
		  << opts.file_path
		  << "\n";
	    }
	  return 1;
	}

      using abigail::tools_utils::temp_file;
      using abigail::tools_utils::temp_file_sptr;

      temp_file_sptr tmp_file = temp_file::create();
      if (!tmp_file)
	{
	  emit_prefix(argv[0], cerr) << "failed to create temporary file\n";
	  return 1;
	}

      std::ostream& of = opts.diff ? tmp_file->get_stream() : cout;
      const abigail::ir::environment* env = 0;
      if (tu)
	env = tu->get_environment();
      else if (corp)
	env = corp->get_environment();
      else if (group)
	env = group->get_environment();

      ABG_ASSERT(env);
      const write_context_sptr ctxt = create_write_context(env, of);

      bool is_ok = true;

      if (tu)
	{
	  if (!opts.noout)
	    is_ok = write_translation_unit(*ctxt, *tu, 0);
	}
      else
	{
	  if (type == abigail::tools_utils::FILE_TYPE_XML_CORPUS
	      ||type == abigail::tools_utils::FILE_TYPE_XML_CORPUS_GROUP
	      || type == abigail::tools_utils::FILE_TYPE_ELF)
	    {
	      if (!opts.noout)
		is_ok = write_corpus(*ctxt, corp, 0);
	    }
	}

      if (!is_ok)
	{
	  string output =
	    (type == abigail::tools_utils::FILE_TYPE_NATIVE_BI)
	    ? "translation unit"
	    : "ABI corpus";
	  emit_prefix(argv[0], cerr)
	    << "failed to write the translation unit "
	    << opts.file_path << " back\n";
	}

      if (is_ok
	  && opts.diff
	  && ((type == abigail::tools_utils::FILE_TYPE_XML_CORPUS)
	      ||type == abigail::tools_utils::FILE_TYPE_XML_CORPUS_GROUP
	      || type == abigail::tools_utils::FILE_TYPE_NATIVE_BI))
	{
	  string cmd = "diff -u " + opts.file_path + " " + tmp_file->get_path();
	  if (system(cmd.c_str()))
	    is_ok = false;
	}

      return is_ok ? 0 : 1;
    }

  return 1;
}

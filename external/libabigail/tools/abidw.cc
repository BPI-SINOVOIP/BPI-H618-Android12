// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program reads an elf file, try to load its debug info (in
/// DWARF format) and emit it back in a set of "text sections" in native
/// libabigail XML format.

#include <unistd.h>
#include <cassert>
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
#include "abg-corpus.h"
#include "abg-dwarf-reader.h"
#include "abg-writer.h"
#include "abg-reader.h"
#include "abg-comparison.h"

using std::string;
using std::cerr;
using std::cout;
using std::ostream;
using std::ofstream;
using std::vector;
using std::shared_ptr;
using abigail::tools_utils::emit_prefix;
using abigail::tools_utils::temp_file;
using abigail::tools_utils::temp_file_sptr;
using abigail::tools_utils::check_file;
using abigail::tools_utils::build_corpus_group_from_kernel_dist_under;
using abigail::tools_utils::timer;
using abigail::ir::environment_sptr;
using abigail::ir::environment;
using abigail::corpus;
using abigail::corpus_sptr;
using abigail::translation_units;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::read_suppressions;
using abigail::comparison::corpus_diff;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::comparison::diff_context_sptr;
using abigail::comparison::diff_context;
using abigail::xml_writer::SEQUENCE_TYPE_ID_STYLE;
using abigail::xml_writer::HASH_TYPE_ID_STYLE;
using abigail::xml_writer::create_write_context;
using abigail::xml_writer::type_id_style_kind;
using abigail::xml_writer::write_context_sptr;
using abigail::xml_writer::write_corpus;
using abigail::xml_reader::read_corpus_from_native_xml_file;
using abigail::dwarf_reader::read_context;
using abigail::dwarf_reader::read_context_sptr;
using abigail::dwarf_reader::read_corpus_from_elf;
using abigail::dwarf_reader::create_read_context;
using namespace abigail;

struct options
{
  string		wrong_option;
  string		in_file_path;
  string		out_file_path;
  vector<char*>	di_root_paths;
  vector<char**>	prepared_di_root_paths;
  vector<string>	headers_dirs;
  vector<string>	header_files;
  string		vmlinux;
  vector<string>	suppression_paths;
  vector<string>	kabi_whitelist_paths;
  suppressions_type	kabi_whitelist_supprs;
  bool			display_version;
  bool			check_alt_debug_info_path;
  bool			show_base_name_alt_debug_info_path;
  bool			write_architecture;
  bool			write_corpus_path;
  bool			write_comp_dir;
  bool			write_elf_needed;
  bool			write_parameter_names;
  bool			short_locs;
  bool			default_sizes;
  bool			load_all_types;
  bool			linux_kernel_mode;
  bool			corpus_group_for_linux;
  bool			show_stats;
  bool			noout;
  bool			show_locs;
  bool			abidiff;
  bool			annotate;
  bool			do_log;
  bool			drop_private_types;
  bool			drop_undefined_syms;
  bool			merge_translation_units;
  type_id_style_kind	type_id_style;

  options()
    : display_version(),
      check_alt_debug_info_path(),
      show_base_name_alt_debug_info_path(),
      write_architecture(true),
      write_corpus_path(true),
      write_comp_dir(true),
      write_elf_needed(true),
      write_parameter_names(true),
      short_locs(false),
      default_sizes(true),
      load_all_types(),
      linux_kernel_mode(true),
      corpus_group_for_linux(false),
      show_stats(),
      noout(),
      show_locs(true),
      abidiff(),
      annotate(),
      do_log(),
      drop_private_types(false),
      drop_undefined_syms(false),
      merge_translation_units(false),
      type_id_style(SEQUENCE_TYPE_ID_STYLE)
  {}

  ~options()
  {
    for (vector<char*>::iterator i = di_root_paths.begin();
	 i != di_root_paths.end();
	 ++i)
      free(*i);

    prepared_di_root_paths.clear();
  }
};

static void
display_usage(const string& prog_name, ostream& out)
{
  emit_prefix(prog_name, out)
    << "usage: " << prog_name << " [options] [<path-to-elf-file>]\n"
    << " where options can be: \n"
    << "  --help|-h  display this message\n"
    << "  --version|-v  display program version information and exit\n"
    << "  --debug-info-dir|-d <dir-path>  look for debug info under 'dir-path'\n"
    << "  --headers-dir|--hd <path> the path to headers of the elf file\n"
    << "  --header-file|--hf <path> the path one header of the elf file\n"
    << "  --out-file <file-path>  write the output to 'file-path'\n"
    << "  --noout  do not emit anything after reading the binary\n"
    << "  --suppressions|--suppr <path> specify a suppression file\n"
    << "  --no-architecture  do not emit architecture info in the output\n"
    << "  --no-corpus-path  do not take the path to the corpora into account\n"
    << "  --no-show-locs  do not show location information\n"
    << "  --short-locs  only print filenames rather than paths\n"
    << "  --drop-private-types  drop private types from representation\n"
    << "  --drop-undefined-syms  drop undefined symbols from representation\n"
    << "  --merge-translation-units  merge translation units for same language\n"
    << "  --no-comp-dir-path  do not show compilation path information\n"
    << "  --no-elf-needed  do not show the DT_NEEDED information\n"
    << "  --no-write-default-sizes  do not emit pointer size when it equals"
    " the default address size of the translation unit\n"
    << "  --no-parameter-names  do not show names of function parameters\n"
    << "  --type-id-style <sequence|hash>  type id style (sequence(default): "
       "\"type-id-\" + number; hash: hex-digits)\n"
    << "  --check-alternate-debug-info <elf-path>  check alternate debug info "
    "of <elf-path>\n"
    << "  --check-alternate-debug-info-base-name <elf-path>  check alternate "
    "debug info of <elf-path>, and show its base name\n"
    << "  --load-all-types  read all types including those not reachable from "
    "exported declarations\n"
    << "  --no-linux-kernel-mode  don't consider the input binary as "
       "a Linux Kernel binary\n"
    << "  --kmi-whitelist|-w  path to a linux kernel "
    "abi whitelist\n"
    << "  --linux-tree|--lt  emit the ABI for the union of a "
    "vmlinux and its modules\n"
    << "  --vmlinux <path>  the path to the vmlinux binary to consider to emit "
       "the ABI of the union of vmlinux and its modules\n"
    << "  --abidiff  compare the loaded ABI against itself\n"
    << "  --annotate  annotate the ABI artifacts emitted in the output\n"
    << "  --stats  show statistics about various internal stuff\n"
    << "  --verbose show verbose messages about internal stuff\n";
  ;
}

static bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (opts.in_file_path.empty())
	    opts.in_file_path = argv[i];
	  else
	    return false;
	}
      else if (!strcmp(argv[i], "--version")
	       || !strcmp(argv[i], "-v"))
	opts.display_version = true;
      else if (!strcmp(argv[i], "--debug-info-dir")
	       || !strcmp(argv[i], "-d"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-')
	    return false;
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.di_root_paths.push_back
	    (abigail::tools_utils::make_path_absolute_to_be_freed(argv[i + 1]));
	  ++i;
	}
      else if (!strcmp(argv[i], "--headers-dir")
	       || !strcmp(argv[i], "--hd"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.headers_dirs.push_back(argv[j]);
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
      else if (!strcmp(argv[i], "--out-file"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-'
	      || !opts.out_file_path.empty())
	    return false;

	  opts.out_file_path = argv[i + 1];
	  ++i;
	}
      else if (!strcmp(argv[i], "--suppressions")
	       || !strcmp(argv[i], "--suppr"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.suppression_paths.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--kmi-whitelist")
	       || !strcmp(argv[i], "-w"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.kabi_whitelist_paths.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--linux-tree")
	       || !strcmp(argv[i], "--lt"))
	opts.corpus_group_for_linux = true;
      else if (!strcmp(argv[i], "--vmlinux"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.vmlinux = argv[j];
	  ++i;
	}
      else if (!strcmp(argv[i], "--noout"))
	opts.noout = true;
      else if (!strcmp(argv[i], "--no-architecture"))
	opts.write_architecture = false;
      else if (!strcmp(argv[i], "--no-corpus-path"))
	opts.write_corpus_path = false;
      else if (!strcmp(argv[i], "--no-show-locs"))
	opts.show_locs = false;
      else if (!strcmp(argv[i], "--short-locs"))
	opts.short_locs = true;
      else if (!strcmp(argv[i], "--no-comp-dir-path"))
	opts.write_comp_dir = false;
      else if (!strcmp(argv[i], "--no-elf-needed"))
	opts.write_elf_needed = false;
      else if (!strcmp(argv[i], "--no-write-default-sizes"))
	opts.default_sizes = false;
      else if (!strcmp(argv[i], "--no-parameter-names"))
	opts.write_parameter_names = false;
      else if (!strcmp(argv[i], "--type-id-style"))
        {
          ++i;
          if (i >= argc)
            return false;
          if (!strcmp(argv[i], "sequence"))
            opts.type_id_style = SEQUENCE_TYPE_ID_STYLE;
          else if (!strcmp(argv[i], "hash"))
            opts.type_id_style = HASH_TYPE_ID_STYLE;
          else
            return false;
        }
      else if (!strcmp(argv[i], "--check-alternate-debug-info")
	       || !strcmp(argv[i], "--check-alternate-debug-info-base-name"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-'
	      || !opts.in_file_path.empty())
	    return false;
	  if (!strcmp(argv[i], "--check-alternate-debug-info-base-name"))
	    opts.show_base_name_alt_debug_info_path = true;
	  opts.check_alt_debug_info_path = true;
	  opts.in_file_path = argv[i + 1];
	  ++i;
	}
      else if (!strcmp(argv[i], "--load-all-types"))
	opts.load_all_types = true;
      else if (!strcmp(argv[i], "--drop-private-types"))
	opts.drop_private_types = true;
      else if (!strcmp(argv[i], "--drop-undefined-syms"))
	opts.drop_undefined_syms = true;
      else if (!strcmp(argv[i], "--merge-translation-units"))
	opts.merge_translation_units = true;
      else if (!strcmp(argv[i], "--no-linux-kernel-mode"))
	opts.linux_kernel_mode = false;
      else if (!strcmp(argv[i], "--abidiff"))
	opts.abidiff = true;
      else if (!strcmp(argv[i], "--annotate"))
	opts.annotate = true;
      else if (!strcmp(argv[i], "--stats"))
	opts.show_stats = true;
      else if (!strcmp(argv[i], "--verbose"))
	opts.do_log = true;
      else if (!strcmp(argv[i], "--help")
	       || !strcmp(argv[i], "--h"))
	return false;
      else
	{
	  if (strlen(argv[i]) >= 2 && argv[i][0] == '-' && argv[i][1] == '-')
	    opts.wrong_option = argv[i];
	  return false;
	}
    }

  return true;
}

/// Initialize the context use for driving ABI comparison.
///
/// @param ctxt the context to initialize.
static void
set_diff_context(diff_context_sptr& ctxt)
{
  ctxt->default_output_stream(&cerr);
  ctxt->error_output_stream(&cerr);
  // Filter out changes that are not meaningful from an ABI
  // standpoint, from the diff output.
  ctxt->switch_categories_off
    (abigail::comparison::ACCESS_CHANGE_CATEGORY
     | abigail::comparison::COMPATIBLE_TYPE_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_DECL_NAME_CHANGE_CATEGORY);
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
    if (!check_file(*i, cerr, "abidw"))
      return false;

  for (vector<string>::const_iterator i =
	 opts.kabi_whitelist_paths.begin();
       i != opts.kabi_whitelist_paths.end();
       ++i)
    if (!check_file(*i, cerr, "abidw"))
      return false;

  return true;
}

/// Check that the header files supplied are present.
/// If not, emit an error on stderr.
///
/// @param opts the options instance to use.
///
/// @return true if all header files are present, false otherwise.
static bool
maybe_check_header_files(const options& opts)
{
  for (vector<string>::const_iterator file = opts.header_files.begin();
       file != opts.header_files.end();
       ++file)
    if (!check_file(*file, cerr, "abidw"))
      return false;

  return true;
}

/// Set suppression specifications to the @p read_context used to load
/// the ABI corpus from the ELF/DWARF file.
///
/// These suppression specifications are going to be applied to drop
/// some ABI artifacts on the floor (while reading the ELF/DWARF file)
/// and thus minimize the size of the resulting ABI corpus.
///
/// @param read_ctxt the read context to apply the suppression
/// specifications to.
///
/// @param opts the options where to get the suppression
/// specifications from.
static void
set_suppressions(read_context& read_ctxt, options& opts)
{
  suppressions_type supprs;
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    read_suppressions(*i, supprs);

  suppression_sptr suppr =
    abigail::tools_utils::gen_suppr_spec_from_headers(opts.headers_dirs,
						      opts.header_files);
  if (suppr)
    {
      if (opts.drop_private_types)
	suppr->set_drops_artifact_from_ir(true);
      supprs.push_back(suppr);
    }

  using abigail::tools_utils::gen_suppr_spec_from_kernel_abi_whitelists;
  const suppressions_type& wl_suppr =
      gen_suppr_spec_from_kernel_abi_whitelists(opts.kabi_whitelist_paths);

  opts.kabi_whitelist_supprs.insert(opts.kabi_whitelist_supprs.end(),
				    wl_suppr.begin(), wl_suppr.end());

  add_read_context_suppressions(read_ctxt, supprs);
  add_read_context_suppressions(read_ctxt, opts.kabi_whitelist_supprs);
}

/// Load an ABI @ref corpus (the internal representation of the ABI of
/// a binary) and write it out as an abixml.
///
/// @param argv the arguments the program was called with.
///
/// @param env the environment the ABI artifacts are being created in.
///
/// @param context the context of the ELF reading.
///
/// @param opts the options of the program.
///
/// @return the exit code: 0 if everything went fine, non-zero
/// otherwise.
static int
load_corpus_and_write_abixml(char* argv[],
			     environment_sptr& env,
			     read_context_sptr& context,
			     const options& opts)
{
  int exit_code = 0;
  timer t;

  read_context& ctxt = *context;
  corpus_sptr corp;
  dwarf_reader::status s = dwarf_reader::STATUS_UNKNOWN;
  t.start();
  corp = read_corpus_from_elf(ctxt, s);
  t.stop();
  if (opts.do_log)
    emit_prefix(argv[0], cerr)
      << "read corpus from elf file in: " << t << "\n";

  t.start();
  context.reset();
  t.stop();

  if (opts.do_log)
    emit_prefix(argv[0], cerr)
      << "reset read context in: " << t << "\n";

  if (!corp)
    {
      if (s == dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
	{
	  if (opts.di_root_paths.empty())
	    {
	      emit_prefix(argv[0], cerr)
		<< "Could not read debug info from "
		<< opts.in_file_path << "\n";

	      emit_prefix(argv[0], cerr)
		<< "You might want to supply the root directory where "
		"to search debug info from, using the "
		"--debug-info-dir option "
		"(e.g --debug-info-dir /usr/lib/debug)\n";
	    }
	  else
	    {
	      emit_prefix(argv[0], cerr)
		<< "Could not read debug info for '" << opts.in_file_path
		<< "' from debug info root directory '";
	      for (vector<char*>::const_iterator i =
		     opts.di_root_paths.begin();
		   i != opts.di_root_paths.end();
		   ++i)
		{
		  if (i != opts.di_root_paths.begin())
		    cerr << ", ";
		  cerr << *i;
		}
	    }
	}
      else if (s == dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
	emit_prefix(argv[0], cerr)
	  << "Could not read ELF symbol information from "
	  << opts.in_file_path << "\n";

      return 1;
    }
  else
    {
      t.start();
      const write_context_sptr& write_ctxt
	  = create_write_context(corp->get_environment(), cout);
      set_common_options(*write_ctxt, opts);
      t.stop();

      if (opts.do_log)
	emit_prefix(argv[0], cerr)
	  << "created & initialized write context in: "
	  << t << "\n";

      if (opts.abidiff)
	{
	  // Save the abi in abixml format in a temporary file, read
	  // it back, and compare the ABI of what we've read back
	  // against the ABI of the input ELF file.
	  temp_file_sptr tmp_file = temp_file::create();
	  set_ostream(*write_ctxt, tmp_file->get_stream());
	  write_corpus(*write_ctxt, corp, 0);
	  tmp_file->get_stream().flush();
	  t.start();
	  corpus_sptr corp2 =
	    read_corpus_from_native_xml_file(tmp_file->get_path(),
					     env.get());
	  t.stop();
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "Read corpus in: " << t << "\n";

	  if (!corp2)
	    {
	      emit_prefix(argv[0], cerr)
		<< "Could not read temporary XML representation of "
		"elf file back\n";
	      return 1;
	    }

	  diff_context_sptr ctxt(new diff_context);
	  set_diff_context(ctxt);
	  ctxt->show_locs(opts.show_locs);
	  t.start();
	  corpus_diff_sptr diff = compute_diff(corp, corp2, ctxt);
	  t.stop();
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "computed diff in: " << t << "\n";

	  bool has_error = diff->has_changes();
	  if (has_error)
	    {
	      t.start();
	      diff->report(cerr);
	      t.stop();
	      if (opts.do_log)
		emit_prefix(argv[0], cerr)
		  << "emitted report in: " << t << "\n";
	      return 1;
	    }
	  return 0;
	}

      if (opts.noout)
	return 0;

      if (!opts.out_file_path.empty())
	{
	  ofstream of(opts.out_file_path.c_str(), std::ios_base::trunc);
	  if (!of.is_open())
	    {
	      emit_prefix(argv[0], cerr)
		<< "could not open output file '"
		<< opts.out_file_path << "'\n";
	      return 1;
	    }
	  set_ostream(*write_ctxt, of);
	  t.start();
	  write_corpus(*write_ctxt, corp, 0);
	  t.stop();
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitted abixml output in: " << t << "\n";
	  of.close();
	  return 0;
	}
      else
	{
	  t.start();
	  exit_code = !write_corpus(*write_ctxt, corp, 0);
	  t.stop();
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitted abixml out in: " << t << "\n";
	}
    }

  return exit_code;
}

/// Load a corpus group representing the union of a Linux Kernel
/// vmlinux binary and its modules, and emit an abixml representation
/// for it.
///
/// @param argv the arguments this program was called with.
///
/// @param env the environment the ABI artifacts are created in.
///
/// @param opts the options this program was created with.
///
/// @return the exit code.  Zero if everything went well, non-zero
/// otherwise.
static int
load_kernel_corpus_group_and_write_abixml(char* argv[],
					  environment_sptr& env,
					  options& opts)
{
  if (!(tools_utils::is_dir(opts.in_file_path) && opts.corpus_group_for_linux))
    return 1;

  int exit_code = 0;

  if (!opts.vmlinux.empty())
    if (!abigail::tools_utils::check_file(opts.vmlinux, cerr, argv[0]))
      return 1;

  timer t, global_timer;
  suppressions_type supprs;

  if (opts.do_log)
    emit_prefix(argv[0], cerr)
      << "going to build ABI representation of the Linux Kernel ...\n";

  global_timer.start();
  t.start();
  corpus_group_sptr group =
    build_corpus_group_from_kernel_dist_under(opts.in_file_path,
					      /*debug_info_root=*/"",
					      opts.vmlinux,
					      opts.suppression_paths,
					      opts.kabi_whitelist_paths,
					      supprs, opts.do_log, env);
  t.stop();

  if (opts.do_log)
    {
      emit_prefix(argv[0], cerr)
	<< "built ABI representation of the Linux Kernel in: "
	<< t << "\n";
    }

  if (!group)
    return 1;

  if (!opts.noout)
    {
      const xml_writer::write_context_sptr& ctxt
	  = xml_writer::create_write_context(group->get_environment(), cout);
      set_common_options(*ctxt, opts);

      if (!opts.out_file_path.empty())
	{
	  ofstream of(opts.out_file_path.c_str(), std::ios_base::trunc);
	  if (!of.is_open())
	    {
	      emit_prefix(argv[0], cerr)
		<< "could not open output file '"
		<< opts.out_file_path << "'\n";
	      return 1;
	    }

	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitting the abixml output ...\n";
	  set_ostream(*ctxt, of);
	  t.start();
	  exit_code = !write_corpus_group(*ctxt, group, 0);
	  t.stop();
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitted abixml output in: " << t << "\n";
	}
      else
	{
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitting the abixml output ...\n";
	  t.start();
	  exit_code = !write_corpus_group(*ctxt, group, 0);
	  t.stop();
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitted abixml output in: " << t << "\n";
	}
    }

  global_timer.stop();
  if (opts.do_log)
    emit_prefix(argv[0], cerr)
      << "total processing done in " << global_timer << "\n";
  return exit_code;
}

/// Convert options::di_root_paths into
/// options::prepared_di_root_paths which is the suitable type format
/// that the dwarf_reader expects.
///
/// @param o the options to consider.
static void
prepare_di_root_paths(options& o)
{
  tools_utils::convert_char_stars_to_char_star_stars(o.di_root_paths,
						     o.prepared_di_root_paths);
}

int
main(int argc, char* argv[])
{
  options opts;

  if (!parse_command_line(argc, argv, opts)
      || (opts.in_file_path.empty()
	  && !opts.display_version))
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
	<< abigail::tools_utils::get_library_version_string()
	<< "\n";
      return 0;
    }

  ABG_ASSERT(!opts.in_file_path.empty());
  if (opts.corpus_group_for_linux)
    {
      if (!abigail::tools_utils::check_dir(opts.in_file_path, cerr, argv[0]))
	return 1;
    }
  else
    {
      if (!abigail::tools_utils::check_file(opts.in_file_path, cerr, argv[0]))
	return 1;
    }

  prepare_di_root_paths(opts);

  if (!maybe_check_suppression_files(opts))
    return 1;

  if (!maybe_check_header_files(opts))
    return 1;

  abigail::tools_utils::file_type type =
    abigail::tools_utils::guess_file_type(opts.in_file_path);
  if (type != abigail::tools_utils::FILE_TYPE_ELF
      && type != abigail::tools_utils::FILE_TYPE_AR
      && type != abigail::tools_utils::FILE_TYPE_DIR)
    {
      emit_prefix(argv[0], cerr)
	<< "files of the kind of "<< opts.in_file_path << " are not handled\n";
      return 1;
    }

  environment_sptr env(new environment);
  int exit_code = 0;

  if (tools_utils::is_regular_file(opts.in_file_path))
    {
      read_context_sptr c = create_read_context(opts.in_file_path,
						opts.prepared_di_root_paths,
						env.get(),
						opts.load_all_types,
						opts.linux_kernel_mode);
      read_context& ctxt = *c;
      set_drop_undefined_syms(ctxt, opts.drop_undefined_syms);
      set_merge_translation_units(ctxt, opts.merge_translation_units);
      set_show_stats(ctxt, opts.show_stats);
      set_suppressions(ctxt, opts);
      abigail::dwarf_reader::set_do_log(ctxt, opts.do_log);

      if (opts.check_alt_debug_info_path)
	{
	  bool has_alt_di = false;
	  string alt_di_path;
	  abigail::dwarf_reader::status status =
	    abigail::dwarf_reader::has_alt_debug_info(ctxt,
						      has_alt_di,
						      alt_di_path);
	  if (status & abigail::dwarf_reader::STATUS_OK)
	    {
	      if (alt_di_path.empty())
		;
	      else
		{
		  cout << "found the alternate debug info file";
		  if (opts.show_base_name_alt_debug_info_path)
		    {
		      tools_utils::base_name(alt_di_path, alt_di_path);
		      cout << " '" << alt_di_path << "'";
		    }
		  cout << "\n";
		}
	      return 0;
	    }
	  else
	    {
	      emit_prefix(argv[0], cerr)
		<< "could not find alternate debug info file\n";
	      return 1;
	    }
	}
      exit_code = load_corpus_and_write_abixml(argv, env, c, opts);
    }
  else
    exit_code = load_kernel_corpus_group_and_write_abixml(argv, env, opts);

  return exit_code;
}

// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2014-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program reads a program A, one library L in version V which A
/// links against, and the same library L in a different version, V+P.
/// The program then checks that A is still ABI compatible with L in
/// version V+P.
///
/// The program also comes with a "weak mode" in which just the
/// application and the library in version V+P need to be provided by
/// the user.  In that case, the types of functions and variables of
/// the library that are consumed by the application are compared to
/// the types of the functions and variables expected by the
/// application.  If they match exactly, then the types of functions
/// and variables that the application expects from the library are
/// honoured by the library.  Otherwise, the library might provide
/// functions and variables that mean something different from what
/// the application expects and that might signal an ABI
/// incompatibility between what the application expects and what the
/// library provides.

#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include "abg-config.h"
#include "abg-tools-utils.h"
#include "abg-corpus.h"
#include "abg-dwarf-reader.h"
#include "abg-comparison.h"
#include "abg-suppression.h"

using std::string;
using std::cerr;
using std::cout;
using std::ostream;
using std::ofstream;
using std::vector;
using std::shared_ptr;

using abigail::tools_utils::emit_prefix;

class options
{
  options();

public:
  string		prog_name;
  string		unknow_option;
  string		app_path;
  string		lib1_path;
  string		lib2_path;
  shared_ptr<char>	app_di_root_path;
  shared_ptr<char>	lib1_di_root_path;
  shared_ptr<char>	lib2_di_root_path;
  vector<string>	suppression_paths;
  bool			display_help;
  bool			display_version;
  bool			weak_mode;
  bool			list_undefined_symbols_only;
  bool			show_base_names;
  bool			show_redundant;
  bool			show_locs;

  options(const char* program_name)
    :prog_name(program_name),
     display_help(),
     display_version(),
     weak_mode(),
     list_undefined_symbols_only(),
     show_base_names(),
     show_redundant(true),
     show_locs(true)
  {}
}; // end struct options

static void
display_usage(const string& prog_name, ostream& out)
{
  emit_prefix(prog_name, out)
    << "usage: " << prog_name
    << " [options] [application-path] [lib-v1-path] [lib-v2-path]"
    << "\n"
    << " where options can be: \n"
    << "  --help|-h  display this help message\n"
    << "  --version|-v  show program version information and exit\n"
    << "  --list-undefined-symbols|-u  display the list of "
    "undefined symbols of the application\n"
    << "  --show-base-names|b  in the report, only show the base names "
    " of the files; not the full paths\n"
    << "  --app-debug-info-dir|--appd <path-to-app-debug-info>  set the path "
    "to the debug information directory for the application\n"
    << "  --lib-debug-info-dir1|--libd1 <path-to-lib-debug-info1>  set the path "
    "to the debug information directory for the first library\n"
    << "  --lib-debug-info-dir2|--libd2 <path-to-lib-debug-info2>  set the path "
    "to the debug information directory for the second library\n"
    <<  "--suppressions|--suppr <path> specify a suppression file\n"
    << "--no-redundant  do not display redundant changes\n"
    << "--no-show-locs  do now show location information\n"
    << "--redundant  display redundant changes (this is the default)\n"
    << "--weak-mode  check compatibility between the application and "
    "just one version of the library."
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
	  if (opts.app_path.empty())
	    opts.app_path = argv[i];
	  else if (opts.lib1_path.empty())
	    opts.lib1_path = argv[i];
	  else if (opts.lib2_path.empty())
	    opts.lib2_path = argv[i];
	  else
	    return false;
	}
      else if (!strcmp(argv[i], "--version")
	       || !strcmp(argv[i], "-v"))
	{
	  opts.display_version = true;
	  return true;
	}
      else if (!strcmp(argv[i], "--list-undefined-symbols")
	       || !strcmp(argv[i], "-u"))
	opts.list_undefined_symbols_only = true;
      else if (!strcmp(argv[i], "--show-base-names")
	       || !strcmp(argv[i], "-b"))
	opts.show_base_names = true;
      else if (!strcmp(argv[i], "--app-debug-info-dir")
	       || !strcmp(argv[i], "--appd"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-')
	    return false;
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.app_di_root_path =
	    abigail::tools_utils::make_path_absolute(argv[i + 1]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--lib-debug-info-dir1")
	       || !strcmp(argv[i], "--libd1"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-')
	    return false;
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.lib1_di_root_path =
	    abigail::tools_utils::make_path_absolute(argv[i + 1]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--lib-debug-info-dir2")
	       || !strcmp(argv[i], "--libd2"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-')
	    return false;
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.lib2_di_root_path =
	    abigail::tools_utils::make_path_absolute(argv[i + 1]);
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
      else if (!strcmp(argv[i], "--redundant"))
	opts.show_redundant = true;
      else if (!strcmp(argv[i], "--no-redundant"))
	opts.show_redundant = false;
      else if (!strcmp(argv[i], "--no-show-locs"))
	opts.show_locs = false;
      else if (!strcmp(argv[i], "--help")
	       || !strcmp(argv[i], "-h"))
	{
	  opts.display_help = true;
	  return true;
	}
      else if (!strcmp(argv[i], "--weak-mode"))
	opts.weak_mode = true;
      else
	{
	  opts.unknow_option = argv[i];
	  return false;
	}
    }

  if (!opts.list_undefined_symbols_only)
    {
      if (opts.app_path.empty()
	  || opts.lib1_path.empty())
	return false;
      if (!opts.weak_mode && opts.lib2_path.empty())
	opts.weak_mode = true;
    }

  return true;
}

using abigail::tools_utils::check_file;
using abigail::tools_utils::base_name;
using abigail::tools_utils::abidiff_status;
using abigail::ir::environment;
using abigail::ir::environment_sptr;
using abigail::corpus;
using abigail::corpus_sptr;
using abigail::ir::elf_symbols;
using abigail::ir::demangle_cplus_mangled_name;
using abigail::ir::type_base_sptr;
using abigail::ir::function_type_sptr;
using abigail::ir::function_decl;
using abigail::ir::var_decl;
using abigail::dwarf_reader::status;
using abigail::dwarf_reader::read_corpus_from_elf;
using abigail::comparison::diff_context_sptr;
using abigail::comparison::diff_context;
using abigail::comparison::diff_sptr;
using abigail::comparison::corpus_diff;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::function_type_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::read_suppressions;

/// Create the context of a diff.
///
/// Create the diff context, initialize it and return a smart pointer
/// to it.
///
/// @param opts the options of the program.
///
/// @return a smart pointer to the newly created diff context.
static diff_context_sptr
create_diff_context(const options& opts)
{
  diff_context_sptr ctxt(new diff_context());
  ctxt->show_added_fns(false);
  ctxt->show_added_vars(false);
  ctxt->show_added_symbols_unreferenced_by_debug_info(false);
  ctxt->show_linkage_names(true);
  ctxt->show_redundant_changes(opts.show_redundant);
  ctxt->show_locs(opts.show_locs);
  ctxt->switch_categories_off
    (abigail::comparison::ACCESS_CHANGE_CATEGORY
     | abigail::comparison::COMPATIBLE_TYPE_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_DECL_NAME_CHANGE_CATEGORY
     | abigail::comparison::NON_VIRT_MEM_FUN_CHANGE_CATEGORY
     | abigail::comparison::STATIC_DATA_MEMBER_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_ENUM_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_SYMBOL_ALIAS_CHANGE_CATEGORY);

  // Load suppression specifications, if there are any.
  suppressions_type supprs;
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    if (check_file(*i, cerr, opts.prog_name))
      read_suppressions(*i, supprs);

  if (!supprs.empty())
    ctxt->add_suppressions(supprs);

  return ctxt;
}

/// Perform a compatibility check of an application corpus linked
/// against a first version of library corpus, with a second version
/// of the same library.
///
/// @param opts the options the tool got invoked with.
///
/// @param ctxt the context of the diff to be performed.
///
/// @param app_corpus the application corpus to consider.
///
/// @param lib1_corpus the library corpus that got linked with the
/// application which corpus is @p app_corpus.
///
/// @param lib2_corpus the second version of the library corpus @p
/// lib1_corpus.  This function checks that the functions and
/// variables that @p app_corpus expects from lib1_corpus are still
/// present in @p lib2_corpus and that their types mean the same
/// thing.
///
/// @return a status bitfield.
static abidiff_status
perform_compat_check_in_normal_mode(options& opts,
				    diff_context_sptr& ctxt,
				    corpus_sptr app_corpus,
				    corpus_sptr lib1_corpus,
				    corpus_sptr lib2_corpus)
{
  ABG_ASSERT(lib1_corpus);
  ABG_ASSERT(lib2_corpus);
  ABG_ASSERT(app_corpus);

  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;

  // compare lib1 and lib2 only by looking at the functions and
  // variables which symbols are those undefined in the app.

  for (elf_symbols::const_iterator i =
	 app_corpus->get_sorted_undefined_fun_symbols().begin();
       i != app_corpus->get_sorted_undefined_fun_symbols().end();
       ++i)
    {
      string id = (*i)->get_id_string();
      lib1_corpus->get_sym_ids_of_fns_to_keep().push_back(id);
      lib2_corpus->get_sym_ids_of_fns_to_keep().push_back(id);
    }
  for (elf_symbols::const_iterator i =
	 app_corpus->get_sorted_undefined_var_symbols().begin();
       i != app_corpus->get_sorted_undefined_var_symbols().end();
       ++i)
    {
      string id = (*i)->get_id_string();
      lib1_corpus->get_sym_ids_of_vars_to_keep().push_back(id);
      lib2_corpus->get_sym_ids_of_vars_to_keep().push_back(id);
    }

  if (!app_corpus->get_sorted_undefined_var_symbols().empty()
      || !app_corpus->get_sorted_undefined_fun_symbols().empty())
    {
      lib1_corpus->maybe_drop_some_exported_decls();
      lib2_corpus->maybe_drop_some_exported_decls();
    }

  // Now really do the diffing.
  corpus_diff_sptr changes = compute_diff(lib1_corpus, lib2_corpus, ctxt);

  if (changes->has_net_changes())
    {
      string app_path = opts.app_path,
	lib1_path = opts.lib1_path,
	lib2_path = opts.lib2_path;

      if (opts.show_base_names)
	{
	  base_name(opts.app_path, app_path);
	  base_name(opts.lib1_path, lib1_path);
	  base_name(opts.lib2_path, lib2_path);
	}

      status |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;

      bool abi_broke_for_sure = changes->has_incompatible_changes();

      cout << "ELF file '" << app_path << "'";
      if (abi_broke_for_sure)
	{
	  cout << " is not ";
	  status |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;
	}
      else
	  cout << " might not be ";

      cout << "ABI compatible with '" << lib2_path
	   << "' due to differences with '" << lib1_path
	   << "' below:\n";
      changes->report(cout);
    }

  return status;
}

/// An description of a change of the type of a function.  It contains
/// the declaration of the function we are interested in, as well as
/// the differences found in the type of that function.
struct fn_change
{
  function_decl* decl;
  function_type_diff_sptr diff;

  fn_change()
    : decl()
  {}

  fn_change(function_decl* decl,
	    function_type_diff_sptr difference)
    : decl(decl),
      diff(difference)
  {}
}; // end struct fn_change

/// An description of a change of the type of a variable.  It contains
/// the declaration of the variable we are interested in, as well as
/// the differences found in the type of that variable.
struct var_change
{
  var_decl* decl;
  diff_sptr diff;

  var_change()
    : decl()
  {}

  var_change(var_decl* var,
	     diff_sptr difference)
    : decl(var),
      diff(difference)
  {}
}; // end struct var_change

/// Perform a compatibility check of an application corpus and a
/// library corpus.
///
/// The types of the variables and functions exported by the library
/// and consumed by the application are compared with the types
/// expected by the application.  This function checks that the types
/// mean the same thing; otherwise it emits on standard output type
/// layout differences found.
///
/// @param opts the options the tool got invoked with.
///
/// @param app_corpus the application corpus to consider.
///
/// @param lib_corpus the library corpus to consider.
///
/// @return a status bitfield.
static abidiff_status
perform_compat_check_in_weak_mode(options& opts,
				  diff_context_sptr& ctxt,
				  corpus_sptr app_corpus,
				  corpus_sptr lib_corpus)
{
  ABG_ASSERT(lib_corpus);
  ABG_ASSERT(app_corpus);

  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;

  // Functions and variables defined and exported by lib_corpus which
  // symbols are undefined in app_corpus are the artifacts we are
  // interested in.
  //
  // So let's drop all functions and variables from lib_corpus that
  // are so that their symbols are *NOT* undefined in app_corpus.
  //
  // In other words, let's only keep the functiond and variables from
  // lib_corpus that are consumed by app_corpus.

  for (elf_symbols::const_iterator i =
	 app_corpus->get_sorted_undefined_fun_symbols().begin();
       i != app_corpus->get_sorted_undefined_fun_symbols().end();
       ++i)
    {
      string id = (*i)->get_id_string();
      lib_corpus->get_sym_ids_of_fns_to_keep().push_back(id);
    }

  for (elf_symbols::const_iterator i =
	 app_corpus->get_sorted_undefined_var_symbols().begin();
       i != app_corpus->get_sorted_undefined_var_symbols().end();
       ++i)
    {
      string id = (*i)->get_id_string();
      lib_corpus->get_sym_ids_of_vars_to_keep().push_back(id);
    }

  if (!app_corpus->get_sorted_undefined_var_symbols().empty()
      || !app_corpus->get_sorted_undefined_fun_symbols().empty())
    lib_corpus->maybe_drop_some_exported_decls();

  // OK now, lib_corpus only contains functions and variables which
  // symbol are consumed by app_corpus.

  // So we are now going to compare the functions that are exported by
  // lib_corpus against those that app_corpus expects.
  //
  // In other words, the functions which symbols are defined by
  // lib_corpus are going to be compared to the functions and
  // variables which are undefined in app_corpus.

  {
    function_type_sptr lib_fn_type, app_fn_type;
    vector<fn_change> fn_changes;
    for (corpus::functions::const_iterator i =
	   lib_corpus->get_functions().begin();
	 i != lib_corpus->get_functions().end();
	 ++i)
      {
	// lib_fn_type contains the type of a function that is defined
	// in lib_corpus.
	lib_fn_type = (*i)->get_type();
	ABG_ASSERT(lib_fn_type);

	// app_fn_type contains the the "version" of lib_fn_type that
	// is expected by app_corpus.
	app_fn_type = lookup_or_synthesize_fn_type(lib_fn_type, *app_corpus);

	// Now lets compare the type expected by app_corpus against
	// the type actually provided by lib_fn_type.
	function_type_diff_sptr fn_type_diff;
	if (app_fn_type)
	  fn_type_diff = compute_diff(app_fn_type, lib_fn_type, ctxt);

	// If the two types of functions are different, then let's
	// store their difference in the "fn_changes" vector.
	if (fn_type_diff && fn_type_diff->to_be_reported())
	  fn_changes.push_back(fn_change(*i, fn_type_diff));
      }

    string lib1_path = opts.lib1_path, app_path = opts.app_path;
    if (opts.show_base_names)
      {
	base_name(opts.lib1_path, lib1_path);
	base_name(opts.app_path, app_path);
      }

    // If some function changes were detected, then report them.
    if (!fn_changes.empty())
      {
	cout << "functions defined in library "
	     << "'" << lib1_path << "'\n"
	     << "have sub-types that are different from what application "
	     << "'" << app_path << "' "
	     << "expects:\n\n";
	for (vector<fn_change>::const_iterator i = fn_changes.begin();
	     i != fn_changes.end();
	     ++i)
	  {
	    cout << "  "
		 << i->decl->get_pretty_representation()
		 << ":\n";
	    i->diff->report(cout, "    ");
	    cout << "\n";
	  }
      }

    if (!fn_changes.empty())
      status |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;

    // OK now, let's do something similar for *variables* changes.
    //
    // That is, let's compare the variables expected by app_corpus
    // against the variables actually provided by lib_corpus and
    // report the difference that might have been found.

    type_base_sptr lib_var_type, app_var_type;
    vector<var_change> var_changes;
    for (corpus::variables::const_iterator i =
	   lib_corpus->get_variables().begin();
	 i != lib_corpus->get_variables().end();
	 ++i)
      {
	lib_var_type = (*i)->get_type();
	ABG_ASSERT(lib_var_type);
	app_var_type = lookup_type(lib_var_type, *app_corpus);
	diff_sptr type_diff;
	if (app_var_type)
	  type_diff = compute_diff(app_var_type, lib_var_type, ctxt);
	if (type_diff && type_diff->to_be_reported())
	  var_changes.push_back(var_change(*i, type_diff));
      }
    if (!var_changes.empty())
      {
	cout << "variables defined in library "
	     << "'" << lib1_path << "'\n"
	     << "have sub-types that are different from what application "
	     << "'" << app_path << "' "
	     << "expects:\n\n";
	for (vector<var_change>::const_iterator i = var_changes.begin();
	     i != var_changes.end();
	     ++i)
	  {
	    cout << "  "
		 << i->decl->get_pretty_representation()
		 << ":\n";
	    i->diff->report(cout, "    ");
	    cout << "\n";
	  }
      }
  }
  return status;
}

int
main(int argc, char* argv[])
{
  options opts(argv[0]);

  if (!parse_command_line(argc, argv, opts))
    {
      if (!opts.unknow_option.empty())
	{
	  emit_prefix(argv[0], cerr)
	    << "unrecognized option: " << opts.unknow_option << "\n"
	    << "try the --help option for more information\n";
	  return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
	}

      emit_prefix(argv[0], cerr)
	<< "wrong invocation\n"
	<< "try the --help option for more information\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.display_help)
    {
      display_usage(argv[0], cout);
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.display_version)
    {
      emit_prefix(argv[0], cout)
	<< abigail::tools_utils::get_library_version_string();
      return 0;
    }

  ABG_ASSERT(!opts.app_path.empty());
  if (!abigail::tools_utils::check_file(opts.app_path, cerr, opts.prog_name))
    return abigail::tools_utils::ABIDIFF_ERROR;

  abigail::tools_utils::file_type type =
    abigail::tools_utils::guess_file_type(opts.app_path);
  if (type != abigail::tools_utils::FILE_TYPE_ELF)
    {
      emit_prefix(argv[0], cerr)
	<< opts.app_path << " is not an ELF file\n";
      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  // Create the context of the diff
  diff_context_sptr ctxt = create_diff_context(opts);

  // Check if any suppression specification prevents us from
  // performing the compatibility checking.
  suppressions_type& supprs = ctxt->suppressions();
  bool files_suppressed = (file_is_suppressed(opts.app_path, supprs)
			   || file_is_suppressed(opts.lib1_path, supprs)
			   ||file_is_suppressed(opts.lib2_path, supprs));

  if (files_suppressed)
    // We don't have to compare anything because a user
    // suppression specification file instructs us to avoid
    // loading either one of the input files.
    return abigail::tools_utils::ABIDIFF_OK;

  // Read the application ELF file.
  char * app_di_root = opts.app_di_root_path.get();
  vector<char**> app_di_roots;
  app_di_roots.push_back(&app_di_root);
  status status = abigail::dwarf_reader::STATUS_UNKNOWN;
  environment_sptr env(new environment);
  corpus_sptr app_corpus=
    read_corpus_from_elf(opts.app_path,
			 app_di_roots, env.get(),
			 /*load_all_types=*/opts.weak_mode,
			 status);

  if (status & abigail::dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
    {
      emit_prefix(argv[0], cerr)
	<< "could not read symbols from " << opts.app_path << "\n";
      return abigail::tools_utils::ABIDIFF_ERROR;
    }
  if (!(status & abigail::dwarf_reader::STATUS_OK))
    {
      emit_prefix(argv[0], cerr)
	<< "could not read file " << opts.app_path << "\n";
      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  if (opts.list_undefined_symbols_only)
    {
      for (elf_symbols::const_iterator i =
	     app_corpus->get_sorted_undefined_fun_symbols().begin();
	   i != app_corpus->get_sorted_undefined_fun_symbols().end();
	   ++i)
	{
	  string id = (*i)->get_id_string();
	  string sym_name = (*i)->get_name();
	  string demangled_name = demangle_cplus_mangled_name(sym_name);
	  if (demangled_name != sym_name)
	    cout << demangled_name << "  {" << id << "}\n";
	  else
	    cout << id << "\n";
	}
      return abigail::tools_utils::ABIDIFF_OK;
    }

  // Read the first version of the library.
  ABG_ASSERT(!opts.lib1_path.empty());
  if (!abigail::tools_utils::check_file(opts.lib1_path, cerr, opts.prog_name))
    return abigail::tools_utils::ABIDIFF_ERROR;
  type = abigail::tools_utils::guess_file_type(opts.lib1_path);
  if (type != abigail::tools_utils::FILE_TYPE_ELF)
    {
      emit_prefix(argv[0], cerr) << opts.lib1_path << " is not an ELF file\n";
      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  char * lib1_di_root = opts.lib1_di_root_path.get();
  vector<char**> lib1_di_roots;
  lib1_di_roots.push_back(&lib1_di_root);
  corpus_sptr lib1_corpus = read_corpus_from_elf(opts.lib1_path,
						 lib1_di_roots, env.get(),
						 /*load_all_types=*/false,
						 status);
  if (status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
    emit_prefix(argv[0], cerr)
      << "could not read debug info for " << opts.lib1_path << "\n";
  if (status & abigail::dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
    {
      cerr << "could not read symbols from " << opts.lib1_path << "\n";
      return abigail::tools_utils::ABIDIFF_ERROR;
    }
  if (!(status & abigail::dwarf_reader::STATUS_OK))
    {
      emit_prefix(argv[0], cerr)
	<< "could not read file " << opts.lib1_path << "\n";
      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  // Read the second version of the library.
  corpus_sptr lib2_corpus;
  if (!opts.weak_mode)
    {
      ABG_ASSERT(!opts.lib2_path.empty());
      char * lib2_di_root = opts.lib2_di_root_path.get();
      vector<char**> lib2_di_roots;
      lib2_di_roots.push_back(&lib2_di_root);
      lib2_corpus = read_corpus_from_elf(opts.lib2_path,
					 lib2_di_roots, env.get(),
					 /*load_all_types=*/false,
					 status);
      if (status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
	emit_prefix(argv[0], cerr)
	  << "could not read debug info for " << opts.lib2_path << "\n";
      if (status & abigail::dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
	{
	  emit_prefix(argv[0], cerr)
	    << "could not read symbols from " << opts.lib2_path << "\n";
	  return abigail::tools_utils::ABIDIFF_ERROR;
	}
      if (!(status & abigail::dwarf_reader::STATUS_OK))
	{
	  emit_prefix(argv[0], cerr)
	    << "could not read file " << opts.lib2_path << "\n";
	  return abigail::tools_utils::ABIDIFF_ERROR;
	}
    }

  abidiff_status s = abigail::tools_utils::ABIDIFF_OK;

  if (opts.weak_mode)
    s = perform_compat_check_in_weak_mode(opts, ctxt,
					  app_corpus,
					  lib1_corpus);
  else
    s = perform_compat_check_in_normal_mode(opts, ctxt,
					    app_corpus,
					    lib1_corpus,
					    lib2_corpus);

  return s;
}

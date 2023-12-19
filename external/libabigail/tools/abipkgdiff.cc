// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2015-2020 Red Hat, Inc.
//
// Author: Sinny Kumari

/// @file

/// This program compares the ABIs of binaries inside two packages.
///
/// For now, the supported package formats are Deb and RPM, but
/// support for other formats would be greatly appreciated.
///
/// The program takes the two packages to compare as well as their
/// associated debug info packages.
///
/// The program extracts the content of the two packages into a
/// temporary directory , looks for the ELF binaries in there,
/// compares their ABIs and emit a report about the changes.
/// As this program uses libpthread to perform several tasks
/// concurrently, here is a coarse grain description of the sequence
/// of actions performed, including where things are done
/// concurrently.
///
/// (steps 1/ and 2/ are performed concurrently.  Then steps 3/ and 4/
/// are performed in sequence)
///
/// 1/ the first package and its ancillary packages (debug info and
/// devel packages) are extracted concurrently.
/// There is one thread per package being extracted.  So if there are
/// 3 thread packages (one package, one debug info package and one
/// devel package), then there are 3 threads to extracts them.  Then
/// when the extracting is done, another thread performs the analysis
/// of th1 extracted content.
///
/// 2/ A similar thing is done for the second package.
///
/// 3/ comparisons are performed concurrently.
///
/// 4/ the reports are then emitted to standard output, always in the same
/// order.


// In case we have a bad fts we include this before config.h because
// it can't handle _FILE_OFFSET_BITS.  Everything we need here is fine
// if its declarations just come first.  Also, include sys/types.h
// before fts. On some systems fts.h is not self contained.
#ifdef BAD_FTS
  #include <sys/types.h>
  #include <fts.h>
#endif

// For package configuration macros.
#include "config.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

// If fts.h is included before config.h, its indirect inclusions may
// not give us the right LFS aliases of these functions, so map them
// manually.
#ifdef BAD_FTS
  #ifdef _FILE_OFFSET_BITS
    #define open open64
    #define fopen fopen64
  #endif
#else
  #include <sys/types.h>
  #include <fts.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "abg-workers.h"
#include "abg-config.h"
#include "abg-tools-utils.h"
#include "abg-comparison.h"
#include "abg-suppression.h"
#include "abg-dwarf-reader.h"
#include "abg-reader.h"
#include "abg-writer.h"

using std::cout;
using std::cerr;
using std::string;
using std::ostream;
using std::ofstream;
using std::vector;
using std::map;
using std::unordered_set;
using std::set;
using std::ostringstream;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using abigail::workers::task;
using abigail::workers::task_sptr;
using abigail::workers::queue;
using abigail::tools_utils::maybe_get_symlink_target_file_path;
using abigail::tools_utils::file_exists;
using abigail::tools_utils::is_dir;
using abigail::tools_utils::emit_prefix;
using abigail::tools_utils::check_file;
using abigail::tools_utils::ensure_dir_path_created;
using abigail::tools_utils::guess_file_type;
using abigail::tools_utils::string_ends_with;
using abigail::tools_utils::dir_name;
using abigail::tools_utils::real_path;
using abigail::tools_utils::string_suffix;
using abigail::tools_utils::sorted_strings_common_prefix;
using abigail::tools_utils::file_type;
using abigail::tools_utils::make_path_absolute;
using abigail::tools_utils::base_name;
using abigail::tools_utils::get_rpm_arch;
using abigail::tools_utils::file_is_kernel_package;
using abigail::tools_utils::gen_suppr_spec_from_headers;
using abigail::tools_utils::get_default_system_suppression_file_path;
using abigail::tools_utils::get_default_user_suppression_file_path;
using abigail::tools_utils::get_vmlinux_path_from_kernel_dist;
using abigail::tools_utils::get_dsos_provided_by_rpm;
using abigail::tools_utils::build_corpus_group_from_kernel_dist_under;
using abigail::tools_utils::load_default_system_suppressions;
using abigail::tools_utils::load_default_user_suppressions;
using abigail::tools_utils::abidiff_status;
using abigail::ir::corpus_sptr;
using abigail::ir::corpus_group_sptr;
using abigail::comparison::diff_context;
using abigail::comparison::diff_context_sptr;
using abigail::comparison::compute_diff;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::get_default_harmless_categories_bitmap;
using abigail::comparison::get_default_harmful_categories_bitmap;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::read_suppressions;
using abigail::dwarf_reader::read_context_sptr;
using abigail::dwarf_reader::create_read_context;
using abigail::dwarf_reader::get_soname_of_elf_file;
using abigail::dwarf_reader::get_type_of_elf_file;
using abigail::dwarf_reader::read_corpus_from_elf;
using abigail::xml_writer::create_write_context;
using abigail::xml_writer::write_context_sptr;
using abigail::xml_writer::write_corpus;

class package;

/// Convenience typedef for a shared pointer to a @ref package.
typedef shared_ptr<package> package_sptr;

/// The options passed to the current program.
class options
{
  options();

public:
  string	wrong_option;
  string	wrong_arg;
  string	prog_name;
  bool		display_usage;
  bool		display_version;
  bool		missing_operand;
  bool		nonexistent_file;
  bool		abignore;
  bool		parallel;
  string	package1;
  string	package2;
  vector<string> debug_packages1;
  vector<string> debug_packages2;
  string	devel_package1;
  string	devel_package2;
  size_t	num_workers;
  bool		verbose;
  bool		drop_private_types;
  bool		show_relative_offset_changes;
  bool		no_default_suppression;
  bool		keep_tmp_files;
  bool		compare_dso_only;
  bool		compare_private_dsos;
  bool		leaf_changes_only;
  bool		show_all_types;
  bool		show_hexadecimal_values;
  bool		show_offsets_sizes_in_bits;
  bool		show_impacted_interfaces;
  bool		show_full_impact_report;
  bool		show_linkage_names;
  bool		show_redundant_changes;
  bool		show_harmless_changes;
  bool		show_locs;
  bool		show_added_syms;
  bool		show_symbols_not_referenced_by_debug_info;
  bool		show_added_binaries;
  bool		fail_if_no_debug_info;
  bool		show_identical_binaries;
  bool		self_check;
  vector<string> kabi_whitelist_packages;
  vector<string> suppression_paths;
  vector<string> kabi_whitelist_paths;
  suppressions_type kabi_suppressions;
  package_sptr  pkg1;
  package_sptr  pkg2;

  options(const string& program_name)
    : prog_name(program_name),
      display_usage(),
      display_version(),
      missing_operand(),
      nonexistent_file(),
      abignore(true),
      parallel(true),
      verbose(),
      drop_private_types(),
      show_relative_offset_changes(true),
      no_default_suppression(),
      keep_tmp_files(),
      compare_dso_only(),
      compare_private_dsos(),
      leaf_changes_only(),
      show_all_types(),
      show_hexadecimal_values(),
      show_offsets_sizes_in_bits(true),
      show_impacted_interfaces(),
      show_full_impact_report(),
      show_linkage_names(true),
      show_redundant_changes(),
      show_harmless_changes(),
      show_locs(true),
      show_added_syms(true),
      show_symbols_not_referenced_by_debug_info(true),
      show_added_binaries(true),
      fail_if_no_debug_info(),
      show_identical_binaries(),
      self_check()
  {
    // set num_workers to the default number of threads of the
    // underlying maching.  This is the default value for the number
    // of workers to use in workers queues throughout the code.
    num_workers = abigail::workers::get_number_of_threads();
  }
};

static bool
get_interesting_files_under_dir(const string	dir,
				const string&	file_name_to_look_for,
				options&	opts,
				vector<string>& interesting_files);

/// Abstract ELF files from the packages which ABIs ought to be
/// compared
class elf_file
{
private:
  elf_file();

public:
  string				path;
  string				name;
  string				soname;
  off_t				size;
  abigail::dwarf_reader::elf_type	type;

  /// The path to the elf file.
  ///
  /// @param path the path to the elf file.
  elf_file(const string& path)
    : path(path)
   {
     abigail::tools_utils::base_name(path, name);
     get_soname_of_elf_file(path, soname);
     get_type_of_elf_file(path, type);
     struct stat estat;
     stat(path.c_str(), &estat);
     size = estat.st_size;
  }
};

/// A convenience typedef for a shared pointer to elf_file.
typedef shared_ptr<elf_file> elf_file_sptr;

/// Abstract the result of comparing two packages.
///
/// This contains the the paths of the set of added binaries, removed
/// binaries, and binaries whic ABI changed.
struct abi_diff
{
  vector<elf_file_sptr> added_binaries;
  vector<elf_file_sptr> removed_binaries;
  vector<string> changed_binaries;

  /// Test if the current diff carries changes.
  ///
  /// @return true iff the current diff carries changes.
  bool
  has_changes()
  {
    return (!added_binaries.empty()
	    || !removed_binaries.empty()
	    ||!changed_binaries.empty());
  }
};

/// Abstracts a package.
class package
{
public:

  /// The kind of package we are looking at.
  enum kind
  {
    /// Main package. Contains binaries to ABI-compare.
    KIND_MAIN = 0,
    /// Devel package.  Contains public headers files in which public
    /// types are defined.
    KIND_DEVEL,
    /// Debug info package.  Contains the debug info for the binaries
    /// int he main packge.
    KIND_DEBUG_INFO,
    /// Contains kernel ABI whitelists
    KIND_KABI_WHITELISTS,
    /// Source package.  Contains the source of the binaries in the
    /// main package.
    KIND_SRC
  };

private:
  string				path_;
  string				extracted_dir_path_;
  string				common_paths_prefix_;
  abigail::tools_utils::file_type	type_;
  kind					kind_;
  map<string, elf_file_sptr>		path_elf_file_sptr_map_;
  vector<package_sptr>			debug_info_packages_;
  package_sptr				devel_package_;
  package_sptr				kabi_whitelist_package_;
  vector<string>			elf_file_paths_;
  set<string>				public_dso_sonames_;

public:
  /// Constructor for the @ref package type.
  ///
  /// @param path the path to the package.
  ///
  /// @parm dir the temporary directory where to extract the content
  /// of the package.
  ///
  /// @param pkg_kind the kind of package.
  package(const string&			path,
	  const string&			dir,
          kind					pkg_kind = package::KIND_MAIN)
    : path_(path),
      kind_(pkg_kind)
  {
    type_ = guess_file_type(path);
    if (type_ == abigail::tools_utils::FILE_TYPE_DIR)
      extracted_dir_path_ = path;
    else
      extracted_dir_path_ = extracted_packages_parent_dir() + "/" + dir;
  }

  /// Getter of the path of the package.
  ///
  /// @return the path of the package.
  const string&
  path() const
  {return path_;}

  /// Setter of the path of the package.
  ///
  /// @param s the new path.
  void
  path(const string& s)
  {path_ = s;}

  /// Getter of the base name of the package.
  ///
  /// @return the base name of the package.
  string
  base_name() const
  {
    string name;
    abigail::tools_utils::base_name(path(), name);
    return name;
  }

  /// Getter for the path to the root dir where the packages are
  /// extracted.
  ///
  /// @return the path to the root dir where the packages are
  /// extracted.
  static const string&
  extracted_packages_parent_dir();

  /// Getter for the path to the directory where the packages are
  /// extracted for the current thread.
  ///
  /// @return the path to the directory where the packages are
  /// extracted for the current thread.
  const string&
  extracted_dir_path() const
  {return extracted_dir_path_;}

  /// Setter for the path to the directory where the packages are
  /// extracted for the current thread.
  ///
  /// @param p the new path.
  void
  extracted_dir_path(const string& p)
  {extracted_dir_path_ = p;}

  /// Getter of the the prefix that is common to all the paths of all
  /// the elements of the package.
  ///
  /// @return the common path prefix of package elements.
  const string&
  common_paths_prefix() const
  {return common_paths_prefix_;}

  /// Getter of the the prefix that is common to all the paths of all
  /// the elements of the package.
  ///
  /// @return the common path prefix of package elements.
  string&
  common_paths_prefix()
  {return common_paths_prefix_;}

  /// Setter of the the prefix that is common to all the paths of all
  /// the elements of the package.
  ///
  ///
  ///@param p the new prefix. 
  void
  common_paths_prefix(const string& p)
  {common_paths_prefix_ = p;}

  /// Getter for the file type of the current package.
  ///
  /// @return the file type of the current package.
  abigail::tools_utils::file_type
  type() const
  {return type_;}

  /// Setter for the file type of the current package.
  ///
  /// @param t the new file type.
  void type(abigail::tools_utils::file_type t)
  {type_ = t;}

  /// Get the package kind
  ///
  /// @return the package kind
  kind
  get_kind() const
  {return kind_;}

  /// Set the package kind
  ///
  /// @param k the package kind.
  void
  set_kind(kind k)
  {kind_ = k;}

  /// Getter for the path <-> elf_file map.
  ///
  /// @return the the path <-> elf_file map.
  const map<string, elf_file_sptr>&
  path_elf_file_sptr_map() const
  {return path_elf_file_sptr_map_;}

  /// Getter for the path <-> elf_file map.
  ///
  /// @return the the path <-> elf_file map.
  map<string, elf_file_sptr>&
  path_elf_file_sptr_map()
  {return path_elf_file_sptr_map_;}

  /// Getter for the debug info packages associated to the current
  /// package.
  ///
  /// There can indeed be several debug info packages needed for one
  /// input package, as the debug info for that input package can be
  /// split across several debuginfo packages.
  ///
  /// @return the debug info packages associated to the current
  /// package.
  const vector<package_sptr>&
  debug_info_packages() const
  {return debug_info_packages_;}

  /// Getter for the debug info packages associated to the current
  /// package.
  ///
  /// There can indeed be several debug info packages needed for one
  /// input package, as the debug info for that input package can be
  /// split across several debuginfo packages.
  ///
  /// @return the debug info packages associated to the current
  /// package.
  vector<package_sptr>&
  debug_info_packages()
  {return debug_info_packages_;}

  /// Setter for the debug info packages associated to the current
  /// package.
  ///
  /// There can indeed be several debug info packages needed for one
  /// input package, as the debug info for that input package can be
  /// split across several debuginfo packages.
  ///
  /// @param p the new debug info package.
  void
  debug_info_packages(const vector<package_sptr> &p)
  {debug_info_packages_ = p;}

  /// Getter for the devel package associated to the current package.
  ///
  /// @return the devel package associated to the current package.
  const package_sptr&
  devel_package() const
  {return devel_package_;}

  /// Setter of the devel package associated to the current package.
  ///
  /// @param p the new devel package associated to the current package.
  void
  devel_package(const package_sptr& p)
  {devel_package_ = p;}

  /// Getter of the associated kernel abi whitelist package, if any.
  ///
  /// @return the associated kernel abi whitelist package.
  const package_sptr
  kabi_whitelist_package() const
  {return kabi_whitelist_package_;}

  /// Setter of the associated kernel abi whitelist package.
  ///
  /// @param p the new kernel abi whitelist package.
  void
  kabi_whitelist_package(const package_sptr& p)
  {kabi_whitelist_package_ = p;}

  /// Getter of the path to the elf files of the package.
  ///
  /// @return the path tothe elf files of the package.
  const vector<string>&
  elf_file_paths() const
  {return elf_file_paths_;}

  /// Getter of the path to the elf files of the package.
  ///
  /// @return the path tothe elf files of the package.
  vector<string>&
  elf_file_paths()
  {return elf_file_paths_;}

  /// Getter of the SONAMEs of the public DSOs carried by this
  /// package.
  ///
  /// This is relevant only if the --private-dso option was *NOT*
  /// provided.
  ///
  /// @return the SONAMEs of the public DSOs carried by this package.
  const set<string>&
  public_dso_sonames() const
  {return public_dso_sonames_;}

  /// Getter of the SONAMEs of the public DSOs carried by this
  /// package.
  ///
  /// This is relevant only if the --private-dso option was *NOT*
  /// provided.
  ///
  /// @return the SONAMEs of the public DSOs carried by this package.
  set<string>&
  public_dso_sonames()
  {return public_dso_sonames_;}

  /// Convert the absolute path of an element of this package into a
  /// path relative to the root path pointing to this package.
  ///
  /// That is, suppose the content of a package named 'pkg' is located
  /// at /root/path/pkg.  Suppose an element of that package is named
  /// is at '/root/path/pkg/somewhere/inside/element'.
  ///
  /// This function will return the path:
  /// /pkg/somewhere/inside/element.
  ///
  /// @param path the path to consider.
  ///
  /// @param converted_path the resulting converted path.  This is set
  /// iff the function returns true.
  ///
  /// @return true if the path could be converted to being relative to
  /// the extracted directory.
  bool
  convert_path_to_relative(const string& path, string& converted_path) const
  {
    string root = extracted_dir_path_;
    real_path(root, root);
    string p = path;
    real_path(p, p);
    return string_suffix(p, root, converted_path);
  }

  // Convert the absolute path of an element of this package into a
  // path relative to the prefix common to the paths of all elements
  // of the package.
  //
  // @param path the path to conver.
  //
  // @param converted_path the resulting converted path.  This is set
  // iff the function returns true.
  //
  // @return true iff the function could successfully convert @p path
  // and put the result into @p converted_path.
  bool
  convert_path_to_unique_suffix(const string& path,
				string& converted_path) const
  {return string_suffix(path, common_paths_prefix(), converted_path);}

  /// Retrieve the set of "interesting" package element paths by
  /// walking the package.
  ///
  /// And then compute the path prefix that is common to all the
  /// collected elements.
  ///
  /// @param the options of this application.
  void
  load_elf_file_paths(options& opts)
  {
    if (!common_paths_prefix().empty()
	|| !elf_file_paths().empty())
      // We have already loaded the elf file paths, don't do it again.
      return;

    get_interesting_files_under_dir(extracted_dir_path(),
				    /*file_name_to_look_for=*/"",
				    opts, elf_file_paths());
    std::sort(elf_file_paths().begin(), elf_file_paths().end());
    string common_prefix;
    sorted_strings_common_prefix(elf_file_paths(), common_paths_prefix());
  }

  /// Create the path of an ABI file to be associated with a given
  /// binary.
  ///
  /// @param elf_file_path the path to the binary to consider.
  ///
  /// @param abi_file_path the resulting ABI file path.  This is set
  /// iff the function return true.
  ///
  /// @return true if the ABI file path could be constructed and the
  /// directory tree containing it could be created.  In that case,
  /// the resulting ABI file path is set to the @p abi_file_path
  /// output parameter.
  bool
  create_abi_file_path(const string &elf_file_path,
		       string &abi_file_path) const
  {
    string abi_path, dir, parent;
    if (!abigail::tools_utils::string_suffix(elf_file_path,
					     extracted_dir_path(),
					     abi_path))
      return false;
    abi_path = extracted_dir_path() + "/abixml" + abi_path + ".abi";
    if (!abigail::tools_utils::ensure_parent_dir_created(abi_path))
      return false;
    abi_file_path = abi_path;
    return true;
  }

  /// Erase the content of the temporary extraction directory that has
  /// been populated by the @ref extract_package() function;
  ///
  /// @param opts the options passed to the current program.
  void
  erase_extraction_directory(const options &opts) const
  {
    if (type() == abigail::tools_utils::FILE_TYPE_DIR)
      // If we are comparing two directories, do not erase the
      // directory as it was provided by the user; it's not a
      // temporary directory we created ourselves.
      return;

    if (opts.verbose)
      emit_prefix("abipkgdiff", cerr)
	<< "Erasing temporary extraction directory "
	<< extracted_dir_path()
	<< " ...";

    string cmd = "rm -rf " + extracted_dir_path();
    if (system(cmd.c_str()))
      {
	if (opts.verbose)
	  emit_prefix("abipkgdiff", cerr) << " FAILED\n";
      }
    else
      {
	if (opts.verbose)
	  emit_prefix("abipkgdiff", cerr) << " DONE\n";
      }
  }

  /// Erase the content of all the temporary extraction directories.
  ///
  /// @param opts the options passed to the current program.
  void
  erase_extraction_directories(const options &opts) const
  {
    erase_extraction_directory(opts);
    if (!debug_info_packages().empty())
      debug_info_packages().front()->erase_extraction_directory(opts);
    if (devel_package())
      devel_package()->erase_extraction_directory(opts);
    if (kabi_whitelist_package())
      kabi_whitelist_package()->erase_extraction_directory(opts);
  }
}; // end class package.

/// Arguments passed to the comparison tasks.
struct compare_args
{
  const elf_file		elf1;
  const string&		debug_dir1;
  const suppressions_type	private_types_suppr1;
  const elf_file		elf2;
  const string&		debug_dir2;
  const suppressions_type	private_types_suppr2;
  const options&		opts;

  /// Constructor for compare_args, which is used to pass
  /// information to the comparison threads.
  ///
  /// @param elf1 the first elf file to consider.
  ///
  /// @param debug_dir1 the directory where the debug info file for @p
  /// elf1 is stored.
  ///
  /// @param elf2 the second elf file to consider.
  ///
  /// @param debug_dir2 the directory where the debug info file for @p
  /// elf2 is stored.
  ///
  /// @param opts the options the current program has been called with.
  compare_args(const elf_file &elf1, const string& debug_dir1,
	       const suppressions_type& priv_types_suppr1,
	       const elf_file &elf2, const string& debug_dir2,
	       const suppressions_type& priv_types_suppr2,
	       const options& opts)
    : elf1(elf1), debug_dir1(debug_dir1),
      private_types_suppr1(priv_types_suppr1),
      elf2(elf2), debug_dir2(debug_dir2),
      private_types_suppr2(priv_types_suppr2),
      opts(opts)
  {}
}; // end struct compare_args

/// A convenience typedef for arguments passed to the comparison workers.
typedef shared_ptr<compare_args> compare_args_sptr;

static bool extract_package_and_map_its_content(const package_sptr &pkg,
						options &opts);

/// Getter for the path to the parent directory under which packages
/// extracted by the current thread are placed.
///
/// @return the path to the parent directory under which packages
/// extracted by the current thread are placed.
const string&
package::extracted_packages_parent_dir()
{
  // I tried to declare this in thread-local storage, but GCC 4.4.7
  // won't let me.  So for now, I am just making it static.  I'll deal
  // with this later when I have to.

  //static __thread string p;
  static string p;

  if (p.empty())
    {
      const char *cachedir = getenv("XDG_CACHE_HOME");

      if (cachedir != NULL)
        p = cachedir;
      else
        {
	  const char* s = getenv("HOME");
	  if (s != NULL)
	    p = s;
	  if (p.empty())
	    {
	      s = getenv("TMPDIR");
	      if (s != NULL)
		p = s;
	      else
		p = "/tmp";
	    }
	  p += "/.cache/libabigail";
        }

      // Create the cache directory if it doesn't exist
      ABG_ASSERT(ensure_dir_path_created(p));

      string libabigail_tmp_dir_template = p;
      libabigail_tmp_dir_template += "/abipkgdiff-tmp-dir-XXXXXX";

      if (!mkdtemp(const_cast<char*>(libabigail_tmp_dir_template.c_str())))
	abort();

      p = libabigail_tmp_dir_template;
    }

  return p;
}

/// A convenience typedef for shared_ptr of package.
typedef shared_ptr<package> package_sptr;

/// Show the usage of this program.
///
/// @param prog_name the name of the program.
///
/// @param out the output stream to emit the usage to .
static void
display_usage(const string& prog_name, ostream& out)
{
  emit_prefix(prog_name, out)
    << "usage: " << prog_name << " [options] <package1> <package2>\n"
    << " where options can be:\n"
    << " --debug-info-pkg1|--d1 <path>  path of debug-info package of package1\n"
    << " --debug-info-pkg2|--d2 <path>  path of debug-info package of package2\n"
    << " --devel-pkg1|--devel1 <path>   path of devel package of pakage1\n"
    << " --devel-pkg2|--devel2 <path>   path of devel package of pakage1\n"
    << " --drop-private-types  drop private types from "
    "internal representation\n"
    << " --no-default-suppression       don't load any default "
       "suppression specifications\n"
    << " --suppressions|--suppr <path>  specify supression specification path\n"
    << " --linux-kernel-abi-whitelist|-w path to a "
    "linux kernel abi whitelist\n"
    << " --wp <path>                    path to a linux kernel abi whitelist package\n"
    << " --keep-tmp-files               don't erase created temporary files\n"
    << " --dso-only                     compare shared libraries only\n"
    << " --private-dso                  compare DSOs that are private "
    "to the package as well\n"
    << " --leaf-changes-only|-l  only show leaf changes, "
    "so no change impact analysis (implies --redundant)\n"
    << " --impacted-interfaces|-i  display interfaces impacted by leaf changes\n"
    << " --full-impact|-f  when comparing kernel packages, show the "
    "full impact analysis report rather than the default leaf changes reports\n"
    << " --non-reachable-types|-t  consider types non reachable"
    " from public interfaces\n"
    << " --no-linkage-name		do not display linkage names of "
    "added/removed/changed\n"
    << " --redundant                    display redundant changes\n"
    << " --harmless                     display the harmless changes\n"
    << " --no-show-locs                 do not show location information\n"
    << " --show-bytes  show size and offsets in bytes\n"
    << " --show-bits  show size and offsets in bits\n"
    << " --show-hex  show size and offset in hexadecimal\n"
    << " --show-dec  show size and offset in decimal\n"
    << " --no-show-relative-offset-changes  do not show relative"
    " offset changes\n"
    << " --no-added-syms                do not display added functions or variables\n"
    << " --no-unreferenced-symbols do not display changes "
    "about symbols not referenced by debug info\n"
    << " --no-added-binaries            do not display added binaries\n"
    << " --no-abignore                  do not look for *.abignore files\n"
    << " --no-parallel                  do not execute in parallel\n"
    << " --fail-no-dbg                  fail if no debug info was found\n"
    << " --show-identical-binaries      show the names of identical binaries\n"
    << " --verbose                      emit verbose progress messages\n"
    << " --self-check                   perform a sanity check by comparing "
    "binaries inside the input package against their ABIXML representation\n"
    << " --help|-h                      display this help message\n"
    << " --version|-v                   display program version information"
    " and exit\n";
}

#ifdef WITH_RPM

/// Extract an RPM package.
///
/// @param package_path the path to the package to extract.
///
/// @param extracted_package_dir_path the path where to extract the
/// package to.
///
/// @param opts the options passed to the current program.
///
/// @return true upon successful completion, false otherwise.
static bool
extract_rpm(const string& package_path,
	    const string& extracted_package_dir_path,
	    const options &opts)
{
  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Extracting package "
      << package_path
      << " to "
      << extracted_package_dir_path
      << " ...";

  string cmd = "test -d " + extracted_package_dir_path
    + " || mkdir -p " + extracted_package_dir_path + " ; cd " +
    extracted_package_dir_path + " && rpm2cpio " + package_path +
    " | cpio -dium --quiet";

  if (system(cmd.c_str()))
    {
      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr) << " FAILED\n";
      return false;
    }

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr) << " DONE\n";

  return true;
}

#endif // WITH_RPM

#ifdef WITH_DEB

/// Extract a Debian binary package.
///
/// @param package_path the path to the package to extract.
///
/// @param extracted_package_dir_path the path where to extract the
/// package to.
///
/// @param opts the options passed to the current program.
///
/// @return true upon successful completion, false otherwise.
static bool
extract_deb(const string& package_path,
	    const string& extracted_package_dir_path,
	    const options &opts)
{
  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Extracting package "
      << package_path
      << " to "
      << extracted_package_dir_path
      << " ...\n";

  string cmd = "mkdir -p " + extracted_package_dir_path + " && dpkg -x " +
    package_path + " " + extracted_package_dir_path;

  if (system(cmd.c_str()))
    {
      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr) << " FAILED\n";
      return false;
    }

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr) << " DONE\n";

  return true;
}

#endif // WITH_DEB

#ifdef WITH_TAR

/// Extract a GNU Tar archive.
///
/// @param package_path the path to the archive to extract.
///
/// @param extracted_package_dir_path the path where to extract the
/// archive to.
///
/// @param opts the options passed to the current program.
///
/// @return true upon successful completion, false otherwise.
static bool
extract_tar(const string& package_path,
	    const string& extracted_package_dir_path,
	    const options &opts)
{
  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Extracting tar archive "
      << package_path
      << " to "
      << extracted_package_dir_path
      << " ...";

  string cmd = "test -d " +
    extracted_package_dir_path +
    " && rm -rf " + extracted_package_dir_path;

  if (system(cmd.c_str()))
    {
      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr) << "command " << cmd << " FAILED\n";
    }

  cmd = "mkdir -p " + extracted_package_dir_path + " && cd " +
    extracted_package_dir_path + " && tar -xf " + package_path;

  if (system(cmd.c_str()))
    {
      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr) << " FAILED\n";
      return false;
    }

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr) << " DONE\n";

  return true;
}

#endif // WITH_TAR

/// Erase the temporary directories created for the extraction of two
/// packages.
///
/// @param first_package the first package to consider.
///
/// @param opts the options passed to the current program.
///
/// @param second_package the second package to consider.
static void
erase_created_temporary_directories(const package& first_package,
				    const package& second_package,
				    const options &opts)
{
  first_package.erase_extraction_directories(opts);
  second_package.erase_extraction_directories(opts);
}

/// Erase the root of all the temporary directories created by the
/// current thread.
static void
erase_created_temporary_directories_parent(const options &opts)
{
  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Erasing temporary extraction parent directory "
      << package::extracted_packages_parent_dir()
      << " ...";

  string cmd = "rm -rf " + package::extracted_packages_parent_dir();
  if (system(cmd.c_str()))
    {
      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr) << "FAILED\n";
    }
  else
    {
      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr) << "DONE\n";
    }
}

/// Extract the content of a package.
///
/// @param package the package we are looking at.
///
/// @param opts the options passed to the current program.
static bool
extract_package(const package& package,
		const options &opts)
{
  switch(package.type())
    {
    case abigail::tools_utils::FILE_TYPE_RPM:
#ifdef WITH_RPM
      if (!extract_rpm(package.path(), package.extracted_dir_path(), opts))
        {
          emit_prefix("abipkgdiff", cerr)
	    << "Error while extracting package " << package.path() << "\n";
          return false;
        }
      return true;
#else
      emit_prefix("abipkgdiff", cerr)
	<< "Support for rpm hasn't been enabled.  Please consider "
      "enabling it at package configure time\n";
      return false;
#endif // WITH_RPM
      break;
    case abigail::tools_utils::FILE_TYPE_DEB:
#ifdef WITH_DEB
      if (!extract_deb(package.path(), package.extracted_dir_path(), opts))
        {
          emit_prefix("abipkgdiff", cerr)
	    << "Error while extracting package" << package.path() << "\n";
          return false;
        }
      return true;
#else
      emit_prefix("abipkgdiff", cerr)
	<< "Support for deb hasn't been enabled.  Please consider "
	"enabling it at package configure time\n";
      return false;
#endif // WITH_DEB
      break;

    case  abigail::tools_utils::FILE_TYPE_DIR:
      // The input package is just a directory that contains binaries,
      // there is nothing to extract.
      break;

    case abigail::tools_utils::FILE_TYPE_TAR:
#ifdef WITH_TAR
      if (!extract_tar(package.path(), package.extracted_dir_path(), opts))
        {
          emit_prefix("abipkgdiff", cerr)
	    << "Error while extracting GNU tar archive "
	    << package.path() << "\n";
          return false;
        }
      return true;
#else
      emit_prefix("abipkgdiff", cerr)
	<< "Support for GNU tar hasn't been enabled.  Please consider "
	"enabling it at package configure time\n";
      return false;
#endif // WITH_TAR
      break;

    default:
      return false;
    }
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
    if (!check_file(*i, cerr, opts.prog_name))
      return false;

  for (vector<string>::const_iterator i =
	 opts.kabi_whitelist_paths.begin();
       i != opts.kabi_whitelist_paths.end();
       ++i)
    if (!check_file(*i, cerr, "abidiff"))
      return false;

  return true;
}

/// Update the diff context from the @ref options data structure.
///
/// @param ctxt the diff context to update.
///
/// @param opts the instance of @ref options to consider.
static void
set_diff_context_from_opts(diff_context_sptr ctxt,
			   const options& opts)
{
  ctxt->default_output_stream(&cout);
  ctxt->error_output_stream(&cerr);
  // See comment in abidiff.cc's set_diff_context_from_opts.
  ctxt->show_redundant_changes(opts.show_redundant_changes
                               || opts.leaf_changes_only);
  ctxt->show_leaf_changes_only(opts.leaf_changes_only);
  ctxt->show_impacted_interfaces(opts.show_impacted_interfaces);
  ctxt->show_unreachable_types(opts.show_all_types);
  ctxt->show_hex_values(opts.show_hexadecimal_values);
  ctxt->show_offsets_sizes_in_bits(opts.show_offsets_sizes_in_bits);
  ctxt->show_relative_offset_changes(opts.show_relative_offset_changes);
  ctxt->show_locs(opts.show_locs);
  ctxt->show_linkage_names(opts.show_linkage_names);
  ctxt->show_added_fns(opts.show_added_syms);
  ctxt->show_added_vars(opts.show_added_syms);
  ctxt->show_added_symbols_unreferenced_by_debug_info
    (opts.show_added_syms);
  ctxt->show_symbols_unreferenced_by_debug_info
    (opts.show_symbols_not_referenced_by_debug_info);

  if (!opts.show_harmless_changes)
    ctxt->switch_categories_off(get_default_harmless_categories_bitmap());

  suppressions_type supprs;
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    read_suppressions(*i, supprs);
  ctxt->add_suppressions(supprs);
}

/// Compare the ABI two elf files, using their associated debug info.
///
/// The result of the comparison is emitted to standard output.
///
/// @param elf1 the first elf file to consider.
///
/// @param debug_dir1 the directory where the debug info file for @p
/// elf1 is stored.
/// The result of the comparison is saved to a global corpus map.
///
/// @param elf2 the second eld file to consider.
/// @args the list of argument sets used for comparison
///
/// @param debug_dir2 the directory where the debug info file for @p
/// elf2 is stored.
///
/// @param opts the options the current program has been called with.
///
/// @param env the environment encapsulating the entire comparison.
///
/// @param diff the shared pointer to be set to the result of the comparison.
///
/// @param detailed_error_status is this pointer is non-null and if
/// the function returns ABIDIFF_ERROR, then the function sets the
/// pointed-to parameter to the abigail::dwarf_reader::status value
/// that gives details about the rror.
///
/// @return the status of the comparison.
static abidiff_status
compare(const elf_file& elf1,
	const string&	debug_dir1,
	const suppressions_type& priv_types_supprs1,
	const elf_file& elf2,
	const string&	debug_dir2,
	const suppressions_type& priv_types_supprs2,
	const options&	opts,
	abigail::ir::environment_sptr	&env,
	corpus_diff_sptr	&diff,
	diff_context_sptr	&ctxt,
	abigail::dwarf_reader::status *detailed_error_status = 0)
{
  char *di_dir1 = (char*) debug_dir1.c_str(),
	*di_dir2 = (char*) debug_dir2.c_str();

  vector<char**> di_dirs1, di_dirs2;
  di_dirs1.push_back(&di_dir1);
  di_dirs2.push_back(&di_dir2);

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Comparing the ABIs of file "
      << elf1.path
      << " and "
      << elf2.path
      << "...\n";

  abigail::dwarf_reader::status c1_status = abigail::dwarf_reader::STATUS_OK,
    c2_status = abigail::dwarf_reader::STATUS_OK;

  ctxt.reset(new diff_context);
  set_diff_context_from_opts(ctxt, opts);
  suppressions_type& supprs = ctxt->suppressions();
  bool files_suppressed = (file_is_suppressed(elf1.path, supprs)
			   ||file_is_suppressed(elf2.path, supprs));

  if (files_suppressed)
    {
      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr)
	  << "  input file "
	  << elf1.path << " or " << elf2.path
	  << " has been suppressed by a suppression specification.\n"
	  << " Not reading any of them\n";
      return abigail::tools_utils::ABIDIFF_OK;
    }

  // Add the first private type suppressions set to the set of
  // suppressions.
  for (suppressions_type::const_iterator i = priv_types_supprs1.begin();
       i != priv_types_supprs1.end();
       ++i)
    supprs.push_back(*i);

  // Add the second private type suppressions set to the set of
  // suppressions.
  for (suppressions_type::const_iterator i = priv_types_supprs2.begin();
       i != priv_types_supprs2.end();
       ++i)
    supprs.push_back(*i);

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Reading file "
      << elf1.path
      << " ...\n";

  corpus_sptr corpus1;
  {
    read_context_sptr c =
      create_read_context(elf1.path, di_dirs1, env.get(),
			  /*load_all_types=*/opts.show_all_types);
    add_read_context_suppressions(*c, priv_types_supprs1);
    if (!opts.kabi_suppressions.empty())
      add_read_context_suppressions(*c, opts.kabi_suppressions);

    corpus1 = read_corpus_from_elf(*c, c1_status);

    bool bail_out = false;
    if (!(c1_status & abigail::dwarf_reader::STATUS_OK))
      {
	if (opts.verbose)
	  emit_prefix("abipkgdiff", cerr)
	    << "Could not read file '"
	    << elf1.path
	    << "' properly\n";

	if (detailed_error_status)
	  *detailed_error_status = c1_status;

	bail_out = true;
      }

    if (opts.fail_if_no_debug_info)
      {
	bool debug_info_error = false;
	if (c1_status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
	  {
	    if (opts.verbose)
	      emit_prefix("abipkgdiff", cerr)
		<< "while reading file" << elf1.path << "\n";

	    emit_prefix("abipkgdiff", cerr) << "Could not find debug info file";
	    if (di_dir1 && strcmp(di_dir1, ""))
	      cerr << " under " << di_dir1 << "\n";
	    else
	       cerr << "\n";

	    if (detailed_error_status)
	      *detailed_error_status = c1_status;
	    debug_info_error = true;
	  }

	if (c1_status & abigail::dwarf_reader::STATUS_ALT_DEBUG_INFO_NOT_FOUND)
	  {
	    if (opts.verbose)
	      emit_prefix("abipkgdiff", cerr)
		<< "while reading file" << elf1.path << "\n";

	    emit_prefix("abipkgdiff", cerr)
	      << "Could not find alternate debug info file";
	    string alt_di_path;
	    abigail::dwarf_reader::refers_to_alt_debug_info(*c, alt_di_path);
	    if (!alt_di_path.empty())
	      cerr << ": " << alt_di_path << "\n";
	    else
	      cerr << "\n";

	    if (detailed_error_status)
	      *detailed_error_status = c1_status;
	    debug_info_error = true;
	  }

	if (debug_info_error)
	  bail_out = true;
      }

    if (bail_out)
      return abigail::tools_utils::ABIDIFF_ERROR;
  }

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "DONE reading file "
      << elf1.path
      << "\n";

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Reading file "
      << elf2.path
      << " ...\n";

  corpus_sptr corpus2;
  {
    read_context_sptr c =
      create_read_context(elf2.path, di_dirs2, env.get(),
			  /*load_all_types=*/opts.show_all_types);
    add_read_context_suppressions(*c, priv_types_supprs2);

    if (!opts.kabi_suppressions.empty())
      add_read_context_suppressions(*c, opts.kabi_suppressions);

    corpus2 = read_corpus_from_elf(*c, c2_status);

    bool bail_out = false;
    if (!(c2_status & abigail::dwarf_reader::STATUS_OK))
      {
	if (opts.verbose)
	  emit_prefix("abipkgdiff", cerr)
	    << "Could not find the read file '"
	    << elf2.path
	    << "' properly\n";

	if (detailed_error_status)
	  *detailed_error_status = c2_status;

	bail_out = true;
      }

    if (opts.fail_if_no_debug_info)
      {
	bool debug_info_error = false;
	if (c2_status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
	  {
	    if (opts.verbose)
	      emit_prefix("abipkgdiff", cerr)
		<< "while reading file" << elf2.path << "\n";

	    emit_prefix("abipkgdiff", cerr) << "Could not find debug info file";
	    if (di_dir2 && strcmp(di_dir2, ""))
	      cerr << " under " << di_dir2 << "\n";
	    else
	      cerr << "\n";

	    if (detailed_error_status)
	      *detailed_error_status = c2_status;
	    debug_info_error = true;
	  }

	if (c2_status & abigail::dwarf_reader::STATUS_ALT_DEBUG_INFO_NOT_FOUND)
	  {
	    if (opts.verbose)
	      emit_prefix("abipkgdiff", cerr)
		<< "while reading file" << elf2.path << "\n";

	    emit_prefix("abipkgdiff", cerr)
	      << "Could not find alternate debug info file";
	    string alt_di_path;
	    abigail::dwarf_reader::refers_to_alt_debug_info(*c, alt_di_path);
	    if (!alt_di_path.empty())
	      cerr << ": " << alt_di_path << "\n";
	    else
	      cerr << "\n";

	    if (detailed_error_status)
	      *detailed_error_status = c2_status;
	    debug_info_error = true;
	  }

	if (debug_info_error)
	  bail_out = true;
      }

    if (bail_out)
      return abigail::tools_utils::ABIDIFF_ERROR;
  }

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << " DONE reading file " << elf2.path << "\n";

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "  Comparing the ABIs of: \n"
      << "    " << elf1.path << "\n"
      << "    " << elf2.path << "\n";

  diff = compute_diff(corpus1, corpus2, ctxt);

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Comparing the ABIs of file "
      << elf1.path
      << " and "
      << elf2.path
      << " is DONE\n";

  abidiff_status s = abigail::tools_utils::ABIDIFF_OK;
  if (diff->has_net_changes())
    s |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;
  if (diff->has_incompatible_changes())
    s |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;

  return s;
}

/// Compare an ELF file to its ABIXML representation.
///
/// @param elf the ELF file to compare.
///
/// @param debug_dir the debug directory of the ELF file.
///
/// @param opts the options passed the user.
///
/// @param env the environment to use for the comparison.
///
/// @param diff the diff object resulting from the comparison of @p
/// elf against its ABIXML representation.
///
/// @param ctxt the resulting diff context used for the comparison
/// that yielded @p diff.
///
/// @param detailed_error_status the detailed error satus returned by
/// this function.
///
/// @return the status of the self comparison.
static abidiff_status
compare_to_self(const elf_file& elf,
		const string&	debug_dir,
		const options&	opts,
		abigail::ir::environment_sptr	&env,
		corpus_diff_sptr	&diff,
		diff_context_sptr	&ctxt,
		abigail::dwarf_reader::status *detailed_error_status = 0)
{
  char *di_dir = (char*) debug_dir.c_str();

  vector<char**> di_dirs;
  di_dirs.push_back(&di_dir);

  abigail::dwarf_reader::status c_status = abigail::dwarf_reader::STATUS_OK;

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Comparing the ABI of file '"
      << elf.path
      << "' against itself ...\n";

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Reading file "
      << elf.path
      << " ...\n";

  corpus_sptr corp;
  {
    read_context_sptr c =
      create_read_context(elf.path, di_dirs, env.get(),
			  /*read_all_types=*/opts.show_all_types);

    corp = read_corpus_from_elf(*c, c_status);

    if (!(c_status & abigail::dwarf_reader::STATUS_OK))
      {
	if (opts.verbose)
	  emit_prefix("abipkgdiff", cerr)
	    << "Could not read file '"
	    << elf.path
	    << "' propertly\n";

	if (detailed_error_status)
	  *detailed_error_status = c_status;

	return abigail::tools_utils::ABIDIFF_ERROR;
      }

    if (opts.verbose)
      emit_prefix("abipkgdiff", cerr)
	<< "Read file '"
	<< elf.path
	<< "' OK\n";


    ABG_ASSERT(corp);
  }

  corpus_sptr reread_corp;
  string abi_file_path;
  {
    if (!opts.pkg1->create_abi_file_path(elf.path, abi_file_path))
      {
	if (opts.verbose)
	  emit_prefix("abipkgdiff", cerr)
	    << "Could not create the directory tree to store the abi for '"
	    << elf.path
	    << "'\n";
      }
    ofstream of(abi_file_path.c_str(), std::ios_base::trunc);

    {
      const abigail::xml_writer::write_context_sptr c =
	abigail::xml_writer::create_write_context(env.get(), of);

      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr)
	  << "Writting ABIXML file '"
	  << abi_file_path
	  << "' ...\n";

      if (!write_corpus(*c, corp, 0))
	{
	  if (opts.verbose)
	    emit_prefix("abipkgdiff", cerr)
	      << "Could not write the ABIXML file to '"
	      << abi_file_path << "'\n";

	  return abigail::tools_utils::ABIDIFF_ERROR;
	}

      of.flush();
      of.close();

      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr)
	  << "Wrote ABIXML file '"
	  << abi_file_path
	  << "' OK\n";
    }

    {
      abigail::xml_reader::read_context_sptr c =
	abigail::xml_reader::create_native_xml_read_context(abi_file_path,
							    env.get());
      if (!c)
	{
	  if (opts.verbose)
	    emit_prefix("abipkgdiff", cerr)
	      << "Could not create read context for ABIXML file '"
	      << abi_file_path << "'\n";

	  return abigail::tools_utils::ABIDIFF_ERROR;
	}

      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr)
	  << "Reading ABIXML file '"
	  << abi_file_path
	  << "' ...\n";

      reread_corp = read_corpus_from_input(*c);
      if (!reread_corp)
	{
	  if (opts.verbose)
	    emit_prefix("abipkgdiff", cerr)
	      << "Could not read temporary ABIXML file '"
	      << abi_file_path << "'\n";

	  return abigail::tools_utils::ABIDIFF_ERROR;
	}

      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr)
	  << "Read file '"
	  << abi_file_path
	  << "' OK\n";
    }
  }

  ctxt.reset(new diff_context);
  set_diff_context_from_opts(ctxt, opts);

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Comparing the ABIs of: \n"
      << "   '" << corp->get_path() << "' against \n"
      << "   '" << abi_file_path << "'...\n";

  diff = compute_diff(corp, reread_corp, ctxt);
  if (opts.verbose)
    emit_prefix("abipkgdfiff", cerr)
      << "... Comparing the ABIs: DONE\n";

  abidiff_status s = abigail::tools_utils::ABIDIFF_OK;
  if (diff->has_changes())
    s |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;
  if (diff->has_incompatible_changes())
    s |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;

  if (opts.verbose)
    emit_prefix("abipkgdfiff", cerr)
      << "Comparison against self "
      << (s == abigail::tools_utils::ABIDIFF_OK ? "SUCCEEDED" : "FAILED")
      << '\n';

  return s;
}

/// If devel packages were associated to the main package we are
/// looking at, use the names of the header files (extracted from the
/// package) to generate suppression specification to filter out types
/// that are not defined in those header files.
///
/// Filtering out types not defined in publi headers amounts to filter
/// out types that are deemed private to the package we are looking
/// at.
///
/// If the function succeeds, it returns a non-empty vector of
/// suppression specifications.
///
/// @param pkg the main package we are looking at.
///
/// @param opts the options of the current program.
///
/// @return a vector of suppression_sptr.  If no suppressions
/// specification were constructed, the returned vector is empty.
static suppressions_type
create_private_types_suppressions(const package& pkg, const options &opts)
{
  suppressions_type supprs;

  package_sptr devel_pkg = pkg.devel_package();
  if (!devel_pkg
      || !file_exists(devel_pkg->extracted_dir_path())
      || !is_dir(devel_pkg->extracted_dir_path()))
    return supprs;

  string headers_path = devel_pkg->extracted_dir_path();
  if (devel_pkg->type() == abigail::tools_utils::FILE_TYPE_RPM
      ||devel_pkg->type() == abigail::tools_utils::FILE_TYPE_DEB)
    // For RPM and DEB packages, header files are under the
    // /usr/include sub-directories.
    headers_path += "/usr/include";

  if (!is_dir(headers_path))
    return supprs;

  suppression_sptr suppr =
    gen_suppr_spec_from_headers(headers_path);

  if (suppr)
    {
      if (opts.drop_private_types)
	suppr->set_drops_artifact_from_ir(true);
      supprs.push_back(suppr);
    }

  return supprs;
}

/// If the user wants to avoid comparing DSOs that are private to this
/// package, then we build the set of public DSOs as advertised in the
/// package's "provides" property.
///
/// Note that at the moment this function only works for RPMs.  It
/// doesn't yet support other packaging formats.
///
/// @param pkg the package to consider.
///
/// @param opts the options of this program.
///
/// @return true iff the set of public DSOs was built.
static bool
maybe_create_public_dso_sonames_set(package& pkg, const options &opts)
{
  if (opts.compare_private_dsos || !pkg.public_dso_sonames().empty())
    return false;

  if (pkg.type() == abigail::tools_utils::FILE_TYPE_RPM)
    return get_dsos_provided_by_rpm(pkg.path(), pkg.public_dso_sonames());

  // We don't support this yet for non-RPM packages.
  return false;
}

/// Test if we should only compare the public DSOs of a given package.
///
/// @param pkg the package to consider.
///
/// @param opts the options of this program
static bool
must_compare_public_dso_only(package& pkg, options& opts)
{
  if (pkg.type() == abigail::tools_utils::FILE_TYPE_RPM
      && !opts.compare_private_dsos)
    return true;

  return false;
}

/// While walking a file directory, check if a directory entry is a
/// kabi whitelist of a particular architecture.
///
/// If it is, then save its file path in a vector of whitelists.
///
/// @param entry the directory entry to consider.
///
/// @param arch the architecture to consider.
///
/// @param whitelists out parameter.  If @p entry is the whitelist we
/// are looking for, add its path to this output parameter.
static void
maybe_collect_kabi_whitelists(const FTSENT *entry,
			      const string arch,
			      vector<string> &whitelists)
{
  if (entry == NULL
      || (entry->fts_info != FTS_F && entry->fts_info != FTS_SL)
      || entry->fts_info == FTS_ERR
      || entry->fts_info == FTS_NS)
    return;

  string path = entry->fts_path;
  maybe_get_symlink_target_file_path(path, path);

  string kabi_whitelist_name = "kabi_whitelist_" + arch;

  if (string_ends_with(path, kabi_whitelist_name))
    whitelists.push_back(path);
}

/// Get the kabi whitelist for a particular architecture under a given
/// directory.
///
/// @param dir the directory to look at.
///
/// @param arch the architecture to consider.
///
/// @param whitelist_paths the vector where to add the whitelists
/// found.  Note that a whitelist is added to this parameter iff the
/// function returns true.
///
/// @return true iff the function found a whitelist at least.
static bool
get_kabi_whitelists_from_arch_under_dir(const string& dir,
					const string& arch,
					vector<string>& whitelist_paths)
{
 bool is_ok = false;
  char* paths[] = {const_cast<char*>(dir.c_str()), 0};

  FTS *file_hierarchy = fts_open(paths, FTS_LOGICAL|FTS_NOCHDIR, NULL);
  if (!file_hierarchy)
    return is_ok;

  FTSENT *entry;
  while ((entry = fts_read(file_hierarchy)))
    maybe_collect_kabi_whitelists(entry, arch, whitelist_paths);

  fts_close(file_hierarchy);

  return true;
}

/// Find a kabi whitelist in a linux kernel RPM package.
///
/// Note that the linux kernel RPM package must have been extracted
/// somewhere already.
///
/// This function then looks for the whitelist under the /lib/modules
/// directory inside the extracted content of the package.  If it
/// finds it and saves its file path in the
/// options::kabi_whitelist_paths data member.
///
/// @param pkg the linux kernel package to consider.
///
/// @param opts the options the program was invoked with.
static bool
maybe_handle_kabi_whitelist_pkg(const package& pkg, options &opts)
{
  if (opts.kabi_whitelist_packages.empty()
      || !opts.kabi_whitelist_paths.empty()
      || !pkg.kabi_whitelist_package())
    return false;

  if (pkg.type() != abigail::tools_utils::FILE_TYPE_RPM)
    return false;

  string pkg_name = pkg.base_name();
  bool is_linux_kernel_package = file_is_kernel_package(pkg_name, pkg.type());

  if (!is_linux_kernel_package)
    return false;

  package_sptr kabi_wl_pkg = pkg.kabi_whitelist_package();
  assert(kabi_wl_pkg);

  if (!file_exists(kabi_wl_pkg->extracted_dir_path())
      || !is_dir(kabi_wl_pkg->extracted_dir_path()))
    return false;

  string rpm_arch;
  if (!get_rpm_arch(pkg_name, rpm_arch))
    return false;

  string kabi_wl_path = kabi_wl_pkg->extracted_dir_path();
  kabi_wl_path += "/lib/modules";
  vector<string> whitelist_paths;

  get_kabi_whitelists_from_arch_under_dir(kabi_wl_path, rpm_arch,
					  whitelist_paths);

  if (!whitelist_paths.empty())
    {
      std::sort(whitelist_paths.begin(), whitelist_paths.end());
      opts.kabi_whitelist_paths.push_back(whitelist_paths.back());
    }

  return true;
}

/// The task that performs the extraction of the content of several
/// packages into a temporary directory.
///
/// If this task has several packages to extract, then it extracts
/// them in sequence.
///
/// Note that several instances of tasks can perform their jobs (i.e
/// extract packages in sequence) in parallel.
class pkg_extraction_task : public task
{
  pkg_extraction_task();

public:
  vector<package_sptr> pkgs;
  const options &opts;
  bool is_ok;

  pkg_extraction_task(const package_sptr &p, const options &o)
    : opts(o), is_ok(true)
  {pkgs.push_back(p);}

  pkg_extraction_task(const vector<package_sptr> &packages, const options &o)
    : pkgs(packages), opts(o), is_ok(true)
  {}

  /// The job performed by the current task, which is to extract its
  /// packages in sequence.  This job is to be performed in parallel
  /// with other jobs of other tasks.
  virtual void
  perform()
  {
    for (vector<package_sptr>::const_iterator p = pkgs.begin();
	 p != pkgs.end();
	 ++p)
      is_ok &= extract_package(**p, opts);
  }
}; //end class pkg_extraction_task

/// A convenience typedef for a shared pointer to @f pkg_extraction_task.
typedef shared_ptr<pkg_extraction_task> pkg_extraction_task_sptr;

/// The worker task which job is to prepares a package.
///
/// Preparing a package means:
///
/// 	1/ Extract the package and its ancillary packages.
///
/// 	2/ Analyze the extracted content, map that content so that we
/// 	determine what the ELF files to be analyze are.
class pkg_prepare_task : public abigail::workers::task
{
  pkg_prepare_task();

public:
  package_sptr pkg;
  options &opts;
  bool is_ok;

  pkg_prepare_task(package_sptr &p, options &o)
    : pkg(p), opts(o), is_ok(false)
  {}

  /// The job performed by this task.
  virtual void
  perform()
  {
    is_ok = pkg && extract_package_and_map_its_content(pkg, opts);
  }
}; //end class pkg_prepare_task

/// A convenience typedef for a shared_ptr to @ref pkg_prepare_task
typedef shared_ptr<pkg_prepare_task> pkg_prepare_task_sptr;

/// The worker task which job is to compare two ELF binaries
class compare_task : public abigail::workers::task
{
public:

  compare_args_sptr args;
  abidiff_status status;
  ostringstream out;
  string pretty_output;

  compare_task()
    : status(abigail::tools_utils::ABIDIFF_OK)
  {}

  compare_task(const compare_args_sptr& a)
    : args(a),
      status(abigail::tools_utils::ABIDIFF_OK)
  {}

  /// The job performed by the task.
  ///
  /// This compares two ELF files, gets the resulting test report and
  /// stores it in an output stream.
  virtual void
  perform()
  {
    abigail::ir::environment_sptr env(new abigail::ir::environment);
    diff_context_sptr ctxt;
    corpus_diff_sptr diff;

    abigail::dwarf_reader::status detailed_status =
      abigail::dwarf_reader::STATUS_UNKNOWN;

    status |= compare(args->elf1, args->debug_dir1, args->private_types_suppr1,
		      args->elf2, args->debug_dir2, args->private_types_suppr2,
		      args->opts, env, diff, ctxt, &detailed_status);

    // If there is an ABI change, tell the user about it.
    if ((status & abigail::tools_utils::ABIDIFF_ABI_CHANGE)
	||( diff && diff->has_net_changes()))
      {
	diff->report(out, /*prefix=*/"  ");
	string name = args->elf1.name;

	pretty_output +=
	  string("================ changes of '") + name + "'===============\n"
	  + out.str()
	  + "================ end of changes of '"
	  + name + "'===============\n\n";
      }
    else
      {
	if (args->opts.show_identical_binaries)
	  out << "No ABI change detected\n";
      }

    // If an error happened while comparing the two binaries, tell the
    // user about it.
    if (status & abigail::tools_utils::ABIDIFF_ERROR)
      {
	string diagnostic =
	  abigail::dwarf_reader::status_to_diagnostic_string(detailed_status);
	if (diagnostic.empty())
	  diagnostic =
	    "Unknown error.  Please run the tool again with --verbose\n";

	string name = args->elf1.name;
	pretty_output +=
	  "==== Error happened during processing of '" + name + "' ====\n";
	pretty_output += diagnostic;
	pretty_output +=
	  "==== End of error for '" + name + "' ====\n";
      }
  }
}; // end class compare_task

/// Convenience typedef for a shared_ptr of @ref compare_task.
typedef shared_ptr<compare_task> compare_task_sptr;

/// The worker task which job is to compare an ELF binary to its ABI
/// representation.
class self_compare_task : public compare_task
{
public:
  self_compare_task(const compare_args_sptr& a)
    : compare_task(a)
  {}

  /// The job performed by the task.
  ///
  /// This compares an ELF file to its ABIXML representation and
  /// expects the result to be the empty set.
  virtual void
  perform()
  {
    abigail::ir::environment_sptr env(new abigail::ir::environment);
    diff_context_sptr ctxt;
    corpus_diff_sptr diff;

    abigail::dwarf_reader::status detailed_status =
      abigail::dwarf_reader::STATUS_UNKNOWN;

    status |= compare_to_self(args->elf1, args->debug_dir1,
			      args->opts, env, diff, ctxt,
			      &detailed_status);

    string name = args->elf1.name;
    if (status == abigail::tools_utils::ABIDIFF_OK)
      pretty_output += "==== SELF CHECK SUCCEEDED for '"+ name + "' ====\n";
    else if ((status & abigail::tools_utils::ABIDIFF_ABI_CHANGE)
	     ||( diff && diff->has_net_changes()))
      {
	// There is an ABI change, tell the user about it.
	diff->report(out, /*indent=*/"  ");

	pretty_output +=
	  string("======== comparing'") + name +
	  "' to itself wrongly yielded result: ===========\n"
	  + out.str()
	  + "===SELF CHECK FAILED for '"+ name + "'\n";
      }

    // If an error happened while comparing the two binaries, tell the
    // user about it.
    if (status & abigail::tools_utils::ABIDIFF_ERROR)
      {
	string diagnostic =
	  abigail::dwarf_reader::status_to_diagnostic_string(detailed_status);

	if (diagnostic.empty())
	  diagnostic =
	    "Unknown error.  Please run the tool again with --verbose\n";

	string name = args->elf1.name;
	pretty_output +=
	  "==== Error happened during self check of '" + name + "' ====\n";
	pretty_output += diagnostic;
	pretty_output +=
	  "==== SELF CHECK FAILED for '" + name + "' ====\n";

      }
  }
}; // end class self_compare

/// Convenience typedef for a shared_ptr of @ref compare_task.
typedef shared_ptr<self_compare_task> self_compare_task_sptr;

/// This function is a sub-routine of create_maps_of_package_content.
///
/// It's called during the walking of the directory tree containing
/// the extracted content of package.  It's called with an entry of
/// that directory tree.
///
/// Depending on the kind of file this function is called on, it
/// updates the vector of paths of the directory and the set of
/// suppression paths found.
///
/// @param entry the directory entry to analyze.
///
/// @param opts the options of the current program.
///
/// @param file_name_to_look_for if this parameter is set, the
/// function only looks for a file name which name is the same as the
/// value of this parameter.
///
/// @param paths out parameter.  This is the set of meaningful paths
/// of the current directory tree being analyzed.  These paths are
/// those that are going to be involved in ABI comparison.
static void
maybe_update_package_content(const FTSENT *entry,
			     options &opts,
			     const string& file_name_to_look_for,
			     unordered_set<string>& paths)
{
  if (entry == NULL
      || (entry->fts_info != FTS_F && entry->fts_info != FTS_SL)
      || entry->fts_info == FTS_ERR
      || entry->fts_info == FTS_NS)
    return;

  string path = entry->fts_path;
  maybe_get_symlink_target_file_path(path, path);

  if (!file_name_to_look_for.empty())
    {
      string name;
      abigail::tools_utils::base_name(path, name);
      if (name == file_name_to_look_for)
	paths.insert(path);
      return;
    }

  if (guess_file_type(path) == abigail::tools_utils::FILE_TYPE_ELF)
    paths.insert(path);
  else if (opts.abignore && string_ends_with(path, ".abignore"))
    opts.suppression_paths.push_back(path);
}

/// Walk a given directory to collect files that are "interesting" to
/// analyze.  By default, "interesting" means interesting from either
/// a kernel package or a userspace binary analysis point of view.
///
/// @param dir the directory to walk.
///
/// @param file_name_to_look_for if this parameter is set, only a file
/// with this name is going to be collected.
///
/// @param interesting_files out parameter.  This parameter is
/// populated with the interesting files found by the function iff the
/// function returns true.
///
/// @return true iff the function completed successfully.
static bool
get_interesting_files_under_dir(const string dir,
				const string& file_name_to_look_for,
				options& opts,
				vector<string>& interesting_files)
{
  bool is_ok = false;
  string root;
  real_path(dir, root);
  if (root.empty())
    root = dir;

  char* paths[] = {const_cast<char*>(root.c_str()), 0};

  FTS *file_hierarchy = fts_open(paths, FTS_LOGICAL|FTS_NOCHDIR, NULL);
  if (!file_hierarchy)
    return is_ok;

  FTSENT *entry;
  unordered_set<string> files;
  while ((entry = fts_read(file_hierarchy)))
    maybe_update_package_content(entry, opts, file_name_to_look_for, files);

  for (unordered_set<string>::const_iterator i = files.begin();
       i != files.end();
       ++i)
    interesting_files.push_back(*i);

  fts_close(file_hierarchy);

  is_ok = true;

  return is_ok;
}

/// Create maps of the content of a given package.
///
/// The maps contain relevant metadata about the content of the
/// files.  These maps are used afterwards during the comparison of
/// the content of the package.  Note that the maps are stored in the
/// object that represents that package.
///
/// @param package the package to consider.
///
/// @param opts the options the current program has been called with.
///
/// @param true upon successful completion, false otherwise.
static bool
create_maps_of_package_content(package& package, options& opts)
{
  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Analyzing the content of package "
      << package.path()
      << " extracted to "
      << package.extracted_dir_path()
      << " ...\n";

  bool is_ok = true;
  vector<string> elf_file_paths;

  // if package is linux kernel package and its associated debug
  // info package looks like a kernel debuginfo package, then try to
  // go find the vmlinux file in that debug info file.
  string pkg_name = package.base_name();
  bool is_linux_kernel_package = file_is_kernel_package(pkg_name,
							package.type());
  if (is_linux_kernel_package)
    {
      // For a linux kernel package, no analysis is done.  It'll be
      // done later at comparison time by
      // compare_prepared_linux_kernel_packages
      is_ok = true;
      if (opts.verbose)
	emit_prefix("abipkgdiff", cerr)
	  << " Analysis of " << package.path() << " DONE\n";
      return is_ok;
    }

  is_ok &= get_interesting_files_under_dir(package.extracted_dir_path(),
					   /*file_name_to_look_for=*/"",
					   opts, elf_file_paths);

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Found " << elf_file_paths.size() << " files in "
      << package.extracted_dir_path() << "\n";

  // determine if all files have the same prefix.  Compute that prefix
  // and stick it into the package!  That prefix is going to be used
  // later by the package::convert_path_to_unique_suffix method.
  package.load_elf_file_paths(opts);

  maybe_create_public_dso_sonames_set(package, opts);

  for (vector<string>::const_iterator file = elf_file_paths.begin();
       file != elf_file_paths.end();
       ++file)
    {
      elf_file_sptr e (new elf_file(*file));
      if (opts.compare_dso_only)
	{
	  if (e->type != abigail::dwarf_reader::ELF_TYPE_DSO)
	    {
	      if (opts.verbose)
		emit_prefix("abipkgdiff", cerr)
		  << "skipping non-DSO file " << e->path << "\n";
	      continue;
	    }
	}
      else
	{
	  if (e->type != abigail::dwarf_reader::ELF_TYPE_DSO
	      && e->type != abigail::dwarf_reader::ELF_TYPE_EXEC
              && e->type != abigail::dwarf_reader::ELF_TYPE_PI_EXEC)
	    {
	      if (is_linux_kernel_package)
		{
		  if (e->type == abigail::dwarf_reader::ELF_TYPE_RELOCATABLE)
		    {
		      // This is a Linux Kernel module.
		      ;
		    }
		}
	      else if (opts.verbose)
		{
		  emit_prefix("abipkgdiff", cerr)
		    << "skipping non-DSO non-executable file "
		    << e->path
		    << "\n";
		  continue;
		}
	    }
	}

      if (e->soname.empty())
	{
	  if (e->type == abigail::dwarf_reader::ELF_TYPE_DSO
	      && must_compare_public_dso_only(package, opts))
	    {
	      // We are instructed to compare public DSOs only.  Yet
	      // this DSO does not have a soname.  so it can not be a
	      // public DSO.  Let's skip it.
	      if (opts.verbose)
		emit_prefix("abipkgdiff", cerr)
		  << "DSO " << e->path
		  << " does not have a soname so it's private.  Skipping it\n";
	      continue;
	    }

	  // Several binaries at different paths can have the same
	  // base name.  So let's consider the full path of the binary
	  // inside the extracted directory.
	  string key = e->name;
	  package.convert_path_to_unique_suffix(e->path, key);
	  package.path_elf_file_sptr_map()[key] = e;
	  if (opts.verbose)
	    emit_prefix("abipkgdiff", cerr)
	      << "mapped binary with key '" << key << "'"
	      << "\n";
	}
      else
	{
	  // Several binaries at different paths can have the same
	  // soname.  So let's *also* consider the full path of the
	  // binary inside the extracted directory, not just the
	  // soname.
	  string key = e->soname;

	  if (must_compare_public_dso_only(package, opts))
	    {
	      if (package.public_dso_sonames().find(key)
		  == package.public_dso_sonames().end())
		{
		  // We are instructed to compare public DSOs only and
		  // this one seems to be private.  So skip it.
		  if (opts.verbose)
		    emit_prefix("abipkgdiff", cerr)
		      << "DSO " << e->path << " of soname " << key
		      << " seems to be private.  Skipping it\n";
		  continue;
		}
	    }

	  if (package.convert_path_to_unique_suffix(e->path, key))
	    {
	      dir_name(key, key);
	      key += string("/@soname:") + e->soname;
	    }
	  package.path_elf_file_sptr_map()[key] = e;
	  if (opts.verbose)
	    emit_prefix("abipkgdiff", cerr)
	      << "mapped binary with key '" << key << "'"
	      << "\n";
	}
    }

  if (opts.verbose)
    emit_prefix("abipkgdiff", cerr)
      << " Analysis of " << package.path() << " DONE\n";

  is_ok = true;

  return is_ok;
}

/// Extract the content of a package (and its ancillary packages) and
/// map its content.
///
/// First, the content of the package and its ancillary packages are
/// extracted, in parallel.
///
/// Then, after that extraction is done, the content of the package if
/// walked and analyzed.
///
/// @param pkg the package to extract and to analyze.
///
/// @param opts the options of the current program.
///
/// @return true iff the extraction and analyzing went well.
static bool
extract_package_and_map_its_content(const package_sptr &pkg, options &opts)
{
  assert(pkg);

  pkg_extraction_task_sptr main_pkg_extraction;
  pkg_extraction_task_sptr dbg_extraction;
  pkg_extraction_task_sptr devel_extraction;
  pkg_extraction_task_sptr kabi_whitelist_extraction;

  size_t NUM_EXTRACTIONS = 1;

  main_pkg_extraction.reset(new pkg_extraction_task(pkg, opts));

  if (!pkg->debug_info_packages().empty())
    {
      dbg_extraction.reset(new pkg_extraction_task(pkg->debug_info_packages(),
						   opts));
      ++NUM_EXTRACTIONS;
    }

  if (package_sptr devel_pkg = pkg->devel_package())
    {
      devel_extraction.reset(new pkg_extraction_task(devel_pkg, opts));
      ++NUM_EXTRACTIONS;
    }

  if (package_sptr kabi_wl_pkg = pkg->kabi_whitelist_package())
    {
      kabi_whitelist_extraction.reset(new pkg_extraction_task(kabi_wl_pkg,
							      opts));
      ++NUM_EXTRACTIONS;
    }

  size_t num_workers = (opts.parallel
			? std::min(opts.num_workers, NUM_EXTRACTIONS)
			: 1);
  abigail::workers::queue extraction_queue(num_workers);

  // Perform the extraction of the NUM_WORKERS packages in parallel.
  extraction_queue.schedule_task(dbg_extraction);
  extraction_queue.schedule_task(main_pkg_extraction);
  extraction_queue.schedule_task(devel_extraction);
  extraction_queue.schedule_task(kabi_whitelist_extraction);

  // Wait for the extraction to be done.
  extraction_queue.wait_for_workers_to_complete();

  // Analyze and map the content of the extracted package.
  bool is_ok = false;
  if (main_pkg_extraction->is_ok)
    is_ok = create_maps_of_package_content(*pkg, opts);

  if (is_ok)
    maybe_handle_kabi_whitelist_pkg(*pkg, opts);

  return is_ok;
}

/// Extract the two packages (and their ancillary packages) and
/// analyze their content, so that we later know what files from the
/// first package to compare against what files from the second
/// package.
///
/// Note that preparing the first package and its ancillary packages
/// happens in parallel with preparing the second package and its
/// ancillary packages.  The function then waits for the two
/// preparations to complete before returning.
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param opts the options of the current program.
///
/// @return true iff the preparation went well.
static bool
prepare_packages(package_sptr &first_package,
		 package_sptr &second_package,
		 options &opts)
{
  pkg_prepare_task_sptr first_pkg_prepare;
  pkg_prepare_task_sptr second_pkg_prepare;
  size_t NUM_PREPARATIONS = 2;

  first_pkg_prepare.reset(new pkg_prepare_task(first_package, opts));
  second_pkg_prepare.reset(new pkg_prepare_task(second_package, opts));

  size_t num_workers = (opts.parallel
			? std::min(opts.num_workers, NUM_PREPARATIONS)
			: 1);
  abigail::workers::queue preparation_queue(num_workers);

  preparation_queue.schedule_task(first_pkg_prepare);
  preparation_queue.schedule_task(second_pkg_prepare);

  preparation_queue.wait_for_workers_to_complete();

  return first_pkg_prepare->is_ok && second_pkg_prepare->is_ok;
}

/// Prepare one package for the sake of comparing it to its ABIXML
/// representation.
///
/// The preparation entails unpacking the content of the package into
/// a temporary directory and mapping its content.
///
/// @param pkg the package to prepare.
///
/// @param opts the options provided by the user.
///
/// @return true iff the preparation succeeded.
static bool
prepare_package(package_sptr& pkg, options &opts)
{return extract_package_and_map_its_content(pkg, opts);}

/// Compare the added sizes of an ELF pair (specified by a comparison
/// task that compares two ELF files) against the added sizes of a
/// second ELF pair.
///
/// Larger filesize strongly raises the possibility of larger debug-info,
/// hence longer diff time. For a package containing several relatively
/// large and small ELFs, it is often more efficient to start working on
/// the larger ones first. This function is used to order the pairs by
/// size, starting from the largest.
///
/// @param t1 the first comparison task that compares a pair of ELF
/// files.
///
/// @param t2 the second comparison task that compares a pair of ELF
/// files.
///
/// @return true if @p task1 is greater than @p task2.
bool
elf_size_is_greater(const task_sptr &task1,
		    const task_sptr &task2)
{
  compare_task_sptr t1 = dynamic_pointer_cast<compare_task>(task1);
  compare_task_sptr t2 = dynamic_pointer_cast<compare_task>(task2);

  ABG_ASSERT(t1->args && t2->args);
  off_t s1 = t1->args->elf1.size + t1->args->elf2.size;
  off_t s2 = t2->args->elf1.size + t2->args->elf2.size;

  if (s1 != s2)
    return s1 > s2;

  // The sizes of the compared binaries are the same.  So sort them
  // lexicographically.
  return t1->args->elf1.name < t2->args->elf1.name;

}

/// This type is used to notify the calling thread that the comparison
/// of two ELF files is done.
class comparison_done_notify : public abigail::workers::queue::task_done_notify
{
  comparison_done_notify();

public:
  abi_diff& diff;
  abidiff_status status;

  comparison_done_notify(abi_diff &d)
    : diff(d),
      status(abigail::tools_utils::ABIDIFF_OK)
  {}

  /// This operator is invoked by the worker queue whenever a
  /// comparison task is done.
  ///
  /// The operator collects the status of the job of the task and also
  /// updates the the count of binaries that have ABI changes.
  ///
  /// @param task_done the task that is done.
  virtual void
  operator()(const task_sptr& task_done)
  {
    compare_task_sptr comp_task = dynamic_pointer_cast<compare_task>(task_done);
    assert(comp_task);

    status |= comp_task->status;

    if (status != abigail::tools_utils::ABIDIFF_OK)
      {
	string name = comp_task->args->elf1.name;

	if (status & abigail::tools_utils::ABIDIFF_ABI_CHANGE)
	  diff.changed_binaries.push_back(name);
      }
  }
}; // end struct comparison_done_notify

/// Erase the temporary directories that might have been created while
/// handling two packages, unless the user asked to keep the temporary
/// directories around.
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param opts the options passed to the program.
static void
maybe_erase_temp_dirs(package& first_package, package& second_package,
		      options& opts)
{
  if (opts.keep_tmp_files)
    return;

  erase_created_temporary_directories(first_package, second_package, opts);
  erase_created_temporary_directories_parent(opts);
}

/// Compare the ABI of two prepared packages that contain userspace
/// binaries.
///
/// A prepared package is a package which content has been extracted
/// and mapped.
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param options the options the current program has been called
/// with.
///
/// @param diff out parameter.  If this function returns true, then
/// this parameter is set to the result of the comparison.
///
/// @param opts the options of the current program.
///
/// @return the status of the comparison.
static abidiff_status
compare_prepared_userspace_packages(package& first_package,
				    package& second_package,
				    abi_diff& diff, options& opts)
{
  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;
  abigail::workers::queue::tasks_type compare_tasks;
  string pkg_name = first_package.base_name();

  // Setting debug-info path of libraries
  string debug_dir1, debug_dir2, relative_debug_path = "/usr/lib/debug/";
  if (!first_package.debug_info_packages().empty()
      && !second_package.debug_info_packages().empty())
    {
      debug_dir1 =
	first_package.debug_info_packages().front()->extracted_dir_path() +
	relative_debug_path;
      debug_dir2 =
	second_package.debug_info_packages().front()->extracted_dir_path() +
	relative_debug_path;
    }

  for (map<string, elf_file_sptr>::iterator it =
	 first_package.path_elf_file_sptr_map().begin();
       it != first_package.path_elf_file_sptr_map().end();
       ++it)
    {
      map<string, elf_file_sptr>::iterator iter =
	second_package.path_elf_file_sptr_map().find(it->first);

      if (iter != second_package.path_elf_file_sptr_map().end()
	  && (iter->second->type == abigail::dwarf_reader::ELF_TYPE_DSO
	      || iter->second->type == abigail::dwarf_reader::ELF_TYPE_EXEC
              || iter->second->type == abigail::dwarf_reader::ELF_TYPE_PI_EXEC
	      || iter->second->type == abigail::dwarf_reader::ELF_TYPE_RELOCATABLE))
	{
	  if (iter->second->type != abigail::dwarf_reader::ELF_TYPE_RELOCATABLE)
	    {
	      compare_args_sptr args
		(new compare_args(*it->second,
				  debug_dir1,
				  create_private_types_suppressions
				  (first_package, opts),
				  *iter->second,
				  debug_dir2,
				  create_private_types_suppressions
				  (second_package, opts), opts));
	      compare_task_sptr t(new compare_task(args));
	      compare_tasks.push_back(t);
	    }
	  second_package.path_elf_file_sptr_map().erase(iter);
	}
      else if (iter == second_package.path_elf_file_sptr_map().end())
	{
	  diff.removed_binaries.push_back(it->second);
	  status |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;
	  status |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;
	}
    }

  if (compare_tasks.empty())
    {
      maybe_erase_temp_dirs(first_package, second_package, opts);
      return abigail::tools_utils::ABIDIFF_OK;
    }

  // Larger elfs are processed first, since it's usually safe to assume
  // their debug-info is larger as well, but the results are still
  // in a map ordered by looked up in elf.name order.
  std::sort(compare_tasks.begin(), compare_tasks.end(), elf_size_is_greater);

  // There's no reason to spawn more workers than there are ELF pairs
  // to be compared.
  size_t num_workers = (opts.parallel
			? std::min(opts.num_workers, compare_tasks.size())
			: 1);
  assert(num_workers >= 1);

  comparison_done_notify notifier(diff);
  abigail::workers::queue comparison_queue(num_workers, notifier);

  // Compare all the binaries, in parallel and then wait for the
  // comparisons to complete.
  comparison_queue.schedule_tasks(compare_tasks);
  comparison_queue.wait_for_workers_to_complete();

  // Get the set of comparison tasks that were perform and sort them.
  queue::tasks_type& done_tasks = comparison_queue.get_completed_tasks();
  std::sort(done_tasks.begin(), done_tasks.end(), elf_size_is_greater);

  // Print the reports of the comparison to standard output.
  for (queue::tasks_type::const_iterator i = done_tasks.begin();
       i != done_tasks.end();
       ++i)
    {
      compare_task_sptr t = dynamic_pointer_cast<compare_task>(*i);
      cout << t->pretty_output;
    }

  // Update the count of added binaries.
  for (map<string, elf_file_sptr>::iterator it =
	 second_package.path_elf_file_sptr_map().begin();
       it != second_package.path_elf_file_sptr_map().end();
       ++it)
    diff.added_binaries.push_back(it->second);

  // Print information about removed binaries on standard output.
  if (diff.removed_binaries.size())
    {
      cout << "Removed binaries:\n";
      for (vector<elf_file_sptr>::iterator it = diff.removed_binaries.begin();
	   it != diff.removed_binaries.end(); ++it)
	{
	  string relative_path;
	  first_package.convert_path_to_relative((*it)->path, relative_path);
	  cout << "  [D] " << relative_path << ", ";
	  string soname;
	  get_soname_of_elf_file((*it)->path, soname);
	  if (!soname.empty())
	    cout << "SONAME: " << soname;
	  else
	    cout << "no SONAME";
	  cout << "\n";
	}
    }

  // Print information about added binaries on standard output.
  if (opts.show_added_binaries && diff.added_binaries.size())
    {
      cout << "Added binaries:\n";
      for (vector<elf_file_sptr>::iterator it = diff.added_binaries.begin();
	   it != diff.added_binaries.end(); ++it)
	{
	  string relative_path;
	  second_package.convert_path_to_relative((*it)->path, relative_path);
	  cout << "  [A] " << relative_path << ", ";
	  string soname;
	  get_soname_of_elf_file((*it)->path, soname);
	  if (!soname.empty())
	    cout << "SONAME: " << soname;
	  else
	    cout << "no SONAME";
	  cout << "\n";
	}
    }

  // Erase temporary directory tree we might have left behind.
  maybe_erase_temp_dirs(first_package, second_package, opts);

  status = notifier.status;

  return status;
}

/// In the context of the unpacked content of a given package, compare
/// the binaries inside the package against their ABIXML
/// representation.  This should yield the empty set.
///
/// @param pkg (unpacked) package
///
/// @param diff the representation of the changes between the binaries
/// and their ABIXML.  This should obviously be the empty set.
///
/// @param diff a textual representation of the diff.
///
/// @param opts the options provided by the user.
static abidiff_status
self_compare_prepared_userspace_package(package&	pkg,
					abi_diff&	diff,
					options&	opts)
{
  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;
  abigail::workers::queue::tasks_type self_compare_tasks;
  string pkg_name = pkg.base_name();

  // Setting debug-info path of libraries
  string debug_dir, relative_debug_path = "/usr/lib/debug/";
  if (!pkg.debug_info_packages().empty())
    debug_dir =
      pkg.debug_info_packages().front()->extracted_dir_path() +
      relative_debug_path;

  suppressions_type supprs;
  for (map<string, elf_file_sptr>::iterator it =
	 pkg.path_elf_file_sptr_map().begin();
       it != pkg.path_elf_file_sptr_map().end();
       ++it)
    {
      if (it != pkg.path_elf_file_sptr_map().end()
	  && (it->second->type == abigail::dwarf_reader::ELF_TYPE_DSO
	      || it->second->type == abigail::dwarf_reader::ELF_TYPE_EXEC
              || it->second->type == abigail::dwarf_reader::ELF_TYPE_PI_EXEC
	      || it->second->type == abigail::dwarf_reader::ELF_TYPE_RELOCATABLE))
	{
	  if (it->second->type != abigail::dwarf_reader::ELF_TYPE_RELOCATABLE)
	    {
	      compare_args_sptr args
		(new compare_args(*it->second,
				  debug_dir,
				  supprs,
				  *it->second,
				  debug_dir,
				  supprs,
				  opts));
	      self_compare_task_sptr t(new self_compare_task(args));
	      self_compare_tasks.push_back(t);
	    }
	}
    }

  if (self_compare_tasks.empty())
    {
      maybe_erase_temp_dirs(pkg, pkg, opts);
      return abigail::tools_utils::ABIDIFF_OK;
    }

  // Larger elfs are processed first, since it's usually safe to assume
  // their debug-info is larger as well, but the results are still
  // in a map ordered by looked up in elf.name order.
  std::sort(self_compare_tasks.begin(),
	    self_compare_tasks.end(),
	    elf_size_is_greater);

  // There's no reason to spawn more workers than there are ELF pairs
  // to be compared.
  size_t num_workers = (opts.parallel
			? std::min(opts.num_workers, self_compare_tasks.size())
			: 1);
  assert(num_workers >= 1);

  comparison_done_notify notifier(diff);
  abigail::workers::queue comparison_queue(num_workers, notifier);

  // Compare all the binaries, in parallel and then wait for the
  // comparisons to complete.
  comparison_queue.schedule_tasks(self_compare_tasks);
  comparison_queue.wait_for_workers_to_complete();

  // Get the set of comparison tasks that were perform and sort them.
  queue::tasks_type& done_tasks = comparison_queue.get_completed_tasks();
  std::sort(done_tasks.begin(), done_tasks.end(), elf_size_is_greater);

  // Print the reports of the comparison to standard output.
  for (queue::tasks_type::const_iterator i = done_tasks.begin();
       i != done_tasks.end();
       ++i)
    {
      self_compare_task_sptr t = dynamic_pointer_cast<self_compare_task>(*i);
      if (t)
	cout << t->pretty_output;
    }

  // Erase temporary directory tree we might have left behind.
  maybe_erase_temp_dirs(pkg, pkg, opts);

  status = notifier.status;

  return status;
}

/// Compare the ABI of two prepared packages that contain linux kernel
/// binaries.
///
/// A prepared package is a package which content has been extracted
/// and mapped.
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param options the options the current program has been called
/// with.
///
/// @param diff out parameter.  If this function returns true, then
/// this parameter is set to the result of the comparison.
///
/// @param opts the options of the current program.
///
/// @return the status of the comparison.
static abidiff_status
compare_prepared_linux_kernel_packages(package& first_package,
				       package& second_package,
				       options& opts)
{
  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;
  string pkg_name = first_package.base_name();

  // Setting debug-info path of binaries
  string debug_dir1, debug_dir2, relative_debug_path = "/usr/lib/debug/";
  if (!first_package.debug_info_packages().empty()
      && !second_package.debug_info_packages().empty())
    {
      debug_dir1 =
	first_package.debug_info_packages().front()->extracted_dir_path() +
	relative_debug_path;
      debug_dir2 =
	second_package.debug_info_packages().front()->extracted_dir_path() +
	relative_debug_path;
    }

  string vmlinux_path1, vmlinux_path2;

  if (!get_vmlinux_path_from_kernel_dist(debug_dir1, vmlinux_path1))
    return abigail::tools_utils::ABIDIFF_ERROR;

  if (!get_vmlinux_path_from_kernel_dist(debug_dir2, vmlinux_path2))
    return abigail::tools_utils::ABIDIFF_ERROR;

  string dist_root1 = first_package.extracted_dir_path();
  string dist_root2 = second_package.extracted_dir_path();

  abigail::ir::environment_sptr env(new abigail::ir::environment);
  suppressions_type supprs;
  corpus_group_sptr corpus1, corpus2;
  corpus1 = build_corpus_group_from_kernel_dist_under(dist_root1,
						      debug_dir1,
						      vmlinux_path1,
						      opts.suppression_paths,
						      opts.kabi_whitelist_paths,
						      supprs,
						      opts.verbose,
						      env);

  if (!corpus1)
    return abigail::tools_utils::ABIDIFF_ERROR;

  corpus2 = build_corpus_group_from_kernel_dist_under(dist_root2,
						      debug_dir2,
						      vmlinux_path2,
						      opts.suppression_paths,
						      opts.kabi_whitelist_paths,
						      supprs,
						      opts.verbose,
						      env);

  if (!corpus2)
    return abigail::tools_utils::ABIDIFF_ERROR;

  diff_context_sptr diff_ctxt(new diff_context);
  set_diff_context_from_opts(diff_ctxt, opts);

  corpus_diff_sptr diff = compute_diff(corpus1, corpus2, diff_ctxt);

  if (diff->has_net_changes())
    status |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;
  if (diff->has_incompatible_changes())
    status |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;

  if (status & abigail::tools_utils::ABIDIFF_ABI_CHANGE)
    {
      cout << "== Kernel ABI changes between packages '"
	   << first_package.path() << "' and '"
	   << second_package.path() << "' are: ===\n";
      diff->report(cout);
      cout << "== End of kernel ABI changes between packages '"
	   << first_package.path()
	   << "' and '"
	   << second_package.path() << "' ===\n\n";
    }

  return status;
}

/// Compare the ABI of two prepared packages.
///
/// A prepared package is a package which content has been extracted
/// and mapped.
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param options the options the current program has been called
/// with.
///
/// @param diff out parameter.  If this function returns true, then
/// this parameter is set to the result of the comparison.
///
/// @param opts the options of the current program.
///
/// @return the status of the comparison.
static abidiff_status
compare_prepared_package(package& first_package, package& second_package,
			 abi_diff& diff, options& opts)
{
  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;

  if (abigail::tools_utils::file_is_kernel_package(first_package.base_name(),
						   first_package.type()))
    {
      opts.show_symbols_not_referenced_by_debug_info = false;
      status = compare_prepared_linux_kernel_packages(first_package,
						      second_package,
						      opts);
    }
  else
    status = compare_prepared_userspace_packages(first_package,
						 second_package,
						 diff, opts);

  return status;
}

/// Compare binaries in a package against their ABIXML
/// representations.
///
/// @param pkg the package to consider.
///
/// @param diff the textual representation of the resulting
/// comparison.
///
/// @param opts the options provided by the user
///
/// @return the status of the comparison.
static abidiff_status
self_compare_prepared_package(package& pkg,
			      abi_diff& diff,
			      options& opts)
{
  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;

  status = self_compare_prepared_userspace_package(pkg, diff, opts);

  return status;
}

/// Compare the ABI of two packages
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param options the options the current program has been called
/// with.
///
/// @param diff out parameter.  If this function returns true, then
/// this parameter is set to the result of the comparison.
///
/// @param opts the options of the current program.
///
/// @return the status of the comparison.
static abidiff_status
compare(package_sptr& first_package, package_sptr& second_package,
	abi_diff& diff, options& opts)
{
  // Prepare (extract and analyze the contents) the packages and their
  // ancillary packages.
  //
  // Note that the package preparations happens in parallel.
  if (!prepare_packages(first_package, second_package, opts))
    {
      maybe_erase_temp_dirs(*first_package, *second_package, opts);
      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  return compare_prepared_package(*first_package, *second_package, diff, opts);
}

/// Compare binaries in a package against their ABIXML
/// representations.
///
/// @param pkg the package to consider.
///
/// @param opts the options provided by the user
///
/// @return the status of the comparison.
static abidiff_status
compare_to_self(package_sptr& pkg, options& opts)
{
  if (!prepare_package(pkg, opts))
    return abigail::tools_utils::ABIDIFF_ERROR;

  abi_diff diff;
  return self_compare_prepared_package(*pkg, diff, opts);
}

/// Compare the ABI of two packages.
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param opts the options the current program has been called with.
///
/// @return the status of the comparison.
static abidiff_status
compare(package_sptr& first_package,
	package_sptr& second_package,
	options& opts)
{
  abi_diff diff;
  return compare(first_package, second_package, diff, opts);
}

/// Parse the command line of the current program.
///
/// @param argc the number of arguments in the @p argv parameter.
///
/// @param argv the array of arguemnts passed to the function.  The
/// first argument is the name of this program.
///
/// @param opts the resulting options.
///
/// @return true upon successful parsing.
static bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
        {
          if (opts.package1.empty())
            {
              opts.package1 = make_path_absolute(argv[i]).get();
              opts.nonexistent_file = !file_exists(opts.package1);
            }
          else if (opts.package2.empty())
            {
              opts.package2 = make_path_absolute(argv[i]).get();
              opts.nonexistent_file = !file_exists(opts.package2);
            }
          else
	    {
	      opts.wrong_arg = argv[i];
	      return false;
	    }

          if (opts.nonexistent_file)
            {
              opts.wrong_option = argv[i];
              return true;
            }
        }
      else if (!strcmp(argv[i], "--debug-info-pkg1")
	       || !strcmp(argv[i], "--d1"))
        {
          int j = i + 1;
          if (j >= argc)
            {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
            }
          opts.debug_packages1.push_back
	    (abigail::tools_utils::make_path_absolute(argv[j]).get());
          ++i;
        }
      else if (!strcmp(argv[i], "--debug-info-pkg2")
	       || !strcmp(argv[i], "--d2"))
        {
          int j = i + 1;
          if (j >= argc)
            {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
            }
          opts.debug_packages2.push_back
	    (abigail::tools_utils::make_path_absolute(argv[j]).get());
          ++i;
        }
      else if (!strcmp(argv[i], "--devel-pkg1")
	       || !strcmp(argv[i], "--devel1"))
        {
          int j = i + 1;
          if (j >= argc)
            {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
            }
          opts.devel_package1 =
	    abigail::tools_utils::make_path_absolute(argv[j]).get();
          ++i;
        }
      else if (!strcmp(argv[i], "--devel-pkg2")
	       || !strcmp(argv[i], "--devel2"))
        {
          int j = i + 1;
          if (j >= argc)
            {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
            }
          opts.devel_package2 =
	    abigail::tools_utils::make_path_absolute(argv[j]).get();
          ++i;
        }
      else if (!strcmp(argv[i], "--drop-private-types"))
	opts.drop_private_types = true;
      else if (!strcmp(argv[i], "--no-default-suppression"))
	opts.no_default_suppression = true;
      else if (!strcmp(argv[i], "--keep-tmp-files"))
	opts.keep_tmp_files = true;
      else if (!strcmp(argv[i], "--dso-only"))
	opts.compare_dso_only = true;
      else if (!strcmp(argv[i], "--private-dso"))
	opts.compare_private_dsos = true;
      else if (!strcmp(argv[i], "--leaf-changes-only")
	       ||!strcmp(argv[i], "-l"))
	opts.leaf_changes_only = true;
      else if (!strcmp(argv[i], "--impacted-interfaces")
	       ||!strcmp(argv[i], "-i"))
	opts.show_impacted_interfaces = true;
      else if (!strcmp(argv[i], "--non-reachable-types")
	       ||!strcmp(argv[i], "-t"))
	opts.show_all_types = true;
      else if (!strcmp(argv[i], "--full-impact")
	       ||!strcmp(argv[i], "-f"))
	opts.show_full_impact_report = true;
      else if (!strcmp(argv[i], "--no-linkage-name"))
	opts.show_linkage_names = false;
      else if (!strcmp(argv[i], "--redundant"))
	opts.show_redundant_changes = true;
      else if (!strcmp(argv[i], "--harmless"))
	opts.show_harmless_changes = true;
      else if (!strcmp(argv[i], "--no-show-locs"))
	opts.show_locs = false;
      else if (!strcmp(argv[i], "--show-bytes"))
	opts.show_offsets_sizes_in_bits = false;
      else if (!strcmp(argv[i], "--show-bits"))
	opts.show_offsets_sizes_in_bits = true;
      else if (!strcmp(argv[i], "--show-hex"))
	opts.show_hexadecimal_values = true;
      else if (!strcmp(argv[i], "--show-dec"))
	opts.show_hexadecimal_values = false;
      else if (!strcmp(argv[i], "--no-show-relative-offset-changes"))
	opts.show_relative_offset_changes = false;
      else if (!strcmp(argv[i], "--no-added-syms"))
	opts.show_added_syms = false;
      else if (!strcmp(argv[i], "--no-unreferenced-symbols"))
	opts.show_symbols_not_referenced_by_debug_info = false;
      else if (!strcmp(argv[i], "--no-added-binaries"))
	opts.show_added_binaries = false;
      else if (!strcmp(argv[i], "--fail-no-dbg"))
	opts.fail_if_no_debug_info = true;
      else if (!strcmp(argv[i], "--verbose"))
	opts.verbose = true;
      else if (!strcmp(argv[i], "--no-abignore"))
	opts.abignore = false;
      else if (!strcmp(argv[i], "--no-parallel"))
	opts.parallel = false;
      else if (!strcmp(argv[i], "--show-identical-binaries"))
	opts.show_identical_binaries = true;
      else if (!strcmp(argv[i], "--self-check"))
	opts.self_check = true;
      else if (!strcmp(argv[i], "--suppressions")
	       || !strcmp(argv[i], "--suppr"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.suppression_paths.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--linux-kernel-abi-whitelist")
	       || !strcmp(argv[i], "-w"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  if (guess_file_type(argv[j]) == abigail::tools_utils::FILE_TYPE_RPM)
	    // The kernel abi whitelist is actually a whitelist
	    // *package*.  Take that into account.
	    opts.kabi_whitelist_packages.push_back
	      (make_path_absolute(argv[j]).get());
	  else
	    // We assume the kernel abi whitelist is a white list
	    // file.
	    opts.kabi_whitelist_paths.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--wp"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.kabi_whitelist_packages.push_back
	    (make_path_absolute(argv[j]).get());
	  ++i;
	}
      else if (!strcmp(argv[i], "--help")
	       || !strcmp(argv[i], "-h"))
        {
          opts.display_usage = true;
          return true;
        }
      else if (!strcmp(argv[i], "--version")
	       || !strcmp(argv[i], "-v"))
	{
	  opts.display_version = true;
	  return true;
	}
      else
	{
	  if (strlen(argv[i]) >= 2 && argv[i][0] == '-' && argv[i][1] == '-')
	    opts.wrong_option = argv[i];
	  return false;
	}
    }

  return true;
}

int
main(int argc, char* argv[])
{
  options opts(argv[0]);

  if (!parse_command_line(argc, argv, opts))
    {
      if (!opts.wrong_option.empty())
	emit_prefix("abipkgdiff", cerr)
	  << "unrecognized option: " << opts.wrong_option
	  << "\ntry the --help option for more information\n";
      else
	emit_prefix("abipkgdiff", cerr)
	  << "unrecognized argument: " << opts.wrong_arg
	  << "\ntry the --help option for more information\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.missing_operand)
    {
      emit_prefix("abipkgdiff", cerr)
	<< "missing operand\n"
        "try the --help option for more information\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.nonexistent_file)
    {
      string input_file;
      base_name(opts.wrong_option, input_file);
      emit_prefix("abipkgdiff", cerr)
	<< "The input file " << input_file << " doesn't exist\n"
	"try the --help option for more information\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.kabi_whitelist_packages.size() > 2)
    {
      emit_prefix("abipkgdiff", cerr)
	<< "no more than 2 Linux kernel white list packages can be provided\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.display_usage)
    {
      display_usage(argv[0], cout);
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.display_version)
    {
      emit_prefix(argv[0], cout)
	<< abigail::tools_utils::get_library_version_string()
	<< "\n";
      return 0;
    }

    if (!opts.no_default_suppression && opts.suppression_paths.empty())
    {
      // Load the default system and user suppressions.
      string default_system_suppr_file =
	get_default_system_suppression_file_path();
      if (file_exists(default_system_suppr_file))
	opts.suppression_paths.push_back(default_system_suppr_file);

      string default_user_suppr_file =
	get_default_user_suppression_file_path();
      if (file_exists(default_user_suppr_file))
	opts.suppression_paths.push_back(default_user_suppr_file);
    }

  if (!maybe_check_suppression_files(opts))
    return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	    | abigail::tools_utils::ABIDIFF_ERROR);

  bool need_just_one_input_package = opts.self_check;

  if (need_just_one_input_package)
    {
      bool bail_out = false;
      if (!opts.package2.empty())
	{
	  // We don't need the second package, we'll ignore it later
	  // down below.
	  ;
	}
      if (opts.package1.empty())
	{
	  // We need at least one package to work with!
	  emit_prefix("abipkgdiff", cerr)
	    << "missing input package\n";
	  if (bail_out)
	    return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		    | abigail::tools_utils::ABIDIFF_ERROR);
	}
    }
  else if(opts.package1.empty() || opts.package2.empty())
    {
      emit_prefix("abipkgdiff", cerr)
	<< "please enter two packages to compare" << "\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  package_sptr first_package(new package(opts.package1, "package1"));

  package_sptr second_package(new package(opts.package2, "package2"));
  opts.pkg1 = first_package;
  opts.pkg2 = second_package;

  for (vector<string>::const_iterator p = opts.debug_packages1.begin();
       p != opts.debug_packages1.end();
       ++p)
    first_package->debug_info_packages().push_back
      (package_sptr(new package(*p,
				"debug_package1",
				/*pkg_kind=*/package::KIND_DEBUG_INFO)));

  for (vector<string>::const_iterator p = opts.debug_packages2.begin();
       p != opts.debug_packages2.end();
       ++p)
    second_package->debug_info_packages().push_back
      (package_sptr(new package(*p,
				"debug_package2",
				/*pkg_kind=*/package::KIND_DEBUG_INFO)));

  if (!opts.devel_package1.empty())
    first_package->devel_package
      (package_sptr(new package(opts.devel_package1,
				"devel_package1",
				/*pkg_kind=*/package::KIND_DEVEL)));
    ;

  if (!opts.devel_package2.empty())
    second_package->devel_package
      (package_sptr(new package(opts.devel_package2,
				"devel_package2",
				/*pkg_kind=*/package::KIND_DEVEL)));

  if (!opts.kabi_whitelist_packages.empty())
    {
      first_package->kabi_whitelist_package
	(package_sptr(new package
		      (opts.kabi_whitelist_packages[0],
		       "kabi_whitelist_package1",
		       /*pkg_kind=*/package::KIND_KABI_WHITELISTS)));
      if (opts.kabi_whitelist_packages.size() >= 2)
	second_package->kabi_whitelist_package
	  (package_sptr(new package
			(opts.kabi_whitelist_packages[1],
			 "kabi_whitelist_package2",
			 /*pkg_kind=*/package::KIND_KABI_WHITELISTS)));
    }

  string package_name;
  switch (first_package->type())
    {
    case abigail::tools_utils::FILE_TYPE_RPM:
      if (!second_package->path().empty()
	  && second_package->type() != abigail::tools_utils::FILE_TYPE_RPM)
	{
	  base_name(opts.package2, package_name);
	  emit_prefix("abipkgdiff", cerr)
	    << package_name << " should be an RPM file\n";
	  return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
	}

      if (file_is_kernel_package(first_package->base_name(),
				 abigail::tools_utils::FILE_TYPE_RPM)
	  || file_is_kernel_package(second_package->base_name(),
				    abigail::tools_utils::FILE_TYPE_RPM))
	{
	  if (file_is_kernel_package(first_package->base_name(),
				     abigail::tools_utils::FILE_TYPE_RPM)
	      != file_is_kernel_package(second_package->base_name(),
					abigail::tools_utils::FILE_TYPE_RPM))
	    {
	      emit_prefix("abipkgdiff", cerr)
		<< "a Linux kernel package can only be compared to another "
		"Linux kernel package\n";
	      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		      | abigail::tools_utils::ABIDIFF_ERROR);
	    }

	  if (first_package->debug_info_packages().empty()
	      || (!second_package->path().empty()
		  && second_package->debug_info_packages().empty()))
	    {
	      emit_prefix("abipkgdiff", cerr)
		<< "a Linux Kernel package must be accompanied with its "
		"debug info package\n";
	      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		      | abigail::tools_utils::ABIDIFF_ERROR);
	    }
	  // We are looking at kernel packages.  If the user provided
	  // the --full-impact option then it means we want to display
	  // the default libabigail report format where a full impact
	  // analysis is done for each ABI change.
	  //
	  // Otherwise, let's just emit the leaf change report.
	  if (opts.show_full_impact_report)
	    opts.leaf_changes_only = false;
	  else
	    opts.leaf_changes_only = true;
	}

      break;

    case abigail::tools_utils::FILE_TYPE_DEB:
      if (!second_package->path().empty()
	  && second_package->type() != abigail::tools_utils::FILE_TYPE_DEB)
	{
	  base_name(opts.package2, package_name);
	  emit_prefix("abipkgdiff", cerr)
	    << package_name << " should be a DEB file\n";
	  return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
	}
      break;

    case abigail::tools_utils::FILE_TYPE_DIR:
      if (!second_package->path().empty()
	  && second_package->type() != abigail::tools_utils::FILE_TYPE_DIR)
	{
	  base_name(opts.package2, package_name);
	  emit_prefix("abipkgdiff", cerr)
	    << package_name << " should be a directory\n";
	  return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
	}
      break;

    case abigail::tools_utils::FILE_TYPE_TAR:
      if (!second_package->path().empty()
	  && second_package->type() != abigail::tools_utils::FILE_TYPE_TAR)
	{
	  base_name(opts.package2, package_name);
	  emit_prefix("abipkgdiff", cerr)
	    << package_name << " should be a GNU tar archive\n";
	  return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
	}
      break;

    default:
      base_name(opts.package1, package_name);
      emit_prefix("abipkgdiff", cerr)
	<< package_name << " should be a valid package file \n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.self_check)
    return compare_to_self(first_package, opts);

  return compare(first_package, second_package, opts);
}

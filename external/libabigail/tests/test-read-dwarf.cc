// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file read ELF binaries containing DWARF, save them in XML corpus
/// files and diff the corpus files against reference XML corpus
/// files.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "abg-ir.h"
#include "abg-dwarf-reader.h"
#include "abg-workers.h"
#include "abg-writer.h"
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::vector;
using std::string;
using std::ofstream;
using std::cerr;
using std::dynamic_pointer_cast;
using abigail::tests::get_build_dir;
using abigail::dwarf_reader::read_corpus_from_elf;
using abigail::dwarf_reader::read_context;
using abigail::dwarf_reader::read_context_sptr;
using abigail::dwarf_reader::create_read_context;
using abigail::xml_writer::SEQUENCE_TYPE_ID_STYLE;
using abigail::xml_writer::HASH_TYPE_ID_STYLE;
using abigail::xml_writer::create_write_context;
using abigail::xml_writer::set_type_id_style;
using abigail::xml_writer::type_id_style_kind;
using abigail::xml_writer::write_context_sptr;
using abigail::xml_writer::write_corpus;

/// This is an aggregate that specifies where a test shall get its
/// input from, and where it shall write its ouput to.
struct InOutSpec
{
  const char* in_elf_path;
  const char* in_suppr_spec_path;
  type_id_style_kind type_id_style;
  const char* in_abi_path;
  const char* out_abi_path;
};// end struct InOutSpec


InOutSpec in_out_specs[] =
{
  {
    "data/test-read-dwarf/test0",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test0.abi",
    "output/test-read-dwarf/test0.abi"
  },
  {
    "data/test-read-dwarf/test0",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test0.hash.abi",
    "output/test-read-dwarf/test0.hash.abi"
  },
  {
    "data/test-read-dwarf/test1",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test1.abi",
    "output/test-read-dwarf/test1.abi"
  },
  {
    "data/test-read-dwarf/test1",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test1.hash.abi",
    "output/test-read-dwarf/test1.hash.abi"
  },
  {
    "data/test-read-dwarf/test2.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test2.so.abi",
    "output/test-read-dwarf/test2.so.abi"
  },
  {
    "data/test-read-dwarf/test2.so",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test2.so.hash.abi",
    "output/test-read-dwarf/test2.so.hash.abi"
  },
  {
    "data/test-read-dwarf/test3.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test3.so.abi",
    "output/test-read-dwarf/test3.so.abi"
  },
  {
    "data/test-read-dwarf/test3.so",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test3.so.hash.abi",
    "output/test-read-dwarf/test3.so.hash.abi"
  },
  // suppress all except the main symbol of a group of aliases
  {
    "data/test-read-dwarf/test3.so",
    "data/test-read-dwarf/test3-alias-1.suppr",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test3-alias-1.so.hash.abi",
    "output/test-read-dwarf/test3-alias-1.so.hash.abi"
  },
  // suppress the main symbol of a group of aliases
  {
    "data/test-read-dwarf/test3.so",
    "data/test-read-dwarf/test3-alias-2.suppr",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test3-alias-2.so.hash.abi",
    "output/test-read-dwarf/test3-alias-2.so.hash.abi"
  },
  // suppress all except one non main symbol of a group of aliases
  {
    "data/test-read-dwarf/test3.so",
    "data/test-read-dwarf/test3-alias-3.suppr",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test3-alias-3.so.hash.abi",
    "output/test-read-dwarf/test3-alias-3.so.hash.abi"
  },
  // suppress all symbols of a group of aliases
  {
    "data/test-read-dwarf/test3.so",
    "data/test-read-dwarf/test3-alias-4.suppr",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test3-alias-4.so.hash.abi",
    "output/test-read-dwarf/test3-alias-4.so.hash.abi"
  },
  // suppress the main symbols with alias (function+variable) in .o file
  {
    "data/test-read-dwarf/test-suppressed-alias.o",
    "data/test-read-dwarf/test-suppressed-alias.suppr",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test-suppressed-alias.o.abi",
    "output/test-read-dwarf/test-suppressed-alias.o.abi",
  },
  {
    "data/test-read-dwarf/test4.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test4.so.abi",
    "output/test-read-dwarf/test4.so.abi"
  },
  {
    "data/test-read-dwarf/test4.so",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test4.so.hash.abi",
    "output/test-read-dwarf/test4.so.hash.abi"
  },
  {
    "data/test-read-dwarf/test5.o",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test5.o.abi",
    "output/test-read-dwarf/test5.o.abi"
  },
  {
    "data/test-read-dwarf/test5.o",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test5.o.hash.abi",
    "output/test-read-dwarf/test5.o.hash.abi"
  },
  {
    "data/test-read-dwarf/test6.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test6.so.abi",
    "output/test-read-dwarf/test6.so.abi"
  },
  {
    "data/test-read-dwarf/test6.so",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test6.so.hash.abi",
    "output/test-read-dwarf/test6.so.hash.abi"
  },
  {
    "data/test-read-dwarf/test7.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test7.so.abi",
    "output/test-read-dwarf/test7.so.abi"
  },
  {
    "data/test-read-dwarf/test7.so",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test7.so.hash.abi",
    "output/test-read-dwarf/test7.so.hash.abi"
  },
  {
    "data/test-read-dwarf/test8-qualified-this-pointer.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test8-qualified-this-pointer.so.abi",
    "output/test-read-dwarf/test8-qualified-this-pointer.so.abi"
  },
  {
    "data/test-read-dwarf/test8-qualified-this-pointer.so",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test8-qualified-this-pointer.so.hash.abi",
    "output/test-read-dwarf/test8-qualified-this-pointer.so.hash.abi"
  },
  {
    "data/test-read-dwarf/test9-pr18818-clang.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test9-pr18818-clang.so.abi",
    "output/test-read-dwarf/test9-pr18818-clang.so.abi"
  },
  {
    "data/test-read-dwarf/test10-pr18818-gcc.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test10-pr18818-gcc.so.abi",
    "output/test-read-dwarf/test10-pr18818-gcc.so.abi"
  },
  {
    "data/test-read-dwarf/test11-pr18828.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test11-pr18828.so.abi",
    "output/test-read-dwarf/test11-pr18828.so.abi",
  },
  {
    "data/test-read-dwarf/test12-pr18844.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test12-pr18844.so.abi",
    "output/test-read-dwarf/test12-pr18844.so.abi",
  },
  {
    "data/test-read-dwarf/test13-pr18894.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test13-pr18894.so.abi",
    "output/test-read-dwarf/test13-pr18894.so.abi",
  },
  {
    "data/test-read-dwarf/test14-pr18893.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test14-pr18893.so.abi",
    "output/test-read-dwarf/test14-pr18893.so.abi",
  },
  {
    "data/test-read-dwarf/test15-pr18892.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test15-pr18892.so.abi",
    "output/test-read-dwarf/test15-pr18892.so.abi",
  },
  {
    "data/test-read-dwarf/test16-pr18904.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test16-pr18904.so.abi",
    "output/test-read-dwarf/test16-pr18904.so.abi",
  },
  {
    "data/test-read-dwarf/test17-pr19027.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test17-pr19027.so.abi",
    "output/test-read-dwarf/test17-pr19027.so.abi",
  },
  {
    "data/test-read-dwarf/test18-pr19037-libvtkRenderingLIC-6.1.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test18-pr19037-libvtkRenderingLIC-6.1.so.abi",
    "output/test-read-dwarf/test18-pr19037-libvtkRenderingLIC-6.1.so.abi",
  },
  {
    "data/test-read-dwarf/test19-pr19023-libtcmalloc_and_profiler.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test19-pr19023-libtcmalloc_and_profiler.so.abi",
    "output/test-read-dwarf/test19-pr19023-libtcmalloc_and_profiler.so.abi",
  },
  {
    "data/test-read-dwarf/test20-pr19025-libvtkParallelCore-6.1.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test20-pr19025-libvtkParallelCore-6.1.so.abi",
    "output/test-read-dwarf/test20-pr19025-libvtkParallelCore-6.1.so.abi",
  },
  {
    "data/test-read-dwarf/test21-pr19092.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test21-pr19092.so.abi",
    "output/test-read-dwarf/test21-pr19092.so.abi",
  },
  {
    "data/test-read-dwarf/test22-pr19097-libstdc++.so.6.0.17.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test22-pr19097-libstdc++.so.6.0.17.so.abi",
    "output/test-read-dwarf/test22-pr19097-libstdc++.so.6.0.17.so.abi",
  },
  {
    "data/test-read-dwarf/libtest23.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/libtest23.so.abi",
    "output/test-read-dwarf/libtest23.so.abi",
  },
  {
    "data/test-read-dwarf/libtest24-drop-fns.so",
    "data/test-read-dwarf/test24-drop-fns-0.suppr",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/libtest24-drop-fns.so.abi",
    "output/test-read-dwarf/libtest24-drop-fns.so.abi",
  },
  {
    "data/test-read-dwarf/libtest24-drop-fns.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/libtest24-drop-fns-2.so.abi",
    "output/test-read-dwarf/libtest24-drop-fns-2.so.abi",
  },
  {
    "data/test-read-dwarf/PR22015-libboost_iostreams.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/PR22015-libboost_iostreams.so.abi",
    "output/test-read-dwarf/PR22015-libboost_iostreams.so.abi",
  },
  {
    "data/test-read-dwarf/PR22122-libftdc.so",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/PR22122-libftdc.so.abi",
    "output/test-read-dwarf/PR22122-libftdc.so.abi",
  },
  {
    "data/test-read-dwarf/PR24378-fn-is-not-scope.o",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/PR24378-fn-is-not-scope.abi",
    "output/test-read-dwarf/PR24378-fn-is-not-scope.abi",
  },
#if defined(HAVE_R_AARCH64_ABS64_MACRO) && defined(HAVE_R_AARCH64_PREL32_MACRO)
  {
    "data/test-read-dwarf/PR25007-sdhci.ko",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/PR25007-sdhci.ko.abi",
    "output/test-read-dwarf/PR25007-sdhci.ko.abi",
  },
#endif
#if defined HAVE_DW_FORM_strx
  {
    "data/test-read-dwarf/PR25042-libgdbm-clang-dwarf5.so.6.0.0",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/PR25042-libgdbm-clang-dwarf5.so.6.0.0.abi",
    "output/test-read-dwarf/PR25042-libgdbm-clang-dwarf5.so.6.0.0.abi",
  },
#endif
  {
    "data/test-read-dwarf/test25-bogus-binary.elf",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    NULL,
    NULL,
  },
  {
    "data/test-read-dwarf/test26-bogus-binary.elf",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    NULL,
    NULL,
  },
  {
    "data/test-read-dwarf/test27-bogus-binary.elf",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    NULL,
    NULL,
  },
  {
    "data/test-read-dwarf/PR26261/PR26261-exe",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/PR26261/PR26261-exe.abi",
    "output/test-read-dwarf/PR26261/PR26261-exe.abi",
  },
  {
    "data/test-read-dwarf/test-PR26568-1.o",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test-PR26568-1.o.abi",
    "output/test-read-dwarf/test-PR26568-1.o.abi",
  },
  {
    "data/test-read-dwarf/test-PR26568-2.o",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-dwarf/test-PR26568-2.o.abi",
    "output/test-read-dwarf/test-PR26568-2.o.abi",
  },
  {
    "data/test-read-dwarf/test-libandroid.so",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-dwarf/test-libandroid.so.abi",
    "output/test-read-dwarf/test-libandroid.so.abi",
  },
  // This should be the last entry.
  {NULL, NULL, SEQUENCE_TYPE_ID_STYLE, NULL, NULL}
};

using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::read_suppressions;

/// Set the suppression specification to use when reading the ELF binary.
///
/// @param read_ctxt the context used to read the ELF binary.
///
/// @param path the path to the suppression specification to read.
static void
set_suppressions(read_context& read_ctxt, const string& path)
{
  suppressions_type supprs;
  read_suppressions(path, supprs);
  add_read_context_suppressions(read_ctxt, supprs);
}

/// The task that peforms the tests.
struct test_task : public abigail::workers::task
{
  bool is_ok;
  InOutSpec spec;
  string error_message;
  string out_abi_base;
  string in_elf_base;
  string in_abi_base;

  test_task(const InOutSpec &s,
	    string& a_out_abi_base,
	    string& a_in_elf_base,
	    string& a_in_abi_base)
    : is_ok(true),
      spec(s),
      out_abi_base(a_out_abi_base),
      in_elf_base(a_in_elf_base),
      in_abi_base(a_in_abi_base)
  {}

  /// The actual test.
  ///
  /// This reads the corpus into memory, saves it to disk, loads it
  /// again and compares the new in-memory representation against the
  /// saved one.
  virtual void
  perform()
  {
    string in_elf_path, in_abi_path, in_suppr_spec_path, out_abi_path;
    abigail::ir::environment_sptr env;

    in_elf_path = in_elf_base + spec.in_elf_path;
    if (spec.in_suppr_spec_path)
      in_suppr_spec_path = in_elf_base + spec.in_suppr_spec_path;
    else
      in_suppr_spec_path.clear();

    env.reset(new abigail::ir::environment);
    abigail::dwarf_reader::status status =
    abigail::dwarf_reader::STATUS_UNKNOWN;
    vector<char**> di_roots;
    ABG_ASSERT(abigail::tools_utils::file_exists(in_elf_path));
    read_context_sptr ctxt = create_read_context(in_elf_path,
						 di_roots,
						 env.get());
    ABG_ASSERT(ctxt);
    if (!in_suppr_spec_path.empty())
      set_suppressions(*ctxt, in_suppr_spec_path);

    abigail::corpus_sptr corp = read_corpus_from_elf(*ctxt, status);
    // if there is no output and no input, assume that we do not care about the
    // actual read result, just that it succeeded.
    if (!spec.in_abi_path && !spec.out_abi_path)
      {
	// Phew! we made it here and we did not crash! yay!
	return;
      }
    if (!corp)
      {
	error_message = string("failed to read ") + in_elf_path  + "\n";
	is_ok = false;
	return;
      }
    corp->set_path(spec.in_elf_path);
    // Do not take architecture names in comparison so that these
    // test input binaries can come from whatever arch the
    // programmer likes.
    corp->set_architecture_name("");

    out_abi_path = out_abi_base + spec.out_abi_path;
    if (!abigail::tools_utils::ensure_parent_dir_created(out_abi_path))
      {
	error_message =
	  string("Could not create parent directory for ") + out_abi_path;
	is_ok = false;
	return;
      }

    ofstream of(out_abi_path.c_str(), std::ios_base::trunc);
    if (!of.is_open())
      {
	error_message = string("failed to read ") + out_abi_path + "\n";
	is_ok = false;
	return;
      }
    write_context_sptr write_ctxt
	= create_write_context(corp->get_environment(), of);
    set_type_id_style(*write_ctxt, spec.type_id_style);
    is_ok = write_corpus(*write_ctxt, corp, /*indent=*/0);
    of.close();

    string abidw = string(get_build_dir()) + "/tools/abidw";
    string cmd = abidw + " --abidiff " + in_elf_path;
    if (system(cmd.c_str()))
      {
	error_message = string("ABIs differ:\n")
	  + in_elf_path
	  + "\nand:\n"
	  + out_abi_path
	  + "\n";
	is_ok = false;
      }

    in_abi_path = in_abi_base + spec.in_abi_path;
    cmd = "diff -u " + in_abi_path + " " + out_abi_path;
    if (system(cmd.c_str()))
      is_ok = false;
  }
}; // end struct test_task

typedef shared_ptr<test_task> test_task_sptr;

int
main(int argc, char *argv[])
{
  bool no_parallel = false;

  if (argc == 2)
    {
      if (argv[1] == string("--no-parallel"))
	no_parallel = true;
      else
	{
	  cerr << "unrecognized option\n";
	  cerr << "usage: " << argv[0] << " [--no-parallel]\n" ;
	  return 1;
	}
    }

  /// Create a task queue.  The max number of worker threads of the
  /// queue is the number of the concurrent threads supported by the
  /// processor of the machine this code runs on.  But if
  /// --no-parallel was provided then the number of worker threads
  /// equals 1.
  const size_t num_tests = sizeof(in_out_specs) / sizeof (InOutSpec) - 1;
  size_t num_workers = (no_parallel
			? 1
			: std::min(abigail::workers::get_number_of_threads(),
				   num_tests));
  abigail::workers::queue task_queue(num_workers);
  bool is_ok = true;

  string out_abi_base = string(get_build_dir()) + "/tests/";
  string in_elf_base  = string(abigail::tests::get_src_dir()) + "/tests/";
  string in_abi_base = in_elf_base;

  for (InOutSpec *s = in_out_specs; s->in_elf_path; ++s)
    {
      test_task_sptr t(new test_task(*s, out_abi_base,
				     in_elf_base,
				     in_abi_base));
      ABG_ASSERT(task_queue.schedule_task(t));
    }

  /// Wait for all worker threads to finish their job, and wind down.
  task_queue.wait_for_workers_to_complete();

  // Now walk the results and print whatever error messages need to be
  // printed.

  const vector<abigail::workers::task_sptr>& completed_tasks =
    task_queue.get_completed_tasks();

  ABG_ASSERT(completed_tasks.size() == num_tests);

  for (vector<abigail::workers::task_sptr>::const_iterator ti =
	 completed_tasks.begin();
       ti != completed_tasks.end();
       ++ti)
    {
      test_task_sptr t = dynamic_pointer_cast<test_task>(*ti);
      if (!t->is_ok)
	{
	  is_ok = false;
	  if (!t->error_message.empty())
	    cerr << t->error_message << '\n';
	}
    }

  return !is_ok;
}

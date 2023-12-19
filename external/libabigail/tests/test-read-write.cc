// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file read an XML corpus file (in the native Abigail XML format),
/// save it back and diff the resulting XML file against the input
/// file.  They should be identical.

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "abg-ir.h"
#include "abg-reader.h"
#include "abg-writer.h"
#include "abg-workers.h"
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::string;
using std::vector;
using std::ofstream;
using std::cerr;

using std::dynamic_pointer_cast;

using abigail::tools_utils::file_type;
using abigail::tools_utils::check_file;
using abigail::tools_utils::guess_file_type;
using abigail::tests::get_build_dir;
using abigail::ir::environment;
using abigail::ir::environment_sptr;
using abigail::translation_unit_sptr;
using abigail::corpus_sptr;
using abigail::xml_reader::read_translation_unit_from_file;
using abigail::xml_reader::read_corpus_from_native_xml_file;
using abigail::xml_writer::write_translation_unit;

using abigail::workers::queue;
using abigail::workers::task;
using abigail::workers::task_sptr;
using abigail::workers::get_number_of_threads;

/// This is an aggregate that specifies where a test shall get its
/// input from, and where it shall write its ouput to.
struct InOutSpec
{
  const char* in_path;
  const char* in_suppr_spec_path;
  const char* ref_out_path;
  const char* out_path;
};// end struct InOutSpec


InOutSpec in_out_specs[] =
{
  {
    "data/test-read-write/test0.xml",
    "",
    "data/test-read-write/test0.xml",
    "output/test-read-write/test0.xml"
  },
  {
    "data/test-read-write/test1.xml",
    "",
    "data/test-read-write/test1.xml",
    "output/test-read-write/test1.xml"
  },
  {
    "data/test-read-write/test2.xml",
    "",
    "data/test-read-write/test2.xml",
    "output/test-read-write/test2.xml"
  },
  {
    "data/test-read-write/test3.xml",
    "",
    "data/test-read-write/test3.xml",
    "output/test-read-write/test3.xml"
  },
  {
    "data/test-read-write/test4.xml",
    "",
    "data/test-read-write/test4.xml",
    "output/test-read-write/test4.xml"
  },
  {
    "data/test-read-write/test5.xml",
    "",
    "data/test-read-write/test5.xml",
    "output/test-read-write/test5.xml"
  },
  {
    "data/test-read-write/test6.xml",
    "",
    "data/test-read-write/test6.xml",
    "output/test-read-write/test6.xml"
  },
  {
    "data/test-read-write/test7.xml",
    "",
    "data/test-read-write/test7.xml",
    "output/test-read-write/test7.xml"
  },
  {
    "data/test-read-write/test8.xml",
    "",
    "data/test-read-write/test8.xml",
    "output/test-read-write/test8.xml"
  },
  {
    "data/test-read-write/test9.xml",
    "",
    "data/test-read-write/test9.xml",
    "output/test-read-write/test9.xml"
  },
  {
    "data/test-read-write/test10.xml",
    "",
    "data/test-read-write/test10.xml",
    "output/test-read-write/test10.xml"
  },
  {
    "data/test-read-write/test11.xml",
    "",
    "data/test-read-write/test11.xml",
    "output/test-read-write/test11.xml"
  },
  {
    "data/test-read-write/test12.xml",
    "",
    "data/test-read-write/test12.xml",
    "output/test-read-write/test12.xml"
  },
  {
    "data/test-read-write/test13.xml",
    "",
    "data/test-read-write/test13.xml",
    "output/test-read-write/test13.xml"
  },
  {
    "data/test-read-write/test14.xml",
    "",
    "data/test-read-write/test14.xml",
    "output/test-read-write/test14.xml"
  },
  {
    "data/test-read-write/test15.xml",
    "",
    "data/test-read-write/test15.xml",
    "output/test-read-write/test15.xml"
  },
  {
    "data/test-read-write/test16.xml",
    "",
    "data/test-read-write/test16.xml",
    "output/test-read-write/test16.xml"
  },
  {
    "data/test-read-write/test17.xml",
    "",
    "data/test-read-write/test17.xml",
    "output/test-read-write/test17.xml"
  },
  {
    "data/test-read-write/test18.xml",
    "",
    "data/test-read-write/test18.xml",
    "output/test-read-write/test18.xml"
  },
  {
    "data/test-read-write/test19.xml",
    "",
    "data/test-read-write/test19.xml",
    "output/test-read-write/test19.xml"
  },
  {
    "data/test-read-write/test20.xml",
    "",
    "data/test-read-write/test20.xml",
    "output/test-read-write/test20.xml"
  },
  {
    "data/test-read-write/test21.xml",
    "",
    "data/test-read-write/test21.xml",
    "output/test-read-write/test21.xml"
  },
  {
    "data/test-read-write/test22.xml",
    "",
    "data/test-read-write/test22.xml",
    "output/test-read-write/test22.xml"
  },
  {
    "data/test-read-write/test23.xml",
    "",
    "data/test-read-write/test23.xml",
    "output/test-read-write/test23.xml"
  },
  {
    "data/test-read-write/test24.xml",
    "",
    "data/test-read-write/test24.xml",
    "output/test-read-write/test24.xml"
  },
  {
    "data/test-read-write/test25.xml",
    "",
    "data/test-read-write/test25.xml",
    "output/test-read-write/test25.xml"
  },
  {
    "data/test-read-write/test26.xml",
    "",
    "data/test-read-write/test26.xml",
    "output/test-read-write/test26.xml"
  },
  {
    "data/test-read-write/test27.xml",
    "",
    "data/test-read-write/test27.xml",
    "output/test-read-write/test27.xml"
  },
  {
    "data/test-read-write/test28.xml",
    "data/test-read-write/test28-drop-std-fns.abignore",
    "data/test-read-write/test28-without-std-fns-ref.xml",
    "output/test-read-write/test28-without-std-fns.xml"
  },
  {
    "data/test-read-write/test28.xml",
    "data/test-read-write/test28-drop-std-vars.abignore",
    "data/test-read-write/test28-without-std-vars-ref.xml",
    "output/test-read-write/test28-without-std-vars.xml"
  },
  {
    "data/test-read-write/test-crc.xml",
    "",
    "data/test-read-write/test-crc.xml",
    "output/test-read-write/test-crc.xml",
  },
  // This should be the last entry.
  {NULL, NULL, NULL, NULL}
};

/// A task wihch reads an abixml file using abilint and compares its
/// output against a reference output.
struct test_task : public abigail::workers::task
{
  InOutSpec spec;
  bool is_ok;
  string in_path, out_path, in_suppr_spec_path, ref_out_path;
  string diff_cmd, error_message;

  /// Constructor of the task.
  ///
  /// @param the spec of where to find the abixml file to read and the
  /// reference output of the test.
  test_task( InOutSpec& s)
    : spec(s),
      is_ok(true)
  {}

  /// This method defines what the task performs.
  virtual void
  perform()
  {
    string input_suffix(spec.in_path);
    in_path =
      string(abigail::tests::get_src_dir()) + "/tests/" + input_suffix;

    if (!check_file(in_path, cerr))
      {
	is_ok = false;
	return;
      }

    string ref_out_path_suffix(spec.ref_out_path);
    ref_out_path =
      string(abigail::tests::get_src_dir())
      + "/tests/" + ref_out_path_suffix;

    if (!check_file(ref_out_path, cerr))
      {
	is_ok = false;
	return;
      }

    if (spec.in_suppr_spec_path && strcmp(spec.in_suppr_spec_path, ""))
      {
	in_suppr_spec_path = string(spec.in_suppr_spec_path);
	in_suppr_spec_path =
	  string(abigail::tests::get_src_dir())
	  + "/tests/"
	  + in_suppr_spec_path;
      }
    else
      in_suppr_spec_path.clear();

    environment_sptr env(new environment);
    translation_unit_sptr tu;
    corpus_sptr corpus;

    file_type t = guess_file_type(in_path);
    if (t == abigail::tools_utils::FILE_TYPE_UNKNOWN)
      {
	cerr << in_path << "is an unknown file type\n";
	is_ok = false;
	return;
      }

    string output_suffix(spec.out_path);
    out_path =
      string(abigail::tests::get_build_dir()) + "/tests/" + output_suffix;
    if (!abigail::tools_utils::ensure_parent_dir_created(out_path))
      {
	error_message =
	  "Could not create parent director for " + out_path;
	is_ok = false;
	return;
      }

    string abilint = string(get_build_dir()) + "/tools/abilint";
    if (!in_suppr_spec_path.empty())
      abilint +=string(" --suppr ") + in_suppr_spec_path;
    string cmd = abilint + " " + in_path + " > " + out_path;

    if (system(cmd.c_str()))
      {
	error_message =
	  "ABI XML file doesn't pass abilint: " + out_path + "\n";
	is_ok = false;
      }

    cmd = "diff -u " + ref_out_path + " " + out_path;
    diff_cmd = cmd;
    if (system(cmd.c_str()))
      is_ok = false;
  }
};// end struct test_task

/// A convenience typedef for shared
typedef shared_ptr<test_task> test_task_sptr;

/// Walk the array of InOutSpecs above, read the input files it points
/// to, write it into the output it points to and diff them.
int
main()
{
  using abigail::workers::queue;
  using abigail::workers::task;
  using abigail::workers::task_sptr;
  using abigail::workers::get_number_of_threads;

  const size_t num_tests = sizeof(in_out_specs) / sizeof (InOutSpec) - 1;
  size_t num_workers = std::min(get_number_of_threads(), num_tests);
  queue task_queue(num_workers);

  bool is_ok = true;


  string in_path, out_path, in_suppr_spec_path, ref_out_path;
  for (InOutSpec* s = in_out_specs; s->in_path; ++s)
    {
      test_task_sptr t(new test_task(*s));
      ABG_ASSERT(task_queue.schedule_task(t));
    }

  /// Wait for all worker threads to finish their job, and wind down.
  task_queue.wait_for_workers_to_complete();

  // Now walk the results and print whatever error messages need to be
  // printed.

  const vector<task_sptr>& completed_tasks =
    task_queue.get_completed_tasks();

  ABG_ASSERT(completed_tasks.size() == num_tests);

  for (vector<task_sptr>::const_iterator ti = completed_tasks.begin();
       ti != completed_tasks.end();
       ++ti)
    {
      test_task_sptr t = dynamic_pointer_cast<test_task>(*ti);
      if (!t->is_ok)
	{
	  is_ok = false;

	  if (!t->error_message.empty())
	    cerr << t->error_message << '\n';

	  if (!t->diff_cmd.empty())
	    if (system(t->diff_cmd.c_str()) == -1)
	      cerr << "execution of '" << t->diff_cmd << "' failed\n";
	}
    }

  return !is_ok;
}

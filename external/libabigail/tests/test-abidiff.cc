// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program runs a diff between input files and compares the
/// resulting report with a reference report.  If the resulting report
/// is different from the reference report, the test has failed.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "abg-reader.h"
#include "test-utils.h"
#include "abg-comparison.h"
#include "abg-corpus.h"

using std::string;
using std::ofstream;
using std::cerr;

struct InOutSpec
{
  const char* first_in_path;
  const char* second_in_path;
  const char* ref_diff_path;
  const char* out_path;
};// end struct InOutSpec

static InOutSpec specs[] =
{
  {
    "data/test-abidiff/test-enum0-v0.cc.bi",
    "data/test-abidiff/test-enum0-v1.cc.bi",
    "data/test-abidiff/test-enum0-report.txt",
    "output/test-abidiff/test-enum0-report.txt"
  },
  {
    "data/test-abidiff/test-enum1-v0.cc.bi",
    "data/test-abidiff/test-enum1-v1.cc.bi",
    "data/test-abidiff/test-enum1-report.txt",
    "output/test-abidiff/test-enum1-report.txt"
  },
  {
    "data/test-abidiff/test-qual-type0-v0.cc.bi",
    "data/test-abidiff/test-qual-type0-v1.cc.bi",
    "data/test-abidiff/test-qual-type0-report.txt",
    "output/test-abidiff/test-qual-type0-report.txt"
  },
  {
    "data/test-abidiff/test-struct0-v0.cc.bi",
    "data/test-abidiff/test-struct0-v1.cc.bi",
    "data/test-abidiff/test-struct0-report.txt",
    "output/test-abidiff/test-struct0-report.txt"
  },
  {
    "data/test-abidiff/test-struct1-v0.cc.bi",
    "data/test-abidiff/test-struct1-v1.cc.bi",
    "data/test-abidiff/test-struct1-report.txt",
    "output/test-abidiff/test-struct1-report.txt"
  },
  {
    "data/test-abidiff/test-var0-v0.cc.bi",
    "data/test-abidiff/test-var0-v1.cc.bi",
    "data/test-abidiff/test-var0-report.txt",
    "output/test-abidiff/test-var0-report.txt"
  },
  {
    "data/test-abidiff/test-PR18166-libtirpc.so.abi",
    "data/test-abidiff/test-PR18166-libtirpc.so.abi",
    "data/test-abidiff/empty-report.txt",
    "output/test-abidiff/test-PR18166-libtirpc.so.report.txt"
  },
  {
    "data/test-abidiff/test-PR18791-v0.so.abi",
    "data/test-abidiff/test-PR18791-v1.so.abi",
    "data/test-abidiff/test-PR18791-report0.txt",
    "output/test-abidiff/test-PR18791-report0.txt"
  },
  {
    "data/test-abidiff/test-PR24552-v0.abi",
    "data/test-abidiff/test-PR24552-v1.abi",
    "data/test-abidiff/empty-report.txt",
    "output/test-abidiff/test-PR24552-report0.txt"
  },
  {
    "data/test-abidiff/test-empty-corpus-0.xml",
    "data/test-abidiff/test-empty-corpus-0.xml",
    "data/test-abidiff/empty-report.txt",
    "output/test-abidiff/test-empty-corpus-report-0.txt"
  },
  {
    "data/test-abidiff/test-empty-corpus-1.xml",
    "data/test-abidiff/test-empty-corpus-1.xml",
    "data/test-abidiff/empty-report.txt",
    "output/test-abidiff/test-empty-corpus-report-1.txt"
  },
  {
    "data/test-abidiff/test-empty-corpus-2.xml",
    "data/test-abidiff/test-empty-corpus-2.xml",
    "data/test-abidiff/empty-report.txt",
    "output/test-abidiff/test-empty-corpus-report-1.txt"
  },
  {
    "data/test-abidiff/test-crc-0.xml",
    "data/test-abidiff/test-crc-1.xml",
    "data/test-abidiff/empty-report.txt",
    "output/test-abidiff/test-crc-report-0-1.txt"
  },
  {
    "data/test-abidiff/test-crc-1.xml",
    "data/test-abidiff/test-crc-0.xml",
    "data/test-abidiff/empty-report.txt",
    "output/test-abidiff/test-crc-report-1-0.txt"
  },
  {
    "data/test-abidiff/test-crc-1.xml",
    "data/test-abidiff/test-crc-2.xml",
    "data/test-abidiff/test-crc-report.txt",
    "output/test-abidiff/test-crc-report-1-2.txt"
  },
  // This should be the last entry.
  {0, 0, 0, 0}
};

#define  NUM_SPEC_ELEMS \
  ((sizeof(specs) / sizeof(InOutSpec)) - 1)

using std::string;
using std::cerr;
using std::ofstream;
using abigail::tools_utils::file_type;
using abigail::tools_utils::check_file;
using abigail::tools_utils::guess_file_type;
using abigail::ir::environment;
using abigail::ir::environment_sptr;
using abigail::corpus_sptr;
using abigail::corpus_group_sptr;
using abigail::translation_unit;
using abigail::translation_unit_sptr;
using abigail::xml_reader::read_translation_unit_from_file;
using abigail::xml_reader::read_corpus_from_native_xml_file;
using abigail::xml_reader::read_corpus_group_from_native_xml_file;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::translation_unit_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::comparison::diff_context_sptr;
using abigail::comparison::diff_context;

int
main(int, char*[])
{
  bool is_ok = true;

  string out_path =
    string(abigail::tests::get_build_dir()) + "/tests/" + specs->out_path;
  if (!abigail::tools_utils::ensure_parent_dir_created(out_path))
    {
      cerr << "Could not create parent director for " << out_path;
      return 1;
    }

  string first_in_path, second_in_path, ref_diff_path;
  for (InOutSpec *s = specs; s->first_in_path; ++s)
    {
      first_in_path =
	string(abigail::tests::get_src_dir()) + "/tests/" + s->first_in_path;
      second_in_path =
	string(abigail::tests::get_src_dir()) + "/tests/" + s->second_in_path;
      ref_diff_path =
	string(abigail::tests::get_src_dir()) + "/tests/" + s->ref_diff_path;
      out_path =
	string(abigail::tests::get_build_dir()) + "/tests/" + s->out_path;

      if (!abigail::tools_utils::ensure_parent_dir_created(out_path))
	{
	  cerr << "Could not create parent directory for " << out_path;
	  continue;
	}

      environment_sptr env(new environment);
      translation_unit_sptr tu1, tu2;
      corpus_sptr corpus1, corpus2;
      corpus_group_sptr corpus_group1, corpus_group2;
      file_type t = guess_file_type(first_in_path);
      if (t == abigail::tools_utils::FILE_TYPE_NATIVE_BI)
	tu1 = read_translation_unit_from_file(first_in_path, env.get());
      else if (t == abigail::tools_utils::FILE_TYPE_XML_CORPUS)
	corpus1 = read_corpus_from_native_xml_file(first_in_path, env.get());
      else if (t == abigail::tools_utils::FILE_TYPE_XML_CORPUS_GROUP)
	corpus_group1 = read_corpus_group_from_native_xml_file(first_in_path,
							       env.get());
      else
	abort();
      if (!tu1 && !corpus1 && !corpus_group1)
	{
	  cerr << "failed to read " << first_in_path << "\n";
	  is_ok = false;
	  continue;
	}

      t = guess_file_type(second_in_path);
      if (t == abigail::tools_utils::FILE_TYPE_NATIVE_BI)
	tu2 = read_translation_unit_from_file(second_in_path, env.get());
      else if (t == abigail::tools_utils::FILE_TYPE_XML_CORPUS)
	corpus2 = read_corpus_from_native_xml_file(second_in_path, env.get());
      else if (t == abigail::tools_utils::FILE_TYPE_XML_CORPUS_GROUP)
	corpus_group2 = read_corpus_group_from_native_xml_file(first_in_path,
							       env.get());
      else
	abort();
      if (!tu2 && !corpus2 && !corpus_group2)
	{
	  cerr << "failed to read " << second_in_path << "\n";
	  is_ok = false;
	  continue;
	}

      translation_unit_diff_sptr d1;
      corpus_diff_sptr d2;
      diff_context_sptr ctxt(new diff_context);
      ctxt->show_locs(false);
      if (tu1)
	d1= compute_diff(tu1, tu2, ctxt);
      else if (corpus1)
	d2 = compute_diff(corpus1, corpus2, ctxt);
      else if (corpus_group1)
	d2 = compute_diff(corpus_group1, corpus_group2, ctxt);
      ofstream of(out_path.c_str(), std::ios_base::trunc);
      if (!of.is_open())
	{
	  cerr << "failed to read " << s->out_path << "\n";
	  is_ok = false;
	  continue;
	}

      if (d1)
	d1->report(of);
      else
	d2->report(of);
      of.close();

      string cmd = "diff -u " + ref_diff_path + " " + out_path;
      if (system(cmd.c_str()))
	is_ok = false;
    }

  return !is_ok;
}

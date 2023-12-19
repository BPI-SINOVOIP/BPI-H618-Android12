// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2021 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the declarations of the entry points to
/// de-serialize an instance of @ref abigail::translation_unit from an
/// ABI Instrumentation file in libabigail native XML format.

#ifndef __ABG_READER_H__
#define __ABG_READER_H__

#include <istream>
#include "abg-corpus.h"
#include "abg-suppression.h"

namespace abigail
{

namespace xml_reader
{

using namespace abigail::ir;

translation_unit_sptr
read_translation_unit_from_file(const std::string&	file_path,
				environment*		env);

translation_unit_sptr
read_translation_unit_from_buffer(const std::string&	file_path,
				  environment*		env);

translation_unit_sptr
read_translation_unit_from_istream(std::istream*	in,
				   environment*	env);

class read_context;

/// A convenience typedef for a shared pointer to read_context.
typedef shared_ptr<read_context> read_context_sptr;

read_context_sptr
create_native_xml_read_context(const string& path, environment *env);

read_context_sptr
create_native_xml_read_context(std::istream* in, environment* env);

const string&
read_context_get_path(const read_context&);

corpus_sptr
read_corpus_from_native_xml(std::istream* in,
			    environment*  env);

corpus_sptr
read_corpus_from_native_xml_file(const string& path,
				 environment*  env);

corpus_sptr
read_corpus_from_input(read_context& ctxt);

corpus_group_sptr
read_corpus_group_from_input(read_context& ctxt);

corpus_group_sptr
read_corpus_group_from_native_xml(std::istream* in,
				  environment*  env);

corpus_group_sptr
read_corpus_group_from_native_xml_file(const string& path,
				       environment*  env);

void
add_read_context_suppressions(read_context& ctxt,
			      const suppr::suppressions_type& supprs);

void
consider_types_not_reachable_from_public_interfaces(read_context& ctxt,
						    bool flag);
}//end xml_reader
}//end namespace abigail

#endif // __ABG_READER_H__

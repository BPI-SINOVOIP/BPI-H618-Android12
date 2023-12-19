// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the declarations of the entry points to
/// de-serialize an instance of @ref abigail::translation_unit to an
/// ABI Instrumentation file in libabigail native XML format.

#ifndef __ABG_WRITER_H__
#define __ABG_WRITER_H__

#include "abg-fwd.h"

namespace abigail
{
namespace xml_writer
{

using namespace abigail::ir;

/// The style of type id the XML writer will output.
enum type_id_style_kind
{
  SEQUENCE_TYPE_ID_STYLE,
  HASH_TYPE_ID_STYLE
};

class write_context;

/// A convenience typedef for a shared pointer to write_context.
typedef shared_ptr<write_context> write_context_sptr;

write_context_sptr
create_write_context(const environment *env,
		     ostream& output_stream);

void
set_show_locs(write_context& ctxt, bool flag);

void
set_annotate(write_context& ctxt, bool flag);

void
set_write_architecture(write_context& ctxt, bool flag);

void
set_write_corpus_path(write_context& ctxt, bool flag);

void
set_write_comp_dir(write_context& ctxt, bool flag);

void
set_write_elf_needed(write_context& ctxt, bool flag);

void
set_write_default_sizes(write_context& ctxt, bool flag);

void
set_short_locs(write_context& ctxt, bool flag);

void
set_write_parameter_names(write_context& ctxt, bool flag);

void
set_type_id_style(write_context& ctxt, type_id_style_kind style);

/// A convenience generic function to set common options (usually used
/// by Libabigail tools) from a generic options carrying-object, into
/// a given @ref write_context.
///
/// @param ctxt the @ref the write_context to consider.
///
/// @param opts the option-carrying object to set the options from.
/// It must contain data members named: annotate, and show_locs, at
/// very least.
template <typename OPTS>
void
set_common_options(write_context& ctxt, const OPTS& opts)
{
  set_annotate(ctxt, opts.annotate);
  set_show_locs(ctxt, opts.show_locs);
  set_write_architecture(ctxt, opts.write_architecture);
  set_write_corpus_path(ctxt, opts.write_corpus_path);
  set_write_comp_dir(ctxt, opts.write_comp_dir);
  set_write_elf_needed(ctxt, opts.write_elf_needed);
  set_write_parameter_names(ctxt, opts.write_parameter_names);
  set_short_locs(ctxt, opts.short_locs);
  set_write_default_sizes(ctxt, opts.default_sizes);
  set_type_id_style(ctxt, opts.type_id_style);
}

void
set_ostream(write_context& ctxt, ostream& os);

bool
write_translation_unit(write_context&	       ctxt,
		       const translation_unit& tu,
		       const unsigned	       indent);

bool
write_corpus_to_archive(const corpus& corp,
			const string& path,
			const bool  annotate = false);

bool
write_corpus_to_archive(const corpus& corp,
			const bool annotate = false);

bool
write_corpus_to_archive(const corpus_sptr corp,
			const bool annotate = false);

bool
write_corpus(write_context&	ctxt,
	     const corpus_sptr& corpus,
	     unsigned		indent,
	     bool		member_of_group = false);

bool
write_corpus_group(write_context&	    ctx,
		   const corpus_group_sptr& group,
		   unsigned		    indent);

}// end namespace xml_writer
}// end namespace abigail

#endif //  __ABG_WRITER_H__

// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2017-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

#include <libgen.h>
#include <algorithm>
#include "abg-comparison-priv.h"
#include "abg-reporter-priv.h"

namespace abigail
{

namespace comparison
{

/// Convert a number in bits into a number in bytes.
///
/// @param bits the number in bits to convert.
///
/// @return the number @p bits converted into bytes.
uint64_t
convert_bits_to_bytes(size_t bits)
{return bits / 8;}

/// Emit a numerical value to an output stream.
///
/// Depending on the current @ref diff_context, the number is going to
/// be emitted either in decimal or hexadecimal base.
///
/// @param value the value to emit.
///
/// @param ctxt the current diff context.
///
/// @param out the output stream to emit the numerical value to.
void
emit_num_value(uint64_t value, const diff_context& ctxt, ostream& out)
{
  if (ctxt.show_hex_values())
    out << std::hex << std::showbase ;
  else
    out << std::dec;
  out << value << std::dec << std::noshowbase;
}

/// Convert a bits value into a byte value if the current diff context
/// instructs us to do so.
///
/// @param bits the bits value to convert.
///
/// @param ctxt the current diff context to consider.
///
/// @return the resulting bits or bytes value, depending on what the
/// diff context instructs us to do.
uint64_t
maybe_convert_bits_to_bytes(uint64_t bits, const diff_context& ctxt)
{
  if (ctxt.show_offsets_sizes_in_bits())
    return bits;
  return convert_bits_to_bytes(bits);
}

/// Emit a message showing the numerical change between two values, to
/// a given output stream.
///
/// The function emits a message like
///
///      "XXX changes from old_bits to new_bits (in bits)"
///
/// or
///
///      "XXX changes from old_bits to new_bits (in bytes)"
///
/// Depending on if the current diff context instructs us to emit the
/// change in bits or bytes.  XXX is the string content of the @p what
/// parameter.
///
/// @param what the string that tells us what the change represents.
/// This is the "XXX" we refer to in the explanation above.
///
/// @param old_bits the initial value (which changed) in bits.
///
/// @param new_bits the final value (resulting from the change or @p
/// old_bits) in bits.
///
/// @param ctxt the current diff context to consider.
///
/// @param out the output stream to send the change message to.
///
/// @param show_bits_or_byte if this is true, then the message is
/// going to precise if the changed value is in bits or bytes.
/// Otherwise, no mention of that is made.
void
show_numerical_change(const string&	what,
		      uint64_t		old_bits,
		      uint64_t		new_bits,
		      const diff_context& ctxt,
		      ostream&		out,
		      bool show_bits_or_byte)
{
  bool can_convert_bits_to_bytes = (old_bits % 8 == 0 && new_bits % 8 == 0);
  uint64_t o = can_convert_bits_to_bytes
    ? maybe_convert_bits_to_bytes(old_bits, ctxt)
    : old_bits;
  uint64_t n = can_convert_bits_to_bytes
    ? maybe_convert_bits_to_bytes(new_bits, ctxt)
    : new_bits;
  string bits_or_bytes =
    (!can_convert_bits_to_bytes || ctxt.show_offsets_sizes_in_bits ())
    ? "bits"
    : "bytes";

  out << what << " changed from ";
  emit_num_value(o, ctxt, out);
  out << " to ";
  emit_num_value(n, ctxt, out);
  if (show_bits_or_byte)
    {
      out << " (in ";
      out << bits_or_bytes;
      out << ")";
    }
}

/// Emit a message showing the value of a numerical value representing
/// a size or an offset, preceded by a string.  The message is ended
/// by a part which says if the value is in bits or bytes.
///
/// @param what the string prefix of the message to emit.
///
/// @param value the numerical value to emit.
///
/// @param ctxt the diff context to take into account.
///
/// @param out the output stream to emit the message to.
void
show_offset_or_size(const string&	what,
		    uint64_t		value,
		    const diff_context& ctxt,
		    ostream&		out)
{
  uint64_t v = value;
  bool can_convert_bits_to_bytes = (value % 8 == 0);
  if (can_convert_bits_to_bytes)
    v = maybe_convert_bits_to_bytes(v, ctxt);
  string bits_or_bytes =
    (!can_convert_bits_to_bytes || ctxt.show_offsets_sizes_in_bits())
    ? "bits"
    : "bytes";

  if (!what.empty())
    out << what << " ";
  emit_num_value(v, ctxt, out);
  out << " (in " << bits_or_bytes << ")";
}

/// Emit a message showing the value of a numerical value representing
/// a size or an offset.  The message is ended by a part which says if
/// the value is in bits or bytes.
///
/// @param value the numerical value to emit.
///
/// @param ctxt the diff context to take into account.
///
/// @param out the output stream to emit the message to.
void
show_offset_or_size(uint64_t		value,
		    const diff_context& ctxt,
		    ostream&		out)
{show_offset_or_size("", value, ctxt, out);}

/// Stream a string representation for a member function.
///
/// @param ctxt the current diff context.
///
/// @param mem_fn the member function to stream
///
/// @param out the output stream to send the representation to
void
represent(const diff_context& ctxt,
	  method_decl_sptr mem_fn,
	  ostream& out)
{
  if (!mem_fn || !is_member_function(mem_fn))
    return;

  method_decl_sptr meth =
    dynamic_pointer_cast<method_decl>(mem_fn);
  ABG_ASSERT(meth);

  out << "'" << mem_fn->get_pretty_representation() << "'";
  report_loc_info(meth, ctxt, out);
  if (get_member_function_is_virtual(mem_fn))
    {

      ssize_t voffset = get_member_function_vtable_offset(mem_fn);
      ssize_t biggest_voffset =
	is_class_type(meth->get_type()->get_class_type())->
	get_biggest_vtable_offset();
      if (voffset > -1)
	{
	  out << ", virtual at voffset ";
	  emit_num_value(get_member_function_vtable_offset(mem_fn),
			 ctxt, out);
	  out << "/";
	  emit_num_value(biggest_voffset, ctxt, out);
	}
    }

  if (ctxt.show_linkage_names()
      && (mem_fn->get_symbol()))
    {
      out << "    {"
	  << mem_fn->get_symbol()->get_id_string()
	  << "}";
    }
  out << "\n";
}

/// Stream a string representation for a data member.
///
/// @param d the data member to stream
///
/// @param ctxt the current diff context.
///
/// @param out the output stream to send the representation to
///
/// @param indent the indentation string to use for the change report.
void
represent_data_member(var_decl_sptr d,
		      const diff_context_sptr& ctxt,
		      ostream& out,
		      const string& indent)
{
  if (!is_data_member(d)
      || (!get_member_is_static(d) && !get_data_member_is_laid_out(d)))
    return;

  out << indent
      << "'"
      << d->get_pretty_representation(/*internal=*/false,
				      /*qualified_name=*/false)
      << "'";
  if (!get_member_is_static(d))
    {
      // Do not emit offset information for data member of a union
      // type because all data members of a union are supposed to be
      // at offset 0.
      if (!is_union_type(d->get_scope()))
	show_offset_or_size(", at offset",
			    get_data_member_offset(d),
			    *ctxt, out);
      report_loc_info(d, *ctxt, out);
    }
  out << "\n";
}

/// If a given @ref var_diff node carries a data member change in
/// which the offset of the data member actually changed, then emit a
/// string (to an output stream) that represents that offset change.
///
/// For instance, if the offset of the data member increased by 32
/// bits then the string emitted is going to be "by +32 bits".
///
/// If, on the other hand, the offset of the data member decreased by
/// 64 bits then the string emitted is going to be "by -64 bits".
///
/// This function is a sub-routine used by the reporting system.
///
/// @param diff the diff node that potentially carries the data member
/// change.
///
/// @param ctxt the context in which the diff is being reported.
///
/// @param out the output stream to emit the string to.
void
maybe_show_relative_offset_change(const var_diff_sptr &diff,
				  diff_context& ctxt,
				  ostream&	out)
{
  if (!ctxt.show_relative_offset_changes())
    return;

  var_decl_sptr o = diff->first_var();
  var_decl_sptr n = diff->second_var();

  uint64_t first_offset = get_data_member_offset(o),
    second_offset = get_data_member_offset(n);

  string sign;
  uint64_t change = 0;
  if (first_offset < second_offset)
    {
      sign = "+";
      change = second_offset - first_offset;
    }
  else if (first_offset > second_offset)
    {
      sign = "-";
      change = first_offset - second_offset;
    }
  else
    return;

  if (!ctxt.show_offsets_sizes_in_bits())
    change = convert_bits_to_bytes(change);

  string bits_or_bytes = ctxt.show_offsets_sizes_in_bits()
    ? "bits"
    : "bytes";

  out << " (by " << sign;
  emit_num_value(change, ctxt, out);
  out << " " << bits_or_bytes << ")";
}

/// If a given @ref var_diff node carries a hange in which the size of
/// the variable actually changed, then emit a string (to an output
/// stream) that represents that size change.
///
/// For instance, if the size of the variable increased by 32 bits
/// then the string emitted is going to be "by +32 bits".
///
/// If, on the other hand, the size of the variable decreased by 64
/// bits then the string emitted is going to be "by -64 bits".
///
/// This function is a sub-routine used by the reporting system.
///
/// @param diff the diff node that potentially carries the variable
/// change.
///
/// @param ctxt the context in which the diff is being reported.
///
/// @param out the output stream to emit the string to.
void
maybe_show_relative_size_change(const var_diff_sptr	&diff,
				diff_context&		ctxt,
				ostream&		out)
{
  if (!ctxt.show_relative_offset_changes())
    return;

  var_decl_sptr o = diff->first_var();
  var_decl_sptr n = diff->second_var();

  uint64_t first_size = get_var_size_in_bits(o),
    second_size = get_var_size_in_bits(n);

  string sign;
  uint64_t change = 0;
  if (first_size < second_size)
    {
      sign = "+";
      change = second_size - first_size;
    }
  else if (first_size > second_size)
    {
      sign = "-";
      change = first_size - second_size;
    }
  else
    return;

  if (!ctxt.show_offsets_sizes_in_bits())
    change = convert_bits_to_bytes(change);

  string bits_or_bytes = ctxt.show_offsets_sizes_in_bits()
    ? "bits"
    : "bytes";

  out << " (by " << sign;
  emit_num_value(change, ctxt, out);
  out << " " << bits_or_bytes << ")";
}

/// Represent the changes carried by an instance of @ref var_diff that
/// represent a difference between two class data members.
///
/// @param diff diff the diff node to represent.
///
/// @param ctxt the diff context to use.
///
/// @param local_only if true, only display local changes.
///
/// @param out the output stream to send the representation to.
///
/// @param indent the indentation string to use for the change report.
void
represent(const var_diff_sptr	&diff,
	  diff_context_sptr	ctxt,
	  ostream&		out,
	  const string&	indent,
	  bool			local_only)
{
  if (!ctxt->get_reporter()->diff_to_be_reported(diff.get()))
    return;

  const var_decl_sptr o = diff->first_var();
  const var_decl_sptr n = diff->second_var();
  const bool o_anon = !!is_anonymous_data_member(o);
  const bool n_anon = !!is_anonymous_data_member(n);
  const bool is_strict_anonymous_data_member_change = o_anon && n_anon;
  const string o_name = o->get_qualified_name();
  const string n_name = n->get_qualified_name();
  const uint64_t o_size = get_var_size_in_bits(o);
  const uint64_t n_size = get_var_size_in_bits(n);
  const uint64_t o_offset = get_data_member_offset(o);
  const uint64_t n_offset = get_data_member_offset(n);
  const string o_pretty_representation =
    o->get_pretty_representation(/*internal=*/false, /*qualified_name=*/false);
  // no n_pretty_representation here as it's only needed in a couple of places
  const bool show_size_offset_changes = ctxt->get_allowed_category()
					& SIZE_OR_OFFSET_CHANGE_CATEGORY;

  // Has the main diff text been output?
  bool emitted = false;
  // Are we continuing on a new line? (implies emitted)
  bool begin_with_and = false;
  // Have we reported a size change already?
  bool size_reported = false;

  //----------------------------------------------------------------
  // First we'll try to emit a report about the type change of this
  // var_decl_diff.
  //
  // In the context of that type change report, we need to keep in
  // mind that because we want to emit specific (useful) reports about
  // anonymous data member changes, we'll try to detect the various
  // scenarii that involve anonymous data member changes.
  //
  // Then, as a fallback method, we'll emit a more generic type change
  // report for the other generic type changes.
  //----------------------------------------------------------------

  if (is_strict_anonymous_data_member_change)
    {
      const string n_pretty_representation =
	n->get_pretty_representation(/*internal=*/false,
				     /*qualified_name=*/false);
      const type_base_sptr o_type = o->get_type(), n_type = n->get_type();
      if (o_pretty_representation != n_pretty_representation)
	{
	  show_offset_or_size(indent + "anonymous data member at offset",
			      o_offset, *ctxt, out);

	  out << " changed from:\n"
	      << indent << "  " << o_pretty_representation << "\n"
	      << indent << "to:\n"
	      << indent << "  " << n_pretty_representation << "\n";

	  begin_with_and = true;
	  emitted = true;
	}
      else if (get_type_name(o_type) != get_type_name(n_type)
	       && is_decl(o_type) && is_decl(n_type)
	       && is_decl(o_type)->get_is_anonymous()
	       && is_decl(n_type)->get_is_anonymous())
	{
	  out << indent << "while looking at anonymous data member '"
	      << o_pretty_representation << "':\n"
	      << indent << "the internal name of that anonymous data member"
			   " changed from:\n"
	      << indent << " " << get_type_name(o_type) << "\n"
	      << indent << "to:\n"
	      << indent << " " << get_type_name(n_type) << "\n"
	      << indent << " This is usually due to "
	      << "an anonymous member type being added or removed from "
	      << "the containing type\n";

	  begin_with_and = true;
	  emitted = true;
	}
    }
  else if (filtering::has_anonymous_data_member_change(diff))
    {
      ABG_ASSERT(o_anon != n_anon);
      // So we are looking at a non-anonymous data member change from
      // or to an anonymous data member.
      const string n_pretty_representation =
	n->get_pretty_representation(/*internal=*/false,
				     /*qualified_name=*/false);
      out << indent << (o_anon ? "anonymous " : "")
	  << "data member " << o_pretty_representation;
      show_offset_or_size(" at offset", o_offset, *ctxt, out);
      out << " became " << (n_anon ? "anonymous " : "")
	  << "data member '" << n_pretty_representation << "'\n";

      begin_with_and = true;
      emitted = true;
    }

  //
  // If we haven't succeeded in emitting a specific type change report
  // (mainly related to anonymous data data member changes) then let's
  // try to emit a more generic report about the type change.
  //
  // This is the fallback method outlined in the comment at the
  // beginning of this section.
  //
  if (!emitted)
    if (const diff_sptr d = diff->type_diff())
      {
	if (ctxt->get_reporter()->diff_to_be_reported(d.get()))
	  {
	    if (local_only)
	      out << indent << "type '"
		  << get_pretty_representation(o->get_type())
		  << "' of '"
		  << (o_anon ?
		      string("anonymous data member")
		      : o->get_qualified_name())
		  << "' changed";
	    else
	      out << indent
		  << "type of '"<< (o_anon ? "anonymous data member ": "")
		  << o_pretty_representation << "' changed";

	    if (d->currently_reporting())
	      out << ", as being reported\n";
	    else if (d->reported_once())
	      out << ", as reported earlier\n";
	    else
	      {
		out << ":\n";
		d->report(out, indent + "  ");
	      }

	    begin_with_and = true;
	    emitted = true;
	    size_reported = true;
	  }
      }

  //
  // Okay, now we are done with report type changes.  Let's report the
  // other potential kinds of changes.
  //

  if (!filtering::has_anonymous_data_member_change(diff) && o_name != n_name)
    {
      if (filtering::has_harmless_name_change(o, n)
	  && !(ctxt->get_allowed_category()
	       & HARMLESS_DECL_NAME_CHANGE_CATEGORY))
	;
      else
	{
	  if (begin_with_and)
	    {
	      out << indent << "and ";
	      begin_with_and = false;
	    }
	  else if (!emitted)
	    out << indent;
	  else
	    out << ", ";
	  out << "name of '" << o_name << "' changed to '" << n_name << "'";
	  report_loc_info(n, *ctxt, out);
	  emitted = true;
	}
    }

  if (get_data_member_is_laid_out(o)
      != get_data_member_is_laid_out(n))
    {
      if (begin_with_and)
	{
	  out << indent << "and ";
	  begin_with_and = false;
	}
      else if (!emitted)
	out << indent << "'" << o_pretty_representation << "' ";
      else
	out << ", ";
      if (get_data_member_is_laid_out(o))
	out << "is no more laid out";
      else
	out << "now becomes laid out";
      emitted = true;
    }
  if (show_size_offset_changes)
    {
      if (o_offset != n_offset)
	{
	  if (begin_with_and)
	    {
	      out << indent << "and ";
	      begin_with_and = false;
	    }
	  else if (!emitted)
	    {
	      out << indent;
	      if (is_strict_anonymous_data_member_change)
		out << "anonymous data member ";
	      out << "'" << o_pretty_representation << "' ";
	    }
	  else
	    out << ", ";

	  show_numerical_change("offset", o_offset, n_offset, *ctxt, out);
	  maybe_show_relative_offset_change(diff, *ctxt, out);
	  emitted = true;
	}

      if (!size_reported && o_size != n_size)
	{
	  if (begin_with_and)
	    {
	      out << indent << "and ";
	      begin_with_and = false;
	    }
	  else if (!emitted)
	    {
	      out << indent;
	      if (is_strict_anonymous_data_member_change)
		out << "anonymous data member ";
	      out << "'" << o_pretty_representation << "' ";
	    }
	  else
	    out << ", ";

	  show_numerical_change("size", o_size, n_size, *ctxt, out);
	  maybe_show_relative_size_change(diff, *ctxt, out);
	  emitted = true;
	}
    }
  if (o->get_binding() != n->get_binding())
    {
      if (begin_with_and)
	{
	  out << indent << "and ";
	  begin_with_and = false;
	}
      else if (!emitted)
	out << indent << "'" << o_pretty_representation << "' ";
      else
	out << ", ";
      out << "elf binding changed from " << o->get_binding()
	  << " to " << n->get_binding();
      emitted = true;
    }
  if (o->get_visibility() != n->get_visibility())
    {
      if (begin_with_and)
	{
	  out << indent << "and ";
	  begin_with_and = false;
	}
      else if (!emitted)
	out << indent << "'" << o_pretty_representation << "' ";
      else
	out << ", ";
      out << "visibility changed from " << o->get_visibility()
	  << " to " << n->get_visibility();
      emitted = true;
    }
  if ((ctxt->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (get_member_access_specifier(o)
	  != get_member_access_specifier(n)))
    {
      if (begin_with_and)
	{
	  out << indent << "and ";
	  begin_with_and = false;
	}
      else if (!emitted)
	out << indent << "'" << o_pretty_representation << "' ";
      else
	out << ", ";

      out << "access changed from '"
	  << get_member_access_specifier(o)
	  << "' to '"
	  << get_member_access_specifier(n) << "'";
      emitted = true;
    }
  if (get_member_is_static(o)
      != get_member_is_static(n))
    {
      if (begin_with_and)
	{
	  out << indent << "and ";
	  begin_with_and = false;
	}
      else if (!emitted)
	out << indent << "'" << o_pretty_representation << "' ";
      else
	out << ", ";

      if (get_member_is_static(o))
	out << "is no more static";
      else
	out << "now becomes static";
      emitted = true;
    }

  if (begin_with_and)
    // do nothing as begin_with_and implies emitted
    ;
  else if (!emitted)
    // We appear to have fallen off the edge of the map.
    out << indent << "'" << o_pretty_representation
	<< "' has *some* difference - please report as a bug";
  else
    {
      ;// do nothing
    }
  emitted = true;

  if (!begin_with_and)
    out << "\n";
}

/// Report the size and alignment changes of a type.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param ctxt the content of the current diff.
///
/// @param out the output stream to report the change to.
///
/// @param indent the string to use for indentation.
void
report_size_and_alignment_changes(type_or_decl_base_sptr	first,
				  type_or_decl_base_sptr	second,
				  diff_context_sptr		ctxt,
				  ostream&			out,
				  const string&			indent)
{
  type_base_sptr f = dynamic_pointer_cast<type_base>(first),
    s = dynamic_pointer_cast<type_base>(second);

  if (!s || !f)
    return;

  class_or_union_sptr first_class = is_class_or_union_type(first),
    second_class = is_class_or_union_type(second);

  if (filtering::has_class_decl_only_def_change(first_class, second_class))
    // So these two classes differ only by the fact that one is the
    // declaration-only form of the second.  The declaration-only class
    // is of unknown size (recorded as 0) and it is not meaningful to
    // report a size change.
    return;

  unsigned fs = f->get_size_in_bits(), ss = s->get_size_in_bits(),
    fa = f->get_alignment_in_bits(), sa = s->get_alignment_in_bits();
  array_type_def_sptr first_array = is_array_type(is_type(first)),
    second_array = is_array_type(is_type(second));
  unsigned fdc = first_array ? first_array->get_dimension_count(): 0,
    sdc = second_array ? second_array->get_dimension_count(): 0;

  if (ctxt->get_allowed_category() & SIZE_OR_OFFSET_CHANGE_CATEGORY)
    {
      if (fs != ss || fdc != sdc)
	{
	  if (first_array && second_array)
	    {
	      // We are looking at size or alignment changes between two
	      // arrays ...
	      out << indent << "array type size changed from ";
	      if (first_array->is_infinite())
		out << "infinity";
	      else
		emit_num_value(first_array->get_size_in_bits(), *ctxt, out);
	      out << " to ";
	      if (second_array->is_infinite())
		out << "infinity";
	      else
		emit_num_value(second_array->get_size_in_bits(), *ctxt, out);
	      out << "\n";

	      if (sdc != fdc)
		{
		  out << indent + "  "
		      << "number of dimensions changed from "
		      << fdc
		      << " to "
		      << sdc
		      << "\n";
		}
	      array_type_def::subranges_type::const_iterator i, j;
	      for (i = first_array->get_subranges().begin(),
		     j = second_array->get_subranges().begin();
		   (i != first_array->get_subranges().end()
		    && j != second_array->get_subranges().end());
		   ++i, ++j)
		{
		  if ((*i)->get_length() != (*j)->get_length())
		    {
		      out << indent
			  << "array type subrange "
			  << i - first_array->get_subranges().begin() + 1
			  << " changed length from ";

		      if ((*i)->is_infinite())
			out << "infinity";
		      else
			out << (*i)->get_length();

		      out << " to ";

		      if ((*j)->is_infinite())
			out << "infinity";
		      else
			out << (*j)->get_length();
		      out << "\n";
		    }
		}
	    } // end if (first_array && second_array)
	  else if (fs != ss)
	    {
	      out << indent;
	      show_numerical_change("type size", fs, ss, *ctxt, out);
	      out << "\n";
	    }
	} // end if (fs != ss || fdc != sdc)
      else
	if (ctxt->show_relative_offset_changes())
	  {
	    out << indent << "type size hasn't changed\n";
	  }
    }
  if ((ctxt->get_allowed_category() & SIZE_OR_OFFSET_CHANGE_CATEGORY)
      && (fa != sa))
    {
      out << indent;
      show_numerical_change("type alignment", fa, sa, *ctxt, out,
			    /*show_bits_or_bytes=*/false);
      out << "\n";
    }
}

/// @param tod the type or declaration to emit loc info about
///
/// @param ctxt the content of the current diff.
///
/// @param out the output stream to report the change to.
///
/// @return true iff something was reported.
bool
report_loc_info(const type_or_decl_base_sptr& tod,
		const diff_context& ctxt,
		ostream &out)
{
  if (!ctxt.show_locs())
    return false;

  decl_base_sptr decl = is_decl(tod);

  if (!decl)
    return false;

  location loc;
  translation_unit* tu = get_translation_unit(decl);

  if (tu && (loc = decl->get_location()))
  {
    string path;
    unsigned line, column;

    loc.expand(path, line, column);
    //tu->get_loc_mgr().expand_location(loc, path, line, column);
    path = basename(const_cast<char*>(path.c_str()));

    out  << " at " << path << ":" << line << ":" << column;

    return true;
  }
  return false;
}

/// Report the name, size and alignment changes of a type.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param ctxt the content of the current diff.
///
/// @param out the output stream to report the change to.
///
/// @param indent the string to use for indentation.
void
report_name_size_and_alignment_changes(decl_base_sptr		first,
				       decl_base_sptr		second,
				       diff_context_sptr	ctxt,
				       ostream&			out,
				       const string&		indent)
{
  string fn = first->get_qualified_name(),
    sn = second->get_qualified_name();

  if (fn != sn)
    {
      if (!(ctxt->get_allowed_category() & HARMLESS_DECL_NAME_CHANGE_CATEGORY)
	  && filtering::has_harmless_name_change(first, second))
	// This is a harmless name change.  but then
	// HARMLESS_DECL_NAME_CHANGE_CATEGORY doesn't seem allowed.
	;
      else
	{
	  out << indent;
	  if (is_type(first))
	    out << "type";
	  else
	    out << "declaration";
	  out << " name changed from '" << fn << "' to '" << sn << "'";
	  out << "\n";
	}
    }

  report_size_and_alignment_changes(first, second, ctxt, out, indent);
}

/// Output the header preceding the the report for
/// insertion/deletion/change of a part of a class.  This is a
/// subroutine of class_diff::report.
///
/// @param out the output stream to output the report to.
///
/// @param number the number of insertion/deletion to refer to in the
/// header.
///
/// @param num_filtered the number of filtered changes.
///
/// @param k the kind of diff (insertion/deletion/change) we want the
/// head to introduce.
///
/// @param section_name the name of the sub-part of the class to
/// report about.
///
/// @param indent the string to use as indentation prefix in the
/// header.
void
report_mem_header(ostream& out,
		  size_t number,
		  size_t num_filtered,
		  diff_kind k,
		  const string& section_name,
		  const string& indent)
{
  size_t net_number = number - num_filtered;
  string change;
  char colon_or_semi_colon = ':';

  switch (k)
    {
    case del_kind:
      change = (number > 1) ? "deletions" : "deletion";
      break;
    case ins_kind:
      change = (number > 1) ? "insertions" : "insertion";
      break;
    case subtype_change_kind:
    case change_kind:
      change = (number > 1) ? "changes" : "change";
      break;
    }

  if (net_number == 0)
    {
      out << indent << "no " << section_name << " " << change;
      colon_or_semi_colon = ';';
    }
  else if (net_number == 1)
    out << indent << "1 " << section_name << " " << change;
  else
    out << indent << net_number << " " << section_name
	<< " " << change;

  if (num_filtered)
    out << " (" << num_filtered << " filtered)";
  out << colon_or_semi_colon << "\n";
}

/// Output the header preceding the the report for
/// insertion/deletion/change of a part of a class.  This is a
/// subroutine of class_diff::report.
///
/// @param out the output stream to output the report to.
///
/// @param k the kind of diff (insertion/deletion/change) we want the
/// head to introduce.
///
/// @param section_name the name of the sub-part of the class to
/// report about.
///
/// @param indent the string to use as indentation prefix in the
/// header.
void
report_mem_header(ostream& out,
		  diff_kind k,
		  const string& section_name,
		  const string& indent)
{
  string change;

  switch (k)
    {
    case del_kind:
      change = "deletions";
      break;
    case ins_kind:
      change = "insertions";
      break;
    case subtype_change_kind:
    case change_kind:
      change = "changes";
      break;
    }

  out << indent << "there are " << section_name << " " << change << ":\n";
}

/// Report the differences in access specifiers and static-ness for
/// class members.
///
/// @param decl1 the first class member to consider.
///
/// @param decl2 the second class member to consider.
///
/// @param out the output stream to send the report to.
///
/// @param indent the indentation string to use for the report.
///
/// @return true if something was reported, false otherwise.
bool
maybe_report_diff_for_member(const decl_base_sptr&	decl1,
			     const decl_base_sptr&	decl2,
			     const diff_context_sptr&	ctxt,
			     ostream&			out,
			     const string&		indent)

{
  bool reported = false;
  if (!is_member_decl(decl1) || !is_member_decl(decl2))
    return reported;

  string decl1_repr = decl1->get_pretty_representation(),
    decl2_repr = decl2->get_pretty_representation();

  if (get_member_is_static(decl1) != get_member_is_static(decl2))
    {
      bool lost = get_member_is_static(decl1);
      out << indent << "'" << decl1_repr << "' ";
      if (report_loc_info(decl2, *ctxt, out))
	out << " ";
      if (lost)
	out << "became non-static";
      else
	out << "became static";
      out << "\n";
      reported = true;
    }
  if ((ctxt->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (get_member_access_specifier(decl1)
	  != get_member_access_specifier(decl2)))
    {
      out << indent << "'" << decl1_repr << "' access changed from '"
	  << get_member_access_specifier(decl1)
	  << "' to '"
	  << get_member_access_specifier(decl2)
	  << "'\n";
      reported = true;
    }
  return reported;
}

/// Report the difference between two ELF symbols, if there is any.
///
/// @param symbol1 the first symbol to consider.
///
/// @param symbol2 the second symbol to consider.
///
/// @param ctxt the diff context.
///
/// @param the output stream to emit the report to.
///
/// @param indent the indentation string to use.
void
maybe_report_diff_for_symbol(const elf_symbol_sptr&	symbol1,
			     const elf_symbol_sptr&	symbol2,
			     const diff_context_sptr&	ctxt,
			     ostream&			out,
			     const string&		indent)
{
  if (!symbol1 || !symbol2 || symbol1 == symbol2)
    return;

  if (symbol1->get_size() != symbol2->get_size())
    {
      out << indent;
      show_numerical_change("size of symbol",
			    symbol1->get_size(),
			    symbol2->get_size(),
			    *ctxt, out,
			    /*show_bits_or_bytes=*/false);
      out << "\n";
    }

  if (symbol1->get_name() != symbol2->get_name())
    {
      out << indent << "symbol name changed from "
	  << symbol1->get_name()
	  << " to "
	  << symbol2->get_name()
	  << "\n";
    }

  if (symbol1->get_type() != symbol2->get_type())
    {
      out << indent << "symbol type changed from '"
	  << symbol1->get_type()
	  << "' to '"
	  << symbol2->get_type()
	  << "'\n";
    }

  if (symbol1->is_public() != symbol2->is_public())
    {
      out << indent << "symbol became ";
      if (symbol2->is_public())
	out << "exported";
      else
	out << "non-exported";
      out << "\n";
    }

  if (symbol1->is_defined() != symbol2->is_defined())
    {
      out << indent << "symbol became ";
      if (symbol2->is_defined())
	out << "defined";
      else
	out << "undefined";
      out << "\n";
    }

  if (symbol1->get_version() != symbol2->get_version())
    {
      out << indent << "symbol version changed from "
	  << symbol1->get_version().str()
	  << " to "
	  << symbol2->get_version().str()
	  << "\n";
    }

  if (symbol1->get_crc() != 0 && symbol2->get_crc() != 0
      && symbol1->get_crc() != symbol2->get_crc())
    {
      out << indent << "CRC (modversions) changed from "
	  << std::showbase << std::hex
	  << symbol1->get_crc() << " to " << symbol2->get_crc()
	  << std::noshowbase << std::dec
	  << "\n";
    }
}

/// For a given symbol, emit a string made of its name and version.
/// The string also contains the list of symbols that alias this one.
///
/// @param out the output string to emit the resulting string to.
///
/// @param indent the indentation string to use before emitting the
/// resulting string.
///
/// @param symbol the symbol to emit the representation string for.
///
/// @param sym_map the symbol map to consider to look for aliases of
/// @p symbol.
void
show_linkage_name_and_aliases(ostream& out,
			      const string& indent,
			      const elf_symbol& symbol,
			      const string_elf_symbols_map_type& sym_map)
{
  out << indent << symbol.get_id_string();
  string aliases =
    symbol.get_aliases_id_string(sym_map,
				 /*include_symbol_itself=*/false);
  if (!aliases.empty())
    out << ", aliases " << aliases;
}

/// Report changes about types that are not reachable from global
/// functions and variables, in a given @param corpus_diff.
///
/// @param d the corpus_diff to consider.
///
/// @param s the statistics of the changes, after filters and
/// suppressions are reported.  This is typically what is returned by
/// corpus_diff::apply_filters_and_suppressions_before_reporting().
///
/// @param indent the indendation string (usually a string of white
/// spaces) to use for indentation during the reporting.
///
/// @param out the output stream to emit the report to.
void
maybe_report_unreachable_type_changes(const corpus_diff& d,
				      const corpus_diff::diff_stats &s,
				      const string& indent,
				      ostream& out)
{
  const diff_context_sptr& ctxt = d.context();

  if (!(ctxt->show_unreachable_types()
	&& (!d.priv_->deleted_unreachable_types_.empty()
	    || !d.priv_->added_unreachable_types_.empty()
	    || !d.priv_->changed_unreachable_types_.empty())))
    // The user either doesn't want us to show changes about
    // unreachable types or there are not such changes.
    return;

  // Handle removed unreachable types.
  if (s.net_num_removed_unreachable_types() == 1)
    out << indent
	<< "1 removed type unreachable from any public interface:\n\n";
  else if (s.net_num_removed_unreachable_types() > 1)
    out << indent
	<< s.net_num_removed_unreachable_types()
	<< " removed types unreachable from any public interface:\n\n";

  vector<type_base_sptr> sorted_removed_unreachable_types;
  sort_string_type_base_sptr_map(d.priv_->deleted_unreachable_types_,
				 sorted_removed_unreachable_types);
  bool emitted = false;
  for (vector<type_base_sptr>::const_iterator i =
	 sorted_removed_unreachable_types.begin();
       i != sorted_removed_unreachable_types.end();
       ++i)
    {
      if (d.priv_->deleted_unreachable_type_is_suppressed((*i).get()))
	continue;

      out << indent << "  [D] '" << get_pretty_representation(*i) << "'";
      report_loc_info(*i, *ctxt, out);
      out << "\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";

  // Handle changed unreachable types!
  if (s.net_num_changed_unreachable_types() == 1)
    out << indent
	<< "1 changed type unreachable from any public interface:\n\n";
  else if (s.net_num_changed_unreachable_types() > 1)
    out << indent
	<< s.net_num_changed_unreachable_types()
	<< " changed types unreachable from any public interface:\n\n";

  diff_sptrs_type sorted_diff_sptrs;
  sort_string_diff_sptr_map(d.priv_->changed_unreachable_types_,
			    sorted_diff_sptrs);
  for (diff_sptrs_type::const_iterator i = sorted_diff_sptrs.begin();
       i != sorted_diff_sptrs.end();
       ++i)
    {
      diff_sptr diff = *i;
      if (!diff || !diff->to_be_reported())
	continue;

      string repr = diff->first_subject()->get_pretty_representation();

      out << indent << "  [C] '" << repr << "' changed:\n";
      diff->report(out, indent + "    ");
      // Extra spacing.
      out << "\n";
    }
  // Changed types have extra spacing already. No new line here.

  // Handle added unreachable types.
  if (s.net_num_added_unreachable_types() == 1)
    out << indent
	<< "1 added type unreachable from any public interface:\n\n";
  else if (s.net_num_added_unreachable_types() > 1)
    out << indent
	<< s.net_num_added_unreachable_types()
	<< " added types unreachable from any public interface:\n\n";

  vector<type_base_sptr> sorted_added_unreachable_types;
  sort_string_type_base_sptr_map(d.priv_->added_unreachable_types_,
				 sorted_added_unreachable_types);
  emitted = false;
  for (vector<type_base_sptr>::const_iterator i =
	 sorted_added_unreachable_types.begin();
       i != sorted_added_unreachable_types.end();
       ++i)
    {
      if (d.priv_->added_unreachable_type_is_suppressed((*i).get()))
	continue;

      out << indent << "  [A] '" << get_pretty_representation(*i) << "'";
      report_loc_info(*i, *ctxt, out);
      out << "\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";
}

/// If a given diff node impacts some public interfaces, then report
/// about those impacted interfaces on a given output stream.
///
/// @param d the diff node to get the impacted interfaces for.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
maybe_report_interfaces_impacted_by_diff(const diff	*d,
					 ostream	&out,
					 const string	&indent)
{
  const diff_context_sptr &ctxt = d->context();
  const corpus_diff_sptr &corp_diff = ctxt->get_corpus_diff();
  if (!corp_diff)
    return;

  if (!ctxt->show_impacted_interfaces())
    return;

  const diff_maps &maps = corp_diff->get_leaf_diffs();
  artifact_sptr_set_type* impacted_artifacts =
    maps.lookup_impacted_interfaces(d);
  if (impacted_artifacts == 0)
    return;

  if (impacted_artifacts->empty())
    return;

  vector<type_or_decl_base_sptr> sorted_impacted_interfaces;
  sort_artifacts_set(*impacted_artifacts, sorted_impacted_interfaces);

  size_t num_impacted_interfaces = impacted_artifacts->size();
  if (num_impacted_interfaces == 1)
    out << indent << "one impacted interface:\n";
  else
    out << indent << num_impacted_interfaces << " impacted interfaces:\n";

  string cur_indent = indent + "  ";
  vector<type_or_decl_base_sptr>::const_iterator it;
  for (it = sorted_impacted_interfaces.begin();
       it != sorted_impacted_interfaces.end();
       ++it)
    {
      out << cur_indent << get_pretty_representation(*it) << "\n";
    }
}

/// If a given diff node impacts some public interfaces, then report
/// about those impacted interfaces on standard output.
///
/// @param d the diff node to get the impacted interfaces for.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
maybe_report_interfaces_impacted_by_diff(const diff_sptr	&d,
					 ostream		&out,
					 const string		&indent)
{return maybe_report_interfaces_impacted_by_diff(d.get(), out, indent);}

/// Tests if the diff node is to be reported.
///
/// @param p the diff to consider.
///
/// @return true iff the diff is to be reported.
bool
reporter_base::diff_to_be_reported(const diff *d) const
{return d && d->to_be_reported();}

/// Report about data members replaced by an anonymous data member
/// without changing the overall bit-layout of the class or union in
/// an ABI-meaningful way.
///
/// @param d the diff to consider.
///
/// @param out the output stream to emit the change report to.
///
/// @param indent the indentation string to use.
void
maybe_report_data_members_replaced_by_anon_dm(const class_or_union_diff &d,
					      ostream			&out,
					      const string		indent)
{
  const diff_context_sptr& ctxt = d.context();

  if ((ctxt->get_allowed_category() & HARMLESS_DATA_MEMBER_CHANGE_CATEGORY)
      && !d.data_members_replaced_by_adms().empty())
    {
      // Let's detect all the data members that are replaced by
      // members of the same anonymous data member and report them
      // in one go.
      for (changed_var_sptrs_type::const_iterator i =
	     d.ordered_data_members_replaced_by_adms().begin();
	   i != d.ordered_data_members_replaced_by_adms().end();)
	{
	  // This contains the data members replaced by the same
	  // anonymous data member.
	  vector<var_decl_sptr> dms_replaced_by_same_anon_dm;
	  dms_replaced_by_same_anon_dm.push_back(i->first);
	  // This contains the anonymous data member that replaced the
	  // data members in the variable above.
	  var_decl_sptr anonymous_data_member = i->second;
	  // Let's look forward to see if the subsequent data
	  // members were replaced by members of
	  // anonymous_data_member.
	  for (++i;
	       i != d.ordered_data_members_replaced_by_adms().end()
		 && *i->second == *anonymous_data_member;
	       ++i)
	    dms_replaced_by_same_anon_dm.push_back(i->first);

	  bool several_data_members_replaced =
	    dms_replaced_by_same_anon_dm.size() > 1;

	  out << indent << "data member";
	  if (several_data_members_replaced)
	    out << "s";

	  bool first_data_member = true;
	  for (vector<var_decl_sptr>::const_iterator it =
		 dms_replaced_by_same_anon_dm.begin();
	       it != dms_replaced_by_same_anon_dm.end();
	       ++it)
	    {
	      string name = (*it)->get_qualified_name();
	      if (!first_data_member)
		out << ",";
	      out << " '" << name << "'";
	      first_data_member = false;
	    }

	  if (several_data_members_replaced)
	    out << " were ";
	  else
	    out << " was ";

	  out << "replaced by anonymous data member:\n"
	      << indent + "  "
	      << "'"
	      << anonymous_data_member->get_pretty_representation()
	      << "'\n";
	}
    }
}

} // Namespace comparison
} // end namespace abigail

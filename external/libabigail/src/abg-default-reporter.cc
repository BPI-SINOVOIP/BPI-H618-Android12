// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2017-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli


/// @file
///
/// This is the implementation of the
/// abigail::comparison::default_reporter type.

#include "abg-comparison-priv.h"
#include "abg-reporter.h"
#include "abg-reporter-priv.h"

namespace abigail
{
namespace comparison
{

/// Test if a given instance of @ref corpus_diff carries changes whose
/// reports are not suppressed by any suppression specification.  In
/// effect, these are deemed incompatible ABI changes.
///
/// @param d the @ref corpus_diff to consider
///
/// @return true iff @p d carries subtype changes that are deemed
/// incompatible ABI changes.
bool
default_reporter::diff_has_net_changes(const corpus_diff *d) const
{
  if (!d)
    return false;

  const corpus_diff::diff_stats& stats = const_cast<corpus_diff*>(d)->
    apply_filters_and_suppressions_before_reporting();

  // Logic here should match emit_diff_stats.
  return (d->architecture_changed()
	  || d->soname_changed()
	  || stats.net_num_func_removed()
	  || stats.net_num_func_changed()
	  || stats.net_num_func_added()
	  || stats.net_num_vars_removed()
	  || stats.net_num_vars_changed()
	  || stats.net_num_vars_added()
	  || stats.net_num_removed_unreachable_types()
	  || stats.net_num_changed_unreachable_types()
	  || stats.net_num_added_unreachable_types()
	  || stats.net_num_removed_func_syms()
	  || stats.net_num_added_func_syms()
	  || stats.net_num_removed_var_syms()
	  || stats.net_num_added_var_syms());
}

/// Ouputs a report of the differences between of the two type_decl
/// involved in the @ref type_decl_diff.
///
/// @param d the @ref type_decl_diff to consider.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the string to use for indentatino indent.
void
default_reporter::report(const type_decl_diff& d,
			 ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  type_decl_sptr f = d.first_type_decl(), s = d.second_type_decl();

  string name = f->get_pretty_representation();

  report_name_size_and_alignment_changes(f, s, d.context(),
					 out, indent);

  if (f->get_visibility() != s->get_visibility())
    {
      out << indent
	  << "visibility changed from '"
	  << f->get_visibility() << "' to '" << s->get_visibility()
	  << "\n";
    }

  if (f->get_linkage_name() != s->get_linkage_name())
    {
      out << indent
	  << "mangled name changed from '"
	  << f->get_linkage_name() << "' to "
	  << s->get_linkage_name()
	  << "\n";
    }
}

/// Report the differences between the two enums.
///
/// @param d the enum diff to consider.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
default_reporter::report(const enum_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER3(d.first_subject(),
						   d.second_subject(),
						   "enum type");

  string name = d.first_enum()->get_pretty_representation();

  enum_type_decl_sptr first = d.first_enum(), second = d.second_enum();

  const diff_context_sptr& ctxt = d.context();

  // Report enum decl-only <-> definition changes.
  if (ctxt->get_allowed_category() & TYPE_DECL_ONLY_DEF_CHANGE_CATEGORY)
    if (filtering::has_enum_decl_only_def_change(first, second))
      {
	string was =
	  first->get_is_declaration_only()
	  ? " was a declaration-only enum type"
	  : " was a defined enum type";

	string is_now =
	  second->get_is_declaration_only()
	  ? " and is now a declaration-only enum type"
	  : " and is now a defined enum type";

	out << indent << "enum type " << name << was << is_now << "\n";
	return;
      }

  report_name_size_and_alignment_changes(first, second, ctxt,
					 out, indent);
  maybe_report_diff_for_member(first, second, ctxt, out, indent);

  //underlying type
  d.underlying_type_diff()->report(out, indent);

  //report deletions/insertions/change of enumerators
  unsigned numdels = d.deleted_enumerators().size();
  unsigned numins = d.inserted_enumerators().size();
  unsigned numchanges = d.changed_enumerators().size();

  if (numdels)
    {
      report_mem_header(out, numdels, 0, del_kind, "enumerator", indent);
      enum_type_decl::enumerators sorted_deleted_enumerators;
      sort_enumerators(d.deleted_enumerators(), sorted_deleted_enumerators);
      for (enum_type_decl::enumerators::const_iterator i =
	     sorted_deleted_enumerators.begin();
	   i != sorted_deleted_enumerators.end();
	   ++i)
	{
	  out << indent
	      << "  '"
	      << i->get_qualified_name()
	      << "' value '"
	      << i->get_value()
	      << "'";
	  out << "\n";
	}
    }
  if (numins)
    {
      report_mem_header(out, numins, 0, ins_kind, "enumerator", indent);
      enum_type_decl::enumerators sorted_inserted_enumerators;
      sort_enumerators(d.inserted_enumerators(), sorted_inserted_enumerators);
      for (enum_type_decl::enumerators::const_iterator i =
	     sorted_inserted_enumerators.begin();
	   i != sorted_inserted_enumerators.end();
	   ++i)
	{
	  out << indent
	      << "  '"
	      << i->get_qualified_name()
	      << "' value '"
	      << i->get_value()
	      << "'";
	  out << "\n";
	}
    }
  if (numchanges)
    {
      report_mem_header(out, numchanges, 0, change_kind, "enumerator", indent);
      changed_enumerators_type sorted_changed_enumerators;
      sort_changed_enumerators(d.changed_enumerators(),
			       sorted_changed_enumerators);
      for (changed_enumerators_type::const_iterator i =
	     sorted_changed_enumerators.begin();
	   i != sorted_changed_enumerators.end();
	   ++i)
	{
	  out << indent
	      << "  '"
	      << i->first.get_qualified_name()
	      << "' from value '"
	      << i->first.get_value() << "' to '"
	      << i->second.get_value() << "'";
	  report_loc_info(second, *ctxt, out);
	  out << "\n";
	}
    }

  if (ctxt->show_leaf_changes_only())
    maybe_report_interfaces_impacted_by_diff(&d, out, indent);

  d.reported_once(true);
}

/// For a @ref typedef_diff node, report the local changes to the
/// typedef rather the changes to its underlying type.
///
/// Note that changes to the underlying type are also considered
/// local.
///
/// @param d the @ref typedef_diff node to consider.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
default_reporter::report_non_type_typedef_changes(const typedef_diff &d,
						  ostream& out,
						  const string& indent) const
{
  if (!d.to_be_reported())
    return;

  typedef_decl_sptr f = d.first_typedef_decl(), s = d.second_typedef_decl();

  maybe_report_diff_for_member(f, s, d.context(), out, indent);

  if ((filtering::has_harmless_name_change(f, s)
       && ((d.context()->get_allowed_category()
	    & HARMLESS_DECL_NAME_CHANGE_CATEGORY)
	   || d.context()->show_leaf_changes_only()))
      || f->get_qualified_name() != s->get_qualified_name())
    {
      out << indent << "typedef name changed from "
	  << f->get_qualified_name()
	  << " to "
	  << s->get_qualified_name();
      report_loc_info(s, *d.context(), out);
      out << "\n";
    }
}

/// Reports the difference between the two subjects of the diff in a
/// serialized form.
///
/// @param d @ref typedef_diff node to consider.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the indentation string to use.
void
default_reporter::report(const typedef_diff& d,
			 ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  typedef_decl_sptr f = d.first_typedef_decl(), s = d.second_typedef_decl();

  report_non_type_typedef_changes(d, out, indent);

  diff_sptr dif = d.underlying_type_diff();
  if (dif && dif->has_changes())
    {
      if (dif->to_be_reported())
	{
	  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif,
							    "underlying type");
	  out << indent
	      << "underlying type '"
	      << dif->first_subject()->get_pretty_representation() << "'";
	  report_loc_info(dif->first_subject(), *d.context(), out);
	  out << " changed:\n";
	  dif->report(out, indent + "  ");
	}
      else
	{
	  // The typedef change is to be reported, so we'll report its
	  // underlying type change too (even if its redundant),
	  // unless it's suppressed.  It makes sense in this
	  // particular case to emit the underlying type change
	  // because of the informative value underneath.  We don't
	  // want to just know about the local changes of the typedef,
	  // but also about the changes on the underlying type.
	  diff_category c = dif->get_category();
	  if (!(c & (SUPPRESSED_CATEGORY | PRIVATE_TYPE_CATEGORY)))
	    {
	      out << indent
		  << "underlying type '"
		  << dif->first_subject()->get_pretty_representation() << "'";
	      report_loc_info(dif->first_subject(), *d.context(), out);
	      out << " changed:\n";
	      if (c & REDUNDANT_CATEGORY)
		dif->set_category(c & ~REDUNDANT_CATEGORY);
	      dif->report(out, indent + "  ");
	      if (c & REDUNDANT_CATEGORY)
		dif->set_category(c | REDUNDANT_CATEGORY);
	    }
	}
    }

  d.reported_once(true);
}

/// For a @ref qualified_type_diff node, report the changes that are
/// local.
///
/// @param d the @ref qualified_type_diff node to consider.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the white string to use for indentation.
///
/// @return true iff a local change has been emitted.  In this case,
/// the local change is a name change.
bool
default_reporter::report_local_qualified_type_changes(const qualified_type_diff& d,
						      ostream& out,
						      const string& indent) const
{
  if (!d.to_be_reported())
    return false;

  string fname = d.first_qualified_type()->get_pretty_representation(),
    sname = d.second_qualified_type()->get_pretty_representation();

  if (fname != sname)
    {
      out << indent << "'" << fname << "' changed to '" << sname << "'\n";
      return true;
    }
  return false;
}

/// Report a @ref qualified_type_diff in a serialized form.
///
/// @param d the @ref qualified_type_diff node to consider.
///
/// @param out the output stream to serialize to.
///
/// @param indent the string to use to indent the lines of the report.
void
default_reporter::report(const qualified_type_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(d.first_qualified_type(),
						   d.second_qualified_type());

  if (report_local_qualified_type_changes(d, out, indent))
    // The local change was emitted and it's a name change.  If the
    // type name changed, the it means the type changed altogether.
    // It makes a little sense to detail the changes in extenso here.
    return;

  diff_sptr dif = d.leaf_underlying_type_diff();
  ABG_ASSERT(dif);
  ABG_ASSERT(dif->to_be_reported());
  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif,
						    "unqualified "
						    "underlying type");

  string fltname = dif->first_subject()->get_pretty_representation();
  out << indent << "in unqualified underlying type '" << fltname << "'";
  report_loc_info(dif->second_subject(), *d.context(), out);
  out << ":\n";
  dif->report(out, indent + "  ");
}

/// Report the @ref pointer_diff in a serialized form.
///
/// @param d the @ref pointer_diff node to consider.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
default_reporter::report(const pointer_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  if (diff_sptr dif = d.underlying_type_diff())
    {
      RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif, "pointed to type");
      string repr = dif->first_subject()
	? dif->first_subject()->get_pretty_representation()
	: string("void");

      out << indent
	  << "in pointed to type '" << repr << "'";
      report_loc_info(dif->second_subject(), *d.context(), out);
      out << ":\n";
      dif->report(out, indent + "  ");
    }
}

/// For a @reference_diff node, report the local changes carried by
/// the diff node.
///
/// @param d the @reference_diff node to consider.
///
/// @param out the output stream to report to.
///
/// @param indent the white space indentation to use in the report.
void
default_reporter::report_local_reference_type_changes(const reference_diff& d,
						      ostream& out,
						      const string& indent) const
{
  if (!d.to_be_reported())
    return;

  reference_type_def_sptr f = d.first_reference(), s = d.second_reference();
  ABG_ASSERT(f && s);

  string f_repr = f->get_pretty_representation(),
    s_repr = s->get_pretty_representation();

  if (f->is_lvalue() != s->is_lvalue())
    {
      out << indent;
      if (f->is_lvalue())
	out << "lvalue reference type '" << f_repr
	    << " became an rvalue reference type: '"
	    << s_repr
	    << "'\n";
      else
	out << "rvalue reference type '" << f_repr
	    << " became an lvalue reference type: '"
	    << s_repr
	    << "'\n";
    }
  else if (!types_have_similar_structure(f->get_pointed_to_type().get(),
					 s->get_pointed_to_type().get()))
    out << indent
	<< "reference type changed from: '"
	<< f_repr << "' to: '" << s_repr << "'\n";
}

/// Report a @ref reference_diff in a serialized form.
///
/// @param d the @ref reference_diff node to consider.
///
/// @param out the output stream to serialize the dif to.
///
/// @param indent the string to use for indenting the report.
void
default_reporter::report(const reference_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  enum change_kind k = ir::NO_CHANGE_KIND;
  equals(*d.first_reference(), *d.second_reference(), &k);

  if ((k & ALL_LOCAL_CHANGES_MASK) && !(k & SUBTYPE_CHANGE_KIND))
    report_local_reference_type_changes(d, out, indent);

  if (k & SUBTYPE_CHANGE_KIND)
    if (diff_sptr dif = d.underlying_type_diff())
      {
	RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif,
							  "referenced type");

	out << indent
	    << "in referenced type '"
	    << dif->first_subject()->get_pretty_representation() << "'";
	report_loc_info(dif->second_subject(), *d.context(), out);
	out << ":\n";
	dif->report(out, indent + "  ");
      }
}

/// Emit a textual report about the a @ref fn_parm_diff instance.
///
/// @param d the @ref fn_parm_diff to consider.
///
/// @param out the output stream to emit the textual report to.
///
/// @param indent the indentation string to use in the report.
void
default_reporter::report(const fn_parm_diff& d, ostream& out,
			 const string& indent) const
{
  function_decl::parameter_sptr f = d.first_parameter(),
    s = d.second_parameter();

  // either the parameter has a sub-type change (if its type name
  // hasn't changed) or it has a "grey" change (that is, a change that
  // changes his type name w/o changing the signature of the
  // function).
  bool has_sub_type_change =
    type_has_sub_type_changes(d.first_parameter()->get_type(),
			      d.second_parameter()->get_type());

  if (d.to_be_reported())
    {
      diff_sptr type_diff = d.type_diff();
      ABG_ASSERT(type_diff->has_changes());
      diff_category saved_category = type_diff->get_category();
      // Parameter type changes are never redundants.
      type_diff->set_category(saved_category & ~REDUNDANT_CATEGORY);
      out << indent;
      if (f->get_is_artificial())
	out << "implicit ";
      out << "parameter " << f->get_index();
      report_loc_info(f, *d.context(), out);
      out << " of type '"
	  << f->get_type_pretty_representation();

      if (has_sub_type_change)
	out << "' has sub-type changes:\n";
      else
	out << "' changed:\n";

      type_diff->report(out, indent + "  ");
      type_diff->set_category(saved_category);
    }
}

/// For a @ref function_type_diff node, report the local changes
/// carried by the diff node.
///
/// @param d the @ref function_type_diff node to consider.
///
/// @param out the output stream to report to.
///
/// @param indent the white space indentation string to use.
void
default_reporter::report_local_function_type_changes(const function_type_diff& d,
						     ostream& out,
						     const string& indent) const

{
  if (!d.to_be_reported())
    return;

  function_type_sptr fft = d.first_function_type();
  function_type_sptr sft = d.second_function_type();

  diff_context_sptr ctxt = d.context();

  // Report about the size of the function address
  if (fft->get_size_in_bits() != sft->get_size_in_bits())
    {
      out << indent << "address size of function changed from "
	  << fft->get_size_in_bits()
	  << " bits to "
	  << sft->get_size_in_bits()
	  << " bits\n";
    }

  // Report about the alignment of the function address
  if (fft->get_alignment_in_bits()
      != sft->get_alignment_in_bits())
    {
      out << indent << "address alignment of function changed from "
	  << fft->get_alignment_in_bits()
	  << " bits to "
	  << sft->get_alignment_in_bits()
	  << " bits\n";
    }

  // Hmmh, the above was quick.  Now report about function parameters;
  // this shouldn't be as straightforward.

  // Report about the parameters that got removed.
  for (vector<function_decl::parameter_sptr>::const_iterator i =
	 d.priv_->sorted_deleted_parms_.begin();
       i != d.priv_->sorted_deleted_parms_.end();
       ++i)
    {
      out << indent << "parameter " << (*i)->get_index()
	  << " of type '" << (*i)->get_type_pretty_representation()
	  << "' was removed\n";
    }

  // Report about the parameters that got added
  for (vector<function_decl::parameter_sptr>::const_iterator i =
	 d.priv_->sorted_added_parms_.begin();
       i != d.priv_->sorted_added_parms_.end();
       ++i)
    {
      out << indent << "parameter " << (*i)->get_index()
	  << " of type '" << (*i)->get_type_pretty_representation()
	  << "' was added\n";
    }
}

/// Build and emit a textual report about a @ref function_type_diff.
///
/// @param d the @ref function_type_diff to consider.
///
/// @param out the output stream.
///
/// @param indent the indentation string to use.
void
default_reporter::report(const function_type_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  function_type_sptr fft = d.first_function_type();
  function_type_sptr sft = d.second_function_type();

  diff_context_sptr ctxt = d.context();
  corpus_sptr fc = ctxt->get_first_corpus();
  corpus_sptr sc = ctxt->get_second_corpus();

  // Report about return type differences.
  if (d.priv_->return_type_diff_
      && d.priv_->return_type_diff_->to_be_reported())
    {
      out << indent << "return type changed:\n";
      d.priv_->return_type_diff_->report(out, indent + "  ");
    }

  // Report about the parameter types that have changed sub-types.
  for (vector<fn_parm_diff_sptr>::const_iterator i =
	 d.priv_->sorted_subtype_changed_parms_.begin();
       i != d.priv_->sorted_subtype_changed_parms_.end();
       ++i)
    {
      diff_sptr dif = *i;
      if (dif && dif->to_be_reported())
	dif->report(out, indent);
    }

  report_local_function_type_changes(d, out, indent);

}

/// Report a @ref array_diff in a serialized form.
///
/// @param d the @ref array_diff to consider.
///
/// @param out the output stream to serialize the dif to.
///
/// @param indent the string to use for indenting the report.
void
default_reporter::report(const array_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  string name = d.first_array()->get_pretty_representation();
  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER3(d.first_array(),
						    d.second_array(),
						    "array type");

  diff_sptr dif = d.element_type_diff();
  if (dif->to_be_reported())
    {
      string fn = ir::get_pretty_representation(is_type(dif->first_subject()));
      // report array element type changes
      out << indent << "array element type '"
	  << fn << "' changed:\n";
      dif->report(out, indent + "  ");
    }

  report_name_size_and_alignment_changes(d.first_array(),
					 d.second_array(),
					 d.context(),
					 out, indent);
}

/// Generates a report for an intance of @ref base_diff.
///
/// @param d the @ref base_diff to consider.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
default_reporter::report(const base_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  class_decl::base_spec_sptr f = d.first_base(), s = d.second_base();
  string repr = f->get_base_class()->get_pretty_representation();
  bool emitted = false;

  if (f->get_is_static() != s->get_is_static())
    {
      if (f->get_is_static())
	out << indent << "is no more static";
      else
	out << indent << "now becomes static";
      emitted = true;
    }

  if ((d.context()->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (f->get_access_specifier() != s->get_access_specifier()))
    {
      if (emitted)
	out << ", ";

      out << "has access changed from '"
	  << f->get_access_specifier()
	  << "' to '"
	  << s->get_access_specifier()
	  << "'";

      emitted = true;
    }

  if (class_diff_sptr dif = d.get_underlying_class_diff())
    {
      if (dif->to_be_reported())
	{
	  if (emitted)
	    out << "\n";
	  dif->report(out, indent);
	}
    }
}

/// Report the changes carried by a @ref scope_diff.
///
/// @param d the @ref scope_diff to consider.
///
/// @param out the out stream to report the changes to.
///
/// @param indent the string to use for indentation.
void
default_reporter::report(const scope_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  // Report changed types.
  unsigned num_changed_types = d.changed_types().size();
  if (num_changed_types == 0)
    ;
  else if (num_changed_types == 1)
    out << indent << "1 changed type:\n";
  else
    out << indent << num_changed_types << " changed types:\n";

  for (diff_sptrs_type::const_iterator dif = d.changed_types().begin();
       dif != d.changed_types().end();
       ++dif)
    {
      if (!*dif)
	continue;

      out << indent << "  '"
	  << (*dif)->first_subject()->get_pretty_representation()
	  << "' changed:\n";
      (*dif)->report(out, indent + "    ");
    }

  // Report changed decls
  unsigned num_changed_decls = d.changed_decls().size();
  if (num_changed_decls == 0)
    ;
  else if (num_changed_decls == 1)
    out << indent << "1 changed declaration:\n";
  else
    out << indent << num_changed_decls << " changed declarations:\n";

  for (diff_sptrs_type::const_iterator dif= d.changed_decls().begin();
       dif != d.changed_decls().end ();
       ++dif)
    {
      if (!*dif)
	continue;

      out << indent << "  '"
	  << (*dif)->first_subject()->get_pretty_representation()
	  << "' was changed to '"
	  << (*dif)->second_subject()->get_pretty_representation() << "'";
      report_loc_info((*dif)->second_subject(), *d.context(), out);
      out << ":\n";

      (*dif)->report(out, indent + "    ");
    }

  // Report removed types/decls
  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->deleted_types_.begin();
       i != d.priv_->deleted_types_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";

  if (d.priv_->deleted_types_.size())
    out << "\n";

  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->deleted_decls_.begin();
       i != d.priv_->deleted_decls_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";

  if (d.priv_->deleted_decls_.size())
    out << "\n";

  // Report added types/decls
  bool emitted = false;
  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->inserted_types_.begin();
       i != d.priv_->inserted_types_.end();
       ++i)
    {
      // Do not report about type_decl as these are usually built-in
      // types.
      if (dynamic_pointer_cast<type_decl>(i->second))
	continue;
      out << indent
	  << "  '"
	  << i->second->get_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }

  if (emitted)
    out << "\n";

  emitted = false;
  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->inserted_decls_.begin();
       i != d.priv_->inserted_decls_.end();
       ++i)
    {
      // Do not report about type_decl as these are usually built-in
      // types.
      if (dynamic_pointer_cast<type_decl>(i->second))
	continue;
      out << indent
	  << "  '"
	  << i->second->get_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }

  if (emitted)
    out << "\n";
}

/// Report the changes carried by a @ref class_or_union_diff node in a
/// textual format.
///
/// @param d the @ref class_or_union_diff node to consider.
///
/// @param out the output stream to write the textual report to.
///
/// @param indent the number of white space to use as indentation.
void
default_reporter::report(const class_or_union_diff& d,
			 ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  class_or_union_sptr first = d.first_class_or_union(),
    second = d.second_class_or_union();

  const diff_context_sptr& ctxt = d.context();

  // Report class decl-only <-> definition change.
  if (ctxt->get_allowed_category() & TYPE_DECL_ONLY_DEF_CHANGE_CATEGORY)
    if (filtering::has_class_decl_only_def_change(first, second))
      {
	string was =
	  first->get_is_declaration_only()
	  ? " was a declaration-only type"
	  : " was a defined type";

	string is_now =
	  second->get_is_declaration_only()
	  ? " and is now a declaration-only type"
	  : " and is now a defined type";

	out << indent << "type " << first->get_pretty_representation()
	    << was << is_now << "\n";
	return;
      }

  // member functions
  if (d.member_fns_changes())
    {
      // report deletions
      int numdels = d.get_priv()->deleted_member_functions_.size();
      size_t num_filtered =
	d.get_priv()->count_filtered_deleted_mem_fns(ctxt);
      if (numdels)
	report_mem_header(out, numdels, num_filtered, del_kind,
			  "member function", indent);
      for (class_or_union::member_functions::const_iterator i =
	     d.get_priv()->sorted_deleted_member_functions_.begin();
	   i != d.get_priv()->sorted_deleted_member_functions_.end();
	   ++i)
	{
	  if (!(ctxt->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !get_member_function_is_virtual(*i))
	    continue;

	  method_decl_sptr mem_fun = *i;
	  out << indent << "  ";
	  represent(*ctxt, mem_fun, out);
	}

      // report insertions;
      int numins = d.get_priv()->inserted_member_functions_.size();
      num_filtered = d.get_priv()->count_filtered_inserted_mem_fns(ctxt);
      if (numins)
	report_mem_header(out, numins, num_filtered, ins_kind,
			  "member function", indent);
      for (class_or_union::member_functions::const_iterator i =
	     d.get_priv()->sorted_inserted_member_functions_.begin();
	   i != d.get_priv()->sorted_inserted_member_functions_.end();
	   ++i)
	{
	  if (!(ctxt->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !get_member_function_is_virtual(*i))
	    continue;

	  method_decl_sptr mem_fun = *i;
	  out << indent << "  ";
	  represent(*ctxt, mem_fun, out);
	}

      // report member function with sub-types changes
      int numchanges = d.get_priv()->sorted_changed_member_functions_.size();
      num_filtered = d.get_priv()->count_filtered_changed_mem_fns(ctxt);
      if (numchanges)
	report_mem_header(out, numchanges, num_filtered, change_kind,
			  "member function", indent);
      for (function_decl_diff_sptrs_type::const_iterator i =
	     d.get_priv()->sorted_changed_member_functions_.begin();
	   i != d.get_priv()->sorted_changed_member_functions_.end();
	   ++i)
	{
	  if (!(ctxt->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !(get_member_function_is_virtual
		   ((*i)->first_function_decl()))
	      && !(get_member_function_is_virtual
		   ((*i)->second_function_decl())))
	    continue;

	  diff_sptr diff = *i;
	  if (!diff || !diff->to_be_reported())
	    continue;

	  string repr =
	    (*i)->first_function_decl()->get_pretty_representation();
	  out << indent << "  '" << repr << "' has some sub-type changes:\n";
	  diff->report(out, indent + "    ");
	}
    }

  // data members
  if (d.data_members_changes())
    {
      // report deletions
      int numdels = d.class_or_union_diff::get_priv()->
	get_deleted_non_static_data_members_number();
      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "data member", indent);
	  vector<decl_base_sptr> sorted_dms;
	  sort_data_members
	    (d.class_or_union_diff::get_priv()->deleted_data_members_,
	     sorted_dms);
	  for (vector<decl_base_sptr>::const_iterator i = sorted_dms.begin();
	       i != sorted_dms.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(*i);
	      ABG_ASSERT(data_mem);
	      if (get_member_is_static(data_mem))
		continue;
	      represent_data_member(data_mem, ctxt, out, indent + "  ");
	    }
	}

      //report insertions
      int numins =
	d.class_or_union_diff::get_priv()->inserted_data_members_.size();
      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "data member", indent);
	  vector<decl_base_sptr> sorted_dms;
	  sort_data_members
	    (d.class_or_union_diff::get_priv()->inserted_data_members_,
	     sorted_dms);
	  for (vector<decl_base_sptr>::const_iterator i = sorted_dms.begin();
	       i != sorted_dms.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(*i);
	      ABG_ASSERT(data_mem);
	      represent_data_member(data_mem, ctxt, out, indent + "  ");
	    }
	}

      // report change
      size_t num_changes =
	(d.sorted_subtype_changed_data_members().size()
	 + d.sorted_changed_data_members().size());

      size_t num_changes_filtered =
	(d.count_filtered_subtype_changed_data_members()
	 + d.count_filtered_changed_data_members());

      if (num_changes)
	{
	  report_mem_header(out, num_changes, num_changes_filtered,
			    change_kind, "data member", indent);

	  for (var_diff_sptrs_type::const_iterator it =
		 d.sorted_changed_data_members().begin();
	       it != d.sorted_changed_data_members().end();
	       ++it)
	    if ((*it)->to_be_reported())
	      represent(*it, ctxt, out, indent + "  ");

	  for (var_diff_sptrs_type::const_iterator it =
		 d.sorted_subtype_changed_data_members().begin();
	       it != d.sorted_subtype_changed_data_members().end();
	       ++it)
	    if ((*it)->to_be_reported())
	      represent(*it, ctxt, out, indent + "  ");
	}

      // Report about data members replaced by an anonymous union data
      // member.
      maybe_report_data_members_replaced_by_anon_dm(d, out, indent);
    }

  // member types
  if (const edit_script& e = d.member_types_changes())
    {
      int numchanges =
	d.class_or_union_diff::get_priv()->sorted_changed_member_types_.size();
      int numdels =
	d.class_or_union_diff::get_priv()->deleted_member_types_.size();

      // report deletions
      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "member type", indent);

	  for (string_decl_base_sptr_map::const_iterator i =
		 d.class_or_union_diff::get_priv()->deleted_member_types_.begin();
	       i != d.class_or_union_diff::get_priv()->deleted_member_types_.end();
	       ++i)
	    {
	      decl_base_sptr mem_type = i->second;
	      out << indent << "  '"
		  << mem_type->get_pretty_representation()
		  << "'\n";
	    }
	  out << "\n";
	}
      // report changes
      if (numchanges)
	{
	  report_mem_header(out, numchanges, 0, change_kind,
			    "member type", indent);

	  for (diff_sptrs_type::const_iterator it =
		 d.class_or_union_diff::get_priv()->sorted_changed_member_types_.begin();
	       it != d.class_or_union_diff::get_priv()->sorted_changed_member_types_.end();
	       ++it)
	    {
	      if (!(*it)->to_be_reported())
		continue;

	      type_or_decl_base_sptr o = (*it)->first_subject();
	      type_or_decl_base_sptr n = (*it)->second_subject();
	      out << indent << "  '"
		  << o->get_pretty_representation()
		  << "' changed ";
	      report_loc_info(n, *ctxt, out);
	      out << ":\n";
	      (*it)->report(out, indent + "    ");
	    }
	  out << "\n";
	}

      // report insertions
      int numins = e.num_insertions();
      ABG_ASSERT(numchanges <= numins);
      numins -= numchanges;

      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "member type", indent);

	  for (vector<insertion>::const_iterator i = e.insertions().begin();
	       i != e.insertions().end();
	       ++i)
	    {
	      type_base_sptr mem_type;
	      for (vector<unsigned>::const_iterator j =
		     i->inserted_indexes().begin();
		   j != i->inserted_indexes().end();
		   ++j)
		{
		  mem_type = second->get_member_types()[*j];
		  if (!d.class_or_union_diff::get_priv()->
		      member_type_has_changed(get_type_declaration(mem_type)))
		    {
		      out << indent << "  '"
			  << get_type_declaration(mem_type)->
			get_pretty_representation()
			  << "'\n";
		    }
		}
	    }
	  out << "\n";
	}
    }

  // member function templates
  if (const edit_script& e = d.member_fn_tmpls_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, 0, del_kind,
			  "member function template", indent);
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  member_function_template_sptr mem_fn_tmpl =
	    first->get_member_function_templates()[i->index()];
	  out << indent << "  '"
	      << mem_fn_tmpl->as_function_tdecl()->get_pretty_representation()
	      << "'\n";
	}

      // report insertions
      int numins = e.num_insertions();
      if (numins)
	report_mem_header(out, numins, 0, ins_kind,
			  "member function template", indent);
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  member_function_template_sptr mem_fn_tmpl;
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      mem_fn_tmpl = second->get_member_function_templates()[*j];
	      out << indent << "  '"
		  << mem_fn_tmpl->as_function_tdecl()->
		get_pretty_representation()
		  << "'\n";
	    }
	}
    }

  // member class templates.
  if (const edit_script& e = d.member_class_tmpls_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, 0, del_kind,
			  "member class template", indent);
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  member_class_template_sptr mem_cls_tmpl =
	    first->get_member_class_templates()[i->index()];
	  out << indent << "  '"
	      << mem_cls_tmpl->as_class_tdecl()->get_pretty_representation()
	      << "'\n";
	}

      // report insertions
      int numins = e.num_insertions();
      if (numins)
	report_mem_header(out, numins, 0, ins_kind,
			  "member class template", indent);
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  member_class_template_sptr mem_cls_tmpl;
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      mem_cls_tmpl = second->get_member_class_templates()[*j];
	      out << indent << "  '"
		  << mem_cls_tmpl->as_class_tdecl()
		->get_pretty_representation()
		  << "'\n";
	    }
	}
    }
}

/// Produce a basic report about the changes carried by a @ref
/// class_diff node.
///
/// @param d the @ref class_diff node to consider.
///
/// @param out the output stream to report the changes to.
///
/// @param indent the string to use as an indentation prefix in the
/// report.
void
default_reporter::report(const class_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  string name = d.first_subject()->get_pretty_representation();

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(d.first_subject(),
						   d.second_subject());

  d.currently_reporting(true);

  // Now report the changes about the differents parts of the type.
  class_decl_sptr first = d.first_class_decl(),
    second = d.second_class_decl();

  report_name_size_and_alignment_changes(first, second, d.context(),
					 out, indent);

  const diff_context_sptr& ctxt = d.context();
  maybe_report_diff_for_member(first, second, ctxt, out, indent);

  // bases classes
  if (d.base_changes())
    {
      // Report deletions.
      int numdels = d.get_priv()->deleted_bases_.size();
      size_t numchanges = d.get_priv()->sorted_changed_bases_.size();

      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "base class", indent);

	  for (class_decl::base_specs::const_iterator i
		 = d.get_priv()->sorted_deleted_bases_.begin();
	       i != d.get_priv()->sorted_deleted_bases_.end();
	       ++i)
	    {
	      if (i != d.get_priv()->sorted_deleted_bases_.begin())
		out << "\n";

	      class_decl::base_spec_sptr base = *i;

	      if (d.get_priv()->base_has_changed(base))
		continue;
	      out << indent << "  "
		  << base->get_base_class()->get_pretty_representation();
	      report_loc_info(base->get_base_class(), *d.context(), out);
	    }
	  out << "\n";
	}

      // Report changes.
      size_t num_filtered = d.get_priv()->count_filtered_bases();
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered, change_kind,
			    "base class", indent);
	  for (base_diff_sptrs_type::const_iterator it =
		 d.get_priv()->sorted_changed_bases_.begin();
	       it != d.get_priv()->sorted_changed_bases_.end();
	       ++it)
	    {
	      base_diff_sptr diff = *it;
	      if (!diff || !diff->to_be_reported())
		continue;

	      class_decl::base_spec_sptr o = diff->first_base();
	      out << indent << "  '"
		  << o->get_base_class()->get_pretty_representation() << "'";
	      report_loc_info(o->get_base_class(), *d.context(), out);
	      out << " changed:\n";
	      diff->report(out, indent + "    ");
	    }
	}

      //Report insertions.
      int numins = d.get_priv()->inserted_bases_.size();
      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "base class", indent);

	  for (class_decl::base_specs::const_iterator i =
		 d.get_priv()->sorted_inserted_bases_.begin();
	       i != d.get_priv()->sorted_inserted_bases_.end();
	       ++i)
	    {
	      class_decl_sptr b = (*i)->get_base_class();
	      out << indent << "  " << b->get_pretty_representation();
	      report_loc_info(b, *ctxt, out);
	      out << "\n";
	    }
	}
    }

  d.class_or_union_diff::report(out, indent);

  d.currently_reporting(false);

  d.reported_once(true);
}

/// Produce a basic report about the changes carried by a @ref
/// union_diff node.
///
/// @param d the @ref union_diff node to consider.
///
/// @param out the output stream to report the changes to.
///
/// @param indent the string to use as an indentation prefix in the
/// report.
void
default_reporter::report(const union_diff& d, ostream& out,
			 const string& indent) const
{
  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(d.first_subject(),
						   d.second_subject());

  d.currently_reporting(true);

  // Now report the changes about the differents parts of the type.
  union_decl_sptr first = d.first_union_decl(), second = d.second_union_decl();

  report_name_size_and_alignment_changes(first, second, d.context(),
					 out, indent);

  maybe_report_diff_for_member(first, second,d. context(), out, indent);

  d.class_or_union_diff::report(out, indent);

  if (d.context()->get_allowed_category() & HARMLESS_UNION_CHANGE_CATEGORY
      && filtering::union_diff_has_harmless_changes(&d))
    {
      // The user wants to see harmless changes and the union diff we
      // are looking at does carry some harmless changes.  Let's show
      // the "before" and "after" carried by the diff node.
      out << indent << "type changed from:\n"
	  << get_class_or_union_flat_representation(first, indent + "  ",
						    /*one_line=*/true,
						    /*internal=*/false,
						    /*qualified_names=*/false)
	  << "\n"
	  << indent << "to:\n"
	  << get_class_or_union_flat_representation(second, indent + "  ",
						    /*one_line=*/true,
						    /*internal=*/false,
						    /*qualified_names=*/false)
	  << "\n";
    }

  d.currently_reporting(false);

  d.reported_once(true);
}

/// Emit a report about the changes carried by a @ref distinct_diff
/// node.
///
/// @param d the @ref distinct_diff node to consider.
///
/// @param out the output stream to send the diff report to.
///
/// @param indent the indentation string to use in the report.
void
default_reporter::report(const distinct_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  type_or_decl_base_sptr f = d.first(), s = d.second();

  string f_repr = f ? f->get_pretty_representation() : "'void'";
  string s_repr = s ? s->get_pretty_representation() : "'void'";

  diff_sptr diff = d.compatible_child_diff();

  string compatible = diff ? " to compatible type '": " to '";

  out << indent << "entity changed from '" << f_repr << "'"
      << compatible << s_repr << "'";
  report_loc_info(s, *d.context(), out);
  out << "\n";

  type_base_sptr fs = strip_typedef(is_type(f)),
    ss = strip_typedef(is_type(s));

  if (diff)
    diff->report(out, indent + "  ");
  else
    report_size_and_alignment_changes(f, s, d.context(), out, indent);
}

/// Serialize a report of the changes encapsulated in the current
/// instance of @ref function_decl_diff over to an output stream.
///
/// @param d the @ref function_decl_diff node to consider.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the string to use an an indentation prefix.
void
default_reporter::report(const function_decl_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  maybe_report_diff_for_member(d.first_function_decl(),
			       d.second_function_decl(),
			       d.context(), out, indent);

  function_decl_sptr ff = d.first_function_decl();
  function_decl_sptr sf = d.second_function_decl();

  diff_context_sptr ctxt = d.context();
  corpus_sptr fc = ctxt->get_first_corpus();
  corpus_sptr sc = ctxt->get_second_corpus();


  string qn1 = ff->get_qualified_name(), qn2 = sf->get_qualified_name(),
    linkage_names1, linkage_names2;
  elf_symbol_sptr s1 = ff->get_symbol(), s2 = sf->get_symbol();

  if (s1)
    linkage_names1 = s1->get_id_string();
  if (s2)
    linkage_names2 = s2->get_id_string();

  // If the symbols for ff and sf have aliases, get all the names of
  // the aliases;
  if (fc && s1)
    linkage_names1 =
      s1->get_aliases_id_string(fc->get_fun_symbol_map());
  if (sc && s2)
    linkage_names2 =
      s2->get_aliases_id_string(sc->get_fun_symbol_map());

  /// If the set of linkage names of the function have changed, report
  /// it.
  if (linkage_names1 != linkage_names2)
    {
      if (linkage_names1.empty())
	{
	  out << indent << ff->get_pretty_representation()
	      << " didn't have any linkage name, and it now has: '"
	      << linkage_names2 << "'\n";
	}
      else if (linkage_names2.empty())
	{
	  out << indent << ff->get_pretty_representation()
	      << " did have linkage names '" << linkage_names1
	      << "'\n"
	      << indent << "but it doesn't have any linkage name anymore\n";
	}
      else
	out << indent << "linkage names of "
	    << ff->get_pretty_representation()
	    << "\n" << indent << "changed from '"
	    << linkage_names1 << "' to '" << linkage_names2 << "'\n";
    }

  if (qn1 != qn2
      && d.type_diff()
      && d.type_diff()->to_be_reported())
    {
      // So the function has sub-type changes that are to be
      // reported.  Let's see if the function name changed too; if it
      // did, then we'd report that change right before reporting the
      // sub-type changes.
      string frep1 = d.first_function_decl()->get_pretty_representation(),
	frep2 = d.second_function_decl()->get_pretty_representation();
      out << indent << "'" << frep1 << " {" << linkage_names1<< "}"
	  << "' now becomes '"
	  << frep2 << " {" << linkage_names2 << "}" << "'\n";
    }

  maybe_report_diff_for_symbol(ff->get_symbol(),
			       sf->get_symbol(),
			       d.context(), out, indent);

  // Now report about inline-ness changes
  if (ff->is_declared_inline() != sf->is_declared_inline())
    {
      out << indent;
      if (ff->is_declared_inline())
	out << sf->get_pretty_representation()
	    << " is not declared inline anymore\n";
      else
	out << sf->get_pretty_representation()
	    << " is now declared inline\n";
    }

  // Report about vtable offset changes.
  if (is_member_function(ff) && is_member_function(sf))
    {
      bool ff_is_virtual = get_member_function_is_virtual(ff),
	sf_is_virtual = get_member_function_is_virtual(sf);
      if (ff_is_virtual != sf_is_virtual)
	{
	  out << indent;
	  if (ff_is_virtual)
	    out << ff->get_pretty_representation()
		<< " is no more declared virtual\n";
	  else
	    out << ff->get_pretty_representation()
		<< " is now declared virtual\n";
	}

      size_t ff_vtable_offset = get_member_function_vtable_offset(ff),
	sf_vtable_offset = get_member_function_vtable_offset(sf);
      if (ff_is_virtual && sf_is_virtual
	  && (ff_vtable_offset != sf_vtable_offset))
	{
	  out << indent
	      << "the vtable offset of "  << ff->get_pretty_representation()
	      << " changed from " << ff_vtable_offset
	      << " to " << sf_vtable_offset << "\n";
	}

      // the parent types (classe or union) of the two member
      // functions.
      class_or_union_sptr f =
	is_class_or_union_type(is_method_type(ff->get_type())->get_class_type());
      class_or_union_sptr s =
	is_class_or_union_type(is_method_type(sf->get_type())->get_class_type());

      class_decl_sptr fc = is_class_type(f);
      class_decl_sptr sc = is_class_type(s);

      // Detect if the virtual member function changes above
      // introduced a vtable change or not.
      bool vtable_added = false, vtable_removed = false;
      if (!f->get_is_declaration_only() && !s->get_is_declaration_only())
	{
	  if (fc && sc)
	    {
	      vtable_added = !fc->has_vtable() && sc->has_vtable();
	      vtable_removed = fc->has_vtable() && !sc->has_vtable();
	    }
	}
      bool vtable_changed = ((ff_is_virtual != sf_is_virtual)
			     || (ff_vtable_offset != sf_vtable_offset));
      bool incompatible_change = (ff_vtable_offset != sf_vtable_offset);

      if (vtable_added)
	out << indent
	    << "  note that a vtable was added to "
	    << fc->get_pretty_representation()
	    << "\n";
      else if (vtable_removed)
	out << indent
	    << "  note that the vtable was removed from "
	    << fc->get_pretty_representation()
	    << "\n";
      else if (vtable_changed)
	{
	  out << indent;
	  if (incompatible_change)
	    out << "  note that this is an ABI incompatible "
	      "change to the vtable of ";
	  else
	    out << "  note that this induces a change to the vtable of ";
	  out << fc->get_pretty_representation()
	      << "\n";
	}

    }

  // Report about function type differences.
  if (d.type_diff() && d.type_diff()->to_be_reported())
    d.type_diff()->report(out, indent);
}

/// Report the changes carried by a @ref var_diff node in a serialized
/// form.
///
/// @param d the @ref var_diff node to consider.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
default_reporter::report(const var_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  decl_base_sptr first = d.first_var(), second = d.second_var();
  string n = first->get_pretty_representation();

  report_name_size_and_alignment_changes(first, second,
					 d.context(),
					 out, indent);

  maybe_report_diff_for_symbol(d.first_var()->get_symbol(),
			       d.second_var()->get_symbol(),
			       d.context(), out, indent);

  maybe_report_diff_for_member(first, second, d.context(), out, indent);

  if (diff_sptr dif = d.type_diff())
    {
      if (dif->to_be_reported())
	{
	  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif, "type");
	  out << indent << "type of variable changed:\n";
	  dif->report(out, indent + "  ");
	}
    }
}

/// Report the changes carried by a @ref translation_unit_diff node in
/// a serialized form.
///
/// @param d the @ref translation_unit_diff node to consider.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the prefix to use as indentation for the report.
void
default_reporter::report(const translation_unit_diff& d,
			 ostream& out,
			 const string& indent) const
{
  static_cast<const scope_diff&>(d).report(out, indent);
}

/// Report the changes carried by a @ref corpus_diff node in a
/// serialized form.
///
/// @param d the @ref corpus_diff node to consider.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the prefix to use as indentation for the report.
void
default_reporter::report(const corpus_diff& d, ostream& out,
			 const string& indent) const
{
  const corpus_diff::diff_stats &s =
    const_cast<corpus_diff&>(d).
    apply_filters_and_suppressions_before_reporting();

  const diff_context_sptr& ctxt = d.context();

  d.priv_->emit_diff_stats(s, out, indent);
  if (ctxt->show_stats_only())
    return;
  out << "\n";

  if (ctxt->show_soname_change()
      && !d.priv_->sonames_equal_)
    out << indent << "SONAME changed from '"
	<< d.first_corpus()->get_soname() << "' to '"
	<< d.second_corpus()->get_soname() << "'\n\n";

  if (ctxt->show_architecture_change()
      && !d.priv_->architectures_equal_)
    out << indent << "architecture changed from '"
	<< d.first_corpus()->get_architecture_name() << "' to '"
	<< d.second_corpus()->get_architecture_name() << "'\n\n";

  /// Report removed/added/changed functions.
  if (ctxt->show_deleted_fns())
    {
      if (s.net_num_func_removed() == 1)
	out << indent << "1 Removed function:\n\n";
      else if (s.net_num_func_removed() > 1)
	out << indent << s.net_num_func_removed() << " Removed functions:\n\n";

      bool emitted = false;
      vector<function_decl*>sorted_deleted_fns;
      sort_string_function_ptr_map(d.priv_->deleted_fns_, sorted_deleted_fns);
      for (vector<function_decl*>::const_iterator i =
	     sorted_deleted_fns.begin();
	   i != sorted_deleted_fns.end();
	   ++i)
	{
	  if (d.priv_->deleted_function_is_suppressed(*i))
	    continue;

	  out << indent
	      << "  ";
	  out << "[D] ";
	  out << "'" << (*i)->get_pretty_representation() << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *(*i)->get_symbol(),
					    d.first_corpus()->get_fun_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  if (is_member_function(*i) && get_member_function_is_virtual(*i))
	    {
	      class_decl_sptr c =
		is_class_type(is_method_type((*i)->get_type())->get_class_type());
	      out << indent
		  << "    "
		  << "note that this removes an entry from the vtable of "
		  << c->get_pretty_representation()
		  << "\n";
	    }
	  emitted = true;
	}
      if (emitted)
	out << "\n";
    }

  if (ctxt->show_added_fns())
    {
      if (s.net_num_func_added() == 1)
	out << indent << "1 Added function:\n\n";
      else if (s.net_num_func_added() > 1)
	out << indent << s.net_num_func_added()
	    << " Added functions:\n\n";
      bool emitted = false;
      vector<function_decl*> sorted_added_fns;
      sort_string_function_ptr_map(d.priv_->added_fns_, sorted_added_fns);
      for (vector<function_decl*>::const_iterator i = sorted_added_fns.begin();
	   i != sorted_added_fns.end();
	   ++i)
	{
	  if (d.priv_->added_function_is_suppressed(*i))
	    continue;

	  out
	    << indent
	    << "  ";
	  out << "[A] ";
	  out << "'"
	      << (*i)->get_pretty_representation()
	      << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases
		(out, "", *(*i)->get_symbol(),
		 d.second_corpus()->get_fun_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  if (is_member_function(*i) && get_member_function_is_virtual(*i))
	    {
	      class_decl_sptr c =
		is_class_type(is_method_type((*i)->get_type())->get_class_type());
	      out << indent
		  << "    "
		  << "note that this adds a new entry to the vtable of "
		  << c->get_pretty_representation()
		  << "\n";
	    }
	  emitted = true;
	}
      if (emitted)
	out << "\n";
    }

  if (ctxt->show_changed_fns())
    {
      size_t num_changed = s.num_func_changed() - s.num_changed_func_filtered_out();
      if (num_changed == 1)
	out << indent << "1 function with some indirect sub-type change:\n\n";
      else if (num_changed > 1)
	out << indent << num_changed
	    << " functions with some indirect sub-type change:\n\n";

      vector<function_decl_diff_sptr> sorted_changed_fns;
      sort_string_function_decl_diff_sptr_map(d.priv_->changed_fns_map_,
					      sorted_changed_fns);
      for (vector<function_decl_diff_sptr>::const_iterator i =
	     sorted_changed_fns.begin();
	   i != sorted_changed_fns.end();
	   ++i)
	{
	  diff_sptr diff = *i;
	  if (!diff)
	    continue;

	  if (diff->to_be_reported())
	    {
	      function_decl_sptr fn = (*i)->first_function_decl();
	      out << indent << "  [C] '"
		  << fn->get_pretty_representation() << "'";
	      report_loc_info((*i)->second_function_decl(), *ctxt, out);
	      out << " has some indirect sub-type changes:\n";
	      if (// The symbol of the function has aliases and the
		  // function is not a cdtor (yeah because c++ cdtors
		  // usually have several aliases).
		  (fn->get_symbol()->has_aliases()
		   && !(is_member_function(fn)
			&& get_member_function_is_ctor(fn))
		   && !(is_member_function(fn)
			&& get_member_function_is_dtor(fn)))
		  || // We are in C and the name of the function is
		     // different from the symbol name -- without
		     // taking the possible symbol version into
		     // account (this usually means the programmers
		     // was playing tricks with symbol names and
		     // versions).
		  (is_c_language(get_translation_unit(fn)->get_language())
		      && fn->get_name() != fn->get_symbol()->get_name()))
		{
		  // As the name of the symbol of the function doesn't
		  // seem to be obvious here, make sure to tell the
		  // user about the name of the (function) symbol she
		  // is looking at here.
		  int number_of_aliases =
		    fn->get_symbol()->get_number_of_aliases();
		  if (number_of_aliases == 0)
		    {
		      out << indent << "    "
			  << "Please note that the exported symbol of "
			"this function is "
			  << fn->get_symbol()->get_id_string()
			  << "\n";
		    }
		  else
		    {
		      out << indent << "    "
			  << "Please note that the symbol of this function is "
			  << fn->get_symbol()->get_id_string()
			  << "\n     and it aliases symbol";
		      if (number_of_aliases > 1)
			out << "s";
		      out << ": "
			  << fn->get_symbol()->get_aliases_id_string(false)
			  << "\n";
		    }
		}
	      diff->report(out, indent + "    ");
	      // Extra spacing.
	      out << "\n";
	    }
	}
      // Changed functions have extra spacing already. No new line here.
    }

  // Report removed/added/changed variables.
  if (ctxt->show_deleted_vars())
    {
      if (s.net_num_vars_removed() == 1)
	out << indent << "1 Removed variable:\n\n";
      else if (s.net_num_vars_removed() > 1)
	out << indent << s.net_num_vars_removed()
	    << " Removed variables:\n\n";
      string n;
      bool emitted = false;
      vector<var_decl*> sorted_deleted_vars;
      sort_string_var_ptr_map(d.priv_->deleted_vars_, sorted_deleted_vars);
      for (vector<var_decl*>::const_iterator i =
	     sorted_deleted_vars.begin();
	   i != sorted_deleted_vars.end();
	   ++i)
	{
	  if (d.priv_->deleted_variable_is_suppressed(*i))
	    continue;

	  n = (*i)->get_pretty_representation();

	  out << indent
	      << "  ";
	  out << "[D] ";
	  out << "'"
	      << n
	      << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *(*i)->get_symbol(),
					    d.first_corpus()->get_var_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  emitted = true;
	}
      if (emitted)
	  out << "\n";
    }

  if (ctxt->show_added_vars())
    {
      if (s.net_num_vars_added() == 1)
	out << indent << "1 Added variable:\n\n";
      else if (s.net_num_vars_added() > 1)
	out << indent << s.net_num_vars_added()
	    << " Added variables:\n\n";
      string n;
      bool emitted = false;
      vector<var_decl*> sorted_added_vars;
      sort_string_var_ptr_map(d.priv_->added_vars_, sorted_added_vars);
      for (vector<var_decl*>::const_iterator i =
	     sorted_added_vars.begin();
	   i != sorted_added_vars.end();
	   ++i)
	{
	  if (d.priv_->added_variable_is_suppressed(*i))
	    continue;

	  n = (*i)->get_pretty_representation();

	  out << indent
	      << "  ";
	  out << "[A] ";
	  out << "'" << n << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *(*i)->get_symbol(),
					    d.second_corpus()->get_var_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  emitted = true;
	}
      if (emitted)
	out << "\n";
    }

  if (ctxt->show_changed_vars())
    {
      size_t num_changed =
	s.num_vars_changed() - s.num_changed_vars_filtered_out();
      if (num_changed == 1)
	out << indent << "1 Changed variable:\n\n";
      else if (num_changed > 1)
	out << indent << num_changed
	    << " Changed variables:\n\n";
      string n1, n2;

      for (var_diff_sptrs_type::const_iterator i =
	     d.priv_->sorted_changed_vars_.begin();
	   i != d.priv_->sorted_changed_vars_.end();
	   ++i)
	{
	  diff_sptr diff = *i;

	  if (!diff)
	    continue;

	  if (!diff->to_be_reported())
	    continue;

	  n1 = diff->first_subject()->get_pretty_representation();
	  n2 = diff->second_subject()->get_pretty_representation();

	  out << indent << "  [C] '" << n1 << "' was changed";
	  if (n1 != n2)
	    out << " to '" << n2 << "'";
	  report_loc_info(diff->second_subject(), *ctxt, out);
	  out << ":\n";
	  diff->report(out, indent + "    ");
	  // Extra spacing.
	  out << "\n";
	}
      // Changed variables have extra spacing already. No new line here.
    }

  // Report removed function symbols not referenced by any debug info.
  if (ctxt->show_symbols_unreferenced_by_debug_info()
      && d.priv_->deleted_unrefed_fn_syms_.size())
    {
      if (s.net_num_removed_func_syms() == 1)
	out << indent
	    << "1 Removed function symbol not referenced by debug info:\n\n";
      else if (s.net_num_removed_func_syms() > 0)
	out << indent
	    << s.net_num_removed_func_syms()
	    << " Removed function symbols not referenced by debug info:\n\n";

      bool emitted = false;
      vector<elf_symbol_sptr> sorted_deleted_unrefed_fn_syms;
      sort_string_elf_symbol_map(d.priv_->deleted_unrefed_fn_syms_,
				 sorted_deleted_unrefed_fn_syms);
      for (vector<elf_symbol_sptr>::const_iterator i =
	     sorted_deleted_unrefed_fn_syms.begin();
	   i != sorted_deleted_unrefed_fn_syms.end();
	   ++i)
	{
	  if (d.priv_->deleted_unrefed_fn_sym_is_suppressed((*i).get()))
	    continue;

	  out << indent << "  ";
	  out << "[D] ";

	  show_linkage_name_and_aliases(out, "", **i,
					d.first_corpus()->get_fun_symbol_map());
	  out << "\n";
	  emitted = true;
	}
      if (emitted)
	out << "\n";
    }

  // Report added function symbols not referenced by any debug info.
  if (ctxt->show_symbols_unreferenced_by_debug_info()
      && ctxt->show_added_symbols_unreferenced_by_debug_info()
      && d.priv_->added_unrefed_fn_syms_.size())
    {
      if (s.net_num_added_func_syms() == 1)
	out << indent
	    << "1 Added function symbol not referenced by debug info:\n\n";
      else if (s.net_num_added_func_syms() > 0)
	out << indent
	    << s.net_num_added_func_syms()
	    << " Added function symbols not referenced by debug info:\n\n";

      bool emitted = false;
      vector<elf_symbol_sptr> sorted_added_unrefed_fn_syms;
      sort_string_elf_symbol_map(d.priv_->added_unrefed_fn_syms_,
				 sorted_added_unrefed_fn_syms);
      for (vector<elf_symbol_sptr>::const_iterator i =
	     sorted_added_unrefed_fn_syms.begin();
	   i != sorted_added_unrefed_fn_syms.end();
	   ++i)
	{
	  if (d.priv_->added_unrefed_fn_sym_is_suppressed((*i).get()))
	    continue;

	  out << indent << "  ";
	  out << "[A] ";
	  show_linkage_name_and_aliases(out, "",
					**i,
					d.second_corpus()->get_fun_symbol_map());
	  out << "\n";
	  emitted = true;
	}
      if (emitted)
	out << "\n";
    }

  // Report removed variable symbols not referenced by any debug info.
  if (ctxt->show_symbols_unreferenced_by_debug_info()
      && d.priv_->deleted_unrefed_var_syms_.size())
    {
      if (s.net_num_removed_var_syms() == 1)
	out << indent
	    << "1 Removed variable symbol not referenced by debug info:\n\n";
      else if (s.net_num_removed_var_syms() > 0)
	out << indent
	    << s.net_num_removed_var_syms()
	    << " Removed variable symbols not referenced by debug info:\n\n";

      bool emitted = false;
      vector<elf_symbol_sptr> sorted_deleted_unrefed_var_syms;
      sort_string_elf_symbol_map(d.priv_->deleted_unrefed_var_syms_,
				 sorted_deleted_unrefed_var_syms);
      for (vector<elf_symbol_sptr>::const_iterator i =
	     sorted_deleted_unrefed_var_syms.begin();
	   i != sorted_deleted_unrefed_var_syms.end();
	   ++i)
	{
	  if (d.priv_->deleted_unrefed_var_sym_is_suppressed((*i).get()))
	    continue;

	  out << indent << "  ";
	  out << "[D] ";

	  show_linkage_name_and_aliases
	    (out, "", **i,
	     d.first_corpus()->get_fun_symbol_map());

	  out << "\n";
	  emitted = true;
	}
      if (emitted)
	out << "\n";
    }

  // Report added variable symbols not referenced by any debug info.
  if (ctxt->show_symbols_unreferenced_by_debug_info()
      && ctxt->show_added_symbols_unreferenced_by_debug_info()
      && d.priv_->added_unrefed_var_syms_.size())
    {
      if (s.net_num_added_var_syms() == 1)
	out << indent
	    << "1 Added variable symbol not referenced by debug info:\n\n";
      else if (s.net_num_added_var_syms() > 0)
	out << indent
	    << s.net_num_added_var_syms()
	    << " Added variable symbols not referenced by debug info:\n\n";

      bool emitted = false;
      vector<elf_symbol_sptr> sorted_added_unrefed_var_syms;
      sort_string_elf_symbol_map(d.priv_->added_unrefed_var_syms_,
				 sorted_added_unrefed_var_syms);
      for (vector<elf_symbol_sptr>::const_iterator i =
	     sorted_added_unrefed_var_syms.begin();
	   i != sorted_added_unrefed_var_syms.end();
	   ++i)
	{
	  if (d.priv_->added_unrefed_var_sym_is_suppressed((*i).get()))
	    continue;

	  out << indent << "  ";
	  out << "[A] ";
	  show_linkage_name_and_aliases(out, "", **i,
					d.second_corpus()->get_fun_symbol_map());
	  out << "\n";
	  emitted = true;
	}
      if (emitted)
	out << "\n";
    }

  // Report added/removed/changed types not reacheable from public
  // interfaces.
  maybe_report_unreachable_type_changes(d, s, indent, out);

  d.priv_->maybe_dump_diff_tree();
}

} // end namespace comparison
}// end namespace libabigail

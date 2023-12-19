// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2017-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// The private data and functions of the @ref abigail::ir::comparison types.
///
/// Interfaces declared/defined in this file are to be used by parts
/// of libabigail but *NOT* by clients of libabigail.
///

#ifndef __ABG_COMPARISON_PRIV_H__
#define __ABG_COMPARISON_PRIV_H__

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
#include <memory>
#include <unordered_set>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-hash.h"
#include "abg-suppression.h"
#include "abg-comparison.h"
#include "abg-comp-filter.h"
#include "abg-sptr-utils.h"
#include "abg-tools-utils.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{

namespace comparison
{

using std::unordered_set;
using namespace abigail::suppr;

// Inject types from outside in here.
using std::vector;
using std::dynamic_pointer_cast;
using std::static_pointer_cast;
using abigail::sptr_utils::noop_deleter;

/// Convenience typedef for a pair of decls or types.
typedef std::pair<const type_or_decl_base_sptr,
		  const type_or_decl_base_sptr> types_or_decls_type;

/// A hashing functor for @ref types_or_decls_type.
struct types_or_decls_hash
{
  size_t
  operator()(const types_or_decls_type& d) const
  {
    size_t h1 = hash_type_or_decl(d.first);
    size_t h2 = hash_type_or_decl(d.second);
    return hashing::combine_hashes(h1, h2);
  }
};

/// An equality functor for @ref types_or_decls_type.
struct types_or_decls_equal
{
  bool
  operator()(const types_or_decls_type &d1, const types_or_decls_type &d2) const
  {return d1.first == d2.first && d1.second == d2.second;}
};

/// A convenience typedef for a map of @ref types_or_decls_type and
/// diff_sptr.
typedef unordered_map<types_or_decls_type, diff_sptr,
		      types_or_decls_hash, types_or_decls_equal>
  types_or_decls_diff_map_type;

/// A hashing functor for using @ref diff_sptr and @ref diff* in a
/// hash map or set.
struct diff_hash
{
  /// The function-call operator to hash a @ref diff node.
  ///
  /// @param d the @ref diff node to hash.
  ///
  /// @return the hash value of @p d.
  size_t
  operator()(const diff_sptr& d) const
  {return operator()(*d);}

  /// The function-call operator to hash a @ref diff node.
  ///
  /// @param d the @ref diff node to hash.
  ///
  /// @return the hash value of @p d.
  size_t
  operator()(const diff *d) const
  {return operator()(*d);}

  /// The function-call operator to hash a @ref diff node.
  ///
  /// @param d the @ref diff node to hash.
  ///
  /// @return the hash value of @p d.
  size_t
  operator()(const diff& d) const
  {
    diff* canonical_diff = d.get_canonical_diff();
    ABG_ASSERT(canonical_diff);
    return reinterpret_cast<size_t>(canonical_diff);
  }
}; // end struct diff_hash

/// A comparison functor for using @ref diff_sptr and @ref diff* in a
/// hash map or set.
struct diff_equal
{
  /// The function-call operator to compare two @ref diff nodes.
  ///
  /// @param d1 the first diff node involved in the comparison.
  ///
  /// @param d2 the second diff node involved in the comparison.
  ///
  /// @return true iff @p d1 equals @p d2.
  bool
  operator()(const diff* d1, const diff* d2) const
  {return operator()(*d1, *d2);}

  /// The function-call operator to compare two @ref diff nodes.
  ///
  /// @param d1 the first diff node involved in the comparison.
  ///
  /// @param d2 the second diff node involved in the comparison.
  ///
  /// @return true iff @p d1 equals @p d2.
  bool
  operator()(const diff_sptr& d1, const diff_sptr& d2) const
  {return operator()(*d1, *d2);}

  /// The function-call operator to compare two @ref diff nodes.
  ///
  /// @param d1 the first diff node involved in the comparison.
  ///
  /// @param d2 the second diff node involved in the comparison.
  ///
  /// @return true iff @p d1 equals @p d2.
  bool
  operator()(const diff& d1, const diff& d2) const
  {
    diff* canonical_diff1 = d1.get_canonical_diff();
    ABG_ASSERT(canonical_diff1);

    diff *canonical_diff2 = d2.get_canonical_diff();
    ABG_ASSERT(canonical_diff2);

    return canonical_diff1 == canonical_diff2;
  }
}; // end struct diff_equal

/// A convenience typedef for an unordered_map which key is a @ref
/// diff* and which value is a @ref artifact_sptr_set_type.
typedef unordered_map<const diff*, artifact_sptr_set_type,
		      diff_hash, diff_equal>
diff_artifact_set_map_type;

/// The private member (pimpl) for @ref diff_context.
struct diff_context::priv
{
  diff_category			allowed_category_;
  reporter_base_sptr			reporter_;
  types_or_decls_diff_map_type		types_or_decls_diff_map;
  unordered_diff_sptr_set		live_diffs_;
  vector<diff_sptr>			canonical_diffs;
  vector<filtering::filter_base_sptr>	filters_;
  suppressions_type			suppressions_;
  pointer_map				visited_diff_nodes_;
  corpus_diff_sptr			corpus_diff_;
  ostream*				default_output_stream_;
  ostream*				error_output_stream_;
  bool					leaf_changes_only_;
  bool					forbid_visiting_a_node_twice_;
  bool					reset_visited_diffs_for_each_interface_;
  bool					hex_values_;
  bool					show_offsets_sizes_in_bits_;
  bool					show_relative_offset_changes_;
  bool					show_stats_only_;
  bool					show_soname_change_;
  bool					show_architecture_change_;
  bool					show_deleted_fns_;
  bool					show_changed_fns_;
  bool					show_added_fns_;
  bool					show_deleted_vars_;
  bool					show_changed_vars_;
  bool					show_added_vars_;
  bool					show_linkage_names_;
  bool					show_locs_;
  bool					show_redundant_changes_;
  bool					flag_indirect_changes_;
  bool					show_syms_unreferenced_by_di_;
  bool					show_added_syms_unreferenced_by_di_;
  bool					show_unreachable_types_;
  bool					show_impacted_interfaces_;
  bool					dump_diff_tree_;

  priv()
    : allowed_category_(EVERYTHING_CATEGORY),
      reporter_(),
      default_output_stream_(),
      error_output_stream_(),
      leaf_changes_only_(),
      forbid_visiting_a_node_twice_(true),
      reset_visited_diffs_for_each_interface_(),
      hex_values_(),
      show_offsets_sizes_in_bits_(true),
      show_relative_offset_changes_(true),
      show_stats_only_(false),
      show_soname_change_(true),
      show_architecture_change_(true),
      show_deleted_fns_(true),
      show_changed_fns_(true),
      show_added_fns_(true),
      show_deleted_vars_(true),
      show_changed_vars_(true),
      show_added_vars_(true),
      show_linkage_names_(false),
      show_locs_(true),
      show_redundant_changes_(true),
      flag_indirect_changes_(false),
      show_syms_unreferenced_by_di_(true),
      show_added_syms_unreferenced_by_di_(true),
      show_unreachable_types_(false),
      show_impacted_interfaces_(true),
      dump_diff_tree_()
   {}
};// end struct diff_context::priv

struct type_diff_base::priv
{
public:
  friend class type_diff_base;
}; // end class type_diff_base

/// Private data for the @ref diff type.
struct diff::priv
{
  bool				finished_;
  bool				traversing_;
  type_or_decl_base_sptr	first_subject_;
  type_or_decl_base_sptr	second_subject_;
  vector<diff*>		children_;
  diff*			parent_;
  diff*			parent_interface_;
  diff*			canonical_diff_;
  diff_context_wptr		ctxt_;
  diff_category		local_category_;
  diff_category		category_;
  mutable bool			reported_once_;
  mutable bool			currently_reporting_;
  mutable string		pretty_representation_;

  priv();

public:

  priv(type_or_decl_base_sptr first_subject,
       type_or_decl_base_sptr second_subject,
       diff_context_sptr ctxt,
       diff_category category,
       bool reported_once,
       bool currently_reporting)
    : finished_(),
      traversing_(),
      first_subject_(first_subject),
      second_subject_(second_subject),
      parent_(),
      parent_interface_(),
      canonical_diff_(),
      ctxt_(ctxt),
      local_category_(category),
      category_(category),
      reported_once_(reported_once),
      currently_reporting_(currently_reporting)
  {}

  /// Getter of the diff context associated with this diff.
  ///
  /// @returnt a smart pointer to the diff context.
  diff_context_sptr
  get_context() const
  {return ctxt_.lock();}

  /// Check if a given categorization of a diff node should make it be
  /// filtered out.
  ///
  /// @param category the categorization to take into account.
  bool
  is_filtered_out(diff_category category)
  {
    diff_context_sptr ctxt = get_context();
    if (!ctxt)
      return false;

    if (ctxt->get_allowed_category() == EVERYTHING_CATEGORY)
      return false;

  /// We don't want to display nodes suppressed by a user-provided
  /// suppression specification or by a "private type" suppression
  /// specification.
    if (category & (SUPPRESSED_CATEGORY | PRIVATE_TYPE_CATEGORY))
    return true;

  // We don't want to display redundant diff nodes, when the user
  // asked to avoid seeing redundant diff nodes.
  if (!ctxt->show_redundant_changes()
      && (category & REDUNDANT_CATEGORY))
    return true;

  if (category == NO_CHANGE_CATEGORY)
    return false;

  // Ignore the REDUNDANT_CATEGORY bit when comparing allowed
  // categories and the current set of categories.
  return !((category & ~REDUNDANT_CATEGORY)
	   & (ctxt->get_allowed_category()
	      & ~REDUNDANT_CATEGORY));
  }
};// end class diff::priv

/// A functor to compare two instances of @ref diff_sptr.
struct diff_less_than_functor
{
  /// An operator that takes two instances of @ref diff_sptr returns
  /// true if its first operand compares less than its second operand.
  ///
  /// @param l the first operand to consider.
  ///
  /// @param r the second operand to consider.
  ///
  /// @return true if @p l compares less than @p r.
  bool
  operator()(const diff* l, const diff* r) const
  {
    if (!l || !r || !l->first_subject() || !r->first_subject())
      return false;

    string l_qn = get_name(l->first_subject());
    string r_qn = get_name(r->first_subject());

    return l_qn < r_qn;
  }

  /// An operator that takes two instances of @ref diff_sptr returns
  /// true if its first operand compares less than its second operand.
  ///
  /// @param l the first operand to consider.
  ///
  /// @param r the second operand to consider.
  ///
  /// @return true if @p l compares less than @p r.
  bool
  operator()(const diff_sptr& l, const diff_sptr& r) const
  {return operator()(l.get(), r.get());}
}; // end struct diff_less_than_functor

struct decl_diff_base::priv
{
public:
  friend class decl_diff_base;
};//end class priv

/// The private data structure for @ref distinct_diff.
struct distinct_diff::priv
{
  diff_sptr compatible_child_diff;
};// end struct distinct_diff

/// The internal type for the impl idiom implementation of @ref
/// var_diff.
struct var_diff::priv
{
  diff_wptr type_diff_;
};//end struct var_diff

/// The internal type for the impl idiom implementation of @ref
/// pointer_diff.
struct pointer_diff::priv
{
  diff_sptr underlying_type_diff_;

  priv(diff_sptr ud)
    : underlying_type_diff_(ud)
  {}
};//end struct pointer_diff::priv

struct array_diff::priv
{
  /// The diff between the two array element types.
  diff_sptr element_type_diff_;

  priv(diff_sptr element_type_diff)
    : element_type_diff_(element_type_diff)
  {}
};//end struct array_diff::priv

struct reference_diff::priv
{
  diff_sptr underlying_type_diff_;
  priv(diff_sptr underlying)
    : underlying_type_diff_(underlying)
  {}
};//end struct reference_diff::priv

struct qualified_type_diff::priv
{
  diff_sptr underlying_type_diff;
  mutable diff_sptr leaf_underlying_type_diff;

  priv(diff_sptr underlying)
    : underlying_type_diff(underlying)
  {}
};// end struct qualified_type_diff::priv

struct enum_diff::priv
{
  diff_sptr underlying_type_diff_;
  edit_script enumerators_changes_;
  string_enumerator_map deleted_enumerators_;
  string_enumerator_map inserted_enumerators_;
  string_changed_enumerator_map changed_enumerators_;

  priv(diff_sptr underlying)
    : underlying_type_diff_(underlying)
  {}
};//end struct enum_diff::priv

/// A functor to compare two enumerators based on their value.  This
/// implements the "less than" operator.
struct enumerator_value_comp
{
  bool
  operator()(const enum_type_decl::enumerator& f,
	     const enum_type_decl::enumerator& s) const
  {return f.get_value() < s.get_value();}
};//end struct enumerator_value_comp

/// A functor to compare two changed enumerators, based on their
/// initial value.
struct changed_enumerator_comp
{
  bool
  operator()(const changed_enumerator& f,
	     const changed_enumerator& s) const
  {return f.first.get_value() < s.first.get_value();}
};// end struct changed_enumerator_comp.

/// The type of private data of @ref class_or_union_diff.
struct class_or_union_diff::priv
{
  edit_script member_types_changes_;
  edit_script data_members_changes_;
  edit_script member_fns_changes_;
  edit_script member_fn_tmpls_changes_;
  edit_script member_class_tmpls_changes_;

  string_decl_base_sptr_map deleted_member_types_;
  string_decl_base_sptr_map inserted_member_types_;
  string_diff_sptr_map changed_member_types_;
  diff_sptrs_type sorted_changed_member_types_;
  string_decl_base_sptr_map deleted_data_members_;
  unsigned_decl_base_sptr_map deleted_dm_by_offset_;
  string_decl_base_sptr_map inserted_data_members_;
  unsigned_decl_base_sptr_map inserted_dm_by_offset_;
  // This map contains the data member which sub-type changed.
  string_var_diff_sptr_map subtype_changed_dm_;
  var_diff_sptrs_type sorted_subtype_changed_dm_;
  // This one contains the list of data members changes that can be
  // represented as a data member foo that got removed from offset N,
  // and a data member bar that got inserted at offset N; IOW, this
  // can be translated as data member foo that got changed into data
  // member bar at offset N.
  unsigned_var_diff_sptr_map changed_dm_;
  var_diff_sptrs_type sorted_changed_dm_;

  // This is a data structure to represent data members that have been
  // replaced by anonymous data members.  It's a map that associates
  // the name of the data member to the anonymous data member that
  // replaced it.
  string_decl_base_sptr_map dms_replaced_by_adms_;
  mutable changed_var_sptrs_type dms_replaced_by_adms_ordered_;
  string_member_function_sptr_map deleted_member_functions_;
  class_or_union::member_functions sorted_deleted_member_functions_;
  string_member_function_sptr_map inserted_member_functions_;
  class_or_union::member_functions sorted_inserted_member_functions_;
  string_function_decl_diff_sptr_map changed_member_functions_;
  function_decl_diff_sptrs_type sorted_changed_member_functions_;
  string_decl_base_sptr_map deleted_member_class_tmpls_;
  string_decl_base_sptr_map inserted_member_class_tmpls_;
  string_diff_sptr_map changed_member_class_tmpls_;
  diff_sptrs_type sorted_changed_member_class_tmpls_;

  type_or_decl_base_sptr
  member_type_has_changed(decl_base_sptr) const;

  decl_base_sptr
  subtype_changed_dm(decl_base_sptr) const;

  decl_base_sptr
  member_class_tmpl_has_changed(decl_base_sptr) const;

  size_t
  get_deleted_non_static_data_members_number() const;

  size_t
  get_inserted_non_static_data_members_number() const;

  size_t
  count_filtered_subtype_changed_dm(bool local_only = false);

  size_t
  count_filtered_changed_dm(bool local_only = false);

  size_t
  count_filtered_changed_mem_fns(const diff_context_sptr&);

  size_t
  count_filtered_inserted_mem_fns(const diff_context_sptr&);

  size_t
  count_filtered_deleted_mem_fns(const diff_context_sptr&);

  priv()
  {}
}; // end struct class_or_union_diff::priv

/// A comparison functor to compare two data members based on their
/// offset.
struct data_member_comp
{

  /// Compare two data members.
  ///
  /// First look at their offset and then their name.
  ///
  /// @parm first_dm the first data member to consider.
  ///
  /// @param second_dm the second data member to consider.
  bool
  compare_data_members(const var_decl_sptr& first_dm,
		       const var_decl_sptr& second_dm) const
  {
    ABG_ASSERT(first_dm);
    ABG_ASSERT(second_dm);

    size_t first_offset = get_data_member_offset(first_dm);
    size_t second_offset = get_data_member_offset(second_dm);

    // The data member at the smallest offset comes first.
    if (first_offset != second_offset)
      return first_offset < second_offset;

    string first_dm_name = first_dm->get_name();
    string second_dm_name = second_dm->get_name();

    // But in case the two data members are at the same offset, then
    // sort them lexicographically.
    return first_dm_name < second_dm_name;
  }

  /// Compare two data members.
  ///
  /// First look at their offset and then their name.
  ///
  /// @parm first_dm the first data member to consider.
  ///
  /// @param second_dm the second data member to consider.
  bool
  operator()(const decl_base_sptr& f,
	     const decl_base_sptr& s) const
  {
    var_decl_sptr first_dm = is_data_member(f);
    var_decl_sptr second_dm = is_data_member(s);

    return compare_data_members(first_dm, second_dm);
  }

  /// Compare two data members.
  ///
  /// First look at their offset and then their name.
  ///
  /// @parm first_dm the first data member to consider.
  ///
  /// @param second_dm the second data member to consider.
  bool
  operator()(const changed_var_sptr& f,
	     const changed_var_sptr& s) const
  {
    var_decl_sptr first_dm = is_data_member(is_decl(f.first));
    var_decl_sptr second_dm = is_data_member(is_decl(s.first));

    return compare_data_members(first_dm, second_dm);
  }
};//end struct data_member_comp

/// The type of the private data (pimpl sub-object) of the @ref
/// class_diff type.
struct class_diff::priv
{
  edit_script base_changes_;
  string_base_sptr_map deleted_bases_;
  class_decl::base_specs sorted_deleted_bases_;
  string_base_sptr_map inserted_bases_;
  class_decl::base_specs sorted_inserted_bases_;
  string_base_diff_sptr_map changed_bases_;
  base_diff_sptrs_type sorted_changed_bases_;

  class_decl::base_spec_sptr
  base_has_changed(class_decl::base_spec_sptr) const;

  size_t
  count_filtered_bases();

  priv()
  {}
};//end struct class_diff::priv

/// A functor to compare instances of @ref class_decl::base_spec.
struct base_spec_comp
{
  bool
  operator()(const class_decl::base_spec&l,
	     const class_decl::base_spec&r)
  {
    string str1 = l.get_pretty_representation();
    string str2 = r.get_pretty_representation();
    return str1 < str2;
  }
  bool
  operator()(const class_decl::base_spec_sptr&l,
	     const class_decl::base_spec_sptr&r)
  {return operator()(*l, *r);}
}; // end base_spec_comp

/// A comparison function for instances of @ref base_diff.
struct base_diff_comp
{
  bool
  operator()(const base_diff& l, const base_diff& r) const
  {
    class_decl::base_spec_sptr f = l.first_base(), s = r.first_base();
    if (f->get_offset_in_bits() >= 0
	&& s->get_offset_in_bits() >= 0)
      return f->get_offset_in_bits() < s->get_offset_in_bits();
    else
      return (f->get_base_class()->get_pretty_representation()
	      < s->get_base_class()->get_pretty_representation());
  }

  bool
  operator()(const base_diff* l, const base_diff* r) const
  {return operator()(*l, *r);}

  bool
  operator()(const base_diff_sptr l, const base_diff_sptr r) const
  {return operator()(l.get(), r.get());}
}; // end struct base_diff_comp

/// A comparison functor to compare two instances of @ref var_diff
/// that represent changed data members based on the offset of the
/// initial data members, or if equal, based on their qualified name.
/// If equal again, then the offset and qualified name of the new data
/// members are considered.
struct data_member_diff_comp
{
  /// @param f the first change to data member to take into account
  ///
  /// @param s the second change to data member to take into account.
  ///
  /// @return true iff f is before s.
  bool
  operator()(const var_diff_sptr f,
	     const var_diff_sptr s) const
  {
    var_decl_sptr first_dm = f->first_var();
    var_decl_sptr second_dm = s->first_var();

    ABG_ASSERT(is_data_member(first_dm));
    ABG_ASSERT(is_data_member(second_dm));

    size_t off1 = get_data_member_offset(first_dm);
    size_t off2 = get_data_member_offset(second_dm);

    if (off1 != off2)
      return off1 < off2;

    // The two offsets of the initial data members are the same.  So
    // lets compare the qualified name of these initial data members.

    string name1 = first_dm->get_qualified_name();
    string name2 = second_dm->get_qualified_name();

    if (name1 != name2)
      return name1 < name2;

    // The offsets and the qualified names of the initial data members
    // are the same.  Let's now compare the offsets of the *new* data
    // members.

    first_dm = f->second_var();
    second_dm = s->second_var();

    ABG_ASSERT(is_data_member(first_dm));
    ABG_ASSERT(is_data_member(second_dm));

    off1 = get_data_member_offset(first_dm);
    off2 = get_data_member_offset(second_dm);

    if (off1 != off2)
      return off1 < off2;

    // The offsets of the new data members are the same, dang!  Let's
    // compare the qualified names of these new data members then.

    name1 = first_dm->get_qualified_name();
    name2 = second_dm->get_qualified_name();

    return name1 < name2;
  }
}; // end struct var_diff_comp

/// A comparison functor for instances of @ref function_decl_diff that
/// represent changes between two virtual member functions.
struct virtual_member_function_diff_comp
{
  bool
  operator()(const function_decl_diff& l,
	     const function_decl_diff& r) const
  {
    ABG_ASSERT(get_member_function_is_virtual(l.first_function_decl()));
    ABG_ASSERT(get_member_function_is_virtual(r.first_function_decl()));

    return (get_member_function_vtable_offset(l.first_function_decl())
	    < get_member_function_vtable_offset(r.first_function_decl()));
  }

  bool
  operator()(const function_decl_diff* l,
	     const function_decl_diff* r)
  {return operator()(*l, *r);}

  bool
  operator()(const function_decl_diff_sptr l,
	     const function_decl_diff_sptr r)
  {return operator()(l.get(), r.get());}
}; // end struct virtual_member_function_diff_comp

struct base_diff::priv
{
  class_diff_sptr underlying_class_diff_;

  priv(class_diff_sptr underlying)
    : underlying_class_diff_(underlying)
  {}
}; // end struct base_diff::priv

struct scope_diff::priv
{
  // The edit script built by the function compute_diff.
  edit_script member_changes_;

  // Below are the useful lookup tables.
  //
  // If you add a new lookup table, please update member functions
  // clear_lookup_tables, lookup_tables_empty and
  // ensure_lookup_tables_built.

  // The deleted/inserted types/decls.  These basically map what is
  // inside the member_changes_ data member.  Note that for instance,
  // a given type T might be deleted from the first scope and added to
  // the second scope again; this means that the type was *changed*.
  string_decl_base_sptr_map deleted_types_;
  string_decl_base_sptr_map deleted_decls_;
  string_decl_base_sptr_map inserted_types_;
  string_decl_base_sptr_map inserted_decls_;

  // The changed types/decls lookup tables.
  //
  // These lookup tables are populated from the lookup tables above.
  //
  // Note that the value stored in each of these tables is a pair
  // containing the old decl/type and the new one.  That way it is
  // easy to run a diff between the old decl/type and the new one.
  //
  // A changed type/decl is one that has been deleted from the first
  // scope and that has been inserted into the second scope.
  string_diff_sptr_map changed_types_;
  diff_sptrs_type sorted_changed_types_;
  string_diff_sptr_map changed_decls_;
  diff_sptrs_type sorted_changed_decls_;

  // The removed types/decls lookup tables.
  //
  // A removed type/decl is one that has been deleted from the first
  // scope and that has *NOT* been inserted into it again.
  string_decl_base_sptr_map removed_types_;
  string_decl_base_sptr_map removed_decls_;

  // The added types/decls lookup tables.
  //
  // An added type/decl is one that has been inserted to the first
  // scope but that has not been deleted from it.
  string_decl_base_sptr_map added_types_;
  string_decl_base_sptr_map added_decls_;
};//end struct scope_diff::priv

/// A comparison functor for instances of @ref diff.
struct diff_comp
{
  /// Lexicographically compare two diff nodes.
  ///
  /// Compare the pretty representation of the first subjects of two
  /// diff nodes.
  ///
  /// @return true iff @p l is less than @p r.
  bool
  operator()(const diff& l, diff& r) const
  {
    return (get_pretty_representation(l.first_subject(), true)
	    <
	    get_pretty_representation(r.first_subject(), true));
  }

  /// Lexicographically compare two diff nodes.
  ///
  /// Compare the pretty representation of the first subjects of two
  /// diff nodes.
  ///
  /// @return true iff @p l is less than @p r.
  bool
  operator()(const diff* l, diff* r) const
  {return operator()(*l, *r);}

  /// Lexicographically compare two diff nodes.
  ///
  /// Compare the pretty representation of the first subjects of two
  /// diff nodes.
  ///
  /// @return true iff @p l is less than @p r.
  bool
  operator()(const diff_sptr l, diff_sptr r) const
  {return operator()(l.get(), r.get());}
}; // end struct diff_comp;

struct fn_parm_diff::priv
{
  mutable diff_sptr type_diff;
}; // end struct fn_parm_diff::priv

struct function_type_diff::priv
{
  diff_sptr	return_type_diff_;
  edit_script	parm_changes_;

  // useful lookup tables.
  string_parm_map			deleted_parms_;
  vector<function_decl::parameter_sptr> sorted_deleted_parms_;
  string_parm_map			added_parms_;
  vector<function_decl::parameter_sptr> sorted_added_parms_;
  // This map contains parameters sub-type changes that don't change
  // the name of the type of the parameter.
  string_fn_parm_diff_sptr_map		subtype_changed_parms_;
  vector<fn_parm_diff_sptr>		sorted_subtype_changed_parms_;
  // This map contains parameter type changes that actually change the
  // name of the type of the parameter, but in a compatible way;
  // otherwise, the mangling of the function would have changed (in
  // c++ at least).
  unsigned_fn_parm_diff_sptr_map	changed_parms_by_id_;
  vector<fn_parm_diff_sptr>		sorted_changed_parms_by_id_;
  unsigned_parm_map			deleted_parms_by_id_;
  unsigned_parm_map			added_parms_by_id_;

  priv()
  {}
}; // end struct function_type_diff::priv

struct function_decl_diff::priv
{
  function_type_diff_sptr type_diff_;

  priv()
  {}
};// end struct function_decl_diff::priv

/// A comparison functor to compare two instances of @ref fn_parm_diff
/// based on their indexes.
struct fn_parm_diff_comp
{
  /// @param f the first diff
  ///
  /// @param s the second diff
  ///
  /// @return true if the index of @p f is less than the index of @p
  /// s.
  bool
  operator()(const fn_parm_diff& f, const fn_parm_diff& s)
  {return f.first_parameter()->get_index() < s.first_parameter()->get_index();}

  bool
  operator()(const fn_parm_diff_sptr& f, const fn_parm_diff_sptr& s)
  {return operator()(*f, *s);}
}; // end struct fn_parm_diff_comp

/// Functor that compares two function parameters for the purpose of
/// sorting them.
struct parm_comp
{
  /// Returns true iff the index of the first parameter is smaller
  /// than the of the second parameter.
  ///
  /// @param l the first parameter to compare.
  ///
  /// @param r the second parameter to compare.
  ///
  /// @return true iff the index of the first parameter is smaller
  /// than the of the second parameter.
  bool
  operator()(const function_decl::parameter& l,
	     const function_decl::parameter& r)
  {return l.get_index() < r.get_index();}

  /// Returns true iff the index of the first parameter is smaller
  /// than the of the second parameter.
  ///
  /// @param l the first parameter to compare.
  ///
  /// @param r the second parameter to compare.
  ///
  /// @return true iff the index of the first parameter is smaller
  /// than the of the second parameter.
  bool
  operator()(const function_decl::parameter_sptr& l,
	     const function_decl::parameter_sptr& r)
  {return operator()(*l, *r);}
}; // end struct parm_comp

/// A functor to compare instances of @ref var_decl base on their
/// qualified names.
struct var_comp
{
  bool
  operator() (const var_decl& l, const var_decl& r) const
  {
    string name1 = l.get_qualified_name(), name2 = r.get_qualified_name();
    return name1 < name2;
  }

  bool
  operator() (const var_decl* l, const var_decl* r) const
  {return operator()(*l, *r);}
};// end struct var_comp

/// A functor to compare instances of @ref elf_symbol base on their
/// names.
struct elf_symbol_comp
{
  bool
  operator()(const elf_symbol& l, const elf_symbol& r)
  {
    string name1 = l.get_id_string(), name2 = r.get_id_string();
    return name1 < name2;
  }

  bool
  operator()(const elf_symbol* l, const elf_symbol* r)
  {return operator()(*l, *r);}

  bool
  operator()(const elf_symbol_sptr& l, const elf_symbol_sptr& r)
  {return operator()(l.get(), r.get());}
}; //end struct elf_symbol_comp

struct typedef_diff::priv
{
  diff_sptr underlying_type_diff_;

  priv(const diff_sptr underlying_type_diff)
    : underlying_type_diff_(underlying_type_diff)
  {}
};//end struct typedef_diff::priv

struct translation_unit_diff::priv
{
  translation_unit_sptr first_;
  translation_unit_sptr second_;

  priv(translation_unit_sptr f, translation_unit_sptr s)
    : first_(f), second_(s)
  {}
};//end struct translation_unit_diff::priv

struct corpus_diff::priv
{
  bool					finished_;
  string				pretty_representation_;
  vector<diff*>			children_;
  corpus_sptr				first_;
  corpus_sptr				second_;
  diff_context_wptr			ctxt_;
  corpus_diff::diff_stats_sptr		diff_stats_;
  bool					sonames_equal_;
  bool					architectures_equal_;
  edit_script				fns_edit_script_;
  edit_script				vars_edit_script_;
  edit_script				unrefed_fn_syms_edit_script_;
  edit_script				unrefed_var_syms_edit_script_;
  string_function_ptr_map		deleted_fns_;
  string_function_ptr_map		suppressed_deleted_fns_;
  string_function_ptr_map		added_fns_;
  string_function_ptr_map		suppressed_added_fns_;
  string_function_decl_diff_sptr_map	changed_fns_map_;
  function_decl_diff_sptrs_type	changed_fns_;
  string_var_ptr_map			deleted_vars_;
  string_var_ptr_map			suppressed_deleted_vars_;
  string_var_ptr_map			added_vars_;
  string_var_ptr_map			suppressed_added_vars_;
  string_var_diff_sptr_map		changed_vars_map_;
  var_diff_sptrs_type			sorted_changed_vars_;
  string_elf_symbol_map		added_unrefed_fn_syms_;
  string_elf_symbol_map		suppressed_added_unrefed_fn_syms_;
  string_elf_symbol_map		deleted_unrefed_fn_syms_;
  string_elf_symbol_map		suppressed_deleted_unrefed_fn_syms_;
  string_elf_symbol_map		added_unrefed_var_syms_;
  string_elf_symbol_map		suppressed_added_unrefed_var_syms_;
  string_elf_symbol_map		deleted_unrefed_var_syms_;
  string_elf_symbol_map		suppressed_deleted_unrefed_var_syms_;
  edit_script				unreachable_types_edit_script_;
  string_type_base_sptr_map		deleted_unreachable_types_;
  vector<type_base_sptr>		deleted_unreachable_types_sorted_;
  string_type_base_sptr_map		suppressed_deleted_unreachable_types_;
  string_type_base_sptr_map		added_unreachable_types_;
  vector<type_base_sptr>		added_unreachable_types_sorted_;
  string_type_base_sptr_map		suppressed_added_unreachable_types_;
  string_diff_sptr_map			changed_unreachable_types_;
  mutable vector<diff_sptr>		changed_unreachable_types_sorted_;
  diff_maps				leaf_diffs_;

  /// Default constructor of corpus_diff::priv.
  priv()
    : finished_(false),
      sonames_equal_(false),
      architectures_equal_(false)
  {}

  /// Constructor of corpus_diff::priv.
  ///
  /// @param first the first corpus of this diff.
  ///
  /// @param second the second corpus of this diff.
  ///
  /// @param ctxt the context of the diff.
  priv(corpus_sptr first,
       corpus_sptr second,
       diff_context_sptr ctxt)
    : finished_(false),
      first_(first),
      second_(second),
      ctxt_(ctxt),
      sonames_equal_(false),
      architectures_equal_(false)
  {}

  diff_context_sptr
  get_context();

  bool
  lookup_tables_empty() const;

  void
  clear_lookup_tables();

  void
  ensure_lookup_tables_populated();

  void
  apply_supprs_to_added_removed_fns_vars_unreachable_types();

  bool
  deleted_function_is_suppressed(const function_decl* fn) const;

  bool
  added_function_is_suppressed(const function_decl* fn) const;

  bool
  deleted_variable_is_suppressed(const var_decl* var) const;

  bool
  added_variable_is_suppressed(const var_decl* var) const;

  bool
  added_unreachable_type_is_suppressed(const type_base *t)const ;

  bool
  deleted_unreachable_type_is_suppressed(const type_base *t)const ;

  bool
  deleted_unrefed_fn_sym_is_suppressed(const elf_symbol*) const;

  bool
  added_unrefed_fn_sym_is_suppressed(const elf_symbol*) const;

  bool
  deleted_unrefed_var_sym_is_suppressed(const elf_symbol*) const;

  bool
  added_unrefed_var_sym_is_suppressed(const elf_symbol*) const;

  void count_leaf_changes(size_t &num_changes, size_t &num_filtered);

  void count_leaf_type_changes(size_t &num_type_changes,
			       size_t &num_type_changes_filtered);

  void count_unreachable_types(size_t &num_added,
			       size_t &num_removed,
			       size_t &num_changed,
			       size_t &num_filtered_added,
			       size_t &num_filtered_removed,
			       size_t &num_filtered_changed);

  const vector<diff_sptr>&
  changed_unreachable_types_sorted() const;

  void
  apply_filters_and_compute_diff_stats(corpus_diff::diff_stats&);

  void
  emit_diff_stats(const diff_stats&	stats,
		  ostream&		out,
		  const string&	indent);

  void
  categorize_redundant_changed_sub_nodes();

  void
  clear_redundancy_categorization();

  void
  maybe_dump_diff_tree();
}; // end corpus::priv

/// "Less than" functor to compare instances of @ref function_decl.
struct function_comp
{
  /// The actual "less than" operator for instances of @ref
  /// function_decl.  It returns true if the first @ref function_decl
  /// is lest than the second one.
  ///
  /// @param f the first @ref function_decl to take in account.
  ///
  /// @param s the second @ref function_decl to take in account.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const function_decl& f, const function_decl& s)
  {return abigail::ir::function_decl_is_less_than(f, s);}

  /// The actual "less than" operator for instances of @ref
  /// function_decl.  It returns true if the first @ref function_decl
  /// is lest than the second one.
  ///
  /// @param f the first @ref function_decl to take in account.
  ///
  /// @param s the second @ref function_decl to take in account.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const function_decl* f, const function_decl* s)
  {return operator()(*f, *s);}

  /// The actual "less than" operator for instances of @ref
  /// function_decl.  It returns true if the first @ref function_decl
  /// is lest than the second one.
  ///
  /// @param f the first @ref function_decl to take in account.
  ///
  /// @param s the second @ref function_decl to take in account.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const function_decl_sptr f, const function_decl_sptr s)
  {return operator()(f.get(), s.get());}
}; // end function_comp

/// A "Less Than" functor to compare instance of @ref
/// function_decl_diff.
struct function_decl_diff_comp
{
  /// The actual less than operator.
  ///
  /// It returns true if the first @ref function_decl_diff is less
  /// than the second one.
  ///
  /// param first the first @ref function_decl_diff to consider.
  ///
  /// @param second the second @ref function_decl_diff to consider.
  ///
  /// @return true iff @p first is less than @p second.
  bool
  operator()(const function_decl_diff& first,
	     const function_decl_diff& second)
  {
    function_decl_sptr f = first.first_function_decl(),
      s = second.first_function_decl();

    string fr = f->get_qualified_name(),
      sr = s->get_qualified_name();

    if (fr == sr)
      {
	if (f->get_symbol())
	  fr = f->get_symbol()->get_id_string();
	else if (!f->get_linkage_name().empty())
	  fr = f->get_linkage_name();
	else
	  fr = f->get_pretty_representation();

	if (s->get_symbol())
	  sr = s->get_symbol()->get_id_string();
	else if (!s->get_linkage_name().empty())
	  sr = s->get_linkage_name();
	else
	  sr = s->get_pretty_representation();
      }

    return (fr.compare(sr) < 0);
  }

  /// The actual less than operator.
  ///
  /// It returns true if the first @ref function_decl_diff_sptr is
  /// less than the second one.
  ///
  /// param first the first @ref function_decl_diff_sptr to consider.
  ///
  /// @param second the second @ref function_decl_diff_sptr to
  /// consider.
  ///
  /// @return true iff @p first is less than @p second.
  bool
  operator()(const function_decl_diff_sptr first,
	     const function_decl_diff_sptr second)
  {return operator()(*first, *second);}
}; // end struct function_decl_diff_comp

/// Functor to sort instances of @ref var_diff_sptr
struct var_diff_sptr_comp
{
  /// Return true if the first argument is less than the second one.
  ///
  /// @param f the first argument to consider.
  ///
  /// @param s the second argument to consider.
  ///
  /// @return true if @p f is less than @p s.
  bool
  operator()(const var_diff_sptr f,
	     const var_diff_sptr s)
  {
    return (f->first_var()->get_qualified_name()
	    < s->first_var()->get_qualified_name());
  }
}; // end struct var_diff_sptr_comp

/// The type of the private data of corpus_diff::diff_stats.
struct corpus_diff::diff_stats::priv
{
  friend class corpus_diff::diff_stats;

  diff_context_wptr	ctxt_;
  size_t		num_func_removed;
  size_t		num_removed_func_filtered_out;
  size_t		num_func_added;
  size_t		num_added_func_filtered_out;
  size_t		num_func_changed;
  size_t		num_changed_func_filtered_out;
  size_t		num_func_with_virt_offset_changes;
  size_t		num_vars_removed;
  size_t		num_removed_vars_filtered_out;
  size_t		num_vars_added;
  size_t		num_added_vars_filtered_out;
  size_t		num_vars_changed;
  size_t		num_changed_vars_filtered_out;
  size_t		num_func_syms_removed;
  size_t		num_removed_func_syms_filtered_out;
  size_t		num_func_syms_added;
  size_t		num_added_func_syms_filtered_out;
  size_t		num_var_syms_removed;
  size_t		num_removed_var_syms_filtered_out;
  size_t		num_var_syms_added;
  size_t		num_added_var_syms_filtered_out;
  size_t		num_leaf_changes;
  size_t		num_leaf_changes_filtered_out;
  size_t		num_leaf_type_changes;
  size_t		num_leaf_type_changes_filtered_out;
  size_t		num_leaf_func_changes;
  size_t		num_leaf_func_changes_filtered_out;
  size_t		num_leaf_var_changes;
  size_t		num_leaf_var_changes_filtered_out;
  size_t		num_added_unreachable_types;
  size_t		num_added_unreachable_types_filtered_out;
  size_t		num_removed_unreachable_types;
  size_t		num_removed_unreachable_types_filtered_out;
  size_t		num_changed_unreachable_types;
  size_t		num_changed_unreachable_types_filtered_out;

  priv(diff_context_sptr ctxt)
    : ctxt_(ctxt),
      num_func_removed(),
      num_removed_func_filtered_out(),
      num_func_added(),
      num_added_func_filtered_out(),
      num_func_changed(),
      num_changed_func_filtered_out(),
      num_func_with_virt_offset_changes(),
      num_vars_removed(),
      num_removed_vars_filtered_out(),
      num_vars_added(),
      num_added_vars_filtered_out(),
      num_vars_changed(),
      num_changed_vars_filtered_out(),
      num_func_syms_removed(),
      num_removed_func_syms_filtered_out(),
      num_func_syms_added(),
      num_added_func_syms_filtered_out(),
      num_var_syms_removed(),
      num_removed_var_syms_filtered_out(),
      num_var_syms_added(),
      num_added_var_syms_filtered_out(),
      num_leaf_changes(),
      num_leaf_changes_filtered_out(),
      num_leaf_type_changes(),
      num_leaf_type_changes_filtered_out(),
      num_leaf_func_changes(),
      num_leaf_func_changes_filtered_out(),
      num_leaf_var_changes(),
      num_leaf_var_changes_filtered_out(),
      num_added_unreachable_types(),
      num_added_unreachable_types_filtered_out(),
      num_removed_unreachable_types(),
      num_removed_unreachable_types_filtered_out(),
      num_changed_unreachable_types(),
      num_changed_unreachable_types_filtered_out()
  {}

  diff_context_sptr
  ctxt()
  {return ctxt_.lock();}
}; // end class corpus_diff::diff_stats::priv

void
sort_enumerators(const string_enumerator_map& enumerators_map,
		 enum_type_decl::enumerators& sorted);

void
sort_changed_enumerators(const string_changed_enumerator_map& enumerators_map,
			 changed_enumerators_type& sorted);

void
sort_data_members(const string_decl_base_sptr_map &data_members,
		  vector<decl_base_sptr>& sorted);

void
sort_changed_data_members(changed_var_sptrs_type& input);

void
sort_string_function_ptr_map(const string_function_ptr_map& map,
			     vector<function_decl*>& sorted);

void
sort_string_member_function_sptr_map(const string_member_function_sptr_map& map,
				     class_or_union::member_functions& sorted);

void
sort_string_type_base_sptr_map(string_type_base_sptr_map& map,
			       vector<type_base_sptr>& sorted);

void
sort_string_function_decl_diff_sptr_map
(const string_function_decl_diff_sptr_map& map,
 function_decl_diff_sptrs_type& sorted);

void
sort_string_var_diff_sptr_map(const string_var_diff_sptr_map& map,
			      var_diff_sptrs_type& sorted);

void
sort_string_elf_symbol_map(const string_elf_symbol_map& map,
			   vector<elf_symbol_sptr>& sorted);

void
sort_string_var_ptr_map(const string_var_ptr_map& map,
			vector<var_decl*>& sorted);

void
sort_string_data_member_diff_sptr_map(const string_var_diff_sptr_map& map,
				      var_diff_sptrs_type& sorted);

void
sort_unsigned_data_member_diff_sptr_map(const unsigned_var_diff_sptr_map map,
					var_diff_sptrs_type& sorted);

void
sort_string_virtual_member_function_diff_sptr_map
(const string_function_decl_diff_sptr_map& map,
 function_decl_diff_sptrs_type& sorted);

void
sort_string_diff_sptr_map(const string_diff_sptr_map& map,
			  diff_sptrs_type& sorted);

void
sort_string_diff_ptr_map(const string_diff_ptr_map& map,
			 diff_ptrs_type& sorted);

void
sort_string_base_diff_sptr_map(const string_base_diff_sptr_map& map,
			       base_diff_sptrs_type& sorted);

void
sort_string_base_sptr_map(const string_base_sptr_map& m,
			  class_decl::base_specs& sorted);

void
sort_string_fn_parm_diff_sptr_map(const unsigned_fn_parm_diff_sptr_map& map,
				  vector<fn_parm_diff_sptr>&		sorted);
void
sort_string_fn_parm_diff_sptr_map(const string_fn_parm_diff_sptr_map&	map,
				  vector<fn_parm_diff_sptr>&		sorted);
void
sort_string_parm_map(const string_parm_map& map,
		     vector<function_decl::parameter_sptr>& sorted);

void
sort_artifacts_set(const artifact_sptr_set_type& set,
		   vector<type_or_decl_base_sptr>& sorted);

type_base_sptr
get_leaf_type(qualified_type_def_sptr t);

diff*
get_fn_decl_or_var_decl_diff_ancestor(const diff *);

bool
is_diff_of_global_decls(const diff*);

} // end namespace comparison

} // namespace abigail

#endif // __ABG_COMPARISON_PRIV_H__

// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This contains the implementation of the comparison engine of
/// libabigail.

#include <ctype.h>
#include <libgen.h>
#include <algorithm>
#include <sstream>

#include "abg-comparison-priv.h"
#include "abg-reporter-priv.h"

namespace abigail
{

namespace comparison
{

///
///
///@defgroup DiffNode Internal Representation of the comparison engine
/// @{
///
/// @brief How changes are represented in libabigail's comparison engine.
///
///@par diff nodes
///
/// The internal representation of the comparison engine is basically
/// a graph of @ref instances of @ref diff node.  We refer to these
/// just as <em>diff nodes</em>.  A diff node represents a change
/// between two ABI artifacts represented by instances of types of the
/// abigail::ir namespace.  These two artifacts that are being
/// compared are called the <em>subjects of the diff</em>.
///
/// The types of that IR are in the abigail::comparison namespace.
///
///@par comparing diff nodes
///
/// Comparing two instances of @ref diff nodes amounts to comparing
/// the subject of the diff.  In other words, two @ref diff nodes are
/// equal if and only if their subjects are equal.  Thus, two @ref
/// diff nodes can have different memory addresses and yet be equal.
///
///@par diff reporting and context
///
/// A diff node can be serialized to an output stream to express, in
/// a human-readable textual form, the different changes that exist
/// between its two subjects.  This is done by invoking the
/// diff::report() method.  That reporting is controlled by several
/// parameters that are conceptually part of the context of the diff.
/// That context is materialized by an instance of the @ref
/// diff_context type.
///
/// Please note that the role of the instance(s) of @ref diff_context
/// is boreader than just controlling the reporting of @ref diff
/// nodes.  Basically, a @ref diff node itself is created following
/// behaviours that are controlled by a particular instance of
/// diff_context.  A diff node is created in a particular diff
/// context, so to speak.
///
/// @}
///

///
///@defgroup CanonicalDiff Canonical diff tree nodes
/// @{
///
/// @brief How equivalent diff nodes are quickly spotted.
///
/// @par Equivalence of diff nodes.
///
/// Each @ref diff node has a property named <em>Canonical Diff
/// Node</em>.  If \c D is a diff node, the canonical diff node of @c
/// D, noted @c C(D) is a particular diff node that is equal to @c D.
/// Thus, a fast way to compare two @ref diff node is to perform a
/// pointer comparison of their canonical diff nodes.
///
/// A set of equivalent @ref diff nodes is a set of diff nodes that
/// all have the same canonical node.  All the nodes of that set are
/// equal.
///
/// A canonical node is registereded for a given diff node by invoking
/// the method diff_context::initialize_canonical_diff().
///
/// Please note that the diff_context holds all the canonical diffs
/// that got registered through it.  Thus, the life time of all of
/// canonical diff objects is the same as the life time of the @ref
/// diff_context they relate to.
///
/// @}
///

// -----------------------------------------
// <private functions re-usable elsewhere>
// -----------------------------------------
/// Sort a map of enumerators by their value.
///
/// @param enumerators_map the map to sort.
///
/// @param sorted the resulting vector of sorted enumerators.
void
sort_enumerators(const string_enumerator_map& enumerators_map,
		 enum_type_decl::enumerators& sorted)
{
  for (string_enumerator_map::const_iterator i = enumerators_map.begin();
       i != enumerators_map.end();
       ++i)
    sorted.push_back(i->second);
  enumerator_value_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of changed enumerators.
///
/// @param enumerators_map the map to sort.
///
///@param output parameter.  The resulting sorted enumerators.
void
sort_changed_enumerators(const string_changed_enumerator_map& enumerators_map,
			 changed_enumerators_type& sorted)
{
  for (string_changed_enumerator_map::const_iterator i =
	 enumerators_map.begin();
       i != enumerators_map.end();
       ++i)
    sorted.push_back(i->second);

  changed_enumerator_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of data members by the offset of their initial value.
///
/// @param data_members the map of changed data members to sort.
///
/// @param sorted the resulting vector of sorted changed data members.
void
sort_data_members(const string_decl_base_sptr_map &data_members,
		  vector<decl_base_sptr>& sorted)
{
  sorted.reserve(data_members.size());
  for (string_decl_base_sptr_map::const_iterator i = data_members.begin();
       i != data_members.end();
       ++i)
    sorted.push_back(i->second);

  data_member_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort (in place) a vector of changed data members.
///
/// @param to_sort the vector to sort.
void
sort_changed_data_members(changed_var_sptrs_type& to_sort)
{
  data_member_comp comp;
  std::sort(to_sort.begin(), to_sort.end(), comp);
}

/// Sort an instance of @ref string_function_ptr_map map and stuff a
/// resulting sorted vector of pointers to function_decl.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector.
void
sort_string_function_ptr_map(const string_function_ptr_map& map,
			     vector<function_decl*>& sorted)
{
  sorted.reserve(map.size());
  for (string_function_ptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  function_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map that's an instance of @ref
/// string_member_function_sptr_map and fill a vector of member
/// functions with the sorted result.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector.
void
sort_string_member_function_sptr_map(const string_member_function_sptr_map& map,
				     class_or_union::member_functions& sorted)
{
  sorted.reserve(map.size());
  for (string_member_function_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  function_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort the values of a @ref string_function_decl_diff_sptr_map map
/// and store the result in a vector of @ref function_decl_diff_sptr
/// objects.
///
/// @param map the map whose values to store.
///
/// @param sorted the vector of function_decl_diff_sptr to store the
/// result of the sort into.
void
sort_string_function_decl_diff_sptr_map
(const string_function_decl_diff_sptr_map& map,
 function_decl_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_function_decl_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);
  function_decl_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort of an instance of @ref string_var_diff_sptr_map map.
///
/// @param map the input map to sort.
///
/// @param sorted the ouptut sorted vector of @ref var_diff_sptr.
/// It's populated with the sorted content.
void
sort_string_var_diff_sptr_map(const string_var_diff_sptr_map& map,
			      var_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_var_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  var_diff_sptr_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of string -> pointer to @ref elf_symbol.
///
/// The result is a vector of @ref elf_symbol_sptr sorted by the
/// name of the symbol.
///
/// @param map the map to sort.
///
/// @param sorted out parameter; the sorted vector of @ref
/// elf_symbol_sptr.
void
sort_string_elf_symbol_map(const string_elf_symbol_map& map,
			   vector<elf_symbol_sptr>& sorted)
{
  for (string_elf_symbol_map::const_iterator i = map.begin();
       i!= map.end();
       ++i)
    sorted.push_back(i->second);

  elf_symbol_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of string -> pointer to @ref var_decl.
///
/// The result is a vector of var_decl* sorted by the qualified name
/// of the variables.
///
/// @param map the map to sort.
///
/// @param sorted out parameter; the sorted vector of @ref var_decl.
void
sort_string_var_ptr_map(const string_var_ptr_map& map,
			vector<var_decl*>& sorted)
{
  for (string_var_ptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  var_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort the values of a string_var_diff_sptr_map and store the result
/// in a vector of var_diff_sptr.
///
/// @param map the map of changed data members to sort.
///
/// @param sorted the resulting vector of var_diff_sptr.
void
sort_string_data_member_diff_sptr_map(const string_var_diff_sptr_map& map,
				      var_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_var_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);
  data_member_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort the values of a unsigned_var_diff_sptr_map map and store the
/// result into a vector of var_diff_sptr.
///
/// @param map the map of changed data members to sort.
///
/// @param sorted the resulting vector of sorted var_diff_sptr.
void
sort_unsigned_data_member_diff_sptr_map(const unsigned_var_diff_sptr_map map,
					var_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (unsigned_var_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);
  data_member_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort an map of string -> virtual member function into a vector of
/// virtual member functions.  The virtual member functions are sorted
/// by increasing order of their virtual index.
///
/// @param map the input map.
///
/// @param sorted the resulting sorted vector of virtual function
/// member.
void
sort_string_virtual_member_function_diff_sptr_map
(const string_function_decl_diff_sptr_map& map,
 function_decl_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_function_decl_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  virtual_member_function_diff_comp comp;
  sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map ofg string -> @ref diff_sptr into a vector of @ref
/// diff_sptr.  The diff_sptr are sorted lexicographically wrt
/// qualified names of their first subjects.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector.
void
sort_string_diff_sptr_map(const string_diff_sptr_map& map,
			  diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  diff_comp comp;
  sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map ofg string -> @ref diff* into a vector of @ref
/// diff_ptr.  The diff_ptr are sorted lexicographically wrt
/// qualified names of their first subjects.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector.
void
sort_string_diff_ptr_map(const string_diff_ptr_map& map,
			  diff_ptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_diff_ptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  diff_comp comp;
  sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of string -> base_diff_sptr into a sorted vector of
/// base_diff_sptr.  The base_diff_sptr are sorted by increasing value
/// of their offset in their containing type.
///
/// @param map the input map to sort.
///
/// @param sorted the resulting sorted vector.
void
sort_string_base_diff_sptr_map(const string_base_diff_sptr_map& map,
			       base_diff_sptrs_type& sorted)
{
  for (string_base_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);
  base_diff_comp comp;
  sort(sorted.begin(), sorted.end(), comp);
}

/// Lexicographically sort base specifications found
/// in instances of string_base_sptr_map.
void
sort_string_base_sptr_map(const string_base_sptr_map& m,
			  class_decl::base_specs& sorted)
{
  for (string_base_sptr_map::const_iterator i = m.begin();
       i != m.end();
       ++i)
    sorted.push_back(i->second);

  base_spec_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of @ref fn_parm_diff by the indexes of the function
/// parameters.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector of changed function
/// parms.
void
sort_string_fn_parm_diff_sptr_map(const unsigned_fn_parm_diff_sptr_map& map,
				  vector<fn_parm_diff_sptr>&		sorted)
{
  sorted.reserve(map.size());
  for (unsigned_fn_parm_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  fn_parm_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of changed function parameters by the indexes of the
/// function parameters.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector of instances of @ref
/// fn_parm_diff_sptr
void
sort_string_fn_parm_diff_sptr_map(const string_fn_parm_diff_sptr_map&	map,
				  vector<fn_parm_diff_sptr>&		sorted)
{
  sorted.reserve(map.size());
  for (string_fn_parm_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  fn_parm_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of string -> function parameters.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector of
/// @ref vector<function_decl::parameter_sptr>
void
sort_string_parm_map(const string_parm_map& map,
		     vector<function_decl::parameter_sptr>& sorted)
{
  for (string_parm_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  parm_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort the set of ABI artifacts contained in a @ref
/// artifact_sptr_set_type.
///
/// @param set the set of ABI artifacts to sort.
///
/// @param output parameter the vector containing the sorted ABI
/// artifacts.
void
sort_artifacts_set(const artifact_sptr_set_type& set,
		   vector<type_or_decl_base_sptr>& sorted)
{

  for (artifact_sptr_set_type::const_iterator it = set.begin();
       it != set.end();
       ++it)
    sorted.push_back(*it);

  type_or_decl_base_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of string to type_base_sptr entities.
///
/// The entries are sorted based on the lexicographic order of the
/// pretty representation of the type_sptr_sptr.  The sorted result is
/// put in a vector of type_base_sptr.
///
/// @param map the map to sort.
///
/// @param sorted the resulting vector of type_base_sptr
/// lexicographically sorted using their pretty representation.
void
sort_string_type_base_sptr_map(string_type_base_sptr_map& map,
			       vector<type_base_sptr>& sorted)
{
  for (string_type_base_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  type_or_decl_base_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Return the first underlying type that is not a qualified type.
/// @param t the qualified type to consider.
///
/// @return the first underlying type that is not a qualified type, or
/// NULL if t is NULL.
type_base_sptr
get_leaf_type(qualified_type_def_sptr t)
{
  if (!t)
    return type_base_sptr();

  type_base_sptr ut = t->get_underlying_type();
  qualified_type_def_sptr qut = dynamic_pointer_cast<qualified_type_def>(ut);

  if (!qut)
    return ut;
  return get_leaf_type(qut);
}

/// Tests if a given diff node is to represent the changes between two
/// gobal decls.
///
/// @param d the diff node to consider.
///
/// @return true iff @p d represents the changes between two global
/// decls.
bool
is_diff_of_global_decls(const diff* d)
{
  ABG_ASSERT(d != 0);

  if (d == 0)
    return false;

  type_or_decl_base_sptr first = d->first_subject();
  ABG_ASSERT(first);

  type_or_decl_base_sptr second = d->first_subject();
  ABG_ASSERT(second);

  if (decl_base_sptr decl = is_decl(first))
    if (is_at_global_scope(decl))
      if ((decl = is_decl(second)))
	if (is_at_global_scope(decl))
	  return true;

  return false;
}

// -----------------------------------------
// </private functions re-usable elsewhere>
// -----------------------------------------

/// The overloaded or operator for @ref visiting_kind.
visiting_kind
operator|(visiting_kind l, visiting_kind r)
{return static_cast<visiting_kind>(static_cast<unsigned>(l)
				   | static_cast<unsigned>(r));}

/// The overloaded and operator for @ref visiting_kind.
visiting_kind
operator&(visiting_kind l, visiting_kind r)
{
  return static_cast<visiting_kind>(static_cast<unsigned>(l)
				    & static_cast<unsigned>(r));
}

/// The overloaded 'bit inversion' operator for @ref visiting_kind.
visiting_kind
operator~(visiting_kind l)
{return static_cast<visiting_kind>(~static_cast<unsigned>(l));}

/// Test if a diff node is about differences between types.
///
/// @param diff the diff node to test.
///
/// @return a pointer to the actual type_diff_base* that @p diff
/// extends, iff it is about differences between types.
const type_diff_base*
is_type_diff(const diff* diff)
{return dynamic_cast<const type_diff_base*>(diff);}

/// Test if a diff node is about differences between declarations.
///
/// @param diff the diff node to test.
///
/// @return a pointer to the actual decl_diff_base @p diff extends,
/// iff it is about differences between declarations.
const decl_diff_base*
is_decl_diff(const diff* diff)
{return dynamic_cast<const decl_diff_base*>(diff);}

/// Test if a diff node is a @ref class_diff node.
///
/// @param diff the diff node to consider.
///
/// @return a non-nil pointer to a @ref class_diff iff @p diff is a
/// @ref class_diff node.
const class_diff*
is_class_diff(const diff* diff)
{return dynamic_cast<const class_diff*>(diff);}

/// Test if a diff node is a @ref enum_diff node.
///
/// @param diff the diff node to consider.
///
/// @return a non-nil pointer to ad @ref enum_diff node iff @p diff is
/// a @ref enum_diff node.
const enum_diff*
is_enum_diff(const diff *diff)
{return dynamic_cast<const enum_diff*>(diff);}

/// Test if a diff node is a @ref union_diff node.
///
/// @param diff the diff node to consider.
///
/// @return a non-nil pointer to a @ref union_diff iff @p diff is a
/// @ref union_diff node.
const union_diff*
is_union_diff(const diff* diff)
{return dynamic_cast<const union_diff*>(diff);}

/// Test if a diff node is a @ref class_or_union_diff node.
///
/// @param d the diff node to consider.
///
/// @return a non-nil pointer to the @ref class_or_union_diff denoted
/// by @p d iff @p d is a @ref class_or_union_diff.
const class_or_union_diff*
is_class_or_union_diff(const diff* d)
{return dynamic_cast<const class_or_union_diff*>(d);}

/// Test if a diff node is a @ref class_or_union_diff between two
/// anonymous classes or unions.
///
/// @param d the diff node to consider.
///
/// @return a non-nil pointer to the @ref class_or_union_diff iff @p
/// denoted by @p d iff @p is pointer to an anonymous class or union
/// diff.
const class_or_union_diff*
is_anonymous_class_or_union_diff(const diff* d)
{
  if (const class_or_union_diff *dif = is_class_or_union_diff(d))
    if (dif->first_class_or_union()->get_is_anonymous())
      return dif;
  return 0;
}

/// Test if a diff node is a @ref typedef_diff node.
///
/// @param diff the diff node to consider.
///
/// @return a non-nil pointer to a @ref typedef_diff iff @p diff is a
/// @ref typedef_diff node.
const typedef_diff*
is_typedef_diff(const diff *diff)
{return dynamic_cast<const typedef_diff*>(diff);}

/// Test if a diff node is a @ref array_diff node.
///
/// @param diff the diff node to consider.
///
/// @return a non-nil pointer to a @ref array_diff iff @p diff is a
/// @ref array_diff node.
const array_diff*
is_array_diff(const diff* diff)
{return dynamic_cast<const array_diff*>(diff);}

/// Test if a diff node is a @ref function_type_diff node.
///
/// @param diff the diff node to consider.
///
/// @return a non-nil pointer to a @ref function_type_diff iff @p diff is a
/// @ref function_type_diff node.
const function_type_diff*
is_function_type_diff(const diff* diff)
{return dynamic_cast<const function_type_diff*>(diff);}

/// Test if a given diff node carries a function type change with
/// local changes.
///
/// @param diff the diff node to consider.
///
/// @return a non-nil pointer to a @ref function_type_diff iff @p diff
/// is a function_type_diff node that carries a local change.
const function_type_diff*
is_function_type_diff_with_local_changes(const diff* diff)
{
  if (const function_type_diff* d = is_function_type_diff(diff))
    if (d->has_local_changes())
      return d;

  return 0;
}

/// Test if a diff node is about differences between variables.
///
/// @param diff the diff node to test.
///
/// @return a pointer to the actual var_diff that @p diff is a type
/// of, iff it is about differences between variables.
const var_diff*
is_var_diff(const diff* diff)
{
  const var_diff* d = dynamic_cast<const var_diff*>(diff);
  if (d)
    ABG_ASSERT(is_decl_diff(diff));
  return d;
}

/// Test if a diff node is about differences between functions.
///
/// @param diff the diff node to test.
///
/// @return a pointer to the actual var_diff that @p diff is a type
/// of, iff it is about differences between variables.
const function_decl_diff*
is_function_decl_diff(const diff* diff)
{
  const function_decl_diff *d = dynamic_cast<const function_decl_diff*>(diff);
  if (d)
    ABG_ASSERT(is_decl_diff(diff));
  return d;
}

/// Test if a diff node is about differences between two pointers.
///
/// @param diff the diff node to consider.
///
/// @return the @p diff converted into an instance of @ref
/// pointer_diff iff @p diff is about differences between two
/// pointers.
const pointer_diff*
is_pointer_diff(const diff* diff)
{return dynamic_cast<const pointer_diff*>(diff);}

/// Test if a diff node is about differences between two references.
///
/// @param diff the diff node to consider.
///
/// @return the @p diff converted into an instance of @ref
/// reference_diff iff @p diff is about differences between two
/// references.
const reference_diff*
is_reference_diff(const diff* diff)
{return dynamic_cast<const reference_diff*>(diff);}

/// Test if a diff node is about differences between two qualified
/// types.
///
/// @param diff the diff node to consider.
///
/// @return @p diff converted into an instance of @ref
/// qualified_type_diff iff @p diff is about differences between two
/// qualified types.
const qualified_type_diff*
is_qualified_type_diff(const diff* diff)
{return dynamic_cast<const qualified_type_diff*>(diff);}

/// Test if a diff node is a reference or pointer diff node to a
/// change that is neither basic type change nor distinct type change.
///
/// Note that this function also works on diffs of typedefs of
/// reference or pointer.
///
/// @param diff the diff node to consider.
///
/// @return true iff @p diff is a eference or pointer diff node to a
/// change that is neither basic type change nor distinct type change.
bool
is_reference_or_ptr_diff_to_non_basic_nor_distinct_types(const diff* diff)
{
  diff = peel_typedef_diff(diff);
  if (const reference_diff* d = is_reference_diff(diff))
    {
      diff = peel_reference_diff(d);
      if (is_diff_of_basic_type(diff) || is_distinct_diff(diff))
	return false;
      return true;
    }
  else if (const pointer_diff *d = is_pointer_diff(diff))
    {
      diff = peel_pointer_diff(d);
      if (is_diff_of_basic_type(diff) || is_distinct_diff(diff))
	return false;
      return true;
    }

  return false;
}

/// Test if a diff node is about differences between two function
/// parameters.
///
/// @param diff the diff node to consider.
///
/// @return the @p diff converted into an instance of @ref
/// reference_diff iff @p diff is about differences between two
/// function parameters.
const fn_parm_diff*
is_fn_parm_diff(const diff* diff)
{return dynamic_cast<const fn_parm_diff*>(diff);}

/// Test if a diff node is about differences between two base class
/// specifiers.
///
/// @param diff the diff node to consider.
///
/// @return the @p diff converted into an instance of @ref base_diff
/// iff @p diff is about differences between two base class
/// specifiers.
const base_diff*
is_base_diff(const diff* diff)
{return dynamic_cast<const base_diff*>(diff);}

/// Test if a diff node is about differences between two diff nodes of
/// different kinds.
///
/// @param diff the diff node to consider.
///
/// @return the @p diff converted into an instance of @ref
/// distintc_diff iff @p diff is about differences between two diff
/// nodes of different kinds.
const distinct_diff*
is_distinct_diff(const diff *diff)
{return dynamic_cast<const distinct_diff*>(diff);}

/// Test if a diff node is a @ref corpus_diff node.
///
/// @param diff the diff node to consider.
///
/// @return a non-nil pointer to a @ref corpus_diff iff @p diff is a
/// @ref corpus_diff node.
const corpus_diff*
is_corpus_diff(const diff* diff)
{return dynamic_cast<const corpus_diff*>(diff);}

/// Test if a diff node is a child node of a function parameter diff node.
///
/// @param diff the diff node to test.
///
/// @return true iff @p diff is a child node of a function parameter
/// diff node.
bool
is_child_node_of_function_parm_diff(const diff* diff)
{return diff && is_fn_parm_diff(diff->parent_node());}

/// Test if a diff node is a child node of a base diff node.
///
/// @param diff the diff node to test.
///
/// @return true iff @p diff is a child node of a base diff node.
bool
is_child_node_of_base_diff(const diff* diff)
{return diff && is_base_diff(diff->parent_node());}

/// The default traverse function.
///
/// @return true.
bool
diff_traversable_base::traverse(diff_node_visitor&)
{return true;}

diff_context::diff_context()
  : priv_(new diff_context::priv)
{
  // Setup all the diff output filters we have.
  filtering::filter_base_sptr f;

  f.reset(new filtering::harmless_harmful_filter);
  add_diff_filter(f);

  // f.reset(new filtering::harmless_filter);
  // add_diff_filter(f);

  // f.reset(new filtering::harmful_filter);
  // add_diff_filter(f);
}

/// Set the corpus diff relevant to this context.
///
/// @param d the corpus_diff we are interested in.
void
diff_context::set_corpus_diff(const corpus_diff_sptr& d)
{priv_->corpus_diff_ = d;}

/// Get the corpus diff for the current context.
///
/// @return the corpus diff of this context.
const corpus_diff_sptr&
diff_context::get_corpus_diff() const
{return priv_->corpus_diff_;}

/// Getter for the first corpus of the corpus diff of the current context.
///
/// @return the first corpus of the corpus diff of the current
/// context, if no corpus diff is associated to the context.
corpus_sptr
diff_context::get_first_corpus() const
{
  if (priv_->corpus_diff_)
    return priv_->corpus_diff_->first_corpus();
  return corpus_sptr();
}

/// Getter for the second corpus of the corpus diff of the current
/// context.
///
/// @return the second corpus of the corpus diff of the current
/// context, if no corpus diff is associated to the context.
corpus_sptr
diff_context::get_second_corpus() const
{
  if (priv_->corpus_diff_)
    return priv_->corpus_diff_->second_corpus();
  return corpus_sptr();
}

/// Getter of the reporter to be used in this context.
///
/// @return the reporter to be used in this context.
reporter_base_sptr
diff_context::get_reporter() const
{
  if (!priv_->reporter_)
    {
      if (show_leaf_changes_only())
	priv_->reporter_.reset(new leaf_reporter);
      else
	priv_->reporter_.reset(new default_reporter);
    }
  ABG_ASSERT(priv_->reporter_);
  return priv_->reporter_;
}

/// Setter of the reporter to be used in this context.
///
/// @param r the reporter to be used in this context.
void
diff_context::set_reporter(reporter_base_sptr& r)
{priv_->reporter_ = r;}

/// Tests if the current diff context already has a diff for two decls.
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @return a pointer to the diff for @p first @p second if found,
/// null otherwise.
diff_sptr
diff_context::has_diff_for(const type_or_decl_base_sptr first,
			   const type_or_decl_base_sptr second) const
{
  types_or_decls_diff_map_type::const_iterator i =
    priv_->types_or_decls_diff_map.find(std::make_pair(first, second));
  if (i != priv_->types_or_decls_diff_map.end())
    return i->second;
  return diff_sptr();
}

/// Tests if the current diff context already has a diff for two types.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @return a pointer to the diff for @p first @p second if found,
/// null otherwise.
diff_sptr
diff_context::has_diff_for_types(const type_base_sptr first,
				  const type_base_sptr second) const
{return has_diff_for(first, second);}

/// Tests if the current diff context already has a given diff.
///
///@param d the diff to consider.
///
/// @return a pointer to the diff found for @p d
const diff*
diff_context::has_diff_for(const diff* d) const
{return has_diff_for(d->first_subject(), d->second_subject()).get();}

/// Tests if the current diff context already has a given diff.
///
///@param d the diff to consider.
///
/// @return a pointer to the diff found for @p d
diff_sptr
diff_context::has_diff_for(const diff_sptr d) const
{return has_diff_for(d->first_subject(), d->second_subject());}

/// Getter for the bitmap that represents the set of categories that
/// the user wants to see reported.
///
/// @return a bitmap that represents the set of categories that the
/// user wants to see reported.
diff_category
diff_context::get_allowed_category() const
{return priv_->allowed_category_;}

/// Setter for the bitmap that represents the set of categories that
/// the user wants to see reported.
///
/// @param c a bitmap that represents the set of categories that the
/// user wants to see represented.
void
diff_context::set_allowed_category(diff_category c)
{priv_->allowed_category_ = c;}

/// Setter for the bitmap that represents the set of categories that
/// the user wants to see reported
///
/// This function perform a bitwise or between the new set of
/// categories and the current ones, and then sets the current
/// categories to the result of the or.
///
/// @param c a bitmap that represents the set of categories that the
/// user wants to see represented.
void
diff_context::switch_categories_on(diff_category c)
{priv_->allowed_category_ = priv_->allowed_category_ | c;}

/// Setter for the bitmap that represents the set of categories that
/// the user wants to see reported
///
/// This function actually unsets bits from the current categories.
///
/// @param c a bitmap that represents the set of categories to unset
/// from the current categories.
void
diff_context::switch_categories_off(diff_category c)
{priv_->allowed_category_ = priv_->allowed_category_ & ~c;}

/// Add a diff for two decls to the cache of the current diff_context.
///
/// Doing this allows to later find the added diff from its two
/// subject decls.
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @param the diff to add.
void
diff_context::add_diff(type_or_decl_base_sptr first,
		       type_or_decl_base_sptr second,
		       const diff_sptr d)
{priv_->types_or_decls_diff_map[std::make_pair(first, second)] = d;}

/// Add a diff tree node to the cache of the current diff_context
///
/// @param d the diff tree node to add.
void
diff_context::add_diff(const diff* d)
{
  if (d)
    {
      diff_sptr dif(const_cast<diff*>(d), noop_deleter());
      add_diff(d->first_subject(), d->second_subject(), dif);
    }
}

/// Add a diff tree node to the cache of the current diff_context
///
/// @param d the diff tree node to add.
void
diff_context::add_diff(const diff_sptr d)
{
  if (d)
      add_diff(d->first_subject(), d->second_subject(), d);
}

/// Getter for the @ref CanonicalDiff "canonical diff node" for the
/// @ref diff represented by their two subjects.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @return the canonical diff for the diff node represented by the
/// two diff subjects @p first and @p second.  If no canonical diff
/// node was registered for these subjects, then a nil node is
/// returned.
diff_sptr
diff_context::get_canonical_diff_for(const type_or_decl_base_sptr first,
				     const type_or_decl_base_sptr second) const
{return has_diff_for(first, second);}

/// Getter for the @ref CanonicalDiff "canonical diff node" for the
/// @ref diff represented by the two subjects of a given diff node.
///
/// @param d the diff node to get the canonical node for.
///
/// @return the canonical diff for the diff node represented by the
/// two diff subjects of @p d.  If no canonical diff node was
/// registered for these subjects, then a nil node is returned.
diff_sptr
diff_context::get_canonical_diff_for(const diff_sptr d) const
{return has_diff_for(d);}

/// Setter for the @ref CanonicalDiff "canonical diff node" for the
/// @ref diff represented by their two subjects.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param d the new canonical diff.
void
diff_context::set_canonical_diff_for(const type_or_decl_base_sptr first,
				     const type_or_decl_base_sptr second,
				     const diff_sptr d)
{
  ABG_ASSERT(d);
  if (!has_diff_for(first, second))
    {
      add_diff(first, second, d);
      priv_->canonical_diffs.push_back(d);
    }
}

/// If there is is a @ref CanonicalDiff "canonical diff node"
/// registered for two diff subjects, return it.  Otherwise, register
/// a canonical diff node for these two diff subjects and return it.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param d the new canonical diff node.
///
/// @return the canonical diff node.
diff_sptr
diff_context::set_or_get_canonical_diff_for(const type_or_decl_base_sptr first,
					    const type_or_decl_base_sptr second,
					    const diff_sptr canonical_diff)
{
  ABG_ASSERT(canonical_diff);

  diff_sptr canonical = get_canonical_diff_for(first, second);
  if (!canonical)
    {
      canonical = canonical_diff;
      set_canonical_diff_for(first, second, canonical);
    }
  return canonical;
}

/// Set the canonical diff node property of a given diff node
/// appropriately.
///
/// For a given diff node that has no canonical diff node, retrieve
/// the canonical diff node (by looking at its diff subjects and at
/// the current context) and set the canonical diff node property of
/// the diff node to that canonical diff node.  If no canonical diff
/// node has been registered to the diff context for the subjects of
/// the diff node then, register the canonical diff node as being the
/// diff node itself; and set its canonical diff node property as
/// such.  Otherwise, if the diff node already has a canonical diff
/// node, do nothing.
///
/// @param diff the diff node to initialize the canonical diff node
/// property for.
void
diff_context::initialize_canonical_diff(const diff_sptr diff)
{
  if (diff->get_canonical_diff() == 0)
    {
      diff_sptr canonical =
	set_or_get_canonical_diff_for(diff->first_subject(),
				      diff->second_subject(),
				      diff);
      diff->set_canonical_diff(canonical.get());
    }
}

/// Add a diff node to the set of diff nodes that are kept alive for
/// the life time of the current instance of diff_context.
///
/// Note that diff added to the diff cache are kept alive as well, and
/// don't need to be passed to this function to be kept alive.
///
/// @param d the diff node to be kept alive during the life time of
/// the current instance of @ref diff_context.
void
diff_context::keep_diff_alive(diff_sptr& d)
{priv_->live_diffs_.insert(d);}

/// Test if a diff node has been traversed.
///
/// @param d the diff node to consider.
///
/// @return the first diff node against which @p d is redundant.
diff*
diff_context::diff_has_been_visited(const diff* d) const
{
  const diff* canonical = d->get_canonical_diff();
  ABG_ASSERT(canonical);

  size_t ptr_value = reinterpret_cast<size_t>(canonical);
  pointer_map::iterator it = priv_->visited_diff_nodes_.find(ptr_value);
  if (it != priv_->visited_diff_nodes_.end())
    return reinterpret_cast<diff*>(it->second);
  else
    return 0;
}

/// Test if a diff node has been traversed.
///
/// @param d the diff node to consider.
///
/// @return the first diff node against which @p d is redundant.
diff_sptr
diff_context::diff_has_been_visited(const diff_sptr d) const
{
  diff_sptr diff(diff_has_been_visited(d.get()));
  return diff;
}

/// Mark a diff node as traversed by a traversing algorithm.
///
/// Actually, it's the @ref CanonicalDiff "canonical diff" of this
/// node that is marked as traversed.
///
/// Subsequent invocations of diff_has_been_visited() on the diff node
/// will yield true.
void
diff_context::mark_diff_as_visited(const diff* d)
{
  if (diff_has_been_visited(d))
    return;

  const diff* canonical = d->get_canonical_diff();
  ABG_ASSERT(canonical);

   size_t canonical_ptr_value = reinterpret_cast<size_t>(canonical);
   size_t diff_ptr_value = reinterpret_cast<size_t>(d);
   priv_->visited_diff_nodes_[canonical_ptr_value] = diff_ptr_value;
}

/// Unmark all the diff nodes that were marked as being traversed.
void
diff_context::forget_visited_diffs()
{priv_->visited_diff_nodes_.clear();}

/// This sets a flag that, if it's true, then during the traversing of
/// a diff nodes tree each node is visited at most once.
///
/// @param f if true then during the traversing of a diff nodes tree
/// each node is visited at most once.
///
void
diff_context::forbid_visiting_a_node_twice(bool f)
{priv_->forbid_visiting_a_node_twice_ = f;}

/// This function sets a flag os that if @ref
///  forbid_visiting_a_node_twice() returns true, then each time the
///  node visitor starts visiting a new interface, it resets the
///  memory the systems has about already visited node.
///
///  @param f the flag to set.
void
diff_context::forbid_visiting_a_node_twice_per_interface(bool f)
{priv_->reset_visited_diffs_for_each_interface_ = f;}

/// Return a flag that, if true, then during the traversing of a diff
/// nodes tree each node is visited at most once.
///
/// @return the boolean flag.
bool
diff_context::visiting_a_node_twice_is_forbidden() const
{return priv_->forbid_visiting_a_node_twice_;}

/// Return a flag that, if true, then during the traversing of a diff
/// nodes tree each node is visited at most once, while visiting the
/// diff tree underneath a given interface (public function or
/// variable).  Each time a new interface is visited, the nodes
/// visited while visiting previous interfaces can be visited again.
///
/// @return the boolean flag.
///
/// @return the boolean flag.
bool
diff_context::visiting_a_node_twice_is_forbidden_per_interface() const
{
  return (priv_->forbid_visiting_a_node_twice_
	  && priv_->reset_visited_diffs_for_each_interface_);
}

/// Getter for the diff tree nodes filters to apply to diff sub-trees.
///
/// @return the vector of tree filters to apply to diff sub-trees.
const filtering::filters&
diff_context::diff_filters() const
{return priv_->filters_;}

/// Setter for the diff filters to apply to a given diff sub-tree.
///
/// @param f the new diff filter to add to the vector of diff filters
/// to apply to diff sub-trees.
void
diff_context::add_diff_filter(filtering::filter_base_sptr f)
{priv_->filters_.push_back(f);}

/// Apply the diff filters to a given diff sub-tree.
///
/// If the current context is instructed to filter out some categories
/// then this function walks the given sub-tree and categorizes its
/// nodes by using the filters held by the context.
///
/// @param diff the diff sub-tree to apply the filters to.
void
diff_context::maybe_apply_filters(diff_sptr diff)
{
  if (!diff)
    return;

  if (get_allowed_category() == EVERYTHING_CATEGORY)
    return;

  if (!diff->has_changes())
    return;

  for (filtering::filters::const_iterator i = diff_filters().begin();
       i != diff_filters().end();
       ++i)
    {
      filtering::apply_filter(*i, diff);
      propagate_categories(diff);
    }

 }

/// Apply the diff filters to the diff nodes of a @ref corpus_diff
/// instance.
///
/// If the current context is instructed to filter out some categories
/// then this function walks the diff tree and categorizes its nodes
/// by using the filters held by the context.
///
/// @param diff the corpus diff to apply the filters to.
void
diff_context::maybe_apply_filters(corpus_diff_sptr diff)
{

  if (!diff || !diff->has_changes())
    return;

  for (filtering::filters::const_iterator i = diff_filters().begin();
       i != diff_filters().end();
       ++i)
    {
      filtering::apply_filter(**i, diff);
      propagate_categories(diff);
    }
}

/// Getter for the vector of suppressions that specify which diff node
/// reports should be dropped on the floor.
///
/// @return the set of suppressions.
suppressions_type&
diff_context::suppressions() const
{return priv_->suppressions_;}

/// Add a new suppression specification that specifies which diff node
/// reports should be dropped on the floor.
///
/// @param suppr the new suppression specification to add to the
/// existing set of suppressions specifications of the diff context.
void
diff_context::add_suppression(const suppression_sptr suppr)
{priv_->suppressions_.push_back(suppr);}

/// Add new suppression specifications that specify which diff node
/// reports should be dropped on the floor.
///
/// @param supprs the new suppression specifications to add to the
/// existing set of suppression specifications of the diff context.
void
diff_context::add_suppressions(const suppressions_type& supprs)
{
  priv_->suppressions_.insert(priv_->suppressions_.end(),
			      supprs.begin(), supprs.end());
}

/// Set the flag that indicates if the diff using this context should
/// show only leaf changes or not.
///
/// @param f the new value of the flag that indicates if the diff
/// using this context should show only leaf changes or not.
void
diff_context::show_leaf_changes_only(bool f)
{
  // This function can be called only if the reporter hasn't yet been
  // created.  Once it's been created, we are supposed to live with
  // it.
  ABG_ASSERT(priv_->reporter_ == 0);
  priv_->leaf_changes_only_ = f;
}

/// Get the flag that indicates if the diff using this context should
/// show only leaf changes or not.
///
/// @return the value of the flag that indicates if the diff using
/// this context should show only leaf changes or not.
bool
diff_context::show_leaf_changes_only() const
{return priv_->leaf_changes_only_;}

/// Get the flag that indicates if the diff reports using this context
/// should show sizes and offsets in an hexadecimal base or not.  If
/// not, then they are to be shown in a decimal base.
///
/// @return true iff sizes and offsets are to be shown in an
/// hexadecimal base.
bool
diff_context::show_hex_values() const
{return priv_->hex_values_;}

/// Set the flag that indicates if diff reports using this context
/// should show sizes and offsets in an hexadecimal base or not.  If
/// not, then they are to be shown in a decimal base.
///
/// @param f if true then sizes and offsets are to be shown in an
/// hexadecimal base.
void
diff_context::show_hex_values(bool f)
{priv_->hex_values_ = f;}

/// Get the flag that indicates if diff reports using this context
/// should show sizes and offsets in bits, rather than bytes.
///
/// @return true iff sizes and offsets are to be shown in bits.
/// Otherwise they are to be shown in bytes.
bool
diff_context::show_offsets_sizes_in_bits() const
{return priv_->show_offsets_sizes_in_bits_;}

/// Set the flag that indicates if diff reports using this context
/// should show sizes and offsets in bits, rather than bytes.
///
/// @param f if true then sizes and offsets are to be shown in bits.
/// Otherwise they are to be shown in bytes.
void
diff_context::show_offsets_sizes_in_bits(bool f)
{priv_->show_offsets_sizes_in_bits_ = f;}

/// Set a flag saying if offset changes should be reported in a
/// relative way.  That is, if the report should say how of many bits
/// a class/struct data member did move.
///
/// @param f the new boolean value of the flag.
void
diff_context::show_relative_offset_changes(bool f)
{priv_->show_relative_offset_changes_ = f;}

/// Get the flag saying if offset changes should be reported in a
/// relative way.  That is, if the report should say how of many bits
/// a class/struct data member did move.
///
/// @return the boolean value of the flag.
bool
diff_context::show_relative_offset_changes(void)
{return priv_->show_relative_offset_changes_;}

/// Set a flag saying if the comparison module should only show the
/// diff stats.
///
/// @param f the flag to set.
void
diff_context::show_stats_only(bool f)
{priv_->show_stats_only_ = f;}

/// Test if the comparison module should only show the diff stats.
///
/// @return true if the comparison module should only show the diff
/// stats, false otherwise.
bool
diff_context::show_stats_only() const
{return priv_->show_stats_only_;}

/// Setter for the property that says if the comparison module should
/// show the soname changes in its report.
///
/// @param f the new value of the property.
void
diff_context::show_soname_change(bool f)
{priv_->show_soname_change_ = f;}

/// Getter for the property that says if the comparison module should
/// show the soname changes in its report.
///
/// @return the value of the property.
bool
diff_context::show_soname_change() const
{return priv_->show_soname_change_;}

/// Setter for the property that says if the comparison module should
/// show the architecture changes in its report.
///
/// @param f the new value of the property.
void
diff_context::show_architecture_change(bool f)
{priv_->show_architecture_change_ = f;}

/// Getter for the property that says if the comparison module should
/// show the architecture changes in its report.
///
/// @return the value of the property.
bool
diff_context::show_architecture_change() const
{return priv_->show_architecture_change_;}

/// Set a flag saying to show the deleted functions.
///
/// @param f true to show deleted functions.
void
diff_context::show_deleted_fns(bool f)
{priv_->show_deleted_fns_ = f;}

/// @return true if we want to show the deleted functions, false
/// otherwise.
bool
diff_context::show_deleted_fns() const
{return priv_->show_deleted_fns_;}

/// Set a flag saying to show the changed functions.
///
/// @param f true to show the changed functions.
void
diff_context::show_changed_fns(bool f)
{priv_->show_changed_fns_ = f;}

/// @return true if we want to show the changed functions, false otherwise.
bool
diff_context::show_changed_fns() const
{return priv_->show_changed_fns_;}

/// Set a flag saying to show the added functions.
///
/// @param f true to show the added functions.
void
diff_context::show_added_fns(bool f)
{priv_->show_added_fns_ = f;}

/// @return true if we want to show the added functions, false
/// otherwise.
bool
diff_context::show_added_fns() const
{return priv_->show_added_fns_;}

/// Set a flag saying to show the deleted variables.
///
/// @param f true to show the deleted variables.
void
diff_context::show_deleted_vars(bool f)
{priv_->show_deleted_vars_ = f;}

/// @return true if we want to show the deleted variables, false
/// otherwise.
bool
diff_context::show_deleted_vars() const
{return priv_->show_deleted_vars_;}

/// Set a flag saying to show the changed variables.
///
/// @param f true to show the changed variables.
void
diff_context::show_changed_vars(bool f)
{priv_->show_changed_vars_ = f;}

/// @return true if we want to show the changed variables, false otherwise.
bool
diff_context::show_changed_vars() const
{return priv_->show_changed_vars_;}

/// Set a flag saying to show the added variables.
///
/// @param f true to show the added variables.
void
diff_context::show_added_vars(bool f)
{priv_->show_added_vars_ = f;}

/// @return true if we want to show the added variables, false
/// otherwise.
bool
diff_context::show_added_vars() const
{return priv_->show_added_vars_;}

bool
diff_context::show_linkage_names() const
{return priv_->show_linkage_names_;}

void
diff_context::show_linkage_names(bool f)
{priv_->show_linkage_names_= f;}

/// Set a flag saying to show location information.
///
/// @param f true to show location information.
void
diff_context::show_locs(bool f)
{priv_->show_locs_= f;}

/// @return true if we want to show location information, false
/// otherwise.
bool
diff_context::show_locs() const
{return priv_->show_locs_;}

/// A getter for the flag that says if we should report about
/// functions or variables diff nodes that have *exclusively*
/// redundant diff tree children nodes.
///
/// @return the flag.
bool
diff_context::show_redundant_changes() const
{return priv_->show_redundant_changes_;}

/// A setter for the flag that says if we should report about
/// functions or variables diff nodes that have *exclusively*
/// redundant diff tree children nodes.
///
/// @param f the flag to set.
void
diff_context::show_redundant_changes(bool f)
{priv_->show_redundant_changes_ = f;}

/// A getter for the flag that says if we should flag indirect class
/// and union changes in leaf-changes-only mode.
///
/// @return the flag.
bool
diff_context::flag_indirect_changes() const
{return priv_->flag_indirect_changes_;}

/// A setter for the flag that says if we should flag indirect class
/// and union changes in leaf-changes-only mode.
///
/// @param f the flag to set.
void
diff_context::flag_indirect_changes(bool f)
{priv_->flag_indirect_changes_ = f;}

/// Getter for the flag that indicates if symbols not referenced by
/// any debug info are to be compared and reported about.
///
/// @return the boolean flag.
bool
diff_context::show_symbols_unreferenced_by_debug_info() const
{return priv_->show_syms_unreferenced_by_di_;}

/// Setter for the flag that indicates if symbols not referenced by
/// any debug info are to be compared and reported about.
///
/// @param f the new flag to set.
void
diff_context::show_symbols_unreferenced_by_debug_info(bool f)
{priv_->show_syms_unreferenced_by_di_ = f;}

/// Getter for the flag that indicates if symbols not referenced by
/// any debug info and that got added are to be reported about.
///
/// @return true iff symbols not referenced by any debug info and that
/// got added are to be reported about.
bool
diff_context::show_added_symbols_unreferenced_by_debug_info() const
{return priv_->show_added_syms_unreferenced_by_di_;}

/// Setter for the flag that indicates if symbols not referenced by
/// any debug info and that got added are to be reported about.
///
/// @param f the new flag that says if symbols not referenced by any
/// debug info and that got added are to be reported about.
void
diff_context::show_added_symbols_unreferenced_by_debug_info(bool f)
{priv_->show_added_syms_unreferenced_by_di_ = f;}

/// Setter for the flag that indicates if changes on types unreachable
/// from global functions and variables are to be reported.
///
/// @param f if true, then changes on types unreachable from global
/// functions and variables are to be reported.
void
diff_context::show_unreachable_types(bool f)
{priv_->show_unreachable_types_ = f;}

/// Getter for the flag that indicates if changes on types unreachable
/// from global functions and variables are to be reported.
///
/// @return true iff changes on types unreachable from global
/// functions and variables are to be reported.
bool
diff_context::show_unreachable_types()
{return priv_->show_unreachable_types_;}

/// Getter of the flag that indicates if the leaf reporter should
/// display a summary of the interfaces impacted by a given leaf
/// change or not.
///
/// @return the flag that indicates if the leaf reporter should
/// display a summary of the interfaces impacted by a given leaf
/// change or not.
bool
diff_context::show_impacted_interfaces() const
{return priv_->show_impacted_interfaces_;}

/// Setter of the flag that indicates if the leaf reporter should
/// display a summary of the interfaces impacted by a given leaf
/// change or not.
///
/// @param f the new value of the flag that indicates if the leaf
/// reporter should display a summary of the interfaces impacted by a
/// given leaf change or not.
void
diff_context::show_impacted_interfaces(bool f)
{priv_->show_impacted_interfaces_ = f;}

/// Setter for the default output stream used by code of the
/// comparison engine.  By default the default output stream is a NULL
/// pointer.
///
/// @param o a pointer to the default output stream.
void
diff_context::default_output_stream(ostream* o)
{priv_->default_output_stream_ = o;}

/// Getter for the default output stream used by code of the
/// comparison engine.  By default the default output stream is a NULL
/// pointer.
///
/// @return a pointer to the default output stream.
ostream*
diff_context::default_output_stream()
{return priv_->default_output_stream_;}

/// Setter for the errror output stream used by code of the comparison
/// engine.  By default the error output stream is a NULL pointer.
///
/// @param o a pointer to the error output stream.
void
diff_context::error_output_stream(ostream* o)
{priv_->error_output_stream_ = o;}

/// Getter for the errror output stream used by code of the comparison
/// engine.  By default the error output stream is a NULL pointer.
///
/// @return a pointer to the error output stream.
ostream*
diff_context::error_output_stream() const
{return priv_->error_output_stream_;}

/// Test if the comparison engine should dump the diff tree for the
/// changed functions and variables it has.
///
/// @return true if after the comparison, the engine should dump the
/// diff tree for the changed functions and variables it has.
bool
diff_context::dump_diff_tree() const
{return priv_->dump_diff_tree_;}

/// Set if the comparison engine should dump the diff tree for the
/// changed functions and variables it has.
///
/// @param f true if after the comparison, the engine should dump the
/// diff tree for the changed functions and variables it has.
void
diff_context::dump_diff_tree(bool f)
{priv_->dump_diff_tree_ = f;}

/// Emit a textual representation of a diff tree to the error output
/// stream of the current context, for debugging purposes.
///
/// @param d the diff tree to serialize to the error output associated
/// to the current instance of @ref diff_context.
void
diff_context::do_dump_diff_tree(const diff_sptr d) const
{
  if (error_output_stream())
    print_diff_tree(d, *error_output_stream());
}

/// Emit a textual representation of a @ref corpus_diff tree to the error
/// output stream of the current context, for debugging purposes.
///
/// @param d the @ref corpus_diff tree to serialize to the error
/// output associated to the current instance of @ref diff_context.
void
diff_context::do_dump_diff_tree(const corpus_diff_sptr d) const
{
  if (error_output_stream())
    print_diff_tree(d, *error_output_stream());
}
// </diff_context stuff>

// <diff stuff>

/// Constructor for the @ref diff type.
///
/// This constructs a diff between two subjects that are actually
/// declarations; the first and the second one.
///
/// @param first_subject the first decl (subject) of the diff.
///
/// @param second_subject the second decl (subject) of the diff.
diff::diff(type_or_decl_base_sptr first_subject,
	   type_or_decl_base_sptr second_subject)
  : priv_(new priv(first_subject, second_subject,
		   diff_context_sptr(),
		   NO_CHANGE_CATEGORY,
		   /*reported_once=*/false,
		   /*currently_reporting=*/false))
{}

/// Constructor for the @ref diff type.
///
/// This constructs a diff between two subjects that are actually
/// declarations; the first and the second one.
///
/// @param first_subject the first decl (subject) of the diff.
///
/// @param second_subject the second decl (subject) of the diff.
///
/// @param ctxt the context of the diff.  Note that this context
/// object must stay alive during the entire life time of the current
/// instance of @ref diff.  Otherwise, memory corruption issues occur.
diff::diff(type_or_decl_base_sptr	first_subject,
	   type_or_decl_base_sptr	second_subject,
	   diff_context_sptr	ctxt)
  : priv_(new priv(first_subject, second_subject,
		   ctxt, NO_CHANGE_CATEGORY,
		   /*reported_once=*/false,
		   /*currently_reporting=*/false))
{}

/// Flag a given diff node as being traversed.
///
/// For certain diff nodes like @ref class_diff, it's important to
/// avoid traversing the node again while it's already being
/// traversed; otherwise this leads to infinite loops.  So the
/// diff::begin_traversing() and diff::end_traversing() methods flag a
/// given node as being traversed (or not), so that
/// diff::is_traversing() can tell if the node is being traversed.
///
/// Note that traversing a node means visiting it *and* visiting its
/// children nodes.
///
/// The canonical node is marked as being traversed too.
///
/// These functions are called by the traversing code.
void
diff::begin_traversing()
{
  ABG_ASSERT(!is_traversing());
  if (priv_->canonical_diff_)
    priv_->canonical_diff_->priv_->traversing_ = true;
  priv_->traversing_ = true;
}

/// Tell if a given node is being traversed or not.
///
/// Note that traversing a node means visiting it *and* visiting its
/// children nodes.
///
/// It's the canonical node which is looked at, actually.
///
/// Please read the comments for the diff::begin_traversing() for mode
/// context.
///
/// @return true if the current instance of @diff is being traversed.
bool
diff::is_traversing() const
{
  if (priv_->canonical_diff_)
    return priv_->canonical_diff_->priv_->traversing_;
  return priv_->traversing_;
}

/// Flag a given diff node as not being traversed anymore.
///
/// Note that traversing a node means visiting it *and* visiting its
/// children nodes.
///
/// Please read the comments of the function diff::begin_traversing()
/// for mode context.
void
diff::end_traversing()
{
  ABG_ASSERT(is_traversing());
  if (priv_->canonical_diff_)
    priv_->canonical_diff_->priv_->traversing_ = false;
  priv_->traversing_ = false;
}

/// Finish the building of a given kind of a diff tree node.
///
/// For instance, certain kinds of diff tree node have specific
/// children nodes that are populated after the constructor of the
/// diff tree node has been called.  In that case, calling overloads
/// of this method ensures that these children nodes are properly
/// gathered and setup.
void
diff::finish_diff_type()
{
}

/// Getter of the first subject of the diff.
///
/// @return the first subject of the diff.
type_or_decl_base_sptr
diff::first_subject() const
{return dynamic_pointer_cast<type_or_decl_base>(priv_->first_subject_);}

/// Getter of the second subject of the diff.
///
/// @return the second subject of the diff.
type_or_decl_base_sptr
diff::second_subject() const
{return dynamic_pointer_cast<type_or_decl_base>(priv_->second_subject_);}

/// Getter for the children nodes of the current @ref diff node.
///
/// @return a vector of the children nodes.
const vector<diff*>&
diff::children_nodes() const
{return priv_->children_;}

/// Getter for the parent node of the current @ref diff node.
///
/// @return the parent node of the current @ref diff node.
const diff*
diff::parent_node() const
{return priv_->parent_;}

/// Getter for the canonical diff of the current instance of @ref
/// diff.
///
/// Note that the canonical diff node for the current instanc eof diff
/// node must have been set by invoking
/// class_diff::initialize_canonical_diff() on the current instance of
/// diff node.
///
/// @return the canonical diff node or null if none was set.
diff*
diff::get_canonical_diff() const
{return priv_->canonical_diff_;}

/// Setter for the canonical diff of the current instance of @ref
/// diff.
///
/// @param d the new canonical node to set.
void
diff::set_canonical_diff(diff * d)
{priv_->canonical_diff_ = d;}

/// Add a new child node to the vector of children nodes for the
/// current @ref diff node.
///
/// @param d the new child node to add to the children nodes.
void
diff::append_child_node(diff_sptr d)
{
  ABG_ASSERT(d);

  // Ensure 'd' is kept alive for the life time of the context of this
  // diff.
  context()->keep_diff_alive(d);

  // Add the underlying pointer of 'd' to the vector of children.
  // Note that this vector holds no reference to 'd'. This is to avoid
  // reference cycles.  The reference to 'd' is held by the context of
  // this diff, thanks to the call to context()->keep_diff_alive(d)
  // above.
  priv_->children_.push_back(d.get());

  diff_less_than_functor comp;
  std::sort(priv_->children_.begin(),
	    priv_->children_.end(),
	    comp);

  d->priv_->parent_ = this;
}

/// Getter of the context of the current diff.
///
/// @return the context of the current diff.
const diff_context_sptr
diff::context() const
{return priv_->get_context();}

/// Setter of the context of the current diff.
///
/// @param c the new context to set.
void
diff::context(diff_context_sptr c)
{priv_->ctxt_ = c;}

/// Tests if we are currently in the middle of emitting a report for
/// this diff.
///
/// @return true if we are currently emitting a report for the
/// current diff, false otherwise.
bool
diff::currently_reporting() const
{
  if (priv_->canonical_diff_)
    return priv_->canonical_diff_->priv_->currently_reporting_;
  return priv_->currently_reporting_;
}

/// Sets a flag saying if we are currently in the middle of emitting
/// a report for this diff.
///
/// @param f true if we are currently emitting a report for the
/// current diff, false otherwise.
void
diff::currently_reporting(bool f) const
{
  if (priv_->canonical_diff_)
    priv_->canonical_diff_->priv_->currently_reporting_ = f;
  priv_->currently_reporting_ = f;
}

/// Tests if a report has already been emitted for the current diff.
///
/// @return true if a report has already been emitted for the
/// current diff, false otherwise.
bool
diff::reported_once() const
{
  ABG_ASSERT(priv_->canonical_diff_);
  return priv_->canonical_diff_->priv_->reported_once_;
}

/// The generic traversing code that walks a given diff sub-tree.
///
/// Note that there is a difference between traversing a diff node and
/// visiting it.  Basically, traversing a diff node means visiting it
/// and visiting its children nodes too.  So one can visit a node
/// without traversing it.  But traversing a node without visiting it
/// is not possible.
///
/// Note that by default this traversing code visits a given class of
/// equivalence of a diff node only once.  This behaviour can been
/// changed by calling
/// diff_context::visiting_a_node_twice_is_forbidden(), but this is
/// very risky as it might create endless loops while visiting a diff
/// tree graph that has changes that refer to themselves; that is,
/// diff tree graphs with cycles.
///
/// When a diff node is encountered, the
/// diff_node_visitor::visit_begin() method is invoked on the diff
/// node first.
///
/// If the diff node has already been visited, then
/// node_visitor::visit_end() is called on it and the node traversing
/// is done; the children of the diff node are not visited in this
/// case.
///
/// If the diff node has *NOT* been visited yet, then the
/// diff_node_visitor::visit() method is invoked with it's 'pre'
/// argument set to true.  Then if the diff_node_visitor::visit()
/// returns true, then the children nodes of the diff node are
/// visited.  Otherwise, no children nodes of the diff node is
/// visited and the diff_node_visitor::visit_end() is called.

/// After the children nodes are visited (and only if they are
/// visited) the diff_node_visitor::visit() method is invoked with
/// it's 'pre' argument set to false.  And then the
/// diff_node_visitor::visit_end() is called.
///
/// @param v the entity that visits each node of the diff sub-tree.
///
/// @return true to tell the caller that all of the sub-tree could be
/// walked.  This instructs the caller to keep walking the rest of the
/// tree.  Return false otherwise.
bool
diff::traverse(diff_node_visitor& v)
{
  finish_diff_type();

  v.visit_begin(this);

  bool already_visited = false;
  if (context()->visiting_a_node_twice_is_forbidden()
      && context()->diff_has_been_visited(this))
    already_visited = true;

  bool mark_visited_nodes_as_traversed =
    !(v.get_visiting_kind() & DO_NOT_MARK_VISITED_NODES_AS_VISITED);

  if (!already_visited && !v.visit(this, /*pre=*/true))
    {
      v.visit_end(this);
      if (mark_visited_nodes_as_traversed)
	context()->mark_diff_as_visited(this);
      return false;
    }

  if (!(v.get_visiting_kind() & SKIP_CHILDREN_VISITING_KIND)
      && !is_traversing()
      && !already_visited)
    {
      begin_traversing();
      for (vector<diff*>::const_iterator i = children_nodes().begin();
	   i != children_nodes().end();
	   ++i)
	{
	  if (!(*i)->traverse(v))
	    {
	      v.visit_end(this);
	      if (mark_visited_nodes_as_traversed)
		context()->mark_diff_as_visited(this);
	      end_traversing();
	      return false;
	    }
	}
      end_traversing();
    }

  if (!v.visit(this, /*pref=*/false))
    {
      v.visit_end(this);
      if (mark_visited_nodes_as_traversed)
	context()->mark_diff_as_visited(this);
      return false;
    }

  v.visit_end(this);
  if (!already_visited && mark_visited_nodes_as_traversed)
    context()->mark_diff_as_visited(this);

  return true;
}

/// Sets a flag saying if a report has already been emitted for the
/// current diff.
///
/// @param f true if a report has already been emitted for the
/// current diff, false otherwise.
void
diff::reported_once(bool f) const
{
  ABG_ASSERT(priv_->canonical_diff_);
  priv_->canonical_diff_->priv_->reported_once_ = f;
  priv_->reported_once_ = f;
}

/// Getter for the local category of the current diff tree node.
///
/// The local category represents the set of categories of a diff
/// node, not taking in account the categories inherited from its
/// children nodes.
///
/// @return the local category of the current diff tree node.
diff_category
diff::get_local_category() const
{return priv_->local_category_;}

/// Getter of the category of the class of equivalence of the current
/// diff tree node.
///
/// That is, if the current diff tree node has a canonical node,
/// return the category of that canonical node.  Otherwise, return the
/// category of the current node.
///
/// @return the category of the class of equivalence of the current
/// tree node.
diff_category
diff::get_class_of_equiv_category() const
{
  diff* canonical = get_canonical_diff();
  return canonical ? canonical->get_category() : get_category();
}

/// Getter for the category of the current diff tree node.
///
/// This category represents the union of the local category and the
/// categories inherited from the children diff nodes.
///
/// @return the category of the current diff tree node.
diff_category
diff::get_category() const
{return priv_->category_;}

/// Adds the current diff tree node to an additional set of
/// categories.  Note that the categories include thoses inherited
/// from the children nodes of this diff node.
///
/// @param c a bit-map representing the set of categories to add the
/// current diff tree node to.
///
/// @return the resulting bit-map representing the categories this
/// current diff tree node belongs to, including those inherited from
/// its children nodes.
diff_category
diff::add_to_category(diff_category c)
{
  priv_->category_ = priv_->category_ | c;
  return priv_->category_;
}

/// Adds the current diff tree node to the categories resulting from
/// the local changes of the current diff node.
///
/// @param c a bit-map representing the set of categories to add the
/// current diff tree node to.
///
/// @return the resulting bit-map representing the categories this
/// current diff tree node belongs to.
diff_category
diff::add_to_local_category(diff_category c)
{
  priv_->local_category_ = priv_->local_category_ | c;
  return priv_->local_category_;
}

/// Adds the current diff tree node to the categories resulting from
/// the local and inherited changes of the current diff node.
///
/// @param c a bit-map representing the set of categories to add the
/// current diff tree node to.
void
diff::add_to_local_and_inherited_categories(diff_category c)
{
  add_to_local_category(c);
  add_to_category(c);
}

/// Remove the current diff tree node from an a existing sef of
/// categories.  The categories include those inherited from the
/// children nodes of the current diff node.
///
/// @param c a bit-map representing the set of categories to add the
/// current diff tree node to.
///
/// @return the resulting bit-map representing the categories this
/// current diff tree onde belongs to, including the categories
/// inherited from the children nodes of the current diff node.
diff_category
diff::remove_from_category(diff_category c)
{
  priv_->category_ = priv_->category_ & ~c;
  return priv_->category_;
}

/// Remove the current diff tree node from the categories resulting
/// from the local changes.
///
/// @param c a bit-map representing the set of categories to add the
/// current diff tree node to.
///
/// @return the resulting bit-map representing the categories this
/// current diff tree onde belongs to.
diff_category
diff::remove_from_local_category(diff_category c)
{
  priv_->local_category_ = priv_->local_category_ & ~c;
  return priv_->local_category_;
}

/// Set the category of the current @ref diff node.  This category
/// includes the categories inherited from the children nodes of the
/// current diff node.
///
/// @param c the new category for the current diff node.
void
diff::set_category(diff_category c)
{priv_->category_ = c;}

/// Set the local category of the current @ref diff node.
///
/// @param c the new category for the current diff node.
void
diff::set_local_category(diff_category c)
{priv_->local_category_ = c;}

/// Test if this diff tree node is to be filtered out for reporting
/// purposes.
///
/// The function tests if the categories of the diff tree node are
/// "forbidden" by the context or not.
///
/// @return true iff the current diff node should NOT be reported.
bool
diff::is_filtered_out() const
{
  if (diff * canonical = get_canonical_diff())
    if (canonical->get_category() & SUPPRESSED_CATEGORY
	|| canonical->get_category() & PRIVATE_TYPE_CATEGORY)
      // The canonical type was suppressed either by a user-provided
      // suppression specification or by a "private-type" suppression
      // specification..  This means all the class of equivalence of
      // that canonical type was suppressed.  So this node should be
      // suppressed too.
      return true;
  return priv_->is_filtered_out(get_category());
}

/// Test if this diff tree node is to be filtered out for reporting
/// purposes, but by considering only the categories that were *NOT*
/// inherited from its children nodes.
///
/// The function tests if the local categories of the diff tree node
/// are "forbidden" by the context or not.
///
/// @return true iff the current diff node should NOT be reported,
/// with respect to its local categories.
bool
diff::is_filtered_out_wrt_non_inherited_categories() const
{return priv_->is_filtered_out(get_local_category());}

/// Test if the current diff node has been suppressed by a
/// user-provided suppression specification.
///
/// @return true if the current diff node has been suppressed by a
/// user-provided suppression list.
bool
diff::is_suppressed() const
{
  bool is_private = false;
  return is_suppressed(is_private);
}

/// Test if the current diff node has been suppressed by a
/// user-provided suppression specification or by an auto-generated
/// "private type" suppression specification.
///
/// Note that private type suppressions are auto-generated from the
/// path to where public headers are, as given by the user.
///
/// @param is_private_type out parameter if the current diff node was
/// suppressed because it's a private type then this parameter is set
/// to true.
///
/// @return true if the current diff node has been suppressed by a
/// user-provided suppression list.
bool
diff::is_suppressed(bool &is_private_type) const
{
  const suppressions_type& suppressions = context()->suppressions();
  for (suppressions_type::const_iterator i = suppressions.begin();
       i != suppressions.end();
       ++i)
    {
      if ((*i)->suppresses_diff(this))
	{
	  if (is_private_type_suppr_spec(*i))
	    is_private_type = true;
	  return true;
	}
    }
  return false;
}

/// Test if this diff tree node should be reported.
///
/// @return true iff the current node should be reported.
bool
diff::to_be_reported() const
{
  if (has_changes() && !is_filtered_out())
    return true;
  return false;
}

/// Test if this diff tree node should be reported when considering
/// the categories that were *NOT* inherited from its children nodes.
///
/// @return true iff the current node should be reported.
bool
diff::has_local_changes_to_be_reported() const
{
  if (has_local_changes()
      && !is_filtered_out_wrt_non_inherited_categories())
    return true;
  return false;
}

/// Get a pretty representation of the current @ref diff node.
///
/// This is suitable for e.g. emitting debugging traces for the diff
/// tree nodes.
///
/// @return the pretty representation of the diff node.
const string&
diff::get_pretty_representation() const
{
  if (priv_->pretty_representation_.empty())
    priv_->pretty_representation_ = "empty_diff";
  return priv_->pretty_representation_;
}

/// Default implementation of the hierachy chaining virtual function.
///
/// There are several types of diff nodes that have logical children
/// nodes; for instance, a typedef_diff has the diff of the underlying
/// type as a child node.  A var_diff has the diff of the types of the
/// variables as a child node, etc.
///
/// But because the @ref diff base has a generic representation for
/// children nodes of the all the types of @ref diff nodes (regardless
/// of the specific most-derived type of diff node) that one can get
/// using the method diff::children_nodes(), one need to populate that
/// vector of children node.
///
/// Populating that vector of children node is done by this function;
/// it must be overloaded by each most-derived type of diff node that
/// extends the @ref diff type.
void
diff::chain_into_hierarchy()
{}

// </diff stuff>

// <type_diff_base stuff>

type_diff_base::type_diff_base(type_base_sptr	first_subject,
			       type_base_sptr	second_subject,
			       diff_context_sptr	ctxt)
  : diff(first_subject, second_subject, ctxt),
    priv_(new priv)
{}

type_diff_base::~type_diff_base()
{}
// </type_diff_base stuff>

// <decl_diff_base stuff>

/// Constructor of @ref decl_diff_base.
///
/// @param first_subject the first subject of the diff.
///
/// @param second_subject the second subject of the diff.
///
/// @param ctxt the context of the diff.  This object must stay alive
/// at least during the life time of the current instance of @ref
/// decl_diff_base, otherwise, memory corruption issues occur.
decl_diff_base::decl_diff_base(decl_base_sptr	first_subject,
			       decl_base_sptr	second_subject,
			       diff_context_sptr	ctxt)
  : diff(first_subject, second_subject, ctxt),
    priv_(new priv)
{}

decl_diff_base::~decl_diff_base()
{}

// </decl_diff_base stuff>

// <distinct_diff stuff>

/// @return a pretty representation for the @ref distinct_diff node.
const string&
distinct_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "distinct_diff[";
      if (first_subject())
	o << first_subject()->get_pretty_representation();
      else
	o << "null";
      o << ", ";
      if (second_subject())
	o << second_subject()->get_pretty_representation() ;
      else
	o << "null";
      o << "]" ;
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @distinct_diff.
///
/// The children nodes can then later be retrieved using
/// diff::children_nodes().
void
distinct_diff::chain_into_hierarchy()
{
  ABG_ASSERT(entities_are_of_distinct_kinds(first(), second()));

  if (diff_sptr d = compatible_child_diff())
    append_child_node(d);
}

/// Constructor for @ref distinct_diff.
///
/// Note that the two entities considered for the diff (and passed in
/// parameter) must be of different kinds.
///
/// @param first the first entity to consider for the diff.
///
/// @param second the second entity to consider for the diff.
///
/// @param ctxt the context of the diff.  Note that this context
/// object must stay alive at least during the life time of the
/// current instance of @ref distinct_diff.  Otherwise memory
/// corruption issues occur.
distinct_diff::distinct_diff(type_or_decl_base_sptr first,
			     type_or_decl_base_sptr second,
			     diff_context_sptr ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)
{ABG_ASSERT(entities_are_of_distinct_kinds(first, second));}

/// Finish building the current instance of @ref distinct_diff.
void
distinct_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;

  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first subject of the diff.
///
/// @return the first subject of the diff.
const type_or_decl_base_sptr
distinct_diff::first() const
{return first_subject();}

/// Getter for the second subject of the diff.
///
/// @return the second subject of the diff.
const type_or_decl_base_sptr
distinct_diff::second() const
{return second_subject();}

/// Getter for the child diff of this distinct_diff instance.
///
/// When a distinct_diff has two subjects that are different but
/// compatible, then the distinct_diff instance has a child diff node
/// (named the compatible child diff) that is the diff between the two
/// subjects stripped from their typedefs.  Otherwise, the compatible
/// child diff is nul.
///
/// Note that two diff subjects (that compare different) are
/// considered compatible if stripping typedefs out of them makes them
/// comparing equal.
///
/// @return the compatible child diff node, if any.  Otherwise, null.
const diff_sptr
distinct_diff::compatible_child_diff() const
{
  if (!priv_->compatible_child_diff)
    {
      type_base_sptr fs = strip_typedef(is_type(first())),
	ss = strip_typedef(is_type(second()));

      if (fs && ss
	  && !entities_are_of_distinct_kinds(get_type_declaration(fs),
					     get_type_declaration(ss)))
	priv_->compatible_child_diff = compute_diff(get_type_declaration(fs),
						    get_type_declaration(ss),
						    context());
    }
  return priv_->compatible_child_diff;
}

/// Test if the two arguments are of different kind, or that are both
/// NULL.
///
/// @param first the first argument to test for similarity in kind.
///
/// @param second the second argument to test for similarity in kind.
///
/// @return true iff the two arguments are of different kind.
bool
distinct_diff::entities_are_of_distinct_kinds(type_or_decl_base_sptr first,
					      type_or_decl_base_sptr second)
{
  if (!!first != !!second)
    return true;
  if (!first && !second)
    // We do consider diffs of two empty decls as a diff of distinct
    // kinds, for now.
    return true;
  if (first == second)
    return false;

  const type_or_decl_base &f = *first, &s = *second;
  return typeid(f) != typeid(s);
}

/// @return true if the two subjects of the diff are different, false
/// otherwise.
bool
distinct_diff::has_changes() const
{return first() != second();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
distinct_diff::has_local_changes() const
{
  // Changes on a distinct_diff are all local.
  if (has_changes())
    return LOCAL_TYPE_CHANGE_KIND;
  return NO_CHANGE_KIND;
}

/// Emit a report about the current diff instance.
///
/// @param out the output stream to send the diff report to.
///
/// @param indent the indentation string to use in the report.
void
distinct_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Try to diff entities that are of distinct kinds.
///
/// @param first the first entity to consider for the diff.
///
/// @param second the second entity to consider for the diff.
///
/// @param ctxt the context of the diff.
///
/// @return a non-null diff if a diff object could be built, null
/// otherwise.
distinct_diff_sptr
compute_diff_for_distinct_kinds(const type_or_decl_base_sptr first,
				const type_or_decl_base_sptr second,
				diff_context_sptr ctxt)
{
  if (!distinct_diff::entities_are_of_distinct_kinds(first, second))
    return distinct_diff_sptr();

  distinct_diff_sptr result(new distinct_diff(first, second, ctxt));

  ctxt->initialize_canonical_diff(result);

  return result;
}

/// </distinct_diff stuff>

/// Try to compute a diff on two instances of DiffType representation.
///
/// The function template performs the diff if and only if the decl
/// representations are of a DiffType.
///
/// @tparm DiffType the type of instances to diff.
///
/// @param first the first representation of decl to consider in the
/// diff computation.
///
/// @param second the second representation of decl to consider in the
/// diff computation.
///
/// @param ctxt the diff context to use.
///
///@return the diff of the two types @p first and @p second if and
///only if they represent the parametrized type DiffType.  Otherwise,
///returns a NULL pointer value.
template<typename DiffType>
diff_sptr
try_to_diff(const type_or_decl_base_sptr first,
	    const type_or_decl_base_sptr second,
	    diff_context_sptr ctxt)
{
  if (shared_ptr<DiffType> f =
      dynamic_pointer_cast<DiffType>(first))
    {
      shared_ptr<DiffType> s =
	dynamic_pointer_cast<DiffType>(second);
      if (!s)
	return diff_sptr();
      return compute_diff(f, s, ctxt);
    }
  return diff_sptr();
}


/// This is a specialization of @ref try_to_diff() template to diff
/// instances of @ref class_decl.
///
/// @param first the first representation of decl to consider in the
/// diff computation.
///
/// @param second the second representation of decl to consider in the
/// diff computation.
///
/// @param ctxt the diff context to use.
template<>
diff_sptr
try_to_diff<class_decl>(const type_or_decl_base_sptr first,
			const type_or_decl_base_sptr second,
			diff_context_sptr ctxt)
{
  if (class_decl_sptr f =
      dynamic_pointer_cast<class_decl>(first))
    {
      class_decl_sptr s = dynamic_pointer_cast<class_decl>(second);
      if (!s)
	return diff_sptr();

      if (f->get_is_declaration_only())
	{
	  class_decl_sptr f2 =
	    is_class_type (f->get_definition_of_declaration());
	  if (f2)
	    f = f2;
	}
      if (s->get_is_declaration_only())
	{
	  class_decl_sptr s2 =
	    is_class_type(s->get_definition_of_declaration());
	  if (s2)
	    s = s2;
	}
      return compute_diff(f, s, ctxt);
    }
  return diff_sptr();
}

/// Try to diff entities that are of distinct kinds.
///
/// @param first the first entity to consider for the diff.
///
/// @param second the second entity to consider for the diff.
///
/// @param ctxt the context of the diff.
///
/// @return a non-null diff if a diff object could be built, null
/// otherwise.
static diff_sptr
try_to_diff_distinct_kinds(const type_or_decl_base_sptr first,
			   const type_or_decl_base_sptr second,
			   diff_context_sptr ctxt)
{return compute_diff_for_distinct_kinds(first, second, ctxt);}

/// Compute the difference between two types.
///
/// The function considers every possible types known to libabigail
/// and runs the appropriate diff function on them.
///
/// Whenever a new kind of type decl is supported by abigail, if we
/// want to be able to diff two instances of it, we need to update
/// this function to support it.
///
/// @param first the first type decl to consider for the diff
///
/// @param second the second type decl to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff.  It's a pointer to a descendent of
/// abigail::comparison::diff.
static diff_sptr
compute_diff_for_types(const type_or_decl_base_sptr& first,
		       const type_or_decl_base_sptr& second,
		       const diff_context_sptr& ctxt)
{
  type_or_decl_base_sptr f = first;
  type_or_decl_base_sptr s = second;

  // Look through no-op qualified types.
  f = look_through_no_op_qualified_type(is_type(f));
  s = look_through_no_op_qualified_type(is_type(s));

  diff_sptr d;

  ((d = try_to_diff<type_decl>(f, s, ctxt))
   ||(d = try_to_diff<enum_type_decl>(f, s, ctxt))
   ||(d = try_to_diff<union_decl>(f, s,ctxt))
   ||(d = try_to_diff<class_decl>(f, s,ctxt))
   ||(d = try_to_diff<pointer_type_def>(f, s, ctxt))
   ||(d = try_to_diff<reference_type_def>(f, s, ctxt))
   ||(d = try_to_diff<array_type_def>(f, s, ctxt))
   ||(d = try_to_diff<qualified_type_def>(f, s, ctxt))
   ||(d = try_to_diff<typedef_decl>(f, s, ctxt))
   ||(d = try_to_diff<function_type>(f, s, ctxt))
   ||(d = try_to_diff_distinct_kinds(f, s, ctxt)));

  ABG_ASSERT(d);

  return d;
}

diff_category
operator|(diff_category c1, diff_category c2)
{return static_cast<diff_category>(static_cast<unsigned>(c1)
				   | static_cast<unsigned>(c2));}

diff_category&
operator|=(diff_category& c1, diff_category c2)
{
  c1 = c1 | c2;
  return c1;
}

diff_category&
operator&=(diff_category& c1, diff_category c2)
{
  c1 = c1 & c2;
  return c1;
}

diff_category
operator^(diff_category c1, diff_category c2)
{return static_cast<diff_category>(static_cast<unsigned>(c1)
				   ^ static_cast<unsigned>(c2));}

diff_category
operator&(diff_category c1, diff_category c2)
{return static_cast<diff_category>(static_cast<unsigned>(c1)
				   & static_cast<unsigned>(c2));}

diff_category
operator~(diff_category c)
{return static_cast<diff_category>(~static_cast<unsigned>(c));}


/// Getter of a bitmap made of the set of change categories that are
/// considered harmless.
///
/// @return the bitmap made of the set of change categories that are
/// considered harmless.
diff_category
get_default_harmless_categories_bitmap()
{
  return (abigail::comparison::ACCESS_CHANGE_CATEGORY
	  | abigail::comparison::COMPATIBLE_TYPE_CHANGE_CATEGORY
	  | abigail::comparison::HARMLESS_DECL_NAME_CHANGE_CATEGORY
	  | abigail::comparison::NON_VIRT_MEM_FUN_CHANGE_CATEGORY
	  | abigail::comparison::STATIC_DATA_MEMBER_CHANGE_CATEGORY
	  | abigail::comparison::HARMLESS_ENUM_CHANGE_CATEGORY
	  | abigail::comparison::HARMLESS_SYMBOL_ALIAS_CHANGE_CATEGORY
	  | abigail::comparison::HARMLESS_UNION_CHANGE_CATEGORY
	  | abigail::comparison::HARMLESS_DATA_MEMBER_CHANGE_CATEGORY
	  | abigail::comparison::TYPE_DECL_ONLY_DEF_CHANGE_CATEGORY
	  | abigail::comparison::FN_PARM_TYPE_TOP_CV_CHANGE_CATEGORY
	  | abigail::comparison::FN_PARM_TYPE_CV_CHANGE_CATEGORY
	  | abigail::comparison::FN_RETURN_TYPE_CV_CHANGE_CATEGORY
	  | abigail::comparison::VAR_TYPE_CV_CHANGE_CATEGORY
	  | abigail::comparison::VOID_PTR_TO_PTR_CHANGE_CATEGORY
	  | abigail::comparison::BENIGN_INFINITE_ARRAY_CHANGE_CATEGORY);
}

/// Getter of a bitmap made of the set of change categories that are
/// considered harmful.
///
/// @return the bitmap made of the set of change categories that are
/// considered harmful.
diff_category
get_default_harmful_categories_bitmap()
{
  return (abigail::comparison::SIZE_OR_OFFSET_CHANGE_CATEGORY
	  | abigail::comparison::VIRTUAL_MEMBER_CHANGE_CATEGORY
	  | abigail::comparison::FN_PARM_ADD_REMOVE_CHANGE_CATEGORY);
}

/// Serialize an instance of @ref diff_category to an output stream.
///
/// @param o the output stream to serialize @p c to.
///
/// @param c the instance of diff_category to serialize.
///
/// @return the output stream to serialize @p c to.
ostream&
operator<<(ostream& o, diff_category c)
{
  bool emitted_a_category = false;

  if (c == NO_CHANGE_CATEGORY)
    {
      o << "NO_CHANGE_CATEGORY";
      emitted_a_category = true;
    }

  if (c & ACCESS_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "ACCESS_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & COMPATIBLE_TYPE_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "COMPATIBLE_TYPE_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & HARMLESS_DECL_NAME_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "HARMLESS_DECL_NAME_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "NON_VIRT_MEM_FUN_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & STATIC_DATA_MEMBER_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "STATIC_DATA_MEMBER_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & HARMLESS_ENUM_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "HARMLESS_ENUM_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

    if (c & HARMLESS_DATA_MEMBER_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "HARMLESS_DATA_MEMBER_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & HARMLESS_SYMBOL_ALIAS_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "HARMLESS_SYMBOL_ALIAS_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & HARMLESS_UNION_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "HARMLESS_UNION_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & SUPPRESSED_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "SUPPRESSED_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & PRIVATE_TYPE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "PRIVATE_TYPE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & SIZE_OR_OFFSET_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "SIZE_OR_OFFSET_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & VIRTUAL_MEMBER_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "VIRTUAL_MEMBER_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & REDUNDANT_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "REDUNDANT_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & TYPE_DECL_ONLY_DEF_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "TYPE_DECL_ONLY_DEF_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & FN_PARM_TYPE_TOP_CV_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "FN_PARM_TYPE_TOP_CV_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & FN_PARM_TYPE_CV_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "FN_PARM_TYPE_CV_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & FN_RETURN_TYPE_CV_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "FN_RETURN_TYPE_CV_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

    if (c & FN_PARM_ADD_REMOVE_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "FN_PARM_ADD_REMOVE_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & VAR_TYPE_CV_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "VAR_TYPE_CV_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & VOID_PTR_TO_PTR_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "VOID_PTR_TO_PTR_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & BENIGN_INFINITE_ARRAY_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "BENIGN_INFINITE_ARRAY_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  return o;
}

/// Compute the difference between two decls.
///
/// The function consider every possible decls known to libabigail and
/// runs the appropriate diff function on them.
///
/// Whenever a new kind of non-type decl is supported by abigail, if
/// we want to be able to diff two instances of it, we need to update
/// this function to support it.
///
/// @param first the first decl to consider for the diff
///
/// @param second the second decl to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff.
static diff_sptr
compute_diff_for_decls(const decl_base_sptr first,
		       const decl_base_sptr second,
		       diff_context_sptr ctxt)
{

  diff_sptr d;

  ((d = try_to_diff<function_decl>(first, second, ctxt))
   || (d = try_to_diff<var_decl>(first, second, ctxt))
   || (d = try_to_diff_distinct_kinds(first, second, ctxt)));

   ABG_ASSERT(d);

  return d;
}

/// Compute the difference between two decls.  The decls can represent
/// either type declarations, or non-type declaration.
///
/// Note that the two decls must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff, or NULL if the diff could not be
/// computed.
diff_sptr
compute_diff(const decl_base_sptr	first,
	     const decl_base_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (!first || !second)
    return diff_sptr();

  ABG_ASSERT(first->get_environment() == second->get_environment());

  diff_sptr d;
  if (is_type(first) && is_type(second))
    d = compute_diff_for_types(first, second, ctxt);
  else
    d = compute_diff_for_decls(first, second, ctxt);
  ABG_ASSERT(d);
  return d;
}

/// Compute the difference between two types.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff, or NULL if the diff couldn't be
/// computed.
diff_sptr
compute_diff(const type_base_sptr	first,
	     const type_base_sptr	second,
	     diff_context_sptr		ctxt)
{
  decl_base_sptr f = get_type_declaration(first),
    s = get_type_declaration(second);

  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  diff_sptr d = compute_diff_for_types(f,s, ctxt);
  ABG_ASSERT(d);
  return d;
}

/// Get a copy of the pretty representation of a diff node.
///
/// @param d the diff node to consider.
///
/// @return the pretty representation string.
string
get_pretty_representation(diff* d)
{
  if (!d)
    return "";
  string prefix= "diff of ";
  return prefix + get_pretty_representation(d->first_subject());
}

// <var_diff stuff>

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref var_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
var_diff::chain_into_hierarchy()
{append_child_node(type_diff());}

/// @return the pretty representation for this current instance of
/// @ref var_diff.
const string&
var_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "var_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}
/// Constructor for @ref var_diff.
///
/// @param first the first instance of @ref var_decl to consider in
/// the diff.
///
/// @param second the second instance of @ref var_decl to consider in
/// the diff.
///
/// @param type_diff the diff between types of the instances of
/// var_decl.
///
/// @param ctxt the diff context to use.
var_diff::var_diff(var_decl_sptr	first,
		   var_decl_sptr	second,
		   diff_sptr		type_diff,
		   diff_context_sptr	ctxt)
  : decl_diff_base(first, second, ctxt),
    priv_(new priv)
{priv_->type_diff_ = type_diff;}

/// Finish building the current instance of @ref var_diff.
void
var_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first @ref var_decl of the diff.
///
/// @return the first @ref var_decl of the diff.
var_decl_sptr
var_diff::first_var() const
{return dynamic_pointer_cast<var_decl>(first_subject());}

/// Getter for the second @ref var_decl of the diff.
///
/// @return the second @ref var_decl of the diff.
var_decl_sptr
var_diff::second_var() const
{return dynamic_pointer_cast<var_decl>(second_subject());}

/// Getter for the diff of the types of the instances of @ref
/// var_decl.
///
/// @return the diff of the types of the instances of @ref var_decl.
diff_sptr
var_diff::type_diff() const
{
  if (diff_sptr result = priv_->type_diff_.lock())
    return result;
  else
    {
      result = compute_diff(first_var()->get_type(),
			    second_var()->get_type(),
			    context());
      context()->keep_diff_alive(result);
      priv_->type_diff_ = result;
      return result;
    }
}

/// Return true iff the diff node has a change.
///
/// @return true iff the diff node has a change.
bool
var_diff::has_changes() const
{return *first_var() != *second_var();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
var_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_var(), *second_var(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
var_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the diff between two instances of @ref var_decl.
///
/// Note that the two decls must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first @ref var_decl to consider for the diff.
///
/// @param second the second @ref var_decl to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff between the two @ref var_decl.
var_diff_sptr
compute_diff(const var_decl_sptr	first,
	     const var_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  var_diff_sptr d(new var_diff(first, second, diff_sptr(), ctxt));
  ctxt->initialize_canonical_diff(d);

  return d;
}

// </var_diff stuff>

// <pointer_type_def stuff>

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref pointer_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
pointer_diff::chain_into_hierarchy()
{append_child_node(underlying_type_diff());}

/// Constructor for a pointer_diff.
///
/// @param first the first pointer to consider for the diff.
///
/// @param second the secon pointer to consider for the diff.
///
/// @param ctxt the diff context to use.
pointer_diff::pointer_diff(pointer_type_def_sptr	first,
			   pointer_type_def_sptr	second,
			   diff_sptr			underlying,
			   diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv(underlying))
{}

/// Finish building the current instance of @ref pointer_diff.
void
pointer_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first subject of a pointer diff
///
/// @return the first pointer considered in this pointer diff.
const pointer_type_def_sptr
pointer_diff::first_pointer() const
{return dynamic_pointer_cast<pointer_type_def>(first_subject());}

/// Getter for the second subject of a pointer diff
///
/// @return the second pointer considered in this pointer diff.
const pointer_type_def_sptr
pointer_diff::second_pointer() const
{return dynamic_pointer_cast<pointer_type_def>(second_subject());}

/// @return the pretty represenation for the current instance of @ref
/// pointer_diff.
const string&
pointer_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "pointer_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
pointer_diff::has_changes() const
{return first_pointer() != second_pointer();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
pointer_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_pointer(), *second_pointer(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Getter for the diff between the pointed-to types of the pointers
/// of this diff.
///
/// @return the diff between the pointed-to types.
diff_sptr
pointer_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// Setter for the diff between the pointed-to types of the pointers
/// of this diff.
///
/// @param d the new diff between the pointed-to types of the pointers
/// of this diff.
void
pointer_diff::underlying_type_diff(const diff_sptr d)
{priv_->underlying_type_diff_ = d;}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
pointer_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the diff between between two pointers.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the pointer to consider for the diff.
///
/// @param second the pointer to consider for the diff.
///
/// @return the resulting diff between the two pointers.
///
/// @param ctxt the diff context to use.
pointer_diff_sptr
compute_diff(pointer_type_def_sptr	first,
	     pointer_type_def_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  diff_sptr d = compute_diff_for_types(first->get_pointed_to_type(),
				       second->get_pointed_to_type(),
				       ctxt);
  pointer_diff_sptr result(new pointer_diff(first, second, d, ctxt));
  ctxt->initialize_canonical_diff(result);

  return result;
}

// </pointer_type_def>

// <array_type_def>

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref array_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
array_diff::chain_into_hierarchy()
{append_child_node(element_type_diff());}

/// Constructor for array_diff
///
/// @param first the first array_type of the diff.
///
/// @param second the second array_type of the diff.
///
/// @param element_type_diff the diff between the two array element
/// types.
///
/// @param ctxt the diff context to use.
array_diff::array_diff(const array_type_def_sptr	first,
		       const array_type_def_sptr	second,
		       diff_sptr			element_type_diff,
		       diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv(element_type_diff))
{}

/// Finish building the current instance of @ref array_diff.
void
array_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first array of the diff.
///
/// @return the first array of the diff.
const array_type_def_sptr
array_diff::first_array() const
{return dynamic_pointer_cast<array_type_def>(first_subject());}

/// Getter for the second array of the diff.
///
/// @return for the second array of the diff.
const array_type_def_sptr
array_diff::second_array() const
{return dynamic_pointer_cast<array_type_def>(second_subject());}

/// Getter for the diff between the two types of array elements.
///
/// @return the diff between the two types of array elements.
const diff_sptr&
array_diff::element_type_diff() const
{return priv_->element_type_diff_;}

/// Setter for the diff between the two array element types.
///
/// @param d the new diff betweend the two array element types.
void
array_diff::element_type_diff(diff_sptr d)
{priv_->element_type_diff_ = d;}

/// @return the pretty representation for the current instance of @ref
/// array_diff.
const string&
array_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "array_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
array_diff::has_changes() const
{
  bool l = false;

  //  the array element types match check for differing dimensions
  //  etc...
  array_type_def_sptr
    f = dynamic_pointer_cast<array_type_def>(first_subject()),
    s = dynamic_pointer_cast<array_type_def>(second_subject());

  if (f->get_name() != s->get_name())
    l |= true;
  if (f->get_size_in_bits() != s->get_size_in_bits())
    l |= true;
  if (f->get_alignment_in_bits() != s->get_alignment_in_bits())
    l |= true;

  l |=  element_type_diff()
    ? element_type_diff()->has_changes()
    : false;

  return l;
}


/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
array_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_array(), *second_array(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize the dif to.
///
/// @param indent the string to use for indenting the report.
void
array_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the diff between two arrays.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first array to consider for the diff.
///
/// @param second the second array to consider for the diff.
///
/// @param ctxt the diff context to use.
array_diff_sptr
compute_diff(array_type_def_sptr	first,
	     array_type_def_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  diff_sptr d = compute_diff_for_types(first->get_element_type(),
				       second->get_element_type(),
				       ctxt);
  array_diff_sptr result(new array_diff(first, second, d, ctxt));
  ctxt->initialize_canonical_diff(result);
  return result;
}
// </array_type_def>

// <reference_type_def>

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref reference_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
reference_diff::chain_into_hierarchy()
{append_child_node(underlying_type_diff());}

/// Constructor for reference_diff
///
/// @param first the first reference_type of the diff.
///
/// @param second the second reference_type of the diff.
///
/// @param ctxt the diff context to use.
reference_diff::reference_diff(const reference_type_def_sptr	first,
			       const reference_type_def_sptr	second,
			       diff_sptr			underlying,
			       diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
	priv_(new priv(underlying))
{}

/// Finish building the current instance of @ref reference_diff.
void
reference_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first reference of the diff.
///
/// @return the first reference of the diff.
reference_type_def_sptr
reference_diff::first_reference() const
{return dynamic_pointer_cast<reference_type_def>(first_subject());}

/// Getter for the second reference of the diff.
///
/// @return for the second reference of the diff.
reference_type_def_sptr
reference_diff::second_reference() const
{return dynamic_pointer_cast<reference_type_def>(second_subject());}


/// Getter for the diff between the two referred-to types.
///
/// @return the diff between the two referred-to types.
const diff_sptr&
reference_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// Setter for the diff between the two referred-to types.
///
/// @param d the new diff betweend the two referred-to types.
diff_sptr&
reference_diff::underlying_type_diff(diff_sptr d)
{
  priv_->underlying_type_diff_ = d;
  return priv_->underlying_type_diff_;
}

/// @return the pretty representation for the current instance of @ref
/// reference_diff.
const string&
reference_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "reference_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
reference_diff::has_changes() const
{
  return first_reference() != second_reference();
}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
reference_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_reference(), *second_reference(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize the dif to.
///
/// @param indent the string to use for indenting the report.
void
reference_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the diff between two references.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first reference to consider for the diff.
///
/// @param second the second reference to consider for the diff.
///
/// @param ctxt the diff context to use.
reference_diff_sptr
compute_diff(reference_type_def_sptr	first,
	     reference_type_def_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  diff_sptr d = compute_diff_for_types(first->get_pointed_to_type(),
				       second->get_pointed_to_type(),
				       ctxt);
  reference_diff_sptr result(new reference_diff(first, second, d, ctxt));
  ctxt->initialize_canonical_diff(result);
  return result;
}
// </reference_type_def>

// <qualified_type_diff stuff>

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref qualified_type_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
qualified_type_diff::chain_into_hierarchy()
{append_child_node(leaf_underlying_type_diff());}

/// Constructor for qualified_type_diff.
///
/// @param first the first qualified type of the diff.
///
/// @param second the second qualified type of the diff.
///
/// @param ctxt the diff context to use.
qualified_type_diff::qualified_type_diff(qualified_type_def_sptr	first,
					 qualified_type_def_sptr	second,
					 diff_sptr			under,
					 diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv(under))
{}

/// Finish building the current instance of @ref qualified_type_diff.
void
qualified_type_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first qualified type of the diff.
///
/// @return the first qualified type of the diff.
const qualified_type_def_sptr
qualified_type_diff::first_qualified_type() const
{return dynamic_pointer_cast<qualified_type_def>(first_subject());}

/// Getter for the second qualified type of the diff.
///
/// @return the second qualified type of the diff.
const qualified_type_def_sptr
qualified_type_diff::second_qualified_type() const
{return dynamic_pointer_cast<qualified_type_def>(second_subject());}

/// Getter for the diff between the underlying types of the two
/// qualified types.
///
/// @return the diff between the underlying types of the two qualified
/// types.
diff_sptr
qualified_type_diff::underlying_type_diff() const
{return priv_->underlying_type_diff;}

/// Getter for the diff between the most underlying non-qualified
/// types of two qualified types.
///
/// @return the diff between the most underlying non-qualified types
/// of two qualified types.
diff_sptr
qualified_type_diff::leaf_underlying_type_diff() const
{
  if (!priv_->leaf_underlying_type_diff)
    priv_->leaf_underlying_type_diff
      = compute_diff_for_types(get_leaf_type(first_qualified_type()),
			       get_leaf_type(second_qualified_type()),
			       context());

  return priv_->leaf_underlying_type_diff;
}

/// Setter for the diff between the underlying types of the two
/// qualified types.
///
/// @return the diff between the underlying types of the two qualified
/// types.
void
qualified_type_diff::underlying_type_diff(const diff_sptr d)
{priv_->underlying_type_diff = d;}

/// @return the pretty representation of the current instance of @ref
/// qualified_type_diff.
const string&
qualified_type_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "qualified_type_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
qualified_type_diff::has_changes() const
{return first_qualified_type() != second_qualified_type();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
qualified_type_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_qualified_type(), *second_qualified_type(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize to.
///
/// @param indent the string to use to indent the lines of the report.
void
qualified_type_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the diff between two qualified types.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first qualified type to consider for the diff.
///
/// @param second the second qualified type to consider for the diff.
///
/// @param ctxt the diff context to use.
qualified_type_diff_sptr
compute_diff(const qualified_type_def_sptr	first,
	     const qualified_type_def_sptr	second,
	     diff_context_sptr			ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  diff_sptr d = compute_diff_for_types(first->get_underlying_type(),
				       second->get_underlying_type(),
				       ctxt);
  qualified_type_diff_sptr result(new qualified_type_diff(first, second,
							  d, ctxt));
  ctxt->initialize_canonical_diff(result);
  return result;
}

// </qualified_type_diff stuff>

// <enum_diff stuff>

/// Clear the lookup tables useful for reporting an enum_diff.
///
/// This function must be updated each time a lookup table is added or
/// removed from the class_diff::priv.
void
enum_diff::clear_lookup_tables()
{
  priv_->deleted_enumerators_.clear();
  priv_->inserted_enumerators_.clear();
  priv_->changed_enumerators_.clear();
}

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
enum_diff::lookup_tables_empty() const
{
  return (priv_->deleted_enumerators_.empty()
	  && priv_->inserted_enumerators_.empty()
	  && priv_->changed_enumerators_.empty());
}

/// If the lookup tables are not yet built, walk the differences and
/// fill the lookup tables.
void
enum_diff::ensure_lookup_tables_populated()
{
  if (!lookup_tables_empty())
    return;

  {
    edit_script e = priv_->enumerators_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	const enum_type_decl::enumerator& n =
	  first_enum()->get_enumerators()[i];
	const string& name = n.get_name();
	ABG_ASSERT(priv_->deleted_enumerators_.find(n.get_name())
	       == priv_->deleted_enumerators_.end());
	priv_->deleted_enumerators_[name] = n;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    const enum_type_decl::enumerator& n =
	      second_enum()->get_enumerators()[i];
	    const string& name = n.get_name();
	    ABG_ASSERT(priv_->inserted_enumerators_.find(n.get_name())
		   == priv_->inserted_enumerators_.end());
	    string_enumerator_map::const_iterator j =
	      priv_->deleted_enumerators_.find(name);
	    if (j == priv_->deleted_enumerators_.end())
	      priv_->inserted_enumerators_[name] = n;
	    else
	      {
		if (j->second != n)
		  priv_->changed_enumerators_[j->first] =
		    std::make_pair(j->second, n);
		priv_->deleted_enumerators_.erase(j);
	      }
	  }
      }
  }
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref enum_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
enum_diff::chain_into_hierarchy()
{append_child_node(underlying_type_diff());}

/// Constructor for enum_diff.
///
/// @param first the first enum type of the diff.
///
/// @param second the second enum type of the diff.
///
/// @param underlying_type_diff the diff of the two underlying types
/// of the two enum types.
///
/// @param ctxt the diff context to use.
enum_diff::enum_diff(const enum_type_decl_sptr	first,
		     const enum_type_decl_sptr	second,
		     const diff_sptr		underlying_type_diff,
		     const diff_context_sptr	ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv(underlying_type_diff))
{}

/// Finish building the current instance of @ref enum_diff.
void
enum_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// @return the first enum of the diff.
const enum_type_decl_sptr
enum_diff::first_enum() const
{return dynamic_pointer_cast<enum_type_decl>(first_subject());}

/// @return the second enum of the diff.
const enum_type_decl_sptr
enum_diff::second_enum() const
{return dynamic_pointer_cast<enum_type_decl>(second_subject());}

/// @return the diff of the two underlying enum types.
diff_sptr
enum_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// @return a map of the enumerators that were deleted.
const string_enumerator_map&
enum_diff::deleted_enumerators() const
{return priv_->deleted_enumerators_;}

/// @return a map of the enumerators that were inserted
const string_enumerator_map&
enum_diff::inserted_enumerators() const
{return priv_->inserted_enumerators_;}

/// @return a map of the enumerators that were changed
const string_changed_enumerator_map&
enum_diff::changed_enumerators() const
{return priv_->changed_enumerators_;}

/// @return the pretty representation of the current instance of @ref
/// enum_diff.
const string&
enum_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "enum_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
enum_diff::has_changes() const
{return first_enum() != second_enum();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
enum_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_enum(), *second_enum(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Report the differences between the two enums.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
enum_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the set of changes between two instances of @ref
/// enum_type_decl.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first a pointer to the first enum_type_decl to consider.
///
/// @param second a pointer to the second enum_type_decl to consider.
///
/// @return the resulting diff of the two enums @p first and @p
/// second.
///
/// @param ctxt the diff context to use.
enum_diff_sptr
compute_diff(const enum_type_decl_sptr first,
	     const enum_type_decl_sptr second,
	     diff_context_sptr ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  diff_sptr ud = compute_diff_for_types(first->get_underlying_type(),
					second->get_underlying_type(),
					ctxt);
  enum_diff_sptr d(new enum_diff(first, second, ud, ctxt));

  compute_diff(first->get_enumerators().begin(),
	       first->get_enumerators().end(),
	       second->get_enumerators().begin(),
	       second->get_enumerators().end(),
	       d->priv_->enumerators_changes_);

  d->ensure_lookup_tables_populated();

  ctxt->initialize_canonical_diff(d);

  return d;
}
// </enum_diff stuff>

// <class_or_union_diff stuff>

/// Test if the current diff node carries a member type change for a
/// member type which name is the same as the name of a given type
/// declaration.
///
/// @param d the type declaration which name should be equal to the
/// name of the member type that might have changed.
///
/// @return the member type that has changed, iff there were a member
/// type (which name is the same as the name of @p d) that changed.
/// Note that the member type that is returned is the new value of the
/// member type that changed.
type_or_decl_base_sptr
class_or_union_diff::priv::member_type_has_changed(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_diff_sptr_map::const_iterator it =
    changed_member_types_.find(qname);

  return ((it == changed_member_types_.end())
	  ? type_or_decl_base_sptr()
	  : it->second->second_subject());
}

/// Test if the current diff node carries a data member change for a
/// data member which name is the same as the name of a given type
/// declaration.
///
/// @param d the type declaration which name should be equal to the
/// name of the data member that might have changed.
///
/// @return the data member that has changed, iff there were a data
/// member type (which name is the same as the name of @p d) that
/// changed.  Note that the data member that is returned is the new
/// value of the data member that changed.
decl_base_sptr
class_or_union_diff::priv::subtype_changed_dm(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_var_diff_sptr_map::const_iterator it =
    subtype_changed_dm_.find(qname);

  if (it == subtype_changed_dm_.end())
    return decl_base_sptr();
  return it->second->second_var();
}

/// Test if the current diff node carries a member class template
/// change for a member class template which name is the same as the
/// name of a given type declaration.
///
/// @param d the type declaration which name should be equal to the
/// name of the member class template that might have changed.
///
/// @return the member class template that has changed, iff there were
/// a member class template (which name is the same as the name of @p
/// d) that changed.  Note that the member class template that is
/// returned is the new value of the member class template that
/// changed.
decl_base_sptr
class_or_union_diff::priv::member_class_tmpl_has_changed(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_diff_sptr_map::const_iterator it =
    changed_member_class_tmpls_.find(qname);

  return ((it == changed_member_class_tmpls_.end())
	  ? decl_base_sptr()
	  : dynamic_pointer_cast<decl_base>(it->second->second_subject()));
}

/// Get the number of non static data members that were deleted.
///
/// @return the number of non static data members that were deleted.
size_t
class_or_union_diff::priv::get_deleted_non_static_data_members_number() const
{
  size_t result = 0;

  for (string_decl_base_sptr_map::const_iterator i =
	 deleted_data_members_.begin();
       i != deleted_data_members_.end();
       ++i)
    if (is_member_decl(i->second)
	&& !get_member_is_static(i->second))
      ++result;

  return result;
}

/// Get the number of non static data members that were inserted.
///
/// @return the number of non static data members that were inserted.
size_t
class_or_union_diff::priv::get_inserted_non_static_data_members_number() const
{
  size_t result = 0;

  for (string_decl_base_sptr_map::const_iterator i =
	 inserted_data_members_.begin();
       i != inserted_data_members_.end();
       ++i)
    if (is_member_decl(i->second)
	&& !get_member_is_static(i->second))
      ++result;

  return result;
}

/// Get the number of data member sub-type changes carried by the
/// current diff node that were filtered out.
///
/// @param local_only if true, it means that only (filtered) local
/// changes are considered.
///
/// @return the number of data member sub-type changes carried by the
/// current diff node that were filtered out.
size_t
class_or_union_diff::priv::count_filtered_subtype_changed_dm(bool local_only)
{
  size_t num_filtered= 0;
  for (var_diff_sptrs_type::const_iterator i =
	 sorted_subtype_changed_dm_.begin();
       i != sorted_subtype_changed_dm_.end();
       ++i)
    {
      if (local_only)
	{
	  if ((*i)->has_changes()
	      && !(*i)->has_local_changes_to_be_reported())
	    ++num_filtered;
	}
      else
	{
	  if ((*i)->is_filtered_out())
	    ++num_filtered;
	}
    }
  return num_filtered;
}

/// Get the number of data member changes carried by the current diff
/// node that were filtered out.
///
/// @param local_only if true, it means that only (filtered) local
/// changes are considered.
///
/// @return the number of data member changes carried by the current
/// diff node that were filtered out.
size_t
class_or_union_diff::priv::count_filtered_changed_dm(bool local_only)
{
  size_t num_filtered= 0;

  for (unsigned_var_diff_sptr_map::const_iterator i = changed_dm_.begin();
       i != changed_dm_.end();
       ++i)
    {
      diff_sptr diff = i->second;
      if (local_only)
	{
	  if ((diff->has_changes() && !diff->has_local_changes_to_be_reported())
	      || diff->is_filtered_out())
	    ++num_filtered;
	}
      else
	{
	  if (diff->is_filtered_out())
	    ++num_filtered;
	}
    }
  return num_filtered;
}

/// Skip the processing of the current member function if its
/// virtual-ness is disallowed by the user.
///
/// This is to be used in the member functions below that are used to
/// count the number of filtered inserted, deleted and changed member
/// functions.
#define SKIP_MEM_FN_IF_VIRTUALITY_DISALLOWED				\
  do {									\
    if (get_member_function_is_virtual(f)					\
	|| get_member_function_is_virtual(s))				\
      {								\
	if (!(allowed_category | VIRTUAL_MEMBER_CHANGE_CATEGORY))	\
	  continue;							\
      }								\
    else								\
      {								\
	if (!(allowed_category | NON_VIRT_MEM_FUN_CHANGE_CATEGORY))	\
	  continue;							\
      }								\
  } while (false)

/// Get the number of member functions changes carried by the current
/// diff node that were filtered out.
///
/// @return the number of member functions changes carried by the
/// current diff node that were filtered out.
size_t
class_or_union_diff::priv::count_filtered_changed_mem_fns
(const diff_context_sptr& ctxt)
{
  size_t count = 0;
  diff_category allowed_category = ctxt->get_allowed_category();

  for (function_decl_diff_sptrs_type::const_iterator i =
	 sorted_changed_member_functions_.begin();
       i != sorted_changed_member_functions_.end();
       ++i)
    {
      method_decl_sptr f =
	dynamic_pointer_cast<method_decl>
	((*i)->first_function_decl());
      ABG_ASSERT(f);

      method_decl_sptr s =
	dynamic_pointer_cast<method_decl>
	((*i)->second_function_decl());
      ABG_ASSERT(s);

      SKIP_MEM_FN_IF_VIRTUALITY_DISALLOWED;

      diff_sptr diff = *i;
      ctxt->maybe_apply_filters(diff);

      if (diff->is_filtered_out())
	++count;
    }

  return count;
}

/// Get the number of member functions insertions carried by the current
/// diff node that were filtered out.
///
/// @return the number of member functions insertions carried by the
/// current diff node that were filtered out.
size_t
class_or_union_diff::priv::count_filtered_inserted_mem_fns
(const diff_context_sptr& ctxt)
{
    size_t count = 0;
  diff_category allowed_category = ctxt->get_allowed_category();

  for (string_member_function_sptr_map::const_iterator i =
	 inserted_member_functions_.begin();
       i != inserted_member_functions_.end();
       ++i)
    {
      method_decl_sptr f = i->second,
	s = i->second;

      SKIP_MEM_FN_IF_VIRTUALITY_DISALLOWED;

      diff_sptr diff = compute_diff_for_decls(f, s, ctxt);
      ctxt->maybe_apply_filters(diff);

      if (diff->get_category() != NO_CHANGE_CATEGORY
	  && diff->is_filtered_out())
	++count;
    }

  return count;
}

/// Get the number of member functions deletions carried by the current
/// diff node that were filtered out.
///
/// @return the number of member functions deletions carried by the
/// current diff node that were filtered out.
size_t
class_or_union_diff::priv::count_filtered_deleted_mem_fns
(const diff_context_sptr& ctxt)
{
  size_t count = 0;
  diff_category allowed_category = ctxt->get_allowed_category();

  for (string_member_function_sptr_map::const_iterator i =
	 deleted_member_functions_.begin();
       i != deleted_member_functions_.end();
       ++i)
    {
      method_decl_sptr f = i->second,
	s = i->second;

      SKIP_MEM_FN_IF_VIRTUALITY_DISALLOWED;

      diff_sptr diff = compute_diff_for_decls(f, s, ctxt);
      ctxt->maybe_apply_filters(diff);

      if (diff->get_category() != NO_CHANGE_CATEGORY
	  && diff->is_filtered_out())
	++count;
    }

  return count;
}

/// Clear the lookup tables useful for reporting.
///
/// This function must be updated each time a lookup table is added or
/// removed from the class_or_union_diff::priv.
void
class_or_union_diff::clear_lookup_tables()
{
  priv_->deleted_member_types_.clear();
  priv_->inserted_member_types_.clear();
  priv_->changed_member_types_.clear();
  priv_->deleted_data_members_.clear();
  priv_->inserted_data_members_.clear();
  priv_->subtype_changed_dm_.clear();
  priv_->deleted_member_functions_.clear();
  priv_->inserted_member_functions_.clear();
  priv_->changed_member_functions_.clear();
  priv_->deleted_member_class_tmpls_.clear();
  priv_->inserted_member_class_tmpls_.clear();
  priv_->changed_member_class_tmpls_.clear();
}

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
class_or_union_diff::lookup_tables_empty(void) const
{
  return (priv_->deleted_member_types_.empty()
	  && priv_->inserted_member_types_.empty()
	  && priv_->changed_member_types_.empty()
	  && priv_->deleted_data_members_.empty()
	  && priv_->inserted_data_members_.empty()
	  && priv_->subtype_changed_dm_.empty()
	  && priv_->inserted_member_functions_.empty()
	  && priv_->deleted_member_functions_.empty()
	  && priv_->changed_member_functions_.empty()
	  && priv_->deleted_member_class_tmpls_.empty()
	  && priv_->inserted_member_class_tmpls_.empty()
	  && priv_->changed_member_class_tmpls_.empty());
}

/// If the lookup tables are not yet built, walk the differences and
/// fill them.
void
class_or_union_diff::ensure_lookup_tables_populated(void) const
{
  {
    edit_script& e = priv_->member_types_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	decl_base_sptr d =
	  get_type_declaration(first_class_or_union()->get_member_types()[i]);
	class_or_union_sptr record_type = is_class_or_union_type(d);
	if (record_type && record_type->get_is_declaration_only())
	  continue;
	string name = d->get_name();
	priv_->deleted_member_types_[name] = d;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    decl_base_sptr d =
	      get_type_declaration(second_class_or_union()->get_member_types()[i]);
	    class_or_union_sptr record_type = is_class_or_union_type(d);
	    if (record_type && record_type->get_is_declaration_only())
	      continue;
	    string name = d->get_name();
	    string_decl_base_sptr_map::const_iterator j =
	      priv_->deleted_member_types_.find(name);
	    if (j != priv_->deleted_member_types_.end())
	      {
		if (*j->second != *d)
		  priv_->changed_member_types_[name] =
		    compute_diff(j->second, d, context());

		priv_->deleted_member_types_.erase(j);
	      }
	    else
	      priv_->inserted_member_types_[name] = d;
	  }
      }
  }

  {
    edit_script& e = priv_->data_members_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	var_decl_sptr data_member =
	  is_var_decl(first_class_or_union()->get_non_static_data_members()[i]);
	string name = data_member->get_anon_dm_reliable_name();

	ABG_ASSERT(priv_->deleted_data_members_.find(name)
		   == priv_->deleted_data_members_.end());
	priv_->deleted_data_members_[name] = data_member;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    decl_base_sptr d =
	      second_class_or_union()->get_non_static_data_members()[i];
	    var_decl_sptr added_dm = is_var_decl(d);
	    string name = added_dm->get_anon_dm_reliable_name();
	    ABG_ASSERT(priv_->inserted_data_members_.find(name)
		       == priv_->inserted_data_members_.end());

	    bool ignore_added_anonymous_data_member = false;
	    if (is_anonymous_data_member(added_dm))
	      {
		//
		// Handle insertion of anonymous data member to
		// replace existing data members.
		//
		// For instance consider this:
		//   struct S
		//   {
		//     int a;
		//     int b;
		//     int c;
		//   };// end struct S
		//
		//   Where the data members 'a' and 'b' are replaced
		//   by an anonymous data member without changing the
		//   effective bit layout of the structure:
		//
		//   struct S
		//   {
		//     struct
		//     {
		//       union
		//       {
		//         int a;
		//         char a_1;
		//       };
		//       union
		//       {
		//         int b;
		//         char b_1;
		//       };
		//     };
		//     int c;
		//   }; // end struct S
		//
		var_decl_sptr replaced_dm, replacing_dm;
		bool added_anon_dm_changes_dm = false;
		// The vector of data members replaced by anonymous
		// data members.
		vector<var_decl_sptr> dms_replaced_by_anon_dm;

		//
		// Let's start collecting the set of data members
		// which have been replaced by anonymous types in a
		// harmless way.  These are going to be collected into
		// dms_replaced_by_anon_dm and, ultimately, into
		// priv_->dms_replaced_by_adms_
		//
		for (string_decl_base_sptr_map::const_iterator it =
		       priv_->deleted_data_members_.begin();
		     it != priv_->deleted_data_members_.end();
		     ++it)
		  {
		    // We don't support this pattern for anonymous
		    // data members themselves being replaced.  If
		    // that occurs then we'll just report it verbatim.
		    if (is_anonymous_data_member(it->second))
		      continue;

		    string deleted_dm_name = it->second->get_name();
		    if ((replacing_dm =
			 find_data_member_from_anonymous_data_member(added_dm,
								     deleted_dm_name)))
		      {
			// So it looks like replacing_dm might have
			// replaced the data member which name is
			// 'deleted_dm_name'.  Let's look deeper to be
			// sure.
			//
			// Note that replacing_dm is part (member) of
			// an anonymous data member that might replace
			// replaced_dm.

			// So let's get that replaced data member.
			replaced_dm = is_var_decl(it->second);
			size_t replaced_dm_offset =
			  get_data_member_offset(replaced_dm),
			replacing_dm_offset =
			  get_absolute_data_member_offset(replacing_dm);

			if (replaced_dm_offset != replacing_dm_offset)
			  {
			    // So the replacing data member and the
			    // replaced data member don't have the
			    // same offset.  This is not the pattern we
			    // are looking for.  Rather, it looks like
			    // the anonymous data member has *changed*
			    // the data member.
			    added_anon_dm_changes_dm = true;
			    break;
			  }

			if (replaced_dm->get_type()->get_size_in_bits()
			    == replaced_dm->get_type()->get_size_in_bits())
			  dms_replaced_by_anon_dm.push_back(replaced_dm);
			else
			  {
			    added_anon_dm_changes_dm = true;
			    break;
			  }
		      }
		  }

		// Now walk dms_replaced_by_anon_dm to fill up
		// priv_->dms_replaced_by_adms_ with the set of data
		// members replaced by anonymous data members.
		if (!added_anon_dm_changes_dm
		    && !dms_replaced_by_anon_dm.empty())
		  {
		    // See if the added data member isn't too big.
		    type_base_sptr added_dm_type = added_dm->get_type();
		    ABG_ASSERT(added_dm_type);
		    var_decl_sptr new_next_dm =
		      get_next_data_member(second_class_or_union(),
					   added_dm);
		    var_decl_sptr old_next_dm =
		      first_class_or_union()->find_data_member(new_next_dm);

		    if (!old_next_dm
			|| (old_next_dm
			    && (get_absolute_data_member_offset(old_next_dm)
				== get_absolute_data_member_offset(new_next_dm))))
		      {
			// None of the data members that are replaced
			// by the added union should be considered as
			// having been deleted.
			ignore_added_anonymous_data_member = true;
			for (vector<var_decl_sptr>::const_iterator i =
			       dms_replaced_by_anon_dm.begin();
			     i != dms_replaced_by_anon_dm.end();
			     ++i)
			  {
			    string n = (*i)->get_name();
			    priv_->dms_replaced_by_adms_[n] =
			      added_dm;
			    priv_->deleted_data_members_.erase(n);
			  }
		      }
		  }
	      }

	    if (!ignore_added_anonymous_data_member)
	      {
		// Detect changed data members.
		//
		// A changed data member (that we shall name D) is a data
		// member that satisfies the conditions below:
		//
		// 1/ It must have been added.
		//
		// 2/ It must have been deleted as well.
		//
		// 3/ It there must be a non-empty difference between the
		// deleted D and the added D.
		string_decl_base_sptr_map::const_iterator j =
		  priv_->deleted_data_members_.find(name);
		if (j != priv_->deleted_data_members_.end())
		  {
		    if (*j->second != *d)
		      {
			var_decl_sptr old_dm = is_var_decl(j->second);
			priv_->subtype_changed_dm_[name]=
			  compute_diff(old_dm, added_dm, context());
		      }
		    priv_->deleted_data_members_.erase(j);
		  }
		else
		  priv_->inserted_data_members_[name] = d;
	      }
	  }
      }

    // Now detect when a data member is deleted from offset N and
    // another one is added to offset N.  In that case, we want to be
    // able to say that the data member at offset N changed.
    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->deleted_data_members_.begin();
	 i != priv_->deleted_data_members_.end();
	 ++i)
      {
	unsigned offset = get_data_member_offset(i->second);
	priv_->deleted_dm_by_offset_[offset] = i->second;
      }

    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->inserted_data_members_.begin();
	 i != priv_->inserted_data_members_.end();
	 ++i)
      {
	unsigned offset = get_data_member_offset(i->second);
	priv_->inserted_dm_by_offset_[offset] = i->second;
      }

    for (unsigned_decl_base_sptr_map::const_iterator i =
	   priv_->inserted_dm_by_offset_.begin();
	 i != priv_->inserted_dm_by_offset_.end();
	 ++i)
      {
	unsigned_decl_base_sptr_map::const_iterator j =
	  priv_->deleted_dm_by_offset_.find(i->first);
	if (j != priv_->deleted_dm_by_offset_.end())
	  {
	    var_decl_sptr old_dm = is_var_decl(j->second);
	    var_decl_sptr new_dm = is_var_decl(i->second);
	    priv_->changed_dm_[i->first] =
	      compute_diff(old_dm, new_dm, context());
	  }
      }

    for (unsigned_var_diff_sptr_map::const_iterator i =
	   priv_->changed_dm_.begin();
	 i != priv_->changed_dm_.end();
	 ++i)
      {
	priv_->deleted_dm_by_offset_.erase(i->first);
	priv_->inserted_dm_by_offset_.erase(i->first);
	priv_->deleted_data_members_.erase
	  (i->second->first_var()->get_anon_dm_reliable_name());
	priv_->inserted_data_members_.erase
	  (i->second->second_var()->get_anon_dm_reliable_name());
      }
  }
  sort_string_data_member_diff_sptr_map(priv_->subtype_changed_dm_,
					priv_->sorted_subtype_changed_dm_);
  sort_unsigned_data_member_diff_sptr_map(priv_->changed_dm_,
					  priv_->sorted_changed_dm_);

  {
    edit_script& e = priv_->member_class_tmpls_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	decl_base_sptr d =
	  first_class_or_union()->get_member_class_templates()[i]->
	  as_class_tdecl();
	string name = d->get_name();
	ABG_ASSERT(priv_->deleted_member_class_tmpls_.find(name)
	       == priv_->deleted_member_class_tmpls_.end());
	priv_->deleted_member_class_tmpls_[name] = d;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    decl_base_sptr d =
	      second_class_or_union()->get_member_class_templates()[i]->
	      as_class_tdecl();
	    string name = d->get_name();
	    ABG_ASSERT(priv_->inserted_member_class_tmpls_.find(name)
		   == priv_->inserted_member_class_tmpls_.end());
	    string_decl_base_sptr_map::const_iterator j =
	      priv_->deleted_member_class_tmpls_.find(name);
	    if (j != priv_->deleted_member_class_tmpls_.end())
	      {
		if (*j->second != *d)
		  priv_->changed_member_types_[name]=
		    compute_diff(j->second, d, context());
		priv_->deleted_member_class_tmpls_.erase(j);
	      }
	    else
	      priv_->inserted_member_class_tmpls_[name] = d;
	  }
      }
  }
  sort_string_diff_sptr_map(priv_->changed_member_types_,
			    priv_->sorted_changed_member_types_);
}

/// Allocate the memory for the priv_ pimpl data member of the @ref
/// class_or_union_diff class.
void
class_or_union_diff::allocate_priv_data()
{
  if (!priv_)
    priv_.reset(new priv);
}

/// Constructor for the @ref class_or_union_diff class.
///
/// @param first_scope the first @ref class_or_union of the diff node.
///
/// @param second_scope the second @ref class_or_union of the diff node.
///
/// @param ctxt the context of the diff.
class_or_union_diff::class_or_union_diff(class_or_union_sptr first_scope,
					 class_or_union_sptr second_scope,
					 diff_context_sptr ctxt)
  : type_diff_base(first_scope, second_scope, ctxt)
    //priv_(new priv)
{}

/// Finish building the current instance of @ref class_or_union_diff.
void
class_or_union_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter of the private data of the @ref class_or_union_diff type.
///
/// Note that due to an optimization, the private data of @ref
/// class_or_union_diff can be shared among several instances of
/// class_or_union_diff, so you should never try to access
/// class_or_union_diff::priv directly.
///
/// When class_or_union_diff::priv is shared, this function returns
/// the correct shared one.
///
/// @return the (possibly) shared private data of the current instance
/// of @ref class_or_union_diff.
const class_or_union_diff::priv_sptr&
class_or_union_diff::get_priv() const
{
  if (priv_)
    return priv_;

  // If the current class_or_union_diff::priv member is empty, then look for
  // the shared one, from the canonical type.
  class_or_union_diff *canonical =
    dynamic_cast<class_or_union_diff*>(get_canonical_diff());
  ABG_ASSERT(canonical);
  ABG_ASSERT(canonical->priv_);

  return canonical->priv_;
}

/// Destructor of class_or_union_diff.
class_or_union_diff::~class_or_union_diff()
{
}

/// @return the first @ref class_or_union involved in the diff.
class_or_union_sptr
class_or_union_diff::first_class_or_union() const
{return is_class_or_union_type(first_subject());}

/// @return the second @ref class_or_union involved in the diff.
class_or_union_sptr
class_or_union_diff::second_class_or_union() const
{return is_class_or_union_type(second_subject());}

/// @return the edit script of the member types of the two @ref
/// class_or_union.
const edit_script&
class_or_union_diff::member_types_changes() const
{return get_priv()->member_types_changes_;}

/// @return the edit script of the member types of the two @ref
/// class_or_union.
edit_script&
class_or_union_diff::member_types_changes()
{return get_priv()->member_types_changes_;}

/// @return the edit script of the data members of the two @ref
/// class_or_union.
const edit_script&
class_or_union_diff::data_members_changes() const
{return get_priv()->data_members_changes_;}

/// @return the edit script of the data members of the two @ref
/// class_or_union.
edit_script&
class_or_union_diff::data_members_changes()
{return get_priv()->data_members_changes_;}

/// Getter for the data members that got inserted.
///
/// @return a map of data members that got inserted.
const string_decl_base_sptr_map&
class_or_union_diff::inserted_data_members() const
{return get_priv()->inserted_data_members_;}

/// Getter for the data members that got deleted.
///
/// @return a map of data members that got deleted.
const string_decl_base_sptr_map&
class_or_union_diff::deleted_data_members() const
{return get_priv()->deleted_data_members_;}

/// @return the edit script of the member functions of the two @ref
/// class_or_union.
const edit_script&
class_or_union_diff::member_fns_changes() const
{return get_priv()->member_fns_changes_;}

/// Getter for the virtual members functions that have had a change in
/// a sub-type, without having a change in their symbol name.
///
/// @return a sorted vector of virtual member functions that have a
/// sub-type change.
const function_decl_diff_sptrs_type&
class_or_union_diff::changed_member_fns() const
{return get_priv()->sorted_changed_member_functions_;}

/// @return the edit script of the member functions of the two
/// classes.
edit_script&
class_or_union_diff::member_fns_changes()
{return get_priv()->member_fns_changes_;}

/// @return a map of member functions that got deleted.
const string_member_function_sptr_map&
class_or_union_diff::deleted_member_fns() const
{return get_priv()->deleted_member_functions_;}

/// @return a map of member functions that got inserted.
const string_member_function_sptr_map&
class_or_union_diff::inserted_member_fns() const
{return get_priv()->inserted_member_functions_;}

/// Getter of the sorted vector of data members that got replaced by
/// another data member.
///
/// @return sorted vector of changed data member.
const var_diff_sptrs_type&
class_or_union_diff::sorted_changed_data_members() const
{return get_priv()->sorted_changed_dm_;}

/// Count the number of /filtered/ data members that got replaced by
/// another data member.
///
/// @return the number of changed data member that got filtered out.
size_t
class_or_union_diff::count_filtered_changed_data_members(bool local) const
{return get_priv()->count_filtered_changed_dm(local);}

/// Getter of the sorted vector of data members with a (sub-)type change.
///
/// @return sorted vector of changed data member.
const var_diff_sptrs_type&
class_or_union_diff::sorted_subtype_changed_data_members() const
{return get_priv()->sorted_subtype_changed_dm_;}

/// Count the number of /filtered/ data members with a sub-type change.
///
/// @return the number of changed data member that got filtered out.
size_t
class_or_union_diff::count_filtered_subtype_changed_data_members(bool local) const
{return get_priv()->count_filtered_subtype_changed_dm(local);}

/// Get the map of data members that got replaced by anonymous data
/// members.
///
/// The key of a map entry is the name of the replaced data member and
/// the value is the anonymous data member that replaces it.
///
/// @return the map of data members replaced by anonymous data
/// members.
const string_decl_base_sptr_map&
class_or_union_diff::data_members_replaced_by_adms() const
{return get_priv()->dms_replaced_by_adms_;}

/// Get an ordered vector of of data members that got replaced by
/// anonymous data members.
///
/// This returns a vector of pair of two data members: the one that
/// was replaced, and the anonymous data member that replaced it.
///
/// @return the sorted vector data members replaced by anonymous data members.
const changed_var_sptrs_type&
class_or_union_diff::ordered_data_members_replaced_by_adms() const
{
  if (priv_->dms_replaced_by_adms_ordered_.empty())
    {
      for (string_decl_base_sptr_map::const_iterator it =
	     priv_->dms_replaced_by_adms_.begin();
	   it != priv_->dms_replaced_by_adms_.end();
	   ++it)
	{
	  const var_decl_sptr dm =
	    first_class_or_union()->find_data_member(it->first);
	  ABG_ASSERT(dm);
	  changed_var_sptr changed_dm(dm, is_data_member(it->second));
	  priv_->dms_replaced_by_adms_ordered_.push_back(changed_dm);
	}
      sort_changed_data_members(priv_->dms_replaced_by_adms_ordered_);
    }

  return priv_->dms_replaced_by_adms_ordered_;
}

/// @return the edit script of the member function templates of the two
/// @ref class_or_union.
const edit_script&
class_or_union_diff::member_fn_tmpls_changes() const
{return get_priv()->member_fn_tmpls_changes_;}

/// @return the edit script of the member function templates of the
/// two @ref class_or_union.
edit_script&
class_or_union_diff::member_fn_tmpls_changes()
{return get_priv()->member_fn_tmpls_changes_;}

/// @return the edit script of the member class templates of the two
/// @ref class_or_union.
const edit_script&
class_or_union_diff::member_class_tmpls_changes() const
{return get_priv()->member_class_tmpls_changes_;}

/// @return the edit script of the member class templates of the two
/// @ref class_or_union.
edit_script&
class_or_union_diff::member_class_tmpls_changes()
{return get_priv()->member_class_tmpls_changes_;}

/// Test if the current diff node carries a change.
bool
class_or_union_diff::has_changes() const
{return first_class_or_union() != second_class_or_union();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
class_or_union_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_class_or_union(), *second_class_or_union(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}


/// Report the changes carried by the current @ref class_or_union_diff
/// node in a textual format.
///
/// @param out the output stream to write the textual report to.
///
/// @param indent the number of white space to use as indentation.
void
class_or_union_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref class_or_union_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
class_or_union_diff::chain_into_hierarchy()
{
  // data member changes
  for (var_diff_sptrs_type::const_iterator i =
	 get_priv()->sorted_subtype_changed_dm_.begin();
       i != get_priv()->sorted_subtype_changed_dm_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);

  for (var_diff_sptrs_type::const_iterator i =
	 get_priv()->sorted_changed_dm_.begin();
       i != get_priv()->sorted_changed_dm_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);

  // member types changes
  for (diff_sptrs_type::const_iterator i =
	 get_priv()->sorted_changed_member_types_.begin();
       i != get_priv()->sorted_changed_member_types_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);

  // member function changes
  for (function_decl_diff_sptrs_type::const_iterator i =
	 get_priv()->sorted_changed_member_functions_.begin();
       i != get_priv()->sorted_changed_member_functions_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);
}

// </class_or_union_diff stuff>

//<class_diff stuff>

/// Clear the lookup tables useful for reporting.
///
/// This function must be updated each time a lookup table is added or
/// removed from the class_diff::priv.
void
class_diff::clear_lookup_tables(void)
{
  priv_->deleted_bases_.clear();
  priv_->inserted_bases_.clear();
  priv_->changed_bases_.clear();
}

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
class_diff::lookup_tables_empty(void) const
{
  return (priv_->deleted_bases_.empty()
	  && priv_->inserted_bases_.empty()
	  && priv_->changed_bases_.empty());
}

/// If the lookup tables are not yet built, walk the differences and
/// fill them.
void
class_diff::ensure_lookup_tables_populated(void) const
{
  class_or_union_diff::ensure_lookup_tables_populated();

  if (!lookup_tables_empty())
    return;

  {
    edit_script& e = get_priv()->base_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	class_decl::base_spec_sptr b =
	  first_class_decl()->get_base_specifiers()[i];
	string name = b->get_base_class()->get_name();
	ABG_ASSERT(get_priv()->deleted_bases_.find(name)
	       == get_priv()->deleted_bases_.end());
	get_priv()->deleted_bases_[name] = b;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    class_decl::base_spec_sptr b =
	      second_class_decl()->get_base_specifiers()[i];
	    string name = b->get_base_class()->get_name();
	    ABG_ASSERT(get_priv()->inserted_bases_.find(name)
		   == get_priv()->inserted_bases_.end());
	    string_base_sptr_map::const_iterator j =
	      get_priv()->deleted_bases_.find(name);
	    if (j != get_priv()->deleted_bases_.end())
	      {
		if (j->second != b)
		  get_priv()->changed_bases_[name] =
		    compute_diff(j->second, b, context());
		get_priv()->deleted_bases_.erase(j);
	      }
	    else
	      get_priv()->inserted_bases_[name] = b;
	  }
      }
  }

  sort_string_base_sptr_map(get_priv()->deleted_bases_,
			    get_priv()->sorted_deleted_bases_);
  sort_string_base_sptr_map(get_priv()->inserted_bases_,
			    get_priv()->sorted_inserted_bases_);
  sort_string_base_diff_sptr_map(get_priv()->changed_bases_,
				 get_priv()->sorted_changed_bases_);

  {
    const class_or_union_diff::priv_sptr &p = class_or_union_diff::get_priv();

    edit_script& e = p->member_fns_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	method_decl_sptr mem_fn =
	  first_class_decl()->get_virtual_mem_fns()[i];
	string name = mem_fn->get_linkage_name();
	if (name.empty())
	  name = mem_fn->get_pretty_representation();
	ABG_ASSERT(!name.empty());
	if (p->deleted_member_functions_.find(name)
	    != p->deleted_member_functions_.end())
	  continue;
	p->deleted_member_functions_[name] = mem_fn;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;

	    method_decl_sptr mem_fn =
	      second_class_decl()->get_virtual_mem_fns()[i];
	    string name = mem_fn->get_linkage_name();
	    if (name.empty())
	      name = mem_fn->get_pretty_representation();
	    ABG_ASSERT(!name.empty());
	    if (p->inserted_member_functions_.find(name)
		!= p->inserted_member_functions_.end())
	      continue;
	    string_member_function_sptr_map::const_iterator j =
	      p->deleted_member_functions_.find(name);

	    if (j != p->deleted_member_functions_.end())
	      {
		if (*j->second != *mem_fn)
		  p->changed_member_functions_[name] =
		    compute_diff(static_pointer_cast<function_decl>(j->second),
				 static_pointer_cast<function_decl>(mem_fn),
				 context());
		p->deleted_member_functions_.erase(j);
	      }
	    else
	      p->inserted_member_functions_[name] = mem_fn;
	  }
      }

    // Now walk the allegedly deleted member functions; check if their
    // underlying symbols are deleted as well; otherwise, consider
    // that the member function in question hasn't been deleted.

    vector<string> to_delete;
    corpus_sptr f = context()->get_first_corpus(),
      s = context()->get_second_corpus();
    if (s)
      for (string_member_function_sptr_map::const_iterator i =
	     deleted_member_fns().begin();
	   i != deleted_member_fns().end();
	   ++i)
	{
	  if (get_member_function_is_virtual(i->second))
	    continue;
	  // We assume that all non-virtual member functions functions
	  // we look at here have ELF symbols.
	  if (!i->second->get_symbol()
	      || s->lookup_function_symbol(*i->second->get_symbol()))
	    to_delete.push_back(i->first);
	}


    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      p->deleted_member_functions_.erase(*i);

    // Do something similar for added functions.
    to_delete.clear();
    if (f)
      for (string_member_function_sptr_map::const_iterator i =
	     inserted_member_fns().begin();
	   i != inserted_member_fns().end();
	   ++i)
	{
	  if (get_member_function_is_virtual(i->second))
	    continue;
	  // We assume that all non-virtual member functions functions
	  // we look at here have ELF symbols.
	  if (!i->second->get_symbol()
	      || f->lookup_function_symbol(*i->second->get_symbol()))
	    to_delete.push_back(i->first);
	}

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      p->inserted_member_functions_.erase(*i);

    sort_string_member_function_sptr_map(p->deleted_member_functions_,
					 p->sorted_deleted_member_functions_);

    sort_string_member_function_sptr_map(p->inserted_member_functions_,
					 p->sorted_inserted_member_functions_);

    sort_string_virtual_member_function_diff_sptr_map
      (p->changed_member_functions_,
       p->sorted_changed_member_functions_);
  }
}

/// Allocate the memory for the priv_ pimpl data member of the @ref
/// class_diff class.
void
class_diff::allocate_priv_data()
{
  class_or_union_diff::allocate_priv_data();
  if (!priv_)
    priv_.reset(new priv);
}

/// Test whether a given base class has changed.  A base class has
/// changed if it's in both in deleted *and* inserted bases.
///
///@param d the declaration for the base class to consider.
///
/// @return the new base class if the given base class has changed, or
/// NULL if it hasn't.
class_decl::base_spec_sptr
class_diff::priv::base_has_changed(class_decl::base_spec_sptr d) const
{
  string qname = d->get_base_class()->get_qualified_name();
  string_base_diff_sptr_map::const_iterator it =
    changed_bases_.find(qname);

  return (it == changed_bases_.end())
    ? class_decl::base_spec_sptr()
    : it->second->second_base();

}

/// Count the number of bases classes whose changes got filtered out.
///
/// @return the number of bases classes whose changes got filtered
/// out.
size_t
class_diff::priv::count_filtered_bases()
{
  size_t num_filtered = 0;
  for (base_diff_sptrs_type::const_iterator i = sorted_changed_bases_.begin();
       i != sorted_changed_bases_.end();
       ++i)
    {
      diff_sptr diff = *i;
      if (diff && diff->is_filtered_out())
	++num_filtered;
    }
  return num_filtered;
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref class_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
class_diff::chain_into_hierarchy()
{
  class_or_union_diff::chain_into_hierarchy();

  // base class changes.
  for (base_diff_sptrs_type::const_iterator i =
	 get_priv()->sorted_changed_bases_.begin();
       i != get_priv()->sorted_changed_bases_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);
}

/// Constructor of class_diff
///
/// @param first_scope the first class of the diff.
///
/// @param second_scope the second class of the diff.
///
/// @param ctxt the diff context to use.
class_diff::class_diff(class_decl_sptr first_scope,
		       class_decl_sptr second_scope,
		       diff_context_sptr ctxt)
  : class_or_union_diff(first_scope, second_scope, ctxt)
    //  We don't initialize the priv_ data member here.  This is an
    //  optimization to reduce memory consumption (and also execution
    //  time) for cases where there are a lot of instances of
    //  class_diff in the same equivalence class.  In compute_diff(),
    //  the priv_ is set to the priv_ of the canonical diff node.
    //  See PR libabigail/17948.
{}

class_diff::~class_diff()
{}

/// Getter of the private data of the @ref class_diff type.
///
/// Note that due to an optimization, the private data of @ref
/// class_diff can be shared among several instances of class_diff, so
/// you should never try to access class_diff::priv directly.
///
/// When class_diff::priv is shared, this function returns the correct
/// shared one.
///
/// @return the (possibly) shared private data of the current instance
/// of class_diff.
const class_diff::priv_sptr&
class_diff::get_priv() const
{
  if (priv_)
    return priv_;

  // If the current class_diff::priv member is empty, then look for
  // the shared one, from the canonical type.
  class_diff *canonical =
    dynamic_cast<class_diff*>(get_canonical_diff());
  ABG_ASSERT(canonical);
  ABG_ASSERT(canonical->priv_);

  return canonical->priv_;
}

/// Finish building the current instance of @ref class_diff.
void
class_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// @return the pretty representation of the current instance of @ref
/// class_diff.
const string&
class_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "class_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
class_diff::has_changes() const
{return (first_class_decl() != second_class_decl());}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
class_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_class_decl(), *second_class_decl(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// @return the first class invoveld in the diff.
shared_ptr<class_decl>
class_diff::first_class_decl() const
{return dynamic_pointer_cast<class_decl>(first_subject());}

/// Getter of the second class involved in the diff.
///
/// @return the second class invoveld in the diff
shared_ptr<class_decl>
class_diff::second_class_decl() const
{return dynamic_pointer_cast<class_decl>(second_subject());}

/// @return the edit script of the bases of the two classes.
const edit_script&
class_diff::base_changes() const
{return get_priv()->base_changes_;}

/// Getter for the deleted base classes of the diff.
///
/// @return a map containing the deleted base classes, keyed with
/// their pretty representation.
const string_base_sptr_map&
class_diff::deleted_bases() const
{return get_priv()->deleted_bases_;}

/// Getter for the inserted base classes of the diff.
///
/// @return a map containing the inserted base classes, keyed with
/// their pretty representation.
const string_base_sptr_map&
class_diff::inserted_bases() const
{return get_priv()->inserted_bases_;}

/// Getter for the changed base classes of the diff.
///
/// @return a sorted vector containing the changed base classes
const base_diff_sptrs_type&
class_diff::changed_bases()
{return get_priv()->sorted_changed_bases_;}

/// @return the edit script of the bases of the two classes.
edit_script&
class_diff::base_changes()
{return get_priv()->base_changes_;}

/// Produce a basic report about the changes between two class_decl.
///
/// @param out the output stream to report the changes to.
///
/// @param indent the string to use as an indentation prefix in the
/// report.
void
class_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the set of changes between two instances of class_decl.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first class_decl to consider.
///
/// @param second the second class_decl to consider.
///
/// @return changes the resulting changes.
///
/// @param ctxt the diff context to use.
class_diff_sptr
compute_diff(const class_decl_sptr	first,
	     const class_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  class_decl_sptr f = is_class_type(look_through_decl_only_class(first)),
    s = is_class_type(look_through_decl_only_class(second));

  class_diff_sptr changes(new class_diff(f, s, ctxt));

  ctxt->initialize_canonical_diff(changes);
  ABG_ASSERT(changes->get_canonical_diff());

  if (!ctxt->get_canonical_diff_for(first, second))
    {
      // Either first or second is a decl-only class; let's set the
      // canonical diff here in that case.
      diff_sptr canonical_diff = ctxt->get_canonical_diff_for(changes);
      ABG_ASSERT(canonical_diff);
      ctxt->set_canonical_diff_for(first, second, canonical_diff);
    }

  // Ok, so this is an optimization.  Do not freak out if it looks
  // weird, because, well, it does look weird.  This speeds up
  // greatly, for instance, the test case given at PR
  // libabigail/17948.
  //
  // We are setting the private data of the new instance of class_diff
  // (which is 'changes') to the private data of its canonical
  // instance.  That is, we are sharing the private data of 'changes'
  // with the private data of its canonical instance to consume less
  // memory in cases where the equivalence class of 'changes' is huge.
  //
  // But if changes is its own canonical instance, then we initialize
  // its private data properly
  if (is_class_diff(changes->get_canonical_diff()) == changes.get())
    // changes is its own canonical instance, so it gets a brand new
    // private data.
    changes->allocate_priv_data();
  else
    {
      // changes has a non-empty equivalence class so it's going to
      // share its private data with its canonical instance.  Next
      // time class_diff::get_priv() is invoked, it's going to return
      // the shared private data of the canonical instance.
      return changes;
    }

  // Compare base specs
  compute_diff(f->get_base_specifiers().begin(),
	       f->get_base_specifiers().end(),
	       s->get_base_specifiers().begin(),
	       s->get_base_specifiers().end(),
	       changes->base_changes());

  // Do *not* compare member types because it generates lots of noise
  // and I doubt it's really useful.
#if 0
  compute_diff(f->get_member_types().begin(),
	       f->get_member_types().end(),
	       s->get_member_types().begin(),
	       s->get_member_types().end(),
	       changes->member_types_changes());
#endif

  // Compare data member
  compute_diff(f->get_non_static_data_members().begin(),
	       f->get_non_static_data_members().end(),
	       s->get_non_static_data_members().begin(),
	       s->get_non_static_data_members().end(),
	       changes->data_members_changes());

  // Compare virtual member functions
  compute_diff(f->get_virtual_mem_fns().begin(),
	       f->get_virtual_mem_fns().end(),
	       s->get_virtual_mem_fns().begin(),
	       s->get_virtual_mem_fns().end(),
	       changes->member_fns_changes());

  // Compare member function templates
  compute_diff(f->get_member_function_templates().begin(),
	       f->get_member_function_templates().end(),
	       s->get_member_function_templates().begin(),
	       s->get_member_function_templates().end(),
	       changes->member_fn_tmpls_changes());

  // Likewise, do not compare member class templates
#if 0
  compute_diff(f->get_member_class_templates().begin(),
	       f->get_member_class_templates().end(),
	       s->get_member_class_templates().begin(),
	       s->get_member_class_templates().end(),
	       changes->member_class_tmpls_changes());
#endif

  changes->ensure_lookup_tables_populated();

  return changes;
}

//</class_diff stuff>

// <base_diff stuff>

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref base_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
base_diff::chain_into_hierarchy()
{append_child_node(get_underlying_class_diff());}

/// @param first the first base spec to consider.
///
/// @param second the second base spec to consider.
///
/// @param ctxt the context of the diff.  Note that this context
/// object must stay alive at least during the life time of the
/// current instance of @ref base_diff.  Otherwise memory corruption
/// issues occur.
base_diff::base_diff(class_decl::base_spec_sptr first,
		     class_decl::base_spec_sptr second,
		     class_diff_sptr		underlying,
		     diff_context_sptr		ctxt)
  : diff(first, second, ctxt),
    priv_(new priv(underlying))
{}

/// Finish building the current instance of @ref base_diff.
void
base_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;

  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first base spec of the diff object.
///
/// @return the first base specifier for the diff object.
class_decl::base_spec_sptr
base_diff::first_base() const
{return dynamic_pointer_cast<class_decl::base_spec>(first_subject());}

/// Getter for the second base spec of the diff object.
///
/// @return the second base specifier for the diff object.
class_decl::base_spec_sptr
base_diff::second_base() const
{return dynamic_pointer_cast<class_decl::base_spec>(second_subject());}

/// Getter for the diff object for the diff of the underlying base
/// classes.
///
/// @return the diff object for the diff of the underlying base
/// classes.
const class_diff_sptr
base_diff::get_underlying_class_diff() const
{return priv_->underlying_class_diff_;}

/// Setter for the diff object for the diff of the underlyng base
/// classes.
///
/// @param d the new diff object for the diff of the underlying base
/// classes.
void
base_diff::set_underlying_class_diff(class_diff_sptr d)
{priv_->underlying_class_diff_ = d;}

/// @return the pretty representation for the current instance of @ref
/// base_diff.
const string&
base_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "base_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// Return true iff the current diff node carries a change.
bool
base_diff::has_changes() const
{return first_base() != second_base();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
base_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_base(), *second_base(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Generates a report for the current instance of base_diff.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
base_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Constructs the diff object representing a diff between two base
/// class specifications.
///
/// Note that the two artifacts must have been created in the same
/// @ref environment, otherwise, this function aborts.
///
/// @param first the first base class specification.
///
/// @param second the second base class specification.
///
/// @param ctxt the content of the diff.
///
/// @return the resulting diff object.
base_diff_sptr
compute_diff(const class_decl::base_spec_sptr	first,
	     const class_decl::base_spec_sptr	second,
	     diff_context_sptr			ctxt)
{
  if (first && second)
    {
      ABG_ASSERT(first->get_environment() == second->get_environment());
      ABG_ASSERT(first->get_base_class()->get_environment()
	     == second->get_base_class()->get_environment());
      ABG_ASSERT(first->get_environment()
	     == first->get_base_class()->get_environment());
    }

  class_diff_sptr cl = compute_diff(first->get_base_class(),
				    second->get_base_class(),
				    ctxt);
  base_diff_sptr changes(new base_diff(first, second, cl, ctxt));

  ctxt->initialize_canonical_diff(changes);

  return changes;
}

// </base_diff stuff>


// <union_diff stuff>

/// Clear the lookup tables useful for reporting.
///
/// This function must be updated each time a lookup table is added or
/// removed from the union_diff::priv.
void
union_diff::clear_lookup_tables(void)
{class_or_union_diff::clear_lookup_tables();}

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
union_diff::lookup_tables_empty(void) const
{return class_or_union_diff::lookup_tables_empty();}

/// If the lookup tables are not yet built, walk the differences and
/// fill them.
void
union_diff::ensure_lookup_tables_populated(void) const
{class_or_union_diff::ensure_lookup_tables_populated();}

/// Allocate the memory for the priv_ pimpl data member of the @ref
/// union_diff class.
void
union_diff::allocate_priv_data()
{
  class_or_union_diff::allocate_priv_data();
}

/// Constructor for the @ref union_diff type.
///
/// @param first_union the first object of the comparison.
///
/// @param second_union the second object of the comparison.
///
/// @param ctxt the context of the comparison.
union_diff::union_diff(union_decl_sptr first_union,
		       union_decl_sptr second_union,
		       diff_context_sptr ctxt)
  : class_or_union_diff(first_union, second_union, ctxt)
{}

/// Finish building the current instance of @ref union_diff.
void
union_diff::finish_diff_type()
{class_or_union_diff::finish_diff_type();}

/// Destructor of the union_diff node.
union_diff::~union_diff()
{}

/// @return the first object of the comparison.
union_decl_sptr
union_diff::first_union_decl() const
{return is_union_type(first_subject());}

/// @return the second object of the comparison.
union_decl_sptr
union_diff::second_union_decl() const
{return is_union_type(second_subject());}

/// @return the pretty representation of the current diff node.
const string&
union_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "union_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Report the changes carried by the current @ref union_diff node in
/// a textual format.
///
/// @param out the output stream to write the textual report to.
///
/// @param indent the number of white space to use as indentation.
void
union_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the difference between two @ref union_decl types.
///
/// Note that the two types must hav been created in the same
/// environment, otherwise, this function aborts.
///
/// @param first the first @ref union_decl to consider.
///
/// @param second the second @ref union_decl to consider.
///
/// @param ctxt the context of the diff to use.
union_diff_sptr
compute_diff(const union_decl_sptr	first,
	     const union_decl_sptr	second,
	     diff_context_sptr	ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  union_diff_sptr changes(new union_diff(first, second, ctxt));

  ctxt->initialize_canonical_diff(changes);
  ABG_ASSERT(changes->get_canonical_diff());

  // Ok, so this is an optimization.  Do not freak out if it looks
  // weird, because, well, it does look weird.  This speeds up
  // greatly, for instance, the test case given at PR
  // libabigail/17948.
  //
  // We are setting the private data of the new instance of class_diff
  // (which is 'changes') to the private data of its canonical
  // instance.  That is, we are sharing the private data of 'changes'
  // with the private data of its canonical instance to consume less
  // memory in cases where the equivalence class of 'changes' is huge.
  //
  // But if changes is its own canonical instance, then we initialize
  // its private data properly.
  if (is_union_diff(changes->get_canonical_diff()) ==  changes.get())
    // changes is its own canonical instance, so it gets a brand new
    // private data.
    changes->allocate_priv_data();
  else
    {
      // changes has a non-empty equivalence class so it's going to
      // share its private data with its canonical instance.  Next
      // time class_diff::get_priv() is invoked, it's going to return
      // the shared private data of the canonical instance.
      return changes;
    }

  // Compare data member
  compute_diff(first->get_non_static_data_members().begin(),
	       first->get_non_static_data_members().end(),
	       second->get_non_static_data_members().begin(),
	       second->get_non_static_data_members().end(),
	       changes->data_members_changes());

#if 0
  // Compare member functions
  compute_diff(first->get_mem_fns().begin(),
	       first->get_mem_fns().end(),
	       second->get_mem_fns().begin(),
	       second->get_mem_fns().end(),
	       changes->member_fns_changes());

  // Compare member function templates
  compute_diff(first->get_member_function_templates().begin(),
	       first->get_member_function_templates().end(),
	       second->get_member_function_templates().begin(),
	       second->get_member_function_templates().end(),
	       changes->member_fn_tmpls_changes());
#endif

  changes->ensure_lookup_tables_populated();

  return changes;
}

// </union_diff stuff>

//<scope_diff stuff>

/// Clear the lookup tables that are useful for reporting.
///
/// This function must be updated each time a lookup table is added or
/// removed.
void
scope_diff::clear_lookup_tables()
{
  priv_->deleted_types_.clear();
  priv_->deleted_decls_.clear();
  priv_->inserted_types_.clear();
  priv_->inserted_decls_.clear();
  priv_->changed_types_.clear();
  priv_->changed_decls_.clear();
  priv_->removed_types_.clear();
  priv_->removed_decls_.clear();
  priv_->added_types_.clear();
  priv_->added_decls_.clear();
}

/// Tests if the lookup tables are empty.
///
/// This function must be updated each time a lookup table is added or
/// removed.
///
/// @return true iff all the lookup tables are empty.
bool
scope_diff::lookup_tables_empty() const
{
  return (priv_->deleted_types_.empty()
	  && priv_->deleted_decls_.empty()
	  && priv_->inserted_types_.empty()
	  && priv_->inserted_decls_.empty()
	  && priv_->changed_types_.empty()
	  && priv_->changed_decls_.empty()
	  && priv_->removed_types_.empty()
	  && priv_->removed_decls_.empty()
	  && priv_->added_types_.empty()
	  && priv_->added_decls_.empty());
}

/// If the lookup tables are not yet built, walk the member_changes_
/// member and fill the lookup tables.
void
scope_diff::ensure_lookup_tables_populated()
{
  if (!lookup_tables_empty())
    return;

  edit_script& e = priv_->member_changes_;

  // Populate deleted types & decls lookup tables.
  for (vector<deletion>::const_iterator i = e.deletions().begin();
       i != e.deletions().end();
       ++i)
    {
      decl_base_sptr decl = deleted_member_at(i);
      string qname = decl->get_qualified_name();
      if (is_type(decl))
	{
	  class_decl_sptr klass_decl = dynamic_pointer_cast<class_decl>(decl);
	  if (klass_decl && klass_decl->get_is_declaration_only())
	    continue;

	  ABG_ASSERT(priv_->deleted_types_.find(qname)
		 == priv_->deleted_types_.end());
	  priv_->deleted_types_[qname] = decl;
	}
      else
	{
	  ABG_ASSERT(priv_->deleted_decls_.find(qname)
		 == priv_->deleted_decls_.end());
	  priv_->deleted_decls_[qname] = decl;
	}
    }

  // Populate inserted types & decls as well as chagned types & decls
  // lookup tables.
  for (vector<insertion>::const_iterator it = e.insertions().begin();
       it != e.insertions().end();
       ++it)
    {
      for (vector<unsigned>::const_iterator i = it->inserted_indexes().begin();
	   i != it->inserted_indexes().end();
	   ++i)
	{
	  decl_base_sptr decl = inserted_member_at(i);
	  string qname = decl->get_qualified_name();
	  if (is_type(decl))
	    {
	      class_decl_sptr klass_decl =
		dynamic_pointer_cast<class_decl>(decl);
	      if (klass_decl && klass_decl->get_is_declaration_only())
		continue;

	      ABG_ASSERT(priv_->inserted_types_.find(qname)
		     == priv_->inserted_types_.end());
	      string_decl_base_sptr_map::const_iterator j =
		priv_->deleted_types_.find(qname);
	      if (j != priv_->deleted_types_.end())
		{
		  if (*j->second != *decl)
		    priv_->changed_types_[qname] =
		      compute_diff(j->second, decl, context());
		  priv_->deleted_types_.erase(j);
		}
	      else
		priv_->inserted_types_[qname] = decl;
	    }
	  else
	    {
	      ABG_ASSERT(priv_->inserted_decls_.find(qname)
		     == priv_->inserted_decls_.end());
	      string_decl_base_sptr_map::const_iterator j =
		priv_->deleted_decls_.find(qname);
	      if (j != priv_->deleted_decls_.end())
		{
		  if (*j->second != *decl)
		    priv_->changed_decls_[qname] =
		      compute_diff(j->second, decl, context());
		  priv_->deleted_decls_.erase(j);
		}
	      else
		priv_->inserted_decls_[qname] = decl;
	    }
	}
    }

  sort_string_diff_sptr_map(priv_->changed_decls_,
			    priv_->sorted_changed_decls_);
  sort_string_diff_sptr_map(priv_->changed_types_,
			    priv_->sorted_changed_types_);

  // Populate removed types/decls lookup tables
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_types_.begin();
       i != priv_->deleted_types_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->inserted_types_.find(i->first);
      if (r == priv_->inserted_types_.end())
	priv_->removed_types_[i->first] = i->second;
    }
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_decls_.begin();
       i != priv_->deleted_decls_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->inserted_decls_.find(i->first);
      if (r == priv_->inserted_decls_.end())
	priv_->removed_decls_[i->first] = i->second;
    }

  // Populate added types/decls.
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_types_.begin();
       i != priv_->inserted_types_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->deleted_types_.find(i->first);
      if (r == priv_->deleted_types_.end())
	priv_->added_types_[i->first] = i->second;
    }
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_decls_.begin();
       i != priv_->inserted_decls_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->deleted_decls_.find(i->first);
      if (r == priv_->deleted_decls_.end())
	priv_->added_decls_[i->first] = i->second;
    }
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref scope_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
scope_diff::chain_into_hierarchy()
{
  for (diff_sptrs_type::const_iterator i = changed_types().begin();
       i != changed_types().end();
       ++i)
    if (*i)
      append_child_node(*i);

  for (diff_sptrs_type::const_iterator i = changed_decls().begin();
       i != changed_decls().end();
       ++i)
    if (*i)
      append_child_node(*i);
}

/// Constructor for scope_diff
///
/// @param first_scope the first scope to consider for the diff.
///
/// @param second_scope the second scope to consider for the diff.
///
/// @param ctxt the diff context to use.  Note that this context
/// object must stay alive at least during the life time of the
/// current instance of @ref scope_diff.  Otherwise memory corruption
/// issues occur.
scope_diff::scope_diff(scope_decl_sptr first_scope,
		       scope_decl_sptr second_scope,
		       diff_context_sptr ctxt)
  : diff(first_scope, second_scope, ctxt),
    priv_(new priv)
{}

/// Finish building the current instance of @ref scope_diff.
void
scope_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first scope of the diff.
///
/// @return the first scope of the diff.
const scope_decl_sptr
scope_diff::first_scope() const
{return dynamic_pointer_cast<scope_decl>(first_subject());}

/// Getter for the second scope of the diff.
///
/// @return the second scope of the diff.
const scope_decl_sptr
scope_diff::second_scope() const
{return dynamic_pointer_cast<scope_decl>(second_subject());}

/// Accessor of the edit script of the members of a scope.
///
/// This edit script is computed using the equality operator that
/// applies to shared_ptr<decl_base>.
///
/// That has interesting consequences.  For instance, consider two
/// scopes S0 and S1.  S0 contains a class C0 and S1 contains a class
/// S0'.  C0 and C0' have the same qualified name, but have different
/// members.  The edit script will consider that C0 has been deleted
/// from S0 and that S0' has been inserted.  This is a low level
/// canonical representation of the changes; a higher level
/// representation would give us a simpler way to say "the class C0
/// has been modified into C0'".  But worry not.  We do have such
/// higher representation as well; that is what changed_types() and
/// changed_decls() is for.
///
/// @return the edit script of the changes encapsulatd in this
/// instance of scope_diff.
const edit_script&
scope_diff::member_changes() const
{return priv_->member_changes_;}

/// Accessor of the edit script of the members of a scope.
///
/// This edit script is computed using the equality operator that
/// applies to shared_ptr<decl_base>.
///
/// That has interesting consequences.  For instance, consider two
/// scopes S0 and S1.  S0 contains a class C0 and S1 contains a class
/// S0'.  C0 and C0' have the same qualified name, but have different
/// members.  The edit script will consider that C0 has been deleted
/// from S0 and that S0' has been inserted.  This is a low level
/// canonical representation of the changes; a higher level
/// representation would give us a simpler way to say "the class C0
/// has been modified into C0'".  But worry not.  We do have such
/// higher representation as well; that is what changed_types() and
/// changed_decls() is for.
///
/// @return the edit script of the changes encapsulatd in this
/// instance of scope_diff.
edit_script&
scope_diff::member_changes()
{return priv_->member_changes_;}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member that is reported
/// (in the edit script) as deleted at a given index.
///
/// @param i the index (in the edit script) of an element of the first
/// scope that has been reported as being delete.
///
/// @return the scope member that has been reported by the edit script
/// as being deleted at index i.
const decl_base_sptr
scope_diff::deleted_member_at(unsigned i) const
{
  scope_decl_sptr scope = dynamic_pointer_cast<scope_decl>(first_subject());
 return scope->get_member_decls()[i];
}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member (of the first scope
/// of this diff instance) that is reported (in the edit script) as
/// deleted at a given iterator.
///
/// @param i the iterator of an element of the first scope that has
/// been reported as being delete.
///
/// @return the scope member of the first scope of this diff that has
/// been reported by the edit script as being deleted at iterator i.
const decl_base_sptr
scope_diff::deleted_member_at(vector<deletion>::const_iterator i) const
{return deleted_member_at(i->index());}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member (of the second
/// scope of this diff instance) that is reported as being inserted
/// from a given index.
///
/// @param i the index of an element of the second scope this diff
/// that has been reported by the edit script as being inserted.
///
/// @return the scope member of the second scope of this diff that has
/// been reported as being inserted from index i.
const decl_base_sptr
scope_diff::inserted_member_at(unsigned i)
{
  scope_decl_sptr scope = dynamic_pointer_cast<scope_decl>(second_subject());
  return scope->get_member_decls()[i];
}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member (of the second
/// scope of this diff instance) that is reported as being inserted
/// from a given iterator.
///
/// @param i the iterator of an element of the second scope this diff
/// that has been reported by the edit script as being inserted.
///
/// @return the scope member of the second scope of this diff that has
/// been reported as being inserted from iterator i.
const decl_base_sptr
scope_diff::inserted_member_at(vector<unsigned>::const_iterator i)
{return inserted_member_at(*i);}

/// @return a sorted vector of the types which content has changed
/// from the first scope to the other.
const diff_sptrs_type&
scope_diff::changed_types() const
{return priv_->sorted_changed_types_;}

/// @return a sorted vector of the decls which content has changed
/// from the first scope to the other.
const diff_sptrs_type&
scope_diff::changed_decls() const
{return priv_->sorted_changed_decls_;}

const string_decl_base_sptr_map&
scope_diff::removed_types() const
{return priv_->removed_types_;}

const string_decl_base_sptr_map&
scope_diff::removed_decls() const
{return priv_->removed_decls_;}

const string_decl_base_sptr_map&
scope_diff::added_types() const
{return priv_->added_types_;}

const string_decl_base_sptr_map&
scope_diff::added_decls() const
{return priv_->added_decls_;}

/// @return the pretty representation for the current instance of @ref
/// scope_diff.
const string&
scope_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "scope_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// Return true iff the current diff node carries a change.
bool
scope_diff::has_changes() const
{
  // TODO: add the number of really removed/added stuff.
  return changed_types().size() + changed_decls().size();
}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
scope_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_scope(), *second_scope(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Report the changes of one scope against another.
///
/// @param out the out stream to report the changes to.
///
/// @param indent the string to use for indentation.
void
scope_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the diff between two scopes.
///
/// Note that the two decls must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first scope to consider in computing the diff.
///
/// @param second the second scope to consider in the diff
/// computation.  The second scope is diffed against the first scope.
///
/// @param d a pointer to the diff object to populate with the
/// computed diff.
///
/// @return return the populated \a d parameter passed to this
/// function.
///
/// @param ctxt the diff context to use.
scope_diff_sptr
compute_diff(const scope_decl_sptr	first,
	     const scope_decl_sptr	second,
	     scope_diff_sptr		d,
	     diff_context_sptr		ctxt)
{
  ABG_ASSERT(d->first_scope() == first && d->second_scope() == second);

  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  compute_diff(first->get_member_decls().begin(),
	       first->get_member_decls().end(),
	       second->get_member_decls().begin(),
	       second->get_member_decls().end(),
	       d->member_changes());

  d->ensure_lookup_tables_populated();
  d->context(ctxt);

  return d;
}

/// Compute the diff between two scopes.
///
/// Note that the two decls must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first_scope the first scope to consider in computing the diff.
///
/// @param second_scope the second scope to consider in the diff
/// computation.  The second scope is diffed against the first scope.
///
/// @param ctxt the diff context to use.
///
/// @return return the resulting diff
scope_diff_sptr
compute_diff(const scope_decl_sptr	first_scope,
	     const scope_decl_sptr	second_scope,
	     diff_context_sptr		ctxt)
{
  if (first_scope && second_scope)
    ABG_ASSERT(first_scope->get_environment()
	   == second_scope->get_environment());

  scope_diff_sptr d(new scope_diff(first_scope, second_scope, ctxt));
  d = compute_diff(first_scope, second_scope, d, ctxt);
  ctxt->initialize_canonical_diff(d);
  return d;
}

//</scope_diff stuff>

// <fn_parm_diff stuff>

/// Constructor for the fn_parm_diff type.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param ctxt the context of the diff.  Note that this context
/// object must stay alive at least during the life time of the
/// current instance of @ref fn_parm_diff.  Otherwise memory
/// corruption issues occur.
fn_parm_diff::fn_parm_diff(const function_decl::parameter_sptr	first,
			   const function_decl::parameter_sptr	second,
			   diff_context_sptr			ctxt)
  : decl_diff_base(first, second, ctxt),
    priv_(new priv)
{
  ABG_ASSERT(first->get_index() == second->get_index());
  priv_->type_diff = compute_diff(first->get_type(),
				  second->get_type(),
				  ctxt);
  ABG_ASSERT(priv_->type_diff);
}

/// Finish the building of the current instance of @ref fn_parm_diff.
void
fn_parm_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first subject of this diff node.
///
/// @return the first function_decl::parameter_sptr subject of this
/// diff node.
const function_decl::parameter_sptr
fn_parm_diff::first_parameter() const
{return dynamic_pointer_cast<function_decl::parameter>(first_subject());}

/// Getter for the second subject of this diff node.
///
/// @return the second function_decl::parameter_sptr subject of this
/// diff node.
const function_decl::parameter_sptr
fn_parm_diff::second_parameter() const
{return dynamic_pointer_cast<function_decl::parameter>(second_subject());}

/// Getter for the diff representing the changes on the type of the
/// function parameter involved in the current instance of @ref
/// fn_parm_diff.
///
/// @return a diff_sptr representing the changes on the type of the
/// function parameter we are interested in.
diff_sptr
fn_parm_diff::type_diff() const
{return priv_->type_diff;}

/// Build and return a textual representation of the current instance
/// of @ref fn_parm_diff.
///
/// @return the string representing the current instance of
/// fn_parm_diff.
const string&
fn_parm_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "function_parameter_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
fn_parm_diff::has_changes() const
{return *first_parameter() != *second_parameter();}

/// Check if the current diff node carries a local change.
///
/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
fn_parm_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_parameter(), *second_parameter(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Emit a textual report about the current fn_parm_diff instance.
///
/// @param out the output stream to emit the textual report to.
///
/// @param indent the indentation string to use in the report.
void
fn_parm_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Populate the vector of children nodes of the @ref diff base type
/// sub-object of this instance of @ref fn_parm_diff.
///
/// The children nodes can then later be retrieved using
/// diff::children_nodes()
void
fn_parm_diff::chain_into_hierarchy()
{
  if (type_diff())
    append_child_node(type_diff());
}

/// Compute the difference between two function_decl::parameter_sptr;
/// that is, between two function parameters.  Return a resulting
/// fn_parm_diff_sptr that represents the changes.
///
/// Note that the two decls must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param ctxt the context of the diff.
///
/// @return fn_parm_diff_sptr the resulting diff node.
fn_parm_diff_sptr
compute_diff(const function_decl::parameter_sptr	first,
	     const function_decl::parameter_sptr	second,
	     diff_context_sptr				ctxt)
{
  if (!first || !second)
    return fn_parm_diff_sptr();

  ABG_ASSERT(first->get_environment() == second->get_environment());

  fn_parm_diff_sptr result(new fn_parm_diff(first, second, ctxt));
  ctxt->initialize_canonical_diff(result);

  return result;
}
// </fn_parm_diff stuff>

// <function_type_diff stuff>

void
function_type_diff::ensure_lookup_tables_populated()
{
  priv_->return_type_diff_ =
    compute_diff(first_function_type()->get_return_type(),
		 second_function_type()->get_return_type(),
		 context());

  string parm_name;
  function_decl::parameter_sptr parm;
  for (vector<deletion>::const_iterator i =
	 priv_->parm_changes_.deletions().begin();
       i != priv_->parm_changes_.deletions().end();
       ++i)
    {
      parm = *(first_function_type()->get_first_parm()
	       + i->index());
      parm_name = parm->get_name_id();
      // If for a reason the type name is empty we want to know and
      // fix that.
      ABG_ASSERT(!parm_name.empty());
      priv_->deleted_parms_[parm_name] = parm;
      priv_->deleted_parms_by_id_[parm->get_index()] = parm;
    }

  for (vector<insertion>::const_iterator i =
	 priv_->parm_changes_.insertions().begin();
       i != priv_->parm_changes_.insertions().end();
       ++i)
    {
      for (vector<unsigned>::const_iterator j =
	     i->inserted_indexes().begin();
	   j != i->inserted_indexes().end();
	   ++j)
	{
	  parm = *(second_function_type()->get_first_parm() + *j);
	  parm_name = parm->get_name_id();
	  // If for a reason the type name is empty we want to know and
	  // fix that.
	  ABG_ASSERT(!parm_name.empty());
	  {
	    string_parm_map::const_iterator k =
	      priv_->deleted_parms_.find(parm_name);
	    if (k != priv_->deleted_parms_.end())
	      {
		if (*k->second != *parm)
		  priv_->subtype_changed_parms_[parm_name] =
		    compute_diff(k->second, parm, context());
		priv_->deleted_parms_.erase(parm_name);
	      }
	    else
	      priv_->added_parms_[parm_name] = parm;
	  }
	  {
	    unsigned_parm_map::const_iterator k =
	      priv_->deleted_parms_by_id_.find(parm->get_index());
	    if (k != priv_->deleted_parms_by_id_.end())
	      {
		if (*k->second != *parm
		    && (k->second->get_name_id() != parm_name))
		  priv_->changed_parms_by_id_[parm->get_index()] =
		    compute_diff(k->second, parm, context());
		priv_->added_parms_.erase(parm_name);
		priv_->deleted_parms_.erase(k->second->get_name_id());
		priv_->deleted_parms_by_id_.erase(parm->get_index());
	      }
	    else
	      priv_->added_parms_by_id_[parm->get_index()] = parm;
	  }
	}
    }

  sort_string_fn_parm_diff_sptr_map(priv_->subtype_changed_parms_,
				    priv_->sorted_subtype_changed_parms_);
  sort_string_fn_parm_diff_sptr_map(priv_->changed_parms_by_id_,
				    priv_->sorted_changed_parms_by_id_);
  sort_string_parm_map(priv_->deleted_parms_,
		       priv_->sorted_deleted_parms_);

  sort_string_parm_map(priv_->added_parms_,
		       priv_->sorted_added_parms_);
}

/// In the vector of deleted parameters, get the one that is at a given
/// index.
///
/// @param i the index of the deleted parameter to get.
///
/// @return the parameter returned.
const function_decl::parameter_sptr
function_type_diff::deleted_parameter_at(int i) const
{return first_function_type()->get_parameters()[i];}

/// Getter for the sorted vector of deleted parameters.
///
/// @return the sorted vector of deleted parameters.
const vector<function_decl::parameter_sptr>&
function_type_diff::sorted_deleted_parms() const
{return priv_->sorted_deleted_parms_;}

/// Getter for the sorted vector of added parameters .
///
/// @return the sorted vector of added parameters.
const vector<function_decl::parameter_sptr>&
function_type_diff::sorted_added_parms() const
{return priv_->sorted_added_parms_;}

/// In the vector of inserted parameters, get the one that is at a
/// given index.
///
/// @param i the index of the inserted parameter to get.
///
/// @return the parameter returned.
const function_decl::parameter_sptr
function_type_diff::inserted_parameter_at(int i) const
{return second_function_type()->get_parameters()[i];}

/// Consutrctor of the @ref function_type type.
///
/// @param first the first @ref function_type subject of the diff to
/// create.
///
/// @param second the second @ref function_type subject of the diff to
/// create.
///
/// @param ctxt the diff context to be used by the newly created
/// instance of function_type_diff.  Note that this context object
/// must stay alive at least during the life time of the current
/// instance of @ref function_type_diff.  Otherwise memory corruption
/// issues occur.
function_type_diff::function_type_diff(const function_type_sptr first,
				       const function_type_sptr second,
				       diff_context_sptr	ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv)
{}

/// Finish building the current instance of @ref function_type_diff
void
function_type_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first subject of the diff.
///
/// @return the first function type involved in the diff.
const function_type_sptr
function_type_diff::first_function_type() const
{return dynamic_pointer_cast<function_type>(first_subject());}

/// Getter for the second subject of the diff.
///
/// @return the second function type involved in the diff.
const function_type_sptr
function_type_diff::second_function_type() const
{return dynamic_pointer_cast<function_type>(second_subject());}

/// Getter for the diff of the return types of the two function types
/// of the current diff.
///
/// @return the diff of the return types of the two function types of
/// the current diff.
const diff_sptr
function_type_diff::return_type_diff() const
{return priv_->return_type_diff_;}

/// Getter for the map of function parameter changes of the current diff.
///
/// @return a map of function parameter changes of the current diff.
const string_fn_parm_diff_sptr_map&
function_type_diff::subtype_changed_parms() const
{return priv_->subtype_changed_parms_;}

/// Getter for the map of parameters that got removed.
///
/// @return the map of parameters that got removed.
const string_parm_map&
function_type_diff::removed_parms() const
{return priv_->deleted_parms_;}

/// Getter for the map of parameters that got added.
///
/// @return the map of parameters that got added.
const string_parm_map&
function_type_diff::added_parms() const
{return priv_->added_parms_;}

/// Build and return a copy of a pretty representation of the current
/// instance of @ref function_type_diff.
///
/// @return a copy of the pretty representation of the current
/// instance of @ref function_type_diff.
const string&
function_type_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "function_type_diff["
	<< abigail::ir::get_pretty_representation(first_function_type())
	<< ", "
	<< abigail::ir::get_pretty_representation(second_function_type())
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Test if the current diff node carries changes.
///
/// @return true iff the current diff node carries changes.
bool
function_type_diff::has_changes() const
{return *first_function_type() != *second_function_type();}

/// Test if the current diff node carries local changes.
///
/// A local change is a change that is carried by this diff node, not
/// by any of its children nodes.
///
/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
function_type_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_function_type(), *second_function_type(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Build and emit a textual report about the current @ref
/// function_type_diff instance.
///
/// @param out the output stream.
///
/// @param indent the indentation string to use.
void
function_type_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref function_type_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
function_type_diff::chain_into_hierarchy()
{
  if (diff_sptr d = return_type_diff())
    append_child_node(d);

  for (vector<fn_parm_diff_sptr>::const_iterator i =
	 priv_->sorted_subtype_changed_parms_.begin();
       i != priv_->sorted_subtype_changed_parms_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);

  for (vector<fn_parm_diff_sptr>::const_iterator i =
	 priv_->sorted_changed_parms_by_id_.begin();
       i != priv_->sorted_changed_parms_by_id_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);
}

/// Compute the diff between two instances of @ref function_type.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first @ref function_type to consider for the diff.
///
/// @param second the second @ref function_type to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff between the two @ref function_type.
function_type_diff_sptr
compute_diff(const function_type_sptr	first,
	     const function_type_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (!first || !second)
    {
      // TODO: implement this for either first or second being NULL.
      return function_type_diff_sptr();
    }

  ABG_ASSERT(first->get_environment() == second->get_environment());

  function_type_diff_sptr result(new function_type_diff(first, second, ctxt));

  diff_utils::compute_diff(first->get_first_parm(),
			   first->get_parameters().end(),
			   second->get_first_parm(),
			   second->get_parameters().end(),
			   result->priv_->parm_changes_);

  result->ensure_lookup_tables_populated();

  ctxt->initialize_canonical_diff(result);

  return result;
}
// </function_type_diff stuff>

// <function_decl_diff stuff>

/// Build the lookup tables of the diff, if necessary.
void
function_decl_diff::ensure_lookup_tables_populated()
{
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref function_decl_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
function_decl_diff::chain_into_hierarchy()
{
  if (diff_sptr d = type_diff())
    append_child_node(d);
}

/// Constructor for function_decl_diff
///
/// @param first the first function considered by the diff.
///
/// @param second the second function considered by the diff.
///
/// @param ctxt the context of the diff.  Note that this context
/// object must stay alive at least during the life time of the
/// current instance of @ref function_decl_diff.  Otherwise memory
/// corruption issues occur.
function_decl_diff::function_decl_diff(const function_decl_sptr first,
				       const function_decl_sptr second,
				       diff_context_sptr	ctxt)
  : decl_diff_base(first, second, ctxt),
    priv_(new priv)
{
}

/// Finish building the current instance of @ref function_decl_diff.
void
function_decl_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// @return the first function considered by the diff.
const function_decl_sptr
function_decl_diff::first_function_decl() const
{return dynamic_pointer_cast<function_decl>(first_subject());}

/// @return the second function considered by the diff.
const function_decl_sptr
function_decl_diff::second_function_decl() const
{return dynamic_pointer_cast<function_decl>(second_subject());}

const function_type_diff_sptr
function_decl_diff::type_diff() const
{return priv_->type_diff_;}

/// @return the pretty representation for the current instance of @ref
/// function_decl_diff.
const string&
function_decl_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "function_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
function_decl_diff::has_changes() const
{return *first_function_decl() != *second_function_decl();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
function_decl_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_function_decl(), *second_function_decl(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Serialize a report of the changes encapsulated in the current
/// instance of @ref function_decl_diff over to an output stream.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the string to use an an indentation prefix.
void
function_decl_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute the diff between two function_decl.
///
/// Note that the two decls must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first the first function_decl to consider for the diff
///
/// @param second the second function_decl to consider for the diff
///
/// @param ctxt the diff context to use.
///
/// @return the computed diff
function_decl_diff_sptr
compute_diff(const function_decl_sptr first,
	     const function_decl_sptr second,
	     diff_context_sptr ctxt)
{
  if (!first || !second)
    {
      // TODO: implement this for either first or second being NULL.
      return function_decl_diff_sptr();
    }

  ABG_ASSERT(first->get_environment() == second->get_environment());

  function_type_diff_sptr type_diff = compute_diff(first->get_type(),
						   second->get_type(),
						   ctxt);

  function_decl_diff_sptr result(new function_decl_diff(first, second,
							ctxt));
  result->priv_->type_diff_ = type_diff;

  result->ensure_lookup_tables_populated();

  ctxt->initialize_canonical_diff(result);

  return result;
}

// </function_decl_diff stuff>

// <type_decl_diff stuff>

/// Constructor for type_decl_diff.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param ctxt the context of the diff.  Note that this context
/// object must stay alive at least during the life time of the
/// current instance of @ref type_decl_diff.  Otherwise memory
/// corruption issues occur.
type_decl_diff::type_decl_diff(const type_decl_sptr first,
			       const type_decl_sptr second,
			       diff_context_sptr ctxt)
  : type_diff_base(first, second, ctxt)
{}

/// Finish building the current instance of @ref type_decl_diff.
void
type_decl_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  diff::priv_->finished_ = true;
}

/// Getter for the first subject of the type_decl_diff.
///
/// @return the first type_decl involved in the diff.
const type_decl_sptr
type_decl_diff::first_type_decl() const
{return dynamic_pointer_cast<type_decl>(first_subject());}

/// Getter for the second subject of the type_decl_diff.
///
/// @return the second type_decl involved in the diff.
const type_decl_sptr
type_decl_diff::second_type_decl() const
{return dynamic_pointer_cast<type_decl>(second_subject());}

/// @return the pretty representation for the current instance of @ref
/// type_decl_diff.
const string&
type_decl_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "type_decl_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}
/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
type_decl_diff::has_changes() const
{return first_type_decl() != second_type_decl();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
type_decl_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_type_decl(), *second_type_decl(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}
/// Ouputs a report of the differences between of the two type_decl
/// involved in the type_decl_diff.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the string to use for indentatino indent.
void
type_decl_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute a diff between two type_decl.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// This function doesn't actually compute a diff.  As a type_decl is
/// very simple (unlike compound constructs like function_decl or
/// class_decl) it's easy to just compare the components of the
/// type_decl to know what has changed.  Thus this function just
/// builds and return a type_decl_diff object.  The
/// type_decl_diff::report function will just compare the components
/// of the the two type_decl and display where and how they differ.
///
/// @param first a pointer to the first type_decl to
/// consider.
///
/// @param second a pointer to the second type_decl to consider.
///
/// @param ctxt the diff context to use.
///
/// @return a pointer to the resulting type_decl_diff.
type_decl_diff_sptr
compute_diff(const type_decl_sptr	first,
	     const type_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  type_decl_diff_sptr result(new type_decl_diff(first, second, ctxt));

  // We don't need to actually compute a diff here as a type_decl
  // doesn't have complicated sub-components.  type_decl_diff::report
  // just walks the members of the type_decls and display information
  // about the ones that have changed.  On a similar note,
  // type_decl_diff::length returns 0 if the two type_decls are equal,
  // and 1 otherwise.

  ctxt->initialize_canonical_diff(result);

  return result;
}

// </type_decl_diff stuff>

// <typedef_diff stuff>

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref typedef_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
typedef_diff::chain_into_hierarchy()
{append_child_node(underlying_type_diff());}

/// Constructor for typedef_diff.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param underlying the underlying diff of the @ref typedef_diff.
/// That is the diff between the underlying types of @p first and @p
/// second.
///
/// @param ctxt the context of the diff.  Note that this context
/// object must stay alive at least during the life time of the
/// current instance of @ref typedef_diff.  Otherwise memory
/// corruption issues occur.
typedef_diff::typedef_diff(const typedef_decl_sptr	first,
			   const typedef_decl_sptr	second,
			   const diff_sptr		underlying,
			   diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv(underlying))
{}

/// Finish building the current instance of @ref typedef_diff.
void
typedef_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the firt typedef_decl involved in the diff.
///
/// @return the first subject of the diff.
const typedef_decl_sptr
typedef_diff::first_typedef_decl() const
{return dynamic_pointer_cast<typedef_decl>(first_subject());}

/// Getter for the second typedef_decl involved in the diff.
///
/// @return the second subject of the diff.
const typedef_decl_sptr
typedef_diff::second_typedef_decl() const
{return dynamic_pointer_cast<typedef_decl>(second_subject());}

/// Getter for the diff between the two underlying types of the
/// typedefs.
///
/// @return the diff object reprensenting the difference between the
/// two underlying types of the typedefs.
const diff_sptr
typedef_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// Setter for the diff between the two underlying types of the
/// typedefs.
///
/// @param d the new diff object reprensenting the difference between
/// the two underlying types of the typedefs.
void
typedef_diff::underlying_type_diff(const diff_sptr d)
{priv_->underlying_type_diff_ = d;}

/// @return the pretty representation for the current instance of @ref
/// typedef_diff.
const string&
typedef_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "typedef_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
typedef_diff::has_changes() const
{
  decl_base_sptr second = second_typedef_decl();
  return !(*first_typedef_decl() == *second);
}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
typedef_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_typedef_decl(), *second_typedef_decl(), &k))
    return k & ir::ALL_LOCAL_CHANGES_MASK;
  return ir::NO_CHANGE_KIND;
}

/// Reports the difference between the two subjects of the diff in a
/// serialized form.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the indentation string to use.
void
typedef_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Compute a diff between two typedef_decl.
///
/// Note that the two types must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param first a pointer to the first typedef_decl to consider.
///
/// @param second a pointer to the second typedef_decl to consider.
///
/// @param ctxt the diff context to use.
///
/// @return a pointer to the the resulting typedef_diff.
typedef_diff_sptr
compute_diff(const typedef_decl_sptr	first,
	     const typedef_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (first && second)
    ABG_ASSERT(first->get_environment() == second->get_environment());

  diff_sptr d = compute_diff_for_types(first->get_underlying_type(),
				       second->get_underlying_type(),
				       ctxt);
  typedef_diff_sptr result(new typedef_diff(first, second, d, ctxt));

  ctxt->initialize_canonical_diff(result);

  return result;
}

/// Return the leaf underlying diff node of a @ref typedef_diff node.
///
/// If the underlying diff node of a @ref typedef_diff node is itself
/// a @ref typedef_diff node, then recursively look at the underlying
/// diff nodes to get the first one that is not a a @ref typedef_diff
/// node.  This is what a leaf underlying diff node means.
///
/// Otherwise, if the underlying diff node of @ref typedef_diff is
/// *NOT* a @ref typedef_diff node, then just return the underlying
/// diff node.
///
/// And if the diff node considered is not a @ref typedef_diff node,
/// then just return it.
///
/// @return the leaf underlying diff node of a @p diff.
const diff*
get_typedef_diff_underlying_type_diff(const diff* diff)
{
  const typedef_diff* d = dynamic_cast<const typedef_diff*>(diff);
  if (!d)
    return diff;

  if (const typedef_diff* deef =
      dynamic_cast<const typedef_diff*>(d->underlying_type_diff().get()))
    return get_typedef_diff_underlying_type_diff(deef);

  return d->underlying_type_diff().get();
}

// </typedef_diff stuff>

// <translation_unit_diff stuff>

/// Constructor for translation_unit_diff.
///
/// @param first the first translation unit to consider for this diff.
///
/// @param second the second translation unit to consider for this diff.
///
/// @param ctxt the context of the diff.  Note that this context
/// object must stay alive at least during the life time of the
/// current instance of @ref translation_unit_diff.  Otherwise memory
/// corruption issues occur.
translation_unit_diff::translation_unit_diff(translation_unit_sptr first,
					     translation_unit_sptr second,
					     diff_context_sptr ctxt)
  : scope_diff(first->get_global_scope(), second->get_global_scope(), ctxt),
    priv_(new priv(first, second))
{
}

/// Getter for the first translation unit of this diff.
///
/// @return the first translation unit of this diff.
const translation_unit_sptr
translation_unit_diff::first_translation_unit() const
{return priv_->first_;}

/// Getter for the second translation unit of this diff.
///
/// @return the second translation unit of this diff.
const translation_unit_sptr
translation_unit_diff::second_translation_unit() const
{return priv_->second_;}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
translation_unit_diff::has_changes() const
{return scope_diff::has_changes();}

/// @return the kind of local change carried by the current diff node.
/// The value returned is zero if the current node carries no local
/// change.
enum change_kind
translation_unit_diff::has_local_changes() const
{return ir::NO_CHANGE_KIND;}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the prefix to use as indentation for the report.
void
translation_unit_diff::report(ostream& out, const string& indent) const
{scope_diff::report(out, indent);}

/// Compute the diff between two translation_units.
///
/// Note that the two translation units must have been created in the
/// same @ref environment, otherwise, this function aborts.
///
/// @param first the first translation_unit to consider.
///
/// @param second the second translation_unit to consider.
///
/// @param ctxt the diff context to use.  If null, this function will
/// create a new context and set to the diff object returned.
///
/// @return the newly created diff object.
translation_unit_diff_sptr
compute_diff(const translation_unit_sptr	first,
	     const translation_unit_sptr	second,
	     diff_context_sptr			ctxt)
{
  ABG_ASSERT(first && second);

  ABG_ASSERT(first->get_environment() == second->get_environment());

  if (!ctxt)
    ctxt.reset(new diff_context);

  // TODO: handle first or second having empty contents.
  translation_unit_diff_sptr tu_diff(new translation_unit_diff(first, second,
							       ctxt));
  scope_diff_sptr sc_diff = dynamic_pointer_cast<scope_diff>(tu_diff);

  compute_diff(static_pointer_cast<scope_decl>(first->get_global_scope()),
	       static_pointer_cast<scope_decl>(second->get_global_scope()),
	       sc_diff,
	       ctxt);

  ctxt->initialize_canonical_diff(tu_diff);

  return tu_diff;
}

// </translation_unit_diff stuff>

// <diff_maps stuff>

/// The private data of the @ref diff_maps type.
struct diff_maps::priv
{
  string_diff_ptr_map type_decl_diff_map_;
  string_diff_ptr_map enum_diff_map_;
  string_diff_ptr_map class_diff_map_;
  string_diff_ptr_map union_diff_map_;
  string_diff_ptr_map typedef_diff_map_;
  string_diff_ptr_map array_diff_map_;
  string_diff_ptr_map reference_diff_map_;
  string_diff_ptr_map function_type_diff_map_;
  string_diff_ptr_map function_decl_diff_map_;
  string_diff_ptr_map var_decl_diff_map_;
  string_diff_ptr_map distinct_diff_map_;
  string_diff_ptr_map fn_parm_diff_map_;
  diff_artifact_set_map_type impacted_artifacts_map_;
}; // end struct diff_maps::priv

/// Default constructor of the @ref diff_maps type.
diff_maps::diff_maps()
  : priv_(new diff_maps::priv())
{}

/// Getter of the map that contains basic type diffs.
///
/// @return the map that contains basic type diffs.
const string_diff_ptr_map&
diff_maps::get_type_decl_diff_map() const
{return priv_->type_decl_diff_map_;}

/// Getter of the map that contains basic type diffs.
///
/// @return the map that contains basic type diffs.
string_diff_ptr_map&
diff_maps::get_type_decl_diff_map()
{return priv_->type_decl_diff_map_;}

/// Getter of the map that contains enum type diffs.
///
/// @return the map that contains enum type diffs.
const string_diff_ptr_map&
diff_maps::get_enum_diff_map() const
{return priv_->enum_diff_map_;}

/// Getter of the map that contains enum type diffs.
///
/// @return the map that contains enum type diffs.
string_diff_ptr_map&
diff_maps::get_enum_diff_map()
{return priv_->enum_diff_map_;}

/// Getter of the map that contains class type diffs.
///
/// @return the map that contains class type diffs.
const string_diff_ptr_map&
diff_maps::get_class_diff_map() const
{return priv_->class_diff_map_;}

/// Getter of the map that contains class type diffs.
///
/// @return the map that contains class type diffs.
string_diff_ptr_map&
diff_maps::get_class_diff_map()
{return priv_->class_diff_map_;}

/// Getter of the map that contains union type diffs.
///
/// @return the map that contains union type diffs.
const string_diff_ptr_map&
diff_maps::get_union_diff_map() const
{return priv_->union_diff_map_;}

/// Getter of the map that contains union type diffs.
///
/// @return the map that contains union type diffs.
string_diff_ptr_map&
diff_maps::get_union_diff_map()
{return priv_->union_diff_map_;}

/// Getter of the map that contains typedef type diffs.
///
/// @return the map that contains typedef type diffs.
const string_diff_ptr_map&
diff_maps::get_typedef_diff_map() const
{return priv_->typedef_diff_map_;}

/// Getter of the map that contains typedef type diffs.
///
/// @return the map that contains typedef type diffs.
string_diff_ptr_map&
diff_maps::get_typedef_diff_map()
{return priv_->typedef_diff_map_;}

/// Getter of the map that contains array type diffs.
///
/// @return the map that contains array type diffs.
const string_diff_ptr_map&
diff_maps::get_array_diff_map() const
{return priv_->array_diff_map_;}

/// Getter of the map that contains array type diffs.
///
/// @return the map that contains array type diffs.
string_diff_ptr_map&
diff_maps::get_array_diff_map()
{return priv_->array_diff_map_;}

/// Getter of the map that contains reference type diffs.
///
/// @return the map that contains reference type diffs.
const string_diff_ptr_map&
diff_maps::get_reference_diff_map() const
{return priv_->reference_diff_map_;}

/// Getter of the map that contains reference type diffs.
///
/// @return the map that contains reference type diffs.
string_diff_ptr_map&
diff_maps::get_reference_diff_map()
{{return priv_->reference_diff_map_;}}

/// Getter of the map that contains function parameter diffs.
///
/// @return the map that contains function parameter diffs.
const string_diff_ptr_map&
diff_maps::get_fn_parm_diff_map() const
{return priv_->fn_parm_diff_map_;}

/// Getter of the map that contains function parameter diffs.
///
/// @return the map that contains function parameter diffs.
string_diff_ptr_map&
diff_maps::get_fn_parm_diff_map()
{return priv_->fn_parm_diff_map_;}

/// Getter of the map that contains function type diffs.
///
/// @return the map that contains function type diffs.
const string_diff_ptr_map&
diff_maps::get_function_type_diff_map() const
{return priv_->function_type_diff_map_;}

/// Getter of the map that contains function type diffs.
///
/// @return the map that contains function type diffs.
string_diff_ptr_map&
diff_maps::get_function_type_diff_map()
{return priv_->function_type_diff_map_;}

/// Getter of the map that contains function decl diffs.
///
/// @return the map that contains function decl diffs.
const string_diff_ptr_map&
diff_maps::get_function_decl_diff_map() const
{return priv_->function_decl_diff_map_;}

/// Getter of the map that contains function decl diffs.
///
/// @return the map that contains function decl diffs.
string_diff_ptr_map&
diff_maps::get_function_decl_diff_map()
{return priv_->function_decl_diff_map_;}

/// Getter of the map that contains var decl diffs.
///
/// @return the map that contains var decl diffs.
const string_diff_ptr_map&
diff_maps::get_var_decl_diff_map() const
{return priv_->var_decl_diff_map_;}

/// Getter of the map that contains var decl diffs.
///
/// @return the map that contains var decl diffs.
string_diff_ptr_map&
diff_maps::get_var_decl_diff_map()
{return priv_->var_decl_diff_map_;}

/// Getter of the map that contains distinct diffs.
///
/// @return the map that contains distinct diffs.
const string_diff_ptr_map&
diff_maps::get_distinct_diff_map() const
{return priv_->distinct_diff_map_;}

/// Getter of the map that contains distinct diffs.
///
/// @return the map that contains distinct diffs.
string_diff_ptr_map&
diff_maps::get_distinct_diff_map()
{return priv_->distinct_diff_map_;}

/// Insert a new diff node into the current instance of @ref diff_maps.
///
/// @param dif the new diff node to insert into the @ref diff_maps.
///
/// @param impacted_iface the interface (global function or variable)
/// currently being analysed that led to analysing the diff node @p
/// dif.  In other words, this is the interface impacted by the diff
/// node @p dif.  Note that this can be nil in cases where we are
/// directly analysing changes to a type that is not reachable from
/// any global function or variable.
///
/// @return true iff the diff node could be added to the current
/// instance of @ref diff_maps.
bool
diff_maps::insert_diff_node(const diff *dif,
			    const type_or_decl_base_sptr& impacted_iface)
{
  string n = get_pretty_representation(dif->first_subject(),
				       /*internal=*/true);
  if (const type_decl_diff *d = is_diff_of_basic_type(dif))
    get_type_decl_diff_map()[n] = const_cast<type_decl_diff*>(d);
  else if (const enum_diff *d = is_enum_diff(dif))
    get_enum_diff_map()[n] = const_cast<enum_diff*>(d);
  else if (const class_diff *d = is_class_diff(dif))
      get_class_diff_map()[n] = const_cast<class_diff*>(d);
  else if (const union_diff *d = is_union_diff(dif))
    get_union_diff_map()[n] = const_cast<union_diff*>(d);
  else if (const typedef_diff *d = is_typedef_diff(dif))
    get_typedef_diff_map()[n] = const_cast<typedef_diff*>(d);
  else if (const array_diff *d = is_array_diff(dif))
      get_array_diff_map()[n] = const_cast<array_diff*>(d);
  else if (const reference_diff *d = is_reference_diff(dif))
    get_reference_diff_map()[n] = const_cast<reference_diff*>(d);
  else if (const fn_parm_diff *d = is_fn_parm_diff(dif))
    get_fn_parm_diff_map()[n] = const_cast<fn_parm_diff*>(d);
  else if (const function_type_diff *d = is_function_type_diff(dif))
    get_function_type_diff_map()[n] = const_cast<function_type_diff*>(d);
  else if (const var_diff *d = is_var_diff(dif))
    get_var_decl_diff_map()[n] = const_cast<var_diff*>(d);
  else if (const function_decl_diff *d = is_function_decl_diff(dif))
    get_function_decl_diff_map()[n] = const_cast<function_decl_diff*>(d);
  else if (const distinct_diff *d = is_distinct_diff(dif))
    get_distinct_diff_map()[n] = const_cast<distinct_diff*>(d);
  else if (is_base_diff(dif))
    // we silently drop this case.
    return true;
  else
      ABG_ASSERT_NOT_REACHED;

  // Update the map that associates this diff node to the set of
  // interfaces it impacts.

  if (impacted_iface)
    {
      diff_artifact_set_map_type::iterator i =
	priv_->impacted_artifacts_map_.find(dif);

      if (i == priv_->impacted_artifacts_map_.end())
	{
	  artifact_sptr_set_type set;
	  set.insert(impacted_iface);
	  priv_->impacted_artifacts_map_[dif] = set;
	}
      else
	i->second.insert(impacted_iface);
    }

  return true;
}

/// Lookup the interfaces that are impacted by a given leaf diff node.
///
/// @param d the diff node to consider.
///
/// @return the set of artifacts impacted by @p d.
artifact_sptr_set_type*
diff_maps::lookup_impacted_interfaces(const diff *d) const
{
  diff_artifact_set_map_type::iterator i =
    priv_->impacted_artifacts_map_.find(d);

  if (i == priv_->impacted_artifacts_map_.end())
    return 0;

  return &i->second;
}

//
// </diff_maps stuff>

/// Constructor for the @ref diff_stat type.
///
/// @param ctxt the context of the corpus diff.  Note that this
/// context object must stay alive at least during the life time of
/// the current instance of @ref corpus_diff::diff_stats.  Otherwise
/// memory corruption issues occur.
corpus_diff::diff_stats::diff_stats(diff_context_sptr ctxt)
  : priv_(new priv(ctxt))
{}

/// Getter for the number of functions removed.
///
/// @return the number of functions removed.
size_t
corpus_diff::diff_stats::num_func_removed() const
{return priv_->num_func_removed;}

/// Setter for the number of functions removed.
///
/// @param n the new number of functions removed.
void
corpus_diff::diff_stats::num_func_removed(size_t n)
{priv_->num_func_removed = n;}

/// Getter for the number of removed functions that have been filtered
/// out.
///
/// @return the number of removed functions that have been filtered
/// out.
size_t
corpus_diff::diff_stats::num_removed_func_filtered_out() const
{
  if (priv_->ctxt() && !priv_->ctxt()->show_deleted_fns())
    return num_func_removed();
  return priv_->num_removed_func_filtered_out;
}

/// Setter for the number of removed functions that have been filtered
/// out.
///
/// @param t the new value.
void
corpus_diff::diff_stats::num_removed_func_filtered_out(size_t t)
{priv_->num_removed_func_filtered_out = t;}

/// Getter for the net number of function removed.
///
/// This is the difference between the number of functions removed and
/// the number of functons removed that have been filtered out.
///
/// @return the net number of function removed.
size_t
corpus_diff::diff_stats::net_num_func_removed() const
{
  ABG_ASSERT(num_func_removed() >= num_removed_func_filtered_out());
  return num_func_removed() - num_removed_func_filtered_out();
}

/// Getter for the number of functions added.
///
/// @return the number of functions added.
size_t
corpus_diff::diff_stats::num_func_added() const
{return priv_->num_func_added;}

/// Setter for the number of functions added.
///
/// @param n the new number of functions added.
void
corpus_diff::diff_stats::num_func_added(size_t n)
{priv_->num_func_added = n;}

/// Getter for the number of added function that have been filtered out.
///
/// @return the number of added function that have been filtered out.
size_t
corpus_diff::diff_stats::num_added_func_filtered_out() const
{
  if (priv_->ctxt() && !priv_->ctxt()->show_added_fns())
    return num_func_added();
  return priv_->num_added_func_filtered_out;
}

/// Setter for the number of added function that have been filtered
/// out.
///
/// @param n the new value.
void
corpus_diff::diff_stats::num_added_func_filtered_out(size_t n)
{priv_->num_added_func_filtered_out = n;}

/// Getter for the net number of added functions.
///
/// The net number of added functions is the difference between the
/// number of added functions and the number of added functions that
/// have been filtered out.
///
/// @return the net number of added functions.
size_t
corpus_diff::diff_stats::net_num_func_added() const
{
  ABG_ASSERT(num_func_added() >= num_added_func_filtered_out());
  return num_func_added() - num_added_func_filtered_out();
}

/// Getter for the number of functions that have a change in one of
/// their sub-types.
///
/// @return the number of functions that have a change in one of their
/// sub-types.
size_t
corpus_diff::diff_stats::num_func_changed() const
{return priv_->num_func_changed;}

/// Setter for the number of functions that have a change in one of
/// their sub-types.
///
/// @@param n the new number of functions that have a change in one of
/// their sub-types.
void
corpus_diff::diff_stats::num_func_changed(size_t n)
{priv_->num_func_changed = n;}

/// Getter for the number of functions that have a change in one of
/// their sub-types, and that have been filtered out.
///
/// @return the number of functions that have a change in one of their
/// sub-types, and that have been filtered out.
size_t
corpus_diff::diff_stats::num_changed_func_filtered_out() const
{return priv_->num_changed_func_filtered_out;}

/// Setter for the number of functions that have a change in one of
/// their sub-types, and that have been filtered out.
///
/// @param n the new number of functions that have a change in one of their
/// sub-types, and that have been filtered out.
void
corpus_diff::diff_stats::num_changed_func_filtered_out(size_t n)
{priv_->num_changed_func_filtered_out = n;}

/// Getter for the number of functions that carry virtual member
/// offset changes.
///
/// @return the number of functions that carry virtual member changes.
size_t
corpus_diff::diff_stats::num_func_with_virtual_offset_changes() const
{return priv_->num_func_with_virt_offset_changes;}

/// Setter for the number of functions that carry virtual member
/// offset changes.
///
/// @param n the new number of functions that carry virtual member
/// offset.  changes.
void
corpus_diff::diff_stats::num_func_with_virtual_offset_changes(size_t n)
{priv_->num_func_with_virt_offset_changes = n;}

/// Getter for the number of functions that have a change in their
/// sub-types, minus the number of these functions that got filtered
/// out from the diff.
///
/// @return for the the number of functions that have a change in
/// their sub-types, minus the number of these functions that got
/// filtered out from the diff.
size_t
corpus_diff::diff_stats::net_num_func_changed() const
{return num_func_changed() - num_changed_func_filtered_out();}

/// Getter for the number of variables removed.
///
/// @return the number of variables removed.
size_t
corpus_diff::diff_stats::num_vars_removed() const
{return priv_->num_vars_removed;}

/// Setter for the number of variables removed.
///
/// @param n the new number of variables removed.
void
corpus_diff::diff_stats::num_vars_removed(size_t n)
{priv_->num_vars_removed = n;}

/// Getter for the number removed variables that have been filtered
/// out.
///
/// @return the number removed variables that have been filtered out.
size_t
corpus_diff::diff_stats::num_removed_vars_filtered_out() const
{
  if (priv_->ctxt() && !priv_->ctxt()->show_deleted_vars())
    return num_vars_removed();
  return priv_->num_removed_vars_filtered_out;
}

/// Setter for the number of removed variables that have been filtered
/// out.
///
/// @param n the new value.
void
corpus_diff::diff_stats::num_removed_vars_filtered_out(size_t n) const
{priv_->num_removed_vars_filtered_out = n;}

/// Getter for the net number of removed variables.
///
/// The net number of removed variables is the difference between the
/// number of removed variables and the number of removed variables
/// that have been filtered out.
///
/// @return the net number of removed variables.
size_t
corpus_diff::diff_stats::net_num_vars_removed() const
{
  ABG_ASSERT(num_vars_removed() >= num_removed_vars_filtered_out());
  return num_vars_removed() - num_removed_vars_filtered_out();
}

/// Getter for the number of variables added.
///
/// @return the number of variables added.
size_t
corpus_diff::diff_stats::num_vars_added() const
{return priv_->num_vars_added;}

/// Setter for the number of variables added.
///
/// @param n the new number of variables added.
void
corpus_diff::diff_stats::num_vars_added(size_t n)
{priv_->num_vars_added = n;}

/// Getter for the number of added variables that have been filtered
/// out.
///
/// @return the number of added variables that have been filtered out.
size_t
corpus_diff::diff_stats::num_added_vars_filtered_out() const
{
  if (priv_->ctxt() && !priv_->ctxt()->show_added_vars())
    return num_vars_added();
  return priv_->num_added_vars_filtered_out;
}

/// Setter for the number of added variables that have been filtered
/// out.
///
/// @param n the new value.
void
corpus_diff::diff_stats::num_added_vars_filtered_out(size_t n)
{priv_->num_added_vars_filtered_out = n;}

/// Getter for the net number of added variables.
///
/// The net number of added variables is the difference between the
/// number of added variables and the number of added variables that
/// have been filetered out.
///
/// @return the net number of added variables.
size_t
corpus_diff::diff_stats::net_num_vars_added() const
{
  ABG_ASSERT(num_vars_added() >= num_added_vars_filtered_out());
  return num_vars_added() - num_added_vars_filtered_out();
}

/// Getter for the number of variables that have a change in one of
/// their sub-types.
///
/// @return the number of variables that have a change in one of their
/// sub-types.
size_t
corpus_diff::diff_stats::num_vars_changed() const
{return priv_->num_vars_changed;}

/// Setter for the number of variables that have a change in one of
/// their sub-types.
///
/// @param n the new number of variables that have a change in one of
/// their sub-types.
void
corpus_diff::diff_stats::num_vars_changed(size_t n)
{priv_->num_vars_changed = n;}

/// Getter for the number of variables that have a change in one of
/// their sub-types, and that have been filtered out.
///
/// @return the number of functions that have a change in one of their
/// sub-types, and that have been filtered out.
size_t
corpus_diff::diff_stats::num_changed_vars_filtered_out() const
{return priv_->num_changed_vars_filtered_out;}

/// Setter for the number of variables that have a change in one of
/// their sub-types, and that have been filtered out.
///
/// @param n the new number of variables that have a change in one of their
/// sub-types, and that have been filtered out.
void
corpus_diff::diff_stats::num_changed_vars_filtered_out(size_t n)
{priv_->num_changed_vars_filtered_out = n;}

/// Getter for the number of variables that have a change in their
/// sub-types, minus the number of these variables that got filtered
/// out from the diff.
///
/// @return for the the number of variables that have a change in
/// their sub-types, minus the number of these variables that got
/// filtered out from the diff.
size_t
corpus_diff::diff_stats::net_num_vars_changed() const
{return num_vars_changed() - num_changed_vars_filtered_out();}

/// Getter for the number of function symbols (not referenced by any
/// debug info) that got removed.
///
/// @return the number of function symbols (not referenced by any
/// debug info) that got removed.
size_t
corpus_diff::diff_stats::num_func_syms_removed() const
{return priv_->num_func_syms_removed;}

/// Setter for the number of function symbols (not referenced by any
/// debug info) that got removed.
///
/// @param n the number of function symbols (not referenced by any
/// debug info) that got removed.
void
corpus_diff::diff_stats::num_func_syms_removed(size_t n)
{priv_->num_func_syms_removed = n;}

/// Getter for the number of removed function symbols, not referenced
/// by debug info, that have been filtered out.
///
/// @return the number of removed function symbols, not referenced by
/// debug info, that have been filtered out.
size_t
corpus_diff::diff_stats::num_removed_func_syms_filtered_out() const
{
  if (priv_->ctxt()
      && !priv_->ctxt()->show_symbols_unreferenced_by_debug_info())
    return num_func_syms_removed();
  return priv_->num_removed_func_syms_filtered_out;
}

/// Setter for the number of removed function symbols, not referenced
/// by debug info, that have been filtered out.
///
/// @param n the new the number of removed function symbols, not
/// referenced by debug info, that have been filtered out.
void
corpus_diff::diff_stats::num_removed_func_syms_filtered_out(size_t n)
{priv_->num_removed_func_syms_filtered_out = n;}

/// Getter of the net number of removed function symbols that are not
/// referenced by any debug info.
///
/// This is the difference between the total number of removed
/// function symbols and the number of removed function symbols that
/// have been filteted out.  Both numbers are for symbols not
/// referenced by debug info.
///
/// return the net number of removed function symbols that are not
/// referenced by any debug info.
size_t
corpus_diff::diff_stats::net_num_removed_func_syms() const
{
  ABG_ASSERT(num_func_syms_removed() >= num_removed_func_syms_filtered_out());
  return num_func_syms_removed() - num_removed_func_syms_filtered_out();
}

/// Getter for the number of function symbols (not referenced by any
/// debug info) that got added.
///
/// @return the number of function symbols (not referenced by any
/// debug info) that got added.
size_t
corpus_diff::diff_stats::num_func_syms_added() const
{return priv_->num_func_syms_added;}

/// Setter for the number of function symbols (not referenced by any
/// debug info) that got added.
///
/// @param n the new number of function symbols (not referenced by any
/// debug info) that got added.
void
corpus_diff::diff_stats::num_func_syms_added(size_t n)
{priv_->num_func_syms_added = n;}

/// Getter for the number of added function symbols, not referenced by
/// any debug info, that have been filtered out.
///
/// @return the number of added function symbols, not referenced by
/// any debug info, that have been filtered out.
size_t
corpus_diff::diff_stats::num_added_func_syms_filtered_out() const
{
  if (priv_->ctxt()
      && !(priv_->ctxt()->show_added_symbols_unreferenced_by_debug_info()
	   && priv_->ctxt()->show_symbols_unreferenced_by_debug_info()))
    return num_func_syms_added();
  return priv_->num_added_func_syms_filtered_out;
}

/// Setter for the number of added function symbols, not referenced by
/// any debug info, that have been filtered out.
///
/// @param n the new number of added function symbols, not referenced
/// by any debug info, that have been filtered out.
void
corpus_diff::diff_stats::num_added_func_syms_filtered_out(size_t n)
{priv_->num_added_func_syms_filtered_out = n;}

/// Getter of the net number of added function symbols that are not
/// referenced by any debug info.
///
/// This is the difference between the total number of added
/// function symbols and the number of added function symbols that
/// have been filteted out.  Both numbers are for symbols not
/// referenced by debug info.
///
/// return the net number of added function symbols that are not
/// referenced by any debug info.
size_t
corpus_diff::diff_stats::net_num_added_func_syms() const
{
  ABG_ASSERT(num_func_syms_added() >= num_added_func_syms_filtered_out());
  return num_func_syms_added()- num_added_func_syms_filtered_out();
}

/// Getter for the number of variable symbols (not referenced by any
/// debug info) that got removed.
///
/// @return the number of variable symbols (not referenced by any
/// debug info) that got removed.
size_t
corpus_diff::diff_stats::num_var_syms_removed() const
{return priv_->num_var_syms_removed;}

/// Setter for the number of variable symbols (not referenced by any
/// debug info) that got removed.
///
/// @param n the number of variable symbols (not referenced by any
/// debug info) that got removed.
void
corpus_diff::diff_stats::num_var_syms_removed(size_t n)
{priv_->num_var_syms_removed = n;}

/// Getter for the number of removed variable symbols, not referenced
/// by any debug info, that have been filtered out.
///
/// @return the number of removed variable symbols, not referenced
/// by any debug info, that have been filtered out.
size_t
corpus_diff::diff_stats::num_removed_var_syms_filtered_out() const
{
  if (priv_->ctxt()
      && !priv_->ctxt()->show_symbols_unreferenced_by_debug_info())
    return num_var_syms_removed();
  return priv_->num_removed_var_syms_filtered_out;
}

/// Setter for the number of removed variable symbols, not referenced
/// by any debug info, that have been filtered out.
///
/// @param n the number of removed variable symbols, not referenced by
/// any debug info, that have been filtered out.
void
corpus_diff::diff_stats::num_removed_var_syms_filtered_out(size_t n)
{priv_->num_removed_var_syms_filtered_out = n;}

/// Getter of the net number of removed variable symbols that are not
/// referenced by any debug info.
///
/// This is the difference between the total number of removed
/// variable symbols and the number of removed variable symbols that
/// have been filteted out.  Both numbers are for symbols not
/// referenced by debug info.
///
/// return the net number of removed variable symbols that are not
/// referenced by any debug info.
size_t
corpus_diff::diff_stats::net_num_removed_var_syms() const
{
  ABG_ASSERT(num_var_syms_removed() >= num_removed_var_syms_filtered_out());
  return num_var_syms_removed() - num_removed_var_syms_filtered_out();
}

/// Getter for the number of variable symbols (not referenced by any
/// debug info) that got added.
///
/// @return the number of variable symbols (not referenced by any
/// debug info) that got added.
size_t
corpus_diff::diff_stats::num_var_syms_added() const
{return priv_->num_var_syms_added;}

/// Setter for the number of variable symbols (not referenced by any
/// debug info) that got added.
///
/// @param n the new number of variable symbols (not referenced by any
/// debug info) that got added.
void
corpus_diff::diff_stats::num_var_syms_added(size_t n)
{priv_->num_var_syms_added = n;}

/// Getter for the number of added variable symbols, not referenced by
/// any debug info, that have been filtered out.
///
/// @return the number of added variable symbols, not referenced by
/// any debug info, that have been filtered out.
size_t
corpus_diff::diff_stats::num_added_var_syms_filtered_out() const
{
  if (priv_->ctxt()
      && !(priv_->ctxt()->show_added_symbols_unreferenced_by_debug_info()
	   && priv_->ctxt()->show_symbols_unreferenced_by_debug_info()))
    return num_var_syms_added();
  return priv_->num_added_var_syms_filtered_out;
}

/// Setter for the number of added variable symbols, not referenced by
/// any debug info, that have been filtered out.
///
/// @param n the new number of added variable symbols, not referenced
/// by any debug info, that have been filtered out.
void
corpus_diff::diff_stats::num_added_var_syms_filtered_out(size_t n)
{priv_->num_added_var_syms_filtered_out = n;}

/// Getter of the net number of added variable symbols that are not
/// referenced by any debug info.
///
/// This is the difference between the total number of added
/// variable symbols and the number of added variable symbols that
/// have been filteted out.  Both numbers are for symbols not
/// referenced by debug info.
///
/// return the net number of added variable symbols that are not
/// referenced by any debug info.
size_t
corpus_diff::diff_stats::net_num_added_var_syms() const
{
  ABG_ASSERT(num_var_syms_added() >= num_added_var_syms_filtered_out());
  return num_var_syms_added() - num_added_var_syms_filtered_out();
}

/// Getter of the number of leaf type change diff nodes.
///
/// @return the number of leaf type change diff nodes.
size_t
corpus_diff::diff_stats::num_leaf_changes() const
{return priv_->num_leaf_changes;}

/// Setter of the number of leaf type change diff nodes.
///
/// @param n the new number of leaf type change diff nodes.
void
corpus_diff::diff_stats::num_leaf_changes(size_t n)
{priv_->num_leaf_changes = n;}

/// Getter of the number of leaf type change diff nodes that have been
/// filtered out.
///
/// @return the number of leaf type change diff nodes that have been
size_t
corpus_diff::diff_stats::num_leaf_changes_filtered_out() const
{return priv_->num_leaf_changes_filtered_out;}

/// Setter of the number of leaf type change diff nodes that have been
/// filtered out.
///
/// @param n the new number of leaf type change diff nodes that have
/// been filtered out.
void
corpus_diff::diff_stats::num_leaf_changes_filtered_out(size_t n)
{priv_->num_leaf_changes_filtered_out = n;}

/// Getter of the net number of leaf change diff nodes.
///
/// This is the difference between the total number of leaf change
/// diff nodes, and the number of the leaf change diff nodes that have
/// been filtered out.
///
/// A leaf change is either a type change, a function change or a
/// variable change.
size_t
corpus_diff::diff_stats::net_num_leaf_changes() const
{
  ABG_ASSERT(num_leaf_changes() >= num_leaf_changes_filtered_out());
  return num_leaf_changes() - num_leaf_changes_filtered_out();
}

/// Getter for the number of leaf type change diff nodes.
///
/// @return the number of leaf type changes diff nodes.
size_t
corpus_diff::diff_stats::num_leaf_type_changes() const
{return priv_->num_leaf_type_changes;}

/// Setter for the number of leaf type change diff nodes.
///
/// @param n the new number of leaf type change diff nodes.
void
corpus_diff::diff_stats::num_leaf_type_changes(size_t n)
{priv_->num_leaf_type_changes = n;}

/// Getter for the number of filtered out leaf type change diff nodes.
///
/// @return the number of filtered out leaf type change diff nodes.
size_t
corpus_diff::diff_stats::num_leaf_type_changes_filtered_out() const
{return priv_->num_leaf_type_changes_filtered_out;}

/// Setter for the number of filtered out leaf type change diff nodes.
/// @param n the new number of filtered out leaf type change diff nodes.
void
corpus_diff::diff_stats::num_leaf_type_changes_filtered_out(size_t n)
{priv_->num_leaf_type_changes_filtered_out = n;}

/// Getter for the net number of leaf type change diff nodes.
///
/// This is the difference between the number of leaf type changes and
/// the number of filtered out leaf type changes.
///
/// @return the net number of leaf type change diff nodes.
size_t
corpus_diff::diff_stats::net_num_leaf_type_changes() const
{return num_leaf_type_changes() - num_leaf_type_changes_filtered_out();}

/// Getter for the number of leaf function change diff nodes.
///
/// @return the number of leaf function change diff nodes.
size_t
corpus_diff::diff_stats::num_leaf_func_changes() const
{return priv_->num_leaf_func_changes;}

/// Setter for the number of leaf function change diff nodes.
///
/// @param n the new number of leaf function change diff nodes.
void
corpus_diff::diff_stats::num_leaf_func_changes(size_t n)
{priv_->num_leaf_func_changes = n;}

/// Getter for the number of leaf function change diff nodes that were
/// filtered out.
///
/// @return the number of leaf function change diff nodes that were
/// filtered out.
size_t
corpus_diff::diff_stats::num_leaf_func_changes_filtered_out() const
{return priv_->num_leaf_func_changes_filtered_out;}

/// Setter for the number of leaf function change diff nodes that were
/// filtered out.
///
/// @param n the new number of leaf function change diff nodes that
/// were filtered out.
void
corpus_diff::diff_stats::num_leaf_func_changes_filtered_out(size_t n)
{priv_->num_leaf_func_changes_filtered_out = n;}

/// Getter for the net number of leaf function change diff nodes.
///
/// This is the difference between the number of leaf function change
/// diff nodes and the number of filtered out leaf function change
/// diff nodes.
///
/// @return the net number of leaf function change diff nodes.
size_t
corpus_diff::diff_stats::net_num_leaf_func_changes() const
{return num_leaf_func_changes() - num_leaf_func_changes_filtered_out();}

/// Getter for the number of leaf variable change diff nodes.
///
/// @return the number of leaf variable change diff nodes.
size_t
corpus_diff::diff_stats::num_leaf_var_changes() const
{return priv_->num_leaf_var_changes;}

/// Setter for the number of leaf variable change diff nodes.
///
/// @param n the number of leaf variable change diff nodes.
void
corpus_diff::diff_stats::num_leaf_var_changes(size_t n)
{priv_->num_leaf_var_changes = n;}

/// Getter of the number of added types that are unreachable from the
/// public interface of the ABI corpus.
///
/// Public interface means the set of defined and publicly exported
/// functions and variables of the ABI corpus.
///
/// @return the number of added types that are unreachable from the
/// public interface of the ABI corpus.
size_t
corpus_diff::diff_stats::num_added_unreachable_types() const
{return priv_->num_added_unreachable_types;}

/// Setter of the number of added types that are unreachable from the
/// public interface (global functions or variables) of the ABI
/// corpus.
///
/// Public interface means the set of defined and publicly exported
/// functions and variables of the ABI corpus.
///
/// @param n the new number of added types that are unreachable from
/// the public interface of the ABI corpus.
void
corpus_diff::diff_stats::num_added_unreachable_types(size_t n)
{priv_->num_added_unreachable_types = n;}

/// Getter of the number of added types that are unreachable from
/// public interfaces and that are filtered out by suppression
/// specifications.
///
/// @return the number of added types that are unreachable from public
/// interfaces and that are filtered out by suppression
/// specifications.
size_t
corpus_diff::diff_stats::num_added_unreachable_types_filtered_out() const
{return priv_->num_added_unreachable_types_filtered_out;}

/// Setter of the number of added types that are unreachable from
/// public interfaces and that are filtered out by suppression
/// specifications.
///
/// @param n the new number of added types that are unreachable from
/// public interfaces and that are filtered out by suppression
/// specifications.
void
corpus_diff::diff_stats::num_added_unreachable_types_filtered_out(size_t n)
{priv_->num_added_unreachable_types_filtered_out = n;}

/// Getter of the number of added types that are unreachable from
/// public interfaces and that are *NOT* filtered out by suppression
/// specifications.
///
/// @return the number of added types that are unreachable from public
/// interfaces and that are *NOT* filtered out by suppression
/// specifications.
size_t
corpus_diff::diff_stats::net_num_added_unreachable_types() const
{
  ABG_ASSERT(num_added_unreachable_types()
	     >=
	     num_added_unreachable_types_filtered_out());

  return (num_added_unreachable_types()
	  -
	  num_added_unreachable_types_filtered_out());
}

/// Getter of the number of removed types that are unreachable from
/// the public interface of the ABI corpus.
///
/// Public interface means the set of defined and publicly exported
/// functions and variables of the ABI corpus.
///
/// @return the number of removed types that are unreachable from
/// the public interface of the ABI corpus.
size_t
corpus_diff::diff_stats::num_removed_unreachable_types() const
{return priv_->num_removed_unreachable_types;}

/// Setter of the number of removed types that are unreachable from
/// the public interface of the ABI corpus.
///
/// Public interface means the set of defined and publicly exported
/// functions and variables of the ABI corpus.
///
///@param n the new number of removed types that are unreachable from
/// the public interface of the ABI corpus.
void
corpus_diff::diff_stats::num_removed_unreachable_types(size_t n)
{priv_->num_removed_unreachable_types = n;}

/// Getter of the number of removed types that are not reachable from
/// public interfaces and that have been filtered out by suppression
/// specifications.
///
/// @return the number of removed types that are not reachable from
/// public interfaces and that have been filtered out by suppression
/// specifications.
size_t
corpus_diff::diff_stats::num_removed_unreachable_types_filtered_out() const
{return priv_->num_removed_unreachable_types_filtered_out;}

/// Setter of the number of removed types that are not reachable from
/// public interfaces and that have been filtered out by suppression
/// specifications.
///
/// @param n the new number of removed types that are not reachable
/// from public interfaces and that have been filtered out by
/// suppression specifications.
void
corpus_diff::diff_stats::num_removed_unreachable_types_filtered_out(size_t n)
{priv_->num_removed_unreachable_types_filtered_out = n;}

/// Getter of the number of removed types that are not reachable from
/// public interfaces and that have *NOT* been filtered out by
/// suppression specifications.
///
/// @return the number of removed types that are not reachable from
/// public interfaces and that have *NOT* been filtered out by
/// suppression specifications.
size_t
corpus_diff::diff_stats::net_num_removed_unreachable_types() const
{
  ABG_ASSERT(num_removed_unreachable_types()
	     >=
	     num_removed_unreachable_types_filtered_out());

  return (num_removed_unreachable_types()
	  -
	  num_removed_unreachable_types_filtered_out());
}

/// Getter of the number of changed types that are unreachable from
/// the public interface of the ABI corpus.
///
/// Public interface means the set of defined and publicly exported
/// functions and variables of the ABI corpus.
///
/// @return the number of changed types that are unreachable from the
/// public interface of the ABI corpus.
size_t
corpus_diff::diff_stats::num_changed_unreachable_types() const
{return priv_->num_changed_unreachable_types;}

/// Setter of the number of changed types that are unreachable from
/// the public interface of the ABI corpus.
///
/// Public interface means the set of defined and publicly exported
/// functions and variables of the ABI corpus.
///
///@param n the new number of changed types that are unreachable from
/// the public interface of the ABI corpus.
void
corpus_diff::diff_stats::num_changed_unreachable_types(size_t n)
{priv_->num_changed_unreachable_types = n;}

/// Getter of the number of changed types that are unreachable from
/// public interfaces and that have been filtered out by suppression
/// specifications.
///
/// @return the number of changed types that are unreachable from
/// public interfaces and that have been filtered out by suppression
/// specifications.
size_t
corpus_diff::diff_stats::num_changed_unreachable_types_filtered_out() const
{return priv_->num_changed_unreachable_types_filtered_out;}

/// Setter of the number of changed types that are unreachable from
/// public interfaces and that have been filtered out by suppression
/// specifications.
///
/// @param n the new number of changed types that are unreachable from
/// public interfaces and that have been filtered out by suppression
/// specifications.
void
corpus_diff::diff_stats::num_changed_unreachable_types_filtered_out(size_t n)
{priv_->num_changed_unreachable_types_filtered_out = n;}

/// Getter of the number of changed types that are unreachable from
/// public interfaces and that have *NOT* been filtered out by
/// suppression specifications.
///
/// @return the number of changed types that are unreachable from
/// public interfaces and that have *NOT* been filtered out by
/// suppression specifications.
size_t
corpus_diff::diff_stats::net_num_changed_unreachable_types() const
{
  ABG_ASSERT(num_changed_unreachable_types()
	     >=
	     num_changed_unreachable_types_filtered_out());

  return (num_changed_unreachable_types()
	  -
	  num_changed_unreachable_types_filtered_out());
}

/// Getter for the number of leaf variable changes diff nodes that
/// have been filtered out.
///
/// @return the number of leaf variable changes diff nodes that have
/// been filtered out.
size_t
corpus_diff::diff_stats::num_leaf_var_changes_filtered_out() const
{return priv_->num_leaf_var_changes_filtered_out;}

/// Setter for the number of leaf variable changes diff nodes that
/// have been filtered out.
///
/// @param n the number of leaf variable changes diff nodes that have
/// been filtered out.
void
corpus_diff::diff_stats::num_leaf_var_changes_filtered_out(size_t n)
{priv_->num_leaf_var_changes_filtered_out = n;}

/// Getter for the net number of leaf variable change diff nodes.
///
/// This the difference between the number of leaf variable change
/// diff nodes and the number of filtered out leaf variable change
/// diff nodes.
///
/// @return the net number of leaf variable change diff nodes.
size_t
corpus_diff::diff_stats::net_num_leaf_var_changes() const
{return num_leaf_var_changes() - num_leaf_var_changes_filtered_out();}


// <corpus_diff stuff>

/// Getter of the context associated with this corpus.
///
/// @return a smart pointer to the context associate with the corpus.
diff_context_sptr
corpus_diff::priv::get_context()
{return ctxt_.lock();}

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
corpus_diff::priv::lookup_tables_empty() const
{
  return (deleted_fns_.empty()
	  && added_fns_.empty()
	  && changed_fns_map_.empty()
	  && deleted_vars_.empty()
	  && added_vars_.empty()
	  && changed_vars_map_.empty());
}

/// Clear the lookup tables useful for reporting an enum_diff.
void
corpus_diff::priv::clear_lookup_tables()
{
  deleted_fns_.clear();
  added_fns_.clear();
  changed_fns_map_.clear();
  deleted_vars_.clear();
  added_vars_.clear();
  changed_vars_map_.clear();
}

/// If the lookup tables are not yet built, walk the differences and
/// fill the lookup tables.
void
corpus_diff::priv::ensure_lookup_tables_populated()
{
  if (!lookup_tables_empty())
    return;

  diff_context_sptr ctxt = get_context();

  {
    edit_script& e = fns_edit_script_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	ABG_ASSERT(i < first_->get_functions().size());

	function_decl* deleted_fn = first_->get_functions()[i];
	string n = deleted_fn->get_id();
	ABG_ASSERT(!n.empty());
	// The below is commented out because there can be several
	// functions with the same ID in the corpus.  So several
	// functions with the same ID can be deleted.
	// ABG_ASSERT(deleted_fns_.find(n) == deleted_fns_.end());
	deleted_fns_[n] = deleted_fn;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    function_decl* added_fn = second_->get_functions()[i];
	    string n = added_fn->get_id();
	    ABG_ASSERT(!n.empty());
	    // The below is commented out because there can be several
	    // functions with the same ID in the corpus.  So several
	    // functions with the same ID can be added.
	    // ABG_ASSERT(added_fns_.find(n) == added_fns_.end());
	    string_function_ptr_map::const_iterator j =
	      deleted_fns_.find(n);
	    if (j != deleted_fns_.end())
	      {
		function_decl_sptr f(j->second, noop_deleter());
		function_decl_sptr s(added_fn, noop_deleter());
		function_decl_diff_sptr d = compute_diff(f, s, ctxt);
		if (*j->second != *added_fn)
		  changed_fns_map_[j->first] = d;
		deleted_fns_.erase(j);
	      }
	    else
	      added_fns_[n] = added_fn;
	  }
      }
    sort_string_function_decl_diff_sptr_map(changed_fns_map_, changed_fns_);

    // Now walk the allegedly deleted functions; check if their
    // underlying symbols are deleted as well; otherwise, consider
    // that the function in question hasn't been deleted.

    vector<string> to_delete;
    for (string_function_ptr_map::const_iterator i = deleted_fns_.begin();
	 i != deleted_fns_.end();
	 ++i)
      if (second_->lookup_function_symbol(*i->second->get_symbol()))
	to_delete.push_back(i->first);

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      deleted_fns_.erase(*i);

    // Do something similar for added functions.

    to_delete.clear();
    for (string_function_ptr_map::const_iterator i = added_fns_.begin();
	 i != added_fns_.end();
	 ++i)
      {
	if (first_->lookup_function_symbol(*i->second->get_symbol()))
	  to_delete.push_back(i->first);
	else if (! i->second->get_symbol()->get_version().is_empty()
		 && i->second->get_symbol()->get_version().is_default())
	  // We are looking for a symbol that has a default version,
	  // and which seems to be newly added.  Let's see if the same
	  // symbol with *no* version was already present in the
	  // former corpus.  If yes, then the symbol shouldn't be
	  // considered as 'added'.
	  {
	    elf_symbol::version empty_version;
	    if (first_->lookup_function_symbol(i->second->get_symbol()->get_name(),
					       empty_version))
	      to_delete.push_back(i->first);
	  }
      }

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      added_fns_.erase(*i);
  }

  {
    edit_script& e = vars_edit_script_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	ABG_ASSERT(i < first_->get_variables().size());

	var_decl* deleted_var = first_->get_variables()[i];
	string n = deleted_var->get_id();
	ABG_ASSERT(!n.empty());
	ABG_ASSERT(deleted_vars_.find(n) == deleted_vars_.end());
	deleted_vars_[n] = deleted_var;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    var_decl* added_var = second_->get_variables()[i];
	    string n = added_var->get_id();
	    ABG_ASSERT(!n.empty());
	    {
	      string_var_ptr_map::const_iterator k = added_vars_.find(n);
	      if ( k != added_vars_.end())
		{
		  ABG_ASSERT(is_member_decl(k->second)
			 && get_member_is_static(k->second));
		  continue;
		}
	    }
	    string_var_ptr_map::const_iterator j =
	      deleted_vars_.find(n);
	    if (j != deleted_vars_.end())
	      {
		if (*j->second != *added_var)
		  {
		    var_decl_sptr f(j->second, noop_deleter());
		    var_decl_sptr s(added_var, noop_deleter());
		    changed_vars_map_[n] = compute_diff(f, s, ctxt);
		  }
		deleted_vars_.erase(j);
	      }
	    else
	      added_vars_[n] = added_var;
	  }
      }
    sort_string_var_diff_sptr_map(changed_vars_map_,
				  sorted_changed_vars_);

    // Now walk the allegedly deleted variables; check if their
    // underlying symbols are deleted as well; otherwise consider
    // that the variable in question hasn't been deleted.

    vector<string> to_delete;
    for (string_var_ptr_map::const_iterator i = deleted_vars_.begin();
	 i != deleted_vars_.end();
	 ++i)
      if (second_->lookup_variable_symbol(*i->second->get_symbol()))
	to_delete.push_back(i->first);

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      deleted_vars_.erase(*i);

    // Do something similar for added variables.

    to_delete.clear();
    for (string_var_ptr_map::const_iterator i = added_vars_.begin();
	 i != added_vars_.end();
	 ++i)
      if (first_->lookup_variable_symbol(*i->second->get_symbol()))
	to_delete.push_back(i->first);
      else if (! i->second->get_symbol()->get_version().is_empty()
		 && i->second->get_symbol()->get_version().is_default())
	// We are looking for a symbol that has a default version,
	// and which seems to be newly added.  Let's see if the same
	// symbol with *no* version was already present in the
	// former corpus.  If yes, then the symbol shouldn't be
	// considered as 'added'.
	{
	  elf_symbol::version empty_version;
	  if (first_->lookup_variable_symbol(i->second->get_symbol()->get_name(),
					     empty_version))
	    to_delete.push_back(i->first);
	}

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      added_vars_.erase(*i);
  }

  // Massage the edit script for added/removed function symbols that
  // were not referenced by any debug info and turn them into maps of
  // {symbol_name, symbol}.
  {
    edit_script& e = unrefed_fn_syms_edit_script_;
    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	ABG_ASSERT(i < first_->get_unreferenced_function_symbols().size());
	elf_symbol_sptr deleted_sym =
	  first_->get_unreferenced_function_symbols()[i];
	if (!second_->lookup_function_symbol(*deleted_sym))
	  deleted_unrefed_fn_syms_[deleted_sym->get_id_string()] = deleted_sym;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    ABG_ASSERT(i < second_->get_unreferenced_function_symbols().size());
	    elf_symbol_sptr added_sym =
	      second_->get_unreferenced_function_symbols()[i];
	    if ((deleted_unrefed_fn_syms_.find(added_sym->get_id_string())
		 == deleted_unrefed_fn_syms_.end()))
	      {
		if (!first_->lookup_function_symbol(*added_sym))
		  {
		    bool do_add = true;
		    if (! added_sym->get_version().is_empty()
			&& added_sym->get_version().is_default())
		      {
			// So added_seem has a default version.  If
			// the former corpus had a symbol with the
			// same name as added_sym but with *no*
			// version, then added_sym shouldn't be
			// considered as a newly added symbol.
			elf_symbol::version empty_version;
			if (first_->lookup_function_symbol(added_sym->get_name(),
							   empty_version))
			  do_add = false;
		      }

		    if (do_add)
		      added_unrefed_fn_syms_[added_sym->get_id_string()] =
			added_sym;
		  }
	      }
	    else
	      deleted_unrefed_fn_syms_.erase(added_sym->get_id_string());
	  }
      }
  }

  // Massage the edit script for added/removed variable symbols that
  // were not referenced by any debug info and turn them into maps of
  // {symbol_name, symbol}.
  {
    edit_script& e = unrefed_var_syms_edit_script_;
    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	ABG_ASSERT(i < first_->get_unreferenced_variable_symbols().size());
	elf_symbol_sptr deleted_sym =
	  first_->get_unreferenced_variable_symbols()[i];
	if (!second_->lookup_variable_symbol(*deleted_sym))
	  deleted_unrefed_var_syms_[deleted_sym->get_id_string()] = deleted_sym;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    ABG_ASSERT(i < second_->get_unreferenced_variable_symbols().size());
	    elf_symbol_sptr added_sym =
	      second_->get_unreferenced_variable_symbols()[i];
	    if (deleted_unrefed_var_syms_.find(added_sym->get_id_string())
		== deleted_unrefed_var_syms_.end())
	      {
		if (!first_->lookup_variable_symbol(*added_sym))
		  {
		    bool do_add = true;
		    if (! added_sym->get_version().is_empty()
			&& added_sym->get_version().is_default())
		      {
			// So added_seem has a default version.  If
			// the former corpus had a symbol with the
			// same name as added_sym but with *no*
			// version, then added_sym shouldn't be
			// considered as a newly added symbol.
			elf_symbol::version empty_version;
			if (first_->lookup_variable_symbol(added_sym->get_name(),
							   empty_version))
			  do_add = false;
		      }

		    if (do_add)
		      added_unrefed_var_syms_[added_sym->get_id_string()] =
			added_sym;
		  }
	      }
	    else
	      deleted_unrefed_var_syms_.erase(added_sym->get_id_string());
	  }
      }
  }

  // Handle the unreachable_types_edit_script_
  {
    edit_script& e = unreachable_types_edit_script_;

    // Populate the map of deleted unreachable types from the
    // deletions of the edit script.
    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	type_base_sptr t
	  (first_->get_types_not_reachable_from_public_interfaces()[i]);

	if (!is_user_defined_type(t))
	  continue;

	string repr = abigail::ir::get_pretty_representation(t, true);
	deleted_unreachable_types_[repr] = t;
      }

    // Populate the map of added and change unreachable types from the
    // insertions of the edit script.
    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    type_base_sptr t
	      (second_->get_types_not_reachable_from_public_interfaces()[i]);

	    if (!is_user_defined_type(t))
	      continue;

	    string repr = abigail::ir::get_pretty_representation(t, true);

	    // Let's see if the inserted type we are looking at was
	    // reported as deleted as well.
	    //
	    // If it's been deleted and a different version of it has
	    // now been added, it means it's been *changed*.  In that
	    // case we'll compute the diff of that change and store it
	    // in the map of changed unreachable types.
	    //
	    // Otherwise, it means the type's been added so we'll add
	    // it to the set of added unreachable types.

	    string_type_base_sptr_map::const_iterator j =
	      deleted_unreachable_types_.find(repr);
	    if (j != deleted_unreachable_types_.end())
	      {
		// So there was another type of the same pretty
		// representation which was reported as deleted.
		// Let's see if they are different or not ...
		decl_base_sptr old_type = is_decl(j->second);
		decl_base_sptr new_type = is_decl(t);
		if (old_type != new_type)
		  {
		    // The previously added type is different from this
		    // one that is added.  That means the initial type
		    // was changed.  Let's compute its diff and store it
		    // as a changed type.
		    diff_sptr d = compute_diff(old_type, new_type, ctxt);
		    ABG_ASSERT(d->has_changes());
		    changed_unreachable_types_[repr]= d;
		  }

		// In any case, the type was both deleted and added,
		// so we cannot have it marked as being deleted.  So
		// let's remove it from the deleted types.
		deleted_unreachable_types_.erase(j);
	      }
	    else
	      // The type wasn't previously reported as deleted, so
	      // it's really added.
	      added_unreachable_types_[repr] = t;
	  }
      }
  }
}

/// Test if a change reports about a given @ref function_decl that is
/// changed in a certain way is suppressed by a given suppression
/// specifiation
///
/// @param fn the @ref function_decl to consider.
///
/// @param suppr the suppression specification to consider.
///
/// @param k the kind of change that happened to @p fn.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff the suppression specification @p suppr suppresses
/// change reports about function @p fn, if that function changes in
/// the way expressed by @p k.
static bool
function_is_suppressed(const function_decl* fn,
		       const suppression_sptr suppr,
		       function_suppression::change_kind k,
		       const diff_context_sptr ctxt)
{
  function_suppression_sptr fn_suppr = is_function_suppression(suppr);
  if (!fn_suppr)
    return false;
  return fn_suppr->suppresses_function(fn, k, ctxt);
}

/// Test if a change reports about a given @ref var_decl that is
/// changed in a certain way is suppressed by a given suppression
/// specifiation
///
/// @param fn the @ref var_decl to consider.
///
/// @param suppr the suppression specification to consider.
///
/// @param k the kind of change that happened to @p fn.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff the suppression specification @p suppr suppresses
/// change reports about variable @p fn, if that variable changes in
/// the way expressed by @p k.
static bool
variable_is_suppressed(const var_decl* var,
		       const suppression_sptr suppr,
		       variable_suppression::change_kind k,
		       const diff_context_sptr ctxt)
{
  variable_suppression_sptr var_suppr = is_variable_suppression(suppr);
  if (!var_suppr)
    return false;
  return var_suppr->suppresses_variable(var, k, ctxt);
}

/// Apply suppression specifications for this corpus diff to the set
/// of added/removed functions/variables, as well as to types not
/// reachable from global functions/variables.
void
corpus_diff::priv::apply_supprs_to_added_removed_fns_vars_unreachable_types()
{
  diff_context_sptr ctxt = get_context();

  const suppressions_type& suppressions = ctxt->suppressions();
  for (suppressions_type::const_iterator i = suppressions.begin();
       i != suppressions.end();
       ++i)
    {
      // Added/Deleted functions.
      if (function_suppression_sptr fn_suppr = is_function_suppression(*i))
	{
	  // Added functions
	  for (string_function_ptr_map::const_iterator e = added_fns_.begin();
	       e != added_fns_.end();
	       ++e)
	    if (function_is_suppressed(e->second, fn_suppr,
				       function_suppression::ADDED_FUNCTION_CHANGE_KIND,
				       ctxt))
	      suppressed_added_fns_[e->first] = e->second;

	  // Deleted functions.
	  for (string_function_ptr_map::const_iterator e = deleted_fns_.begin();
	       e != deleted_fns_.end();
	       ++e)
	    if (function_is_suppressed(e->second, fn_suppr,
				       function_suppression::DELETED_FUNCTION_CHANGE_KIND,
				       ctxt))
	      suppressed_deleted_fns_[e->first] = e->second;

	  // Added function symbols not referenced by any debug info
	  for (string_elf_symbol_map::const_iterator e =
		 added_unrefed_fn_syms_.begin();
	       e != added_unrefed_fn_syms_.end();
	       ++e)
	    if (fn_suppr->suppresses_function_symbol(e->second,
						     function_suppression::ADDED_FUNCTION_CHANGE_KIND,
						     ctxt))
	      suppressed_added_unrefed_fn_syms_[e->first] = e->second;

	  // Removed function symbols not referenced by any debug info
	  for (string_elf_symbol_map::const_iterator e =
		 deleted_unrefed_fn_syms_.begin();
	       e != deleted_unrefed_fn_syms_.end();
	       ++e)
	    if (fn_suppr->suppresses_function_symbol(e->second,
						     function_suppression::DELETED_FUNCTION_CHANGE_KIND,
						     ctxt))
	      suppressed_deleted_unrefed_fn_syms_[e->first] = e->second;
	}
      // Added/Delete virtual member functions changes that might be
      // suppressed by a type_suppression that matches the enclosing
      // class of the virtual member function.
      else if (type_suppression_sptr type_suppr = is_type_suppression(*i))
	{
	  // Added virtual functions
	  for (string_function_ptr_map::const_iterator e = added_fns_.begin();
	       e != added_fns_.end();
	       ++e)
	    if (is_member_function(e->second)
		&& get_member_function_is_virtual(e->second))
	      {
		function_decl *f = e->second;
		class_decl_sptr c =
		  is_class_type(is_method_type(f->get_type())->get_class_type());
		ABG_ASSERT(c);
		if (type_suppr->suppresses_type(c, ctxt))
		  suppressed_added_fns_[e->first] = e->second;
	      }
	  // Deleted virtual functions
	  for (string_function_ptr_map::const_iterator e = deleted_fns_.begin();
	       e != deleted_fns_.end();
	       ++e)
	    if (is_member_function(e->second)
		&& get_member_function_is_virtual(e->second))
	      {
		function_decl *f = e->second;
		class_decl_sptr c =
		  is_class_type(is_method_type(f->get_type())->get_class_type());
		ABG_ASSERT(c);
		if (type_suppr->suppresses_type(c, ctxt))
		  suppressed_deleted_fns_[e->first] = e->second;
	      }

	  // Apply this type suppression to deleted types
	  // non-reachable from a public interface.
	  for (string_type_base_sptr_map::const_iterator e =
		 deleted_unreachable_types_.begin();
	       e != deleted_unreachable_types_.end();
	       ++e)
	    if (type_suppr->suppresses_type(e->second, ctxt))
	      suppressed_deleted_unreachable_types_[e->first] = e->second;

	  // Apply this type suppression to added types
	  // non-reachable from a public interface.
	  for (string_type_base_sptr_map::const_iterator e =
		 added_unreachable_types_.begin();
	       e != added_unreachable_types_.end();
	       ++e)
	    if (type_suppr->suppresses_type(e->second, ctxt))
	      suppressed_added_unreachable_types_[e->first] = e->second;
	}
      // Added/Deleted variables
      else if (variable_suppression_sptr var_suppr =
	       is_variable_suppression(*i))
	{
	  // Added variables
	  for (string_var_ptr_map::const_iterator e = added_vars_.begin();
	       e != added_vars_.end();
	       ++e)
	    if (variable_is_suppressed(e->second, var_suppr,
				       variable_suppression::ADDED_VARIABLE_CHANGE_KIND,
				       ctxt))
	      suppressed_added_vars_[e->first] = e->second;

	  //Deleted variables
	  for (string_var_ptr_map::const_iterator e = deleted_vars_.begin();
	       e != deleted_vars_.end();
	       ++e)
	    if (variable_is_suppressed(e->second, var_suppr,
				       variable_suppression::DELETED_VARIABLE_CHANGE_KIND,
				       ctxt))
	      suppressed_deleted_vars_[e->first] = e->second;

	  // Added variable symbols not referenced by any debug info
	  for (string_elf_symbol_map::const_iterator e =
		 added_unrefed_var_syms_.begin();
	       e != added_unrefed_var_syms_.end();
	       ++e)
	    if (var_suppr->suppresses_variable_symbol(e->second,
						      variable_suppression::ADDED_VARIABLE_CHANGE_KIND,
						      ctxt))
	      suppressed_added_unrefed_var_syms_[e->first] = e->second;

	  // Removed variable symbols not referenced by any debug info
	  for (string_elf_symbol_map::const_iterator e =
		 deleted_unrefed_var_syms_.begin();
	       e != deleted_unrefed_var_syms_.end();
	       ++e)
	    if (var_suppr->suppresses_variable_symbol(e->second,
						      variable_suppression::DELETED_VARIABLE_CHANGE_KIND,
						      ctxt))
	      suppressed_deleted_unrefed_var_syms_[e->first] = e->second;
	}
    }
}

/// Test if the change reports for a given deleted function have
/// been deleted.
///
/// @param fn the function to consider.
///
/// @return true iff the change reports for a give given deleted
/// function have been deleted.
bool
corpus_diff::priv::deleted_function_is_suppressed(const function_decl* fn) const
{
  if (!fn)
    return false;

  string_function_ptr_map::const_iterator i =
    suppressed_deleted_fns_.find(fn->get_id());

  return (i != suppressed_deleted_fns_.end());
}

/// Test if an added type that is unreachable from public interface
/// has been suppressed by a suppression specification.
///
/// @param t the added unreachable type to be considered.
///
/// @return true iff @p t has been suppressed by a suppression
/// specification.
bool
corpus_diff::priv::added_unreachable_type_is_suppressed(const type_base *t)const
{
  if (!t)
    return false;

  string repr = abigail::ir::get_pretty_representation(t, /*internal=*/true);
  string_type_base_sptr_map::const_iterator i =
    suppressed_added_unreachable_types_.find(repr);
  if (i == suppressed_added_unreachable_types_.end())
    return false;

  return true;
}

/// Test if a deleted type that is unreachable from public interface
/// has been suppressed by a suppression specification.
///
/// @param t the deleted unreachable type to be considered.
///
/// @return true iff @p t has been suppressed by a suppression
/// specification.
bool
corpus_diff::priv::deleted_unreachable_type_is_suppressed(const type_base *t) const
{
  if (!t)
    return false;

  string repr = abigail::ir::get_pretty_representation(t, /*internal=*/true);
  string_type_base_sptr_map::const_iterator i =
    suppressed_deleted_unreachable_types_.find(repr);
  if (i == suppressed_deleted_unreachable_types_.end())
    return false;

  return true;
}

/// Test if the change reports for a give given added function has
/// been deleted.
///
/// @param fn the function to consider.
///
/// @return true iff the change reports for a give given added
/// function has been deleted.
bool
corpus_diff::priv::added_function_is_suppressed(const function_decl* fn) const
{
  if (!fn)
    return false;

  string_function_ptr_map::const_iterator i =
    suppressed_added_fns_.find(fn->get_id());

  return (i != suppressed_added_fns_.end());
}

/// Test if the change reports for a give given deleted variable has
/// been deleted.
///
/// @param var the variable to consider.
///
/// @return true iff the change reports for a give given deleted
/// variable has been deleted.
bool
corpus_diff::priv::deleted_variable_is_suppressed(const var_decl* var) const
{
  if (!var)
    return false;

  string_var_ptr_map::const_iterator i =
    suppressed_deleted_vars_.find(var->get_id());

  return (i != suppressed_deleted_vars_.end());
}

/// Test if the change reports for a given added variable have been
/// suppressed.
///
/// @param var the variable to consider.
///
/// @return true iff the change reports for a given deleted
/// variable has been deleted.
bool
corpus_diff::priv::added_variable_is_suppressed(const var_decl* var) const
{
  if (!var)
    return false;

  string_var_ptr_map::const_iterator i =
    suppressed_added_vars_.find(var->get_id());

  return (i != suppressed_added_vars_.end());
}

/// Test if the change reports for a given deleted function symbol
/// (that is not referenced by any debug info) has been suppressed.
///
/// @param var the function to consider.
///
/// @return true iff the change reports for a given deleted function
/// symbol has been suppressed.
bool
corpus_diff::priv::deleted_unrefed_fn_sym_is_suppressed(const elf_symbol* s) const
{
  if (!s)
    return false;

  string_elf_symbol_map::const_iterator i =
    suppressed_deleted_unrefed_fn_syms_.find(s->get_id_string());

  return (i != suppressed_deleted_unrefed_fn_syms_.end());
}

/// Test if the change reports for a given added function symbol
/// (that is not referenced by any debug info) has been suppressed.
///
/// @param var the function to consider.
///
/// @return true iff the change reports for a given added function
/// symbol has been suppressed.
bool
corpus_diff::priv::added_unrefed_fn_sym_is_suppressed(const elf_symbol* s) const
{
  if (!s)
    return false;

  string_elf_symbol_map::const_iterator i =
    suppressed_added_unrefed_fn_syms_.find(s->get_id_string());

  return (i != suppressed_added_unrefed_fn_syms_.end());
}

/// Test if the change reports for a given deleted variable symbol
/// (that is not referenced by any debug info) has been suppressed.
///
/// @param var the variable to consider.
///
/// @return true iff the change reports for a given deleted variable
/// symbol has been suppressed.
bool
corpus_diff::priv::deleted_unrefed_var_sym_is_suppressed(const elf_symbol* s) const
{
  if (!s)
    return false;

  string_elf_symbol_map::const_iterator i =
    suppressed_deleted_unrefed_var_syms_.find(s->get_id_string());

  return (i != suppressed_deleted_unrefed_var_syms_.end());
}

/// Test if the change reports for a given added variable symbol
/// (that is not referenced by any debug info) has been suppressed.
///
/// @param var the variable to consider.
///
/// @return true iff the change reports for a given added variable
/// symbol has been suppressed.
bool
corpus_diff::priv::added_unrefed_var_sym_is_suppressed(const elf_symbol* s) const
{
  if (!s)
    return false;

  string_elf_symbol_map::const_iterator i =
    suppressed_added_unrefed_var_syms_.find(s->get_id_string());

  return (i != suppressed_added_unrefed_var_syms_.end());
}

#ifdef do_count_diff_map_changes
#undef do_count_diff_map_changes
#endif
#define do_count_diff_map_changes(diff_map, n_changes, n_filtered)	\
  {									\
    string_diff_ptr_map::const_iterator i;				\
    for (i = diff_map.begin();						\
	 i != diff_map.end();						\
	 ++i)								\
      { \
	if (const var_diff* d = is_var_diff(i->second))		\
	  if (is_data_member(d->first_var()))				\
	    continue;							\
									\
	if (i->second->has_local_changes())				\
	  ++n_changes;							\
	if (!i->second->get_canonical_diff()->to_be_reported())		\
	  ++n_filtered;						\
      }								\
  }

/// Count the number of leaf changes as well as the number of the
/// changes that have been filtered out.
///
/// @param num_changes out parameter.  This is set to the total number
/// of leaf changes.
///
/// @param num_filtered out parameter.  This is set to the number of
/// leaf changes that have been filtered out.
void
corpus_diff::priv::count_leaf_changes(size_t &num_changes, size_t &num_filtered)
{
  count_leaf_type_changes(num_changes, num_filtered);

  // Now count the non-type changes.
  do_count_diff_map_changes(leaf_diffs_.get_function_decl_diff_map(),
    num_changes, num_filtered);
  do_count_diff_map_changes(leaf_diffs_.get_var_decl_diff_map(),
    num_changes, num_filtered);
}

/// Count the number of leaf *type* changes as well as the number of
/// the leaf type changes that have been filtered out.
///
/// @param num_changes out parameter.  This is set to the total number
/// of leaf type changes.
///
/// @param num_filtered out parameter.  This is set to the number of
/// leaf type changes that have been filtered out.
void
corpus_diff::priv::count_leaf_type_changes(size_t &num_changes,
					   size_t &num_filtered)
{
  do_count_diff_map_changes(leaf_diffs_.get_type_decl_diff_map(),
    num_changes, num_filtered);
  do_count_diff_map_changes(leaf_diffs_.get_enum_diff_map(),
    num_changes, num_filtered);
  do_count_diff_map_changes(leaf_diffs_.get_class_diff_map(),
    num_changes, num_filtered);
  do_count_diff_map_changes(leaf_diffs_.get_union_diff_map(),
    num_changes, num_filtered);
  do_count_diff_map_changes(leaf_diffs_.get_typedef_diff_map(),
    num_changes, num_filtered);
  do_count_diff_map_changes(leaf_diffs_.get_array_diff_map(),
    num_changes, num_filtered);
  do_count_diff_map_changes(leaf_diffs_.get_distinct_diff_map(),
    num_changes, num_filtered);
  do_count_diff_map_changes(leaf_diffs_.get_fn_parm_diff_map(),
			    num_changes, num_filtered);
}

/// Count the number of types not reachable from the interface (i.e,
/// not reachable from global functions or variables).
///
/// @param num_added this is set to the number of added types not
/// reachable from the interface.
///
/// @param num_deleted this is set to the number of deleted types not
/// reachable from the interface.
///
/// @param num_changed this is set to the number of changed types not
/// reachable from the interface.
///
/// @param num_filtered_added this is set to the number of added types
/// not reachable from the interface and that have been filtered out
/// by suppression specifications.
///
/// @param num_filtered_deleted this is set to the number of deleted
/// types not reachable from the interface and that have been filtered
/// out by suppression specifications.
///
/// @param num_filtered_changed this is set to the number of changed
/// types not reachable from the interface and that have been filtered
/// out by suppression specifications.
void
corpus_diff::priv::count_unreachable_types(size_t &num_added,
					   size_t &num_deleted,
					   size_t &num_changed,
					   size_t &num_filtered_added,
					   size_t &num_filtered_deleted,
					   size_t &num_filtered_changed)
{
  num_added = added_unreachable_types_.size();
  num_deleted = deleted_unreachable_types_.size();
  num_changed = changed_unreachable_types_.size();
  num_filtered_added = suppressed_added_unreachable_types_.size();
  num_filtered_deleted = suppressed_deleted_unreachable_types_.size();

  for (vector<diff_sptr>::const_iterator i =
	 changed_unreachable_types_sorted().begin();
       i != changed_unreachable_types_sorted().end();
       ++i)
    if (!(*i)->to_be_reported())
      ++num_filtered_changed;
}

/// Get the sorted vector of diff nodes representing changed
/// unreachable types.
///
/// Upon the first invocation of this method, if the vector is empty,
/// this function gets the diff nodes representing changed
/// unreachable, sort them, and return the sorted vector.
///
/// @return the sorted vector of diff nodes representing changed
/// unreachable types.
const vector<diff_sptr>&
corpus_diff::priv::changed_unreachable_types_sorted() const
{
if (changed_unreachable_types_sorted_.empty())
  if (!changed_unreachable_types_.empty())
    sort_string_diff_sptr_map(changed_unreachable_types_,
			      changed_unreachable_types_sorted_);

 return changed_unreachable_types_sorted_;
}

/// Compute the diff stats.
///
/// To know the number of functions that got filtered out, this
/// function applies the categorizing filters to the diff sub-trees of
/// each function changes diff, prior to calculating the stats.
///
/// @param num_removed the number of removed functions.
///
/// @param num_added the number of added functions.
///
/// @param num_changed the number of changed functions.
///
/// @param num_filtered_out the number of changed functions that are
/// got filtered out from the report
void
corpus_diff::priv::apply_filters_and_compute_diff_stats(diff_stats& stat)
{
  stat.num_func_removed(deleted_fns_.size());
  stat.num_removed_func_filtered_out(suppressed_deleted_fns_.size());
  stat.num_func_added(added_fns_.size());
  stat.num_added_func_filtered_out(suppressed_added_fns_.size());
  stat.num_func_changed(changed_fns_map_.size());

  stat.num_vars_removed(deleted_vars_.size());
  stat.num_removed_vars_filtered_out(suppressed_deleted_vars_.size());
  stat.num_vars_added(added_vars_.size());
  stat.num_added_vars_filtered_out(suppressed_added_vars_.size());
  stat.num_vars_changed(changed_vars_map_.size());

  diff_context_sptr ctxt = get_context();

  // Walk the changed function diff nodes to apply the categorization
  // filters.
  diff_sptr diff;
  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_fns_.begin();
       i != changed_fns_.end();
       ++i)
    {
      diff_sptr diff = *i;
      ctxt->maybe_apply_filters(diff);
    }

  // Walk the changed variable diff nodes to apply the categorization
  // filters.
  for (var_diff_sptrs_type::const_iterator i = sorted_changed_vars_.begin();
       i != sorted_changed_vars_.end();
       ++i)
    {
      diff_sptr diff = *i;
      ctxt->maybe_apply_filters(diff);
    }

  // walk the changed unreachable types to apply categorization
  // filters
  for (diff_sptrs_type::const_iterator i =
	  changed_unreachable_types_sorted().begin();
	i != changed_unreachable_types_sorted().end();
       ++i)
    {
      diff_sptr diff = *i;
      ctxt->maybe_apply_filters(diff);
    }

  categorize_redundant_changed_sub_nodes();

  // Walk the changed function diff nodes to count the number of
  // filtered-out functions and the number of functions with virtual
  // offset changes.
  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_fns_.begin();
       i != changed_fns_.end();
       ++i)
    {
      if ((*i)->is_filtered_out())
	{
	  stat.num_changed_func_filtered_out
	    (stat.num_changed_func_filtered_out() + 1);

	  if ((*i)->has_local_changes())
	    stat.num_leaf_func_changes_filtered_out
	      (stat.num_leaf_func_changes_filtered_out() + 1);
	}
      else
	{
	  if ((*i)->get_category() & VIRTUAL_MEMBER_CHANGE_CATEGORY)
	    stat.num_func_with_virtual_offset_changes
	      (stat.num_func_with_virtual_offset_changes() + 1);
	}

      if ((*i)->has_local_changes())
	stat.num_leaf_func_changes
	  (stat.num_leaf_func_changes() + 1);
    }

  // Walk the changed variables diff nodes to count the number of
  // filtered-out variables.
  for (var_diff_sptrs_type ::const_iterator i = sorted_changed_vars_.begin();
       i != sorted_changed_vars_.end();
       ++i)
    {
      if ((*i)->is_filtered_out())
	{
	  stat.num_changed_vars_filtered_out
	    (stat.num_changed_vars_filtered_out() + 1);

	  if ((*i)->has_local_changes())
	    stat.num_leaf_var_changes_filtered_out
	      (stat.num_leaf_var_changes_filtered_out() + 1);
	}
      if ((*i)->has_local_changes())
	stat.num_leaf_var_changes
	  (stat.num_leaf_var_changes() + 1);
    }

  stat.num_func_syms_added(added_unrefed_fn_syms_.size());
  stat.num_added_func_syms_filtered_out(suppressed_added_unrefed_fn_syms_.size());
  stat.num_func_syms_removed(deleted_unrefed_fn_syms_.size());
  stat.num_removed_func_syms_filtered_out(suppressed_deleted_unrefed_fn_syms_.size());
  stat.num_var_syms_added(added_unrefed_var_syms_.size());
  stat.num_added_var_syms_filtered_out(suppressed_added_unrefed_var_syms_.size());
  stat.num_var_syms_removed(deleted_unrefed_var_syms_.size());
  stat.num_removed_var_syms_filtered_out(suppressed_deleted_unrefed_var_syms_.size());

  // Walk the general leaf type diff nodes to count them
  {
    size_t num_type_changes = 0, num_type_filtered = 0;
    count_leaf_type_changes(num_type_changes, num_type_filtered);

    stat.num_leaf_type_changes(num_type_changes);
    stat.num_leaf_type_changes_filtered_out(num_type_filtered);
  }

  // Walk the general leaf artefacts diff nodes to count them
  {
    size_t num_changes = 0, num_filtered = 0;
    count_leaf_changes(num_changes, num_filtered);

    stat.num_leaf_changes(num_changes);
    stat.num_leaf_changes_filtered_out(num_filtered);
  }

  // Walk the unreachable types to count them
  {
    size_t num_added_unreachable_types = 0,
      num_changed_unreachable_types = 0,
      num_deleted_unreachable_types = 0,
      num_added_unreachable_types_filtered = 0,
      num_changed_unreachable_types_filtered = 0,
      num_deleted_unreachable_types_filtered = 0;

    count_unreachable_types(num_added_unreachable_types,
			    num_deleted_unreachable_types,
			    num_changed_unreachable_types,
			    num_added_unreachable_types_filtered,
			    num_deleted_unreachable_types_filtered,
			    num_changed_unreachable_types_filtered);

    stat.num_added_unreachable_types(num_added_unreachable_types);
    stat.num_removed_unreachable_types(num_deleted_unreachable_types);
    stat.num_changed_unreachable_types(num_changed_unreachable_types);
    stat.num_added_unreachable_types_filtered_out
      (num_added_unreachable_types_filtered);
    stat.num_removed_unreachable_types_filtered_out
      (num_deleted_unreachable_types_filtered);
    stat.num_changed_unreachable_types_filtered_out
      (num_changed_unreachable_types_filtered);
  }
}

/// Emit the summary of the functions & variables that got
/// removed/changed/added.
///
/// TODO: This should be handled by the reporters, just like what is
/// done for reporter_base::diff_to_be_reported.
///
/// @param out the output stream to emit the stats to.
///
/// @param indent the indentation string to use in the summary.
void
corpus_diff::priv::emit_diff_stats(const diff_stats&	s,
				   ostream&		out,
				   const string&	indent)
{
  /// Report added/removed/changed functions.
  size_t net_num_leaf_changes =
    s.net_num_func_removed() +
    s.net_num_func_added() +
    s.net_num_leaf_func_changes() +
    s.net_num_vars_removed() +
    s.net_num_vars_added() +
    s.net_num_leaf_var_changes() +
    s.net_num_leaf_type_changes();

  if (!sonames_equal_)
    out << indent << "ELF SONAME changed\n";

  if (!architectures_equal_)
    out << indent << "ELF architecture changed\n";

  diff_context_sptr ctxt = get_context();

  if (ctxt->show_leaf_changes_only())
    {
      out << "Leaf changes summary: ";
      out << net_num_leaf_changes << " artifact";
      if (net_num_leaf_changes > 1)
	out << "s";
      out << " changed";

      if (size_t num_filtered = s.num_leaf_changes_filtered_out())
	out << " (" << num_filtered << " filtered out)";
      out << "\n";

      out << indent << "Changed leaf types summary: "
	  << s.net_num_leaf_type_changes();
      if (s.num_leaf_type_changes_filtered_out())
	out << " (" << s.num_leaf_type_changes_filtered_out()
	    << " filtered out)";
      out << " leaf type";
      if (s.num_leaf_type_changes() > 1)
	out << "s";
      out << " changed\n";

      // function changes summary
      out << indent << "Removed/Changed/Added functions summary: ";
      out << s.net_num_func_removed() << " Removed";
     if (s.num_removed_func_filtered_out())
	out << " ("
	    << s.num_removed_func_filtered_out()
	    << " filtered out)";
      out << ", ";

      out << s.net_num_leaf_func_changes() << " Changed";
      if (s.num_leaf_func_changes_filtered_out())
	    out << " ("
		<< s.num_leaf_func_changes_filtered_out()
		<< " filtered out)";
      out << ", ";

      out << s.net_num_func_added()<< " Added ";
      if (s.net_num_func_added() <= 1)
	out << "function";
      else
	out << "functions";
      if (s.num_added_func_filtered_out())
	out << " (" << s.num_added_func_filtered_out() << " filtered out)";
      out << "\n";

      // variables changes summary
      out << indent << "Removed/Changed/Added variables summary: ";
      out << s.net_num_vars_removed() << " Removed";
      if (s.num_removed_vars_filtered_out())
	out << " (" << s.num_removed_vars_filtered_out()
	    << " filtered out)";
      out << ", ";

      out << s.net_num_leaf_var_changes() << " Changed";
      if (s.num_leaf_var_changes_filtered_out())
	out << " ("
	    << s.num_leaf_var_changes_filtered_out()
	    << " filtered out)";
      out << ", ";

      out << s.net_num_vars_added() << " Added ";
      if (s.net_num_vars_added() <= 1)
	out << "variable";
      else
	out << "variables";
      if (s.num_added_vars_filtered_out())
	out << " (" << s.num_added_vars_filtered_out()
	    << " filtered out)";
      out << "\n";
    }
  else // if (ctxt->show_leaf_changes_only())
    {
      size_t total_nb_function_changes = s.num_func_removed()
	+ s.num_func_changed() +  s.num_func_added();

      // function changes summary
      out << indent << "Functions changes summary: ";
      out << s.net_num_func_removed() << " Removed";
      if (s.num_removed_func_filtered_out())
	out << " ("
	    << s.num_removed_func_filtered_out()
	    << " filtered out)";
      out << ", ";

      out << s.net_num_func_changed() << " Changed";
      if (s.num_changed_func_filtered_out())
	out << " (" << s.num_changed_func_filtered_out() << " filtered out)";
      out << ", ";

      out << s.net_num_func_added() << " Added";
      if (s.num_added_func_filtered_out())
	out << " (" << s.num_added_func_filtered_out() << " filtered out)";
      if (total_nb_function_changes <= 1)
	out << " function";
      else
	out << " functions";
      out << "\n";

      // variables changes summary
      size_t total_nb_variable_changes = s.num_vars_removed()
	+ s.num_vars_changed() + s.num_vars_added();

      out << indent << "Variables changes summary: ";
      out << s.net_num_vars_removed() << " Removed";
      if (s.num_removed_vars_filtered_out())
	out << " (" << s.num_removed_vars_filtered_out()
	    << " filtered out)";
      out << ", ";

      out << s.num_vars_changed() - s.num_changed_vars_filtered_out() << " Changed";
      if (s.num_changed_vars_filtered_out())
	out << " (" << s.num_changed_vars_filtered_out() << " filtered out)";
      out << ", ";

      out << s.net_num_vars_added() << " Added";
      if (s.num_added_vars_filtered_out())
	out << " (" << s.num_added_vars_filtered_out()
	    << " filtered out)";
      if (total_nb_variable_changes <= 1)
	out << " variable";
      else
	out << " variables";
      out << "\n";
    }

  // Show statistics about types not reachable from global
  // functions/variables.
  if (ctxt->show_unreachable_types())
    {
      size_t total_nb_unreachable_type_changes =
	s.num_removed_unreachable_types()
	+ s.num_changed_unreachable_types()
	+ s.num_added_unreachable_types();

      // Show summary of unreachable types
      out << indent << "Unreachable types summary: "
	  << s.net_num_removed_unreachable_types()
	  << " removed";
      if (s.num_removed_unreachable_types_filtered_out())
	out << " (" << s.num_removed_unreachable_types_filtered_out()
	    << " filtered out)";
      out << ", ";

      out << s.net_num_changed_unreachable_types()
	  << " changed";
      if (s.num_changed_unreachable_types_filtered_out())
	out << " (" << s.num_changed_unreachable_types_filtered_out()
	    << " filtered out)";
      out << ", ";

      out << s.net_num_added_unreachable_types()
	  << " added";
      if (s.num_added_unreachable_types_filtered_out())
	out << " (" << s.num_added_unreachable_types_filtered_out()
	    << " filtered out)";
      if (total_nb_unreachable_type_changes <= 1)
	out << " type";
      else
	out << " types";
      out << "\n";
    }

  if (ctxt->show_symbols_unreferenced_by_debug_info()
      && (s.num_func_syms_removed()
	  || s.num_func_syms_added()
	  || s.num_var_syms_removed()
	  || s.num_var_syms_added()))
    {
      // function symbols changes summary.

      if (!ctxt->show_added_symbols_unreferenced_by_debug_info()
	  && s.num_func_syms_removed() == 0
	  && s.num_func_syms_added() != 0)
	// If the only unreferenced function symbol change is function
	// syms that got added, but we were forbidden to show function
	// syms being added, do nothing.
	;
      else
	{
	  out << indent
	      << "Function symbols changes summary: "
	      << s.net_num_removed_func_syms() << " Removed";
	  if (s.num_removed_func_syms_filtered_out())
	    out << " (" << s.num_removed_func_syms_filtered_out()
		<< " filtered out)";
	  out << ", ";
	  out << s.net_num_added_func_syms() << " Added";
	  if (s.num_added_func_syms_filtered_out())
	    out << " (" << s.num_added_func_syms_filtered_out()
		<< " filtered out)";
	  out << " function symbol";
	  if (s.num_func_syms_added() + s.num_func_syms_removed() > 1)
	    out << "s";
	  out << " not referenced by debug info\n";
	}

      // variable symbol changes summary.

      if (!ctxt->show_added_symbols_unreferenced_by_debug_info()
	  && s.num_var_syms_removed() == 0
	  && s.num_var_syms_added() != 0)
	// If the only unreferenced variable symbol change is variable
	// syms that got added, but we were forbidden to show variable
	// syms being added, do nothing.
	;
      else
	{
	  out << indent
	      << "Variable symbols changes summary: "
	      << s.net_num_removed_var_syms() << " Removed";
	  if (s.num_removed_var_syms_filtered_out())
	    out << " (" << s.num_removed_var_syms_filtered_out()
		<< " filtered out)";
	  out << ", ";
	  out << s.net_num_added_var_syms() << " Added";
	  if (s.num_added_var_syms_filtered_out())
	    out << " (" << s.num_added_var_syms_filtered_out()
		<< " filtered out)";
	  out << " variable symbol";
	  if (s.num_var_syms_added() + s.num_var_syms_removed() > 1)
	    out << "s";
	  out << " not referenced by debug info\n";
	}
    }
}

/// Walk the changed functions and variables diff nodes to categorize
/// redundant nodes.
void
corpus_diff::priv::categorize_redundant_changed_sub_nodes()
{
  diff_sptr diff;

  diff_context_sptr ctxt = get_context();

  ctxt->forget_visited_diffs();
  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_fns_.begin();
       i!= changed_fns_.end();
       ++i)
    {
      diff = *i;
      categorize_redundancy(diff);
    }

  for (var_diff_sptrs_type::const_iterator i = sorted_changed_vars_.begin();
       i!= sorted_changed_vars_.end();
       ++i)
    {
      diff_sptr diff = *i;
      categorize_redundancy(diff);
    }

  for (diff_sptrs_type::const_iterator i =
	 changed_unreachable_types_sorted().begin();
       i!= changed_unreachable_types_sorted().end();
       ++i)
    {
      diff_sptr diff = *i;
      categorize_redundancy(diff);
    }
}

/// Walk the changed functions and variables diff nodes and clear the
/// redundancy categorization they might carry.
void
corpus_diff::priv::clear_redundancy_categorization()
{
  diff_sptr diff;
  for (function_decl_diff_sptrs_type::const_iterator i = changed_fns_.begin();
       i!= changed_fns_.end();
       ++i)
    {
      diff = *i;
      abigail::comparison::clear_redundancy_categorization(diff);
    }

  for (var_diff_sptrs_type::const_iterator i = sorted_changed_vars_.begin();
       i!= sorted_changed_vars_.end();
       ++i)
    {
      diff = *i;
      abigail::comparison::clear_redundancy_categorization(diff);
    }
}

/// If the user asked to dump the diff tree node (for changed
/// variables and functions) on the error output stream, then just do
/// that.
///
/// This function is used for debugging purposes.
void
corpus_diff::priv::maybe_dump_diff_tree()
{
  diff_context_sptr ctxt = get_context();

  if (!ctxt->dump_diff_tree()
      || ctxt->error_output_stream() == 0)
    return;

  if (!changed_fns_.empty())
    {
      *ctxt->error_output_stream() << "changed functions diff tree: \n\n";
      for (function_decl_diff_sptrs_type::const_iterator i =
	     changed_fns_.begin();
	   i != changed_fns_.end();
	   ++i)
	{
	  diff_sptr d = *i;
	  print_diff_tree(d, *ctxt->error_output_stream());
	}
    }

  if (!sorted_changed_vars_.empty())
    {
      *ctxt->error_output_stream() << "\nchanged variables diff tree: \n\n";
      for (var_diff_sptrs_type::const_iterator i =
	     sorted_changed_vars_.begin();
	   i != sorted_changed_vars_.end();
	   ++i)
	{
	  diff_sptr d = *i;
	  print_diff_tree(d, *ctxt->error_output_stream());
	}
    }

  if (!changed_unreachable_types_sorted().empty())
    {
      *ctxt->error_output_stream() << "\nchanged unreachable "
	"types diff tree: \n\n";
      for (vector<diff_sptr>::const_iterator i =
	     changed_unreachable_types_sorted().begin();
	   i != changed_unreachable_types_sorted().end();
	   ++i)
	{
	  diff_sptr d = *i;
	  print_diff_tree(d, *ctxt->error_output_stream());
	}
    }
}

/// Populate the vector of children node of the @ref corpus_diff type.
///
/// The children node can then later be retrieved using
/// corpus_diff::children_node().
void
corpus_diff::chain_into_hierarchy()
{
  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_functions_sorted().begin();
       i != changed_functions_sorted().end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);
}

/// Constructor for @ref corpus_diff.
///
/// @param first the first corpus of the diff.
///
/// @param second the second corpus of the diff.
///
/// @param ctxt the diff context to use.  Note that this context
/// object must stay alive at least during the life time of the
/// current instance of @ref corpus_diff.  Otherwise memory corruption
/// issues occur.
corpus_diff::corpus_diff(corpus_sptr first,
			 corpus_sptr second,
			 diff_context_sptr ctxt)
  : priv_(new priv(first, second, ctxt))
{}

/// Finish building the current instance of @ref corpus_diff.
void
corpus_diff::finish_diff_type()
{
  if (priv_->finished_)
    return;
  chain_into_hierarchy();
  priv_->finished_ = true;
}

/// @return the first corpus of the diff.
corpus_sptr
corpus_diff::first_corpus() const
{return priv_->first_;}

/// @return the second corpus of the diff.
corpus_sptr
corpus_diff::second_corpus() const
{return priv_->second_;}

/// @return the children nodes of the current instance of corpus_diff.
const vector<diff*>&
corpus_diff::children_nodes() const
{return priv_->children_;}

/// Append a new child node to the vector of children nodes for the
/// current instance of @ref corpus_diff node.
///
/// Note that the vector of children nodes for the current instance of
/// @ref corpus_diff node must remain sorted, using
/// diff_less_than_functor.
///
/// @param d the new child node.  Note that the life time of the
/// object held by @p d will thus equal the life time of the current
/// instance of @ref corpus_diff.
void
corpus_diff::append_child_node(diff_sptr d)
{
  ABG_ASSERT(d);

  diff_less_than_functor is_less_than;
  bool inserted = false;
  for (vector<diff*>::iterator i = priv_->children_.begin();
       i != priv_->children_.end();
       ++i)
    // Look for the point where to insert the diff child node.
    if (!is_less_than(d.get(), *i))
      {
	context()->keep_diff_alive(d);
	priv_->children_.insert(i, d.get());
	// As we have just inserted 'd' into the vector, the iterator
	// 'i' is invalidated.  We must *NOT* use it anymore.
	inserted = true;
	break;
      }

  if (!inserted)
    {
      context()->keep_diff_alive(d);
      // We didn't insert anything to the vector, presumably b/c it was
      // empty or had one element that was "less than" 'd'.  We can thus
      // just append 'd' to the end of the vector.
      priv_->children_.push_back(d.get());
    }
}

/// @return the bare edit script of the functions changed as recorded
/// by the diff.
edit_script&
corpus_diff::function_changes() const
{return priv_->fns_edit_script_;}

/// @return the bare edit script of the variables changed as recorded
/// by the diff.
edit_script&
corpus_diff::variable_changes() const
{return priv_->vars_edit_script_;}

/// Test if the soname of the underlying corpus has changed.
///
/// @return true iff the soname has changed.
bool
corpus_diff::soname_changed() const
{return !priv_->sonames_equal_;}

/// Test if the architecture of the underlying corpus has changed.
///
/// @return true iff the architecture has changed.
bool
corpus_diff::architecture_changed() const
{return !priv_->architectures_equal_;}

/// Getter for the deleted functions of the diff.
///
/// @return the the deleted functions of the diff.
const string_function_ptr_map&
corpus_diff::deleted_functions() const
{return priv_->deleted_fns_;}

/// Getter for the added functions of the diff.
///
/// @return the added functions of the diff.
const string_function_ptr_map&
corpus_diff::added_functions()
{return priv_->added_fns_;}

/// Getter for the functions which signature didn't change, but which
/// do have some indirect changes in their parms.
///
/// @return a non-sorted map of functions which signature didn't
/// change, but which do have some indirect changes in their parms.
/// The key of the map is a unique identifier for the function; it's
/// usually made of the name and version of the underlying ELF symbol
/// of the function for corpora that were built from ELF files.
const string_function_decl_diff_sptr_map&
corpus_diff::changed_functions()
{return priv_->changed_fns_map_;}

/// Getter for a sorted vector of functions which signature didn't
/// change, but which do have some indirect changes in their parms.
///
/// @return a sorted vector of functions which signature didn't
/// change, but which do have some indirect changes in their parms.
const function_decl_diff_sptrs_type&
corpus_diff::changed_functions_sorted()
{return priv_->changed_fns_;}

/// Getter for the variables that got deleted from the first subject
/// of the diff.
///
/// @return the map of deleted variable.
const string_var_ptr_map&
corpus_diff::deleted_variables() const
{return priv_->deleted_vars_;}

/// Getter for the added variables of the diff.
///
/// @return the map of added variable.
const string_var_ptr_map&
corpus_diff::added_variables() const
{return priv_->added_vars_;}

/// Getter for the non-sorted map of variables which signature didn't
/// change but which do have some indirect changes in some sub-types.
///
/// @return the non-sorted map of changed variables.
const string_var_diff_sptr_map&
corpus_diff::changed_variables()
{return priv_->changed_vars_map_;}

/// Getter for the sorted vector of variables which signature didn't
/// change but which do have some indirect changes in some sub-types.
///
/// @return a sorted vector of changed variables.
const var_diff_sptrs_type&
corpus_diff::changed_variables_sorted()
{return priv_->sorted_changed_vars_;}

/// Getter for function symbols not referenced by any debug info and
/// that got deleted.
///
/// @return a map of elf function symbols not referenced by any debug
/// info and that got deleted.
const string_elf_symbol_map&
corpus_diff::deleted_unrefed_function_symbols() const
{return priv_->deleted_unrefed_fn_syms_;}

/// Getter for function symbols not referenced by any debug info and
/// that got added.
///
/// @return a map of elf function symbols not referenced by any debug
/// info and that got added.
const string_elf_symbol_map&
corpus_diff::added_unrefed_function_symbols() const
{return priv_->added_unrefed_fn_syms_;}

/// Getter for variable symbols not referenced by any debug info and
/// that got deleted.
///
/// @return a map of elf variable symbols not referenced by any debug
/// info and that got deleted.
const string_elf_symbol_map&
corpus_diff::deleted_unrefed_variable_symbols() const
{return priv_->deleted_unrefed_var_syms_;}

/// Getter for variable symbols not referenced by any debug info and
/// that got added.
///
/// @return a map of elf variable symbols not referenced by any debug
/// info and that got added.
const string_elf_symbol_map&
corpus_diff::added_unrefed_variable_symbols() const
{return priv_->added_unrefed_var_syms_;}

/// Getter for a map of deleted types that are not reachable from
/// global functions/variables.
///
/// @return a map that associates pretty representation of deleted
/// unreachable types and said types.
const string_type_base_sptr_map&
corpus_diff::deleted_unreachable_types() const
{return priv_->deleted_unreachable_types_;}

/// Getter of a sorted vector of deleted types that are not reachable
/// from global functions/variables.
///
/// @return a sorted vector of deleted types that are not reachable
/// from global functions/variables.  The types are lexicographically
/// sorted by considering their pretty representation.
const vector<type_base_sptr>&
corpus_diff::deleted_unreachable_types_sorted() const
{
  if (priv_->deleted_unreachable_types_sorted_.empty())
    if (!priv_->deleted_unreachable_types_.empty())
      sort_string_type_base_sptr_map(priv_->deleted_unreachable_types_,
				     priv_->deleted_unreachable_types_sorted_);

  return priv_->deleted_unreachable_types_sorted_;
}

/// Getter for a map of added types that are not reachable from global
/// functions/variables.
///
/// @return a map that associates pretty representation of added
/// unreachable types and said types.
const string_type_base_sptr_map&
corpus_diff::added_unreachable_types() const
{return priv_->added_unreachable_types_;}

/// Getter of a sorted vector of added types that are not reachable
/// from global functions/variables.
///
/// @return a sorted vector of added types that are not reachable from
/// global functions/variables.  The types are lexicographically
/// sorted by considering their pretty representation.
const vector<type_base_sptr>&
corpus_diff::added_unreachable_types_sorted() const
{
  if (priv_->added_unreachable_types_sorted_.empty())
    if (!priv_->added_unreachable_types_.empty())
      sort_string_type_base_sptr_map(priv_->added_unreachable_types_,
				     priv_->added_unreachable_types_sorted_);

  return priv_->added_unreachable_types_sorted_;
}

/// Getter for a map of changed types that are not reachable from
/// global functions/variables.
///
/// @return a map that associates pretty representation of changed
/// unreachable types and said types.
const string_diff_sptr_map&
corpus_diff::changed_unreachable_types() const
{return priv_->changed_unreachable_types_;}

/// Getter of a sorted vector of changed types that are not reachable
/// from global functions/variables.
///
/// @return a sorted vector of changed types that are not reachable
/// from global functions/variables.  The diffs are lexicographically
/// sorted by considering their pretty representation.
const vector<diff_sptr>&
corpus_diff::changed_unreachable_types_sorted() const
{return priv_->changed_unreachable_types_sorted();}

/// Getter of the diff context of this diff
///
/// @return the diff context for this diff.
const diff_context_sptr
corpus_diff::context() const
{return priv_->get_context();}

/// @return the pretty representation for the current instance of @ref
/// corpus_diff
const string&
corpus_diff::get_pretty_representation() const
{
  if (priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "corpus_diff["
	<< first_corpus()->get_path()
	<< ", "
	<< second_corpus()->get_path()
	<< "]";
      priv_->pretty_representation_ = o.str();
    }
  return priv_->pretty_representation_;
}
/// Return true iff the current @ref corpus_diff node carries a
/// change.
///
/// @return true iff the current diff node carries a change.
bool
corpus_diff::has_changes() const
{
  return (soname_changed()
	  || architecture_changed()
	  || !(priv_->deleted_fns_.empty()
	       && priv_->added_fns_.empty()
	       && priv_->changed_fns_map_.empty()
	       && priv_->deleted_vars_.empty()
	       && priv_->added_vars_.empty()
	       && priv_->changed_vars_map_.empty()
	       && priv_->added_unrefed_fn_syms_.empty()
	       && priv_->deleted_unrefed_fn_syms_.empty()
	       && priv_->added_unrefed_var_syms_.empty()
	       && priv_->deleted_unrefed_var_syms_.empty()
	       && priv_->deleted_unreachable_types_.empty()
	       && priv_->added_unreachable_types_.empty()
	       && priv_->changed_unreachable_types_.empty()));
}

/// Test if the current instance of @ref corpus_diff carries changes
/// that we are sure are incompatible.  By incompatible change we mean
/// a change that "breaks" the ABI of the corpus we are looking at.
///
/// In concrete terms, this function considers the following changes
/// as being ABI incompatible for sure:
///
///   - a soname change
///   - if exported functions or variables got removed
///
/// Note that subtype changes *can* represent changes that break ABI
/// too.  But they also can be changes that are OK, ABI-wise.
///
/// It's up to the user to provide suppression specifications to say
/// explicitely which subtype change is OK.  The remaining sub-type
/// changes are then considered to be ABI incompatible.  But to test
/// if such ABI incompatible subtype changes are present you need to
/// use the function @ref corpus_diff::has_net_subtype_changes()
///
/// @return true iff the current instance of @ref corpus_diff carries
/// changes that we are sure are ABI incompatible.
bool
corpus_diff::has_incompatible_changes() const
{
  const diff_stats& stats = const_cast<corpus_diff*>(this)->
    apply_filters_and_suppressions_before_reporting();

  return (soname_changed() || architecture_changed()
	  || stats.net_num_func_removed() != 0
	  || (stats.num_func_with_virtual_offset_changes() != 0
	      // If all reports about functions with sub-type changes
	      // have been suppressed, then even those about functions
	      // that are virtual don't matter anymore because the
	      // user willingly requested to shut them down
	      && stats.net_num_func_changed() != 0)
	  || stats.net_num_vars_removed() != 0
	  || stats.net_num_removed_func_syms() != 0
	  || stats.net_num_removed_var_syms() != 0
	  || stats.net_num_removed_unreachable_types() != 0
	  || stats.net_num_changed_unreachable_types() != 0);
}

/// Test if the current instance of @ref corpus_diff carries subtype
/// changes whose reports are not suppressed by any suppression
/// specification.  In effect, these are deemed incompatible ABI
/// changes.
///
/// @return true iff the the current instance of @ref corpus_diff
/// carries subtype changes that are deemed incompatible ABI changes.
bool
corpus_diff::has_net_subtype_changes() const
{
  const diff_stats& stats = const_cast<corpus_diff*>(this)->
      apply_filters_and_suppressions_before_reporting();

  return (stats.net_num_func_changed() != 0
	  || stats.net_num_vars_changed() != 0
	  || stats.net_num_removed_unreachable_types() != 0
	  || stats.net_num_changed_unreachable_types() != 0);
}

/// Test if the current instance of @ref corpus_diff carries changes
/// whose reports are not suppressed by any suppression specification.
/// In effect, these are deemed incompatible ABI changes.
///
/// @return true iff the the current instance of @ref corpus_diff
/// carries subtype changes that are deemed incompatible ABI changes.
bool
corpus_diff::has_net_changes() const
{return  context()->get_reporter()->diff_has_net_changes(this);}

/// Apply the different filters that are registered to be applied to
/// the diff tree; that includes the categorization filters.  Also,
/// apply the suppression interpretation filters.
///
/// After the filters are applied, this function calculates some
/// statistics about the changes carried by the current instance of
/// @ref corpus_diff.  These statistics are represented by an instance
/// of @ref corpus_diff::diff_stats.
///
/// This member function is called by the reporting function
/// corpus_diff::report().
///
/// Note that for a given instance of corpus_diff, this function
/// applies the filters and suppressions only the first time it is
/// invoked.  Subsequent invocations just return the instance of
/// corpus_diff::diff_stats that was cached after the first
/// invocation.
///
/// @return a reference to the statistics about the changes carried by
/// the current instance of @ref corpus_diff.
const corpus_diff::diff_stats&
corpus_diff::apply_filters_and_suppressions_before_reporting()
{
  if (priv_->diff_stats_)
    return *priv_->diff_stats_;

  apply_suppressions(this);
  priv_->diff_stats_.reset(new diff_stats(context()));
  mark_leaf_diff_nodes();
  priv_->apply_filters_and_compute_diff_stats(*priv_->diff_stats_);
  return *priv_->diff_stats_;
}

/// A visitor that marks leaf diff nodes by storing them in the
/// instance of @ref diff_maps returned by
/// corpus_diff::get_leaf_diffs() invoked on the current instance of
/// corpus_diff.
struct leaf_diff_node_marker_visitor : public diff_node_visitor
{
  /// This is called when the visitor visits a diff node.
  ///
  /// It basically tests if the diff node being visited is a leaf diff
  /// node - that is, it contains local changes.  If it does, then the
  /// node is added to the set of maps that hold leaf diffs in the
  /// current corpus_diff.
  ///
  /// Note that only leaf nodes that are reachable from public
  /// interfaces (global functions or variables) are collected by this
  /// visitor.
  ///
  /// @param d the diff node being visited.
  virtual void
  visit_begin(diff *d)
  {
    if (d->has_local_changes()
	// A leaf basic (or class/union) type name change makes no
	// sense when showing just leaf changes.  It only makes sense
	// when it can explain the details about a non-leaf change.
	// In other words, it doesn't make sense to say that an "int"
	// became "unsigned int".  But it does make sense to say that
	// a typedef changed because its underlying type was 'int' and
	// is now an "unsigned int".
	&& !filtering::has_basic_or_class_type_name_change(d)
	// Similarly, a *local* change describing a type that changed
	// its nature doesn't make sense.
	&& !is_distinct_diff(d)
	// Similarly, a pointer (or reference or array), a typedef or
	// qualified type change in itself doesn't make sense.  It
	// would rather make sense to show that pointer change as part
	// of the variable change whose pointer type changed, for
	// instance.
	&& !is_pointer_diff(d)
	&& !is_reference_diff(d)
	&& !is_qualified_type_diff(d)
	&& !is_typedef_diff(d)
	&& !is_array_diff(d)
	// Similarly a parameter change in itself doesn't make sense.
	// It should have already been reported as part of the change
	// of the function it belongs to.
	&& !is_fn_parm_diff(d)
	// An anonymous class or union diff doesn't make sense on its
	// own.  It must have been described already by the diff of
	// the enclosing struct or union if 'd' is from an anonymous
	// data member, or from a typedef change if 'd' is from a
	// typedef change which underlying type is an anonymous
	// struct/union.
	&& !is_anonymous_class_or_union_diff(d)
	// Don't show decl-only-ness changes either.
	&& !filtering::has_decl_only_def_change(d)
	// Sometime, we can encounter artifacts of bogus DWARF that
	// yield a diff node for a decl-only class (and empty class
	// with the is_declaration flag set) that carries a non-zero
	// size!  And of course at some point that non-zero size
	// changes.  We need to be able to detect that.
	&& !filtering::is_decl_only_class_with_size_change(d))
      {
	diff_context_sptr ctxt = d->context();
	const corpus_diff *corpus_diff_node = ctxt->get_corpus_diff().get();
	ABG_ASSERT(corpus_diff_node);

	if (diff *iface_diff = get_current_topmost_iface_diff())
	  {
	    type_or_decl_base_sptr iface = iface_diff->first_subject();
	    // So, this diff node that is reachable from a global
	    // function or variable carries a leaf change.  Let's add
	    // it to the set of of leaf diffs of corpus_diff_node.
	    const_cast<corpus_diff*>(corpus_diff_node)->
	      get_leaf_diffs().insert_diff_node(d, iface);
	  }
      }
  }
}; // end struct leaf_diff_node_marker_visitor

/// Walks the diff nodes associated to the current corpus diff and
/// mark those that carry local changes.  They are said to be leaf
/// diff nodes.
///
/// The marked nodes are available from the
/// corpus_diff::get_leaf_diffs() function.
void
corpus_diff::mark_leaf_diff_nodes()
{
  if (!has_changes())
    return;

  if (!context()->show_leaf_changes_only())
    return;

  leaf_diff_node_marker_visitor v;
  context()->forget_visited_diffs();
  bool s = context()->visiting_a_node_twice_is_forbidden();
  context()->forbid_visiting_a_node_twice(true);
  context()->forbid_visiting_a_node_twice_per_interface(true);
  traverse(v);
  context()->forbid_visiting_a_node_twice(s);
  context()->forbid_visiting_a_node_twice_per_interface(false);
}

/// Get the set of maps that contain leaf nodes.  A leaf node being a
/// node with a local change.
///
/// @return the set of maps that contain leaf nodes.  A leaf node
/// being a node with a local change.
diff_maps&
corpus_diff::get_leaf_diffs()
{return priv_->leaf_diffs_;}

/// Get the set of maps that contain leaf nodes.  A leaf node being a
/// node with a local change.
///
/// @return the set of maps that contain leaf nodes.  A leaf node
/// being a node with a local change.
const diff_maps&
corpus_diff::get_leaf_diffs() const
{return priv_->leaf_diffs_;}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
corpus_diff::report(ostream& out, const string& indent) const
{
  context()->get_reporter()->report(*this, out, indent);
}

/// Traverse the diff sub-tree under the current instance corpus_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going on, false otherwise.
bool
corpus_diff::traverse(diff_node_visitor& v)
{
  finish_diff_type();

  v.visit_begin(this);

  if (!v.visit(this, true))
    {
      v.visit_end(this);
      return false;
    }

  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_functions_sorted().begin();
       i != changed_functions_sorted().end();
       ++i)
    {
      if (diff_sptr d = *i)
	{
	  const diff_context_sptr &ctxt = context();
	  if (ctxt->visiting_a_node_twice_is_forbidden_per_interface())
	    ctxt->forget_visited_diffs();

	  v.set_current_topmost_iface_diff(d.get());

	  if (!d->traverse(v))
	    {
	      v.visit_end(this);
	      v.set_current_topmost_iface_diff(0);
	      return false;
	    }
	}
    }

  for (var_diff_sptrs_type::const_iterator i =
	 changed_variables_sorted().begin();
       i != changed_variables_sorted().end();
       ++i)
    {
      if (diff_sptr d = *i)
	{
	  const diff_context_sptr &ctxt = context();
	  if (ctxt->visiting_a_node_twice_is_forbidden_per_interface())
	    ctxt->forget_visited_diffs();

	  v.set_current_topmost_iface_diff(d.get());

	  if (!d->traverse(v))
	    {
	      v.visit_end(this);
	      v.set_current_topmost_iface_diff(0);
	      return false;
	    }
	}
    }

  v.set_current_topmost_iface_diff(0);

  // Traverse the changed unreachable type diffs.  These diffs are on
  // types that are not reachable from global functions or variables.
  for (vector<diff_sptr>::const_iterator i =
	 changed_unreachable_types_sorted().begin();
       i != changed_unreachable_types_sorted().end();
       ++i)
    {
      if (diff_sptr d = *i)
	{
	  const diff_context_sptr &ctxt = context();
	  if (ctxt->visiting_a_node_twice_is_forbidden_per_interface())
	    ctxt->forget_visited_diffs();

	  if (!d->traverse(v))
	    {
	      v.visit_end(this);
	      return false;
	    }
	}
    }

  v.visit_end(this);
  return true;
}

/// Compute the diff between two instances of @ref corpus.
///
/// Note that the two corpora must have been created in the same @ref
/// environment, otherwise, this function aborts.
///
/// @param f the first @ref corpus to consider for the diff.
///
/// @param s the second @ref corpus to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff between the two @ref corpus.
corpus_diff_sptr
compute_diff(const corpus_sptr	f,
	     const corpus_sptr	s,
	     diff_context_sptr	ctxt)
{
  typedef corpus::functions::const_iterator fns_it_type;
  typedef corpus::variables::const_iterator vars_it_type;
  typedef elf_symbols::const_iterator symbols_it_type;
  typedef diff_utils::deep_ptr_eq_functor eq_type;
  typedef vector<type_base_wptr>::const_iterator type_base_wptr_it_type;

  ABG_ASSERT(f && s);

  // We can only compare two corpora that were built out of the same
  // environment.
  ABG_ASSERT(f->get_environment() == s->get_environment());

  if (!ctxt)
    ctxt.reset(new diff_context);

  corpus_diff_sptr r(new corpus_diff(f, s, ctxt));

  ctxt->set_corpus_diff(r);

  r->priv_->sonames_equal_ = f->get_soname() == s->get_soname();

  r->priv_->architectures_equal_ =
    f->get_architecture_name() == s->get_architecture_name();

  // Compute the diff of publicly defined and exported functions
  diff_utils::compute_diff<fns_it_type, eq_type>(f->get_functions().begin(),
						 f->get_functions().end(),
						 s->get_functions().begin(),
						 s->get_functions().end(),
						 r->priv_->fns_edit_script_);

  // Compute the diff of publicly defined and exported variables.
  diff_utils::compute_diff<vars_it_type, eq_type>
    (f->get_variables().begin(), f->get_variables().end(),
     s->get_variables().begin(), s->get_variables().end(),
     r->priv_->vars_edit_script_);

  // Compute the diff of function elf symbols not referenced by debug
  // info.
  diff_utils::compute_diff<symbols_it_type, eq_type>
    (f->get_unreferenced_function_symbols().begin(),
     f->get_unreferenced_function_symbols().end(),
     s->get_unreferenced_function_symbols().begin(),
     s->get_unreferenced_function_symbols().end(),
     r->priv_->unrefed_fn_syms_edit_script_);

  // Compute the diff of variable elf symbols not referenced by debug
  // info.
    diff_utils::compute_diff<symbols_it_type, eq_type>
    (f->get_unreferenced_variable_symbols().begin(),
     f->get_unreferenced_variable_symbols().end(),
     s->get_unreferenced_variable_symbols().begin(),
     s->get_unreferenced_variable_symbols().end(),
     r->priv_->unrefed_var_syms_edit_script_);

    if (ctxt->show_unreachable_types())
      // Compute the diff of types not reachable from public functions
      // or global variables that are exported.
      diff_utils::compute_diff<type_base_wptr_it_type, eq_type>
	(f->get_types_not_reachable_from_public_interfaces().begin(),
	 f->get_types_not_reachable_from_public_interfaces().end(),
	 s->get_types_not_reachable_from_public_interfaces().begin(),
	 s->get_types_not_reachable_from_public_interfaces().end(),
	 r->priv_->unreachable_types_edit_script_);

  r->priv_->ensure_lookup_tables_populated();

  return r;
}

// </corpus stuff>

/// Compute the diff between two instances of @ref corpus_group.
///
/// Note that the two corpus_diff must have been created in the same
/// @ref environment, otherwise, this function aborts.
///
/// @param f the first @ref corpus_group to consider for the diff.
///
/// @param s the second @ref corpus_group to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff between the two @ref corpus_group.
corpus_diff_sptr
compute_diff(const corpus_group_sptr&	f,
	     const corpus_group_sptr&	s,
	     diff_context_sptr	ctxt)
{

  corpus_sptr c1 = f;
  corpus_sptr c2 = s;

  return compute_diff(c1, c2, ctxt);
}

// <corpus_group stuff>

// </corpus_group stuff>
// <diff_node_visitor stuff>

/// The private data of the @diff_node_visitor type.
struct diff_node_visitor::priv
{
  diff*	topmost_interface_diff;
  visiting_kind kind;

  priv()
    : topmost_interface_diff(),
      kind()
  {}

  priv(visiting_kind k)
    : topmost_interface_diff(),
      kind(k)
  {}
}; // end struct diff_node_visitor

/// Default constructor of the @ref diff_node_visitor type.
diff_node_visitor::diff_node_visitor()
  : priv_(new priv)
{}

/// Constructor of the @ref diff_node_visitor type.
///
/// @param k how the visiting has to be performed.
diff_node_visitor::diff_node_visitor(visiting_kind k)
  : priv_(new priv(k))
{}

/// Getter for the visiting policy of the traversing code while
/// invoking this visitor.
///
/// @return the visiting policy used by the traversing code when
/// invoking this visitor.
visiting_kind
diff_node_visitor::get_visiting_kind() const
{return priv_->kind;}

/// Setter for the visiting policy of the traversing code while
/// invoking this visitor.
///
/// @param v a bit map representing the new visiting policy used by
/// the traversing code when invoking this visitor.
void
diff_node_visitor::set_visiting_kind(visiting_kind v)
{priv_->kind = v;}

/// Setter for the visiting policy of the traversing code while
/// invoking this visitor.  This one makes a logical or between the
/// current policy and the bitmap given in argument and assigns the
/// current policy to the result.
///
/// @param v a bitmap representing the visiting policy to or with
/// the current policy.
void
diff_node_visitor::or_visiting_kind(visiting_kind v)
{priv_->kind = priv_->kind | v;}

/// Setter of the diff current topmost interface which is impacted by
/// the current diff node being visited.
///
/// @param d the current topmost interface diff impacted.
void
diff_node_visitor::set_current_topmost_iface_diff(diff* d)
{priv_->topmost_interface_diff = d;}

/// Getter of the diff current topmost interface which is impacted by
/// the current diff node being visited.
///
/// @return the current topmost interface diff impacted.
diff*
diff_node_visitor::get_current_topmost_iface_diff() const
{return priv_->topmost_interface_diff;}

/// This is called by the traversing code on a @ref diff node just
/// before visiting it.  That is, before visiting it and its children
/// node.
///
/// @param d the diff node to visit.
void
diff_node_visitor::visit_begin(diff* /*p*/)
{}

/// This is called by the traversing code on a @ref diff node just
/// after visiting it.  That is after visiting it and its children
/// nodes.
///
/// @param d the diff node that got visited.
void
diff_node_visitor::visit_end(diff* /*p*/)
{}

/// This is called by the traversing code on a @ref corpus_diff node
/// just before visiting it.  That is, before visiting it and its
/// children node.
///
/// @param p the corpus_diff node to visit.
///
void
diff_node_visitor::visit_begin(corpus_diff* /*p*/)
{}

/// This is called by the traversing code on a @ref corpus_diff node
/// just after visiting it.  That is after visiting it and its children
/// nodes.
///
/// @param d the diff node that got visited.
void
diff_node_visitor::visit_end(corpus_diff* /*d*/)
{}

/// Default visitor implementation
///
/// @return true
bool
diff_node_visitor::visit(diff*, bool)
{return true;}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(distinct_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(var_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(pointer_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(reference_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(qualified_type_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(enum_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(class_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(base_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(scope_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(function_decl_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(type_decl_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(typedef_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(translation_unit_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(corpus_diff*, bool)
{return true;}

// </diff_node_visitor stuff>

// <redundant diff node marking>

// </redundant diff node marking>

// <diff tree category propagation>

/// A visitor to propagate the category of a node up to its parent
/// nodes.  This visitor doesn't touch the REDUNDANT_CATEGORY or the
/// SUPPRESSED_CATEGORY because those are propagated using other
/// specific visitors.
struct category_propagation_visitor : public diff_node_visitor
{
  virtual void
  visit_end(diff* d)
  {
    // Has this diff node 'd' been already visited ?
    bool already_visited = d->context()->diff_has_been_visited(d);

    // The canonical diff node of the class of equivalence of the diff
    // node 'd'.
    diff* canonical = d->get_canonical_diff();

    // If this class of equivalence of diff node is being visited for
    // the first time, then update its canonical node's category too.
    bool update_canonical = !already_visited && canonical;

    for (vector<diff*>::const_iterator i = d->children_nodes().begin();
	 i != d->children_nodes().end();
	 ++i)
      {
	// If we are visiting the class of equivalence of 'd' for the
	// first time, then let's look at the children of 'd' and
	// propagate their categories to 'd'.
	//
	// If the class of equivalence of 'd' has already been
	// visited, then let's look at the canonical diff nodes of the
	// children of 'd' and propagate their categories to 'd'.
	diff* diff = already_visited
	  ? (*i)->get_canonical_diff()
	  : *i;

	ABG_ASSERT(diff);

	diff_category c = diff->get_category();
	// Do not propagate redundant and suppressed categories. Those
	// are propagated in a specific pass elsewhere.
	c &= ~(REDUNDANT_CATEGORY
	       | SUPPRESSED_CATEGORY
	       | PRIVATE_TYPE_CATEGORY);
	// Also, if a (class) type has got a harmful name change, do not
	// propagate harmless name changes coming from its sub-types
	// (i.e, data members) to the class itself.
	if (filtering::has_harmful_name_change(d))
	  c &= ~HARMLESS_DECL_NAME_CHANGE_CATEGORY;

	d->add_to_category(c);
	if (!already_visited && canonical)
	  if (update_canonical)
	    canonical->add_to_category(c);
      }
  }
};// end struct category_propagation_visitor

/// Visit all the nodes of a given sub-tree.  For each node that has a
/// particular category set, propagate that category set up to its
/// parent nodes.
///
/// @param diff_tree the diff sub-tree to walk for categorization
/// purpose;
void
propagate_categories(diff* diff_tree)
{
  category_propagation_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(true);
  diff_tree->context()->forget_visited_diffs();
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Visit all the nodes of a given sub-tree.  For each node that has a
/// particular category set, propagate that category set up to its
/// parent nodes.
///
/// @param diff_tree the diff sub-tree to walk for categorization
/// purpose;
void
propagate_categories(diff_sptr diff_tree)
{propagate_categories(diff_tree.get());}

/// Visit all the nodes of a given corpus tree.  For each node that
/// has a particular category set, propagate that category set up to
/// its parent nodes.
///
/// @param diff_tree the corpus_diff tree to walk for categorization
/// purpose;
void
propagate_categories(corpus_diff* diff_tree)
{
  category_propagation_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Visit all the nodes of a given corpus tree.  For each node that
/// has a particular category set, propagate that category set up to
/// its parent nodes.
///
/// @param diff_tree the corpus_diff tree to walk for categorization
/// purpose;
void
propagate_categories(corpus_diff_sptr diff_tree)
{propagate_categories(diff_tree.get());}

/// A tree node visitor that knows how to categorizes a given diff
/// node in the SUPPRESSED_CATEGORY category and how to propagate that
/// categorization.
struct suppression_categorization_visitor : public diff_node_visitor
{

  /// Before visiting the children of the diff node, check if the node
  /// is suppressed by a suppression specification.  If it is, mark
  /// the node as belonging to the SUPPRESSED_CATEGORY category.
  ///
  /// @param p the diff node to visit.
  virtual void
  visit_begin(diff* d)
  {
    bool is_private_type = false;
    if (d->is_suppressed(is_private_type))
      {
	diff_category c = is_private_type
	  ? PRIVATE_TYPE_CATEGORY
	  : SUPPRESSED_CATEGORY;
	d->add_to_local_and_inherited_categories(c);

	// If a node was suppressed, all the other nodes of its class
	// of equivalence are suppressed too.
	diff *canonical_diff = d->get_canonical_diff();
	if (canonical_diff != d)
	  canonical_diff->add_to_category(c);
      }
  }

  /// After visiting the children nodes of a given diff node,
  /// propagate the SUPPRESSED_CATEGORY from the children nodes to the
  /// diff node, if need be.
  ///
  /// That is, if all children nodes carry a suppressed change the
  /// current node should be marked as suppressed as well.
  ///
  /// In practice, this might be too strong of a condition.  If the
  /// current node carries a local change (i.e, a change not carried
  /// by any of its children node) and if that change is not
  /// suppressed, then the current node should *NOT* be suppressed.
  ///
  /// But right now, the IR doesn't let us know about local vs
  /// children-carried changes.  So we cannot be that precise yet.
  virtual void
  visit_end(diff* d)
  {
    bool has_non_suppressed_child = false;
    bool has_non_empty_child = false;
    bool has_suppressed_child = false;
    bool has_non_private_child = false;
    bool has_private_child = false;

    if (// A node to which we can propagate the "SUPPRESSED_CATEGORY"
	// (or the PRIVATE_TYPE_CATEGORY for the same matter)
	// category from its children is a node which:
	//
	//  1/ hasn't been suppressed already
	//
	//  2/ and has no local change (unless it's a pointer,
	//  reference or qualified diff node).
	//
	//  Note that qualified type and typedef diff nodes are a bit
	//  special.  The local changes of the underlying type are
	//  considered local for the qualified/typedef type, just like
	//  for pointer/reference types.  But then the qualified or
	//  typedef type itself can have local changes of its own, and
	//  those changes are of the kind LOCAL_NON_TYPE_CHANGE_KIND.
	//  So a qualified type which have local changes that are
	//  *NOT* of LOCAL_NON_TYPE_CHANGE_KIND (or that has no local
	//  changes at all) and which is in the PRIVATE_TYPE_CATEGORY
	//  or SUPPRESSED_CATEGORY can see these categories be
	//  propagated.
	//
	// Note that all pointer/reference diff node changes are
	// potentially considered local, i.e, local changes of the
	// pointed-to-type are considered local to the pointer itself.
	//
	// Similarly, changes local to the type of function parameters,
	// variables (and data members) and classes (that are not of
	// LOCAL_NON_TYPE_CHANGE_KIND kind) and that have been
	// suppressed can propagate their SUPPRESSED_CATEGORY-ness to
	// those kinds of diff node.
	!(d->get_category() & SUPPRESSED_CATEGORY)
	&& (!d->has_local_changes()
	    || is_pointer_diff(d)
	    || is_reference_diff(d)
	    || (is_qualified_type_diff(d)
		&& (!(d->has_local_changes() & LOCAL_NON_TYPE_CHANGE_KIND)))
	    || (is_typedef_diff(d)
		&& (!(d->has_local_changes() & LOCAL_NON_TYPE_CHANGE_KIND)))
	    || (is_function_decl_diff(d)
		&& (!(d->has_local_changes() & LOCAL_NON_TYPE_CHANGE_KIND)))
	    || (is_fn_parm_diff(d)
		&& (!(d->has_local_changes() & LOCAL_NON_TYPE_CHANGE_KIND)))
	    || (is_function_type_diff(d)
		&& (!(d->has_local_changes() & LOCAL_NON_TYPE_CHANGE_KIND)))
	    || (is_var_diff(d)
		&& (!(d->has_local_changes() & LOCAL_NON_TYPE_CHANGE_KIND)))
	    ||  (is_class_diff(d)
		&& (!(d->has_local_changes() & LOCAL_NON_TYPE_CHANGE_KIND)))))
      {
	// Note that we handle private diff nodes differently from
	// generally suppressed diff nodes.  E.g, it's not because a
	// type is private (and suppressed because of that; i.e, in
	// the category PRIVATE_TYPE_CATEGORY) that a typedef to that
	// type should also be private and so suppressed.  Private
	// diff nodes thus have different propagation rules than
	// generally suppressed rules.
	for (vector<diff*>::const_iterator i = d->children_nodes().begin();
	     i != d->children_nodes().end();
	     ++i)
	  {
	    diff* child = *i;
	    if (child->has_changes())
	      {
		has_non_empty_child = true;
		if (child->get_class_of_equiv_category() & SUPPRESSED_CATEGORY)
		  has_suppressed_child = true;
		else if (child->get_class_of_equiv_category()
			 & PRIVATE_TYPE_CATEGORY)
		  // Propagation of the PRIVATE_TYPE_CATEGORY is going
		  // to be handled later below.
		  ;
		else
		  has_non_suppressed_child = true;

		if (child->get_class_of_equiv_category()
		    & PRIVATE_TYPE_CATEGORY)
		  has_private_child = true;
		else if (child->get_class_of_equiv_category()
			 & SUPPRESSED_CATEGORY)
		  // Propagation of the SUPPRESSED_CATEGORY has been
		  // handled above already.
		  ;
		else
		  has_non_private_child = true;
	      }
	  }

	if (has_non_empty_child
	    && has_suppressed_child
	    && !has_non_suppressed_child)
	  {
	    d->add_to_category(SUPPRESSED_CATEGORY);
	    // If a node was suppressed, all the other nodes of its class
	    // of equivalence are suppressed too.
	    diff *canonical_diff = d->get_canonical_diff();
	    if (canonical_diff != d)
	      canonical_diff->add_to_category(SUPPRESSED_CATEGORY);
	  }

	// Note that the private-ness of a an underlying type won't be
	// propagated to its parent typedef, by virtue of the big "if"
	// clause at the beginning of this function.  So we don't have
	// to handle that case here.  So the idiom of defining
	// typedefs of private (opaque) types will be respected;
	// meaning that changes to opaque underlying type will be
	// flagged as private and the typedef will be flagged private
	// as well, unless the typedef itself has local non-type
	// changes.  In the later case, changes to the typedef will be
	// emitted because the typedef won't inherit the privateness
	// of its underlying type.  So in practise, the typedef
	// remains public for the purpose of change reporting.
	if (has_non_empty_child
	    && has_private_child
	    && !has_non_private_child)
	  {
	    d->add_to_category(PRIVATE_TYPE_CATEGORY);
	    // If a node was suppressed, all the other nodes of its class
	    // of equivalence are suppressed too.
	    diff *canonical_diff = d->get_canonical_diff();
	    if (canonical_diff != d)
	      canonical_diff->add_to_category(PRIVATE_TYPE_CATEGORY);
	  }

	// If the underlying type of a typedef is private and carries
	// changes (that are implicitely suppressed because it's
	// private) then the typedef must be suppressed too, so that
	// those changes to the underlying type are not seen.
	if (is_typedef_diff(d)
	    && !d->has_local_changes()
	    && has_private_child
	    && has_non_empty_child)
	  {
	    d->add_to_category(SUPPRESSED_CATEGORY|PRIVATE_TYPE_CATEGORY);
	    // If a node was suppressed, all the other nodes of its class
	    // of equivalence are suppressed too.
	    diff *canonical_diff = d->get_canonical_diff();
	    if (canonical_diff != d)
	      canonical_diff->add_to_category
		(SUPPRESSED_CATEGORY|PRIVATE_TYPE_CATEGORY);
	  }

	if (const function_decl_diff *fn_diff = is_function_decl_diff(d))
	  if (!(d->has_local_changes() & LOCAL_NON_TYPE_CHANGE_KIND))
	    {
	      // d is a function diff that carries a local *type*
	      // change (that means it's a change to the function
	      // type).  Let's see if the child function type diff
	      // node is suppressed.  That would mean that we are
	      // instructed to show details of a diff that is deemed
	      // suppressed; this means the suppression conflicts with
	      // a local type change.  In that case, let's follow what
	      // the user asked and suppress the function altogether,
	      if (function_type_diff_sptr fn_type_diff = fn_diff->type_diff())
		if (fn_type_diff->is_suppressed())
		  {
		    d->add_to_category(SUPPRESSED_CATEGORY);
		    // If a node was suppressed, all the other nodes
		    // of its class of equivalence are suppressed too.
		    diff *canonical_diff = d->get_canonical_diff();
		    if (canonical_diff != d)
		      canonical_diff->add_to_category(SUPPRESSED_CATEGORY);
		  }
	  }
      }
  }
}; //end struct suppression_categorization_visitor

/// Walk a given diff-sub tree and appply the suppressions carried by
/// the context.  If the suppression applies to a given node than
/// categorize the node into the SUPPRESSED_CATEGORY category and
/// propagate that categorization.
///
/// @param diff_tree the diff-sub tree to apply the suppressions to.
void
apply_suppressions(diff* diff_tree)
{
  if (diff_tree && !diff_tree->context()->suppressions().empty())
    {
      // Apply suppressions to functions and variables that have
      // changed sub-types.
      suppression_categorization_visitor v;
      diff_tree->context()->forget_visited_diffs();
      bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
      diff_tree->context()->forbid_visiting_a_node_twice(true);
      diff_tree->traverse(v);
      diff_tree->context()->forbid_visiting_a_node_twice(s);
    }
}

/// Walk a given diff-sub tree and appply the suppressions carried by
/// the context.  If the suppression applies to a given node than
/// categorize the node into the SUPPRESSED_CATEGORY category and
/// propagate that categorization.
///
/// @param diff_tree the diff-sub tree to apply the suppressions to.
void
apply_suppressions(diff_sptr diff_tree)
{apply_suppressions(diff_tree.get());}

/// Walk a @ref corpus_diff tree and appply the suppressions carried
/// by the context.  If the suppression applies to a given node then
/// categorize the node into the SUPPRESSED_CATEGORY category and
/// propagate that categorization.
///
/// @param diff_tree the diff tree to apply the suppressions to.
void
apply_suppressions(const corpus_diff* diff_tree)
{
  if (diff_tree && !diff_tree->context()->suppressions().empty())
    {
      // First, visit the children trees of changed constructs:
      // changed functions, variables, as well as sub-types of these,
      // and apply suppression specifications to these ...
      suppression_categorization_visitor v;
      diff_tree->context()->forget_visited_diffs();
      bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
      diff_tree->context()->forbid_visiting_a_node_twice(true);
      const_cast<corpus_diff*>(diff_tree)->traverse(v);
      diff_tree->context()->forbid_visiting_a_node_twice(s);

      // ... then also visit the set of added and removed functions,
      // variables, symbols, and types not reachable from global
      // functions and variables.
      diff_tree->priv_->
	apply_supprs_to_added_removed_fns_vars_unreachable_types();
    }
}

/// Walk a diff tree and appply the suppressions carried by the
/// context.  If the suppression applies to a given node than
/// categorize the node into the SUPPRESSED_CATEGORY category and
/// propagate that categorization.
///
/// @param diff_tree the diff tree to apply the suppressions to.
void
apply_suppressions(corpus_diff_sptr  diff_tree)
{apply_suppressions(diff_tree.get());}

// </diff tree category propagation>

// <diff tree printing stuff>

/// A visitor to print (to an output stream) a pretty representation
/// of a @ref diff sub-tree or of a complete @ref corpus_diff tree.
struct diff_node_printer : public diff_node_visitor
{
  ostream& out_;
  unsigned level_;

  /// Emit a certain number of spaces to the output stream associated
  /// to this diff_node_printer.
  ///
  /// @param level half of the numver of spaces to emit.
  void
  do_indent(unsigned level)
  {
    for (unsigned i = 0; i < level; ++i)
      out_ << "  ";
  }

  diff_node_printer(ostream& out)
    : diff_node_visitor(DO_NOT_MARK_VISITED_NODES_AS_VISITED),
      out_(out),
      level_(0)
  {}

  virtual void
  visit_begin(diff*)
  {
    ++level_;
  }

  virtual void
  visit_end(diff*)
  {
    --level_;
  }

  virtual void
  visit_begin(corpus_diff*)
  {
    ++level_;
  }

  virtual void
  visit_end(corpus_diff*)
  {
    --level_;
  }

  virtual bool
  visit(diff* d, bool pre)
  {
    if (!pre)
      // We are post-visiting the diff node D.  Which means, we have
      // printed a pretty representation for it already.  So do
      // nothing now.
      return true;

    do_indent(level_);
    out_ << d->get_pretty_representation();
    out_ << "\n";
    do_indent(level_);
    out_ << "{\n";
    do_indent(level_ + 1);
    out_ << "category: "<< d->get_category() << "\n";
    do_indent(level_ + 1);
    out_ << "@: " << std::hex << d << std::dec << "\n";
    do_indent(level_ + 1);
    out_ << "@-canonical: " << std::hex
	 << d->get_canonical_diff()
	 << std::dec << "\n";
    do_indent(level_);
    out_ << "}\n";

    return true;
  }

  virtual bool
  visit(corpus_diff* d, bool pre)
  {
    if (!pre)
      // We are post-visiting the diff node D.  Which means, we have
      // printed a pretty representation for it already.  So do
      // nothing now.
      return true;

    // indent
    for (unsigned i = 0; i < level_; ++i)
      out_ << ' ';
    out_ << d->get_pretty_representation();
    out_ << '\n';
    return true;
  }
}; // end struct diff_printer_visitor

// </ diff tree printing stuff>

/// Emit a textual representation of a @ref diff sub-tree to an
/// output stream.
///
/// @param diff_tree the sub-tree to emit the textual representation
/// for.
///
/// @param out the output stream to emit the textual representation
/// for @p diff_tree to.
void
print_diff_tree(diff* diff_tree, ostream& out)
{
  diff_node_printer p(out);
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(p);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Emit a textual representation of a @ref corpus_diff tree to an
/// output stream.
///
/// @param diff_tree the @ref corpus_diff tree to emit the textual
/// representation for.
///
/// @param out the output stream to emit the textual representation
/// for @p diff_tree to.
void
print_diff_tree(corpus_diff* diff_tree, std::ostream& out)
{
  diff_node_printer p(out);
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(p);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Emit a textual representation of a @ref diff sub-tree to an
/// output stream.
///
/// @param diff_tree the sub-tree to emit the textual representation
/// for.
///
/// @param out the output stream to emit the textual representation
/// for @p diff_tree to.
void
print_diff_tree(diff_sptr diff_tree,
		std::ostream& o)
{print_diff_tree(diff_tree.get(), o);}

/// Emit a textual representation of a @ref corpus_diff tree to an
/// output stream.
///
/// @param diff_tree the @ref corpus_diff tree to emit the textual
/// representation for.
///
/// @param out the output stream to emit the textual representation
/// for @p diff_tree to.
void
print_diff_tree(corpus_diff_sptr diff_tree,
		std::ostream& o)
{print_diff_tree(diff_tree.get(), o);}

// <redundancy_marking_visitor>

/// A tree visitor to categorize nodes with respect to the
/// REDUNDANT_CATEGORY.  That is, detect if a node is redundant (is
/// present on several spots of the tree) and mark such nodes
/// appropriatly.  This visitor also takes care of propagating the
/// REDUNDANT_CATEGORY of a given node to its parent nodes as
/// appropriate.
struct redundancy_marking_visitor : public diff_node_visitor
{
  bool skip_children_nodes_;

  redundancy_marking_visitor()
    : skip_children_nodes_()
  {}

  virtual void
  visit_begin(diff* d)
  {
    if (d->to_be_reported())
      {
	// A diff node that carries a change and that has been already
	// traversed elsewhere is considered redundant.  So let's mark
	// it as such and let's not traverse it; that is, let's not
	// visit its children.
	if ((d->context()->diff_has_been_visited(d)
	     || d->get_canonical_diff()->is_traversing())
	    && d->has_changes())
	  {
	    // But if two diff nodes are redundant sibbling that carry
	    // changes of base types, do not mark them as being
	    // redundant.  This is to avoid marking nodes as redundant
	    // in this case:
	    //
	    //     int foo(int a, int b);
	    // compared with:
	    //     float foo(float a, float b); (in C).
	    //
	    // In this case, we want to report all the occurences of
	    // the int->float change because logically, they are at
	    // the same level in the diff tree.

	    bool redundant_with_sibling_node = false;
	    const diff* p = d->parent_node();

	    // If this is a child node of a fn_parm_diff, look through
	    // the fn_parm_diff node to get the function diff node.
	    if (p && dynamic_cast<const fn_parm_diff*>(p))
	      p = p->parent_node();

	    if (p)
	      for (vector<diff*>::const_iterator s =
		     p->children_nodes().begin();
		   s != p->children_nodes().end();
		   ++s)
		{
		  if (*s == d)
		    continue;
		  diff* sib = *s;
		  // If this is a fn_parm_diff, look through the
		  // fn_parm_diff node to get at the real type node.
		  if (fn_parm_diff* f = dynamic_cast<fn_parm_diff*>(*s))
		    sib = f->type_diff().get();
		  if (sib == d)
		    continue;
		  if (sib->get_canonical_diff() == d->get_canonical_diff()
		      // Sibbling diff nodes that carry base type
		      // changes ar to be marked as redundant.
		      && (is_base_diff(sib) || is_distinct_diff(sib)))
		    {
		      redundant_with_sibling_node = true;
		      break;
		    }
		}
	    if (!redundant_with_sibling_node
		// Changes to basic types should never be considered
		// redundant.  For instance, if a member of integer
		// type is changed into a char type in both a struct A
		// and a struct B, we want to see both changes.
		&& !has_basic_type_change_only(d)
		// The same goes for distinct type changes
		&& !filtering::is_mostly_distinct_diff(d)
		// Functions with similar *local* changes are never marked
		// redundant because otherwise one could miss important
		// similar local changes that are applied to different
		// functions.
		&& !is_function_type_diff_with_local_changes(d)
		// Changes involving variadic parameters of functions
		// should never be marked redundant because we want to see
		// them all.
		&& !is_diff_of_variadic_parameter(d)
		&& !is_diff_of_variadic_parameter_type(d)
		// If the canonical diff itself has been filtered out,
		// then this one is not marked redundant, unless the
		// canonical diff was already redundant.
		&& (!d->get_canonical_diff()->is_filtered_out()
		    || (d->get_canonical_diff()->get_category()
			& REDUNDANT_CATEGORY))
		// If the *same* diff node (not one that is merely
		// equivalent to this one) has already been visited
		// the do not mark it as beind redundant.  It's only
		// the other nodes that are equivalent to this one
		// that must be marked redundant.
		&& d->context()->diff_has_been_visited(d) != d
		// If the diff node is a function parameter and is not
		// a reference/pointer (to a non basic or a non
		// distinct type diff) then do not mark it as
		// redundant.
		//
		// Children nodes of base class diff nodes are never
		// redundant either, we want to see them all.
		&& (is_reference_or_ptr_diff_to_non_basic_nor_distinct_types(d)
		    || (!is_child_node_of_function_parm_diff(d)
			&& !is_child_node_of_base_diff(d))))
	      {
		d->add_to_category(REDUNDANT_CATEGORY);
		// As we said in preamble, as this node is marked as
		// being redundant, let's not visit its children.
		// This is not an optimization; it's needed for
		// correctness.  In the case of a diff node involving
		// a class type that refers to himself, visiting the
		// children nodes might cause them to be wrongly
		// marked as redundant.
		set_visiting_kind(get_visiting_kind()
				  | SKIP_CHILDREN_VISITING_KIND);
		skip_children_nodes_ = true;
	      }
	  }
      }
    else
      {
	// If the node is not to be reported, do not look at it children.
	set_visiting_kind(get_visiting_kind() | SKIP_CHILDREN_VISITING_KIND);
	skip_children_nodes_ = true;
      }
  }

  virtual void
  visit_begin(corpus_diff*)
  {
  }

  virtual void
  visit_end(diff* d)
  {
    if (skip_children_nodes_)
      // When visiting this node, we decided to skip its children
      // node.  Now that we are done visiting the node, lets stop
      // avoiding the children nodes visiting for the other tree
      // nodes.
      {
	set_visiting_kind(get_visiting_kind() & (~SKIP_CHILDREN_VISITING_KIND));
	skip_children_nodes_ = false;
      }
    else
      {
	// Propagate the redundancy categorization of the children nodes
	// to this node.  But if this node has local changes, then it
	// doesn't inherit redundancy from its children nodes.
	if (!(d->get_category() & REDUNDANT_CATEGORY)
	    && (!d->has_local_changes_to_be_reported()
		// By default, pointer, reference and qualified types
		// consider that a local changes to their underlying
		// type is always a local change for themselves.
		//
		// This is as if those types don't have local changes
		// in the same sense as other types.  So we always
		// propagate redundancy to them, regardless of if they
		// have local changes or not.
		//
		// We also propagate redundancy to typedef types if
		// these /only/ carry changes to their underlying
		// type.
		//
		// Note that changes to the underlying type of a
		// typedef is considered local of
		// LOCAL_TYPE_CHANGE_KIND kind.  The other changes to the
		// typedef itself are considered local of
		// LOCAL_NON_TYPE_CHANGE_KIND kind.
		|| is_pointer_diff(d)
		|| is_qualified_type_diff(d)
		|| (is_typedef_diff(d)
		    && (!(d->has_local_changes()
			  & LOCAL_NON_TYPE_CHANGE_KIND)))))
	  {
	    bool has_non_redundant_child = false;
	    bool has_non_empty_child = false;
	    for (vector<diff*>::const_iterator i =
		   d->children_nodes().begin();
		 i != d->children_nodes().end();
		 ++i)
	      {
		if ((*i)->has_changes())
		  {
		    has_non_empty_child = true;
		    // Let's see if the current child node '*i' is
		    // "non-redundant".
		    //
		    // A non-redundant node would be a node that
		    // carries a change to be reported and has not
		    // been marked as being redundant.
		    if ((*i)->to_be_reported()
			&& ((*i)->get_category() & REDUNDANT_CATEGORY) == 0)
		      has_non_redundant_child = true;
		  }
		if (has_non_redundant_child)
		  break;
	      }

	    // A diff node for which at least a child node carries a
	    // change, and for which all the children are redundant is
	    // deemed redundant too, unless it has local changes.
	    if (has_non_empty_child
		&& !has_non_redundant_child)
	      d->add_to_category(REDUNDANT_CATEGORY);
	  }
      }
  }

  virtual void
  visit_end(corpus_diff*)
  {
  }

  virtual bool
  visit(diff*, bool)
  {return true;}

  virtual bool
  visit(corpus_diff*, bool)
  {
    return true;
  }
};// end struct redundancy_marking_visitor

/// A visitor of @ref diff nodes that clears the REDUNDANT_CATEGORY
/// category out of the nodes.
struct redundancy_clearing_visitor : public diff_node_visitor
{
  bool
  visit(corpus_diff*, bool)
  {return true;}

  bool
  visit(diff* d, bool)
  {
    // clear the REDUNDANT_CATEGORY out of the current node.
    diff_category c = d->get_category();
    c &= ~REDUNDANT_CATEGORY;
    d->set_category(c);
    return true;
  }
}; // end struct redundancy_clearing_visitor

/// Walk a given @ref diff sub-tree to categorize each of the nodes
/// with respect to the REDUNDANT_CATEGORY.
///
/// @param diff_tree the @ref diff sub-tree to walk.
void
categorize_redundancy(diff* diff_tree)
{
  if (diff_tree->context()->show_redundant_changes())
    return;
  redundancy_marking_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Walk a given @ref diff sub-tree to categorize each of the nodes
/// with respect to the REDUNDANT_CATEGORY.
///
/// @param diff_tree the @ref diff sub-tree to walk.
void
categorize_redundancy(diff_sptr diff_tree)
{categorize_redundancy(diff_tree.get());}

/// Walk a given @ref corpus_diff tree to categorize each of the nodes
/// with respect to the REDUNDANT_CATEGORY.
///
/// @param diff_tree the @ref corpus_diff tree to walk.
void
categorize_redundancy(corpus_diff* diff_tree)
{
  redundancy_marking_visitor v;
  diff_tree->context()->forget_visited_diffs();
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Walk a given @ref corpus_diff tree to categorize each of the nodes
/// with respect to the REDUNDANT_CATEGORY.
///
/// @param diff_tree the @ref corpus_diff tree to walk.
void
categorize_redundancy(corpus_diff_sptr diff_tree)
{categorize_redundancy(diff_tree.get());}

// </redundancy_marking_visitor>

/// Walk a given @ref diff sub-tree to clear the REDUNDANT_CATEGORY
/// out of the category of the nodes.
///
/// @param diff_tree the @ref diff sub-tree to walk.
void
clear_redundancy_categorization(diff* diff_tree)
{
  redundancy_clearing_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
  diff_tree->context()->forget_visited_diffs();
}

/// Walk a given @ref diff sub-tree to clear the REDUNDANT_CATEGORY
/// out of the category of the nodes.
///
/// @param diff_tree the @ref diff sub-tree to walk.
void
clear_redundancy_categorization(diff_sptr diff_tree)
{clear_redundancy_categorization(diff_tree.get());}

/// Walk a given @ref corpus_diff tree to clear the REDUNDANT_CATEGORY
/// out of the category of the nodes.
///
/// @param diff_tree the @ref corpus_diff tree to walk.
void
clear_redundancy_categorization(corpus_diff* diff_tree)
{
  redundancy_clearing_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
  diff_tree->context()->forget_visited_diffs();
}

/// Walk a given @ref corpus_diff tree to clear the REDUNDANT_CATEGORY
/// out of the category of the nodes.
///
/// @param diff_tree the @ref corpus_diff tree to walk.
void
clear_redundancy_categorization(corpus_diff_sptr diff_tree)
{clear_redundancy_categorization(diff_tree.get());}

/// Apply the @ref diff tree filters that have been associated to the
/// context of the a given @ref corpus_diff tree.  As a result, the
/// nodes of the @diff tree are going to be categorized into one of
/// several of the categories of @ref diff_category.
///
/// @param diff_tree the @ref corpus_diff instance which @ref diff are
/// to be categorized.
void
apply_filters(corpus_diff_sptr diff_tree)
{
  diff_tree->context()->maybe_apply_filters(diff_tree);
  propagate_categories(diff_tree);
}

/// Test if a diff node represents the difference between a variadic
/// parameter type and something else.
///
/// @param d the diff node to consider.
///
/// @return true iff @p d is a diff node that represents the
/// difference between a variadic parameter type and something else.
bool
is_diff_of_variadic_parameter_type(const diff* d)
{
  if (!d)
    return false;

  type_base_sptr t = is_type(d->first_subject());
  if (t && t->get_environment()->is_variadic_parameter_type(t))
    return true;

  t = is_type(d->second_subject());
  if (t && t->get_environment()->is_variadic_parameter_type(t))
    return true;

  return false;
}

/// Test if a diff node represents the difference between a variadic
/// parameter type and something else.
///
/// @param d the diff node to consider.
///
/// @return true iff @p d is a diff node that represents the
/// difference between a variadic parameter type and something else.
bool
is_diff_of_variadic_parameter_type(const diff_sptr& d)
{return is_diff_of_variadic_parameter_type(d.get());}

/// Test if a diff node represents the difference between a variadic
/// parameter and something else.
///
/// @param d the diff node to consider.
///
/// @return true iff @p d is a diff node that represents the
/// difference between a variadic parameter and something else.
bool
is_diff_of_variadic_parameter(const diff* d)
{
  fn_parm_diff* diff =
    dynamic_cast<fn_parm_diff*>(const_cast<abigail::comparison::diff*>(d));
  return (diff && is_diff_of_variadic_parameter_type(diff->type_diff()));
}

/// Test if a diff node represents the difference between a variadic
/// parameter and something else.
///
/// @param d the diff node to consider.
///
/// @return true iff @p d is a diff node that represents the
/// difference between a variadic parameter and something else.
bool
is_diff_of_variadic_parameter(const diff_sptr& d)
{return is_diff_of_variadic_parameter(d.get());}

/// Test if a diff node represents a diff between two basic types.
///
/// @param d the diff node to consider.
///
/// @return true iff @p d is a diff between two basic types.
const type_decl_diff*
is_diff_of_basic_type(const diff *d)
{return dynamic_cast<const type_decl_diff*>(d);}

/// Test if a diff node represents a diff between two basic types, or
/// between pointers, references or qualified type to basic types.
///
/// @param diff the diff node to consider.
///
/// @param allow_indirect_type if true, then this function looks into
/// pointer, reference or qualified diff types to see if they "point
/// to" basic types.
///
/// @return true iff @p d is a diff between two basic types.
const type_decl_diff*
is_diff_of_basic_type(const diff* diff, bool allow_indirect_type)
{
  if (allow_indirect_type)
      diff = peel_pointer_or_qualified_type_diff(diff);
  return is_diff_of_basic_type(diff);
}

/// If a diff node is about changes between two typedef types, get the
/// diff node about changes between the underlying types.
///
/// Note that this function walks the tree of underlying diff nodes
/// returns the first diff node about types that are not typedefs.
///
/// @param dif the dif node to consider.
///
/// @return the underlying diff node of @p dif, or just return @p dif
/// if it's not a typedef diff node.
const diff*
peel_typedef_diff(const diff* dif)
{
  const typedef_diff *d = 0;
  while ((d = is_typedef_diff(dif)))
    dif = d->underlying_type_diff().get();
  return dif;
}

/// If a diff node is about changes between two pointer types, get the
/// diff node about changes between the underlying (pointed-to) types.
///
/// Note that this function walks the tree of underlying diff nodes
/// returns the first diff node about types that are not pointers.
///
/// @param dif the dif node to consider.
///
/// @return the underlying diff node of @p dif, or just return @p dif
/// if it's not a pointer diff node.
const diff*
peel_pointer_diff(const diff* dif)
{
  const pointer_diff *d = 0;
  while ((d = is_pointer_diff(dif)))
    dif = d->underlying_type_diff().get();
  return dif;
}

/// If a diff node is about changes between two reference types, get
/// the diff node about changes between the underlying (pointed-to)
/// types.
///
/// Note that this function walks the tree of underlying diff nodes
/// returns the first diff node about types that are not references.
///
/// @param dif the dif node to consider.
///
/// @return the underlying diff node of @p dif, or just return @p dif
/// if it's not a reference diff node.
const diff*
peel_reference_diff(const diff* dif)
{
  const reference_diff *d = 0;
  while ((d = is_reference_diff(dif)))
    dif = d->underlying_type_diff().get();
  return dif;
}

/// If a diff node is about changes between two qualified types, get
/// the diff node about changes between the underlying (non-qualified)
/// types.
///
/// Note that this function walks the tree of underlying diff nodes
/// returns the first diff node about types that are not qualified.
///
/// @param dif the dif node to consider.
///
/// @return the underlying diff node of @p dif, or just return @p dif
/// if it's not a qualified diff node.
const diff*
peel_qualified_diff(const diff* dif)
{
  const qualified_type_diff *d = 0;
  while ((d = is_qualified_type_diff(dif)))
    dif = d->underlying_type_diff().get();
  return dif;
}

/// If a diff node is about changes between two pointer, reference or
/// qualified types, get the diff node about changes between the
/// underlying types.
///
/// Note that this function walks the tree of underlying diff nodes
/// returns the first diff node about types that are not pointer,
/// reference or qualified.
///
/// @param dif the dif node to consider.
///
/// @return the underlying diff node of @p dif, or just return @p dif
/// if it's not a pointer, reference or qualified diff node.
const diff*
peel_pointer_or_qualified_type_diff(const diff*dif)
{
  while (true)
    {
      if (const pointer_diff *d = is_pointer_diff(dif))
	dif = peel_pointer_diff(d);
      else if (const reference_diff *d = is_reference_diff(dif))
	dif = peel_reference_diff(d);
      else if (const qualified_type_diff *d = is_qualified_type_diff(dif))
	dif = peel_qualified_diff(d);
      else
	break;
    }
  return dif;
}

/// If a diff node is about changes between two typedefs or qualified
/// types, get the diff node about changes between the underlying
/// types.
///
/// Note that this function walks the tree of underlying diff nodes
/// returns the first diff node about types that are not typedef or
/// qualified types.
///
/// @param dif the dif node to consider.
///
/// @return the underlying diff node of @p dif, or just return @p dif
/// if it's not typedef or qualified diff node.
const diff*
peel_typedef_or_qualified_type_diff(const diff *dif)
{
  while (true)
    {
      if (const typedef_diff *d = is_typedef_diff(dif))
	dif = peel_typedef_diff(d);
      else if (const qualified_type_diff *d = is_qualified_type_diff(dif))
	dif = peel_qualified_diff(d);
      else
	break;
    }
  return dif;
}

/// Test if a diff node represents a diff between two class or union
/// types.
///
/// @param d the diff node to consider.
///
/// @return iff @p is a diff between two class or union types then
/// return the instance of @ref class_or_union_diff that @p derives
/// from.  Otherwise, return nil.
const class_or_union_diff*
is_diff_of_class_or_union_type(const diff *d)
{return dynamic_cast<const class_or_union_diff*>(d);}

/// Test if a given diff node carries *only* a local type change.
///
/// @param d the diff node to consider.
///
/// @return true iff @p has a change and that change is a local type
/// change.
static bool
has_local_type_change_only(const diff *d)
{
  if (enum change_kind k = d->has_local_changes())
    if ((k & LOCAL_NON_TYPE_CHANGE_KIND) == 0
	&& (k & LOCAL_TYPE_CHANGE_KIND) != 0)
      return true;

  return false;
}

/// Test if a diff node is a decl diff that only carries a basic type
/// change on its type diff sub-node.
///
///Note that that pointers/references/qualified types diffs to basic
/// type diffs are considered as having basic type change only.
///
/// @param d the diff node to consider.
///
/// @return true iff @p d is a decl diff that only carries a basic
/// type change on its type diff sub-node.
bool
has_basic_type_change_only(const diff *d)
{
  if (is_diff_of_basic_type(d, true) && d->has_changes())
    return true;
  else if (const var_diff * v = dynamic_cast<const var_diff*>(d))
    return (has_local_type_change_only(v)
	    && is_diff_of_basic_type(v->type_diff().get(), true));
  else if (const fn_parm_diff * p = dynamic_cast<const fn_parm_diff*>(d))
    return (has_local_type_change_only(p)
	    && is_diff_of_basic_type(p->type_diff().get(), true));
  else if (const function_decl_diff* f =
	   dynamic_cast<const function_decl_diff*>(d))
    return (has_local_type_change_only(f)
	    && f->type_diff()
	    && is_diff_of_basic_type(f->type_diff()->return_type_diff().get(),
				     true));
  return false;
}
}// end namespace comparison
} // end namespace abigail

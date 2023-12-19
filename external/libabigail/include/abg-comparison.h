// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

#ifndef __ABG_COMPARISON_H__
#define __ABG_COMPARISON_H__

/// @file

#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include "abg-corpus.h"
#include "abg-diff-utils.h"
#include "abg-reporter.h"

namespace abigail
{

/// @brief utilities to compare abi artifacts
///
/// The main entry points of the namespace are the compute_diff()
/// overloads used to compute the difference between two abi artifacts.
namespace comparison
{

namespace filtering
{
struct filter_base;
typedef shared_ptr<filter_base> filter_base_sptr;
typedef std::vector<filter_base_sptr> filters;
}

// Inject types we need into this namespace.
using std::ostream;
using std::vector;
using std::unordered_map;
using std::unordered_set;
using std::pair;

using diff_utils::insertion;
using diff_utils::deletion;
using diff_utils::edit_script;

class diff;

/// Convenience typedef for a shared_ptr for the @ref diff class
typedef shared_ptr<diff> diff_sptr;

/// Convenience typedef for a weak_ptr for the @ref diff class
typedef weak_ptr<diff> diff_wptr;

/// Hasher for @ref diff_sptr.
struct diff_sptr_hasher
{
  /// The actual hashing functor.
  size_t
  operator()(const diff_sptr& t) const
  {return reinterpret_cast<size_t>(t.get());}
}; // end struct diff_sptr_hasher

/// Convenience typedef for a vector of @ref diff_sptr.
typedef vector<diff_sptr> diff_sptrs_type;

/// Convenience typedef for a vector of @ref diff*.
typedef vector<diff*> diff_ptrs_type;

/// Convenience typedef for an unoredered set of @ref diff_sptr
typedef unordered_set<diff_sptr, diff_sptr_hasher> unordered_diff_sptr_set;

class decl_diff_base;

/// Convenience typedef for a shared_ptr of @ref decl_diff_base.
typedef shared_ptr<decl_diff_base> decl_diff_base_sptr;

/// Convenience typedef for a vector of @ref decl_diff_base_sptr.
typedef vector<decl_diff_base_sptr> decl_diff_base_sptrs_type;

class type_diff_base;
/// Convenience pointer for a shared pointer to a type_diff_base
typedef shared_ptr<type_diff_base> type_diff_base_sptr;

/// Convenience typedef for a vector of @ref type_diff_base_sptr
typedef vector<type_diff_base_sptr> type_diff_base_sptrs_type;

class function_decl_diff;

/// Convenience typedef for a shared pointer to a @ref function_decl type.
typedef shared_ptr<function_decl_diff> function_decl_diff_sptr;

/// Convenience typedef for a vector of @ref function_decl_diff_sptr
typedef vector<function_decl_diff_sptr> function_decl_diff_sptrs_type;

class fn_parm_diff;

/// Convenience typedef for a shared pointer to a @ref fn_parm_diff
/// type.
typedef shared_ptr<fn_parm_diff> fn_parm_diff_sptr;

class var_diff;

/// Convenience typedef for a shared pointer to a @ref var_diff type.
typedef shared_ptr<var_diff> var_diff_sptr;

/// Convenience typedef for a vector of @ref var_diff_sptr.
typedef vector<var_diff_sptr> var_diff_sptrs_type;

class base_diff;

/// Convenience typedef for a shared pointer to a @ref base_diff type.
typedef shared_ptr<base_diff> base_diff_sptr;

/// Convenience typedef for a vector of @ref base_diff_sptr.
typedef vector<base_diff_sptr> base_diff_sptrs_type;

class class_diff;

/// Convenience typedef for a shared pointer on a @ref class_diff type.
typedef shared_ptr<class_diff> class_diff_sptr;

/// Convenience typedef for a map of pointer values.  The Key is a
/// pointer value and the value is potentially another pointer value
/// associated to the first one.
typedef unordered_map<size_t, size_t> pointer_map;

/// Convenience typedef for a map which key is a string and which
/// value is a @ref decl_base_sptr.
typedef unordered_map<string, decl_base_sptr> string_decl_base_sptr_map;

/// Convenience typedef for a map which key is a string and which
/// value is a @ref type_base_sptr.
typedef unordered_map<string, type_base_sptr> string_type_base_sptr_map;

/// Convenience typedef for a map which key is an unsigned integer and
/// which value is a @ref decl_base_sptr
typedef unordered_map<unsigned, decl_base_sptr> unsigned_decl_base_sptr_map;

/// Convenience typedef for a map of string and class_decl::basse_spec_sptr.
typedef unordered_map<string, class_decl::base_spec_sptr> string_base_sptr_map;

/// Convenience typedef for a map of string and @ref base_diff_sptr.
typedef unordered_map<string, base_diff_sptr> string_base_diff_sptr_map;

/// Convenience typedef for a map which value is a changed function
/// parameter and which key is the name of the function parameter.
typedef unordered_map<string, fn_parm_diff_sptr> string_fn_parm_diff_sptr_map;

/// Convenience typedef for a map which key is an integer and which
/// value is a changed parameter.
typedef unordered_map<unsigned, fn_parm_diff_sptr>
unsigned_fn_parm_diff_sptr_map;

/// Convenience typedef for a map which key is an integer and which
/// value is a parameter.
typedef unordered_map<unsigned,
		      function_decl::parameter_sptr> unsigned_parm_map;

/// Convenience typedef for a map which value is a
/// type_diff_base_sptr.  The key of the map is the qualified name of
/// the changed type.
typedef unordered_map<string,
		      type_diff_base_sptr> string_type_diff_base_sptr_map;

/// Convenience typedef for a map which value is a
/// decl_diff_base_sptr.  The key of the map is the qualified name of
/// the changed type.
typedef unordered_map<string,
		      decl_diff_base_sptr> string_decl_diff_base_sptr_map;

/// Convenience typedef for a map which value is a diff_sptr.  The key
/// of the map is the qualified name of the changed type.
typedef unordered_map<string, diff_sptr> string_diff_sptr_map;

/// Convenience typedef for a map which value is a diff*.  The key of
/// the map is the qualified name of the changed type.
typedef unordered_map<string, diff*> string_diff_ptr_map;

/// Convenience typedef for a map whose key is a string and whose
/// value is a changed variable of type @ref var_diff_sptr.
typedef unordered_map<string,
		      var_diff_sptr> string_var_diff_sptr_map;


/// Convenience typedef for a map whose key is an unsigned int and
/// whose value is a changed variable of type @ref var_diff_sptr.
typedef unordered_map<unsigned, var_diff_sptr> unsigned_var_diff_sptr_map;

/// Convenience typedef for a map which value is a function
/// parameter.  The key is the name of the function parm.
typedef unordered_map<string, function_decl::parameter_sptr> string_parm_map;

/// Convenience typedef for a map which value is an enumerator.  The
/// key is the name of the enumerator.
typedef unordered_map<string, enum_type_decl::enumerator> string_enumerator_map;

/// Convenience typedef for a changed enumerator.  The first element
/// of the pair is the old enumerator and the second one is the new enumerator.
typedef std::pair<enum_type_decl::enumerator,
		  enum_type_decl::enumerator> changed_enumerator;

/// Convenience typedef for a vector of changed enumerators.
typedef vector<changed_enumerator> changed_enumerators_type;

/// Convenience typedef for a map which value is a changed enumerator.
/// The key is the name of the changed enumerator.
typedef unordered_map<string, changed_enumerator> string_changed_enumerator_map;

/// Convenience typedef for a map which key is a string and which
/// value is a pointer to @ref decl_base.
typedef unordered_map<string, function_decl*> string_function_ptr_map;

/// Convenience typedef for a map which key is a string and which
/// value is a @ref function_decl_diff_sptr.
typedef unordered_map<string,
		      function_decl_diff_sptr>
				string_function_decl_diff_sptr_map;

/// Convenience typedef for a pair of class_decl::member_function_sptr
/// representing a changed member function.  The first element of the
/// pair is the initial member function and the second element is the
/// changed one.
typedef pair<method_decl_sptr,
	     method_decl_sptr> changed_member_function_sptr;

/// Convenience typedef for a hash map of strings and changed member functions.
typedef unordered_map<string,
		      changed_member_function_sptr>
				string_changed_member_function_sptr_map;

/// Convenience typedef for a hash map of strings  and member functions.
typedef unordered_map<string, method_decl_sptr> string_member_function_sptr_map;

/// Convenience typedef for a map which key is a string and which
/// value is a point to @ref var_decl.
typedef unordered_map<string, var_decl*> string_var_ptr_map;

/// Convenience typedef for a pair of pointer to @ref var_decl
/// representing a @ref var_decl change.  The first member of the pair
/// represents the initial variable and the second member represents
/// the changed variable.
typedef std::pair<var_decl*, var_decl*> changed_var_ptr;

/// Convenience typedef for a pair of @ref var_decl_sptr representing
/// a @ref var_decl change.  The first member of the pair represents
/// the initial variable and the second member represents the changed
/// variable.
typedef std::pair<var_decl_sptr, var_decl_sptr> changed_var_sptr;

/// Convenience typedef for a vector of @changed_var_sptr.gg381
typedef vector<changed_var_sptr> changed_var_sptrs_type;

/// Convenience typedef for a map whose key is a string and whose
/// value is an @ref elf_symbol_sptr.
typedef unordered_map<string, elf_symbol_sptr> string_elf_symbol_map;

/// Convenience typedef for a map which key is a string and which
/// value is a @ref var_diff_sptr.
typedef unordered_map<string, var_diff_sptr> string_var_diff_ptr_map;

class diff_context;

/// Convenience typedef for a shared pointer of @ref diff_context.
typedef shared_ptr<diff_context> diff_context_sptr;

/// Convenience typedef for a weak pointer of @ref diff_context.
typedef weak_ptr<diff_context> diff_context_wptr;

class diff_node_visitor;

class diff_traversable_base;

/// Convenience typedef for shared_ptr on diff_traversable_base.
typedef shared_ptr<diff_traversable_base> diff_traversable_base_sptr;

/// An enum for the different ways to visit a diff tree node.
///
/// This is used by the node traversing code, to know when to avoid
/// visiting children nodes, for instance.
enum visiting_kind
{
  /// The default enumerator value of this enum.  It doesn't have any
  /// particular meaning yet.
  DEFAULT_VISITING_KIND = 0,

  /// This says that the traversing code should avoid visiting the
  /// children nodes of the current node being visited.
  SKIP_CHILDREN_VISITING_KIND = 1,

  /// This says that the traversing code should not mark visited nodes
  /// as having been traversed.  This is useful, for instance, for
  /// visitors which have debugging purposes.
  DO_NOT_MARK_VISITED_NODES_AS_VISITED = 1 << 1
};

visiting_kind
operator|(visiting_kind l, visiting_kind r);

visiting_kind
operator&(visiting_kind l, visiting_kind r);

visiting_kind
operator~(visiting_kind l);

///  The base class for the diff classes that are to be traversed.
class diff_traversable_base : public traversable_base
{
public:
  virtual bool
  traverse(diff_node_visitor& v);
}; // end struct diff_traversable_base

/// An enum for the different categories that a diff tree node falls
/// into, regarding the kind of changes it represents.
///
/// Note that if you add an enumerator to this enum, you need to
/// update a few spots accordingly:
///
///   * update the ACCESS_CHANGE_CATEGORY enumerator (which is the
///     last enumerator of this enum by OR-ing its initializer with
///     the new enumerator.
///
///   * update the categorize_harmless_diff_node or
///     categorize_harmful_diff_node function depending on if the new
///     enumerator classifies diff nodes as harmless or harmful.
///
///   * update the get_default_harmless_categories_bitmap or
///    get_default_harmful_categories_bitmap function as well, just
///    like above.
///
///   * update the "operator<<(ostream& o, diff_category c)" streaming
///     operator so that it can stream the new enumerator to a textual
///     output stream.
enum diff_category
{
  /// This means the diff node does not carry any (meaningful) change,
  /// or that it carries changes that have not yet been categorized.
  NO_CHANGE_CATEGORY = 0,

  /// This means the diff node (or at least one of its descendant
  /// nodes) carries access related changes, e.g, a private member
  /// that becomes public.
  ACCESS_CHANGE_CATEGORY = 1,

  /// This means the diff node (or at least one of its descendant
  /// nodes) carries a change involving two compatible types.  For
  /// instance a type and its typedefs.
  COMPATIBLE_TYPE_CHANGE_CATEGORY = 1 << 1,

  /// This means that a diff node in the sub-tree carries a harmless
  /// declaration name change.  This is set only for name changes for
  /// data members and typedefs.
  HARMLESS_DECL_NAME_CHANGE_CATEGORY = 1 << 2,

  /// This means that a diff node in the sub-tree carries an addition
  /// or removal of a non-virtual member function.
  NON_VIRT_MEM_FUN_CHANGE_CATEGORY = 1 << 3,

  /// This means that a diff node in the sub-tree carries an addition
  /// or removal of a static data member.
  STATIC_DATA_MEMBER_CHANGE_CATEGORY = 1 << 4,

  /// This means that a diff node in the sub-tree carries an addition
  /// of enumerator to an enum type.
  HARMLESS_ENUM_CHANGE_CATEGORY = 1 << 5,

  /// This means that a diff node in the sub-tree carries an a symbol
  /// alias change that is harmless.
  HARMLESS_SYMBOL_ALIAS_CHANGE_CATEGORY = 1 << 6,

  /// This means that a diff node in the sub-tree carries a harmless
  /// union change.
  HARMLESS_UNION_CHANGE_CATEGORY = 1 << 7,

  /// This means that a diff node in the sub-tree carries a harmless
  /// data member change.  An example of harmless data member change
  /// is an anonymous data member that replaces a given data member
  /// without locally changing the layout.
  HARMLESS_DATA_MEMBER_CHANGE_CATEGORY = 1 << 8,

  /// This means that a diff node was marked as suppressed by a
  /// user-provided suppression specification.
  SUPPRESSED_CATEGORY = 1 << 9,

  /// This means that a diff node was warked as being for a private
  /// type.  That is, the diff node is meant to be suppressed by a
  /// suppression specification that was auto-generated to filter out
  /// changes to private types.
  PRIVATE_TYPE_CATEGORY = 1 << 10,

  /// This means the diff node (or at least one of its descendant
  /// nodes) carries a change that modifies the size of a type or an
  /// offset of a type member.  Removal or changes of enumerators in a
  /// enum fall in this category too.
  SIZE_OR_OFFSET_CHANGE_CATEGORY = 1 << 11,

  /// This means that a diff node in the sub-tree carries an
  /// incompatible change to a vtable.
  VIRTUAL_MEMBER_CHANGE_CATEGORY = 1 << 12,

  /// A diff node in this category is redundant.  That means it's
  /// present as a child of a other nodes in the diff tree.
  REDUNDANT_CATEGORY = 1 << 13,

  /// This means that a diff node in the sub-tree carries a type that
  /// was declaration-only and that is now defined, or vice versa.
  TYPE_DECL_ONLY_DEF_CHANGE_CATEGORY = 1 << 14,

  /// A diff node in this category is a function parameter type which
  /// top cv-qualifiers change.
  FN_PARM_TYPE_TOP_CV_CHANGE_CATEGORY = 1 << 15,

  /// A diff node in this category has a function parameter type with a
  /// cv-qualifiers change.
  FN_PARM_TYPE_CV_CHANGE_CATEGORY = 1 << 16,

  /// A diff node in this category is a function return type with a
  /// cv-qualifier change.
  FN_RETURN_TYPE_CV_CHANGE_CATEGORY = 1 << 17,

  /// A diff node in this category is a function (or function type)
  /// with at least one parameter added or removed.
  FN_PARM_ADD_REMOVE_CHANGE_CATEGORY = 1 << 18,

  /// A diff node in this category is for a variable which type holds
  /// a cv-qualifier change.
  VAR_TYPE_CV_CHANGE_CATEGORY = 1 << 19,

  /// A diff node in this category carries a change from void pointer
  /// to non-void pointer.
  VOID_PTR_TO_PTR_CHANGE_CATEGORY = 1 << 20,

  /// A diff node in this category carries a change in the size of the
  /// array type of a global variable, but the ELF size of the
  /// variable didn't change.
  BENIGN_INFINITE_ARRAY_CHANGE_CATEGORY = 1 << 21,

  /// A special enumerator that is the logical 'or' all the
  /// enumerators above.
  ///
  /// This one must stay the last enumerator.  Please update it each
  /// time you add a new enumerator above.
  EVERYTHING_CATEGORY =
  ACCESS_CHANGE_CATEGORY
  | COMPATIBLE_TYPE_CHANGE_CATEGORY
  | HARMLESS_DECL_NAME_CHANGE_CATEGORY
  | NON_VIRT_MEM_FUN_CHANGE_CATEGORY
  | STATIC_DATA_MEMBER_CHANGE_CATEGORY
  | HARMLESS_ENUM_CHANGE_CATEGORY
  | HARMLESS_SYMBOL_ALIAS_CHANGE_CATEGORY
  | HARMLESS_UNION_CHANGE_CATEGORY
  | HARMLESS_DATA_MEMBER_CHANGE_CATEGORY
  | SUPPRESSED_CATEGORY
  | PRIVATE_TYPE_CATEGORY
  | SIZE_OR_OFFSET_CHANGE_CATEGORY
  | VIRTUAL_MEMBER_CHANGE_CATEGORY
  | REDUNDANT_CATEGORY
  | TYPE_DECL_ONLY_DEF_CHANGE_CATEGORY
  | FN_PARM_TYPE_TOP_CV_CHANGE_CATEGORY
  | FN_PARM_TYPE_CV_CHANGE_CATEGORY
  | FN_RETURN_TYPE_CV_CHANGE_CATEGORY
  | FN_PARM_ADD_REMOVE_CHANGE_CATEGORY
  | VAR_TYPE_CV_CHANGE_CATEGORY
  | VOID_PTR_TO_PTR_CHANGE_CATEGORY
  | BENIGN_INFINITE_ARRAY_CHANGE_CATEGORY
}; // enum diff_category

diff_category
operator|(diff_category c1, diff_category c2);

diff_category&
operator|=(diff_category& c1, diff_category c2);

diff_category&
operator&=(diff_category& c1, diff_category c2);

diff_category
operator^(diff_category c1, diff_category c2);

diff_category
operator&(diff_category c1, diff_category c2);

diff_category
operator~(diff_category c);

diff_category
get_default_harmless_categories_bitmap();

diff_category
get_default_harmful_categories_bitmap();

ostream&
operator<<(ostream& o, diff_category);

class corpus_diff;

/// This type contains maps.  Each map associates a type name to a
/// diff of that type. Not all kinds of diffs are present; only those
/// that carry leaf changes are, for now.
class diff_maps
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

public:

  diff_maps();

  const string_diff_ptr_map&
  get_type_decl_diff_map() const;

  string_diff_ptr_map&
  get_type_decl_diff_map();

  const string_diff_ptr_map&
  get_enum_diff_map() const;

  string_diff_ptr_map&
  get_enum_diff_map();

  const string_diff_ptr_map&
  get_class_diff_map() const;

  string_diff_ptr_map&
  get_class_diff_map();

  const string_diff_ptr_map&
  get_union_diff_map() const;

  string_diff_ptr_map&
  get_union_diff_map();

  const string_diff_ptr_map&
  get_typedef_diff_map() const;

  string_diff_ptr_map&
  get_typedef_diff_map();

  const string_diff_ptr_map&
  get_array_diff_map() const;

  string_diff_ptr_map&
  get_array_diff_map();

  const string_diff_ptr_map&
  get_reference_diff_map() const;

  string_diff_ptr_map&
  get_reference_diff_map();

  const string_diff_ptr_map&
  get_fn_parm_diff_map() const;

  string_diff_ptr_map&
  get_fn_parm_diff_map();

  const string_diff_ptr_map&
  get_function_type_diff_map() const;

  string_diff_ptr_map&
  get_function_type_diff_map();

  const string_diff_ptr_map&
  get_function_decl_diff_map() const;

  string_diff_ptr_map&
  get_function_decl_diff_map();

  const string_diff_ptr_map&
  get_var_decl_diff_map() const;

  string_diff_ptr_map&
  get_var_decl_diff_map();

  const string_diff_ptr_map&
  get_distinct_diff_map() const;

  string_diff_ptr_map&
  get_distinct_diff_map();

  bool
  insert_diff_node(const diff *d,
		   const type_or_decl_base_sptr& impacted_iface);

  artifact_sptr_set_type*
  lookup_impacted_interfaces(const diff *d) const;
}; // end class diff_maps

/// A convenience typedef for a shared pointer to @ref corpus_diff.
typedef shared_ptr<corpus_diff> corpus_diff_sptr;

/// The context of the diff.  This type holds various bits of
/// information that is going to be used throughout the diffing of two
/// entities and the reporting that follows.
class diff_context
{
  struct priv;
  shared_ptr<priv> priv_;

  diff_sptr
  has_diff_for(const type_or_decl_base_sptr first,
	       const type_or_decl_base_sptr second) const;

  diff_sptr
  has_diff_for_types(const type_base_sptr first,
		     const type_base_sptr second) const;

  const diff*
  has_diff_for(const diff* d) const;

  diff_sptr
  has_diff_for(const diff_sptr d) const;

  void
  add_diff(const type_or_decl_base_sptr first,
	   const type_or_decl_base_sptr second,
	   const diff_sptr d);

  void
  add_diff(const diff_sptr d);

  void
  add_diff(const diff* d);

  void
  set_canonical_diff_for(const type_or_decl_base_sptr first,
			 const type_or_decl_base_sptr second,
			 const diff_sptr);

  diff_sptr
  set_or_get_canonical_diff_for(const type_or_decl_base_sptr first,
				const type_or_decl_base_sptr second,
				const diff_sptr canonical_diff);

public:
  diff_context();

  void
  set_corpus_diff(const corpus_diff_sptr&);

  const corpus_diff_sptr&
  get_corpus_diff() const;

  corpus_sptr
  get_first_corpus() const;

  corpus_sptr
  get_second_corpus() const;

  reporter_base_sptr
  get_reporter() const;

  void
  set_reporter(reporter_base_sptr&);

  diff_sptr
  get_canonical_diff_for(const type_or_decl_base_sptr first,
			 const type_or_decl_base_sptr second) const;

  diff_sptr
  get_canonical_diff_for(const diff_sptr d) const;

  void
  initialize_canonical_diff(const diff_sptr diff);

  void
  keep_diff_alive(diff_sptr&);

  diff*
  diff_has_been_visited(const diff*) const;

  diff_sptr
  diff_has_been_visited(const diff_sptr) const;

  void
  mark_diff_as_visited(const diff*);

  void
  forget_visited_diffs();

  void
  mark_last_diff_visited_per_class_of_equivalence(const diff*);

  void
  clear_last_diffs_visited_per_class_of_equivalence();

  const diff*
  get_last_visited_diff_of_class_of_equivalence(const diff*);

  void
  forbid_visiting_a_node_twice(bool f);

  bool
  visiting_a_node_twice_is_forbidden() const;

  void
  forbid_visiting_a_node_twice_per_interface(bool);

  bool
  visiting_a_node_twice_is_forbidden_per_interface() const;

  diff_category
  get_allowed_category() const;

  void
  set_allowed_category(diff_category c);

  void
  switch_categories_on(diff_category c);

  void
  switch_categories_off(diff_category c);

  const filtering::filters&
  diff_filters() const;

  void
  add_diff_filter(filtering::filter_base_sptr);

  void
  maybe_apply_filters(diff_sptr diff);

  void
  maybe_apply_filters(corpus_diff_sptr diff);

  suppr::suppressions_type&
  suppressions() const;

  void
  add_suppression(const suppr::suppression_sptr suppr);

  void
  add_suppressions(const suppr::suppressions_type& supprs);

  void
  show_leaf_changes_only(bool f);

  bool
  show_leaf_changes_only() const;

  bool
  show_hex_values() const;

  void
  show_hex_values(bool f);

  bool
  show_offsets_sizes_in_bits() const;

  void
  show_offsets_sizes_in_bits(bool f);

  void
  show_relative_offset_changes(bool f);

  bool
  show_relative_offset_changes(void);

  void
  show_stats_only(bool f);

  bool
  show_stats_only() const;

  void
  show_soname_change(bool f);

  bool
  show_soname_change() const;

  void
  show_architecture_change(bool f);

  bool
  show_architecture_change() const;

  void
  show_deleted_fns(bool f);

  bool
  show_deleted_fns() const;

  void
  show_changed_fns(bool f);

  bool
  show_changed_fns() const;

  void
  show_added_fns(bool f);

  bool
  show_added_fns() const;

  void
  show_deleted_vars(bool f);

  bool
  show_deleted_vars() const;

  void
  show_changed_vars(bool f);

  bool
  show_changed_vars() const;

  void
  show_added_vars(bool f);

  bool
  show_added_vars() const;

  bool
  show_linkage_names() const;

  void
  show_linkage_names(bool f);

  bool
  show_locs() const;

  void
  show_locs(bool f);

  bool
  show_redundant_changes() const;

  void
  show_redundant_changes(bool f);

  bool
  flag_indirect_changes() const;

  void
  flag_indirect_changes(bool f);

  bool
  show_symbols_unreferenced_by_debug_info() const;

  void
  show_symbols_unreferenced_by_debug_info(bool f);

  bool
  show_added_symbols_unreferenced_by_debug_info() const;

  void
  show_added_symbols_unreferenced_by_debug_info(bool f);

  void show_unreachable_types(bool f);

  bool show_unreachable_types();

  bool
  show_impacted_interfaces() const;

  void
  show_impacted_interfaces(bool f);

  void
  default_output_stream(ostream*);

  ostream*
  default_output_stream();

  void
  error_output_stream(ostream*);

  ostream*
  error_output_stream() const;

  bool
  dump_diff_tree() const;

  void
  dump_diff_tree(bool f);

  void
  do_dump_diff_tree(const diff_sptr) const;

  void
  do_dump_diff_tree(const corpus_diff_sptr) const;

  friend class_diff_sptr
  compute_diff(const class_decl_sptr	first,
	       const class_decl_sptr	second,
	       diff_context_sptr	ctxt);
};//end struct diff_context.

/// The abstraction of a change between two ABI artifacts.
///
/// Please read more about the @ref DiffNode "IR" of the comparison
/// engine to learn more about this.
///
/// This type encapsulates an edit script (a set of insertions and
/// deletions) for two constructs that are to be diff'ed.  The two
/// constructs are called the "subjects" of the diff.
class diff : public diff_traversable_base
{
  friend class diff_context;

  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  // Forbidden
  diff();

protected:
  priv_sptr priv_;

  diff(type_or_decl_base_sptr first_subject,
       type_or_decl_base_sptr second_subject);

  diff(type_or_decl_base_sptr	first_subject,
       type_or_decl_base_sptr	second_subject,
       diff_context_sptr	ctxt);

  void
  begin_traversing();

  void
  end_traversing();

  virtual void
  finish_diff_type();

  void
  set_canonical_diff(diff *);

public:
  type_or_decl_base_sptr
  first_subject() const;

  type_or_decl_base_sptr
  second_subject() const;

  const vector<diff*>&
  children_nodes() const;

  const diff*
  parent_node() const;

  diff* get_canonical_diff() const;

  bool
  is_traversing() const;

  void
  append_child_node(diff_sptr);

  const diff_context_sptr
  context() const;

  void
  context(diff_context_sptr c);

  bool
  currently_reporting() const;

  void
  currently_reporting(bool f) const;

  bool
  reported_once() const;

  void
  reported_once(bool f) const;

  diff_category
  get_category() const;

  diff_category
  get_local_category() const;

  diff_category
  get_class_of_equiv_category() const;

  diff_category
  add_to_category(diff_category c);

  diff_category
  add_to_local_category(diff_category c);

  void
  add_to_local_and_inherited_categories(diff_category c);

  diff_category
  remove_from_category(diff_category c);

  diff_category
  remove_from_local_category(diff_category c);

  void
  set_category(diff_category c);

  void
  set_local_category(diff_category c);

  bool
  is_filtered_out() const;

  bool
  is_filtered_out_wrt_non_inherited_categories() const;

  bool
  is_suppressed() const;

  bool
  is_suppressed(bool &is_private_type) const;

  bool
  to_be_reported() const;

  bool
  has_local_changes_to_be_reported() const;

  virtual const string&
  get_pretty_representation() const;

  virtual void
  chain_into_hierarchy();

  /// Pure interface to get the length of the changes encapsulated by
  /// this diff.  A length of zero means that the current instance of
  /// @ref diff doesn't carry any change.
  ///
  /// This is to be implemented by all descendants of this type.
  virtual bool
  has_changes() const = 0;

  /// Pure interface to know if the current instance of @diff carries
  /// a local change.  A local change is a change that is on the @ref
  /// diff object itself, as opposed to a change that is carried by
  /// some of its children nodes.
  ///
  /// This is to be implemented by all descendants of this type.
  virtual enum change_kind
  has_local_changes() const = 0;

  /// Pure interface to report the diff in a serialized form that is
  /// legible for the user.
  ///
  /// Note that the serializd report has to leave one empty line at
  /// the end of its content.
  ///
  /// @param out the output stream to serialize the report to.
  ///
  /// @param indent the indentation string to use.
  virtual void
  report(ostream& out, const string& indent = "") const = 0;

  virtual bool
  traverse(diff_node_visitor& v);
};// end class diff

diff_sptr
compute_diff(const decl_base_sptr,
	     const decl_base_sptr,
	     diff_context_sptr ctxt);

diff_sptr
compute_diff(const type_base_sptr,
	     const type_base_sptr,
	     diff_context_sptr ctxt);

/// The base class of diff between types.
class type_diff_base : public diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  type_diff_base();

protected:
  type_diff_base(type_base_sptr	first_subject,
		 type_base_sptr	second_subject,
		 diff_context_sptr	ctxt);

public:

  virtual enum change_kind
  has_local_changes() const = 0;

  virtual ~type_diff_base();
};// end class type_diff_base

/// The base class of diff between decls.
class decl_diff_base : public diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

protected:
  decl_diff_base(decl_base_sptr	first_subject,
		 decl_base_sptr	second_subject,
		 diff_context_sptr	ctxt);

public:

  virtual enum change_kind
  has_local_changes() const = 0;

  virtual ~decl_diff_base();
};// end class decl_diff_base

string
get_pretty_representation(diff*);

class distinct_diff;

/// Convenience typedef for a shared pointer to distinct_types_diff
typedef shared_ptr<distinct_diff> distinct_diff_sptr;

/// An abstraction of a diff between entities that are of a different
/// kind (disctinct).
class distinct_diff : public diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  distinct_diff(type_or_decl_base_sptr first,
		type_or_decl_base_sptr second,
		diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:

  const type_or_decl_base_sptr
  first() const;

  const type_or_decl_base_sptr
  second() const;

  const diff_sptr
  compatible_child_diff() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  static bool
  entities_are_of_distinct_kinds(type_or_decl_base_sptr first,
				 type_or_decl_base_sptr second);

  friend distinct_diff_sptr
  compute_diff_for_distinct_kinds(const type_or_decl_base_sptr first,
				  const type_or_decl_base_sptr second,
				  diff_context_sptr ctxt);
};// end class distinct_types_diff

distinct_diff_sptr
compute_diff_for_distinct_kinds(const type_or_decl_base_sptr,
				const type_or_decl_base_sptr,
				diff_context_sptr ctxt);

/// Abstracts a diff between two instances of @ref var_decl
class var_diff : public decl_diff_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  var_diff(var_decl_sptr first,
	   var_decl_sptr second,
	   diff_sptr type_diff,
	   diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  var_decl_sptr
  first_var() const;

  var_decl_sptr
  second_var() const;

  diff_sptr
  type_diff() const;

  virtual void
  chain_into_hierarchy();

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual const string&
  get_pretty_representation() const;

  friend var_diff_sptr
  compute_diff(const var_decl_sptr	first,
	       const var_decl_sptr	second,
	       diff_context_sptr	ctxt);
};// end class var_diff

var_diff_sptr
compute_diff(const var_decl_sptr, const var_decl_sptr, diff_context_sptr);

class pointer_diff;
/// Convenience typedef for a shared pointer on a @ref
/// pointer_diff type.
typedef shared_ptr<pointer_diff> pointer_diff_sptr;

/// The abstraction of a diff between two pointers.
class pointer_diff : public type_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  pointer_diff(pointer_type_def_sptr	first,
	       pointer_type_def_sptr	second,
	       diff_sptr		underlying_type_diff,
	       diff_context_sptr	ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  const pointer_type_def_sptr
  first_pointer() const;

  const pointer_type_def_sptr
  second_pointer() const;

  diff_sptr
  underlying_type_diff() const;

  void
  underlying_type_diff(const diff_sptr);

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend pointer_diff_sptr
  compute_diff(pointer_type_def_sptr	first,
	       pointer_type_def_sptr	second,
	       diff_context_sptr	ctxt);
};// end class pointer_diff

pointer_diff_sptr
compute_diff(pointer_type_def_sptr first,
	     pointer_type_def_sptr second,
	     diff_context_sptr ctxt);

class reference_diff;

/// Convenience typedef for a shared pointer on a @ref
/// reference_diff type.
typedef shared_ptr<reference_diff> reference_diff_sptr;

/// The abstraction of a diff between two references.
class reference_diff : public type_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  reference_diff(const reference_type_def_sptr	first,
		 const reference_type_def_sptr	second,
		 diff_sptr			underlying,
		 diff_context_sptr		ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  reference_type_def_sptr
  first_reference() const;

  reference_type_def_sptr
  second_reference() const;

  const diff_sptr&
  underlying_type_diff() const;

  diff_sptr&
  underlying_type_diff(diff_sptr);

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend reference_diff_sptr
  compute_diff(reference_type_def_sptr first,
	       reference_type_def_sptr second,
	       diff_context_sptr ctxt);
};// end class reference_diff

reference_diff_sptr
compute_diff(reference_type_def_sptr first,
	     reference_type_def_sptr second,
	     diff_context_sptr ctxt);

class array_diff;

/// Convenience typedef for a shared pointer on a @ref
/// array_diff type.
typedef shared_ptr<array_diff> array_diff_sptr;

/// The abstraction of a diff between two arrays.
class array_diff : public type_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  array_diff(const array_type_def_sptr	first,
	     const array_type_def_sptr	second,
	     diff_sptr			element_type_diff,
	     diff_context_sptr		ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  const array_type_def_sptr
  first_array() const;

  const array_type_def_sptr
  second_array() const;

  const diff_sptr&
  element_type_diff() const;

  void
  element_type_diff(diff_sptr);

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend array_diff_sptr
  compute_diff(array_type_def_sptr first,
	       array_type_def_sptr second,
	       diff_context_sptr ctxt);
};// end class array_diff

array_diff_sptr
compute_diff(array_type_def_sptr first,
	     array_type_def_sptr second,
	     diff_context_sptr ctxt);

class qualified_type_diff;
typedef class shared_ptr<qualified_type_diff> qualified_type_diff_sptr;

/// Abstraction of a diff between two qualified types.
class qualified_type_diff : public type_diff_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  qualified_type_diff(qualified_type_def_sptr	first,
		      qualified_type_def_sptr	second,
		      diff_sptr		underling,
		      diff_context_sptr	ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  const qualified_type_def_sptr
  first_qualified_type() const;

  const qualified_type_def_sptr
  second_qualified_type() const;

  diff_sptr
  underlying_type_diff() const;

  void
  underlying_type_diff(const diff_sptr);

  diff_sptr
  leaf_underlying_type_diff() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend qualified_type_diff_sptr
  compute_diff(const qualified_type_def_sptr first,
	       const qualified_type_def_sptr second,
	       diff_context_sptr ctxt);
};// end class qualified_type_diff.

qualified_type_diff_sptr
compute_diff(const qualified_type_def_sptr first,
	     const qualified_type_def_sptr second,
	     diff_context_sptr ctxt);

class enum_diff;
typedef shared_ptr<enum_diff> enum_diff_sptr;

/// Abstraction of a diff between two enums.
class enum_diff : public type_diff_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  void
  clear_lookup_tables();

  bool
  lookup_tables_empty() const;

  void
  ensure_lookup_tables_populated();

protected:
  enum_diff(const enum_type_decl_sptr,
	    const enum_type_decl_sptr,
	    const diff_sptr,
	    diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  const enum_type_decl_sptr
  first_enum() const;

  const enum_type_decl_sptr
  second_enum() const;

  diff_sptr
  underlying_type_diff() const;

  const string_enumerator_map&
  deleted_enumerators() const;

  const string_enumerator_map&
  inserted_enumerators() const;

  const string_changed_enumerator_map&
  changed_enumerators() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend enum_diff_sptr
  compute_diff(const enum_type_decl_sptr first,
	       const enum_type_decl_sptr second,
	       diff_context_sptr ctxt);
};//end class enum_diff;

enum_diff_sptr
compute_diff(const enum_type_decl_sptr,
	     const enum_type_decl_sptr,
	     diff_context_sptr);

/// This is the base class of @ref class_diff and @ref union_diff.
class class_or_union_diff : public type_diff_base
{
protected:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  void
  clear_lookup_tables(void);

  bool
  lookup_tables_empty(void) const;

  void
  ensure_lookup_tables_populated(void) const;

  void
  allocate_priv_data();

protected:
  class_or_union_diff(class_or_union_sptr first_scope,
		      class_or_union_sptr second_scope,
		      diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:

  const class_or_union_diff::priv_sptr&
  get_priv() const;

  //TODO: add change of the name of the type.

  virtual ~class_or_union_diff();

  class_or_union_sptr
  first_class_or_union() const;

  class_or_union_sptr
  second_class_or_union() const;

  const edit_script&
  member_types_changes() const;

  edit_script&
  member_types_changes();

  const edit_script&
  data_members_changes() const;

  edit_script&
  data_members_changes();

  const string_decl_base_sptr_map&
  inserted_data_members() const;

  const string_decl_base_sptr_map&
  deleted_data_members() const;

  const edit_script&
  member_fns_changes() const;

  edit_script&
  member_fns_changes();

  const function_decl_diff_sptrs_type&
  changed_member_fns() const;

  const string_member_function_sptr_map&
  deleted_member_fns() const;

  const string_member_function_sptr_map&
  inserted_member_fns() const;

  const var_diff_sptrs_type&
  sorted_changed_data_members() const;

  size_t
  count_filtered_changed_data_members(bool local_only = false) const;

  const var_diff_sptrs_type&
  sorted_subtype_changed_data_members() const;

  size_t
  count_filtered_subtype_changed_data_members(bool local_only = false) const;

  const string_decl_base_sptr_map&
  data_members_replaced_by_adms() const;

  const changed_var_sptrs_type&
  ordered_data_members_replaced_by_adms() const;

  const edit_script&
  member_fn_tmpls_changes() const;

  edit_script&
  member_fn_tmpls_changes();

  const edit_script&
  member_class_tmpls_changes() const;

  edit_script&
  member_class_tmpls_changes();

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend class default_reporter;
}; // end class_or_union_diff;

/// This type abstracts changes for a class_decl.
class class_diff : public class_or_union_diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  const priv_sptr& get_priv()const;

  void
  clear_lookup_tables(void);

  bool
  lookup_tables_empty(void) const;

  void
  ensure_lookup_tables_populated(void) const;

  void
   allocate_priv_data();

protected:
  class_diff(class_decl_sptr first_scope,
	     class_decl_sptr second_scope,
	     diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  //TODO: add change of the name of the type.

  virtual ~class_diff();

  class_decl_sptr
  first_class_decl() const;

  class_decl_sptr
  second_class_decl() const;

  const edit_script&
  base_changes() const;

  edit_script&
  base_changes();

  const string_base_sptr_map&
  deleted_bases() const;

  const string_base_sptr_map&
  inserted_bases() const;

  const base_diff_sptrs_type&
  changed_bases();

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual const string&
  get_pretty_representation() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend class_diff_sptr
  compute_diff(const class_decl_sptr	first,
	       const class_decl_sptr	second,
	       diff_context_sptr	ctxt);

  friend class default_reporter;
};// end class_diff

class_diff_sptr
compute_diff(const class_decl_sptr	first,
	     const class_decl_sptr	second,
	     diff_context_sptr		ctxt);

class union_diff;
typedef shared_ptr<union_diff> union_diff_sptr;

class union_diff : public class_or_union_diff
{
  void
  clear_lookup_tables(void);

  bool
  lookup_tables_empty(void) const;

  void
  ensure_lookup_tables_populated(void) const;

  void
  allocate_priv_data();

protected:
  union_diff(union_decl_sptr first_union,
	     union_decl_sptr second_union,
	     diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:

  virtual ~union_diff();

  union_decl_sptr
  first_union_decl() const;

  union_decl_sptr
  second_union_decl() const;

  virtual const string&
  get_pretty_representation() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  friend union_diff_sptr
  compute_diff(const union_decl_sptr	first,
	       const union_decl_sptr	second,
	       diff_context_sptr	ctxt);
}; // end class union_diff

union_diff_sptr
compute_diff(const union_decl_sptr	first,
	     const union_decl_sptr	second,
	     diff_context_sptr	ctxt);

/// An abstraction of a diff between two instances of class_decl::base_spec.
class base_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  base_diff(class_decl::base_spec_sptr	first,
	    class_decl::base_spec_sptr	second,
	    class_diff_sptr		underlying,
	    diff_context_sptr		ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  class_decl::base_spec_sptr
  first_base() const;

  class_decl::base_spec_sptr
  second_base() const;

  const class_diff_sptr
  get_underlying_class_diff() const;

  void
  set_underlying_class_diff(class_diff_sptr d);

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend base_diff_sptr
  compute_diff(const class_decl::base_spec_sptr first,
	       const class_decl::base_spec_sptr second,
	       diff_context_sptr		ctxt);
};// end class base_diff

base_diff_sptr
compute_diff(const class_decl::base_spec_sptr first,
	     const class_decl::base_spec_sptr second,
	     diff_context_sptr		ctxt);

class scope_diff;

/// Convenience typedef for a shared pointer on a @ref scope_diff.
typedef shared_ptr<scope_diff> scope_diff_sptr;

/// An abstractions of the changes between two scopes.
class scope_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

  bool
  lookup_tables_empty() const;

  void
  clear_lookup_tables();

  void
  ensure_lookup_tables_populated();

protected:
  scope_diff(scope_decl_sptr first_scope,
	     scope_decl_sptr second_scope,
	     diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:

  friend scope_diff_sptr
  compute_diff(const scope_decl_sptr	first,
	       const scope_decl_sptr	second,
	       scope_diff_sptr		d,
	       diff_context_sptr	ctxt);

  friend scope_diff_sptr
  compute_diff(const scope_decl_sptr	first_scope,
	       const scope_decl_sptr	second_scope,
	       diff_context_sptr	ctxt);

  const scope_decl_sptr
  first_scope() const;

  const scope_decl_sptr
  second_scope() const;

  const edit_script&
  member_changes() const;

  edit_script&
  member_changes();

  const decl_base_sptr
  deleted_member_at(unsigned index) const;

  const decl_base_sptr
  deleted_member_at(vector<deletion>::const_iterator) const;

  const decl_base_sptr
  inserted_member_at(unsigned i);

  const decl_base_sptr
  inserted_member_at(vector<unsigned>::const_iterator i);

  const diff_sptrs_type&
  changed_types() const;

  const diff_sptrs_type&
  changed_decls() const;

  const string_decl_base_sptr_map&
  removed_types() const;

  const string_decl_base_sptr_map&
  removed_decls() const;

  const string_decl_base_sptr_map&
  added_types() const;

  const string_decl_base_sptr_map&
  added_decls() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend class default_reporter;
  friend class leaf_reporter;
};// end class scope_diff

scope_diff_sptr
compute_diff(const scope_decl_sptr first,
	     const scope_decl_sptr second,
	     scope_diff_sptr d,
	     diff_context_sptr ctxt);

scope_diff_sptr
compute_diff(const scope_decl_sptr first_scope,
	     const scope_decl_sptr second_scope,
	     diff_context_sptr ctxt);

/// Abstraction of a diff between two function parameters.
class fn_parm_diff : public decl_diff_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  virtual void
  finish_diff_type();

  fn_parm_diff(const function_decl::parameter_sptr	first,
	       const function_decl::parameter_sptr	second,
	       diff_context_sptr			ctxt);

public:
  friend fn_parm_diff_sptr
  compute_diff(const function_decl::parameter_sptr	first,
	       const function_decl::parameter_sptr	second,
	       diff_context_sptr			ctxt);

  const function_decl::parameter_sptr
  first_parameter() const;

  const function_decl::parameter_sptr
  second_parameter() const;

  diff_sptr
  type_diff() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();
}; // end class fn_parm_diff

fn_parm_diff_sptr
compute_diff(const function_decl::parameter_sptr	first,
	     const function_decl::parameter_sptr	second,
	     diff_context_sptr				ctxt);

class function_type_diff;

/// A convenience typedef for a shared pointer to @ref
/// function_type_type_diff
typedef shared_ptr<function_type_diff> function_type_diff_sptr;

/// Abstraction of a diff between two function types.
class function_type_diff: public type_diff_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  void
  ensure_lookup_tables_populated();

  const function_decl::parameter_sptr
  deleted_parameter_at(int i) const;

  const function_decl::parameter_sptr
  inserted_parameter_at(int i) const;

protected:
  function_type_diff(const function_type_sptr	first,
		     const function_type_sptr	second,
		     diff_context_sptr		ctxt);

  virtual void
  finish_diff_type();

public:
  friend function_type_diff_sptr
  compute_diff(const function_type_sptr	first,
	       const function_type_sptr	second,
	       diff_context_sptr		ctxt);

  const function_type_sptr
  first_function_type() const;

  const function_type_sptr
  second_function_type() const;

  const diff_sptr
  return_type_diff() const;

  const string_fn_parm_diff_sptr_map&
  subtype_changed_parms() const;

  const string_parm_map&
  removed_parms() const;

  const string_parm_map&
  added_parms() const;

  const vector<function_decl::parameter_sptr>&
  sorted_deleted_parms() const;

  const vector<function_decl::parameter_sptr>&
  sorted_added_parms() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend class default_reporter;
  friend class leaf_reporter;
};// end class function_type_diff

function_type_diff_sptr
compute_diff(const function_type_sptr	first,
	     const function_type_sptr	second,
	     diff_context_sptr		ctxt);

/// Abstraction of a diff between two function_decl.
class function_decl_diff : public decl_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

  void
  ensure_lookup_tables_populated();


protected:
  function_decl_diff(const function_decl_sptr	first,
		     const function_decl_sptr	second,
		     diff_context_sptr		ctxt);

  virtual void
  finish_diff_type();

public:

friend function_decl_diff_sptr
compute_diff(const function_decl_sptr	first,
	     const function_decl_sptr	second,
	     diff_context_sptr		ctxt);

  const function_decl_sptr
  first_function_decl() const;

  const function_decl_sptr
  second_function_decl() const;

  const function_type_diff_sptr
  type_diff() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();
}; // end class function_decl_diff

function_decl_diff_sptr
compute_diff(const function_decl_sptr	first,
	     const function_decl_sptr	second,
	     diff_context_sptr		ctxt);

class type_decl_diff;

/// Convenience typedef for a shared pointer on a @ref type_decl_diff type.
typedef shared_ptr<type_decl_diff> type_decl_diff_sptr;

/// Abstraction of a diff between two basic type declarations.
class type_decl_diff : public type_diff_base
{
  type_decl_diff();

protected:
  type_decl_diff(const type_decl_sptr first,
		 const type_decl_sptr second,
		 diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  friend type_decl_diff_sptr
  compute_diff(const type_decl_sptr	first,
	       const type_decl_sptr	second,
	       diff_context_sptr	ctxt);

  const type_decl_sptr
  first_type_decl() const;

  const type_decl_sptr
  second_type_decl() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;
};// end type_decl_diff

type_decl_diff_sptr
compute_diff(const type_decl_sptr,
	     const type_decl_sptr,
	     diff_context_sptr);

class typedef_diff;

/// Convenience typedef for a shared pointer on a typedef_diff type.
typedef shared_ptr<typedef_diff> typedef_diff_sptr;

/// Abstraction of a diff between two typedef_decl.
class typedef_diff : public type_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

  typedef_diff();

protected:
  typedef_diff(const typedef_decl_sptr	first,
	       const typedef_decl_sptr	second,
	       const diff_sptr		underlying_type_diff,
	       diff_context_sptr	ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  friend typedef_diff_sptr
  compute_diff(const typedef_decl_sptr	first,
	       const typedef_decl_sptr	second,
	       diff_context_sptr	ctxt);

  const typedef_decl_sptr
  first_typedef_decl() const;

  const typedef_decl_sptr
  second_typedef_decl() const;

  const diff_sptr
  underlying_type_diff() const;

  void
  underlying_type_diff(const diff_sptr);

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();
};// end class typedef_diff

typedef_diff_sptr
compute_diff(const typedef_decl_sptr,
	     const typedef_decl_sptr,
	     diff_context_sptr ctxt);

const diff*
get_typedef_diff_underlying_type_diff(const diff* diff);

class translation_unit_diff;

/// Convenience typedef for a shared pointer on a
/// @ref translation_unit_diff type.
typedef shared_ptr<translation_unit_diff> translation_unit_diff_sptr;

/// An abstraction of a diff between two translation units.
class translation_unit_diff : public scope_diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  translation_unit_diff(translation_unit_sptr	first,
			translation_unit_sptr	second,
			diff_context_sptr	ctxt = diff_context_sptr());

public:

  const translation_unit_sptr
  first_translation_unit() const;

  const translation_unit_sptr
  second_translation_unit() const;

  friend translation_unit_diff_sptr
  compute_diff(const translation_unit_sptr	first,
	       const translation_unit_sptr	second,
	       diff_context_sptr		ctxt);

  virtual bool
  has_changes() const;

  virtual enum change_kind
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;
};//end class translation_unit_diff

translation_unit_diff_sptr
compute_diff(const translation_unit_sptr first,
	     const translation_unit_sptr second,
	     diff_context_sptr ctxt = diff_context_sptr());

/// An abstraction of a diff between between two abi corpus.
class corpus_diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  corpus_diff(corpus_sptr	first,
	      corpus_sptr	second,
	      diff_context_sptr ctxt = diff_context_sptr());

  void
  finish_diff_type();

public:

  class diff_stats;

  virtual ~corpus_diff() {}

  /// A convenience typedef for a shared pointer to @ref diff_stats
  typedef shared_ptr<diff_stats> diff_stats_sptr;

  corpus_sptr
  first_corpus() const;

  corpus_sptr
  second_corpus() const;

  const vector<diff*>&
  children_nodes() const;

  void
  append_child_node(diff_sptr);

  edit_script&
  function_changes() const;

  edit_script&
  variable_changes() const;

  bool
  soname_changed() const;

  bool
  architecture_changed() const;

  const string_function_ptr_map&
  deleted_functions() const;

  const string_function_ptr_map&
  added_functions();

  const string_function_decl_diff_sptr_map&
  changed_functions();

  const function_decl_diff_sptrs_type&
  changed_functions_sorted();

  const string_var_ptr_map&
  deleted_variables() const;

  const string_var_ptr_map&
  added_variables() const;

  const string_var_diff_sptr_map&
  changed_variables();

  const var_diff_sptrs_type&
  changed_variables_sorted();

  const string_elf_symbol_map&
  deleted_unrefed_function_symbols() const;

  const string_elf_symbol_map&
  added_unrefed_function_symbols() const;

  const string_elf_symbol_map&
  deleted_unrefed_variable_symbols() const;

  const string_elf_symbol_map&
  added_unrefed_variable_symbols() const;

  const string_type_base_sptr_map&
  deleted_unreachable_types() const;

  const vector<type_base_sptr>&
  deleted_unreachable_types_sorted() const;

  const string_type_base_sptr_map&
  added_unreachable_types() const;

  const vector<type_base_sptr>&
  added_unreachable_types_sorted() const;

  const string_diff_sptr_map&
  changed_unreachable_types() const;

  const vector<diff_sptr>&
  changed_unreachable_types_sorted() const;

  const diff_context_sptr
  context() const;

  const string&
  get_pretty_representation() const;

  bool
  has_changes() const;

  bool
  has_incompatible_changes() const;

  bool
  has_net_subtype_changes() const;

  bool
  has_net_changes() const;

  const diff_stats&
  apply_filters_and_suppressions_before_reporting();

  void
  mark_leaf_diff_nodes();

  diff_maps&
  get_leaf_diffs();

  const diff_maps&
  get_leaf_diffs() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  virtual void
  chain_into_hierarchy();

  friend corpus_diff_sptr
  compute_diff(const corpus_sptr f,
	       const corpus_sptr s,
	       diff_context_sptr ctxt);

  friend void
  apply_suppressions(const corpus_diff* diff_tree);

  friend void
  maybe_report_unreachable_type_changes(const corpus_diff& d,
					const corpus_diff::diff_stats &s,
					const string& indent,
					ostream& out);

  friend class default_reporter;
  friend class leaf_reporter;
}; // end class corpus_diff

corpus_diff_sptr
compute_diff(const corpus_sptr,
	     const corpus_sptr,
	     diff_context_sptr = diff_context_sptr());

corpus_diff_sptr
compute_diff(const corpus_group_sptr&,
	     const corpus_group_sptr&,
	     diff_context_sptr	ctxt);

/// This is a document class that aims to capture statistics about the
/// changes carried by a @ref corpus_diff type.
///
/// Its values are populated by the member function
/// corpus_diff::apply_filters_and_suppressions_before_reporting()
class corpus_diff::diff_stats
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  diff_stats();

public:

  diff_stats(diff_context_sptr);

  size_t num_func_removed() const;
  void num_func_removed(size_t);

  size_t num_removed_func_filtered_out() const;
  void num_removed_func_filtered_out(size_t);

  size_t net_num_func_removed() const;

  size_t num_func_added() const;
  void num_func_added(size_t);

  size_t num_added_func_filtered_out() const;
  void num_added_func_filtered_out(size_t);

  size_t net_num_func_added() const;

  size_t num_func_changed() const;
  void num_func_changed(size_t);

  size_t num_changed_func_filtered_out() const;
  void num_changed_func_filtered_out(size_t);

  size_t num_func_with_virtual_offset_changes() const;
  void num_func_with_virtual_offset_changes(size_t);

  size_t net_num_func_changed() const;

  size_t num_vars_removed() const;
  void num_vars_removed(size_t);

  size_t num_removed_vars_filtered_out() const;
  void num_removed_vars_filtered_out(size_t) const;

  size_t net_num_vars_removed() const;

  size_t num_vars_added() const;
  void num_vars_added(size_t);

  size_t num_added_vars_filtered_out() const;
  void num_added_vars_filtered_out(size_t);

  size_t net_num_vars_added() const;

  size_t num_vars_changed() const;
  void num_vars_changed(size_t);

  size_t num_changed_vars_filtered_out() const;
  void num_changed_vars_filtered_out(size_t);

  size_t net_num_vars_changed() const;

  size_t num_func_syms_removed() const;
  void num_func_syms_removed(size_t);

  size_t num_removed_func_syms_filtered_out() const;
  void num_removed_func_syms_filtered_out(size_t);

  size_t num_func_syms_added() const;
  void num_func_syms_added(size_t);

  size_t num_added_func_syms_filtered_out() const;
  void num_added_func_syms_filtered_out(size_t);

  size_t net_num_removed_func_syms() const;
  size_t net_num_added_func_syms() const;

  size_t num_var_syms_removed() const;
  void num_var_syms_removed(size_t);

  size_t num_removed_var_syms_filtered_out() const;
  void num_removed_var_syms_filtered_out(size_t);

  size_t num_var_syms_added() const;
  void num_var_syms_added(size_t);

  size_t num_added_var_syms_filtered_out() const;
  void num_added_var_syms_filtered_out(size_t);

  size_t net_num_removed_var_syms() const;
  size_t net_num_added_var_syms() const;

  size_t num_leaf_changes() const;
  void num_leaf_changes(size_t);

  size_t num_leaf_changes_filtered_out() const;
  void num_leaf_changes_filtered_out(size_t);

  size_t net_num_leaf_changes() const;

  size_t num_leaf_type_changes() const;
  void num_leaf_type_changes(size_t);

  size_t num_leaf_type_changes_filtered_out() const;
  void num_leaf_type_changes_filtered_out(size_t);
  size_t net_num_leaf_type_changes() const;

  size_t num_leaf_func_changes() const;
  void num_leaf_func_changes(size_t);

  size_t num_leaf_func_changes_filtered_out() const;
  void num_leaf_func_changes_filtered_out(size_t);
  size_t net_num_leaf_func_changes() const;

  size_t num_leaf_var_changes() const;
  void num_leaf_var_changes(size_t);

  size_t num_leaf_var_changes_filtered_out() const;
  void num_leaf_var_changes_filtered_out(size_t);
  size_t net_num_leaf_var_changes() const;

  size_t num_added_unreachable_types() const;
  void num_added_unreachable_types(size_t);

  size_t num_added_unreachable_types_filtered_out() const;
  void num_added_unreachable_types_filtered_out(size_t);
  size_t net_num_added_unreachable_types() const;

  size_t num_removed_unreachable_types() const;
  void num_removed_unreachable_types(size_t);

  size_t num_removed_unreachable_types_filtered_out() const;
  void num_removed_unreachable_types_filtered_out(size_t);
  size_t net_num_removed_unreachable_types() const;

  size_t num_changed_unreachable_types() const;
  void num_changed_unreachable_types(size_t);

  size_t num_changed_unreachable_types_filtered_out() const;
  void num_changed_unreachable_types_filtered_out(size_t);
  size_t net_num_changed_unreachable_types() const;

}; // end class corpus_diff::diff_stats

/// The base class for the node visitors.  These are the types used to
/// visit each node traversed by the diff_traversable_base::traverse() method.
class diff_node_visitor : public node_visitor_base
{
protected:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

public:

  diff_node_visitor();

  virtual ~diff_node_visitor() {}

  diff_node_visitor(visiting_kind k);

  visiting_kind
  get_visiting_kind() const;

  void
  set_visiting_kind(visiting_kind v);

  void
  or_visiting_kind(visiting_kind v);

  void
  set_current_topmost_iface_diff(diff*);

  diff*
  get_current_topmost_iface_diff() const;

  virtual void
  visit_begin(diff*);

  virtual void
  visit_begin(corpus_diff*);

  virtual void
  visit_end(diff*);

  virtual void
  visit_end(corpus_diff*);

  virtual bool
  visit(diff*, bool);

  virtual bool
  visit(distinct_diff*, bool);

  virtual bool
  visit(var_diff*, bool);

  virtual bool
  visit(pointer_diff*, bool);

  virtual bool
  visit(reference_diff*, bool);

  virtual bool
  visit(qualified_type_diff*, bool);

  virtual bool
  visit(enum_diff*, bool);

  virtual bool
  visit(class_diff*, bool);

  virtual bool
  visit(base_diff*, bool);

  virtual bool
  visit(scope_diff*, bool);

  virtual bool
  visit(function_decl_diff*, bool);

  virtual bool
  visit(type_decl_diff*, bool);

  virtual bool
  visit(typedef_diff*, bool);

  virtual bool
  visit(translation_unit_diff*, bool);

  virtual bool
  visit(corpus_diff*, bool);
}; // end struct diff_node_visitor

void
propagate_categories(diff* diff_tree);

void
propagate_categories(diff_sptr diff_tree);

void
propagate_categories(corpus_diff* diff_tree);

void
propagate_categories(corpus_diff_sptr diff_tree);

void
apply_suppressions(diff* diff_tree);

void
apply_suppressions(const corpus_diff* diff_tree);

void
apply_suppressions(diff_sptr diff_tree);

void
apply_suppressions(corpus_diff_sptr diff_tree);

void
print_diff_tree(diff* diff_tree, std::ostream&);

void
print_diff_tree(corpus_diff* diff_tree,
		std::ostream&);

void
print_diff_tree(diff_sptr diff_tree,
		std::ostream&);

void
print_diff_tree(corpus_diff_sptr diff_tree,
		std::ostream&);

void
categorize_redundancy(diff* diff_tree);

void
categorize_redundancy(diff_sptr diff_tree);

void
categorize_redundancy(corpus_diff* diff_tree);

void
categorize_redundancy(corpus_diff_sptr diff_tree);

void
clear_redundancy_categorization(diff* diff_tree);

void
clear_redundancy_categorization(diff_sptr diff_tree);

void
clear_redundancy_categorization(corpus_diff* diff_tree);

void
clear_redundancy_categorization(corpus_diff_sptr diff_tree);

void
apply_filters(corpus_diff_sptr diff_tree);

bool
is_diff_of_variadic_parameter_type(const diff*);

bool
is_diff_of_variadic_parameter_type(const diff_sptr&);

bool
is_diff_of_variadic_parameter(const diff*);

bool
is_diff_of_variadic_parameter(const diff_sptr&);

const type_diff_base*
is_type_diff(const diff* diff);

const decl_diff_base*
is_decl_diff(const diff* diff);

const type_decl_diff*
is_diff_of_basic_type(const diff* diff);

const type_decl_diff*
is_diff_of_basic_type(const diff* diff, bool);

const class_or_union_diff*
is_diff_of_class_or_union_type(const diff *d);

bool
has_basic_type_change_only(const diff* diff);

const enum_diff*
is_enum_diff(const diff *diff);

const class_diff*
is_class_diff(const diff* diff);

const union_diff*
is_union_diff(const diff* diff);

const class_or_union_diff*
is_class_or_union_diff(const diff* d);

const class_or_union_diff*
is_anonymous_class_or_union_diff(const diff* d);

const array_diff*
is_array_diff(const diff* diff);

const function_type_diff*
is_function_type_diff(const diff* diff);

const function_type_diff*
is_function_type_diff_with_local_changes(const diff* diff);

const typedef_diff*
is_typedef_diff(const diff *diff);

const var_diff*
is_var_diff(const diff* diff);

const function_decl_diff*
is_function_decl_diff(const diff* diff);

const pointer_diff*
is_pointer_diff(const diff* diff);

const reference_diff*
is_reference_diff(const diff* diff);

const qualified_type_diff*
is_qualified_type_diff(const diff* diff);

const fn_parm_diff*
is_fn_parm_diff(const diff* diff);

const base_diff*
is_base_diff(const diff* diff);

const distinct_diff*
is_distinct_diff(const diff *diff);

bool
is_child_node_of_function_parm_diff(const diff* diff);

bool
is_child_node_of_base_diff(const diff* diff);

const corpus_diff*
is_corpus_diff(const diff* diff);

const diff*
peel_typedef_diff(const diff* dif);

const diff*
peel_pointer_diff(const diff* dif);

const diff*
peel_reference_diff(const diff* dif);

const diff*
peel_qualified_diff(const diff* dif);

const diff*
peel_pointer_or_qualified_type_diff(const diff* dif);

const diff*
peel_typedef_or_qualified_type_diff(const diff* dif);
}// end namespace comparison

}// end namespace abigail

#endif //__ABG_COMPARISON_H__

// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// Types of the main internal representation of libabigail.
///
/// This internal representation abstracts the artifacts that make up
/// an application binary interface.

#ifndef __ABG_IR_H__
#define __ABG_IR_H__

#include <assert.h>
#include <stdint.h>
#include <cstdlib>
#include <functional>
#include <set>
#include <unordered_map>
#include "abg-fwd.h"
#include "abg-hash.h"
#include "abg-traverse.h"
#include "abg-config.h"

/// @file
///
/// This file contains the declarations of the Internal Representation
/// of libabigail.

/// @defgroup Memory Memory management
/// @{
///
/// How objects' lifetime is handled in libabigail.
///
/// For memory management and garbage collection of libabigail's IR
/// artifacts, we use std::shared_ptr and std::weak_ptr.
///
/// When manipulating these IR artifacts, there are a few rules to keep in
/// mind.
///
/// <b>The declaration for a type is owned by only one scope </b>
///
/// This means that for each instance of abigail::type_base (a type) there
/// is an instance of abigail::scope_decl that owns a @ref
/// abigail::decl_base_sptr (a shared pointer to an abigail::decl_base)
/// that points to the declaration of that type.  The
/// abigail::type_base_sptr is added to the scope using the function
/// abigail::add_decl_to_scope().
///
/// There is a kind of type that is usually not syntactically owned by
/// a scope: it's function type.  In libabigail, function types are
/// represented by abigail::function_type and abigail::method_type.
/// These types must be owned by the translation unit they originate
/// from.  Adding them to the translation unit must be done by a call
/// to the method function
/// abigail::translation::bind_function_type_life_time().
///
/// <b> A declaration that has a type does NOT own the type </b>
///
/// This means that, for instance, in an abigail::var_decl (a variable
/// declaration), the type of the declaration is not owned by the
/// declaration.  In other (concrete) words, the variable declaration
/// doesn't have a shared pointer to the type.  Rather, it has a *weak*
/// pointer to its type.  That means that it has a data member of type
/// abigail::type_base_wptr that contains the type of the declaration.
///
/// But then abigail::var_decl::get_type() returns a shared pointer that
/// is constructed from the internal weak pointer to the type.  That way,
/// users of the type of the var can own a temporary reference on it and
/// be assured that the type's life time is long enough for their need.
///
/// Likewise, data members, function and template parameters similarly
/// have weak pointers on their type.
///
/// If, for a reason, you really need to keep a type alive for the
/// entire lifetime of the type system, then you can bind the life
/// time of that type to the life time of the @ref environment that is
/// supposed to outlive the type system.  You do that by passing the
/// type to the function environment::keep_type_alive().
///
/// @}

namespace abigail
{

/// The namespace of the internal representation of ABI artifacts like
/// types and decls.
namespace ir
{

// Inject some std types in here.
using std::unordered_map;

/// A convenience typedef fo r an ordered set of size_t.
typedef unordered_set<size_t> pointer_set;

/// Functor to hash a canonical type by using its pointer value.
struct canonical_type_hash
{
  size_t operator()(const type_base_sptr& l) const;
  size_t operator()(const type_base *l) const;
}; //end struct canonical_type_hash

/// Helper typedef for an unordered set of type_base_sptr which uses
/// pointer value to tell its members appart, because the members are
/// canonical types.
typedef unordered_set<type_base_sptr,
		      canonical_type_hash> canonical_type_sptr_set_type;

/// Helper typedef for a vector of pointer to type_base.
typedef vector<type_base*> type_base_ptrs_type;

/// Helper typedef for a vector of shared pointer to a type_base.
typedef vector<type_base_sptr> type_base_sptrs_type;

/// This is an abstraction of the set of resources necessary to manage
/// several aspects of the internal representations of the Abigail
/// library.
///
/// An environment can be seen as the boundaries in which all related
/// Abigail artifacts live.  So before doing anything using this
/// library, the first thing to create is, well, you know it now, an
/// environment.
///
/// Note that the lifetime of environment objects must be longer than
/// the lifetime of any other type in the Abigail system.  So a given
/// instance of @ref environment must stay around as long as you are
/// using libabigail.  It's only when you are done using the library
/// that you can de-allocate the environment instance.
class environment
{
public:

  /// A convenience typedef for a map of canonical types.  The key is
  /// the pretty representation string of a particular type and the
  /// value is the vector of canonical types that have the same pretty
  /// representation string.
  typedef std::unordered_map<string, std::vector<type_base_sptr> >
      canonical_types_map_type;

private:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;
public:

  environment();
  virtual ~environment();

  canonical_types_map_type&
  get_canonical_types_map();

  const canonical_types_map_type&
  get_canonical_types_map() const;

  const type_base_sptr&
  get_void_type() const;

  const type_base_sptr&
  get_variadic_parameter_type() const;

  bool
  canonicalization_is_done() const;

  void
  canonicalization_is_done(bool);

  bool
  do_on_the_fly_canonicalization() const;

  void
  do_on_the_fly_canonicalization(bool f);

  bool
  decl_only_class_equals_definition() const;

  void
  decl_only_class_equals_definition(bool f) const;

  bool
  is_void_type(const type_base_sptr&) const;

  bool
  is_void_type(const type_base*) const;

  bool
  is_variadic_parameter_type(const type_base*) const;

  bool
  is_variadic_parameter_type(const type_base_sptr&) const;

  interned_string
  intern(const string&) const;

  const config&
  get_config() const;

  friend class class_or_union;
  friend class class_decl;
  friend class function_type;

  friend void keep_type_alive(type_base_sptr);
}; // end class environment

class location_manager;
/// @brief The source location of a token.
///
/// This represents the location of a token coming from a given
/// translation unit.  This location is actually an abstraction of
/// cursor in the table of all the locations of all the tokens of the
/// translation unit.  That table is managed by the @ref location_manager
/// type.  To get the file path, line and column numbers associated to
/// a given instance of @ref location, you need to use the
/// location_manager::expand_location method.
class location
{
  unsigned		value_;
  // The location manager to use to decode the value above.  There is
  // one location manager per translation unit, and the location
  // manager's life time is managed by its translation unit.
  location_manager*	loc_manager_;

  location(unsigned v, location_manager* m)
    : value_(v), loc_manager_(m)
  {}

  /// Get the location manager to use to decode the value of this
  /// location.
  ///
  /// @return the location manager for the current location value.
  location_manager*
  get_location_manager() const
  {return loc_manager_;}

public:

  /// Copy constructor of the location.
  ///
  /// @param l the location to copy from.
  location(const location& l)
    : value_(l.value_),
      loc_manager_(l.loc_manager_)
  {}

  /// Assignment operator of the location.
  ///
  /// @param l the location to assign to the current one.
  location&
  operator=(const location& l)
  {
    value_ = l.value_;
    loc_manager_ = l.loc_manager_;
    return *this;
  }

  /// Default constructor for the @ref location type.
  location()
    : value_(), loc_manager_()
  {}

  /// Get the value of the location.
  unsigned
  get_value() const
  {return value_;}

  /// Convert the location into a boolean.
  ///
  /// @return true iff the value of the location is different from
  /// zero.
  operator bool() const
  {return !!value_;}

  /// Equality operator of the @ref location type.
  ///
  /// @param other the other location to compare against.
  ///
  /// @return true iff both locations are equal.
  bool
  operator==(const location &other) const
  {return value_ == other.value_;}

  /// "Less than" operator of the @ref location type.
  ///
  /// @parm other the other location type to compare against.
  ///
  /// @return true iff the current instance is less than the @p other
  /// one.
  bool
  operator<(const location &other) const
  {return value_ < other.value_;}

  /// Expand the current location into a tripplet file path, line and
  /// column number.
  ///
  /// @param path the output parameter this function sets the expanded
  /// path to.
  ///
  /// @param line the output parameter this function sets the expanded
  /// line number to.
  ///
  /// @param column the output parameter this function sets the
  /// expanded column number to.
  void
  expand(std::string& path, unsigned& line, unsigned& column) const;

  string
  expand(void) const;

  friend class location_manager;
}; // end class location

/// @brief The entry point to manage locations.
///
/// This type keeps a table of all the locations for tokens of a
/// given translation unit.
class location_manager
{
  struct priv;

  /// Pimpl.
  shared_ptr<priv> priv_;

public:

  location_manager();

  location
  create_new_location(const std::string& fle, size_t lne, size_t col);

  void
  expand_location(const location& location, std::string& path,
		  unsigned& line, unsigned& column) const;
};

/// The base of an entity of the intermediate representation that is
/// to be traversed.
struct ir_traversable_base : public traversable_base
{
  /// Traverse a given IR node and its children, calling an visitor on
  /// each node.
  ///
  /// @param v the visitor to call on each traversed node.
  ///
  /// @return true if the all the IR node tree was traversed.
  virtual bool
  traverse(ir_node_visitor& v);
}; // end class ir_traversable_base

/// The hashing functor for using instances of @ref type_or_decl_base
/// as values in a hash map or set.
struct type_or_decl_hash
{

  /// Function-call Operator to hash the string representation of an
  /// ABI artifact.
  ///
  /// @param artifact the ABI artifact to hash.
  ///
  /// @return the hash value of the string representation of @p
  /// artifact.
  size_t
  operator()(const type_or_decl_base *artifact) const
  {
    string repr =  get_pretty_representation(artifact);
    std::hash<string> do_hash;
    return do_hash(repr);
  }

  /// Function-call Operator to hash the string representation of an
  /// ABI artifact.
  ///
  /// @param artifact the ABI artifact to hash.
  ///
  /// @return the hash value of the string representation of @p
  /// artifact.
  size_t
  operator()(const type_or_decl_base_sptr& artifact) const
  {return operator()(artifact.get());}
}; // end struct type_or_decl_hash

/// The comparison functor for using instances of @ref
/// type_or_decl_base as values in a hash map or set.
struct type_or_decl_equal
{

  /// The function-call operator to compare the string representations
  /// of two ABI artifacts.
  ///
  /// @param l the left hand side ABI artifact operand of the
  /// comparison.
  ///
  /// @param r the right hand side ABI artifact operand of the
  /// comparison.
  ///
  /// @return true iff the string representation of @p l equals the one
  /// of @p r.
  bool
  operator()(const type_or_decl_base *l, const type_or_decl_base *r) const
  {
    string repr1 = get_pretty_representation(l);
    string repr2 = get_pretty_representation(r);

    return repr1 == repr2;
  }

  /// The function-call operator to compare the string representations
  /// of two ABI artifacts.
  ///
  /// @param l the left hand side ABI artifact operand of the
  /// comparison.
  ///
  /// @param r the right hand side ABI artifact operand of the
  /// comparison.
  ///
  /// @return true iff the string representation of @p l equals the one
  /// of @p r.
  bool
  operator()(const type_or_decl_base_sptr &l,
	     const type_or_decl_base_sptr &r) const
  {return operator()(l.get(), r.get());}
}; // end type_or_decl_equal

/// A convenience typedef for a hash set of type_or_decl_base_sptr
typedef unordered_set<type_or_decl_base_sptr,
		      type_or_decl_hash,
		      type_or_decl_equal> artifact_sptr_set_type;

/// A convenience typedef for a hash set of const type_or_decl_base*
typedef unordered_set<const type_or_decl_base*,
		      type_or_decl_hash,
		      type_or_decl_equal> artifact_ptr_set_type;

/// A convenience typedef for a map which key is a string and which
/// value is a @ref type_base_wptr.
typedef unordered_map<string, type_base_wptr> string_type_base_wptr_map_type;

/// A convenience typedef for a map which key is an @ref
/// interned_string and which value is a @ref type_base_wptr.
typedef unordered_map<interned_string, type_base_wptr, hash_interned_string>
istring_type_base_wptr_map_type;

/// A convenience typedef for a map which key is an @ref
/// interned_string and which value is a @ref type_base_wptr.
typedef unordered_map<interned_string,
		      type_or_decl_base_sptr,
		      hash_interned_string>
istring_type_or_decl_base_sptr_map_type;

/// This is a type that aggregates maps of all the kinds of types that
/// are supported by libabigail.
///
/// For instance, the type_maps contains a map of string to basic
/// type, a map of string to class type, a map of string to union
/// types, etc.  The key of a map entry is the pretty representation
/// of the type, and the value of the map entry is the type.
class type_maps
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

public:

  type_maps();

  bool
  empty() const;

  const istring_type_base_wptrs_map_type&
  basic_types() const;

  istring_type_base_wptrs_map_type&
  basic_types();

  const istring_type_base_wptrs_map_type&
  class_types() const;

  istring_type_base_wptrs_map_type&
  class_types();

  istring_type_base_wptrs_map_type&
  union_types();

  const istring_type_base_wptrs_map_type&
  union_types() const;

  istring_type_base_wptrs_map_type&
  enum_types();

  const istring_type_base_wptrs_map_type&
  enum_types() const;

  istring_type_base_wptrs_map_type&
  typedef_types();

  const istring_type_base_wptrs_map_type&
  typedef_types() const;

  istring_type_base_wptrs_map_type&
  qualified_types();

  const istring_type_base_wptrs_map_type&
  qualified_types() const;

  istring_type_base_wptrs_map_type&
  pointer_types();

  const istring_type_base_wptrs_map_type&
  pointer_types() const;

  istring_type_base_wptrs_map_type&
  reference_types();

  const istring_type_base_wptrs_map_type&
  reference_types() const;

  istring_type_base_wptrs_map_type&
  array_types();

  const istring_type_base_wptrs_map_type&
  array_types() const;

  const istring_type_base_wptrs_map_type&
  subrange_types() const;

  istring_type_base_wptrs_map_type&
  subrange_types();

  istring_type_base_wptrs_map_type&
  function_types();

  const istring_type_base_wptrs_map_type&
  function_types() const;

  const vector<type_base_wptr>&
  get_types_sorted_by_name() const;
}; // end class type_maps;

/// This is the abstraction of the set of relevant artefacts (types,
/// variable declarations, functions, templates, etc) bundled together
/// into a translation unit.
class translation_unit : public traversable_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  translation_unit();

public:
  /// Convenience typedef for a shared pointer on a @ref global_scope.
  typedef shared_ptr<scope_decl> global_scope_sptr;

  /// The language of the translation unit.
  enum language
  {
    LANG_UNKNOWN = 0,
    LANG_Cobol74,
    LANG_Cobol85,
    LANG_C89,
    LANG_C99,
    LANG_C11,
    LANG_C,
    LANG_C_plus_plus_03,
    LANG_C_plus_plus_11,
    LANG_C_plus_plus_14,
    LANG_C_plus_plus,
    LANG_ObjC,
    LANG_ObjC_plus_plus,
    LANG_Fortran77,
    LANG_Fortran90,
    LANG_Fortran95,
    LANG_Ada83,
    LANG_Ada95,
    LANG_Pascal83,
    LANG_Modula2,
    LANG_Java,
    LANG_PL1,
    LANG_UPC,
    LANG_D,
    LANG_Python,
    LANG_Go,
    LANG_Rust,
    LANG_Mips_Assembler
  };

public:
  translation_unit(const ir::environment*	env,
		   const std::string&		path,
		   char			address_size = 0);

  virtual ~translation_unit();

  const environment*
  get_environment() const;

  environment*
  get_environment();

  void
  set_environment(const environment*);

  language
  get_language() const;

  void
  set_language(language l);

  const std::string&
  get_path() const;

  void
  set_path(const string&);

  const std::string&
  get_compilation_dir_path() const;

  void
  set_compilation_dir_path(const std::string&);

  const std::string&
  get_absolute_path() const;

  void
  set_corpus(corpus*);

  const corpus*
  get_corpus() const;

  corpus*
  get_corpus();

  const scope_decl_sptr&
  get_global_scope() const;

  scope_decl_sptr&
  get_global_scope();

  const type_maps&
  get_types() const;

  type_maps&
  get_types();

  const vector<function_type_sptr>&
  get_live_fn_types() const;

  location_manager&
  get_loc_mgr();

  const location_manager&
  get_loc_mgr() const;

  bool
  is_empty() const;

  char
  get_address_size() const;

  void
  set_address_size(char);

  bool
  is_constructed() const;

  void
  set_is_constructed(bool);

  bool
  operator==(const translation_unit&) const;

  bool
  operator!=(const translation_unit&) const;

  void
  bind_function_type_life_time(function_type_sptr) const;

  virtual bool
  traverse(ir_node_visitor& v);

  friend function_type_sptr
  lookup_function_type_in_translation_unit(const function_type& t,
					   const translation_unit& tu);

  friend function_type_sptr
  synthesize_function_type_from_translation_unit(const function_type& fn_type,
						 translation_unit& tu);

  friend type_base_sptr
  synthesize_type_from_translation_unit(const type_base_sptr& type,
					translation_unit& tu);
};//end class translation_unit

/// A comparison functor to compare translation units based on their
/// absolute paths.
struct shared_translation_unit_comp
{
  /// Compare two translations units based on their absolute paths.
  ///
  /// @param lhs the first translation unit to consider for the
  /// comparison.
  ///
  /// @param rhs the second translatin unit to consider for the
  /// comparison.
  bool
  operator()(const translation_unit_sptr& lhs,
	     const translation_unit_sptr& rhs) const
  {return lhs->get_absolute_path() < rhs->get_absolute_path();}
}; // end struct shared_translation_unit_comp

/// Convenience typedef for an ordered set of @ref
/// translation_unit_sptr.
typedef std::set<translation_unit_sptr,
		 shared_translation_unit_comp> translation_units;

string
translation_unit_language_to_string(translation_unit::language);

translation_unit::language
string_to_translation_unit_language(const string&);

bool
is_c_language(translation_unit::language l);

bool
is_cplus_plus_language(translation_unit::language l);

bool
is_java_language(translation_unit::language l);

bool
is_ada_language(translation_unit::language l);

bool
operator==(const translation_unit_sptr&, const translation_unit_sptr&);

bool
operator!=(const translation_unit_sptr&, const translation_unit_sptr&);

/// Access specifier for class members.
enum access_specifier
{
  no_access,
  public_access,
  protected_access,
  private_access,
};

class elf_symbol;
/// A convenience typedef for a shared pointer to elf_symbol.
typedef shared_ptr<elf_symbol> elf_symbol_sptr;

/// A convenience typedef for a weak pointer to elf_symbol.
typedef weak_ptr<elf_symbol> elf_symbol_wptr;

/// Convenience typedef for a map which key is a string and which
/// value if the elf symbol of the same name.
typedef std::unordered_map<string, elf_symbol_sptr>
string_elf_symbol_sptr_map_type;

/// Convenience typedef for a shared pointer to an
/// string_elf_symbol_sptr_map_type.
typedef shared_ptr<string_elf_symbol_sptr_map_type>
string_elf_symbol_sptr_map_sptr;

/// Convenience typedef for a vector of elf_symbol
typedef std::vector<elf_symbol_sptr> elf_symbols;

/// Convenience typedef for a map which key is a string and which
/// value is a vector of elf_symbol.
typedef std::unordered_map<string, elf_symbols>
string_elf_symbols_map_type;

/// Convenience typedef for a shared pointer to
/// string_elf_symbols_map_type.
typedef shared_ptr<string_elf_symbols_map_type> string_elf_symbols_map_sptr;

/// Abstraction of an elf symbol.
///
/// This is useful when a given corpus has been read from an ELF file.
/// In that case, a given decl might be associated to its underlying
/// ELF symbol, if that decl is publicly exported in the ELF file.  In
/// that case, comparing decls might involve comparing their
/// underlying symbols as well.
class elf_symbol
{
public:
  /// The type of a symbol.
  enum type
  {
    NOTYPE_TYPE = 0,
    OBJECT_TYPE,
    FUNC_TYPE,
    SECTION_TYPE,
    FILE_TYPE,
    COMMON_TYPE,
    TLS_TYPE,
    GNU_IFUNC_TYPE
  };

  /// The binding of a symbol.
  enum binding
  {
    LOCAL_BINDING = 0,
    GLOBAL_BINDING,
    WEAK_BINDING,
    GNU_UNIQUE_BINDING
  };

  /// The visibility of the symbol.
  enum visibility
  {
    DEFAULT_VISIBILITY,
    PROTECTED_VISIBILITY,
    HIDDEN_VISIBILITY,
    INTERNAL_VISIBILITY,
  };

  /// Inject the elf_symbol::version here.
  class version;

private:
  struct priv;
  shared_ptr<priv> priv_;

  elf_symbol();

  elf_symbol(const environment* e,
	     size_t		i,
	     size_t		s,
	     const string&	n,
	     type		t,
	     binding		b,
	     bool		d,
	     bool		c,
	     const version&	ve,
	     visibility		vi,
	     bool		is_linux_string_cst = false,
	     bool		is_in_ksymtab = false,
	     uint64_t		crc = 0,
	     bool		is_suppressed = false);

  elf_symbol(const elf_symbol&);

  elf_symbol&
  operator=(const elf_symbol& s);

public:

  static elf_symbol_sptr
  create();

  static elf_symbol_sptr
  create(const environment* e,
	 size_t		    i,
	 size_t		    s,
	 const string&	    n,
	 type		    t,
	 binding	    b,
	 bool		    d,
	 bool		    c,
	 const version&	    ve,
	 visibility	    vi,
	 bool		    is_linux_string_cst = false,
	 bool		    is_in_ksymtab = false,
	 uint64_t	    crc = 0,
	 bool		    is_suppressed = false);

  const environment*
  get_environment() const;

  void
  set_environment(const environment*) const;

  size_t
  get_index() const;

  void
  set_index(size_t);

  bool
  get_is_linux_string_cst() const;

  const string&
  get_name() const;

  void
  set_name(const string& n);

  type
  get_type() const;

  void
  set_type(type t);

  size_t
  get_size() const;

  void
  set_size(size_t);

  binding
  get_binding() const;

  void
  set_binding(binding b);

  version&
  get_version() const;

  void
  set_version(const version& v);

  void
  set_visibility(visibility v);

  visibility
  get_visibility() const;

  bool
  is_defined() const;

  void
  is_defined(bool d);

  bool
  is_public() const;

  bool
  is_function() const;

  bool
  is_variable() const;

  bool
  is_in_ksymtab() const;

  void
  set_is_in_ksymtab(bool is_in_ksymtab);

  uint64_t
  get_crc() const;

  void
  set_crc(uint64_t crc);

  bool
  is_suppressed() const;

  void
  set_is_suppressed(bool is_suppressed);

  const elf_symbol_sptr
  get_main_symbol() const;

  elf_symbol_sptr
  get_main_symbol();

  bool
  is_main_symbol() const;

  elf_symbol_sptr
  update_main_symbol(const std::string&);

  elf_symbol_sptr
  get_next_alias() const;

  bool
  has_aliases() const;

  int
  get_number_of_aliases() const;

  void
  add_alias(const elf_symbol_sptr&);

  bool
  is_common_symbol() const;

  bool
  has_other_common_instances() const;

  elf_symbol_sptr
  get_next_common_instance() const;

  void
  add_common_instance(const elf_symbol_sptr&);

  const string&
  get_id_string() const;

  elf_symbol_sptr
  get_alias_from_name(const string& name) const;

  elf_symbol_sptr
  get_alias_which_equals(const elf_symbol& other) const;

  string
  get_aliases_id_string(const string_elf_symbols_map_type& symtab,
			bool include_symbol_itself = true) const;

  string
  get_aliases_id_string(bool include_symbol_itself = true) const;

  static bool
  get_name_and_version_from_id(const string& id,
			       string& name,
			       string& ver);

  bool
  operator==(const elf_symbol&) const;

  bool
  does_alias(const elf_symbol&) const;
}; // end class elf_symbol.

std::ostream&
operator<<(std::ostream& o, elf_symbol::type t);

std::ostream&
operator<<(std::ostream& o, elf_symbol::binding t);

bool
string_to_elf_symbol_type(const string&, elf_symbol::type&);

bool
string_to_elf_symbol_binding(const string&, elf_symbol::binding&);

bool
string_to_elf_symbol_visibility(const string&, elf_symbol::visibility&);

bool
elf_symbol_is_function(elf_symbol::type);

bool
elf_symbol_is_variable(elf_symbol::type);

bool
operator==(const elf_symbol_sptr& lhs, const elf_symbol_sptr& rhs);

bool
operator!=(const elf_symbol_sptr& lhs, const elf_symbol_sptr& rhs);

bool
elf_symbols_alias(const elf_symbol& s1, const elf_symbol& s2);

void
compute_aliases_for_elf_symbol(const elf_symbol& symbol,
			       const string_elf_symbols_map_type& symtab,
			       vector<elf_symbol_sptr>& alias_set);

/// The abstraction of the version of an ELF symbol.
class elf_symbol::version
{
  struct priv;
  shared_ptr<priv> priv_;

public:
  version();

  version(const string& v,
	  bool is_default);

  version(const version& v);

  operator const string&() const;

  const string&
  str() const;

  void
  str(const string& s);

  bool
  is_default() const;

  void
  is_default(bool f);

  bool
  is_empty() const;

  bool
  operator==(const version& o) const;

  bool
  operator!=(const version& o) const;

  version&
  operator=(const version& o);
};// end class elf_symbol::version

class context_rel;
/// A convenience typedef for shared pointers to @ref context_rel
typedef shared_ptr<context_rel> context_rel_sptr;

/// The abstraction of the relationship between an entity and its
/// containing scope (its context).  That relationship can carry
/// properties like access rights (if the parent is a class_decl),
/// etc.
///
/// But importantly, this relationship carries a pointer to the
/// actualy parent.
class context_rel
{
protected:
  scope_decl*		scope_;
  enum access_specifier access_;
  bool			is_static_;

public:
  context_rel()
    : scope_(0),
      access_(no_access),
      is_static_(false)
  {}

  context_rel(scope_decl* s)
    : scope_(s),
      access_(no_access),
      is_static_(false)
  {}

  context_rel(scope_decl* s,
	      access_specifier a,
	      bool f)
    : scope_(s),
      access_(a),
      is_static_(f)
  {}

  scope_decl*
  get_scope() const
  {return scope_;}

  access_specifier
  get_access_specifier() const
  {return access_;}

  void
  set_access_specifier(access_specifier a)
  {access_ = a;}

  bool
  get_is_static() const
  {return is_static_;}

  void
  set_is_static(bool s)
  {is_static_ = s;}

  void
  set_scope(scope_decl* s)
  {scope_ = s;}

  bool
  operator==(const context_rel& o)const
  {
    return (access_ == o.access_
	    && is_static_ == o.is_static_);
  }

  /// Inequality operator.
  ///
  /// @param o the other instance of @ref context_rel to compare the
  /// current instance against.
  ///
  /// @return true iff the current instance of @ref context_rel is
  /// different from @p o.
  bool
  operator!=(const context_rel& o) const
  {return !operator==(o);}

  virtual ~context_rel();
};// end class context_rel

/// A bitfield that gives callers of abigail::ir::equals() some
/// insight about how different two internal representation artifacts
/// are.
enum change_kind
{
  NO_CHANGE_KIND = 0,

  /// This means that a given IR artifact has a local type change.
  LOCAL_TYPE_CHANGE_KIND = 1 << 0,

  /// This means that a given IR artifact has a local non-type change.
  /// That is a change that is carried by the artifact itself, not by
  /// its type.
  LOCAL_NON_TYPE_CHANGE_KIND = 1 << 1,

  /// Testing (anding) against this mask means that a given IR artifact has
  /// local differences, with respect to the other artifact it was compared
  /// against. A local change is a change that is carried by the artifact
  /// itself (or its type), rather than by one off its sub-types.
  ALL_LOCAL_CHANGES_MASK = LOCAL_TYPE_CHANGE_KIND | LOCAL_NON_TYPE_CHANGE_KIND,

  /// This means that a given IR artifact has changes in some of its
  /// sub-types, with respect to the other artifact it was compared
  /// against.
  SUBTYPE_CHANGE_KIND = 1 << 2,
};// end enum change_kind

change_kind
operator|(change_kind, change_kind);

change_kind
operator&(change_kind, change_kind);

change_kind&
operator|=(change_kind&, change_kind);

change_kind&
operator&=(change_kind&, change_kind);

bool
maybe_compare_as_member_decls(const decl_base& l,
			      const decl_base& r,
			      change_kind* k);

bool
equals(const decl_base&, const decl_base&, change_kind*);

/// The base class of both types and declarations.
class type_or_decl_base : public ir_traversable_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  mutable priv_sptr priv_;

  type_or_decl_base();

protected:

  /// This is a bitmap type which instance is meant to contain the
  /// runtime type of a given ABI artifact.  Bits of the identifiers
  /// of the type of a given artifact as well as the types it inherits
  /// from are to be set to 1.
  enum type_or_decl_kind
  {
    ABSTRACT_TYPE_OR_DECL,
    ABSTRACT_DECL_BASE = 1,
    ABSTRACT_SCOPE_DECL = 1 << 1,
    GLOBAL_SCOPE_DECL = 1 << 2,
    NAMESPACE_DECL = 1 << 3,
    VAR_DECL = 1 << 4,
    FUNCTION_DECL = 1 << 5,
    FUNCTION_PARAMETER_DECL = 1 << 6,
    METHOD_DECL = 1 << 7,
    TEMPLATE_DECL = 1 << 8,
    ABSTRACT_TYPE_BASE = 1 << 9,
    ABSTRACT_SCOPE_TYPE_DECL = 1 << 10,
    BASIC_TYPE = 1 << 11,
    QUALIFIED_TYPE = 1 << 12,
    POINTER_TYPE = 1 << 13,
    REFERENCE_TYPE = 1 << 14,
    ARRAY_TYPE = 1 << 15,
    ENUM_TYPE = 1 << 16,
    TYPEDEF_TYPE = 1 << 17,
    CLASS_TYPE = 1 << 18,
    UNION_TYPE = 1 << 19,
    FUNCTION_TYPE = 1 << 20,
    METHOD_TYPE = 1 << 21,
  }; // end enum type_or_decl_kind

  enum type_or_decl_kind
  kind() const;

  void
  kind(enum type_or_decl_kind);

  const void*
  runtime_type_instance() const;

  void*
  runtime_type_instance();

  void
  runtime_type_instance(void*);

  const void*
  type_or_decl_base_pointer() const;

  void*
  type_or_decl_base_pointer();

  bool hashing_started() const;

  void hashing_started(bool) const;

public:

  type_or_decl_base(const environment*,
		    enum type_or_decl_kind k = ABSTRACT_TYPE_OR_DECL);

  type_or_decl_base(const type_or_decl_base&);

  virtual ~type_or_decl_base();

  const environment*
  get_environment() const;

  environment*
  get_environment();

  void
  set_environment(const environment*);

  const corpus*
  get_corpus() const;

  corpus*
  get_corpus();

  void
  set_translation_unit(translation_unit*);

  const translation_unit*
  get_translation_unit() const;

  translation_unit*
  get_translation_unit();

  type_or_decl_base&
  operator=(const type_or_decl_base&);

  virtual bool
  traverse(ir_node_visitor&);

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const = 0;

  friend type_or_decl_base::type_or_decl_kind
  operator|(type_or_decl_base::type_or_decl_kind,
	    type_or_decl_base::type_or_decl_kind);

  friend type_or_decl_base::type_or_decl_kind&
  operator|=(type_or_decl_base::type_or_decl_kind&,
	     type_or_decl_base::type_or_decl_kind);

  friend type_or_decl_base::type_or_decl_kind
  operator&(type_or_decl_base::type_or_decl_kind,
	    type_or_decl_base::type_or_decl_kind);

  friend type_or_decl_base::type_or_decl_kind&
  operator&=(type_or_decl_base::type_or_decl_kind&,
	     type_or_decl_base::type_or_decl_kind);

  friend class_decl*
  is_class_type(const type_or_decl_base*);

  friend pointer_type_def*
  is_pointer_type(type_or_decl_base*);

  friend type_base*
  is_type(const type_or_decl_base*);

  friend decl_base*
  is_decl(const type_or_decl_base* d);
}; // end class type_or_decl_base

type_or_decl_base::type_or_decl_kind
operator|(type_or_decl_base::type_or_decl_kind,
	  type_or_decl_base::type_or_decl_kind);

type_or_decl_base::type_or_decl_kind&
operator|=(type_or_decl_base::type_or_decl_kind&,
	   type_or_decl_base::type_or_decl_kind);

type_or_decl_base::type_or_decl_kind
operator&(type_or_decl_base::type_or_decl_kind,
	  type_or_decl_base::type_or_decl_kind);

type_or_decl_base::type_or_decl_kind&
operator&=(type_or_decl_base::type_or_decl_kind&,
	   type_or_decl_base::type_or_decl_kind);

bool
operator==(const type_or_decl_base&, const type_or_decl_base&);

bool
operator==(const type_or_decl_base_sptr&, const type_or_decl_base_sptr&);

bool
operator!=(const type_or_decl_base_sptr&, const type_or_decl_base_sptr&);

void
set_environment_for_artifact(type_or_decl_base* artifact,
			     const environment* env);

void
set_environment_for_artifact(type_or_decl_base_sptr artifact,
			     const environment* env);

/// The base type of all declarations.
class decl_base : public virtual type_or_decl_base
{
  // Forbidden
  decl_base();

  struct priv;

protected:

  const interned_string&
  peek_qualified_name() const;

  void
  clear_qualified_name();

  void
  set_qualified_name(const interned_string&) const;

  const interned_string&
  peek_temporary_qualified_name() const;

  void
  set_temporary_qualified_name(const interned_string&) const;

public:
  // This is public because some internals of the library need to
  // update it.  But it's opaque to client code anyway, so no big
  // deal.  Also, it's not handled by a shared_ptr because accessing
  // the data members of the priv struct for this decl_base shows up
  // on performance profiles when dealing with big binaries with a lot
  // of types; dereferencing the shared_ptr involves locking of some
  // sort and that is slower than just dereferencing a pointer likere
  // here.  There are other types for which the priv pointer is
  // managed using shared_ptr just fine, because those didn't show up
  // during our performance profiling.
  priv* priv_;

  /// Facility to hash instances of decl_base.
  struct hash;

  /// ELF visibility
  enum visibility
  {
    VISIBILITY_NONE,
    VISIBILITY_DEFAULT,
    VISIBILITY_PROTECTED,
    VISIBILITY_HIDDEN,
    VISIBILITY_INTERNAL
  };

  /// ELF binding
  enum binding
  {
    BINDING_NONE,
    BINDING_LOCAL,
    BINDING_GLOBAL,
    BINDING_WEAK
  };

  virtual void
  set_scope(scope_decl*);

protected:
  const context_rel*
  get_context_rel() const;

  context_rel*
  get_context_rel();

  void
  set_context_rel(context_rel *c);

public:
  decl_base(const environment* e,
	    const string& name,
	    const location& locus,
	    const string& mangled_name = "",
	    visibility vis = VISIBILITY_DEFAULT);

  decl_base(const environment* e,
	    const interned_string& name,
	    const location& locus,
	    const interned_string& mangled_name = interned_string(),
	    visibility vis = VISIBILITY_DEFAULT);

  decl_base(const environment*, const location&);

  decl_base(const decl_base&);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator!=(const decl_base&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~decl_base();

  virtual size_t
  get_hash() const;

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  virtual void
  get_qualified_name(interned_string& qualified_name,
		     bool internal = false) const;

  virtual const interned_string&
  get_qualified_name(bool internal = false) const;

  virtual const interned_string&
  get_scoped_name() const;

  bool
  get_is_in_public_symbol_table() const;

  void
  set_is_in_public_symbol_table(bool);

  const location&
  get_location() const;

  void
  set_location(const location& l);

  const interned_string&
  get_name() const;

  const interned_string&
  get_qualified_parent_name() const;

  void
  set_name(const string& n);

  bool
  get_is_anonymous() const;

  void
  set_is_anonymous(bool);

  bool
  get_is_artificial() const;

  void
  set_is_artificial(bool);

  bool
  get_has_anonymous_parent() const;

  void
  set_has_anonymous_parent(bool f) const;

  bool
  get_is_anonymous_or_has_anonymous_parent() const;

  const interned_string&
  get_linkage_name() const;

  virtual void
  set_linkage_name(const string& m);

  scope_decl*
  get_scope() const;

  visibility
  get_visibility() const;

  void
  set_visibility(visibility v);

  const decl_base_sptr
  get_earlier_declaration() const;

  void
  set_earlier_declaration(const decl_base_sptr&);

  const decl_base_sptr
  get_definition_of_declaration() const;

  void
  set_definition_of_declaration(const decl_base_sptr&);

  const decl_base*
  get_naked_definition_of_declaration() const;

  bool
  get_is_declaration_only() const;

  void
  set_is_declaration_only(bool f);

  friend type_base_sptr
  canonicalize(type_base_sptr);

  friend bool
  equals(const decl_base&, const decl_base&, change_kind*);

  friend bool
  equals(const var_decl&, const var_decl&, change_kind*);

  friend bool
  maybe_compare_as_member_decls(const decl_base& l,
				const decl_base& r,
				change_kind* k);

  friend decl_base_sptr
  add_decl_to_scope(decl_base_sptr decl, scope_decl* scpe);

  friend void
  remove_decl_from_scope(decl_base_sptr);

  friend decl_base_sptr
  insert_decl_into_scope(decl_base_sptr,
			 vector<shared_ptr<decl_base> >::iterator,
			 scope_decl*);

  friend enum access_specifier
  get_member_access_specifier(const decl_base& d);

  friend enum access_specifier
  get_member_access_specifier(const decl_base_sptr& d);

  friend void
  set_member_access_specifier(decl_base& d,
			      access_specifier a);

  friend bool
  get_member_is_static(const decl_base& d);

  friend bool
  get_member_is_static(const decl_base_sptr& d);

  friend void
  set_member_is_static(const decl_base_sptr& d, bool s);

  friend void
  set_member_is_static(decl_base& d, bool s);

  friend bool
  get_member_function_is_virtual(const function_decl& f);

  friend void
  set_member_function_is_virtual(function_decl&, bool);

  friend class class_or_union;
  friend class class_decl;
  friend class scope_decl;
};// end class decl_base

bool
operator==(const decl_base_sptr&, const decl_base_sptr&);

bool
operator!=(const decl_base_sptr&, const decl_base_sptr&);

bool
operator==(const type_base_sptr&, const type_base_sptr&);

bool
operator!=(const type_base_sptr&, const type_base_sptr&);

std::ostream&
operator<<(std::ostream&, decl_base::visibility);

std::ostream&
operator<<(std::ostream&, decl_base::binding);

bool
equals(const scope_decl&, const scope_decl&, change_kind*);

/// A declaration that introduces a scope.
class scope_decl : public virtual decl_base
{
public:

  /// Convenience typedef for a vector of @ref decl_base_sptr.
  typedef std::vector<decl_base_sptr >	declarations;
  /// Convenience typedef for a vector of @ref function_type_sptr.
  typedef std::vector<function_type_sptr >	function_types;
  /// Convenience typedef for a vector of @ref scope_decl_sptr.
  typedef std::vector<scope_decl_sptr>	scopes;
  /// The type of the private data of @ref scope_decl.
  struct priv;
  /// A convenience typedef for a shared pointer to scope_decl::priv.
  typedef shared_ptr<priv> priv_sptr;

private:
  priv_sptr priv_;

  scope_decl();

protected:
  virtual decl_base_sptr
  add_member_decl(const decl_base_sptr& member);

  virtual decl_base_sptr
  insert_member_decl(decl_base_sptr member, declarations::iterator before);

  virtual void
  remove_member_decl(decl_base_sptr member);

public:
  struct hash;

  scope_decl(const environment* env,
	     const string& name, const location& locus,
	     visibility	vis = VISIBILITY_DEFAULT);

  scope_decl(const environment* env, location& l);

  virtual size_t
  get_hash() const;

  virtual bool
  operator==(const decl_base&) const;

  const canonical_type_sptr_set_type&
  get_canonical_types() const;

  canonical_type_sptr_set_type&
  get_canonical_types();

  const type_base_sptrs_type&
  get_sorted_canonical_types() const;

  const declarations&
  get_member_decls() const;

  declarations&
  get_member_decls();

  const declarations&
  get_sorted_member_decls() const;

  virtual size_t
  get_num_anonymous_member_classes() const;

  virtual size_t
  get_num_anonymous_member_unions() const;

  virtual size_t
  get_num_anonymous_member_enums() const;

  scopes&
  get_member_scopes();

  const scopes&
  get_member_scopes() const;

  bool
  is_empty() const;

  bool
  find_iterator_for_member(const decl_base*, declarations::iterator&);

  bool
  find_iterator_for_member(const decl_base_sptr, declarations::iterator&);

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~scope_decl();

  friend decl_base_sptr
  add_decl_to_scope(decl_base_sptr decl, scope_decl* scope);

  friend decl_base_sptr
  insert_decl_into_scope(decl_base_sptr decl,
			 scope_decl::declarations::iterator before,
			 scope_decl* scope);

  friend void
  remove_decl_from_scope(decl_base_sptr decl);

  friend type_base_sptr
  canonicalize(type_base_sptr);
};//end class scope_decl

bool
operator==(const scope_decl_sptr&, const scope_decl_sptr&);

bool
operator!=(const scope_decl_sptr&, const scope_decl_sptr&);

/// Hasher for the @ref scope_decl type.
struct scope_decl::hash
{
  size_t
  operator()(const scope_decl& d) const;

  size_t
  operator()(const scope_decl* d) const;
};

/// This abstracts the global scope of a given translation unit.
///
/// Only one instance of this class must be present in a given
/// translation_unit.  That instance is implicitely created the first
/// time translatin_unit::get_global_scope is invoked.
class global_scope : public scope_decl
{
  translation_unit* translation_unit_;

  global_scope(translation_unit *tu);

public:

  friend class translation_unit;

  translation_unit*
  get_translation_unit() const
  {return translation_unit_;}

  virtual ~global_scope();
};

bool
equals(const type_base&, const type_base&, change_kind*);

/// An abstraction helper for type declarations
class type_base : public virtual type_or_decl_base
{
  struct priv;

public:
  // This priv pointer is not handled by a shared_ptr because
  // accessing the data members of the priv struct for this type_base
  // shows up on performance profiles when dealing with big binaries
  // with a lot of types; dereferencing the shared_ptr involves
  // locking of some sort and that is slower than just dereferencing a
  // pointer likere here.  There are other types for which the priv
  // pointer is managed using shared_ptr just fine, because those
  // didn't show up during our performance profiling.
  priv* priv_;

private:
  // Forbid this.
  type_base();

  static type_base_sptr
  get_canonical_type_for(type_base_sptr);

protected:
  virtual void
  on_canonical_type_set();

public:

  /// A hasher for type_base types.
  struct hash;

  /// A hasher for types.  It gets the dynamic type of the current
  /// instance of type and hashes it accordingly.  Note that the hashing
  /// function of this hasher must be updated each time a new kind of
  /// type is added to the IR.
  struct dynamic_hash;

  /// A hasher for shared_ptr<type_base> that will hash it based on the
  /// runtime type of the type pointed to.
  struct shared_ptr_hash;

  type_base(const environment* e, size_t s, size_t a);

  friend type_base_sptr canonicalize(type_base_sptr);

  type_base_sptr
  get_canonical_type() const;

  type_base*
  get_naked_canonical_type() const;

  const interned_string&
  get_cached_pretty_representation(bool internal = false) const;

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator!=(const type_base&) const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~type_base();

  virtual void
  set_size_in_bits(size_t);

  virtual size_t
  get_size_in_bits() const;

  virtual void
  set_alignment_in_bits(size_t);

  virtual size_t
  get_alignment_in_bits() const;
};//end class type_base

/// Hash functor for instances of @ref type_base.
struct type_base::hash
{
  size_t
  operator()(const type_base& t) const;

  size_t
  operator()(const type_base* t) const;

  size_t
  operator()(const type_base_sptr t) const;
}; // end struct type_base::hash

/// A predicate for deep equality of instances of
/// type_base*
struct type_ptr_equal
{
  bool
  operator()(const type_base* l, const type_base* r) const
  {
    if (!!l != !!r)
      return false;

    if (l == r)
      return true;

    if (l)
      return *l == *r;

    return true;
  }
};

/// A predicate for deep equality of instances of
/// shared_ptr<type_base>
struct type_shared_ptr_equal
{
  bool
  operator()(const type_base_sptr l, const type_base_sptr r) const
  {
    if (!!l != !!r)
      return false;

    if (l.get() == r.get())
      return true;

    if (l)
      return *l == *r;

    return true;
  }
};

bool
equals(const type_decl&, const type_decl&, change_kind*);

/// A basic type declaration that introduces no scope.
class type_decl : public virtual decl_base, public virtual type_base
{
  // Forbidden.
  type_decl();

public:

  /// Facility to hash instance of type_decl
  struct hash;

  type_decl(const environment*	env,
	    const string&	name,
	    size_t		size_in_bits,
	    size_t		alignment_in_bits,
	    const location&	locus,
	    const string&	mangled_name = "",
	    visibility		vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_decl&) const;

  bool operator!=(const type_decl&)const;

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~type_decl();
};// end class type_decl.

bool
equals(const scope_type_decl&, const scope_type_decl&, change_kind*);

bool
operator==(const type_decl_sptr&, const type_decl_sptr&);

bool
operator!=(const type_decl_sptr&, const type_decl_sptr&);

/// A type that introduces a scope.
class scope_type_decl : public scope_decl, public virtual type_base
{
  scope_type_decl();

public:

  /// Hasher for instances of scope_type_decl
  struct hash;

  scope_type_decl(const environment* env, const string& name,
		  size_t size_in_bits, size_t alignment_in_bits,
		  const location& locus, visibility vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~scope_type_decl();
};

/// The abstraction of a namespace declaration
class namespace_decl : public scope_decl
{
public:

  namespace_decl(const environment* env, const string& name,
		 const location& locus, visibility vis = VISIBILITY_DEFAULT);

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~namespace_decl();

  bool is_empty_or_has_empty_sub_namespaces() const;
};// end class namespace_decl

bool
equals(const qualified_type_def&, const qualified_type_def&, change_kind*);

/// The abstraction of a qualified type.
class qualified_type_def : public virtual type_base, public virtual decl_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  // Forbidden.
  qualified_type_def();

protected:
  string build_name(bool, bool internal = false) const;
  virtual void on_canonical_type_set();

public:

  /// A Hasher for instances of qualified_type_def
  struct hash;

  /// Bit field values representing the cv qualifiers of the
  /// underlying type.
  enum CV
  {
    CV_NONE = 0,
    CV_CONST = 1,
    CV_VOLATILE = 1 << 1,
    CV_RESTRICT = 1 << 2
  };

  qualified_type_def(type_base_sptr type, CV quals, const location& locus);

  virtual size_t
  get_size_in_bits() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const qualified_type_def&) const;

  CV
  get_cv_quals() const;

  void
  set_cv_quals(CV cv_quals);

  string
  get_cv_quals_string_prefix() const;

  type_base_sptr
  get_underlying_type() const;

  void
  set_underlying_type(const type_base_sptr&);

  virtual void
  get_qualified_name(interned_string& qualified_name,
		     bool internal = false) const;

  virtual const interned_string&
  get_qualified_name(bool internal = false) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~qualified_type_def();
}; // end class qualified_type_def.

bool
operator==(const qualified_type_def_sptr&, const qualified_type_def_sptr&);

bool
operator!=(const qualified_type_def_sptr&, const qualified_type_def_sptr&);

qualified_type_def::CV
operator|(qualified_type_def::CV, qualified_type_def::CV);

qualified_type_def::CV&
operator|=(qualified_type_def::CV&, qualified_type_def::CV);

qualified_type_def::CV
operator&(qualified_type_def::CV, qualified_type_def::CV);

qualified_type_def::CV
operator~(qualified_type_def::CV);

std::ostream&
operator<<(std::ostream&, qualified_type_def::CV);

string
get_string_representation_of_cv_quals(const qualified_type_def::CV);

interned_string
get_name_of_qualified_type(const type_base_sptr& underlying_type,
			   qualified_type_def::CV quals,
			   bool qualified = true, bool internal = false);

qualified_type_def_sptr
lookup_qualified_type(const type_base_sptr&,
		      qualified_type_def::CV,
		      const translation_unit&);
bool
equals(const pointer_type_def&, const pointer_type_def&, change_kind*);

/// The abstraction of a pointer type.
class pointer_type_def : public virtual type_base, public virtual decl_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden.
  pointer_type_def();

protected:
  virtual void on_canonical_type_set();

public:

  /// A hasher for instances of pointer_type_def
  struct hash;

  pointer_type_def(const type_base_sptr& pointed_to_type, size_t size_in_bits,
		   size_t alignment_in_bits, const location& locus);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  bool
  operator==(const pointer_type_def&) const;

  const type_base_sptr
  get_pointed_to_type() const;

  type_base*
  get_naked_pointed_to_type() const;

  virtual void
  get_qualified_name(interned_string&, bool internal = false) const;

  virtual const interned_string&
  get_qualified_name(bool internal = false) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~pointer_type_def();
}; // end class pointer_type_def

bool
operator==(const pointer_type_def_sptr&, const pointer_type_def_sptr&);

bool
operator!=(const pointer_type_def_sptr&, const pointer_type_def_sptr&);

bool
equals(const reference_type_def&, const reference_type_def&, change_kind*);


/// Abstracts a reference type.
class reference_type_def : public virtual type_base, public virtual decl_base
{
  type_base_wptr	pointed_to_type_;
  bool			is_lvalue_;

  // Forbidden.
  reference_type_def();

protected:
  virtual void on_canonical_type_set();

public:

  /// Hasher for intances of reference_type_def.
  struct hash;

  reference_type_def(const type_base_sptr pointed_to_type,
		     bool lvalue, size_t size_in_bits,
		     size_t alignment_in_bits, const location& locus);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  bool
  operator==(const reference_type_def&) const;

  type_base_sptr
  get_pointed_to_type() const;

  bool
  is_lvalue() const;

  virtual void
  get_qualified_name(interned_string& qualified_name,
		     bool internal = false) const;

  virtual const interned_string&
  get_qualified_name(bool internal = false) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~reference_type_def();
}; // end class reference_type_def

bool
operator==(const reference_type_def_sptr&, const reference_type_def_sptr&);

bool
operator!=(const reference_type_def_sptr&, const reference_type_def_sptr&);

bool
equals(const array_type_def&, const array_type_def&, change_kind*);

/// The abstraction of an array type.
class array_type_def : public virtual type_base, public virtual decl_base
{
private:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  // Forbidden.
  array_type_def();

public:

  /// Hasher for intances of array_type_def.
  struct hash;

  class subrange_type;

  /// Convenience typedef for a shared pointer on a @ref
  /// function_decl::subrange
  typedef shared_ptr<subrange_type> subrange_sptr;

  /// Convenience typedef for a vector of @ref subrange_sptr
  typedef std::vector<subrange_sptr> subranges_type;

  /// Abstraction for an array range type, like in Ada, or just for an
  /// array dimension like in C or C++.
  class subrange_type : public virtual type_base,  public virtual decl_base
  {
    struct priv;
    typedef shared_ptr<priv> priv_sptr;
    priv_sptr priv_;

    // Forbidden.
    subrange_type();
  public:

    /// This class is to hold the value of the bound of a subrange.
    /// The value can be either signed or unsigned, at least when it
    /// comes from DWARF.  The class keeps the sign information, but
    /// allows users to access the value as signed or unsigned as they
    /// see fit.
    class bound_value
    {
    public:
      enum signedness
      {
	UNSIGNED_SIGNEDNESS,
	SIGNED_SIGNEDNESS
      };

    private:
      signedness s_;

    public:
      union
      {
	uint64_t unsigned_;
	int64_t signed_;
      } v_;
      bound_value();
      bound_value(uint64_t);
      bound_value(int64_t);
      enum signedness get_signedness() const;
      void set_signedness(enum signedness s);
      int64_t get_signed_value() const;
      uint64_t get_unsigned_value();
      void set_unsigned(uint64_t v);
      void set_signed(int64_t v);
      bool operator==(const bound_value&) const;
    }; //end class bound_value

    /// Hasher for an instance of array::subrange
    struct hash;

    subrange_type(const environment*	env,
		  const string&	name,
		  bound_value		lower_bound,
		  bound_value		upper_bound,
		  const type_base_sptr& underlying_type,
		  const location&	loc,
		  translation_unit::language l = translation_unit::LANG_C11);

    subrange_type(const environment* env,
		  const string& name,
		  bound_value lower_bound,
		  bound_value upper_bound,
		  const location& loc,
		  translation_unit::language l = translation_unit::LANG_C11);

    subrange_type(const environment* env,
		  const string& name,
		  bound_value upper_bound,
		  const location& loc,
		  translation_unit::language l = translation_unit::LANG_C11);

    type_base_sptr
    get_underlying_type() const;

    void
    set_underlying_type(const type_base_sptr &);

    int64_t
    get_upper_bound() const;

    int64_t
    get_lower_bound() const;

    void
    set_upper_bound(int64_t ub);

    void
    set_lower_bound(int64_t lb);

    uint64_t
    get_length() const;

    bool
    is_infinite() const;

    void
    is_infinite(bool);

    translation_unit::language
    get_language() const;

    virtual bool
    operator==(const decl_base&) const;

    virtual bool
    operator==(const type_base&) const;

    bool
    operator==(const subrange_type& o) const;

    bool
    operator!=(const subrange_type& o) const;

    string
    as_string() const;

    static string
    vector_as_string(const vector<subrange_sptr>&);

    virtual string
    get_pretty_representation(bool internal = false,
			      bool qualified_name = true) const;

    virtual bool
    traverse(ir_node_visitor&);
  }; // end class subrange_type

  array_type_def(const type_base_sptr type,
		 const std::vector<subrange_sptr>& subs,
		 const location& locus);

  translation_unit::language
  get_language() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual void
  get_qualified_name(interned_string& qualified_name,
		     bool internal = false) const;

  virtual const interned_string&
  get_qualified_name(bool internal = false) const;

  const type_base_sptr
  get_element_type() const;

  void
  set_element_type(const type_base_sptr& element_type);

  virtual void
  append_subranges(const std::vector<subrange_sptr>& subs);

  virtual int
  get_dimension_count() const;

  virtual bool
  is_infinite() const;

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  virtual string
  get_subrange_representation() const;

  virtual bool
  traverse(ir_node_visitor& v);

  const location&
  get_location() const;

  const std::vector<subrange_sptr>&
  get_subranges() const;

  virtual ~array_type_def();

}; // end class array_type_def

array_type_def::subrange_type*
is_subrange_type(const type_or_decl_base *type);

array_type_def::subrange_sptr
is_subrange_type(const type_or_decl_base_sptr &type);

bool
equals(const enum_type_decl&, const enum_type_decl&, change_kind*);

/// Abstracts a declaration for an enum type.
class enum_type_decl : public virtual type_base, public virtual decl_base
{
public:

  /// A hasher for an enum_type_decl.
  struct hash;

  /// Enumerator Datum.
  class enumerator;

  /// Convenience typedef for a list of @ref enumerator.
  typedef std::vector<enumerator> enumerators;

private:

  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  enum_type_decl();

public:

  /// Constructor of an enum type declaration.
  ///
  /// @param name the name of the enum
  ///
  /// @param locus the locus at which the enum appears in the source
  /// code.
  ///
  /// @param underlying_type the underlying type of the enum
  ///
  /// @param enms a list of enumerators for this enum.
  ///
  /// @param mangled_name the mangled name of the enum type.
  ///
  /// @param vis the visibility of instances of this type.
  enum_type_decl(const string&		name,
		 const location&	locus,
		 type_base_sptr	underlying_type,
		 enumerators&		enms,
		 const string&		mangled_name = "",
		 visibility		vis = VISIBILITY_DEFAULT);

  type_base_sptr
  get_underlying_type() const;

  const enumerators&
  get_enumerators() const;

  enumerators&
  get_enumerators();

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~enum_type_decl();

  friend bool
  enum_has_non_name_change(const enum_type_decl& l,
			   const enum_type_decl& r,
			   change_kind* k);
}; // end class enum_type_decl

bool
operator==(const enum_type_decl_sptr& l, const enum_type_decl_sptr& r);

bool
operator!=(const enum_type_decl_sptr& l, const enum_type_decl_sptr& r);

bool
enum_has_non_name_change(const enum_type_decl& l,
			 const enum_type_decl& r,
			 change_kind* k);

/// The abstraction of an enumerator
class enum_type_decl::enumerator
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;


public:

  enumerator();

  enumerator(const environment* env, const string& name, int64_t value);

  enumerator(const enumerator&);

  enumerator&
  operator=(const enumerator&);

  bool
  operator==(const enumerator& other) const;

  bool
  operator!=(const enumerator& other) const;

  const environment*
  get_environment() const;

  const interned_string&
  get_name() const;

  const interned_string&
  get_qualified_name(bool internal = false) const;

  void
  set_name(const string& n);

  int64_t
  get_value() const;

  void
  set_value(int64_t v);

  enum_type_decl*
  get_enum_type() const;

  void
  set_enum_type(enum_type_decl*);
}; // end class enum_type_def::enumerator

bool
equals(const typedef_decl&, const typedef_decl&, change_kind*);

/// The abstraction of a typedef declaration.
class typedef_decl : public virtual type_base, public virtual decl_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  typedef_decl();

public:

  /// Hasher for the typedef_decl type.
  struct hash;

  typedef_decl(const string& name,
	       const type_base_sptr underlying_type,
	       const location& locus,
	       const string& mangled_name = "",
	       visibility vis = VISIBILITY_DEFAULT);

  virtual size_t
  get_size_in_bits() const;

  virtual size_t
  get_alignment_in_bits() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  type_base_sptr
  get_underlying_type() const;

  void
  set_underlying_type(const type_base_sptr&);

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~typedef_decl();
};// end class typedef_decl

/// The abstraction for a data member context relationship.  This
/// relates a data member to its parent class.
///
/// The relationship carries properties like the offset of the data
/// member, if applicable.
class dm_context_rel : public context_rel
{
protected:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:
  dm_context_rel();

  dm_context_rel(scope_decl* s,
		 bool is_laid_out,
		 size_t offset_in_bits,
		 access_specifier a,
		 bool is_static);

  dm_context_rel(scope_decl* s);

  bool
  get_is_laid_out() const;

  void
  set_is_laid_out(bool f);

  size_t
  get_offset_in_bits() const;

  void
  set_offset_in_bits(size_t o);

  const var_decl*
  get_anonymous_data_member() const;

  void
  set_anonymous_data_member(var_decl *);

  bool
  operator==(const dm_context_rel& o) const;

  bool
  operator!=(const dm_context_rel& o) const;

  virtual ~dm_context_rel();
};// end class class_decl::dm_context_rel

bool
equals(const var_decl&, const var_decl&, change_kind*);

bool
equals_modulo_cv_qualifier(const array_type_def*, const array_type_def*);

/// Abstracts a variable declaration.
class var_decl : public virtual decl_base
{
  struct priv;
  shared_ptr<priv> priv_;

  // Forbidden
  var_decl();

  virtual void
  set_scope(scope_decl*);

public:

  /// Hasher for a var_decl type.
  struct hash;

  /// Equality functor to compare pointers to variable_decl.
  struct ptr_equal;

  var_decl(const string&	name,
	   type_base_sptr	type,
	   const location&	locus,
	   const string&	mangled_name,
	   visibility		vis = VISIBILITY_DEFAULT,
	   binding		bind = BINDING_NONE);

  virtual bool
  operator==(const decl_base&) const;

  const type_base_sptr
  get_type() const;

  const type_base*
  get_naked_type() const;

  binding
  get_binding() const;

  void
  set_binding(binding b);

  void
  set_symbol(const elf_symbol_sptr& sym);

  const elf_symbol_sptr&
  get_symbol() const;

  var_decl_sptr
  clone() const;

  interned_string
  get_id() const;

  virtual const interned_string&
  get_qualified_name(bool internal = false) const;

  virtual size_t
  get_hash() const;

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  string
  get_anon_dm_reliable_name(bool qualified = true) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~var_decl();

  friend void
  set_data_member_offset(var_decl_sptr m, uint64_t o);

  friend uint64_t
  get_data_member_offset(const var_decl_sptr m);

  friend uint64_t
  get_data_member_offset(const var_decl& m);

  friend uint64_t
  get_absolute_data_member_offset(const var_decl& m);

  friend uint64_t
  get_absolute_data_member_offset(const var_decl_sptr& m);

  friend void
  set_data_member_is_laid_out(var_decl_sptr m, bool l);

  friend bool
  get_data_member_is_laid_out(const var_decl& m);

  friend bool
  get_data_member_is_laid_out(const var_decl_sptr m);
}; // end class var_decl

bool
equals(const function_decl&, const function_decl&, change_kind*);

/// Abstraction for a function declaration.
class function_decl : public virtual decl_base
{
  struct priv;
  // This priv pointer is not handled by a shared_ptr because
  // accessing the data members of the priv struct for this
  // function_decl shows up on performance profiles when dealing with
  // big binaries with a lot of types; dereferencing the shared_ptr
  // involves locking of some sort and that is slower than just
  // dereferencing a pointer likere here.  There are other types for
  // which the priv pointer is managed using shared_ptr just fine,
  // because those didn't show up during our performance profiling.
  priv* priv_;

public:
  /// Hasher for function_decl
  struct hash;

  /// Equality functor to compare pointers to function_decl
  struct ptr_equal;

  /// Abstraction for the parameter of a function.
  class parameter;

  /// Convenience typedef for a shared pointer on a @ref
  /// function_decl::parameter
  typedef shared_ptr<parameter> parameter_sptr;

  /// Convenience typedef for a vector of @ref parameter_sptr
  typedef std::vector<parameter_sptr> parameters;

  function_decl(const string& name,
		function_type_sptr function_type,
		bool declared_inline,
		const location& locus,
		const string& mangled_name,
		visibility vis,
		binding bind);

  function_decl(const string& name,
		type_base_sptr fn_type,
		bool declared_inline,
		const location& locus,
		const string& mangled_name = "",
		visibility vis = VISIBILITY_DEFAULT,
		binding bind = BINDING_GLOBAL);

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  string
  get_pretty_representation_of_declarator (bool internal = false) const;

  const std::vector<parameter_sptr >&
  get_parameters() const;

  void
  append_parameter(parameter_sptr parm);

  void
  append_parameters(std::vector<parameter_sptr >& parms);

  parameters::const_iterator
  get_first_non_implicit_parm() const;

  const function_type_sptr
  get_type() const;

  const function_type*
  get_naked_type() const;

  const type_base_sptr
  get_return_type() const;

  void
  set_type(const function_type_sptr& fn_type);

  void
  set_symbol(const elf_symbol_sptr& sym);

  const elf_symbol_sptr&
  get_symbol() const;

  bool
  is_declared_inline() const;

  binding
  get_binding() const;

  function_decl_sptr
  clone() const;

  virtual bool
  operator==(const decl_base& o) const;

  /// Return true iff the function takes a variable number of
  /// parameters.
  ///
  /// @return true if the function taks a variable number
  /// of parameters.
  bool
  is_variadic() const;

  virtual size_t
  get_hash() const;

  interned_string
  get_id() const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~function_decl();
}; // end class function_decl

bool
operator==(const function_decl_sptr& l, const function_decl_sptr& r);

bool
operator!=(const function_decl_sptr& l, const function_decl_sptr& r);

bool
function_decls_alias(const function_decl& f1, const function_decl& f2);

bool
equals(const function_decl::parameter&,
       const function_decl::parameter&,
       change_kind*);

/// A comparison functor to compare pointer to instances of @ref
/// type_or_decl_base.
struct type_or_decl_base_comp
{
  /// Comparison operator for ABI artifacts.
  ///
  /// @param f the first ABI artifact to consider for the comparison.
  ///
  /// @param s the second  ABI artifact to consider for the comparison.
  ///
  /// @return true iff @p f is lexicographically less than than @p s.
  bool
  operator()(const type_or_decl_base *f,
	     const type_or_decl_base *s)
  {
    function_decl *f_fn = is_function_decl(f), *s_fn = is_function_decl(s);
    if (f_fn && s_fn)
      return function_decl_is_less_than(*f_fn, *s_fn);

    var_decl *f_var = is_var_decl(f), *s_var = is_var_decl(s);
    if (f_var && s_var)
      return get_name(f_var) < get_name(s_var);

    string l_repr = get_pretty_representation(f),
      r_repr = get_pretty_representation(s);

    return l_repr < r_repr;
  }

  /// Comparison operator for ABI artifacts.
  ///
  /// @param f the first ABI artifact to consider for the comparison.
  ///
  /// @param s the second  ABI artifact to consider for the comparison.
  ///
  /// @return true iff @p f is lexicographically less than than @p s.
  bool
  operator()(const type_or_decl_base_sptr& f,
	     const type_or_decl_base_sptr& s)
  {return operator()(f.get(), s.get());}
}; // end struct type_or_decl_base_comp

/// Abstraction of a function parameter.
class function_decl::parameter : public decl_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:

  /// Hasher for an instance of function::parameter
  struct hash;

  parameter(const type_base_sptr	type,
	    unsigned			index,
	    const string&		name,
	    const location&		loc,
	    bool			variadic_marker = false);

  parameter(const type_base_sptr	type,
	    unsigned			index,
	    const string&		name,
	    const location&		loc,
	    bool			variadic_marker,
	    bool			is_artificial);

  parameter(const type_base_sptr	type,
	    const string&		name,
	    const location&		loc,
	    bool			variadic_marker = false,
	    bool			is_artificial	= false);

  parameter(const type_base_sptr	type,
	    unsigned			index = 0,
	    bool			variadic_marker = false);

  const type_base_sptr
  get_type()const;

  interned_string
  get_type_name() const;

  const string
  get_type_pretty_representation() const;

  interned_string
  get_name_id() const;

  unsigned
  get_index() const;

  void
  set_index(unsigned i);

  bool
  get_variadic_marker() const;

  bool
  operator==(const parameter& o) const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual size_t
  get_hash() const;

  virtual void
  get_qualified_name(interned_string& qualified_name,
		     bool internal = false) const;

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;
}; // end class function_decl::parameter

bool
operator==(const function_decl::parameter_sptr&,
	   const function_decl::parameter_sptr&);

/// A hashing functor for a function_decl::parameter.
struct function_decl::parameter::hash
{
  size_t
  operator()(const function_decl::parameter&) const;

  size_t
  operator()(const function_decl::parameter*) const;

  size_t
  operator()(const function_decl::parameter_sptr) const;
}; // end struct function_decl::parameter::hash

function_decl::parameter*
is_function_parameter(const type_or_decl_base*);

function_decl::parameter_sptr
is_function_parameter(const type_or_decl_base_sptr tod);

bool
equals(const function_type&, const function_type&, change_kind*);

/// Abstraction of a function type.
class function_type : public virtual type_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

protected:
  virtual void on_canonical_type_set();

public:
  /// Hasher for an instance of function_type
  struct hash;

  /// Convenience typedef for a shared pointer on a @ref
  /// function_decl::parameter
  typedef shared_ptr<function_decl::parameter>	parameter_sptr;
  /// Convenience typedef for a vector of @ref parameter_sptr
  typedef std::vector<parameter_sptr>		parameters;

  priv_sptr priv_;

private:
  function_type();

public:

  function_type(type_base_sptr		return_type,
		const parameters&	parms,
		size_t			size_in_bits,
		size_t			alignment_in_bits);

  function_type(type_base_sptr	return_type,
		size_t		size_in_bits,
		size_t		alignment_in_bits);

  function_type(const environment*	env,
		size_t		size_in_bits,
		size_t		alignment_in_bits);

  type_base_sptr
  get_return_type() const;

  void
  set_return_type(type_base_sptr t);

  const parameters&
  get_parameters() const;

  const parameter_sptr
  get_parm_at_index_from_first_non_implicit_parm(size_t) const;

  void
  set_parameters(const parameters &p);

  void
  append_parameter(parameter_sptr parm);

  bool
  is_variadic() const;

  parameters::const_iterator
  get_first_non_implicit_parm() const;

  parameters::const_iterator
  get_first_parm() const;

  const interned_string&
  get_cached_name(bool internal = false) const;

  virtual bool
  operator==(const type_base&) const;

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~function_type();

  friend bool
  equals(const function_type&, const function_type&, change_kind*);
};//end class function_type

/// The hashing functor for @ref function_type.
struct function_type::hash
{
  size_t
  operator()(const function_type& t) const;

  size_t
  operator()(const function_type* t) const;

  size_t
  operator()(const function_type_sptr t) const;
};// end struct function_type::hash

/// Abstracts the type of a class member function.
class method_type : public function_type
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  method_type();

public:

  /// Hasher for intances of method_type
  struct hash;

  method_type(type_base_sptr return_type,
	      class_or_union_sptr class_type,
	      const std::vector<function_decl::parameter_sptr>& parms,
	      bool is_const,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

  method_type(type_base_sptr return_type,
	      type_base_sptr class_type,
	      const std::vector<function_decl::parameter_sptr>& parms,
	      bool is_const,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

  method_type(class_or_union_sptr class_type,
	      bool is_const,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

  method_type(const environment* env,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

  class_or_union_sptr
  get_class_type() const;

  void
  set_class_type(const class_or_union_sptr& t);

  void set_is_const(bool);

  bool get_is_const() const;

  virtual ~method_type();

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  friend interned_string
  get_method_type_name(const method_type& fn_type, bool internal);
};// end class method_type.

/// The base class of templates.
class template_decl : public virtual decl_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  template_decl();

public:

  /// Hasher.
  struct hash;

  template_decl(const environment*	env,
		const string&		name,
		const location&	locus,
		visibility		vis = VISIBILITY_DEFAULT);

  void
  add_template_parameter(const template_parameter_sptr p);

  const std::list<template_parameter_sptr>&
  get_template_parameters() const;

  virtual bool
  operator==(const template_decl& o) const;

  virtual ~template_decl();
};//end class template_decl

/// Base class for a template parameter.  Client code should use the
/// more specialized type_template_parameter,
/// non_type_template_parameter and template_template_parameter below.
class template_parameter
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  // Forbidden
  template_parameter();

 public:

  /// Hashers.
  struct hash;
  struct dynamic_hash;
  struct shared_ptr_hash;

  template_parameter(unsigned			index,
		     template_decl_sptr	enclosing_tdecl);

  virtual bool
  operator==(const template_parameter&) const;

  bool
  operator!=(const template_parameter&) const;

  unsigned
  get_index() const;

  const template_decl_sptr
  get_enclosing_template_decl() const;

  bool
  get_hashing_has_started() const;

  void
  set_hashing_has_started(bool f) const;

  virtual ~template_parameter();
};//end class template_parameter

struct template_decl::hash
{
    size_t
    operator()(const template_decl& t) const;
};// end struct template_decl::hash

/// Abstracts a type template parameter.
class type_tparameter : public template_parameter, public virtual type_decl
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  type_tparameter();

public:

  /// Hasher.
  struct hash;

  type_tparameter(unsigned		index,
		  template_decl_sptr	enclosing_tdecl,
		  const string&	name,
		  const location&	locus);

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const template_parameter&) const;

  virtual bool
  operator==(const type_tparameter&) const;

  virtual ~type_tparameter();
};// end class type_tparameter.

/// Abstracts non type template parameters.
class non_type_tparameter : public template_parameter, public virtual decl_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  type_base_wptr type_;

  // Forbidden
  non_type_tparameter();

public:
  /// Hasher.
  struct hash;

  non_type_tparameter(unsigned			index,
		      template_decl_sptr	enclosing_tdecl,
		      const string&		name,
		      type_base_sptr		type,
		      const location&		locus);
  virtual size_t
  get_hash() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const template_parameter&) const;

  const type_base_sptr
  get_type() const;

  virtual ~non_type_tparameter();
};// end class non_type_tparameter

/// Hasher for the @ref non_type_tparameter type.
struct non_type_tparameter::hash
{
  size_t
  operator()(const non_type_tparameter& t) const;

  size_t
  operator()(const non_type_tparameter* t) const;
};

class template_tparameter;

/// Abstracts a template template parameter.
class template_tparameter : public type_tparameter, public template_decl
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  // Forbidden
  template_tparameter();

public:

  /// A hasher for instances of template_tparameter
  struct hash;

  template_tparameter(unsigned			index,
		      template_decl_sptr	enclosing_tdecl,
		      const string&		name,
		      const location&		locus);

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const template_parameter&) const;

  virtual bool
  operator==(const template_decl&) const;

  virtual ~template_tparameter();
};

/// This abstracts a composition of types based on template type
/// parameters.  The result of the composition is a type that can be
/// referred to by a template non-type parameter.  Instances of this
/// type can appear at the same level as template parameters, in the
/// scope of a template_decl.
class type_composition : public template_parameter, public virtual decl_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  type_composition();

public:
  struct hash;

  type_composition(unsigned		index,
		   template_decl_sptr	tdecl,
		   type_base_sptr	composed_type);

  const type_base_sptr
  get_composed_type() const;

  void
  set_composed_type(type_base_sptr t);

  virtual size_t
  get_hash() const;

  virtual ~type_composition();
};

/// Hasher for the @ref type_composition type.
struct type_composition::hash
{
  size_t
  operator()(const type_composition& t) const;

  size_t
  operator()(const type_composition* t) const;

}; //struct type_composition::hash

/// Abstract a function template declaration.
class function_tdecl : public template_decl, public scope_decl
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  function_tdecl();

public:

  /// Hash functor for function templates.
  struct hash;
  struct shared_ptr_hash;

  function_tdecl(const environment*	env,
		 const location&	locus,
		 visibility		vis = VISIBILITY_DEFAULT,
		 binding		bind = BINDING_NONE);

  function_tdecl(function_decl_sptr	pattern,
		 const location&	locus,
		 visibility		vis = VISIBILITY_DEFAULT,
		 binding		bind = BINDING_NONE);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const template_decl&) const;

  virtual bool
  operator==(const function_tdecl&) const;

  void
  set_pattern(shared_ptr<function_decl> p);

  shared_ptr<function_decl>
  get_pattern() const;

  binding
  get_binding() const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~function_tdecl();
}; // end class function_tdecl.

/// Abstract a class template.
class class_tdecl : public template_decl, public scope_decl
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  class_tdecl();

public:

  /// Hashers.
  struct hash;
  struct shared_ptr_hash;

  class_tdecl(const environment* env, const location& locus,
	      visibility vis = VISIBILITY_DEFAULT);

  class_tdecl(class_decl_sptr	pattern,
	      const location&	locus,
	      visibility	vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const template_decl&) const;

  virtual bool
  operator==(const class_tdecl&) const;

  void
  set_pattern(class_decl_sptr p);

  shared_ptr<class_decl>
  get_pattern() const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~class_tdecl();
};// end class class_tdecl

/// The base class for member types, data members and member
/// functions.  Its purpose is mainly to carry the access specifier
/// (and possibly other properties that might be shared by all class
/// members) for the member.
class member_base
{
protected:
  enum access_specifier access_;
  bool			is_static_;

private:
  // Forbidden
  member_base();

public:
  /// Hasher.
  struct hash;

  member_base(access_specifier a, bool is_static = false)
    : access_(a), is_static_(is_static)
  {}

  /// Getter for the access specifier of this member.
  ///
  /// @return the access specifier for this member.
  access_specifier
  get_access_specifier() const
  {return access_;}

  /// Setter for the access specifier of this member.
  ///
  /// @param a the new access specifier.
  void
  set_access_specifier(access_specifier a)
  {access_ = a;}

  /// @return true if the member is static, false otherwise.
  bool
  get_is_static() const
  {return is_static_;}

  /// Set a flag saying if the parameter is static or not.
  ///
  /// @param f set to true if the member is static, false otherwise.
  void
  set_is_static(bool f)
  {is_static_ = f;}

  virtual bool
  operator==(const member_base& o) const;
};// end class member_base

/// Abstraction of the declaration of a method.
class method_decl : public function_decl
{
  method_decl();

  virtual void
  set_scope(scope_decl*);

public:

  method_decl(const string& name, method_type_sptr type,
	      bool declared_inline, const location& locus,
	      const string& mangled_name = "",
	      visibility vis = VISIBILITY_DEFAULT,
	      binding	bind = BINDING_GLOBAL);

  method_decl(const string& name,
	      function_type_sptr type,
	      bool declared_inline,
	      const location& locus,
	      const string& mangled_name = "",
	      visibility vis  = VISIBILITY_DEFAULT,
	      binding	bind = BINDING_GLOBAL);

  method_decl(const string& name, type_base_sptr type,
	      bool declared_inline, const location& locus,
	      const string& mangled_name = "",
	      visibility vis = VISIBILITY_DEFAULT,
	      binding bind = BINDING_GLOBAL);

  virtual void
  set_linkage_name(const string&);

  /// @return the type of the current instance of the
  /// method_decl.
  const method_type_sptr
  get_type() const;

  void
  set_type(const method_type_sptr fn_type)
  {function_decl::set_type(fn_type);}

  friend bool
  get_member_function_is_ctor(const function_decl&);

  friend void
  set_member_function_is_ctor(function_decl&, bool);

  friend void
  set_member_function_is_ctor(const function_decl_sptr&, bool);

  friend bool
  get_member_function_is_dtor(const function_decl&);

  friend void
  set_member_function_is_dtor(function_decl&, bool);

  friend void
  set_member_function_is_dtor(const function_decl_sptr&, bool);

  friend bool
  get_member_function_is_static(const function_decl&);

  friend void
  set_member_function_is_static(const function_decl&, bool);

  friend bool
  get_member_function_is_const(const function_decl&);

  friend void
  set_member_function_is_const(function_decl&, bool);

  friend void
  set_member_function_is_const(const function_decl_sptr&, bool);

  friend bool
  member_function_has_vtable_offset(const function_decl&);

  friend ssize_t
  get_member_function_vtable_offset(const function_decl&);

  friend void
  set_member_function_vtable_offset(function_decl&, ssize_t);

  friend void
  set_member_function_vtable_offset(const function_decl_sptr&, ssize_t);

  friend bool
  get_member_function_is_virtual(const function_decl&);

  friend void
  set_member_function_is_virtual(function_decl&, bool);

  virtual ~method_decl();
};// end class method_decl

bool
operator==(const method_decl_sptr& l, const method_decl_sptr& r);

bool
operator!=(const method_decl_sptr& l, const method_decl_sptr& r);

/// The base type of @ref class_decl and @ref union_decl
class class_or_union : public scope_type_decl
{
public:
  struct priv;
  priv *priv_;

private:
  // Forbidden
  class_or_union();

protected:

  virtual decl_base_sptr
  add_member_decl(const decl_base_sptr&);

  virtual decl_base_sptr
  insert_member_decl(decl_base_sptr member, declarations::iterator before);

  virtual void
  remove_member_decl(decl_base_sptr);

  void
  maybe_fixup_members_of_anon_data_member(var_decl_sptr& anon_dm);

public:
  /// Hasher.
  struct hash;

  /// Convenience typedef
  /// @{
  typedef vector<type_base_sptr>		member_types;
  typedef vector<var_decl_sptr>		data_members;
  typedef vector<method_decl_sptr>		member_functions;
  typedef unordered_map<ssize_t, member_functions> virtual_mem_fn_map_type;
  typedef unordered_map<string, method_decl*> string_mem_fn_ptr_map_type;
  typedef unordered_map<string, method_decl_sptr> string_mem_fn_sptr_map_type;
  /// @}

  class_or_union(const environment* env, const string& name,
		 size_t size_in_bits, size_t align_in_bits,
		 const location& locus, visibility vis,
		 member_types& mbrs, data_members& data_mbrs,
		 member_functions& member_fns);

  class_or_union(const environment* env, const string& name,
		 size_t size_in_bits, size_t align_in_bits,
		 const location& locus, visibility vis);

  class_or_union(const environment* env, const string& name,
		 bool is_declaration_only = true);

  virtual void
  set_size_in_bits(size_t);

  virtual size_t
  get_size_in_bits() const;

  virtual size_t
  get_alignment_in_bits() const;

  virtual void
  set_alignment_in_bits(size_t);

  typedef_decl_sptr
  get_naming_typedef() const;

  void
  set_naming_typedef(const typedef_decl_sptr&);

 void
  insert_member_type(type_base_sptr t,
		     declarations::iterator before);

  void
  add_member_type(type_base_sptr t);

  type_base_sptr
  add_member_type(type_base_sptr t, access_specifier a);

  void
  remove_member_type(type_base_sptr t);

  const member_types&
  get_member_types() const;

  virtual size_t
  get_num_anonymous_member_classes() const;

  virtual size_t
  get_num_anonymous_member_unions() const;

  virtual size_t
  get_num_anonymous_member_enums() const;

  type_base_sptr
  find_member_type(const string& name) const;

  void
  add_data_member(var_decl_sptr v, access_specifier a,
		  bool is_laid_out, bool is_static,
		  size_t offset_in_bits);

  const data_members&
  get_data_members() const;

  const var_decl_sptr
  find_data_member(const string&) const;

  const var_decl_sptr
  find_data_member(const var_decl_sptr&) const;

  const var_decl_sptr
  find_anonymous_data_member(const var_decl_sptr&) const;

  const data_members&
  get_non_static_data_members() const;

  void
  add_member_function(method_decl_sptr f,
		      access_specifier a,
		      bool is_static, bool is_ctor,
		      bool is_dtor, bool is_const);

  void
  add_member_function(method_decl_sptr f,
		      access_specifier a,
		      bool is_virtual,
		      size_t vtable_offset,
		      bool is_static, bool is_ctor,
		      bool is_dtor, bool is_const);

  const member_functions&
  get_member_functions() const;

  const method_decl*
  find_member_function(const string& mangled_name) const;

  method_decl*
  find_member_function(const string& mangled_name);

  method_decl_sptr
  find_member_function_sptr(const string& mangled_name);

  const method_decl*
  find_member_function_from_signature(const string& s) const;

  method_decl*
  find_member_function_from_signature(const string& s);

  void
  add_member_function_template(member_function_template_sptr);

  const member_function_templates&
  get_member_function_templates() const;

  void
  add_member_class_template(member_class_template_sptr m);

  const member_class_templates&
  get_member_class_templates() const;

  bool
  has_no_member() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const class_or_union&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~class_or_union();

  friend method_decl_sptr
  copy_member_function(class_or_union_sptr& t,
		       const method_decl*m);

  friend method_decl_sptr
  copy_member_function(class_or_union_sptr& t,
		       const method_decl_sptr& m);

  friend void
  fixup_virtual_member_function(method_decl_sptr method);

  friend void
  set_member_is_static(decl_base& d, bool s);

  friend bool
  equals(const class_or_union&, const class_or_union&, change_kind*);

  friend bool
  equals(const class_decl&, const class_decl&, change_kind*);

  friend class method_decl;
  friend class class_decl;
}; // end class class_or_union

method_decl_sptr
copy_member_function(const class_or_union_sptr& clazz,
		     const method_decl_sptr& f);

method_decl_sptr
copy_member_function(const class_or_union_sptr& clazz,
		     const method_decl* f);

bool
operator==(const class_or_union_sptr& l, const class_or_union_sptr& r);

bool
operator!=(const class_or_union_sptr& l, const class_or_union_sptr& r);

/// Hasher for the @ref class_or_union type
struct class_or_union::hash
{
  size_t
  operator()(const class_or_union& t) const;

  size_t
  operator()(const class_or_union* t) const;
}; // end struct class_decl::hash

/// Abstracts a class declaration.
class class_decl : public class_or_union
{
  // Forbidden
  class_decl();

protected:

  virtual decl_base_sptr
  insert_member_decl(decl_base_sptr member, declarations::iterator before);

public:
  /// Hasher.
  struct hash;

  /// Forward declarations.
  class base_spec;

  /// Convenience typedef
  /// @{
  typedef shared_ptr<base_spec>			base_spec_sptr;
  typedef vector<base_spec_sptr>			base_specs;

  /// @}

protected:
  virtual void
  on_canonical_type_set();

private:
  struct priv;
  // This priv it's not handled by a shared_ptr because accessing the
  // data members of the priv struct for this class_decl shows up on
  // performance profiles when dealing with big binaries with a lot of
  // types; dereferencing the shared_ptr involves locking of some sort
  // and that is slower than just dereferencing a pointer likere here.
  // There are other types for which the priv pointer is managed using
  // shared_ptr just fine, because those didn't show up during our
  // performance profiling.
  priv * priv_;

public:

  class_decl(const environment* env, const string& name,
	     size_t size_in_bits, size_t align_in_bits,
	     bool is_struct, const location& locus,
	     visibility vis, base_specs& bases,
	     member_types& mbrs, data_members& data_mbrs,
	     member_functions& member_fns);

  class_decl(const environment* env, const string& name,
	     size_t size_in_bits, size_t align_in_bits,
	     bool is_struct, const location& locus,
	     visibility vis, base_specs& bases,
	     member_types& mbrs, data_members& data_mbrs,
	     member_functions& member_fns, bool is_anonymous);

  class_decl(const environment* env, const string& name,
	     size_t size_in_bits, size_t align_in_bits,
	     bool is_struct, const location& locus, visibility vis);

  class_decl(const environment* env, const string& name,
	     size_t size_in_bits, size_t align_in_bits,
	     bool is_struct, const location& locus,
	     visibility vis, bool is_anonymous);

  class_decl(const environment* env, const string& name, bool is_struct,
	     bool is_declaration_only = true);

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  void
  is_struct(bool f);

  bool
  is_struct() const;

  void
  add_base_specifier(shared_ptr<base_spec> b);

  const base_specs&
  get_base_specifiers() const;

  class_decl_sptr
  find_base_class(const string&) const;

  const member_functions&
  get_virtual_mem_fns() const;

  const virtual_mem_fn_map_type&
  get_virtual_mem_fns_map() const;

  void
  sort_virtual_mem_fns();

  bool
  has_no_base_nor_member() const;

  bool
  has_virtual_member_functions() const;

  bool
  has_virtual_bases() const;

  bool
  has_vtable() const;

  ssize_t
  get_biggest_vtable_offset() const;

  virtual size_t
  get_hash() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const class_decl&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~class_decl();

  friend void
  fixup_virtual_member_function(method_decl_sptr method);

  friend void
  set_member_is_static(decl_base& d, bool s);

  friend bool
  equals(const class_decl&, const class_decl&, change_kind*);

  friend class method_decl;
  friend class class_or_union;
};// end class class_decl

bool
equals(const class_decl&, const class_decl&, change_kind*);

method_decl_sptr
copy_member_function(const class_decl_sptr& clazz,
		     const method_decl_sptr& f);

method_decl_sptr
copy_member_function(const class_decl_sptr& clazz,
		     const method_decl* f);
void
fixup_virtual_member_function(method_decl_sptr method);

/// Hasher for the @ref class_decl type
struct class_decl::hash
{
  size_t
  operator()(const class_decl& t) const;

  size_t
  operator()(const class_decl* t) const;
}; // end struct class_decl::hash

enum access_specifier
get_member_access_specifier(const decl_base&);

enum access_specifier
get_member_access_specifier(const decl_base_sptr&);

void
set_member_access_specifier(decl_base&,
			    access_specifier);

void
set_member_access_specifier(const decl_base_sptr&,
			    access_specifier);

std::ostream&
operator<<(std::ostream&, access_specifier);

bool
operator==(const class_decl_sptr& l, const class_decl_sptr& r);

bool
operator!=(const class_decl_sptr& l, const class_decl_sptr& r);

bool
equals(const class_decl::base_spec&,
       const class_decl::base_spec&,
       change_kind*);

/// Abstraction of a base specifier in a class declaration.
class class_decl::base_spec : public member_base,
			      public virtual decl_base
{
  struct priv;
  typedef shared_ptr<priv>priv_sptr;

  priv_sptr priv_;

  // Forbidden
  base_spec();

public:

  /// Hasher.
  struct hash;

  base_spec(const class_decl_sptr& base, access_specifier a,
	    long offset_in_bits = -1, bool is_virtual = false);

  base_spec(const type_base_sptr& base, access_specifier a,
	    long offset_in_bits = -1, bool is_virtual = false);

  class_decl_sptr
  get_base_class() const;

  bool
  get_is_virtual() const;

  long
  get_offset_in_bits() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const member_base&) const;

  virtual size_t
  get_hash() const;

  virtual bool
  traverse(ir_node_visitor&);
};// end class class_decl::base_spec

bool
operator==(const class_decl::base_spec_sptr& l,
	   const class_decl::base_spec_sptr& r);

bool
operator!=(const class_decl::base_spec_sptr& l,
	   const class_decl::base_spec_sptr& r);

class_decl::base_spec*
is_class_base_spec(type_or_decl_base*);

class_decl::base_spec_sptr
is_class_base_spec(type_or_decl_base_sptr);

/// Abstracts a union type declaration.
class union_decl : public class_or_union
{
  // Forbid
  union_decl();

public:

  union_decl(const environment* env, const string& name,
	     size_t size_in_bits, const location& locus,
	     visibility vis, member_types& mbrs,
	     data_members& data_mbrs, member_functions& member_fns);

  union_decl(const environment* env, const string& name,
	     size_t size_in_bits, const location& locus,
	     visibility vis, member_types& mbrs,
	     data_members& data_mbrs, member_functions& member_fns,
	     bool is_anonymous);

  union_decl(const environment* env, const string& name,
	     size_t size_in_bits, const location& locus,
	     visibility vis);

  union_decl(const environment* env, const string& name,
	     size_t size_in_bits, const location& locus,
	     visibility vis, bool is_anonymous);

  union_decl(const environment* env, const string& name,
	     bool is_declaration_only = true);

  virtual string
  get_pretty_representation(bool internal = false,
			    bool qualified_name = true) const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const union_decl&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~union_decl();
}; // union_decl

bool
equals(const union_decl&, const union_decl&, change_kind*);

method_decl_sptr
copy_member_function(const union_decl_sptr& union_type,
		     const method_decl_sptr& f);

method_decl_sptr
copy_member_function(const union_decl_sptr& union_type,
		     const method_decl* f);

bool
operator==(const union_decl_sptr& l, const union_decl_sptr& r);

bool
operator!=(const union_decl_sptr& l, const union_decl_sptr& r);

/// Abstraction of a member function context relationship.  This
/// relates a member function to its parent class.
class mem_fn_context_rel : public context_rel
{
protected:
  bool		is_virtual_;
  ssize_t	vtable_offset_in_bits_;
  bool		is_constructor_;
  bool		is_destructor_;
  bool		is_const_;

public:
  mem_fn_context_rel()
    : context_rel(),
      is_virtual_(false),
      vtable_offset_in_bits_(-1),
      is_constructor_(false),
      is_destructor_(false),
      is_const_(false)
  {}

  mem_fn_context_rel(scope_decl* s)
    : context_rel(s),
      is_virtual_(false),
      vtable_offset_in_bits_(-1),
      is_constructor_(false),
      is_destructor_(false),
      is_const_(false)
  {}

  mem_fn_context_rel(scope_decl* s,
		     bool is_constructor,
		     bool is_destructor,
		     bool is_const,
		     bool is_virtual,
		     size_t vtable_offset_in_bits,
		     access_specifier access,
		     bool is_static)
    : context_rel(s, access, is_static),
      is_virtual_(is_virtual),
      vtable_offset_in_bits_(vtable_offset_in_bits),
      is_constructor_(is_constructor),
      is_destructor_(is_destructor),
      is_const_(is_const)
  {}

  bool
  is_virtual() const
  {return is_virtual_;}

  void
  is_virtual(bool is_virtual)
  {is_virtual_ = is_virtual;}

  /// Getter for the vtable offset property.
  ///
  /// This is the vtable offset of the member function of this
  /// relation.
  ///
  /// @return the vtable offset property of the relation.
  size_t
  vtable_offset() const
  {return vtable_offset_in_bits_;}

  /// Setter for the vtable offset property.
  ///
  /// This is the vtable offset of the member function of this
  /// relation.
  ///
  /// @partam s the new vtable offset.
  void
  vtable_offset(size_t s)
  {vtable_offset_in_bits_ = s;}

  /// Getter for the 'is-constructor' property.
  ///
  /// This tells if the member function of this relation is a
  /// constructor.
  ///
  /// @return the is-constructor property of the relation.
  bool
  is_constructor() const
  {return is_constructor_;}

  /// Setter for the 'is-constructor' property.
  ///
  /// @param f the new value of the the property.  Is true if this is
  /// for a constructor, false otherwise.
  void
  is_constructor(bool f)
  {is_constructor_ = f;}

  /// Getter for the 'is-destructor' property.
  ///
  /// Tells if the member function of this relation is a destructor.
  ///
  /// @return the is-destructor property of the relation;
  bool
  is_destructor() const
  {return is_destructor_;}

  /// Setter for the 'is-destructor' property.
  ///
  /// @param f the new value of the property.  Is true if this is for
  /// a destructor, false otherwise.
  void
  is_destructor(bool f)
  {is_destructor_ = f;}

  /// Getter for the 'is-const' property.
  ///
  /// Tells if the member function of this relation is a const member
  /// function.
  ///
  /// @return the 'is-const' property of the relation.
  bool
  is_const() const
  {return is_const_;}

  /// Setter for the 'is-const' property.
  ///
  /// @param f the new value of the property.  Is true if this is for
  /// a const entity, false otherwise.
  void
  is_const(bool f)
  {is_const_ = f;}

  virtual ~mem_fn_context_rel();
}; // end class mem_fn_context_rel

method_decl*
is_method_decl(const type_or_decl_base*);

method_decl*
is_method_decl(const type_or_decl_base&);

method_decl_sptr
is_method_decl(const type_or_decl_base_sptr&);

const var_decl*
lookup_data_member(const type_base* type,
		   const char* dm_name);

const function_decl::parameter*
get_function_parameter(const decl_base* fun,
		       unsigned parm_num);

/// Abstract a member function template.
class member_function_template : public member_base, public virtual decl_base
{
  bool is_constructor_;
  bool is_const_;
  shared_ptr<function_tdecl> fn_tmpl_;

  // Forbiden
  member_function_template();

public:
  /// Hasher.
  struct hash;

  member_function_template(function_tdecl_sptr f,
			   access_specifier access, bool is_static,
			   bool is_constructor, bool is_const)
    : type_or_decl_base(f->get_environment()),
      decl_base(f->get_environment(), f->get_name(), location()),
      member_base(access, is_static), is_constructor_(is_constructor),
      is_const_(is_const), fn_tmpl_(f)
  {}

  bool
  is_constructor() const
  {return is_constructor_;}

  bool
  is_const() const
  {return is_const_;}

  operator const function_tdecl& () const
  {return *fn_tmpl_;}

  function_tdecl_sptr
  as_function_tdecl() const
  {return fn_tmpl_;}

  virtual bool
  operator==(const member_base& o) const;

  virtual bool
  traverse(ir_node_visitor&);
};// end class member_function_template

bool
operator==(const member_function_template_sptr& l,
	   const member_function_template_sptr& r);

bool
operator!=(const member_function_template_sptr& l,
	   const member_function_template_sptr& r);

/// Abstracts a member class template template
class member_class_template
  : public member_base,
    public virtual decl_base
{
  shared_ptr<class_tdecl> class_tmpl_;

  // Forbidden
  member_class_template();

public:

  /// Hasher.
  struct hash;

  member_class_template(class_tdecl_sptr c,
			access_specifier access, bool is_static)
    : type_or_decl_base(c->get_environment()),
      decl_base(c->get_environment(), c->get_name(), location()),
      member_base(access, is_static),
      class_tmpl_(c)
  {}

  operator const class_tdecl& () const
  { return *class_tmpl_; }

  class_tdecl_sptr
  as_class_tdecl() const
  {return class_tmpl_;}

  virtual bool
  operator==(const member_base& o) const;

  virtual bool
  operator==(const member_class_template&) const;

  virtual bool
  traverse(ir_node_visitor& v);
};// end class member_class_template

bool
operator==(const member_class_template_sptr& l,
	   const member_class_template_sptr& r);

bool
operator!=(const member_class_template_sptr& l,
	   const member_class_template_sptr& r);

// Forward declarations for select nested hashers.
struct type_base::shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<type_base> t) const;
};

struct type_base::dynamic_hash
{
  size_t
  operator()(const type_base* t) const;
};

/// A hashing functor for instances and pointers of @ref var_decl.
struct var_decl::hash
{
  size_t
  operator()(const var_decl& t) const;

  size_t
  operator()(const var_decl* t) const;
}; //end struct var_decl::hash

/// A comparison functor for pointers to @ref var_decl.
struct var_decl::ptr_equal
{
  /// Return true if the two instances of @ref var_decl are equal.
  ///
  /// @param l the first variable to compare.
  ///
  /// @param r the second variable to compare.
  ///
  /// @return true if @p l equals @p r.
  bool
  operator()(const var_decl* l, const var_decl* r) const
  {
    if (l == r)
      return true;
    if (!!l != !!r)
      return false;
    return (*l == *r);
  }
};// end struct var_decl::ptr_equal

/// A hashing functor fo instances and pointers of @ref function_decl.
struct function_decl::hash
{
  size_t
  operator()(const function_decl& t) const;

  size_t
  operator()(const function_decl* t) const;
};//end struct function_decl::hash

/// Equality functor for instances of @ref function_decl
struct function_decl::ptr_equal
{
  /// Tests if two pointers to @ref function_decl are equal.
  ///
  /// @param l the first pointer to @ref function_decl to consider in
  /// the comparison.
  ///
  /// @param r the second pointer to @ref function_decl to consider in
  /// the comparison.
  ///
  /// @return true if the two functions @p l and @p r are equal, false
  /// otherwise.
  bool
  operator()(const function_decl* l, const function_decl* r) const
  {
    if (l == r)
      return true;
    if (!!l != !!r)
      return false;
    return (*l == *r);
  }
};// function_decl::ptr_equal

/// The hashing functor for class_decl::base_spec.
struct class_decl::base_spec::hash
{
  size_t
  operator()(const base_spec& t) const;
};

/// The hashing functor for member_base.
struct member_base::hash
{
  size_t
  operator()(const member_base& m) const;
};

/// The hashing functor for member_function_template.
struct member_function_template::hash
{
  size_t
  operator()(const member_function_template& t) const;
};

/// The hashing functor for member_class_template
struct member_class_template::hash
{
  size_t
  operator()(const member_class_template& t) const;
};

struct function_tdecl::hash
{
  size_t
  operator()(const function_tdecl& t) const;
};

struct function_tdecl::shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<function_tdecl> f) const;
};

struct class_tdecl::hash
{
  size_t
  operator()(const class_tdecl& t) const;
};

struct class_tdecl::shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<class_tdecl> t) const;
};

/// The base class for the visitor type hierarchy used for traversing
/// a translation unit.
///
/// Client code willing to get notified for a certain kind of node
/// during the IR traversal might want to define a visitor class that
/// inherit ir_node_visitor, overload the ir_node_visitor::visit_begin()
/// or ir_node_visitor::visit_end() method of its choice, and provide
/// and implementation for it.  If either
/// ir_node_visitor::visit_begin() or ir_node_visitor::visit_end()
/// return false, it means the traversal has to stop immediately after
/// the methods' return.  If the methods return true, it means the
/// traversal keeps going.
///
/// That new visitor class would then be passed to e.g,
/// translation_unit::traverse or to the traverse method of any type
/// where the traversal is supposed to start from.
class ir_node_visitor : public node_visitor_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:

  ir_node_visitor();

  void allow_visiting_already_visited_type_node(bool);
  bool allow_visiting_already_visited_type_node() const;
  void mark_type_node_as_visited(type_base *);
  void forget_visited_type_nodes();
  bool type_node_has_been_visited(type_base*) const;

  virtual bool visit_begin(decl_base*);
  virtual bool visit_end(decl_base*);

  virtual bool visit_begin(scope_decl*);
  virtual bool visit_end(scope_decl*);

  virtual bool visit_begin(type_base*);
  virtual bool visit_end(type_base*);

  virtual bool visit_begin(scope_type_decl*);
  virtual bool visit_end(scope_type_decl*);

  virtual bool visit_begin(type_decl*);
  virtual bool visit_end(type_decl*);

  virtual bool visit_begin(namespace_decl*);
  virtual bool visit_end(namespace_decl*);

  virtual bool visit_begin(qualified_type_def*);
  virtual bool visit_end(qualified_type_def*);

  virtual bool visit_begin(pointer_type_def*);
  virtual bool visit_end(pointer_type_def*);

  virtual bool visit_begin(reference_type_def*);
  virtual bool visit_end(reference_type_def*);

  virtual bool visit_begin(array_type_def*);
  virtual bool visit_end(array_type_def*);

  virtual bool visit_begin(array_type_def::subrange_type*);
  virtual bool visit_end(array_type_def::subrange_type*);

  virtual bool visit_begin(enum_type_decl*);
  virtual bool visit_end(enum_type_decl*);

  virtual bool visit_begin(typedef_decl*);
  virtual bool visit_end(typedef_decl*);

  virtual bool visit_begin(function_type*);
  virtual bool visit_end(function_type*);

  virtual bool visit_begin(var_decl*);
  virtual bool visit_end(var_decl*);

  virtual bool visit_begin(function_decl*);
  virtual bool visit_end(function_decl*);

  virtual bool visit_begin(function_decl::parameter*);
  virtual bool visit_end(function_decl::parameter*);

  virtual bool visit_begin(function_tdecl*);
  virtual bool visit_end(function_tdecl*);

  virtual bool visit_begin(class_tdecl*);
  virtual bool visit_end(class_tdecl*);

  virtual bool visit_begin(class_or_union *);
  virtual bool visit_end(class_or_union *);

  virtual bool visit_begin(class_decl*);
  virtual bool visit_end(class_decl*);

  virtual bool visit_begin(union_decl*);
  virtual bool visit_end(union_decl*);

  virtual bool visit_begin(class_decl::base_spec*);
  virtual bool visit_end(class_decl::base_spec*);

  virtual bool visit_begin(member_function_template*);
  virtual bool visit_end(member_function_template*);

  virtual bool visit_begin(member_class_template*);
  virtual bool visit_end(member_class_template*);
}; // end struct ir_node_visitor

// Debugging facility
void
fns_to_str(vector<function_decl*>::const_iterator a_begin,
	   vector<function_decl*>::const_iterator a_end,
	   vector<function_decl*>::const_iterator b_begin,
	   vector<function_decl*>::const_iterator b_end,
	   std::ostream& o);

}// end namespace ir
} // end namespace abigail
#endif // __ABG_IR_H__

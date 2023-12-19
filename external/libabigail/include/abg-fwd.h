// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#ifndef __ABG_IRFWD_H__
#define __ABG_IRFWD_H__

#include <stdint.h>
#include <cstddef>
#include <cstdlib>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility> // for std::rel_ops, at least.
#include <vector>
#include "abg-interned-str.h"
#include "abg-hash.h"

/// Toplevel namespace for libabigail.
namespace abigail
{
/**
   @mainpage libabigail

   This is the API documentation of the Application Binary
   Interface Generic Analysis and Instrumentation Library, aka,
   <em>libabigail</em>.

   Check out <a href="http://sourceware.org/libabigail"> the project
   homepage</a>!

   The current libabigail source code can be browsed at
   http://sourceware.org/git/gitweb.cgi?p=libabigail.git

   It can be checked out with:
       <em>git clone git://sourceware.org/git/libabigail.git</em>

   The mailing list to send messages and patches to is
   libabigail@sourceware.org.

   You can hang out with libabigail developers and users on irc at
   irc://irc.oftc.net\#libabigail.
*/

// Inject some types.
using std::shared_ptr;
using std::weak_ptr;
using std::unordered_map;
using std::string;
using std::vector;

// Pull in relational operators.
using namespace std::rel_ops;

namespace ir
{

// Forward declarations for corpus.

class corpus;
typedef shared_ptr<corpus> corpus_sptr;

class corpus_group;
typedef shared_ptr<corpus_group> corpus_group_sptr;

// Forward declarations for ir.

class  ir_node_visitor;

struct ir_traversable_base;

/// Convenience typedef for a shared pointer to @ref
/// ir_traversable_base.
typedef shared_ptr<ir_traversable_base> ir_traversable_base_sptr;

class environment;
/// Convenience typedef for a shared pointer to an @ref environment
typedef shared_ptr<environment> environment_sptr;

class location;
class location_manager;

class type_or_decl_base;
/// A convenience typedef for a shared_ptr to @ref type_or_decl_base.
typedef shared_ptr<type_or_decl_base> type_or_decl_base_sptr;

class type_base;

// Convenience typedef for a shared pointer on a @ref type_base
typedef shared_ptr<type_base> type_base_sptr;

/// Convenience typedef for a weak pointer on a @ref type_base
typedef weak_ptr<type_base> type_base_wptr;

/// Convenience typedef for a weak pointer to a @ref corpus.
typedef weak_ptr<corpus> corpus_wptr;

class translation_unit;
/// Convenience typedef for a shared pointer on a @ref
/// translation_unit type.
typedef shared_ptr<translation_unit> translation_unit_sptr;
/// Convenience typedef for a map that associates a string to a
/// translation unit.
typedef unordered_map<string, translation_unit_sptr> string_tu_map_type;

/// A convenience typedef for a vector of type_base_wptr.
typedef vector<type_base_wptr> type_base_wptrs_type;

/// A convenience typedef for a map which key is an interned_string
/// and which value is a vector of type_base_wptr.
typedef unordered_map<interned_string,
		      type_base_wptrs_type,
		      hash_interned_string> istring_type_base_wptrs_map_type;

class decl_base;

// Convenience typedef for a smart pointer on @ref decl_base.
typedef shared_ptr<decl_base> decl_base_sptr;

class type_decl;
/// Convenience typedef for a shared pointer on a @ref type_decl.
typedef shared_ptr<type_decl> type_decl_sptr;


class typedef_decl;

/// Convenience typedef for a shared pointer on a @ref typedef_decl.
typedef shared_ptr<typedef_decl> typedef_decl_sptr;

/// Convenience typedef for a weak pointer on a @ref typedef_decl.
typedef weak_ptr<typedef_decl> typedef_decl_wptr;

class enum_type_decl;

/// Convenience typedef for shared pointer to a @ref enum_type_decl.
typedef shared_ptr<enum_type_decl> enum_type_decl_sptr;

/// Convenience typedef for a vector of @ref enum_type_decl_sptr
typedef vector<enum_type_decl_sptr> enums_type;

/// Convenience typedef for a weak pointer to a @ref decl_base.
typedef weak_ptr<decl_base> decl_base_wptr;

class class_or_union;

typedef shared_ptr<class_or_union> class_or_union_sptr;
typedef weak_ptr<class_or_union> class_or_union_wptr;

class scope_type_decl;

class class_decl;

/// Convenience typedef for a shared pointer on a @ref class_decl
typedef shared_ptr<class_decl> class_decl_sptr;

/// Convenience typedef for a vector of @ref class_decl_sptr
typedef vector<class_decl_sptr> classes_type;

/// Convenience typedef for a weak pointer on a @ref class_decl.
typedef weak_ptr<class_decl> class_decl_wptr;

class union_decl;

typedef shared_ptr<union_decl> union_decl_sptr;

class function_type;
/// Convenience typedef for a shared pointer on a @ref function_type
typedef shared_ptr<function_type> function_type_sptr;

/// Convenience typedef fo a vector of @ref function_type_sptr
typedef vector<function_type_sptr> function_types_type;

/// Convenience typedef for a weak pointer on a @ref function_type
typedef weak_ptr<function_type> function_type_wptr;

class method_type;

/// Convenience typedef for shared pointer to @ref method_type.
typedef shared_ptr<method_type> method_type_sptr;

class pointer_type_def;

/// Convenience typedef for a shared pointer on a @ref pointer_type_def
typedef shared_ptr<pointer_type_def> pointer_type_def_sptr;

class qualified_type_def;

typedef shared_ptr<qualified_type_def> qualified_type_def_sptr;

class reference_type_def;

/// Convenience typedef for a shared pointer on a @ref reference_type_def
typedef shared_ptr<reference_type_def> reference_type_def_sptr;

class array_type_def;

/// Convenience typedef for a shared pointer on a @ref array_type_def
typedef shared_ptr<array_type_def> array_type_def_sptr;

class subrange_type;

class dm_context_rel;

/// A convenience typedef for a shared pointer to dm_context_rel.
typedef shared_ptr<dm_context_rel> dm_context_rel_sptr;

class var_decl;

/// Convenience typedef for a shared pointer on a @ref var_decl
typedef shared_ptr<var_decl> var_decl_sptr;

/// Convenience typedef for a weak pointer on a @ref var_decl
typedef weak_ptr<var_decl> var_decl_wptr;

typedef unordered_map<interned_string,
		      var_decl*,
		      hash_interned_string> istring_var_decl_ptr_map_type;

class scope_decl;

/// Convenience typedef for a shared pointer on a @ref scope_decl.
typedef shared_ptr<scope_decl> scope_decl_sptr;

class function_decl;

/// Convenience typedef for a shared pointer on a @ref function_decl
typedef shared_ptr<function_decl> function_decl_sptr;

typedef unordered_map<interned_string,
		      function_decl*,
		      hash_interned_string> istring_function_decl_ptr_map_type;

class method_decl;

typedef shared_ptr<method_decl> method_decl_sptr;

class mem_fn_context_rel;

/// A convenience typedef for a shared pointer to @ref
/// mem_fn_context_rel.
typedef shared_ptr<mem_fn_context_rel> mem_fn_context_rel_sptr;

class namespace_decl;

/// Convenience typedef for a shared pointer on namespace_decl.
typedef shared_ptr<namespace_decl> namespace_decl_sptr;

class class_tdecl;

/// Convenience typedef for a shared pointer on a @ref class_tdecl
typedef shared_ptr<class_tdecl> class_tdecl_sptr;

class function_tdecl;

/// Convenience typedef for a shared pointer on a @ref function_tdecl
typedef shared_ptr<function_tdecl> function_tdecl_sptr;

class global_scope;

/// Convenience typedef for shared pointer on @ref global_scope.
typedef shared_ptr<global_scope> global_scope_sptr;

class node_visitor;

class template_decl;

/// Convenience typedef for a shared pointer to @ref template_decl
typedef shared_ptr<template_decl> template_decl_sptr;

/// Convenience typedef for a weak pointer to template_decl
typedef weak_ptr<template_decl> template_decl_wptr;

class template_parameter;

/// Convenience typedef for shared pointer to template parameter
typedef shared_ptr<template_parameter> template_parameter_sptr;

class non_type_tparameter;

/// Convenience typedef for shared pointer to @ref
/// non_type_template_parameter
typedef shared_ptr<non_type_tparameter> non_type_tparameter_sptr;

class type_tparameter;

class template_tparameter;

/// Convenience typedef for a shared_ptr to @ref template_tparameter.
typedef shared_ptr<template_tparameter> template_tparameter_sptr;

/// Convenience typedef for a shared pointer to @ref type_tparameter.
typedef shared_ptr<type_tparameter> type_tparameter_sptr;

class type_composition;

class member_function_template;
typedef shared_ptr<member_function_template> member_function_template_sptr;
typedef vector<member_function_template_sptr> member_function_templates;

class member_class_template;
typedef shared_ptr<member_class_template> member_class_template_sptr;
typedef vector<member_class_template_sptr> member_class_templates;

/// Convenience typedef for shared pointer to type_composition
typedef shared_ptr<type_composition> type_composition_sptr;

decl_base_sptr
add_decl_to_scope(decl_base_sptr, scope_decl*);

decl_base_sptr
add_decl_to_scope(decl_base_sptr, const scope_decl_sptr&);

const global_scope*
get_global_scope(const decl_base&);

const global_scope*
get_global_scope(const decl_base*);

const global_scope*
get_global_scope(const decl_base_sptr);

translation_unit*
get_translation_unit(const decl_base&);

translation_unit*
get_translation_unit(const decl_base*);

translation_unit*
get_translation_unit(const decl_base_sptr);

bool
is_global_scope(const scope_decl&);

const global_scope*
is_global_scope(const scope_decl*);

bool
is_global_scope(const scope_decl_sptr);

bool
is_at_global_scope(const decl_base&);

bool
is_at_global_scope(const decl_base_sptr);

class_or_union*
is_at_class_scope(const decl_base_sptr);

class_or_union*
is_at_class_scope(const decl_base*);

class_or_union*
is_at_class_scope(const decl_base&);

bool
is_at_template_scope(const decl_base_sptr);

bool
is_template_parameter(const decl_base_sptr);

function_decl*
is_function_decl(const type_or_decl_base*);

function_decl_sptr
is_function_decl(const type_or_decl_base_sptr&);

bool
is_function_decl(const type_or_decl_base&);

decl_base*
is_decl(const type_or_decl_base*);

decl_base_sptr
is_decl(const type_or_decl_base_sptr&);

decl_base*
is_decl_slow(const type_or_decl_base*);

decl_base_sptr
is_decl_slow(const type_or_decl_base_sptr&);

bool
is_type(const type_or_decl_base&);

type_base*
is_type(const type_or_decl_base*);

type_base_sptr
is_type(const type_or_decl_base_sptr& tod);

bool
is_anonymous_type(type_base*);

bool
is_anonymous_type(const type_base_sptr&);

const type_decl*
is_type_decl(const type_or_decl_base*);

type_decl_sptr
is_type_decl(const type_or_decl_base_sptr&);

typedef_decl_sptr
is_typedef(const type_or_decl_base_sptr);

const typedef_decl*
is_typedef(const type_base*);

typedef_decl*
is_typedef(type_base*);

enum_type_decl_sptr
is_compatible_with_enum_type(const type_base_sptr&);

enum_type_decl_sptr
is_compatible_with_enum_type(const decl_base_sptr&);

enum_type_decl_sptr
is_enum_type(const type_or_decl_base_sptr&);

const enum_type_decl*
is_enum_type(const type_or_decl_base*);

bool
is_class_type(const type_or_decl_base&);

class_decl*
is_class_type(const type_or_decl_base*);

class_decl_sptr
is_class_type(const type_or_decl_base_sptr&);

bool
is_declaration_only_class_or_union_type(const type_base *t);

bool
is_declaration_only_class_or_union_type(const type_base_sptr&);

class_or_union*
is_class_or_union_type(const type_or_decl_base*);

class_or_union_sptr
is_class_or_union_type(const type_or_decl_base_sptr&);

bool
is_union_type(const type_or_decl_base&);

union_decl*
is_union_type(const type_or_decl_base*);

union_decl_sptr
is_union_type(const type_or_decl_base_sptr&);

class_decl_sptr
is_compatible_with_class_type(const type_base_sptr&);

class_decl_sptr
is_compatible_with_class_type(const decl_base_sptr&);

pointer_type_def*
is_pointer_type(type_or_decl_base*);

const pointer_type_def*
is_pointer_type(const type_or_decl_base*);

pointer_type_def_sptr
is_pointer_type(const type_or_decl_base_sptr&);

reference_type_def*
is_reference_type(type_or_decl_base*);

const reference_type_def*
is_reference_type(const type_or_decl_base*);

reference_type_def_sptr
is_reference_type(const type_or_decl_base_sptr&);

const type_base*
is_void_pointer_type(const type_base*);

qualified_type_def*
is_qualified_type(const type_or_decl_base*);

qualified_type_def_sptr
is_qualified_type(const type_or_decl_base_sptr&);

type_base_sptr
look_through_no_op_qualified_type(const shared_ptr<type_base>& t);

function_type_sptr
is_function_type(const type_or_decl_base_sptr&);

function_type*
is_function_type(type_or_decl_base*);

const function_type*
is_function_type(const type_or_decl_base*);

method_type_sptr
is_method_type(const type_or_decl_base_sptr&);

const method_type*
is_method_type(const type_or_decl_base*);

method_type*
is_method_type(type_or_decl_base*);

class_or_union_sptr
look_through_decl_only_class(const class_or_union&);

class_or_union_sptr
look_through_decl_only_class(class_or_union_sptr);

class_or_union*
look_through_decl_only_class(class_or_union*);

enum_type_decl_sptr
look_through_decl_only_enum(const enum_type_decl&);

enum_type_decl_sptr
look_through_decl_only_enum(enum_type_decl_sptr);

decl_base_sptr
look_through_decl_only(const decl_base&);

decl_base*
look_through_decl_only(decl_base*);

decl_base_sptr
look_through_decl_only(const decl_base_sptr&);

var_decl*
is_var_decl(const type_or_decl_base*);

var_decl_sptr
is_var_decl(const type_or_decl_base_sptr&);

namespace_decl_sptr
is_namespace(const decl_base_sptr&);

namespace_decl*
is_namespace(const decl_base*);

bool
is_template_parm_composition_type(const decl_base_sptr);

bool
is_template_decl(const decl_base_sptr);

bool
is_function_template_pattern(const decl_base_sptr);


decl_base_sptr
insert_decl_into_scope(decl_base_sptr,
		       vector<decl_base_sptr >::iterator,
		       scope_decl*);

decl_base_sptr
insert_decl_into_scope(decl_base_sptr,
		       vector<decl_base_sptr >::iterator,
		       scope_decl_sptr);

bool
has_scope(const decl_base&);

bool
has_scope(const decl_base_sptr);

bool
is_member_decl(const decl_base_sptr);

bool
is_member_decl(const decl_base*);

bool
is_member_decl(const decl_base&);

scope_decl*
is_scope_decl(decl_base*);

scope_decl_sptr
is_scope_decl(const decl_base_sptr&);

bool
is_member_type(const type_base_sptr&);

bool
is_user_defined_type(const type_base*);

bool
is_user_defined_type(const type_base_sptr&);

void
remove_decl_from_scope(decl_base_sptr);

bool
get_member_is_static(const decl_base&);

bool
get_member_is_static(const decl_base*);

bool
get_member_is_static(const decl_base_sptr&);

void
set_member_is_static(decl_base&, bool);

void
set_member_is_static(const decl_base_sptr&, bool);

bool
is_data_member(const var_decl&);

var_decl*
is_data_member(const type_or_decl_base*);

bool
is_data_member(const var_decl*);

var_decl_sptr
is_data_member(const type_or_decl_base_sptr&);

bool
is_data_member(const var_decl_sptr);

var_decl_sptr
is_data_member(const decl_base_sptr&);

var_decl*
is_data_member(const decl_base *);

var_decl*
is_data_member(const decl_base *);

const var_decl_sptr
get_next_data_member(const class_or_union_sptr&, const var_decl_sptr&);

bool
is_anonymous_data_member(const decl_base&);

const var_decl*
is_anonymous_data_member(const type_or_decl_base*);

const var_decl*
is_anonymous_data_member(const decl_base*);

var_decl_sptr
is_anonymous_data_member(const type_or_decl_base_sptr&);

var_decl_sptr
is_anonymous_data_member(const decl_base_sptr&);

var_decl_sptr
is_anonymous_data_member(const var_decl_sptr&);

const var_decl*
is_anonymous_data_member(const var_decl*);

bool
is_anonymous_data_member(const var_decl&);

const var_decl_sptr
get_first_non_anonymous_data_member(const var_decl_sptr);

var_decl_sptr
find_data_member_from_anonymous_data_member(const var_decl_sptr&,
					    const string&);

class_or_union*
anonymous_data_member_to_class_or_union(const var_decl*);

class_or_union_sptr
anonymous_data_member_to_class_or_union(const var_decl_sptr&);

const class_or_union_sptr
data_member_has_anonymous_type(const var_decl& d);

const class_or_union_sptr
data_member_has_anonymous_type(const var_decl* d);

const class_or_union_sptr
data_member_has_anonymous_type(const var_decl_sptr& d);

array_type_def*
is_array_type(const type_or_decl_base* decl);

array_type_def_sptr
is_array_type(const type_or_decl_base_sptr& decl);

array_type_def_sptr
is_array_of_qualified_element(const type_base_sptr&);

qualified_type_def_sptr
is_array_of_qualified_element(const array_type_def_sptr&);

array_type_def_sptr
is_typedef_of_array(const type_base_sptr&);

void
set_data_member_offset(var_decl_sptr, uint64_t);

uint64_t
get_data_member_offset(const var_decl&);

uint64_t
get_data_member_offset(const var_decl_sptr);

uint64_t
get_data_member_offset(const decl_base_sptr);

uint64_t
get_absolute_data_member_offset(const var_decl&);

uint64_t
get_var_size_in_bits(const var_decl_sptr&);

void
set_data_member_is_laid_out(var_decl_sptr, bool);

bool
get_data_member_is_laid_out(const var_decl&);

bool
get_data_member_is_laid_out(const var_decl_sptr);

bool
is_member_function(const function_decl&);

bool
is_member_function(const function_decl*);

bool
is_member_function(const function_decl_sptr&);

bool
get_member_function_is_ctor(const function_decl&);

bool
get_member_function_is_ctor(const function_decl_sptr&);

void
set_member_function_is_ctor(const function_decl&, bool);

void
set_member_function_is_ctor(const function_decl_sptr&, bool);

bool
get_member_function_is_dtor(const function_decl&);

bool
get_member_function_is_dtor(const function_decl_sptr&);

void
set_member_function_is_dtor(function_decl&, bool);

void
set_member_function_is_dtor(const function_decl_sptr&, bool);

bool
get_member_function_is_const(const function_decl&);

bool
get_member_function_is_const(const function_decl_sptr&);

void
set_member_function_is_const(function_decl&, bool);

void
set_member_function_is_const(const function_decl_sptr&, bool);

bool
member_function_has_vtable_offset(const function_decl&);

ssize_t
get_member_function_vtable_offset(const function_decl&);

ssize_t
get_member_function_vtable_offset(const function_decl_sptr&);

void
set_member_function_vtable_offset(const function_decl& f,
				  ssize_t s);

void
set_member_function_vtable_offset(const function_decl_sptr &f,
				  ssize_t s);

bool
get_member_function_is_virtual(const function_decl&);

bool
get_member_function_is_virtual(const function_decl_sptr&);

bool
get_member_function_is_virtual(const function_decl*);

void
set_member_function_is_virtual(function_decl&, bool);

void
set_member_function_is_virtual(const function_decl_sptr&, bool);

type_base_sptr
strip_typedef(const type_base_sptr);

type_base_sptr
peel_typedef_type(const type_base_sptr&);

const type_base*
peel_typedef_type(const type_base*);

type_base_sptr
peel_pointer_type(const type_base_sptr&);

const type_base*
peel_pointer_type(const type_base*);

type_base_sptr
peel_reference_type(const type_base_sptr&);

const type_base*
peel_reference_type(const type_base*);

const type_base_sptr
peel_array_type(const type_base_sptr&);

const type_base*
peel_array_type(const type_base*);

const type_base*
peel_qualified_type(const type_base*);

const type_base_sptr
peel_qualified_type(const type_base_sptr&);

type_base*
peel_qualified_or_typedef_type(const type_base* type);

type_base_sptr
peel_qualified_or_typedef_type(const type_base_sptr &type);

type_base_sptr
peel_typedef_pointer_or_reference_type(const type_base_sptr);

type_base*
peel_typedef_pointer_or_reference_type(const type_base* type);

type_base*
peel_pointer_or_reference_type(const type_base *type,
			       bool peel_qualified_type = true);

array_type_def_sptr
clone_array(const array_type_def_sptr& array);

typedef_decl_sptr
clone_typedef(const typedef_decl_sptr& t);

qualified_type_def_sptr
clone_qualified_type(const qualified_type_def_sptr& t);

type_base_sptr
clone_array_tree(const type_base_sptr t);

string
get_name(const type_or_decl_base*, bool qualified = true);

string
get_name(const type_or_decl_base_sptr&,
	 bool qualified = true);

location
get_location(const type_base_sptr& type);

location
get_location(const decl_base_sptr& decl);

string
build_qualified_name(const scope_decl* scope, const string& name);

string
build_qualified_name(const scope_decl* scope,
		     const type_base_sptr& type);

scope_decl*
get_type_scope(type_base*);

scope_decl*
get_type_scope(const type_base_sptr&);

interned_string
get_type_name(const type_base_sptr&,
	      bool qualified = true,
	      bool internal = false);

interned_string
get_type_name(const type_base*,
	      bool qualified = true,
	      bool internal = false);

interned_string
get_type_name(const type_base&,
	      bool qualified = true,
	      bool internal = false);

interned_string
get_name_of_pointer_to_type(const type_base& pointed_to_type,
			    bool qualified = true,
			    bool internal = false);

interned_string
get_name_of_reference_to_type(const type_base& pointed_to_type,
			      bool lvalue_reference = false,
			      bool qualified = true,
			      bool internal = false);

interned_string
get_function_type_name(const function_type_sptr&,
		       bool internal = false);

interned_string
get_function_type_name(const function_type*, bool internal = false);

interned_string
get_function_type_name(const function_type&, bool internal = false);

interned_string
get_method_type_name(const method_type_sptr&, bool internal = false);

interned_string
get_method_type_name(const method_type*, bool internal = false);

interned_string
get_method_type_name(const method_type&, bool internal = false);

string
get_pretty_representation(const decl_base*, bool internal = false);

string
get_pretty_representation(const type_base*, bool internal = false);

string
get_pretty_representation(const type_or_decl_base*, bool internal = false);

string
get_pretty_representation(const type_or_decl_base_sptr&,
			  bool internal = false);

string
get_pretty_representation(const decl_base_sptr&, bool internal = false);

string
get_pretty_representation(const type_base_sptr&, bool internal = false);

string
get_pretty_representation(const function_type&, bool internal = false);

string
get_pretty_representation(const function_type*, bool internal = false);

string
get_pretty_representation(const function_type_sptr&,
			  bool internal = false);

string
get_pretty_representation(const method_type&, bool internal = false);

string
get_pretty_representation(const method_type*, bool internal = false);

string
get_pretty_representation(const method_type_sptr&,
			  bool internal = false);

string
get_class_or_union_flat_representation(const class_or_union& cou,
				       const string& indent,
				       bool one_line,
				       bool internal,
				       bool qualified_name = true);

string
get_class_or_union_flat_representation(const class_or_union* cou,
				       const string& indent,
				       bool one_line,
				       bool internal,
				       bool qualified_name = true);

string
get_class_or_union_flat_representation(const class_or_union_sptr& cou,
				       const string& indent,
				       bool one_line,
				       bool internal,
				       bool qualified_name = true);

bool
odr_is_relevant(const type_or_decl_base&);

const decl_base*
get_type_declaration(const type_base*);

decl_base*
get_type_declaration(type_base*);

decl_base_sptr
get_type_declaration(const type_base_sptr);

bool
types_are_compatible(const type_base_sptr,
		     const type_base_sptr);

bool
types_are_compatible(const decl_base_sptr,
		     const decl_base_sptr);

const scope_decl*
get_top_most_scope_under(const decl_base*,
			 const scope_decl*);

const scope_decl*
get_top_most_scope_under(const decl_base_sptr,
			 const scope_decl*);

const scope_decl*
get_top_most_scope_under(const decl_base_sptr,
			 const scope_decl_sptr);

void
fqn_to_components(const std::string&,
		  std::list<string>&);

string
components_to_type_name(const std::list<string>&);

type_decl_sptr
lookup_basic_type(const type_decl&, const translation_unit&);

type_decl_sptr
lookup_basic_type(const interned_string&, const translation_unit&);

type_decl_sptr
lookup_basic_type(const string&, const translation_unit&);

type_decl_sptr
lookup_basic_type(const type_decl&, const corpus&);

type_decl_sptr
lookup_basic_type(const string&, const corpus&);

type_decl_sptr
lookup_basic_type(const interned_string&, const corpus&);

type_decl_sptr
lookup_basic_type_per_location(const interned_string&, const corpus&);

type_decl_sptr
lookup_basic_type_per_location(const string&, const corpus&);

class_decl_sptr
lookup_class_type(const class_decl&, const translation_unit&);

class_decl_sptr
lookup_class_type(const interned_string&, const translation_unit&);

class_decl_sptr
lookup_class_type(const string&, const translation_unit&);

class_decl_sptr
lookup_class_type(const class_decl&, const corpus&);

class_decl_sptr
lookup_class_type(const interned_string&, const corpus&);

const type_base_wptrs_type*
lookup_class_types(const interned_string&, const corpus&);

const type_base_wptrs_type*
lookup_class_types(const string&, const corpus&);

class_decl_sptr
lookup_class_type_per_location(const interned_string&, const corpus&);

class_decl_sptr
lookup_class_type_per_location(const string&, const corpus&);

class_decl_sptr
lookup_class_type(const string&, const corpus&);

class_decl_sptr
lookup_class_type_through_scopes(const std::list<string>&,
				 const translation_unit&);

union_decl_sptr
lookup_union_type(const interned_string&, const translation_unit&);

union_decl_sptr
lookup_union_type(const interned_string&, const corpus&);

union_decl_sptr
lookup_union_type_per_location(const interned_string&, const corpus&);

union_decl_sptr
lookup_union_type_per_location(const string&, const corpus&);

union_decl_sptr
lookup_union_type(const string&, const corpus&);

enum_type_decl_sptr
lookup_enum_type(const enum_type_decl&, const translation_unit&);

enum_type_decl_sptr
lookup_enum_type(const string&, const translation_unit&);

enum_type_decl_sptr
lookup_enum_type(const enum_type_decl&, const corpus&);

enum_type_decl_sptr
lookup_enum_type(const string&, const corpus&);

enum_type_decl_sptr
lookup_enum_type(const interned_string&, const corpus&);

const type_base_wptrs_type*
lookup_enum_types(const interned_string&, const corpus&);

const type_base_wptrs_type*
lookup_enum_types(const string&, const corpus&);

enum_type_decl_sptr
lookup_enum_type_per_location(const interned_string&, const corpus&);

enum_type_decl_sptr
lookup_enum_type_per_location(const string&, const corpus&);

typedef_decl_sptr
lookup_typedef_type(const typedef_decl&, const translation_unit&);

typedef_decl_sptr
lookup_typedef_type(const typedef_decl&, const corpus&);

typedef_decl_sptr
lookup_typedef_type(const interned_string& type_name,
		    const translation_unit& tu);

typedef_decl_sptr
lookup_typedef_type(const string& type_name, const translation_unit& tu);

typedef_decl_sptr
lookup_typedef_type(const interned_string&, const corpus&);

typedef_decl_sptr
lookup_typedef_type_per_location(const interned_string&, const corpus &);

typedef_decl_sptr
lookup_typedef_type_per_location(const string&, const corpus &);

typedef_decl_sptr
lookup_typedef_type(const string&, const corpus&);

type_base_sptr
lookup_class_or_typedef_type(const string&, const translation_unit&);

type_base_sptr
lookup_class_typedef_or_enum_type(const string&, const translation_unit&);

type_base_sptr
lookup_class_or_typedef_type(const string&, const corpus&);

type_base_sptr
lookup_class_typedef_or_enum_type(const string&, const corpus&);

qualified_type_def_sptr
lookup_qualified_type(const qualified_type_def&, const translation_unit&);

qualified_type_def_sptr
lookup_qualified_type(const string&, const translation_unit&);

qualified_type_def_sptr
lookup_qualified_type(const qualified_type_def&, const corpus&);

qualified_type_def_sptr
lookup_qualified_type(const interned_string&, const corpus&);

pointer_type_def_sptr
lookup_pointer_type(const pointer_type_def&, const translation_unit&);

pointer_type_def_sptr
lookup_pointer_type(const string&, const translation_unit&);

pointer_type_def_sptr
lookup_pointer_type(const type_base_sptr& pointed_to_type,
		    const translation_unit& tu);

pointer_type_def_sptr
lookup_pointer_type(const pointer_type_def&, const corpus&);

pointer_type_def_sptr
lookup_pointer_type(const interned_string&, const corpus&);

const reference_type_def_sptr
lookup_reference_type(const reference_type_def&, const translation_unit&);

const reference_type_def_sptr
lookup_reference_type(const string&, const translation_unit&);

const reference_type_def_sptr
lookup_reference_type(const type_base_sptr& pointed_to_type,
		      bool lvalue_reference,
		      const translation_unit& tu);

reference_type_def_sptr
lookup_reference_type(const reference_type_def&, const corpus&);

reference_type_def_sptr
lookup_reference_type(const interned_string&, const corpus&);

array_type_def_sptr
lookup_array_type(const array_type_def&, const translation_unit&);

array_type_def_sptr
lookup_array_type(const string&, const translation_unit&);

array_type_def_sptr
lookup_array_type(const array_type_def&, const corpus&);

array_type_def_sptr
lookup_array_type(const interned_string&, const corpus&);

function_type_sptr
lookup_function_type(const string&,
		     const translation_unit&);

function_type_sptr
lookup_function_type(const interned_string&,
		     const translation_unit&);

function_type_sptr
lookup_function_type(const function_type&,
		     const translation_unit&);

function_type_sptr
lookup_function_type(const function_type_sptr&,
		     const translation_unit&);

function_type_sptr
lookup_function_type(const function_type&, const corpus&);

function_type_sptr
lookup_function_type(const function_type_sptr&, const corpus&);

function_type_sptr
lookup_function_type(const function_type&, const corpus&);

function_type_sptr
lookup_function_type(const interned_string&, const corpus&);

type_base_sptr
lookup_type(const string&, const translation_unit&);

const type_base_sptr
lookup_type(const type_base_sptr, const translation_unit&);

type_base_sptr
lookup_type(const interned_string&, const corpus&);

type_base_sptr
lookup_type_per_location(const interned_string&, const corpus&);

type_base_sptr
lookup_type(const type_base&, const corpus&);

type_base_sptr
lookup_type(const type_base_sptr&, const corpus&);

type_base_sptr
lookup_type_through_scopes(const std::list<string>&,
			   const translation_unit&);

type_base_sptr
lookup_type_through_translation_units(const string&, const corpus&);

type_base_sptr
lookup_type_from_translation_unit(const string& type_name,
				  const string& tu_path,
				  const corpus& corp);

function_type_sptr
lookup_or_synthesize_fn_type(const function_type_sptr&,
			     const corpus&);

type_base_sptr
synthesize_type_from_translation_unit(const type_base_sptr&,
				      translation_unit&);

function_type_sptr
synthesize_function_type_from_translation_unit(const function_type&,
					       translation_unit&);

const type_base_sptr
lookup_type_in_scope(const string&,
		     const scope_decl_sptr&);

const type_base_sptr
lookup_type_in_scope(const std::list<string>&,
		     const scope_decl_sptr&);

const decl_base_sptr
lookup_var_decl_in_scope(const string&,
			 const scope_decl_sptr&);

const decl_base_sptr
lookup_var_decl_in_scope(const std::list<string>&,
			 const scope_decl_sptr&);

string
demangle_cplus_mangled_name(const string&);

type_base_sptr
type_or_void(const type_base_sptr, const environment*);

type_base_sptr
canonicalize(type_base_sptr);

type_base*
type_has_non_canonicalized_subtype(type_base_sptr t);

bool
type_has_sub_type_changes(type_base_sptr t_v1,
			  type_base_sptr t_v2);

void
keep_type_alive(type_base_sptr t);

size_t
hash_type(const type_base *t);

size_t
hash_type_or_decl(const type_or_decl_base *);

size_t
hash_type_or_decl(const type_or_decl_base_sptr &);

bool
function_decl_is_less_than(const function_decl&f, const function_decl &s);

bool
types_have_similar_structure(const type_base_sptr& first,
			     const type_base_sptr& second,
			     bool indirect_type = false);

bool
types_have_similar_structure(const type_base* first,
			     const type_base* second,
			     bool indirect_type = false);

} // end namespace ir

using namespace abigail::ir;

namespace suppr
{
class suppression_base;

/// Convenience typedef for a shared pointer to a @ref suppression.
typedef shared_ptr<suppression_base> suppression_sptr;

/// Convenience typedef for a vector of @ref suppression_sptr
typedef vector<suppression_sptr> suppressions_type;

} // end namespace suppr

namespace symtab_reader
{

class symtab;
/// Convenience typedef for a shared pointer to a @ref symtab
typedef std::shared_ptr<symtab> symtab_sptr;

} // end namespace symtab_reader

void
dump(const decl_base_sptr, std::ostream&);

void
dump(const decl_base_sptr);

void
dump(const type_base_sptr, std::ostream&);

void
dump(const type_base_sptr);

void
dump(const var_decl_sptr, std::ostream&);

void
dump(const var_decl_sptr);

void
dump(const translation_unit&, std::ostream&);

void
dump(const translation_unit&);

void
dump(const translation_unit_sptr, std::ostream&);

void
dump(const translation_unit_sptr);

void
dump_decl_location(const decl_base&);

void
dump_decl_location(const decl_base*);

void
dump_decl_location(const decl_base_sptr&);

#ifndef ABG_ASSERT
/// This is a wrapper around the 'assert' glibc call.  It allows for
/// its argument to have side effects, so that it keeps working when
/// the code of libabigail is compiled with the NDEBUG macro defined.
#define ABG_ASSERT(cond) do {({bool __abg_cond__ = bool(cond); assert(__abg_cond__); !!__abg_cond__;});} while (false)
#endif

} // end namespace abigail
#endif // __ABG_IRFWD_H__

// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#include <functional>
#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-hash.h"
#include "abg-ir.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{

namespace hashing
{

// Mix 3 32 bits values reversibly.  Borrowed from hashtab.c in gcc tree.
#define abigail_hash_mix(a, b, c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<< 8); \
  c -= a; c -= b; c ^= ((b&0xffffffff)>>13); \
  a -= b; a -= c; a ^= ((c&0xffffffff)>>12); \
  b -= c; b -= a; b = (b ^ (a<<16)) & 0xffffffff; \
  c -= a; c -= b; c = (c ^ (b>> 5)) & 0xffffffff; \
  a -= b; a -= c; a = (a ^ (c>> 3)) & 0xffffffff; \
  b -= c; b -= a; b = (b ^ (a<<10)) & 0xffffffff; \
  c -= a; c -= b; c = (c ^ (b>>15)) & 0xffffffff; \
}

size_t
combine_hashes(size_t val1, size_t val2)
{
  /* the golden ratio; an arbitrary value.  */
  size_t a = 0x9e3779b9;
  abigail_hash_mix(a, val1, val2);
  return val2;
}

/// Compute a stable string hash.
///
/// std::hash has no portability or stability guarantees so is
/// unsuitable where reproducibility is a requirement such as in XML
/// output.
///
/// This is the 32-bit FNV-1a algorithm. The algorithm, reference code
/// and constants are all unencumbered. It is fast and has reasonable
/// distribution properties.
///
/// https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
///
/// @param str the string to hash.
///
/// @return an unsigned 32 bit hash value.
uint32_t
fnv_hash(const std::string& str)
{
  const uint32_t prime = 0x01000193;
  const uint32_t offset_basis = 0x811c9dc5;
  uint32_t hash = offset_basis;
  for (std::string::const_iterator i = str.begin(); i != str.end(); ++i)
    {
      uint8_t byte = *i;
      hash = hash ^ byte;
      hash = hash * prime;
    }
  return hash;
}

}//end namespace hashing

using std::list;
using std::vector;

using namespace abigail::ir;

// See forward declarations in abg-ir.h.

// Definitions.

/// Hash function for an instance of @ref type_base.
///
/// @param t the type to hash.
///
/// @return the type value.
size_t
type_base::hash::operator()(const type_base& t) const
{
  std::hash<size_t> size_t_hash;
  std::hash<string> str_hash;

  size_t v = str_hash(typeid(t).name());
  v = hashing::combine_hashes(v, size_t_hash(t.get_size_in_bits()));
  v = hashing::combine_hashes(v, size_t_hash(t.get_alignment_in_bits()));

  return v;
}

/// Hash function for an instance of @ref type_base.
///
/// @param t the type to hash.
///
/// @return the type value.
size_t
type_base::hash::operator()(const type_base* t) const
{return operator()(*t);}

/// Hash function for an instance of @ref type_base.
///
/// @param t the type to hash.
///
/// @return the type value.
size_t
type_base::hash::operator()(const type_base_sptr t) const
{return operator()(*t);}

struct decl_base::hash
{
  size_t
  operator()(const decl_base& d) const
  {
    std::hash<string> str_hash;

    size_t v = str_hash(typeid(d).name());
    if (!d.get_linkage_name().empty())
      v = hashing::combine_hashes(v, str_hash(d.get_linkage_name()));
    if (!d.get_name().empty())
      v = hashing::combine_hashes(v, str_hash(d.get_qualified_name()));
    if (is_member_decl(d))
      {
	v = hashing::combine_hashes(v, get_member_access_specifier(d));
	v = hashing::combine_hashes(v, get_member_is_static(d));
      }
    return v;
  }
}; // end struct decl_base::hash

struct type_decl::hash
{
  size_t
  operator()(const type_decl& t) const
  {
    decl_base::hash decl_hash;
    type_base::hash type_hash;
    std::hash<string> str_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, decl_hash(t));
    v = hashing::combine_hashes(v, type_hash(t));

    return v;
  }
};

/// Hashing operator for the @ref scope_decl type.
///
/// @param d the scope_decl to hash.
///
/// @return the hash value.
size_t
scope_decl::hash::operator()(const scope_decl& d) const
{
  std::hash<string> hash_string;
  size_t v = hash_string(typeid(d).name());
  for (scope_decl::declarations::const_iterator i =
	 d.get_member_decls().begin();
       i != d.get_member_decls().end();
       ++i)
    v = hashing::combine_hashes(v, (*i)->get_hash());

  return v;
}

/// Hashing operator for the @ref scope_decl type.
///
/// @param d the scope_decl to hash.
///
/// @return the hash value.
size_t
scope_decl::hash::operator()(const scope_decl* d) const
{return d? operator()(*d) : 0;}

struct scope_type_decl::hash
{
  size_t
  operator()(const scope_type_decl& t) const
  {
    decl_base::hash decl_hash;
    type_base::hash type_hash;
    std::hash<string> str_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, decl_hash(t));
    v = hashing::combine_hashes(v, type_hash(t));

    return v;
  }
};

struct qualified_type_def::hash
{
  size_t
  operator()(const qualified_type_def& t) const
  {
    type_base::hash type_hash;
    decl_base::hash decl_hash;
    std::hash<string> str_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, type_hash(t));
    v = hashing::combine_hashes(v, decl_hash(t));
    v = hashing::combine_hashes(v, t.get_cv_quals());
    return v;
  }
};

struct pointer_type_def::hash
{
  size_t
  operator()(const pointer_type_def& t) const
  {
    std::hash<string> str_hash;
    type_base::hash type_base_hash;
    decl_base::hash decl_hash;
    type_base::shared_ptr_hash hash_type_ptr;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, decl_hash(t));
    v = hashing::combine_hashes(v, type_base_hash(t));
    v = hashing::combine_hashes(v, hash_type_ptr(t.get_pointed_to_type()));
    return v ;
  }
};

struct reference_type_def::hash
{
  size_t
  operator()(const reference_type_def& t)
  {
    std::hash<string> hash_str;
    type_base::hash hash_type_base;
    decl_base::hash hash_decl;
    type_base::shared_ptr_hash hash_type_ptr;

    size_t v = hash_str(typeid(t).name());
    v = hashing::combine_hashes(v, hash_str(t.is_lvalue()
					    ? "lvalue"
					    : "rvalue"));
    v = hashing::combine_hashes(v, hash_type_base(t));
    v = hashing::combine_hashes(v, hash_decl(t));
    v = hashing::combine_hashes(v, hash_type_ptr(t.get_pointed_to_type()));
    return v;
  }
};

struct array_type_def::subrange_type::hash
{
  size_t
  operator()(const array_type_def::subrange_type& s) const
  {
    std::hash<int> hash_size_t;
    size_t v = hash_size_t(hash_size_t(s.get_lower_bound()));
    v = hashing::combine_hashes(v, hash_size_t(s.get_upper_bound()));
    return v;
  }
};

struct array_type_def::hash
{
  size_t
  operator()(const array_type_def& t)
  {
    std::hash<string> hash_str;
    type_base::hash hash_type_base;
    decl_base::hash hash_decl;
    type_base::shared_ptr_hash hash_type_ptr;
    array_type_def::subrange_type::hash hash_subrange;

    size_t v = hash_str(typeid(t).name());

    v = hashing::combine_hashes(v, hash_type_base(t));
    v = hashing::combine_hashes(v, hash_decl(t));
    v = hashing::combine_hashes(v, hash_type_ptr(t.get_element_type()));

    for (vector<array_type_def::subrange_sptr >::const_iterator i =
	   t.get_subranges().begin();
	 i != t.get_subranges().end();
	 ++i)
      v = hashing::combine_hashes(v, hash_subrange(**i));

    return v;
  }
};

struct enum_type_decl::hash
{
  size_t
  operator()(const enum_type_decl& t) const
  {
    std::hash<string> str_hash;
    decl_base::hash decl_hash;
    type_base::shared_ptr_hash type_ptr_hash;
    std::hash<size_t> size_t_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, decl_hash(t));
    v = hashing::combine_hashes(v, type_ptr_hash(t.get_underlying_type()));
    for (enum_type_decl::enumerators::const_iterator i =
	   t.get_enumerators().begin();
	 i != t.get_enumerators().end();
	 ++i)
      {
	v = hashing::combine_hashes(v, str_hash(i->get_name()));
	v = hashing::combine_hashes(v, size_t_hash(i->get_value()));
      }
    return v;
  }
};

struct typedef_decl::hash
{
  size_t
  operator()(const typedef_decl& t) const
  {
    std::hash<string> str_hash;
    type_base::hash hash_type;
    decl_base::hash decl_hash;
    type_base::shared_ptr_hash type_ptr_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, hash_type(t));
    v = hashing::combine_hashes(v, decl_hash(t));
    v = hashing::combine_hashes(v, type_ptr_hash(t.get_underlying_type()));
    return v;
  }
 };

/// Compute a hash for an instance @ref var_decl.
///
/// Note that this function caches the hashing value the
/// decl_base::hash_ data member of the input instance and re-uses it
/// when it is already calculated.
///
/// @param t the instance of @ref var_decl to compute the hash for.
///
/// @return the calculated hash value, or the one that was previously
/// calculated.
size_t
var_decl::hash::operator()(const var_decl& t) const
{
  std::hash<string> hash_string;
  decl_base::hash hash_decl;
  type_base::shared_ptr_hash hash_type_ptr;
  std::hash<size_t> hash_size_t;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_decl(t));
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_type()));

  if (is_data_member(t) && get_data_member_is_laid_out(t))
    {
      v = hashing::combine_hashes(v, hash_decl(*t.get_scope()));
      v = hashing::combine_hashes(v, hash_size_t(get_data_member_offset(t)));
    }

  return v;
}

/// Compute a hash for a pointer to @ref var_decl.
///
/// @param t the pointer to @ref var_decl to compute the hash for.
///
/// @return the calculated hash value
size_t
var_decl::hash::operator()(const var_decl* t) const
{return operator()(*t);}

/// Compute a hash value for an instance of @ref function_decl.
///
/// Note that this function caches the resulting hash in the
/// decl_base::hash_ data member of the instance of @ref
/// function_decl, and just returns if it is already calculated.
///
/// @param t the function to calculate the hash for.
///
/// @return the hash value.
size_t
function_decl::hash::operator()(const function_decl& t) const
{
  std::hash<int> hash_int;
  std::hash<size_t> hash_size_t;
  std::hash<bool> hash_bool;
  std::hash<string> hash_string;
  decl_base::hash hash_decl_base;
  type_base::shared_ptr_hash hash_type_ptr;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_decl_base(t));
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_type()));
  v = hashing::combine_hashes(v, hash_bool(t.is_declared_inline()));
  v = hashing::combine_hashes(v, hash_int(t.get_binding()));
  if (is_member_function(t))
    {
      bool is_ctor = get_member_function_is_ctor(t),
	is_dtor = get_member_function_is_dtor(t),
	is_static = get_member_is_static(t),
	is_const = get_member_function_is_const(t);
      size_t voffset = get_member_function_vtable_offset(t);

      v = hashing::combine_hashes(v, hash_bool(is_ctor));
      v = hashing::combine_hashes(v, hash_bool(is_dtor));
      v = hashing::combine_hashes(v, hash_bool(is_static));
      v = hashing::combine_hashes(v, hash_bool(is_const));
      if (!is_static && !is_ctor)
	v = hashing::combine_hashes(v, hash_size_t(voffset));
    }

  return v;
}

/// Compute a hash for a pointer to @ref function_decl.
///
/// @param t the pointer to @ref function_decl to compute the hash for.
///
/// @return the calculated hash value
size_t
function_decl::hash::operator()(const function_decl* t) const
{return operator()(*t);}

size_t
function_decl::parameter::hash::operator()
  (const function_decl::parameter& p) const
{
  type_base::shared_ptr_hash hash_type_ptr;
  std::hash<bool> hash_bool;
  std::hash<unsigned> hash_unsigned;
  size_t v = hash_type_ptr(p.get_type());
  v = hashing::combine_hashes(v, hash_unsigned(p.get_index()));
  v = hashing::combine_hashes(v, hash_bool(p.get_variadic_marker()));
  return v;
}

size_t
function_decl::parameter::hash::operator()
  (const function_decl::parameter* p) const
{return operator()(*p);}

size_t
function_decl::parameter::hash::operator()
  (const function_decl::parameter_sptr p) const
{return operator()(p.get());}

/// Hashing functor for the @ref method_type type.
struct method_type::hash
{
  size_t
  operator()(const method_type& t) const
  {
    std::hash<string> hash_string;
    type_base::shared_ptr_hash hash_type_ptr;
    function_decl::parameter::hash hash_parameter;

    size_t v = hash_string(typeid(t).name());
    string class_name = t.get_class_type()->get_qualified_name();
    v = hashing::combine_hashes(v, hash_string(class_name));
    v = hashing::combine_hashes(v, hash_type_ptr(t.get_return_type()));
    vector<shared_ptr<function_decl::parameter> >::const_iterator i =
      t.get_first_non_implicit_parm();

    for (; i != t.get_parameters().end(); ++i)
      v = hashing::combine_hashes(v, hash_parameter(**i));

    return v;
  }

  size_t
  operator()(const method_type* t)
  {return operator()(*t);}

  size_t
  operator()(const method_type_sptr t)
  {return operator()(t.get());}
}; // end struct method_type::hash

// <struct function_type::hash stuff>

/// Hashing function for @ref function_type.
///
/// @param t the function type to hash.
///
/// @return the resulting hash value.
size_t
function_type::hash::operator()(const function_type& t) const
{
  std::hash<string> hash_string;
  type_base::shared_ptr_hash hash_type_ptr;
  function_decl::parameter::hash hash_parameter;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_return_type()));
  for (vector<shared_ptr<function_decl::parameter> >::const_iterator i =
	 t.get_first_non_implicit_parm();
       i != t.get_parameters().end();
       ++i)
    v = hashing::combine_hashes(v, hash_parameter(**i));
  return v;
}

/// Hashing function for a pointer to @ref function_type.
///
/// @param t the pointer to @ref function_type to hash.
///
/// @return the resulting hash value.
size_t
function_type::hash::operator()(const function_type* t) const
{
  if (const method_type* m = dynamic_cast<const method_type*>(t))
    {
      method_type::hash h;
      return h(m);
    }
  return operator()(*t);
}

/// Hashing function for a shared pointer to @ref function_type.
///
/// @param t the pointer to @ref function_type to hash.
///
/// @return the resulting hash value.
size_t
function_type::hash::operator()(const function_type_sptr t) const
{return operator()(t.get());}

// </struct function_type::hash stuff>

size_t
member_base::hash::operator()(const member_base& m) const
{
  std::hash<int> hash_int;
  return hash_int(m.get_access_specifier());
}

size_t
class_decl::base_spec::hash::operator()(const base_spec& t) const
{
  member_base::hash hash_member;
  type_base::shared_ptr_hash hash_type_ptr;
  std::hash<size_t> hash_size;
  std::hash<bool> hash_bool;
  std::hash<string> hash_string;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_member(t));
  v = hashing::combine_hashes(v, hash_size(t.get_offset_in_bits()));
  v = hashing::combine_hashes(v, hash_bool(t.get_is_virtual()));
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_base_class()));
  return v;
}

size_t
member_function_template::hash::operator()
  (const member_function_template& t) const
{
  std::hash<bool> hash_bool;
  function_tdecl::hash hash_function_tdecl;
  member_base::hash hash_member;
  std::hash<string> hash_string;

  size_t v = hash_member(t);
  string n = t.get_qualified_name();
  v = hashing::combine_hashes(v, hash_string(n));
  v = hashing::combine_hashes(v, hash_function_tdecl(t));
  v = hashing::combine_hashes(v, hash_bool(t.is_constructor()));
  v = hashing::combine_hashes(v, hash_bool(t.is_const()));
  return v;
}

size_t
member_class_template::hash::operator()
  (const member_class_template& t) const
{
  member_base::hash hash_member;
  class_tdecl::hash hash_class_tdecl;
  std::hash<string> hash_string;

  size_t v = hash_member(t);
  string n = t.get_qualified_name();
  v = hashing::combine_hashes(v, hash_string(n));
  v = hashing::combine_hashes(v, hash_class_tdecl(t));
  return v;
}

/// Compute a hash for a @ref class_or_union
///
/// @param t the class_or_union for which to compute the hash value.
///
/// @return the computed hash value.
size_t
class_or_union::hash::operator()(const class_or_union& t) const
{
  if (t.hashing_started()
      || (t.get_is_declaration_only() && !t.get_definition_of_declaration()))
    // All non-resolved decl-only types have a hash of zero.  Their hash
    // will differ from the resolved hash, but then at least, having
    // it be zero will give a hint that we couldn't actually compute
    // the hash.
    return 0;

  // If the type is decl-only and now has a definition, then hash its
  // definition instead.

  if (t.get_is_declaration_only())
    {
      ABG_ASSERT(t.get_definition_of_declaration());
      size_t v = operator()
	(*is_class_or_union_type(t.get_definition_of_declaration()));
      return v;
    }

  ABG_ASSERT(!t.get_is_declaration_only());

  std::hash<string> hash_string;
  scope_type_decl::hash hash_scope_type;
  var_decl::hash hash_data_member;
  member_function_template::hash hash_member_fn_tmpl;
  member_class_template::hash hash_member_class_tmpl;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_scope_type(t));

  t.hashing_started(true);

  // Hash data members.
  for (class_decl::data_members::const_iterator d =
	 t.get_non_static_data_members().begin();
       d != t.get_non_static_data_members().end();
       ++d)
    v = hashing::combine_hashes(v, hash_data_member(**d));

  // Do not hash member functions. All of them are not necessarily
  // emitted per class, in a given TU so do not consider them when
  // hashing a class.

  // Hash member function templates
  for (member_function_templates::const_iterator f =
	 t.get_member_function_templates().begin();
       f != t.get_member_function_templates().end();
       ++f)
    v = hashing::combine_hashes(v, hash_member_fn_tmpl(**f));

  // Hash member class templates
  for (member_class_templates::const_iterator c =
	 t.get_member_class_templates().begin();
       c != t.get_member_class_templates().end();
       ++c)
    v = hashing::combine_hashes(v, hash_member_class_tmpl(**c));

  t.hashing_started(false);

  return v;
};

/// Compute a hash for a @ref class_or_union
///
/// @param t the class_or_union for which to compute the hash value.
///
/// @return the computed hash value.
size_t
class_or_union::hash::operator()(const class_or_union *t) const
{return t ? operator()(*t) : 0;}

/// Compute a hash for a @ref class_decl
///
/// @param t the class_decl for which to compute the hash value.
///
/// @return the computed hash value.
size_t
class_decl::hash::operator()(const class_decl& t) const
{
  if (t.hashing_started()
      || (t.get_is_declaration_only() && !t.get_definition_of_declaration()))
    // All non-resolved decl-only types have a hash of zero.  Their hash
    // will differ from the resolved hash, but then at least, having
    // it be zero will give a hint that we couldn't actually compute
    // the hash.
    return 0;


  // If the type is decl-only and now has a definition, then hash its
  // definition instead.

  if (t.get_is_declaration_only())
    {
      ABG_ASSERT(t.get_definition_of_declaration());
      size_t v = operator()(*is_class_type(t.get_definition_of_declaration()));
      return v;
    }

  ABG_ASSERT(!t.get_is_declaration_only());

  std::hash<string> hash_string;
  class_decl::base_spec::hash hash_base;
  class_or_union::hash hash_class_or_union;

  size_t v = hash_string(typeid(t).name());

  t.hashing_started(true);

  // Hash bases.
  for (class_decl::base_specs::const_iterator b =
	 t.get_base_specifiers().begin();
       b != t.get_base_specifiers().end();
       ++b)
    {
      class_decl_sptr cl = (*b)->get_base_class();
      v = hashing::combine_hashes(v, hash_base(**b));
    }

  v = hashing::combine_hashes(v, hash_class_or_union(t));

  t.hashing_started(false);

  return v;
}

/// Compute a hash for a @ref class_decl
///
/// @param t the class_decl for which to compute the hash value.
///
/// @return the computed hash value.
size_t
class_decl::hash::operator()(const class_decl* t) const
{return t ? operator()(*t) : 0;}

struct template_parameter::hash
{
  size_t
  operator()(const template_parameter& t) const
  {
    // Let's avoid infinite recursion triggered from the fact that
    // hashing a template parameter triggers hashing the enclosed
    // template decl, which in turn triggers the hashing of its
    // template parameters; so the initial template parameter that
    // triggered the hashing could be hashed again ...
    if (t.get_hashing_has_started())
      return 0;

    t.set_hashing_has_started(true);

    std::hash<unsigned> hash_unsigned;
    std::hash<std::string> hash_string;
    template_decl::hash hash_template_decl;

    size_t v = hash_string(typeid(t).name());
    v = hashing::combine_hashes(v, hash_unsigned(t.get_index()));
    v = hashing::combine_hashes(v, hash_template_decl
				(*t.get_enclosing_template_decl()));

    t.set_hashing_has_started(false);

    return v;
  }
};

struct template_parameter::dynamic_hash
{
  size_t
  operator()(const template_parameter* t) const;
};

struct template_parameter::shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<template_parameter> t) const
  {return template_parameter::dynamic_hash()(t.get());}
};

size_t
template_decl::hash::operator()(const template_decl& t) const
{
  std::hash<string> hash_string;
  template_parameter::shared_ptr_hash hash_template_parameter;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_string(t.get_qualified_name()));

  for (list<template_parameter_sptr>::const_iterator p =
	 t.get_template_parameters().begin();
       p != t.get_template_parameters().end();
       ++p)
    if (!(*p)->get_hashing_has_started())
      v = hashing::combine_hashes(v, hash_template_parameter(*p));

  return v;
}

struct type_tparameter::hash
{
  size_t
  operator()(const type_tparameter& t) const
  {
    std::hash<string> hash_string;
    template_parameter::hash hash_template_parameter;
    type_decl::hash hash_type;

    size_t v = hash_string(typeid(t).name());
    v = hashing::combine_hashes(v, hash_template_parameter(t));
    v = hashing::combine_hashes(v, hash_type(t));

    return v;
  }
};

/// Compute a hash value for a @ref non_type_tparameter
///
/// @param t the non_type_tparameter for which to compute the value.
///
/// @return the computed hash value.
size_t
non_type_tparameter::hash::operator()(const non_type_tparameter& t) const
{
  template_parameter::hash hash_template_parameter;
  std::hash<string> hash_string;
  type_base::shared_ptr_hash hash_type;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_template_parameter(t));
  v = hashing::combine_hashes(v, hash_string(t.get_name()));
  v = hashing::combine_hashes(v, hash_type(t.get_type()));

  return v;
}

/// Compute a hash value for a @ref non_type_tparameter
///
/// @param t the non_type_tparameter to compute the hash value for.
///
/// @return the computed hash value.
size_t
non_type_tparameter::hash::operator()(const non_type_tparameter* t) const
{return t ? operator()(*t) : 0;}

struct template_tparameter::hash
{
  size_t
  operator()(const template_tparameter& t) const
  {
    std::hash<string> hash_string;
    type_tparameter::hash hash_template_type_parm;
    template_decl::hash hash_template_decl;

    size_t v = hash_string(typeid(t).name());
    v = hashing::combine_hashes(v, hash_template_type_parm(t));
    v = hashing::combine_hashes(v, hash_template_decl(t));

    return v;
  }
};

size_t
template_parameter::dynamic_hash::
operator()(const template_parameter* t) const
{
  if (const template_tparameter* p =
      dynamic_cast<const template_tparameter*>(t))
    return template_tparameter::hash()(*p);
  else if (const type_tparameter* p =
	   dynamic_cast<const type_tparameter*>(t))
    return type_tparameter::hash()(*p);
  if (const non_type_tparameter* p =
      dynamic_cast<const non_type_tparameter*>(t))
    return non_type_tparameter::hash()(*p);

  // Poor man's fallback.
  return template_parameter::hash()(*t);
}

/// Compute a hash value for a @ref type_composition type.
///
/// @param t the type_composition to compute the hash value for.
///
/// @return the computed hash value.
size_t
type_composition::hash::operator()(const type_composition& t) const
{
  std::hash<string> hash_string;
  type_base::dynamic_hash hash_type;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_type(t.get_composed_type().get()));
  return v;
}

/// Compute a hash value for a @ref type_composition type.
///
/// @param t the type_composition to compute the hash value for.
///
/// @return the computed hash value.
size_t
type_composition::hash::operator()(const type_composition* t) const
{return t ? operator()(*t): 0;}

size_t
function_tdecl::hash::
operator()(const function_tdecl& t) const
{
  std::hash<string> hash_string;
  decl_base::hash hash_decl_base;
  template_decl::hash hash_template_decl;
  function_decl::hash hash_function_decl;

  size_t v = hash_string(typeid(t).name());

  v = hashing::combine_hashes(v, hash_decl_base(t));
  v = hashing::combine_hashes(v, hash_template_decl(t));
  if (t.get_pattern())
    v = hashing::combine_hashes(v, hash_function_decl(*t.get_pattern()));

  return v;
}

size_t
function_tdecl::shared_ptr_hash::
operator()(const shared_ptr<function_tdecl> f) const
{
  function_tdecl::hash hash_fn_tmpl_decl;
  if (f)
    return hash_fn_tmpl_decl(*f);
  return 0;
}

size_t
class_tdecl::hash::
operator()(const class_tdecl& t) const
{
  std::hash<string> hash_string;
  decl_base::hash hash_decl_base;
  template_decl::hash hash_template_decl;
  class_decl::hash hash_class_decl;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_decl_base(t));
  v = hashing::combine_hashes(v, hash_template_decl(t));
  if (t.get_pattern())
    v = hashing::combine_hashes(v, hash_class_decl(*t.get_pattern()));

  return v;
}

size_t
class_tdecl::shared_ptr_hash::
operator()(const shared_ptr<class_tdecl> t) const
{
  class_tdecl::hash hash_class_tmpl_decl;

  if (t)
    return hash_class_tmpl_decl(*t);
  return 0;
}

/// A hashing function for type declarations.
///
/// This function gets the dynamic type of the actual type
/// declaration and calls the right hashing function for that type.
///
/// Note that each time a new type declaration kind is added to the
/// system, this function needs to be updated.  For a given
/// inheritance hierarchy, make sure to handle the most derived type
/// first.
///
/// FIXME: This hashing function is not maintained and is surely
/// broken in subtle ways.  In pratice, the various *::hash functors
/// should be audited before they are used here.  They should all
/// match what is done in the 'equals' functions in abg-ir.cc.
///
/// @param t a pointer to the type declaration to be hashed
///
/// @return the resulting hash
size_t
type_base::dynamic_hash::operator()(const type_base* t) const
{
  if (t == 0)
    return 0;

  if (const member_function_template* d =
      dynamic_cast<const member_function_template*>(t))
    return member_function_template::hash()(*d);
  if (const member_class_template* d =
      dynamic_cast<const member_class_template*>(t))
    return member_class_template::hash()(*d);
  if (const template_tparameter* d =
      dynamic_cast<const template_tparameter*>(t))
    return template_tparameter::hash()(*d);
  if (const type_tparameter* d =
      dynamic_cast<const type_tparameter*>(t))
    return type_tparameter::hash()(*d);
  if (const type_decl* d = dynamic_cast<const type_decl*> (t))
    return type_decl::hash()(*d);
  if (const qualified_type_def* d = dynamic_cast<const qualified_type_def*>(t))
    return qualified_type_def::hash()(*d);
  if (const pointer_type_def* d = dynamic_cast<const pointer_type_def*>(t))
    return pointer_type_def::hash()(*d);
  if (const reference_type_def* d = dynamic_cast<const reference_type_def*>(t))
    return reference_type_def::hash()(*d);
  if (const array_type_def* d = dynamic_cast<const array_type_def*>(t))
    return array_type_def::hash()(*d);
  if (const enum_type_decl* d = dynamic_cast<const enum_type_decl*>(t))
    return enum_type_decl::hash()(*d);
  if (const typedef_decl* d = dynamic_cast<const typedef_decl*>(t))
    return typedef_decl::hash()(*d);
  if (const class_decl* d = dynamic_cast<const class_decl*>(t))
    return class_decl::hash()(*d);
  if (const union_decl* d = dynamic_cast<const union_decl*>(t))
    return union_decl::hash()(*d);
  if (const scope_type_decl* d = dynamic_cast<const scope_type_decl*>(t))
    return scope_type_decl::hash()(*d);
  if (const method_type* d = dynamic_cast<const method_type*>(t))
    return method_type::hash()(*d);
  if (const function_type* d = dynamic_cast<const function_type*>(t))
    return function_type::hash()(*d);

  // Poor man's fallback case.
  return type_base::hash()(*t);
}

size_t
type_base::shared_ptr_hash::operator()(const shared_ptr<type_base> t) const
{return type_base::dynamic_hash()(t.get());}

}//end namespace abigail

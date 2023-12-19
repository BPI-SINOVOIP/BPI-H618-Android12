// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2016-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This contains the private implementation of the suppression engine
/// of libabigail.

#ifndef __ABG_IR_PRIV_H__
#define __ABG_IR_PRIV_H__

#include <string>

#include "abg-ir.h"

namespace abigail
{

namespace ir
{

using std::string;

/// The internal representation of an integral type.
///
/// This is a "utility type" used internally to canonicalize the name
/// of fundamental integral types, so that "unsignd long" and "long
/// unsined int" end-up having the same name.
class integral_type
{
public:
  /// The possible base types of integral types.  We might have
  /// forgotten many of these, so do not hesitate to add new ones.
  ///
  /// If you do add new ones, please also consider updating functions
  /// parse_base_integral_type and integral_type::to_string.
  enum base_type
  {
    /// The "int" base type.
    INT_BASE_TYPE,
    /// The "char" base type.
    CHAR_BASE_TYPE,
    /// The "bool" base type in C++ or "_Bool" in C11.
    BOOL_BASE_TYPE,
    /// The "double" base type.
    DOUBLE_BASE_TYPE,
    /// The "float" base type.
    FLOAT_BASE_TYPE,
    /// The "char16_t base type.
    CHAR16_T_BASE_TYPE,
    /// The "char32_t" base type.
    CHAR32_T_BASE_TYPE,
    /// The "wchar_t" base type.
    WCHAR_T_BASE_TYPE
  };

  /// The modifiers of the base types above.  Several modifiers can be
  /// combined for a given base type.  The presence of modifiers is
  /// usually modelled by a bitmap of modifiers.
  ///
  /// If you add a new modifier, please consider updating functions
  /// parse_integral_type_modifier and integral_type::to_string.
  enum modifiers_type
  {
    NO_MODIFIER = 0,
    /// The "signed" modifier.
    SIGNED_MODIFIER = 1,
    /// The "unsigned" modier.
    UNSIGNED_MODIFIER = 1 << 1,
    /// The "short" modifier.
    SHORT_MODIFIER = 1 << 2,
    /// The "long" modifier.
    LONG_MODIFIER = 1 << 3,
    /// The "long long" modifier.
    LONG_LONG_MODIFIER = 1 << 4
  };

private:
  base_type	base_;
  modifiers_type modifiers_;

public:

  integral_type();
  integral_type(const string& name);
  integral_type(base_type, modifiers_type);

  base_type
  get_base_type() const;

  modifiers_type
  get_modifiers() const;

  bool
  operator==(const integral_type&) const;

  string
  to_string() const;

  operator string() const;
}; // end class integral_type

integral_type::modifiers_type
operator|(integral_type::modifiers_type l, integral_type::modifiers_type r);

integral_type::modifiers_type
operator&(integral_type::modifiers_type l, integral_type::modifiers_type r);

integral_type::modifiers_type&
operator|=(integral_type::modifiers_type& l, integral_type::modifiers_type r);

bool
parse_integral_type(const string& type_name,
		    integral_type& type);

/// Private type to hold private members of @ref translation_unit
struct translation_unit::priv
{
  const environment*				env_;
  corpus*					corp;
  bool						is_constructed_;
  char						address_size_;
  language					language_;
  std::string					path_;
  std::string					comp_dir_path_;
  std::string					abs_path_;
  location_manager				loc_mgr_;
  mutable global_scope_sptr			global_scope_;
  mutable vector<type_base_sptr>		synthesized_types_;
  vector<function_type_sptr>			live_fn_types_;
  type_maps					types_;


  priv(const environment* env)
    : env_(env),
      corp(),
      is_constructed_(),
      address_size_(),
      language_(LANG_UNKNOWN)
  {}

  ~priv()
  {}

  type_maps&
  get_types()
  {return types_;}
}; // end translation_unit::priv

} // end namespace ir

} // end namespace abigail

#endif // __ABG_IR_PRIV_H__


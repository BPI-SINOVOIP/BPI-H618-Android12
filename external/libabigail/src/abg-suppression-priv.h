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

#ifndef __ABG_SUPPRESSION_PRIV_H__
#define __ABG_SUPPRESSION_PRIV_H__

#include "abg-fwd.h"
#include "abg-regex.h"
#include "abg-sptr-utils.h"
#include "abg-suppression.h"

namespace abigail
{

namespace suppr
{

// <suppression_base stuff>

/// The private data of @ref suppression_base.
class suppression_base::priv
{
  bool					is_artificial_;
  bool					drops_artifact_;
  string				label_;
  string				file_name_regex_str_;
  mutable regex::regex_t_sptr		file_name_regex_;
  string				file_name_not_regex_str_;
  mutable regex::regex_t_sptr		file_name_not_regex_;
  string				soname_regex_str_;
  mutable regex::regex_t_sptr		soname_regex_;
  string				soname_not_regex_str_;
  mutable regex::regex_t_sptr		soname_not_regex_;

public:
  priv()
    : is_artificial_(),
      drops_artifact_()
  {}

  priv(const string& label)
    : is_artificial_(),
      drops_artifact_(),
      label_(label)
  {}

  priv(const string& label,
       const string& file_name_regex_str,
       const string& file_name_not_regex_str)
    : is_artificial_(),
      drops_artifact_(),
      label_(label),
      file_name_regex_str_(file_name_regex_str),
      file_name_not_regex_str_(file_name_not_regex_str)
  {}

  friend class suppression_base;

  /// Get the regular expression object associated to the 'file_name_regex'
  /// property of @ref suppression_base.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'file_name_regex' property of @ref suppression_base is
  /// empty then this method returns nil.
  const regex::regex_t_sptr&
  get_file_name_regex() const
  {
    if (!file_name_regex_ && !file_name_regex_str_.empty())
      file_name_regex_ = regex::compile(file_name_regex_str_);
    return file_name_regex_;
  }

  /// Get the regular expression object associated to the
  /// 'file_name_not_regex' property of @ref suppression_base.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'file_name_not_regex' property of @ref suppression_base
  /// is empty then this method returns nil.
  const regex::regex_t_sptr&
  get_file_name_not_regex() const
  {
    if (!file_name_not_regex_ && !file_name_not_regex_str_.empty())
      file_name_not_regex_ = regex::compile(file_name_not_regex_str_);
    return file_name_not_regex_;
  }

  /// Get the regular expression object associated to the
  /// 'soname_regex' property of @ref suppression_base.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'soname_regex' property of @ref suppression_base is empty
  /// then this method returns nil.
  const regex::regex_t_sptr&
  get_soname_regex() const
  {
    if (!soname_regex_ && !soname_regex_str_.empty())
      soname_regex_ = regex::compile(soname_regex_str_);
    return soname_regex_;
  }

  /// Get the regular expression object associated to the
  /// 'soname_not_regex' property of @ref suppression_base.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'soname_not_regex' property of @ref suppression_base is
  /// empty then this method returns nil.
  const regex::regex_t_sptr&
  get_soname_not_regex() const
  {
    if (!soname_not_regex_ && !soname_not_regex_str_.empty())
      soname_not_regex_ = regex::compile(soname_not_regex_str_);
    return soname_not_regex_;
  }

  /// Test if the current suppression matches a given SONAME.
  ///
  /// @param soname the SONAME to consider.
  ///
  /// @return true iff the suppression matches the SONAME denoted by
  /// @p soname.
  ///
  /// Note that if the suppression contains no property that is
  /// related to SONAMEs, the function returns false.
  bool
  matches_soname(const string& soname) const
  {
    bool has_regexp = false;
    if (regex::regex_t_sptr regexp = get_soname_regex())
      {
	has_regexp = true;
	if (!regex::match(regexp, soname))
	  return false;
      }

    if (regex::regex_t_sptr regexp = get_soname_not_regex())
      {
	has_regexp = true;
	if (regex::match(regexp, soname))
	  return false;
      }

      if (!has_regexp)
	return false;

    return true;
  }

  /// Test if the current suppression matches the full file path to a
  /// given binary.
  ///
  /// @param binary_name the full path to the binary.
  ///
  /// @return true iff the suppression matches the path denoted by @p
  /// binary_name.
  ///
  /// Note that if the suppression contains no property that is
  /// related to file name, the function returns false.
  bool
  matches_binary_name(const string& binary_name) const
  {
    bool has_regexp = false;

    if (regex::regex_t_sptr regexp = get_file_name_regex())
      {
	has_regexp = true;
	if (!regex::match(regexp, binary_name))
	  return false;
      }

    if (regex::regex_t_sptr regexp = get_file_name_not_regex())
      {
	has_regexp = true;
	if (regex::match(regexp, binary_name))
	  return false;
      }

    if (!has_regexp)
      return false;

    return true;
  }

}; // end clas suppression_base::priv

// </suppression_base stuff>

// <function_suppression stuff>

class function_suppression::parameter_spec::priv
{
  friend class function_suppression::parameter_spec;
  friend class function_suppression;

  size_t				index_;
  string				type_name_;
  string				type_name_regex_str_;
  mutable regex::regex_t_sptr		type_name_regex_;

  priv()
    : index_()
  {}

  priv(size_t i, const string& tn)
    : index_(i), type_name_(tn)
  {}

  priv(size_t i, const string& tn, const string& tn_regex)
    : index_(i), type_name_(tn), type_name_regex_str_(tn_regex)
  {}

  const regex::regex_t_sptr
  get_type_name_regex() const
  {
    if (!type_name_regex_ && !type_name_regex_str_.empty())
      type_name_regex_ = regex::compile(type_name_regex_str_);
    return type_name_regex_;
  }
}; // end class function_suppression::parameter_spec::priv


/// The type of the private data of the @ref function_suppression
/// type.
struct function_suppression::priv
{
  friend class function_suppression;

  change_kind				change_kind_;
  string				name_;
  string				name_regex_str_;
  mutable regex::regex_t_sptr		name_regex_;
  string				name_not_regex_str_;
  mutable regex::regex_t_sptr		name_not_regex_;
  string				return_type_name_;
  string				return_type_regex_str_;
  mutable regex::regex_t_sptr		return_type_regex_;
  parameter_specs_type			parm_specs_;
  string				symbol_name_;
  string				symbol_name_regex_str_;
  mutable regex::regex_t_sptr		symbol_name_regex_;
  string				symbol_name_not_regex_str_;
  mutable regex::regex_t_sptr		symbol_name_not_regex_;
  string				symbol_version_;
  string				symbol_version_regex_str_;
  mutable regex::regex_t_sptr		symbol_version_regex_;
  bool					allow_other_aliases_;

  priv():
    change_kind_(ALL_CHANGE_KIND),
    allow_other_aliases_(true)
  {}

  priv(const string&			name,
       const string&			name_regex_str,
       const string&			return_type_name,
       const string&			return_type_regex_str,
       const parameter_specs_type&	parm_specs,
       const string&			symbol_name,
       const string&			symbol_name_regex_str,
       const string&			symbol_version,
       const string&			symbol_version_regex_str)
    : change_kind_(ALL_CHANGE_KIND),
      name_(name),
      name_regex_str_(name_regex_str),
      return_type_name_(return_type_name),
      return_type_regex_str_(return_type_regex_str),
      parm_specs_(parm_specs),
      symbol_name_(symbol_name),
      symbol_name_regex_str_(symbol_name_regex_str),
      symbol_version_(symbol_version),
      symbol_version_regex_str_(symbol_version_regex_str),
      allow_other_aliases_(true)
  {}


  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::name_regex_str_..
  const regex::regex_t_sptr
  get_name_regex() const
  {
    if (!name_regex_ && !name_regex_str_.empty())
      name_regex_ = regex::compile(name_regex_str_);
    return name_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::name_not_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::name_not_regex_str_..
  const regex::regex_t_sptr
  get_name_not_regex() const
  {
    if (!name_not_regex_ && !name_not_regex_str_.empty())
      name_not_regex_ = regex::compile(name_not_regex_str_);
    return name_not_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::return_type_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::return_type_regex_str_.
  const regex::regex_t_sptr
  get_return_type_regex() const
  {
    if (!return_type_regex_ && !return_type_regex_str_.empty())
      return_type_regex_ = regex::compile(return_type_regex_str_);
    return return_type_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::symbol_name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::symbol_name_regex_str_.
  const regex::regex_t_sptr
  get_symbol_name_regex() const
  {
    if (!symbol_name_regex_ && !symbol_name_regex_str_.empty())
      symbol_name_regex_ = regex::compile(symbol_name_regex_str_);
    return symbol_name_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::symbol_name_not_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::symbol_name_not_regex_str_.
  const regex::regex_t_sptr
  get_symbol_name_not_regex() const
  {
    if (!symbol_name_not_regex_ && !symbol_name_not_regex_str_.empty())
      symbol_name_not_regex_ = regex::compile(symbol_name_not_regex_str_);
    return symbol_name_not_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::symbol_version_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::symbol_version_regex_str_.
  const regex::regex_t_sptr
  get_symbol_version_regex() const
  {
    if (!symbol_version_regex_ && !symbol_version_regex_str_.empty())
      symbol_version_regex_ = regex::compile(symbol_version_regex_str_);
    return symbol_version_regex_;
  }
}; // end class function_suppression::priv

bool
suppression_matches_function_name(const suppr::function_suppression& s,
				  const string& fn_name);

bool
suppression_matches_function_sym_name(const suppr::function_suppression& s,
				      const string& fn_linkage_name);

bool
suppression_matches_variable_name(const suppr::variable_suppression& s,
				  const string& var_name);


bool
suppression_matches_variable_sym_name(const suppr::variable_suppression& s,
				      const string& var_linkage_name);

/// Test if a given function denoted by its name and linkage name is
/// suppressed by any of the suppression specifications associated to
/// a given read context used to build the current internal
/// representation of ABI corpus.
///
/// @param ctxt the reading context of interest.
///
/// @param fn_name the name of the function that a specification can
/// match.
///
/// @param fn_linkage_name the linkage name of the function that a
/// specification can match.
///
/// @param require_drop_property if set to "true", tests if the
/// function is suppressed and if its representation is dropped from
/// the ABI corpus being built.  Otherwise, if set to "false", only
/// test if the function is suppressed.
///
/// @return true iff at least one function specification matches a
/// function with name @p fn_name or with linkage name @p
/// fn_linkage_name.
template <typename ReadContextType>
bool
function_is_suppressed(const ReadContextType&	ctxt,
		       const string&		fn_name,
		       const string&		fn_linkage_name,
		       bool			require_drop_property = false)
{
  for (suppr::suppressions_type::const_iterator i =
	 ctxt.get_suppressions().begin();
       i != ctxt.get_suppressions().end();
       ++i)
    if (suppr::function_suppression_sptr suppr = is_function_suppression(*i))
      {
	if (require_drop_property && !(*i)->get_drops_artifact_from_ir())
	  continue;
	if (!fn_name.empty()
	    && ctxt.suppression_matches_function_name(*suppr, fn_name))
	  return true;
	if (!fn_linkage_name.empty()
	    && ctxt.suppression_matches_function_sym_name(*suppr,
							  fn_linkage_name))
	  return true;
      }
  return false;
}
// </function_suppression stuff>

// <variable_suppression stuff>
/// The type of the private data of the @ref variable_suppression
/// type.
struct variable_suppression::priv
{
  friend class variable_suppression;

  change_kind				change_kind_;
  string				name_;
  string				name_regex_str_;
  mutable regex::regex_t_sptr		name_regex_;
  string				name_not_regex_str_;
  mutable regex::regex_t_sptr		name_not_regex_;
  string				symbol_name_;
  string				symbol_name_regex_str_;
  mutable regex::regex_t_sptr		symbol_name_regex_;
  string				symbol_name_not_regex_str_;
  mutable regex::regex_t_sptr		symbol_name_not_regex_;
  string				symbol_version_;
  string				symbol_version_regex_str_;
  mutable regex::regex_t_sptr		symbol_version_regex_;
  string				type_name_;
  string				type_name_regex_str_;
  mutable regex::regex_t_sptr		type_name_regex_;

  priv(const string& name,
       const string& name_regex_str,
       const string& symbol_name,
       const string& symbol_name_regex_str,
       const string& symbol_version,
       const string& symbol_version_regex_str,
       const string& type_name,
       const string& type_name_regex_str)
    : change_kind_(ALL_CHANGE_KIND),
      name_(name),
      name_regex_str_(name_regex_str),
      symbol_name_(symbol_name),
      symbol_name_regex_str_(symbol_name_regex_str),
      symbol_version_(symbol_version),
      symbol_version_regex_str_(symbol_version_regex_str),
      type_name_(type_name),
      type_name_regex_str_(type_name_regex_str)
  {}

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::name_regex_str_.
  const regex::regex_t_sptr
  get_name_regex() const
  {
    if (!name_regex_ && !name_regex_str_.empty())
      name_regex_ = regex::compile(name_regex_str_);
    return name_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::name_not_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::name_not_regex_str_..
  const regex::regex_t_sptr
  get_name_not_regex() const
  {
    if (!name_not_regex_ && !name_not_regex_str_.empty())
      name_not_regex_ = regex::compile(name_not_regex_str_);
    return name_not_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::symbol_name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::symbol_name_regex_str_.
  const regex::regex_t_sptr
  get_symbol_name_regex() const
  {
    if (!symbol_name_regex_ && !symbol_name_regex_str_.empty())
      symbol_name_regex_ = regex::compile(symbol_name_regex_str_);
    return symbol_name_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::symbol_name_not_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::symbol_name_not_regex_str_.
  const regex::regex_t_sptr
  get_symbol_name_not_regex() const
  {
    if (!symbol_name_not_regex_ && !symbol_name_not_regex_str_.empty())
      symbol_name_not_regex_ = regex::compile(symbol_name_not_regex_str_);
    return symbol_name_not_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::symbol_version_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::symbol_version_regex_str_.
  const regex::regex_t_sptr
  get_symbol_version_regex()  const
  {
    if (!symbol_version_regex_ && !symbol_version_regex_str_.empty())
      symbol_version_regex_ = regex::compile(symbol_version_regex_str_);
    return symbol_version_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::type_name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::type_name_regex_str_.
  const regex::regex_t_sptr
  get_type_name_regex() const
  {
    if (!type_name_regex_ && !type_name_regex_str_.empty())
      type_name_regex_ = regex::compile(type_name_regex_str_);
    return type_name_regex_;
  }
};// end class variable_supppression::priv

template <typename ReadContextType>
bool
variable_is_suppressed(const ReadContextType&	ctxt,
		       const string&		var_name,
		       const string&		var_linkage_name,
		       bool			require_drop_property = false)
{
  for (suppr::suppressions_type::const_iterator i =
	 ctxt.get_suppressions().begin();
       i != ctxt.get_suppressions().end();
       ++i)
    if (suppr::variable_suppression_sptr suppr = is_variable_suppression(*i))
      {
	if (require_drop_property && !(*i)->get_drops_artifact_from_ir())
	  continue;
	if (!var_name.empty()
	    && ctxt.suppression_matches_variable_name(*suppr, var_name))
	  return true;
	if (!var_linkage_name.empty()
	    && ctxt.suppression_matches_variable_sym_name(*suppr,
							  var_linkage_name))
	  return true;
      }
  return false;
}

// </variable_suppression stuff>

// <type_suppression stuff>
/// The private data for @ref type_suppression.
class type_suppression::priv
{
  string				type_name_regex_str_;
  mutable regex::regex_t_sptr		type_name_regex_;
  string				type_name_;
  string				type_name_not_regex_str_;
  mutable regex::regex_t_sptr		type_name_not_regex_;
  bool					consider_type_kind_;
  type_suppression::type_kind		type_kind_;
  bool					consider_reach_kind_;
  type_suppression::reach_kind		reach_kind_;
  type_suppression::insertion_ranges	insertion_ranges_;
  unordered_set<string>			source_locations_to_keep_;
  string				source_location_to_keep_regex_str_;
  mutable regex::regex_t_sptr		source_location_to_keep_regex_;
  mutable vector<string>		changed_enumerator_names_;

  priv();

public:
  priv(const string&			type_name_regexp,
       const string&			type_name,
       bool				consider_type_kind,
       type_suppression::type_kind	type_kind,
       bool				consider_reach_kind,
       type_suppression::reach_kind	reach_kind)
    : type_name_regex_str_(type_name_regexp),
      type_name_(type_name),
      consider_type_kind_(consider_type_kind),
      type_kind_(type_kind),
      consider_reach_kind_(consider_reach_kind),
      reach_kind_(reach_kind)
  {}

  /// Get the regular expression object associated to the 'type_name_regex'
  /// property of @ref type_suppression.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'type_name_regex' property of @ref type_suppression is
  /// empty then this method returns nil.
  const regex::regex_t_sptr
  get_type_name_regex() const
  {
    if (!type_name_regex_ && !type_name_regex_str_.empty())
      type_name_regex_ = regex::compile(type_name_regex_str_);
    return type_name_regex_;
  }

  /// Setter for the type_name_regex object.
  ///
  /// @param r the new type_name_regex object.
  void
  set_type_name_regex(regex::regex_t_sptr r)
  {type_name_regex_ = r;}

  /// Get the regular expression object associated to the
  /// 'type_name_not_regex' property of @ref type_suppression.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'type_name_not_regex' property of @ref type_suppression is
  /// empty then this method returns nil.
  const regex::regex_t_sptr
  get_type_name_not_regex() const
  {
    if (!type_name_not_regex_ && !type_name_not_regex_str_.empty())
      type_name_not_regex_ = regex::compile(type_name_not_regex_str_);
    return type_name_not_regex_;
  }

  /// Setter for the type_name_not_regex object.
  ///
  /// @param r the new type_name_not_regex object.
  void
  set_type_name_not_regex(regex::regex_t_sptr r)
  {type_name_not_regex_ = r;}

  /// Getter for the string that denotes the 'type_name_not_regex'
  /// property.
  ///
  /// @return the value of the string value of the
  /// 'type_name_not_regex' property.
  const string&
  get_type_name_not_regex_str() const
  {return type_name_not_regex_str_;}

  /// Setter for the string that denotes the 'type_name_not_regex'
  /// property.
  ///
  /// @return the value of the string value of the
  /// 'type_name_not_regex' property.
  void
  set_type_name_not_regex_str(const string regex_str)
  {type_name_not_regex_str_ = regex_str;}

  /// Getter for the source_location_to_keep_regex object.
  ///
  /// This function builds the regex if it's not yet built.
  const regex::regex_t_sptr
  get_source_location_to_keep_regex() const
  {
    if (!source_location_to_keep_regex_
	&& !source_location_to_keep_regex_str_.empty())
      source_location_to_keep_regex_ =
	  regex::compile(source_location_to_keep_regex_str_);
    return source_location_to_keep_regex_;
  }

  /// Setter for the source_location_to_keep_regex object.
  ///
  /// @param r the new regex object.
  void
  set_source_location_to_keep_regex(regex::regex_t_sptr r)
  {source_location_to_keep_regex_ = r;}

  friend class type_suppression;
}; // class type_suppression::priv

bool
suppression_matches_type_name(const suppr::type_suppression&	s,
			      const string&			type_name);

bool
suppression_matches_type_name(const suppr::type_suppression&	s,
			      const scope_decl*		scope,
			      const type_base_sptr&		type);

bool
suppression_matches_type_location(const type_suppression&	s,
				  const location&		loc);

bool
suppression_matches_type_location(const type_suppression&	s,
				  const type_base_sptr&	type);

bool
suppression_matches_type_name_or_location(const type_suppression& s,
					  const string& type_name,
					  const location& type_location);

/// Test if a type (designated by its name and location) is suppressed
/// by at least one suppression specification associated with a given
/// read context.
///
/// @param ctxt the read context to consider.
///
/// @param type_name the name of the type to consider.
///
/// @param type_location the location of the type to consider.
///
/// @param require_drop_property if set to "true", tests if the type
/// is suppressed and if its representation is dropped from the ABI
/// corpus being built.  Otherwise, if set to "false", only test if
/// the type is suppressed.
///
/// @return true iff at least one type specification matches a type
/// with name @p type_name and with location @p type_location.
template <typename ReadContextType>
bool
type_is_suppressed(const ReadContextType&	ctxt,
		   const string&		type_name,
		   const location&		type_location)
{
  bool type_is_private = false;
  return type_is_suppressed(ctxt, type_name, type_location, type_is_private);
}

/// Test if a type (designated by its name and location) is suppressed
/// by at least one suppression specification associated with a given
/// read context.
///
/// @param ctxt the read context to consider.
///
/// @param type_name the name of the type to consider.
///
/// @param type_location the location of the type to consider.
///
/// @param type_is_private out parameter. If the type is suppressed
/// because it's private then this out parameter is set to true.
///
/// @param require_drop_property if set to "true", tests if the type
/// is suppressed and if its representation is dropped from the ABI
/// corpus being built.  Otherwise, if set to "false", only test if
/// the type is suppressed.
///
/// @return true iff at least one type specification matches a type
/// with name @p type_name and with location @p type_location.
template <typename ReadContextType>
bool
type_is_suppressed(const ReadContextType&	ctxt,
		   const string&		type_name,
		   const location&		type_location,
		   bool&			type_is_private,
		   bool require_drop_property = false)
{
  for (suppr::suppressions_type::const_iterator i =
	 ctxt.get_suppressions().begin();
       i != ctxt.get_suppressions().end();
       ++i)
    if (suppr::type_suppression_sptr suppr = is_type_suppression(*i))
      {
	if (require_drop_property && !(*i)->get_drops_artifact_from_ir())
	  continue;
	if (ctxt.suppression_matches_type_name_or_location(*suppr, type_name,
							   type_location))
	  {
	    if (is_private_type_suppr_spec(*suppr))
	      type_is_private = true;

	    return true;
	  }
      }

  type_is_private = false;
  return false;
}

/// Test if a given ELF symbol is suppressed by a suppression
/// specification.
///
/// @param ctxt the read context to use.
///
/// @param sym_name the name of the symbol to consider.
///
/// @param sym_type the type of the symbol to consider.
///
/// @return true iff the elf symbol denoted by @p sym_name and @p
/// sym_type is suppressed.
template<typename ReadContextType>
bool
is_elf_symbol_suppressed(const ReadContextType& ctxt,
			 const string& sym_name,
			 elf_symbol::type sym_type)
{
  if (elf_symbol_is_function(sym_type))
    return suppr::function_is_suppressed(ctxt, /*fn_name=*/"",
					 /*symbol_name=*/sym_name);
  else if (elf_symbol_is_variable(sym_type))
    return suppr::variable_is_suppressed(ctxt, /*var_name=*/"",
					 /*symbol_name=*/sym_name);

  return false;
}

// </type_suppression stuff>

}// end namespace suppr
} // end namespace abigail

#endif // __ABG_SUPPRESSION_PRIV_H__

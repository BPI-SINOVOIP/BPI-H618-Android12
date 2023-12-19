// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the declarations for the ini file reader used in
/// the libabigail library.

#ifndef __ABG_INI_H__
#define __ABG_INI_H__

#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace abigail
{
/// Namespace for handling ini-style files
namespace ini
{
// Inject some standard types in this namespace.
using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::string;
using std::vector;
using std:: pair;

class property;
/// Convenience typefef for shared_ptr to @ref property.
typedef shared_ptr<property> property_sptr;

/// The base class of the different kinds of properties of an INI
/// file.
class property
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

public:

  property();

  property(const string& name);

  const string&
  get_name() const;

  void
  set_name(const string& name);

  virtual ~property();
}; // end class property

class property_value;

/// Convenience typedef for a shared_ptr to @ref property_value.
typedef shared_ptr<property_value> property_value_sptr;

/// Base class of propertie values.
class property_value
{
public:
  enum value_kind
  {
    ABSTRACT_PROPERTY_VALUE = 0,
    STRING_PROPERTY_VALUE = 1,
    LIST_PROPERTY_VALUE = 2,
    TUPLE_PROPERTY_VALUE = 3,
  };

private:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

public:

  property_value();
  property_value(value_kind);

  value_kind
  get_kind() const;

  virtual const string&
  as_string() const = 0;

  operator const string& () const;

  virtual ~property_value();
}; // end class property_value.

class string_property_value;

/// A convenience typedef for a shared_ptr to @ref string_property_value.
typedef shared_ptr<string_property_value> string_property_value_sptr;

/// A property value which is a string.
class string_property_value : public property_value
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

public:
  string_property_value();
  string_property_value(const string& value);

  void
  set_content(const string&);

  virtual const string&
  as_string() const;

  operator string() const;

  virtual ~string_property_value();
}; // end class string_property_value

string_property_value*
is_string_property_value(const property_value*);

string_property_value_sptr
is_string_property_value(const property_value_sptr);

class list_property_value;

/// A convenience typedef for a shared_ptr to @ref
/// list_property_value.
typedef shared_ptr<list_property_value> list_property_value_sptr;

/// Abstracts the value of a property representing a list of strings.
///
/// It's the right hand side of the construct which syntax looks like:
///
///   name = val1, val2, val3
///
/// where val1, val2 and val3 are strings.
///
/// So this class abstracts the set [val1, val2, val3].
class list_property_value : public property_value
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:
  list_property_value();
  list_property_value(const vector<string>& values);

  const vector<string>&
  get_content() const;

  void
  set_content(const vector<string>&);

  virtual const string&
  as_string() const;
}; // end class list_property_value

list_property_value*
is_list_property_value(const property_value*);

list_property_value_sptr
is_list_property_value(const property_value_sptr&);

class tuple_property_value;

/// Convenience typedef for a shared_ptr to a @ref
/// tuple_property_value.
typedef shared_ptr<tuple_property_value> tuple_property_value_sptr;

/// A property value that is a tuple.
///
/// Each element of the tuple is itself a property value that can
/// either be a string, or another tuple, for instance.
class tuple_property_value : public property_value
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

public:
  tuple_property_value(const vector<property_value_sptr>&);

  const vector<property_value_sptr>&
  get_value_items() const;

  vector<property_value_sptr>&
  get_value_items();

  virtual const string&
  as_string() const;

  operator string() const;

  virtual ~tuple_property_value();
}; // end class tuple_property_value

tuple_property_value*
is_tuple_property_value(const property_value*);

tuple_property_value_sptr
is_tuple_property_value(const property_value_sptr);

class simple_property;
/// Convenience typedef for a shared_ptr to an @ref simple_property.
typedef shared_ptr<simple_property> simple_property_sptr;

/// A simple property.  That is, one which value is a
/// @ref string_property_value.
class simple_property : public property
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:
  simple_property();

  simple_property(const string& name,
		  const string_property_value_sptr& value);

  simple_property(const string& name);

  const string_property_value_sptr&
  get_value() const;

  void
  set_value(const string_property_value_sptr& value);

  bool
  has_empty_value() const;

  virtual ~simple_property();
}; // end class simple_property

simple_property*
is_simple_property(const property* p);

simple_property_sptr
is_simple_property(const property_sptr p);

class list_property;

/// A convenience typedef for a shared_ptr to a @ref list_property.
typedef shared_ptr<list_property> list_property_sptr;

/// A class representing a list property.
///
/// It abstracts a construct which syntax looks like:
///
///    name = val1, val2, val3
///
/// The value of a list property is a @ref list_property_value, i.e, a
/// list of strings.
class list_property : public property
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:
  list_property();

  list_property(const string& name,
		const list_property_value_sptr& value);

  const list_property_value_sptr&
  get_value() const;

  void
  set_value(const list_property_value_sptr& value);

  virtual ~list_property();
}; // end class list_property

list_property*
is_list_property(const property* p);

list_property_sptr
is_list_property(const property_sptr p);

class tuple_property;
/// Convenience typedef for a shared_ptr of @ref tuple_property.
typedef shared_ptr<tuple_property> tuple_property_sptr;

/// Abstraction of a tuple property.  A tuple property is a property
/// which value is a @ref tuple_property_value.
class tuple_property : public property
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:
  tuple_property();

  tuple_property(const string& name,
		 const tuple_property_value_sptr v);

  void
  set_value(const tuple_property_value_sptr value);

  const tuple_property_value_sptr&
  get_value() const;

  virtual
  ~tuple_property();
}; // end class tuple_property

tuple_property*
is_tuple_property(const property* p);

tuple_property_sptr
is_tuple_property(const property_sptr p);

class config;

/// A convenience typedef for a shared pointer to @ref config
typedef shared_ptr<config> config_sptr;

/// The abstraction of the structured content of an .ini file.  This
/// roughly follows what is explained at
/// http://en.wikipedia.org/wiki/INI_file.
class config
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

public:
  class section;
  /// A convenience typedef for a shared pointer to a config::section.
  typedef shared_ptr<section> section_sptr;

  /// A convenience typedef for a vector of config::section_sptr.
  typedef vector<section_sptr> sections_type;

  /// A convenience typedef for a vector of @ref property_sptr
  typedef vector<property_sptr> properties_type;

private:
  priv_sptr priv_;

public:

  config();

  config(const string& path,
	 sections_type& sections);

  virtual ~config();

  const string&
  get_path() const;

  void
  set_path(const string& path);

  const sections_type&
  get_sections() const;

  void
  set_sections(const sections_type& sections);
}; // end class config

/// The abstraction of one section of the .ini config.
class config::section
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbid this
  section();

public:
  section(const string& name);

  section(const string& name, const properties_type& properties);

  const string&
  get_name() const;

  const properties_type&
  get_properties() const;

  void
  set_properties(const properties_type& properties);

  void
  add_property(const property_sptr prop);

  property_sptr
  find_property(const string& prop_name) const;

  virtual ~section();
}; //end class config::section

bool
read_sections(std::istream& input,
	      config::sections_type& sections);

bool
read_sections(const string& path,
	      config::sections_type& sections);

bool
read_config(std::istream& input,
	    config& conf);

config_sptr
read_config(std::istream& input);

bool
read_config(const string& path,
	    config& conf);

config_sptr
read_config(const string& path);

bool
write_sections(const config::sections_type& sections,
	       std::ostream& output);

bool
write_sections(const config::sections_type& sections,
	       const string& path);

bool
write_config(const config& conf,
	     std::ostream& output);

bool
write_config(const config& conf,
	     const string& path);

class function_call_expr;

/// Convenience typedef for a shared pointer to function_call_expr
typedef shared_ptr<function_call_expr> function_call_expr_sptr;

/// The abstraction of a function call expression.
class function_call_expr
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  function_call_expr();

public:
  function_call_expr(const string& name,
		     const vector<string>& args);

  const string&
  get_name() const;

  const vector<string>&
  get_arguments() const;

  vector<string>&
  get_arguments();
}; //end function_call_expr

bool
read_function_call_expr(std::istream& input,
			function_call_expr_sptr& expr);

bool
read_function_call_expr(const string& input,
			function_call_expr_sptr& expr);

function_call_expr_sptr
read_function_call_expr(const string& input);
}// end namespace ini
}// end namespace abigail
#endif // __ABG_INI_H__

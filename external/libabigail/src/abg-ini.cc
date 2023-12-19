// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the definitions for the ini file reader used in
/// the libabigail library.

#include <cassert>
#include <cstdlib>
#include <utility>
#include <memory>
#include <fstream>
#include <sstream>

#include "abg-fwd.h"
#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-ini.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{
namespace ini
{

using std::istream;
using std::pair;

static bool
char_is_white_space(int b);

static bool
char_is_comment_start(int b);

/// Test if a given character is a delimiter.
///
///
///@param b the value of the character to test for.
///
///@param include_white_space if true, consider white spaces as a
///delimiter.
///
///@param include_square_bracket if true, consider square brackets as
/// delimiters
///
/// @param include_equal if true, consider the equal character ('=')
/// as a delimiter.
///
/// @return true iff @p b is a delimiter.
static bool
char_is_delimiter(int b, bool include_white_space = true,
		  bool include_square_bracket = true,
		  bool include_equal = true)
{
  return ((include_square_bracket && (b == '['))
	  || (include_square_bracket && (b == ']'))
	  || b == '{'
	  || b == '}'
	  || (include_equal && (b == '='))
	  || b == ','
	  || (include_white_space && char_is_white_space(b))
	  || char_is_comment_start(b));
}

/// Return true iff a given character can be part of a property
/// value.
///
/// Note that white spaces, square brackets and the equal character can be
/// part of a property value.  The reason why we accept the equal
/// character is because it can appear in an URL.
///
/// @param b the character to test against.
///
/// @return true iff @p b is a character that can be part of a
/// property value.
static bool
char_is_property_value_char(int b)
{
  if (char_is_delimiter(b, /*include_white_space=*/false,
			/*include_square_bracket=*/false,
			/*include_equal=*/false)
      || b == '\n')
    return false;
  return true;
}

/// Test if a given character is meant to be part of a section name.
///
/// @param b the character to test against.
///
/// @return true iff @p b is a character that is meant to be part of
/// a section name.
static bool
char_is_section_name_char(int b)
{
  if (b == '[' || b == ']' || b == '\n' || char_is_comment_start(b))
    return false;
  return true;
}

/// Test if a given character is meant to be part of a property name.
///
/// @param b the character to test against.
///
/// @return true iff @p b is a character that is meant to be part of
/// a section name.
static bool
char_is_property_name_char(int b)
{
  if (char_is_delimiter(b))
    return false;
  return true;
}

/// Test if a given character is meant to be part of a function name.
///
/// @param b the character to test against.
///
/// @return true iff @p b is a character that is meant to be part of a
/// function name.
static bool
char_is_function_name_char(int b)
{
  if (char_is_delimiter(b) || b == '(' || b == ')')
    return false;
  return true;
}

/// Test if a given character is meant to be part of a function
/// argument.
///
/// @param b the character to test against.
///
/// @return true iff @p b is a character that is meant to be part of a
/// function argument.
static bool
char_is_function_argument_char(int b)
{
  if (char_is_delimiter(b) || b == '(' || b == ')')
    return false;
  return true;
}

/// Test if a given character is meant to be the start of a comment.
///
/// @param b the character to test against.
///
/// @return true iff @p b is the start of a comment.
static bool
char_is_comment_start(int b)
{return b == ';' || b == '#';}

/// Test if a character is meant to be a white space.
///
/// @param b the character to test against.
///
/// @return true iff @p b is a white space.
static bool
char_is_white_space(int b)
{return b == ' ' || b == '\t' || b == '\n';}

/// Remove the spaces at the begining and at the end of a given string.
///
/// @param str the string to remove leading and trailing white spaces from.
///
/// @return the string resulting from the removal of white space from @p str.
static string
trim_white_space(const string& str)
{
  if (str.empty())
    return str;

  unsigned s = 0, e = str.size() -1;

  for (; s <= e; ++s)
    if (!char_is_white_space(str[s]))
      break;

  for (; e > s; --e)
    if (!char_is_white_space(str[e]))
      break;

  return str.substr(s, e - s + 1);
}

// <property stuff>

/// Private data of @ref property type.
struct property::priv
{
  string name_;

  priv()
  {}

  priv(const string& name)
    : name_(name)
  {}
}; // end struct property::priv

/// Constructor of @ref property.
property::property()
  : priv_(new priv)
{}

/// Constructor of @ref property
///
/// @param name the name of the property.
property::property(const string& name)
  : priv_(new priv(name))
{}

/// Getter of the name of the property.
///
/// @return the name of the property.
const string&
property::get_name()const
{return priv_->name_;}

/// Setter of the name of the property.
///
/// @param name the new name of the property.
void
property::set_name(const string& name)
{priv_->name_ = name;}

/// Destructor of the property.
property::~property()
{}
// </property stuff>

// <property_value stuff>

/// Private data for the @ref property_value type.
struct property_value::priv
{
  enum property_value::value_kind kind_;

  priv(property_value::value_kind kind = ABSTRACT_PROPERTY_VALUE)
    : kind_(kind)
  {}
}; // property_value::priv

/// Default constructor for the @ref property_value type.
///
/// @param kind the of @ref property_value that is being constructed.
property_value::property_value()
  : priv_(new priv(ABSTRACT_PROPERTY_VALUE))
{}

/// Constructor for the @ref property_value type.
///
/// @param kind the of @ref property_value that is being constructed.
property_value::property_value(value_kind kind)
  : priv_(new priv(kind))
{}

/// Getter for the kind of the @ref property_value type.
///
/// @return the kind of @ref property_value we are looking at.
property_value::value_kind
property_value::get_kind() const
{return priv_->kind_;}

/// Converts the current property value to a string.
///
/// @return the string representation of the property value.
property_value::operator const string& () const
{return as_string();}

/// Destructor for the @ref proprerty_value type.
property_value::~property_value()
{}
// </property_value stuff>

// <string_property stuff>

/// The private data for the @ref string_property_value type.
struct string_property_value::priv
{
  string content_;

  priv()
  {}

  priv(const string& c)
    : content_(c)
  {}
}; // end struct string_property::priv

/// Constructor of the @ref string_property_value type.
string_property_value::string_property_value()
  : property_value(STRING_PROPERTY_VALUE),
    priv_(new priv())
{}

/// Constructor of the @ref string_property_value.
///
/// @param content the string content of the property value.
string_property_value::string_property_value(const string& content)
  : property_value(STRING_PROPERTY_VALUE),
    priv_(new priv(content))
{}

/// Setter of the content of the string property value.
///
/// @param c the new content.
void
string_property_value::set_content(const string& c)
{priv_->content_ = c;}

/// Convert the string property value into a string.
///
/// @return the string contained in the string property value.
const string&
string_property_value::as_string() const
{return priv_->content_;}

/// Conversion operator to a string, for the @ref
/// string_property_value type.
///
/// @return the string representing this string_property_value.
string_property_value::operator string() const
{return as_string();}

/// Test if a given property value is a string property value.
///
/// @return a pointer to the @ref string_property_value sub-object of
/// the @ref property_value instance, if it's an instance of @ref
/// string_property_value too.
string_property_value*
is_string_property_value(const property_value* v)
{return dynamic_cast<string_property_value*>(const_cast<property_value*>(v));}

/// Test if a given property value is a string property value.
///
/// @return a pointer to the @ref string_property_value sub-object of
/// the @ref property_value instance, if it's an instance of @ref
/// string_property_value too.
string_property_value_sptr
is_string_property_value(const property_value_sptr v)
{return dynamic_pointer_cast<string_property_value>(v);}

/// Destructor for the @ref string_property_value
string_property_value::~string_property_value()
{}

// </string_property_value stuff>

// <list_property_value stuff>
struct list_property_value::priv
{
  vector<string> values_;
  string representation_;

  priv()
  {}

  priv(const vector<string>& vals)
    : values_(vals)
  {}
}; // end struct list_property_value::priv

/// Default constructor of the @ref list_property_value type.
list_property_value::list_property_value()
  : property_value(property_value::LIST_PROPERTY_VALUE),
   priv_(new priv)
{}

/// Copy constructor of the @ref list_property_value type.
///
/// @param values the instance of @ref list_property_value to copy from.
list_property_value::list_property_value(const vector<string>& values)
  : property_value(property_value::LIST_PROPERTY_VALUE),
    priv_(new priv(values))
{
}

/// Getter of the content of the @ref list_property_value.
///
/// The content of the @ref list_property_value is a vector of
/// strings.
///
/// @return the vector of strings contained in the @ref
/// list_property_value.
const vector<string>&
list_property_value::get_content() const
{return priv_->values_;}

/// Setter of the content of the @ref list_property_value.
///
/// @param values the new content, which is a vector of strings.
void
list_property_value::set_content(const vector<string>& values)
{
  priv_->values_ = values;
  priv_->representation_.clear();
}

/// Return a string representation of the @list_property_value.
///
/// @return the string representation.
const string&
list_property_value::as_string() const
{
  if (priv_->representation_.empty())
    {
      for (vector<string>::const_iterator i = priv_->values_.begin();
	   i != priv_->values_.end();
	   ++i)
	{
	  if (i != priv_->values_.begin())
	    priv_->representation_ += ",";
	  priv_->representation_ += *i;
	}
    }
  return priv_->representation_;
}

/// Test if an instance of @property_value is a @ref list_property_value.
///
/// @param v the property_value to consider.
///
/// @return the @ref property_value converted into a @ref
/// list_property_value if the @p v is a @ref list_property_value, nil
/// otherwise.
list_property_value*
is_list_property_value(const property_value* v)
{return dynamic_cast<list_property_value*>(const_cast<property_value*>(v));}

/// Test if an instance of @property_value is a @ref list_property_value.
///
/// @param v the property_value to consider.
///
/// @return the @ref property_value converted into a @ref
/// list_property_value if the @p v is a @ref list_property_value, nil
/// otherwise.
list_property_value_sptr
is_list_property_value(const property_value_sptr&v)
{return dynamic_pointer_cast<list_property_value>(v);}

// </list_property_value stuff>

// <tuple_property_value>

/// The private data of the @ref tuple_property_value type.
struct tuple_property_value::priv
{
  vector<property_value_sptr>	value_items_;
  string			string_rep_;

  priv()
  {}

  priv(const vector<property_value_sptr>& value_items)
    : value_items_(value_items)
  {}
}; // end struct tuple_property_value::priv

/// Constructor for the @ref tuple_property_value type.
///
/// @param v the tuple content of the value.
tuple_property_value::tuple_property_value(const vector<property_value_sptr>& v)
  : property_value(TUPLE_PROPERTY_VALUE),
    priv_(new priv(v))
{}

/// Getter for the content of the @ref tuple_property_value instance.
///
/// @return the content of the @ref tuple_property_value instance.
const vector<property_value_sptr>&
tuple_property_value::get_value_items() const
{return priv_->value_items_;}

/// Getter for the content of the @ref tuple_property_value instance.
///
/// @return the content of the @ref tuple_property_value instance.
vector<property_value_sptr>&
tuple_property_value::get_value_items()
{return priv_->value_items_;}

/// Destructor of the @ref tuple_property_value type.
tuple_property_value::~tuple_property_value()
{}

/// Convert to the instance of @ref tuple_property_value to a string.
///
/// @return the string representation of the @ref tuple_property_value.
const string&
tuple_property_value::as_string() const
{
  if (priv_->string_rep_.empty())
    {
      priv_->string_rep_ += '{';
      for (vector<property_value_sptr>::const_iterator i =
	     get_value_items().begin();
	   i != get_value_items().end();
	   ++i)
	{
	  if (i != get_value_items().begin())
	    priv_->string_rep_ += ",";
	  priv_->string_rep_ += (*i)->as_string();
	}
      priv_->string_rep_ += '}';
    }
  return priv_->string_rep_;
}

/// Test if a given instance of @ref property_value is an instance of
/// @ref tuple_property_value too.
///
/// @return the @ref tuple_property_value sub-object of the @ref
/// property_value instance, if it's an instance of @ref
/// tuple_property_value too.
tuple_property_value*
is_tuple_property_value(const property_value* v)
{return dynamic_cast<tuple_property_value*>(const_cast<property_value*>(v));}

/// Test if a given instance of @ref property_value is an instance of
/// @ref tuple_property_value too.
///
/// @return the @ref tuple_property_value sub-object of the @ref
/// property_value instance, if it's an instance of @ref
/// tuple_property_value too.
tuple_property_value_sptr
is_tuple_property_value(const property_value_sptr v)
{return dynamic_pointer_cast<tuple_property_value>(v);}

// </tuple_property_value>

// <simple_property stuff>

/// Private data of the @ref simple_property type.
struct simple_property::priv
{
  string_property_value_sptr value_;

  priv()
  {}

  priv(const string_property_value_sptr value)
    : value_(value)
  {}
}; // end struct simple_property::priv

/// Default constructor of the @ref simple_property type.
simple_property::simple_property()
  : property(),
    priv_(new priv)
{}

/// Constructor for the @ref simple_property type.
///
/// @param name the name of the property.
///
/// @param value the value of the property.
simple_property::simple_property(const string& name,
				 const string_property_value_sptr& value)
  : property(name),
    priv_(new priv(value))
{}

/// Constructor for the @ref simple_property type.
///
/// This one constructs a property with an empty value.
///
/// @param name the name of the property.
simple_property::simple_property(const string& name)
  : property(name),
    priv_(new priv)
{}

/// Getter for the string value of the property.
///
/// @return the string value of the property.
const string_property_value_sptr&
simple_property::get_value() const
{return priv_->value_;}

/// Setter for the string value of the property.
///
/// @param value the new string value of the property.
void
simple_property::set_value(const string_property_value_sptr& value)
{priv_->value_ = value;}

/// Test if the property has an empty value.
///
/// An empty value is either no value at all or an empty string value.
///
/// @return true iff the property has an empty value.
bool
simple_property::has_empty_value() const
{
  if (!priv_->value_)
    return true;
  return  priv_->value_->as_string().empty();
}

/// Destructor of the @ref simple_property type.
simple_property::~simple_property()
{}

/// Tests if a @ref property is a simple property.
///
/// @return a pointer to the @ref simple_property sub-object of the
/// @ref property instance, iff it's an @ref simple_property
/// instance.
simple_property*
is_simple_property(const property* p)
{return dynamic_cast<simple_property*>(const_cast<property*>(p));}

/// Tests if a @ref property is a simple property.
///
/// @return a smart pointer to the @ref simple_property sub-object of
/// the @ref property instance, iff it's an @ref simple_property
/// instance.
simple_property_sptr
is_simple_property(const property_sptr p)
{return dynamic_pointer_cast<simple_property>(p);}

// </simple_property stuff>

// <list_property stuff>
struct list_property::priv
{
  list_property_value_sptr value_;

  priv()
  {}

  priv(const list_property_value_sptr value)
    : value_(value)
  {}
}; //end struct list_property

/// Default constructor for @ref list_property.
list_property::list_property()
  : priv_(new priv)
{}

/// Constructor for @ref list_property.
///
/// @param name the name of the property.
///
/// @param value the value of the property.
list_property::list_property(const string& name,
			     const list_property_value_sptr& value)
  : property(name),
    priv_(new priv(value))
{}

/// Getter for the value of the @ref list_property_value
const list_property_value_sptr&
list_property::get_value() const
{return priv_->value_;}

/// Setter for the value of the @ref list_property.
///
/// @param value the new value.
void
list_property::set_value(const list_property_value_sptr& value)
{priv_->value_ = value;}

/// Destructor of the @ref list_property type.
list_property::~list_property()
{}

/// Test if an instance of a @ref property is actually an instance of
/// @ref list_property.
///
/// @param p the @ref property to test.
///
/// @return the @p p converted into a @ref list_property if it's of
/// type @ref list_property, or nil otherwise.
list_property*
is_list_property(const property* p)
{return dynamic_cast<list_property*>(const_cast<property*>(p));}

/// Test if an instance of a @ref property is actually an instance of
/// @ref list_property.
///
/// @param p the @ref property to test.
///
/// @return the @p p converted into a @ref list_property if it's of
/// type @ref list_property, or nil otherwise.
list_property_sptr
is_list_property(const property_sptr p)
{return dynamic_pointer_cast<list_property>(p);}
// </list_property stuff>

// <tuple_property stuff>
struct tuple_property::priv
{
  tuple_property_value_sptr value_;

  priv()
  {}

  priv(const tuple_property_value_sptr value)
    : value_(value)
  {}
}; // end struct tuple_property::priv

/// Default constructor of the @ref tuple_property type.
tuple_property::tuple_property()
  : property(),
    priv_(new priv)
{}

/// Constructor of the @ref tuple_property type.
///
/// @param name the name of the property.
///
/// @param values the tuple value of the property.
tuple_property::tuple_property(const string& name,
			       const tuple_property_value_sptr value)
  : property(name),
    priv_(new priv(value))
{}

/// Setter for the tuple value of the property.
///
/// @param values the new tuple value of the property.
void
tuple_property::set_value(const tuple_property_value_sptr value)
{priv_->value_ = value;}

/// Getter for the tuple value of the property.
///
/// @return the tuple value of the property.
const tuple_property_value_sptr&
tuple_property::get_value() const
{return priv_->value_;}

/// Destructor for the @ref tuple_property type.
tuple_property::~tuple_property()
{}

/// Test if an instance of @ref property is an instance of @ref
/// tuple_property.
///
/// @param p the instance of @ref property to test for.
///
/// @return return a pointer to the sub-object of @ref tuple_property
/// iff @p p is an instance of @ref tuple_property.
tuple_property*
is_tuple_property(const property* p)
{return dynamic_cast<tuple_property*>(const_cast<property*>(p));}

/// Test if an instance of @ref property is an instance of @ref
/// tuple_property.
///
/// @param p the instance of @ref property to test for.
///
/// @return return a smart pointer to the sub-object of @ref
/// tuple_property iff @p p is an instance of @ref tuple_property.
tuple_property_sptr
is_tuple_property(const property_sptr p)
{return dynamic_pointer_cast<tuple_property>(p);}

// </tuple_property stuff>

class config::section::priv
{
  string name_;
  properties_type properties_;

  // Forbid this;
  priv();

public:
  priv(const string& name)
    : name_(name)
  {}

  friend class config::section;
};//end struct config::section::priv

// <config::section stuff>

/// Constructor for config::section.
///
/// @param name the name of the ini section.
config::section::section(const string& name)
  : priv_(new priv(name))
{}

/// Constructor for the config::section.
///
/// @param name the name of the ini section.
///
/// @param properties the properties of the section.
config::section::section(const string& name,
			 const properties_type& properties)
  : priv_(new priv(name))
{set_properties(properties);}

/// Get the name of the section.
///
/// @return the name of the section.
const string&
config::section::get_name() const
{return priv_->name_;}

/// Get the properties of the section.
///
/// @return a vector of the properties of the section.
const config::properties_type&
config::section::get_properties() const
{return priv_->properties_;}

/// Set the properties of the section.
///
/// @param properties the new properties to set.
void
config::section::set_properties(const properties_type& properties)
{priv_->properties_ = properties;}

/// Add one property to this section.
///
/// @param prop the property to add to the section.
void
config::section::add_property(const property_sptr prop)
{priv_->properties_.push_back(prop);}

/// Find a property that has a given name.
///
/// Note that this only returns the first property with that name.
///
/// @param prop_name the name of the property to find.
///
/// @return the found property, or nil if no property with the name @p
/// prop_name was found.
property_sptr
config::section::find_property(const string& prop_name) const
{
  for (properties_type::const_iterator i = get_properties().begin();
       i != get_properties().end();
       ++i)
    if ((*i)->get_name() == prop_name)
      return *i;
  return property_sptr();
}

/// Destructor of config::section.
config::section::~section()
{}
// /<config::section stuff>

// <read_context stuff>

/// The context of the ini file parsing.
///
/// This is a private type that is used only in the internals of the
/// ini file parsing.
class read_context
{
  /// The input stream we are parsing from.
  istream& in_;
  /// The current line being parsed.
  unsigned cur_line_;
  /// The current column on the current line.
  unsigned cur_column_;
  vector<char> buf_;

  // Forbid this;
  read_context();

public:

  /// The constructor of @ref read_context.
  ///
  /// @param in the input stream to parse from.
  read_context(istream& in)
    : in_(in),
      cur_line_(0),
      cur_column_(0)
  {}

  /// @return the character that is going to be read by the next
  /// invocation of read_next_char().
  ///
  /// Note that this function doesn't alter the input stream.
  ///
  /// Also note that this function handles escaping using the '\'
  /// (backslash) character.
  ///
  /// @param escaped This is an output parameter.  It's set to true by
  /// this function if it escaped the peeked character.  Otherwise,
  /// it's set to false.
  ///
  /// @return peeked character.
  char
  peek(bool& escaped)
  {
    if (!buf_.empty())
      return buf_.back();

    escaped = false;
    char c = in_.peek();
    if (handle_escape(c, /*peek=*/true))
      {
	put_back(c);
	escaped = true;
      }
    return c;
  }

  /// @return the character that is going to be read by the next
  /// invocation of read_next_char().
  ///
  /// Note that this function doesn't alter the input stream.
  ///
  /// Also note that this function handles escaping using the '\'
  /// (backslash) character.
  ///
  /// @return peeked character.
  char
  peek()
  {
    bool escaped = false;
    return peek(escaped);
  }

  /// Get the next character of the input stream.
  ///
  /// This function knows how to handles escaped characters from the
  /// input stream.
  ///
  /// @param do_handle_escape if yes, this function handles escaped
  /// characters from the input stream.
  ///
  /// @return the next character of the input stream.
  char
  get(bool do_handle_escape = true)
  {
    char result = 0;
    if (!buf_.empty())
      {
	result = buf_.back();
	buf_.pop_back();
      }
    else
      {
	result = in_.get();
	if (do_handle_escape)
	  handle_escape(result);
      }
    return result;
  }

  /// Put a character that was read from the input stream, back into
  /// that input stream, so that a subsequent call to
  /// read_context::get() returns that same character.
  ///
  /// @param c the character to put back into the stream.
  void
  put_back(char c)
  {buf_.push_back(c);}

  /// Test if the status of the input stream is good.
  ///
  /// @return true iff the status of the input stream is good.
  bool
  good() const
  {
    if (!buf_.empty())
      return true;
    return in_.good();
  }

  /// Tests if the input stream has reached end of file.
  ///
  /// @return true iff the input stream has reached end of file.
  bool
  eof() const
  {
    if (!buf_.empty())
      return false;
    return in_.eof();
  }

  /// Handles the escaping of a character.
  ///
  /// This function must be called whenever the low level character
  /// reading function encountered a backslash character ('\').  In
  /// that case, this function reads the subsequent characters from
  /// the input stream, sees if it needs to escape them and then
  /// handles the escaping if need be.  Otherwise, it does nothing.
  ///
  /// This is a subroutine of the read_context::get() and
  /// read_context::peek() functions.
  ///
  /// @param peek if true, it means this function was called after the
  /// caller issued a read_context::peek() call, rather than a
  /// read_context::get() call.
  ///
  /// @return true if an escaping took place.
  bool
  handle_escape(char& c, bool peek = false)
  {
    bool escaped = false;
    char b = c;

    if (b == '\\')
      {
	escaped = true;
	b = get(/*escape=*/false);
	if (!good())
	  return escaped;
	if (peek)
	  {
	    ABG_ASSERT(b == c);
	    b = get(/*escape=*/false);
	    if (!good())
	      return escaped;
	  }

	switch (b)
	  {
	  case '0':
	  case 'a':
	  case 'b':
	  case 'r':
	    // let's replace this by a space
	    c = ' ';
	    break;
	  case 't':
	    c = '\t';
	    break;
	  case '\n':
	    // continuation line.  So we should drop both the backslash
	    // character and this end-of-line character on the floor
	    // just like if they never existed.
	    ++cur_column_;
	    b = get(/*escape=*/false);
	    if (!good())
	      return escaped;
	    c = b;
	    break;
	  case '\\':
	  case ';':
	  case '#':
	  case '[':
	  case ']':
	  default:
	    c = b;
	    break;
	  }
      }
    else
      c = b;

    return escaped;
  }

  /// Read the next character from the input stream.
  ///
  /// This method updates the current line/column number after looking
  /// at the actual char that got read.  Note that escaped characters
  /// are handled transparently at this point.
  ///
  /// @param c output parameter.  This is set by this function to the
  /// character that was read.  It's set iff the function returned
  /// true.
  ///
  /// @return true if the reading went well and if the input stream is
  /// in a non-erratic state.
  bool
  read_next_char(char& c)
  {
    char b = get();
    if (!good())
      return false;

    c = b;

    if (cur_line_ == 0)
      cur_line_ = 1;

    if (b == '\n')
      {
	++cur_line_;
	cur_column_ = 0;
      }
    else
      ++cur_column_;

    return true;
  }

  /// Skip (that is, read characters and drop them on the floor) all
  /// the characters up to the next line.
  ///
  /// Note that the new line character (\n' on unices) is skipped as
  /// well.
  ///
  /// @return true iff the skipping proceeded successfully and that
  /// the input stream is left in a non-erratic state.
  bool
  skip_line()
  {
    char c = 0;
    for (bool is_ok = read_next_char(c);
	 is_ok;
	 is_ok = read_next_char(c))
      if (c == '\n')
	break;

    return (c == '\n' || eof());
  }

  /// If the current character is a white space, skip it and all the
  /// contiguous ones that follow.
  ///
  /// @return true iff the input stream is left in a non-erratic state.
  bool
  skip_white_spaces()
  {
    for (char c = peek(); good(); c = peek())
      if (char_is_white_space(c))
	ABG_ASSERT(read_next_char(c));
      else
	break;
    return good() || eof();
  }

  /// If the current character is the beginning of a comment, skip
  /// (read and drop on the floor) the entire remaining line,
  /// including the current character.
  ///
  /// @return true if the input stream is left in a non-erratic state.
  bool
  skip_comments()
  {
    for (char c = peek(); good(); c = peek())
      if (char_is_comment_start(c))
	skip_line();
      else
	break;
    return good() || eof();
  }

  /// If the current character is either the beginning of a comment or
  /// a white space, skip the entire commented line or the subsequent
  /// contiguous white spaces.
  ///
  /// @return true iff the stream is left in a non-erratic state.
  bool
  skip_white_spaces_or_comments()
  {
    int b = 0;
    while (good())
      {
	b = peek();
	if (char_is_white_space(b))
	  skip_white_spaces();
	else if (char_is_comment_start(b))
	  skip_comments();
	else
	  break;
      }
    return good() || eof();
  }

  /// Read a property name.
  ///
  /// @param name out parameter.  Is set to the parsed property name,
  /// if any.  Note that this is set only if the function returned
  /// true.
  ///
  /// @return true iff the input stream is left in a non-erratic
  /// state.
  bool
  read_property_name(string& name)
  {
    char c = peek();
    if (!good() || !char_is_property_name_char(c))
      return false;

    ABG_ASSERT(read_next_char(c));
    name += c;

    for (c = peek(); good(); c = peek())
      {
	if (!char_is_property_name_char(c))
	  break;
	ABG_ASSERT(read_next_char(c));
	name += c;
      }

    return true;
  }

  /// Read a function name.
  ///
  /// @param name the name of the function.  This is an output
  /// parameter when this puts the function name that was read, iff
  /// this function returns true.
  ///
  /// @return true iff the function name was successfully read into @p
  /// name.
  bool
  read_function_name(string& name)
  {
    char c = peek();
    if (!good() || !char_is_function_name_char(c))
      return false;

    ABG_ASSERT(read_next_char(c));
    name += c;

    for (c = peek(); good(); c = peek())
      {
	if (!char_is_function_name_char(c))
	  break;
	ABG_ASSERT(read_next_char(c));
	name += c;
      }

    return true;
  }

  /// Read a function argument.
  ///
  /// @param argument the argument of the function that was just
  /// read.  This is an ouput parameter that is set iff the function
  /// returns true.
  ///
  /// @return true iff parameter @p argument was successful set.
  bool
  read_function_argument(string& argument)
  {
    char c = peek();
    if (!good() || !char_is_function_argument_char(c))
      return false;

    ABG_ASSERT(read_next_char(c));
    argument += c;

    for (c = peek(); good(); c = peek())
      {
	if (!char_is_function_argument_char(c))
	  break;
	ABG_ASSERT(read_next_char(c));
	argument += c;
      }

    return true;
  }

  /// Read a function call expression.
  ///
  /// The expression this function can read has the form:
  ///		'foo(bar,baz, blah)'
  ///
  /// @param expr this is an output parameter that is set with the
  /// resulting function call expression that was read, iff this
  /// function returns true.
  ///
  /// @param return true iff @p expr was successful set with the
  /// function call expression that was read.
  bool
  read_function_call_expr(function_call_expr_sptr& expr)
  {
    if (!good())
      return false;

    skip_white_spaces_or_comments();
    if (!good())
      return false;

    string name;
    if (!read_function_name(name) || name.empty())
      return false;

    skip_white_spaces_or_comments();

    int b = peek();
    if (!good() || b != '(')
      return false;

    char c = 0;
    if (!read_next_char(c))
      return false;
    ABG_ASSERT(c == '(');

    skip_white_spaces_or_comments();
    if (!good())
      return false;

    // Read function call arguments.
    vector<string> arguments;
    for (;;)
      {
	if (peek() == ')')
	  break;

	string arg;
	if (!read_function_argument(arg))
	  return true;

	skip_white_spaces_or_comments();
	if (!good())
	  return false;

	if (peek() == ',')
	  {
	    c = 0;
	    ABG_ASSERT(read_next_char(c) && c == ',');
	    skip_white_spaces_or_comments();
	    if (!good())
	      return false;
	  }

	arguments.push_back(arg);
      }

    c = 0;
    ABG_ASSERT(read_next_char(c) && c == ')');

    expr.reset(new function_call_expr(name, arguments));
    return true;
  }

  /// Read a property value.
  ///
  /// @return the property value read.
  property_value_sptr
  read_property_value()
  {
    property_value_sptr nil, result;

    int b = peek();
    if (!good())
      return nil;

    if (b == '{')
      {
	if (tuple_property_value_sptr t = read_tuple_property_value())
	  return t;
	return nil;
      }

    list_property_value_sptr list = read_list_property_value();
    if (list->get_content().size() == 1)
      result.reset(new string_property_value(list->get_content()[0]));
    else
      result = list;

    return result;
  }

  /// Reads a string from the input stream.
  ///
  /// A string is just a contiguous set of characters that test
  /// positive when passed to
  /// read_context::char_is_property_name_char().
  ///
  /// Note that all escaped characters are suitable to be in a string.
  ///
  /// @return the string read.
  string
  read_string()
  {
    bool escaped = false;
    int b = peek(escaped);
    if (!good())
      return "";

    if (!escaped && char_is_delimiter(b, /*include_white_space=*/false))
      // Empty property value.  This is accepted.
      return "";

    string v;
    for (b = peek(escaped); good(); b = peek(escaped))
      {
	// If the current character is not suitable to be a in string,
	// then we reached the end of the string.  Note that espaced
	// characters are always suitable to be a string.
	if (!escaped && !char_is_property_value_char(b))
	  break;
	char c = 0;
	ABG_ASSERT(read_next_char(c));
	v += c;
      }
    return trim_white_space(v);
  }

  /// Read a string property value.
  ///
  /// @return the property value that has been parsed.
  string_property_value_sptr
  read_string_property_value()
  {
    string_property_value_sptr nil, result;
    if (!good())
      return nil;

    string value = read_string();
    result.reset(new string_property_value(value));
    return result;
  }

  /// Read a @ref list_property_value.
  ///
  /// @return the instance of @ref list_property_value read, or nil if
  /// none was read.
  list_property_value_sptr
  read_list_property_value()
  {
    list_property_value_sptr nil, result;
    string str;
    vector<string> content;

    for (;;)
      {
	str = read_string();
	if (str.empty())
	  break;
	content.push_back(str);

	skip_white_spaces();

	int b = peek();
	if (!good() || b != ',')
	  break;
	skip_white_spaces();

	char c = 0;
	read_next_char(c);
	ABG_ASSERT(c == ',');
      }

    if (!content.empty())
      result.reset(new list_property_value(content));

    return result;
  }

  /// A property value that is a tuple.
  ///
  /// @param tuple the read property tuple value.
  ///
  /// @return true iff the tuple property value could be read
  /// correctly.
  tuple_property_value_sptr
  read_tuple_property_value()
  {
    tuple_property_value_sptr nil, result;
    int b = peek();
    if (!good())
      return nil;

    if (b != '{')
      return nil;

    char c = 0;
    ABG_ASSERT(read_next_char(c));

    property_value_sptr value;
    vector<property_value_sptr> values;
    while (good() && peek() != '}')
      {
	skip_white_spaces();
	if ((value = read_property_value()))
	  values.push_back(value);
	skip_white_spaces();
	if (good() && peek() == ',')
	  {
	    c = 0;
	    read_next_char(c);
	  }
      }

    b = peek();
    if (b != '}')
      return nil;

    c = 0;
    read_next_char(c);

    result.reset(new tuple_property_value(values));
    return result;
  }

  /// Read the name of a section.
  ///
  /// @param name out parameter.  Is set to the name of the section
  /// that was parsed.  Note that this is set only if the function
  /// returned true.
  ///
  /// @return true if the input stream was left in a non-erratic
  /// state.
  bool
  read_section_name(string& name)
  {
    int b = peek();
    if (!good() || !char_is_section_name_char(b))
      return false;

    char c = 0;
    ABG_ASSERT(read_next_char(c) || char_is_section_name_char(b));
    name += c;

    for (b = peek(); good(); b = peek())
      {
	if (!char_is_section_name_char(b))
	  break;
	ABG_ASSERT(read_next_char(c));
	name += c;
      }

    return true;
  }

  /// Read a property (<name> = <value>).
  ///
  /// @return the resulting pointer to property iff one could be
  /// parsed.
  property_sptr
  read_property()
  {
    property_sptr nil;

    string name;
    if (!read_property_name(name))
      return nil;

    skip_white_spaces();

    property_sptr result;

    char c = peek();
    if (c == '=')
      {
	ABG_ASSERT(read_next_char(c));
	ABG_ASSERT(c == '=');
	skip_white_spaces();
      }
    else
      {
	property_sptr empty_value_property(new simple_property(name));
	return empty_value_property;
      }

    if (!good())
      return nil;

    property_value_sptr value = read_property_value();
    if (!value)
      return nil;

    if (tuple_property_value_sptr tv = is_tuple_property_value(value))
      result.reset(new tuple_property(name, tv));
    else if (list_property_value_sptr lv = is_list_property_value(value))
      result.reset(new list_property(name, lv));
    else if (string_property_value_sptr sv = is_string_property_value(value))
      result.reset(new simple_property(name, sv));
    else
      // This new kind of property is not yet supported!
      std::abort();

    return result;
  }

  /// Read an ini section.
  ///
  /// @return a pointer to a section iff it could be successfully
  /// parsed.
  config::section_sptr
  read_section()
  {
    config::section_sptr nil;

    int b = peek();
    if (!good())
      return nil;

    char c = 0;
    if (b == '[')
      ABG_ASSERT(read_next_char(c) && c == '[');

    string name;
    if (!read_section_name(name))
      return nil;

    if (!skip_white_spaces())
      return nil;

    if (! read_next_char(c) || c != ']')
      return nil;

    if (!skip_white_spaces_or_comments())
      return nil;

    config::properties_type properties;
    while (property_sptr prop = read_property())
      {
	properties.push_back(prop);
	skip_white_spaces_or_comments();
      }

    if (!properties.empty())
      {
	config::section_sptr section(new config::section(name, properties));
	return section;
      }

    return nil;
  }
};//end struct read_context

// </read_context stuff>

// <config stuff>

class config::priv
{
  string path_;
  sections_type sections_;

public:
  friend class config;

  priv()
  {}

  priv(const string& path,
       sections_type& sections)
    : path_(path),
      sections_(sections)
  {}

};

/// @param path the path to the config file.
///
/// @param sections the sections of the config file.
config::config(const string& path,
	       sections_type& sections)
  : priv_(new priv(path, sections))
{}

config::config()
  : priv_(new priv)
{}

config::~config()
{}

/// @return the path to the config file.
const string&
config::get_path() const
{return priv_->path_;}

/// Set the path to the config file.
///
/// @param the new path to the config file.
void
config::set_path(const string& path)
{priv_->path_ = path;}

/// @return the sections of the config file.
const config::sections_type&
config::get_sections() const
{return priv_->sections_;}

/// Set new sections to the ini config
///
/// @param sections the new sections to set.
void
config::set_sections(const sections_type& sections)
{priv_->sections_ = sections;}

// </config stuff>

// <config reader stuff>

/// Parse the sections of an *.ini file.
///
/// @param input the input stream to parse the ini file from.
///
/// @param section out parameter.  This is set to the vector of
/// sections that have been parsed from the input stream.
///
/// @return true upon successful completion and if if the stream is
/// left in a non-erratic state.
bool
read_sections(std::istream& input,
	      config::sections_type& sections)
{
  read_context ctxt(input);

  while (input.good())
    {
      ctxt.skip_white_spaces_or_comments();
      if (config::section_sptr section = ctxt.read_section())
	sections.push_back(section);
      else
	break;
    }

  return input.good() || input.eof();
}

/// Parse the sections of an *.ini file.
///
/// @param path the path of the ini file to parse.
///
/// @param section out parameter.  This is set to the vector of
/// sections that have been parsed from the input stream.
///
/// @return true upon successful completion and if if the stream is
/// left in a non-erratic state.
bool
read_sections(const string& path,
	      config::sections_type& sections)
{
  std::ifstream in(path.c_str(), std::ifstream::binary);
  if (!in.good())
    return false;

  bool is_ok = read_sections(in, sections);
  in.close();

  return is_ok;
}

/// Parse an ini config file from an input stream.
///
/// @param input the input stream to parse the ini config file from.
///
/// @return true upon successful parsing.
bool
read_config(istream& input,
	    config& conf)
{
  config::sections_type sections;
  if (!read_sections(input, sections))
    return false;
  conf.set_sections(sections);
  return true;
}

/// Parse an ini config file from a file on disk.
///
/// @param path the path to the ini file to parse.
///
/// @param conf the resulting config file to populate as a result of
/// the parsing.  This is populated iff the function returns true.
///
/// @return true upon succcessful completion.
bool
read_config(const string& path,
	    config& conf)
{
  config::sections_type sections;
  if (!read_sections(path, sections))
    return false;
  conf.set_path(path);
  conf.set_sections(sections);
  return true;
}

/// Parse an ini config file from an input stream.
///
/// @return a shared pointer to the resulting config, or nil if it
/// couldn't be parsed.
config_sptr
read_config(std::istream& input)
{
  config_sptr c(new config);
  if (!read_config(input, *c))
    return config_sptr();
  return c;
}


/// Parse an ini config file from an on-disk file.
///
/// @return a shared pointer to the resulting config, or nil if it
/// couldn't be parsed.
config_sptr
read_config(const string& path)
{
  config_sptr c(new config);
  if (!read_config(path, *c))
    return config_sptr();
  return c;
}
// <config reader stuff>

// <config writer stuff>

/// Serialize the value of a property to a string.
///
/// @param prop the property which value to serialize.
///
/// @return the string that represents the value of @p prop.
static string
write_property_value(const property_sptr& prop)
{
  string result;
  if (simple_property_sptr simple_prop = is_simple_property(prop))
    {
      if (!simple_prop->has_empty_value())
	result = simple_prop->get_value()->as_string();
    }
  else if (list_property_sptr list_prop = is_list_property(prop))
    result = list_prop->get_value()->as_string();
  else if (tuple_property_sptr tuple_prop = is_tuple_property(prop))
      result = tuple_prop->get_value()->as_string();
  else
    // This new kind of property is not yet supported!
    abort();
  return result;
}

/// Serialize an ini property to an output stream.
///
/// @param prop the property to serialize to the output stream.
///
/// @param out the output stream to serialize to.
///
/// @return true if the ouput stream is left in a non-erratic state.
static bool
write_property(const property_sptr& prop,
	       std::ostream& out)
{
  out << prop->get_name();
  string value = write_property_value(prop);
  if (!value.empty())
    out << " = " << write_property_value(prop);
  return out.good();
}

/// Serialize an ini section to an output stream.
///
/// @param section the ini section to serialize.
///
/// @param out the output stream to serialize the section to.
static bool
write_section(const config::section& section,
	      std::ostream& out)
{
  out << "[" << section.get_name() << "]\n";
  for (config::properties_type::const_iterator i =
	 section.get_properties().begin();
       i != section.get_properties().end();
       ++i)
    {
      out << "  ";
      write_property(*i, out);
      out << "\n";
    }
  return out.good();
}

/// Serialize a vector of sections that make up an ini config file to
/// an output stream.
///
/// Note that an ini config is just a collection of sections.
///
/// @param sections the vector of sections to serialize.
///
/// @param out the output stream.
///
/// @return true if the output stream is left in a non-erratic state.
bool
write_sections(const config::sections_type& sections,
	       std::ostream& out)
{
  for (config::sections_type::const_iterator i = sections.begin();
       i != sections.end();
       ++i)
    {
      write_section(**i, out);
      out << "\n";
    }
  return out.good();
}

/// Serialize a vector of sections that make up an ini config to a
/// file.
///
/// @param sections the vector of sections to serialize.
///
/// @param out the output stream.
///
/// @return true if the output stream is left in a non-erratic state.
bool
write_sections(const config::sections_type& sections,
	       const string& path)
{
  std::ofstream f(path.c_str(), std::ofstream::binary);

  if (!f.good())
    return false;

  bool is_ok = write_sections(sections, f);

  f.close();

  return is_ok;
}

/// Serialize an instance of @ref config to an output stream.
///
/// @param conf the instance of @ref config to serialize.
///
/// @param output the output stream to serialize @p conf to.
///
/// @return true upon successful completion.
bool
write_config(const config& conf,
	     std::ostream& output)
{
  if (!write_sections(conf.get_sections(), output))
    return false;
  return true;
}

/// Serialize an instance of @ref conf to an on-disk file.
///
/// @param conf the instance of @ref config to serialize.
///
/// @param path the path to the on-disk file to serialize to.
///
/// @return true upon successful completion.
bool
write_config(const config& conf,
	     const string& path)
{
  if (!write_sections(conf.get_sections(), path))
    return false;
  return true;
}
// </config writer stuff>

// <function_call_expr stuff>

/// The private data type of @ref function_call_expr.
struct function_call_expr::priv
{
  string		name_;
  vector<string>	arguments_;

  priv()
  {}

  priv(const string& name,
       const vector<string>& arguments)
    : name_(name),
      arguments_(arguments)
  {}
}; // end struct function_call_expr::priv

/// Constructor for the @ref function_call_expr type.
///
/// @param name the name of the function being called.
///
/// @param args a vector of the arguements of the function being
/// called.
function_call_expr::function_call_expr(const string& name,
				       const vector<string>& args)
  : priv_(new priv(name, args))
{}

/// Getter of the name of the function being called.
///
/// @return the name of the function being called.
const string&
function_call_expr::get_name() const
{return priv_->name_;}

/// Getter for the arguments of the function call expression.
///
/// That is, a getter for the arguments of the function call.
///
/// @return the operands of the function call expression.
const vector<string>&
function_call_expr::get_arguments() const
{return priv_->arguments_;}

/// Getter for the arguments of the function call expression.
///
/// That is, a getter for the arguments of the function call.
///
/// @return the operands of the function call expression.
vector<string>&
function_call_expr::get_arguments()
{return priv_->arguments_;}

/// Read a function call expression and build its representation.
///
/// @param input the input stream where to read the function call
/// expression from.
///
/// @param expr the expression resulting from the parsing.  This is an
/// output parameter that is set iff this function returns true.
///
/// @return true iff the parameter @p expr is successfully set with
/// the resulting of parsing the @p input.
bool
read_function_call_expr(std::istream& input,
			function_call_expr_sptr& expr)
{
  read_context ctxt(input);
  return ctxt.read_function_call_expr(expr);
}

/// Read a function call expression and build its representation.
///
/// @param input a string where to read the function call expression
/// from.
///
/// @param expr the expression resulting from the parsing.  This is an
/// output parameter that is set iff this function returns true.
///
/// @return true iff the parameter @p expr is successfully set with
/// the resulting of parsing the @p input.
bool
read_function_call_expr(const string& input,
			function_call_expr_sptr& expr)
{
  std::istringstream in(input);
  return read_function_call_expr(in, expr);
}

/// Read a function call expression and build its representation.
///
/// @param input a string where to read the function call expression
/// from.
///
/// @return the representation of the expression resulting from the
/// parsing.
function_call_expr_sptr
read_function_call_expr(const string& input)
{
  function_call_expr_sptr expr;
  read_function_call_expr(input, expr);
  return expr;
}
// </function_call_expr stuff>
}// end namespace ini
}// end namespace abigail

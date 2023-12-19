// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
//Author: Dodji Seketeli

/// @file
///
/// Definitions for the Internal Representation artifacts of libabigail.

#include <cxxabi.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <sstream>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-interned-str.h"
#include "abg-ir.h"
#include "abg-corpus.h"
#include "abg-corpus-priv.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

#include "abg-tools-utils.h"
#include "abg-comp-filter.h"
#include "abg-ir-priv.h"

namespace
{
/// This internal type is a tree walker that walks the sub-tree of a
/// type and sets the environment of the type (including its sub-type)
/// to a new environment.
class environment_setter : public abigail::ir::ir_node_visitor
{
  const abigail::ir::environment* env_;

public:
  environment_setter(const abigail::ir::environment* env)
    : env_(env)
  {}

  /// This function is called on each sub-tree node that is a
  /// declaration.  Note that it's also called on some types because
  /// most types that have a declarations also inherit the type @ref
  /// decl_base.
  ///
  /// @param d the declaration being visited.
  bool
  visit_begin(abigail::ir::decl_base* d)
  {
    if (const abigail::ir::environment* env = d->get_environment())
      {
	ABG_ASSERT(env == env_);
	return false;
      }
    else
      d->set_environment(env_);

    return true;

  }

  /// This function is called on each sub-tree node that is a type.
  ///
  /// @param t the type being visited.
  bool
  visit_begin(abigail::ir::type_base* t)
  {
    if (abigail::ir::environment* env = t->get_environment())
      {
	ABG_ASSERT(env == env_);
	return false;
      }
    else
      {
	ABG_ASSERT(!t->get_environment());
	t->set_environment(env_);
      }
    return true;
  }
};

/// This internal type is a tree walking that is used to set the
/// qualified name of a tree of decls and types.  It used by the
/// function update_qualified_name().
class qualified_name_setter : public abigail::ir::ir_node_visitor
{

public:
  bool
  do_update(abigail::ir::decl_base* d);

  bool
  visit_begin(abigail::ir::decl_base* d);

  bool
  visit_begin(abigail::ir::type_base* d);
}; // end class qualified_name_setter

}// end anon namespace

namespace abigail
{

// Inject.
using std::string;
using std::list;
using std::vector;
using std::unordered_map;
using std::dynamic_pointer_cast;
using std::static_pointer_cast;

/// Convenience typedef for a map of string -> string*.
typedef unordered_map<string, string*> pool_map_type;

/// The type of the private data structure of type @ref
/// intered_string_pool.
struct interned_string_pool::priv
{
  pool_map_type map;
}; //end struc struct interned_string_pool::priv

/// Default constructor.
interned_string_pool::interned_string_pool()
  : priv_(new priv)
{
  priv_->map[""] = 0;
}

/// Test if the interned string pool already contains a string with a
/// given value.
///
/// @param s the string to test for.
///
/// @return true if the pool contains a string with the value @p s.
bool
interned_string_pool::has_string(const char* s) const
{return priv_->map.find(s) != priv_->map.end();}

/// Get a pointer to the interned string which has a given value.
///
/// @param s the value of the interned string to look for.
///
/// @return a pointer to the raw string of characters which has the
/// value of @p s.  Or null if no string with value @p s was interned.
const char*
interned_string_pool::get_string(const char* s) const
{
  unordered_map<string, string*>::const_iterator i =
    priv_->map.find(s);
  if (i == priv_->map.end())
    return 0;
  if (i->second)
    return i->second->c_str();
  return "";
}

/// Create an interned string with a given value.
///
/// @param str_value the value of the interned string to create.
///
/// @return the new created instance of @ref interned_string created.
interned_string
interned_string_pool::create_string(const std::string& str_value)
{
  string*& result = priv_->map[str_value];
  if (!result && !str_value.empty())
    result = new string(str_value);
  return interned_string(result);
}

/// Destructor.
interned_string_pool::~interned_string_pool()
{
  for (pool_map_type::iterator i = priv_->map.begin();
       i != priv_->map.end();
       ++i)
    if (i->second)
      delete i->second;
}

/// Equality operator.
///
/// @param l the instance of std::string on the left-hand-side of the
/// equality operator.
///
/// @param r the instance of @ref interned_string on the
/// right-hand-side of the equality operator.
///
/// @return true iff the two string are equal.
bool
operator==(const std::string& l, const interned_string& r)
{return r.operator==(l);}

bool
operator!=(const std::string& l, const interned_string& r)
{return !(l == r);}

/// Streaming operator.
///
/// Streams an instance of @ref interned_string to an output stream.
///
/// @param o the destination output stream.
///
/// @param s the instance of @ref interned_string to stream out.
///
/// @return the output stream this function just streamed to.
std::ostream&
operator<<(std::ostream& o, const interned_string& s)
{
  o << static_cast<std::string>(s);
  return o;
}

/// Concatenation operator.
///
/// Concatenate two instances of @ref interned_string, builds an
/// instance of std::string with the resulting string and return it.
///
/// @param s1 the first string to consider.
///
/// @param s2 the second string to consider.
///
/// @return the resuting concatenated string.
std::string
operator+(const interned_string& s1,const std::string& s2)
{return static_cast<std::string>(s1) + s2;}

/// Concatenation operator.
///
/// Concatenate two instances of @ref interned_string, builds an
/// instance of std::string with the resulting string and return it.
///
/// @param s1 the first string to consider.
///
/// @param s2 the second string to consider.
///
/// @return the resuting concatenated string.
std::string
operator+(const std::string& s1, const interned_string& s2)
{return s1 + static_cast<std::string>(s2);}

namespace ir
{

static size_t
hash_as_canonical_type_or_constant(const type_base *t);

static bool
has_generic_anonymous_internal_type_name(const decl_base *d);

static interned_string
get_generic_anonymous_internal_type_name(const decl_base *d);

/// @brief the location of a token represented in its simplest form.
/// Instances of this type are to be stored in a sorted vector, so the
/// type must have proper relational operators.
class expanded_location
{
  string	path_;
  unsigned	line_;
  unsigned	column_;

  expanded_location();

public:

  friend class location_manager;

  expanded_location(const string& path, unsigned line, unsigned column)
  : path_(path), line_(line), column_(column)
  {}

  bool
  operator==(const expanded_location& l) const
  {
    return (path_ == l.path_
	    && line_ == l.line_
	    && column_ && l.column_);
  }

  bool
  operator<(const expanded_location& l) const
  {
    if (path_ < l.path_)
      return true;
    else if (path_ > l.path_)
      return false;

    if (line_ < l.line_)
      return true;
    else if (line_ > l.line_)
      return false;

    return column_ < l.column_;
  }
};

/// Expand the location into a tripplet path, line and column number.
///
/// @param path the output parameter where this function sets the
/// expanded path.
///
/// @param line the output parameter where this function sets the
/// expanded line.
///
/// @param column the ouptut parameter where this function sets the
/// expanded column.
void
location::expand(std::string& path, unsigned& line, unsigned& column) const
{
  ABG_ASSERT(get_location_manager());
  get_location_manager()->expand_location(*this, path, line, column);
}


/// Expand the location into a string.
///
/// @return the string representing the location.
string
location::expand(void) const
{
  string path, result;
  unsigned line = 0, column = 0;
  expand(path, line, column);

  std::ostringstream o;
  o << path << ":" << line << ":" << column;
  return o.str();
}

struct location_manager::priv
{
  /// This sorted vector contains the expanded locations of the tokens
  /// coming from a given ABI Corpus.  The index of a given expanded
  /// location in the table gives us an integer that is used to build
  /// instance of location types.
  std::vector<expanded_location> locs;
};

location_manager::location_manager()
{priv_ = shared_ptr<location_manager::priv>(new location_manager::priv);}

/// Insert the triplet representing a source locus into our internal
/// vector of location triplet.  Return an instance of location type,
/// built from an integral type that represents the index of the
/// source locus triplet into our source locus table.
///
/// @param file_path the file path of the source locus
/// @param line the line number of the source location
/// @param col the column number of the source location
location
location_manager::create_new_location(const std::string&	file_path,
				      size_t			line,
				      size_t			col)
{
  expanded_location l(file_path, line, col);

  // Just append the new expanded location to the end of the vector
  // and return its index.  Note that indexes start at 1.
  priv_->locs.push_back(l);
  return location(priv_->locs.size(), this);
}

/// Given an instance of location type, return the triplet
/// {path,line,column} that represents the source locus.  Note that
/// the location must have been previously created from the function
/// location_manager::create_new_location, otherwise this function yields
/// unexpected results, including possibly a crash.
///
/// @param location the instance of location type to expand
/// @param path the resulting path of the source locus
/// @param line the resulting line of the source locus
/// @param column the resulting colum of the source locus
void
location_manager::expand_location(const location&	location,
				  std::string&		path,
				  unsigned&		line,
				  unsigned&		column) const
{
  if (location.value_ == 0)
    return;
  expanded_location &l = priv_->locs[location.value_ - 1];
  path = l.path_;
  line = l.line_;
  column = l.column_;
}

typedef unordered_map<function_type_sptr,
		      bool,
		      function_type::hash,
		      type_shared_ptr_equal> fn_type_ptr_map;

// <type_maps stuff>

struct type_maps::priv
{
  mutable istring_type_base_wptrs_map_type	basic_types_;
  mutable istring_type_base_wptrs_map_type	class_types_;
  mutable istring_type_base_wptrs_map_type	union_types_;
  mutable istring_type_base_wptrs_map_type	enum_types_;
  mutable istring_type_base_wptrs_map_type	typedef_types_;
  mutable istring_type_base_wptrs_map_type	qualified_types_;
  mutable istring_type_base_wptrs_map_type	pointer_types_;
  mutable istring_type_base_wptrs_map_type	reference_types_;
  mutable istring_type_base_wptrs_map_type	array_types_;
  mutable istring_type_base_wptrs_map_type	subrange_types_;
  mutable istring_type_base_wptrs_map_type	function_types_;
  mutable vector<type_base_wptr>		sorted_types_;
}; // end struct type_maps::priv

type_maps::type_maps()
  : priv_(new priv)
{}

/// Test if the type_maps is empty.
///
/// @return true iff the type_maps is empty.
bool
type_maps::empty() const
{
  return (basic_types().empty()
	  && class_types().empty()
	  && union_types().empty()
	  && enum_types().empty()
	  && typedef_types().empty()
	  && qualified_types().empty()
	  && pointer_types().empty()
	  && reference_types().empty()
	  && array_types().empty()
	  && subrange_types().empty()
	  && function_types().empty());
}

/// Getter for the map that associates the name of a basic type to the
/// vector instances of type_decl_sptr that represents that type.
const istring_type_base_wptrs_map_type&
type_maps::basic_types() const
{return priv_->basic_types_;}

/// Getter for the map that associates the name of a basic type to the
/// vector of instances of @ref type_decl_sptr that represents that
/// type.
istring_type_base_wptrs_map_type&
type_maps::basic_types()
{return priv_->basic_types_;}

/// Getter for the map that associates the name of a class type to the
/// vector of instances of @ref class_decl_sptr that represents that
/// type.
const istring_type_base_wptrs_map_type&
type_maps::class_types() const
{return priv_->class_types_;}

/// Getter for the map that associates the name of a class type to the
/// vector of instances of @ref class_decl_sptr that represents that
/// type.
istring_type_base_wptrs_map_type&
type_maps::class_types()
{return priv_->class_types_;}

/// Getter for the map that associates the name of a union type to the
/// vector of instances of @ref union_decl_sptr that represents that
/// type.
istring_type_base_wptrs_map_type&
type_maps::union_types()
{return priv_->union_types_;}

/// Getter for the map that associates the name of a union type to the
/// vector of instances of @ref union_decl_sptr that represents that
/// type.
const istring_type_base_wptrs_map_type&
type_maps::union_types() const
{return priv_->union_types_;}

/// Getter for the map that associates the name of an enum type to the
/// vector of instances of @ref enum_type_decl_sptr that represents
/// that type.
istring_type_base_wptrs_map_type&
type_maps::enum_types()
{return priv_->enum_types_;}

/// Getter for the map that associates the name of an enum type to the
/// vector of instances of @ref enum_type_decl_sptr that represents
/// that type.
const istring_type_base_wptrs_map_type&
type_maps::enum_types() const
{return priv_->enum_types_;}

/// Getter for the map that associates the name of a typedef to the
/// vector of instances of @ref typedef_decl_sptr that represents tha
/// type.
istring_type_base_wptrs_map_type&
type_maps::typedef_types()
{return priv_->typedef_types_;}

/// Getter for the map that associates the name of a typedef to the
/// vector of instances of @ref typedef_decl_sptr that represents tha
/// type.
const istring_type_base_wptrs_map_type&
type_maps::typedef_types() const
{return priv_->typedef_types_;}

/// Getter for the map that associates the name of a qualified type to
/// the vector of instances of @ref qualified_type_def_sptr.
istring_type_base_wptrs_map_type&
type_maps::qualified_types()
{return priv_->qualified_types_;}

/// Getter for the map that associates the name of a qualified type to
/// the vector of instances of @ref qualified_type_def_sptr.
const istring_type_base_wptrs_map_type&
type_maps::qualified_types() const
{return priv_->qualified_types_;}

/// Getter for the map that associates the name of a pointer type to
/// the vector of instances of @ref pointer_type_def_sptr that
/// represents that type.
istring_type_base_wptrs_map_type&
type_maps::pointer_types()
{return priv_->pointer_types_;}

/// Getter for the map that associates the name of a pointer type to
/// the vector of instances of @ref pointer_type_def_sptr that
/// represents that type.
const istring_type_base_wptrs_map_type&
type_maps::pointer_types() const
{return priv_->pointer_types_;}

/// Getter for the map that associates the name of a reference type to
/// the vector of instances of @ref reference_type_def_sptr that
/// represents that type.
istring_type_base_wptrs_map_type&
type_maps::reference_types()
{return priv_->reference_types_;}

/// Getter for the map that associates the name of a reference type to
/// the vector of instances of @ref reference_type_def_sptr that
/// represents that type.
const istring_type_base_wptrs_map_type&
type_maps::reference_types() const
{return priv_->reference_types_;}

/// Getter for the map that associates the name of an array type to
/// the vector of instances of @ref array_type_def_sptr that
/// represents that type.
istring_type_base_wptrs_map_type&
type_maps::array_types()
{return priv_->array_types_;}

/// Getter for the map that associates the name of an array type to
/// the vector of instances of @ref array_type_def_sptr that
/// represents that type.
const istring_type_base_wptrs_map_type&
type_maps::array_types() const
{return priv_->array_types_;}

/// Getter for the map that associates the name of a subrange type to
/// the vector of instances of @ref array_type_def::subrange_sptr that
/// represents that type.
istring_type_base_wptrs_map_type&
type_maps::subrange_types()
{return priv_->subrange_types_;}

/// Getter for the map that associates the name of a subrange type to
/// the vector of instances of @ref array_type_def::subrange_sptr that
/// represents that type.
const istring_type_base_wptrs_map_type&
type_maps::subrange_types() const
{return priv_->subrange_types_;}

/// Getter for the map that associates the name of a function type to
/// the vector of instances of @ref function_type_sptr that represents
/// that type.
const istring_type_base_wptrs_map_type&
type_maps::function_types() const
{return priv_->function_types_;}

/// Getter for the map that associates the name of a function type to
/// the vector of instances of @ref function_type_sptr that represents
/// that type.
istring_type_base_wptrs_map_type&
type_maps::function_types()
{return priv_->function_types_;}

/// A comparison functor to compare/sort types based on their pretty
/// representations.
struct type_name_comp
{
  /// Comparison operator for two instances of @ref type_base.
  ///
  /// This compares the two types by lexicographically comparing their
  /// pretty representation.
  ///
  /// @param l the left-most type to compare.
  ///
  /// @param r the right-most type to compare.
  ///
  /// @return true iff @p l < @p r.
  bool
  operator()(type_base *l, type_base *r) const
  {
    if (l == 0 && r == 0)
      return false;

    string l_repr = get_pretty_representation(l);
    string r_repr = get_pretty_representation(r);
    return l_repr < r_repr;
  }

  /// Comparison operator for two instances of @ref type_base.
  ///
  /// This compares the two types by lexicographically comparing their
  /// pretty representation.
  ///
  /// @param l the left-most type to compare.
  ///
  /// @param r the right-most type to compare.
  ///
  /// @return true iff @p l < @p r.
  bool
  operator()(const type_base_sptr &l, const type_base_sptr &r) const
  {return operator()(l.get(), r.get());}

  /// Comparison operator for two instances of @ref type_base.
  ///
  /// This compares the two types by lexicographically comparing their
  /// pretty representation.
  ///
  /// @param l the left-most type to compare.
  ///
  /// @param r the right-most type to compare.
  ///
  /// @return true iff @p l < @p r.
  bool
  operator()(const type_base_wptr &l, const type_base_wptr &r) const
  {return operator()(type_base_sptr(l), type_base_sptr(r));}
}; // end struct type_name_comp

/// Compare two types by comparing their canonical types if present.
///
/// If the canonical types are not present (because the types have not
/// yet been canonicalized, for instance) then the types are compared
/// structurally.
///
/// @param l the first type to take into account in the comparison.
///
/// @param r the second type to take into account in the comparison.
template<typename T>
bool
try_canonical_compare(const T *l, const T *r)
{
  if (const type_base *lc = l->get_naked_canonical_type())
    if (const type_base *rc = r->get_naked_canonical_type())
      return lc == rc;
  return equals(*l, *r, 0);
}

/// Getter of all types types sorted by their pretty representation.
///
/// @return a sorted vector of all types sorted by their pretty
/// representation.
const vector<type_base_wptr>&
type_maps::get_types_sorted_by_name() const
{
  if (priv_->sorted_types_.empty())
    {
      istring_type_base_wptrs_map_type::const_iterator i;
      vector<type_base_wptr>::const_iterator j;

      for (i = basic_types().begin(); i != basic_types().end(); ++i)
	for (j = i->second.begin(); j != i->second.end(); ++j)
	  priv_->sorted_types_.push_back(*j);

      for (i = class_types().begin(); i != class_types().end(); ++i)
	for (j = i->second.begin(); j != i->second.end(); ++j)
	  priv_->sorted_types_.push_back(*j);

      for (i = union_types().begin(); i != union_types().end(); ++i)
	for (j = i->second.begin(); j != i->second.end(); ++j)
	  priv_->sorted_types_.push_back(*j);

      for (i = enum_types().begin(); i != enum_types().end(); ++i)
	for (j = i->second.begin(); j != i->second.end(); ++j)
	  priv_->sorted_types_.push_back(*j);

      for (i = typedef_types().begin(); i != typedef_types().end(); ++i)
	for (j = i->second.begin(); j != i->second.end(); ++j)
	  priv_->sorted_types_.push_back(*j);

      type_name_comp comp;
      sort(priv_->sorted_types_.begin(), priv_->sorted_types_.end(), comp);
    }

  return priv_->sorted_types_;
}

// </type_maps stuff>

// <translation_unit stuff>

/// Constructor of translation_unit.
///
/// @param env the environment of this translation unit.  Please note
/// that the life time of the environment must be greater than the
/// life time of the translation unit because the translation uses
/// resources that are allocated in the environment.
///
/// @param path the location of the translation unit.
///
/// @param address_size the size of addresses in the translation unit,
/// in bits.
translation_unit::translation_unit(const environment*	env,
				   const std::string&	path,
				   char		address_size)
  : priv_(new priv(env))
{
  priv_->path_ = path;
  priv_->address_size_ = address_size;
}

/// Getter of the the global scope of the translation unit.
///
/// @return the global scope of the current translation unit.  If
/// there is not global scope allocated yet, this function creates one
/// and returns it.
const scope_decl_sptr&
translation_unit::get_global_scope() const
{
  return const_cast<translation_unit*>(this)->get_global_scope();
}

/// Getter of the the global scope of the translation unit.
///
/// @return the global scope of the current translation unit.  If
/// there is not global scope allocated yet, this function creates one
/// and returns it.
scope_decl_sptr&
translation_unit::get_global_scope()
{
  if (!priv_->global_scope_)
    {
      priv_->global_scope_.reset
	(new global_scope(const_cast<translation_unit*>(this)));
      // The global scope must be out of the same environment as its
      // translation unit.
      priv_->global_scope_->
	set_environment(const_cast<environment*>(get_environment()));
      priv_->global_scope_->set_translation_unit
	(const_cast<translation_unit*>(this));
    }
  return priv_->global_scope_;
}

/// Getter of the types of the current @ref translation_unit.
///
/// @return the maps of the types of the translation unit.
const type_maps&
translation_unit::get_types() const
{return priv_->types_;}

/// Getter of the types of the current @ref translation_unit.
///
/// @return the maps of the types of the translation unit.
type_maps&
translation_unit::get_types()
{return priv_->types_;}

/// Get the vector of function types that are used in the current
/// translation unit.
///
/// @return the vector of function types that are used in the current
/// translation unit.
const vector<function_type_sptr>&
translation_unit::get_live_fn_types() const
{return priv_->live_fn_types_;}

/// Getter of the environment of the current @ref translation_unit.
///
/// @return the translation unit of the current translation unit.
const environment*
translation_unit::get_environment() const
{return priv_->env_;}

/// Getter of the environment of the current @ref translation_unit.
///
/// @return the translation unit of the current translation unit.
environment*
translation_unit::get_environment()
{return const_cast<environment*>(priv_->env_);}

/// Setter of the environment of the current @ref translation_unit.
///
/// @param env the environment.
void
translation_unit::set_environment(const environment* env)
{priv_->env_ = env;}

/// Getter of the language of the source code of the translation unit.
///
/// @return the language of the source code.
translation_unit::language
translation_unit::get_language() const
{return priv_->language_;}

/// Setter of the language of the source code of the translation unit.
///
/// @param l the new language.
void
translation_unit::set_language(language l)
{priv_->language_ = l;}


/// Get the path of the current translation unit.
///
/// This path is relative to the build directory of the translation
/// unit as returned by translation_unit::get_compilation_dir_path.
///
/// @return the relative path of the compilation unit associated to
/// the current instance of translation_unit.
//
const std::string&
translation_unit::get_path() const
{return priv_->path_;}

/// Set the path associated to the current instance of
/// translation_unit.
///
/// This path is relative to the build directory of the translation
/// unit as returned by translation_unit::get_compilation_dir_path.
///
/// @param a_path the new relative path to set.
void
translation_unit::set_path(const string& a_path)
{priv_->path_ = a_path;}


/// Get the path of the directory that was 'current' when the
/// translation unit was compiled.
///
/// Note that the path returned by translation_unit::get_path is
/// relative to the path returned by this function.
///
/// @return the compilation directory for the current translation
/// unit.
const std::string&
translation_unit::get_compilation_dir_path() const
{return priv_->comp_dir_path_;}

/// Set the path of the directory that was 'current' when the
/// translation unit was compiled.
///
/// Note that the path returned by translation_unit::get_path is
/// relative to the path returned by this function.
///
/// @param the compilation directory for the current translation unit.
void
translation_unit::set_compilation_dir_path(const std::string& d)
{priv_->comp_dir_path_ = d;}

/// Get the concatenation of the build directory and the relative path
/// of the translation unit.
///
/// @return the absolute path of the translation unit.
const std::string&
translation_unit::get_absolute_path() const
{
  if (priv_->abs_path_.empty())
    {
      string path;
      if (!priv_->path_.empty())
	{
	  if (!priv_->comp_dir_path_.empty())
	    {
	      path = priv_->comp_dir_path_;
	      path += "/";
	    }
	  path += priv_->path_;
	}
      priv_->abs_path_ = path;
    }

  return priv_->abs_path_;
}

/// Set the corpus this translation unit is a member of.
///
/// Note that adding a translation unit to a @ref corpus automatically
/// triggers a call to this member function.
///
/// @param corpus the corpus.
void
translation_unit::set_corpus(corpus* c)
{priv_->corp = c;}

/// Get the corpus this translation unit is a member of.
///
/// @return the parent corpus, or nil if this doesn't belong to any
/// corpus yet.
corpus*
translation_unit::get_corpus()
{return priv_->corp;}

/// Get the corpus this translation unit is a member of.
///
/// @return the parent corpus, or nil if this doesn't belong to any
/// corpus yet.
const corpus*
translation_unit::get_corpus() const
{return const_cast<translation_unit*>(this)->get_corpus();}

/// Getter of the location manager for the current translation unit.
///
/// @return a reference to the location manager for the current
/// translation unit.
location_manager&
translation_unit::get_loc_mgr()
{return priv_->loc_mgr_;}

/// const Getter of the location manager.
///
/// @return a const reference to the location manager for the current
/// translation unit.
const location_manager&
translation_unit::get_loc_mgr() const
{return priv_->loc_mgr_;}

/// Tests whether if the current translation unit contains ABI
/// artifacts or not.
///
/// @return true iff the current translation unit is empty.
bool
translation_unit::is_empty() const
{return get_global_scope()->is_empty();}

/// Getter of the address size in this translation unit.
///
/// @return the address size, in bits.
char
translation_unit::get_address_size() const
{return priv_->address_size_;}

/// Setter of the address size in this translation unit.
///
/// @param a the new address size in bits.
void
translation_unit::set_address_size(char a)
{priv_->address_size_= a;}

/// Getter of the 'is_constructed" flag.  It says if the translation
/// unit is fully constructed or not.
///
/// This flag is important for cases when comparison might depend on
/// if the translation unit is fully built or not.  For instance, when
/// reading types from DWARF, the virtual methods of a class are not
/// necessarily fully constructed until we have reached the end of the
/// translation unit.  In that case, before we've reached the end of
/// the translation unit, we might not take virtual functions into
/// account when comparing classes.
///
/// @return true if the translation unit is constructed.
bool
translation_unit::is_constructed() const
{return priv_->is_constructed_;}

/// Setter of the 'is_constructed" flag.  It says if the translation
/// unit is fully constructed or not.
///
/// This flag is important for cases when comparison might depend on
/// if the translation unit is fully built or not.  For instance, when
/// reading types from DWARF, the virtual methods of a class are not
/// necessarily fully constructed until we have reached the end of the
/// translation unit.  In that case, before we've reached the end of
/// the translation unit, we might not take virtual functions into
/// account when comparing classes.
///
/// @param f true if the translation unit is constructed.
void
translation_unit::set_is_constructed(bool f)
{priv_->is_constructed_ = f;}

/// Compare the current translation unit against another one.
///
/// @param other the other tu to compare against.
///
/// @return true if the two translation units are equal, false
/// otherwise.
bool
translation_unit::operator==(const translation_unit& other)const
{
  if (get_address_size() != other.get_address_size())
    return false;

  return *get_global_scope() == *other.get_global_scope();
}

/// Inequality operator.
///
/// @param o the instance of @ref translation_unit to compare the
/// current instance against.
///
/// @return true iff the current instance is different from @p o.
bool
translation_unit::operator!=(const translation_unit& o) const
{return ! operator==(o);}

/// Ensure that the life time of a function type is bound to the life
/// time of the current translation unit.
///
/// @param ftype the function time which life time to bind to the life
/// time of the current instance of @ref translation_unit.  That is,
/// it's onlyh when the translation unit is destroyed that the
/// function type can be destroyed to.
void
translation_unit::bind_function_type_life_time(function_type_sptr ftype) const
{
  const environment* env = get_environment();
  ABG_ASSERT(env);

  const_cast<translation_unit*>(this)->priv_->live_fn_types_.push_back(ftype);

  interned_string repr = get_type_name(ftype);
  const_cast<translation_unit*>(this)->get_types().function_types()[repr].
    push_back(ftype);

  // The function type must be out of the same environment as its
  // translation unit.
  if (const environment* e = ftype->get_environment())
    ABG_ASSERT(env == e);
  ftype->set_environment(const_cast<environment*>(env));

  if (const translation_unit* existing_tu = ftype->get_translation_unit())
    ABG_ASSERT(existing_tu == this);
  else
    ftype->set_translation_unit(const_cast<translation_unit*>(this));
}

/// This implements the ir_traversable_base::traverse virtual
/// function.
///
/// @param v the visitor used on the member nodes of the translation
/// unit during the traversal.
///
/// @return true if the entire type IR tree got traversed, false
/// otherwise.
bool
translation_unit::traverse(ir_node_visitor& v)
{return get_global_scope()->traverse(v);}

translation_unit::~translation_unit()
{}

/// Converts a translation_unit::language enumerator into a string.
///
/// @param l the language enumerator to translate.
///
/// @return the resulting string.
string
translation_unit_language_to_string(translation_unit::language l)
{
  switch (l)
    {
    case translation_unit::LANG_UNKNOWN:
      return "LANG_UNKNOWN";
    case translation_unit::LANG_Cobol74:
      return "LANG_Cobol74";
    case translation_unit::LANG_Cobol85:
      return "LANG_Cobol85";
    case translation_unit::LANG_C89:
      return "LANG_C89";
    case translation_unit::LANG_C99:
      return "LANG_C99";
    case translation_unit::LANG_C11:
      return "LANG_C11";
    case translation_unit::LANG_C:
      return "LANG_C";
    case translation_unit::LANG_C_plus_plus_11:
      return "LANG_C_plus_plus_11";
    case translation_unit::LANG_C_plus_plus_14:
      return "LANG_C_plus_plus_14";
    case translation_unit::LANG_C_plus_plus:
      return "LANG_C_plus_plus";
    case translation_unit::LANG_ObjC:
      return "LANG_ObjC";
    case translation_unit::LANG_ObjC_plus_plus:
      return "LANG_ObjC_plus_plus";
    case translation_unit::LANG_Fortran77:
      return "LANG_Fortran77";
    case translation_unit::LANG_Fortran90:
      return "LANG_Fortran90";
    case translation_unit::LANG_Fortran95:
      return "LANG_Fortran95";
    case translation_unit::LANG_Ada83:
      return "LANG_Ada83";
    case translation_unit::LANG_Ada95:
      return "LANG_Ada95";
    case translation_unit::LANG_Pascal83:
      return "LANG_Pascal83";
    case translation_unit::LANG_Modula2:
      return "LANG_Modula2";
    case translation_unit::LANG_Java:
      return "LANG_Java";
    case translation_unit::LANG_PL1:
      return "LANG_PL1";
    case translation_unit::LANG_UPC:
      return "LANG_UPC";
    case translation_unit::LANG_D:
      return "LANG_D";
    case translation_unit::LANG_Python:
      return "LANG_Python";
    case translation_unit::LANG_Go:
      return "LANG_Go";
    case translation_unit::LANG_Mips_Assembler:
      return "LANG_Mips_Assembler";
    default:
      return "LANG_UNKNOWN";
    }

  return "LANG_UNKNOWN";
}

/// Parse a string representing a language into a
/// translation_unit::language enumerator into a string.
///
/// @param l the string representing the language.
///
/// @return the resulting translation_unit::language enumerator.
translation_unit::language
string_to_translation_unit_language(const string& l)
{
  if (l == "LANG_Cobol74")
    return translation_unit::LANG_Cobol74;
  else if (l == "LANG_Cobol85")
    return translation_unit::LANG_Cobol85;
  else if (l == "LANG_C89")
    return translation_unit::LANG_C89;
  else if (l == "LANG_C99")
    return translation_unit::LANG_C99;
  else if (l == "LANG_C11")
    return translation_unit::LANG_C11;
  else if (l == "LANG_C")
    return translation_unit::LANG_C;
  else if (l == "LANG_C_plus_plus_11")
    return translation_unit::LANG_C_plus_plus_11;
  else if (l == "LANG_C_plus_plus_14")
    return translation_unit::LANG_C_plus_plus_14;
  else if (l == "LANG_C_plus_plus")
    return translation_unit::LANG_C_plus_plus;
  else if (l == "LANG_ObjC")
    return translation_unit::LANG_ObjC;
  else if (l == "LANG_ObjC_plus_plus")
    return translation_unit::LANG_ObjC_plus_plus;
  else if (l == "LANG_Fortran77")
    return translation_unit::LANG_Fortran77;
  else if (l == "LANG_Fortran90")
    return translation_unit::LANG_Fortran90;
    else if (l == "LANG_Fortran95")
    return translation_unit::LANG_Fortran95;
  else if (l == "LANG_Ada83")
    return translation_unit::LANG_Ada83;
  else if (l == "LANG_Ada95")
    return translation_unit::LANG_Ada95;
  else if (l == "LANG_Pascal83")
    return translation_unit::LANG_Pascal83;
  else if (l == "LANG_Modula2")
    return translation_unit::LANG_Modula2;
  else if (l == "LANG_Java")
    return translation_unit::LANG_Java;
  else if (l == "LANG_PL1")
    return translation_unit::LANG_PL1;
  else if (l == "LANG_UPC")
    return translation_unit::LANG_UPC;
  else if (l == "LANG_D")
    return translation_unit::LANG_D;
  else if (l == "LANG_Python")
    return translation_unit::LANG_Python;
  else if (l == "LANG_Go")
    return translation_unit::LANG_Go;
  else if (l == "LANG_Mips_Assembler")
    return translation_unit::LANG_Mips_Assembler;

  return translation_unit::LANG_UNKNOWN;
}

/// Test if a language enumerator designates the C language.
///
/// @param l the language enumerator to consider.
///
/// @return true iff @p l designates the C language.
bool
is_c_language(translation_unit::language l)
{
  return (l == translation_unit::LANG_C89
	  || l == translation_unit::LANG_C99
	  || l == translation_unit::LANG_C11
	  || l == translation_unit::LANG_C);
}

/// Test if a language enumerator designates the C++ language.
///
/// @param l the language enumerator to consider.
///
/// @return true iff @p l designates the C++ language.
bool
is_cplus_plus_language(translation_unit::language l)
{
  return (l == translation_unit::LANG_C_plus_plus_03
	  || l == translation_unit::LANG_C_plus_plus_11
	  || l == translation_unit::LANG_C_plus_plus_14
	  || l == translation_unit::LANG_C_plus_plus);
}

/// Test if a language enumerator designates the Java language.
///
/// @param l the language enumerator to consider.
///
/// @return true iff @p l designates the Java language.
bool
is_java_language(translation_unit::language l)
{return l == translation_unit::LANG_Java;}

/// Test if a language enumerator designates the Ada language.
///
/// @param l the language enumerator to consider.
///
/// @return true iff @p l designates the Ada language.
bool
is_ada_language(translation_unit::language l)
{
  return (l == translation_unit::LANG_Ada83
	 || l == translation_unit::LANG_Ada95);
}

/// A deep comparison operator for pointers to translation units.
///
/// @param l the first translation unit to consider for the comparison.
///
/// @param r the second translation unit to consider for the comparison.
///
/// @return true if the two translation units are equal, false otherwise.
bool
operator==(const translation_unit_sptr& l, const translation_unit_sptr& r)
{
  if (l.get() == r.get())
    return true;

  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// A deep inequality operator for pointers to translation units.
///
/// @param l the first translation unit to consider for the comparison.
///
/// @param r the second translation unit to consider for the comparison.
///
/// @return true iff the two translation units are different.
bool
operator!=(const translation_unit_sptr& l, const translation_unit_sptr& r)
{return !operator==(l, r);}

// </translation_unit stuff>

// <elf_symbol stuff>
struct elf_symbol::priv
{
  const environment*	env_;
  size_t		index_;
  size_t		size_;
  string		name_;
  elf_symbol::type	type_;
  elf_symbol::binding	binding_;
  elf_symbol::version	version_;
  elf_symbol::visibility visibility_;
  bool			is_defined_;
  // This flag below says if the symbol is a common elf symbol.  In
  // relocatable files, a common symbol is a symbol defined in a
  // section of kind SHN_COMMON.
  //
  // Note that a symbol of kind STT_COMMON is also considered a common
  // symbol.  Here is what the gABI says about STT_COMMON and
  // SHN_COMMON:
  //
  //     Symbols with type STT_COMMON label uninitialized common
  //     blocks. In relocatable objects, these symbols are not
  //     allocated and must have the special section index SHN_COMMON
  //     (see below). In shared objects and executables these symbols
  //     must be allocated to some section in the defining object.
  //
  //     In relocatable objects, symbols with type STT_COMMON are
  //     treated just as other symbols with index SHN_COMMON. If the
  //     link-editor allocates space for the SHN_COMMON symbol in an
  //     output section of the object it is producing, it must
  //     preserve the type of the output symbol as STT_COMMON.
  //
  //     When the dynamic linker encounters a reference to a symbol
  //     that resolves to a definition of type STT_COMMON, it may (but
  //     is not required to) change its symbol resolution rules as
  //     follows: instead of binding the reference to the first symbol
  //     found with the given name, the dynamic linker searches for
  //     the first symbol with that name with type other than
  //     STT_COMMON. If no such symbol is found, it looks for the
  //     STT_COMMON definition of that name that has the largest size.
  bool			is_common_;
  bool			is_linux_string_cst_;
  bool			is_in_ksymtab_;
  uint64_t		crc_;
  bool			is_suppressed_;
  elf_symbol_wptr	main_symbol_;
  elf_symbol_wptr	next_alias_;
  elf_symbol_wptr	next_common_instance_;
  string		id_string_;

  priv()
    : env_(),
      index_(),
      size_(),
      type_(elf_symbol::NOTYPE_TYPE),
      binding_(elf_symbol::GLOBAL_BINDING),
      visibility_(elf_symbol::DEFAULT_VISIBILITY),
      is_defined_(false),
      is_common_(false),
      is_linux_string_cst_(false),
      is_in_ksymtab_(false),
      crc_(0),
      is_suppressed_(false)
  {}

  priv(const environment*	  e,
       size_t			  i,
       size_t			  s,
       const string&		  n,
       elf_symbol::type		  t,
       elf_symbol::binding	  b,
       bool			  d,
       bool			  c,
       const elf_symbol::version& ve,
       elf_symbol::visibility	  vi,
       bool			  is_linux_string_cst,
       bool			  is_in_ksymtab,
       uint64_t			  crc,
       bool			  is_suppressed)
    : env_(e),
      index_(i),
      size_(s),
      name_(n),
      type_(t),
      binding_(b),
      version_(ve),
      visibility_(vi),
      is_defined_(d),
      is_common_(c),
      is_linux_string_cst_(is_linux_string_cst),
      is_in_ksymtab_(is_in_ksymtab),
      crc_(crc),
      is_suppressed_(is_suppressed)
  {
    if (!is_common_)
      is_common_ = type_ == COMMON_TYPE;
  }
}; // end struct elf_symbol::priv

/// Default constructor of the @ref elf_symbol type.
///
/// Note that this constructor is private, so client code cannot use
/// it to create instances of @ref elf_symbol.  Rather, client code
/// should use the @ref elf_symbol::create() function to create
/// instances of @ref elf_symbol instead.
elf_symbol::elf_symbol()
  : priv_(new priv)
{}

/// Constructor of the @ref elf_symbol type.
///
/// Note that this constructor is private, so client code cannot use
/// it to create instances of @ref elf_symbol.  Rather, client code
/// should use the @ref elf_symbol::create() function to create
/// instances of @ref elf_symbol instead.
///
/// @param e the environment we are operating from.
///
/// @param i the index of the symbol in the (ELF) symbol table.
///
/// @param s the size of the symbol.
///
/// @param n the name of the symbol.
///
/// @param t the type of the symbol.
///
/// @param b the binding of the symbol.
///
/// @param d true if the symbol is defined, false otherwise.
///
/// @param c true if the symbol is a common symbol, false otherwise.
///
/// @param ve the version of the symbol.
///
/// @param vi the visibility of the symbol.
///
/// @param is_linux_string_cst true if the symbol is a Linux Kernel
/// string constant defined in the __ksymtab_strings section.
///
/// @param crc the CRC (modversions) value of Linux Kernel symbols
elf_symbol::elf_symbol(const environment* e,
		       size_t		  i,
		       size_t		  s,
		       const string&	  n,
		       type		  t,
		       binding		  b,
		       bool		  d,
		       bool		  c,
		       const version&	  ve,
		       visibility	  vi,
		       bool		  is_linux_string_cst,
		       bool		  is_in_ksymtab,
		       uint64_t		  crc,
		       bool		  is_suppressed)
  : priv_(new priv(e,
		   i,
		   s,
		   n,
		   t,
		   b,
		   d,
		   c,
		   ve,
		   vi,
		   is_linux_string_cst,
		   is_in_ksymtab,
		   crc,
		   is_suppressed))
{}

/// Factory of instances of @ref elf_symbol.
///
/// This is the function to use to create instances of @ref elf_symbol.
///
/// @return a (smart) pointer to a newly created instance of @ref
/// elf_symbol.
elf_symbol_sptr
elf_symbol::create()
{
  elf_symbol_sptr e(new elf_symbol());
  e->priv_->main_symbol_ = e;
  return e;
}

/// Factory of instances of @ref elf_symbol.
///
/// This is the function to use to create instances of @ref elf_symbol.
///
/// @param e the environment we are operating from.
///
/// @param i the index of the symbol in the (ELF) symbol table.
///
/// @param s the size of the symbol.
///
/// @param n the name of the symbol.
///
/// @param t the type of the symbol.
///
/// @param b the binding of the symbol.
///
/// @param d true if the symbol is defined, false otherwise.
///
/// @param c true if the symbol is a common symbol.
///
/// @param ve the version of the symbol.
///
/// @param vi the visibility of the symbol.
///
/// @param is_linux_string_cst if true, it means the symbol represents
/// a string constant from a linux kernel binary.
///
/// @param crc the CRC (modversions) value of Linux Kernel symbols
///
/// @return a (smart) pointer to a newly created instance of @ref
/// elf_symbol.
elf_symbol_sptr
elf_symbol::create(const environment* e,
		   size_t	      i,
		   size_t	      s,
		   const string&      n,
		   type		      t,
		   binding	      b,
		   bool		      d,
		   bool		      c,
		   const version&     ve,
		   visibility	      vi,
		   bool		      is_linux_string_cst,
		   bool		      is_in_ksymtab,
		   uint64_t	      crc,
		   bool		      is_suppressed)
{
  elf_symbol_sptr sym(new elf_symbol(e, i, s, n, t, b, d, c, ve, vi,
				     is_linux_string_cst,
				     is_in_ksymtab, crc, is_suppressed));
  sym->priv_->main_symbol_ = sym;
  return sym;
}

/// Test textual equality between two symbols.
///
/// Textual equality means that the aliases of the compared symbols
/// are not taken into account.  Only the name, type, and version of
/// the symbols are compared.
///
/// @return true iff the two symbols are textually equal.
static bool
textually_equals(const elf_symbol&l,
		 const elf_symbol&r)
{
  bool equals = (l.get_name() == r.get_name()
		 && l.get_type() == r.get_type()
		 && l.is_public() == r.is_public()
		 && l.is_defined() == r.is_defined()
		 && l.is_common_symbol() == r.is_common_symbol()
		 && l.get_version() == r.get_version()
		 && (l.get_crc() == 0 || r.get_crc() == 0
		     || l.get_crc() == r.get_crc()));

  if (equals && l.is_variable())
    // These are variable symbols.  Let's compare their symbol size.
    // The symbol size in this case is the size taken by the storage
    // of the variable.  If that size changes, then it's an ABI
    // change.
    equals = l.get_size() == r.get_size();

  return equals;
}

/// Getter of the environment used by the current instance of @ref
/// elf_symbol.
///
/// @return the enviroment used by the current instance of @ref elf_symbol.
const environment*
elf_symbol::get_environment() const
{return priv_->env_;}

/// Setter of the environment used by the current instance of @ref
/// elf_symbol.
///
/// @param The new enviroment used by the current instance of @ref
/// elf_symbol.
void
elf_symbol::set_environment(const environment* e) const
{priv_->env_ = e;}

/// Getter for the index
///
/// @return the index of the symbol.
size_t
elf_symbol::get_index() const
{return priv_->index_;}

/// Setter for the index.
///
/// @param s the new index.
void
elf_symbol::set_index(size_t s)
{priv_->index_ = s;}

/// Test if the ELF symbol is for a string constant of a Linux binary
/// defined in the __ksymtab_strings symbol table.
///
/// @return true iff ELF symbol is for a string constant of a Linux
/// binary defined in the __ksymtab_strings symbol table.
bool
elf_symbol::get_is_linux_string_cst() const
{return priv_->is_linux_string_cst_;}

/// Getter for the name of the @ref elf_symbol.
///
/// @return a reference to the name of the @ref symbol.
const string&
elf_symbol::get_name() const
{return priv_->name_;}

/// Setter for the name of the current intance of @ref elf_symbol.
///
/// @param n the new name.
void
elf_symbol::set_name(const string& n)
{
  priv_->name_ = n;
  priv_->id_string_.clear();
}

/// Getter for the type of the current instance of @ref elf_symbol.
///
/// @return the type of the elf symbol.
elf_symbol::type
elf_symbol::get_type() const
{return priv_->type_;}

/// Setter for the type of the current instance of @ref elf_symbol.
///
/// @param t the new symbol type.
void
elf_symbol::set_type(type t)
{priv_->type_ = t;}

/// Getter of the size of the symbol.
///
/// @return the size of the symbol, in bytes.
size_t
elf_symbol::get_size() const
{return priv_->size_;}

/// Setter of the size of the symbol.
///
/// @param size the new size of the symbol, in bytes.
void
elf_symbol::set_size(size_t size)
{priv_->size_ = size;}

/// Getter for the binding of the current instance of @ref elf_symbol.
///
/// @return the binding of the symbol.
elf_symbol::binding
elf_symbol::get_binding() const
{return priv_->binding_;}

/// Setter for the binding of the current instance of @ref elf_symbol.
///
/// @param b the new binding.
void
elf_symbol::set_binding(binding b)
{priv_->binding_ = b;}

/// Getter for the version of the current instanc of @ref elf_symbol.
///
/// @return the version of the elf symbol.
elf_symbol::version&
elf_symbol::get_version() const
{return priv_->version_;}

/// Setter for the version of the current instance of @ref elf_symbol.
///
/// @param v the new version of the elf symbol.
void
elf_symbol::set_version(const version& v)
{
  priv_->version_ = v;
  priv_->id_string_.clear();
}

/// Setter of the visibility of the current instance of @ref
/// elf_symbol.
///
/// @param v the new visibility of the elf symbol.
void
elf_symbol::set_visibility(visibility v)
{priv_->visibility_ = v;}

/// Getter of the visibility of the current instance of @ref
/// elf_symbol.
///
/// @return the visibility of the elf symbol.
elf_symbol::visibility
elf_symbol::get_visibility() const
{return priv_->visibility_;}

/// Test if the current instance of @ref elf_symbol is defined or not.
///
/// @return true if the current instance of @ref elf_symbol is
/// defined, false otherwise.
bool
elf_symbol::is_defined() const
{return priv_->is_defined_;}

/// Sets a flag saying if the current instance of @ref elf_symbol is
/// defined
///
/// @param b the new value of the flag.
void
elf_symbol::is_defined(bool d)
{priv_->is_defined_ = d;}

/// Test if the current instance of @ref elf_symbol is public or not.
///
/// This tests if the symbol is defined, has default or protected
///visibility, and either:
///		- has global binding
///		- has weak binding
///		- or has a GNU_UNIQUE binding.
///
/// return true if the current instance of @ref elf_symbol is public,
/// false otherwise.
bool
elf_symbol::is_public() const
{
  return (is_defined()
	  && (get_binding() == GLOBAL_BINDING
	      || get_binding() == WEAK_BINDING
	      || get_binding() == GNU_UNIQUE_BINDING)
	  && (get_visibility() == DEFAULT_VISIBILITY
	      || get_visibility() == PROTECTED_VISIBILITY));
}

/// Test if the current instance of @ref elf_symbol is a function
/// symbol or not.
///
/// @return true if the current instance of @ref elf_symbol is a
/// function symbol, false otherwise.
bool
elf_symbol::is_function() const
{return get_type() == FUNC_TYPE || get_type() == GNU_IFUNC_TYPE;}

/// Test if the current instance of @ref elf_symbol is a variable
/// symbol or not.
///
/// @return true if the current instance of @ref elf_symbol is a
/// variable symbol, false otherwise.
bool
elf_symbol::is_variable() const
{return get_type() == OBJECT_TYPE || get_type() == TLS_TYPE;}

/// Getter of the 'is-in-ksymtab' property.
///
/// @return true iff the current symbol is in the Linux Kernel
/// specific 'ksymtab' symbol table.
bool
elf_symbol::is_in_ksymtab() const
{return priv_->is_in_ksymtab_;}

/// Setter of the 'is-in-ksymtab' property.
///
/// @param is_in_ksymtab this is true iff the current symbol is in the
/// Linux Kernel specific 'ksymtab' symbol table.
void
elf_symbol::set_is_in_ksymtab(bool is_in_ksymtab)
{priv_->is_in_ksymtab_ = is_in_ksymtab;}

/// Getter of the 'crc' property.
///
/// @return the CRC (modversions) value for Linux Kernel symbols (if present)
uint64_t
elf_symbol::get_crc() const
{return priv_->crc_;}

/// Setter of the 'crc' property.
///
/// @param crc the new CRC (modversions) value for Linux Kernel symbols
void
elf_symbol::set_crc(uint64_t crc)
{priv_->crc_ = crc;}

/// Getter for the 'is-suppressed' property.
///
/// @return true iff the current symbol has been suppressed by a
/// suppression specification that was provided in the context that
/// led to the creation of the corpus this ELF symbol belongs to.
bool
elf_symbol::is_suppressed() const
{return priv_->is_suppressed_;}

/// Setter for the 'is-suppressed' property.
///
/// @param true iff the current symbol has been suppressed by a
/// suppression specification that was provided in the context that
/// led to the creation of the corpus this ELF symbol belongs to.
void
elf_symbol::set_is_suppressed(bool is_suppressed)
{priv_->is_suppressed_ = is_suppressed;}

/// @name Elf symbol aliases
///
/// An alias A for an elf symbol S is a symbol that is defined at the
/// same address as S.  S is chained to A through the
/// elf_symbol::get_next_alias() method.
///
/// When there are several aliases to a symbol, the main symbol is the
/// the first symbol found in the symbol table for a given address.
///
/// The alias chain is circular.  That means if S is the main symbol
/// and A is the alias, S is chained to A and A
/// is chained back to the main symbol S.  The last alias in an alias
///chain is always chained to the main symbol.
///
/// Thus, when looping over the aliases of an elf_symbol A, detecting
/// an alias that is equal to the main symbol should logically be a
/// loop exit condition.
///
/// Accessing and adding aliases for instances of elf_symbol is done
/// through the member functions below.

/// @{

/// Get the main symbol of an alias chain.
///
///@return the main symbol.
const elf_symbol_sptr
elf_symbol::get_main_symbol() const
{return priv_->main_symbol_.lock();}

/// Get the main symbol of an alias chain.
///
///@return the main symbol.
elf_symbol_sptr
elf_symbol::get_main_symbol()
{return priv_->main_symbol_.lock();}

/// Tests whether this symbol is the main symbol.
///
/// @return true iff this symbol is the main symbol.
bool
elf_symbol::is_main_symbol() const
{return get_main_symbol().get() == this;}

/// Get the next alias of the current symbol.
///
///@return the alias, or NULL if there is no alias.
elf_symbol_sptr
elf_symbol::get_next_alias() const
{return priv_->next_alias_.lock();}


/// Check if the current elf_symbol has an alias.
///
///@return true iff the current elf_symbol has an alias.
bool
elf_symbol::has_aliases() const
{return bool(get_next_alias());}

/// Get the number of aliases to this elf symbol
///
/// @return the number of aliases to this elf symbol.
int
elf_symbol::get_number_of_aliases() const
{
  int result = 0;

  for (elf_symbol_sptr a = get_next_alias();
       a && a.get() != get_main_symbol().get();
       a = a->get_next_alias())
    ++result;

  return result;
}

/// Add an alias to the current elf symbol.
///
/// @param alias the new alias.  Note that this elf_symbol should *NOT*
/// have aliases prior to the invocation of this function.
void
elf_symbol::add_alias(const elf_symbol_sptr& alias)
{
  if (!alias)
    return;

  ABG_ASSERT(!alias->has_aliases());
  ABG_ASSERT(is_main_symbol());

  if (has_aliases())
    {
      elf_symbol_sptr last_alias;
      for (elf_symbol_sptr a = get_next_alias();
	   a && !a->is_main_symbol();
	   a = a->get_next_alias())
	{
	  if (a->get_next_alias()->is_main_symbol())
	    {
	      ABG_ASSERT(last_alias == 0);
	      last_alias = a;
	    }
	}
      ABG_ASSERT(last_alias);

      last_alias->priv_->next_alias_ = alias;
    }
  else
    priv_->next_alias_ = alias;

  alias->priv_->next_alias_ = get_main_symbol();
  alias->priv_->main_symbol_ = get_main_symbol();
}

/// Update the main symbol for a group of aliased symbols
///
/// If after the construction of the symbols (in order of discovery), the
/// actual main symbol can be identified (e.g. as the symbol that actually is
/// defined in the code), this method offers a way of updating the main symbol
/// through one of the aliased symbols.
///
/// For that, locate the new main symbol by name and update all references to
/// the main symbol among the group of aliased symbols.
///
/// @param name the name of the main symbol
///
/// @return the new main elf_symbol
elf_symbol_sptr
elf_symbol::update_main_symbol(const std::string& name)
{
  ABG_ASSERT(is_main_symbol());
  if (!has_aliases() || get_name() == name)
    return get_main_symbol();

  // find the new main symbol
  elf_symbol_sptr new_main;
  // we've already checked this; check the rest of the aliases
  for (elf_symbol_sptr a = get_next_alias(); a.get() != this;
       a = a->get_next_alias())
    if (a->get_name() == name)
      {
	new_main = a;
	break;
      }

  if (!new_main)
    return get_main_symbol();

  // now update all main symbol references
  priv_->main_symbol_ = new_main;
  for (elf_symbol_sptr a = get_next_alias(); a.get() != this;
       a = a->get_next_alias())
    a->priv_->main_symbol_ = new_main;

  return new_main;
}

/// Return true if the symbol is a common one.
///
/// @return true iff the symbol is common.
bool
elf_symbol::is_common_symbol() const
{return priv_->is_common_;}

/// Return true if this common common symbol has other common instances.
///
/// A common instance of a given common symbol is another common
/// symbol with the same name.  Those exist in relocatable files.  The
/// linker normally allocates all the instances into a common block in
/// the final output file.
///
/// Note that the current object must be a common symbol, otherwise,
/// this function aborts.
///
/// @return true iff the current common symbol has other common
/// instances.
bool
elf_symbol::has_other_common_instances() const
{
  ABG_ASSERT(is_common_symbol());
  return bool(get_next_common_instance());
}

/// Get the next common instance of the current common symbol.
///
/// A common instance of a given common symbol is another common
/// symbol with the same name.  Those exist in relocatable files.  The
/// linker normally allocates all the instances into a common block in
/// the final output file.
///
/// @return the next common instance, or nil if there is not any.
elf_symbol_sptr
elf_symbol::get_next_common_instance() const
{return priv_->next_common_instance_.lock();}

/// Add a common instance to the current common elf symbol.
///
/// Note that this symbol must be the main symbol.  Being the main
/// symbol means being the first common symbol to appear in the symbol
/// table.
///
/// @param common the other common instance to add.
void
elf_symbol::add_common_instance(const elf_symbol_sptr& common)
{
  if (!common)
    return;

  ABG_ASSERT(!common->has_other_common_instances());
  ABG_ASSERT(is_common_symbol());
  ABG_ASSERT(is_main_symbol());

  if (has_other_common_instances())
    {
      elf_symbol_sptr last_common_instance;
      for (elf_symbol_sptr c = get_next_common_instance();
	   c && (c.get() != get_main_symbol().get());
	   c = c->get_next_common_instance())
	{
	  if (c->get_next_common_instance().get() == get_main_symbol().get())
	    {
	      ABG_ASSERT(last_common_instance == 0);
	      last_common_instance = c;
	    }
	}
      ABG_ASSERT(last_common_instance);

      last_common_instance->priv_->next_common_instance_ = common;
    }
  else
    priv_->next_common_instance_ = common;

  common->priv_->next_common_instance_ = get_main_symbol();
  common->priv_->main_symbol_ = get_main_symbol();
}

/// Get a string that is representative of a given elf_symbol.
///
/// If the symbol has a version, then the ID string is the
/// concatenation of the name of the symbol, the '@' character, and
/// the version of the symbol.  If the version is the default version
/// of the symbol then the '@' character is replaced by a "@@" string.
///
/// Otherwise, if the symbol does not have any version, this function
/// returns the name of the symbol.
///
/// @return a the ID string.
const string&
elf_symbol::get_id_string() const
{
  if (priv_->id_string_.empty())
    {
      string s = get_name ();

      if (!get_version().is_empty())
	{
	  if (get_version().is_default())
	    s += "@@";
	  else
	    s += "@";
	  s += get_version().str();
	}
      priv_->id_string_ = s;
    }

  return priv_->id_string_;
}

/// From the aliases of the current symbol, lookup one with a given name.
///
/// @param name the name of symbol alias we are looking for.
///
/// @return the symbol alias that has the name @p name, or nil if none
/// has been found.
elf_symbol_sptr
elf_symbol::get_alias_from_name(const string& name) const
{
  if (name == get_name())
    return elf_symbol_sptr(priv_->main_symbol_);

   for (elf_symbol_sptr a = get_next_alias();
	a && a.get() != get_main_symbol().get();
	a = a->get_next_alias())
     if (a->get_name() == name)
       return a;

   return elf_symbol_sptr();
}

/// In the list of aliases of a given elf symbol, get the alias that
/// equals this current symbol.
///
/// @param other the elf symbol to get the potential aliases from.
///
/// @return the alias of @p other that texually equals the current
/// symbol, or nil if no alias textually equals the current symbol.
elf_symbol_sptr
elf_symbol::get_alias_which_equals(const elf_symbol& other) const
{
  for (elf_symbol_sptr a = other.get_next_alias();
       a && a.get() != a->get_main_symbol().get();
       a = a->get_next_alias())
    if (textually_equals(*this, *a))
      return a;
  return elf_symbol_sptr();
}

/// Return a comma separated list of the id of the current symbol as
/// well as the id string of its aliases.
///
/// @param syms a map of all the symbols of the corpus the current
/// symbol belongs to.
///
/// @param include_symbol_itself if set to true, then the name of the
/// current symbol is included in the list of alias names that is emitted.
///
/// @return the string.
string
elf_symbol::get_aliases_id_string(const string_elf_symbols_map_type& syms,
				  bool include_symbol_itself) const
{
  string result;

  if (include_symbol_itself)
      result = get_id_string();

  vector<elf_symbol_sptr> aliases;
  compute_aliases_for_elf_symbol(*this, syms, aliases);
  if (!aliases.empty() && include_symbol_itself)
    result += ", ";

  for (vector<elf_symbol_sptr>::const_iterator i = aliases.begin();
       i != aliases.end();
       ++i)
    {
      if (i != aliases.begin())
	result += ", ";
      result += (*i)->get_id_string();
    }
  return result;
}

/// Return a comma separated list of the id of the current symbol as
/// well as the id string of its aliases.
///
/// @param include_symbol_itself if set to true, then the name of the
/// current symbol is included in the list of alias names that is emitted.
///
/// @return the string.
string
elf_symbol::get_aliases_id_string(bool include_symbol_itself) const
{
  vector<elf_symbol_sptr> aliases;
  if (include_symbol_itself)
    aliases.push_back(get_main_symbol());

  for (elf_symbol_sptr a = get_next_alias();
       a && a.get() != get_main_symbol().get();
       a = a->get_next_alias())
    aliases.push_back(a);

  string result;
  for (vector<elf_symbol_sptr>::const_iterator i = aliases.begin();
       i != aliases.end();
       ++i)
    {
      if (i != aliases.begin())
	result += ", ";
      result += (*i)->get_id_string();
    }

  return result;
}

/// Given the ID of a symbol, get the name and the version of said
/// symbol.
///
/// @param id the symbol ID to consider.
///
/// @param name the symbol name extracted from the ID.  This is set
/// only if the function returned true.
///
/// @param ver the symbol version extracted from the ID.
bool
elf_symbol::get_name_and_version_from_id(const string&	id,
					 string&	name,
					 string&	ver)
{
  name.clear(), ver.clear();

  string::size_type i = id.find('@');
  if (i == string::npos)
    {
      name = id;
      return true;
    }

  name = id.substr(0, i);
  ++i;

  if (i >= id.size())
    return true;

  string::size_type j = id.find('@', i);
  if (j == string::npos)
    j = i;
  else
    ++j;

  if (j >= id.size())
    {
      ver = "";
      return true;
    }

  ver = id.substr(j);
  return true;
}

///@}

/// Test if two main symbols are textually equal, or, if they have
/// aliases that are textually equal.
///
/// @param other the symbol to compare against.
///
/// @return true iff the current instance of elf symbol equals the @p
/// other.
bool
elf_symbol::operator==(const elf_symbol& other) const
{
  bool are_equal = textually_equals(*this, other);
  if (!are_equal)
    are_equal = bool(get_alias_which_equals(other));
  return are_equal;
}

/// Test if the current symbol aliases another one.
///
/// @param o the other symbol to test against.
///
/// @return true iff the current symbol aliases @p o.
bool
elf_symbol::does_alias(const elf_symbol& o) const
{
  if (*this == o)
    return true;

  if (get_main_symbol() == o.get_main_symbol())
    return true;

  for (elf_symbol_sptr a = get_next_alias();
       a && !a->is_main_symbol();
       a = a->get_next_alias())
    {
      if (o == *a)
	return true;
    }
  return false;
}

/// Equality operator for smart pointers to elf_symbol.
///
/// @param lhs the first elf symbol to consider.
///
/// @param rhs the second elf symbol to consider.
///
/// @return true iff @p lhs equals @p rhs.
bool
operator==(const elf_symbol_sptr& lhs, const elf_symbol_sptr& rhs)
{
  if (!!lhs != !!rhs)
    return false;

  if (!lhs)
    return true;

  return *lhs == *rhs;
}

/// Inequality operator for smart pointers to elf_symbol.
///
/// @param lhs the first elf symbol to consider.
///
/// @param rhs the second elf symbol to consider.
///
/// @return true iff @p lhs is different from @p rhs.
bool
operator!=(const elf_symbol_sptr& lhs, const elf_symbol_sptr& rhs)
{return !operator==(lhs, rhs);}

/// Test if two symbols alias.
///
/// @param s1 the first symbol to consider.
///
/// @param s2 the second symbol to consider.
///
/// @return true if @p s1 aliases @p s2.
bool
elf_symbols_alias(const elf_symbol& s1, const elf_symbol& s2)
{return s1.does_alias(s2) || s2.does_alias(s1);}

void
compute_aliases_for_elf_symbol(const elf_symbol& sym,
			       const string_elf_symbols_map_type& symtab,
			       vector<elf_symbol_sptr>& aliases)
{

  if (elf_symbol_sptr a = sym.get_next_alias())
    for (; a && !a->is_main_symbol(); a = a->get_next_alias())
      aliases.push_back(a);
  else
    for (string_elf_symbols_map_type::const_iterator i = symtab.begin();
	 i != symtab.end();
	 ++i)
      for (elf_symbols::const_iterator j = i->second.begin();
	   j != i->second.end();
	   ++j)
	{
	  if (**j == sym)
	    for (elf_symbol_sptr s = (*j)->get_next_alias();
		 s && !s->is_main_symbol();
		 s = s->get_next_alias())
	      aliases.push_back(s);
	  else
	    for (elf_symbol_sptr s = (*j)->get_next_alias();
		 s && !s->is_main_symbol();
		 s = s->get_next_alias())
	      if (*s == sym)
		aliases.push_back(*j);
	}
}

/// Test if two symbols alias.
///
/// @param s1 the first symbol to consider.
///
/// @param s2 the second symbol to consider.
///
/// @return true if @p s1 aliases @p s2.
bool
elf_symbols_alias(const elf_symbol* s1, const elf_symbol* s2)
{
  if (!!s1 != !!s2)
    return false;
  if (s1 == s2)
    return true;
  return elf_symbols_alias(*s1, *s2);
}

/// Test if two symbols alias.
///
/// @param s1 the first symbol to consider.
///
/// @param s2 the second symbol to consider.
///
/// @return true if @p s1 aliases @p s2.
bool
elf_symbols_alias(const elf_symbol_sptr s1, const elf_symbol_sptr s2)
{return elf_symbols_alias(s1.get(), s2.get());}

/// Serialize an instance of @ref symbol_type and stream it to a given
/// output stream.
///
/// @param o the output stream to serialize the symbole type to.
///
/// @param t the symbol type to serialize.
std::ostream&
operator<<(std::ostream& o, elf_symbol::type t)
{
  string repr;

  switch (t)
    {
    case elf_symbol::NOTYPE_TYPE:
      repr = "unspecified symbol type";
      break;
    case elf_symbol::OBJECT_TYPE:
      repr = "variable symbol type";
      break;
    case elf_symbol::FUNC_TYPE:
      repr = "function symbol type";
      break;
    case elf_symbol::SECTION_TYPE:
      repr = "section symbol type";
      break;
    case elf_symbol::FILE_TYPE:
      repr = "file symbol type";
      break;
    case elf_symbol::COMMON_TYPE:
      repr = "common data object symbol type";
      break;
    case elf_symbol::TLS_TYPE:
      repr = "thread local data object symbol type";
      break;
    case elf_symbol::GNU_IFUNC_TYPE:
      repr = "indirect function symbol type";
      break;
    default:
      {
	std::ostringstream s;
	s << "unknown symbol type (" << (char)t << ')';
	repr = s.str();
      }
      break;
    }

  o << repr;
  return o;
}

/// Serialize an instance of @ref symbol_binding and stream it to a
/// given output stream.
///
/// @param o the output stream to serialize the symbole type to.
///
/// @param b the symbol binding to serialize.
std::ostream&
operator<<(std::ostream& o, elf_symbol::binding b)
{
  string repr;

  switch (b)
    {
    case elf_symbol::LOCAL_BINDING:
      repr = "local binding";
      break;
    case elf_symbol::GLOBAL_BINDING:
      repr = "global binding";
      break;
    case elf_symbol::WEAK_BINDING:
      repr = "weak binding";
      break;
    case elf_symbol::GNU_UNIQUE_BINDING:
      repr = "GNU unique binding";
      break;
    default:
      {
	std::ostringstream s;
	s << "unknown binding (" << (unsigned char) b << ")";
	repr = s.str();
      }
      break;
    }

  o << repr;
  return o;
}

/// Serialize an instance of @ref elf_symbol::visibility and stream it
/// to a given output stream.
///
/// @param o the output stream to serialize the symbole type to.
///
/// @param v the symbol visibility to serialize.
std::ostream&
operator<<(std::ostream& o, elf_symbol::visibility v)
{
  string repr;

  switch (v)
    {
    case elf_symbol::DEFAULT_VISIBILITY:
      repr = "default visibility";
      break;
    case elf_symbol::PROTECTED_VISIBILITY:
      repr = "protected visibility";
      break;
    case elf_symbol::HIDDEN_VISIBILITY:
      repr = "hidden visibility";
      break;
    case elf_symbol::INTERNAL_VISIBILITY:
      repr = "internal visibility";
      break;
    default:
      {
	std::ostringstream s;
	s << "unknown visibility (" << (unsigned char) v << ")";
	repr = s.str();
      }
      break;
    }

  o << repr;
  return o;
}

/// Convert a string representing a symbol type into an
/// elf_symbol::type.
///
///@param s the string to convert.
///
///@param t the resulting elf_symbol::type.
///
/// @return true iff the conversion completed successfully.
bool
string_to_elf_symbol_type(const string& s, elf_symbol::type& t)
{
  if (s == "no-type")
    t = elf_symbol::NOTYPE_TYPE;
  else if (s == "object-type")
    t = elf_symbol::OBJECT_TYPE;
  else if (s == "func-type")
    t = elf_symbol::FUNC_TYPE;
  else if (s == "section-type")
    t = elf_symbol::SECTION_TYPE;
  else if (s == "file-type")
    t = elf_symbol::FILE_TYPE;
  else if (s == "common-type")
    t = elf_symbol::COMMON_TYPE;
  else if (s == "tls-type")
    t = elf_symbol::TLS_TYPE;
  else if (s == "gnu-ifunc-type")
    t = elf_symbol::GNU_IFUNC_TYPE;
  else
    return false;

  return true;
}

/// Convert a string representing a an elf symbol binding into an
/// elf_symbol::binding.
///
/// @param s the string to convert.
///
/// @param b the resulting elf_symbol::binding.
///
/// @return true iff the conversion completed successfully.
bool
string_to_elf_symbol_binding(const string& s, elf_symbol::binding& b)
{
    if (s == "local-binding")
      b = elf_symbol::LOCAL_BINDING;
    else if (s == "global-binding")
      b = elf_symbol::GLOBAL_BINDING;
    else if (s == "weak-binding")
      b = elf_symbol::WEAK_BINDING;
    else if (s == "gnu-unique-binding")
      b = elf_symbol::GNU_UNIQUE_BINDING;
    else
      return false;

    return true;
}

/// Convert a string representing a an elf symbol visibility into an
/// elf_symbol::visibility.
///
/// @param s the string to convert.
///
/// @param b the resulting elf_symbol::visibility.
///
/// @return true iff the conversion completed successfully.
bool
string_to_elf_symbol_visibility(const string& s, elf_symbol::visibility& v)
{
  if (s == "default-visibility")
    v = elf_symbol::DEFAULT_VISIBILITY;
  else if (s == "protected-visibility")
    v = elf_symbol::PROTECTED_VISIBILITY;
  else if (s == "hidden-visibility")
    v = elf_symbol::HIDDEN_VISIBILITY;
  else if (s == "internal-visibility")
    v = elf_symbol::INTERNAL_VISIBILITY;
  else
    return false;

  return true;
}

/// Test if the type of an ELF symbol denotes a function symbol.
///
/// @param t the type of the ELF symbol.
///
/// @return true iff elf symbol type @p t denotes a function symbol
/// type.
bool
elf_symbol_is_function(elf_symbol::type t)
{return t == elf_symbol::FUNC_TYPE;}

/// Test if the type of an ELF symbol denotes a function symbol.
///
/// @param t the type of the ELF symbol.
///
/// @return true iff elf symbol type @p t denotes a function symbol
/// type.
bool
elf_symbol_is_variable(elf_symbol::type t)
{return t == elf_symbol::OBJECT_TYPE;}

// <elf_symbol::version stuff>

struct elf_symbol::version::priv
{
  string	version_;
  bool		is_default_;

  priv()
    : is_default_(false)
  {}

  priv(const string& v,
       bool d)
    : version_(v),
      is_default_(d)
  {}
}; // end struct elf_symbol::version::priv

elf_symbol::version::version()
  : priv_(new priv)
{}

/// @param v the name of the version.
///
/// @param is_default true if this is a default version.
elf_symbol::version::version(const string& v,
			     bool is_default)
  : priv_(new priv(v, is_default))
{}

elf_symbol::version::version(const elf_symbol::version& v)
  : priv_(new priv(v.str(), v.is_default()))
{
}

/// Cast the version_type into a string that is its name.
///
/// @return the name of the version.
elf_symbol::version::operator const string&() const
{return priv_->version_;}

/// Getter for the version name.
///
/// @return the version name.
const string&
elf_symbol::version::str() const
{return priv_->version_;}

/// Setter for the version name.
///
/// @param s the version name.
void
elf_symbol::version::str(const string& s)
{priv_->version_ = s;}

/// Getter for the 'is_default' property of the version.
///
/// @return true iff this is a default version.
bool
elf_symbol::version::is_default() const
{return priv_->is_default_;}

/// Setter for the 'is_default' property of the version.
///
/// @param f true if this is the default version.
void
elf_symbol::version::is_default(bool f)
{priv_->is_default_ = f;}

bool
elf_symbol::version::is_empty() const
{return str().empty();}

/// Compares the current version against another one.
///
/// @param o the other version to compare the current one to.
///
/// @return true iff the current version equals @p o.
bool
elf_symbol::version::operator==(const elf_symbol::version& o) const
{return str() == o.str();}

/// Inequality operator.
///
/// @param o the version to compare against the current one.
///
/// @return true iff both versions are different.
bool
elf_symbol::version::operator!=(const version& o) const
{return !operator==(o);}

/// Assign a version to the current one.
///
/// @param o the other version to assign to this one.
///
/// @return a reference to the assigned version.
elf_symbol::version&
elf_symbol::version::operator=(const elf_symbol::version& o)
{
  str(o.str());
  is_default(o.is_default());
  return *this;
}

// </elf_symbol::version stuff>

// </elf_symbol stuff>

// <class dm_context_rel stuff>
struct dm_context_rel::priv
{
  bool is_laid_out_;
  size_t offset_in_bits_;
  var_decl* anonymous_data_member_;

  priv(bool is_static = false)
    : is_laid_out_(!is_static),
      offset_in_bits_(0),
      anonymous_data_member_()
  {}

  priv(bool is_laid_out, size_t offset_in_bits)
    : is_laid_out_(is_laid_out),
      offset_in_bits_(offset_in_bits),
      anonymous_data_member_()
  {}
}; //end struct dm_context_rel::priv

dm_context_rel::dm_context_rel()
  : context_rel(),
    priv_(new priv)
{}

dm_context_rel::dm_context_rel(scope_decl* s,
			       bool is_laid_out,
			       size_t offset_in_bits,
			       access_specifier a,
			       bool is_static)
  : context_rel(s, a, is_static),
    priv_(new priv(is_laid_out, offset_in_bits))
{}

dm_context_rel::dm_context_rel(scope_decl* s)
  : context_rel(s),
    priv_(new priv())
{}

bool
dm_context_rel::get_is_laid_out() const
{return priv_->is_laid_out_;}

void
dm_context_rel::set_is_laid_out(bool f)
{priv_->is_laid_out_ = f;}

size_t
dm_context_rel::get_offset_in_bits() const
{return priv_->offset_in_bits_;}

void
dm_context_rel::set_offset_in_bits(size_t o)
{priv_->offset_in_bits_ = o;}

bool
dm_context_rel::operator==(const dm_context_rel& o) const
{
  if (!context_rel::operator==(o))
    return false;

  return (priv_->is_laid_out_ == o.priv_->is_laid_out_
	    && priv_->offset_in_bits_ == o.priv_->offset_in_bits_);
}

bool
dm_context_rel::operator!=(const dm_context_rel& o) const
{return !operator==(o);}

/// Return a non-nil value if this data member context relationship
/// has an anonymous data member.  That means, if the data member this
/// relation belongs to is part of an anonymous data member.
///
/// @return the containing anonymous data member of this data member
/// relationship.  Nil if there is none.
const var_decl*
dm_context_rel::get_anonymous_data_member() const
{return priv_->anonymous_data_member_;}

/// Set the containing anonymous data member of this data member
/// context relationship. That means that the data member this
/// relation belongs to is part of an anonymous data member.
///
/// @param anon_dm the containing anonymous data member of this data
/// member relationship.  Nil if there is none.
void
dm_context_rel::set_anonymous_data_member(var_decl* anon_dm)
{priv_->anonymous_data_member_ = anon_dm;}

dm_context_rel::~dm_context_rel()
{}
// </class dm_context_rel stuff>

// <environment stuff>

/// Convenience typedef for a map of interned_string -> bool.
typedef unordered_map<interned_string,
		      bool, hash_interned_string> interned_string_bool_map_type;

/// The private data of the @ref environment type.
struct environment::priv
{
  config			 config_;
  canonical_types_map_type	 canonical_types_;
  mutable vector<type_base_sptr> sorted_canonical_types_;
  type_base_sptr		 void_type_;
  type_base_sptr		 variadic_marker_type_;
  unordered_set<const class_or_union*>	classes_being_compared_;
  unordered_set<const function_type*>	fn_types_being_compared_;
  vector<type_base_sptr>	 extra_live_types_;
  interned_string_pool		 string_pool_;
  bool				 canonicalization_is_done_;
  bool				 do_on_the_fly_canonicalization_;
  bool				 decl_only_class_equals_definition_;

  priv()
    : canonicalization_is_done_(),
      do_on_the_fly_canonicalization_(true),
      decl_only_class_equals_definition_(false)
  {}
};// end struct environment::priv

/// Default constructor of the @ref environment type.
environment::environment()
  :priv_(new priv)
{}

/// Destructor for the @ref environment type.
environment::~environment()
{}

/// Getter the map of canonical types.
///
/// @return the map of canonical types.  The key of the map is the
/// hash of the canonical type and its value if the canonical type.
environment::canonical_types_map_type&
environment::get_canonical_types_map()
{return priv_->canonical_types_;}

/// Getter the map of canonical types.
///
/// @return the map of canonical types.  The key of the map is the
/// hash of the canonical type and its value if the canonical type.
const environment::canonical_types_map_type&
environment::get_canonical_types_map() const
{return const_cast<environment*>(this)->get_canonical_types_map();}

/// Helper to detect if a type is either a reference, a pointer, or a
/// qualified type.
static bool
is_ptr_ref_or_qual_type(const type_base *t)
{
  if (is_pointer_type(t)
      || is_reference_type(t)
      || is_qualified_type(t))
    return true;
  return false;
}

/// A functor to sort decls somewhat topologically.  That is, types
/// are sorted in a way that makes the ones that are defined "first"
/// to come first.
///
/// The topological criteria is a lexicographic sort of the definition
/// location of the type.  For types that have no location (or the
/// same location), it's their qualified name that is used for the
/// lexicographic sort.
struct decl_topo_comp
{

  /// The "Less Than" comparison operator of this functor.
  ///
  /// @param f the first decl to be considered for the comparison.
  ///
  /// @param s the second decl to be considered for the comparison.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const decl_base *f,
	     const decl_base *s)
  {
    if (!!f != !!s)
      return f && !s;

    if (!f)
      return false;

    location fl = f->get_location();
    location sl = s->get_location();
    if (fl.get_value() != sl.get_value())
      return fl.get_value() < sl.get_value();

    // We reach this point if location data is useless.
    return (get_pretty_representation(f, true)
	    < get_pretty_representation(s, true));
  }

  /// The "Less Than" comparison operator of this functor.
  ///
  /// @param f the first decl to be considered for the comparison.
  ///
  /// @param s the second decl to be considered for the comparison.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const decl_base_sptr &f,
	     const decl_base_sptr &s)
  {return operator()(f.get(), s.get());}

}; // end struct decl_topo_comp

/// A functor to sort types somewhat topologically.  That is, types
/// are sorted in a way that makes the ones that are defined "first"
/// to come first.
///
/// The topological criteria is a lexicographic sort of the definition
/// location of the type.  For types that have no location, it's their
/// qualified name that is used for the lexicographic sort.
struct type_topo_comp
{
  /// The "Less Than" comparison operator of this functor.
  ///
  /// @param f the first type to be considered for the comparison.
  ///
  /// @param s the second type to be considered for the comparison.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const type_base_sptr &f,
	     const type_base_sptr &s)
  {return operator()(f.get(), s.get());}

  /// The "Less Than" comparison operator of this functor.
  ///
  /// @param f the first type to be considered for the comparison.
  ///
  /// @param s the second type to be considered for the comparison.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const type_base *f,
	     const type_base *s)
  {
    bool f_is_ptr_ref_or_qual = is_ptr_ref_or_qual_type(f);
    bool s_is_ptr_ref_or_qual = is_ptr_ref_or_qual_type(s);

    if (f_is_ptr_ref_or_qual != s_is_ptr_ref_or_qual)
      return !f_is_ptr_ref_or_qual && s_is_ptr_ref_or_qual;

    if (f_is_ptr_ref_or_qual && s_is_ptr_ref_or_qual)
      {
	string s1 = get_pretty_representation(f, true);
	string s2 = get_pretty_representation(s, true);
	if (s1 == s2)
	  if (qualified_type_def * q = is_qualified_type(f))
	    if (q->get_cv_quals() == qualified_type_def::CV_NONE)
	      if (!is_qualified_type(s))
		// We are looking at two types that are the result of
		// an optimization that happens during the IR
		// construction.  Namely, type f is a cv-qualified
		// type with no qualifier (no const, no volatile, no
		// nothing, we call it an empty-qualified type).
		// These are the result of an optimization which
		// removes "redundant qualifiers" from some types.
		// For instance, consider a "const reference".  The
		// const there is redundant because a reference is
		// always const.  So as a result of the optimizaton
		// that type is going to be transformed into an
		// empty-qualified reference. If we don't make that
		// optimization, then we risk having spurious change
		// reports down the road.  But then, as a consequence
		// of that optimization, we need to sort the
		// empty-qualified type and its non-qualified variant
		// e.g, to ensure stability in the abixml output; both
		// types are logically equal, but here, we decide that
		// the empty-qualified one is topologically "less
		// than" the non-qualified counterpart.
		//
		// So here, type f is an empty-qualified type and type
		// s is its non-qualified variant.  We decide that f
		// is topologically less than s.
		return true;
	return (s1 < s2);
      }

    decl_base *fd = is_decl(f);
    decl_base *sd = is_decl(s);

    if (!!fd != !!sd)
      return fd && !sd;

    if (!fd)
      {
	type_base *peeled_f = peel_pointer_or_reference_type(f);
	type_base *peeled_s = peel_pointer_or_reference_type(s);

	fd = is_decl(peeled_f);
	sd = is_decl(peeled_s);

	if (!!fd != !!sd)
	  return fd && !sd;

	if (!fd)
	  return (get_pretty_representation(f, true)
		  < get_pretty_representation(s, true));
      }

    // From this point, fd and sd should be non-nil
    decl_topo_comp decl_comp;
    return decl_comp(fd, sd);
  }
}; //end struct type_topo_comp

/// Get a @ref type_decl that represents a "void" type for the current
/// environment.
///
/// @return the @ref type_decl that represents a "void" type.
const type_base_sptr&
environment::get_void_type() const
{
  if (!priv_->void_type_)
    priv_->void_type_.reset(new type_decl(const_cast<environment*>(this),
					  intern("void"),
					  0, 0, location()));
  return priv_->void_type_;
}

/// Get a @ref type_decl instance that represents a the type of a
/// variadic function parameter.
///
/// @return the Get a @ref type_decl instance that represents a the
/// type of a variadic function parameter.
const type_base_sptr&
environment::get_variadic_parameter_type() const
{
  if (!priv_->variadic_marker_type_)
    priv_->variadic_marker_type_.
      reset(new type_decl(const_cast<environment*>(this),
			  intern("variadic parameter type"),
			  0, 0, location()));
  return priv_->variadic_marker_type_;
}

/// Test if the canonicalization of types created out of the current
/// environment is done.
///
/// @return true iff the canonicalization of types created out of the current
/// environment is done.
bool
environment::canonicalization_is_done() const
{return priv_->canonicalization_is_done_;}

/// Set a flag saying if the canonicalization of types created out of
/// the current environment is done or not.
///
/// Note that this function must only be called by internal code of
/// the library that creates ABI artifacts (e.g, read an abi corpus
/// from elf or from our own xml format and creates representations of
/// types out of it) and thus needs to canonicalize types to speed-up
/// further type comparison.
///
/// @param f the new value of the flag.
void
environment::canonicalization_is_done(bool f)
{priv_->canonicalization_is_done_ = f;}

/// Getter for the "on-the-fly-canonicalization" flag.
///
/// @return true iff @ref OnTheFlyCanonicalization
/// "on-the-fly-canonicalization" is to be performed during
/// comparison.
bool
environment::do_on_the_fly_canonicalization() const
{return priv_->do_on_the_fly_canonicalization_;}

/// Setter for the "on-the-fly-canonicalization" flag.
///
/// @param f If this is true then @ref OnTheFlyCanonicalization
/// "on-the-fly-canonicalization" is to be performed during
/// comparison.
void
environment::do_on_the_fly_canonicalization(bool f)
{priv_->do_on_the_fly_canonicalization_ = f;}

/// Getter of the "decl-only-class-equals-definition" flag.
///
/// Usually, a declaration-only class named 'struct foo' compares
/// equal to any class definition named "struct foo'.  This is at
/// least true for C++.
///
/// In C, though, because there can be multiple definitions of 'struct
/// foo' in the binary, a declaration-only "struct foo" might be
/// considered to *NOT* resolve to any of the struct foo defined.  In
/// that case, the declaration-only "struct foo" is considered
/// different from the definitions.
///
/// This flag controls the behaviour of the comparison of an
/// unresolved decl-only class against a definition of the same name.
///
/// If set to false, the the declaration equals the definition.  If
/// set to false, then the decalration is considered different from
/// the declaration.
///
/// @return the value of the "decl-only-class-equals-definition" flag.
bool
environment::decl_only_class_equals_definition() const
{return priv_->decl_only_class_equals_definition_;}

/// Setter of the "decl-only-class-equals-definition" flag.
///
/// Usually, a declaration-only class named 'struct foo' compares
/// equal to any class definition named "struct foo'.  This is at
/// least true for C++.
///
/// In C, though, because there can be multiple definitions of 'struct
/// foo' in the binary, a declaration-only "struct foo" might be
/// considered to *NOT* resolve to any of the struct foo defined.  In
/// that case, the declaration-only "struct foo" is considered
/// different from the definitions.
///
/// This flag controls the behaviour of the comparison of an
/// unresolved decl-only class against a definition of the same name.
///
/// If set to false, the the declaration equals the definition.  If
/// set to false, then the decalration is considered different from
/// the declaration.
///
/// @param the new value of the "decl-only-class-equals-definition"
/// flag.
void
environment::decl_only_class_equals_definition(bool f) const
{priv_->decl_only_class_equals_definition_ = f;}

/// Test if a given type is a void type as defined in the current
/// environment.
///
/// @param t the type to consider.
///
/// @return true iff @p t is a void type as defined in the current
/// environment.
bool
environment::is_void_type(const type_base_sptr& t) const
{
  if (!t)
    return false;
  return t.get() == get_void_type().get();
}

/// Test if a given type is a void type as defined in the current
/// environment.
///
/// @param t the type to consider.
///
/// @return true iff @p t is a void type as defined in the current
/// environment.
bool
environment::is_void_type(const type_base* t) const
{
  if (!t)
    return false;
  return t == get_void_type().get();
}

/// Test if a type is a variadic parameter type as defined in the
/// current environment.
///
/// @param t the type to consider.
///
/// @return true iff @p t is a variadic parameter type as defined in
/// the current environment.
bool
environment::is_variadic_parameter_type(const type_base* t) const
{
  if (!t)
    return false;
  return t == get_variadic_parameter_type().get();
}

/// Test if a type is a variadic parameter type as defined in the
/// current environment.
///
/// @param t the type to consider.
///
/// @return true iff @p t is a variadic parameter type as defined in
/// the current environment.
bool
environment::is_variadic_parameter_type(const type_base_sptr& t) const
{return is_variadic_parameter_type(t.get());}

/// Do intern a string.
///
/// If a value of this string already exists in the interned string
/// pool of the current environment, then this function returns a new
/// interned_string pointing to that already existing string.
/// Otherwise, a new string is created, stored in the interned string
/// pool and a new interned_string instance is created to point to
/// that new intrerned string, and it's return.
///
/// @param s the value of the string to intern.
///
/// @return the interned string.
interned_string
environment::intern(const string& s) const
{return const_cast<environment*>(this)->priv_->string_pool_.create_string(s);}

/// Getter of the general configuration object.
///
/// @return the configuration object.
const config&
environment::get_config() const
{return priv_->config_;}

// </environment stuff>

// <type_or_decl_base stuff>

/// The private data of @ref type_or_decl_base.
struct type_or_decl_base::priv
{
  // This holds the kind of dynamic type of particular instance.
  // Yes, this is part of the implementation of a "poor man" runtime
  // type identification.  We are doing this because profiling shows
  // that using dynamic_cast in some places is really to slow and is
  // constituting a hotspot.  This poor man's implementation made
  // things be much faster.
  enum type_or_decl_kind	kind_;
  // This holds the runtime type instance pointer of particular
  // instance.  In other words, this is the "this pointer" of the
  // dynamic type of a particular instance.
  void*			rtti_;
  // This holds a pointer to either the type_base sub-object (if the
  // current instance is a type) or the decl_base sub-object (if the
  // current instance is a decl).  This is used by the is_decl() and
  // is_type() functions, which also show up during profiling as
  // hotspots, due to their use of dynamic_cast.
  void*			type_or_decl_ptr_;
  bool				hashing_started_;
  const environment*		env_;
  translation_unit*		translation_unit_;

  /// Constructor of the type_or_decl_base::priv private type.
  ///
  /// @param e the environment in which the ABI artifact was created.
  ///
  /// @param k the identifier of the runtime type of the current
  /// instance of ABI artifact.
  priv(const environment* e = 0,
       enum type_or_decl_kind k = ABSTRACT_TYPE_OR_DECL)
    : kind_(k),
      rtti_(),
      type_or_decl_ptr_(),
      hashing_started_(),
      env_(e),
      translation_unit_()
  {}

  enum type_or_decl_kind
  kind() const
  {return kind_;}

  void
  kind (enum type_or_decl_kind k)
  {kind_ |= k;}
}; // end struct type_or_decl_base::priv

/// bitwise "OR" operator for the type_or_decl_base::type_or_decl_kind
/// bitmap type.
type_or_decl_base::type_or_decl_kind
operator|(type_or_decl_base::type_or_decl_kind l,
	  type_or_decl_base::type_or_decl_kind r)
{
  return static_cast<type_or_decl_base::type_or_decl_kind>
    (static_cast<unsigned>(l) | static_cast<unsigned>(r));
}

/// bitwise "|=" operator for the type_or_decl_base::type_or_decl_kind
/// bitmap type.
type_or_decl_base::type_or_decl_kind&
operator|=(type_or_decl_base::type_or_decl_kind& l,
	   type_or_decl_base::type_or_decl_kind r)
{
  l = l | r;
  return l;
}

/// bitwise "AND" operator for the
/// type_or_decl_base::type_or_decl_kind bitmap type.
type_or_decl_base::type_or_decl_kind
operator&(type_or_decl_base::type_or_decl_kind l,
	  type_or_decl_base::type_or_decl_kind r)
{
  return static_cast<type_or_decl_base::type_or_decl_kind>
    (static_cast<unsigned>(l) & static_cast<unsigned>(r));
}

/// bitwise "A&=" operator for the
/// type_or_decl_base::type_or_decl_kind bitmap type.
type_or_decl_base::type_or_decl_kind&
operator&=(type_or_decl_base::type_or_decl_kind& l,
	  type_or_decl_base::type_or_decl_kind r)
{
  l = l & r;
  return l;
}

/// Default constructor of @ref type_or_decl_base.
type_or_decl_base::type_or_decl_base()
  :priv_(new priv)
{}

/// Constructor of @ref type_or_decl_base.
///
/// @param the environment the current ABI artifact is constructed
/// from.
///
/// @param k the runtime identifier bitmap of the type being built.
type_or_decl_base::type_or_decl_base(const environment* e,
				     enum type_or_decl_kind k)
  :priv_(new priv(e, k))
{}

/// Copy constructor of @ref type_or_decl_base.
type_or_decl_base::type_or_decl_base(const type_or_decl_base& o)
{*priv_ = *o.priv_;}

/// The destructor of the @ref type_or_decl_base type.
type_or_decl_base::~type_or_decl_base()
{}

/// Getter for the "kind" property of @ref type_or_decl_base type.
///
/// This property holds the identifier bitmap of the runtime type of
/// an ABI artifact.
///
/// @return the runtime type identifier bitmap of the current ABI
/// artifact.
enum type_or_decl_base::type_or_decl_kind
type_or_decl_base::kind() const
{return priv_->kind();}

/// Setter for the "kind" property of @ref type_or_decl_base type.
///
/// This property holds the identifier bitmap of the runtime type of
/// an ABI artifact.
///
/// @param the runtime type identifier bitmap of the current ABI
/// artifact.
void
type_or_decl_base::kind(enum type_or_decl_kind k)
{priv_->kind(k);}

/// Getter of the pointer to the runtime type sub-object of the
/// current instance.
///
/// @return the pointer to the runtime type sub-object of the current
/// instance.
const void*
type_or_decl_base::runtime_type_instance() const
{return priv_->rtti_;}

/// Getter of the pointer to the runtime type sub-object of the
/// current instance.
///
/// @return the pointer to the runtime type sub-object of the current
/// instance.
void*
type_or_decl_base::runtime_type_instance()
{return priv_->rtti_;}

/// Setter of the pointer to the runtime type sub-object of the
/// current instance.
///
/// @param i the new pointer to the runtime type sub-object of the
/// current instance.
void
type_or_decl_base::runtime_type_instance(void* i)
{
  priv_->rtti_ = i;
  if (type_base* t = dynamic_cast<type_base*>(this))
    priv_->type_or_decl_ptr_ = t;
  else if (decl_base *d = dynamic_cast<decl_base*>(this))
    priv_->type_or_decl_ptr_ = d;
}

/// Getter of the pointer to either the type_base sub-object of the
/// current instance if it's a type, or to the decl_base sub-object of
/// the current instance if it's a decl.
///
/// @return the pointer to either the type_base sub-object of the
/// current instance if it's a type, or to the decl_base sub-object of
/// the current instance if it's a decl.
const void*
type_or_decl_base::type_or_decl_base_pointer() const
{return const_cast<type_or_decl_base*>(this)->type_or_decl_base_pointer();}

/// Getter of the pointer to either the type_base sub-object of the
/// current instance if it's a type, or to the decl_base sub-object of
/// the current instance if it's a decl.
///
/// @return the pointer to either the type_base sub-object of the
/// current instance if it's a type, or to the decl_base sub-object of
/// the current instance if it's a decl.
void*
type_or_decl_base::type_or_decl_base_pointer()
{return priv_->type_or_decl_ptr_;}

/// Getter for the 'hashing_started' property.
///
/// @return the 'hashing_started' property.
bool
type_or_decl_base::hashing_started() const
{return priv_->hashing_started_;}

/// Setter for the 'hashing_started' property.
///
/// @param b the value to set the 'hashing_property' to.
void
type_or_decl_base::hashing_started(bool b) const
{priv_->hashing_started_ = b;}

/// Setter of the environment of the current ABI artifact.
///
/// This just sets the environment artifact of the current ABI
/// artifact, not on its sub-trees.  If you want to set the
/// environment of an ABI artifact including its sub-tree, use the
/// abigail::ir::set_environment_for_artifact() function.
///
/// @param env the new environment.
void
type_or_decl_base::set_environment(const environment* env)
{priv_->env_ = env;}

/// Getter of the environment of the current ABI artifact.
///
/// @return the environment of the artifact.
const environment*
type_or_decl_base::get_environment() const
{return priv_->env_;}

/// Getter of the environment of the current ABI artifact.
///
/// @return the environment of the artifact.
environment*
type_or_decl_base::get_environment()
{return const_cast<environment*>(priv_->env_);}

/// Get the @ref corpus this ABI artifact belongs to.
///
/// @return the corpus this ABI artifact belongs to, or nil if it
/// belongs to none for now.
corpus*
type_or_decl_base::get_corpus()
{
  translation_unit* tu = get_translation_unit();
  if (!tu)
    return 0;
  return tu->get_corpus();
}


/// Get the @ref corpus this ABI artifact belongs to.
///
/// @return the corpus this ABI artifact belongs to, or nil if it
/// belongs to none for now.
const corpus*
type_or_decl_base::get_corpus() const
{return const_cast<type_or_decl_base*>(this)->get_corpus();}

/// Set the @ref translation_unit this ABI artifact belongs to.
///
/// Note that adding an ABI artifact to a containining on should
/// invoke this member function.
void
type_or_decl_base::set_translation_unit(translation_unit* tu)
{priv_->translation_unit_ = tu;}


/// Get the @ref translation_unit this ABI artifact belongs to.
///
/// @return the translation unit this ABI artifact belongs to, or nil
/// if belongs to none for now.
translation_unit*
type_or_decl_base::get_translation_unit()
{return priv_->translation_unit_;}

/// Get the @ref translation_unit this ABI artifact belongs to.
///
/// @return the translation unit this ABI artifact belongs to, or nil
/// if belongs to none for now.
const translation_unit*
type_or_decl_base::get_translation_unit() const
{return const_cast<type_or_decl_base*>(this)->get_translation_unit();}

/// Assignment operator for @ref type_or_decl_base.
///
/// @param o the other instance to assign the current instance to.
///
/// return a reference to the assigned instance of @ref
/// type_or_decl_base.
type_or_decl_base&
type_or_decl_base::operator=(const type_or_decl_base& o)
{
  *priv_ = *o.priv_;
  return *this;
}

/// Traverse the the ABI artifact.
///
/// @param v the visitor used to traverse the sub-tree nodes of the
/// artifact.
bool
type_or_decl_base::traverse(ir_node_visitor&)
{return true;}

/// Set the environment of a given ABI artifact, including recursively
/// setting the environment on the sub-trees of the artifact.
///
/// @param artifact the artifact to set the environment for.
///
/// @param env the new environment.
void
set_environment_for_artifact(type_or_decl_base* artifact,
			     const environment* env)
{
  ABG_ASSERT(artifact && env);

  ::environment_setter s(env);
  artifact->traverse(s);
}

/// Set the environment of a given ABI artifact, including recursively
/// setting the environment on the sub-trees of the artifact.
///
/// @param artifact the artifact to set the environment for.
///
/// @param env the new environment.
void
set_environment_for_artifact(type_or_decl_base_sptr artifact,
			     const environment* env)
{set_environment_for_artifact(artifact.get(), env);}

/// Non-member equality operator for the @type_or_decl_base type.
///
/// @param lr the left-hand operand of the equality.
///
/// @param rr the right-hand operatnr of the equality.
///
/// @return true iff @p lr equals @p rr.
bool
operator==(const type_or_decl_base& lr, const type_or_decl_base& rr)
{
  const type_or_decl_base* l = &lr;
  const type_or_decl_base* r = &rr;

  const decl_base* dl = dynamic_cast<const decl_base*>(l),
    *dr = dynamic_cast<const decl_base*>(r);

  if (!!dl != !!dr)
    return false;

  if (dl && dr)
    return *dl == *dr;

  const type_base* tl = dynamic_cast<const type_base*>(l),
    *tr = dynamic_cast<const type_base*>(r);

  if (!!tl != !!tr)
    return false;

  if (tl && tr)
    return *tl == *tr;

  return false;
}

/// Non-member equality operator for the @type_or_decl_base type.
///
/// @param l the left-hand operand of the equality.
///
/// @param r the right-hand operatnr of the equality.
///
/// @return true iff @p l equals @p r.
bool
operator==(const type_or_decl_base_sptr& l, const type_or_decl_base_sptr& r)
{
  if (!! l != !!r)
    return false;

  if (!l)
    return true;

  return *r == *l;
}

/// Non-member inequality operator for the @type_or_decl_base type.
///
/// @param l the left-hand operand of the equality.
///
/// @param r the right-hand operator of the equality.
///
/// @return true iff @p l is different from @p r.
bool
operator!=(const type_or_decl_base_sptr& l, const type_or_decl_base_sptr& r)
{return !operator==(l, r);}

// </type_or_decl_base stuff>

// <Decl definition>

struct decl_base::priv
{
  bool			in_pub_sym_tab_;
  bool			is_anonymous_;
  bool			is_artificial_;
  bool			has_anonymous_parent_;
  location		location_;
  context_rel		*context_;
  interned_string	name_;
  interned_string	qualified_parent_name_;
  // This temporary qualified name is the cache used for the qualified
  // name before the type associated to this decl (if applicable) is
  // canonicalized.  Once the type is canonicalized, the cached use is
  // the data member qualified_parent_name_ above.
  interned_string	temporary_qualified_name_;
  // This is the fully qualified name of the decl.  It contains the
  // name of the decl and the qualified name of its scope.  So if in
  // the parent scopes of the decl, there is one anonymous struct,
  // somewhere in the name, there is going to by an
  // __anonymous_struct__ string, even if the anonymous struct is not
  // the direct containing scope of this decl.
  interned_string	qualified_name_;
  // Unline qualified_name_, scoped_name_ contains the name of the
  // decl and the name of its scope; not the qualified name of the
  // scope.
    interned_string	scoped_name_;
  interned_string	linkage_name_;
  visibility		visibility_;
  decl_base_sptr	declaration_;
  decl_base_wptr	definition_of_declaration_;
  decl_base*		naked_definition_of_declaration_;
  bool			is_declaration_only_;

  priv()
    : in_pub_sym_tab_(false),
      is_anonymous_(true),
      is_artificial_(false),
      has_anonymous_parent_(false),
      context_(),
      visibility_(VISIBILITY_DEFAULT),
      naked_definition_of_declaration_(),
      is_declaration_only_(false)
  {}

  priv(interned_string name, const location& locus,
       interned_string linkage_name, visibility vis)
    : in_pub_sym_tab_(false),
      location_(locus),
      context_(),
      name_(name),
      qualified_name_(name),
      linkage_name_(linkage_name),
      visibility_(vis),
      naked_definition_of_declaration_(),
      is_declaration_only_(false)
  {
    is_anonymous_ = name_.empty();
    has_anonymous_parent_ = false;
  }

  priv(const location& l)
    : in_pub_sym_tab_(false),
      is_anonymous_(true),
      has_anonymous_parent_(false),
      location_(l),
      context_(),
      visibility_(VISIBILITY_DEFAULT),
      naked_definition_of_declaration_(),
      is_declaration_only_(false)
  {}

  ~priv()
  {
    delete context_;
  }
};// end struct decl_base::priv

/// Constructor for the @ref decl_base type.
///
/// @param e the environment the current @ref decl_base is being
/// created in.
///
/// @param name the name of the declaration.
///
/// @param locus the location where to find the declaration in the
/// source code.
///
/// @param linkage_name the linkage name of the declaration.
///
/// @param vis the visibility of the declaration.
decl_base::decl_base(const environment* e,
		     const string&	name,
		     const location&	locus,
		     const string&	linkage_name,
		     visibility	vis)
  : type_or_decl_base(e, ABSTRACT_DECL_BASE),
    priv_(new priv(e->intern(name), locus, e->intern(linkage_name), vis))
{
}

/// Constructor.
///
/// @param e the environment this instance of @ref decl_base is
/// created in.
///
/// @param name the name of the declaration being constructed.
///
/// @param locus the source location of the declaration being constructed.
///
/// @param linkage_name the linkage name of the declaration being
/// constructed.
///
/// @param vis the visibility of the declaration being constructed.
decl_base::decl_base(const environment* e,
		     const interned_string& name,
		     const location& locus,
		     const interned_string& linkage_name,
		     visibility vis)
  : type_or_decl_base(e, ABSTRACT_DECL_BASE),
    priv_(new priv(name, locus, linkage_name, vis))
{}

/// Constructor for the @ref decl_base type.
///
///@param environment the environment this instance of @ref decl_base
/// is being constructed in.
///
/// @param l the location where to find the declaration in the source
/// code.
decl_base::decl_base(const environment* e, const location& l)
  : type_or_decl_base(e, ABSTRACT_DECL_BASE),
    priv_(new priv(l))
{}

decl_base::decl_base(const decl_base& d)
  : type_or_decl_base(d)
{
  priv_->in_pub_sym_tab_ = d.priv_->in_pub_sym_tab_;
  priv_->location_ = d.priv_->location_;
  priv_->name_ = d.priv_->name_;
  priv_->qualified_parent_name_ = d.priv_->qualified_parent_name_;
  priv_->qualified_name_ = d.priv_->qualified_name_;
  priv_->linkage_name_ = d.priv_->linkage_name_;
  priv_->context_ = d.priv_->context_;
  priv_->visibility_ = d.priv_->visibility_;
}

/// Getter for the qualified name.
///
/// Unlike decl_base::get_qualified_name() this doesn't try to update
/// the qualified name.
///
/// @return the qualified name.
const interned_string&
decl_base::peek_qualified_name() const
{return priv_->qualified_name_;}

/// Clear the qualified name of this decl.
///
/// This is useful to ensure that the cache for the qualified name of
/// the decl is refreshed right after type canonicalization, for
/// instance.
void
decl_base::clear_qualified_name()
{priv_->qualified_name_.clear();}

/// Setter for the qualified name.
///
/// @param n the new qualified name.
void
decl_base::set_qualified_name(const interned_string& n) const
{priv_->qualified_name_ = n;}

/// Getter of the temporary qualified name of the current declaration.
///
/// This temporary qualified name is used as a qualified name cache by
/// the type for which this is the declaration (when applicable)
/// before the type is canonicalized.  Once the type is canonicalized,
/// it's the result of decl_base::peek_qualified_name() that becomes
/// the qualified name cached.
///
/// @return the temporary qualified name.
const interned_string&
decl_base::peek_temporary_qualified_name() const
{return priv_->temporary_qualified_name_;}

/// Setter for the temporary qualified name of the current
/// declaration.
///
///@param n the new temporary qualified name.
///
/// This temporary qualified name is used as a qualified name cache by
/// the type for which this is the declaration (when applicable)
/// before the type is canonicalized.  Once the type is canonicalized,
/// it's the result of decl_base::peek_qualified_name() that becomes
/// the qualified name cached.
void
decl_base::set_temporary_qualified_name(const interned_string& n) const
{priv_->temporary_qualified_name_ = n;}

///Getter for the context relationship.
///
///@return the context relationship for the current decl_base.
const context_rel*
decl_base::get_context_rel() const
{return priv_->context_;}

///Getter for the context relationship.
///
///@return the context relationship for the current decl_base.
context_rel*
decl_base::get_context_rel()
{return priv_->context_;}

void
decl_base::set_context_rel(context_rel *c)
{priv_->context_ = c;}

/// Get the hash of a decl.  If the hash hasn't been computed yet,
/// compute it ans store its value; otherwise, just return the hash.
///
/// @return the hash of the decl.
size_t
decl_base::get_hash() const
{
  size_t result = 0;

  if (const type_base* t = dynamic_cast<const type_base*>(this))
    {
      type_base::dynamic_hash hash;
      result = hash(t);
    }
  else
    // If we reach this point, it mean we are missing a virtual
    // overload for decl_base::get_hash.  Add it!
    abort();

  return result;
}

/// Test if the decl is defined in a ELF symbol table as a public
/// symbol.
///
/// @return true iff the decl is defined in a ELF symbol table as a
/// public symbol.
bool
decl_base::get_is_in_public_symbol_table() const
{return priv_->in_pub_sym_tab_;}

/// Set the flag saying if this decl is from a symbol that is in
/// a public symbols table, defined as public (global or weak).
///
/// @param f the new flag value.
void
decl_base::set_is_in_public_symbol_table(bool f)
{priv_->in_pub_sym_tab_ = f;}

/// Get the location of a given declaration.
///
/// The location is an abstraction for the tripplet {file path,
/// line, column} that defines where the declaration appeared in the
/// source code.
///
/// To get the value of the tripplet {file path, line, column} from
/// the @ref location, you need to use the
/// location_manager::expand_location() method.
///
/// The instance of @ref location_manager that you want is
/// accessible from the instance of @ref translation_unit that the
/// current instance of @ref decl_base belongs to, via a call to
/// translation_unit::get_loc_mgr().
///
/// @return the location of the current instance of @ref decl_base.
const location&
decl_base::get_location() const
{return priv_->location_;}

/// Set the location for a given declaration.
///
/// The location is an abstraction for the tripplet {file path,
/// line, column} that defines where the declaration appeared in the
/// source code.
///
/// To create a location from a tripplet {file path, line, column},
/// you need to use the method @ref
/// location_manager::create_new_location().
///
/// The instance of @ref location_manager that you want is
/// accessible from the instance of @ref translation_unit that the
/// current instance of @ref decl_base belongs to, via a call to
/// translation_unit::get_loc_mgr().
void
decl_base::set_location(const location& l)
{priv_->location_ = l;}

/// Setter for the name of the decl.
///
/// @param n the new name to set.
void
decl_base::set_name(const string& n)
{
  priv_->name_ = get_environment()->intern(n);
  priv_->is_anonymous_ = n.empty();
}

/// Test if the current declaration is anonymous.
///
/// Being anonymous means that the declaration was created without a
/// name.  This can usually happen for enum or struct types.
///
/// @return true iff the type is anonymous.
bool
decl_base::get_is_anonymous() const
{return priv_->is_anonymous_;}

/// Set the "is_anonymous" flag of the current declaration.
///
/// Being anonymous means that the declaration was created without a
/// name.  This can usually happen for enum or struct types.
///
/// @param f the new value of the flag.
void
decl_base::set_is_anonymous(bool f)
{priv_->is_anonymous_ = f;}

/// Getter of the flag that says if the declaration is artificial.
///
/// Being artificial means the parameter was not explicitely
/// mentionned in the source code, but was rather artificially created
/// by the compiler.
///
/// @return true iff the declaration is artificial.
bool
decl_base::get_is_artificial() const
{return priv_->is_artificial_;}

/// Setter of the flag that says if the declaration is artificial.
///
/// Being artificial means the parameter was not explicitely
/// mentionned in the source code, but was rather artificially created
/// by the compiler.
///
/// @param f the new value of the flag that says if the declaration is
/// artificial.
void
decl_base::set_is_artificial(bool f)
{priv_->is_artificial_ = f;}

/// Get the "has_anonymous_parent" flag of the current declaration.
///
/// Having an anoymous parent means having a anonymous parent scope
/// (containing type or namespace) which is either direct or indirect.
///
/// @return true iff the current decl has a direct or indirect scope
/// which is anonymous.
bool
decl_base::get_has_anonymous_parent() const
{return priv_->has_anonymous_parent_;}

/// Set the "has_anonymous_parent" flag of the current declaration.
///
/// Having an anonymous parent means having a anonymous parent scope
/// (containing type or namespace) which is either direct or indirect.
///
/// @param f set the flag which says if the current decl has a direct
/// or indirect scope which is anonymous.
void
decl_base::set_has_anonymous_parent(bool f) const
{priv_->has_anonymous_parent_ = f;}

/// @return the logical "OR" of decl_base::get_is_anonymous() and
/// decl_base::get_has_anonymous_parent().
bool
decl_base::get_is_anonymous_or_has_anonymous_parent() const
{return get_is_anonymous() || get_has_anonymous_parent();}

/// Getter for the mangled name.
///
/// @return the new mangled name.
const interned_string&
decl_base::get_linkage_name() const
{return priv_->linkage_name_;}

/// Setter for the linkage name.
///
/// @param m the new linkage name.
void
decl_base::set_linkage_name(const string& m)
{
  const environment* env = get_environment();
  ABG_ASSERT(env);
  priv_->linkage_name_ = env->intern(m);
}

/// Getter for the visibility of the decl.
///
/// @return the new visibility.
decl_base::visibility
decl_base::get_visibility() const
{return priv_->visibility_;}

/// Setter for the visibility of the decl.
///
/// @param v the new visibility.
void
decl_base::set_visibility(visibility v)
{priv_->visibility_ = v;}

/// Return the type containing the current decl, if any.
///
/// @return the type that contains the current decl, or NULL if there
/// is none.
scope_decl*
decl_base::get_scope() const
{
  if (priv_->context_)
    return priv_->context_->get_scope();
  return 0;
}

/// Return a copy of the qualified name of the parent of the current
/// decl.
///
/// @return the newly-built qualified name of the of the current decl.
const interned_string&
decl_base::get_qualified_parent_name() const
{return priv_->qualified_parent_name_;}

/// Getter for the name of the current decl.
///
/// @return the name of the current decl.
const interned_string&
decl_base::get_name() const
{return priv_->name_;}

/// Compute the qualified name of the decl.
///
/// @param qn the resulting qualified name.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
decl_base::get_qualified_name(interned_string& qn, bool internal) const
{qn = get_qualified_name(internal);}

/// Get the pretty representatin of the current declaration.
///
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qualified_name if true, names emitted in the pretty
/// representation are fully qualified.
///
/// @return the default pretty representation for a decl.  This is
/// basically the fully qualified name of the decl optionally prefixed
/// with a meaningful string to add context for the user.
string
decl_base::get_pretty_representation(bool internal,
				     bool qualified_name) const
{
  if (internal
      && get_is_anonymous()
      && has_generic_anonymous_internal_type_name(this))
    {
      // We are looking at an anonymous enum, union or class and we
      // want an *internal* pretty representation for it.  All
      // anonymous types of this kind in the same namespace must have
      // the same internal representation for type canonicalization to
      // work properly.
      //
      // OK, in practise, we are certainly looking at an enum because
      // classes and unions should have their own overloaded virtual
      // member function for this.
      string name = get_generic_anonymous_internal_type_name(this);
      if (qualified_name && !get_qualified_parent_name().empty())
	name = get_qualified_parent_name() + "::" + name;
      return name;
    }

  if (qualified_name)
    return get_qualified_name(internal);
  return get_name();
}

/// Return the qualified name of the decl.
///
/// This is the fully qualified name of the decl.  It's made of the
/// concatenation of the name of the decl with the qualified name of
/// its scope.
///
/// Note that the value returned by this function is computed by @ref
/// update_qualified_name when the decl is added to its scope.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the resulting qualified name.
const interned_string&
decl_base::get_qualified_name(bool /*internal*/) const
{return priv_->qualified_name_;}

/// Return the scoped name of the decl.
///
/// This is made of the concatenation of the name of the decl with the
/// name of its scope.  It doesn't contain the qualified name of its
/// scope, unlike what is returned by decl_base::get_qualified_name.
///
/// Note that the value returned by this function is computed by @ref
/// update_qualified_name when the decl is added to its scope.
///
/// @return the scoped name of the decl.
const interned_string&
decl_base::get_scoped_name() const
{return priv_->scoped_name_;}

/// If this @ref decl_base is a definition, get its earlier
/// declaration.
///
/// @return the earlier declaration of the class, if any.
const decl_base_sptr
decl_base::get_earlier_declaration() const
{return priv_->declaration_;}

/// set the earlier declaration of this @ref decl_base definition.
///
/// @param d the earlier declaration to set.  Note that it's set only
/// if it's a pure declaration.
void
decl_base::set_earlier_declaration(const decl_base_sptr& d)
{
  if (d && d->get_is_declaration_only())
    priv_->declaration_ = d;
}


/// If this @ref decl_base is declaration-only, get its definition, if
/// any.
///
/// @return the definition of this decl-only @ref decl_base.
const decl_base_sptr
decl_base::get_definition_of_declaration() const
{return priv_->definition_of_declaration_.lock();}

///  If this @ref decl_base is declaration-only, get its definition,
///  if any.
///
/// Note that this function doesn't return a smart pointer, but rather
/// the underlying pointer managed by the smart pointer.  So it's as
/// fast as possible.  This getter is to be used in code paths that
/// are proven to be performance hot spots; especially, when comparing
/// sensitive types like enums, classes or unions.  Those are compared
/// extremely frequently and thus, their access to the definition of
/// declaration must be fast.
///
/// @return the definition of the declaration.
const decl_base*
decl_base::get_naked_definition_of_declaration() const
{return priv_->naked_definition_of_declaration_;}

/// Test if a @ref decl_base is a declaration-only decl.
///
/// @return true iff the current @ref decl_base is declaration-only.
bool
decl_base::get_is_declaration_only() const
{return priv_->is_declaration_only_;}

/// Set a flag saying if the @ref enum_type_decl is a declaration-only
/// @ref enum_type_decl.
///
/// @param f true if the @ref enum_type_decl is a declaration-only
/// @ref enum_type_decl.
void
decl_base::set_is_declaration_only(bool f)
{
  bool update_types_lookup_map = !f && priv_->is_declaration_only_;

  priv_->is_declaration_only_ = f;

  if (update_types_lookup_map)
    if (scope_decl* s = get_scope())
      {
	scope_decl::declarations::iterator i;
	if (s->find_iterator_for_member(this, i))
	  maybe_update_types_lookup_map(*i);
	else
	  ABG_ASSERT_NOT_REACHED;
      }
}

change_kind
operator|(change_kind l, change_kind r)
{
  return static_cast<change_kind>(static_cast<unsigned>(l)
				  | static_cast<unsigned>(r));
}

change_kind
operator&(change_kind l, change_kind r)
{
  return static_cast<change_kind>(static_cast<unsigned>(l)
				  & static_cast<unsigned>(r));
}

change_kind&
operator|=(change_kind& l, change_kind r)
{
  l = l | r;
  return l;
}

change_kind&
operator&=(change_kind& l, change_kind r)
{
  l = l & r;
  return l;
}

/// Compare the properties that belong to the "is-a-member-relation"
/// of a decl.
///
/// For instance, access specifiers are part of the
/// "is-a-member-relation" of a decl.
///
/// This comparison however doesn't take decl names into account.  So
/// typedefs for instance are decls that we want to compare with this
/// function.
///
/// This function is a sub-routine of the more general 'equals'
/// overload for instances of decl_base.
///
/// @param l the left-hand side operand of the comparison.
///
/// @param r the right-hand side operand of the comparison.
///
/// @return true iff @p l compare equals, as a member decl, to @p r.
bool
maybe_compare_as_member_decls(const decl_base& l,
			      const decl_base& r,
			      change_kind* k)
{
  bool result = true;
  if (is_member_decl(l) && is_member_decl(r))
    {
      context_rel* r1 = const_cast<context_rel*>(l.get_context_rel());
      context_rel *r2 = const_cast<context_rel*>(r.get_context_rel());

      access_specifier la = no_access, ra = no_access;
      bool member_types_or_functions =
	((is_type(l) && is_type(r))
	 || (is_function_decl(l) && is_function_decl(r)));

      if (member_types_or_functions)
	{
	  // Access specifiers on member types in DWARF is not
	  // reliable; in the same DSO, the same struct can be either
	  // a class or a struct, and the access specifiers of its
	  // member types are not necessarily given, so they
	  // effectively can be considered differently, again, in the
	  // same DSO.  So, here, let's avoid considering those!
	  // during comparison.
	  la = r1->get_access_specifier();
	  ra = r2->get_access_specifier();
	  r1->set_access_specifier(no_access);
	  r2->set_access_specifier(no_access);
	}

      bool rels_are_different = *r1 != *r2;

      if (member_types_or_functions)
	{
	  // restore the access specifiers.
	  r1->set_access_specifier(la);
	  r2->set_access_specifier(ra);
	}

      if (rels_are_different)
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	}
    }
  return result;
}

/// Compares two instances of @ref decl_base.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff it's non-null and if the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const decl_base& l, const decl_base& r, change_kind* k)
{
  bool result = true;
  const interned_string &l_linkage_name = l.get_linkage_name();
  const interned_string &r_linkage_name = r.get_linkage_name();
  if (!l_linkage_name.empty() && !r_linkage_name.empty())
    {
      if (l_linkage_name != r_linkage_name)
	{
	  // Linkage names are different.  That usually means the two
	  // decls are different, unless we are looking at two
	  // function declarations which have two different symbols
	  // that are aliases of each other.
	  const function_decl *f1 = is_function_decl(&l),
	    *f2 = is_function_decl(&r);
	  if (f1 && f2 && function_decls_alias(*f1, *f2))
	    ;// The two functions are aliases, so they are not different.
	  else
	    {
	      result = false;
	      if (k)
		*k |= LOCAL_NON_TYPE_CHANGE_KIND;
	      else
		return false;
	    }
	}
    }

  // This is the name of the decls that we want to compare.
  interned_string ln = l.get_qualified_name(), rn = r.get_qualified_name();

  /// If both of the current decls have an anonymous scope then let's
  /// compare their name component by component by properly handling
  /// anonymous scopes. That's the slow path.
  ///
  /// Otherwise, let's just compare their name, the obvious way.
  /// That's the fast path because in that case the names are
  /// interned_string and comparing them is much faster.
  bool decls_are_same = (ln == rn);
  if (!decls_are_same
      && l.get_is_anonymous()
      && !l.get_has_anonymous_parent()
      && r.get_is_anonymous()
      && !r.get_has_anonymous_parent()
      && (l.get_qualified_parent_name() == r.get_qualified_parent_name()))
    // Both decls are anonymous and their scope are *NOT* anonymous.
    // So we consider the decls to have equivalent names (both
    // anonymous, remember).  We are still in the fast path here.
    decls_are_same = true;

  if (!decls_are_same
      && l.get_has_anonymous_parent()
      && r.get_has_anonymous_parent())
    // This is the slow path as we are comparing the decl qualified
    // names component by component, properly handling anonymous
    // scopes.
    decls_are_same = tools_utils::decl_names_equal(ln, rn);

  if (!decls_are_same)
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }

  result &= maybe_compare_as_member_decls(l, r, k);

  return result;
}

/// Return true iff the two decls have the same name.
///
/// This function doesn't test if the scopes of the the two decls are
/// equal.
///
/// Note that this virtual function is to be implemented by classes
/// that extend the \p decl_base class.
bool
decl_base::operator==(const decl_base& other) const
{return equals(*this, other, 0);}

/// Inequality operator.
///
/// @param other to other instance of @ref decl_base to compare the
/// current instance to.
///
/// @return true iff the current instance of @ref decl_base is
/// different from @p other.
bool
decl_base::operator!=(const decl_base& other) const
{return !operator==(other);}

/// Destructor of the @ref decl_base type.
decl_base::~decl_base()
{delete priv_;}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the member nodes of the translation
/// unit during the traversal.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
decl_base::traverse(ir_node_visitor&)
{
  // Do nothing in the base class.
  return true;
}

/// Setter of the scope of the current decl.
///
/// Note that the decl won't hold a reference on the scope.  It's
/// rather the scope that holds a reference on its members.
void
decl_base::set_scope(scope_decl* scope)
{
  if (!priv_->context_)
    priv_->context_ = new context_rel(scope);
  else
    priv_->context_->set_scope(scope);
}

// </decl_base definition>

/// Streaming operator for the decl_base::visibility.
///
/// @param o the output stream to serialize the visibility to.
///
/// @param v the visibility to serialize.
///
/// @return the output stream.
std::ostream&
operator<<(std::ostream& o, decl_base::visibility v)
{
  string r;
  switch (v)
    {
    case decl_base::VISIBILITY_NONE:
      r = "none";
      break;
    case decl_base::VISIBILITY_DEFAULT:
      r = "default";
      break;
    case decl_base::VISIBILITY_PROTECTED:
      r = "protected";
      break;
    case decl_base::VISIBILITY_HIDDEN:
      r = "hidden";
      break;
    case decl_base::VISIBILITY_INTERNAL:
      r = "internal";
      break;
    }
  return o;
}

/// Streaming operator for decl_base::binding.
///
/// @param o the output stream to serialize the visibility to.
///
/// @param b the binding to serialize.
///
/// @return the output stream.
std::ostream&
operator<<(std::ostream& o, decl_base::binding b)
{
  string r;
  switch (b)
    {
    case decl_base::BINDING_NONE:
      r = "none";
      break;
    case decl_base::BINDING_LOCAL:
      r = "local";
      break;
    case decl_base::BINDING_GLOBAL:
      r = "global";
      break;
    case decl_base::BINDING_WEAK:
      r = "weak";
      break;
    }
  o << r;
  return o;
}

/// Turn equality of shared_ptr of decl_base into a deep equality;
/// that is, make it compare the pointed to objects, not just the
/// pointers.
///
/// @param l the shared_ptr of decl_base on left-hand-side of the
/// equality.
///
/// @param r the shared_ptr of decl_base on right-hand-side of the
/// equality.
///
/// @return true if the decl_base pointed to by the shared_ptrs are
/// equal, false otherwise.
bool
operator==(const decl_base_sptr& l, const decl_base_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Inequality operator of shared_ptr of @ref decl_base.
///
/// This is a deep equality operator, that is, it compares the
/// pointed-to objects, rather than just the pointers.
///
/// @param l the left-hand-side operand.
///
/// @param r the right-hand-side operand.
///
/// @return true iff @p l is different from @p r.
bool
operator!=(const decl_base_sptr& l, const decl_base_sptr& r)
{return !operator==(l, r);}

/// Turn equality of shared_ptr of type_base into a deep equality;
/// that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of type_base on left-hand-side of the
/// equality.
///
/// @param r the shared_ptr of type_base on right-hand-side of the
/// equality.
///
/// @return true if the type_base pointed to by the shared_ptrs are
/// equal, false otherwise.
bool
operator==(const type_base_sptr& l, const type_base_sptr& r)
{
    if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Turn inequality of shared_ptr of type_base into a deep equality;
/// that is, make it compare the pointed to objects..
///
/// @param l the shared_ptr of type_base on left-hand-side of the
/// equality.
///
/// @param r the shared_ptr of type_base on right-hand-side of the
/// equality.
///
/// @return true iff the type_base pointed to by the shared_ptrs are
/// different.
bool
operator!=(const type_base_sptr& l, const type_base_sptr& r)
{return !operator==(l, r);}

/// Tests if a declaration has got a scope.
///
/// @param d the declaration to consider.
///
/// @return true if the declaration has got a scope, false otherwise.
bool
has_scope(const decl_base& d)
{return (d.get_scope());}

/// Tests if a declaration has got a scope.
///
/// @param d the declaration to consider.
///
/// @return true if the declaration has got a scope, false otherwise.
bool
has_scope(const decl_base_sptr d)
{return has_scope(*d.get());}

/// Tests if a declaration is a class member.
///
/// @param d the declaration to consider.
///
/// @return true if @p d is a class member, false otherwise.
bool
is_member_decl(const decl_base_sptr d)
{return is_at_class_scope(d) || is_method_decl(d);}

/// Tests if a declaration is a class member.
///
/// @param d the declaration to consider.
///
/// @return true if @p d is a class member, false otherwise.
bool
is_member_decl(const decl_base* d)
{return is_at_class_scope(d) || is_method_decl(d);}

/// Tests if a declaration is a class member.
///
/// @param d the declaration to consider.
///
/// @return true if @p d is a class member, false otherwise.
bool
is_member_decl(const decl_base& d)
{return is_at_class_scope(d) || is_method_decl(d);}

/// Test if a declaration is a @ref scope_decl.
///
/// @param d the declaration to take in account.
///
/// @return the a pointer to the @ref scope_decl sub-object of @p d,
/// if d is a @ref scope_decl.
scope_decl*
is_scope_decl(decl_base* d)
{return dynamic_cast<scope_decl*>(d);}

/// Test if a declaration is a @ref scope_decl.
///
/// @param d the declaration to take in account.
///
/// @return the a pointer to the @ref scope_decl sub-object of @p d,
/// if d is a @ref scope_decl.
scope_decl_sptr
is_scope_decl(const decl_base_sptr& d)
{return dynamic_pointer_cast<scope_decl>(d);}

/// Tests if a type is a class member.
///
/// @param t the type to consider.
///
/// @return true if @p t is a class member type, false otherwise.
bool
is_member_type(const type_base_sptr& t)
{
  decl_base_sptr d = get_type_declaration(t);
  return is_member_decl(d);
}

/// Test if a type is user-defined.
///
/// A type is considered user-defined if it's a
/// struct/class/union/enum that is *NOT* artificial.
///
/// @param t the type to consider.
///
/// @return true iff the type @p t is user-defined.
bool
is_user_defined_type(const type_base* t)
{
  if (t == 0)
    return false;

  t = peel_qualified_or_typedef_type(t);
  decl_base *d = is_decl(t);

  if ((is_class_or_union_type(t) || is_enum_type(t))
      && d && !d->get_is_artificial())
    return true;

  return false;
}

/// Test if a type is user-defined.
///
/// A type is considered user-defined if it's a
/// struct/class/union/enum.
///
///
/// @param t the type to consider.
///
/// @return true iff the type @p t is user-defined.
bool
is_user_defined_type(const type_base_sptr& t)
{return is_user_defined_type(t.get());}

/// Gets the access specifier for a class member.
///
/// @param d the declaration of the class member to consider.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return the access specifier for the class member @p d.
access_specifier
get_member_access_specifier(const decl_base& d)
{
  ABG_ASSERT(is_member_decl(d));

  const context_rel* c = d.get_context_rel();
  ABG_ASSERT(c);

  return c->get_access_specifier();
}

/// Gets the access specifier for a class member.
///
/// @param d the declaration of the class member to consider.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return the access specifier for the class member @p d.
access_specifier
get_member_access_specifier(const decl_base_sptr& d)
{return get_member_access_specifier(*d);}

/// Sets the access specifier for a class member.
///
/// @param d the class member to set the access specifier for.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @param a the new access specifier to set the class member to.
void
set_member_access_specifier(decl_base& d,
			    access_specifier a)
{
  ABG_ASSERT(is_member_decl(d));

  context_rel* c = d.get_context_rel();
  ABG_ASSERT(c);

  c->set_access_specifier(a);
}

/// Sets the access specifier for a class member.
///
/// @param d the class member to set the access specifier for.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @param a the new access specifier to set the class member to.
void
set_member_access_specifier(const decl_base_sptr& d,
			    access_specifier a)
{set_member_access_specifier(*d, a);}

/// Gets a flag saying if a class member is static or not.
///
/// @param d the declaration for the class member to consider. Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return true if the class member @p d is static, false otherwise.
bool
get_member_is_static(const decl_base&d)
{
  ABG_ASSERT(is_member_decl(d));

  const context_rel* c = d.get_context_rel();
  ABG_ASSERT(c);

  return c->get_is_static();
}

/// Gets a flag saying if a class member is static or not.
///
/// @param d the declaration for the class member to consider. Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return true if the class member @p d is static, false otherwise.
bool
get_member_is_static(const decl_base* d)
{return get_member_is_static(*d);}

/// Gets a flag saying if a class member is static or not.
///
/// @param d the declaration for the class member to consider.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return true if the class member @p d is static, false otherwise.
bool
get_member_is_static(const decl_base_sptr& d)
{return get_member_is_static(*d);}

/// Test if a var_decl is a data member.
///
/// @param v the var_decl to consider.
///
/// @return true if @p v is data member, false otherwise.
bool
is_data_member(const var_decl& v)
{return is_at_class_scope(v);}

/// Test if a var_decl is a data member.
///
/// @param v the var_decl to consider.
///
/// @return true if @p v is data member, false otherwise.
bool
is_data_member(const var_decl* v)
{return is_data_member(*v);}

/// Test if a var_decl is a data member.
///
/// @param v the var_decl to consider.
///
/// @return true if @p v is data member, false otherwise.
bool
is_data_member(const var_decl_sptr d)
{return is_at_class_scope(d);}

/// Test if a decl is a data member.
///
/// @param d the decl to consider.
///
/// @return a pointer to the data member iff @p d is a data member, or
/// a null pointer.
var_decl_sptr
is_data_member(const decl_base_sptr& d)
{
  if (var_decl_sptr v = is_var_decl(d))
    {
      if (is_data_member(v))
	return v;
    }
  return var_decl_sptr();
}

/// Test if a decl is a data member.
///
/// @param d the decl to consider.
///
/// @return a pointer to the data member iff @p d is a data member, or
/// a null pointer.
var_decl_sptr
is_data_member(const type_or_decl_base_sptr& d)
{
  if (var_decl_sptr v = is_var_decl(d))
    {
      if (is_data_member(v))
	return v;
    }
  return var_decl_sptr();
}

/// Test if a decl is a data member.
///
/// @param d the decl to consider.
///
/// @return a pointer to the data member iff @p d is a data member, or
/// a null pointer.
var_decl*
is_data_member(const type_or_decl_base* d)
{
 if (var_decl *v = is_var_decl(d))
    if (is_data_member(v))
      return v;
  return 0;
}

/// Test if a decl is a data member.
///
/// @param d the decl to consider.
///
/// @return a pointer to the data member iff @p d is a data member, or
/// a null pointer.
var_decl*
is_data_member(const decl_base *d)
{
  if (var_decl *v = is_var_decl(d))
    if (is_data_member(v))
      return v;
  return 0;
}

/// Get the first non-anonymous data member of a given anonymous data
/// member.
///
/// E.g:
///
///   struct S
///   {
///     union // <-- for this anonymous data member, the function
///           // returns a.
///     {
///       int a;
///       charb;
///     };
///   };
///
/// @return anon_dm the anonymous data member to consider.
///
/// @return the first non-anonymous data member of @p anon_dm.  If no
/// data member was found then this function returns @p anon_dm.
const var_decl_sptr
get_first_non_anonymous_data_member(const var_decl_sptr anon_dm)
{
  if (!anon_dm || !is_anonymous_data_member(anon_dm))
    return anon_dm;

  class_or_union_sptr klass = anonymous_data_member_to_class_or_union(anon_dm);
 var_decl_sptr first = *klass->get_non_static_data_members().begin();

 if (is_anonymous_data_member(first))
   return get_first_non_anonymous_data_member(first);

 return first;
}

/// In the context of a given class or union, this function returns
/// the data member that is located after a given data member.
///
/// @param klass the class or union to consider.
///
/// @param the data member to consider.
///
/// @return the data member that is located right after @p
/// data_member.
const var_decl_sptr
get_next_data_member(const class_or_union_sptr &klass,
		     const var_decl_sptr &data_member)
{
  if (!klass ||!data_member)
    return var_decl_sptr();

  for (class_or_union::data_members::const_iterator it =
	 klass->get_non_static_data_members().begin();
       it != klass->get_non_static_data_members().end();
       ++it)
    if (**it == *data_member)
      {
	++it;
	if (it != klass->get_non_static_data_members().end())
	  return get_first_non_anonymous_data_member(*it);
	break;
      }

  return var_decl_sptr();
}

/// Test if a decl is an anonymous data member.
///
/// @param d the decl to consider.
///
/// @return true iff @p d is an anonymous data member.
bool
is_anonymous_data_member(const decl_base& d)
{return is_anonymous_data_member(&d);}

/// Test if a decl is an anonymous data member.
///
/// @param d the decl to consider.
///
/// @return the var_decl representing the data member iff @p d is an
/// anonymous data member.
const var_decl*
is_anonymous_data_member(const type_or_decl_base* d)
{
  if (const var_decl* v = is_data_member(d))
    {
      if (is_anonymous_data_member(v))
	return v;
    }
  return 0;
}

/// Test if a decl is an anonymous data member.
///
/// @param d the decl to consider.
///
/// @return a non-nil pointer to the @ref var_decl denoted by @p d if
/// it's an anonymous data member.  Otherwise returns a nil pointer.
const var_decl*
is_anonymous_data_member(const decl_base* d)
{
  if (const var_decl* v = is_data_member(d))
    {
      if (is_anonymous_data_member(v))
	return v;
    }
  return 0;
}

/// Test if a decl is an anonymous data member.
///
/// @param d the decl to consider.
///
/// @return a non-nil pointer to the @ref var_decl denoted by @p d if
/// it's an anonymous data member.  Otherwise returns a nil pointer.
var_decl_sptr
is_anonymous_data_member(const type_or_decl_base_sptr& d)
{
  if (var_decl_sptr v = is_data_member(d))
    {
      if (is_anonymous_data_member(v))
	return v;
    }
  return var_decl_sptr();
}

/// Test if a decl is an anonymous data member.
///
/// @param d the decl to consider.
///
/// @return a non-nil pointer to the @ref var_decl denoted by @p d if
/// it's an anonymous data member.  Otherwise returns a nil pointer.
var_decl_sptr
is_anonymous_data_member(const decl_base_sptr& d)
{
  if (var_decl_sptr v = is_data_member(d))
    return is_anonymous_data_member(v);
  return var_decl_sptr();
}

/// Test if a @ref var_decl is an anonymous data member.
///
/// @param d the @ref var_decl to consider.
///
/// @return a non-nil pointer to the @ref var_decl denoted by @p d if
/// it's an anonymous data member.  Otherwise returns a nil pointer.
var_decl_sptr
is_anonymous_data_member(const var_decl_sptr& d)
{
  if (is_anonymous_data_member(d.get()))
    return d;
  return var_decl_sptr();
}

/// Test if a @ref var_decl is an anonymous data member.
///
/// @param d the @ref var_decl to consider.
///
/// @return a non-nil pointer to the @ref var_decl denoted by @p d if
/// it's an anonymous data member.  Otherwise returns a nil pointer.
const var_decl*
is_anonymous_data_member(const var_decl* d)
{
  if (d && is_anonymous_data_member(*d))
    return d;
  return 0;
}

/// Test if a @ref var_decl is an anonymous data member.
///
/// @param d the @ref var_decl to consider.
///
/// @return true iff @p d is an anonymous data member.
bool
is_anonymous_data_member(const var_decl& d)
{
  return (is_data_member(d)
	  && d.get_name().empty()
	  && is_class_or_union_type(d.get_type()));
}

/// Get the @ref class_or_union type of a given anonymous data member.
///
/// @param d the anonymous data member to consider.
///
/// @return the @ref class_or_union type of the anonymous data member
/// @p d.
class_or_union*
anonymous_data_member_to_class_or_union(const var_decl* d)
{
  if ((d = is_anonymous_data_member(d)))
    return is_class_or_union_type(d->get_type().get());
  return 0;
}

/// Test if a data member has annonymous type or not.
///
/// @param d the data member to consider.
///
/// @return the anonymous class or union type iff @p turns out to have
/// an anonymous type.  Otherwise, returns nil.
const class_or_union_sptr
data_member_has_anonymous_type(const var_decl& d)
{
  if (is_data_member(d))
    if (const class_or_union_sptr cou = is_class_or_union_type(d.get_type()))
      if (cou->get_is_anonymous())
	return cou;

  return class_or_union_sptr();
}

/// Test if a data member has annonymous type or not.
///
/// @param d the data member to consider.
///
/// @return the anonymous class or union type iff @p turns out to have
/// an anonymous type.  Otherwise, returns nil.
const class_or_union_sptr
data_member_has_anonymous_type(const var_decl* d)
{
  if (d)
    return data_member_has_anonymous_type(*d);
  return class_or_union_sptr();
}

/// Test if a data member has annonymous type or not.
///
/// @param d the data member to consider.
///
/// @return the anonymous class or union type iff @p turns out to have
/// an anonymous type.  Otherwise, returns nil.
const class_or_union_sptr
data_member_has_anonymous_type(const var_decl_sptr& d)
{return data_member_has_anonymous_type(d.get());}

/// Get the @ref class_or_union type of a given anonymous data member.
///
/// @param d the anonymous data member to consider.
///
/// @return the @ref class_or_union type of the anonymous data member
/// @p d.
class_or_union_sptr
anonymous_data_member_to_class_or_union(const var_decl_sptr &d)
{
  if (var_decl_sptr v = is_anonymous_data_member(d))
    return is_class_or_union_type(v->get_type());
  return class_or_union_sptr();
}

/// Set the offset of a data member into its containing class.
///
/// @param m the data member to consider.
///
/// @param o the offset, in bits.
void
set_data_member_offset(var_decl_sptr m, uint64_t o)
{
  ABG_ASSERT(is_data_member(m));

  dm_context_rel* ctxt_rel =
    dynamic_cast<dm_context_rel*>(m->get_context_rel());
  ABG_ASSERT(ctxt_rel);

  ctxt_rel->set_offset_in_bits(o);
}

/// Get the offset of a data member.
///
/// @param m the data member to consider.
///
/// @return the offset (in bits) of @p m in its containing class.
uint64_t
get_data_member_offset(const var_decl& m)
{
  ABG_ASSERT(is_data_member(m));
  const dm_context_rel* ctxt_rel =
    dynamic_cast<const dm_context_rel*>(m.get_context_rel());
  ABG_ASSERT(ctxt_rel);
  return ctxt_rel->get_offset_in_bits();
}

/// Get the offset of a data member.
///
/// @param m the data member to consider.
///
/// @return the offset (in bits) of @p m in its containing class.
uint64_t
get_data_member_offset(const var_decl_sptr m)
{return get_data_member_offset(*m);}

/// Get the offset of a data member.
///
/// @param m the data member to consider.
///
/// @return the offset (in bits) of @p m in its containing class.
uint64_t
get_data_member_offset(const decl_base_sptr d)
{return get_data_member_offset(dynamic_pointer_cast<var_decl>(d));}

/// Get the absolute offset of a data member.
///
/// If the data member is part of an anonymous data member then this
/// returns the absolute offset -- relative to the beginning of the
/// containing class of the anonymous data member.
///
/// @param m the data member to consider.
///
/// @return the aboslute offset of the data member @p m.
uint64_t
get_absolute_data_member_offset(const var_decl& m)
{
  ABG_ASSERT(is_data_member(m));
  const dm_context_rel* ctxt_rel =
    dynamic_cast<const dm_context_rel*>(m.get_context_rel());
  ABG_ASSERT(ctxt_rel);

  const var_decl *containing_anonymous_data_member =
    ctxt_rel->get_anonymous_data_member();

  uint64_t containing_anonymous_data_member_offset = 0;
  if (containing_anonymous_data_member)
    containing_anonymous_data_member_offset =
      get_absolute_data_member_offset(*containing_anonymous_data_member);

  return (ctxt_rel->get_offset_in_bits()
	  +
	  containing_anonymous_data_member_offset);
}

/// Get the absolute offset of a data member.
///
/// If the data member is part of an anonymous data member then this
/// returns the absolute offset -- relative to the beginning of the
/// containing class of the anonymous data member.
///
/// @param m the data member to consider.
///
/// @return the aboslute offset of the data member @p m.
uint64_t
get_absolute_data_member_offset(const var_decl_sptr& m)
{
  if (!m)
    return 0;
  return get_absolute_data_member_offset(*m);
}

/// Get the size of a given variable.
///
/// @param v the variable to consider.
///
/// @return the size of variable @p v.
uint64_t
get_var_size_in_bits(const var_decl_sptr& v)
{
  type_base_sptr t = v->get_type();
  ABG_ASSERT(t);

  return t->get_size_in_bits();
}

/// Set a flag saying if a data member is laid out.
///
/// @param m the data member to consider.
///
/// @param l true if @p m is to be considered as laid out.
void
set_data_member_is_laid_out(var_decl_sptr m, bool l)
{
  ABG_ASSERT(is_data_member(m));
  dm_context_rel* ctxt_rel =
    dynamic_cast<dm_context_rel*>(m->get_context_rel());
  ctxt_rel->set_is_laid_out(l);
}

/// Test whether a data member is laid out.
///
/// @param m the data member to consider.
///
/// @return true if @p m is laid out, false otherwise.
bool
get_data_member_is_laid_out(const var_decl& m)
{
  ABG_ASSERT(is_data_member(m));
  const dm_context_rel* ctxt_rel =
    dynamic_cast<const dm_context_rel*>(m.get_context_rel());

  return ctxt_rel->get_is_laid_out();
}

/// Test whether a data member is laid out.
///
/// @param m the data member to consider.
///
/// @return true if @p m is laid out, false otherwise.
bool
get_data_member_is_laid_out(const var_decl_sptr m)
{return get_data_member_is_laid_out(*m);}

/// Test whether a function_decl is a member function.
///
/// @param f the function_decl to test.
///
/// @return true if @p f is a member function, false otherwise.
bool
is_member_function(const function_decl& f)
{return is_member_decl(f);}

/// Test whether a function_decl is a member function.
///
/// @param f the function_decl to test.
///
/// @return true if @p f is a member function, false otherwise.
bool
is_member_function(const function_decl* f)
{return is_member_decl(*f);}

/// Test whether a function_decl is a member function.
///
/// @param f the function_decl to test.
///
/// @return true if @p f is a member function, false otherwise.
bool
is_member_function(const function_decl_sptr& f)
{return is_member_decl(*f);}

/// Test whether a member function is a constructor.
///
/// @param f the member function to test.
///
/// @return true if @p f is a constructor, false otherwise.
bool
get_member_function_is_ctor(const function_decl& f)
{
  ABG_ASSERT(is_member_function(f));

  const method_decl* m = is_method_decl(&f);
  ABG_ASSERT(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->is_constructor();
}

/// Test whether a member function is a constructor.
///
/// @param f the member function to test.
///
/// @return true if @p f is a constructor, false otherwise.
bool
get_member_function_is_ctor(const function_decl_sptr& f)
{return get_member_function_is_ctor(*f);}


/// Setter for the is_ctor property of the member function.
///
/// @param f the member function to set.
///
/// @param f the new boolean value of the is_ctor property.  Is true
/// if @p f is a constructor, false otherwise.
void
set_member_function_is_ctor(function_decl& f, bool c)
{
  ABG_ASSERT(is_member_function(f));

  method_decl* m = is_method_decl(&f);
  ABG_ASSERT(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->is_constructor(c);
}

/// Setter for the is_ctor property of the member function.
///
/// @param f the member function to set.
///
/// @param f the new boolean value of the is_ctor property.  Is true
/// if @p f is a constructor, false otherwise.
void
set_member_function_is_ctor(const function_decl_sptr& f, bool c)
{set_member_function_is_ctor(*f, c);}

/// Test whether a member function is a destructor.
///
/// @param f the function to test.
///
/// @return true if @p f is a destructor, false otherwise.
bool
get_member_function_is_dtor(const function_decl& f)
{
  ABG_ASSERT(is_member_function(f));

  const method_decl* m = is_method_decl(&f);
  ABG_ASSERT(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->is_destructor();
}

/// Test whether a member function is a destructor.
///
/// @param f the function to test.
///
/// @return true if @p f is a destructor, false otherwise.
bool
get_member_function_is_dtor(const function_decl_sptr& f)
{return get_member_function_is_dtor(*f);}

/// Set the destructor-ness property of a member function.
///
/// @param f the function to set.
///
/// @param d true if @p f is a destructor, false otherwise.
void
set_member_function_is_dtor(function_decl& f, bool d)
{
    ABG_ASSERT(is_member_function(f));

    method_decl* m = is_method_decl(&f);
  ABG_ASSERT(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->is_destructor(d);
}

/// Set the destructor-ness property of a member function.
///
/// @param f the function to set.
///
/// @param d true if @p f is a destructor, false otherwise.
void
set_member_function_is_dtor(const function_decl_sptr& f, bool d)
{set_member_function_is_dtor(*f, d);}

/// Test whether a member function is const.
///
/// @param f the function to test.
///
/// @return true if @p f is const, false otherwise.
bool
get_member_function_is_const(const function_decl& f)
{
  ABG_ASSERT(is_member_function(f));

  const method_decl* m = is_method_decl(&f);
  ABG_ASSERT(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->is_const();
}

/// Test whether a member function is const.
///
/// @param f the function to test.
///
/// @return true if @p f is const, false otherwise.
bool
get_member_function_is_const(const function_decl_sptr& f)
{return get_member_function_is_const(*f);}

/// set the const-ness property of a member function.
///
/// @param f the function to set.
///
/// @param is_const the new value of the const-ness property of @p f
void
set_member_function_is_const(function_decl& f, bool is_const)
{
  ABG_ASSERT(is_member_function(f));

  method_decl* m = is_method_decl(&f);
  ABG_ASSERT(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->is_const(is_const);
}

/// set the const-ness property of a member function.
///
/// @param f the function to set.
///
/// @param is_const the new value of the const-ness property of @p f
void
set_member_function_is_const(const function_decl_sptr& f, bool is_const)
{set_member_function_is_const(*f, is_const);}

/// Test if a virtual member function has a vtable offset set.
///
/// @param f the virtual member function to consider.
///
/// @return true iff the virtual member function has its vtable offset
/// set, i.e, if the vtable offset of @p is different from -1.
bool
member_function_has_vtable_offset(const function_decl& f)
{return get_member_function_vtable_offset(f) != -1;}

/// Get the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @return the vtable offset of @p f.  Note that a vtable offset of
/// value -1 means that the member function does *NOT* yet have a
/// vtable offset associated to it.
ssize_t
get_member_function_vtable_offset(const function_decl& f)
{
  ABG_ASSERT(is_member_function(f));

  const method_decl* m =
    dynamic_cast<const method_decl*>(&f);
  ABG_ASSERT(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->vtable_offset();
}

/// Get the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @return the vtable offset of @p f.  Note that a vtable offset of
/// value -1 means that the member function does *NOT* yet have a
/// vtable offset associated to it.
ssize_t
get_member_function_vtable_offset(const function_decl_sptr& f)
{return get_member_function_vtable_offset(*f);}

/// Set the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @param s the new vtable offset.  Please note that a vtable offset
/// of value -1 means that the virtual member function does not (yet)
/// have any vtable offset associated to it.
void
set_member_function_vtable_offset(function_decl& f, ssize_t s)
{
  ABG_ASSERT(is_member_function(f));

  method_decl* m = is_method_decl(&f);
  ABG_ASSERT(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->vtable_offset(s);
}

/// Get the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @param s the new vtable offset.  Please note that a vtable offset
/// of value -1 means that the virtual member function does not (yet)
/// have any vtable offset associated to it.
void
set_member_function_vtable_offset(const function_decl_sptr& f, ssize_t s)
{return set_member_function_vtable_offset(*f, s);}

/// Test if a given member function is virtual.
///
/// @param mem_fn the member function to consider.
///
/// @return true iff a @p mem_fn is virtual.
bool
get_member_function_is_virtual(const function_decl& f)
{
  ABG_ASSERT(is_member_function(f));

  const method_decl* m =
    dynamic_cast<const method_decl*>(&f);
  ABG_ASSERT(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->is_virtual();
}

/// Test if a given member function is virtual.
///
/// @param mem_fn the member function to consider.
///
/// @return true iff a @p mem_fn is virtual.
bool
get_member_function_is_virtual(const function_decl_sptr& mem_fn)
{return mem_fn ? get_member_function_is_virtual(*mem_fn) : false;}

/// Test if a given member function is virtual.
///
/// @param mem_fn the member function to consider.
///
/// @return true iff a @p mem_fn is virtual.
bool
get_member_function_is_virtual(const function_decl* mem_fn)
{return mem_fn ? get_member_function_is_virtual(*mem_fn) : false;}

/// Set the virtual-ness of a member function.
///
/// @param f the member function to consider.
///
/// @param is_virtual set to true if the function is virtual.
void
set_member_function_is_virtual(function_decl& f, bool is_virtual)
{
  ABG_ASSERT(is_member_function(f));

  method_decl* m = is_method_decl(&f);
  ABG_ASSERT(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->is_virtual(is_virtual);
}

/// Set the virtual-ness of a member function.
///
/// @param f the member function to consider.
///
/// @param is_virtual set to true if the function is virtual.
void
set_member_function_is_virtual(const function_decl_sptr& fn, bool is_virtual)
{
  if (fn)
    {
      set_member_function_is_virtual(*fn, is_virtual);
      fixup_virtual_member_function
	(dynamic_pointer_cast<method_decl>(fn));
    }
}

/// Recursively returns the the underlying type of a typedef.  The
/// return type should not be a typedef of anything anymore.
///
///
/// Also recursively strip typedefs from the sub-types of the type
/// given in arguments.
///
/// Note that this function builds types in which typedefs are
/// stripped off.  Usually, types are held by their scope, so their
/// life time is bound to the life time of their scope.  But as this
/// function cannot really insert the built type into it's scope, it
/// must ensure that the newly built type stays live long enough.
///
/// So, if the newly built type has a canonical type, this function
/// returns the canonical type.  Otherwise, this function ensure that
/// the newly built type has a life time that is the same as the life
/// time of the entire libabigail library.
///
/// @param type the type to strip the typedefs from.
///
/// @return the resulting type stripped from its typedefs, or just
/// return @p type if it has no typedef in any of its sub-types.
type_base_sptr
strip_typedef(const type_base_sptr type)
{
  if (!type)
    return type;

  // If type is a class type then do not try to strip typedefs from it.
  // And if it has no canonical type (which can mean that it's a
  // declaration-only class), then, make sure its live for ever and
  // return it.
  if (class_decl_sptr cl = is_class_type(type))
    {
      if (!cl->get_canonical_type())
	keep_type_alive(type);
      return type;
    }

  environment* env = type->get_environment();
  ABG_ASSERT(env);
  type_base_sptr t = type;

  if (const typedef_decl_sptr ty = is_typedef(t))
    t = strip_typedef(type_or_void(ty->get_underlying_type(), env));
  else if (const reference_type_def_sptr ty = is_reference_type(t))
    {
      type_base_sptr p = strip_typedef(type_or_void(ty->get_pointed_to_type(),
						    env));
      ABG_ASSERT(p);
      t.reset(new reference_type_def(p,
				     ty->is_lvalue(),
				     ty->get_size_in_bits(),
				     ty->get_alignment_in_bits(),
				     ty->get_location()));
    }
  else if (const pointer_type_def_sptr ty = is_pointer_type(t))
    {
      type_base_sptr p = strip_typedef(type_or_void(ty->get_pointed_to_type(),
						    env));
      ABG_ASSERT(p);
      t.reset(new pointer_type_def(p,
				   ty->get_size_in_bits(),
				   ty->get_alignment_in_bits(),
				   ty->get_location()));
    }
  else if (const qualified_type_def_sptr ty = is_qualified_type(t))
    {
      type_base_sptr p = strip_typedef(type_or_void(ty->get_underlying_type(),
						    env));
      ABG_ASSERT(p);
      t.reset(new qualified_type_def(p,
				     ty->get_cv_quals(),
				     ty->get_location()));
    }
  else if (const array_type_def_sptr ty = is_array_type(t))
    {
      type_base_sptr p = strip_typedef(ty->get_element_type());
      ABG_ASSERT(p);
      t.reset(new array_type_def(p, ty->get_subranges(), ty->get_location()));
    }
  else if (const method_type_sptr ty = is_method_type(t))
    {
      function_decl::parameters parm;
      for (function_decl::parameters::const_iterator i =
	     ty->get_parameters().begin();
	   i != ty->get_parameters().end();
	   ++i)
	{
	  function_decl::parameter_sptr p = *i;
	  type_base_sptr typ = strip_typedef(p->get_type());
	  ABG_ASSERT(typ);
	  function_decl::parameter_sptr stripped
	    (new function_decl::parameter(typ,
					  p->get_index(),
					  p->get_name(),
					  p->get_location(),
					  p->get_variadic_marker(),
					  p->get_is_artificial()));
	  parm.push_back(stripped);
	}
      type_base_sptr p = strip_typedef(ty->get_return_type());
      ABG_ASSERT(!!p == !!ty->get_return_type());
      t.reset(new method_type(p, ty->get_class_type(),
			      parm, ty->get_is_const(),
			      ty->get_size_in_bits(),
			      ty->get_alignment_in_bits()));
    }
  else if (const function_type_sptr ty = is_function_type(t))
    {
      function_decl::parameters parm;
      for (function_decl::parameters::const_iterator i =
	     ty->get_parameters().begin();
	   i != ty->get_parameters().end();
	   ++i)
	{
	  function_decl::parameter_sptr p = *i;
	  type_base_sptr typ = strip_typedef(p->get_type());
	  ABG_ASSERT(typ);
	  function_decl::parameter_sptr stripped
	    (new function_decl::parameter(typ,
					  p->get_index(),
					  p->get_name(),
					  p->get_location(),
					  p->get_variadic_marker(),
					  p->get_is_artificial()));
	  parm.push_back(stripped);
	}
      type_base_sptr p = strip_typedef(ty->get_return_type());
      ABG_ASSERT(!!p == !!ty->get_return_type());
      t.reset(new function_type(p, parm,
				ty->get_size_in_bits(),
				ty->get_alignment_in_bits()));
    }

  if (!t->get_environment())
    set_environment_for_artifact(t, env);

  if (!t->get_translation_unit())
    t->set_translation_unit(type->get_translation_unit());

  if (!(type->get_canonical_type() && canonicalize(t)))
    keep_type_alive(t);

  return t->get_canonical_type() ? t->get_canonical_type() : t;
}

/// Return the leaf underlying type node of a @ref typedef_decl node.
///
/// If the underlying type of a @ref typedef_decl node is itself a
/// @ref typedef_decl node, then recursively look at the underlying
/// type nodes to get the first one that is not a a @ref typedef_decl
/// node.  This is what a leaf underlying type node means.
///
/// Otherwise, if the underlying type node of @ref typedef_decl is
/// *NOT* a @ref typedef_decl node, then just return the underlying
/// type node.
///
/// And if the type node considered is not a @ref typedef_decl node,
/// then just return it.
///
/// @return the leaf underlying type node of a @p type.
type_base_sptr
peel_typedef_type(const type_base_sptr& type)
{
  typedef_decl_sptr t = is_typedef(type);
  if (!t)
    return type;

  if (is_typedef(t->get_underlying_type()))
    return peel_typedef_type(t->get_underlying_type());
  return t->get_underlying_type();
}

/// Return the leaf underlying type node of a @ref typedef_decl node.
///
/// If the underlying type of a @ref typedef_decl node is itself a
/// @ref typedef_decl node, then recursively look at the underlying
/// type nodes to get the first one that is not a a @ref typedef_decl
/// node.  This is what a leaf underlying type node means.
///
/// Otherwise, if the underlying type node of @ref typedef_decl is
/// *NOT* a @ref typedef_decl node, then just return the underlying
/// type node.
///
/// And if the type node considered is not a @ref typedef_decl node,
/// then just return it.
///
/// @return the leaf underlying type node of a @p type.
const type_base*
peel_typedef_type(const type_base* type)
{
  const typedef_decl* t = is_typedef(type);
  if (!t)
    return type;

  return peel_typedef_type(t->get_underlying_type()).get();
}

/// Return the leaf pointed-to type node of a @ref pointer_type_def
/// node.
///
/// If the pointed-to type of a @ref pointer_type_def node is itself a
/// @ref pointer_type_def node, then recursively look at the
/// pointed-to type nodes to get the first one that is not a a @ref
/// pointer_type_def node.  This is what a leaf pointed-to type node
/// means.
///
/// Otherwise, if the pointed-to type node of @ref pointer_type_def is
/// *NOT* a @ref pointer_type_def node, then just return the
/// pointed-to type node.
///
/// And if the type node considered is not a @ref pointer_type_def
/// node, then just return it.
///
/// @return the leaf pointed-to type node of a @p type.
type_base_sptr
peel_pointer_type(const type_base_sptr& type)
{
  pointer_type_def_sptr t = is_pointer_type(type);
  if (!t)
    return type;

  if (is_pointer_type(t->get_pointed_to_type()))
    return peel_pointer_type(t->get_pointed_to_type());
  return t->get_pointed_to_type();
}

/// Return the leaf pointed-to type node of a @ref pointer_type_def
/// node.
///
/// If the pointed-to type of a @ref pointer_type_def node is itself a
/// @ref pointer_type_def node, then recursively look at the
/// pointed-to type nodes to get the first one that is not a a @ref
/// pointer_type_def node.  This is what a leaf pointed-to type node
/// means.
///
/// Otherwise, if the pointed-to type node of @ref pointer_type_def is
/// *NOT* a @ref pointer_type_def node, then just return the
/// pointed-to type node.
///
/// And if the type node considered is not a @ref pointer_type_def
/// node, then just return it.
///
/// @return the leaf pointed-to type node of a @p type.
const type_base*
peel_pointer_type(const type_base* type)
{
  const pointer_type_def* t = is_pointer_type(type);
  if (!t)
    return type;

  return peel_pointer_type(t->get_pointed_to_type()).get();
}

/// Return the leaf pointed-to type node of a @ref reference_type_def
/// node.
///
/// If the pointed-to type of a @ref reference_type_def node is itself
/// a @ref reference_type_def node, then recursively look at the
/// pointed-to type nodes to get the first one that is not a a @ref
/// reference_type_def node.  This is what a leaf pointed-to type node
/// means.
///
/// Otherwise, if the pointed-to type node of @ref reference_type_def
/// is *NOT* a @ref reference_type_def node, then just return the
/// pointed-to type node.
///
/// And if the type node considered is not a @ref reference_type_def
/// node, then just return it.
///
/// @return the leaf pointed-to type node of a @p type.
type_base_sptr
peel_reference_type(const type_base_sptr& type)
{
  reference_type_def_sptr t = is_reference_type(type);
  if (!t)
    return type;

  if (is_reference_type(t->get_pointed_to_type()))
    return peel_reference_type(t->get_pointed_to_type());
  return t->get_pointed_to_type();
}

/// Return the leaf pointed-to type node of a @ref reference_type_def
/// node.
///
/// If the pointed-to type of a @ref reference_type_def node is itself
/// a @ref reference_type_def node, then recursively look at the
/// pointed-to type nodes to get the first one that is not a a @ref
/// reference_type_def node.  This is what a leaf pointed-to type node
/// means.
///
/// Otherwise, if the pointed-to type node of @ref reference_type_def
/// is *NOT* a @ref reference_type_def node, then just return the
/// pointed-to type node.
///
/// And if the type node considered is not a @ref reference_type_def
/// node, then just return it.
///
/// @return the leaf pointed-to type node of a @p type.
const type_base*
peel_reference_type(const type_base* type)
{
  const reference_type_def* t = is_reference_type(type);
  if (!t)
    return type;

  return peel_reference_type(t->get_pointed_to_type()).get();
}

/// Return the leaf element type of an array.
///
/// If the element type is itself an array, then recursively return
/// the element type of that array itself.
///
/// @param type the array type to consider.  If this is not an array
/// type, this type is returned by the function.
///
/// @return the leaf element type of the array @p type, or, if it's
/// not an array type, then just return @p.
const type_base_sptr
peel_array_type(const type_base_sptr& type)
{
  const array_type_def_sptr t = is_array_type(type);
  if (!t)
    return type;

  return peel_array_type(t->get_element_type());
}

/// Return the leaf element type of an array.
///
/// If the element type is itself an array, then recursively return
/// the element type of that array itself.
///
/// @param type the array type to consider.  If this is not an array
/// type, this type is returned by the function.
///
/// @return the leaf element type of the array @p type, or, if it's
/// not an array type, then just return @p.
const type_base*
peel_array_type(const type_base* type)
{
  const array_type_def* t = is_array_type(type);
  if (!t)
    return type;

  return peel_array_type(t->get_element_type()).get();
}

/// Return the leaf underlying type of a qualified type.
///
/// If the underlying type is itself a qualified type, then
/// recursively return the first underlying type of that qualified
/// type to return the first underlying type that is not a qualified type.
///
/// If the underlying type is NOT a qualified type, then just return
/// that underlying type.
///
/// @param type the qualified type to consider.
///
/// @return the leaf underlying type.
const type_base*
peel_qualified_type(const type_base* type)
{
  const qualified_type_def* t = is_qualified_type(type);
  if (!t)
    return type;

  return peel_qualified_type(t->get_underlying_type().get());
}

/// Return the leaf underlying type of a qualified type.
///
/// If the underlying type is itself a qualified type, then
/// recursively return the first underlying type of that qualified
/// type to return the first underlying type that is not a qualified type.
///
/// If the underlying type is NOT a qualified type, then just return
/// that underlying type.
///
/// @param type the qualified type to consider.
///
/// @return the leaf underlying type.
const type_base_sptr
peel_qualified_type(const type_base_sptr& type)
{
  const qualified_type_def_sptr t = is_qualified_type(type);
  if (!t)
    return type;

  return peel_qualified_type(t->get_underlying_type());
}

/// Return the leaf underlying type of a qualified or typedef type.
///
/// If the underlying type is itself a qualified or typedef type, then
/// recursively return the first underlying type of that qualified or
/// typedef type to return the first underlying type that is not a
/// qualified or typedef type.
///
/// If the underlying type is NOT a qualified nor a typedef type, then
/// just return that underlying type.
///
/// @param type the qualified or typedef type to consider.
///
/// @return the leaf underlying type.
type_base*
peel_qualified_or_typedef_type(const type_base* type)
{
  while (is_typedef(type) || is_qualified_type(type))
    {
      if (const typedef_decl* t = is_typedef(type))
	type = peel_typedef_type(t);

      if (const qualified_type_def* t = is_qualified_type(type))
	type = peel_qualified_type(t);
    }

  return const_cast<type_base*>(type);
}

/// Return the leaf underlying type of a qualified or typedef type.
///
/// If the underlying type is itself a qualified or typedef type, then
/// recursively return the first underlying type of that qualified or
/// typedef type to return the first underlying type that is not a
/// qualified or typedef type.
///
/// If the underlying type is NOT a qualified nor a typedef type, then
/// just return that underlying type.
///
/// @param type the qualified or typedef type to consider.
///
/// @return the leaf underlying type.
type_base_sptr
peel_qualified_or_typedef_type(const type_base_sptr &t)
{
  type_base_sptr type = t;
  while (is_typedef(type) || is_qualified_type(type))
    {
      if (typedef_decl_sptr t = is_typedef(type))
	type = peel_typedef_type(t);

      if (qualified_type_def_sptr t = is_qualified_type(type))
	type = peel_qualified_type(t);
    }

  return type;
}

/// Return the leaf underlying or pointed-to type node of a @ref
/// typedef_decl, @ref pointer_type_def or @ref reference_type_def
/// node.
///
/// @param type the type to peel.
///
/// @return the leaf underlying or pointed-to type node of @p type.
type_base_sptr
peel_typedef_pointer_or_reference_type(const type_base_sptr type)
{
  type_base_sptr typ = type;
  while (is_typedef(typ)
	 || is_pointer_type(typ)
	 || is_reference_type(typ))
    {
      if (typedef_decl_sptr t = is_typedef(typ))
	typ = peel_typedef_type(t);

      if (pointer_type_def_sptr t = is_pointer_type(typ))
	typ = peel_pointer_type(t);

      if (reference_type_def_sptr t = is_reference_type(typ))
	typ = peel_reference_type(t);
    }

  return typ;
}

/// Return the leaf underlying or pointed-to type node of a @ref
/// typedef_decl, @ref pointer_type_def or @ref reference_type_def
/// node.
///
/// @param type the type to peel.
///
/// @return the leaf underlying or pointed-to type node of @p type.
type_base*
peel_typedef_pointer_or_reference_type(const type_base* type)
{
  while (is_typedef(type)
	 || is_pointer_type(type)
	 || is_reference_type(type))
    {
      if (const typedef_decl* t = is_typedef(type))
	type = peel_typedef_type(t);

      if (const pointer_type_def* t = is_pointer_type(type))
	type = peel_pointer_type(t);

      if (const reference_type_def* t = is_reference_type(type))
	type = peel_reference_type(t);
    }

  return const_cast<type_base*>(type);
}

/// Return the leaf underlying or pointed-to type node of a, @ref
/// pointer_type_def, @ref reference_type_def or @ref
/// qualified_type_def type node.
///
/// @param type the type to peel.
///
/// @param peel_qualified_type if true, also peel qualified types.
///
/// @return the leaf underlying or pointed-to type node of @p type.
type_base*
peel_pointer_or_reference_type(const type_base *type,
			       bool peel_qual_type)
{
  while (is_pointer_type(type)
	 || is_reference_type(type)
	 || (peel_qual_type && is_qualified_type(type)))
    {
      if (const pointer_type_def* t = is_pointer_type(type))
	type = peel_pointer_type(t);

      if (const reference_type_def* t = is_reference_type(type))
	type = peel_reference_type(t);

      if (const array_type_def* t = is_array_type(type))
	type = peel_array_type(t);

      if (peel_qual_type)
	if (const qualified_type_def* t = is_qualified_type(type))
	  type = peel_qualified_type(t);
    }

  return const_cast<type_base*>(type);
}

/// Clone an array type.
///
/// Note that the element type of the new array is shared witht the
/// old one.
///
/// @param array the array type to clone.
///
/// @return a newly built array type.  Note that it needs to be added
/// to a scope (e.g, using add_decl_to_scope) for its lifetime to be
/// bound to the one of that scope.  Otherwise, its lifetime is bound
/// to the lifetime of its containing shared pointer.
array_type_def_sptr
clone_array(const array_type_def_sptr& array)
{
  vector<array_type_def::subrange_sptr> subranges;

  for (vector<array_type_def::subrange_sptr>::const_iterator i =
	 array->get_subranges().begin();
       i != array->get_subranges().end();
       ++i)
    {
      array_type_def::subrange_sptr subrange
	(new array_type_def::subrange_type(array->get_environment(),
					   (*i)->get_name(),
					   (*i)->get_lower_bound(),
					   (*i)->get_upper_bound(),
					   (*i)->get_underlying_type(),
					   (*i)->get_location(),
					   (*i)->get_language()));
      subrange->is_infinite((*i)->is_infinite());
      if (scope_decl *scope = (*i)->get_scope())
	add_decl_to_scope(subrange, scope);
      subranges.push_back(subrange);
    }

  array_type_def_sptr result
    (new array_type_def(array->get_element_type(),
			subranges, array->get_location()));

  return result;
}

/// Clone a typedef type.
///
/// Note that the underlying type of the newly constructed typedef is
/// shared with the old one.
///
/// @param t the typedef to clone.
///
/// @return the newly constructed typedef.  Note that it needs to be
/// added to a scope (e.g, using add_decl_to_scope) for its lifetime
/// to be bound to the one of that scope.  Otherwise, its lifetime is
/// bound to the lifetime of its containing shared pointer.
typedef_decl_sptr
clone_typedef(const typedef_decl_sptr& t)
{
  if (!t)
    return t;

  typedef_decl_sptr result
    (new typedef_decl(t->get_name(), t->get_underlying_type(),
		      t->get_location(), t->get_linkage_name(),
		      t->get_visibility()));
  return result;
}

/// Clone a qualifiend type.
///
/// Note that underlying type of the newly constructed qualified type
/// is shared with the old one.
///
/// @param t the qualified type to clone.
///
/// @return the newly constructed qualified type.  Note that it needs
/// to be added to a scope (e.g, using add_decl_to_scope) for its
/// lifetime to be bound to the one of that scope.  Otherwise, its
/// lifetime is bound to the lifetime of its containing shared
/// pointer.
qualified_type_def_sptr
clone_qualified_type(const qualified_type_def_sptr& t)
{
  if (!t)
    return t;

  qualified_type_def_sptr result
    (new qualified_type_def(t->get_underlying_type(),
			    t->get_cv_quals(), t->get_location()));

  return result;
}

/// Clone a typedef, an array or a qualified tree.
///
/// @param type the typedef, array or qualified tree to clone.  any
/// order.
///
/// @return the cloned type, or NULL if @type was neither a typedef,
/// array nor a qualified type.
static type_base_sptr
clone_typedef_array_qualified_type(type_base_sptr type)
{
  if (!type)
    return type;

  scope_decl* scope = is_decl(type) ? is_decl(type)->get_scope() : 0;
  type_base_sptr result;

  if (typedef_decl_sptr t = is_typedef(type))
    result = clone_typedef(is_typedef(t));
  else if (qualified_type_def_sptr t = is_qualified_type(type))
    result = clone_qualified_type(t);
  else if (array_type_def_sptr t = is_array_type(type))
    result = clone_array(t);
  else
    return type_base_sptr();

  if (scope)
    add_decl_to_scope(is_decl(result), scope);

  return result;
}

/// Clone a type tree made of an array or a typedef of array.
///
/// Note that this can be a tree which root node is a typedef an which
/// sub-tree can be any arbitrary combination of typedef, qualified
/// type and arrays.
///
/// @param t the array or typedef of qualified array to consider.
///
/// @return a clone of @p t.
type_base_sptr
clone_array_tree(const type_base_sptr t)
{
  ABG_ASSERT(is_typedef_of_array(t) || is_array_type(t));

  scope_decl* scope = is_decl(t)->get_scope();
  type_base_sptr result = clone_typedef_array_qualified_type(t);
  ABG_ASSERT(is_typedef_of_array(result) || is_array_type(result));

  type_base_sptr subtree;
  if (typedef_decl_sptr type = is_typedef(result))
    {
      type_base_sptr s =
	clone_typedef_array_qualified_type(type->get_underlying_type());
      if (s)
	{
	  subtree = s;
	  type->set_underlying_type(subtree);
	}
    }
  else if (array_type_def_sptr type = is_array_type(result))
    {
      type_base_sptr s =
	clone_typedef_array_qualified_type(type->get_element_type());
      if (s)
	{
	  subtree = s;
	  type->set_element_type(subtree);
	}
    }
  add_decl_to_scope(is_decl(subtree), scope);

  for (;;)
    {
      if (typedef_decl_sptr t = is_typedef(subtree))
	{
	  type_base_sptr s =
	    clone_typedef_array_qualified_type(t->get_underlying_type());
	  if (s)
	    {
	      scope_decl* scope =
		is_decl(t->get_underlying_type())->get_scope();
	      ABG_ASSERT(scope);
	      add_decl_to_scope(is_decl(s), scope);
	      t->set_underlying_type (s);
	      subtree = s;
	    }
	  else
	    break;
	}
      else if (qualified_type_def_sptr t = is_qualified_type(subtree))
	{
	  type_base_sptr s =
	    clone_typedef_array_qualified_type(t->get_underlying_type());
	  if (s)
	    {
	      scope_decl* scope =
		is_decl(t->get_underlying_type())->get_scope();
	      ABG_ASSERT(scope);
	      add_decl_to_scope(is_decl(s), scope);
	      t->set_underlying_type(s);
	      subtree = s;
	    }
	  else
	    break;
	}
      else if (array_type_def_sptr t = is_array_type(subtree))
	{
	  type_base_sptr e = t->get_element_type();
	  if (is_typedef(e) || is_qualified_type(e))
	    {
	      type_base_sptr s =
		clone_typedef_array_qualified_type(e);
	      if (s)
		{
		  scope_decl* scope = is_decl(e)->get_scope();
		  ABG_ASSERT(scope);
		  add_decl_to_scope(is_decl(s), scope);
		  t->set_element_type(s);
		}
	      else
		break;
	    }
	  break;
	}
      else
	break;
    }
  return result;
}

/// Update the qualified name of a given sub-tree.
///
/// @param d the sub-tree for which to update the qualified name.
static void
update_qualified_name(decl_base * d)
{
  ::qualified_name_setter setter;
  d->traverse(setter);
}

/// Update the qualified name of a given sub-tree.
///
/// @param d the sub-tree for which to update the qualified name.
static void
update_qualified_name(decl_base_sptr d)
{return update_qualified_name(d.get());}

// <scope_decl stuff>

/// Hash a type by returning the pointer value of its canonical type.
///
/// @param l the type to hash.
///
/// @return the the pointer value of the canonical type of @p l.
size_t
canonical_type_hash::operator()(const type_base_sptr& l) const
{return operator()(l.get());}

/// Hash a (canonical) type by returning its pointer value
///
/// @param l the canonical type to hash.
///
/// @return the pointer value of the canonical type of @p l.
size_t
canonical_type_hash::operator()(const type_base *l) const
{return reinterpret_cast<size_t>(l);}

struct scope_decl::priv
{
  declarations members_;
  declarations sorted_members_;
  scopes member_scopes_;
  canonical_type_sptr_set_type canonical_types_;
  type_base_sptrs_type sorted_canonical_types_;
}; // end struct scope_decl::priv

/// Constructor of the @ref scope_decl type.
///
/// @param the environment to use for the new instance.
///
/// @param the name of the scope decl.
///
/// @param locus the source location where the scope_decl is defined.
///
/// @param vis the visibility of the declaration.
scope_decl::scope_decl(const environment* env,
		       const string& name,
		       const location& locus,
		       visibility vis)
  : type_or_decl_base(env, ABSTRACT_SCOPE_DECL|ABSTRACT_DECL_BASE),
    decl_base(env, name, locus, /*mangled_name=*/name, vis),
    priv_(new priv)
{}

/// Constructor of the @ref scope_decl type.
///
/// @param the environment to use for the new instance.
///
/// @param l the source location where the scope_decl is defined.
///
/// @param vis the visibility of the declaration.
scope_decl::scope_decl(const environment* env, location& l)
  : type_or_decl_base(env, ABSTRACT_SCOPE_DECL|ABSTRACT_DECL_BASE),
    decl_base(env, "", l),
    priv_(new priv)
{}

/// @eturn the set of canonical types of the the current scope.
canonical_type_sptr_set_type&
scope_decl::get_canonical_types()
{return priv_->canonical_types_;}

/// @eturn the set of canonical types of the the current scope.
const canonical_type_sptr_set_type&
scope_decl::get_canonical_types() const
{return const_cast<scope_decl*>(this)->get_canonical_types();}

/// Return a vector of sorted canonical types of the current scope.
///
/// The types are sorted "almost topologically". That means, they are
/// sorted using the lexicographic order of the string representing
/// the location their definition point.  If a type doesn't have a
/// location, then its pretty representation is used.
///
/// @return a vector of sorted canonical types of the current scope.
const type_base_sptrs_type&
scope_decl::get_sorted_canonical_types() const
{
  if (priv_->sorted_canonical_types_.empty())
    {
      for (canonical_type_sptr_set_type::const_iterator e =
	     get_canonical_types().begin();
	   e != get_canonical_types().end();
	   ++e)
	priv_->sorted_canonical_types_.push_back(*e);

      type_topo_comp comp;
      std::stable_sort(priv_->sorted_canonical_types_.begin(),
		       priv_->sorted_canonical_types_.end(),
		       comp);
    }
  return priv_->sorted_canonical_types_;
}

/// Getter for the member declarations carried by the current @ref
/// scope_decl.
///
/// @return the member declarations carried by the current @ref
/// scope_decl.
const scope_decl::declarations&
scope_decl::get_member_decls() const
{return priv_->members_;}

/// Getter for the member declarations carried by the current @ref
/// scope_decl.
///
/// @return the member declarations carried by the current @ref
/// scope_decl.
scope_decl::declarations&
scope_decl::get_member_decls()
{return priv_->members_;}

/// Getter for the sorted member declarations carried by the current
/// @ref scope_decl.
///
/// @return the sorted member declarations carried by the current @ref
/// scope_decl.  The declarations are sorted topologically.
const scope_decl::declarations&
scope_decl::get_sorted_member_decls() const
{
  decl_topo_comp comp;
  if (priv_->sorted_members_.empty())
    {
      for (declarations::const_iterator i = get_member_decls().begin();
	   i != get_member_decls().end();
	   ++i)
	priv_->sorted_members_.push_back(*i);

      std::stable_sort(priv_->sorted_members_.begin(),
		       priv_->sorted_members_.end(),
		       comp);
    }
  return priv_->sorted_members_;
}

/// Getter for the number of anonymous classes contained in this
/// scope.
///
/// @return the number of anonymous classes contained in this scope.
size_t
scope_decl::get_num_anonymous_member_classes() const
{
  int result = 0;
  for (declarations::const_iterator it = get_member_decls().begin();
       it != get_member_decls().end();
       ++it)
    if (class_decl_sptr t = is_class_type(*it))
      if (t->get_is_anonymous())
	++result;

  return result;
}

/// Getter for the number of anonymous unions contained in this
/// scope.
///
/// @return the number of anonymous unions contained in this scope.
size_t
scope_decl::get_num_anonymous_member_unions() const
{
  int result = 0;
  for (declarations::const_iterator it = get_member_decls().begin();
       it != get_member_decls().end();
       ++it)
    if (union_decl_sptr t = is_union_type(*it))
      if (t->get_is_anonymous())
	++result;

  return result;
}

/// Getter for the number of anonymous enums contained in this
/// scope.
///
/// @return the number of anonymous enums contained in this scope.
size_t
scope_decl::get_num_anonymous_member_enums() const
{
  int result = 0;
  for (declarations::const_iterator it = get_member_decls().begin();
       it != get_member_decls().end();
       ++it)
    if (enum_type_decl_sptr t = is_enum_type(*it))
      if (t->get_is_anonymous())
	++result;

  return result;
}

/// Getter for the scopes carried by the current scope.
///
/// @return the scopes carried by the current scope.
scope_decl::scopes&
scope_decl::get_member_scopes()
{return priv_->member_scopes_;}

/// Getter for the scopes carried by the current scope.
///
/// @return the scopes carried by the current scope.
const scope_decl::scopes&
scope_decl::get_member_scopes() const
{return priv_->member_scopes_;}

/// Test if the current scope is empty.
///
/// @return true iff the current scope is empty.
bool
scope_decl::is_empty() const
{
  return (get_member_decls().empty()
	  && get_canonical_types().empty());
}

/// Add a member decl to this scope.  Note that user code should not
/// use this, but rather use add_decl_to_scope.
///
/// Note that this function updates the qualified name of the member
/// decl that is added.  It also sets the scope of the member.  Thus,
/// it ABG_ASSERTs that member should not have its scope set, prior to
/// calling this function.
///
/// @param member the new member decl to add to this scope.
decl_base_sptr
scope_decl::add_member_decl(const decl_base_sptr& member)
{
  ABG_ASSERT(!has_scope(member));

  member->set_scope(this);
  priv_->members_.push_back(member);

  if (scope_decl_sptr m = dynamic_pointer_cast<scope_decl>(member))
    priv_->member_scopes_.push_back(m);

  update_qualified_name(member);

  // Propagate scope anonymity
  if (get_has_anonymous_parent()
      || (!is_global_scope(this) && get_is_anonymous()))
    member->set_has_anonymous_parent(true);

  if (const environment* env = get_environment())
    set_environment_for_artifact(member, env);

  if (translation_unit* tu = get_translation_unit())
    {
      if (translation_unit* existing_tu = member->get_translation_unit())
	ABG_ASSERT(tu == existing_tu);
      else
	member->set_translation_unit(tu);
    }

  maybe_update_types_lookup_map(member);

  return member;
}

/// Insert a member decl to this scope, right before an element
/// pointed to by a given iterator.  Note that user code should not
/// use this, but rather use insert_decl_into_scope.
///
/// Note that this function updates the qualified name of the inserted
/// member.
///
/// @param member the new member decl to add to this scope.
///
/// @param before an interator pointing to the element before which
/// the new member should be inserted.
decl_base_sptr
scope_decl::insert_member_decl(decl_base_sptr member,
			       declarations::iterator before)
{
  ABG_ASSERT(!member->get_scope());

  member->set_scope(this);
  priv_->members_.insert(before, member);

  if (scope_decl_sptr m = dynamic_pointer_cast<scope_decl>(member))
   priv_-> member_scopes_.push_back(m);

  update_qualified_name(member);

  if (const environment* env = get_environment())
    set_environment_for_artifact(member, env);

  if (translation_unit* tu = get_translation_unit())
    {
      if (translation_unit* existing_tu = member->get_translation_unit())
	ABG_ASSERT(tu == existing_tu);
      else
	member->set_translation_unit(tu);
    }

  maybe_update_types_lookup_map(member);

  return member;
}

/// Remove a declaration from the current scope.
///
/// @param member the declaration to remove from the scope.
void
scope_decl::remove_member_decl(decl_base_sptr member)
{
  for (declarations::iterator i = priv_->members_.begin();
       i != priv_->members_.end();
       ++i)
    {
      if (**i == *member)
	{
	  priv_->members_.erase(i);
	  // Do not access i after this point as it's invalided by the
	  // erase call.
	  break;
	}
    }

  scope_decl_sptr scope = dynamic_pointer_cast<scope_decl>(member);
  if (scope)
    {
      for (scopes::iterator i = priv_->member_scopes_.begin();
	   i != priv_->member_scopes_.end();
	   ++i)
	{
	  if (**i == *member)
	    {
	      priv_->member_scopes_.erase(i);
	      break;
	    }
	}
    }
}

/// Return the hash value for the current instance of scope_decl.
///
/// This method can trigger the computing of the hash value, if need be.
///
/// @return the hash value.
size_t
scope_decl::get_hash() const
{
  scope_decl::hash hash_scope;
  return hash_scope(this);
}

/// Compares two instances of @ref scope_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const scope_decl& l, const scope_decl& r, change_kind* k)
{
  bool result = true;

  if (!l.decl_base::operator==(r))
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }

  scope_decl::declarations::const_iterator i, j;
  for (i = l.get_member_decls().begin(), j = r.get_member_decls().begin();
       i != l.get_member_decls().end() && j != r.get_member_decls().end();
       ++i, ++j)
    {
      if (**i != **j)
	{
	  result = false;
	  if (k)
	    {
	      *k |= SUBTYPE_CHANGE_KIND;
	      break;
	    }
	  else
	    return false;
	}
    }

  if (i != l.get_member_decls().end() || j != r.get_member_decls().end())
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

/// Return true iff both scopes have the same names and have the same
/// member decls.
///
/// This function doesn't check for equality of the scopes of its
/// arguments.
bool
scope_decl::operator==(const decl_base& o) const
{
  const scope_decl* other = dynamic_cast<const scope_decl*>(&o);
  if (!other)
    return false;

  return equals(*this, *other, 0);
}

/// Equality operator for @ref scope_decl_sptr.
///
/// @param l the left hand side operand of the equality operator.
///
/// @pram r the right hand side operand of the equalify operator.
///
/// @return true iff @p l equals @p r.
bool
operator==(const scope_decl_sptr& l, const scope_decl_sptr& r)
{
  if (!!l != !!r)
    return false;
  if (l.get() == r.get())
    return true;
  return *l == *r;
}

/// Inequality operator for @ref scope_decl_sptr.
///
/// @param l the left hand side operand of the equality operator.
///
/// @pram r the right hand side operand of the equalify operator.
///
/// @return true iff @p l equals @p r.
bool
operator!=(const scope_decl_sptr& l, const scope_decl_sptr& r)
{return !operator==(l, r);}

/// Find a member of the current scope and return an iterator on it.
///
/// @param decl the scope member to find.
///
/// @param i the iterator to set to the member @p decl.  This is set
/// iff the function returns true.
///
/// @return true if the member decl was found, false otherwise.
bool
scope_decl::find_iterator_for_member(const decl_base* decl,
				     declarations::iterator& i)
{
  if (!decl)
    return false;

  if (get_member_decls().empty())
    {
      i = get_member_decls().end();
      return false;
    }

  for (declarations::iterator it = get_member_decls().begin();
       it != get_member_decls().end();
       ++it)
    {
      if ((*it).get() == decl)
	{
	  i = it;
	  return true;
	}
    }

  return false;
}

/// Find a member of the current scope and return an iterator on it.
///
/// @param decl the scope member to find.
///
/// @param i the iterator to set to the member @p decl.  This is set
/// iff the function returns true.
///
/// @return true if the member decl was found, false otherwise.
bool
scope_decl::find_iterator_for_member(const decl_base_sptr decl,
				     declarations::iterator& i)
{return find_iterator_for_member(decl.get(), i);}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance of scope_decl
/// and on its member nodes.
///
/// @return true if the traversal of the tree should continue, false
/// otherwise.
bool
scope_decl::traverse(ir_node_visitor &v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      for (scope_decl::declarations::const_iterator i =
	     get_member_decls().begin();
	   i != get_member_decls ().end();
	   ++i)
	if (!(*i)->traverse(v))
	  break;
      visiting(false);
    }
  return v.visit_end(this);
}

scope_decl::~scope_decl()
{}

/// Appends a declaration to a given scope, if the declaration
/// doesn't already belong to one.
///
/// @param decl the declaration to add to the scope
///
/// @param scope the scope to append the declaration to
decl_base_sptr
add_decl_to_scope(decl_base_sptr decl, scope_decl* scope)
{
  ABG_ASSERT(scope);

  if (scope && decl && !decl->get_scope())
    decl = scope->add_member_decl(decl);

  return decl;
}

/// Appends a declaration to a given scope, if the declaration doesn't
/// already belong to a scope.
///
/// @param decl the declaration to add append to the scope
///
/// @param scope the scope to append the decl to
decl_base_sptr
add_decl_to_scope(decl_base_sptr decl, const scope_decl_sptr& scope)
{return add_decl_to_scope(decl, scope.get());}

/// Remove a given decl from its scope
///
/// @param decl the decl to remove from its scope.
void
remove_decl_from_scope(decl_base_sptr decl)
{
  if (!decl)
    return;

  scope_decl* scope = decl->get_scope();
  scope->remove_member_decl(decl);
  decl->set_scope(0);
}

/// Inserts a declaration into a given scope, before a given IR child
/// node of the scope.
///
/// @param decl the declaration to insert into the scope.
///
/// @param before an iterator pointing to the child IR node before
/// which to insert the declaration.
///
/// @param scope the scope into which to insert the declaration.
decl_base_sptr
insert_decl_into_scope(decl_base_sptr decl,
		       scope_decl::declarations::iterator before,
		       scope_decl* scope)
{
  if (scope && decl && !decl->get_scope())
    {
      decl_base_sptr d = scope->insert_member_decl(decl, before);
      decl = d;
    }
  return decl;
}

/// Inserts a declaration into a given scope, before a given IR child
/// node of the scope.
///
/// @param decl the declaration to insert into the scope.
///
/// @param before an iterator pointing to the child IR node before
/// which to insert the declaration.
///
/// @param scope the scope into which to insert the declaration.
decl_base_sptr
insert_decl_into_scope(decl_base_sptr decl,
		       scope_decl::declarations::iterator before,
		       scope_decl_sptr scope)
{return insert_decl_into_scope(decl, before, scope.get());}

/// Constructor of the @ref global_scope type.
///
/// @param tu the translation unit the scope belongs to.
global_scope::global_scope(translation_unit *tu)
  : type_or_decl_base(tu->get_environment(),
		      GLOBAL_SCOPE_DECL
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_DECL),
    decl_base(tu->get_environment(), "", location()),
    scope_decl(tu->get_environment(), "", location()),
    translation_unit_(tu)
{
  runtime_type_instance(this);
}

/// return the global scope as seen by a given declaration.
///
/// @param decl the declaration to consider.
///
/// @return the global scope of the decl, or a null pointer if the
/// decl is not yet added to a translation_unit.
const global_scope*
get_global_scope(const decl_base& decl)
{
  if (const global_scope* s = dynamic_cast<const global_scope*>(&decl))
    return s;

  scope_decl* scope = decl.get_scope();
  while (scope && !dynamic_cast<global_scope*>(scope))
    scope = scope->get_scope();

  return scope ? dynamic_cast<global_scope*> (scope) : 0;
}

/// return the global scope as seen by a given declaration.
///
/// @param decl the declaration to consider.
///
/// @return the global scope of the decl, or a null pointer if the
/// decl is not yet added to a translation_unit.
const global_scope*
get_global_scope(const decl_base* decl)
{return get_global_scope(*decl);}

/// Return the global scope as seen by a given declaration.
///
/// @param decl the declaration to consider.
///
/// @return the global scope of the decl, or a null pointer if the
/// decl is not yet added to a translation_unit.
const global_scope*
get_global_scope(const shared_ptr<decl_base> decl)
{return get_global_scope(decl.get());}

/// Return the a scope S containing a given declaration and that is
/// right under a given scope P.
///
/// Note that @p scope must come before @p decl in topological
/// order.
///
/// @param decl the decl for which to find a scope.
///
/// @param scope the scope under which the resulting scope must be.
///
/// @return the resulting scope.
const scope_decl*
get_top_most_scope_under(const decl_base* decl,
			 const scope_decl* scope)
{
  if (!decl)
    return 0;

  if (scope == 0)
    return get_global_scope(decl);

  // Handle the case where decl is a scope itself.
  const scope_decl* s = dynamic_cast<const scope_decl*>(decl);
  if (!s)
    s = decl->get_scope();

  if (is_global_scope(s))
    return scope;

  // Here, decl is in the scope 'scope', or decl and 'scope' are the
  // same.  The caller needs to be prepared to deal with this case.
  if (s == scope)
    return s;

  while (s && !is_global_scope(s) && s->get_scope() != scope)
    s = s->get_scope();

  if (!s || is_global_scope(s))
    // SCOPE must come before decl in topological order, but I don't
    // know how to ensure that ...
    return scope;
  ABG_ASSERT(s);

  return s;
}

/// Return the a scope S containing a given declaration and that is
/// right under a given scope P.
///
/// @param decl the decl for which to find a scope.
///
/// @param scope the scope under which the resulting scope must be.
///
/// @return the resulting scope.
const scope_decl*
get_top_most_scope_under(const decl_base_sptr decl,
			 const scope_decl* scope)
{return get_top_most_scope_under(decl.get(), scope);}

/// Return the a scope S containing a given declaration and that is
/// right under a given scope P.
///
/// @param decl the decl for which to find a scope.
///
/// @param scope the scope under which the resulting scope must be.
///
/// @return the resulting scope.
const scope_decl*
get_top_most_scope_under(const decl_base_sptr decl,
			 const scope_decl_sptr scope)
{return get_top_most_scope_under(decl, scope.get());}

// </scope_decl stuff>


/// Get the string representation of a CV qualifier bitmap.
///
/// @param cv_quals the bitmap of CV qualifiers to consider.
///
/// @return the string representation.
string
get_string_representation_of_cv_quals(const qualified_type_def::CV cv_quals)
{
  string repr;
  if (cv_quals & qualified_type_def::CV_RESTRICT)
    repr = "restrict";
  if (cv_quals & qualified_type_def::CV_CONST)
    {
      if (!repr.empty())
	repr += ' ';
      repr += "const";
    }
  if (cv_quals & qualified_type_def::CV_VOLATILE)
    {
      if (!repr.empty())
	repr += ' ';
      repr += "volatile";
    }
  return repr;
}

/// Build and return a copy of the name of an ABI artifact that is
/// either a type or a decl.
///
/// @param tod the ABI artifact to get the name for.
///
/// @param qualified if yes, return the qualified name of @p tod;
/// otherwise, return the non-qualified name;
///
/// @return the name of @p tod.
string
get_name(const type_or_decl_base *tod, bool qualified)
{
  string result;

  type_or_decl_base* a = const_cast<type_or_decl_base*>(tod);

  if (type_base* t = dynamic_cast<type_base*>(a))
    result = get_type_name(t, qualified);
  else if (decl_base *d = dynamic_cast<decl_base*>(a))
    {
      if (qualified)
	result = d->get_qualified_name();
      else
	result = d->get_name();
    }
  else
    // We should never reach this point.
    abort();

  return result;
}

/// Build and return a copy of the name of an ABI artifact that is
/// either a type of a decl.
///
/// @param tod the ABI artifact to get the name for.
///
/// @param qualified if yes, return the qualified name of @p tod;
/// otherwise, return the non-qualified name;
///
/// @return the name of @p tod.
string
get_name(const type_or_decl_base_sptr& tod, bool qualified)
{return get_name(tod.get(), qualified);}

/// Build and return a qualified name from a name and its scope.
///
/// The name is supposed to be for an entity that is part of the
/// scope.
///
/// @param the scope to consider.
///
/// @param name of the name to consider.
///
/// @return a copy of the string that represents the qualified name.
string
build_qualified_name(const scope_decl* scope, const string& name)
{
  if (name.empty())
    return "";

  string qualified_name;
  if (scope)
    qualified_name = scope->get_qualified_name();

  if (qualified_name.empty())
    qualified_name = name;
  else
    qualified_name = qualified_name + "::" + name;

  return qualified_name;
}

/// Build and return the qualified name of a type in its scope.
///
/// @param scope the scope of the type to consider.
///
/// @param type the type to consider.
string
build_qualified_name(const scope_decl* scope, const type_base_sptr& type)
{return build_qualified_name(scope, get_name((type)));}

// </scope_decl stuff>

/// Get the location of the declaration of a given type.
///
/// @param type the type to consider.
///
/// @return the location of the declaration of type @p type.
location
get_location(const type_base_sptr& type)
{
  if (decl_base_sptr decl = get_type_declaration(type))
    return get_location(decl);
  return location();
}

/// Get the location of a given declaration.
///
/// @param decl the declaration to consider.
///
/// @return the location of the declaration @p decl.
location
get_location(const decl_base_sptr& decl)
{
  location loc = decl->get_location();
  if (!loc)
    {
      if (class_or_union_sptr c = is_class_or_union_type(decl))
	if (c->get_is_declaration_only() && c->get_definition_of_declaration())
	  {
	    c = is_class_or_union_type(c->get_definition_of_declaration());
	    loc = c->get_location();
	  }
    }
  return loc;
}

/// Get the scope of a given type.
///
/// @param t the type to consider.
///
/// @return the scope of type @p t or 0 if the type has no scope yet.
scope_decl*
get_type_scope(type_base* t)
{
  if (!t)
    return 0;

  decl_base* d = get_type_declaration(t);
  if (d)
    return d->get_scope();
  return 0;
}

/// Get the scope of a given type.
///
/// @param t the type to consider.
///
/// @return the scope of type @p t or 0 if the type has no scope yet.
scope_decl*
get_type_scope(const type_base_sptr& t)
{return get_type_scope(t.get());}

/// Get the name of a given type and return a copy of it.
///
/// @param t the type to consider.
///
/// @param qualified if true then return the qualified name of the
/// type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the type name if the type has a name, or the
/// empty string if it does not.
interned_string
get_type_name(const type_base_sptr& t, bool qualified, bool internal)
{return get_type_name(t.get(), qualified, internal);}

/// Return true iff a decl is for a type type that has a generic
/// anonymous internal type name.
///
/// @param d the decl to considier.
///
/// @return true iff @p d is for a type type that has a generic
/// anonymous internal type name.
static bool
has_generic_anonymous_internal_type_name(const decl_base *d)
{
  return is_class_or_union_type(d) || is_enum_type(d);
}

/// Return the generic internal name of an anonymous type.
///
/// For internal purposes, we want to define a generic name for all
/// anonymous types of a certain kind.  For instance, all anonymous
/// structs will be have a generic name of "__anonymous_struct__", all
/// anonymous unions will have a generic name of
/// "__anonymous_union__", etc.
///
/// That generic name can be used as a hash to put all anonymous types
/// of a certain kind in the same hash table bucket, for instance.
static interned_string
get_generic_anonymous_internal_type_name(const decl_base *d)
{
  ABG_ASSERT(has_generic_anonymous_internal_type_name(d));

  const environment *env = d->get_environment();

  interned_string result;
  if (is_class_type(d))
    result =
      env->intern(tools_utils::get_anonymous_struct_internal_name_prefix());
  else if (is_union_type(d))
    result =
      env->intern(tools_utils::get_anonymous_union_internal_name_prefix());
  else if (is_enum_type(d))
    result =
      env->intern(tools_utils::get_anonymous_enum_internal_name_prefix());
  else
    ABG_ASSERT_NOT_REACHED;

  return result;
}

/// Get the name of a given type and return a copy of it.
///
/// @param t the type to consider.
///
/// @param qualified if true then return the qualified name of the
/// type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the type name if the type has a name, or the
/// empty string if it does not.
interned_string
get_type_name(const type_base* t, bool qualified, bool internal)
{
  const decl_base* d = dynamic_cast<const decl_base*>(t);
  if (!d)
    {
      const function_type* fn_type = is_function_type(t);
      ABG_ASSERT(fn_type);
      return fn_type->get_cached_name(internal);
    }

  // All anonymous types of a given kind get to have the same internal
  // name for internal purpose.  This to allow them to be compared
  // among themselves during type canonicalization.
  if (internal && d->get_is_anonymous())
    {
      string r;
      if (qualified)
	{
	  r = d->get_qualified_parent_name();
	  if (!r.empty())
	    r += "::";
	}
      r += get_generic_anonymous_internal_type_name(d);
      return t->get_environment()->intern(r);
    }

  if (qualified)
    return d->get_qualified_name(internal);
  return d->get_name();
}

/// Get the name of a given type and return a copy of it.
///
/// @param t the type to consider.
///
/// @param qualified if true then return the qualified name of the
/// type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the type name if the type has a name, or the
/// empty string if it does not.
interned_string
get_type_name(const type_base& t, bool qualified, bool internal)
{return get_type_name(&t, qualified, internal);}

/// Get the name of the pointer to a given type.
///
/// @param pointed_to_type the pointed-to-type to consider.
///
/// @param qualified this is true if the resulting name should be of a
/// pointer to a *fully-qualified* pointed-to-type.
///
/// @param internal true if the name is for libabigail-internal
/// purposes.
///
/// @return the name (string representation) of the pointer.
interned_string
get_name_of_pointer_to_type(const type_base& pointed_to_type,
			    bool qualified, bool internal)
{
  const environment* env = pointed_to_type.get_environment();
  ABG_ASSERT(env);

  string tn = get_type_name(pointed_to_type, qualified, internal);
  tn =  tn + "*";

  return env->intern(tn);
}

/// Get the name of the reference to a given type.
///
/// @param pointed_to_type the pointed-to-type to consider.
///
/// @param qualified this is true if the resulting name should be of a
/// reference to a *fully-qualified* pointed-to-type.
///
/// @param internal true if the name is for libabigail-internal
/// purposes.
///
/// @return the name (string representation) of the reference.
interned_string
get_name_of_reference_to_type(const type_base& pointed_to_type,
			      bool lvalue_reference,
			      bool qualified, bool internal)
{
  const environment* env = pointed_to_type.get_environment();
  ABG_ASSERT(env);

  string name = get_type_name(pointed_to_type, qualified, internal);
  if (lvalue_reference)
    name = name + "&";
  else
    name = name + "&&";

  return env->intern(name);
}

/// Get the name of a qualified type, given the underlying type and
/// its qualifiers.
///
/// @param underlying_type the underlying type to consider.
///
/// @param quals the CV qualifiers of the name.
///
/// @param qualified true if we should consider the fully qualified
/// name of @p underlying_type.
///
/// @param internal true if the result is to be used for
/// libabigail-internal purposes.
///
/// @return the name (string representation) of the qualified type.
interned_string
get_name_of_qualified_type(const type_base_sptr& underlying_type,
			   qualified_type_def::CV quals,
			   bool qualified, bool internal)
{
  const environment* env = underlying_type->get_environment();
  ABG_ASSERT(env);

  string quals_repr = get_string_representation_of_cv_quals(quals);
  string name = get_type_name(underlying_type, qualified, internal);

  if (quals_repr.empty() && internal)
    // We are asked to return the internal name, that might be used
    // for type canonicalization.  For that canonicalization, we need
    // to make a difference between a no-op qualified type which
    // underlying type is foo (the qualified type is named "none
    // foo"), and the name of foo, which is just "foo".
    //
    // Please remember that this has to be kept in sync with what is
    // done in die_qualified_name, in abg-dwarf-reader.cc.  So if you
    // change this code here, please change that code there too.
    quals_repr = "";

  if (!quals_repr.empty())
    {
      if (is_pointer_type(underlying_type)
	  || is_reference_type(underlying_type)
	  || is_array_type(underlying_type))
	{
	  name += " ";
	  name += quals_repr;
	}
      else
	name = quals_repr + " " + name;
    }

  return env->intern(name);
}

/// Get the name of a given function type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
interned_string
get_function_type_name(const function_type_sptr& fn_type,
		       bool internal)
{return get_function_type_name(fn_type.get(), internal);}

/// Get the name of a given function type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
interned_string
get_function_type_name(const function_type* fn_type,
		       bool internal)
{
  ABG_ASSERT(fn_type);

  if (const method_type* method = is_method_type(fn_type))
    return get_method_type_name(method, internal);

  return get_function_type_name(*fn_type, internal);
}

/// Get the name of a given function type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
interned_string
get_function_type_name(const function_type& fn_type,
		       bool internal)
{
  std::ostringstream o;
  type_base_sptr return_type = fn_type.get_return_type();
  const environment* env = fn_type.get_environment();
  ABG_ASSERT(env);

  o <<  get_pretty_representation(return_type, internal);

  o << " (";
  for (function_type::parameters::const_iterator i =
	 fn_type.get_parameters().begin();
       i != fn_type.get_parameters().end();
       ++i)
    {
      if (i != fn_type.get_parameters().begin())
	o << ", ";
      o << get_pretty_representation((*i)->get_type(), internal);
    }
  o <<")";

  return env->intern(o.str());
}

/// Get the name of a given method type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
interned_string
get_method_type_name(const method_type_sptr fn_type,
		     bool internal)
{return get_method_type_name(fn_type.get(), internal);}

/// Get the name of a given method type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
interned_string
get_method_type_name(const method_type* fn_type,
		     bool internal)
{
  if (fn_type)
    return get_method_type_name(*fn_type, internal);

  return interned_string();
}

/// Get the name of a given method type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
interned_string
get_method_type_name(const method_type& fn_type,
		     bool internal)
{
  std::ostringstream o;
  type_base_sptr return_type= fn_type.get_return_type();
  const environment* env = fn_type.get_environment();
  ABG_ASSERT(env);

  if (return_type)
    o << return_type->get_cached_pretty_representation(internal);
  else
    // There are still some abixml files out there in which "void"
    // can be expressed as an empty type.
    o << "void";

  class_or_union_sptr class_type = fn_type.get_class_type();
  ABG_ASSERT(class_type);

  o << " (" << class_type->get_qualified_name(internal) << "::*)"
    << " (";

  for (function_type::parameters::const_iterator i =
	 fn_type.get_parameters().begin();
       i != fn_type.get_parameters().end();
       ++i)
    {
      if (i != fn_type.get_parameters().begin())
	o << ", ";
      if (*i)
	o << (*i)->get_type()->get_cached_pretty_representation(internal);
      else
	// There are still some abixml files out there in which "void"
	// can be expressed as an empty type.
	o << "void";
    }
  o <<")";

  return env->intern(o.str());
}

/// Build and return a copy of the pretty representation of an ABI
/// artifact that could be either a type of a decl.
///
/// param tod the ABI artifact to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of an ABI artifact
/// that could be either a type of a decl.
string
get_pretty_representation(const type_or_decl_base* tod, bool internal)
{
  string result;

  if (type_base* t = is_type(const_cast<type_or_decl_base*>(tod)))
    result = get_pretty_representation(t, internal);
  else if (decl_base* d = is_decl(const_cast<type_or_decl_base*>(tod)))
    result =  get_pretty_representation(d, internal);
  else
    // We should never reach this point
    abort();

  return result;
}

/// Build and return a copy of the pretty representation of an ABI
/// artifact that could be either a type of a decl.
///
/// param tod the ABI artifact to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of an ABI artifact
/// that could be either a type of a decl.
string
get_pretty_representation(const type_or_decl_base_sptr& tod, bool internal)
{return get_pretty_representation(tod.get(), internal);}

/// Get a copy of the pretty representation of a decl.
///
/// @param d the decl to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation of the decl.
string
get_pretty_representation(const decl_base* d, bool internal)
{
  if (!d)
    return "";
  return d->get_pretty_representation(internal);
}

/// Get a copy of the pretty representation of a type.
///
/// @param d the type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation of the type.
string
get_pretty_representation(const type_base* t, bool internal)
{
  if (!t)
    return "void";
  if (const function_type* fn_type = is_function_type(t))
    return get_pretty_representation(fn_type, internal);

  const decl_base* d = get_type_declaration(t);
  ABG_ASSERT(d);
  return get_pretty_representation(d, internal);
}

/// Get a copy of the pretty representation of a decl.
///
/// @param d the decl to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation of the decl.
string
get_pretty_representation(const decl_base_sptr& d, bool internal)
{return get_pretty_representation(d.get(), internal);}

/// Get a copy of the pretty representation of a type.
///
/// @param d the type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation of the type.
string
get_pretty_representation(const type_base_sptr& t, bool internal)
{return get_pretty_representation(t.get(), internal);}

/// Get the pretty representation of a function type.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the function type.
string
get_pretty_representation(const function_type_sptr& fn_type,
			  bool internal)
{return get_pretty_representation(fn_type.get(), internal);}

/// Get the pretty representation of a function type.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the function type.
string
get_pretty_representation(const function_type* fn_type, bool internal)
{
  if (!fn_type)
    return "void";

  if (const method_type* method = is_method_type(fn_type))
    return get_pretty_representation(method, internal);

  return get_pretty_representation(*fn_type, internal);
}

/// Get the pretty representation of a function type.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the function type.
string
get_pretty_representation(const function_type& fn_type, bool internal)
{
  std::ostringstream o;
  o << "function type " << get_function_type_name(fn_type, internal);
  return o.str();
}

/// Get the pretty representation of a method type.
///
/// @param method the method type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the method type.
string
get_pretty_representation(const method_type& method, bool internal)
{
  std::ostringstream o;
  o << "method type " << get_method_type_name(method, internal);
  return o.str();
}

/// Get the pretty representation of a method type.
///
/// @param method the method type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the method type.
string
get_pretty_representation(const method_type* method, bool internal)
{
  if (!method)
    return "void";
  return get_pretty_representation(*method, internal);
}

/// Get the pretty representation of a method type.
///
/// @param method the method type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the method type.
string
get_pretty_representation(const method_type_sptr method, bool internal)
{return get_pretty_representation(method.get(), internal);}

/// Get the flat representation of an instance of @ref class_or_union
/// type.
///
/// The flat representation of a given @ref class_or_union type is the
/// actual definition of the type, for instance:
///
///   struct foo {int a; char b;}
///
///@param cou the instance of @ref class_or_union to consider.
///
///@param indent the identation spaces to use in the representation.
///
///@param one_line if true, then the flat representation stands on one
///line.  Otherwise, it stands on multiple lines.
///
///@return the resulting flat representation.
string
get_class_or_union_flat_representation(const class_or_union& cou,
				       const string& indent,
				       bool one_line,
				       bool internal,
				       bool qualified_names)
{
  string repr;
  string local_indent = "  ";

  if (class_decl* clazz = is_class_type(&cou))
    {
      repr = indent;
      if (!internal && clazz->is_struct())
	repr += "struct";
      else
	repr += "class";
    }
  else if (is_union_type(cou))
    repr = indent + "union";
  else
    return "";

  repr += " ";

  string name = cou.get_qualified_name();

  if (!cou.get_is_anonymous())
    repr += name;

  repr += "{";

  if (!one_line)
    repr += "\n";

  string real_indent;
  const class_or_union::data_members &dmems = cou.get_data_members();
  for (class_or_union::data_members::const_iterator dm = dmems.begin();
       dm != dmems.end();
       ++dm)
    {
      if (dm != dmems.begin())
	{
	  if (one_line)
	    real_indent = " ";
	  else
	    real_indent = "\n" + indent + local_indent;
	}

      if (var_decl_sptr v = is_anonymous_data_member(*dm))
	repr +=
	  get_class_or_union_flat_representation
	  (anonymous_data_member_to_class_or_union(*dm),
	   real_indent, one_line, internal, qualified_names);
      else
	{
	  if (one_line)
	    {
	      if (dm != dmems.begin())
		repr += real_indent;
	      repr += (*dm)->get_pretty_representation(internal,
						       qualified_names);
	    }
	  else
	    repr +=
	      real_indent+ (*dm)->get_pretty_representation(internal,
							    qualified_names);
	}
      repr += ";";
    }

  if (one_line)
    repr += "}";
  else
    repr += indent + "}";

  return repr;
}

/// Get the flat representation of an instance of @ref class_or_union
/// type.
///
/// The flat representation of a given @ref class_or_union type is the
/// actual definition of the type, for instance:
///
///   struct foo {int a; char b;}
///
///@param cou the instance of @ref class_or_union to consider.
///
///@param indent the identation spaces to use in the representation.
///
///@param one_line if true, then the flat representation stands on one
///line.  Otherwise, it stands on multiple lines.
///
///@return the resulting flat representation.
string
get_class_or_union_flat_representation(const class_or_union* cou,
				       const string& indent,
				       bool one_line,
				       bool internal,
				       bool qualified_names)
{
  if (cou)
    return get_class_or_union_flat_representation(*cou, indent, one_line,
						  internal, qualified_names);
  return "";
}

/// Get the flat representation of an instance of @ref class_or_union
/// type.
///
/// The flat representation of a given @ref class_or_union type is the
/// actual definition of the type, for instance:
///
///   struct foo {int a; char b;}
///
///@param cou the instance of @ref class_or_union to consider.
///
///@param indent the identation spaces to use in the representation.
///
///@param one_line if true, then the flat representation stands on one
///line.  Otherwise, it stands on multiple lines.
///
///@return the resulting flat representation.
string
get_class_or_union_flat_representation(const class_or_union_sptr& cou,
				       const string& indent,
				       bool one_line,
				       bool internal,
				       bool qualified_names)
{return get_class_or_union_flat_representation(cou.get(),
					       indent,
					       one_line,
					       internal,
					       qualified_names);}

/// By looking at the language of the TU a given ABI artifact belongs
/// to, test if the ONE Definition Rule should apply.
///
/// To date, it applies to c++, java and ada.
///
/// @param artifact the ABI artifact to consider.
///
/// @return true iff the One Definition Rule should apply.
bool
odr_is_relevant(const type_or_decl_base& artifact)
{
  translation_unit::language l =
    artifact.get_translation_unit()->get_language();

  if (is_cplus_plus_language(l)
      || is_java_language(l)
      || is_ada_language(l))
    return true;

  return false;
}

/// Get the declaration for a given type.
///
/// @param t the type to consider.
///
/// @return the declaration for the type to return.
const decl_base*
get_type_declaration(const type_base* t)
{return dynamic_cast<const decl_base*>(t);}

/// Get the declaration for a given type.
///
/// @param t the type to consider.
///
/// @return the declaration for the type to return.
decl_base*
get_type_declaration(type_base* t)
{return dynamic_cast<decl_base*>(t);}

/// Get the declaration for a given type.
///
/// @param t the type to consider.
///
/// @return the declaration for the type to return.
decl_base_sptr
get_type_declaration(const type_base_sptr t)
{return dynamic_pointer_cast<decl_base>(t);}

/// Test if two types are equal modulo a typedef.
///
/// Type A and B are compatible if
///
///	- A and B are equal
///	- or if one type is a typedef of the other one.
///
/// @param type1 the first type to consider.
///
/// @param type2 the second type to consider.
///
/// @return true iff @p type1 and @p type2 are compatible.
bool
types_are_compatible(const type_base_sptr type1,
		     const type_base_sptr type2)
{
  if (!type1 || !type2)
    return false;

  if (type1 == type2)
    return true;

  // Normally we should strip typedefs entirely, but this is
  // potentially costly, especially on binaries with huge changesets
  // like the Linux Kernel.  So we just get the leaf types for now.
  //
  // Maybe there should be an option by which users accepts to pay the
  // CPU usage toll in exchange for finer filtering?

  // type_base_sptr t1 = strip_typedef(type1);
  // type_base_sptr t2 = strip_typedef(type2);

  type_base_sptr t1 = peel_typedef_type(type1);
  type_base_sptr t2 = peel_typedef_type(type2);

  return t1 == t2;
}

/// Test if two types are equal modulo a typedef.
///
/// Type A and B are compatible if
///
///	- A and B are equal
///	- or if one type is a typedef of the other one.
///
/// @param type1 the declaration of the first type to consider.
///
/// @param type2 the declaration of the second type to consider.
///
/// @return true iff @p type1 and @p type2 are compatible.
bool
types_are_compatible(const decl_base_sptr d1,
		     const decl_base_sptr d2)
{return types_are_compatible(is_type(d1), is_type(d2));}

/// Return the translation unit a declaration belongs to.
///
/// @param decl the declaration to consider.
///
/// @return the resulting translation unit, or null if the decl is not
/// yet added to a translation unit.
translation_unit*
get_translation_unit(const decl_base& decl)
{return const_cast<translation_unit*>(decl.get_translation_unit());}

/// Return the translation unit a declaration belongs to.
///
/// @param decl the declaration to consider.
///
/// @return the resulting translation unit, or null if the decl is not
/// yet added to a translation unit.
translation_unit*
get_translation_unit(const decl_base* decl)
{return decl ? get_translation_unit(*decl) : 0;}

/// Return the translation unit a declaration belongs to.
///
/// @param decl the declaration to consider.
///
/// @return the resulting translation unit, or null if the decl is not
/// yet added to a translation unit.
translation_unit*
get_translation_unit(const shared_ptr<decl_base> decl)
{return get_translation_unit(decl.get());}

/// Tests whether if a given scope is the global scope.
///
/// @param scope the scope to consider.
///
/// @return true iff the current scope is the global one.
bool
is_global_scope(const scope_decl& scope)
{return !!dynamic_cast<const global_scope*>(&scope);}

/// Tests whether if a given scope is the global scope.
///
/// @param scope the scope to consider.
///
/// @return the @ref global_scope* representing the scope @p scope or
/// 0 if @p scope is not a global scope.
const global_scope*
is_global_scope(const scope_decl* scope)
{return dynamic_cast<const global_scope*>(scope);}

/// Tests whether if a given scope is the global scope.
///
/// @param scope the scope to consider.
///
/// @return true iff the current scope is the global one.
bool
is_global_scope(const shared_ptr<scope_decl>scope)
{return is_global_scope(scope.get());}

/// Tests whether a given declaration is at global scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at global scope.
bool
is_at_global_scope(const decl_base& decl)
{return (is_global_scope(decl.get_scope()));}

/// Tests whether a given declaration is at global scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at global scope.
bool
is_at_global_scope(const decl_base_sptr decl)
{return (decl && is_global_scope(decl->get_scope()));}

/// Tests whether a given decl is at class scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at class scope.
class_or_union*
is_at_class_scope(const decl_base_sptr decl)
{return is_at_class_scope(decl.get());}

/// Tests whether a given decl is at class scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at class scope.
class_or_union*
is_at_class_scope(const decl_base* decl)
{
  if (!decl)
    return 0;

  return is_at_class_scope(*decl);
}

/// Tests whether a given decl is at class scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at class scope.
class_or_union*
is_at_class_scope(const decl_base& decl)
{
  scope_decl* scope = decl.get_scope();
  if (class_or_union* cl = is_class_type(scope))
    return cl;
  if (class_or_union* cl = is_union_type(scope))
    return cl;
  return 0;
}

/// Find a data member inside an anonymous data member.
///
/// An anonymous data member has a type which is a class or union.
/// This function looks for a data member inside the type of that
/// anonymous data member.
///
/// @param anon_dm the anonymous data member to consider.
///
/// @param name the name of the data member to look for.
var_decl_sptr
find_data_member_from_anonymous_data_member(const var_decl_sptr& anon_dm,
					    const string& name)
{
  const class_or_union* containing_class_or_union =
    anonymous_data_member_to_class_or_union(anon_dm.get());

  if (!containing_class_or_union)
    return var_decl_sptr();

  var_decl_sptr result = containing_class_or_union->find_data_member(name);
  return result;
}

/// Tests whether a given decl is at template scope.
///
/// Note that only template parameters , types that are compositions,
/// and template patterns (function or class) can be at template scope.
///
/// @param decl the decl to consider.
///
/// @return true iff the decl is at template scope.
bool
is_at_template_scope(const shared_ptr<decl_base> decl)
{return (decl && dynamic_cast<template_decl*>(decl->get_scope()));}

/// Tests whether a decl is a template parameter.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is a template parameter.
bool
is_template_parameter(const shared_ptr<decl_base> decl)
{
  return (decl && (dynamic_pointer_cast<type_tparameter>(decl)
		   || dynamic_pointer_cast<non_type_tparameter>(decl)
		   || dynamic_pointer_cast<template_tparameter>(decl)));
}

/// Test whether a declaration is a @ref function_decl.
///
/// @param d the declaration to test for.
///
/// @return a shared pointer to @ref function_decl if @p d is a @ref
/// function_decl.  Otherwise, a nil shared pointer.
function_decl*
is_function_decl(const type_or_decl_base* d)
{return dynamic_cast<function_decl*>(const_cast<type_or_decl_base*>(d));}

/// Test whether a declaration is a @ref function_decl.
///
/// @param d the declaration to test for.
///
/// @return true if @p d is a function_decl.
bool
is_function_decl(const type_or_decl_base& d)
{return is_function_decl(&d);}

/// Test whether a declaration is a @ref function_decl.
///
/// @param d the declaration to test for.
///
/// @return a shared pointer to @ref function_decl if @p d is a @ref
/// function_decl.  Otherwise, a nil shared pointer.
function_decl_sptr
is_function_decl(const type_or_decl_base_sptr& d)
{return dynamic_pointer_cast<function_decl>(d);}

/// Test whether a declaration is a @ref function_decl.
///
/// @param d the declaration to test for.
///
/// @return a pointer to @ref function_decl if @p d is a @ref
/// function_decl.  Otherwise, a nil shared pointer.
function_decl::parameter*
is_function_parameter(const type_or_decl_base* tod)
{
  return dynamic_cast<function_decl::parameter*>
    (const_cast<type_or_decl_base*>(tod));
}

/// Test whether an ABI artifact is a @ref function_decl.
///
/// @param tod the declaration to test for.
///
/// @return a pointer to @ref function_decl if @p d is a @ref
/// function_decl.  Otherwise, a nil shared pointer.
function_decl::parameter_sptr
is_function_parameter(const type_or_decl_base_sptr tod)
{return dynamic_pointer_cast<function_decl::parameter>(tod);}

/// Test if an ABI artifact is a declaration.
///
/// @param d the artifact to consider.
///
/// @param return the declaration sub-object of @p d if it's a
/// declaration, or NULL if it is not.
decl_base*
is_decl(const type_or_decl_base* d)
{
  if (d && (d->kind() & type_or_decl_base::ABSTRACT_DECL_BASE))
    {
      if (!(d->kind() & type_or_decl_base::ABSTRACT_TYPE_BASE))
	// The artifact is a decl-only (like a function or a
	// variable).  That is, it's not a type that also has a
	// declaration.  In this case, we are in the fast path and we
	// have a pointer to the decl sub-object handy.  Just return
	// it ...
	return reinterpret_cast<decl_base*>
	  (const_cast<type_or_decl_base*>(d)->type_or_decl_base_pointer());

      // ... Otherwise, we are in the slow path, which is that the
      // artifact is a type which has a declaration.  In that case,
      // let's use the slow dynamic_cast because we don't have the
      // pointer to the decl sub-object handily present.
      return dynamic_cast<decl_base*>(const_cast<type_or_decl_base*>(d));
    }
  return 0;
}

/// Test if an ABI artifact is a declaration.
///
/// @param d the artifact to consider.
///
/// @param return the declaration sub-object of @p d if it's a
/// declaration, or NULL if it is not.
decl_base_sptr
is_decl(const type_or_decl_base_sptr& d)
{return dynamic_pointer_cast<decl_base>(d);}

/// Test if an ABI artifact is a declaration.
///
/// This is done using a slow path that uses dynamic_cast.
///
/// @param d the artifact to consider.
///
/// @param return the declaration sub-object of @p d if it's a
decl_base*
is_decl_slow(const type_or_decl_base* t)
{return dynamic_cast<decl_base*>(const_cast<type_or_decl_base*>(t));}

/// Test if an ABI artifact is a declaration.
///
/// This is done using a slow path that uses dynamic_cast.
///
/// @param d the artifact to consider.
///
/// @param return the declaration sub-object of @p d if it's a
decl_base_sptr
is_decl_slow(const type_or_decl_base_sptr& t)
{return dynamic_pointer_cast<decl_base>(t);}

/// Test whether a declaration is a type.
///
/// @param d the IR artefact to test for.
///
/// @return true if the artifact is a type, false otherwise.
bool
is_type(const type_or_decl_base& tod)
{
  if (dynamic_cast<const type_base*>(&tod))
    return true;
  return false;
}

/// Test whether a declaration is a type.
///
/// @param d the IR artefact to test for.
///
/// @return true if the artifact is a type, false otherwise.
type_base*
is_type(const type_or_decl_base* t)
{
  if (t && (t->kind() & type_or_decl_base::ABSTRACT_TYPE_BASE))
    return reinterpret_cast<type_base*>
      (const_cast<type_or_decl_base*>(t)->type_or_decl_base_pointer());

  return 0;
}

/// Test whether a declaration is a type.
///
/// @param d the IR artefact to test for.
///
/// @return true if the artifact is a type, false otherwise.
type_base_sptr
is_type(const type_or_decl_base_sptr& tod)
{return dynamic_pointer_cast<type_base>(tod);}

/// Test whether a declaration is a type.
///
/// @param d the declaration to test for.
///
/// @return true if the declaration is a type, false otherwise.

/// Test if a given type is anonymous.
///
/// Note that this function considers that an anonymous class that is
/// named by a typedef is not anonymous anymore.  This is the C idiom:
///
///       typedef struct {int member;} s_type;
///
/// The typedef s_type becomes the name of the originally anonymous
/// struct.
///
/// @param t the type to consider.
///
/// @return true iff @p t is anonymous.
bool
is_anonymous_type(type_base* t)
{
  decl_base* d = get_type_declaration(t);
  if (d)
    if (d->get_is_anonymous())
      {
	if (class_or_union *cou = is_class_or_union_type(t))
	  {
	    // An anonymous class that is named by a typedef is not
	    // considered anonymous anymore.
	    if (!cou->get_naming_typedef())
	      return true;
	  }
	else
	  return true;
      }
  return false;
}

/// Test if a given type is anonymous.
///
/// @param t the type to consider.
///
/// @return true iff @p t is anonymous.
bool
is_anonymous_type(const type_base_sptr& t)
{return is_anonymous_type(t.get());}

/// Test whether a type is a type_decl (a builtin type).
///
/// @return the type_decl* for @t if it's type_decl, otherwise, return
/// nil.
const type_decl*
is_type_decl(const type_or_decl_base* t)
{return dynamic_cast<const type_decl*>(t);}

/// Test whether a type is a type_decl (a builtin type).
///
/// @return the type_decl_sptr for @t if it's type_decl, otherwise,
/// return nil.
type_decl_sptr
is_type_decl(const type_or_decl_base_sptr& t)
{return dynamic_pointer_cast<type_decl>(t);}

/// Test whether a type is a typedef.
///
/// @param t the type to test for.
///
/// @return the typedef declaration of the @p t, or NULL if it's not a
/// typedef.
typedef_decl_sptr
is_typedef(const type_or_decl_base_sptr t)
{return dynamic_pointer_cast<typedef_decl>(t);}

/// Test whether a type is a typedef.
///
/// @param t the declaration of the type to test for.
///
/// @return the typedef declaration of the @p t, or NULL if it's not a
/// typedef.
const typedef_decl*
is_typedef(const type_base* t)
{return dynamic_cast<const typedef_decl*>(t);}

/// Test whether a type is a typedef.
///
/// @param t the declaration of the type to test for.
///
/// @return the typedef declaration of the @p t, or NULL if it's not a
/// typedef.
typedef_decl*
is_typedef(type_base* t)
{return dynamic_cast<typedef_decl*>(t);}

/// Test if a type is an enum. This function looks through typedefs.
///
/// @parm t the type to consider.
///
/// @return the enum_decl if @p t is an @ref enum_decl or null
/// otherwise.
enum_type_decl_sptr
is_compatible_with_enum_type(const type_base_sptr& t)
{
  if (!t)
    return enum_type_decl_sptr();

  // Normally we should strip typedefs entirely, but this is
  // potentially costly, especially on binaries with huge changesets
  // like the Linux Kernel.  So we just get the leaf types for now.
  //
  // Maybe there should be an option by which users accepts to pay the
  // CPU usage toll in exchange for finer filtering?

  // type_base_sptr ty = strip_typedef(t);
  type_base_sptr ty = peel_typedef_type(t);;
  return is_enum_type(ty);
}

/// Test if a type is an enum. This function looks through typedefs.
///
/// @parm t the type to consider.
///
/// @return the enum_decl if @p t is an @ref enum_decl or null
/// otherwise.
enum_type_decl_sptr
is_compatible_with_enum_type(const decl_base_sptr& t)
{return is_compatible_with_enum_type(is_type(t));}

/// Test if a decl is an enum_type_decl
///
/// @param d the decl to test for.
///
/// @return the enum_type_decl* if @p d is an enum, nil otherwise.
const enum_type_decl*
is_enum_type(const type_or_decl_base* d)
{return dynamic_cast<const enum_type_decl*>(d);}

/// Test if a decl is an enum_type_decl
///
/// @param d the decl to test for.
///
/// @return the enum_type_decl_sptr if @p d is an enum, nil otherwise.
enum_type_decl_sptr
is_enum_type(const type_or_decl_base_sptr& d)
{return dynamic_pointer_cast<enum_type_decl>(d);}

/// Test if a type is a class. This function looks through typedefs.
///
/// @parm t the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl_sptr
is_compatible_with_class_type(const type_base_sptr& t)
{
  if (!t)
    return class_decl_sptr();

  // Normally we should strip typedefs entirely, but this is
  // potentially costly, especially on binaries with huge changesets
  // like the Linux Kernel.  So we just get the leaf types for now.
  //
  // Maybe there should be an option by which users accepts to pay the
  // CPU usage toll in exchange for finer filtering?

  // type_base_sptr ty = strip_typedef(t);
  type_base_sptr ty = peel_typedef_type(t);
  return is_class_type(ty);
}

/// Test if a type is a class. This function looks through typedefs.
///
/// @parm t the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl_sptr
is_compatible_with_class_type(const decl_base_sptr& t)
{return is_compatible_with_class_type(is_type(t));}

/// Test whether a type is a class.
///
/// @parm t the type to consider.
///
/// @return true iff @p t is a class_decl.
bool
is_class_type(const type_or_decl_base& t)
{return is_class_type(&t);}

/// Test whether a type is a class.
///
/// @parm t the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl*
is_class_type(const type_or_decl_base* t)
{
  if (!t)
    return 0;

  if (t->kind() & type_or_decl_base::CLASS_TYPE)
    return reinterpret_cast<class_decl*>
      (const_cast<type_or_decl_base*>(t)->runtime_type_instance());

  return 0;
}

/// Test whether a type is a class.
///
/// @parm t the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl_sptr
is_class_type(const type_or_decl_base_sptr& d)
{return dynamic_pointer_cast<class_decl>(d);}


/// Test wheter a type is a declaration-only class.
///
/// @param t the type to considier.
///
/// @return true iff @p t is a declaration-only class.
bool
is_declaration_only_class_or_union_type(const type_base *t)
{
  if (const class_or_union *klass = is_class_or_union_type(t))
    return klass->get_is_declaration_only();
  return false;
}

/// Test wheter a type is a declaration-only class.
///
/// @param t the type to considier.
///
/// @return true iff @p t is a declaration-only class.
bool
is_declaration_only_class_type(const type_base_sptr& t)
{return is_declaration_only_class_or_union_type(t.get());}

/// Test if a type is a @ref class_or_union.
///
/// @param t the type to consider.
///
/// @return the @ref class_or_union is @p is a @ref class_or_union, or
/// nil otherwise.
class_or_union*
is_class_or_union_type(const type_or_decl_base* t)
{return dynamic_cast<class_or_union*>(const_cast<type_or_decl_base*>(t));}

/// Test if a type is a @ref class_or_union.
///
/// @param t the type to consider.
///
/// @return the @ref class_or_union is @p is a @ref class_or_union, or
/// nil otherwise.
shared_ptr<class_or_union>
is_class_or_union_type(const shared_ptr<type_or_decl_base>& t)
{return dynamic_pointer_cast<class_or_union>(t);}

/// Test if a type is a @ref union_decl.
///
/// @param t the type to consider.
///
/// @return true iff @p t is a union_decl.
bool
is_union_type(const type_or_decl_base& t)
{return is_union_type(&t);}

/// Test if a type is a @ref union_decl.
///
/// @param t the type to consider.
///
/// @return the @ref union_decl is @p is a @ref union_decl, or nil
/// otherwise.
union_decl*
is_union_type(const type_or_decl_base* t)
{return dynamic_cast<union_decl*>(const_cast<type_or_decl_base*>(t));}

/// Test if a type is a @ref union_decl.
///
/// @param t the type to consider.
///
/// @return the @ref union_decl is @p is a @ref union_decl, or nil
/// otherwise.
union_decl_sptr
is_union_type(const shared_ptr<type_or_decl_base>& t)
{return dynamic_pointer_cast<union_decl>(t);}

/// Test whether a type is a pointer_type_def.
///
/// @param t the type to test.
///
/// @return the @ref pointer_type_def_sptr if @p t is a
/// pointer_type_def, null otherwise.
pointer_type_def*
is_pointer_type(type_or_decl_base* t)
{
  if (!t)
    return 0;

  if (t->kind() & type_or_decl_base::POINTER_TYPE)
    return reinterpret_cast<pointer_type_def*>
      (const_cast<type_or_decl_base*>(t)->runtime_type_instance());

  return 0;
}

/// Test whether a type is a pointer_type_def.
///
/// @param t the type to test.
///
/// @return the @ref pointer_type_def_sptr if @p t is a
/// pointer_type_def, null otherwise.
const pointer_type_def*
is_pointer_type(const type_or_decl_base* t)
{
  return is_pointer_type(const_cast<type_or_decl_base*>(t));
}

/// Test whether a type is a pointer_type_def.
///
/// @param t the type to test.
///
/// @return the @ref pointer_type_def_sptr if @p t is a
/// pointer_type_def, null otherwise.
pointer_type_def_sptr
is_pointer_type(const type_or_decl_base_sptr &t)
{return dynamic_pointer_cast<pointer_type_def>(t);}

/// Test whether a type is a reference_type_def.
///
/// @param t the type to test.
///
/// @return the @ref reference_type_def_sptr if @p t is a
/// reference_type_def, null otherwise.
reference_type_def*
is_reference_type(type_or_decl_base* t)
{return dynamic_cast<reference_type_def*>(t);}

/// Test whether a type is a reference_type_def.
///
/// @param t the type to test.
///
/// @return the @ref reference_type_def_sptr if @p t is a
/// reference_type_def, null otherwise.
const reference_type_def*
is_reference_type(const type_or_decl_base* t)
{return dynamic_cast<const reference_type_def*>(t);}

/// Test whether a type is a reference_type_def.
///
/// @param t the type to test.
///
/// @return the @ref reference_type_def_sptr if @p t is a
/// reference_type_def, null otherwise.
reference_type_def_sptr
is_reference_type(const type_or_decl_base_sptr& t)
{return dynamic_pointer_cast<reference_type_def>(t);}

/// Test if a type is a pointer to void type.
///
/// Note that this looks trough typedefs or CV qualifiers to look for
/// the void pointer.
///
/// @param type the type to consider.
///
/// @return the actual void pointer if @p is a void pointer or NULL if
/// it's not.
const type_base*
is_void_pointer_type(const type_base* type)
{
  type = peel_qualified_or_typedef_type(type);

  const pointer_type_def * t = is_pointer_type(type);
  if (!t)
    return 0;

  // Look through typedefs in the pointed-to type as well.
  type_base * ty = t->get_pointed_to_type().get();
  ty = peel_qualified_or_typedef_type(ty);
  if (ty->get_environment()->is_void_type(ty))
    return ty;

  return 0;
}

/// Test whether a type is a reference_type_def.
///
/// @param t the type to test.
///
/// @return the @ref reference_type_def_sptr if @p t is a
/// reference_type_def, null otherwise.
qualified_type_def*
is_qualified_type(const type_or_decl_base* t)
{return dynamic_cast<qualified_type_def*>(const_cast<type_or_decl_base*>(t));}

/// Test whether a type is a qualified_type_def.
///
/// @param t the type to test.
///
/// @return the @ref qualified_type_def_sptr if @p t is a
/// qualified_type_def, null otherwise.
qualified_type_def_sptr
is_qualified_type(const type_or_decl_base_sptr& t)
{return dynamic_pointer_cast<qualified_type_def>(t);}

/// Strip a type from its top level no-op qualifier.
///
/// Note that a no-op qualifier is how we represents, for instance, a
/// "const reference".  As a reference is always const, that const
/// qualifier just adds noise in terms of change analysis.  Se we
/// represent it as a no-op qualifier so that we can strip it.
///
/// @param t to type to strip from its potential top-level no-op
/// qualifier.
///
/// @return If @t is a no-op qualified type, then return the first
/// underlying type that is not a no-op qualified type.
type_base_sptr
look_through_no_op_qualified_type(const type_base_sptr& t)
{
  type_base_sptr ty;
  if (qualified_type_def_sptr qt = is_qualified_type(t))
    if (qt->get_cv_quals() == qualified_type_def::CV_NONE)
      ty = qt->get_underlying_type();

  if (is_qualified_type(ty))
    return look_through_no_op_qualified_type(ty);

  return ty ? ty : t;
}

/// Test whether a type is a function_type.
///
/// @param t the type to test.
///
/// @return the @ref function_type_sptr if @p t is a
/// function_type, null otherwise.
function_type_sptr
is_function_type(const type_or_decl_base_sptr& t)
{return dynamic_pointer_cast<function_type>(t);}

/// Test whether a type is a function_type.
///
/// @param t the type to test.
///
/// @return the @ref function_type_sptr if @p t is a
/// function_type, null otherwise.
function_type*
is_function_type(type_or_decl_base* t)
{return dynamic_cast<function_type*>(t);}

/// Test whether a type is a function_type.
///
/// @param t the type to test.
///
/// @return the @ref function_type_sptr if @p t is a
/// function_type, null otherwise.
const function_type*
is_function_type(const type_or_decl_base* t)
{return dynamic_cast<const function_type*>(t);}

/// Test whether a type is a method_type.
///
/// @param t the type to test.
///
/// @return the @ref method_type_sptr if @p t is a
/// method_type, null otherwise.
method_type_sptr
is_method_type(const type_or_decl_base_sptr& t)
{return dynamic_pointer_cast<method_type>(t);}

/// Test whether a type is a method_type.
///
/// @param t the type to test.
///
/// @return the @ref method_type_sptr if @p t is a
/// method_type, null otherwise.
const method_type*
is_method_type(const type_or_decl_base* t)
{return dynamic_cast<const method_type*>(t);}

/// Test whether a type is a method_type.
///
/// @param t the type to test.
///
/// @return the @ref method_type_sptr if @p t is a
/// method_type, null otherwise.
method_type*
is_method_type(type_or_decl_base* t)
{return dynamic_cast<method_type*>(t);}

/// If a class (or union) is a decl-only class, get its definition.
/// Otherwise, just return the initial class.
///
/// @param the_class the class (or union) to consider.
///
/// @return either the definition of the class, or the class itself.
class_or_union*
look_through_decl_only_class(class_or_union* the_class)
{return is_class_or_union_type(look_through_decl_only(the_class));}

/// If a class (or union) is a decl-only class, get its definition.
/// Otherwise, just return the initial class.
///
/// @param the_class the class (or union) to consider.
///
/// @return either the definition of the class, or the class itself.
class_or_union_sptr
look_through_decl_only_class(const class_or_union& the_class)
{return is_class_or_union_type(look_through_decl_only(the_class));}

/// If a class (or union) is a decl-only class, get its definition.
/// Otherwise, just return the initial class.
///
/// @param klass the class (or union) to consider.
///
/// @return either the definition of the class, or the class itself.
class_or_union_sptr
look_through_decl_only_class(class_or_union_sptr klass)
{return is_class_or_union_type(look_through_decl_only(klass));}

/// If an enum is a decl-only enum, get its definition.
/// Otherwise, just return the initial enum.
///
/// @param the_enum the enum to consider.
///
/// @return either the definition of the enum, or the enum itself.
enum_type_decl_sptr
look_through_decl_only_enum(const enum_type_decl& the_enum)
{return is_enum_type(look_through_decl_only(the_enum));}

/// If an enum is a decl-only enum, get its definition.
/// Otherwise, just return the initial enum.
///
/// @param enom the enum to consider.
///
/// @return either the definition of the enum, or the enum itself.
enum_type_decl_sptr
look_through_decl_only_enum(enum_type_decl_sptr enom)
{return is_enum_type(look_through_decl_only(enom));}

/// If a decl is decl-only get its definition.  Otherwise, just return nil.
///
/// @param d the decl to consider.
///
/// @return either the definition of the decl, or nil.
decl_base_sptr
look_through_decl_only(const decl_base& d)
{
  decl_base_sptr decl;
  if (d.get_is_declaration_only())
    decl = d.get_definition_of_declaration();

  if (!decl)
    return decl;

  while (decl->get_is_declaration_only()
	 && decl->get_definition_of_declaration())
    decl = decl->get_definition_of_declaration();

  return decl;
}

/// If a decl is decl-only enum, get its definition.  Otherwise, just
/// return the initial decl.
///
/// @param d the decl to consider.
///
/// @return either the definition of the enum, or the decl itself.
decl_base*
look_through_decl_only(decl_base* d)
{
  if (!d)
    return d;

  decl_base* result = look_through_decl_only(*d).get();
  if (!result)
    result = d;

  return result;
}

/// If a decl is decl-only get its definition.  Otherwise, just return nil.
///
/// @param d the decl to consider.
///
/// @return either the definition of the decl, or nil.
decl_base_sptr
look_through_decl_only(const decl_base_sptr& d)
{
  if (!d)
    return d;

  decl_base_sptr result = look_through_decl_only(*d);
  if (!result)
    result = d;

  return result;
}

/// Tests if a declaration is a variable declaration.
///
/// @param decl the decl to test.
///
/// @return the var_decl_sptr iff decl is a variable declaration; nil
/// otherwise.
var_decl*
is_var_decl(const type_or_decl_base* tod)
{return dynamic_cast<var_decl*>(const_cast<type_or_decl_base*>(tod));}

/// Tests if a declaration is a variable declaration.
///
/// @param decl the decl to test.
///
/// @return the var_decl_sptr iff decl is a variable declaration; nil
/// otherwise.
var_decl_sptr
is_var_decl(const type_or_decl_base_sptr& decl)
{return dynamic_pointer_cast<var_decl>(decl);}

/// Tests if a declaration is a namespace declaration.
///
/// @param d the decalration to consider.
///
/// @return the namespace declaration if @p d is a namespace.
namespace_decl_sptr
is_namespace(const decl_base_sptr& d)
{return dynamic_pointer_cast<namespace_decl>(d);}

/// Tests if a declaration is a namespace declaration.
///
/// @param d the decalration to consider.
///
/// @return the namespace declaration if @p d is a namespace.
namespace_decl*
is_namespace(const decl_base* d)
{return dynamic_cast<namespace_decl*>(const_cast<decl_base*>(d));}

/// Tests whether a decl is a template parameter composition type.
///
/// @param decl the declaration to consider.
///
/// @return true iff decl is a template parameter composition type.
bool
is_template_parm_composition_type(const shared_ptr<decl_base> decl)
{
  return (decl
	  && is_at_template_scope(decl)
	  && is_type(decl)
	  && !is_template_parameter(decl));
}

/// Test whether a decl is the pattern of a function template.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is the pattern of a function template.
bool
is_function_template_pattern(const shared_ptr<decl_base> decl)
{
  return (decl
	  && dynamic_pointer_cast<function_decl>(decl)
	  && dynamic_cast<template_decl*>(decl->get_scope()));
}

/// Test if a type is an array_type_def.
///
/// @param type the type to consider.
///
/// @return true iff @p type is an array_type_def.
array_type_def*
is_array_type(const type_or_decl_base* type)
{return dynamic_cast<array_type_def*>(const_cast<type_or_decl_base*>(type));}

/// Test if a type is an array_type_def.
///
/// @param type the type to consider.
///
/// @return true iff @p type is an array_type_def.
array_type_def_sptr
is_array_type(const type_or_decl_base_sptr& type)
{return dynamic_pointer_cast<array_type_def>(type);}

/// Tests if the element of a given array is a qualified type.
///
/// @param array the array type to consider.
///
/// @return the qualified element of the array iff it's a qualified
/// type.  Otherwise, return a nil object.
qualified_type_def_sptr
is_array_of_qualified_element(const array_type_def_sptr& array)
{
  if (!array)
    return qualified_type_def_sptr();

  return is_qualified_type(array->get_element_type());
}

/// Test if an array type is an array to a qualified element type.
///
/// @param type the array type to consider.
///
/// @return true the array @p type iff it's an array to a qualified
/// element type.
array_type_def_sptr
is_array_of_qualified_element(const type_base_sptr& type)
{
  if (array_type_def_sptr array = is_array_type(type))
    if (is_array_of_qualified_element(array))
      return array;

  return array_type_def_sptr();
}

/// Test if a type is a typedef of an array.
///
/// Note that the function looks through qualified and typedefs types
/// of the underlying type of the current typedef.  In other words, if
/// we are looking at a typedef of a CV-qualified array, or at a
/// typedef of a CV-qualified typedef of an array, this function will
/// still return TRUE.
///
/// @param t the type to consider.
///
/// @return true if t is a typedef which underlying type is an array.
/// That array might be either cv-qualified array or a typedef'ed
/// array, or a combination of both.
array_type_def_sptr
is_typedef_of_array(const type_base_sptr& t)
{
  array_type_def_sptr result;

  if (typedef_decl_sptr typdef = is_typedef(t))
    {
      type_base_sptr u =
	peel_qualified_or_typedef_type(typdef->get_underlying_type());
      result = is_array_type(u);
    }

  return result;
}

/// Test if a type is an array_type_def::subrange_type.
///
/// @param type the type to consider.
///
/// @return the array_type_def::subrange_type which @p type is a type
/// of, or nil if it's not of that type.
array_type_def::subrange_type*
is_subrange_type(const type_or_decl_base *type)
{
  return dynamic_cast<array_type_def::subrange_type*>
    (const_cast<type_or_decl_base*>(type));
}

/// Test if a type is an array_type_def::subrange_type.
///
/// @param type the type to consider.
///
/// @return the array_type_def::subrange_type which @p type is a type
/// of, or nil if it's not of that type.
array_type_def::subrange_sptr
is_subrange_type(const type_or_decl_base_sptr &type)
{return dynamic_pointer_cast<array_type_def::subrange_type>(type);}

/// Tests whether a decl is a template.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is a function template, class template, or
/// template template parameter.
bool
is_template_decl(const shared_ptr<decl_base> decl)
{return decl && dynamic_pointer_cast<template_decl>(decl);}

/// This enum describe the kind of entity to lookup, while using the
/// lookup API.
enum lookup_entity_kind
{
  LOOKUP_ENTITY_TYPE,
  LOOKUP_ENTITY_VAR,
};

/// Find the first relevant delimiter (the "::" string) in a fully
/// qualified C++ type name, starting from a given position.  The
/// delimiter returned separates a type name from the name of its
/// context.
///
/// This is supposed to work correctly on names in cases like this:
///
///    foo<ns1::name1, ns2::name2>
///
/// In that case when called with with parameter @p begin set to 0, no
/// delimiter is returned, because the type name in this case is:
/// 'foo<ns1::name1, ns2::name2>'.
///
/// But in this case:
///
///   foo<p1, bar::name>::some_type
///
/// The "::" returned is the one right before 'some_type'.
///
/// @param fqn the fully qualified name of the type to consider.
///
/// @param begin the position from which to look for the delimiter.
///
/// @param delim_pos out parameter. Is set to the position of the
/// delimiter iff the function returned true.
///
/// @return true iff the function found and returned the delimiter.
static bool
find_next_delim_in_cplus_type(const string&	fqn,
			      size_t		begin,
			      size_t&		delim_pos)
{
  int angle_count = 0;
  bool found = false;
  size_t i = begin;
  for (; i < fqn.size(); ++i)
    {
      if (fqn[i] == '<')
	++angle_count;
      else if (fqn[i] == '>')
	--angle_count;
      else if (i + 1 < fqn.size()
	       && !angle_count
	       && fqn[i] == ':'
	       && fqn[i+1] == ':')
	{
	  delim_pos = i;
	  found = true;
	  break;
	}
    }
  return found;
}

/// Decompose a fully qualified name into the list of its components.
///
/// @param fqn the fully qualified name to decompose.
///
/// @param comps the resulting list of component to fill.
void
fqn_to_components(const string& fqn,
		  list<string>& comps)
{
  string::size_type fqn_size = fqn.size(), comp_begin = 0, comp_end = fqn_size;
  do
    {
      if (!find_next_delim_in_cplus_type(fqn, comp_begin, comp_end))
	comp_end = fqn_size;

      string comp = fqn.substr(comp_begin, comp_end - comp_begin);
      comps.push_back(comp);

      comp_begin = comp_end + 2;
      if (comp_begin >= fqn_size)
	break;
    } while (true);
}

/// Turn a set of qualified name components (that name a type) into a
/// qualified name string.
///
/// @param comps the name components
///
/// @return the resulting string, which would be the qualified name of
/// a type.
string
components_to_type_name(const list<string>& comps)
{
  string result;
  for (list<string>::const_iterator c = comps.begin();
       c != comps.end();
       ++c)
    if (c == comps.begin())
      result = *c;
    else
      result += "::" + *c;
  return result;
}

/// This predicate returns true if a given container iterator points
/// to the last element of the container, false otherwise.
///
/// @tparam T the type of the container of the iterator.
///
/// @param container the container the iterator points into.
///
/// @param i the iterator to consider.
///
/// @return true iff the iterator points to the last element of @p
/// container.
template<typename T>
static bool
iterator_is_last(T& container,
		 typename T::const_iterator i)
{
  typename T::const_iterator next = i;
  ++next;
  return (next == container.end());
}

//--------------------------------
// <type and decls lookup stuff>
// ------------------------------

/// Lookup all the type*s* that have a given fully qualified name.
///
/// @param type_name the fully qualified name of the type to
/// lookup.
///
/// @param type_map the map to look into.
///
/// @return the vector containing the types named @p type_name.  If
/// the lookup didn't yield any type, then this function returns nil.
static const type_base_wptrs_type*
lookup_types_in_map(const interned_string& type_name,
		    const istring_type_base_wptrs_map_type& type_map)
{
  istring_type_base_wptrs_map_type::const_iterator i = type_map.find(type_name);
  if (i != type_map.end())
    return &i->second;
  return 0;
}

/// Lookup a type (with a given name) in a map that associates a type
/// name to a type.  If there are several types with a given name,
/// then return the last of such types, that is, the last one that got
/// registered.
///
/// @tparam TypeKind the type of the type this function is supposed to
/// return.
///
/// @param type_name the name of the type to lookup.
///
/// @param type_map the map in which to look.
///
/// @return a shared_ptr to the type found.  If no type was found or
/// if the type found was not of type @p TypeKind then the function
/// returns nil.
template <class TypeKind>
static shared_ptr<TypeKind>
lookup_type_in_map(const interned_string& type_name,
		   const istring_type_base_wptrs_map_type& type_map)
{
  istring_type_base_wptrs_map_type::const_iterator i = type_map.find(type_name);
  if (i != type_map.end())
    return dynamic_pointer_cast<TypeKind>(type_base_sptr(i->second.back()));
  return shared_ptr<TypeKind>();
}

/// Lookup a basic type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the basic type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the basic type found or nil if no basic type was found.
type_decl_sptr
lookup_basic_type(const interned_string& type_name, const translation_unit& tu)
{
  return lookup_type_in_map<type_decl>(type_name,
				       tu.get_types().basic_types());
}

/// Lookup a basic type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the basic type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the basic type found or nil if no basic type was found.
type_decl_sptr
lookup_basic_type(const string& type_name, const translation_unit& tu)
{
  const environment* env = tu.get_environment();
  ABG_ASSERT(env);

  interned_string s = env->intern(type_name);
  return lookup_basic_type(s, tu);
}

/// Lookup a class type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param fqn the fully qualified name of the class type node to look
/// up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the class type IR node found, NULL
/// otherwise.
class_decl_sptr
lookup_class_type(const string& fqn, const translation_unit& tu)
{
  const environment* env = tu.get_environment();
  ABG_ASSERT(env);

  interned_string s = env->intern(fqn);
  return lookup_class_type(s, tu);
}

/// Lookup a class type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the class type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the class type found or nil if no class type was found.
class_decl_sptr
lookup_class_type(const interned_string& type_name, const translation_unit& tu)
{
  return lookup_type_in_map<class_decl>(type_name,
					tu.get_types().class_types());
}

/// Lookup a union type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the union type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the union type found or nil if no union type was found.
union_decl_sptr
lookup_union_type(const interned_string& type_name, const translation_unit& tu)
{
  return lookup_type_in_map<union_decl>(type_name,
					tu.get_types().union_types());
}

/// Lookup a union type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param fqn the fully qualified name of the type to lookup.
///
/// @param tu the translation unit to look into.
///
/// @return the union type found or nil if no union type was found.
union_decl_sptr
lookup_union_type(const string& fqn, const translation_unit& tu)
{
  const environment* env = tu.get_environment();
  ABG_ASSERT(env);

  interned_string s = env->intern(fqn);
  return lookup_union_type(s, tu);
}

/// Lookup a union type in a given corpus, from its location.
///
/// @param loc the location of the union type to look for.
///
/// @param corp the corpus to look it from.
///
/// @return the resulting union_decl.
union_decl_sptr
lookup_union_type_per_location(const interned_string &loc, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m =
    corp.get_type_per_loc_map().union_types();
  union_decl_sptr result = lookup_type_in_map<union_decl>(loc, m);

  return result;
}

/// Lookup a union type in a given corpus, from its location.
///
/// @param loc the location of the union type to look for.
///
/// @param corp the corpus to look it from.
///
/// @return the resulting union_decl.
union_decl_sptr
lookup_union_type_per_location(const string& loc, const corpus& corp)
{
  const environment* env = corp.get_environment();
  ABG_ASSERT(env);

  return lookup_union_type_per_location(env->intern(loc), corp);
}

/// Lookup an enum type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the enum type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the enum type found or nil if no enum type was found.
enum_type_decl_sptr
lookup_enum_type(const interned_string& type_name, const translation_unit& tu)
{
  return lookup_type_in_map<enum_type_decl>(type_name,
					    tu.get_types().enum_types());
}

/// Lookup an enum type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the enum type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the enum type found or nil if no enum type was found.
enum_type_decl_sptr
lookup_enum_type(const string& type_name, const translation_unit& tu)
{
  const environment* env = tu.get_environment();
  ABG_ASSERT(env);

  interned_string s = env->intern(type_name);
  return lookup_enum_type(s, tu);
}

/// Lookup a typedef type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the typedef type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the typedef type found or nil if no typedef type was
/// found.
typedef_decl_sptr
lookup_typedef_type(const interned_string& type_name,
		    const translation_unit& tu)
{
  return lookup_type_in_map<typedef_decl>(type_name,
					  tu.get_types().typedef_types());
}

/// Lookup a typedef type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the typedef type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the typedef type found or nil if no typedef type was
/// found.
typedef_decl_sptr
lookup_typedef_type(const string& type_name, const translation_unit& tu)
{
  const environment* env = tu.get_environment();
  ABG_ASSERT(env);

  interned_string s = env->intern(type_name);
  return lookup_typedef_type(s, tu);
}

/// Lookup a qualified type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the qualified type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the qualified type found or nil if no qualified type was
/// found.
qualified_type_def_sptr
lookup_qualified_type(const interned_string& type_name,
		      const translation_unit& tu)
{
  const type_maps& m = tu.get_types();
  return lookup_type_in_map<qualified_type_def>(type_name,
						m.qualified_types());
}

/// Lookup a qualified type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param underlying_type the underying type of the qualified type to
/// look up.
///
/// @param quals the CV-qualifiers of the qualified type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the qualified type found or nil if no qualified type was
/// found.
qualified_type_def_sptr
lookup_qualified_type(const type_base_sptr& underlying_type,
		      qualified_type_def::CV quals,
		      const translation_unit& tu)
{
  interned_string type_name = get_name_of_qualified_type(underlying_type,
							 quals);
  return lookup_qualified_type(type_name, tu);
}

/// Lookup a pointer type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the pointer type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the pointer type found or nil if no pointer type was
/// found.
pointer_type_def_sptr
lookup_pointer_type(const interned_string& type_name,
		    const translation_unit& tu)
{
  const type_maps& m = tu.get_types();
  return lookup_type_in_map<pointer_type_def>(type_name,
					      m.pointer_types());
}

/// Lookup a pointer type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the pointer type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the pointer type found or nil if no pointer type was
/// found.
pointer_type_def_sptr
lookup_pointer_type(const string& type_name, const translation_unit& tu)
{
  const environment* env = tu.get_environment();
  ABG_ASSERT(env);

  interned_string s = env->intern(type_name);
  return lookup_pointer_type(s, tu);
}

/// Lookup a pointer type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param pointed_to_type the pointed-to-type of the pointer to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the pointer type found or nil if no pointer type was
/// found.
pointer_type_def_sptr
lookup_pointer_type(const type_base_sptr& pointed_to_type,
		    const translation_unit& tu)
{
  interned_string type_name = get_name_of_pointer_to_type(*pointed_to_type);
  return lookup_pointer_type(type_name, tu);
}

/// Lookup a reference type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the reference type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the reference type found or nil if no reference type was
/// found.
reference_type_def_sptr
lookup_reference_type(const interned_string& type_name,
		      const translation_unit& tu)
{
  const type_maps& m = tu.get_types();
  return lookup_type_in_map<reference_type_def>(type_name,
						m.reference_types());
}

/// Lookup a reference type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param pointed_to_type the pointed-to-type of the reference to
/// look up.
///
/// @param tu the translation unit to look into.
///
/// @return the reference type found or nil if no reference type was
/// found.
const reference_type_def_sptr
lookup_reference_type(const type_base_sptr& pointed_to_type,
		      bool lvalue_reference,
		      const translation_unit& tu)
{
  interned_string type_name =
    get_name_of_reference_to_type(*pointed_to_type, lvalue_reference);
  return lookup_reference_type(type_name, tu);
}

/// Lookup an array type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the array type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the array type found or nil if no array type was found.
array_type_def_sptr
lookup_array_type(const interned_string& type_name,
		  const translation_unit& tu)
{
  const type_maps& m = tu.get_types();
  return lookup_type_in_map<array_type_def>(type_name,
					    m.array_types());
}

/// Lookup a function type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param type_name the name of the type to lookup.
///
/// @param tu the translation unit to look into.
///
/// @return the function type found, or NULL of none was found.
function_type_sptr
lookup_function_type(const interned_string& type_name,
		     const translation_unit& tu)
{
  const type_maps& m = tu.get_types();
  return lookup_type_in_map<function_type>(type_name,
					   m.function_types());
}

/// Lookup a function type from a translation unit.
///
/// This walks all the function types held by the translation unit and
/// compare their sub-type *names*.  If the names match then return
/// the function type found in the translation unit.
///
/// @param t the function type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the function type found, or NULL of none was found.
function_type_sptr
lookup_function_type(const function_type& t,
		     const translation_unit& tu)
{
  interned_string type_name = get_type_name(t);
  return lookup_function_type(type_name, tu);
}

/// Lookup a function type from a translation unit.
///
/// This is done by looking the type up in the type map that is
/// maintained in the translation unit.  So this is as fast as
/// possible.
///
/// @param t the function type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the function type found, or NULL of none was found.
function_type_sptr
lookup_function_type(const function_type_sptr& t,
		     const translation_unit& tu)
{return lookup_function_type(*t, tu);}

/// Lookup a type in a translation unit.
///
/// @param fqn the fully qualified name of the type to lookup.
///
/// @param tu the translation unit to consider.
///
/// @return the declaration of the type if found, NULL otherwise.
const type_base_sptr
lookup_type(const interned_string& fqn,
	    const translation_unit& tu)
{
  type_base_sptr result;
  ((result = lookup_typedef_type(fqn, tu))
   || (result = lookup_class_type(fqn, tu))
   || (result = lookup_union_type(fqn, tu))
   || (result = lookup_enum_type(fqn, tu))
   || (result = lookup_qualified_type(fqn, tu))
   || (result = lookup_pointer_type(fqn, tu))
   || (result = lookup_reference_type(fqn, tu))
   || (result = lookup_array_type(fqn, tu))
   || (result = lookup_function_type(fqn, tu))
   || (result = lookup_basic_type(fqn, tu)));

  return result;
}

/// Lookup a type in a translation unit, starting from the global
/// namespace.
///
/// @param fqn the fully qualified name of the type to lookup.
///
/// @param tu the translation unit to consider.
///
/// @return the declaration of the type if found, NULL otherwise.
type_base_sptr
lookup_type(const string& fqn, const translation_unit& tu)
{
  const environment *env = tu.get_environment();
  ABG_ASSERT(env);
  interned_string ifqn = env->intern(fqn);
  return lookup_type(ifqn, tu);
}

/// Lookup a type from a translation unit.
///
/// @param fqn the components of the fully qualified name of the node
/// to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the IR node found, NULL otherwise.
const type_base_sptr
lookup_type(const type_base_sptr type,
	    const translation_unit& tu)
{
  interned_string type_name = get_type_name(type);
  return lookup_type(type_name, tu);
}

/// Lookup a type in a scope.
///
/// This is really slow as it walks the member types of the scope in
/// sequence to find the type with a given name.
///
/// If possible, users should prefer looking up types from the
/// enclosing translation unit or even ABI corpus because both the
/// translation unit and the corpus have a map of type, indexed by
/// their name.  Looking up a type from those maps is thus much
/// faster.
///
/// @param fqn the fully qualified name of the type to lookup.
///
/// @param skope the scope to look into.
///
/// @return the declaration of the type if found, NULL otherwise.
const type_base_sptr
lookup_type_in_scope(const string& fqn,
		     const scope_decl_sptr& skope)
{
  list<string> comps;
  fqn_to_components(fqn, comps);
  return lookup_type_in_scope(comps, skope);
}

/// Lookup a @ref var_decl in a scope.
///
/// @param fqn the fuly qualified name of the @var_decl to lookup.
///
/// @param skope the scope to look into.
///
/// @return the declaration of the @ref var_decl if found, NULL
/// otherwise.
const decl_base_sptr
lookup_var_decl_in_scope(const string& fqn,
			 const scope_decl_sptr& skope)
{
  list<string> comps;
  fqn_to_components(fqn, comps);
  return lookup_var_decl_in_scope(comps, skope);
}

/// A generic function (template) to get the name of a node, whatever
/// node it is.  This has to be specialized for the kind of node we
/// want.
///
/// Note that a node is a member of a scope.
///
/// @tparam NodeKind the kind of node to consider.
///
/// @param node the node to get the name from.
///
/// @return the name of the node.
template<typename NodeKind>
static const interned_string&
get_node_name(shared_ptr<NodeKind> node);

/// Gets the name of a class_decl node.
///
/// @param node the decl_base node to get the name from.
///
/// @return the name of the node.
template<>
const interned_string&
get_node_name(class_decl_sptr node)
{return node->get_name();}

/// Gets the name of a type_base node.
///
/// @param node the type_base node to get the name from.
///
/// @return the name of the node.
template<>
const interned_string&
get_node_name(type_base_sptr node)
{return get_type_declaration(node)->get_name();}

/// Gets the name of a var_decl node.
///
/// @param node the var_decl node to get the name from.
///
/// @return the name of the node.
template<>
const interned_string&
get_node_name(var_decl_sptr node)
{return node->get_name();}

/// Generic function to get the declaration of a given node, whatever
/// it is.  There has to be specializations for the kind of the nodes
/// we want to support.
///
/// @tparam NodeKind the type of the node we are looking at.
///
/// @return the declaration.
template<typename NodeKind>
static decl_base_sptr
convert_node_to_decl(shared_ptr<NodeKind> node);

/// Lookup a node in a given scope.
///
/// @tparam the type of the node to lookup.
///
/// @param fqn the components of the fully qualified name of the node
/// to lookup.
///
/// @param skope the scope to look into.
///
/// @return the declaration of the looked up node, or NULL if it
/// wasn't found.
template<typename NodeKind>
static const type_or_decl_base_sptr
lookup_node_in_scope(const list<string>& fqn,
		     const scope_decl_sptr& skope)
{
  type_or_decl_base_sptr resulting_decl;
  shared_ptr<NodeKind> node;
  bool it_is_last = false;
  scope_decl_sptr cur_scope = skope, new_scope, scope;

  for (list<string>::const_iterator c = fqn.begin(); c != fqn.end(); ++c)
    {
      new_scope.reset();
      it_is_last = iterator_is_last(fqn, c);
      for (scope_decl::declarations::const_iterator m =
	     cur_scope->get_member_decls().begin();
	   m != cur_scope->get_member_decls().end();
	   ++m)
	{
	  if (!it_is_last)
	    {
	      // looking for a scope
	      scope = dynamic_pointer_cast<scope_decl>(*m);
	      if (scope && scope->get_name() == *c)
		{
		  new_scope = scope;
		  break;
		}
	    }
	  else
	    {
	      //looking for a final type.
	      node = dynamic_pointer_cast<NodeKind>(*m);
	      if (node && get_node_name(node) == *c)
		{
		  if (class_decl_sptr cl =
		      dynamic_pointer_cast<class_decl>(node))
		    if (cl->get_is_declaration_only()
			&& !cl->get_definition_of_declaration())
		      continue;
		  resulting_decl = node;
		  break;
		}
	    }
	}
      if (!new_scope && !resulting_decl)
	return decl_base_sptr();
      cur_scope = new_scope;
    }
  ABG_ASSERT(resulting_decl);
  return resulting_decl;
}

/// lookup a type in a scope.
///
///
/// This is really slow as it walks the member types of the scope in
/// sequence to find the type with a given name.
///
/// If possible, users should prefer looking up types from the
/// enclosing translation unit or even ABI corpus because both the
/// translation unit and the corpus have a map of type, indexed by
/// their name.  Looking up a type from those maps is thus much
/// faster.
///
/// @param comps the components of the fully qualified name of the
/// type to lookup.
///
/// @param skope the scope to look into.
///
/// @return the declaration of the type found.
const type_base_sptr
lookup_type_in_scope(const list<string>& comps,
		     const scope_decl_sptr& scope)
{return is_type(lookup_node_in_scope<type_base>(comps, scope));}

/// lookup a type in a scope.
///
/// This is really slow as it walks the member types of the scope in
/// sequence to find the type with a given name.
///
/// If possible, users should prefer looking up types from the
/// enclosing translation unit or even ABI corpus because both the
/// translation unit and the corpus have a map of type, indexed by
/// their name.  Looking up a type from those maps is thus much
/// faster.
///
/// @param type the type to look for.
///
/// @param access_path a vector of scopes the path of scopes to follow
/// before reaching the scope into which to look for @p type.  Note
/// that the deepest scope (the one immediately containing @p type) is
/// at index 0 of this vector, and the top-most scope is the last
/// element of the vector.
///
/// @param scope the top-most scope into which to look for @p type.
///
/// @return the scope found in @p scope, or NULL if it wasn't found.
static const type_base_sptr
lookup_type_in_scope(const type_base& type,
		     const vector<scope_decl*>& access_path,
		     const scope_decl* scope)
{
  vector<scope_decl*> a = access_path;
  type_base_sptr result;

  scope_decl* first_scope = 0;
  if (!a.empty())
    {
      first_scope = a.back();
      ABG_ASSERT(first_scope->get_name() == scope->get_name());
      a.pop_back();
    }

  if (a.empty())
    {
      interned_string n = get_type_name(type, false);
      for (scope_decl::declarations::const_iterator i =
	     scope->get_member_decls().begin();
	   i != scope->get_member_decls().end();
	   ++i)
	if (is_type(*i) && (*i)->get_name() == n)
	  {
	    result = is_type(*i);
	    break;
	  }
    }
  else
    {
      first_scope = a.back();
      interned_string scope_name, cur_scope_name = first_scope->get_name();
      for (scope_decl::scopes::const_iterator i =
	     scope->get_member_scopes().begin();
	   i != scope->get_member_scopes().end();
	   ++i)
	{
	  scope_name = (*i)->get_name();
	  if (scope_name == cur_scope_name)
	    {
	      result = lookup_type_in_scope(type, a, (*i).get());
	      break;
	    }
	}
    }
  return result;
}

/// lookup a type in a scope.
///
/// This is really slow as it walks the member types of the scope in
/// sequence to find the type with a given name.
///
/// If possible, users should prefer looking up types from the
/// enclosing translation unit or even ABI corpus because both the
/// translation unit and the corpus have a map of type, indexed by
/// their name.  Looking up a type from those maps is thus much
/// faster.
///
/// @param type the type to look for.
///
/// @param scope the top-most scope into which to look for @p type.
///
/// @return the scope found in @p scope, or NULL if it wasn't found.
static const type_base_sptr
lookup_type_in_scope(const type_base_sptr type,
		     const scope_decl* scope)
{
  if (!type || is_function_type(type))
    return type_base_sptr();

  decl_base_sptr type_decl = get_type_declaration(type);
  ABG_ASSERT(type_decl);
  vector<scope_decl*> access_path;
  for (scope_decl* s = type_decl->get_scope(); s != 0; s = s->get_scope())
    {
      access_path.push_back(s);
      if (is_global_scope(s))
	break;
    }
  return lookup_type_in_scope(*type, access_path, scope);
}

/// Lookup a type from a translation unit by walking the scopes of the
/// translation unit in sequence and looking into them.
///
/// This is really slow as it walks the member types of the scopes in
/// sequence to find the type with a given name.
///
/// If possible, users should prefer looking up types from the
/// translation unit or even ABI corpus in a more direct way, by using
/// the lookup_type() functins.
///
///
/// This is because both the translation unit and the corpus have a
/// map of types, indexed by their name.  Looking up a type from those
/// maps is thus much faster.  @param fqn the components of the fully
/// qualified name of the node to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the IR node found, NULL otherwise.
const type_base_sptr
lookup_type_through_scopes(const type_base_sptr type,
			   const translation_unit& tu)
{
  if (function_type_sptr fn_type = is_function_type(type))
    return lookup_function_type(fn_type, tu);
  return lookup_type_in_scope(type, tu.get_global_scope().get());
}

/// lookup a var_decl in a scope.
///
/// @param comps the components of the fully qualified name of the
/// var_decl to lookup.
///
/// @param skope the scope to look into.
const decl_base_sptr
lookup_var_decl_in_scope(const std::list<string>& comps,
			 const scope_decl_sptr& skope)
{return is_var_decl(lookup_node_in_scope<var_decl>(comps, skope));}

/// Lookup an IR node from a translation unit.
///
/// @tparam NodeKind the type of the IR node to lookup from the
/// translation unit.
///
/// @param fqn the components of the fully qualified name of the node
/// to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the IR node found, NULL otherwise.
template<typename NodeKind>
static const type_or_decl_base_sptr
lookup_node_in_translation_unit(const list<string>& fqn,
				const translation_unit& tu)
{return lookup_node_in_scope<NodeKind>(fqn, tu.get_global_scope());}

/// Lookup a type from a translation unit by walking its scopes in
/// sequence and by looking into them.
///
/// This is much slower than using the lookup_type() function.
///
/// @param fqn the components of the fully qualified name of the node
/// to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the IR node found, NULL otherwise.
type_base_sptr
lookup_type_through_scopes(const list<string>& fqn,
			   const translation_unit& tu)
{return is_type(lookup_node_in_translation_unit<type_base>(fqn, tu));}


/// Lookup a class type from a translation unit by walking its scopes
/// in sequence and by looking into them.
///
/// This is much slower than using the lookup_class_type() function
/// because it walks all the scopes of the translation unit in
/// sequence and lookup the types to find one that has a given name.
///
/// @param fqn the components of the fully qualified name of the class
/// type node to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the class type IR node found, NULL
/// otherwise.
class_decl_sptr
lookup_class_type_through_scopes(const list<string>& fqn,
				 const translation_unit& tu)
{return is_class_type(lookup_node_in_translation_unit<class_decl>(fqn, tu));}

/// Lookup a basic type from all the translation units of a given
/// corpus.
///
/// @param fqn the components of the fully qualified name of the basic
/// type node to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the basic type IR node found, NULL
/// otherwise.
static type_decl_sptr
lookup_basic_type_through_translation_units(const interned_string& type_name,
					    const corpus& abi_corpus)
{
  type_decl_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_basic_type(type_name, **tu)))
      break;

  return result;
}

/// Lookup a union type from all the translation units of a given
/// corpus.
///
/// @param fqn the components of the fully qualified name of the union
/// type node to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the union type IR node found, NULL
/// otherwise.
static union_decl_sptr
lookup_union_type_through_translation_units(const interned_string& type_name,
					    const corpus & abi_corpus)
{
 union_decl_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_union_type(type_name, **tu)))
      break;

  return result;
}

/// Lookup an enum type from all the translation units of a given
/// corpus.
///
/// @param fqn the components of the fully qualified name of the enum
/// type node to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the enum type IR node found, NULL
/// otherwise.
static enum_type_decl_sptr
lookup_enum_type_through_translation_units(const interned_string& type_name,
					   const corpus & abi_corpus)
{
  enum_type_decl_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_enum_type(type_name, **tu)))
      break;

  return result;
}

/// Lookup a typedef type definition in all the translation units of a
/// given ABI corpus.
///
/// @param @param qn the fully qualified name of the typedef type to lookup.
///
/// @param abi_corpus the ABI corpus which to look the type up in.
///
/// @return the type definition if any was found, or a NULL pointer.
static typedef_decl_sptr
lookup_typedef_type_through_translation_units(const interned_string& type_name,
					      const corpus & abi_corpus)
{
  typedef_decl_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_typedef_type(type_name, **tu)))
      break;

  return result;
}

/// Lookup a qualified type definition in all the translation units of a
/// given ABI corpus.
///
/// @param @param qn the fully qualified name of the qualified type to
/// lookup.
///
/// @param abi_corpus the ABI corpus which to look the type up in.
///
/// @return the type definition if any was found, or a NULL pointer.
static qualified_type_def_sptr
lookup_qualified_type_through_translation_units(const interned_string& t_name,
						const corpus & abi_corpus)
{
  qualified_type_def_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_qualified_type(t_name, **tu)))
      break;

  return result;
}

/// Lookup a pointer type definition in all the translation units of a
/// given ABI corpus.
///
/// @param @param qn the fully qualified name of the pointer type to
/// lookup.
///
/// @param abi_corpus the ABI corpus which to look the type up in.
///
/// @return the type definition if any was found, or a NULL pointer.
static pointer_type_def_sptr
lookup_pointer_type_through_translation_units(const interned_string& type_name,
					      const corpus & abi_corpus)
{
  pointer_type_def_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_pointer_type(type_name, **tu)))
      break;

  return result;
}

/// Lookup a reference type definition in all the translation units of a
/// given ABI corpus.
///
/// @param @param qn the fully qualified name of the reference type to
/// lookup.
///
/// @param abi_corpus the ABI corpus which to look the type up in.
///
/// @return the type definition if any was found, or a NULL pointer.
static reference_type_def_sptr
lookup_reference_type_through_translation_units(const interned_string& t_name,
						const corpus & abi_corpus)
{
  reference_type_def_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_reference_type(t_name, **tu)))
      break;

  return result;
}

/// Lookup a array type definition in all the translation units of a
/// given ABI corpus.
///
/// @param @param qn the fully qualified name of the array type to
/// lookup.
///
/// @param abi_corpus the ABI corpus which to look the type up in.
///
/// @return the type definition if any was found, or a NULL pointer.
static array_type_def_sptr
lookup_array_type_through_translation_units(const interned_string& type_name,
					    const corpus & abi_corpus)
{
  array_type_def_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_array_type(type_name, **tu)))
      break;

  return result;
}

/// Lookup a function type definition in all the translation units of
/// a given ABI corpus.
///
/// @param @param qn the fully qualified name of the function type to
/// lookup.
///
/// @param abi_corpus the ABI corpus which to look the type up in.
///
/// @return the type definition if any was found, or a NULL pointer.
static function_type_sptr
lookup_function_type_through_translation_units(const interned_string& type_name,
					       const corpus & abi_corpus)
{
  function_type_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_function_type(type_name, **tu)))
      break;

  return result;
}

/// Lookup a type definition in all the translation units of a given
/// ABI corpus.
///
/// @param @param qn the fully qualified name of the type to lookup.
///
/// @param abi_corpus the ABI corpus which to look the type up in.
///
/// @return the type definition if any was found, or a NULL pointer.
type_base_sptr
lookup_type_through_translation_units(const string& qn,
				      const corpus& abi_corpus)
{
  type_base_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_type(qn, **tu)))
      break;

  return result;
}

/// Lookup a type from a given translation unit present in a give corpus.
///
/// @param type_name the name of the type to look for.
///
/// @parm tu_path the path of the translation unit to consider.
///
/// @param corp the corpus to consider.
///
/// @return the resulting type, if any.
type_base_sptr
lookup_type_from_translation_unit(const string& type_name,
				  const string& tu_path,
				  const corpus& corp)
{
  string_tu_map_type::const_iterator i =  corp.priv_->path_tu_map.find(tu_path);
  if (i == corp.priv_->path_tu_map.end())
    return type_base_sptr();

  translation_unit_sptr tu = i->second;
  ABG_ASSERT(tu);

  type_base_sptr t = lookup_type(type_name, *tu);
  return t;
}

/// Look into an ABI corpus for a function type.
///
/// @param fn_type the function type to be looked for in the ABI
/// corpus.
///
/// @param corpus the ABI corpus into which to look for the function
/// type.
///
/// @return the function type found in the corpus.
function_type_sptr
lookup_or_synthesize_fn_type(const function_type_sptr& fn_t,
			     const corpus& corpus)
{
  ABG_ASSERT(fn_t);

  function_type_sptr result;

  if ((result = lookup_function_type(fn_t, corpus)))
    return result;

  for (translation_units::const_iterator i =
	 corpus.get_translation_units().begin();
       i != corpus.get_translation_units().end();
       ++i)
    if ((result = synthesize_function_type_from_translation_unit(*fn_t,
								 **i)))
      return result;

  return result;
}

/// Look into a given corpus to find a type which has the same
/// qualified name as a giventype.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param t the type which has the same qualified name as the type we
/// are looking for.
///
/// @param corp the ABI corpus to look into for the type.
type_decl_sptr
lookup_basic_type(const type_decl& t, const corpus& corp)
{return lookup_basic_type(t.get_name(), corp);}

/// Look into a given corpus to find a basic type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the basic type to look
/// for.
///
/// @param corp the corpus to look into.
type_decl_sptr
lookup_basic_type(const interned_string &qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().basic_types();
  type_decl_sptr result;

  if (!m.empty())
    result = lookup_type_in_map<type_decl>(qualified_name, m);
  else
    result = lookup_basic_type_through_translation_units(qualified_name, corp);

  return result;
}

/// Lookup a @ref type_decl type from a given corpus, by its location.
///
/// @param loc the location to consider.
///
/// @param corp the corpus to consider.
///
/// @return the resulting basic type, if any.
type_decl_sptr
lookup_basic_type_per_location(const interned_string &loc,
			       const corpus &corp)
{
  const istring_type_base_wptrs_map_type& m =
    corp.get_type_per_loc_map().basic_types();
  type_decl_sptr result;

  result = lookup_type_in_map<type_decl>(loc, m);

  return result;
}

/// Lookup a @ref type_decl type from a given corpus, by its location.
///
/// @param loc the location to consider.
///
/// @param corp the corpus to consider.
///
/// @return the resulting basic type, if any.
type_decl_sptr
lookup_basic_type_per_location(const string &loc, const corpus &corp)
{
  const environment* env = corp.get_environment();
  ABG_ASSERT(env);

  return lookup_basic_type_per_location(env->intern(loc), corp);
}

/// Look into a given corpus to find a basic type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the basic type to look
/// for.
///
/// @param corp the corpus to look into.
type_decl_sptr
lookup_basic_type(const string& qualified_name, const corpus& corp)
{
  return lookup_basic_type(corp.get_environment()->intern(qualified_name),
			   corp);
}

/// Look into a given corpus to find a class type which has the same
/// qualified name as a given type.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param t the class decl type which has the same qualified name as
/// the type we are looking for.
///
/// @param corp the corpus to look into.
class_decl_sptr
lookup_class_type(const class_decl& t, const corpus& corp)
{
  interned_string s = get_type_name(t);
  return lookup_class_type(s, corp);
}

/// Look into a given corpus to find a class type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
class_decl_sptr
lookup_class_type(const string& qualified_name, const corpus& corp)
{
  interned_string s = corp.get_environment()->intern(qualified_name);
  return lookup_class_type(s, corp);
}

/// Look into a given corpus to find a class type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
class_decl_sptr
lookup_class_type(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().class_types();

  class_decl_sptr result = lookup_type_in_map<class_decl>(qualified_name, m);

  return result;
}

/// Look into a given corpus to find the class type*s* that have a
/// given qualified name.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
///
/// @return the vector of class types named @p qualified_name.
const type_base_wptrs_type *
lookup_class_types(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().class_types();

  return lookup_types_in_map(qualified_name, m);
}

/// Look into a given corpus to find the class type*s* that have a
/// given qualified name.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
///
/// @return the vector of class types that which name is @p qualified_name.
const type_base_wptrs_type*
lookup_class_types(const string& qualified_name, const corpus& corp)
{
  interned_string s = corp.get_environment()->intern(qualified_name);
  return lookup_class_types(s, corp);
}

/// Look up a @ref class_decl from a given corpus by its location.
///
/// @param loc the location to consider.
///
/// @param corp the corpus to consider.
///
/// @return the resulting class decl, if any.
class_decl_sptr
lookup_class_type_per_location(const interned_string& loc,
			       const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m =
    corp.get_type_per_loc_map().class_types();
  class_decl_sptr result = lookup_type_in_map<class_decl>(loc, m);

  return result;
}

/// Look up a @ref class_decl from a given corpus by its location.
///
/// @param loc the location to consider.
///
/// @param corp the corpus to consider.
///
/// @return the resulting class decl, if any.
class_decl_sptr
lookup_class_type_per_location(const string &loc, const corpus &corp)
{
  const environment* env = corp.get_environment();
  ABG_ASSERT(env);

  return lookup_class_type_per_location(env->intern(loc), corp);
}

/// Look into a given corpus to find a union type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
union_decl_sptr
lookup_union_type(const interned_string& type_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().union_types();

  union_decl_sptr result = lookup_type_in_map<union_decl>(type_name, m);
  if (!result)
    result = lookup_union_type_through_translation_units(type_name, corp);

  return result;
}

/// Look into a given corpus to find a union type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
union_decl_sptr
lookup_union_type(const string& type_name, const corpus& corp)
{
  interned_string s = corp.get_environment()->intern(type_name);
  return lookup_union_type(s, corp);
}

/// Look into a given corpus to find an enum type which has the same
/// qualified name as a given enum type.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param t the enum type which has the same qualified name as the
/// type we are looking for.
///
/// @param corp the corpus to look into.
enum_type_decl_sptr
lookup_enum_type(const enum_type_decl& t, const corpus& corp)
{
  interned_string s = get_type_name(t);
  return lookup_enum_type(s, corp);
}

/// Look into a given corpus to find an enum type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the enum type to look
/// for.
///
/// @param corp the corpus to look into.
enum_type_decl_sptr
lookup_enum_type(const string& qualified_name, const corpus& corp)
{
  interned_string s = corp.get_environment()->intern(qualified_name);
  return lookup_enum_type(s, corp);
}

/// Look into a given corpus to find an enum type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the enum type to look
/// for.
///
/// @param corp the corpus to look into.
enum_type_decl_sptr
lookup_enum_type(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().enum_types();

  enum_type_decl_sptr result =
    lookup_type_in_map<enum_type_decl>(qualified_name, m);
  if (!result)
    result = lookup_enum_type_through_translation_units(qualified_name, corp);

  return result;
}

/// Look into a given corpus to find the enum type*s* that have a
/// given qualified name.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
///
/// @return the vector of enum types that which name is @p qualified_name.
const type_base_wptrs_type *
lookup_enum_types(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().enum_types();

  return lookup_types_in_map(qualified_name, m);
}

/// Look into a given corpus to find the enum type*s* that have a
/// given qualified name.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
///
/// @return the vector of enum types that which name is @p qualified_name.
const type_base_wptrs_type*
lookup_enum_types(const string& qualified_name, const corpus& corp)
{
  interned_string s = corp.get_environment()->intern(qualified_name);
  return lookup_enum_types(s, corp);
}

/// Look up an @ref enum_type_decl from a given corpus, by its location.
///
/// @param loc the location to consider.
///
/// @param corp the corpus to look the type from.
///
/// @return the resulting enum type, if any.
enum_type_decl_sptr
lookup_enum_type_per_location(const interned_string &loc, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m =
    corp.get_type_per_loc_map().enum_types();
  enum_type_decl_sptr result = lookup_type_in_map<enum_type_decl>(loc, m);

  return result;
}

/// Look up an @ref enum_type_decl from a given corpus, by its location.
///
/// @param loc the location to consider.
///
/// @param corp the corpus to look the type from.
///
/// @return the resulting enum type, if any.
enum_type_decl_sptr
lookup_enum_type_per_location(const string &loc, const corpus &corp)
{
  const environment* env = corp.get_environment();
  ABG_ASSERT(env);

  return lookup_enum_type_per_location(env->intern(loc), corp);
}

/// Look into a given corpus to find a typedef type which has the
/// same qualified name as a given typedef type.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param t the typedef type which has the same qualified name as the
/// typedef type we are looking for.
///
/// @param corp the corpus to look into.
typedef_decl_sptr
lookup_typedef_type(const typedef_decl& t, const corpus& corp)
{
  interned_string s = get_type_name(t);
  return lookup_typedef_type(s, corp);
}

/// Look into a given corpus to find a typedef type which has the
/// same qualified name as a given typedef type.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param t the typedef type which has the same qualified name as the
/// typedef type we are looking for.
///
/// @param corp the corpus to look into.
typedef_decl_sptr
lookup_typedef_type(const string& qualified_name, const corpus& corp)
{
  interned_string s = corp.get_environment()->intern(qualified_name);
  return lookup_typedef_type(s, corp);
}

/// Look into a given corpus to find a typedef type which has a
/// given qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the typedef type to
/// look for.
///
/// @param corp the corpus to look into.
typedef_decl_sptr
lookup_typedef_type(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().typedef_types();

  typedef_decl_sptr result =
    lookup_type_in_map<typedef_decl>(qualified_name, m);
  if (!result)
    result = lookup_typedef_type_through_translation_units(qualified_name,
							   corp);

  return result;
}

/// Lookup a @ref typedef_decl from a corpus, by its location.
///
/// @param loc the location to consider.
///
/// @param corp the corpus to consider.
///
/// @return the typedef_decl found, if any.
typedef_decl_sptr
lookup_typedef_type_per_location(const interned_string &loc, const corpus &corp)
{
  const istring_type_base_wptrs_map_type& m =
    corp.get_type_per_loc_map().typedef_types();
  typedef_decl_sptr result = lookup_type_in_map<typedef_decl>(loc, m);

  return result;
}

/// Lookup a @ref typedef_decl from a corpus, by its location.
///
/// @param loc the location to consider.
///
/// @param corp the corpus to consider.
///
/// @return the typedef_decl found, if any.
typedef_decl_sptr
lookup_typedef_type_per_location(const string &loc, const corpus &corp)
{
  const environment* env = corp.get_environment();
  ABG_ASSERT(env);

  return lookup_typedef_type_per_location(env->intern(loc), corp);
}

/// Look into a corpus to find a class, union or typedef type which
/// has a given qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the name of the type to find.
///
/// @param corp the corpus to look into.
///
/// @return the typedef or class type found.
type_base_sptr
lookup_class_or_typedef_type(const string& qualified_name, const corpus& corp)
{
  type_base_sptr result = lookup_class_type(qualified_name, corp);
  if (!result)
    result = lookup_union_type(qualified_name, corp);

  if (!result)
    result = lookup_typedef_type(qualified_name, corp);
  return result;
}

/// Look into a corpus to find a class, typedef or enum type which has
/// a given qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
///
/// @return the typedef, class or enum type found.
type_base_sptr
lookup_class_typedef_or_enum_type(const string& qualified_name,
				  const corpus& corp)
{
  type_base_sptr result = lookup_class_or_typedef_type(qualified_name, corp);
  if (!result)
    result = lookup_enum_type(qualified_name, corp);

  return result;
}

/// Look into a given corpus to find a qualified type which has the
/// same qualified name as a given type.
///
/// @param t the type which has the same qualified name as the
/// qualified type we are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the qualified type found.
qualified_type_def_sptr
lookup_qualified_type(const qualified_type_def& t, const corpus& corp)
{
  interned_string s = get_type_name(t);
  return lookup_qualified_type(s, corp);
}

/// Look into a given corpus to find a qualified type which has a
/// given qualified name.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
///
/// @return the type found.
qualified_type_def_sptr
lookup_qualified_type(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m =
    corp.get_types().qualified_types();

  qualified_type_def_sptr result =
    lookup_type_in_map<qualified_type_def>(qualified_name, m);

  if (!result)
    result = lookup_qualified_type_through_translation_units(qualified_name,
							     corp);

  return result;
}

/// Look into a given corpus to find a pointer type which has the same
/// qualified name as a given pointer type.
///
/// @param t the pointer type which has the same qualified name as the
/// type we are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the pointer type found.
pointer_type_def_sptr
lookup_pointer_type(const pointer_type_def& t, const corpus& corp)
{
  interned_string s = get_type_name(t);
  return lookup_pointer_type(s, corp);
}

/// Look into a given corpus to find a pointer type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the pointer type to
/// look for.
///
/// @param corp the corpus to look into.
///
/// @return the pointer type found.
pointer_type_def_sptr
lookup_pointer_type(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().pointer_types();

  pointer_type_def_sptr result =
    lookup_type_in_map<pointer_type_def>(qualified_name, m);
  if (!result)
    result = lookup_pointer_type_through_translation_units(qualified_name,
							   corp);

  return result;
}

/// Look into a given corpus to find a reference type which has the
/// same qualified name as a given reference type.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param t the reference type which has the same qualified name as
/// the reference type we are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the reference type found.
reference_type_def_sptr
lookup_reference_type(const reference_type_def& t, const corpus& corp)
{
  interned_string s = get_type_name(t);
  return lookup_reference_type(s, corp);
}

/// Look into a given corpus to find a reference type which has a
/// given qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the reference type to
/// look for.
///
/// @param corp the corpus to look into.
///
/// @return the reference type found.
reference_type_def_sptr
lookup_reference_type(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m =
    corp.get_types().reference_types();

  reference_type_def_sptr result =
    lookup_type_in_map<reference_type_def>(qualified_name, m);
  if (!result)
    result = lookup_reference_type_through_translation_units(qualified_name,
							     corp);

  return result;
}

/// Look into a given corpus to find an array type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the array type to look
/// for.
///
/// @param corp the corpus to look into.
///
/// @return the array type found.
array_type_def_sptr
lookup_array_type(const array_type_def& t, const corpus& corp)
{
  interned_string s = get_type_name(t);
  return lookup_array_type(s, corp);
}

/// Look into a given corpus to find an array type which has the same
/// qualified name as a given array type.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param t the type which has the same qualified name as the type we
/// are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the type found.
array_type_def_sptr
lookup_array_type(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().array_types();

  array_type_def_sptr result =
    lookup_type_in_map<array_type_def>(qualified_name, m);
  if (!result)
    result = lookup_array_type_through_translation_units(qualified_name, corp);

  return result;
}

/// Look into a given corpus to find a function type which has the same
/// qualified name as a given function type.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param t the function type which has the same qualified name as
/// the function type we are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the function type found.
function_type_sptr
lookup_function_type(const function_type&t, const corpus& corp)
{
  interned_string type_name = get_type_name(t);
  return lookup_function_type(type_name, corp);
}

/// Look into a given corpus to find a function type which has the same
/// qualified name as a given function type.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param t the function type which has the same qualified name as
/// the function type we are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the function type found.
function_type_sptr
lookup_function_type(const function_type_sptr& fn_t,
		     const corpus& corpus)
{
  if (fn_t)
    return lookup_function_type(*fn_t, corpus);
  return function_type_sptr();
}

/// Look into a given corpus to find a function type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the function type to
/// look for.
///
/// @param corp the corpus to look into.
///
/// @return the function type found.
function_type_sptr
lookup_function_type(const interned_string& qualified_name, const corpus& corp)
{
  const istring_type_base_wptrs_map_type& m = corp.get_types().function_types();

  function_type_sptr result =
    lookup_type_in_map<function_type>(qualified_name, m);
  if (!result)
    result = lookup_function_type_through_translation_units(qualified_name,
							    corp);

  return result;
}

/// Look into a given corpus to find a type which has a given
/// qualified name.
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the function type to
/// look for.
///
/// @param corp the corpus to look into.
///
/// @return the function type found.
type_base_sptr
lookup_type(const interned_string& n, const corpus& corp)
{
  type_base_sptr result;

  ((result = lookup_basic_type(n, corp))
   || (result = lookup_class_type(n, corp))
   || (result = lookup_union_type(n, corp))
   || (result = lookup_enum_type(n, corp))
   || (result = lookup_typedef_type(n, corp))
   || (result = lookup_qualified_type(n, corp))
   || (result = lookup_pointer_type(n, corp))
   || (result = lookup_reference_type(n, corp))
   || (result = lookup_array_type(n, corp))
   || (result= lookup_function_type(n, corp)));

  return result;
}

/// Lookup a type from a corpus, by its location.
///
/// @param loc the location to consider.
///
/// @param corp the corpus to look the type from.
///
/// @return the resulting type, if any found.
type_base_sptr
lookup_type_per_location(const interned_string& loc, const corpus& corp)
{
  // TODO: finish this.

  //TODO: when we fully support types indexed by their location, this
  //function should return a vector of types because at each location,
  //there can be several types that are defined (yay, C and C++,
  //*sigh*).

  type_base_sptr result;
  ((result = lookup_basic_type_per_location(loc, corp))
   || (result = lookup_class_type_per_location(loc, corp))
   || (result = lookup_union_type_per_location(loc, corp))
   || (result = lookup_enum_type_per_location(loc, corp))
   || (result = lookup_typedef_type_per_location(loc, corp)));

  return result;
}

/// Look into a given corpus to find a type
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the function type to
/// look for.
///
/// @param corp the corpus to look into.
///
/// @return the function type found.
type_base_sptr
lookup_type(const type_base&t, const corpus& corp)
{
  interned_string n = get_type_name(t);
  return lookup_type(n, corp);
}

/// Look into a given corpus to find a type
///
/// If the per-corpus type map is non-empty (because the corpus allows
/// the One Definition Rule) then the type islooked up in that
/// per-corpus type map.  Otherwise, the type is looked-up in each
/// translation unit.
///
/// @param qualified_name the qualified name of the function type to
/// look for.
///
/// @param corp the corpus to look into.
///
/// @return the function type found.
type_base_sptr
lookup_type(const type_base_sptr&t, const corpus& corp)
{
  if (t)
    return lookup_type(*t, corp);
  return type_base_sptr();
}

/// Update the map that associates a fully qualified name of a given
/// type to that type.
///
///
/// @param type the type we are considering.
///
/// @param types_map the map to update.  It's a map that assciates a
/// fully qualified name of a type to the type itself.
///
/// @param use_type_name_as_key if true, use the name of the type as
/// the key to look it up later.  If false, then use the location of
/// the type as a key to look it up later.
///
/// @return true iff the type was added to the map.
template<typename TypeKind>
bool
maybe_update_types_lookup_map(const shared_ptr<TypeKind>& type,
			      istring_type_base_wptrs_map_type& types_map,
			      bool use_type_name_as_key = true)
{
  interned_string s;

  if (use_type_name_as_key)
    s = get_type_name(type);
  else if (location l = type->get_location())
    {
      string str = l.expand();
      s = type->get_environment()->intern(str);
    }

  istring_type_base_wptrs_map_type::iterator i = types_map.find(s);
  bool result = false;

  if (i == types_map.end())
    {
      types_map[s].push_back(type);
      result = true;
    }
  else
    i->second.push_back(type);

  return result;
}

/// This is the specialization for type @ref class_decl of the
/// function template:
///
///    maybe_update_types_lookup_map<T>(scope_decl*,
///					const shared_ptr<T>&,
///					istring_type_base_wptrs_map_type&)
///
/// @param class_type the type to consider.
///
/// @param types_map the type map to update.
///
/// @return true iff the type was added to the map.
template<>
bool
maybe_update_types_lookup_map<class_decl>(const class_decl_sptr& class_type,
					  istring_type_base_wptrs_map_type& map,
					  bool use_type_name_as_key)
{
  class_decl_sptr type = class_type;

  bool update_qname_map = true;
  if (type->get_is_declaration_only())
    {
      if (class_decl_sptr def =
	  is_class_type(class_type->get_definition_of_declaration()))
	type = def;
      else
	update_qname_map = false;
    }

  if (!update_qname_map)
    return false;

  interned_string s;
  if (use_type_name_as_key)
    {
      string qname = type->get_qualified_name();
      s = type->get_environment()->intern(qname);
    }
  else if (location l = type->get_location())
    {
      string str = l.expand();
      s = type->get_environment()->intern(str);
    }

  bool result = false;
  istring_type_base_wptrs_map_type::iterator i = map.find(s);
  if (i == map.end())
    {
      map[s].push_back(type);
      result = true;
    }
  else
    i->second.push_back(type);

  return result;
}

/// This is the specialization for type @ref function_type of the
/// function template:
///
///    maybe_update_types_lookup_map<T>(scope_decl*,
///					const shared_ptr<T>&,
///					istring_type_base_wptrs_map_type&)
///
/// @param scope the scope of the type to consider.
///
/// @param class_type the type to consider.
///
/// @param types_map the type map to update.
///
/// @return true iff the type was added to the map.
template<>
bool
maybe_update_types_lookup_map<function_type>
(const function_type_sptr& type,
 istring_type_base_wptrs_map_type& types_map,
 bool /*use_type_name_as_key*/)
{
  bool result = false;
  interned_string s = get_type_name(type);
  istring_type_base_wptrs_map_type::iterator i = types_map.find(s);
  if (i == types_map.end())
    {
      types_map[s].push_back(type);
      result = true;
    }
  else
    i->second.push_back(type);

  return result;
}

/// Update the map that associates the fully qualified name of a basic
/// type with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param basic_type the basic type to consider.
void
maybe_update_types_lookup_map(const type_decl_sptr& basic_type)
{
  if (translation_unit *tu = basic_type->get_translation_unit())
    maybe_update_types_lookup_map<type_decl>
      (basic_type, tu->get_types().basic_types());

  if (corpus *type_corpus = basic_type->get_corpus())
    {
      maybe_update_types_lookup_map<type_decl>
	(basic_type,
	 type_corpus->priv_->get_types().basic_types());

      maybe_update_types_lookup_map<type_decl>
	(basic_type,
	 type_corpus->get_type_per_loc_map().basic_types(),
	 /*use_type_name_as_key*/false);

      if (corpus *group = type_corpus->get_group())
	{
	  maybe_update_types_lookup_map<type_decl>
	    (basic_type,
	     group->priv_->get_types().basic_types());

	  maybe_update_types_lookup_map<type_decl>
	    (basic_type,
	     group->get_type_per_loc_map().basic_types(),
	     /*use_type_name_as_key*/false);
	}
    }

}

/// Update the map that associates the fully qualified name of a class
/// type with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param class_type the class type to consider.
void
maybe_update_types_lookup_map(const class_decl_sptr& class_type)
{
  if (translation_unit *tu = class_type->get_translation_unit())
    maybe_update_types_lookup_map<class_decl>
      (class_type, tu->get_types().class_types());

  if (corpus *type_corpus = class_type->get_corpus())
    {
      maybe_update_types_lookup_map<class_decl>
	(class_type,
	 type_corpus->priv_->get_types().class_types());

      maybe_update_types_lookup_map<class_decl>
	(class_type,
	 type_corpus->get_type_per_loc_map().class_types(),
	 /*use_type_name_as_key*/false);

      if (corpus *group = type_corpus->get_group())
	{
	  maybe_update_types_lookup_map<class_decl>
	    (class_type,
	     group->priv_->get_types().class_types());

	  maybe_update_types_lookup_map<class_decl>
	    (class_type,
	     group->get_type_per_loc_map().class_types(),
	     /*use_type_name_as_key*/false);
	}
    }
}

/// Update the map that associates the fully qualified name of a union
/// type with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param union_type the union type to consider.
void
maybe_update_types_lookup_map(const union_decl_sptr& union_type)
{
  if (translation_unit *tu = union_type->get_translation_unit())
    maybe_update_types_lookup_map<union_decl>
      (union_type, tu->get_types().union_types());

  if (corpus *type_corpus = union_type->get_corpus())
    {
      maybe_update_types_lookup_map<union_decl>
	(union_type,
	 type_corpus->priv_->get_types().union_types());

      maybe_update_types_lookup_map<union_decl>
	(union_type,
	 type_corpus->get_type_per_loc_map().union_types(),
	 /*use_type_name_as_key*/false);

      if (corpus *group = type_corpus->get_group())
	{
	  maybe_update_types_lookup_map<union_decl>
	    (union_type,
	     group->priv_->get_types().union_types());

	  maybe_update_types_lookup_map<union_decl>
	    (union_type,
	     group->get_type_per_loc_map().union_types(),
	     /*use_type_name_as_key*/false);
	}
    }
}

/// Update the map that associates the fully qualified name of an enum
/// type with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param enum_type the type to consider.
void
maybe_update_types_lookup_map(const enum_type_decl_sptr& enum_type)
{
  if (translation_unit *tu = enum_type->get_translation_unit())
    maybe_update_types_lookup_map<enum_type_decl>
      (enum_type, tu->get_types().enum_types());

  if (corpus *type_corpus = enum_type->get_corpus())
    {
      maybe_update_types_lookup_map<enum_type_decl>
	(enum_type,
	 type_corpus->priv_->get_types().enum_types());

      maybe_update_types_lookup_map<enum_type_decl>
	(enum_type,
	 type_corpus->get_type_per_loc_map().enum_types(),
	 /*use_type_name_as_key*/false);

      if (corpus *group = type_corpus->get_group())
	{
	  maybe_update_types_lookup_map<enum_type_decl>
	    (enum_type,
	     group->priv_->get_types().enum_types());

	  maybe_update_types_lookup_map<enum_type_decl>
	    (enum_type,
	     group->get_type_per_loc_map().enum_types(),
	     /*use_type_name_as_key*/false);
	}
    }

}

/// Update the map that associates the fully qualified name of a
/// typedef type with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param typedef_type the type to consider.
void
maybe_update_types_lookup_map(const typedef_decl_sptr& typedef_type)
{
  if (translation_unit *tu = typedef_type->get_translation_unit())
    maybe_update_types_lookup_map<typedef_decl>
      (typedef_type, tu->get_types().typedef_types());

  if (corpus *type_corpus = typedef_type->get_corpus())
    {
      maybe_update_types_lookup_map<typedef_decl>
	(typedef_type,
	 type_corpus->priv_->get_types().typedef_types());

      maybe_update_types_lookup_map<typedef_decl>
	(typedef_type,
	 type_corpus->get_type_per_loc_map().typedef_types(),
	 /*use_type_name_as_key*/false);

      if (corpus *group = type_corpus->get_group())
	{
	  maybe_update_types_lookup_map<typedef_decl>
	    (typedef_type,
	     group->priv_->get_types().typedef_types());

	  maybe_update_types_lookup_map<typedef_decl>
	    (typedef_type,
	     group->get_type_per_loc_map().typedef_types(),
	     /*use_type_name_as_key*/false);
	}
    }
}

/// Update the map that associates the fully qualified name of a
/// qualified type with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param qualified_type the type to consider.
void
maybe_update_types_lookup_map(const qualified_type_def_sptr& qualified_type)
{
  if (translation_unit *tu = qualified_type->get_translation_unit())
    maybe_update_types_lookup_map<qualified_type_def>
      (qualified_type, tu->get_types().qualified_types());

  if (corpus *type_corpus = qualified_type->get_corpus())
    {
      maybe_update_types_lookup_map<qualified_type_def>
	(qualified_type,
	 type_corpus->priv_->get_types().qualified_types());

      if (corpus *group = type_corpus->get_group())
	{
	  maybe_update_types_lookup_map<qualified_type_def>
	    (qualified_type,
	     group->priv_->get_types().qualified_types());
	}
    }
}

/// Update the map that associates the fully qualified name of a
/// pointer type with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param pointer_type the type to consider.
void
maybe_update_types_lookup_map(const pointer_type_def_sptr& pointer_type)
{
  if (translation_unit *tu = pointer_type->get_translation_unit())
    maybe_update_types_lookup_map<pointer_type_def>
      (pointer_type, tu->get_types().pointer_types());

  if (corpus *type_corpus = pointer_type->get_corpus())
    {
      maybe_update_types_lookup_map<pointer_type_def>
	(pointer_type,
	 type_corpus->priv_->get_types().pointer_types());

      if (corpus *group = type_corpus->get_group())
	{
	  maybe_update_types_lookup_map<pointer_type_def>
	    (pointer_type,
	     group->priv_->get_types().pointer_types());
	}
    }
}

/// Update the map that associates the fully qualified name of a
/// reference type with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param reference_type the type to consider.
void
maybe_update_types_lookup_map(const reference_type_def_sptr& reference_type)
{
  if (translation_unit *tu = reference_type->get_translation_unit())
    maybe_update_types_lookup_map<reference_type_def>
      (reference_type, tu->get_types().reference_types());

  if (corpus *type_corpus = reference_type->get_corpus())
    {
      maybe_update_types_lookup_map<reference_type_def>
	(reference_type,
	 type_corpus->priv_->get_types().reference_types());

      if (corpus *group = type_corpus->get_group())
	{
	  maybe_update_types_lookup_map<reference_type_def>
	    (reference_type,
	     group->priv_->get_types().reference_types());
	}
    }
}

/// Update the map that associates the fully qualified name of a type
/// with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param array_type the type to consider.
void
maybe_update_types_lookup_map(const array_type_def_sptr& array_type)
{
  if (translation_unit *tu = array_type->get_translation_unit())
    maybe_update_types_lookup_map<array_type_def>
      (array_type, tu->get_types().array_types());

  if (corpus *type_corpus = array_type->get_corpus())
    {
      maybe_update_types_lookup_map<array_type_def>
	(array_type,
	 type_corpus->priv_->get_types().array_types());

      maybe_update_types_lookup_map<array_type_def>
	(array_type,
	 type_corpus->get_type_per_loc_map().array_types(),
	 /*use_type_name_as_key*/false);

      if (corpus *group = type_corpus->get_group())
	{
	  maybe_update_types_lookup_map<array_type_def>
	    (array_type,
	     group->priv_->get_types().array_types());

	  maybe_update_types_lookup_map<array_type_def>
	    (array_type,
	     group->get_type_per_loc_map().array_types(),
	     /*use_type_name_as_key*/false);
	}
    }
}

/// Update the map that associates the fully qualified name of a type
/// with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param subrange_type the type to consider.
void
maybe_update_types_lookup_map
(const array_type_def::subrange_sptr& subrange_type)
{
  if (translation_unit *tu = subrange_type->get_translation_unit())
    maybe_update_types_lookup_map<array_type_def::subrange_type>
      (subrange_type, tu->get_types().subrange_types());

  if (corpus *type_corpus = subrange_type->get_corpus())
    {
      maybe_update_types_lookup_map<array_type_def::subrange_type>
	(subrange_type,
	 type_corpus->priv_->get_types().subrange_types());

      maybe_update_types_lookup_map<array_type_def::subrange_type>
	(subrange_type,
	 type_corpus->get_type_per_loc_map().subrange_types(),
	 /*use_type_name_as_key*/false);

      if (corpus *group = subrange_type->get_corpus())
	{
	  maybe_update_types_lookup_map<array_type_def::subrange_type>
	    (subrange_type,
	     group->priv_->get_types().subrange_types());

	  maybe_update_types_lookup_map<array_type_def::subrange_type>
	    (subrange_type,
	     group->get_type_per_loc_map().subrange_types(),
	     /*use_type_name_as_key*/false);
	}
    }
}

/// Update the map that associates the fully qualified name of a
/// function type with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param scope the scope of the function type.
/// @param fn_type the type to consider.
void
maybe_update_types_lookup_map(const function_type_sptr& fn_type)
{
  if (translation_unit *tu = fn_type->get_translation_unit())
    maybe_update_types_lookup_map<function_type>
      (fn_type, tu->get_types().function_types());

  if (corpus *type_corpus = fn_type->get_corpus())
    {
      maybe_update_types_lookup_map<function_type>
	(fn_type,
	 type_corpus->priv_->get_types().function_types());

      if (corpus *group = fn_type->get_corpus())
	{
	  maybe_update_types_lookup_map<function_type>
	    (fn_type,
	     group->priv_->get_types().function_types());
	}
    }
}

/// Update the map that associates the fully qualified name of a type
/// declaration with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param decl the declaration of the type to consider.
void
maybe_update_types_lookup_map(const decl_base_sptr& decl)
{
  if (!is_type(decl))
    return;

  if (type_decl_sptr basic_type = is_type_decl(decl))
    maybe_update_types_lookup_map(basic_type);
  else if (class_decl_sptr class_type = is_class_type(decl))
    maybe_update_types_lookup_map(class_type);
  else if (union_decl_sptr union_type = is_union_type(decl))
    maybe_update_types_lookup_map(union_type);
  else if (enum_type_decl_sptr enum_type = is_enum_type(decl))
    maybe_update_types_lookup_map(enum_type);
  else if (typedef_decl_sptr typedef_type = is_typedef(decl))
    maybe_update_types_lookup_map(typedef_type);
  else if (qualified_type_def_sptr qualified_type = is_qualified_type(decl))
    maybe_update_types_lookup_map(qualified_type);
  else if (pointer_type_def_sptr pointer_type = is_pointer_type(decl))
    maybe_update_types_lookup_map(pointer_type);
  else if (reference_type_def_sptr reference_type = is_reference_type(decl))
    maybe_update_types_lookup_map(reference_type);
  else if (array_type_def_sptr array_type = is_array_type(decl))
    maybe_update_types_lookup_map(array_type);
  else if (array_type_def::subrange_sptr subrange_type = is_subrange_type(decl))
    maybe_update_types_lookup_map(subrange_type);
  else
    ABG_ASSERT_NOT_REACHED;
}

/// Update the map that associates the fully qualified name of a type
/// with the type itself.
///
/// The per-translation unit type map is updated if no type with this
/// name was already existing in that map.
///
/// If no type with this name did already exist in the per-corpus type
/// map, then that per-corpus type map is updated. Otherwise, that
/// type is erased from that per-corpus map.
///
/// @param type the type to consider.
void
maybe_update_types_lookup_map(const type_base_sptr& type)
{
  if (decl_base_sptr decl = get_type_declaration(type))
    maybe_update_types_lookup_map(decl);
  else
    ABG_ASSERT_NOT_REACHED;
}

//--------------------------------
// </type and decls lookup stuff>
// ------------------------------

/// In a translation unit, lookup a given type or synthesize it if
/// it's a qualified type.
///
/// So this function first looks the type up in the translation unit.
/// If it's found, then OK, it's returned.  Otherwise, if it's a
/// qualified, reference or pointer or function type (a composite
/// type), lookup the underlying type, synthesize the type we want
/// from it and return it.
///
/// If the underlying types is not not found, then give up and return
/// nil.
///
/// @return the type that was found or the synthesized type.
type_base_sptr
synthesize_type_from_translation_unit(const type_base_sptr& type,
				      translation_unit& tu)
{
  type_base_sptr result;

   result = lookup_type(type, tu);

  if (!result)
    {
      if (qualified_type_def_sptr qual = is_qualified_type(type))
	{
	  type_base_sptr underlying_type =
	    synthesize_type_from_translation_unit(qual->get_underlying_type(),
						  tu);
	  if (underlying_type)
	    {
	      result.reset(new qualified_type_def(underlying_type,
						  qual->get_cv_quals(),
						  qual->get_location()));
	    }
	}
      else if (pointer_type_def_sptr p = is_pointer_type(type))
	{
	  type_base_sptr pointed_to_type =
	    synthesize_type_from_translation_unit(p->get_pointed_to_type(),
						  tu);
	  if (pointed_to_type)
	    {
	      result.reset(new pointer_type_def(pointed_to_type,
						p->get_size_in_bits(),
						p->get_alignment_in_bits(),
						p->get_location()));
	      result->set_environment(pointed_to_type->get_environment());
	    }
	}
      else if (reference_type_def_sptr r = is_reference_type(type))
	{
	  type_base_sptr pointed_to_type =
	    synthesize_type_from_translation_unit(r->get_pointed_to_type(), tu);
	  if (pointed_to_type)
	    {
	      result.reset(new reference_type_def(pointed_to_type,
						  r->is_lvalue(),
						  r->get_size_in_bits(),
						  r->get_alignment_in_bits(),
						  r->get_location()));
	      result->set_environment(pointed_to_type->get_environment());
	    }
	}
      else if (function_type_sptr f = is_function_type(type))
	result = synthesize_function_type_from_translation_unit(*f, tu);

      if (result)
	{
	  add_decl_to_scope(is_decl(result), tu.get_global_scope());
	  canonicalize(result);
	}
    }

  if (result)
    tu.priv_->synthesized_types_.push_back(result);

  return result;
}

/// In a translation unit, lookup the sub-types that make up a given
/// function type and if the sub-types are all found, synthesize and
/// return a function_type with them.
///
/// This function is like lookup_function_type_in_translation_unit()
/// execept that it constructs the function type from the sub-types
/// found in the translation, rather than just looking for the
/// function types held by the translation unit.  This can be useful
/// if the translation unit doesnt hold the function type we are
/// looking for (i.e, lookup_function_type_in_translation_unit()
/// returned NULL) but we still want to see if the sub-types of the
/// function types are present in the translation unit.
///
/// @param fn_type the function type to consider.
///
/// @param tu the translation unit to look into.
///
/// @return the resulting synthesized function type if all its
/// sub-types have been found, NULL otherwise.
function_type_sptr
synthesize_function_type_from_translation_unit(const function_type& fn_type,
					       translation_unit& tu)
{
  function_type_sptr nil = function_type_sptr();

  environment* env = tu.get_environment();
  ABG_ASSERT(env);

  type_base_sptr return_type = fn_type.get_return_type();
  type_base_sptr result_return_type;
  if (!return_type || env->is_void_type(return_type))
    result_return_type = env->get_void_type();
  else
    result_return_type = synthesize_type_from_translation_unit(return_type, tu);
  if (!result_return_type)
    return nil;

  function_type::parameters parms;
  type_base_sptr parm_type;
  function_decl::parameter_sptr parm;
  for (function_type::parameters::const_iterator i =
	 fn_type.get_parameters().begin();
       i != fn_type.get_parameters().end();
       ++i)
    {
      type_base_sptr t = (*i)->get_type();
      parm_type = synthesize_type_from_translation_unit(t, tu);
      if (!parm_type)
	return nil;
      parm.reset(new function_decl::parameter(parm_type,
					      (*i)->get_index(),
					      (*i)->get_name(),
					      (*i)->get_location(),
					      (*i)->get_variadic_marker(),
					      (*i)->get_is_artificial()));
      parms.push_back(parm);
    }

  class_or_union_sptr class_type;
  const method_type* method = is_method_type(&fn_type);
  if (method)
    {
      class_type = is_class_or_union_type
	(synthesize_type_from_translation_unit(method->get_class_type(), tu));
      ABG_ASSERT(class_type);
    }

  function_type_sptr result_fn_type;

  if (class_type)
    result_fn_type.reset(new method_type(result_return_type,
					 class_type,
					 parms,
					 method->get_is_const(),
					 fn_type.get_size_in_bits(),
					 fn_type.get_alignment_in_bits()));
  else
    result_fn_type.reset(new function_type(result_return_type,
					   parms,
					   fn_type.get_size_in_bits(),
					   fn_type.get_alignment_in_bits()));

  tu.priv_->synthesized_types_.push_back(result_fn_type);
  // The new synthesized type must be in the same environment as its
  // translation unit.
  result_fn_type->set_environment(tu.get_environment());
  tu.bind_function_type_life_time(result_fn_type);

  canonicalize(result_fn_type);
  return result_fn_type;
}

/// Demangle a C++ mangled name and return the resulting string
///
/// @param mangled_name the C++ mangled name to demangle.
///
/// @return the resulting mangled name.
string
demangle_cplus_mangled_name(const string& mangled_name)
{
  if (mangled_name.empty())
    return "";

  size_t l = 0;
  int status = 0;
  char * str = abi::__cxa_demangle(mangled_name.c_str(),
				   NULL, &l, &status);
  string demangled_name = mangled_name;
  if (str)
    {
      ABG_ASSERT(status == 0);
      demangled_name = str;
      free(str);
      str = 0;
    }
  return demangled_name;
}

/// Return either the type given in parameter if it's non-null, or the
/// void type.
///
/// @param t the type to consider.
///
/// @param env the environment to use.  If NULL, just abort the
/// process.
///
/// @return either @p t if it is non-null, or the void type.
type_base_sptr
type_or_void(const type_base_sptr t, const environment* env)
{
  type_base_sptr r;

  if (t)
    r = t;
  else
    {
      ABG_ASSERT(env);
      r = type_base_sptr(env->get_void_type());
    }

  return r;
}

global_scope::~global_scope()
{
}

// <type_base definitions>

/// Definition of the private data of @ref type_base.
struct type_base::priv
{
  size_t		size_in_bits;
  size_t		alignment_in_bits;
  type_base_wptr	canonical_type;
  // The data member below holds the canonical type that is managed by
  // the smart pointer referenced by the canonical_type data member
  // above.  We are storing this underlying (naked) pointer here, so
  // that users can access it *fast*.  Otherwise, accessing
  // canonical_type above implies creating a shared_ptr, and that has
  // been measured to be slow for some performance hot spots.
  type_base*		naked_canonical_type;
  // Computing the representation of a type again and again can be
  // costly.  So we cache the internal and non-internal type
  // representation strings here.
  interned_string	internal_cached_repr_;
  interned_string	cached_repr_;

  priv()
    : size_in_bits(),
      alignment_in_bits(),
      naked_canonical_type()
  {}

  priv(size_t s,
       size_t a,
       type_base_sptr c = type_base_sptr())
    : size_in_bits(s),
      alignment_in_bits(a),
      canonical_type(c),
      naked_canonical_type(c.get())
  {}
}; // end struct type_base::priv

static void
maybe_propagate_canonical_type(const type_base& lhs_type,
			       const type_base& rhs_type);

/// Test if two types are eligible to the "Linux Kernel Fast Type
/// Comparison Optimization", a.k.a LKFTCO.
///
/// Two types T1 and T2 (who are presumably of the same name and kind)
/// are eligible to the LKFTCO if they fulfill the following criteria/
///
/// 1/ T1 and T2 come from the same Linux Kernel Corpus and they are
/// either class, union or enums.
///
/// 2/ They are defined in the same translation unit.
///
/// @param t1 the first type to consider.
///
/// @param t2 the second type to consider.
///
/// @return true iff t1 and t2 are eligible to the LKFTCO.
static bool
types_defined_same_linux_kernel_corpus_public(const type_base& t1,
					      const type_base& t2)
{
  const corpus *t1_corpus = t1.get_corpus(), *t2_corpus = t2.get_corpus();
  string t1_file_path, t2_file_path;

  /// If the t1 (and t2) are classes/unions/enums from the same linux
  /// kernel corpus, let's move on.  Otherwise bail out.
  if (!(t1_corpus && t2_corpus
	&& t1_corpus == t2_corpus
	&& (t1_corpus->get_origin() == corpus::LINUX_KERNEL_BINARY_ORIGIN)
	&& (is_class_or_union_type(&t1)
	    || is_enum_type(&t1))))
    return false;

  class_or_union *c1 = 0, *c2 = 0;
  c1 = is_class_or_union_type(&t1);
  c2 = is_class_or_union_type(&t2);

  // Two anonymous class types with no naming typedefs cannot be
  // eligible to this optimization.
  if ((c1 && c1->get_is_anonymous() && !c1->get_naming_typedef())
      || (c2 && c2->get_is_anonymous() && !c2->get_naming_typedef()))
    return false;

  // Two anonymous enum types cannot be eligible to this optimization.
  if (const enum_type_decl *e1 = is_enum_type(&t1))
    if (const enum_type_decl *e2 = is_enum_type(&t2))
      if (e1->get_is_anonymous() || e2->get_is_anonymous())
	return false;

  // Look through declaration-only types.  That is, get the associated
  // definition type.
  c1 = look_through_decl_only_class(c1);
  c2 = look_through_decl_only_class(c2);

  if (c1 && c2)
    {
      if (c1->get_is_declaration_only() != c2->get_is_declaration_only())
	{
	  if (c1->get_environment()->decl_only_class_equals_definition())
	    // At least one of classes/union is declaration-only.
	    // Because we are in a context in which a declaration-only
	    // class/union is equal to all definitions of that
	    // class/union, we can assume that the two types are
	    // equal.
	    return true;
	}
    }

  if (t1.get_size_in_bits() != t2.get_size_in_bits())
    return false;

  // Look at the file names of the locations of t1 and t2.  If they
  // are equal, then t1 and t2 are defined in the same file.
  {
    location l;

    if (c1)
      l = c1->get_location();
    else
      l = dynamic_cast<const decl_base&>(t1).get_location();

    unsigned line = 0, col = 0;
    if (l)
      l.expand(t1_file_path, line, col);
    if (c2)
      l = c2->get_location();
    else
      l = dynamic_cast<const decl_base&>(t2).get_location();
    if (l)
      l.expand(t2_file_path, line, col);
  }

  if (t1_file_path.empty() || t2_file_path.empty())
    return false;

  if (t1_file_path == t2_file_path)
    return true;

  return false;
}

/// Compute the canonical type for a given instance of @ref type_base.
///
/// Consider two types T and T'.  The canonical type of T, denoted
/// C(T) is a type such as T == T' if and only if C(T) == C(T').  Said
/// otherwise, to compare two types, one just needs to compare their
/// canonical types using pointer equality.  That makes type
/// comparison faster than the structural comparison performed by the
/// abigail::ir::equals() overloads.
///
/// If there is not yet any canonical type for @p t, then @p t is its
/// own canonical type.  Otherwise, this function returns the
/// canonical type of @p t which is the canonical type that has the
/// same hash value as @p t and that structurally equals @p t.  Note
/// that after invoking this function, the life time of the returned
/// canonical time is then equals to the life time of the current
/// process.
///
/// @param t a smart pointer to instance of @ref type_base we want to
/// compute a canonical type for.
///
/// @return the canonical type for the current instance of @ref
/// type_base.
type_base_sptr
type_base::get_canonical_type_for(type_base_sptr t)
{
  if (!t)
    return t;

  environment* env = t->get_environment();
  ABG_ASSERT(env);

  bool decl_only_class_equals_definition =
    (odr_is_relevant(*t) || env->decl_only_class_equals_definition());

  class_or_union_sptr class_or_union = is_class_or_union_type(t);

  // Look through declaration-only classes when we are dealing with
  // C++ or languages where we assume the "One Definition Rule".  In
  // that context, we assume that a declaration-only non-anonymous
  // class equals all fully defined classes of the same name.
  //
  // Otherwise, all classes, including declaration-only classes are
  // canonicalized and only canonical comparison is going to be used
  // in the system.
  if (decl_only_class_equals_definition)
    if (class_or_union)
      {
	class_or_union = look_through_decl_only_class(class_or_union);
	if (class_or_union->get_is_declaration_only())
	  return type_base_sptr();
	else
	  t = class_or_union;
      }

  class_decl_sptr is_class = is_class_type(t);
  if (t->get_canonical_type())
    return t->get_canonical_type();

  // For classes and union, ensure that an anonymous class doesn't
  // have a linkage name.  If it does in the future, then me must be
  // mindful that the linkage name respects the type identity
  // constraints which states that "if two linkage names are different
  // then the two types are different".
  ABG_ASSERT(!class_or_union
	     || !class_or_union->get_is_anonymous()
	     || class_or_union->get_linkage_name().empty());

  // We want the pretty representation of the type, but for an
  // internal use, not for a user-facing purpose.
  //
  // If two classe types Foo are declared, one as a class and the
  // other as a struct, but are otherwise equivalent, we want their
  // pretty representation to be the same.  Hence the 'internal'
  // argument of ir::get_pretty_representation() is set to true here.
  // So in this case, the pretty representation of Foo is going to be
  // "class Foo", regardless of its struct-ness. This also applies to
  // composite types which would have "class Foo" as a sub-type.
  string repr = t->get_cached_pretty_representation(/*internal=*/true);

  // If 't' already has a canonical type 'inside' its corpus
  // (t_corpus), then this variable is going to contain that canonical
  // type.
  type_base_sptr canonical_type_present_in_corpus;
  environment::canonical_types_map_type& types =
    env->get_canonical_types_map();

  type_base_sptr result;
  environment::canonical_types_map_type::iterator i = types.find(repr);
  if (i == types.end())
    {
      vector<type_base_sptr> v;
      v.push_back(t);
      types[repr] = v;
      result = t;
    }
  else
    {
      vector<type_base_sptr> &v = i->second;
      // Let's compare 't' structurally (i.e, compare its sub-types
      // recursively) against the canonical types of the system. If it
      // equals a given canonical type C, then it means C is the
      // canonical type of 't'.  Otherwise, if 't' is different from
      // all the canonical types of the system, then it means 't' is a
      // canonical type itself.
      for (vector<type_base_sptr>::const_reverse_iterator it = v.rbegin();
	   it != v.rend();
	   ++it)
	{
	  // Before the "*it == it" comparison below is done, let's
	  // perform on-the-fly-canonicalization.  For C types, let's
	  // consider that an unresolved struct declaration 'struct S'
	  // is different from a definition 'struct S'.  This is
	  // because normally, at this point all the declarations of
	  // struct S that are compatible with the definition of
	  // struct S have already been resolved to that definition,
	  // during the DWARF parsing.  The remaining unresolved
	  // declaration are thus considered different.  With this
	  // setup we can properly handle cases of two *different*
	  // struct S being defined in the same binary (in different
	  // translation units), and a third struct S being only
	  // declared as an opaque type in a third translation unit of
	  // its own, with no definition in there.  In that case, the
	  // declaration-only struct S should be left alone and not
	  // resolved to any of the two definitions of struct S.
	  bool saved_decl_only_class_equals_definition =
	    env->decl_only_class_equals_definition();
	  env->do_on_the_fly_canonicalization(true);
	  // Compare types by considering that decl-only classes don't
	  // equal their definition.
	  env->decl_only_class_equals_definition(false);
	  bool equal = types_defined_same_linux_kernel_corpus_public(**it, *t)
		       || *it == t;
	  // Restore the state of the on-the-fly-canonicalization and
	  // the decl-only-class-being-equal-to-a-matching-definition
	  // flags.
	  env->do_on_the_fly_canonicalization(false);
	  env->decl_only_class_equals_definition
	    (saved_decl_only_class_equals_definition);
	  if (equal)
	    {
	      result = *it;
	      break;
	    }
	}
      if (!result)
	{
	  v.push_back(t);
	  result = t;
	}
    }

  return result;
}

/// This method is invoked automatically right after the current
/// instance of @ref class_decl has been canonicalized.
void
type_base::on_canonical_type_set()
{}

/// This is a subroutine of the canonicalize() function.
///
/// When the canonical type C of type T has just been computed, there
/// can be cases where T has member functions that C doesn't have.
///
/// This is possible because non virtual member functions are not
/// taken in account when comparing two types.
///
/// In that case, this function updates C so that it contains the
/// member functions.
///
/// There can also be cases where C has a method M which is not linked
/// to any underlying symbol, whereas in T, M is to link to an
/// underlying symbol.  In that case, this function updates M in C so
/// that it's linked to the same underlying symbol as for M in T.
static void
maybe_adjust_canonical_type(const type_base_sptr& canonical,
			    const type_base_sptr& type)
{
  if (// If 'type' is *NOT* a newly canonicalized type ...
      type->get_naked_canonical_type()
      // ... or if 'type' is it's own canonical type, then get out.
      || type.get() == canonical.get())
    return;

  if (class_decl_sptr cl = is_class_type(type))
    {
      class_decl_sptr canonical_class = is_class_type(canonical);

      if (canonical_class)
	{
	  // Set symbols of member functions that might be missing
	  // theirs.
	  for (class_decl::member_functions::const_iterator i =
		 cl->get_member_functions().begin();
	       i != cl->get_member_functions().end();
	       ++i)
	    if ((*i)->get_symbol())
	      {
		if (method_decl *m = canonical_class->
		    find_member_function((*i)->get_linkage_name()))
		  {
		    elf_symbol_sptr s1 = (*i)->get_symbol();
		    if (s1 && !m->get_symbol())
		      // Method 'm' in the canonical type is not
		      // linked to the underlying symbol of '*i'.
		      // Let's link it now.  have th
		      m->set_symbol(s1);
		  }
		else
		  // There is a member function defined and publicly
		  // exported in the other class, and the canonical
		  // class doesn't have that member function.  Let's
		  // copy that member function to the canonical class
		  // then.
		  copy_member_function (canonical_class, *i);
	      }
	}
    }
}

/// Compute the canonical type of a given type.
///
/// It means that after invoking this function, comparing the intance
/// instance @ref type_base and another one (on which
/// type_base::enable_canonical_equality() would have been invoked as
/// well) is performed by just comparing the pointer values of the
/// canonical types of both types.  That equality comparison is
/// supposedly faster than structural comparison of the types.
///
/// @param t a smart pointer to the instance of @ref type_base for
/// which to compute the canonical type.  After this call,
/// t->get_canonical_type() will return the newly computed canonical
/// type.
///
/// @return the canonical type computed for @p t.
type_base_sptr
canonicalize(type_base_sptr t)
{
  if (!t)
    return t;

  if (t->get_canonical_type())
    return t->get_canonical_type();

  type_base_sptr canonical = type_base::get_canonical_type_for(t);
  maybe_adjust_canonical_type(canonical, t);

  t->priv_->canonical_type = canonical;
  t->priv_->naked_canonical_type = canonical.get();

  if (class_decl_sptr cl = is_class_type(t))
    if (type_base_sptr d = is_type(cl->get_earlier_declaration()))
      if ((canonical = d->get_canonical_type()))
	{
	  d->priv_->canonical_type = canonical;
	  d->priv_->naked_canonical_type = canonical.get();
	}

  if (canonical)
    if (decl_base_sptr d = is_decl_slow(canonical))
      {
	scope_decl *scope = d->get_scope();
	// Add the canonical type to the set of canonical types
	// belonging to its scope.
	if (scope)
	  scope->get_canonical_types().insert(canonical);
	//else, if the type doesn't have a scope, it's doesn't meant
	// to be emitted.  This can be the case for the result of the
	// function strip_typedef, for instance.
      }

  t->on_canonical_type_set();
  return canonical;
}


/// Set the definition of this declaration-only @ref decl_base.
///
/// @param d the new definition to set.
void
decl_base::set_definition_of_declaration(const decl_base_sptr& d)
{
  ABG_ASSERT(get_is_declaration_only());
  priv_->definition_of_declaration_ = d;
  if (type_base *t = is_type(this))
    if (type_base_sptr canonical_type = is_type(d)->get_canonical_type())
      t->priv_->canonical_type = canonical_type;

  priv_->naked_definition_of_declaration_ = const_cast<decl_base*>(d.get());
}

/// The constructor of @ref type_base.
///
/// @param s the size of the type, in bits.
///
/// @param a the alignment of the type, in bits.
type_base::type_base(const environment* e, size_t s, size_t a)
  : type_or_decl_base(e, ABSTRACT_TYPE_BASE|ABSTRACT_TYPE_BASE),
    priv_(new priv(s, a))
{}

/// Getter of the canonical type of the current instance of @ref
/// type_base.
///
/// @return a smart pointer to the canonical type of the current
/// intance of @ref type_base, or an empty smart pointer if the
/// current instance of @ref type_base doesn't have any canonical
/// type.
type_base_sptr
type_base::get_canonical_type() const
{return priv_->canonical_type.lock();}

/// Getter of the canonical type pointer.
///
/// Note that this function doesn't return a smart pointer, but rather
/// the underlying pointer managed by the smart pointer.  So it's as
/// fast as possible.  This getter is to be used in code paths that
/// are proven to be performance hot spots; especially, when comparing
/// sensitive types like class, function, pointers and reference
/// types.  Those are compared extremely frequently and thus, their
/// accessing the canonical type must be fast.
///
/// @return the canonical type pointer, not managed by a smart
/// pointer.
type_base*
type_base::get_naked_canonical_type() const
{return priv_->naked_canonical_type;}

/// Get the pretty representation of the current type.
///
/// The pretty representation is retrieved from a cache.  If the cache
/// is empty, this function computes the pretty representation, put it
/// in the cache and returns it.
///
/// Note that if the type is *NOT* canonicalized, the pretty
/// representation is never cached.
///
/// @param internal if true, then the pretty representation is to be
/// used for purpuses that are internal to the libabigail library
/// itself.  If you don't know what this means, then you probably
/// should set this parameter to "false".
const interned_string&
type_base::get_cached_pretty_representation(bool internal) const
{
  if (internal)
    {
      if (!get_naked_canonical_type() || priv_->internal_cached_repr_.empty())
	{
	  string r = ir::get_pretty_representation(this, internal);
	  priv_->internal_cached_repr_ = get_environment()->intern(r);
	}
      return priv_->internal_cached_repr_;
    }

  if (!get_naked_canonical_type() || priv_->cached_repr_.empty())
    {
      string r = ir::get_pretty_representation(this, internal);
      priv_->cached_repr_ = get_environment()->intern(r);
    }

  return priv_->cached_repr_;
}

/// Compares two instances of @ref type_base.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p is non-null and if the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const type_base& l, const type_base& r, change_kind* k)
{
  bool result = (l.get_size_in_bits() == r.get_size_in_bits()
		 && l.get_alignment_in_bits() == r.get_alignment_in_bits());
  if (!result)
    if (k)
      *k |= LOCAL_TYPE_CHANGE_KIND;
  return result;
}

/// Return true iff both type declarations are equal.
///
/// Note that this doesn't test if the scopes of both types are equal.
bool
type_base::operator==(const type_base& other) const
{return equals(*this, other, 0);}

/// Inequality operator.
///
///@param other the instance of @ref type_base to compare the current
/// instance against.
///
/// @return true iff the current instance is different from @p other.
bool
type_base::operator!=(const type_base& other) const
{return !operator==(other);}

/// Setter for the size of the type.
///
/// @param s the new size -- in bits.
void
type_base::set_size_in_bits(size_t s)
{priv_->size_in_bits = s;}

/// Getter for the size of the type.
///
/// @return the size in bits of the type.
size_t
type_base::get_size_in_bits() const
{return priv_->size_in_bits;}

/// Setter for the alignment of the type.
///
/// @param a the new alignment -- in bits.
void
type_base::set_alignment_in_bits(size_t a)
{priv_->alignment_in_bits = a;}

/// Getter for the alignment of the type.
///
/// @return the alignment of the type in bits.
size_t
type_base::get_alignment_in_bits() const
{return priv_->alignment_in_bits;}

/// Default implementation of traversal for types.  This function does
/// nothing.  It must be implemented by every single new type that is
/// written.
///
/// Please look at e.g, class_decl::traverse() for an example of how
/// to implement this.
///
/// @param v the visitor used to visit the type.
bool
type_base::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  v.visit_begin(this);
  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);

  return result;
}

type_base::~type_base()
{delete priv_;}

// </type_base definitions>

// <integral_type definitions>

/// Bitwise OR operator for integral_type::modifiers_type.
///
/// @param l the left-hand side operand.
///
/// @param r the right-hand side operand.
///
/// @return the result of the bitwise OR.
integral_type::modifiers_type
operator|(integral_type::modifiers_type l, integral_type::modifiers_type r)
{
  return static_cast<integral_type::modifiers_type>(static_cast<unsigned>(l)
						    |static_cast<unsigned>(r));
}

/// Bitwise AND operator for integral_type::modifiers_type.
///
/// @param l the left-hand side operand.
///
/// @param r the right-hand side operand.
///
/// @return the result of the bitwise AND.
integral_type::modifiers_type
operator&(integral_type::modifiers_type l, integral_type::modifiers_type r)
{
  return static_cast<integral_type::modifiers_type>(static_cast<unsigned>(l)
						    &static_cast<unsigned>(r));
}

/// Bitwise |= operator for integral_type::modifiers_type.
///
/// @param l the left-hand side operand.
///
/// @param r the right-hand side operand.
///
/// @return the result of the bitwise |=.
integral_type::modifiers_type&
operator|=(integral_type::modifiers_type& l, integral_type::modifiers_type r)
{
  l = l | r;
  return l;
}

/// Parse a word containing one integral type modifier.
///
/// A word is considered to be a string of characters that doesn't
/// contain any white space.
///
/// @param word the word to parse.  It is considered to be a string of
/// characters that doesn't contain any white space.
///
/// @param modifiers out parameter.  It's set by this function to the
/// parsed modifier iff the function returned true.
///
/// @return true iff @word was successfully parsed.
static bool
parse_integral_type_modifier(const string& word,
			     integral_type::modifiers_type &modifiers)
{
    if (word == "signed")
      modifiers |= integral_type::SIGNED_MODIFIER;
    else if (word == "unsigned")
      modifiers |= integral_type::UNSIGNED_MODIFIER;
    else if (word == "short")
      modifiers |= integral_type::SHORT_MODIFIER;
    else if (word == "long")
      modifiers |= integral_type::LONG_MODIFIER;
    else if (word == "long long")
      modifiers |= integral_type::LONG_LONG_MODIFIER;
    else
      return false;

    return true;
}

/// Parse a base type of an integral type from a string.
///
/// @param type_name the type name to parse.
///
/// @param base out parameter.  This is set to the resulting base type
/// parsed, iff the function returned true.
///
/// @return true iff the function could successfully parse the base
/// type.
static bool
parse_base_integral_type(const string& type_name,
			 integral_type::base_type& base)
{
  if (type_name == "int")
    base = integral_type::INT_BASE_TYPE;
  else if (type_name == "char")
    base = integral_type::CHAR_BASE_TYPE;
  else if (type_name == "bool" || type_name == "_Bool")
    base = integral_type::BOOL_BASE_TYPE;
  else if (type_name == "double")
    base = integral_type::DOUBLE_BASE_TYPE;
  else if (type_name =="float")
    base = integral_type::FLOAT_BASE_TYPE;
  else if (type_name == "char16_t")
    base = integral_type::CHAR16_T_BASE_TYPE;
  else if (type_name == "char32_t")
    base = integral_type::CHAR32_T_BASE_TYPE;
  else if (type_name == "wchar_t")
    base = integral_type::WCHAR_T_BASE_TYPE;
  else
    return false;

  return true;
}

/// Parse an integral type from a string.
///
/// @param type_name the string containing the integral type to parse.
///
/// @param base out parameter.  Is set by this function to the base
/// type of the integral type, iff the function returned true.
///
/// @param modifiers out parameter  If set by this function to the
/// modifier of the integral type, iff the function returned true.
///
/// @return true iff the function could parse an integral type from @p
/// type_name.
static bool
parse_integral_type(const string&			type_name,
		    integral_type::base_type&		base,
		    integral_type::modifiers_type&	modifiers)
{
  string input = type_name;
  string::size_type len = input.length();
  string::size_type cur_pos = 0, prev_pos = 0;
  string cur_word, prev_word;
  bool ok = false;

  while (cur_pos < len)
    {
      prev_pos = cur_pos;
      cur_pos = input.find(' ', prev_pos);
      prev_word = cur_word;
      cur_word = input.substr(prev_pos, cur_pos - prev_pos);

      if (cur_pos < len && isspace(input[cur_pos]))
	do
	  ++cur_pos;
	while (cur_pos < len && isspace(input[cur_pos]));

      if (cur_pos < len
	  && cur_word == "long"
	  && prev_word != "long")
	{
	  prev_pos = cur_pos;
	  cur_pos = input.find(' ', prev_pos);
	  string saved_prev_word = prev_word;
	  prev_word = cur_word;
	  cur_word = input.substr(prev_pos, cur_pos - prev_pos);
	  if (cur_word == "long")
	    cur_word = "long long";
	  else
	    {
	      cur_pos = prev_pos;
	      cur_word = prev_word;
	      prev_word = saved_prev_word;
	    }
	}

      if (!parse_integral_type_modifier(cur_word, modifiers))
	{
	  if (!parse_base_integral_type(cur_word, base))
	    return false;
	  else
	    ok = true;
	}
      else
	ok = true;
    }

  return ok;
}

/// Parse an integral type from a string.
///
/// @param str the string containing the integral type to parse.
///
///@param type the resulting @ref integral_type.  Is set to the result
///of the parse, iff the function returns true.
///
/// @return true iff the function could parse an integral type from @p
/// str.
bool
parse_integral_type(const string& str, integral_type& type)
{
  integral_type::base_type base_type = integral_type::INT_BASE_TYPE;
  integral_type::modifiers_type modifiers = integral_type::NO_MODIFIER;

  if (!parse_integral_type(str, base_type, modifiers))
    return false;

  // So this is an integral type.
  integral_type int_type(base_type, modifiers);
  type = int_type;
  return true;
}

/// Default constructor of the @ref integral_type.
integral_type::integral_type()
  : base_(INT_BASE_TYPE),
    modifiers_(NO_MODIFIER)
{}

/// Constructor of the @ref integral_type.
///
/// @param b the base type of the integral type.
///
/// @param m the modifiers of the integral type.
integral_type::integral_type(base_type b, modifiers_type m)
  : base_(b), modifiers_(m)
{}

/// Constructor of the @ref integral_type.
///
/// @param the name of the integral type to parse to initialize the
/// current instance of @ref integral_type.
integral_type::integral_type(const string& type_name)
  : base_(INT_BASE_TYPE),
    modifiers_(NO_MODIFIER)
{
  bool could_parse = parse_integral_type(type_name, base_, modifiers_);
  ABG_ASSERT(could_parse);
}

/// Getter of the base type of the @ref integral_type.
///
/// @return the base type of the @ref integral_type.
integral_type::base_type
integral_type::get_base_type() const
{return base_;}

/// Getter of the modifiers bitmap of the @ref integral_type.
///
/// @return the modifiers bitmap of the @ref integral_type.
integral_type::modifiers_type
integral_type::get_modifiers() const
{return modifiers_;}

/// Equality operator for the @ref integral_type.
///
/// @param other the other integral type to compare against.
///
/// @return true iff @p other equals the current instance of @ref
/// integral_type.
bool
integral_type::operator==(const integral_type&other) const
{return base_ == other.base_ && modifiers_ == other.modifiers_;}

/// Return the string representation of the current instance of @ref
/// integral_type.
///
/// @return the string representation of the current instance of @ref
/// integral_type.
string
integral_type::to_string() const
{
  string result;

  // Look at modifiers ...
  if (modifiers_ & SIGNED_MODIFIER)
    result += "signed ";
  if (modifiers_ & UNSIGNED_MODIFIER)
    result += "unsigned ";
  if (modifiers_ & SHORT_MODIFIER)
    result += "short ";
  if (modifiers_ & LONG_MODIFIER)
    result += "long ";
  if (modifiers_ & LONG_LONG_MODIFIER)
    result += "long long ";

  // ... and look at base types.
  if (base_ == INT_BASE_TYPE)
    result += "int";
  else if (base_ == CHAR_BASE_TYPE)
    result += "char";
  else if (base_ == BOOL_BASE_TYPE)
    result += "bool";
  else if (base_ == DOUBLE_BASE_TYPE)
    result += "double";
  else if (base_ == FLOAT_BASE_TYPE)
    result += "float";
  else if (base_ == CHAR16_T_BASE_TYPE)
    result += "char16_t";
    else if (base_ == CHAR32_T_BASE_TYPE)
    result += "char32_t";
    else if (base_ == WCHAR_T_BASE_TYPE)
    result += "wchar_t";

  return result;
}

/// Convert the current instance of @ref integral_type into its string
/// representation.
///
/// @return the string representation of the current instance of @ref
/// integral_type.
integral_type::operator string() const
{return to_string();}

// </integral_type definitions>

//<type_decl definitions>

/// Constructor.
///
/// @param env the environment we are operating from.
///
/// @param name the name of the type declaration.
///
/// @param size_in_bits the size of the current type_decl, in bits.
///
/// @param alignment_in_bits the alignment of the current typ, in
/// bits.
///
/// @param locus the source location of the current type declaration.
///
/// @param linkage_name the linkage_name of the current type declaration.
///
/// @param vis the visibility of the type declaration.
type_decl::type_decl(const environment* env,
		     const string&	name,
		     size_t		size_in_bits,
		     size_t		alignment_in_bits,
		     const location&	locus,
		     const string&	linkage_name,
		     visibility	vis)

  : type_or_decl_base(env,
		      BASIC_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    decl_base(env, name, locus, linkage_name, vis),
    type_base(env, size_in_bits, alignment_in_bits)
{
  runtime_type_instance(this);

  integral_type::base_type base_type = integral_type::INT_BASE_TYPE;
  integral_type::modifiers_type modifiers = integral_type::NO_MODIFIER;
  integral_type int_type(base_type, modifiers);
  if (parse_integral_type(name, int_type))
    {
      // Convert the integral_type into its canonical string
      // representation.
      string integral_type_name = int_type;

      // Set the name of this type_decl to the canonical string
      // representation above
      set_name(integral_type_name);
      set_qualified_name(get_name());

      if (!get_linkage_name().empty())
	set_linkage_name(integral_type_name);
    }
}

/// Compares two instances of @ref type_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const type_decl& l, const type_decl& r, change_kind* k)
{
  bool result = equals(static_cast<const decl_base&>(l),
		       static_cast<const decl_base&>(r),
		       k);
  if (!k && !result)
    return false;

  result &= equals(static_cast<const type_base&>(l),
		   static_cast<const type_base&>(r),
		   k);
  return result;
}

/// Return true if both types equals.
///
/// This operator re-uses the overload that takes a decl_base.
///
/// Note that this does not check the scopes of any of the types.
///
/// @param o the other type_decl to check agains.
bool
type_decl::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Return true if both types equals.
///
/// Note that this does not check the scopes of any of the types.
///
/// @param o the other type_decl to check against.
bool
type_decl::operator==(const decl_base& o) const
{
  const type_decl* other = dynamic_cast<const type_decl*>(&o);
  if (!other)
    return false;
  return try_canonical_compare(this, other);
}

/// Return true if both types equals.
///
/// Note that this does not check the scopes of any of the types.
///
/// @param o the other type_decl to check against.
///
/// @return true iff the current isntance equals @p o
bool
type_decl::operator==(const type_decl& o) const
{
  const decl_base& other = o;
  return *this == other;
}

/// Inequality operator.
///
/// @param o the other type to compare against.
///
/// @return true iff the current instance is different from @p o.
bool
type_decl::operator!=(const type_decl& o) const
{return !operator==(o);}

/// Equality operator for @ref type_decl_sptr.
///
/// @param l the first operand to compare.
///
/// @param r the second operand to compare.
///
/// @return true iff @p l equals @p r.
bool
operator==(const type_decl_sptr& l, const type_decl_sptr& r)
{
  if (!!l != !!r)
    return false;
  if (l.get() == r.get())
    return true;
  return *l == *r;
}

/// Inequality operator for @ref type_decl_sptr.
///
/// @param l the first operand to compare.
///
/// @param r the second operand to compare.
///
/// @return true iff @p l is different from @p r.
bool
operator!=(const type_decl_sptr& l, const type_decl_sptr& r)
{return !operator==(l, r);}

/// Get the pretty representation of the current instance of @ref
/// type_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qualified_name if true, names emitted in the pretty
/// representation are fully qualified.
///
/// @return the pretty representatin of the @ref type_decl.
string
type_decl::get_pretty_representation(bool internal,
				     bool qualified_name) const
{
  if (qualified_name)
    return get_qualified_name(internal);
  return get_name();
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
type_decl::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  v.visit_begin(this);
  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);

  return result;
}

type_decl::~type_decl()
{}
//</type_decl definitions>

// <scope_type_decl definitions>

/// Constructor.
///
/// @param env the environment we are operating from.
///
/// @param name the name of the type.
///
/// @param size_in_bits the size of the type, in bits.
///
/// @param alignment_in_bits the alignment of the type, in bits.
///
/// @param locus the source location where the type is defined.
///
/// @param vis the visibility of the type.
scope_type_decl::scope_type_decl(const environment*	env,
				 const string&		name,
				 size_t		size_in_bits,
				 size_t		alignment_in_bits,
				 const location&	locus,
				 visibility		vis)
  : type_or_decl_base(env,
		      ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    decl_base(env, name, locus, "", vis),
    type_base(env, size_in_bits, alignment_in_bits),
    scope_decl(env, name, locus)
{}

/// Compares two instances of @ref scope_type_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const scope_type_decl& l, const scope_type_decl& r, change_kind* k)
{
  bool result = equals(static_cast<const scope_decl&>(l),
		  static_cast<const scope_decl&>(r),
		  k);

  if (!k && !result)
    return false;

  result &= equals(static_cast<const type_base&>(l),
		   static_cast<const type_base&>(r),
		   k);

  return result;
}

/// Equality operator between two scope_type_decl.
///
/// Note that this function does not consider the scope of the scope
/// types themselves.
///
/// @return true iff both scope types are equal.
bool
scope_type_decl::operator==(const decl_base& o) const
{
  const scope_type_decl* other = dynamic_cast<const scope_type_decl*>(&o);
  if (!other)
    return false;
  return try_canonical_compare(this, other);
}

/// Equality operator between two scope_type_decl.
///
/// This re-uses the equality operator that takes a decl_base.
///
/// @param o the other scope_type_decl to compare against.
///
/// @return true iff both scope types are equal.
bool
scope_type_decl::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;

  return *this == *other;
}

/// Traverses an instance of @ref scope_type_decl, visiting all the
/// sub-types and decls that it might contain.
///
/// @param v the visitor that is used to visit every IR sub-node of
/// the current node.
///
/// @return true if either
///  - all the children nodes of the current IR node were traversed
///    and the calling code should keep going with the traversing.
///  - or the current IR node is already being traversed.
/// Otherwise, returning false means that the calling code should not
/// keep traversing the tree.
bool
scope_type_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.type_node_has_been_visited(this))
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      for (scope_decl::declarations::const_iterator i =
	     get_member_decls().begin();
	   i != get_member_decls ().end();
	   ++i)
	if (!(*i)->traverse(v))
	  break;
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);

  return result;
}

scope_type_decl::~scope_type_decl()
{}
// </scope_type_decl definitions>

// <namespace_decl>

/// Constructor.
///
/// @param the environment we are operatin from.
///
/// @param name the name of the namespace.
///
/// @param locus the source location where the namespace is defined.
///
/// @param vis the visibility of the namespace.
namespace_decl::namespace_decl(const environment*	env,
			       const string&		name,
			       const location&		locus,
			       visibility		vis)
    // We need to call the constructor of decl_base directly here
    // because it is virtually inherited by scope_decl.  Note that we
    // just implicitely call the default constructor for scope_decl
    // here, as what we really want is to initialize the decl_base
    // subobject.  Wow, virtual inheritance is		useful, but setting it
    // up is ugly.
  : type_or_decl_base(env,
		      NAMESPACE_DECL
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, locus, "", vis),
    scope_decl(env, name, locus)
{
  runtime_type_instance(this);
}

/// Build and return a copy of the pretty representation of the
/// namespace.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qualified_name if true, names emitted in the pretty
/// representation are fully qualified.
///
/// @return a copy of the pretty representation of the namespace.
string
namespace_decl::get_pretty_representation(bool internal,
					  bool qualified_name) const
{
  string r =
    "namespace " + scope_decl::get_pretty_representation(internal,
							 qualified_name);
  return r;
}

/// Return true iff both namespaces and their members are equal.
///
/// Note that this function does not check if the scope of these
/// namespaces are equal.
bool
namespace_decl::operator==(const decl_base& o) const
{
  const namespace_decl* other = dynamic_cast<const namespace_decl*>(&o);
  if (!other)
    return false;
  return scope_decl::operator==(*other);
}

/// Test if the current namespace_decl is empty or contains empty
/// namespaces itself.
///
/// @return true iff the current namespace_decl is empty or contains
/// empty itself.
bool
namespace_decl::is_empty_or_has_empty_sub_namespaces() const
{
  if (is_empty())
    return true;

  for (declarations::const_iterator i = get_member_decls().begin();
       i != get_member_decls().end();
       ++i)
    {
      if (!is_namespace(*i))
	return false;

      namespace_decl_sptr ns = is_namespace(*i);
      ABG_ASSERT(ns);

      if (!ns->is_empty_or_has_empty_sub_namespaces())
	return false;
    }

  return true;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// member nodes.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
namespace_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      scope_decl::declarations::const_iterator i;
      for (i = get_member_decls().begin();
	   i != get_member_decls ().end();
	   ++i)
	{
	  ir_traversable_base_sptr t =
	    dynamic_pointer_cast<ir_traversable_base>(*i);
	  if (t)
	    if (!t->traverse (v))
	      break;
	}
      visiting(false);
    }
  return v.visit_end(this);
}

namespace_decl::~namespace_decl()
{
}

// </namespace_decl>

// <qualified_type_def>

/// Type of the private data of qualified_type_def.
class qualified_type_def::priv
{
  friend class qualified_type_def;

  qualified_type_def::CV	cv_quals_;
  // Before the type is canonicalized, this is used as a temporary
  // internal name.
  interned_string		temporary_internal_name_;
  // Once the type is canonicalized, this is used as the internal
  // name.
  interned_string		internal_name_;
  weak_ptr<type_base>		underlying_type_;

  priv()
    : cv_quals_(CV_NONE)
  {}

  priv(qualified_type_def::CV quals,
       type_base_sptr t)
    : cv_quals_(quals),
      underlying_type_(t)
  {}
};// end class qualified_type_def::priv

/// Build the name of the current instance of qualified type.
///
/// @param fully_qualified if true, build a fully qualified name.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the newly-built name.
string
qualified_type_def::build_name(bool fully_qualified, bool internal) const
{
  ABG_ASSERT(get_underlying_type());

  return get_name_of_qualified_type(get_underlying_type(),
				    get_cv_quals(),
				    fully_qualified, internal);
}

/// This function is automatically invoked whenever an instance of
/// this type is canonicalized.
///
/// It's an overload of the virtual type_base::on_canonical_type_set.
///
/// We put here what is thus meant to be executed only at the point of
/// type canonicalization.
void
qualified_type_def::on_canonical_type_set()
{clear_qualified_name();}

/// Constructor of the qualified_type_def
///
/// @param type the underlying type
///
/// @param quals a bitfield representing the const/volatile qualifiers
///
/// @param locus the location of the qualified type definition
qualified_type_def::qualified_type_def(type_base_sptr		type,
				       CV			quals,
				       const location&		locus)
  : type_or_decl_base(type->get_environment(),
		      QUALIFIED_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    type_base(type->get_environment(), type->get_size_in_bits(),
	      type->get_alignment_in_bits()),
    decl_base(type->get_environment(), "", locus, "",
	      dynamic_pointer_cast<decl_base>(type)->get_visibility()),
    priv_(new priv(quals, type))
{
  runtime_type_instance(this);
  interned_string name = type->get_environment()->intern(build_name(false));
  set_name(name);
}

/// Get the size of the qualified type def.
///
/// This is an overload for type_base::get_size_in_bits().
///
/// @return the size of the qualified type.
size_t
qualified_type_def::get_size_in_bits() const
{
  size_t s = get_underlying_type()->get_size_in_bits();
  if (s != type_base::get_size_in_bits())
    const_cast<qualified_type_def*>(this)->set_size_in_bits(s);
  return type_base::get_size_in_bits();
}

/// Compares two instances of @ref qualified_type_def.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const qualified_type_def& l, const qualified_type_def& r, change_kind* k)
{
  bool result = true;
  if (l.get_cv_quals() != r.get_cv_quals())
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }

  if (l.get_underlying_type() != r.get_underlying_type())
    {
      result = false;
      if (k)
	{
	  if (!types_have_similar_structure(l.get_underlying_type().get(),
					    r.get_underlying_type().get()))
	    // Underlying type changes in which the structure of the
	    // type changed are considered local changes to the
	    // qualified type.
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	  else
	    *k |= SUBTYPE_CHANGE_KIND;
	}
      else
	// okay strictly speaking this is not necessary, but I am
	// putting it here to maintenance; that is, so that adding
	// subsequent clauses needed to compare two qualified types
	// later still works.
	return false;
    }

  return result;
}

/// Equality operator for qualified types.
///
/// Note that this function does not check for equality of the scopes.
///
///@param o the other qualified type to compare against.
///
/// @return true iff both qualified types are equal.
bool
qualified_type_def::operator==(const decl_base& o) const
{
  const qualified_type_def* other =
    dynamic_cast<const qualified_type_def*>(&o);
  if (!other)
    return false;
  return try_canonical_compare(this, other);
}

/// Equality operator for qualified types.
///
/// Note that this function does not check for equality of the scopes.
/// Also, this re-uses the equality operator above that takes a
/// decl_base.
///
///@param o the other qualified type to compare against.
///
/// @return true iff both qualified types are equal.
bool
qualified_type_def::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Equality operator for qualified types.
///
/// Note that this function does not check for equality of the scopes.
/// Also, this re-uses the equality operator above that takes a
/// decl_base.
///
///@param o the other qualified type to compare against.
///
/// @return true iff both qualified types are equal.
bool
qualified_type_def::operator==(const qualified_type_def& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Implementation for the virtual qualified name builder for @ref
/// qualified_type_def.
///
/// @param qualified_name the output parameter to hold the resulting
/// qualified name.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
qualified_type_def::get_qualified_name(interned_string& qualified_name,
				       bool internal) const
{qualified_name = get_qualified_name(internal);}

/// Implementation of the virtual qualified name builder/getter.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the resulting qualified name.
const interned_string&
qualified_type_def::get_qualified_name(bool internal) const
{
  const environment* env = get_environment();
  ABG_ASSERT(env);

  if (!get_canonical_type())
    {
      // The type hasn't been canonicalized yet. We want to return a
      // temporary name that is not cached because the structure of
      // this type (and so its name) can change until its
      // canonicalized.
      if (internal)
	{
	  // We are asked to return a temporary *internal* name.
	  // Lets compute it and return a reference to where it's
	  // stored.
	  priv_->temporary_internal_name_ =
	      env->intern(build_name(true, /*internal=*/true));
	  return priv_->temporary_internal_name_;
	}
      else
	{
	  // We are asked to return a temporary non-internal name.
	    set_temporary_qualified_name
	      (env->intern(build_name(true, /*internal=*/false)));
	  return peek_temporary_qualified_name();
	}
    }
  else
    {
      // The type has already been canonicalized. We want to return
      // the definitive name and cache it.
      if (internal)
	{
	  if (priv_->internal_name_.empty())
	    priv_->internal_name_ =
	      env->intern(build_name(/*qualified=*/true,
				     /*internal=*/true));
	  return priv_->internal_name_;
	}
      else
	{
	  if (peek_qualified_name().empty())
	    set_qualified_name
	      (env->intern(build_name(/*qualified=*/true,
				      /*internal=*/false)));
	  return peek_qualified_name();
	}
    }
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
qualified_type_def::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_underlying_type())
	t->traverse(v);
      visiting(false);
    }
  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

qualified_type_def::~qualified_type_def()
{
}

/// Getter of the const/volatile qualifier bit field
qualified_type_def::CV
qualified_type_def::get_cv_quals() const
{return priv_->cv_quals_;}

/// Setter of the const/value qualifiers bit field
void
qualified_type_def::set_cv_quals(CV cv_quals)
{priv_->cv_quals_ = cv_quals;}

/// Compute and return the string prefix or suffix representing the
/// qualifiers hold by the current instance of @ref
/// qualified_type_def.
///
/// @return the newly-built cv string.
string
qualified_type_def::get_cv_quals_string_prefix() const
{return get_string_representation_of_cv_quals(priv_->cv_quals_);}

/// Getter of the underlying type
type_base_sptr
qualified_type_def::get_underlying_type() const
{return priv_->underlying_type_.lock();}

/// Setter of the underlying type.
///
/// @param t the new underlying type.
void
qualified_type_def::set_underlying_type(const type_base_sptr& t)
{priv_->underlying_type_ = t;}

/// Non-member equality operator for @ref qualified_type_def
///
/// @param l the left-hand side of the equality operator
///
/// @param r the right-hand side of the equality operator
///
/// @return true iff @p l and @p r equals.
bool
operator==(const qualified_type_def_sptr& l, const qualified_type_def_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Non-member inequality operator for @ref qualified_type_def
///
/// @param l the left-hand side of the equality operator
///
/// @param r the right-hand side of the equality operator
///
/// @return true iff @p l and @p r equals.
bool
operator!=(const qualified_type_def_sptr& l, const qualified_type_def_sptr& r)
{return ! operator==(l, r);}

/// Overloaded bitwise OR operator for cv qualifiers.
qualified_type_def::CV
operator|(qualified_type_def::CV lhs, qualified_type_def::CV rhs)
{
  return static_cast<qualified_type_def::CV>
    (static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
}

/// Overloaded bitwise |= operator for cv qualifiers.
qualified_type_def::CV&
operator|=(qualified_type_def::CV& l, qualified_type_def::CV r)
{
  l = l | r;
  return l;
}

/// Overloaded bitwise AND operator for CV qualifiers.
qualified_type_def::CV
operator&(qualified_type_def::CV lhs, qualified_type_def::CV rhs)
{
    return static_cast<qualified_type_def::CV>
    (static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
}

/// Overloaded bitwise inverting operator for CV qualifiers.
qualified_type_def::CV
operator~(qualified_type_def::CV q)
{return static_cast<qualified_type_def::CV>(~static_cast<unsigned>(q));}

/// Streaming operator for qualified_type_decl::CV
///
/// @param o the output stream to serialize the cv qualifier to.
///
/// @param cv the cv qualifier to serialize.
///
/// @return the output stream used.
std::ostream&
operator<<(std::ostream& o, qualified_type_def::CV cv)
{
  string str;

  switch (cv)
    {
    case qualified_type_def::CV_NONE:
      str = "none";
      break;
    case qualified_type_def::CV_CONST:
      str = "const";
      break;
    case qualified_type_def::CV_VOLATILE:
      str = "volatile";
      break;
    case qualified_type_def::CV_RESTRICT:
      str = "restrict";
      break;
    }

  o << str;
  return o;
}

// </qualified_type_def>

//<pointer_type_def definitions>

/// Private data structure of the @ref pointer_type_def.
struct pointer_type_def::priv
{
  type_base_wptr pointed_to_type_;
  type_base* naked_pointed_to_type_;
  interned_string internal_qualified_name_;
  interned_string temp_internal_qualified_name_;

  priv(const type_base_sptr& t)
    : pointed_to_type_(type_or_void(t, 0)),
      naked_pointed_to_type_(t.get())
  {}

  priv()
    : naked_pointed_to_type_()
  {}
}; //end struct pointer_type_def

/// This function is automatically invoked whenever an instance of
/// this type is canonicalized.
///
/// It's an overload of the virtual type_base::on_canonical_type_set.
///
/// We put here what is thus meant to be executed only at the point of
/// type canonicalization.
void
pointer_type_def::on_canonical_type_set()
{clear_qualified_name();}

pointer_type_def::pointer_type_def(const type_base_sptr&	pointed_to,
				   size_t			size_in_bits,
				   size_t			align_in_bits,
				   const location&		locus)
  : type_or_decl_base(pointed_to->get_environment(),
		      POINTER_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    type_base(pointed_to->get_environment(), size_in_bits, align_in_bits),
    decl_base(pointed_to->get_environment(), "", locus, ""),
    priv_(new priv(pointed_to))
{
  runtime_type_instance(this);
  try
    {
      ABG_ASSERT(pointed_to);
      const environment* env = pointed_to->get_environment();
      decl_base_sptr pto = dynamic_pointer_cast<decl_base>(pointed_to);
      string name = (pto ? pto->get_name() : string("void")) + "*";
      set_name(env->intern(name));
      if (pto)
	set_visibility(pto->get_visibility());
    }
  catch (...)
    {}
}

/// Compares two instances of @ref pointer_type_def.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const pointer_type_def& l, const pointer_type_def& r, change_kind* k)
{
  // Compare the pointed-to-types modulo the typedefs they might have
  bool result = (peel_typedef_type(l.get_pointed_to_type())
		 == peel_typedef_type(r.get_pointed_to_type()));
  if (!result)
    if (k)
      {
	if (!types_have_similar_structure(&l, &r))
	  // pointed-to type changes in which the structure of the
	  // type changed are considered local changes to the pointer
	  // type.
	  *k |= LOCAL_TYPE_CHANGE_KIND;
	*k |= SUBTYPE_CHANGE_KIND;
      }

  return result;
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the this
/// types.
bool
pointer_type_def::operator==(const decl_base& o) const
{
  const pointer_type_def* other = is_pointer_type(&o);
  if (!other)
    return false;
  return try_canonical_compare(this, other);
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the
/// types.
///
/// @param other the other type to compare against.
///
/// @return true iff @p other equals the current instance.
bool
pointer_type_def::operator==(const type_base& other) const
{
  const decl_base* o = is_decl(&other);
  if (!o)
    return false;
  return *this == *o;
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the
/// types.
///
/// @param other the other type to compare against.
///
/// @return true iff @p other equals the current instance.
bool
pointer_type_def::operator==(const pointer_type_def& other) const
{
  const decl_base& o = other;
  return *this == o;
}

/// Getter of the pointed-to type.
///
/// @return the pointed-to type.
const type_base_sptr
pointer_type_def::get_pointed_to_type() const
{return priv_->pointed_to_type_.lock();}

/// Getter of a naked pointer to the pointed-to type.
///
/// @return a naked pointed to the pointed-to type.
type_base*
pointer_type_def::get_naked_pointed_to_type() const
{return priv_->naked_pointed_to_type_;}

/// Build and return the qualified name of the current instance of
/// @ref pointer_type_def.
///
/// @param qn output parameter.  The resulting qualified name.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
pointer_type_def::get_qualified_name(interned_string& qn, bool internal) const
{qn = get_qualified_name(internal);}

/// Build, cache and return the qualified name of the current instance
/// of @ref pointer_type_def.  Subsequent invocations of this function
/// return the cached value.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the resulting qualified name.
const interned_string&
pointer_type_def::get_qualified_name(bool internal) const
{
  type_base* pointed_to_type = get_naked_pointed_to_type();

  if (internal)
    {
      if (get_canonical_type())
	{
	  if (priv_->internal_qualified_name_.empty())
	    priv_->internal_qualified_name_ =
	      get_name_of_pointer_to_type(*pointed_to_type,
					  /*qualified_name=*/true,
					  /*internal=*/true);
	  return priv_->internal_qualified_name_;
	}
      else
	{
	  // As the type hasn't yet been canonicalized, its structure
	  // (and so its name) can change.  So let's invalidate the
	  // cache where we store its name at each invocation of this
	  // function.
	  priv_->temp_internal_qualified_name_ =
	    get_name_of_pointer_to_type(*pointed_to_type,
					/*qualified_name=*/true,
					/*internal=*/true);
	  return priv_->temp_internal_qualified_name_;
	}
    }
  else
    {
      if (get_naked_canonical_type())
	{
	  if (decl_base::peek_qualified_name().empty())
	    set_qualified_name
	      (get_name_of_pointer_to_type(*pointed_to_type,
					   /*qualified_name=*/true,
					   /*internal=*/false));
	  return decl_base::peek_qualified_name();
	}
      else
	{
	  // As the type hasn't yet been canonicalized, its structure
	  // (and so its name) can change.  So let's invalidate the
	  // cache where we store its name at each invocation of this
	  // function.
	  set_qualified_name
	    (get_name_of_pointer_to_type(*pointed_to_type,
					 /*qualified_name=*/true,
					 /*internal=*/false));
	  return decl_base::peek_qualified_name();
	}
    }
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
pointer_type_def::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_pointed_to_type())
	t->traverse(v);
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

pointer_type_def::~pointer_type_def()
{}

/// Turn equality of shared_ptr of @ref pointer_type_def into a deep
/// equality; that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of @ref pointer_type_def on left-hand-side
/// of the equality.
///
/// @param r the shared_ptr of @ref pointer_type_def on
/// right-hand-side of the equality.
///
/// @return true if the @ref pointer_type_def pointed to by the
/// shared_ptrs are equal, false otherwise.
bool
operator==(const pointer_type_def_sptr& l, const pointer_type_def_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Turn inequality of shared_ptr of @ref pointer_type_def into a deep
/// equality; that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of @ref pointer_type_def on left-hand-side
/// of the equality.
///
/// @param r the shared_ptr of @ref pointer_type_def on
/// right-hand-side of the equality.
///
/// @return true iff the @ref pointer_type_def pointed to by the
/// shared_ptrs are different.
bool
operator!=(const pointer_type_def_sptr& l, const pointer_type_def_sptr& r)
{return !operator==(l, r);}

// </pointer_type_def definitions>

// <reference_type_def definitions>

/// This function is automatically invoked whenever an instance of
/// this type is canonicalized.
///
/// It's an overload of the virtual type_base::on_canonical_type_set.
///
/// We put here what is thus meant to be executed only at the point of
/// type canonicalization.
void
reference_type_def::on_canonical_type_set()
{clear_qualified_name();}

reference_type_def::reference_type_def(const type_base_sptr	pointed_to,
				       bool			lvalue,
				       size_t			size_in_bits,
				       size_t			align_in_bits,
				       const location&		locus)
  : type_or_decl_base(pointed_to->get_environment(),
		      REFERENCE_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    type_base(pointed_to->get_environment(), size_in_bits, align_in_bits),
    decl_base(pointed_to->get_environment(), "", locus, ""),
    is_lvalue_(lvalue)
{
  runtime_type_instance(this);

  try
    {
      decl_base_sptr pto = dynamic_pointer_cast<decl_base>(pointed_to);
      string name;
      if (pto)
	{
	  set_visibility(pto->get_visibility());
	  name = string(pto->get_name()) + "&";
	}
      else
	name = string(get_type_name(is_function_type(pointed_to),
				    /*qualified_name=*/true)) + "&";

      if (!is_lvalue())
	name += "&";
      environment* env = pointed_to->get_environment();
      ABG_ASSERT(env);
      set_name(env->intern(name));

      pointed_to_type_ = type_base_wptr(type_or_void(pointed_to, 0));
    }
  catch (...)
    {}
}

/// Compares two instances of @ref reference_type_def.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const reference_type_def& l, const reference_type_def& r, change_kind* k)
{
  if (l.is_lvalue() != r.is_lvalue())
    {
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      return false;
    }

  // Compare the pointed-to-types modulo the typedefs they might have
  bool result = (peel_typedef_type(l.get_pointed_to_type())
		 == (peel_typedef_type(r.get_pointed_to_type())));
  if (!result)
    if (k)
      {
	if (!types_have_similar_structure(&l, &r))
	  *k |= LOCAL_TYPE_CHANGE_KIND;
	*k |= SUBTYPE_CHANGE_KIND;
      }
  return result;
}

/// Equality operator of the @ref reference_type_def type.
///
/// @param o the other instance of @ref reference_type_def to compare
/// against.
///
/// @return true iff the two instances are equal.
bool
reference_type_def::operator==(const decl_base& o) const
{
  const reference_type_def* other =
    dynamic_cast<const reference_type_def*>(&o);
  if (!other)
    return false;
  return try_canonical_compare(this, other);
}

/// Equality operator of the @ref reference_type_def type.
///
/// @param o the other instance of @ref reference_type_def to compare
/// against.
///
/// @return true iff the two instances are equal.
bool
reference_type_def::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Equality operator of the @ref reference_type_def type.
///
/// @param o the other instance of @ref reference_type_def to compare
/// against.
///
/// @return true iff the two instances are equal.
bool
reference_type_def::operator==(const reference_type_def& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

type_base_sptr
reference_type_def::get_pointed_to_type() const
{return pointed_to_type_.lock();}

bool
reference_type_def::is_lvalue() const
{return is_lvalue_;}

/// Build and return the qualified name of the current instance of the
/// @ref reference_type_def.
///
/// @param qn output parameter.  Is set to the newly-built qualified
/// name of the current instance of @ref reference_type_def.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
reference_type_def::get_qualified_name(interned_string& qn, bool internal) const
{qn = get_qualified_name(internal);}

/// Build, cache and return the qualified name of the current instance
/// of the @ref reference_type_def.  Subsequent invocations of this
/// function return the cached value.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the newly-built qualified name of the current instance of
/// @ref reference_type_def.
const interned_string&
reference_type_def::get_qualified_name(bool internal) const
{
  if (peek_qualified_name().empty()
      || !get_canonical_type())
    set_qualified_name(get_name_of_reference_to_type(*get_pointed_to_type(),
						     is_lvalue(),
						     /*qualified_name=*/true,
						     internal));
  return peek_qualified_name();
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
reference_type_def::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_pointed_to_type())
	t->traverse(v);
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

reference_type_def::~reference_type_def()
{}

/// Turn equality of shared_ptr of @ref reference_type_def into a deep
/// equality; that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of @ref reference_type_def on left-hand-side
/// of the equality.
///
/// @param r the shared_ptr of @ref reference_type_def on
/// right-hand-side of the equality.
///
/// @return true if the @ref reference_type_def pointed to by the
/// shared_ptrs are equal, false otherwise.
bool
operator==(const reference_type_def_sptr& l, const reference_type_def_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Turn inequality of shared_ptr of @ref reference_type_def into a deep
/// equality; that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of @ref reference_type_def on left-hand-side
/// of the equality.
///
/// @param r the shared_ptr of @ref reference_type_def on
/// right-hand-side of the equality.
///
/// @return true iff the @ref reference_type_def pointed to by the
/// shared_ptrs are different.
bool
operator!=(const reference_type_def_sptr& l, const reference_type_def_sptr& r)
{return !operator==(l, r);}

// </reference_type_def definitions>

// <array_type_def definitions>

// <array_type_def::subrange_type>

// <array_type_def::subrante_type::bound_value>

/// Default constructor of the @ref
/// array_type_def::subrange_type::bound_value class.
///
/// Constructs an unsigned bound_value of value zero.
array_type_def::subrange_type::bound_value::bound_value()
  : s_(UNSIGNED_SIGNEDNESS)
{
  v_.unsigned_ = 0;
}

/// Initialize an unsigned bound_value with a given value.
///
/// @param v the initial bound value.
array_type_def::subrange_type::bound_value::bound_value(uint64_t v)
  : s_(UNSIGNED_SIGNEDNESS)
{
  v_.unsigned_ = v;
}

/// Initialize a signed bound_value with a given value.
///
/// @param v the initial bound value.
array_type_def::subrange_type::bound_value::bound_value(int64_t v)
  : s_(SIGNED_SIGNEDNESS)
{
  v_.signed_ = v;
}

/// Getter of the signedness (unsigned VS signed) of the bound value.
///
/// @return the signedness of the bound value.
enum array_type_def::subrange_type::bound_value::signedness
array_type_def::subrange_type::bound_value::get_signedness() const
{return s_;}

/// Setter of the signedness (unsigned VS signed) of the bound value.
///
/// @param s the new signedness of the bound value.
void
array_type_def::subrange_type::bound_value::set_signedness(enum signedness s)
{ s_ = s;}

/// Getter of the bound value as a signed value.
///
/// @return the bound value as signed.
int64_t
array_type_def::subrange_type::bound_value::get_signed_value() const
{return v_.signed_;
}

/// Getter of the bound value as an unsigned value.
///
/// @return the bound value as unsigned.
uint64_t
array_type_def::subrange_type::bound_value::get_unsigned_value()
{return v_.unsigned_;}

/// Setter of the bound value as unsigned.
///
/// @param v the new unsigned value.
void
array_type_def::subrange_type::bound_value::set_unsigned(uint64_t v)
{
    s_ = UNSIGNED_SIGNEDNESS;
  v_.unsigned_ = v;
}

/// Setter of the bound value as signed.
///
/// @param v the new signed value.
void
array_type_def::subrange_type::bound_value::set_signed(int64_t v)
{
  s_ = SIGNED_SIGNEDNESS;
  v_.signed_ = v;
}

/// Equality operator of the bound value.
///
/// @param v the other bound value to compare with.
///
/// @return true iff the current bound value equals @p v.
bool
array_type_def::subrange_type::bound_value::operator==(const bound_value& v) const
{
  return s_ == v.s_ && v_.unsigned_ == v.v_.unsigned_;
}

// </array_type_def::subrante_type::bound_value>

struct array_type_def::subrange_type::priv
{
  bound_value		lower_bound_;
  bound_value		upper_bound_;
  type_base_wptr	underlying_type_;
  translation_unit::language language_;
  bool			infinite_;

  priv(bound_value ub,
       translation_unit::language l = translation_unit::LANG_C11)
    : upper_bound_(ub), language_(l), infinite_(false)
  {}

  priv(bound_value lb, bound_value ub,
       translation_unit::language l = translation_unit::LANG_C11)
    : lower_bound_(lb), upper_bound_(ub),
      language_(l), infinite_(false)
  {}

  priv(bound_value lb, bound_value ub, const type_base_sptr &u,
       translation_unit::language l = translation_unit::LANG_C11)
    : lower_bound_(lb), upper_bound_(ub), underlying_type_(u),
      language_(l), infinite_(false)
  {}
};

/// Constructor of an array_type_def::subrange_type type.
///
/// @param env the environment this type was created from.
///
/// @param name the name of the subrange type.
///
/// @param lower_bound the lower bound of the array.  This is
/// generally zero (at least for C and C++).
///
/// @param upper_bound the upper bound of the array.
///
/// @param underlying_type the underlying type of the subrange type.
///
/// @param loc the source location where the type is defined.
array_type_def::subrange_type::subrange_type(const environment* env,
					     const string&	name,
					     bound_value	lower_bound,
					     bound_value	upper_bound,
					     const type_base_sptr& utype,
					     const location&	loc,
					     translation_unit::language l)
  : type_or_decl_base(env, ABSTRACT_TYPE_BASE | ABSTRACT_DECL_BASE),
    type_base(env,
	      upper_bound.get_unsigned_value()
	      - lower_bound.get_unsigned_value(),
	      0),
    decl_base(env, name, loc, ""),
    priv_(new priv(lower_bound, upper_bound, utype, l))
{
  runtime_type_instance(this);
}

/// Constructor of the array_type_def::subrange_type type.
///
/// @param env the environment this type is being created in.
///
/// @param name the name of the subrange type.
///
/// @param lower_bound the lower bound of the array.  This is
/// generally zero (at least for C and C++).
///
/// @param upper_bound the upper bound of the array.
///
/// @param loc the source location where the type is defined.
///
/// @param l the language that generated this subrange.
array_type_def::subrange_type::subrange_type(const environment* env,
					     const string&	name,
					     bound_value	lower_bound,
					     bound_value	upper_bound,
					     const location&	loc,
					     translation_unit::language l)
  : type_or_decl_base(env, ABSTRACT_TYPE_BASE | ABSTRACT_DECL_BASE),
    type_base(env,
	      upper_bound.get_unsigned_value()
	      - lower_bound.get_unsigned_value(), 0),
    decl_base(env, name, loc, ""),
    priv_(new priv(lower_bound, upper_bound, l))
{
  runtime_type_instance(this);
}

/// Constructor of the array_type_def::subrange_type type.
///
/// @param env the environment this type is being created from.
///
/// @param name of the name of type.
///
/// @param upper_bound the upper bound of the array.  The lower bound
/// is considered to be zero.
///
/// @param loc the source location of the type.
///
/// @param the language that generated this type.
array_type_def::subrange_type::subrange_type(const environment* env,
					     const string&	name,
					     bound_value	upper_bound,
					     const location&	loc,
					     translation_unit::language l)
  : type_or_decl_base(env, ABSTRACT_TYPE_BASE | ABSTRACT_DECL_BASE),
    type_base(env, upper_bound.get_unsigned_value(), 0),
    decl_base(env, name, loc, ""),
    priv_(new priv(upper_bound, l))
{
  runtime_type_instance(this);
}

/// Getter of the underlying type of the subrange, that is, the type
/// that defines the range.
///
/// @return the underlying type.
type_base_sptr
array_type_def::subrange_type::get_underlying_type() const
{return priv_->underlying_type_.lock();}

/// Setter of the underlying type of the subrange, that is, the type
/// that defines the range.
///
/// @param u the new underlying type.
void
array_type_def::subrange_type::set_underlying_type(const type_base_sptr &u)
{
  ABG_ASSERT(priv_->underlying_type_.expired());
  priv_->underlying_type_ = u;
}

/// Getter of the upper bound of the subrange type.
///
/// @return the upper bound of the subrange type.
int64_t
array_type_def::subrange_type::get_upper_bound() const
{return priv_->upper_bound_.get_signed_value();}

/// Getter of the lower bound of the subrange type.
///
/// @return the lower bound of the subrange type.
int64_t
array_type_def::subrange_type::get_lower_bound() const
{return priv_->lower_bound_.get_signed_value();}

/// Setter of the upper bound of the subrange type.
///
/// @param ub the new value of the upper bound.
void
array_type_def::subrange_type::set_upper_bound(int64_t ub)
{priv_->upper_bound_ = ub;}

/// Setter of the lower bound.
///
/// @param lb the new value of the lower bound.
void
array_type_def::subrange_type::set_lower_bound(int64_t lb)
{priv_->lower_bound_ = lb;}

/// Getter of the length of the subrange type.
///
/// Note that a length of zero means the array has an infinite (or
/// rather a non-known) size.
///
/// @return the length of the subrange type.
uint64_t
array_type_def::subrange_type::get_length() const
{
  if (is_infinite())
    return 0;

  ABG_ASSERT(get_upper_bound() >= get_lower_bound());
  return get_upper_bound() - get_lower_bound() + 1;
}

/// Test if the length of the subrange type is infinite.
///
/// @return true iff the length of the subrange type is infinite.
bool
array_type_def::subrange_type::is_infinite() const
{return priv_->infinite_;}

/// Set the infinite-ness status of the subrange type.
///
/// @param f true iff the length of the subrange type should be set to
/// being infinite.
void
array_type_def::subrange_type::is_infinite(bool f)
{priv_->infinite_ = f;}

/// Getter of the language that generated this type.
///
/// @return the language of this type.
translation_unit::language
array_type_def::subrange_type::get_language() const
{return priv_->language_;}

/// Return a string representation of the sub range.
///
/// @return the string representation of the sub range.
string
array_type_def::subrange_type::as_string() const
{
  std::ostringstream o;

  if (is_ada_language(get_language()))
    {
      type_base_sptr underlying_type = get_underlying_type();
      if (underlying_type)
	o << ir::get_pretty_representation(underlying_type, false) << " ";
      o << "range "<< get_lower_bound() << " .. " << get_upper_bound();
    }
  else if (is_infinite())
    o << "[]";
  else
    o << "["  << get_length() << "]";

  return o.str();
}

/// Return a string representation of a vector of subranges
///
/// @return the string representation of a vector of sub ranges.
string
array_type_def::subrange_type::vector_as_string(const vector<subrange_sptr>& v)
{
  if (v.empty())
    return "[]";

  string r;
  for (vector<subrange_sptr>::const_iterator i = v.begin();
       i != v.end();
       ++i)
    r += (*i)->as_string();

  return r;
}

/// Compares two isntances of @ref array_type_def::subrange_type.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const array_type_def::subrange_type& l,
       const array_type_def::subrange_type& r,
       change_kind* k)
{
  bool result = true;

  if (l.get_lower_bound() != r.get_lower_bound()
      || l.get_upper_bound() != r.get_upper_bound()
      || l.get_name() != r.get_name())
    {
      result = false;
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      else
	return result;
    }

#if 0
  // If we enable this, we need to update the reporting code too, to
  // report changes about range underlying types too.
  if (l.get_underlying_type() != r.get_underlying_type())
    {
      result = false;
      if (k)
	{
	  if (!types_have_similar_structure(l.get_underlying_type().get(),
					    r.get_underlying_type().get()))
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	  else
	    *k |= SUBTYPE_CHANGE_KIND;
	}
      else
	return result;
    }
#endif
  return result;
}

/// Equality operator.
///
/// @param o the other subrange to test against.
///
/// @return true iff @p o equals the current instance of
/// array_type_def::subrange_type.
bool
array_type_def::subrange_type::operator==(const decl_base& o) const
{
  const subrange_type* other =
    dynamic_cast<const subrange_type*>(&o);
  if (!other)
    return false;
  return try_canonical_compare(this, other);
}

/// Equality operator.
///
/// @param o the other subrange to test against.
///
/// @return true iff @p o equals the current instance of
/// array_type_def::subrange_type.
bool
array_type_def::subrange_type::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Equality operator.
///
/// @param o the other subrange to test against.
///
/// @return true iff @p o equals the current instance of
/// array_type_def::subrange_type.
bool
array_type_def::subrange_type::operator==(const subrange_type& o) const
{
  const type_base &t = o;
  return operator==(t);
}

/// Inequality operator.
///
/// @param o the other subrange to test against.
///
/// @return true iff @p o is different from the current instance of
/// array_type_def::subrange_type.
bool
array_type_def::subrange_type::operator!=(const subrange_type& o) const
{return !operator==(o);}

/// Build a pretty representation for an
/// array_type_def::subrange_type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of the current
/// instance of typedef_decl.
string
array_type_def::subrange_type::get_pretty_representation(bool, bool) const
{
  string name = get_name();
  string repr;

  if (name.empty())
    repr += "<anonymous range>";
  else
    repr += "<range " + get_name() + ">";
  repr += as_string();

  return repr;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
array_type_def::subrange_type::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr u = get_underlying_type())
	u->traverse(v);
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

// </array_type_def::subrange_type>

struct array_type_def::priv
{
  type_base_wptr	element_type_;
  subranges_type	subranges_;
  interned_string	temp_internal_qualified_name_;
  interned_string	internal_qualified_name_;

  priv(type_base_sptr t)
    : element_type_(t) {}
  priv(type_base_sptr t, subranges_type subs)
    : element_type_(t), subranges_(subs) {}
};

/// Constructor for the type array_type_def
///
/// Note how the constructor expects a vector of subrange
/// objects. Parsing of the array information always entails
/// parsing the subrange info as well, thus the class subrange_type
/// is defined inside class array_type_def and also parsed
/// simultaneously.
///
/// @param e_type the type of the elements contained in the array
///
/// @param subs a vector of the array's subranges(dimensions)
///
/// @param locus the source location of the array type definition.
array_type_def::array_type_def(const type_base_sptr			e_type,
			       const std::vector<subrange_sptr>&	subs,
			       const location&				locus)
  : type_or_decl_base(e_type->get_environment(),
		      ARRAY_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    type_base(e_type->get_environment(), 0, e_type->get_alignment_in_bits()),
    decl_base(e_type->get_environment(), locus),
    priv_(new priv(e_type))
{
  runtime_type_instance(this);
  append_subranges(subs);
}

string
array_type_def::get_subrange_representation() const
{
  string r = subrange_type::vector_as_string(get_subranges());
  return r;
}

/// Get the string representation of an @ref array_type_def.
///
/// @param a the array type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
static string
get_type_representation(const array_type_def& a, bool internal)
{
  type_base_sptr e_type = a.get_element_type();
  decl_base_sptr d = get_type_declaration(e_type);
  string r;

  if (is_ada_language(a.get_language()))
    {
      std::ostringstream o;
      o << "array ("
	<< a.get_subrange_representation()
	<< ") of "
	<<  e_type->get_pretty_representation(internal);
    }
  else
    {
      if (internal)
	r = get_type_name(e_type, /*qualified=*/true, /*internal=*/true)
	  + a.get_subrange_representation();
      else
	r = get_type_name(e_type, /*qualified=*/false, /*internal=*/false)
	  + a.get_subrange_representation();
    }

  return r;
}

/// Get the pretty representation of the current instance of @ref
/// array_type_def.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
string
array_type_def::get_pretty_representation(bool internal,
					  bool /*qualified_name*/) const
{return get_type_representation(*this, internal);}

/// Compares two instances of @ref array_type_def.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const array_type_def& l, const array_type_def& r, change_kind* k)
{
  std::vector<array_type_def::subrange_sptr > this_subs = l.get_subranges();
  std::vector<array_type_def::subrange_sptr > other_subs = r.get_subranges();

  bool result = true;
  if (this_subs.size() != other_subs.size())
    {
      result = false;
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      else
	return false;
    }

  std::vector<array_type_def::subrange_sptr >::const_iterator i,j;
  for (i = this_subs.begin(), j = other_subs.begin();
       i != this_subs.end() && j != other_subs.end();
       ++i, ++j)
    if (**i != **j)
      {
	result = false;
	if (k)
	  {
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	    break;
	  }
	else
	  return false;
      }

  // Compare the element types modulo the typedefs they might have
  if (peel_typedef_type(l.get_element_type())
      != peel_typedef_type(r.get_element_type()))
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

/// Test if two variables are equals modulo CV qualifiers.
///
/// @param l the first array of the comparison.
///
/// @param r the second array of the comparison.
///
/// @return true iff @p l equals @p r or, if they are different, the
/// difference between the too is just a matter of CV qualifiers.
bool
equals_modulo_cv_qualifier(const array_type_def* l, const array_type_def* r)
{
  if (l == r)
    return true;

  if (!l || !r)
    return false;

  l = is_array_type(peel_qualified_or_typedef_type(l));
  r = is_array_type(peel_qualified_or_typedef_type(r));

  std::vector<array_type_def::subrange_sptr > this_subs = l->get_subranges();
  std::vector<array_type_def::subrange_sptr > other_subs = r->get_subranges();

  if (this_subs.size() != other_subs.size())
    return false;

  std::vector<array_type_def::subrange_sptr >::const_iterator i,j;
  for (i = this_subs.begin(), j = other_subs.begin();
       i != this_subs.end() && j != other_subs.end();
       ++i, ++j)
    if (**i != **j)
      return false;

  type_base *first_element_type =
    peel_qualified_or_typedef_type(l->get_element_type().get());
  type_base *second_element_type =
    peel_qualified_or_typedef_type(r->get_element_type().get());

  if (*first_element_type != *second_element_type)
    return false;

  return true;
}

/// Get the language of the array.
///
/// @return the language of the array.
translation_unit::language
array_type_def::get_language() const
{
  const std::vector<subrange_sptr>& subranges =
    get_subranges();

  if (subranges.empty())
    return translation_unit::LANG_C11;
  return subranges.front()->get_language();
}

bool
array_type_def::operator==(const decl_base& o) const
{
  const array_type_def* other =
    dynamic_cast<const array_type_def*>(&o);
  if (!other)
    return false;
  return try_canonical_compare(this, other);
}

bool
array_type_def::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Getter of the type of an array element.
///
/// @return the type of an array element.
const type_base_sptr
array_type_def::get_element_type() const
{return priv_->element_type_.lock();}

/// Setter of the type of array element.
///
/// Beware that after using this function, one might want to
/// re-compute the canonical type of the array, if one has already
/// been computed.
///
/// The intended use of this method is to permit in-place adjustment
/// of the element type's qualifiers. In particular, the size of the
/// element type should not be changed.
///
/// @param element_type the new element type to set.
void
array_type_def::set_element_type(const type_base_sptr& element_type)
{
  priv_->element_type_ = element_type;
}

/// Append subranges from the vector @param subs to the current
/// vector of subranges.
void
array_type_def::append_subranges(const std::vector<subrange_sptr>& subs)
{
  size_t s = get_element_type()->get_size_in_bits();

  for (std::vector<shared_ptr<subrange_type> >::const_iterator i = subs.begin();
       i != subs.end();
       ++i)
    {
      priv_->subranges_.push_back(*i);
      s *= (*i)->get_length();
    }

  const environment* env = get_environment();
  ABG_ASSERT(env);
  set_name(env->intern(get_pretty_representation()));
  set_size_in_bits(s);
}

/// @return true if one of the sub-ranges of the array is infinite, or
/// if the array has no sub-range at all, also meaning that the size
/// of the array is infinite.
bool
array_type_def::is_infinite() const
{
  if (priv_->subranges_.empty())
    return true;

  for (std::vector<shared_ptr<subrange_type> >::const_iterator i =
	 priv_->subranges_.begin();
       i != priv_->subranges_.end();
       ++i)
    if ((*i)->is_infinite())
      return true;

  return false;
}

int
array_type_def::get_dimension_count() const
{return priv_->subranges_.size();}

/// Build and return the qualified name of the current instance of the
/// @ref array_type_def.
///
/// @param qn output parameter.  Is set to the newly-built qualified
/// name of the current instance of @ref array_type_def.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
array_type_def::get_qualified_name(interned_string& qn, bool internal) const
{qn = get_qualified_name(internal);}

/// Compute the qualified name of the array.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the resulting qualified name.
const interned_string&
array_type_def::get_qualified_name(bool internal) const
{
  const environment* env = get_environment();
  ABG_ASSERT(env);

  if (internal)
    {
      if (get_canonical_type())
	{
	  if (priv_->internal_qualified_name_.empty())
	    priv_->internal_qualified_name_ =
	      env->intern(get_type_representation(*this, /*internal=*/true));
	  return priv_->internal_qualified_name_;
	}
      else
	{
	  priv_->temp_internal_qualified_name_ =
	    env->intern(get_type_representation(*this, /*internal=*/true));
	  return priv_->temp_internal_qualified_name_;
	}
    }
  else
    {
      if (get_canonical_type())
	{
	  if (decl_base::peek_qualified_name().empty())
	    set_qualified_name(env->intern(get_type_representation
					   (*this, /*internal=*/false)));
	  return decl_base::peek_qualified_name();
	}
      else
	{
	  set_qualified_name(env->intern(get_type_representation
					 (*this, /*internal=*/false)));
	  return decl_base::peek_qualified_name();
	}
    }
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
array_type_def::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_element_type())
	t->traverse(v);
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

const location&
array_type_def::get_location() const
{return decl_base::get_location();}

/// Get the array's subranges
const std::vector<array_type_def::subrange_sptr>&
array_type_def::get_subranges() const
{return priv_->subranges_;}

array_type_def::~array_type_def()
{}

// </array_type_def definitions>

// <enum_type_decl definitions>

class enum_type_decl::priv
{
  type_base_sptr	underlying_type_;
  enumerators		enumerators_;

  friend class enum_type_decl;

  priv();

public:
  priv(type_base_sptr underlying_type,
       enumerators& enumerators)
    : underlying_type_(underlying_type),
      enumerators_(enumerators)
  {}
}; // end class enum_type_decl::priv

/// Constructor.
///
/// @param name the name of the type declaration.
///
/// @param locus the source location where the type was defined.
///
/// @param underlying_type the underlying type of the enum.
///
/// @param enums the enumerators of this enum type.
///
/// @param linkage_name the linkage name of the enum.
///
/// @param vis the visibility of the enum type.
enum_type_decl::enum_type_decl(const string&	name,
			       const location&	locus,
			       type_base_sptr	underlying_type,
			       enumerators&	enums,
			       const string&	linkage_name,
			       visibility	vis)
  : type_or_decl_base(underlying_type->get_environment(),
		      ENUM_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    type_base(underlying_type->get_environment(),
	      underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(underlying_type->get_environment(),
	      name, locus, linkage_name, vis),
    priv_(new priv(underlying_type, enums))
{
  runtime_type_instance(this);
  for (enumerators::iterator e = get_enumerators().begin();
       e != get_enumerators().end();
       ++e)
    e->set_enum_type(this);
}

/// Return the underlying type of the enum.
type_base_sptr
enum_type_decl::get_underlying_type() const
{return priv_->underlying_type_;}

/// @return the list of enumerators of the enum.
const enum_type_decl::enumerators&
enum_type_decl::get_enumerators() const
{return priv_->enumerators_;}

/// @return the list of enumerators of the enum.
enum_type_decl::enumerators&
enum_type_decl::get_enumerators()
{return priv_->enumerators_;}

/// Get the pretty representation of the current instance of @ref
/// enum_type_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qualified_name if true, names emitted in the pretty
/// representation are fully qualified.
///
/// @return the pretty representation of the enum type.
string
enum_type_decl::get_pretty_representation(bool internal,
					  bool qualified_name) const
{
  string r = "enum " + decl_base::get_pretty_representation(internal,
							    qualified_name);
  return r;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
enum_type_decl::traverse(ir_node_visitor &v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_underlying_type())
	t->traverse(v);
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

/// Destructor for the enum type declaration.
enum_type_decl::~enum_type_decl()
{}

/// Test if two enums differ, but not by a name change.
///
/// @param l the first enum to consider.
///
/// @param r the second enum to consider.
///
/// @return true iff @p l differs from @p r by anything but a name
/// change.
bool
enum_has_non_name_change(const enum_type_decl& l,
			 const enum_type_decl& r,
			 change_kind* k)
{
  bool result = false;
  if (*l.get_underlying_type() != *r.get_underlying_type())
    {
      result = true;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	return true;
    }

  enum_type_decl::enumerators::const_iterator i, j;
  for (i = l.get_enumerators().begin(), j = r.get_enumerators().begin();
       i != l.get_enumerators().end() && j != r.get_enumerators().end();
       ++i, ++j)
    if (*i != *j)
      {
	result = true;
	if (k)
	  {
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	    break;
	  }
	else
	  return true;
      }

  if (i != l.get_enumerators().end() || j != r.get_enumerators().end())
    {
      result = true;
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      else
	return true;
    }

  enum_type_decl &local_r = const_cast<enum_type_decl&>(r);
  interned_string qn_r = l.get_environment()->intern(r.get_qualified_name());
  interned_string qn_l = l.get_environment()->intern(l.get_qualified_name());
  string n_l = l.get_name();
  string n_r = r.get_name();
  local_r.set_qualified_name(qn_l);
  local_r.set_name(n_l);

  if (!(l.decl_base::operator==(r) && l.type_base::operator==(r)))
    {
      result = true;
      if (k)
	{
	  if (!l.decl_base::operator==(r))
	    *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	  if (!l.type_base::operator==(r))
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	}
      else
	{
	  local_r.set_name(n_r);
	  local_r.set_qualified_name(qn_r);
	  return true;
	}
    }
  local_r.set_qualified_name(qn_r);
  local_r.set_name(n_r);

  return result;
}

/// Compares two instances of @ref enum_type_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const enum_type_decl& l, const enum_type_decl& r, change_kind* k)
{
  bool result = true;
  if (*l.get_underlying_type() != *r.get_underlying_type())
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	return false;
    }

  enum_type_decl::enumerators::const_iterator i, j;
  for (i = l.get_enumerators().begin(), j = r.get_enumerators().begin();
       i != l.get_enumerators().end() && j != r.get_enumerators().end();
       ++i, ++j)
    if (*i != *j)
      {
	result = false;
	if (k)
	  {
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	    break;
	  }
	else
	  return false;
      }

  if (i != l.get_enumerators().end() || j != r.get_enumerators().end())
    {
      result = false;
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      else
	return false;
    }

  if (!(l.decl_base::operator==(r) && l.type_base::operator==(r)))
    {
      result = false;
      if (k)
	{
	  if (!l.decl_base::operator==(r))
	    *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	  if (!l.type_base::operator==(r))
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	}
      else
	return false;
    }

  return result;
}

/// Equality operator.
///
/// @param o the other enum to test against.
///
/// @return true iff @p o equals the current instance of enum type
/// decl.
bool
enum_type_decl::operator==(const decl_base& o) const
{
  const enum_type_decl* op = dynamic_cast<const enum_type_decl*>(&o);
  if (!op)
    return false;
  return try_canonical_compare(this, op);
}

/// Equality operator.
///
/// @param o the other enum to test against.
///
/// @return true iff @p o is equals the current instance of enum type
/// decl.
bool
enum_type_decl::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Equality operator for @ref enum_type_decl_sptr.
///
/// @param l the first operand to compare.
///
/// @param r the second operand to compare.
///
/// @return true iff @p l equals @p r.
bool
operator==(const enum_type_decl_sptr& l, const enum_type_decl_sptr& r)
{
  if (!!l != !!r)
    return false;
  if (l.get() == r.get())
    return true;
  decl_base_sptr o = r;
  return *l == *o;
}

/// Inequality operator for @ref enum_type_decl_sptr.
///
/// @param l the first operand to compare.
///
/// @param r the second operand to compare.
///
/// @return true iff @p l equals @p r.
bool
operator!=(const enum_type_decl_sptr& l, const enum_type_decl_sptr& r)
{return !operator==(l, r);}

/// The type of the private data of an @ref
/// enum_type_decl::enumerator.
class enum_type_decl::enumerator::priv
{
  const environment*	env_;
  interned_string	name_;
  int64_t		value_;
  interned_string	qualified_name_;
  enum_type_decl*	enum_type_;

  friend class	enum_type_decl::enumerator;

public:

  priv()
    : env_(),
      enum_type_()
  {}

  priv(const environment* env,
       const string& name,
       int64_t value,
       enum_type_decl* e = 0)
    : env_(env),
      name_(env ? env->intern(name) : interned_string()),
      value_(value),
      enum_type_(e)
  {}
}; // end class enum_type_def::enumerator::priv

/// Default constructor of the @ref enum_type_decl::enumerator type.
enum_type_decl::enumerator::enumerator()
  : priv_(new priv)
{}

/// Constructor of the @ref enum_type_decl::enumerator type.
///
/// @param env the environment we are operating from.
///
/// @param name the name of the enumerator.
///
/// @param value the value of the enumerator.
enum_type_decl::enumerator::enumerator(const environment* env,
				       const string& name,
				       int64_t value)
  : priv_(new priv(env, name, value))
{}

/// Copy constructor of the @ref enum_type_decl::enumerator type.
///
/// @param other enumerator to copy.
enum_type_decl::enumerator::enumerator(const enumerator& other)
  : priv_(new priv(other.get_environment(),
		   other.get_name(),
		   other.get_value(),
		   other.get_enum_type()))
{}

/// Assignment operator of the @ref enum_type_decl::enumerator type.
///
/// @param o
enum_type_decl::enumerator&
enum_type_decl::enumerator::operator=(const enumerator& o)
{
  priv_->env_ = o.get_environment();
  priv_->name_ = o.get_name();
  priv_->value_ = o.get_value();
  priv_->enum_type_ = o.get_enum_type();
  return *this;
}
/// Equality operator
///
/// @param other the enumerator to compare to the current instance of
/// enum_type_decl::enumerator.
///
/// @return true if @p other equals the current instance of
/// enum_type_decl::enumerator.
bool
enum_type_decl::enumerator::operator==(const enumerator& other) const
{return (get_name() == other.get_name()
	 && get_value() == other.get_value());}

/// Inequality operator.
///
/// @param other the other instance to compare against.
///
/// @return true iff @p other is different from the current instance.
bool
enum_type_decl::enumerator::operator!=(const enumerator& other) const
{return !operator==(other);}

/// Getter of the environment of this enumerator.
///
/// @return the environment of this enumerator.
const environment*
enum_type_decl::enumerator::get_environment() const
{return priv_->env_;}

/// Getter for the name of the current instance of
/// enum_type_decl::enumerator.
///
/// @return a reference to the name of the current instance of
/// enum_type_decl::enumerator.
const interned_string&
enum_type_decl::enumerator::get_name() const
{return priv_->name_;}

/// Getter for the qualified name of the current instance of
/// enum_type_decl::enumerator.  The first invocation of the method
/// builds the qualified name, caches it and return a reference to the
/// cached qualified name.  Subsequent invocations just return the
/// cached value.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the qualified name of the current instance of
/// enum_type_decl::enumerator.
const interned_string&
enum_type_decl::enumerator::get_qualified_name(bool internal) const
{
  if (priv_->qualified_name_.empty())
    {
      const environment* env = priv_->enum_type_->get_environment();
      ABG_ASSERT(env);
      priv_->qualified_name_ =
	env->intern(get_enum_type()->get_qualified_name(internal)
		    + "::"
		    + get_name());
    }
  return priv_->qualified_name_;
}

/// Setter for the name of @ref enum_type_decl::enumerator.
///
/// @param n the new name.
void
enum_type_decl::enumerator::set_name(const string& n)
{
  const environment* env = get_environment();
  ABG_ASSERT(env);
  priv_->name_ = env->intern(n);
}

/// Getter for the value of @ref enum_type_decl::enumerator.
///
/// @return the value of the current instance of
/// enum_type_decl::enumerator.
int64_t
enum_type_decl::enumerator::get_value() const
{return priv_->value_;}

/// Setter for the value of @ref enum_type_decl::enumerator.
///
/// @param v the new value of the enum_type_decl::enumerator.
void
enum_type_decl::enumerator::set_value(int64_t v)
{priv_->value_= v;}

/// Getter for the enum type that this enumerator is for.
///
/// @return the enum type that this enumerator is for.
enum_type_decl*
enum_type_decl::enumerator::get_enum_type() const
{return priv_->enum_type_;}

/// Setter for the enum type that this enumerator is for.
///
/// @param e the new enum type.
void
enum_type_decl::enumerator::set_enum_type(enum_type_decl* e)
{priv_->enum_type_ = e;}
// </enum_type_decl definitions>

// <typedef_decl definitions>

/// Private data structure of the @ref typedef_decl.
struct typedef_decl::priv
{
  type_base_wptr	underlying_type_;
  string		internal_qualified_name_;
  string		temp_internal_qualified_name_;

  priv(const type_base_sptr& t)
    : underlying_type_(t)
  {}
}; // end struct typedef_decl::priv

/// Constructor of the typedef_decl type.
///
/// @param name the name of the typedef.
///
/// @param underlying_type the underlying type of the typedef.
///
/// @param locus the source location of the typedef declaration.
///
/// @param linkage_name the mangled name of the typedef.
///
/// @param vis the visibility of the typedef type.
typedef_decl::typedef_decl(const string&		name,
			   const type_base_sptr	underlying_type,
			   const location&		locus,
			   const string&		linkage_name,
			   visibility vis)
  : type_or_decl_base(underlying_type->get_environment(),
		      TYPEDEF_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    type_base(underlying_type->get_environment(),
	      underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(underlying_type->get_environment(),
	      name, locus, linkage_name, vis),
    priv_(new priv(underlying_type))
{
  runtime_type_instance(this);
}

/// Return the size of the typedef.
///
/// This function looks at the size of the underlying type and ensures
/// that it's the same as the size of the typedef.
///
/// @return the size of the typedef.
size_t
typedef_decl::get_size_in_bits() const
{
  size_t s = get_underlying_type()->get_size_in_bits();
  if (s != type_base::get_size_in_bits())
    const_cast<typedef_decl*>(this)->set_size_in_bits(s);
  return type_base::get_size_in_bits();
}

/// Return the alignment of the typedef.
///
/// This function looks at the alignment of the underlying type and
/// ensures that it's the same as the alignment of the typedef.
///
/// @return the size of the typedef.
size_t
typedef_decl::get_alignment_in_bits() const
{
    size_t s = get_underlying_type()->get_alignment_in_bits();
  if (s != type_base::get_alignment_in_bits())
    const_cast<typedef_decl*>(this)->set_alignment_in_bits(s);
  return type_base::get_alignment_in_bits();
}

/// Compares two instances of @ref typedef_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const typedef_decl& l, const typedef_decl& r, change_kind* k)
{
  bool result = true;
  // Compare the properties of the 'is-a-member-decl" relation of this
  // decl.  For typedefs of a C program, this always return true as
  // there is no "member typedef type" in C.
  //
  // In other words, in C, Only the underlying types of typedefs are
  // compared.  In C++ however, the properties of the
  // 'is-a-member-decl' relation of the typedef are compared.
  if (!maybe_compare_as_member_decls(l, r, k))
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }

  if (*l.get_underlying_type() != *r.get_underlying_type())
    {
      // Changes to the underlying type of a typedef are considered
      // local, a bit like for pointers.
      result = false;
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

/// Equality operator
///
/// @param o the other typedef_decl to test against.
bool
typedef_decl::operator==(const decl_base& o) const
{
  const typedef_decl* other = dynamic_cast<const typedef_decl*>(&o);
  if (!other)
    return false;
  return try_canonical_compare(this, other);
}

/// Equality operator
///
/// @param o the other typedef_decl to test against.
///
/// @return true if the current instance of @ref typedef_decl equals
/// @p o.
bool
typedef_decl::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Build a pretty representation for a typedef_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qualified_name if true, names emitted in the pretty
/// representation are fully qualified.
///
/// @return a copy of the pretty representation of the current
/// instance of typedef_decl.
string
typedef_decl::get_pretty_representation(bool internal,
					bool qualified_name) const
{

  string result = "typedef ";
  if (qualified_name)
    result += get_qualified_name(internal);
  else
    result += get_name();

  return result;
}

/// Getter of the underlying type of the typedef.
///
/// @return the underlying_type.
type_base_sptr
typedef_decl::get_underlying_type() const
{return priv_->underlying_type_.lock();}

/// Setter ofthe underlying type of the typedef.
///
/// @param t the new underlying type of the typedef.
void
typedef_decl::set_underlying_type(const type_base_sptr& t)
{priv_->underlying_type_ = t;}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
typedef_decl::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_underlying_type())
	t->traverse(v);
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

typedef_decl::~typedef_decl()
{}
// </typedef_decl definitions>

// <var_decl definitions>

struct var_decl::priv
{
  type_base_wptr	type_;
  type_base*		naked_type_;
  decl_base::binding	binding_;
  elf_symbol_sptr	symbol_;
  interned_string	id_;

  priv()
    : naked_type_(),
    binding_(decl_base::BINDING_GLOBAL)
  {}

  priv(type_base_sptr t,
       decl_base::binding b)
    : type_(t),
      naked_type_(t.get()),
      binding_(b)
  {}
}; // end struct var_decl::priv

/// Constructor
///
/// @param name the name of the variable declaration
///
/// @param name the type of the variable declaration
///
/// @param locus the source location where the variable was defined.
///
/// @param linkage_name the linkage name of the variable.
///
/// @param vis the visibility of of the variable.
///
/// @param bind the binding kind of the variable.
var_decl::var_decl(const string&	name,
		   type_base_sptr	type,
		   const location&	locus,
		   const string&	linkage_name,
		   visibility		vis,
		   binding		bind)
  : type_or_decl_base(type->get_environment(),
		      VAR_DECL | ABSTRACT_DECL_BASE),
    decl_base(type->get_environment(), name, locus, linkage_name, vis),
    priv_(new priv(type, bind))
{
  runtime_type_instance(this);
}

/// Getter of the type of the variable.
///
/// @return the type of the variable.
const type_base_sptr
var_decl::get_type() const
{return priv_->type_.lock();}

/// Getter of the type of the variable.
///
/// This getter returns a bare pointer, as opposed to a smart pointer.
/// It's to be used on performance sensitive code paths identified by
/// careful profiling.
///
/// @return the type of the variable, as a bare pointer.
const type_base*
var_decl::get_naked_type() const
{return priv_->naked_type_;}

/// Getter of the binding of the variable.
///
/// @return the biding of the variable.
decl_base::binding
var_decl::get_binding() const
{return priv_->binding_;}

/// Setter of the binding of the variable.
///
/// @param b the new binding value.
void
var_decl::set_binding(decl_base::binding b)
{priv_->binding_ = b;}

/// Sets the underlying ELF symbol for the current variable.
///
/// And underlyin$g ELF symbol for the current variable might exist
/// only if the corpus that this variable originates from was
/// constructed from an ELF binary file.
///
/// Note that comparing two variables that have underlying ELF symbols
/// involves comparing their underlying elf symbols.  The decl name
/// for the variable thus becomes irrelevant in the comparison.
///
/// @param sym the new ELF symbol for this variable decl.
void
var_decl::set_symbol(const elf_symbol_sptr& sym)
{
  priv_->symbol_ = sym;
  // The variable id cache that depends on the symbol must be
  // invalidated because the symbol changed.
  priv_->id_ = get_environment()->intern("");
}

/// Gets the the underlying ELF symbol for the current variable,
/// that was set using var_decl::set_symbol().  Please read the
/// documentation for that member function for more information about
/// "underlying ELF symbols".
///
/// @return sym the underlying ELF symbol for this variable decl, if
/// one exists.
const elf_symbol_sptr&
var_decl::get_symbol() const
{return priv_->symbol_;}

/// Create a new var_decl that is a clone of the current one.
///
/// @return the cloned var_decl.
var_decl_sptr
var_decl::clone() const
{
  var_decl_sptr v(new var_decl(get_name(),
			       get_type(),
			       get_location(),
			       get_linkage_name(),
			       get_visibility(),
			       get_binding()));

  v->set_symbol(get_symbol());

  if (is_member_decl(*this))
    {
      class_decl* scope = dynamic_cast<class_decl*>(get_scope());
      scope->add_data_member(v, get_member_access_specifier(*this),
			     get_data_member_is_laid_out(*this),
			     get_member_is_static(*this),
			     get_data_member_offset(*this));
    }
  else
    add_decl_to_scope(v, get_scope());

  return v;
}
/// Setter of the scope of the current var_decl.
///
/// Note that the decl won't hold a reference on the scope.  It's
/// rather the scope that holds a reference on its members.
///
/// @param scope the new scope.
void
var_decl::set_scope(scope_decl* scope)
{
  if (!get_context_rel())
    set_context_rel(new dm_context_rel(scope));
  else
    get_context_rel()->set_scope(scope);
}

/// Compares two instances of @ref var_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const var_decl& l, const var_decl& r, change_kind* k)
{
  bool result = true;

  // First test types of variables.  This should be fast because in
  // the general case, most types should be canonicalized.
  if (*l.get_naked_type() != *r.get_naked_type())
    {
      result = false;
      if (k)
	{
	  if (!types_have_similar_structure(l.get_naked_type(),
					   r.get_naked_type()))
	    *k |= (LOCAL_TYPE_CHANGE_KIND);
	  else
	    *k |= SUBTYPE_CHANGE_KIND;
	}
      else
	return false;
    }

  // If there are underlying elf symbols for these variables,
  // compare them.  And then compare the other parts.
  const elf_symbol_sptr &s0 = l.get_symbol(), &s1 = r.get_symbol();
  if (!!s0 != !!s1)
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }
  else if (s0 && s0 != s1)
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }
  bool symbols_are_equal = (s0 && s1 && result);

  if (symbols_are_equal)
    {
      // The variables have underlying elf symbols that are equal, so
      // now, let's compare the decl_base part of the variables w/o
      // considering their decl names.
      const interned_string &n1 = l.get_name(), &n2 = r.get_name();
      const_cast<var_decl&>(l).set_name("");
      const_cast<var_decl&>(r).set_name("");
      bool decl_bases_different = !l.decl_base::operator==(r);
      const_cast<var_decl&>(l).set_name(n1);
      const_cast<var_decl&>(r).set_name(n2);

      if (decl_bases_different)
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	  else
	    return false;
	}
    }
  else
    if (!l.decl_base::operator==(r))
      {
	result = false;
	if (k)
	  *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	else
	  return false;
      }

  const dm_context_rel* c0 =
    dynamic_cast<const dm_context_rel*>(l.get_context_rel());
  const dm_context_rel* c1 =
    dynamic_cast<const dm_context_rel*>(r.get_context_rel());
  ABG_ASSERT(c0 && c1);

  if (*c0 != *c1)
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

/// Comparison operator of @ref var_decl.
///
/// @param o the instance of @ref var_decl to compare against.
///
/// @return true iff the current instance of @ref var_decl equals @p o.
bool
var_decl::operator==(const decl_base& o) const
{
  const var_decl* other = dynamic_cast<const var_decl*>(&o);
  if (!other)
    return false;

  return equals(*this, *other, 0);
}

/// Return an ID that tries to uniquely identify the variable inside a
/// program or a library.
///
/// So if the variable has an underlying elf symbol, the ID is the
/// concatenation of the symbol name and its version.  Otherwise, the
/// ID is the linkage name if its non-null.  Otherwise, it's the
/// pretty representation of the variable.
///
/// @return the ID.
interned_string
var_decl::get_id() const
{
  if (priv_->id_.empty())
    {
      string repr = get_name();
      string sym_str;
      if (elf_symbol_sptr s = get_symbol())
	sym_str = s->get_id_string();
      else if (!get_linkage_name().empty())
	sym_str = get_linkage_name();
      const environment* env = get_type()->get_environment();
      ABG_ASSERT(env);
      priv_->id_ = env->intern(repr);
      if (!sym_str.empty())
	priv_->id_ = env->intern(priv_->id_ + "{" + sym_str + "}");
    }
  return priv_->id_;
}

/// Return the hash value for the current instance.
///
/// @return the hash value.
size_t
var_decl::get_hash() const
{
  var_decl::hash hash_var;
  return hash_var(this);
}

/// Get the qualified name of a given variable or data member.
///
///
/// Note that if the current instance of @ref var_decl is an anonymous
/// data member, then the qualified name is actually the flat
/// representation (the definition) of the type of the anonymous data
/// member.  We chose the flat representation because otherwise, the
/// name of an *anonymous* data member is empty, by construction, e.g:
///
///   struct foo {
///     int a;
///     union {
///       char b;
///       char c;
///     }; // <---- this data member is anonymous.
///     int d;
///   }
///
///   The string returned for the anonymous member here is going to be:
///
///     "union {char b; char c}"
///
/// @param internal if true then this is for a purpose to the library,
/// otherwise, it's for being displayed to users.
///
/// @return the resulting qualified name.
const interned_string&
var_decl::get_qualified_name(bool internal) const
{
  if (is_anonymous_data_member(this)
      && decl_base::get_qualified_name().empty())
    {
      // Display the anonymous data member in a way that makes sense.
      string r = get_pretty_representation(internal);
      set_qualified_name(get_environment()->intern(r));
    }

    return decl_base::get_qualified_name(internal);
}

/// Build and return the pretty representation of this variable.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qualified_name if true, names emitted in the pretty
/// representation are fully qualified.
///
/// @return a copy of the pretty representation of this variable.
string
var_decl::get_pretty_representation(bool internal, bool qualified_name) const
{
  string result;

  if (is_member_decl(this) && get_member_is_static(this))
    result = "static ";

  // Detect if the current instance of var_decl is a member of
  // an anonymous class or union.
  bool member_of_anonymous_class = false;
  if (class_or_union* scope = is_at_class_scope(this))
    if (scope->get_is_anonymous())
      member_of_anonymous_class = true;

  if (array_type_def_sptr t = is_array_type(get_type()))
    {
      string name;
      if (member_of_anonymous_class || !qualified_name)
	name = get_name();
      else
	name = get_qualified_name(internal);

      result +=
	get_type_declaration(t->get_element_type())->get_qualified_name(internal)
	+ " " + name + t->get_subrange_representation();
    }
  else
    {
      if (/*The current var_decl is to be used as an anonymous data
	    member.  */
	  get_name().empty())
	{
	  // Display the anonymous data member in a way that
	  // makes sense.
	  result +=
	    get_class_or_union_flat_representation
	    (is_class_or_union_type(get_type()),
	     "", /*one_line=*/true, internal);
	}
      else if (data_member_has_anonymous_type(this))
	{
	  result += get_class_or_union_flat_representation
	    (is_class_or_union_type(get_type()),
	     "", /*one_line=*/true, internal);
	  result += " ";
	  if (member_of_anonymous_class || !qualified_name)
	    // It doesn't make sense to name the member of an
	    // anonymous class or union like:
	    // "__anonymous__::data_member_name".  So let's just use
	    // its non-qualified name.
	    result += get_name();
	  else
	    result += get_qualified_name(internal);
	}
      else
	{
	  result +=
	    get_type_declaration(get_type())->get_qualified_name(internal)
	    + " ";

	  if (member_of_anonymous_class || !qualified_name)
	    // It doesn't make sense to name the member of an
	    // anonymous class or union like:
	    // "__anonymous__::data_member_name".  So let's just use
	    // its non-qualified name.
	    result += get_name();
	  else
	    result += get_qualified_name(internal);
	}
    }
  return result;
}

/// Get a name that is valid even for an anonymous data member.
///
/// If the current @ref var_decl is an anonymous data member, then
/// return its pretty representation. As of now, that pretty
/// representation is actually its flat representation as returned by
/// get_class_or_union_flat_representation().
///
/// Otherwise, just return the name of the current @ref var_decl.
///
/// @param qualified if true, return the qualified name.  This doesn't
/// have an effet if the current @ref var_decl represents an anonymous
/// data member.
string
var_decl::get_anon_dm_reliable_name(bool qualified) const
{
  string name;
  if (is_anonymous_data_member(this))
    name = get_pretty_representation(true, qualified);
  else
    name = get_name();

  return name;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
var_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

var_decl::~var_decl()
{}

// </var_decl definitions>

// <function_type>

/// The type of the private data of the @ref function_type type.
struct function_type::priv
{
  parameters parms_;
  type_base_wptr return_type_;
  interned_string cached_name_;
  interned_string internal_cached_name_;

  priv()
  {}

  priv(const parameters&	parms,
       type_base_sptr		return_type)
    : parms_(parms),
      return_type_(return_type)
  {}

  priv(type_base_sptr return_type)
    : return_type_(return_type)
  {}

  /// Mark a given @ref function_type as being compared.
  ///
  /// @param type the @ref function_type to mark as being compared.
  void
  mark_as_being_compared(const function_type& type) const
  {
    const environment* env = type.get_environment();
    ABG_ASSERT(env);
    env->priv_->fn_types_being_compared_.insert(&type);
  }

  /// If a given @ref function_type was marked as being compared, this
  /// function unmarks it.
  ///
  /// @param type the @ref function_type to mark as *NOT* being
  /// compared.
  void
  unmark_as_being_compared(const function_type& type) const
  {
    const environment* env = type.get_environment();
    ABG_ASSERT(env);
    env->priv_->fn_types_being_compared_.erase(&type);
  }

  /// Tests if a @ref function_type is currently being compared.
  ///
  /// @param type the function type to take into account.
  ///
  /// @return true if @p type is being compared.
  bool
  comparison_started(const function_type& type) const
  {
    const environment* env = type.get_environment();
    ABG_ASSERT(env);
    return env->priv_->fn_types_being_compared_.count(&type);
  }
};// end struc function_type::priv

/// This function is automatically invoked whenever an instance of
/// this type is canonicalized.
///
/// It's an overload of the virtual type_base::on_canonical_type_set.
///
/// We put here what is thus meant to be executed only at the point of
/// type canonicalization.
void
function_type::on_canonical_type_set()
{
  priv_->cached_name_.clear();
  priv_->internal_cached_name_.clear();
}

/// The most straightforward constructor for the function_type class.
///
/// @param return_type the return type of the function type.
///
/// @param parms the list of parameters of the function type.
/// Stricto sensu, we just need a list of types; we are using a list
/// of parameters (where each parameter also carries the name of the
/// parameter and its source location) to try and provide better
/// diagnostics whenever it makes sense.  If it appears that this
/// wasts too many resources, we can fall back to taking just a
/// vector of types here.
///
/// @param size_in_bits the size of this type, in bits.
///
/// @param alignment_in_bits the alignment of this type, in bits.
///
/// @param size_in_bits the size of this type.
function_type::function_type(type_base_sptr	return_type,
			     const parameters&	parms,
			     size_t		size_in_bits,
			     size_t		alignment_in_bits)
  : type_or_decl_base(return_type->get_environment(),
		      FUNCTION_TYPE | ABSTRACT_TYPE_BASE),
    type_base(return_type->get_environment(), size_in_bits, alignment_in_bits),
    priv_(new priv(parms, return_type))
{
  runtime_type_instance(this);

  for (parameters::size_type i = 0, j = 1;
       i < priv_->parms_.size();
       ++i, ++j)
    {
      if (i == 0 && priv_->parms_[i]->get_is_artificial())
	// If the first parameter is artificial, then it certainly
	// means that this is a member function, and the first
	// parameter is the implicit this pointer.  In that case, set
	// the index of that implicit parameter to zero.  Otherwise,
	// the index of the first parameter starts at one.
	j = 0;
      priv_->parms_[i]->set_index(j);
    }
}

/// A constructor for a function_type that takes no parameters.
///
/// @param return_type the return type of this function_type.
///
/// @param size_in_bits the size of this type, in bits.
///
/// @param alignment_in_bits the alignment of this type, in bits.
function_type::function_type(type_base_sptr return_type,
			     size_t size_in_bits, size_t alignment_in_bits)
  : type_or_decl_base(return_type->get_environment(),
		      FUNCTION_TYPE | ABSTRACT_TYPE_BASE),
    type_base(return_type->get_environment(), size_in_bits, alignment_in_bits),
    priv_(new priv(return_type))
{
  runtime_type_instance(this);
}

/// A constructor for a function_type that takes no parameter and
/// that has no return_type yet.  These missing parts can (and must)
/// be added later.
///
/// @param env the environment we are operating from.
///
/// @param size_in_bits the size of this type, in bits.
///
/// @param alignment_in_bits the alignment of this type, in bits.
function_type::function_type(const environment* env,
			     size_t		size_in_bits,
			     size_t		alignment_in_bits)
  : type_or_decl_base(env, FUNCTION_TYPE | ABSTRACT_TYPE_BASE),
    type_base(env, size_in_bits, alignment_in_bits),
    priv_(new priv)
{
  runtime_type_instance(this);
}

/// Getter for the return type of the current instance of @ref
/// function_type.
///
/// @return the return type.
type_base_sptr
function_type::get_return_type() const
{return priv_->return_type_.lock();}

/// Setter of the return type of the current instance of @ref
/// function_type.
///
/// @param t the new return type to set.
void
function_type::set_return_type(type_base_sptr t)
{priv_->return_type_ = t;}

/// Getter for the set of parameters of the current intance of @ref
/// function_type.
///
/// @return the parameters of the current instance of @ref
/// function_type.
const function_decl::parameters&
function_type::get_parameters() const
{return priv_->parms_;}

/// Get the Ith parameter of the vector of parameters of the current
/// instance of @ref function_type.
///
/// Note that the first parameter is at index 0.  That parameter is
/// the first parameter that comes after the possible implicit "this"
/// parameter, when the current instance @ref function_type is for a
/// member function.  Otherwise, if the current instance of @ref
/// function_type is for a non-member function, the parameter at index
/// 0 is the first parameter of the function.
///
///
/// @param i the index of the parameter to return.  If i is greater
/// than the index of the last parameter, then this function returns
/// an empty parameter (smart) pointer.
///
/// @return the @p i th parameter that is not implicit.
const function_decl::parameter_sptr
function_type::get_parm_at_index_from_first_non_implicit_parm(size_t i) const
{
  parameter_sptr result;
  if (dynamic_cast<const method_type*>(this))
    {
      if (i + 1 < get_parameters().size())
	result = get_parameters()[i + 1];
    }
  else
    {
      if (i < get_parameters().size())
	result = get_parameters()[i];
    }
  return result;
}

/// Setter for the parameters of the current instance of @ref
/// function_type.
///
/// @param p the new vector of parameters to set.
void
function_type::set_parameters(const parameters &p)
{
  priv_->parms_ = p;
  for (parameters::size_type i = 0, j = 1;
       i < priv_->parms_.size();
       ++i, ++j)
    {
      if (i == 0 && priv_->parms_[i]->get_is_artificial())
	// If the first parameter is artificial, then it certainly
	// means that this is a member function, and the first
	// parameter is the implicit this pointer.  In that case, set
	// the index of that implicit parameter to zero.  Otherwise,
	// the index of the first parameter starts at one.
	j = 0;
      priv_->parms_[i]->set_index(j);
    }
}

/// Append a new parameter to the vector of parameters of the current
/// instance of @ref function_type.
///
/// @param parm the parameter to append.
void
function_type::append_parameter(parameter_sptr parm)
{
  parm->set_index(priv_->parms_.size());
  priv_->parms_.push_back(parm);
}

/// Test if the current instance of @ref function_type is for a
/// variadic function.
///
/// A variadic function is a function that takes a variable number of
/// arguments.
///
/// @return true iff the current instance of @ref function_type is for
/// a variadic function.
bool
function_type::is_variadic() const
{
  return (!priv_->parms_.empty()
	 && priv_->parms_.back()->get_variadic_marker());
}

/// Compare two function types.
///
/// In case these function types are actually method types, this
/// function avoids comparing two parameters (of the function types)
/// if the types of the parameters are actually the types of the
/// classes of the method types.  This prevents infinite recursion
/// during the comparison of two classes that are structurally
/// identical.
///
/// This is a subroutine of the equality operator of function_type.
///
/// @param lhs the first function type to consider
///
/// @param rhs the second function type to consider
///
/// @param k a pointer to a bitfield set by the function to give
/// information about the kind of changes carried by @p lhs and @p
/// rhs.  It is set iff @p k is non-null and the function returns
/// false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
///@return true if lhs == rhs, false otherwise.
bool
equals(const function_type& lhs,
       const function_type& rhs,
       change_kind* k)
{
#define RETURN(value)				\
  do {						\
    lhs.priv_->unmark_as_being_compared(lhs);	\
    lhs.priv_->unmark_as_being_compared(rhs);	\
    if (value == true)				\
      maybe_propagate_canonical_type(lhs, rhs); \
    return value;				\
  } while(0)

  if (lhs.priv_->comparison_started(lhs)
      || lhs.priv_->comparison_started(rhs))
    return true;

  lhs.priv_->mark_as_being_compared(lhs);
  lhs.priv_->mark_as_being_compared(rhs);

  bool result = true;

  if (!lhs.type_base::operator==(rhs))
    {
      result = false;
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      else
	RETURN(result);
    }

  class_or_union* lhs_class = 0, *rhs_class = 0;
  if (const method_type* m = dynamic_cast<const method_type*>(&lhs))
    lhs_class = m->get_class_type().get();

  if (const method_type* m = dynamic_cast<const method_type*>(&rhs))
    rhs_class = m->get_class_type().get();

  // Compare the names of the class of the method

  if (!!lhs_class != !!rhs_class)
    {
      result = false;
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      else
	RETURN(result);
    }
  else if (lhs_class
	   && (lhs_class->get_qualified_name()
	       != rhs_class->get_qualified_name()))
    {
      result = false;
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      else
	RETURN(result);
    }

  // Then compare the return type; Beware if it's t's a class type
  // that is the same as the method class name; we can recurse for
  // ever in that case.

  decl_base* lhs_return_type_decl =
    get_type_declaration(lhs.get_return_type()).get();
  decl_base* rhs_return_type_decl =
    get_type_declaration(rhs.get_return_type()).get();
  bool compare_result_types = true;
  string lhs_rt_name = lhs_return_type_decl
    ? lhs_return_type_decl->get_qualified_name()
    : string();
  string rhs_rt_name = rhs_return_type_decl
    ? rhs_return_type_decl->get_qualified_name()
    : string();

  if ((lhs_class && (lhs_class->get_qualified_name() == lhs_rt_name))
      ||
      (rhs_class && (rhs_class->get_qualified_name() == rhs_rt_name)))
    compare_result_types = false;

  if (compare_result_types)
    {
      if (lhs.get_return_type() != rhs.get_return_type())
	{
	  result = false;
	  if (k)
	    {
	      if (!types_have_similar_structure(lhs.get_return_type(),
						rhs.get_return_type()))
		*k |= LOCAL_TYPE_CHANGE_KIND;
	      else
		*k |= SUBTYPE_CHANGE_KIND;
	    }
	  else
	    RETURN(result);
	}
    }
  else
    if (lhs_rt_name != rhs_rt_name)
      {
	result = false;
	if (k)
	  *k |= SUBTYPE_CHANGE_KIND;
	else
	  RETURN(result);
      }

  vector<shared_ptr<function_decl::parameter> >::const_iterator i,j;
  for (i = lhs.get_first_parm(), j = rhs.get_first_parm();
       i != lhs.get_parameters().end() && j != rhs.get_parameters().end();
       ++i, ++j)
    {
      if (**i != **j)
	{
	  result = false;
	  if (k)
	    {
	      if (!types_have_similar_structure((*i)->get_type(),
						(*j)->get_type()))
		*k |= LOCAL_TYPE_CHANGE_KIND;
	      else
		*k |= SUBTYPE_CHANGE_KIND;
	    }
	  else
	    RETURN(result);
	}
    }

  if ((i != lhs.get_parameters().end()
       || j != rhs.get_parameters().end()))
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	RETURN(result);
    }

  RETURN(result);
#undef RETURN
}

/// Get the first parameter of the function.
///
/// If the function is a non-static member function, the parameter
/// returned is the first one following the implicit 'this' parameter.
///
/// @return the first non implicit parameter of the function.
function_type::parameters::const_iterator
function_type::get_first_non_implicit_parm() const
{
  if (get_parameters().empty())
    return get_parameters().end();

  bool is_method = dynamic_cast<const method_type*>(this);

  parameters::const_iterator i = get_parameters().begin();

  if (is_method)
    ++i;

  return i;
}

/// Get the first parameter of the function.
///
/// Note that if the function is a non-static member function, the
/// parameter returned is the implicit 'this' parameter.
///
/// @return the first parameter of the function.
function_type::parameters::const_iterator
function_type::get_first_parm() const
{return get_parameters().begin();}

/// Get the name of the current @ref function_type.
///
/// The name is retrieved from a cache.  If the cache is empty, this
/// function computes the name of the type, stores it in the cache and
/// returns it.  Subsequent invocation of the function are going to
/// just hit the cache.
///
/// Note that if the type is *NOT* canonicalized then function type
/// name is never cached.
///
/// @param internal if true then it means the function type name is
/// going to be used for purposes that are internal to libabigail
/// itself.  If you don't know what this is then you probably should
/// set this parameter to 'false'.
///
/// @return the name of the function type.
const interned_string&
function_type::get_cached_name(bool internal) const
{
  if (internal)
    {
      if (!get_naked_canonical_type() || priv_->internal_cached_name_.empty())
	priv_->internal_cached_name_ = get_function_type_name(this, internal);

      return priv_->internal_cached_name_;
    }

  if (!get_naked_canonical_type() || priv_->cached_name_.empty())
    priv_->cached_name_ = get_function_type_name(this, internal);

  return priv_->cached_name_;
}

/// Equality operator for function_type.
///
/// @param o the other function_type to compare against.
///
/// @return true iff the two function_type are equal.
bool
function_type::operator==(const type_base& other) const
{
  const function_type* o = dynamic_cast<const function_type*>(&other);
  if (!o)
    return false;
  return try_canonical_compare(this, o);
}

/// Return a copy of the pretty representation of the current @ref
/// function_type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of the current @ref
/// function_type.
string
function_type::get_pretty_representation(bool internal,
					 bool /*qualified_name*/) const
{return ir::get_pretty_representation(this, internal);}

/// Traverses an instance of @ref function_type, visiting all the
/// sub-types and decls that it might contain.
///
/// @param v the visitor that is used to visit every IR sub-node of
/// the current node.
///
/// @return true if either
///  - all the children nodes of the current IR node were traversed
///    and the calling code should keep going with the traversing.
///  - or the current IR node is already being traversed.
/// Otherwise, returning false means that the calling code should not
/// keep traversing the tree.
bool
function_type::traverse(ir_node_visitor& v)
{
  // TODO: should we allow the walker to avoid visiting function type
  // twice?  I think that if we do, then ir_node_visitor needs an
  // option to specifically disallow this feature for function types.

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      bool keep_going = true;

      if (type_base_sptr t = get_return_type())
	{
	  if (!t->traverse(v))
	    keep_going = false;
	}

      if (keep_going)
	for (parameters::const_iterator i = get_parameters().begin();
	     i != get_parameters().end();
	     ++i)
	  if (type_base_sptr parm_type = (*i)->get_type())
	    if (!parm_type->traverse(v))
	      break;

      visiting(false);
    }
  return v.visit_end(this);
}

function_type::~function_type()
{}
// </function_type>

// <method_type>

struct method_type::priv
{
  class_or_union_wptr class_type_;
  bool is_const;

  priv()
    : is_const()
  {}
}; // end struct method_type::priv

/// Constructor for instances of method_type.
///
/// Instances of method_decl must be of type method_type.
///
/// @param return_type the type of the return value of the method.
///
/// @param class_type the base type of the method type.  That is, the
/// type of the class the method belongs to.
///
/// @param p the vector of the parameters of the method.
///
/// @param is_const whether this method type is for a const method.
/// Note that const-ness is a property of the method *type* and of the
/// relationship between a method *declaration* and its scope.
///
/// @param size_in_bits the size of an instance of method_type,
/// expressed in bits.
///
/// @param alignment_in_bits the alignment of an instance of
/// method_type, expressed in bits.
method_type::method_type (type_base_sptr return_type,
			  class_or_union_sptr class_type,
			  const std::vector<function_decl::parameter_sptr>& p,
			  bool is_const,
			  size_t size_in_bits,
			  size_t alignment_in_bits)
  : type_or_decl_base(class_type->get_environment(),
		      METHOD_TYPE | ABSTRACT_TYPE_BASE | FUNCTION_TYPE),
    type_base(class_type->get_environment(), size_in_bits, alignment_in_bits),
    function_type(return_type, p, size_in_bits, alignment_in_bits),
    priv_(new priv)
{
  runtime_type_instance(this);
  set_class_type(class_type);
  set_is_const(is_const);
}

/// Constructor of instances of method_type.
///
///Instances of method_decl must be of type method_type.
///
/// @param return_type the type of the return value of the method.
///
/// @param class_type the type of the class the method belongs to.
/// The actual (dynamic) type of class_type must be a pointer
/// class_type.  We are setting it to pointer to type_base here to
/// help client code that is compiled without rtti and thus cannot
/// perform dynamic casts.
///
/// @param p the vector of the parameters of the method type.
///
/// @param is_const whether this method type is for a const method.
/// Note that const-ness is a property of the method *type* and of the
/// relationship between a method *declaration* and its scope.
///
/// @param size_in_bits the size of an instance of method_type,
/// expressed in bits.
///
/// @param alignment_in_bits the alignment of an instance of
/// method_type, expressed in bits.
method_type::method_type(type_base_sptr return_type,
			 type_base_sptr class_type,
			 const std::vector<function_decl::parameter_sptr>& p,
			 bool is_const,
			 size_t size_in_bits,
			 size_t alignment_in_bits)
  : type_or_decl_base(class_type->get_environment(),
		      METHOD_TYPE | ABSTRACT_TYPE_BASE | FUNCTION_TYPE),
    type_base(class_type->get_environment(), size_in_bits, alignment_in_bits),
    function_type(return_type, p, size_in_bits, alignment_in_bits),
    priv_(new priv)
{
  runtime_type_instance(this);
  set_class_type(is_class_type(class_type));
  set_is_const(is_const);
}

/// Constructor of the qualified_type_def
///
/// @param env the environment we are operating from.
///
/// @param size_in_bits the size of the type, expressed in bits.
///
/// @param alignment_in_bits the alignment of the type, expressed in bits
method_type::method_type(const environment*	env,
			 size_t		size_in_bits,
			 size_t		alignment_in_bits)
  : type_or_decl_base(env, METHOD_TYPE | ABSTRACT_TYPE_BASE | FUNCTION_TYPE),
    type_base(env, size_in_bits, alignment_in_bits),
    function_type(env, size_in_bits, alignment_in_bits),
    priv_(new priv)
{
  runtime_type_instance(this);
}

/// Constructor of instances of method_type.
///
/// When constructed with this constructor, and instane of method_type
/// must set a return type using method_type::set_return_type
///
/// @param class_typ the base type of the method type.  That is, the
/// type of the class (or union) the method belongs to.
///
/// @param size_in_bits the size of an instance of method_type,
/// expressed in bits.
///
/// @param alignment_in_bits the alignment of an instance of
/// method_type, expressed in bits.
method_type::method_type(class_or_union_sptr class_type,
			 bool is_const,
			 size_t size_in_bits,
			 size_t alignment_in_bits)
  : type_or_decl_base(class_type->get_environment(),
		      METHOD_TYPE | ABSTRACT_TYPE_BASE | FUNCTION_TYPE),
    type_base(class_type->get_environment(), size_in_bits, alignment_in_bits),
    function_type(class_type->get_environment(),
		  size_in_bits,
		  alignment_in_bits),
    priv_(new priv)
{
  runtime_type_instance(this);
  set_class_type(class_type);
  set_is_const(is_const);
}

/// Get the class type this method belongs to.
///
/// @return the class type.
class_or_union_sptr
method_type::get_class_type() const
{return class_or_union_sptr(priv_->class_type_);}

/// Sets the class type of the current instance of method_type.
///
/// The class type is the type of the class the method belongs to.
///
/// @param t the new class type to set.
void
method_type::set_class_type(const class_or_union_sptr& t)
{
  if (!t)
    return;

  priv_->class_type_ = t;
}

/// Return a copy of the pretty representation of the current @ref
/// method_type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of the current @ref
/// method_type.
string
method_type::get_pretty_representation(bool internal,
				       bool /*qualified_name*/) const
{return ir::get_pretty_representation(*this, internal);}

/// Setter of the "is-const" property of @ref method_type.
///
/// @param the new value of the "is-const" property.
void
method_type::set_is_const(bool f)
{priv_->is_const = f;}

/// Getter of the "is-const" property of @ref method_type.
///
/// @return true iff the "is-const" property was set.
bool
method_type::get_is_const() const
{return priv_->is_const;}

/// The destructor of method_type
method_type::~method_type()
{}

// </method_type>

// <function_decl definitions>

struct function_decl::priv
{
  bool			declared_inline_;
  decl_base::binding	binding_;
  function_type_wptr	type_;
  function_type*	naked_type_;
  elf_symbol_sptr	symbol_;
  interned_string id_;

  priv()
    : declared_inline_(false),
      binding_(decl_base::BINDING_GLOBAL),
      naked_type_()
  {}

  priv(function_type_sptr t,
       bool declared_inline,
       decl_base::binding binding)
    : declared_inline_(declared_inline),
      binding_(binding),
      type_(t),
      naked_type_(t.get())
  {}

  priv(function_type_sptr t,
       bool declared_inline,
       decl_base::binding binding,
       elf_symbol_sptr s)
    : declared_inline_(declared_inline),
      binding_(binding),
      type_(t),
      naked_type_(t.get()),
      symbol_(s)
  {}
}; // end sruct function_decl::priv

function_decl::function_decl(const string& name,
			     function_type_sptr function_type,
			     bool declared_inline,
			     const location& locus,
			     const string& mangled_name,
			     visibility vis,
			     binding bind)
  : type_or_decl_base(function_type->get_environment(),
		      FUNCTION_DECL | ABSTRACT_DECL_BASE),
    decl_base(function_type->get_environment(), name, locus, mangled_name, vis),
    priv_(new priv(function_type, declared_inline, bind))
{
  runtime_type_instance(this);
}

/// Constructor of the function_decl type.
///
/// This flavour of constructor is for when the pointer to the
/// instance of function_type that the client code has is presented as
/// a pointer to type_base.  In that case, this constructor saves the
/// client code from doing a dynamic_cast to get the function_type
/// pointer.
///
/// @param name the name of the function declaration.
///
/// @param fn_type the type of the function declaration.  The dynamic
/// type of this parameter should be 'pointer to function_type'
///
/// @param declared_inline whether this function was declared inline
///
/// @param locus the source location of the function declaration.
///
/// @param linkage_name the mangled name of the function declaration.
///
/// @param vis the visibility of the function declaration.
///
/// @param bind  the kind of the binding of the function
/// declaration.
function_decl::function_decl(const string&	name,
			     type_base_sptr	fn_type,
			     bool		declared_inline,
			     const location&	locus,
			     const string&	linkage_name,
			     visibility	vis,
			     binding		bind)
  : type_or_decl_base(fn_type->get_environment(),
		      FUNCTION_DECL | ABSTRACT_DECL_BASE),
    decl_base(fn_type->get_environment(), name, locus, linkage_name, vis),
    priv_(new priv(dynamic_pointer_cast<function_type>(fn_type),
		   declared_inline,
		   bind))
{
  runtime_type_instance(this);
}

/// Get the pretty representation of the current instance of @ref function_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation for a function.
string
function_decl::get_pretty_representation(bool internal,
					 bool /*qualified_name*/) const
{
  const method_decl* mem_fn =
    dynamic_cast<const method_decl*>(this);

  string result = mem_fn ? "method ": "function ";

  if (mem_fn
      && is_member_function(mem_fn)
      && get_member_function_is_virtual(mem_fn))
    result += "virtual ";

  decl_base_sptr type;
  if ((mem_fn
       && is_member_function(mem_fn)
       && (get_member_function_is_dtor(*mem_fn)
	   || get_member_function_is_ctor(*mem_fn))))
    /*cdtors do not have return types.  */;
  else
    type = mem_fn
      ? get_type_declaration(mem_fn->get_type()->get_return_type())
      : get_type_declaration(get_type()->get_return_type());

  if (type)
    result += type->get_qualified_name(internal) + " ";

  result += get_pretty_representation_of_declarator(internal);

  return result;
}

/// Compute and return the pretty representation for the part of the
/// function declaration that starts at the declarator.  That is, the
/// return type and the other specifiers of the beginning of the
/// function's declaration ar omitted.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation for the part of the function
/// declaration that starts at the declarator.
string
function_decl::get_pretty_representation_of_declarator (bool internal) const
{
  const method_decl* mem_fn =
    dynamic_cast<const method_decl*>(this);

  string result;

  if (mem_fn)
    {
      result += mem_fn->get_type()->get_class_type()->get_qualified_name()
	+ "::" + mem_fn->get_name();
    }
  else
    result += get_qualified_name();

  result += "(";

  parameters::const_iterator i = get_parameters().begin(),
    end = get_parameters().end();

  // Skip the first parameter if this is a method.
  if (mem_fn && i != end)
    ++i;
  parameter_sptr parm;
  parameter_sptr first_parm;
  if (i != end)
    first_parm = *i;
  for (; i != end; ++i)
    {
      parm = *i;
      if (parm.get() != first_parm.get())
	result += ", ";
      if (parm->get_variadic_marker()
	  || get_environment()->is_variadic_parameter_type(parm->get_type()))
	result += "...";
      else
	{
	  decl_base_sptr type_decl = get_type_declaration(parm->get_type());
	  result += type_decl->get_qualified_name(internal);
	}
    }
  result += ")";

  if (mem_fn
      &&((is_member_function(mem_fn) && get_member_function_is_const(*mem_fn))
	 || is_method_type(mem_fn->get_type())->get_is_const()))
    result += " const";

  return result;
}

/// Getter for the first non-implicit parameter of a function decl.
///
/// If the function is a non-static member function, the parameter
/// returned is the first one following the implicit 'this' parameter.
///
/// @return the first non implicit parm.
function_decl::parameters::const_iterator
function_decl::get_first_non_implicit_parm() const
{
  if (get_parameters().empty())
    return get_parameters().end();

  bool is_method = dynamic_cast<const method_decl*>(this);

  parameters::const_iterator i = get_parameters().begin();
  if (is_method)
    ++i;

  return i;
}

/// Return the type of the current instance of @ref function_decl.
///
/// It's either a function_type or method_type.
/// @return the type of the current instance of @ref function_decl.
const shared_ptr<function_type>
function_decl::get_type() const
{return priv_->type_.lock();}

/// Fast getter of the type of the current instance of @ref function_decl.
///
/// Note that this function returns the underlying pointer managed by
/// the smart pointer returned by function_decl::get_type().  It's
/// faster than function_decl::get_type().  This getter is to be used
/// in code paths that are proven to be performance hot spots;
/// especially (for instance) when comparing function types.  Those
/// are compared extremely frequently when libabigail is used to
/// handle huge binaries with a lot of functions.
///
/// @return the type of the current instance of @ref function_decl.
const function_type*
function_decl::get_naked_type() const
{return priv_->naked_type_;}

void
function_decl::set_type(const function_type_sptr& fn_type)
{
  priv_->type_ = fn_type;
  priv_->naked_type_ = fn_type.get();
}

/// This sets the underlying ELF symbol for the current function decl.
///
/// And underlyin$g ELF symbol for the current function decl might
/// exist only if the corpus that this function decl originates from
/// was constructed from an ELF binary file.
///
/// Note that comparing two function decls that have underlying ELF
/// symbols involves comparing their underlying elf symbols.  The decl
/// name for the function thus becomes irrelevant in the comparison.
///
/// @param sym the new ELF symbol for this function decl.
void
function_decl::set_symbol(const elf_symbol_sptr& sym)
{
  priv_->symbol_ = sym;
  // The function id cache that depends on the symbol must be
  // invalidated because the symbol changed.
  priv_->id_ = get_environment()->intern("");
}

/// Gets the the underlying ELF symbol for the current variable,
/// that was set using function_decl::set_symbol().  Please read the
/// documentation for that member function for more information about
/// "underlying ELF symbols".
///
/// @return sym the underlying ELF symbol for this function decl, if
/// one exists.
const elf_symbol_sptr&
function_decl::get_symbol() const
{return priv_->symbol_;}

bool
function_decl::is_declared_inline() const
{return priv_->declared_inline_;}

decl_base::binding
function_decl::get_binding() const
{return priv_->binding_;}

/// @return the return type of the current instance of function_decl.
const shared_ptr<type_base>
function_decl::get_return_type() const
{return get_type()->get_return_type();}

/// @return the parameters of the function.
const std::vector<shared_ptr<function_decl::parameter> >&
function_decl::get_parameters() const
{return get_type()->get_parameters();}

/// Append a parameter to the type of this function.
///
/// @param parm the parameter to append.
void
function_decl::append_parameter(shared_ptr<parameter> parm)
{get_type()->append_parameter(parm);}

/// Append a vector of parameters to the type of this function.
///
/// @param parms the vector of parameters to append.
void
function_decl::append_parameters(std::vector<shared_ptr<parameter> >& parms)
{
  for (std::vector<shared_ptr<parameter> >::const_iterator i = parms.begin();
       i != parms.end();
       ++i)
    get_type()->append_parameter(*i);
}

/// Create a new instance of function_decl that is a clone of the
/// current one.
///
/// @return the new clone.
function_decl_sptr
function_decl::clone() const
{
  function_decl_sptr f;
  if (is_member_function(*this))
    {
      method_decl_sptr
	m(new method_decl(get_name(),
				      get_type(),
				      is_declared_inline(),
				      get_location(),
				      get_linkage_name(),
				      get_visibility(),
				      get_binding()));
      class_or_union* scope = is_class_or_union_type(get_scope());
      ABG_ASSERT(scope);
      scope->add_member_function(m, get_member_access_specifier(*this),
				 get_member_function_is_virtual(*this),
				 get_member_function_vtable_offset(*this),
				 get_member_is_static(*this),
				 get_member_function_is_ctor(*this),
				 get_member_function_is_dtor(*this),
				 get_member_function_is_const(*this));
      f = m;
    }
  else
    {
      f.reset(new function_decl(get_name(),
				get_type(),
				is_declared_inline(),
				get_location(),
				get_linkage_name(),
				get_visibility(),
				get_binding()));
      add_decl_to_scope(f, get_scope());
    }
  f->set_symbol(get_symbol());

  return f;
}

/// Compares two instances of @ref function_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const function_decl& l, const function_decl& r, change_kind* k)
{
  bool result = true;

  // Compare function types
  const type_base* t0 = l.get_naked_type(), *t1 = r.get_naked_type();
  if (t0 == t1 || *t0 == *t1)
    ; // the types are equal, let's move on to compare the other
      // properties of the functions.
  else
    {
      result = false;
      if (k)
	{
	  if (!types_have_similar_structure(t0, t1))
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	  else
	    *k |= SUBTYPE_CHANGE_KIND;
	}
      else
	return false;
    }

  const elf_symbol_sptr &s0 = l.get_symbol(), &s1 = r.get_symbol();
  if (!!s0 != !!s1)
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }
  else if (s0 && s0 != s1)
    {
      if (!elf_symbols_alias(s0, s1))
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	  else
	    return false;
	}
    }
  bool symbols_are_equal = (s0 && s1 && result);

  if (symbols_are_equal)
    {
      // The functions have underlying elf symbols that are equal,
      // so now, let's compare the decl_base part of the functions
      // w/o considering their decl names.
      interned_string n1 = l.get_name(), n2 = r.get_name();
      interned_string ln1 = l.get_linkage_name(), ln2 = r.get_linkage_name();
      const_cast<function_decl&>(l).set_name("");
      const_cast<function_decl&>(l).set_linkage_name("");
      const_cast<function_decl&>(r).set_name("");
      const_cast<function_decl&>(r).set_linkage_name("");

      bool decl_bases_different = !l.decl_base::operator==(r);

      const_cast<function_decl&>(l).set_name(n1);
      const_cast<function_decl&>(l).set_linkage_name(ln1);
      const_cast<function_decl&>(r).set_name(n2);
      const_cast<function_decl&>(r).set_linkage_name(ln2);

      if (decl_bases_different)
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	  else
	    return false;
	}
    }
  else
    if (!l.decl_base::operator==(r))
      {
	result = false;
	if (k)
	  *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	else
	  return false;
      }

  // Compare the remaining properties
  if (l.is_declared_inline() != r.is_declared_inline()
      || l.get_binding() != r.get_binding())
    {
      result = false;
      if (k)
	*k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }

  if (is_member_function(l) != is_member_function(r))
    {
      result = false;
      if (k)
	  *k |= LOCAL_NON_TYPE_CHANGE_KIND;
      else
	return false;
    }

  if (is_member_function(l) && is_member_function(r))
    {
      if (!((get_member_function_is_ctor(l)
	     == get_member_function_is_ctor(r))
	    && (get_member_function_is_dtor(l)
		== get_member_function_is_dtor(r))
	    && (get_member_is_static(l)
		== get_member_is_static(r))
	    && (get_member_function_is_const(l)
		== get_member_function_is_const(r))
	    && (get_member_function_is_virtual(l)
		== get_member_function_is_virtual(r))
	    && (get_member_function_vtable_offset(l)
		== get_member_function_vtable_offset(r))))
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	  else
	    return false;
	}
    }

  return result;
}

/// Comparison operator for @ref function_decl.
///
/// @param other the other instance of @ref function_decl to compare
/// against.
///
/// @return true iff the current instance of @ref function_decl equals
/// @p other.
bool
function_decl::operator==(const decl_base& other) const
{
  const function_decl* o = dynamic_cast<const function_decl*>(&other);
  if (!o)
    return false;
  return equals(*this, *o, 0);
}

/// Return true iff the function takes a variable number of
/// parameters.
///
/// @return true if the function taks a variable number
/// of parameters.
bool
function_decl::is_variadic() const
{
  return (!get_parameters().empty()
	  && get_parameters().back()->get_variadic_marker());
}

/// The virtual implementation of 'get_hash' for a function_decl.
///
/// This allows decl_base::get_hash to work for function_decls.
///
/// @return the hash value for function decl.
size_t
function_decl::get_hash() const
{
  function_decl::hash hash_fn;
  return hash_fn(*this);
}

/// Return an ID that tries to uniquely identify the function inside a
/// program or a library.
///
/// So if the function has an underlying elf symbol, the ID is the
/// concatenation of the symbol name and its version.  Otherwise, the
/// ID is the linkage name if its non-null.  Otherwise, it's the
/// pretty representation of the function.
///
/// @return the ID.
interned_string
function_decl::get_id() const
{
  if (priv_->id_.empty())
    {
      const environment* env = get_type()->get_environment();
      if (elf_symbol_sptr s = get_symbol())
	{
	  if (s->has_aliases())
	    // The symbol has several aliases, so let's use a scheme
	    // that allows all aliased functions to have different
	    // IDs.
	    priv_->id_ = env->intern(get_name() + "/" + s->get_id_string());
	  else
	    // Let's use the full symbol name with its version as ID.
	    priv_->id_ = env->intern(s->get_id_string());
	}
      else if (!get_linkage_name().empty())
	priv_->id_= env->intern(get_linkage_name());
      else
	priv_->id_ = env->intern(get_pretty_representation());
    }
  return priv_->id_;
}

/// Test if two function declarations are aliases.
///
/// Two functions declarations are aliases if their symbols are
/// aliases, in the ELF sense.
///
/// @param f1 the first function to consider.
///
/// @param f2 the second function to consider.
///
/// @return true iff @p f1 is an alias of @p f2
bool
function_decls_alias(const function_decl& f1, const function_decl& f2)
{
  elf_symbol_sptr s1 = f1.get_symbol(), s2 = f2.get_symbol();

  if (!s1 || !s2)
    return false;

  return elf_symbols_alias(s1, s2);
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
function_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

/// Destructor of the @ref function_decl type.
function_decl::~function_decl()
{delete priv_;}

/// A deep comparison operator for a shared pointer to @ref function_decl
///
/// This function compares to shared pointers to @ref function_decl by
/// looking at the pointed-to instances of @ref function_dec
/// comparing them too.  If the two pointed-to objects are equal then
/// this function returns true.
///
/// @param l the left-hand side argument of the equality operator.
///
/// @param r the right-hand side argument of the equality operator.
///
/// @return true iff @p l equals @p r.
bool
operator==(const function_decl_sptr& l, const function_decl_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// A deep inequality operator for smart pointers to functions.
///
/// @param l the left-hand side argument of the inequality operator.
///
/// @pram r the right-hand side argument of the inequality operator.
///
/// @return true iff @p is not equal to @p r.
bool
operator!=(const function_decl_sptr& l, const function_decl_sptr& r)
{return !operator==(l, r);}

// <function_decl definitions>

// <function_decl::parameter definitions>

struct function_decl::parameter::priv
{
  type_base_wptr	type_;
  unsigned		index_;
  bool			variadic_marker_;

  priv()
    : index_(),
      variadic_marker_()
  {}

  priv(type_base_sptr type,
       unsigned index,
       bool variadic_marker)
    : type_(type),
      index_(index),
      variadic_marker_(variadic_marker)
  {}
};// end struct function_decl::parameter::priv

function_decl::parameter::parameter(const type_base_sptr	type,
				    unsigned			index,
				    const string&		name,
				    const location&		loc,
				    bool			is_variadic)
  : type_or_decl_base(type->get_environment(),
		      FUNCTION_PARAMETER_DECL | ABSTRACT_DECL_BASE),
    decl_base(type->get_environment(), name, loc),
    priv_(new priv(type, index, is_variadic))
{
  runtime_type_instance(this);
}

function_decl::parameter::parameter(const type_base_sptr	type,
				    unsigned			index,
				    const string&		name,
				    const location&		loc,
				    bool			is_variadic,
				    bool			is_artificial)
  : type_or_decl_base(type->get_environment(),
		      FUNCTION_PARAMETER_DECL | ABSTRACT_DECL_BASE),
    decl_base(type->get_environment(), name, loc),
    priv_(new priv(type, index, is_variadic))
{
  runtime_type_instance(this);
  set_is_artificial(is_artificial);
}

function_decl::parameter::parameter(const type_base_sptr	type,
				    const string&		name,
				    const location&		loc,
				    bool			is_variadic,
				    bool			is_artificial)
  : type_or_decl_base(type->get_environment(),
		      FUNCTION_PARAMETER_DECL | ABSTRACT_DECL_BASE),
    decl_base(type->get_environment(), name, loc),
    priv_(new priv(type, 0, is_variadic))
{
  runtime_type_instance(this);
  set_is_artificial(is_artificial);
}

function_decl::parameter::parameter(const type_base_sptr	type,
				    unsigned			index,
				    bool			variad)
  : type_or_decl_base(type->get_environment(),
		      FUNCTION_PARAMETER_DECL | ABSTRACT_DECL_BASE),
    decl_base(type->get_environment(), "", location()),
    priv_(new priv(type, index, variad))
{
  runtime_type_instance(this);
}

const type_base_sptr
function_decl::parameter::get_type()const
{return priv_->type_.lock();}

/// @return a copy of the type name of the parameter.
interned_string
function_decl::parameter::get_type_name() const
{
  const environment* env = get_environment();
  ABG_ASSERT(env);

  type_base_sptr t = get_type();
  string str;
  if (get_variadic_marker() || env->is_variadic_parameter_type(t))
    str = "...";
  else
    {
	ABG_ASSERT(t);
	str = abigail::ir::get_type_name(t);
    }
  return env->intern(str);
}

/// @return a copy of the pretty representation of the type of the
/// parameter.
const string
function_decl::parameter::get_type_pretty_representation() const
{
  type_base_sptr t = get_type();
  string str;
  if (get_variadic_marker()
      || get_environment()->is_variadic_parameter_type(t))
    str = "...";
  else
    {
	ABG_ASSERT(t);
	str += get_type_declaration(t)->get_pretty_representation();
    }
  return str;
}

/// Get a name uniquely identifying the parameter in the function.
///
///@return the unique parm name id.
interned_string
function_decl::parameter::get_name_id() const
{
  const environment* env = get_environment();
  ABG_ASSERT(env);

  std::ostringstream o;
  o << "parameter-" << get_index();

  return env->intern(o.str());
}

unsigned
function_decl::parameter::get_index() const
{return priv_->index_;}

void
function_decl::parameter::set_index(unsigned i)
{priv_->index_ = i;}


bool
function_decl::parameter::get_variadic_marker() const
{return priv_->variadic_marker_;}

/// Compares two instances of @ref function_decl::parameter.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const function_decl::parameter& l,
       const function_decl::parameter& r,
       change_kind* k)
{
  bool result = true;

  if ((l.get_variadic_marker() != r.get_variadic_marker())
      || (l.get_index() != r.get_index())
      || (!!l.get_type() != !!r.get_type()))
    {
      result = false;
      if (k)
	{
	  if (l.get_index() != r.get_index())
	    *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	  if (l.get_variadic_marker() != r.get_variadic_marker()
	      || !!l.get_type() != !!r.get_type())
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	}
      else
	return false;
    }


  // Sometimes, function parameters can be wrapped into a no-op
  // qualifier.  Let's strip that qualifier out.
  type_base_sptr l_type = look_through_no_op_qualified_type(l.get_type());
  type_base_sptr r_type = look_through_no_op_qualified_type(r.get_type());
  if (l_type != r_type)
    {
      result = false;
      if (k)
	{
	  if (!types_have_similar_structure(l_type, r_type))
	    *k |= LOCAL_TYPE_CHANGE_KIND;
	  else
	    *k |= SUBTYPE_CHANGE_KIND;
	}
      else
	return false;
    }

  return result;
}

bool
function_decl::parameter::operator==(const parameter& o) const
{return equals(*this, o, 0);}

bool
function_decl::parameter::operator==(const decl_base& o) const
{
  const function_decl::parameter* p =
    dynamic_cast<const function_decl::parameter*>(&o);
  if (!p)
    return false;
  return function_decl::parameter::operator==(*p);
}

/// Non-member equality operator for @ref function_decl::parameter.
///
/// @param l the left-hand side of the equality operator
///
/// @param r the right-hand side of the equality operator
///
/// @return true iff @p l and @p r equals.
bool
operator==(const function_decl::parameter_sptr& l,
	   const function_decl::parameter_sptr& r)
{
  if (!!l != !!r)
    return false;
  if (!l)
    return true;
  return *l == *r;
}

/// Non-member inequality operator for @ref function_decl::parameter.
///
/// @param l the left-hand side of the equality operator
///
/// @param r the right-hand side of the equality operator
///
/// @return true iff @p l and @p r different.
bool
operator!=(const function_decl::parameter_sptr& l,
	   const function_decl::parameter_sptr& r)
{return !operator==(l, r);}

/// Traverse the diff sub-tree under the current instance
/// function_decl.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going on, false
/// otherwise.
bool
function_decl::parameter::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

/// Get the hash of a decl.  If the hash hasn't been computed yet,
/// compute it ans store its value; otherwise, just return the hash.
///
/// @return the hash of the decl.
size_t
function_decl::parameter::get_hash() const
{
  function_decl::parameter::hash hash_fn_parm;
  return hash_fn_parm(this);
}

/// Compute the qualified name of the parameter.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qn the resulting qualified name.
void
function_decl::parameter::get_qualified_name(interned_string& qualified_name,
					     bool /*internal*/) const
{qualified_name = get_name();}

/// Compute and return a copy of the pretty representation of the
/// current function parameter.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the textual representation of the current
/// function parameter.
string
function_decl::parameter::get_pretty_representation(bool internal,
						    bool /*qualified_name*/) const
{
  const environment* env = get_environment();
  ABG_ASSERT(env);

  string type_repr;
  type_base_sptr t = get_type();
  if (!t)
    type_repr = "void";
  else if (env->is_variadic_parameter_type(t))
    type_repr = "...";
  else
    type_repr = ir::get_pretty_representation(t, internal);

  string result = type_repr;
  string parm_name = get_name_id();

  if (!parm_name.empty())
    result += " " + parm_name;

  return result;
}

// </function_decl::parameter definitions>

// <class_or_union definitions>
struct class_or_union::priv
{
  typedef_decl_wptr		naming_typedef_;
  member_types			member_types_;
  data_members			data_members_;
  data_members			non_static_data_members_;
  member_functions		member_functions_;
  // A map that associates a linkage name to a member function.
  string_mem_fn_sptr_map_type	mem_fns_map_;
  // A map that associates function signature strings to member
  // function.
  string_mem_fn_ptr_map_type	signature_2_mem_fn_map_;
  member_function_templates	member_function_templates_;
  member_class_templates	member_class_templates_;

  priv()
  {}

  priv(class_or_union::member_types& mbr_types,
       class_or_union::data_members& data_mbrs,
       class_or_union::member_functions& mbr_fns)
    : member_types_(mbr_types),
      data_members_(data_mbrs),
      member_functions_(mbr_fns)
  {
    for (data_members::const_iterator i = data_members_.begin();
	 i != data_members_.end();
	 ++i)
      if (!get_member_is_static(*i))
	non_static_data_members_.push_back(*i);
  }

  /// Mark a class or union or union as being currently compared using
  /// the class_or_union== operator.
  ///
  /// Note that is marking business is to avoid infinite loop when
  /// comparing a class or union or union. If via the comparison of a
  /// data member or a member function a recursive re-comparison of
  /// the class or union is attempted, the marking business help to
  /// detect that infinite loop possibility and avoid it.
  ///
  /// @param klass the class or union or union to mark as being
  /// currently compared.
  void
  mark_as_being_compared(const class_or_union& klass) const
  {
    const environment* env = klass.get_environment();
    ABG_ASSERT(env);
    env->priv_->classes_being_compared_.insert(&klass);
  }

  /// Mark a class or union as being currently compared using the
  /// class_or_union== operator.
  ///
  /// Note that is marking business is to avoid infinite loop when
  /// comparing a class or union. If via the comparison of a data
  /// member or a member function a recursive re-comparison of the
  /// class or union is attempted, the marking business help to detect
  /// that infinite loop possibility and avoid it.
  ///
  /// @param klass the class or union to mark as being currently
  /// compared.
  void
  mark_as_being_compared(const class_or_union* klass) const
  {mark_as_being_compared(*klass);}

  /// Mark a class or union as being currently compared using the
  /// class_or_union== operator.
  ///
  /// Note that is marking business is to avoid infinite loop when
  /// comparing a class or union. If via the comparison of a data
  /// member or a member function a recursive re-comparison of the
  /// class or union is attempted, the marking business help to detect
  /// that infinite loop possibility and avoid it.
  ///
  /// @param klass the class or union to mark as being currently
  /// compared.
  void
  mark_as_being_compared(const class_or_union_sptr& klass) const
  {mark_as_being_compared(*klass);}

  /// If the instance of class_or_union has been previously marked as
  /// being compared -- via an invocation of mark_as_being_compared()
  /// this method unmarks it.  Otherwise is has no effect.
  ///
  /// This method is not thread safe because it uses the static data
  /// member classes_being_compared_.  If you wish to use it in a
  /// multi-threaded environment you should probably protect the
  /// access to that static data member with a mutex or somesuch.
  ///
  /// @param klass the instance of class_or_union to unmark.
  void
  unmark_as_being_compared(const class_or_union& klass) const
  {
    const environment* env = klass.get_environment();
    ABG_ASSERT(env);
    env->priv_->classes_being_compared_.erase(&klass);
  }

  /// If the instance of class_or_union has been previously marked as
  /// being compared -- via an invocation of mark_as_being_compared()
  /// this method unmarks it.  Otherwise is has no effect.
  ///
  /// @param klass the instance of class_or_union to unmark.
  void
  unmark_as_being_compared(const class_or_union* klass) const
  {
    if (klass)
      return unmark_as_being_compared(*klass);
  }

  /// Test if a given instance of class_or_union is being currently
  /// compared.
  ///
  ///@param klass the class or union to test.
  ///
  /// @return true if @p klass is being compared, false otherwise.
  bool
  comparison_started(const class_or_union& klass) const
  {
    const environment* env = klass.get_environment();
    ABG_ASSERT(env);
    return env->priv_->classes_being_compared_.count(&klass);
  }

  /// Test if a given instance of class_or_union is being currently
  /// compared.
  ///
  ///@param klass the class or union to test.
  ///
  /// @return true if @p klass is being compared, false otherwise.
  bool
  comparison_started(const class_or_union* klass) const
  {
    if (klass)
      return comparison_started(*klass);
    return false;
  }
}; // end struct class_or_union::priv

/// A Constructor for instances of @ref class_or_union
///
/// @param env the environment we are operating from.
///
/// @param name the identifier of the class.
///
/// @param size_in_bits the size of an instance of @ref
/// class_or_union, expressed in bits
///
/// @param align_in_bits the alignment of an instance of @ref class_or_union,
/// expressed in bits.
///
/// @param locus the source location of declaration point this class.
///
/// @param vis the visibility of instances of @ref class_or_union.
///
/// @param mem_types the vector of member types of this instance of
/// @ref class_or_union.
///
/// @param data_members the vector of data members of this instance of
/// @ref class_or_union.
///
/// @param member_fns the vector of member functions of this instance
/// of @ref class_or_union.
class_or_union::class_or_union(const environment* env, const string& name,
			       size_t size_in_bits, size_t align_in_bits,
			       const location& locus, visibility vis,
			       member_types& mem_types,
			       data_members& data_members,
			       member_functions& member_fns)
  : type_or_decl_base(env,
		      ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, locus, name, vis),
    type_base(env, size_in_bits, align_in_bits),
    scope_type_decl(env, name, size_in_bits, align_in_bits, locus, vis),
    priv_(new priv(mem_types, data_members, member_fns))
{
  for (member_types::iterator i = mem_types.begin();
       i != mem_types.end();
       ++i)
    if (!has_scope(get_type_declaration(*i)))
      add_decl_to_scope(get_type_declaration(*i), this);

  for (data_members::iterator i = data_members.begin();
       i != data_members.end();
       ++i)
    if (!has_scope(*i))
      add_decl_to_scope(*i, this);

  for (member_functions::iterator i = member_fns.begin();
       i != member_fns.end();
       ++i)
    if (!has_scope(static_pointer_cast<decl_base>(*i)))
      add_decl_to_scope(*i, this);
}

/// A constructor for instances of @ref class_or_union.
///
/// @param env the environment we are operating from.
///
/// @param name the name of the class.
///
/// @param size_in_bits the size of an instance of @ref
/// class_or_union, expressed in bits
///
/// @param align_in_bits the alignment of an instance of @ref class_or_union,
/// expressed in bits.
///
/// @param locus the source location of declaration point this class.
///
/// @param vis the visibility of instances of @ref class_or_union.
class_or_union::class_or_union(const environment* env, const string& name,
			       size_t size_in_bits, size_t align_in_bits,
			       const location& locus, visibility vis)
  : type_or_decl_base(env,
		      ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, locus, name, vis),
    type_base(env, size_in_bits, align_in_bits),
    scope_type_decl(env, name, size_in_bits, align_in_bits, locus, vis),
    priv_(new priv)
{}

/// Constructor of the @ref class_or_union type.
///
/// @param env the @ref environment we are operating from.
///
/// @param name the name of the @ref class_or_union.
///
/// @param is_declaration_only a boolean saying whether the instance
/// represents a declaration only, or not.
class_or_union::class_or_union(const environment* env, const string& name,
			       bool is_declaration_only)
  : type_or_decl_base(env,
		      ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, location(), name),
    type_base(env, 0, 0),
    scope_type_decl(env, name, 0, 0, location()),
    priv_(new priv)
{
  set_is_declaration_only(is_declaration_only);
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the member nodes of the translation
/// unit during the traversal.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
class_or_union::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      bool stop = false;

      if (!stop)
	for (data_members::const_iterator i = get_data_members().begin();
	     i != get_data_members().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_functions::const_iterator i= get_member_functions().begin();
	     i != get_member_functions().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_types::const_iterator i = get_member_types().begin();
	     i != get_member_types().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_function_templates::const_iterator i =
	       get_member_function_templates().begin();
	     i != get_member_function_templates().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_class_templates::const_iterator i =
	       get_member_class_templates().begin();
	     i != get_member_class_templates().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

/// Destrcutor of the @ref class_or_union type.
class_or_union::~class_or_union()
{delete priv_;}

/// Add a member declaration to the current instance of class_or_union.
/// The member declaration can be either a member type, data member,
/// member function, or member template.
///
/// @param d the member declaration to add.
decl_base_sptr
class_or_union::add_member_decl(const decl_base_sptr& d)
{return insert_member_decl(d, get_member_decls().end());}

/// Remove a given decl from the current @ref class_or_union scope.
///
/// Note that only type declarations are supported by this method for
/// now.  Support for the other kinds of declaration is left as an
/// exercise for the interested reader of the code.
///
/// @param decl the declaration to remove from this @ref
/// class_or_union scope.
void
class_or_union::remove_member_decl(decl_base_sptr decl)
{
  type_base_sptr t = is_type(decl);

  // For now we want to support just removing types from classes.  For
  // other kinds of IR node, we need more work.
  ABG_ASSERT(t);

  remove_member_type(t);
}

/// Fixup the members of the type of an anonymous data member.
///
/// Walk all data members of (the type of) a given anonymous data
/// member and set a particular property of the relationship between
/// each data member and its containing type.
///
/// That property records the fact that the data member belongs to the
/// anonymous data member we consider.
///
/// In the future, if there are other properties of this relationship
/// to set in this manner, they ought to be added here.
///
/// @param anon_dm the anonymous data member to consider.
void
class_or_union::maybe_fixup_members_of_anon_data_member(var_decl_sptr& anon_dm)
{
  class_or_union * anon_dm_type =
    anonymous_data_member_to_class_or_union(anon_dm.get());
  if (!anon_dm_type)
    return;

  for (class_or_union::data_members::const_iterator it =
	 anon_dm_type->get_non_static_data_members().begin();
       it != anon_dm_type->get_non_static_data_members().end();
       ++it)
    {
      dm_context_rel *rel =
	dynamic_cast<dm_context_rel*>((*it)->get_context_rel());
      ABG_ASSERT(rel);
      rel->set_anonymous_data_member(anon_dm.get());
    }
}

/// Insert a member type.
///
/// @param t the type to insert in the @ref class_or_union type.
///
/// @param an iterator right before which @p t has to be inserted.
void
class_or_union::insert_member_type(type_base_sptr t,
				   declarations::iterator before)
{
  decl_base_sptr d = get_type_declaration(t);
  ABG_ASSERT(d);
  ABG_ASSERT(!has_scope(d));

  priv_->member_types_.push_back(t);
  scope_decl::insert_member_decl(d, before);
}

/// Add a member type to the current instance of class_or_union.
///
/// @param t the member type to add.  It must not have been added to a
/// scope, otherwise this will violate an ABG_ASSERTion.
void
class_or_union::add_member_type(type_base_sptr t)
{insert_member_type(t, get_member_decls().end());}

/// Add a member type to the current instance of class_or_union.
///
/// @param t the type to be added as a member type to the current
/// instance of class_or_union.  An instance of class_or_union::member_type
/// will be created out of @p t and and added to the the class.
///
/// @param a the access specifier for the member type to be created.
type_base_sptr
class_or_union::add_member_type(type_base_sptr t, access_specifier a)
{
  decl_base_sptr d = get_type_declaration(t);
  ABG_ASSERT(d);
  ABG_ASSERT(!is_member_decl(d));
  add_member_type(t);
  set_member_access_specifier(d, a);
  return t;
}

/// Remove a member type from the current @ref class_or_union scope.
///
/// @param t the type to remove.
void
class_or_union::remove_member_type(type_base_sptr t)
{
  for (member_types::iterator i = priv_->member_types_.begin();
       i != priv_->member_types_.end();
       ++i)
    {
      if (*((*i)) == *t)
	{
	  priv_->member_types_.erase(i);
	  return;
	}
    }
}

/// Getter of the alignment of the @ref class_or_union type.
///
/// If this @ref class_or_union is a declaration of a definition that
/// is elsewhere, then the size of the definition is returned.
///
/// @return the alignment of the @ref class_or_union type.
size_t
class_or_union::get_alignment_in_bits() const
{
  if (get_is_declaration_only() && get_definition_of_declaration())
    return is_class_or_union_type
      (get_definition_of_declaration())->get_alignment_in_bits();

   return type_base::get_alignment_in_bits();
}

/// Setter of the alignment of the class type.
///
/// If this class is a declaration of a definition that is elsewhere,
/// then the new alignment is set to the definition.
///
/// @param s the new alignment.
void
class_or_union::set_alignment_in_bits(size_t a)
{
  if (get_is_declaration_only() && get_definition_of_declaration())
    is_class_or_union_type
      (get_definition_of_declaration()) ->set_alignment_in_bits(a);
  else
    type_base::set_alignment_in_bits(a);
}

/// Setter of the size of the @ref class_or_union type.
///
/// If this @ref class_or_union is a declaration of a definition that
/// is elsewhere, then the new size is set to the definition.
///
/// @param s the new size.
void
class_or_union::set_size_in_bits(size_t s)
{
  if (get_is_declaration_only() && get_definition_of_declaration())
    is_class_or_union_type
      (get_definition_of_declaration())->set_size_in_bits(s);
  else
    type_base::set_size_in_bits(s);
}

/// Getter of the size of the @ref class_or_union type.
///
/// If this @ref class_or_union is a declaration of a definition that
/// is elsewhere, then the size of the definition is returned.
///
/// @return the size of the @ref class_or_union type.
size_t
class_or_union::get_size_in_bits() const
{
  if (get_is_declaration_only() && get_definition_of_declaration())
    return is_class_or_union_type
      (get_definition_of_declaration())->get_size_in_bits();

  return type_base::get_size_in_bits();
}


/// Getter for the naming typedef of the current class.
///
/// Consider the C idiom:
///
///    typedef struct {int member;} foo_type;
///
/// In that idiom, foo_type is the naming typedef of the anonymous
/// struct that is declared.
///
/// @return the naming typedef, if any.  Otherwise, returns nil.
typedef_decl_sptr
class_or_union::get_naming_typedef() const
{return priv_->naming_typedef_.lock();}

/// Set the naming typedef of the current instance of @ref class_decl.
///
/// Consider the C idiom:
///
///    typedef struct {int member;} foo_type;
///
/// In that idiom, foo_type is the naming typedef of the anonymous
/// struct that is declared.
///
/// @param typedef_type the new naming typedef.
void
class_or_union::set_naming_typedef(const typedef_decl_sptr& typedef_type)
{
  priv_->naming_typedef_ = typedef_type;
}

/// Get the member types of this @ref class_or_union.
///
/// @return a vector of the member types of this ref class_or_union.
const class_or_union::member_types&
class_or_union::get_member_types() const
{return priv_->member_types_;}

/// Get the number of anonymous member classes contained in this
/// class.
///
/// @return the number of anonymous member classes contained in this
/// class.
size_t
class_or_union::get_num_anonymous_member_classes() const
{
  int result = 0;
  for (member_types::const_iterator it = get_member_types().begin();
       it != get_member_types().end();
       ++it)
    if (class_decl_sptr t = is_class_type(*it))
      if (t->get_is_anonymous())
	++result;

  return result;
}

/// Get the number of anonymous member unions contained in this class.
///
/// @return the number of anonymous member unions contained in this
/// class.
size_t
class_or_union::get_num_anonymous_member_unions() const
{
  int result = 0;
  for (member_types::const_iterator it = get_member_types().begin();
       it != get_member_types().end();
       ++it)
    if (union_decl_sptr t = is_union_type(*it))
      if (t->get_is_anonymous())
	++result;

  return result;
}

/// Get the number of anonymous member enums contained in this class.
///
/// @return the number of anonymous member enums contained in this
/// class.
size_t
class_or_union::get_num_anonymous_member_enums() const
{
  int result = 0;
  for (member_types::const_iterator it = get_member_types().begin();
       it != get_member_types().end();
       ++it)
    if (enum_type_decl_sptr t = is_enum_type(*it))
      if (t->get_is_anonymous())
	++result;

  return result;
}

/// Find a member type of a given name, inside the current @ref
/// class_or_union.
///
/// @param name the name of the member type to look for.
///
/// @return a pointer to the @ref type_base that represents the member
/// type of name @p name, for the current class.
type_base_sptr
class_or_union::find_member_type(const string& name) const
{
  for (member_types::const_iterator i = get_member_types().begin();
       i != get_member_types().end();
       ++i)
    if (get_type_name(*i, /*qualified*/false) == name)
      return *i;
  return type_base_sptr();
}

/// Add a data member to the current instance of class_or_union.
///
/// @param v a var_decl to add as a data member.  A proper
/// class_or_union::data_member is created from @p v and added to the
/// class_or_union.  This var_decl should not have been already added
/// to a scope.
///
/// @param access the access specifier for the data member.
///
/// @param is_laid_out whether the data member was laid out.  That is,
/// if its offset has been computed.  In the pattern of a class
/// template for instance, this would be set to false.
///
/// @param is_static whether the data memer is static.
///
/// @param offset_in_bits if @p is_laid_out is true, this is the
/// offset of the data member, expressed (oh, surprise) in bits.
void
class_or_union::add_data_member(var_decl_sptr v, access_specifier access,
				bool is_laid_out, bool is_static,
				size_t offset_in_bits)
{
  ABG_ASSERT(!has_scope(v));

  priv_->data_members_.push_back(v);
  scope_decl::add_member_decl(v);
  set_data_member_is_laid_out(v, is_laid_out);
  set_data_member_offset(v, offset_in_bits);
  set_member_access_specifier(v, access);
  set_member_is_static(v, is_static);

  if (!is_static)
    {
      // If this is a non-static variable, add it to the set of
      // non-static variables, if it's not only in there.
      bool is_already_in = false;
      for (data_members::const_iterator i =
	     priv_->non_static_data_members_.begin();
	   i != priv_->non_static_data_members_.end();
	   ++i)
	if (*i == v)
	  {
	    is_already_in = true;
	    break;
	  }
      if (!is_already_in)
	priv_->non_static_data_members_.push_back(v);
    }

  // If v is an anonymous data member, then fixup its data members.
  // For now, the only thing the fixup does is to make the data
  // members of the anonymous data member be aware of their containing
  // anonymous data member.  That is helpful to compute the absolute
  // bit offset of each of the members of the anonymous data member.
  maybe_fixup_members_of_anon_data_member(v);
}

/// Get the data members of this @ref class_or_union.
///
/// @return a vector of the data members of this @ref class_or_union.
const class_or_union::data_members&
class_or_union::get_data_members() const
{return priv_->data_members_;}

/// Find a data member of a given name in the current @ref class_or_union.
///
/// @param name the name of the data member to find in the current
/// @ref class_or_union.
///
/// @return a pointer to the @ref var_decl that represents the data
/// member to find inside the current @ref class_or_union.
const var_decl_sptr
class_or_union::find_data_member(const string& name) const
{
  for (data_members::const_iterator i = get_data_members().begin();
       i != get_data_members().end();
       ++i)
    if ((*i)->get_name() == name)
      return *i;

  // We haven't found a data member with the name 'name'.  Let's look
  // closer again, this time in our anonymous data members.
  for (data_members::const_iterator i = get_data_members().begin();
       i != get_data_members().end();
       ++i)
    if (is_anonymous_data_member(*i))
      {
	class_or_union_sptr type = is_class_or_union_type((*i)->get_type());
	ABG_ASSERT(type);
	if (var_decl_sptr data_member = type->find_data_member(name))
	  return data_member;
      }

  return var_decl_sptr();
}

/// Find an anonymous data member in the class.
///
/// @param v the anonymous data member to find.
///
/// @return the anonymous data member found, or nil if none was found.
const var_decl_sptr
class_or_union::find_anonymous_data_member(const var_decl_sptr& v) const
{
  if (!v->get_name().empty())
    return var_decl_sptr();

  for (data_members::const_iterator it = get_non_static_data_members().begin();
       it != get_non_static_data_members().end();
       ++it)
    {
      if (is_anonymous_data_member(*it))
	if ((*it)->get_pretty_representation(true, true)
	    == v->get_pretty_representation(true, true))
	  return *it;
    }

  return var_decl_sptr();
}

/// Find a given data member.
///
/// This function takes a @ref var_decl as an argument.  If it has a
/// non-empty name, then it tries to find a data member which has the
/// same name as the argument.
///
/// If it has an empty name, then the @ref var_decl is considered as
/// an anonymous data member.  In that case, this function tries to
/// find an anonymous data member which type equals that of the @ref
/// var_decl argument.
///
/// @param v this carries either the name of the data member we need
/// to look for, or the type of the anonymous data member we are
/// looking for.
const var_decl_sptr
class_or_union::find_data_member(const var_decl_sptr& v) const
{
  if (!v)
    return var_decl_sptr();

  if (v->get_name().empty())
    return find_anonymous_data_member(v);

  return find_data_member(v->get_name());
}


/// Get the non-static data memebers of this @ref class_or_union.
///
/// @return a vector of the non-static data members of this @ref
/// class_or_union.
const class_or_union::data_members&
class_or_union::get_non_static_data_members() const
{return priv_->non_static_data_members_;}

/// Add a member function.
///
/// @param f the new member function to add.
///
/// @param a the access specifier to use for the new member function.
///
/// @param is_static whether the new member function is static.
///
/// @param is_ctor whether the new member function is a constructor.
///
/// @param is_dtor whether the new member function is a destructor.
///
/// @param is_const whether the new member function is const.
void
class_or_union::add_member_function(method_decl_sptr f,
				    access_specifier a,
				    bool is_static, bool is_ctor,
				    bool is_dtor, bool is_const)
{
  ABG_ASSERT(!has_scope(f));

  scope_decl::add_member_decl(f);

  set_member_function_is_ctor(f, is_ctor);
  set_member_function_is_dtor(f, is_dtor);
  set_member_access_specifier(f, a);
  set_member_is_static(f, is_static);
  set_member_function_is_const(f, is_const);

  priv_->member_functions_.push_back(f);

  // Update the map of linkage name -> member functions.  It's useful,
  // so that class_or_union::find_member_function() can function.
  if (!f->get_linkage_name().empty())
    priv_->mem_fns_map_[f->get_linkage_name()] = f;
}

/// Get the member functions of this @ref class_or_union.
///
/// @return a vector of the member functions of this @ref
/// class_or_union.
const class_or_union::member_functions&
class_or_union::get_member_functions() const
{return priv_->member_functions_;}

/// Find a method, using its linkage name as a key.
///
/// @param linkage_name the linkage name of the method to find.
///
/// @return the method found, or nil if none was found.
const method_decl*
class_or_union::find_member_function(const string& linkage_name) const
{
  return const_cast<class_or_union*>(this)->find_member_function(linkage_name);
}

/// Find a method, using its linkage name as a key.
///
/// @param linkage_name the linkage name of the method to find.
///
/// @return the method found, or nil if none was found.
method_decl*
class_or_union::find_member_function(const string& linkage_name)
{
  string_mem_fn_sptr_map_type::const_iterator i =
    priv_->mem_fns_map_.find(linkage_name);
  if (i == priv_->mem_fns_map_.end())
    return 0;
  return i->second.get();
}

/// Find a method, using its linkage name as a key.
///
/// @param linkage_name the linkage name of the method to find.
///
/// @return the method found, or nil if none was found.
method_decl_sptr
class_or_union::find_member_function_sptr(const string& linkage_name)
{
  string_mem_fn_sptr_map_type::const_iterator i =
    priv_->mem_fns_map_.find(linkage_name);
  if (i == priv_->mem_fns_map_.end())
    return 0;
  return i->second;
}

/// Find a method (member function) using its signature (pretty
/// representation) as a key.
///
/// @param s the signature of the method.
///
/// @return the method found, or nil if none was found.
const method_decl*
class_or_union::find_member_function_from_signature(const string& s) const
{
  return const_cast<class_or_union*>(this)->find_member_function_from_signature(s);
}

/// Find a method (member function) using its signature (pretty
/// representation) as a key.
///
/// @param s the signature of the method.
///
/// @return the method found, or nil if none was found.
method_decl*
class_or_union::find_member_function_from_signature(const string& s)
{
  string_mem_fn_ptr_map_type::const_iterator i =
    priv_->signature_2_mem_fn_map_.find(s);
  if (i == priv_->signature_2_mem_fn_map_.end())
    return 0;
  return i->second;
}

/// Get the member function templates of this class.
///
/// @return a vector of the member function templates of this class.
const member_function_templates&
class_or_union::get_member_function_templates() const
{return priv_->member_function_templates_;}

/// Get the member class templates of this class.
///
/// @return a vector of the member class templates of this class.
const member_class_templates&
class_or_union::get_member_class_templates() const
{return priv_->member_class_templates_;}

/// Append a member function template to the @ref class_or_union.
///
/// @param m the member function template to append.
void
class_or_union::add_member_function_template(member_function_template_sptr m)
{
  decl_base* c = m->as_function_tdecl()->get_scope();
  /// TODO: use our own ABG_ASSERTion facility that adds a meaningful
  /// error message or something like a structured error.
  priv_->member_function_templates_.push_back(m);
  if (!c)
    scope_decl::add_member_decl(m->as_function_tdecl());
}

/// Append a member class template to the @ref class_or_union.
///
/// @param m the member function template to append.
void
class_or_union::add_member_class_template(member_class_template_sptr m)
{
  decl_base* c = m->as_class_tdecl()->get_scope();
  /// TODO: use our own ABG_ASSERTion facility that adds a meaningful
  /// error message or something like a structured error.
  m->set_scope(this);
  priv_->member_class_templates_.push_back(m);
  if (!c)
    scope_decl::add_member_decl(m->as_class_tdecl());
}

///@return true iff the current instance has no member.
bool
class_or_union::has_no_member() const
{
  return (priv_->member_types_.empty()
	  && priv_->data_members_.empty()
	  && priv_->member_functions_.empty()
	  && priv_->member_function_templates_.empty()
	  && priv_->member_class_templates_.empty());
}

/// Insert a data member to this @ref class_or_union type.
///
/// @param d the data member to insert.
///
/// @param before an iterator to the point before which to insert the
/// the data member, in the coontainer that contains all the data
/// members.
decl_base_sptr
class_or_union::insert_member_decl(decl_base_sptr d,
				   declarations::iterator before)
{
  if (type_base_sptr t = dynamic_pointer_cast<type_base>(d))
    insert_member_type(t, before);
  else if (var_decl_sptr v = dynamic_pointer_cast<var_decl>(d))
    {
      add_data_member(v, public_access,
		      /*is_laid_out=*/false,
		      /*is_static=*/true,
		      /*offset_in_bits=*/0);
      d = v;
    }
  else if (method_decl_sptr f = dynamic_pointer_cast<method_decl>(d))
    add_member_function(f, public_access,
			/*is_static=*/false,
			/*is_ctor=*/false,
			/*is_dtor=*/false,
			/*is_const=*/false);
  else if (member_function_template_sptr f =
	   dynamic_pointer_cast<member_function_template>(d))
    add_member_function_template(f);
  else if (member_class_template_sptr c =
	   dynamic_pointer_cast<member_class_template>(d))
    add_member_class_template(c);
  else
    scope_decl::add_member_decl(d);

  return d;
}

/// Equality operator.
///
/// @param other the other @ref class_or_union to compare against.
///
/// @return true iff @p other equals the current @ref class_or_union.
bool
class_or_union::operator==(const decl_base& other) const
{
  const class_or_union* op = dynamic_cast<const class_or_union*>(&other);
  if (!op)
    return false;

  // If this is a decl-only type (and thus with no canonical type),
  // use the canonical type of the definition, if any.
  const class_or_union *l = 0;
  if (get_is_declaration_only())
    l = dynamic_cast<const class_or_union*>(get_naked_definition_of_declaration());
  if (l == 0)
    l = this;

  // Likewise for the other class.
  const class_or_union *r = 0;
  if (op->get_is_declaration_only())
    r = dynamic_cast<const class_or_union*>(op->get_naked_definition_of_declaration());
  if (r == 0)
    r = op;

  return try_canonical_compare(l, r);
}

/// Equality operator.
///
/// @param other the other @ref class_or_union to compare against.
///
/// @return true iff @p other equals the current @ref class_or_union.
bool
class_or_union::operator==(const type_base& other) const
{
  const decl_base* o = dynamic_cast<const decl_base*>(&other);
  if (!o)
    return false;
  return *this == *o;
}

/// Equality operator.
///
/// @param other the other @ref class_or_union to compare against.
///
/// @return true iff @p other equals the current @ref class_or_union.
bool
class_or_union::operator==(const class_or_union& other) const
{
  const decl_base& o = other;
  return class_or_union::operator==(o);
}

/// Compares two instances of @ref class_or_union.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff it's non-null and if the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const class_or_union& l, const class_or_union& r, change_kind* k)
{
#define RETURN(value)				\
  do {						\
    l.priv_->unmark_as_being_compared(l);	\
    l.priv_->unmark_as_being_compared(r);	\
    return value;				\
  } while(0)

  // if one of the classes is declaration-only, look through it to
  // get its definition.
  bool l_is_decl_only = l.get_is_declaration_only();
  bool r_is_decl_only = r.get_is_declaration_only();
  if (l_is_decl_only || r_is_decl_only)
    {
      const class_or_union* def1 = l_is_decl_only
	? is_class_or_union_type(l.get_naked_definition_of_declaration())
	: &l;

      const class_or_union* def2 = r_is_decl_only
	? is_class_or_union_type(r.get_naked_definition_of_declaration())
	: &r;

      if (!def1 || !def2)
	{
	  if (!l.get_is_anonymous()
	      && !r.get_is_anonymous()
	      && l_is_decl_only && r_is_decl_only
	      && comparison::filtering::is_decl_only_class_with_size_change(l, r))
	    // The two decl-only classes differ from their size.  A
	    // true decl-only class should not have a size property to
	    // begin with.  This comes from a DWARF oddity and can
	    // results in a false positive, so let's not consider that
	    // change.
	    return true;

	  if (l.get_environment()->decl_only_class_equals_definition()
	      && !l.get_is_anonymous()
	      && !r.get_is_anonymous())
	    {
	      const interned_string& q1 = l.get_scoped_name();
	      const interned_string& q2 = r.get_scoped_name();
	      if (q1 == q2)
		// Not using RETURN(true) here, because that causes
		// performance issues.  We don't need to do
		// l.priv_->unmark_as_being_compared({l,r}) here because
		// we haven't marked l or r as being compared yet, and
		// doing so has a peformance cost that shows up on
		// performance profiles for *big* libraries.
		return true;
	      else
		{
		  if (k)
		    *k |= LOCAL_TYPE_CHANGE_KIND;
		  // Not using RETURN(true) here, because that causes
		  // performance issues.  We don't need to do
		  // l.priv_->unmark_as_being_compared({l,r}) here because
		  // we haven't marked l or r as being compared yet, and
		  // doing so has a peformance cost that shows up on
		  // performance profiles for *big* libraries.
		  return false;
		}
	    }
	  else // A decl-only class is considered different from a
	       // class definition of the same name.
	    {
	      if (!!def1 != !!def2)
		{
		  if (k)
		    *k |= LOCAL_TYPE_CHANGE_KIND;
		  return false;
		}

	      // both definitions are empty
	      if (!(l.decl_base::operator==(r)
		       && l.type_base::operator==(r)))
		{
		  if (k)
		    *k |= LOCAL_TYPE_CHANGE_KIND;
		  return false;
		}

	      return true;
	    }
	}

      if (l.priv_->comparison_started(l)
	  || l.priv_->comparison_started(r))
	return true;

      l.priv_->mark_as_being_compared(l);
      l.priv_->mark_as_being_compared(r);

      bool val = *def1 == *def2;
      if (!val)
	if (k)
	  *k |= LOCAL_TYPE_CHANGE_KIND;
      RETURN(val);
    }

  // No need to go further if the classes have different names or
  // different size / alignment.
  if (!(l.decl_base::operator==(r) && l.type_base::operator==(r)))
    {
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      return false;
    }

  if (types_defined_same_linux_kernel_corpus_public(l, r))
    return true;

  if (l.priv_->comparison_started(l)
      || l.priv_->comparison_started(r))
    return true;

  l.priv_->mark_as_being_compared(l);
  l.priv_->mark_as_being_compared(r);

  bool result = true;

  //compare data_members
  {
    if (l.get_non_static_data_members().size()
	!= r.get_non_static_data_members().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_TYPE_CHANGE_KIND;
	else
	  RETURN(result);
      }

    for (class_or_union::data_members::const_iterator
	   d0 = l.get_non_static_data_members().begin(),
	   d1 = r.get_non_static_data_members().begin();
	 (d0 != l.get_non_static_data_members().end()
	  && d1 != r.get_non_static_data_members().end());
	 ++d0, ++d1)
      if (**d0 != **d1)
	{
	  result = false;
	  if (k)
	    {
	      // Report any representation change as being local.
	      if (!types_have_similar_structure((*d0)->get_type(),
						(*d1)->get_type())
		  || (*d0)->get_type() == (*d1)->get_type())
		*k |= LOCAL_TYPE_CHANGE_KIND;
	      else
		*k |= SUBTYPE_CHANGE_KIND;
	    }
	  else
	    RETURN(result);
	}
  }

  // Do not compare member functions.  DWARF does not necessarily
  // all the member functions, be they virtual or not, in all
  // translation units.  So we cannot have a clear view of them, per
  // class

  // compare member function templates
  {
    if (l.get_member_function_templates().size()
	!= r.get_member_function_templates().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	else
	  RETURN(result);
      }

    for (member_function_templates::const_iterator
	   fn_tmpl_it0 = l.get_member_function_templates().begin(),
	   fn_tmpl_it1 = r.get_member_function_templates().begin();
	 fn_tmpl_it0 != l.get_member_function_templates().end()
	   &&  fn_tmpl_it1 != r.get_member_function_templates().end();
	 ++fn_tmpl_it0, ++fn_tmpl_it1)
      if (**fn_tmpl_it0 != **fn_tmpl_it1)
	{
	  result = false;
	  if (k)
	    {
	      *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	      break;
	    }
	  else
	    RETURN(result);
	}
  }

  // compare member class templates
  {
    if (l.get_member_class_templates().size()
	!= r.get_member_class_templates().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	else
	  RETURN(result);
      }

    for (member_class_templates::const_iterator
	   cl_tmpl_it0 = l.get_member_class_templates().begin(),
	   cl_tmpl_it1 = r.get_member_class_templates().begin();
	 cl_tmpl_it0 != l.get_member_class_templates().end()
	   &&  cl_tmpl_it1 != r.get_member_class_templates().end();
	 ++cl_tmpl_it0, ++cl_tmpl_it1)
      if (**cl_tmpl_it0 != **cl_tmpl_it1)
	{
	  result = false;
	  if (k)
	    {
	      *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	      break;
	    }
	  else
	    RETURN(result);
	}
  }

  RETURN(result);
#undef RETURN
}


/// Copy a method of a @ref class_or_union into a new @ref
/// class_or_union.
///
/// @param t the @ref class_or_union into which the method is to be copied.
///
/// @param method the method to copy into @p t.
///
/// @return the resulting newly copied method.
method_decl_sptr
copy_member_function(const class_or_union_sptr& t,
		     const method_decl_sptr& method)
{return copy_member_function(t, method.get());}


/// Copy a method of a @ref class_or_union into a new @ref
/// class_or_union.
///
/// @param t the @ref class_or_union into which the method is to be copied.
///
/// @param method the method to copy into @p t.
///
/// @return the resulting newly copied method.
method_decl_sptr
copy_member_function(const class_or_union_sptr& t, const method_decl* method)
{
  ABG_ASSERT(t);
  ABG_ASSERT(method);

  method_type_sptr old_type = method->get_type();
  ABG_ASSERT(old_type);
  method_type_sptr new_type(new method_type(old_type->get_return_type(),
					    t,
					    old_type->get_parameters(),
					    old_type->get_is_const(),
					    old_type->get_size_in_bits(),
					    old_type->get_alignment_in_bits()));
  new_type->set_environment(t->get_environment());
  keep_type_alive(new_type);

  method_decl_sptr
    new_method(new method_decl(method->get_name(),
			       new_type,
			       method->is_declared_inline(),
			       method->get_location(),
			       method->get_linkage_name(),
			       method->get_visibility(),
			       method->get_binding()));
  new_method->set_symbol(method->get_symbol());

  if (class_decl_sptr class_type = is_class_type(t))
    class_type->add_member_function(new_method,
				    get_member_access_specifier(*method),
				    get_member_function_is_virtual(*method),
				    get_member_function_vtable_offset(*method),
				    get_member_is_static(*method),
				    get_member_function_is_ctor(*method),
				    get_member_function_is_dtor(*method),
				    get_member_function_is_const(*method));
  else
    t->add_member_function(new_method,
			   get_member_access_specifier(*method),
			   get_member_is_static(*method),
			   get_member_function_is_ctor(*method),
			   get_member_function_is_dtor(*method),
			   get_member_function_is_const(*method));
  return new_method;
}

// </class_or_union definitions>

/// Test if two @ref class_decl or @ref function_type are already
/// being compared.
///
/// @param lhs_type the first type that would be involved in a
/// potential comparison.
///
/// @param rhs_type the second type that would involved in a potential
/// comparison.
///
/// @return true iff @p lhs_type and @p rhs_type are being compared.
static bool
types_are_being_compared(const type_base& lhs_type,
			 const type_base& rhs_type)
{
  type_base *l = &const_cast<type_base&>(lhs_type);
  type_base *r = &const_cast<type_base&>(rhs_type);

  if (class_or_union *l_cou = is_class_or_union_type(l))
    if (class_or_union *r_cou = is_class_or_union_type(r))
      return (l_cou->priv_->comparison_started(*l_cou)
	      || l_cou->priv_->comparison_started(*r_cou));

  if (function_type *l_fn_type = is_function_type(l))
    if (function_type *r_fn_type = is_function_type(r))
      return (l_fn_type->priv_->comparison_started(*l_fn_type)
	      || l_fn_type->priv_->comparison_started(*r_fn_type));

  return false;
}

/// @defgroup OnTheFlyCanonicalization On-the-fly Canonicalization
/// @{
///
/// During the canonicalization of a type T (which doesn't yet have a
/// canonical type), T is compared structurally (member-wise) against
/// a type C which already has a canonical type.  The comparison
/// expression is C == T.
///
/// During that structural comparison, if a subtype of C (which also
/// already has a canonical type) is structurally compared to a
/// subtype of T (which doesn't yet have a canonical type) and if they
/// are equal, then we can deduce that the canonical type of the
/// subtype of C is the canonical type of the subtype of C.
///
/// Thus, we can canonicalize the sub-type of the T, during the
/// canonicalization of T itself.  That canonicalization of the
/// sub-type of T is what we call the "on-the-fly canonicalization".
/// It's on the fly because it happens during a comparison -- which
/// itself happens during the canonicalization of T.
///
/// For now this on-the-fly canonicalization only happens when
/// comparing @ref class_decl and @ref function_type.
///
/// @}


/// If on-the-fly canonicalization is turned on, then this function
/// sets the canonical type of its second parameter to the canonical
/// type of the first parameter.
///
/// @param lhs_type the type which canonical type to propagate.
///
/// @param rhs_type the type which canonical type to set.
static void
maybe_propagate_canonical_type(const type_base& lhs_type,
			       const type_base& rhs_type)
{

  if (const environment *env = lhs_type.get_environment())
    if (env->do_on_the_fly_canonicalization())
      if (type_base_sptr canonical_type = lhs_type.get_canonical_type())
	if (!rhs_type.get_canonical_type()
	    && !types_are_being_compared(lhs_type, rhs_type))
	  {
	    const_cast<type_base&>(rhs_type).priv_->canonical_type =
	      canonical_type;
	    const_cast<type_base&>(rhs_type).priv_->naked_canonical_type =
	      canonical_type.get();
	  }
}

// <class_decl definitions>

static void
sort_virtual_member_functions(class_decl::member_functions& mem_fns);

/// The private data for the class_decl type.
struct class_decl::priv
{
  base_specs					bases_;
  unordered_map<string, base_spec_sptr>	bases_map_;
  member_functions				virtual_mem_fns_;
  virtual_mem_fn_map_type			virtual_mem_fns_map_;
  bool						is_struct_;

  priv()
    : is_struct_(false)
  {}

  priv(bool is_struct, class_decl::base_specs& bases)
    : bases_(bases),
      is_struct_(is_struct)
  {
  }

  priv(bool is_struct)
    : is_struct_(is_struct)
  {}
};// end struct class_decl::priv

/// A Constructor for instances of \ref class_decl
///
/// @param env the environment we are operating from.
///
/// @param name the identifier of the class.
///
/// @param size_in_bits the size of an instance of class_decl, expressed
/// in bits
///
/// @param align_in_bits the alignment of an instance of class_decl,
/// expressed in bits.
///
/// @param locus the source location of declaration point this class.
///
/// @param vis the visibility of instances of class_decl.
///
/// @param bases the vector of base classes for this instance of class_decl.
///
/// @param mbrs the vector of member types of this instance of
/// class_decl.
///
/// @param data_mbrs the vector of data members of this instance of
/// class_decl.
///
/// @param mbr_fns the vector of member functions of this instance of
/// class_decl.
class_decl::class_decl(const environment* env, const string& name,
		       size_t size_in_bits, size_t align_in_bits,
		       bool is_struct, const location& locus,
		       visibility vis, base_specs& bases,
		       member_types& mbr_types,
		       data_members& data_mbrs,
		       member_functions& mbr_fns)
  : type_or_decl_base(env,
		      CLASS_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, locus, name, vis),
    type_base(env, size_in_bits, align_in_bits),
    class_or_union(env, name, size_in_bits, align_in_bits,
		   locus, vis, mbr_types, data_mbrs, mbr_fns),
    priv_(new priv(is_struct, bases))
{
  runtime_type_instance(this);
}

/// A Constructor for instances of @ref class_decl
///
/// @param env the environment we are operating from.
///
/// @param name the identifier of the class.
///
/// @param size_in_bits the size of an instance of class_decl, expressed
/// in bits
///
/// @param align_in_bits the alignment of an instance of class_decl,
/// expressed in bits.
///
/// @param locus the source location of declaration point this class.
///
/// @param vis the visibility of instances of class_decl.
///
/// @param bases the vector of base classes for this instance of class_decl.
///
/// @param mbrs the vector of member types of this instance of
/// class_decl.
///
/// @param data_mbrs the vector of data members of this instance of
/// class_decl.
///
/// @param mbr_fns the vector of member functions of this instance of
/// class_decl.
///
/// @param is_anonymous whether the newly created instance is
/// anonymous.
class_decl::class_decl(const environment* env, const string& name,
		       size_t size_in_bits, size_t align_in_bits,
		       bool is_struct, const location& locus,
		       visibility vis, base_specs& bases,
		       member_types& mbr_types, data_members& data_mbrs,
		       member_functions& mbr_fns, bool is_anonymous)
  : type_or_decl_base(env,
		      CLASS_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, locus,
	      // If the class is anonymous then by default it won't
	      // have a linkage name.  Also, the anonymous class does
	      // have an internal-only unique name that is generally
	      // not taken into account when comparing classes; such a
	      // unique internal-only name, when used as a linkage
	      // name might introduce spurious comparison false
	      // negatives.
	      /*linkage_name=*/is_anonymous ? string() : name,
	      vis),
    type_base(env, size_in_bits, align_in_bits),
    class_or_union(env, name, size_in_bits, align_in_bits,
		   locus, vis, mbr_types, data_mbrs, mbr_fns),
    priv_(new priv(is_struct, bases))
{
  runtime_type_instance(this);
  set_is_anonymous(is_anonymous);
}

/// A constructor for instances of class_decl.
///
/// @param env the environment we are operating from.
///
/// @param name the name of the class.
///
/// @param size_in_bits the size of an instance of class_decl, expressed
/// in bits
///
/// @param align_in_bits the alignment of an instance of class_decl,
/// expressed in bits.
///
/// @param locus the source location of declaration point this class.
///
/// @param vis the visibility of instances of class_decl.
class_decl::class_decl(const environment* env, const string& name,
		       size_t size_in_bits, size_t align_in_bits,
		       bool is_struct, const location& locus,
		       visibility vis)
  : type_or_decl_base(env,
		      CLASS_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, locus, name, vis),
    type_base(env, size_in_bits, align_in_bits),
    class_or_union(env, name, size_in_bits, align_in_bits,
		   locus, vis),
    priv_(new priv(is_struct))
{
  runtime_type_instance(this);
}

/// A constructor for instances of @ref class_decl.
///
/// @param env the environment we are operating from.
///
/// @param name the name of the class.
///
/// @param size_in_bits the size of an instance of class_decl, expressed
/// in bits
///
/// @param align_in_bits the alignment of an instance of class_decl,
/// expressed in bits.
///
/// @param locus the source location of declaration point this class.
///
/// @param vis the visibility of instances of class_decl.
///
/// @param is_anonymous whether the newly created instance is
/// anonymous.
class_decl:: class_decl(const environment* env, const string& name,
			size_t size_in_bits, size_t align_in_bits,
			bool is_struct, const location& locus,
			visibility vis, bool is_anonymous)
  : type_or_decl_base(env,
		      CLASS_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, locus,
	      // If the class is anonymous then by default it won't
	      // have a linkage name.  Also, the anonymous class does
	      // have an internal-only unique name that is generally
	      // not taken into account when comparing classes; such a
	      // unique internal-only name, when used as a linkage
	      // name might introduce spurious comparison false
	      // negatives.
	      /*linkage_name=*/ is_anonymous ? string() : name,
	      vis),
    type_base(env, size_in_bits, align_in_bits),
    class_or_union(env, name, size_in_bits, align_in_bits,
		   locus, vis),
  priv_(new priv(is_struct))
{
  runtime_type_instance(this);
  set_is_anonymous(is_anonymous);
}

/// A constuctor for instances of class_decl that represent a
/// declaration without definition.
///
/// @param env the environment we are operating from.
///
/// @param name the name of the class.
///
/// @param is_declaration_only a boolean saying whether the instance
/// represents a declaration only, or not.
class_decl::class_decl(const environment* env, const string& name,
		       bool is_struct, bool is_declaration_only)
  : type_or_decl_base(env,
		      CLASS_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, location(), name),
    type_base(env, 0, 0),
    class_or_union(env, name, is_declaration_only),
    priv_(new priv(is_struct))
{
  runtime_type_instance(this);
}

/// This method is invoked automatically right after the current
/// instance of @ref class_decl has been canonicalized.
///
/// Currently, the only thing it does is to sort the virtual member
/// functions vector.
void
class_decl::on_canonical_type_set()
{
  sort_virtual_mem_fns();

  for (class_decl::virtual_mem_fn_map_type::iterator i =
	 priv_->virtual_mem_fns_map_.begin();
       i != priv_->virtual_mem_fns_map_.end();
       ++i)
    sort_virtual_member_functions(i->second);
}

/// Set the "is-struct" flag of the class.
///
/// @param f the new value of the flag.
void
class_decl::is_struct(bool f)
{priv_->is_struct_ = f;}

/// Test if the class is a struct.
///
/// @return true iff the class is a struct.
bool
class_decl::is_struct() const
{return priv_->is_struct_;}

/// Add a base specifier to this class.
///
/// @param b the new base specifier.
void
class_decl::add_base_specifier(base_spec_sptr b)
{
  ABG_ASSERT(get_environment());
  ABG_ASSERT(b->get_environment() == get_environment());
  priv_->bases_.push_back(b);
  priv_->bases_map_[b->get_base_class()->get_qualified_name()] = b;
  if (const environment* env = get_environment())
    b->set_environment(env);
}

/// Get the base specifiers for this class.
///
/// @return a vector of the base specifiers.
const class_decl::base_specs&
class_decl::get_base_specifiers() const
{return priv_->bases_;}

/// Find a base class of a given qualified name for the current class.
///
/// @param qualified_name the qualified name of the base class to look for.
///
/// @return a pointer to the @ref class_decl that represents the base
/// class of name @p qualified_name, if found.
class_decl_sptr
class_decl::find_base_class(const string& qualified_name) const
{
  unordered_map<string, base_spec_sptr>::iterator i =
    priv_->bases_map_.find(qualified_name);

  if (i != priv_->bases_map_.end())
    return i->second->get_base_class();

  return class_decl_sptr();
}

/// Get the virtual member functions of this class.
///
/// @param return a vector of the virtual member functions of this
/// class.
const class_decl::member_functions&
class_decl::get_virtual_mem_fns() const
{return priv_->virtual_mem_fns_;}

/// Get the map that associates a virtual table offset to the virtual
/// member functions with that virtual table offset.
///
/// Usually, there should be a 1:1 mapping between a given vtable
/// offset and virtual member functions of that vtable offset.  But
/// because of some implementation details, there can be several C++
/// destructor functions that are *generated* by compilers, for a
/// given destructor that is defined in the source code.  If the
/// destructor is virtual then those generated functions have some
/// DWARF attributes in common with the constructor that the user
/// actually defined in its source code.  Among those attributes are
/// the vtable offset of the destructor.
///
/// @return the map that associates a virtual table offset to the
/// virtual member functions with that virtual table offset.
const class_decl::virtual_mem_fn_map_type&
class_decl::get_virtual_mem_fns_map() const
{return priv_->virtual_mem_fns_map_;}

/// Sort the virtual member functions by their virtual index.
void
class_decl::sort_virtual_mem_fns()
{sort_virtual_member_functions(priv_->virtual_mem_fns_);}

/// Getter of the pretty representation of the current instance of
/// @ref class_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qualified_name if true, names emitted in the pretty
/// representation are fully qualified.
///
/// @return the pretty representaion for a class_decl.
string
class_decl::get_pretty_representation(bool internal,
				      bool qualified_name) const
{
  string cl = "class ";
  if (!internal && is_struct())
    cl = "struct ";

  // When computing the pretty representation for internal purposes,
  // if an anonymous class is named by a typedef, then consider that
  // it has a name, which is the typedef name.
  if (get_is_anonymous())
    return get_class_or_union_flat_representation(this, "",
						  /*one_line=*/true,
						  internal);

  string result = cl;
  if (qualified_name)
    result += get_qualified_name(internal);
  else
    result += get_name();

  return result;
}

decl_base_sptr
class_decl::insert_member_decl(decl_base_sptr d,
			       declarations::iterator before)
{
  if (method_decl_sptr f = dynamic_pointer_cast<method_decl>(d))
    add_member_function(f, public_access,
			/*is_virtual=*/false,
			/*vtable_offset=*/0,
			/*is_static=*/false,
			/*is_ctor=*/false,
			/*is_dtor=*/false,
			/*is_const=*/false);
  else
    d = class_or_union::insert_member_decl(d, before);

  return d;
}

/// The private data structure of class_decl::base_spec.
struct class_decl::base_spec::priv
{
  class_decl_wptr	base_class_;
  long			offset_in_bits_;
  bool			is_virtual_;

  priv(const class_decl_sptr& cl,
       long offset_in_bits,
       bool is_virtual)
    : base_class_(cl),
      offset_in_bits_(offset_in_bits),
      is_virtual_(is_virtual)
  {}
};

/// Constructor for base_spec instances.
///
/// @param base the base class to consider
///
/// @param a the access specifier of the base class.
///
/// @param offset_in_bits if positive or null, represents the offset
/// of the base in the layout of its containing type..  If negative,
/// means that the current base is not laid out in its containing type.
///
/// @param is_virtual if true, means that the current base class is
/// virtual in it's containing type.
class_decl::base_spec::base_spec(const class_decl_sptr& base,
				 access_specifier a,
				 long offset_in_bits,
				 bool is_virtual)
  : type_or_decl_base(base->get_environment(),
		      ABSTRACT_DECL_BASE),
    decl_base(base->get_environment(), base->get_name(), base->get_location(),
	      base->get_linkage_name(), base->get_visibility()),
    member_base(a),
    priv_(new priv(base, offset_in_bits, is_virtual))
{
  runtime_type_instance(this);
}

/// Get the base class referred to by the current base class
/// specifier.
///
/// @return the base class.
class_decl_sptr
class_decl::base_spec::get_base_class() const
{return priv_->base_class_.lock();}

/// Getter of the "is-virtual" proprerty of the base class specifier.
///
/// @return true iff this specifies a virtual base class.
bool
class_decl::base_spec::get_is_virtual() const
{return priv_->is_virtual_;}

/// Getter of the offset of the base.
///
/// @return the offset of the base.
long
class_decl::base_spec::get_offset_in_bits() const
{return priv_->offset_in_bits_;}

/// Calculate the hash value for a class_decl::base_spec.
///
/// @return the hash value.
size_t
class_decl::base_spec::get_hash() const
{
  base_spec::hash h;
  return h(*this);
}

/// Traverses an instance of @ref class_decl::base_spec, visiting all
/// the sub-types and decls that it might contain.
///
/// @param v the visitor that is used to visit every IR sub-node of
/// the current node.
///
/// @return true if either
///  - all the children nodes of the current IR node were traversed
///    and the calling code should keep going with the traversing.
///  - or the current IR node is already being traversed.
/// Otherwise, returning false means that the calling code should not
/// keep traversing the tree.
bool
class_decl::base_spec::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      get_base_class()->traverse(v);
      visiting(false);
    }

  return v.visit_end(this);
}

/// Constructor for base_spec instances.
///
/// Note that this constructor is for clients that don't support RTTI
/// and that have a base class of type_base, but of dynamic type
/// class_decl.
///
/// @param base the base class to consider.  Must be a pointer to an
/// instance of class_decl
///
/// @param a the access specifier of the base class.
///
/// @param offset_in_bits if positive or null, represents the offset
/// of the base in the layout of its containing type..  If negative,
/// means that the current base is not laid out in its containing type.
///
/// @param is_virtual if true, means that the current base class is
/// virtual in it's containing type.
class_decl::base_spec::base_spec(const type_base_sptr& base,
				 access_specifier a,
				 long offset_in_bits,
				 bool is_virtual)
  : type_or_decl_base(base->get_environment(),
		      ABSTRACT_DECL_BASE),
    decl_base(base->get_environment(), get_type_declaration(base)->get_name(),
	      get_type_declaration(base)->get_location(),
	      get_type_declaration(base)->get_linkage_name(),
	      get_type_declaration(base)->get_visibility()),
    member_base(a),
    priv_(new priv(dynamic_pointer_cast<class_decl>(base),
		   offset_in_bits,
		   is_virtual))
{
  runtime_type_instance(this);
}

/// Compares two instances of @ref class_decl::base_spec.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const class_decl::base_spec& l,
       const class_decl::base_spec& r,
       change_kind* k)
{
  if (!l.member_base::operator==(r))
    {
      if (k)
	*k |= LOCAL_TYPE_CHANGE_KIND;
      return false;
    }

  return (*l.get_base_class() == *r.get_base_class());
}

/// Comparison operator for @ref class_decl::base_spec.
///
/// @param other the instance of @ref class_decl::base_spec to compare
/// against.
///
/// @return true if the current instance of @ref class_decl::base_spec
/// equals @p other.
bool
class_decl::base_spec::operator==(const decl_base& other) const
{
  const class_decl::base_spec* o =
    dynamic_cast<const class_decl::base_spec*>(&other);

  if (!o)
    return false;

  return equals(*this, *o, 0);
}

/// Comparison operator for @ref class_decl::base_spec.
///
/// @param other the instance of @ref class_decl::base_spec to compare
/// against.
///
/// @return true if the current instance of @ref class_decl::base_spec
/// equals @p other.
bool
class_decl::base_spec::operator==(const member_base& other) const
{
  const class_decl::base_spec* o =
    dynamic_cast<const class_decl::base_spec*>(&other);
  if (!o)
    return false;

  return operator==(static_cast<const decl_base&>(*o));
}

mem_fn_context_rel::~mem_fn_context_rel()
{
}

/// A constructor for instances of method_decl.
///
/// @param name the name of the method.
///
/// @param type the type of the method.
///
/// @param declared_inline whether the method was
/// declared inline or not.
///
/// @param locus the source location of the method.
///
/// @param linkage_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
method_decl::method_decl(const string&		name,
			 method_type_sptr	type,
			 bool			declared_inline,
			 const location&	locus,
			 const string&		linkage_name,
			 visibility		vis,
			 binding		bind)
  : type_or_decl_base(type->get_environment(),
		      METHOD_DECL
		      | ABSTRACT_DECL_BASE
		      |FUNCTION_DECL),
    decl_base(type->get_environment(), name, locus, linkage_name, vis),
    function_decl(name, static_pointer_cast<function_type>(type),
		  declared_inline, locus, linkage_name, vis, bind)
{
  runtime_type_instance(this);
  set_context_rel(new mem_fn_context_rel(0));
  set_member_function_is_const(*this, type->get_is_const());
}

/// A constructor for instances of method_decl.
///
/// @param name the name of the method.
///
/// @param type the type of the method.  Must be an instance of
/// method_type.
///
/// @param declared_inline whether the method was
/// declared inline or not.
///
/// @param locus the source location of the method.
///
/// @param linkage_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
method_decl::method_decl(const string&		name,
			 function_type_sptr	type,
			 bool			declared_inline,
			 const location&	locus,
			 const string&		linkage_name,
			 visibility		vis,
			 binding		bind)
  : type_or_decl_base(type->get_environment(),
		      METHOD_DECL
		      | ABSTRACT_DECL_BASE
		      | FUNCTION_DECL),
    decl_base(type->get_environment(), name, locus, linkage_name, vis),
    function_decl(name, static_pointer_cast<function_type>
		  (dynamic_pointer_cast<method_type>(type)),
		  declared_inline, locus, linkage_name, vis, bind)
{
  runtime_type_instance(this);
  set_context_rel(new mem_fn_context_rel(0));
}

/// A constructor for instances of method_decl.
///
/// @param name the name of the method.
///
/// @param type the type of the method.  Must be an instance of
/// method_type.
///
/// @param declared_inline whether the method was
/// declared inline or not.
///
/// @param locus the source location of the method.
///
/// @param linkage_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
method_decl::method_decl(const string&		name,
			 type_base_sptr	type,
			 bool			declared_inline,
			 const location&	locus,
			 const string&		linkage_name,
			 visibility		vis,
			 binding		bind)
  : type_or_decl_base(type->get_environment(),
		      METHOD_DECL
		      | ABSTRACT_DECL_BASE
		      | FUNCTION_DECL),
    decl_base(type->get_environment(), name, locus, linkage_name, vis),
    function_decl(name, static_pointer_cast<function_type>
		  (dynamic_pointer_cast<method_type>(type)),
		  declared_inline, locus, linkage_name, vis, bind)
{
  runtime_type_instance(this);
  set_context_rel(new mem_fn_context_rel(0));
}

/// Set the linkage name of the method.
///
/// @param l the new linkage name of the method.
void
method_decl::set_linkage_name(const string& l)
{
  decl_base::set_linkage_name(l);
  // Update the linkage_name -> member function map of the containing
  // class declaration.
  if (!l.empty())
    {
      method_type_sptr t = get_type();
      class_or_union_sptr cl = t->get_class_type();
      method_decl_sptr m(this, sptr_utils::noop_deleter());
      cl->priv_->mem_fns_map_[l] = m;
    }
}

method_decl::~method_decl()
{}

const method_type_sptr
method_decl::get_type() const
{
  method_type_sptr result;
  if (function_decl::get_type())
    result = dynamic_pointer_cast<method_type>(function_decl::get_type());
  return result;
}

/// Set the containing class of a method_decl.
///
/// @param scope the new containing class_decl.
void
method_decl::set_scope(scope_decl* scope)
{
  if (!get_context_rel())
    set_context_rel(new mem_fn_context_rel(scope));
  else
    get_context_rel()->set_scope(scope);
}

/// Equality operator for @ref method_decl_sptr.
///
/// This is a deep equality operator, as it compares the @ref
/// method_decl that is pointed-to by the smart pointer.
///
/// @param l the left-hand side argument of the equality operator.
///
/// @param r the righ-hand side argument of the equality operator.
///
/// @return true iff @p l equals @p r.
bool
operator==(const method_decl_sptr& l, const method_decl_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Inequality operator for @ref method_decl_sptr.
///
/// This is a deep equality operator, as it compares the @ref
/// method_decl that is pointed-to by the smart pointer.
///
/// @param l the left-hand side argument of the equality operator.
///
/// @param r the righ-hand side argument of the equality operator.
///
/// @return true iff @p l differs from @p r.
bool
operator!=(const method_decl_sptr& l, const method_decl_sptr& r)
{return !operator==(l, r);}

/// Test if a function_decl is actually a method_decl.
///
///@param d the @ref function_decl to consider.
///
/// @return the method_decl sub-object of @p d if inherits
/// a method_decl type.
method_decl*
is_method_decl(const type_or_decl_base *d)
{
  return dynamic_cast<method_decl*>
    (const_cast<type_or_decl_base*>(d));
}

/// Test if a function_decl is actually a method_decl.
///
///@param d the @ref function_decl to consider.
///
/// @return the method_decl sub-object of @p d if inherits
/// a method_decl type.
method_decl*
is_method_decl(const type_or_decl_base&d)
{return is_method_decl(&d);}

/// Test if a function_decl is actually a method_decl.
///
///@param d the @ref function_decl to consider.
///
/// @return the method_decl sub-object of @p d if inherits
/// a method_decl type.
method_decl_sptr
is_method_decl(const type_or_decl_base_sptr& d)
{return dynamic_pointer_cast<method_decl>(d);}

/// A "less than" functor to sort a vector of instances of
/// method_decl that are virtual.
struct virtual_member_function_less_than
{
  /// The less than operator.  First, it sorts the methods by their
  /// vtable index.  If they have the same vtable index, it sorts them
  /// by the name of their ELF symbol.  If they don't have elf
  /// symbols, it sorts them by considering their pretty
  /// representation.
  ///
  ///  Note that this method expects virtual methods.
  ///
  /// @param f the first method to consider.
  ///
  /// @param s the second method to consider.
  ///
  /// @return true if method @p is less than method @s.
  bool
  operator()(const method_decl& f,
	     const method_decl& s)
  {
    ABG_ASSERT(get_member_function_is_virtual(f));
    ABG_ASSERT(get_member_function_is_virtual(s));

    ssize_t f_offset = get_member_function_vtable_offset(f);
    ssize_t s_offset = get_member_function_vtable_offset(s);
    if (f_offset != s_offset) return f_offset < s_offset;

    string fn, sn;

    // If the functions have symbols, then compare their symbol-id
    // string.
    elf_symbol_sptr f_sym = f.get_symbol();
    elf_symbol_sptr s_sym = s.get_symbol();
    if ((!f_sym) != (!s_sym)) return !f_sym;
    if (f_sym && s_sym)
      {
	fn = f_sym->get_id_string();
	sn = s_sym->get_id_string();
	if (fn != sn) return fn < sn;
      }

    // Try the linkage names (important for destructors).
    fn = f.get_linkage_name();
    sn = s.get_linkage_name();
    if (fn != sn) return fn < sn;

    // None of the functions have symbols or linkage names that
    // distinguish them, so compare their pretty representation.
    fn = f.get_pretty_representation();
    sn = s.get_pretty_representation();
    if (fn != sn) return fn < sn;

    /// If it's just the file paths that are different then sort them
    /// too.
    string fn_filepath, sn_filepath;
    unsigned line = 0, column = 0;
    location fn_loc = f.get_location(), sn_loc = s.get_location();
    if (fn_loc)
      fn_loc.expand(fn_filepath, line, column);
    if (sn_loc)
      sn_loc.expand(sn_filepath, line, column);
    return fn_filepath < sn_filepath;
  }

  /// The less than operator.  First, it sorts the methods by their
  /// vtable index.  If they have the same vtable index, it sorts them
  /// by the name of their ELF symbol.  If they don't have elf
  /// symbols, it sorts them by considering their pretty
  /// representation.
  ///
  ///  Note that this method expects to take virtual methods.
  ///
  /// @param f the first method to consider.
  ///
  /// @param s the second method to consider.
  bool
  operator()(const method_decl_sptr f,
	     const method_decl_sptr s)
  {return operator()(*f, *s);}
}; // end struct virtual_member_function_less_than

/// Sort a vector of instances of virtual member functions.
///
/// @param mem_fns the vector of member functions to sort.
static void
sort_virtual_member_functions(class_decl::member_functions& mem_fns)
{
  virtual_member_function_less_than lt;
  std::stable_sort(mem_fns.begin(), mem_fns.end(), lt);
}

/// Add a member function to the current instance of @ref class_or_union.
///
/// @param f a method_decl to add to the current class.  This function
/// should not have been already added to a scope.
///
/// @param access the access specifier for the member function to add.
///
/// @param is_virtual if this is true then it means the function @p f
/// is a virtual function.  That also means that the current instance
/// of @ref class_or_union is actually an instance of @ref class_decl.
///
/// @param vtable_offset the offset of the member function in the
/// virtual table.  This parameter is taken into account only if @p
/// is_virtual is true.
///
/// @param is_static whether the member function is static.
///
/// @param is_ctor whether the member function is a constructor.
///
/// @param is_dtor whether the member function is a destructor.
///
/// @param is_const whether the member function is const.
void
class_or_union::add_member_function(method_decl_sptr f,
				    access_specifier a,
				    bool is_virtual,
				    size_t vtable_offset,
				    bool is_static, bool is_ctor,
				    bool is_dtor, bool is_const)
{
  add_member_function(f, a, is_static, is_ctor,
		      is_dtor, is_const);

  if (class_decl* klass = is_class_type(this))
    {
      set_member_function_is_virtual(f, is_virtual);
      if (is_virtual)
	{
	  set_member_function_vtable_offset(f, vtable_offset);
	  sort_virtual_member_functions(klass->priv_->virtual_mem_fns_);
	}
    }
}

/// When a virtual member function has seen its virtualness set by
/// set_member_function_is_virtual(), this function ensures that the
/// member function is added to the specific vectors and maps of
/// virtual member function of its class.
///
/// @param method the method to fixup.
void
fixup_virtual_member_function(method_decl_sptr method)
{
  if (!method || !get_member_function_is_virtual(method))
    return;

  class_decl_sptr klass = is_class_type(method->get_type()->get_class_type());

  class_decl::member_functions::const_iterator m;
  for (m = klass->priv_->virtual_mem_fns_.begin();
       m != klass->priv_->virtual_mem_fns_.end();
       ++m)
    if (m->get() == method.get())
      break;
  if (m == klass->priv_->virtual_mem_fns_.end())
    klass->priv_->virtual_mem_fns_.push_back(method);

  // Build or udpate the map that associates a vtable offset to the
  // number of virtual member functions that "point" to it.
  ssize_t voffset = get_member_function_vtable_offset(method);
  if (voffset == -1)
    return;

  class_decl::virtual_mem_fn_map_type::iterator i =
    klass->priv_->virtual_mem_fns_map_.find(voffset);
  if (i == klass->priv_->virtual_mem_fns_map_.end())
    {
      class_decl::member_functions virtual_mem_fns_at_voffset;
      virtual_mem_fns_at_voffset.push_back(method);
      klass->priv_->virtual_mem_fns_map_[voffset] = virtual_mem_fns_at_voffset;
    }
  else
    {
      for (m = i->second.begin() ; m != i->second.end(); ++m)
	if (m->get() == method.get())
	  break;
      if (m == i->second.end())
	i->second.push_back(method);
    }
}

/// Return true iff the class has no entity in its scope.
bool
class_decl::has_no_base_nor_member() const
{return priv_->bases_.empty() && has_no_member();}

/// Test if the current instance of @ref class_decl has virtual member
/// functions.
///
/// @return true iff the current instance of @ref class_decl has
/// virtual member functions.
bool
class_decl::has_virtual_member_functions() const
{return !get_virtual_mem_fns().empty();}

/// Test if the current instance of @ref class_decl has at least one
/// virtual base.
///
/// @return true iff the current instance of @ref class_decl has a
/// virtual member function.
bool
class_decl::has_virtual_bases() const
{
  for (base_specs::const_iterator b = get_base_specifiers().begin();
       b != get_base_specifiers().end();
       ++b)
    if ((*b)->get_is_virtual()
	|| (*b)->get_base_class()->has_virtual_bases())
      return true;

  return false;
}

/// Test if the current instance has a vtable.
///
/// This is only valid for a C++ program.
///
/// Basically this function checks if the class has either virtual
/// functions, or virtual bases.
bool
class_decl::has_vtable() const
{
  if (has_virtual_member_functions()
      || has_virtual_bases())
    return true;
  return false;
}

/// Get the highest vtable offset of all the virtual methods of the
/// class.
///
/// @return the highest vtable offset of all the virtual methods of
/// the class.
ssize_t
class_decl::get_biggest_vtable_offset() const
{
  ssize_t offset = -1;
  for (class_decl::virtual_mem_fn_map_type::const_iterator e =
	 get_virtual_mem_fns_map().begin();
       e != get_virtual_mem_fns_map().end();
       ++e)
    if (e->first > offset)
      offset = e->first;

  return offset;
}

/// Return the hash value for the current instance.
///
/// @return the hash value.
size_t
class_decl::get_hash() const
{
  class_decl::hash hash_class;
  return hash_class(this);
}

/// Test if two methods are equal without taking their symbol or
/// linkage name into account.
///
/// @param f the first method.
///
/// @param s the second method.
///
/// @return true iff @p f equals @p s without taking their linkage
/// name or symbol into account.
static bool
methods_equal_modulo_elf_symbol(const method_decl_sptr& f,
				const method_decl_sptr& s)
{
  method_decl_sptr first = f, second = s;
  elf_symbol_sptr saved_first_elf_symbol =
    first->get_symbol();
  elf_symbol_sptr saved_second_elf_symbol =
    second->get_symbol();
  interned_string saved_first_linkage_name =
    first->get_linkage_name();
  interned_string saved_second_linkage_name =
    second->get_linkage_name();

  first->set_symbol(elf_symbol_sptr());
  first->set_linkage_name("");
  second->set_symbol(elf_symbol_sptr());
  second->set_linkage_name("");

  bool equal = *first == *second;

  first->set_symbol(saved_first_elf_symbol);
  first->set_linkage_name(saved_first_linkage_name);
  second->set_symbol(saved_second_elf_symbol);
  second->set_linkage_name(saved_second_linkage_name);

  return equal;
}

/// Test if a given method is equivalent to at least of other method
/// that is in a vector of methods.
///
/// Note that "equivalent" here means being equal without taking the
/// linkage name or the symbol of the methods into account.
///
/// This is a sub-routine of the 'equals' function that compares @ref
/// class_decl.
///
/// @param method the method to compare.
///
/// @param fns the vector of functions to compare @p method against.
///
/// @return true iff @p is equivalent to at least one method in @p
/// fns.
static bool
method_matches_at_least_one_in_vector(const method_decl_sptr& method,
				      const class_decl::member_functions& fns)
{
  for (class_decl::member_functions::const_iterator i = fns.begin();
       i != fns.end();
       ++i)
    if (methods_equal_modulo_elf_symbol(*i, method))
      return true;

  return false;
}

/// Compares two instances of @ref class_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const class_decl& l, const class_decl& r, change_kind* k)
{
  // if one of the classes is declaration-only then we take a fast
  // path here.
  if (l.get_is_declaration_only() || r.get_is_declaration_only())
    return equals(static_cast<const class_or_union&>(l),
		  static_cast<const class_or_union&>(r),
		  k);

  if (l.class_or_union::priv_->comparison_started(l)
      || l.class_or_union::priv_->comparison_started(r))
    return true;

  bool result = true;
  if (!equals(static_cast<const class_or_union&>(l),
	      static_cast<const class_or_union&>(r),
	      k))
    {
      result = false;
      if (!k)
	return result;
    }

  l.class_or_union::priv_->mark_as_being_compared(l);
  l.class_or_union::priv_->mark_as_being_compared(r);

#define RETURN(value)						\
  do {								\
    l.class_or_union::priv_->unmark_as_being_compared(l);	\
    l.class_or_union::priv_->unmark_as_being_compared(r);	\
    if (value == true)						\
      maybe_propagate_canonical_type(l, r);			\
    return value;						\
  } while(0)

  // Compare bases.
    if (l.get_base_specifiers().size() != r.get_base_specifiers().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_TYPE_CHANGE_KIND;
	else
	  RETURN(result);
      }

    for (class_decl::base_specs::const_iterator
	   b0 = l.get_base_specifiers().begin(),
	   b1 = r.get_base_specifiers().begin();
	 (b0 != l.get_base_specifiers().end()
	 && b1 != r.get_base_specifiers().end());
	 ++b0, ++b1)
      if (*b0 != *b1)
	{
	  result = false;
	  if (k)
	    {
	      if (!types_have_similar_structure((*b0)->get_base_class().get(),
						(*b1)->get_base_class().get()))
		*k |= LOCAL_TYPE_CHANGE_KIND;
	      else
		*k |= SUBTYPE_CHANGE_KIND;
	      break;
	    }
	  RETURN(result);
	}

    // Compare virtual member functions

    // We look at the map that associates a given vtable offset to a
    // vector of virtual member functions that point to that offset.
    //
    // This is because there are cases where several functions can
    // point to the same virtual table offset.
    //
    // This is usually the case for virtual destructors.  Even though
    // there can be only one virtual destructor declared in source
    // code, there are actually potentially up to three generated
    // functions for that destructor.  Some of these generated
    // functions can be clones of other functions that are among those
    // generated ones.  In any cases, they all have the same
    // properties, including the vtable offset property.

    // So, there should be the same number of different vtable
    // offsets, the size of two maps must be equals.
    if (l.get_virtual_mem_fns_map().size()
	!= r.get_virtual_mem_fns_map().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	else
	  RETURN(result);
      }

    // Then, each virtual member function of a given vtable offset in
    // the first class type, must match an equivalent virtual member
    // function of a the same vtable offset in the second class type.
    //
    // By "match", I mean that the two virtual member function should
    // be equal if we don't take into account their symbol name or
    // their linkage name.  This is because two destructor functions
    // clones (for instance) might have different linkage name, but
    // are still equivalent if their other properties are the same.
    for (class_decl::virtual_mem_fn_map_type::const_iterator first_v_fn_entry =
	   l.get_virtual_mem_fns_map().begin();
	 first_v_fn_entry != l.get_virtual_mem_fns_map().end();
	 ++first_v_fn_entry)
      {
	unsigned voffset = first_v_fn_entry->first;
	const class_decl::member_functions& first_vfns =
	  first_v_fn_entry->second;

	const class_decl::virtual_mem_fn_map_type::const_iterator
	  second_v_fn_entry = r.get_virtual_mem_fns_map().find(voffset);

	if (second_v_fn_entry == r.get_virtual_mem_fns_map().end())
	  {
	    result = false;
	    if (k)
	      *k |= LOCAL_NON_TYPE_CHANGE_KIND;
	    RETURN(result);
	  }

	const class_decl::member_functions& second_vfns =
	  second_v_fn_entry->second;

	bool matches = false;
	for (class_decl::member_functions::const_iterator i =
	       first_vfns.begin();
	     i != first_vfns.end();
	     ++i)
	  if (method_matches_at_least_one_in_vector(*i, second_vfns))
	    {
	      matches = true;
	      break;
	    }

	if (!matches)
	  {
	    result = false;
	    if (k)
	      *k |= SUBTYPE_CHANGE_KIND;
	    else
	      RETURN(result);
	  }
      }

  RETURN(result);
#undef RETURN
}

/// Copy a method of a class into a new class.
///
/// @param klass the class into which the method is to be copied.
///
/// @param method the method to copy into @p klass.
///
/// @return the resulting newly copied method.
method_decl_sptr
copy_member_function(const class_decl_sptr& clazz, const method_decl_sptr& f)
{return copy_member_function(static_pointer_cast<class_or_union>(clazz), f);}

/// Copy a method of a class into a new class.
///
/// @param klass the class into which the method is to be copied.
///
/// @param method the method to copy into @p klass.
///
/// @return the resulting newly copied method.
method_decl_sptr
copy_member_function(const class_decl_sptr& clazz, const method_decl* f)
{return copy_member_function(static_pointer_cast<class_or_union>(clazz), f);}

/// Comparison operator for @ref class_decl.
///
/// @param other the instance of @ref class_decl to compare against.
///
/// @return true iff the current instance of @ref class_decl equals @p
/// other.
bool
class_decl::operator==(const decl_base& other) const
{
  const class_decl* op = is_class_type(&other);
  if (!op)
    return false;

  // If this is a decl-only type (and thus with no canonical type),
  // use the canonical type of the definition, if any.
  const class_decl *l = 0;
  if (get_is_declaration_only())
    l = dynamic_cast<const class_decl*>(get_naked_definition_of_declaration());
  if (l == 0)
    l = this;

  ABG_ASSERT(l);

  // Likewise for the other type.
  const class_decl *r = 0;
  if (op->get_is_declaration_only())
    r = dynamic_cast<const class_decl*>(op->get_naked_definition_of_declaration());
  if (r == 0)
    r = op;

  ABG_ASSERT(r);

  return try_canonical_compare(l, r);
}

/// Equality operator for class_decl.
///
/// Re-uses the equality operator that takes a decl_base.
///
/// @param other the other class_decl to compare against.
///
/// @return true iff the current instance equals the other one.
bool
class_decl::operator==(const type_base& other) const
{
  const decl_base* o = is_decl(&other);
  if (!o)
    return false;
  return *this == *o;
}

/// Comparison operator for @ref class_decl.
///
/// @param other the instance of @ref class_decl to compare against.
///
/// @return true iff the current instance of @ref class_decl equals @p
/// other.
bool
class_decl::operator==(const class_decl& other) const
{
  const decl_base& o = other;
  return *this == o;
}

/// Turn equality of shared_ptr of class_decl into a deep equality;
/// that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of class_decl on left-hand-side of the
/// equality.
///
/// @param r the shared_ptr of class_decl on right-hand-side of the
/// equality.
///
/// @return true if the class_decl pointed to by the shared_ptrs are
/// equal, false otherwise.
bool
operator==(const class_decl_sptr& l, const class_decl_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Turn inequality of shared_ptr of class_decl into a deep equality;
/// that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of class_decl on left-hand-side of the
/// equality.
///
/// @param r the shared_ptr of class_decl on right-hand-side of the
/// equality.
///
/// @return true if the class_decl pointed to by the shared_ptrs are
/// different, false otherwise.
bool
operator!=(const class_decl_sptr& l, const class_decl_sptr& r)
{return !operator==(l, r);}

/// Turn equality of shared_ptr of class_or_union into a deep
/// equality; that is, make it compare the pointed to objects too.
///
/// @param l the left-hand-side operand of the operator
///
/// @param r the right-hand-side operand of the operator.
///
/// @return true iff @p l equals @p r.
bool
operator==(const class_or_union_sptr& l, const class_or_union_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Turn inequality of shared_ptr of class_or_union into a deep
/// equality; that is, make it compare the pointed to objects too.
///
/// @param l the left-hand-side operand of the operator
///
/// @param r the right-hand-side operand of the operator.
///
/// @return true iff @p l is different from @p r.
bool
operator!=(const class_or_union_sptr& l, const class_or_union_sptr& r)
{return !operator==(l, r);}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// members.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
class_decl::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      bool stop = false;

      for (base_specs::const_iterator i = get_base_specifiers().begin();
	   i != get_base_specifiers().end();
	   ++i)
	{
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }
	}

      if (!stop)
	for (data_members::const_iterator i = get_data_members().begin();
	     i != get_data_members().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_functions::const_iterator i= get_member_functions().begin();
	     i != get_member_functions().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_types::const_iterator i = get_member_types().begin();
	     i != get_member_types().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_function_templates::const_iterator i =
	       get_member_function_templates().begin();
	     i != get_member_function_templates().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_class_templates::const_iterator i =
	       get_member_class_templates().begin();
	     i != get_member_class_templates().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

/// Destructor of the @ref class_decl type.
class_decl::~class_decl()
{delete priv_;}

context_rel::~context_rel()
{}

bool
member_base::operator==(const member_base& o) const
{
  return (get_access_specifier() == o.get_access_specifier()
	  && get_is_static() == o.get_is_static());
}

/// Equality operator for smart pointers to @ref
/// class_decl::base_specs.
///
/// This compares the pointed-to objects.
///
/// @param l the first instance to consider.
///
/// @param r the second instance to consider.
///
/// @return true iff @p l equals @p r.
bool
operator==(const class_decl::base_spec_sptr& l,
	   const class_decl::base_spec_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == static_cast<const decl_base&>(*r);
}

/// Inequality operator for smart pointers to @ref
/// class_decl::base_specs.
///
/// This compares the pointed-to objects.
///
/// @param l the first instance to consider.
///
/// @param r the second instance to consider.
///
/// @return true iff @p l is different from @p r.
bool
operator!=(const class_decl::base_spec_sptr& l,
	   const class_decl::base_spec_sptr& r)
{return !operator==(l, r);}

/// Test if an ABI artifact is a class base specifier.
///
/// @param tod the ABI artifact to consider.
///
/// @return a pointer to the @ref class_decl::base_spec sub-object of
/// @p tod iff it's a class base specifier.
class_decl::base_spec*
is_class_base_spec(const type_or_decl_base* tod)
{
  return dynamic_cast<class_decl::base_spec*>
    (const_cast<type_or_decl_base*>(tod));
}

/// Test if an ABI artifact is a class base specifier.
///
/// @param tod the ABI artifact to consider.
///
/// @return a pointer to the @ref class_decl::base_spec sub-object of
/// @p tod iff it's a class base specifier.
class_decl::base_spec_sptr
is_class_base_spec(type_or_decl_base_sptr tod)
{return dynamic_pointer_cast<class_decl::base_spec>(tod);}

bool
member_function_template::operator==(const member_base& other) const
{
  try
    {
      const member_function_template& o =
	dynamic_cast<const member_function_template&>(other);

      if (!(is_constructor() == o.is_constructor()
	    && is_const() == o.is_const()
	    && member_base::operator==(o)))
	return false;

      if (function_tdecl_sptr ftdecl = as_function_tdecl())
	{
	  function_tdecl_sptr other_ftdecl = o.as_function_tdecl();
	  if (other_ftdecl)
	    return ftdecl->function_tdecl::operator==(*other_ftdecl);
	}
    }
  catch(...)
    {}
  return false;
}

/// Equality operator for smart pointers to @ref
/// member_function_template.  This is compares the
/// pointed-to instances.
///
/// @param l the first instance to consider.
///
/// @param r the second instance to consider.
///
/// @return true iff @p l equals @p r.
bool
operator==(const member_function_template_sptr& l,
	   const member_function_template_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Inequality operator for smart pointers to @ref
/// member_function_template.  This is compares the pointed-to
/// instances.
///
/// @param l the first instance to consider.
///
/// @param r the second instance to consider.
///
/// @return true iff @p l equals @p r.
bool
operator!=(const member_function_template_sptr& l,
	   const member_function_template_sptr& r)
{return !operator==(l, r);}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// underlying function template.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
member_function_template::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (function_tdecl_sptr f = as_function_tdecl())
	f->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

/// Equality operator of the the @ref member_class_template class.
///
/// @param other the other @ref member_class_template to compare against.
///
/// @return true iff the current instance equals @p other.
bool
member_class_template::operator==(const member_base& other) const
{
  try
    {
      const member_class_template& o =
	dynamic_cast<const member_class_template&>(other);

      if (!member_base::operator==(o))
	return false;

      return as_class_tdecl()->class_tdecl::operator==(o);
    }
  catch(...)
    {return false;}
}

/// Comparison operator for the @ref member_class_template
/// type.
///
/// @param other the other instance of @ref
/// member_class_template to compare against.
///
/// @return true iff the two instances are equal.
bool
member_class_template::operator==(const member_class_template& other) const
{
  const decl_base* o = dynamic_cast<const decl_base*>(&other);
  return *this == *o;
}

/// Comparison operator for the @ref member_class_template
/// type.
///
/// @param l the first argument of the operator.
///
/// @param r the second argument of the operator.
///
/// @return true iff the two instances are equal.
bool
operator==(const member_class_template_sptr& l,
	   const member_class_template_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Inequality operator for the @ref member_class_template
/// type.
///
/// @param l the first argument of the operator.
///
/// @param r the second argument of the operator.
///
/// @return true iff the two instances are equal.
bool
operator!=(const member_class_template_sptr& l,
	   const member_class_template_sptr& r)
{return !operator==(l, r);}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on the class
/// pattern of the template.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
member_class_template::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (class_tdecl_sptr t = as_class_tdecl())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

/// Streaming operator for class_decl::access_specifier.
///
/// @param o the output stream to serialize the access specifier to.
///
/// @param a the access specifier to serialize.
///
/// @return the output stream.
std::ostream&
operator<<(std::ostream& o, access_specifier a)
{
  string r;

  switch (a)
  {
  case no_access:
    r = "none";
    break;
  case private_access:
    r = "private";
    break;
  case protected_access:
    r = "protected";
    break;
  case public_access:
    r= "public";
    break;
  };
  o << r;
  return o;
}

/// Sets the static-ness property of a class member.
///
/// @param d the class member to set the static-ness property for.
/// Note that this must be a class member otherwise the function
/// aborts the current process.
///
/// @param s this must be true if the member is to be static, false
/// otherwise.
void
set_member_is_static(decl_base& d, bool s)
{
  ABG_ASSERT(is_member_decl(d));

  context_rel* c = d.get_context_rel();
  ABG_ASSERT(c);

  c->set_is_static(s);

  scope_decl* scope = d.get_scope();

  if (class_or_union* cl = is_class_or_union_type(scope))
    {
      if (var_decl* v = is_var_decl(&d))
	{
	  if (s)
	    // remove from the non-static data members
	    for (class_decl::data_members::iterator i =
		   cl->priv_->non_static_data_members_.begin();
		 i != cl->priv_->non_static_data_members_.end();
		 ++i)
	      {
		if ((*i)->get_name() == v->get_name())
		  {
		    cl->priv_->non_static_data_members_.erase(i);
		    break;
		  }
	      }
	  else
	    {
	      bool is_already_in_non_static_data_members = false;
	      for (class_or_union::data_members::iterator i =
		     cl->priv_->non_static_data_members_.begin();
		   i != cl->priv_->non_static_data_members_.end();
		   ++i)
	      {
		if ((*i)->get_name() == v->get_name())
		  {
		    is_already_in_non_static_data_members = true;
		    break;
		  }
	      }
	      if (!is_already_in_non_static_data_members)
		{
		  var_decl_sptr var;
		  // add to non-static data members.
		  for (class_or_union::data_members::const_iterator i =
			 cl->priv_->data_members_.begin();
		       i != cl->priv_->data_members_.end();
		       ++i)
		    {
		      if ((*i)->get_name() == v->get_name())
			{
			  var = *i;
			  break;
			}
		    }
		  ABG_ASSERT(var);
		  cl->priv_->non_static_data_members_.push_back(var);
		}
	    }
	}
    }
}

/// Sets the static-ness property of a class member.
///
/// @param d the class member to set the static-ness property for.
/// Note that this must be a class member otherwise the function
/// aborts the current process.
///
/// @param s this must be true if the member is to be static, false
/// otherwise.
void
set_member_is_static(const decl_base_sptr& d, bool s)
{set_member_is_static(*d, s);}

// </class_decl>

// <union_decl>

/// Constructor for the @ref union_decl type.
///
/// @param env the @ref environment we are operating from.
///
/// @param name the name of the union type.
///
/// @param size_in_bits the size of the union, in bits.
///
/// @param locus the location of the type.
///
/// @param vis the visibility of instances of @ref union_decl.
///
/// @param mbr_types the member types of the union.
///
/// @param data_mbrs the data members of the union.
///
/// @param member_fns the member functions of the union.
union_decl::union_decl(const environment* env, const string& name,
		       size_t size_in_bits, const location& locus,
		       visibility vis, member_types& mbr_types,
		       data_members& data_mbrs, member_functions& member_fns)
  : type_or_decl_base(env,
		      UNION_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    decl_base(env, name, locus, name, vis),
    type_base(env, size_in_bits, 0),
    class_or_union(env, name, size_in_bits, 0,
		   locus, vis, mbr_types, data_mbrs, member_fns)
{
  runtime_type_instance(this);
}

/// Constructor for the @ref union_decl type.
///
/// @param env the @ref environment we are operating from.
///
/// @param name the name of the union type.
///
/// @param size_in_bits the size of the union, in bits.
///
/// @param locus the location of the type.
///
/// @param vis the visibility of instances of @ref union_decl.
///
/// @param mbr_types the member types of the union.
///
/// @param data_mbrs the data members of the union.
///
/// @param member_fns the member functions of the union.
///
/// @param is_anonymous whether the newly created instance is
/// anonymous.
union_decl::union_decl(const environment* env, const string& name,
		       size_t size_in_bits, const location& locus,
		       visibility vis, member_types& mbr_types,
		       data_members& data_mbrs, member_functions& member_fns,
		       bool is_anonymous)
  : type_or_decl_base(env,
		      UNION_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE),
    decl_base(env, name, locus,
	      // If the class is anonymous then by default it won't
	      // have a linkage name.  Also, the anonymous class does
	      // have an internal-only unique name that is generally
	      // not taken into account when comparing classes; such a
	      // unique internal-only name, when used as a linkage
	      // name might introduce spurious comparison false
	      // negatives.
	      /*linkage_name=*/is_anonymous ? string() : name,
	      vis),
    type_base(env, size_in_bits, 0),
    class_or_union(env, name, size_in_bits, 0,
		   locus, vis, mbr_types, data_mbrs, member_fns)
{
  runtime_type_instance(this);
  set_is_anonymous(is_anonymous);
}

/// Constructor for the @ref union_decl type.
///
/// @param env the @ref environment we are operating from.
///
/// @param name the name of the union type.
///
/// @param size_in_bits the size of the union, in bits.
///
/// @param locus the location of the type.
///
/// @param vis the visibility of instances of @ref union_decl.
union_decl::union_decl(const environment* env, const string& name,
		       size_t size_in_bits, const location& locus,
		       visibility vis)
  : type_or_decl_base(env,
		      UNION_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, locus, name, vis),
    type_base(env, size_in_bits, 0),
    class_or_union(env, name, size_in_bits,
		   0, locus, vis)
{
  runtime_type_instance(this);
}

/// Constructor for the @ref union_decl type.
///
/// @param env the @ref environment we are operating from.
///
/// @param name the name of the union type.
///
/// @param size_in_bits the size of the union, in bits.
///
/// @param locus the location of the type.
///
/// @param vis the visibility of instances of @ref union_decl.
///
/// @param is_anonymous whether the newly created instance is
/// anonymous.
union_decl::union_decl(const environment* env, const string& name,
		       size_t size_in_bits, const location& locus,
		       visibility vis, bool is_anonymous)
  : type_or_decl_base(env,
		      UNION_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, locus,
	      // If the class is anonymous then by default it won't
	      // have a linkage name.  Also, the anonymous class does
	      // have an internal-only unique name that is generally
	      // not taken into account when comparing classes; such a
	      // unique internal-only name, when used as a linkage
	      // name might introduce spurious comparison false
	      // negatives.
	      /*linkage_name=*/is_anonymous ? string() : name,
	      vis),
    type_base(env, size_in_bits, 0),
    class_or_union(env, name, size_in_bits,
		   0, locus, vis)
{
  runtime_type_instance(this);
  set_is_anonymous(is_anonymous);
}

/// Constructor for the @ref union_decl type.
///
/// @param env the @ref environment we are operating from.
///
/// @param name the name of the union type.
///
/// @param is_declaration_only a boolean saying whether the instance
/// represents a declaration only, or not.
union_decl::union_decl(const environment* env,
		       const string& name,
		       bool is_declaration_only)
  : type_or_decl_base(env,
		      UNION_TYPE
		      | ABSTRACT_TYPE_BASE
		      | ABSTRACT_DECL_BASE
		      | ABSTRACT_SCOPE_TYPE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, name, location(), name),
    type_base(env, 0, 0),
    class_or_union(env, name, is_declaration_only)
{
  runtime_type_instance(this);
}

/// Getter of the pretty representation of the current instance of
/// @ref union_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qualified_name if true, names emitted in the pretty
/// representation are fully qualified.
///
/// @return the pretty representaion for a union_decl.
string
union_decl::get_pretty_representation(bool internal,
				      bool qualified_name) const
{
  string repr;
  if (get_is_anonymous())
    repr = get_class_or_union_flat_representation(this, "",
						  /*one_line=*/true,
						  internal);
  else
    {
      repr = "union ";
      if (qualified_name)
	repr += get_qualified_name(internal);
      else
	repr += get_name();
    }

  return repr;
}

/// Comparison operator for @ref union_decl.
///
/// @param other the instance of @ref union_decl to compare against.
///
/// @return true iff the current instance of @ref union_decl equals @p
/// other.
bool
union_decl::operator==(const decl_base& other) const
{
  const union_decl* op = dynamic_cast<const union_decl*>(&other);
  if (!op)
    return false;
  return try_canonical_compare(this, op);
}

/// Equality operator for union_decl.
///
/// Re-uses the equality operator that takes a decl_base.
///
/// @param other the other union_decl to compare against.
///
/// @return true iff the current instance equals the other one.
bool
union_decl::operator==(const type_base& other) const
{
  const decl_base *o = dynamic_cast<const decl_base*>(&other);
  if (!o)
    return false;
  return *this == *o;
}

/// Comparison operator for @ref union_decl.
///
/// @param other the instance of @ref union_decl to compare against.
///
/// @return true iff the current instance of @ref union_decl equals @p
/// other.
bool
union_decl::operator==(const union_decl& other) const
{
  const decl_base& o = other;
  return *this == o;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// members.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
union_decl::traverse(ir_node_visitor& v)
{
  if (v.type_node_has_been_visited(this))
    return true;

  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      bool stop = false;

      if (!stop)
	for (data_members::const_iterator i = get_data_members().begin();
	     i != get_data_members().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_functions::const_iterator i= get_member_functions().begin();
	     i != get_member_functions().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_types::const_iterator i = get_member_types().begin();
	     i != get_member_types().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_function_templates::const_iterator i =
	       get_member_function_templates().begin();
	     i != get_member_function_templates().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_class_templates::const_iterator i =
	       get_member_class_templates().begin();
	     i != get_member_class_templates().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }
      visiting(false);
    }

  bool result = v.visit_end(this);
  v.mark_type_node_as_visited(this);
  return result;
}

/// Destructor of the @ref union_decl type.
union_decl::~union_decl()
{}

/// Compares two instances of @ref union_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const union_decl& l, const union_decl& r, change_kind* k)
{
  bool result = equals(static_cast<const class_or_union&>(l),
		       static_cast<const class_or_union&>(r),
		       k);
  if (result == true)
    maybe_propagate_canonical_type(l, r);
  return result;
}

/// Copy a method of a @ref union_decl into a new @ref
/// union_decl.
///
/// @param t the @ref union_decl into which the method is to be copied.
///
/// @param method the method to copy into @p t.
///
/// @return the resulting newly copied method.
method_decl_sptr
copy_member_function(const union_decl_sptr& union_type,
		     const method_decl_sptr& f)
{return copy_member_function(union_type, f.get());}

/// Copy a method of a @ref union_decl into a new @ref
/// union_decl.
///
/// @param t the @ref union_decl into which the method is to be copied.
///
/// @param method the method to copy into @p t.
///
/// @return the resulting newly copied method.
method_decl_sptr
copy_member_function(const union_decl_sptr& union_type,
		     const method_decl* f)
{
  const class_or_union_sptr t = union_type;
  return copy_member_function(t, f);
}

/// Turn equality of shared_ptr of union_decl into a deep equality;
/// that is, make it compare the pointed to objects too.
///
/// @param l the left-hand-side operand of the operator
///
/// @param r the right-hand-side operand of the operator.
///
/// @return true iff @p l equals @p r.
bool
operator==(const union_decl_sptr& l, const union_decl_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Turn inequality of shared_ptr of union_decl into a deep equality;
/// that is, make it compare the pointed to objects too.
///
/// @param l the left-hand-side operand of the operator
///
/// @param r the right-hand-side operand of the operator.
///
/// @return true iff @p l is different from @p r.
bool
operator!=(const union_decl_sptr& l, const union_decl_sptr& r)
{return !operator==(l, r);}
// </union_decl>

// <template_decl stuff>

/// Data type of the private data of the @template_decl type.
class template_decl::priv
{
  friend class template_decl;

  std::list<template_parameter_sptr> parms_;
public:

  priv()
  {}
}; // end class template_decl::priv

/// Add a new template parameter to the current instance of @ref
/// template_decl.
///
/// @param p the new template parameter to add.
void
template_decl::add_template_parameter(const template_parameter_sptr p)
{priv_->parms_.push_back(p);}

/// Get the list of template parameters of the current instance of
/// @ref template_decl.
///
/// @return the list of template parameters.
const std::list<template_parameter_sptr>&
template_decl::get_template_parameters() const
{return priv_->parms_;}

/// Constructor.
///
/// @param env the environment we are operating from.
///
/// @param name the name of the template decl.
///
/// @param locus the source location where the template declaration is
/// defined.
///
/// @param vis the visibility of the template declaration.
template_decl::template_decl(const environment* env,
			     const string& name,
			     const location& locus,
			     visibility vis)
  : type_or_decl_base(env, TEMPLATE_DECL | ABSTRACT_DECL_BASE),
    decl_base(env, name, locus, /*mangled_name=*/"", vis),
    priv_(new priv)
{
  runtime_type_instance(this);
}

/// Destructor.
template_decl::~template_decl()
{}

/// Equality operator.
///
/// @param o the other instance to compare against.
///
/// @return true iff @p equals the current instance.
bool
template_decl::operator==(const template_decl& o) const
{
  try
    {
      list<shared_ptr<template_parameter> >::const_iterator t0, t1;
      for (t0 = get_template_parameters().begin(),
	     t1 = o.get_template_parameters().begin();
	   (t0 != get_template_parameters().end()
	    && t1 != o.get_template_parameters().end());
	   ++t0, ++t1)
	{
	  if (**t0 != **t1)
	    return false;
	}

      if (t0 != get_template_parameters().end()
	  || t1 != o.get_template_parameters().end())
	return false;

      return true;
    }
  catch(...)
    {return false;}
}

// </template_decl stuff>

//<template_parameter>

/// The type of the private data of the @ref template_parameter type.
class template_parameter::priv
{
  friend class template_parameter;

  unsigned index_;
  template_decl_wptr template_decl_;
  mutable bool hashing_started_;
  mutable bool comparison_started_;

  priv();

public:

  priv(unsigned index, template_decl_sptr enclosing_template_decl)
    : index_(index),
      template_decl_(enclosing_template_decl),
      hashing_started_(),
      comparison_started_()
  {}
}; // end class template_parameter::priv

template_parameter::template_parameter(unsigned	 index,
				       template_decl_sptr enclosing_template)
  : priv_(new priv(index, enclosing_template))
  {}

unsigned
template_parameter::get_index() const
{return priv_->index_;}

const template_decl_sptr
template_parameter::get_enclosing_template_decl() const
{return priv_->template_decl_.lock();}

bool
template_parameter::get_hashing_has_started() const
{return priv_->hashing_started_;}

void
template_parameter::set_hashing_has_started(bool f) const
{priv_->hashing_started_ = f;}

bool
template_parameter::operator==(const template_parameter& o) const
{
  if (get_index() != o.get_index())
    return false;

  if (priv_->comparison_started_)
    return true;

  bool result = false;

  // Avoid inifite loops due to the fact that comparison the enclosing
  // template decl might lead to comparing this very same template
  // parameter with another one ...
  priv_->comparison_started_ = true;

  if (!!get_enclosing_template_decl() != !!o.get_enclosing_template_decl())
    ;
  else if (get_enclosing_template_decl()
	   && (*get_enclosing_template_decl()
	       != *o.get_enclosing_template_decl()))
    ;
  else
    result = true;

  priv_->comparison_started_ = false;

  return result;
}

/// Inequality operator.
///
/// @param other the other instance to compare against.
///
/// @return true iff the other instance is different from the current
/// one.
bool
template_parameter::operator!=(const template_parameter& other) const
{return !operator==(other);}

/// Destructor.
template_parameter::~template_parameter()
{}

/// The type of the private data of the @ref type_tparameter type.
class type_tparameter::priv
{
  friend class type_tparameter;
}; // end class type_tparameter::priv

/// Constructor of the @ref type_tparameter type.
///
/// @param index the index the type template parameter.
///
/// @param enclosing_tdecl the enclosing template declaration.
///
/// @param name the name of the template parameter.
///
/// @param locus the location of the declaration of this type template
/// parameter.
type_tparameter::type_tparameter(unsigned		index,
				 template_decl_sptr	enclosing_tdecl,
				 const string&		name,
				 const location&	locus)
  : type_or_decl_base(enclosing_tdecl->get_environment(),
		      ABSTRACT_DECL_BASE
		      | ABSTRACT_TYPE_BASE
		      | BASIC_TYPE),
    decl_base(enclosing_tdecl->get_environment(), name, locus),
    type_base(enclosing_tdecl->get_environment(), 0, 0),
    type_decl(enclosing_tdecl->get_environment(), name, 0, 0, locus),
    template_parameter(index, enclosing_tdecl),
    priv_(new priv)
{
  runtime_type_instance(this);
}

bool
type_tparameter::operator==(const type_base& other) const
{
  if (!type_decl::operator==(other))
    return false;

  try
    {
      const type_tparameter& o = dynamic_cast<const type_tparameter&>(other);
      return template_parameter::operator==(o);
    }
  catch (...)
    {return false;}
}

bool
type_tparameter::operator==(const template_parameter& other) const
{
  try
    {
      const type_base& o = dynamic_cast<const type_base&>(other);
      return *this == o;
    }
  catch(...)
    {return false;}
}

bool
type_tparameter::operator==(const type_tparameter& other) const
{return *this == static_cast<const type_base&>(other);}

type_tparameter::~type_tparameter()
{}

/// The type of the private data of the @ref non_type_tparameter type.
class non_type_tparameter::priv
{
  friend class non_type_tparameter;

  type_base_wptr type_;

  priv();

public:

  priv(type_base_sptr type)
    : type_(type)
  {}
}; // end class non_type_tparameter::priv

/// The constructor for the @ref non_type_tparameter type.
///
/// @param index the index of the template parameter.
///
/// @param enclosing_tdecl the enclosing template declaration that
/// holds this parameter parameter.
///
/// @param name the name of the template parameter.
///
/// @param type the type of the template parameter.
///
/// @param locus the location of the declaration of this template
/// parameter.
non_type_tparameter::non_type_tparameter(unsigned		index,
					 template_decl_sptr	enclosing_tdecl,
					 const string&		name,
					 type_base_sptr	type,
					 const location&	locus)
  : type_or_decl_base(type->get_environment(), ABSTRACT_DECL_BASE),
    decl_base(type->get_environment(), name, locus, ""),
    template_parameter(index, enclosing_tdecl),
    priv_(new priv(type))
{
  runtime_type_instance(this);
}

/// Getter for the type of the template parameter.
///
/// @return the type of the template parameter.
const type_base_sptr
non_type_tparameter::get_type() const
{return priv_->type_.lock();}

/// Get the hash value of the current instance.
///
/// @return the hash value.
size_t
non_type_tparameter::get_hash() const
{
  non_type_tparameter::hash hash_tparm;
  return hash_tparm(this);
}

bool
non_type_tparameter::operator==(const decl_base& other) const
{
  if (!decl_base::operator==(other))
    return false;

  try
    {
      const non_type_tparameter& o =
	dynamic_cast<const non_type_tparameter&>(other);
      return (template_parameter::operator==(o)
	      && get_type() == o.get_type());
    }
  catch(...)
    {return false;}
}

bool
non_type_tparameter::operator==(const template_parameter& other) const
{
  try
    {
      const decl_base& o = dynamic_cast<const decl_base&>(other);
      return *this == o;
    }
  catch(...)
    {return false;}
}

non_type_tparameter::~non_type_tparameter()
{}

// <template_tparameter stuff>

/// Type of the private data of the @ref template_tparameter type.
class template_tparameter::priv
{
}; //end class template_tparameter::priv

/// Constructor for the @ref template_tparameter.
///
/// @param index the index of the template parameter.
///
/// @param enclosing_tdecl the enclosing template declaration.
///
/// @param name the name of the template parameter.
///
/// @param locus the location of the declaration of the template
/// parameter.
template_tparameter::template_tparameter(unsigned		index,
					 template_decl_sptr	enclosing_tdecl,
					 const string&		name,
					 const location&	locus)
  : type_or_decl_base(enclosing_tdecl->get_environment(),
		      ABSTRACT_DECL_BASE
		      | ABSTRACT_TYPE_BASE
		      | BASIC_TYPE),
    decl_base(enclosing_tdecl->get_environment(), name, locus),
    type_base(enclosing_tdecl->get_environment(), 0, 0),
    type_decl(enclosing_tdecl->get_environment(), name,
	      0, 0, locus, name, VISIBILITY_DEFAULT),
    type_tparameter(index, enclosing_tdecl, name, locus),
    template_decl(enclosing_tdecl->get_environment(), name, locus),
    priv_(new priv)
{
  runtime_type_instance(this);
}

bool
template_tparameter::operator==(const type_base& other) const
{
  try
    {
      const template_tparameter& o =
	dynamic_cast<const template_tparameter&>(other);
      return (type_tparameter::operator==(o)
	      && template_decl::operator==(o));
    }
  catch(...)
    {return false;}
}

bool
template_tparameter::operator==(const template_parameter& o) const
{
  try
    {
      const template_tparameter& other =
	dynamic_cast<const template_tparameter&>(o);
      return *this == static_cast<const type_base&>(other);
    }
  catch(...)
    {return false;}
}

bool
template_tparameter::operator==(const template_decl& o) const
{
  try
    {
      const template_tparameter& other =
	dynamic_cast<const template_tparameter&>(o);
      return type_base::operator==(other);
    }
  catch(...)
    {return false;}
}

template_tparameter::~template_tparameter()
{}

// </template_tparameter stuff>

// <type_composition stuff>

/// The type of the private data of the @ref type_composition type.
class type_composition::priv
{
  friend class type_composition;

  type_base_wptr type_;

  // Forbid this.
  priv();

public:

  priv(type_base_wptr type)
    : type_(type)
  {}
}; //end class type_composition::priv

/// Constructor for the @ref type_composition type.
///
/// @param index the index of the template type composition.
///
/// @param tdecl the enclosing template parameter that owns the
/// composition.
///
/// @param t the resulting type.
type_composition::type_composition(unsigned		index,
				   template_decl_sptr	tdecl,
				   type_base_sptr	t)
  : type_or_decl_base(tdecl->get_environment(),
		      ABSTRACT_DECL_BASE),
    decl_base(tdecl->get_environment(), "", location()),
    template_parameter(index, tdecl),
    priv_(new priv(t))
{
  runtime_type_instance(this);
}

/// Getter for the resulting composed type.
///
/// @return the composed type.
const type_base_sptr
type_composition::get_composed_type() const
{return priv_->type_.lock();}

/// Setter for the resulting composed type.
///
/// @param t the composed type.
void
type_composition::set_composed_type(type_base_sptr t)
{priv_->type_ = t;}

/// Get the hash value for the current instance.
///
/// @return the hash value.
size_t
type_composition::get_hash() const
{
  type_composition::hash hash_type_composition;
  return hash_type_composition(this);
}

type_composition::~type_composition()
{}

// </type_composition stuff>

//</template_parameter stuff>

// <function_template>

class function_tdecl::priv
{
  friend class function_tdecl;

  function_decl_sptr pattern_;
  binding binding_;

  priv();

public:

  priv(function_decl_sptr pattern, binding bind)
    : pattern_(pattern), binding_(bind)
  {}

  priv(binding bind)
    : binding_(bind)
  {}
}; // end class function_tdecl::priv

/// Constructor for a function template declaration.
///
/// @param env the environment we are operating from.
///
/// @param locus the location of the declaration.
///
/// @param vis the visibility of the declaration.  This is the
/// visibility the functions instantiated from this template are going
/// to have.
///
/// @param bind the binding of the declaration.  This is the binding
/// the functions instantiated from this template are going to have.
function_tdecl::function_tdecl(const environment*	env,
			       const location&		locus,
			       visibility		vis,
			       binding			bind)
  : type_or_decl_base(env,
		      ABSTRACT_DECL_BASE
		      | TEMPLATE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, "", locus, "", vis),
    template_decl(env, "", locus, vis),
    scope_decl(env, "", locus),
    priv_(new priv(bind))
{
  runtime_type_instance(this);
}

/// Constructor for a function template declaration.
///
/// @param pattern the pattern of the template.
///
/// @param locus the location of the declaration.
///
/// @param vis the visibility of the declaration.  This is the
/// visibility the functions instantiated from this template are going
/// to have.
///
/// @param bind the binding of the declaration.  This is the binding
/// the functions instantiated from this template are going to have.
function_tdecl::function_tdecl(function_decl_sptr	pattern,
			       const location&		locus,
			       visibility		vis,
			       binding			bind)
  : type_or_decl_base(pattern->get_environment(),
		      ABSTRACT_DECL_BASE
		      | TEMPLATE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(pattern->get_environment(), pattern->get_name(), locus,
	      pattern->get_name(), vis),
    template_decl(pattern->get_environment(), pattern->get_name(), locus, vis),
    scope_decl(pattern->get_environment(), pattern->get_name(), locus),
    priv_(new priv(pattern, bind))
{
  runtime_type_instance(this);
}

/// Set a new pattern to the function template.
///
/// @param p the new pattern.
void
function_tdecl::set_pattern(function_decl_sptr p)
{
  priv_->pattern_ = p;
  add_decl_to_scope(p, this);
  set_name(p->get_name());
}

/// Get the pattern of the function template.
///
/// @return the pattern.
function_decl_sptr
function_tdecl::get_pattern() const
{return priv_->pattern_;}

/// Get the binding of the function template.
///
/// @return the binding
decl_base::binding
function_tdecl::get_binding() const
{return priv_->binding_;}

/// Comparison operator for the @ref function_tdecl type.
///
/// @param other the other instance of @ref function_tdecl to compare against.
///
/// @return true iff the two instance are equal.
bool
function_tdecl::operator==(const decl_base& other) const
{
  const function_tdecl* o = dynamic_cast<const function_tdecl*>(&other);
  if (o)
    return *this == *o;
  return false;
}

/// Comparison operator for the @ref function_tdecl type.
///
/// @param other the other instance of @ref function_tdecl to compare against.
///
/// @return true iff the two instance are equal.
bool
function_tdecl::operator==(const template_decl& other) const
{
  const function_tdecl* o = dynamic_cast<const function_tdecl*>(&other);
  if (o)
    return *this == *o;
  return false;
}

/// Comparison operator for the @ref function_tdecl type.
///
/// @param o the other instance of @ref function_tdecl to compare against.
///
/// @return true iff the two instance are equal.
bool
function_tdecl::operator==(const function_tdecl& o) const
{
  if (!(get_binding() == o.get_binding()
	&& template_decl::operator==(o)
	&& scope_decl::operator==(o)
	&& !!get_pattern() == !!o.get_pattern()))
    return false;

  if (get_pattern())
    return (*get_pattern() == *o.get_pattern());

  return true;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on the
/// function pattern of the template.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
function_tdecl::traverse(ir_node_visitor&v)
{
  if (visiting())
    return true;

  if (!v.visit_begin(this))
    {
      visiting(true);
      if (get_pattern())
	get_pattern()->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

function_tdecl::~function_tdecl()
{}

// </function_template>

// <class template>

/// Type of the private data of the the @ref class_tdecl type.
class class_tdecl::priv
{
  friend class class_tdecl;
  class_decl_sptr pattern_;

public:

  priv()
  {}

  priv(class_decl_sptr pattern)
    : pattern_(pattern)
  {}
}; // end class class_tdecl::priv

/// Constructor for the @ref class_tdecl type.
///
/// @param env the environment we are operating from.
///
/// @param locus the location of the declaration of the class_tdecl
/// type.
///
/// @param vis the visibility of the instance of class instantiated
/// from this template.
class_tdecl::class_tdecl(const environment*	env,
			 const location&	locus,
			 visibility		vis)
  : type_or_decl_base(env,
		      ABSTRACT_DECL_BASE
		      | TEMPLATE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(env, "", locus, "", vis),
    template_decl(env, "", locus, vis),
    scope_decl(env, "", locus),
    priv_(new priv)
{
  runtime_type_instance(this);
}

/// Constructor for the @ref class_tdecl type.
///
/// @param pattern The details of the class template. This must NOT be a
/// null pointer.  If you really this to be null, please use the
/// constructor above instead.
///
/// @param locus the source location of the declaration of the type.
///
/// @param vis the visibility of the instances of class instantiated
/// from this template.
class_tdecl::class_tdecl(class_decl_sptr	pattern,
			 const location&	locus,
			 visibility		vis)
  : type_or_decl_base(pattern->get_environment(),
		      ABSTRACT_DECL_BASE
		      | TEMPLATE_DECL
		      | ABSTRACT_SCOPE_DECL),
    decl_base(pattern->get_environment(), pattern->get_name(),
	      locus, pattern->get_name(), vis),
    template_decl(pattern->get_environment(), pattern->get_name(), locus, vis),
    scope_decl(pattern->get_environment(), pattern->get_name(), locus),
    priv_(new priv(pattern))
{
  runtime_type_instance(this);
}

/// Setter of the pattern of the template.
///
/// @param p the new template.
void
class_tdecl::set_pattern(class_decl_sptr p)
{
  priv_->pattern_ = p;
  add_decl_to_scope(p, this);
  set_name(p->get_name());
}

/// Getter of the pattern of the template.
///
/// @return p the new template.
class_decl_sptr
class_tdecl::get_pattern() const
{return priv_->pattern_;}

bool
class_tdecl::operator==(const decl_base& other) const
{
  try
    {
      const class_tdecl& o = dynamic_cast<const class_tdecl&>(other);

      if (!(template_decl::operator==(o)
	    && scope_decl::operator==(o)
	    && !!get_pattern() == !!o.get_pattern()))
	return false;

      if (!get_pattern() || !o.get_pattern())
	return true;

      return get_pattern()->decl_base::operator==(*o.get_pattern());
    }
  catch(...) {}
  return false;
}

bool
class_tdecl::operator==(const template_decl& other) const
{
  try
    {
      const class_tdecl& o = dynamic_cast<const class_tdecl&>(other);
      return *this == static_cast<const decl_base&>(o);
    }
  catch(...)
    {return false;}
}

bool
class_tdecl::operator==(const class_tdecl& o) const
{return *this == static_cast<const decl_base&>(o);}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on the class
/// pattern of the template.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
class_tdecl::traverse(ir_node_visitor&v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (class_decl_sptr pattern = get_pattern())
	pattern->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

class_tdecl::~class_tdecl()
{}

/// This visitor checks if a given type as non-canonicalized sub
/// types.
class non_canonicalized_subtype_detector : public ir::ir_node_visitor
{
  type_base* type_;
  type_base* has_non_canonical_type_;

private:
  non_canonicalized_subtype_detector();

public:
  non_canonicalized_subtype_detector(type_base* type)
    : type_(type),
      has_non_canonical_type_()
  {}

  /// Return true if the visitor detected that there is a
  /// non-canonicalized sub-type.
  ///
  /// @return true if the visitor detected that there is a
  /// non-canonicalized sub-type.
  type_base*
  has_non_canonical_type() const
  {return has_non_canonical_type_;}

  /// The intent of this visitor handler is to avoid looking into
  /// sub-types of member functions of the type we are traversing.
  bool
  visit_begin(function_decl* f)
  {
    // Do not look at sub-types of non-virtual member functions.
    if (is_member_function(f)
	&& get_member_function_is_virtual(*f))
      return false;
    return true;
  }

  /// When visiting a sub-type, if it's *NOT* been canonicalized, set
  /// the 'has_non_canonical_type' flag.  And in any case, when
  /// visiting a sub-type, do not visit its children nodes.  So this
  /// function only goes to the level below the level of the top-most
  /// type.
  ///
  /// @return true if we are at the same level as the top-most type,
  /// otherwise return false.
  bool
  visit_begin(type_base* t)
  {
    if (t != type_)
      {
	if (!t->get_canonical_type())
	  // We are looking a sub-type of 'type_' which has no
	  // canonical type.  So tada! we found one!  Get out right
	  // now with the trophy.
	  has_non_canonical_type_ = t;

	return false;
      }
    return true;
  }

  /// When we are done visiting a sub-type, if it's been flagged as
  /// been non-canonicalized, then stop the traversing.
  ///
  /// Otherwise, keep going.
  ///
  /// @return false iff the sub-type that has been visited is
  /// non-canonicalized.
  bool
  visit_end(type_base* )
  {
    if (has_non_canonical_type_)
      return false;
    return true;
  }
}; //end class non_canonicalized_subtype_detector

/// Test if a type has sub-types that are non-canonicalized.
///
/// @param t the type which sub-types to consider.
///
/// @return true if a type has sub-types that are non-canonicalized.
type_base*
type_has_non_canonicalized_subtype(type_base_sptr t)
{
  if (!t)
    return 0;

  non_canonicalized_subtype_detector v(t.get());
  t->traverse(v);
  return v.has_non_canonical_type();
}

/// Tests if the change of a given type effectively comes from just
/// its sub-types.  That is, if the type has changed but its type name
/// hasn't changed, then the change of the type mostly likely is a
/// sub-type change.
///
/// @param t_v1 the first version of the type.
///
/// @param t_v2 the second version of the type.
///
/// @return true iff the type changed and the change is about its
/// sub-types.
bool
type_has_sub_type_changes(const type_base_sptr t_v1,
			  const type_base_sptr t_v2)
{
  type_base_sptr t1 = strip_typedef(t_v1);
  type_base_sptr t2 = strip_typedef(t_v2);

  string repr1 = get_pretty_representation(t1),
    repr2 = get_pretty_representation(t2);
  return (t1 != t2 && repr1 == repr2);
}

/// Make sure that the life time of a given (smart pointer to a) type
/// is the same as the life time of the libabigail library.
///
/// @param t the type to consider.
void
keep_type_alive(type_base_sptr t)
{
  environment* env = t->get_environment();
  ABG_ASSERT(env);
  env->priv_->extra_live_types_.push_back(t);
}

/// Hash an ABI artifact that is either a type or a decl.
///
/// This function intends to provides the fastest possible hashing for
/// types and decls, while being completely correct.
///
/// Note that if the artifact is a type and if it has a canonical
/// type, the hash value is going to be the pointer value of the
/// canonical type.  Otherwise, this function computes a hash value
/// for the type by recursively walking the type members.  This last
/// code path is possibly *very* slow and should only be used when
/// only handful of types are going to be hashed.
///
/// If the artifact is a decl, then a combination of the hash of its
/// type and the hash of the other properties of the decl is computed.
///
/// @param tod the type or decl to hash.
///
/// @return the resulting hash value.
size_t
hash_type_or_decl(const type_or_decl_base *tod)
{
  size_t result = 0;

  if (tod == 0)
    ;
  else if (const type_base* t = is_type(tod))
    result = hash_type(t);
  else if (const decl_base* d = is_decl(tod))
    {
      if (var_decl* v = is_var_decl(d))
	{
	  ABG_ASSERT(v->get_type());
	  size_t h = hash_type_or_decl(v->get_type());
	  string repr = v->get_pretty_representation();
	  std::hash<string> hash_string;
	  h = hashing::combine_hashes(h, hash_string(repr));
	  result = h;
	}
      else if (function_decl* f = is_function_decl(d))
	{
	  ABG_ASSERT(f->get_type());
	  size_t h = hash_type_or_decl(f->get_type());
	  string repr = f->get_pretty_representation();
	  std::hash<string> hash_string;
	  h = hashing::combine_hashes(h, hash_string(repr));
	  result = h;
	}
      else if (function_decl::parameter* p = is_function_parameter(d))
	{
	  type_base_sptr parm_type = p->get_type();
	  ABG_ASSERT(parm_type);
	  std::hash<bool> hash_bool;
	  std::hash<unsigned> hash_unsigned;
	  size_t h = hash_type_or_decl(parm_type);
	  h = hashing::combine_hashes(h, hash_unsigned(p->get_index()));
	  h = hashing::combine_hashes(h, hash_bool(p->get_variadic_marker()));
	  result = h;
	}
      else if (class_decl::base_spec *bs = is_class_base_spec(d))
	{
	  member_base::hash hash_member;
	  std::hash<size_t> hash_size;
	  std::hash<bool> hash_bool;
	  type_base_sptr type = bs->get_base_class();
	  size_t h = hash_type_or_decl(type);
	  h = hashing::combine_hashes(h, hash_member(*bs));
	  h = hashing::combine_hashes(h, hash_size(bs->get_offset_in_bits()));
	  h = hashing::combine_hashes(h, hash_bool(bs->get_is_virtual()));
	  result = h;
	}
      else
	// This is a *really* *SLOW* path.  If it shows up in a
	// performance profile, I bet it'd be a good idea to try to
	// avoid it altogether.
	result = d->get_hash();
    }
  else
    // We should never get here.
    abort();
  return result;
}

/// Hash an ABI artifact that is either a type.
///
/// This function intends to provides the fastest possible hashing for
/// types while being completely correct.
///
/// Note that if the type artifact has a canonical type, the hash
/// value is going to be the pointer value of the canonical type.
/// Otherwise, this function computes a hash value for the type by
/// recursively walking the type members.  This last code path is
/// possibly *very* slow and should only be used when only handful of
/// types are going to be hashed.
///
/// @param t the type or decl to hash.
///
/// @return the resulting hash value.
size_t
hash_type(const type_base *t)
{return hash_as_canonical_type_or_constant(t);}

/// Hash an ABI artifact that is either a type of a decl.
///
/// @param tod the ABI artifact to hash.
///
/// @return the hash value of the ABI artifact.
size_t
hash_type_or_decl(const type_or_decl_base_sptr& tod)
{return hash_type_or_decl(tod.get());}

/// Hash a type by either returning the pointer value of its canonical
/// type or by returning a constant if the type doesn't have a
/// canonical type.
///
/// This is a subroutine of hash_type.
///
/// @param t the type to consider.
///
/// @return the hash value.
static size_t
hash_as_canonical_type_or_constant(const type_base *t)
{
  type_base *canonical_type = 0;

  if (t)
    canonical_type = t->get_naked_canonical_type();

  if (!canonical_type)
    {
      // If the type doesn't have a canonical type, maybe it's because
      // it's a declaration-only type?  If that's the case, let's try
      // to get the canonical type of the definition of this
      // declaration.
      decl_base *decl = is_decl(t);
      if (decl
	  && decl->get_is_declaration_only()
	  && decl->get_naked_definition_of_declaration())
	{
	  type_base *definition =
	    is_type(decl->get_naked_definition_of_declaration());
	  ABG_ASSERT(definition);
	  canonical_type = definition->get_naked_canonical_type();
	}
    }

  if (canonical_type)
    return reinterpret_cast<size_t>(canonical_type);

  // If we reached this point, it means we are seeing a
  // non-canonicalized type.  It must be a decl-only class or a
  // function type, otherwise it means that for some weird reason, the
  // type hasn't been canonicalized.  It should be!
  ABG_ASSERT(is_declaration_only_class_or_union_type(t));

  return 0xDEADBABE;
}

/// Test if the pretty representation of a given @ref function_decl is
/// lexicographically less then the pretty representation of another
/// @ref function_decl.
///
/// @param f the first @ref function_decl to consider for comparison.
///
/// @param s the second @ref function_decl to consider for comparison.
///
/// @return true iff the pretty representation of @p f is
/// lexicographically less than the pretty representation of @p s.
bool
function_decl_is_less_than(const function_decl &f, const function_decl &s)
{
  string fr = f.get_pretty_representation_of_declarator(),
    sr = s.get_pretty_representation_of_declarator();

  if (fr != sr)
    return fr < sr;

  fr = f.get_pretty_representation(),
    sr = s.get_pretty_representation();

  if (fr != sr)
    return fr < sr;

  if (f.get_symbol())
    fr = f.get_symbol()->get_id_string();
  else if (!f.get_linkage_name().empty())
    fr = f.get_linkage_name();

  if (s.get_symbol())
    sr = s.get_symbol()->get_id_string();
  else if (!s.get_linkage_name().empty())
    sr = s.get_linkage_name();

  return fr < sr;
}

/// Test if two types have similar structures, even though they are
/// (or can be) different.
///
/// const and volatile qualifiers are completely ignored.
///
/// typedef are resolved to their definitions; their names are ignored.
///
/// Two indirect types (pointers or references) have similar structure
/// if their underlying types are of the same kind and have the same
/// name.  In the indirect types case, the size of the underlying type
/// does not matter.
///
/// Two direct types (i.e, non indirect) have a similar structure if
/// they have the same kind, name and size.  Two class types have
/// similar structure if they have the same name, size, and if the
/// types of their data members have similar types.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param indirect_type whether to do an indirect comparison
///
/// @return true iff @p first and @p second have similar structures.
bool
types_have_similar_structure(const type_base_sptr& first,
			     const type_base_sptr& second,
			     bool indirect_type)
{return types_have_similar_structure(first.get(), second.get(), indirect_type);}

/// Test if two types have similar structures, even though they are
/// (or can be) different.
///
/// const and volatile qualifiers are completely ignored.
///
/// typedef are resolved to their definitions; their names are ignored.
///
/// Two indirect types (pointers, references or arrays) have similar
/// structure if their underlying types are of the same kind and have
/// the same name.  In the indirect types case, the size of the
/// underlying type does not matter.
///
/// Two direct types (i.e, non indirect) have a similar structure if
/// they have the same kind, name and size.  Two class types have
/// similar structure if they have the same name, size, and if the
/// types of their data members have similar types.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param indirect_type if true, then consider @p first and @p
/// second as being underlying types of indirect types.  Meaning that
/// their size does not matter.
///
/// @return true iff @p first and @p second have similar structures.
bool
types_have_similar_structure(const type_base* first,
			     const type_base* second,
			     bool indirect_type)
{
  if (!!first != !!second)
    return false;

  if (!first)
    return false;

  // Treat typedefs purely as type aliases and ignore CV-qualifiers.
  first = peel_qualified_or_typedef_type(first);
  second = peel_qualified_or_typedef_type(second);

  // Eliminate all but N of the N^2 comparison cases. This also guarantees the
  // various ty2 below cannot be null.
  if (typeid(*first) != typeid(*second))
    return false;

  // Peel off matching pointers.
  if (const pointer_type_def* ty1 = is_pointer_type(first))
    {
      const pointer_type_def* ty2 = is_pointer_type(second);
      return types_have_similar_structure(ty1->get_pointed_to_type(),
					  ty2->get_pointed_to_type(),
					  /*indirect_type=*/true);
    }

  // Peel off matching references.
  if (const reference_type_def* ty1 = is_reference_type(first))
    {
      const reference_type_def* ty2 = is_reference_type(second);
      if (ty1->is_lvalue() != ty2->is_lvalue())
	return false;
      return types_have_similar_structure(ty1->get_pointed_to_type(),
					  ty2->get_pointed_to_type(),
					  /*indirect_type=*/true);
    }

  if (const type_decl* ty1 = is_type_decl(first))
    {
      const type_decl* ty2 = is_type_decl(second);
      if (!indirect_type)
	if (ty1->get_size_in_bits() != ty2->get_size_in_bits())
	  return false;

      return ty1->get_name() == ty2->get_name();
    }

  if (const enum_type_decl* ty1 = is_enum_type(first))
    {
      const enum_type_decl* ty2 = is_enum_type(second);
      if (!indirect_type)
	if (ty1->get_size_in_bits() != ty2->get_size_in_bits())
	  return false;

      return (get_name(ty1->get_underlying_type())
	      == get_name(ty2->get_underlying_type()));
    }

  if (const class_decl* ty1 = is_class_type(first))
    {
      const class_decl* ty2 = is_class_type(second);
      if (!ty1->get_is_anonymous() && !ty2->get_is_anonymous()
	  && ty1->get_name() != ty2->get_name())
	return false;

      if (!indirect_type)
	{
	  if ((ty1->get_size_in_bits() != ty2->get_size_in_bits())
	      || (ty1->get_non_static_data_members().size()
		  != ty2->get_non_static_data_members().size()))
	    return false;

	  for (class_or_union::data_members::const_iterator
		 i = ty1->get_non_static_data_members().begin(),
		 j = ty2->get_non_static_data_members().begin();
	       (i != ty1->get_non_static_data_members().end()
		&& j != ty2->get_non_static_data_members().end());
	       ++i, ++j)
	    {
	      var_decl_sptr dm1 = *i;
	      var_decl_sptr dm2 = *j;
	      if (!types_have_similar_structure(dm1->get_type().get(),
						dm2->get_type().get(),
						indirect_type))
		return false;
	    }
	}

      return true;
    }

  if (const union_decl* ty1 = is_union_type(first))
    {
      const union_decl* ty2 = is_union_type(second);
      if (!ty1->get_is_anonymous() && !ty2->get_is_anonymous()
	  && ty1->get_name() != ty2->get_name())
	return false;

      if (!indirect_type)
	return ty1->get_size_in_bits() == ty2->get_size_in_bits();

      return true;
    }

  if (const array_type_def* ty1 = is_array_type(first))
    {
      const array_type_def* ty2 = is_array_type(second);
      // TODO: Handle int[5][2] vs int[2][5] better.
      if (ty1->get_size_in_bits() != ty2->get_size_in_bits()
	  || ty1->get_dimension_count() != ty2->get_dimension_count()
	  || !types_have_similar_structure(ty1->get_element_type(),
					   ty2->get_element_type(),
					   /*indirect_type=*/true))
	return false;

      return true;
    }

  if (const array_type_def::subrange_type *ty1 = is_subrange_type(first))
    {
      const array_type_def::subrange_type *ty2 = is_subrange_type(second);
      if (ty1->get_upper_bound() != ty2->get_upper_bound()
	  || ty1->get_lower_bound() != ty2->get_lower_bound()
	  || ty1->get_language() != ty2->get_language()
	  || !types_have_similar_structure(ty1->get_underlying_type(),
					   ty2->get_underlying_type(),
					   indirect_type))
	return false;

      return true;
    }

  if (const function_type* ty1 = is_function_type(first))
    {
      const function_type* ty2 = is_function_type(second);
      if (!types_have_similar_structure(ty1->get_return_type(),
					ty2->get_return_type(),
					indirect_type))
	return false;

      if (ty1->get_parameters().size() != ty2->get_parameters().size())
	return false;

      for (function_type::parameters::const_iterator
	     i = ty1->get_parameters().begin(),
	     j = ty2->get_parameters().begin();
	   (i != ty1->get_parameters().end()
	    && j != ty2->get_parameters().end());
	   ++i, ++j)
	if (!types_have_similar_structure((*i)->get_type(),
					  (*j)->get_type(),
					  indirect_type))
	  return false;

      return true;
    }

  // All kinds of type should have been handled at this point.
  ABG_ASSERT_NOT_REACHED;

  return false;
}

/// Look for a data member of a given class, struct or union type and
/// return it.
///
/// The data member is designated by its name.
///
/// @param type the class, struct or union type to consider.
///
/// @param dm_name the name of the data member to lookup.
///
/// @return the data member iff it was found in @type or NULL if no
/// data member with that name was found.
const var_decl*
lookup_data_member(const type_base* type,
		   const char* dm_name)

{
  class_or_union *cou = is_class_or_union_type(type);
  if (!cou)
    return 0;

  return cou->find_data_member(dm_name).get();
}

/// Get the function parameter designated by its index.
///
/// Note that the first function parameter has index 0.
///
/// @param fun the function to consider.
///
/// @param parm_index the index of the function parameter to get.
///
/// @return the function parameter designated by its index, of NULL if
/// no function parameter with that index was found.
const function_decl::parameter*
get_function_parameter(const decl_base* fun,
		       unsigned parm_index)
{
  function_decl* fn = is_function_decl(fun);
  if (!fn)
    return 0;

  const function_decl::parameters &parms = fn->get_type()->get_parameters();
  if (parms.size() <= parm_index)
    return 0;

  return parms[parm_index].get();
}

bool
ir_traversable_base::traverse(ir_node_visitor&)
{return true;}

// <ir_node_visitor stuff>

/// The private data structure of the ir_node_visitor type.
struct ir_node_visitor::priv
{
  pointer_set visited_ir_nodes;
  bool allow_visiting_already_visited_type_node;

  priv()
    : allow_visiting_already_visited_type_node(true)
  {}
}; // end struct ir_node_visitory::priv

/// Default Constructor of the ir_node_visitor type.
ir_node_visitor::ir_node_visitor()
  : priv_(new priv)
{}

/// Set if the walker using this visitor is allowed to re-visit a type
/// node that was previously visited or not.
///
/// @param f if true, then the walker using this visitor is allowed to
/// re-visit a type node that was previously visited.
void
ir_node_visitor::allow_visiting_already_visited_type_node(bool f)
{priv_->allow_visiting_already_visited_type_node = f;}

/// Get if the walker using this visitor is allowed to re-visit a type
/// node that was previously visited or not.
///
/// @return true iff the walker using this visitor is allowed to
/// re-visit a type node that was previously visited.
bool
ir_node_visitor::allow_visiting_already_visited_type_node() const
{return priv_->allow_visiting_already_visited_type_node;}

/// Mark a given type node as having been visited.
///
/// Note that for this function to work, the type node must have been
/// canonicalized.  Otherwise the process is aborted.
///
/// @param p the type to mark as having been visited.
void
ir_node_visitor::mark_type_node_as_visited(type_base *p)
{
  if (allow_visiting_already_visited_type_node())
    return;

  if (p == 0 || type_node_has_been_visited(p))
    return;

  type_base* canonical_type = p->get_naked_canonical_type();
  ABG_ASSERT(canonical_type);

  size_t canonical_ptr_value = reinterpret_cast<size_t>(canonical_type);
  priv_->visited_ir_nodes.insert(canonical_ptr_value);
}

/// Un-mark all visited type nodes.
///
/// That is, no type node is going to be considered as having been
/// visited anymore.
///
/// In other words, after invoking this funciton,
/// ir_node_visitor::type_node_has_been_visited() is going to return
/// false on all type nodes.
void
ir_node_visitor::forget_visited_type_nodes()
{priv_->visited_ir_nodes.clear();}

/// Test if a given type node has been marked as visited.
///
/// @param p the type node to consider.
///
/// @return true iff the type node @p p has been marked as visited by
/// the function ir_node_visitor::mark_type_node_as_visited.
bool
ir_node_visitor::type_node_has_been_visited(type_base* p) const
{
  if (allow_visiting_already_visited_type_node())
    return false;

  if (p == 0)
    return false;

  type_base *canonical_type = p->get_naked_canonical_type();
  ABG_ASSERT(canonical_type);

  size_t ptr_value = reinterpret_cast<size_t>(canonical_type);
  pointer_set::iterator it = priv_->visited_ir_nodes.find(ptr_value);
  if (it == priv_->visited_ir_nodes.end())
    return false;

  return true;
}

bool
ir_node_visitor::visit_begin(decl_base*)
{return true;}

bool
ir_node_visitor::visit_end(decl_base*)
{return true;}

bool
ir_node_visitor::visit_begin(scope_decl*)
{return true;}

bool
ir_node_visitor::visit_end(scope_decl*)
{return true;}

bool
ir_node_visitor::visit_begin(type_base*)
{return true;}

bool
ir_node_visitor::visit_end(type_base*)
{return true;}

bool
ir_node_visitor::visit_begin(scope_type_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(scope_type_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(type_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(type_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(namespace_decl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(namespace_decl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(qualified_type_def* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(qualified_type_def* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(pointer_type_def* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(pointer_type_def* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(reference_type_def* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(reference_type_def* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(array_type_def* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(array_type_def* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(array_type_def::subrange_type* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(array_type_def::subrange_type* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(enum_type_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(enum_type_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(typedef_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(typedef_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(function_type* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(function_type* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(var_decl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(var_decl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(function_decl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(function_decl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(function_decl::parameter* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(function_decl::parameter* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(function_tdecl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(function_tdecl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(class_tdecl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(class_tdecl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(class_or_union* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(class_or_union* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(class_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(class_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(union_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(union_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(class_decl::base_spec* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(class_decl::base_spec* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(member_function_template* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(member_function_template* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(member_class_template* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(member_class_template* d)
{return visit_end(static_cast<decl_base*>(d));}

// </ir_node_visitor stuff>

// <debugging facilities>

/// Generate a different string at each invocation.
///
/// @return the resulting string.
static string
get_next_string()
{
  static __thread size_t counter;
  ++counter;
  std::ostringstream o;
  o << counter;
  return o.str();
}

/// Convenience typedef for a hash map of pointer to function_decl and
/// string.
typedef unordered_map<const function_decl*, string,
		      function_decl::hash,
		      function_decl::ptr_equal> fns_to_str_map_type;

/// Return a string associated to a given function.  Two functions
/// that compare equal would yield the same string, as far as this
/// routine is concerned.  And two functions that are different would
/// yield different strings.
///
/// This is used to debug core diffing issues on functions.  The
/// sequence of strings can be given to the 'testdiff2' program that
/// is in the tests/ directory of the source tree, to reproduce core
/// diffing issues on string and thus ease the debugging.
///
/// @param fn the function to generate a string for.
///
/// @param m the function_decl* <-> string map to be used by this
/// function to generate strings associated to a function.
///
/// @return the resulting string.
static const string&
fn_to_str(const function_decl* fn,
	  fns_to_str_map_type& m)
{
  fns_to_str_map_type::const_iterator i = m.find(fn);
  if (i != m.end())
    return i->second;
  string s = get_next_string();
  return m[fn]= s;
}

/// Generate a sequence of string that matches a given sequence of
/// function.  In the resulting sequence, each function is "uniquely
/// representated" by a string.  For instance, if the same function "foo"
/// appears at indexes 1 and 3, then the same string 'schmurf' (okay,
/// we don't care about the actual string) would appear at index 1 and 3.
///
/// @param begin the beginning of the sequence of functions to consider.
///
/// @param end the end of the sequence of functions.  This points to
/// one-passed-the-end of the actual sequence.
///
/// @param m the function_decl* <-> string map to be used by this
/// function to generate strings associated to a function.
///
/// @param o the output stream where to emit the generated list of
/// strings to.
static void
fns_to_str(vector<function_decl*>::const_iterator begin,
	   vector<function_decl*>::const_iterator end,
	   fns_to_str_map_type& m,
	   std::ostream& o)
{
  vector<function_decl*>::const_iterator i;
  for (i = begin; i != end; ++i)
    o << "'" << fn_to_str(*i, m) << "' ";
}

/// For each sequence of functions given in argument, generate a
/// sequence of string that matches a given sequence of function.  In
/// the resulting sequence, each function is "uniquely representated"
/// by a string.  For instance, if the same function "foo" appears at
/// indexes 1 and 3, then the same string 'schmurf' (okay, we don't
/// care about the actual string) would appear at index 1 and 3.
///
/// @param a_begin the beginning of the sequence of functions to consider.
///
/// @param a_end the end of the sequence of functions.  This points to
/// one-passed-the-end of the actual sequence.
///
/// @param b_begin the beginning of the second sequence of functions
/// to consider.
///
/// @param b_end the end of the second sequence of functions.
///
/// @param m the function_decl* <-> string map to be used by this
/// function to generate strings associated to a function.
///
/// @param o the output stream where to emit the generated list of
/// strings to.
static void
fns_to_str(vector<function_decl*>::const_iterator a_begin,
	   vector<function_decl*>::const_iterator a_end,
	   vector<function_decl*>::const_iterator b_begin,
	   vector<function_decl*>::const_iterator b_end,
	   fns_to_str_map_type& m,
	   std::ostream& o)
{
  fns_to_str(a_begin, a_end, m, o);
  o << "->|<- ";
  fns_to_str(b_begin, b_end, m, o);
  o << "\n";
}

/// For each sequence of functions given in argument, generate a
/// sequence of string that matches a given sequence of function.  In
/// the resulting sequence, each function is "uniquely representated"
/// by a string.  For instance, if the same function "foo" appears at
/// indexes 1 and 3, then the same string 'schmurf' (okay, we don't
/// care about the actual string) would appear at index 1 and 3.
///
/// @param a_begin the beginning of the sequence of functions to consider.
///
/// @param a_end the end of the sequence of functions.  This points to
/// one-passed-the-end of the actual sequence.
///
/// @param b_begin the beginning of the second sequence of functions
/// to consider.
///
/// @param b_end the end of the second sequence of functions.
///
/// @param o the output stream where to emit the generated list of
/// strings to.
void
fns_to_str(vector<function_decl*>::const_iterator a_begin,
	   vector<function_decl*>::const_iterator a_end,
	   vector<function_decl*>::const_iterator b_begin,
	   vector<function_decl*>::const_iterator b_end,
	   std::ostream& o)
{
  fns_to_str_map_type m;
  fns_to_str(a_begin, a_end, b_begin, b_end, m, o);
}

// </debugging facilities>

// </class template>

}// end namespace ir
}//end namespace abigail

namespace
{

/// Update the qualified parent name, qualified name and scoped name
/// of a tree decl node.
///
/// @return true if the tree walking should continue, false otherwise.
///
/// @param d the tree node to take in account.
bool
qualified_name_setter::do_update(abigail::ir::decl_base* d)
{
  std::string parent_qualified_name;
  abigail::ir::scope_decl* parent = d->get_scope();
  if (parent)
    d->priv_->qualified_parent_name_ = parent->get_qualified_name();
  else
    d->priv_->qualified_parent_name_ = abigail::interned_string();

  abigail::environment* env = d->get_environment();
  ABG_ASSERT(env);
  if (!d->priv_->qualified_parent_name_.empty())
    {
      if (d->get_name().empty())
	d->priv_->qualified_name_ = abigail::interned_string();
      else
	d->priv_->qualified_name_ =
	  env->intern(d->priv_->qualified_parent_name_ + "::" + d->get_name());
    }

  if (d->priv_->scoped_name_.empty())
    {
      if (parent
	  && !parent->get_is_anonymous()
	  && !parent->get_name().empty())
	d->priv_->scoped_name_ =
	  env->intern(parent->get_name() + "::" + d->get_name());
      else
	d->priv_->scoped_name_ =
	  env->intern(d->get_name());
    }

  if (!is_scope_decl(d))
    return false;

  return true;
}

/// This is called when we start visiting a decl node, during the
/// udpate of the qualified name of a given sub-tree.
///
/// @param d the decl node we are visiting.
///
/// @return true iff the traversal should keep going.
bool
qualified_name_setter::visit_begin(abigail::ir::decl_base* d)
{return do_update(d);}

/// This is called when we start visiting a type node, during the
/// udpate of the qualified name of a given sub-tree.
///
/// @param d the decl node we are visiting.
///
/// @return true iff the traversal should keep going.
bool
qualified_name_setter::visit_begin(abigail::ir::type_base* t)
{
  if (abigail::ir::decl_base* d = get_type_declaration(t))
    return do_update(d);
  return false;
}
}// end anonymous namespace.

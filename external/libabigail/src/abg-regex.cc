// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2016-2020 Red Hat, Inc.

/// @file
///
/// Some specialization for shared pointer utility templates.
///

#include "config.h"

#include <sstream>
#include <ostream>

#include "abg-internal.h"

// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-regex.h"
#include "abg-sptr-utils.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This is used to wrap a pointer to regex_t into a
/// shared_ptr<regex_t>.
///
/// @param p the bare pointer to regex_t to wrap into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> that wraps @p p.
template<>
regex::regex_t_sptr
sptr_utils::build_sptr<regex_t>(regex_t *p)
{return regex::regex_t_sptr(p, regex::regex_t_deleter());}

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This creates a pointer to regex_t and wraps it into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> wrapping the newly created regex_t*
template<>
regex::regex_t_sptr
sptr_utils::build_sptr<regex_t>()
{return sptr_utils::build_sptr(new regex_t);}

namespace regex
{

/// Escape regex special charaters in input string.
///
/// @param os the output stream being written to.
///
/// @param esc the regex_escape object holding a reference to the string
/// needing to be escaped.
///
/// @return the output stream.
std::ostream&
operator<<(std::ostream& os, const escape& esc)
{
  // ']' and '}' are only conditionally special, so could be removed.
  static const std::string specials = "^.[]$()|*+?{}\\";
  const std::string& str = esc.ref;
  for (std::string::const_iterator i = str.begin(); i != str.end(); ++i)
    {
      if (specials.find(*i) != std::string::npos)
	os << '\\';
      os << *i;
    }
  return os;
}

/// Generate a regex pattern equivalent to testing set membership.
///
/// A string will match the resulting pattern regex, if and only if it
/// was present in the vector.
///
/// @param strs a vector of strings
///
/// @return a regex pattern
std::string
generate_from_strings(const std::vector<std::string>& strs)
{
  if (strs.empty())
    // This cute-looking regex does not match any string.
    return "^_^";
  std::ostringstream os;
  std::vector<std::string>::const_iterator i = strs.begin();
  os << "^(" << escape(*i++);
  while (i != strs.end())
    os << "|" << escape(*i++);
  os << ")$";
  return os.str();
}

/// Compile a regex from a string.
///
/// The result is held in a shared pointer. This will be null if regex
/// compilation fails.
///
/// @param str the string representation of the regex.
///
/// @return shared pointer holder of a compiled regex object.
regex_t_sptr
compile(const std::string& str)
{
  regex_t_sptr r = sptr_utils::build_sptr(new regex_t);
  if (regcomp(r.get(), str.c_str(), REG_EXTENDED))
    r.reset();
  return r;
}

/// See if a string matches a regex.
///
/// @param r a shared pointer holder of a compiled regex object.
///
/// @param str a string.
///
/// @return whether there was a match.
bool
match(const regex_t_sptr& r, const std::string& str)
{
  return !regexec(r.get(), str.c_str(), 0, NULL, 0);
}

}//end namespace regex

}//end namespace abigail

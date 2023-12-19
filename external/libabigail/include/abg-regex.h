// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file
///
/// Wrappers around regex types and functions.

#ifndef __ABG_REGEX_H__
#define __ABG_REGEX_H__

#include <regex.h>

#include <memory>
#include <string>
#include <vector>

#include "abg-sptr-utils.h"

namespace abigail
{

/// Namespace for regex types and functions.
namespace regex
{

/// A convenience typedef for a shared pointer of regex_t.
typedef std::shared_ptr<regex_t> regex_t_sptr;

/// A delete functor for a shared_ptr of regex_t.
struct regex_t_deleter
{
  /// The operator called to de-allocate the pointer to regex_t
  /// embedded in a shared_ptr<regex_t>
  ///
  /// @param r the pointer to regex_t to de-allocate.
  void
  operator()(::regex_t* r)
  {
    regfree(r);
    delete r;
  }
};//end struct regex_deleter

/// A class to hold a reference to a string to regex escape.
struct escape
{
  escape(const std::string& str) : ref(str) { }
  const std::string& ref;
};

std::ostream&
operator<<(std::ostream& os, const escape& esc);

std::string
generate_from_strings(const std::vector<std::string>& strs);

regex_t_sptr
compile(const std::string& str);

bool
match(const regex_t_sptr& r, const std::string& str);

}// end namespace regex

namespace sptr_utils
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
build_sptr<regex_t>(regex_t *p);

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This creates a pointer to regex_t and wraps it into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> wrapping the newly created regex_t*
template<>
regex::regex_t_sptr
build_sptr<regex_t>();

}// end namespace sptr_utils

}// end namespace abigail

#endif //__ABG_REGEX_H__

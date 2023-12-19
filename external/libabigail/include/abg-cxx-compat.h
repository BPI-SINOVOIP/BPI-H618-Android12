// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2019-2020 Google, Inc.

/// @file

#ifndef __ABG_CXX_COMPAT_H
#define __ABG_CXX_COMPAT_H

// C++17 support (via custom implementations if compiled with earlier standard)

#if __cplusplus >= 201703L

#include <optional>

#else

#include <stdexcept> // for throwing std::runtime_error("bad_optional_access")

#endif

namespace abg_compat {

#if __cplusplus >= 201703L

using std::optional;

#else

// <optional>

/// Simplified implementation of std::optional just enough to be used as a
/// replacement for our purposes and when compiling with pre C++17.
///
/// The implementation intentionally does not support a whole lot of features
/// to minimize the maintenance effort with this.
template <typename T> class optional
{
  bool has_value_;
  T    value_;

public:
  optional() : has_value_(false), value_() {}
  optional(const T& value) : has_value_(true), value_(value) {}

  bool
  has_value() const
  {
    return has_value_;
  }

  const T&
  value() const
  {
    if (!has_value_)
      throw std::runtime_error("bad_optional_access");
    return value_;
  }

  const T
  value_or(const T& default_value) const
  {
    if (!has_value_)
      return default_value;
    return value_;
  }

  const T&
  operator*() const
  { return value_; }

  T&
  operator*()
  { return value_; }

  const T*
  operator->() const
  { return &value_; }

  T*
  operator->()
  { return &value_; }

  optional&
  operator=(const T& value)
  {
    has_value_ = true;
    value_ = value;
    return *this;
  }

  explicit operator bool() const { return has_value_; }
};

#endif
}

#endif  // __ABG_CXX_COMPAT_H

// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file
///
/// Utilities to ease the wrapping of C types into std::shared_ptr

#ifndef __ABG_SPTR_UTILS_H__
#define __ABG_SPTR_UTILS_H__

#include <regex.h>
#include <memory>


namespace abigail
{

/// Namespace for the utilities to wrap C types into std::shared_ptr.
namespace sptr_utils
{

using std::shared_ptr;

/// This is to be specialized for the diverse C types that needs
/// wrapping in shared_ptr.
///
/// @tparam T the type of the C type to wrap in a shared_ptr.
///
/// @param p a pointer to wrap in a shared_ptr.
///
/// @return then newly created shared_ptr<T>
template<class T>
shared_ptr<T>
build_sptr(T* p);

/// This is to be specialized for the diverse C types that needs
/// wrapping in shared_ptr.
///
/// This variant creates a pointer to T and wraps it into a
/// shared_ptr<T>.
///
/// @tparam T the type of the C type to wrap in a shared_ptr.
///
/// @return then newly created shared_ptr<T>
template<class T>
shared_ptr<T>
build_sptr();

/// A deleter for shared pointers that ... doesn't delete the object
/// managed by the shared pointer.
struct noop_deleter
{
  template<typename T>
  void
  operator()(const T*)
  {}
};

}// end namespace sptr_utils
}// end namespace abigail

#endif //__ABG_SPTR_UTILS_H__

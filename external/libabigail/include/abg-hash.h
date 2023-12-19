// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#ifndef __ABG_HASH_H__
#define __ABG_HASH_H__

#include <stdint.h>
#include <cstddef>
#include <string>

namespace abigail
{
/// Namespace for hashing.
namespace hashing
{
  /// Produce good hash value combining val1 and val2.
  /// This is copied from tree.c in GCC.
  std::size_t
  combine_hashes(std::size_t, std::size_t);

  uint32_t
  fnv_hash(const std::string& str);
}//end namespace hashing
}//end namespace abigail

#endif //__ABG_HASH_H__

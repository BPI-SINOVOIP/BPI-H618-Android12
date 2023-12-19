// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

#include "test-utils.h"

using std::string;

namespace abigail
{
namespace tests
{

/// Returns the absolute path to the source directory.
///
/// \return the absolute path tho the source directory.
const char*
get_src_dir()
{
#ifndef ABIGAIL_SRC_DIR
#error the macro ABIGAIL_SRC_DIR must be set at compile time
#endif

  static __thread const char* s(ABIGAIL_SRC_DIR);
  return s;
}

/// Returns the absolute path to the build directory.
///
/// \return the absolute path the build directory.
const char*
get_build_dir()
{
#ifndef ABIGAIL_BUILD_DIR
#error the macro ABIGAIL_BUILD_DIR must be set at compile time
#endif

  static __thread const char* s(ABIGAIL_BUILD_DIR);
  return s;
}

}//end namespace tests
}//end namespace abigail

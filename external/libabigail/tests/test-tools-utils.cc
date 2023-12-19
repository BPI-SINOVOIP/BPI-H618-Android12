// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file
///
/// This is a collection of unit tests for functions defined in
/// abg-tgools-utils.cc.

#include <iostream>
#include "abg-tools-utils.h"

using namespace abigail::tools_utils;
using std::cerr;

int
main(int, char**)
{

  /// These are unit tests for abigail::tools_utils::decl_names_equal.
  /// Just run the resulting runtesttoolsutils program under the
  /// debugger to debug this if need be.

  ABG_ASSERT(decl_names_equal("foo", "foo") == true);

  ABG_ASSERT(decl_names_equal("foo", "bar") == false);

  ABG_ASSERT(decl_names_equal("__anonymous_struct__1::foo",
			      "__anonymous_struct__2::foo") == true);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::foo::__anonymous_struct__2::bar",
	      "__anonymous_struct__10::foo::__anonymous_struct__11::bar")
	     == true);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_union__1::foo::__anonymous_union__2::bar",
	      "__anonymous_union__10::foo::__anonymous_union__11::bar")
	     == true);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_enum__1::foo::__anonymous_enum__2::bar",
	      "__anonymous_enum__10::foo::__anonymous_enum__11::bar")
	     == true);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::bar::__anonymous_struct__2::baz",
	      "__anonymous_struct__10::foo::__anonymous_struct__11::bar")
	     == false);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::foo::__anonymous_struct__2::baz",
	      "__anonymous_struct__10::foo::__anonymous_struct__11::bar")
	     == false);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::foo::__anonymous_struct__2::bar",
	      "__anonymous_struct__10::foo::__anonymous_union__11::bar")
	     == false);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::foo::__anonymous_struct__2::bar",
	      "__anonymous_struct__10::foo::__anonymous_enum__11::bar")
	     == false);

  ABG_ASSERT(decl_names_equal
	     ("OT::Extension<OT::ExtensionSubst>::__anonymous_union__",
	      "OT::Extension<OT::ExtensionSubst>::__anonymous_union__")
	     == true);

  ABG_ASSERT(decl_names_equal("S0::m2", "S0::m12") == false);

  ABG_ASSERT(!decl_names_equal("S0::S1", "S0"));
  ABG_ASSERT(!decl_names_equal("S0", "S0::S1"));
  ABG_ASSERT(!decl_names_equal("S1::S0", "S0::S1"));
  ABG_ASSERT(!decl_names_equal("__anonymous_struct__::S0", "__anonymous_struct__"));
  ABG_ASSERT(!decl_names_equal("__anonymous_struct__", "__anonymous_struct__::S1"));
  ABG_ASSERT(!decl_names_equal("__anonymous_struct__::S0", "__anonymous_struct__::S1"));
  ABG_ASSERT(!decl_names_equal("S0::__anonymous_struct__", "__anonymous_struct__"));
  ABG_ASSERT(!decl_names_equal("__anonymous_struct__", "S1::__anonymous_struct__"));
  ABG_ASSERT(!decl_names_equal("S0::__anonymous_struct__", "S1::__anonymous_struct__"));

  return 0;
}

// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2020 Google, Inc.
//
// Author: Matthias Maennich

/// @file
///
/// This program tests libabigail's CXX compatibility layer.

#include "lib/catch.hpp"

#include "abg-cxx-compat.h"

using abg_compat::optional;

TEST_CASE("OptionalConstruction", "[abg_compat::optional]")
{
  optional<bool> opt1;
  REQUIRE_FALSE(opt1.has_value());

  optional<bool> opt2(true);
  REQUIRE(opt2.has_value());
  CHECK(opt2.value() == true);

  optional<bool> opt3(false);
  REQUIRE(opt3.has_value());
  CHECK(opt3.value() == false);
}

TEST_CASE("OptionalValue", "[abg_compat::optional]")
{
  optional<bool> opt;
  REQUIRE_FALSE(opt.has_value());
  REQUIRE_THROWS(opt.value());

  opt = true;
  REQUIRE_NOTHROW(opt.value());
  CHECK(opt.value() == true);
}

TEST_CASE("OptionalValueOr", "[abg_compat::optional]")
{
  optional<std::string> opt;
  REQUIRE_FALSE(opt.has_value());

  const std::string& mine = "mine";
  // Ensure we get a copy of our own value.
  CHECK(opt.value_or(mine) == mine);

  // Now set the value
  const std::string& other = "other";
  opt = other;
  CHECK(opt.value_or(mine) != mine);
  CHECK(opt.value_or(mine) == other);
}

TEST_CASE("OptionalDeref", "[abg_compat::optional]")
{
  optional<std::string> opt("asdf");
  REQUIRE(opt.has_value());

  CHECK(*opt == "asdf");
  CHECK(opt->size() == 4);
}

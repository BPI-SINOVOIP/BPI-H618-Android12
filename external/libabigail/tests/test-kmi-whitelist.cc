// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2020 Google, Inc.
//
// Author: Matthias Maennich

/// @file
///
/// This program tests suppression generation from KMI whitelists.

#include <string>

#include "lib/catch.hpp"

#include "abg-fwd.h"
#include "abg-suppression.h"
#include "abg-tools-utils.h"
#include "test-utils.h"

using abigail::tools_utils::gen_suppr_spec_from_kernel_abi_whitelists;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::function_suppression_sptr;
using abigail::suppr::variable_suppression_sptr;
using abigail::suppr::is_function_suppression;
using abigail::suppr::is_variable_suppression;

const static std::string whitelist_with_single_entry
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/whitelist-with-single-entry";

const static std::string whitelist_with_another_single_entry
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/whitelist-with-another-single-entry";

const static std::string whitelist_with_two_sections
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/whitelist-with-two-sections";

const static std::string whitelist_with_duplicate_entry
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/whitelist-with-duplicate-entry";

const static std::string symbol_list_with_single_entry
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/symbol-list-with-single-entry";

const static std::string symbol_list_with_another_single_entry
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/symbol-list-with-another-single-entry";

const static std::string symbol_list_with_two_sections
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/symbol-list-with-two-sections";

const static std::string symbol_list_with_duplicate_entry
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/symbol-list-with-duplicate-entry";

void
test_suppressions_are_consistent(const suppressions_type& suppr,
			    const std::string&	     expr)
{
  REQUIRE(suppr.size() == 2);

  function_suppression_sptr left = is_function_suppression(suppr[0]);
  variable_suppression_sptr right = is_variable_suppression(suppr[1]);

  // correctly casted
  REQUIRE(left);
  REQUIRE(right);
  // same label
  REQUIRE(left->get_label() == right->get_label());
  // same mode
  REQUIRE(left->get_drops_artifact_from_ir()
	  == right->get_drops_artifact_from_ir());
  // same regex
  REQUIRE(left->get_symbol_name_not_regex_str()
     == right->get_symbol_name_not_regex_str());
  // regex as expected
  REQUIRE(left->get_symbol_name_not_regex_str() == expr);
}

TEST_CASE("NoWhitelists", "[whitelists]")
{
  const std::vector<std::string> abi_whitelist_paths;
  suppressions_type		 suppr =
      gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  REQUIRE(suppr.empty());
}

TEST_CASE("WhitelistWithASingleEntry", "[whitelists]")
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_single_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr, "^(test_symbol)$");
}

TEST_CASE("WhitelistWithADuplicateEntry", "[whitelists]")
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_duplicate_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr, "^(test_symbol)$");
}

TEST_CASE("TwoWhitelists", "[whitelists]")
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_single_entry);
  abi_whitelist_paths.push_back(whitelist_with_another_single_entry);
  suppressions_type suppr =
      gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr,
				   "^(test_another_symbol|test_symbol)$");
}

TEST_CASE("TwoWhitelistsWithDuplicates", "[whitelists]")
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_duplicate_entry);
  abi_whitelist_paths.push_back(whitelist_with_another_single_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr,
				   "^(test_another_symbol|test_symbol)$");
}

TEST_CASE("WhitelistWithTwoSections", "[whitelists]")
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_two_sections);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr, "^(test_symbol1|test_symbol2)$");
}

TEST_CASE("NoSymbolLists", "[symbol_lists]")
{
  const std::vector<std::string> abi_symbol_list_paths;
  suppressions_type		 suppr =
      gen_suppr_spec_from_kernel_abi_whitelists(abi_symbol_list_paths);
  REQUIRE(suppr.empty());
}

TEST_CASE("SymbolListWithASingleEntry", "[symbol_lists]")
{
  std::vector<std::string> abi_symbol_list_paths;
  abi_symbol_list_paths.push_back(symbol_list_with_single_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_symbol_list_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr, "^(test_symbol)$");
}

TEST_CASE("SymbolListWithADuplicateEntry", "[symbol_lists]")
{
  std::vector<std::string> abi_symbol_list_paths;
  abi_symbol_list_paths.push_back(symbol_list_with_duplicate_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_symbol_list_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr, "^(test_symbol)$");
}

TEST_CASE("TwoSymbolLists", "[symbol_lists]")
{
  std::vector<std::string> abi_symbol_list_paths;
  abi_symbol_list_paths.push_back(symbol_list_with_single_entry);
  abi_symbol_list_paths.push_back(symbol_list_with_another_single_entry);
  suppressions_type suppr =
      gen_suppr_spec_from_kernel_abi_whitelists(abi_symbol_list_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr,
				   "^(test_another_symbol|test_symbol)$");
}

TEST_CASE("TwoSymbolListsWithDuplicates", "[symbol_lists]")
{
  std::vector<std::string> abi_symbol_list_paths;
  abi_symbol_list_paths.push_back(symbol_list_with_duplicate_entry);
  abi_symbol_list_paths.push_back(symbol_list_with_another_single_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_symbol_list_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr,
				   "^(test_another_symbol|test_symbol)$");
}

TEST_CASE("SymbolListWithTwoSections", "[symbol_lists]")
{
  std::vector<std::string> abi_symbol_list_paths;
  abi_symbol_list_paths.push_back(symbol_list_with_two_sections);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_symbol_list_paths);
  REQUIRE(!suppr.empty());
  test_suppressions_are_consistent(suppr, "^(test_symbol1|test_symbol2)$");
}

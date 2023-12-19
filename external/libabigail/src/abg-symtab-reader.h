// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2020 Google, Inc.
//
// Author: Matthias Maennich

/// @file
///
/// This contains the declarations for the symtab reader.

#ifndef __ABG_SYMTAB_READER_H__
#define __ABG_SYMTAB_READER_H__

#include <gelf.h>

#include <functional>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <vector>

#include "abg-cxx-compat.h"  // for abg_compat::optional
#include "abg-ir.h"

namespace abigail
{
namespace symtab_reader
{

/// The symtab filter is the object passed to the symtab object in order to
/// iterate over the symbols in the symtab while applying filters.
///
/// The general idea is that it consists of a set of optionally enforced flags,
/// such as 'functions' or 'variables'. If not set, those are not filtered for,
/// neither inclusive nor exclusive. If set they are all ANDed together.
class symtab_filter
{
public:
  // Default constructor disabling all features.
  symtab_filter() {}

  bool
  matches(const elf_symbol& symbol) const;

  /// Enable or disable function filtering
  ///
  /// @param new_value whether to filter for functions
  void
  set_functions(bool new_value = true)
  {functions_ = new_value;};

  /// Enable or disable variable filtering
  ///
  /// @param new_value whether to filter for variables
  void
  set_variables(bool new_value = true)
  {variables_ = new_value;};

  /// Enable or disable public symbol filtering
  ///
  /// @param new_value whether to filter for public symbols
  void
  set_public_symbols(bool new_value = true)
  {public_symbols_ = new_value;};

  /// Enable or disable undefined symbol filtering
  ///
  /// @param new_value whether to filter for undefined symbols
  void
  set_undefined_symbols(bool new_value = true)
  {undefined_symbols_ = new_value;};

  /// Enable or disable kernel symbol filtering
  ///
  /// @param new_value whether to filter for kernel symbols
  void
  set_kernel_symbols(bool new_value = true)
  {kernel_symbols_ = new_value;};

private:
  // The symbol is a function (FUNC)
  abg_compat::optional<bool> functions_;

  // The symbol is a variables (OBJECT)
  abg_compat::optional<bool> variables_;

  // The symbol is publicly accessible (global/weak with default/protected
  // visibility)
  abg_compat::optional<bool> public_symbols_;

  // The symbols is not defined (declared)
  abg_compat::optional<bool> undefined_symbols_;

  // The symbol is listed in the ksymtab (for Linux Kernel binaries).
  abg_compat::optional<bool> kernel_symbols_;
};

/// Base iterator for our custom iterator based on whatever the const_iterator
/// is for a vector of symbols.
/// As of writing this, std::vector<elf_symbol_sptr>::const_iterator.
typedef elf_symbols::const_iterator base_iterator;

/// An iterator to walk a vector of elf_symbols filtered by symtab_filter.
///
/// The implementation inherits all properties from the vector's
/// const_iterator, but intercepts where necessary to allow effective
/// filtering. This makes it a STL compatible iterator for general purpose
/// usage.
class symtab_iterator : public base_iterator
{
public:
  typedef base_iterator::value_type	 value_type;
  typedef base_iterator::reference	 reference;
  typedef base_iterator::pointer	 pointer;
  typedef base_iterator::difference_type difference_type;
  typedef std::forward_iterator_tag	 iterator_category;

  /// Construct the iterator based on a pair of underlying iterators and a
  /// symtab_filter object. Immediately fast forward to the next element that
  /// matches the criteria (if any).
  ///
  /// @param begin the underlying begin iterator
  ///
  /// @param begin the underlying end iterator
  ///
  /// @param filter the symtab_filter to apply
  symtab_iterator(base_iterator	       begin,
		  base_iterator	       end,
		  const symtab_filter& filter = symtab_filter())
    : base_iterator(begin), end_(end), filter_(filter)
  {skip_to_next();}

  /// Pre-increment operator to advance to the next matching element.
  ///
  /// @return itself after incrementing
  symtab_iterator&
  operator++()
  {
    base_iterator::operator++();
    skip_to_next();
    return *this;
  }

  /// Post-increment operator to advance to the next matching element.
  ///
  /// @return a copy of the iterator before incrementing
  symtab_iterator
  operator++(int)
  {
    symtab_iterator result(*this);
    ++(*this);
    return result;
  }

private:
  /// The end of the underlying iterator.
  const base_iterator end_;

  /// The symtab_filter used to determine when to advance.
  const symtab_filter& filter_;

  /// Skip to the next element that matches the filter criteria (if any). Hold
  /// off when reaching the end of the underlying iterator.
  void
  skip_to_next()
  {
    while (*this != end_ && !filter_.matches(***this))
      ++(*this);
  }
};

/// Convenience declaration of a unique_ptr<symtab>
class symtab;
typedef std::unique_ptr<symtab> symtab_ptr;

/// symtab is the actual data container of the symtab_reader implementation.
///
/// The symtab is instantiated either via an Elf handle (from binary) or from a
/// set of existing symbol maps (usually when instantiated from XML). It will
/// then discover the symtab, possibly the ksymtab (for Linux Kernel binaries)
/// and setup the data containers and lookup maps for later perusal.
///
/// The symtab is supposed to be used in a const context as all information is
/// already computed at construction time. Symbols are stored sorted to allow
/// deterministic reading of the entries.
///
/// An example use of the symtab class is
///
/// const auto symtab    = symtab::load(elf_handle, env);
/// symtab_filter filter = symtab->make_filter();
/// filter.set_public_symbols();
/// filter.set_functions();
///
/// for (const auto& symbol : filtered_symtab(*symtab, filter))
///   {
///     std::cout << symbol->get_name() << "\n";
///   }
///
/// This uses the filtered_symtab proxy object to capture the filter.
class symtab
{
public:
  typedef std::function<bool(const elf_symbol_sptr&)> symbol_predicate;

  /// Indicate whether any (kernel) symbols have been seen at construction.
  ///
  /// @return true if there are symbols detected earlier.
  bool
  has_symbols() const
  {return is_kernel_binary_ ? has_ksymtab_entries_ : !symbols_.empty();}

  symtab_filter
  make_filter() const;

  /// The (only) iterator type we offer is a const_iterator implemented by the
  /// symtab_iterator.
  typedef symtab_iterator const_iterator;

  /// Obtain an iterator to the beginning of the symtab according to the filter
  /// criteria. Whenever this iterator advances, it skips elements that do not
  /// match the filter criteria.
  ///
  /// @param filter the symtab_filter to match symbols against
  ///
  /// @return a filtering const_iterator of the underlying type
  const_iterator
  begin(const symtab_filter& filter) const
  {return symtab_iterator(symbols_.begin(), symbols_.end(), filter);}

  /// Obtain an iterator to the end of the symtab.
  ///
  /// @return an end iterator
  const_iterator
  end() const
  {return symtab_iterator(symbols_.end(), symbols_.end());}

  const elf_symbols&
  lookup_symbol(const std::string& name) const;

  const elf_symbol_sptr&
  lookup_symbol(GElf_Addr symbol_addr) const;

  static symtab_ptr
  load(Elf*		elf_handle,
       ir::environment* env,
       symbol_predicate is_suppressed = NULL);

  static symtab_ptr
  load(string_elf_symbols_map_sptr function_symbol_map,
       string_elf_symbols_map_sptr variables_symbol_map);

  void
  update_main_symbol(GElf_Addr addr, const std::string& name);

private:
  /// Default constructor. Private to enforce creation by factory methods.
  symtab();

  /// The vector of symbols we discovered.
  elf_symbols symbols_;

  /// Whether this is a Linux Kernel binary
  bool is_kernel_binary_;

  /// Whether this kernel_binary has ksymtab entries
  ///
  /// A kernel module might not have a ksymtab if it does not export any
  /// symbols. In order to quickly decide whether the symbol table is empty, we
  /// remember whether we ever saw ksymtab entries.
  bool has_ksymtab_entries_;

  /// Lookup map name->symbol(s)
  typedef std::unordered_map<std::string, std::vector<elf_symbol_sptr>>
		       name_symbol_map_type;
  name_symbol_map_type name_symbol_map_;

  /// Lookup map addr->symbol
  typedef std::unordered_map<GElf_Addr, elf_symbol_sptr> addr_symbol_map_type;
  addr_symbol_map_type addr_symbol_map_;

  /// Lookup map function entry address -> symbol
  addr_symbol_map_type entry_addr_symbol_map_;

  bool
  load_(Elf* elf_handle, ir::environment* env, symbol_predicate is_suppressed);

  bool
  load_(string_elf_symbols_map_sptr function_symbol_map,
       string_elf_symbols_map_sptr variables_symbol_map);

  void
  update_function_entry_address_symbol_map(Elf*	     elf_handle,
					   GElf_Sym* native_symbol,
					   const elf_symbol_sptr& symbol_sptr);
};

/// Helper class to allow range-for loops on symtabs for C++11 and later code.
/// It serves as a proxy for the symtab iterator and provides a begin() method
/// without arguments, as required for range-for loops (and possibly other
/// iterator based transformations).
///
/// Example usage:
///
///   for (const auto& symbol : filtered_symtab(tab, filter))
///     {
///       std::cout << symbol->get_name() << "\n";
///     }
///
class filtered_symtab
{
  const symtab&	      tab_;
  const symtab_filter filter_;

public:
  /// Construct the proxy object keeping references to the underlying symtab
  /// and the filter object.
  filtered_symtab(const symtab& tab, const symtab_filter& filter)
    : tab_(tab), filter_(filter)
  {}

  /// Pass through symtab.begin(), but also pass on the filter.
  symtab::const_iterator
  begin() const
  {return tab_.begin(filter_);}

  /// Pass through symtab.end().
  symtab::const_iterator
  end() const
  {return tab_.end();}
};

} // end namespace symtab_reader
} // end namespace abigail

#endif // __ABG_SYMTAB_READER_H__

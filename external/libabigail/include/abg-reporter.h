// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2017-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// The declaration of the reporting types of libabigail's diff
/// engine.

#ifndef __ABG_REPORTER_H__
#define __ABG_REPORTER_H__

#include <memory>
#include <ostream>
#include <string>

namespace abigail
{

using std::shared_ptr;

namespace comparison
{
class diff;
class type_decl_diff;
class enum_diff;
class typedef_diff;
class qualified_type_diff;
class distinct_diff;
class pointer_diff;
class reference_diff;
class array_diff;
class base_diff;
class class_or_union_diff;
class class_diff;
class union_diff;
class scope_diff;
class fn_parm_diff;
class function_type_diff;
class function_decl_diff;
class var_diff;
class translation_unit_diff;
class corpus_diff;
class diff_maps;
class reporter_base;

/// A convenience typedef for a shared pointer to a @ref
/// reporter_base.
typedef shared_ptr<reporter_base> reporter_base_sptr;

/// The base class of all the reporting classes.
class reporter_base
{
public:

  virtual bool diff_to_be_reported(const diff *d) const;

  virtual bool diff_has_net_changes(const corpus_diff *d) const = 0;

  virtual void
  report(const type_decl_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const enum_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const typedef_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const qualified_type_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const distinct_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const pointer_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const reference_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const array_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const base_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const class_or_union_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const class_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const union_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const scope_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const fn_parm_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const function_type_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const function_decl_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const var_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const translation_unit_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual void
  report(const corpus_diff& d, std::ostream& out,
	 const std::string& indent = "") const = 0;

  virtual ~reporter_base() {}
}; //end class reporter_base

class default_reporter;

/// A convenience typedef for a shared_ptr to a @ref default_reporter.
typedef shared_ptr<default_reporter> default_reporter_sptr;

/// The default, initial, reporter of the libabigail comparison engine.
class default_reporter : public reporter_base
{
public:

  virtual bool diff_has_net_changes(const corpus_diff *d) const;

  virtual void
  report(const type_decl_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const enum_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  void
  report_non_type_typedef_changes(const typedef_diff &d,
				  std::ostream& out,
				  const std::string& indent) const;

  virtual void
  report(const typedef_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  bool
  report_local_qualified_type_changes(const qualified_type_diff& d,
				      std::ostream& out,
				      const std::string& indent) const;

  virtual void
  report(const qualified_type_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const pointer_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  void
  report_local_reference_type_changes(const reference_diff& d,
				      std::ostream& out,
				      const std::string& indent) const;

  virtual void
  report(const reference_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const fn_parm_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  void
  report_local_function_type_changes(const function_type_diff& d,
				     std::ostream& out,
				     const std::string& indent) const;

  virtual void
  report(const function_type_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const array_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const base_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const scope_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const class_or_union_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const class_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const union_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const distinct_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const function_decl_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const var_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const translation_unit_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const corpus_diff& d, std::ostream& out,
	 const std::string& indent = "") const;
}; // end class default_reporter

/// A reporter that only reports leaf changes
class leaf_reporter : public default_reporter
{
public:

  virtual bool diff_to_be_reported(const diff *d) const;

  virtual bool diff_has_net_changes(const corpus_diff *d) const;

  void
  report_changes_from_diff_maps(const diff_maps&, std::ostream& out,
				const std::string& indent) const;

  virtual void
  report(const typedef_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const qualified_type_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const pointer_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const reference_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const fn_parm_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const function_type_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const array_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const scope_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const class_or_union_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const class_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const union_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const distinct_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const function_decl_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const var_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const translation_unit_diff& d, std::ostream& out,
	 const std::string& indent = "") const;

  virtual void
  report(const corpus_diff& d, std::ostream& out,
	 const std::string& indent = "") const;
}; // end class leaf_reporter

} // end namespace comparison
} // end namespace abigail

#endif // __ABG_REPORTER_H__

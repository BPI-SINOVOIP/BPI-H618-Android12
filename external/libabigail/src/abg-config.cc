// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-config.h"
#include "abg-version.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{
config::config()
  : m_format_minor("0"),
    m_format_major("2"),
    m_xml_element_indent(2),
    m_tu_instr_suffix(".bi"),
    m_tu_instr_archive_suffix(".abi")
{
}

const std::string&
config::get_format_minor_version_number() const
{return m_format_minor;}

void
config::set_format_minor_version_number(const std::string& v)
{m_format_minor = v;}

const std::string&
config::get_format_major_version_number() const
{return m_format_major;}

void
config::set_format_major_version_number(const std::string& v)
{m_format_major= v;}

unsigned
config::get_xml_element_indent() const
{ return m_xml_element_indent; }

void
config::set_xml_element_indent(unsigned indent)
{m_xml_element_indent = indent;}

const std::string&
config::get_tu_instr_suffix() const
{return m_tu_instr_suffix;}

void
config::set_tu_instr_suffix(const std::string& s)
{m_tu_instr_suffix = s;}

const std::string&
config::get_tu_instr_archive_suffix() const
{return m_tu_instr_archive_suffix;}

void
config::set_tu_instr_archive_suffix(const std::string& s)
{m_tu_instr_archive_suffix = s; }

extern "C"
{
void
abigail_get_library_version(std::string& major,
			    std::string& minor,
			    std::string& revision,
			    std::string& suffix)
{
  major = ABIGAIL_VERSION_MAJOR;
  minor = ABIGAIL_VERSION_MINOR;
  revision = ABIGAIL_VERSION_REVISION;
  suffix = ABIGAIL_VERSION_SUFFIX;
}

}
}//end namespace abigail

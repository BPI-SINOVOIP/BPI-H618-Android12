// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#ifndef __ABG_LIBXML_UTILS_H__
#define __ABG_LIBXML_UTILS_H__

#include <libxml/xmlreader.h>

#include <istream>
#include <memory>

#include "abg-sptr-utils.h"

namespace abigail
{

/// Internal namespace for xml manipulation utilities.
namespace xml
{

using sptr_utils::build_sptr;
using std::shared_ptr;

/// A convenience typedef for a shared pointer of xmlTextReader.
typedef shared_ptr<xmlTextReader> reader_sptr;

/// A convenience typedef for a shared pointer of xmlChar.
typedef shared_ptr<xmlChar> xml_char_sptr;

/// This functor is used to instantiate a shared_ptr for the
/// xmlTextReader.
struct textReaderDeleter
{
  void
  operator()(xmlTextReaderPtr reader)
  {xmlFreeTextReader(reader);}
};

/// This functor is used to instantiate a shared_ptr for xmlChar
struct charDeleter
{
  void
  operator()(xmlChar* str)
  { xmlFree(str); }
};

reader_sptr new_reader_from_file(const std::string& path);
reader_sptr new_reader_from_buffer(const std::string& buffer);
reader_sptr new_reader_from_istream(std::istream*);
bool xml_char_sptr_to_string(xml_char_sptr, std::string&);

int get_xml_node_depth(xmlNodePtr);

/// Get the name of the current element node the reader is pointing
/// to.  Note that this macro returns an instance of
/// shared_ptr<xmlChar> so that the caller doesn't have to worry about
/// managing memory itself.  Also note that the reader is a
/// shared_ptr<xmlTextReader>
#define XML_READER_GET_NODE_NAME(reader) \
  xml::build_sptr(xmlTextReaderName(reader.get()))

/// Get the type of the current node of the shared_ptr<xmlTextReader>
/// passed in argument.
#define XML_READER_GET_NODE_TYPE(reader) \
  static_cast<xmlReaderTypes> (xmlTextReaderNodeType(reader.get()))

/// Get the value of attribute 'name' on the current node of 'reader'
/// which is an instance of shared_ptr<xmlTextReader>.
#define XML_READER_GET_ATTRIBUTE(reader, name) \
  xml::build_sptr(xmlTextReaderGetAttribute(reader.get(), BAD_CAST(name)))

/// Get the value of attribute 'name' ont the instance of xmlNodePtr
/// denoted by 'node'.
#define XML_NODE_GET_ATTRIBUTE(node, name) \
  xml::build_sptr(xmlGetProp(node, BAD_CAST(name)))

#define CHAR_STR(xml_char_str) \
  reinterpret_cast<char*>(xml_char_str.get())

xmlNodePtr
advance_to_next_sibling_element(xmlNodePtr node);

void
escape_xml_string(const std::string& str,
		  std::string& escaped);

std::string
escape_xml_string(const std::string& str);

void
escape_xml_comment(const std::string& str,
		   std::string& escaped);

std::string
escape_xml_comment(const std::string& str);

void
unescape_xml_string(const std::string& str,
		    std::string& escaped);

std::string
unescape_xml_string(const std::string& str);

void
unescape_xml_comment(const std::string& str,
		     std::string& escaped);

std::string
unescape_xml_comment(const std::string& str);

}//end namespace xml

namespace sptr_utils
{
/// Specialization of sptr_utils::build_sptr for xmlTextReader
template<>
xml::reader_sptr
build_sptr<xmlTextReader>(xmlTextReader *p);

/// Specialization of build_str for xmlChar.
template<>
xml::xml_char_sptr
build_sptr<xmlChar>(xmlChar *p);
}// end namespace sptr_utils

}//end namespace abigail
#endif //__ABG_LIBXML_UTILS_H__

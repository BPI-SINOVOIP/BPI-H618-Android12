// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#ifndef __ABG_VIZ_SVG_H__
#define __ABG_VIZ_SVG_H__

#include <abg-viz-common.h>

namespace abigail
{

/**
  Row displaying one element of member data.

  Wide open background spaces, what do they look like, what do the
  things in the foreground look like? Rectangle, for one.

  Some characteristics:
  - horizontal label  (text anchor = start ie left).
  - background box
  - text data (text anchor = middle ie centered).
 */
struct row
{
  std::string		_M_id;
  const style&		_M_style;

  units_type		_M_offset;
  units_type		_M_size;
  units_type		_M_align;
};

/// Useful row constants. Maybe just do enum->value map.
extern const style primary_row_sty;
extern const style base_row_sty;
extern const style member_row_sty;
extern const style implementation_row_sty;


/**
  SVG "array" style notation for size/layout/alignment.

  This is a compact SVG representation of class layout.

  It is composed of a minimum of three data points for each member or
  base of a class:

  - offset index
  - size
  - align

  Including annotations for holes, padding, and
  implementation-specified and otherwise invisible objects.

  It's constructed by creating vertical columns for each of the data
  points specified above, and filling in horizontal rows for each of
  the class components.
 */
struct svg
{

private:

  const std::string    	_M_title;
  const canvas&	       	_M_canvas;
  const typography&    	_M_typo;	

  const units_type	_M_x_size = 3;	// Number of columns
  units_type   		_M_x_space;	// Column spacing.
  units_type   		_M_x_origin;	// X origin

  units_type   		_M_y_size;	// Number of rows
  units_type   		_M_y_space;	// Row spacing.
  units_type   		_M_y_origin;	// Y origin

  std::ostringstream   	_M_sstream;
  
  // static const units_type _M_stroke_width = 1;
  // static const units_type _M_text_padding = 10;

public:

  svg(const std::string &__title,
      const canvas& __cv = ansi_letter_canvas,
      const typography& __typo = arial_typo) 
  : _M_title(__title), _M_canvas(__cv), _M_typo(__typo), _M_y_size(0)
  { 
    // Offsets require: typo, canvas units, size.
    _M_x_space = 40;
    _M_y_space = 40;
    _M_x_origin = _M_x_space * 1;
    _M_y_origin = _M_y_space * 2;
  }
  
  // Empty when the output buffer is.
  bool
  empty() { return _M_sstream.str().empty(); }

  void 
  start_element();
  
  void 
  finish_element();
  
  void 
  add_title();

  void 
  add_y_row(const row&);

  void 
  add_y_lines();

  void 
  add_y_label();

  void
  write();

  void 
  start()
  {
    this->start_element();
    this->add_title();
  }

  void 
  finish()
  {
    this->add_y_label();
    this->add_y_lines();
    this->finish_element();
    this->write();
  }
};

// XXX connect external xml file to input. 
// parse input, pick apart elements, attributes.

}// end namespace abigail

#endif //__ABG_VIZ_SVG_H__

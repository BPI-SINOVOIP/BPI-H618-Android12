// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#ifndef __ABG_VIZ_COMMON_H__
#define __ABG_VIZ_COMMON_H__

#include <sstream> //stringstream
#include <string>
#include <tuple>
#include <array>

namespace abigail
{

/// Utility function, like regex_replace.
void
string_replace(std::string&, const std::string&, const std::string&);


/// Measurement abstraction type, conversion function.
enum class units
{
  // NB: 1 pixel = .264583 mm
  millimeter,		// mm
  pixel			// px
};

std::string
units_to_string(units);

typedef unsigned short	units_type;


/**
  Page/Canvas/Drawing area description.
  Size, origin location in 2D (x,y), heigh, width

  ANSI Letter mm == (units::millimeter, 215.9, 279.4);
  ANSI Letter pixels == (units::pixel, 765, 990);
  ISO A4 mm == (units::millimeter, 210, 297);
  ISO A4 pixels == (units::pixel, 744.09, 1052.36);
 */
struct canvas
{
  units			_M_units;
  units_type		_M_width;
  units_type		_M_height;
};

/// Useful canvas constants.
extern const canvas	ansi_letter_canvas;
extern const canvas	iso_a4_canvas;


/// Color, conversion function.
enum class color
{
  white,
  gray25,		// gainsboro
  gray75,		// slategray
  black
};

std::string
color_to_string(color);


/**
  Character rendering, type, fonts, styles.

  Expect to keep changing the output, so use this abstraction to set
  styling defaults, so that one can just assign types instead of doing
  a bunch of search-and-replace operations when changing type
  characteristics.
 */
struct typography
{
  enum anchor { start, middle };

  std::string		_M_face;	// System font name
  unsigned short	_M_size;	// Display size
  color			_M_color;
  std::string		_M_attributes;	// Any other attributes

  std::string
  to_attribute(anchor) const;

  std::string
  anchor_to_string(anchor) const;
};

/// Useful typography constants.
extern const typography arial_typo;
extern const typography source_code_pro_typo;
extern const typography roboto_light_typo;

 
/// Datum consolidating style preferences.
struct style
{
  color	       		_M_text_color;
  color	       		_M_fill_color;
  std::string		_M_attributes;
};


}// end namespace abigail

#endif //__ABG_VIZ_COMMON_H__

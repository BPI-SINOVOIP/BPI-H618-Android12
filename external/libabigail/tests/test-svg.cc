// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2019 Red Hat, Inc.

#include "abg-viz-svg.h"

int main()
{
  using namespace abigail;

  // sa-base
  {
    svg obj("sa-base");
    row r = { "base", primary_row_sty, 0, 1, 1 };
    obj.start(); 
    obj.add_y_row(r);
    obj.finish();
  }

  // sa-A
  {
    svg obj("sa-A");
    row r1 = { "A", primary_row_sty, 0, 24, 8 };
    row r2 = { "base", base_row_sty, 0, 0, 0 };
    row r3 = { "_M_i", member_row_sty, 0, 8, 8 };
    row r4 = { "_M_n", member_row_sty, 8, 8, 8 };
    row r5 = { "_M_type", member_row_sty, 16, 2, 2 };
    row r6 = { "PADDING", implementation_row_sty, 18, 6, 0 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.add_y_row(r5);
    obj.add_y_row(r6);
    obj.finish();
  }

  // sa-B
  {
    svg obj("sa-B");
    row r1 = { "B", primary_row_sty, 0, 24, 8 };
    row r2 = { "base", base_row_sty, 0, 0, 0 };
    row r3 = { "_M_type", member_row_sty, 0, 2, 2 };
    row r4 = { "HOLE", implementation_row_sty, 2, 2, 0 };
    row r5 = { "_M_state", member_row_sty, 4, 4, 4 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.add_y_row(r5);
    obj.finish();
  }

  // sa-C1
  {
    svg obj("sa-C1");
    row r1 = { "C1", primary_row_sty, 0, 2, 2 };
    row r2 = { "base", base_row_sty, 0, 0, 0 };
    row r3 = { "_M_type", member_row_sty, 0, 2, 2 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.finish();
  }

  // sa-C2
  {
    svg obj("sa-C2");
    row r1 = { "C2", primary_row_sty, 0, 16, 8 };
    row r2 = { "base", base_row_sty, 0, 0, 0 };
    row r3 = { "IMPLEMENTATION", implementation_row_sty, 0, 8, 8 };
    row r4 = { "_M_type", member_row_sty, 8, 2, 2 };
    row r5 = { "PADDING", implementation_row_sty, 10, 6, 0 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.add_y_row(r5);
    obj.finish();
  }

  // sa-C3
  {
    svg obj("sa-C3");
    row r1 = { "C3", primary_row_sty, 0, 16, 8 };
    row r2 = { "base", base_row_sty, 0, 0, 0 };
    row r3 = { "IMPLEMENTATION", implementation_row_sty, 0, 8, 8 };
    row r4 = { "_M_type", member_row_sty, 8, 2, 2 };
    row r5 = { "PADDING", implementation_row_sty, 10, 6, 0 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.add_y_row(r5);
    obj.finish();
  }

  // sa-D1
  {
    svg obj("sa-D1");
    row r1 = { "D1", primary_row_sty, 0, 48, 8 };
    row r2 = { "A", base_row_sty, 0, 24, 8 };
    row r3 = { "_M_index", member_row_sty, 24, 8, 8 };
    row r4 = { "_M_array", member_row_sty, 32, 8, 8 };
    row r5 = { "_M_ok", member_row_sty, 40, 1, 1 };
    row r6 = { "PADDING", implementation_row_sty, 41, 7, 0 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.add_y_row(r5);
    obj.add_y_row(r6);
    obj.finish();
  }

  // sa-D2
  {
    svg obj("sa-D2");
    row r1 = { "D2", primary_row_sty, 0, 32, 8 };
    row r2 = { "C1", base_row_sty, 0, 2, 2 };
    row r3 = { "HOLE", implementation_row_sty, 2, 2, 0 };
    row r4 = { "_M_state", member_row_sty, 4, 4, 4 };
    row r5 = { "_M_index", member_row_sty, 8, 8, 8 };
    row r6 = { "_M_array", member_row_sty, 16, 8, 8 };
    row r7 = { "_M_ok", member_row_sty, 24, 1, 1 };
    row r8 = { "PADDING", implementation_row_sty, 41, 7, 0 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.add_y_row(r5);
    obj.add_y_row(r6);
    obj.add_y_row(r7);
    obj.add_y_row(r8);
    obj.finish();
  }

  // sa-D2v
  {
    svg obj("sa-D2v");
    row r1 = { "D2v", primary_row_sty, 0, 56, 8 };
    row r2 = { "C2", base_row_sty, 0, 16, 16 };
    row r3 = { "IMPLEMENTATION", implementation_row_sty, 16, 8, 8 };
    row r4 = { "_M_state", member_row_sty, 24, 4, 4 };
    row r5 = { "HOLE", implementation_row_sty, 28, 4, 0 };
    row r6 = { "_M_index", member_row_sty, 32, 8, 8 };
    row r7 = { "_M_array", member_row_sty, 40, 8, 8 };
    row r8 = { "_M_ok", member_row_sty, 48, 1, 1 };
    row r9 = { "PADDING", implementation_row_sty, 49, 7, 0 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.add_y_row(r5);
    obj.add_y_row(r6);
    obj.add_y_row(r7);
    obj.add_y_row(r8);
    obj.add_y_row(r9);
    obj.finish();
  }

  // sa-D3
  {
    svg obj("sa-D3");
    row r1 = { "D3", primary_row_sty, 0, 24, 8 };
    row r2 = { "C1", base_row_sty, 0, 2, 2 };
    row r3 = { "HOLE", implementation_row_sty, 2, 6, 0 };
    row r4 = { "_M_index", member_row_sty, 8, 8, 8 };
    row r5 = { "_M_array", member_row_sty, 16, 8, 8 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.add_y_row(r5);
    obj.finish();
  }

  // sa-D3v
  {
    svg obj("sa-D3v");
    row r1 = { "D3v", primary_row_sty, 0, 40, 8 };
    row r2 = { "C3", base_row_sty, 0, 16, 16 };
    row r3 = { "IMPLEMENTATION", implementation_row_sty, 16, 8, 8 };
    row r4 = { "_M_index", member_row_sty, 24, 8, 8 };
    row r5 = { "_M_array", member_row_sty, 32, 8, 8 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.add_y_row(r5);
    obj.finish();
  }

  // sa-D4v
  {
    svg obj("sa-D4v");
    row r1 = { "D4v", primary_row_sty, 0, 32, 8 };
    row r2 = { "C4", base_row_sty, 0, 16, 16 };
    row r3 = { "_M_index", member_row_sty, 16, 8, 8 };
    row r4 = { "_M_array", member_row_sty, 24, 8, 8 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.add_y_row(r4);
    obj.finish();
  }

  // sa-Dd1
  {
    svg obj("sa-Dd1");
    row r1 = { "Dd1", primary_row_sty, 0, 32, 8 };
    row r2 = { "A", base_row_sty, 0, 24, 8 };
    row r3 = { "B", base_row_sty, 24, 8, 8 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.finish();
  }

  // sa-Dd2
  {
    svg obj("sa-Dd2");
    row r1 = { "Dd2", primary_row_sty, 0, 56, 8 };
    row r2 = { "A", base_row_sty, 0, 24, 8 };
    row r3 = { "D4v", base_row_sty, 24, 32, 8 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.finish();
  }

  // sa-Dd3
  {
    svg obj("sa-Dd3");
    row r1 = { "Dd3", primary_row_sty, 0, 56, 8 };
    row r2 = { "D2", base_row_sty, 0, 32, 8 };
    row r3 = { "D3", base_row_sty, 32, 24, 8 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.finish();
  }

  // sa-Dd4
  {
    svg obj("sa-Dd4");
    row r1 = { "Dd4", primary_row_sty, 0, 96, 8 };
    row r2 = { "D2v", base_row_sty, 0, 56, 8 };
    row r3 = { "D3v", base_row_sty, 56, 40, 8 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.finish();
  }

  // sa-Dd5
  {
    svg obj("sa-Dd5");
    row r1 = { "Dd5", primary_row_sty, 0, 48, 8 };
    row r2 = { "D5v1", base_row_sty, 0, 32, 8 };
    row r3 = { "D5v2", base_row_sty, 16, 32, 8 };
    obj.start(); 
    obj.add_y_row(r1);
    obj.add_y_row(r2);
    obj.add_y_row(r3);
    obj.finish();
  }

  return 0;
}

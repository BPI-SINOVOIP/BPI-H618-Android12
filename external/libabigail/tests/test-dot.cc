// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2019 Red Hat, Inc.

#include "abg-viz-dot.h"

int main()
{
  using namespace abigail;

  // sa-base
  {
    dot obj("sa-base");
    parent_node p("base");
    obj.start(); 
    obj.add_parent(p);
    obj.finish();
  }


  // sa-A
  {
    dot obj("sa-A");
    parent_node p("base");
    child_node c1("A");
    obj.start(); 
    obj.add_parent(p);
    obj.add_child_to_node(c1, p);
    obj.finish();
  }

  // sa-B
  {
    dot obj("sa-B");
    parent_node p("base");
    child_node c1("B");
    obj.start(); 
    obj.add_parent(p);
    obj.add_child_to_node(c1, p);
    obj.finish();
  }

  // sa-D1
  {
    dot obj("sa-D1");
    parent_node p("base");
    child_node c1("A");
    child_node c2("D1");
    obj.start(); 
    obj.add_parent(p);
    obj.add_child_to_node(c1, p);
    obj.add_child_to_node(c2, c1);
    obj.finish();
  }

  return 0;
}

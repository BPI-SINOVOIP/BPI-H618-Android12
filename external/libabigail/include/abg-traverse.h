// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#ifndef __ABG_TRAVERSE_H__
#define __ABG_TRAVERSE_H__

#include "abg-fwd.h"

namespace abigail
{

namespace ir
{

/// The base class for the visitor type hierarchy used for traversing
/// a hierarchy of nodes.
///
/// Client code willing to get notified for a certain kind of node
/// during traversal might want to define a visitor class that inherit
/// \a node_visitor, define and overload a node_visitor::visit method
/// for it, like what is done for the ir_node_visitor::visit set of
/// functions, for traversing internal representation nodes.
struct node_visitor_base
{};

/// The interface for types which are feeling social and want to be
/// visited during the traversal of a hierarchy of nodes.
///
/// It is expected that classes derived from traversable_base define a
/// traverse method specialised to the node *visitor* type. Such
/// methods should visit nodes recursively, calling
/// ir_node_visitor::visit for each node. The method should return
/// true until all nodes have been visited.
class traversable_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

protected:

  traversable_base();

  bool visiting() const;

  void visiting(bool f);

public:

   virtual ~traversable_base();

  /// This virtual method is overloaded and implemented by any single
  /// type which instance is going to be visited during the traversal
  /// of translation unit nodes.
  ///
  /// The method visits a given node and, for scopes, visits their
  /// member nodes.  Visiting a node means calling the
  /// ir_node_visitor::visit method with the node passed as an
  /// argument.
  ///
  /// @param v the visitor used during the traverse.
  ///
  /// @return true if traversed until the end of the type tree, false
  /// otherwise.
  ///
  /// Note that each class that derives from this one and overloads
  /// this method will have to define a type for its node visitor
  /// argument (the one named v).  That type will have to derive from
  /// the node_visitor_base type.  In that sense, this new overload
  /// method will "hide" this one.  That is why we are keeping this
  /// method commented, for documentation purposes.

  // virtual bool traverse(node_visitor_base& v);
}; // end class traversable_base

}// end namespace ir.
}//end namespace abigail
#endif //__ABG_TRAVERSE_H__

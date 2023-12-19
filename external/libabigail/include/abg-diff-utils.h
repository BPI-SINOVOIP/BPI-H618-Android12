// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file
///
/// This file declares types and operations implementing the "O(ND)
/// Difference Algorithm" (aka diff2) from Eugene W. Myers, to compute
/// the difference between two sequences.
///
/// To understand what is going on here, one must read the paper at
/// http://www.xmailserver.org/diff2.pdf.  Throughout this file, that
/// paper is referred to as "the paper".
///
/// The implementations goes as far as calculating the shortest edit
/// script (the set of insertions and deletions) for transforming a
/// sequence into another.  The main entry point for that is the
/// compute_diff() function.

#ifndef __ABG_DIFF_UTILS_H__
#define __ABG_DIFF_UTILS_H__

#include <cassert>
#include <cstdlib>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "abg-fwd.h"

namespace abigail
{

/// @brief Libabigail's core diffing algorithms
///
/// This is the namespace defining the core diffing algorithm used by
/// libabigail @ref comparison tools.  This based on the diff
/// algorithm from Eugene Myers.
namespace diff_utils
{

using std::shared_ptr;

// Inject the names from std:: below into this namespace
using std::string;
using std::ostream;
using std::vector;
using std::abs;
using std::ostringstream;

/// A class representing a vertex in an edit graph, as explained in
/// the paper.  A vertex is a basically a pair of coordinates
/// (abscissa and ordinate).
class point
{
  int x_;
  int y_;
  bool empty_;

public:

  point()
    : x_(-1), y_(-1),empty_(true)
  {}

  point(int x, int y)
    : x_(x), y_(y), empty_(false)
  {}

  point(const point& p)
    : x_(p.x()), y_(p.y()), empty_(p.is_empty())
  {}

  int
  x() const
  {return x_;}

  void
  x(int x)
  {
    x_ = x;
    empty_ = false;
  }

  int
  y() const
  {return y_;}

  void
  y(int y)
  {
    y_ = y;
    empty_ = false;
  }

  void
  set(int x, int y)
  {
    x_ = x;
    y_ = y;
    empty_ = false;
  }

  void
  set(int x, int y, bool empty)
  {
    x_ = x;
    y_ = y;
    empty_ = empty;
  }

  void
  add(int ax, int ay)
  {set (x() + ax, y() + ay);}

  bool
  operator!=(const point& o) const
  {return (x() != o.x() || y() != o.y() || is_empty() != o.is_empty());}

  bool
  operator==(const point& o) const
  {return !(operator!=(o));}

  bool
  operator<(const point& o) const
  {return (x() < o.x() && y() < o.y());}

  bool
  operator>(const point& o) const
  {return (x() > o.x() && y() > o.y());}

  bool
  operator<=(const point& o) const
  {return (x() <= o.x() && y() <= o.y());}

  bool
  operator>=(const point& o) const
  {return (x() >= o.x() && y() >= o.y());}

  point
  operator+(int val) const
  {return point(x() + val, y() + val);}

  point
  operator-(int val) const
  {return point(x() - val, y() - val);}

  point&
  operator+= (int val)
  {
    set(x_ + val, y_ + val);
    return *this;
  }

  point&
  operator-= (int val)
  {return (*this) += (-val);}

  point&
  operator--()
  {return (*this) -= 1;}

  point&
  operator++()
  {return (*this) += 1;}

  point
  operator--(int)
  {
    point tmp(*this);
    --(*this);
    return tmp;
  }

  point
  operator++(int)
  {
    point tmp(*this);
    ++(*this);
    return tmp;
  }

  point&
  operator=(int val)
  {
    set(val, val);
    return *this;
  }

  point&
  operator=(const point& p)
  {
    set(p.x(), p.y(), p.is_empty());
    return *this;
  }

  bool
  is_empty() const
  {return empty_;}

  operator bool () const
  {return !is_empty();}

  bool
  operator!() const
  {return is_empty();}

  void
  clear()
  {
    x_ = -1;
    y_ = -1;
    empty_ = true;
  }
};// end point

/// The abstraction of the Snake concept, from the paper.
///
/// In a given path from the edit graph, a snake is a non-diagonal
/// edge followed by zero or more diagonal edges.
///
/// The starting poing of the non-diagonal edge is the beginning of
/// the snake.  This is given by the snake::begin() method.  This point
/// is not explicitely referenced in the paper, but we need it for
/// some grunt implementation details of the algorithm.
///
/// The end point of the non-diagonal edge is the intermediate point
/// of the snake; it's given by the snake::intermediate() method.
/// This point is what is referred to as "the begining of the snake"
/// in the paper.
///
/// The end point of the first diagonal edge is given by the
/// snake::diagonal_start() method.
///
/// The end point of the last diagonal edge is given by the
/// snake::end() method.  Note that when the snake contains no
/// diagonal edge, snake::intermediate(), and snake::end() return the
/// same point; snake::diagonal_start() contains an empty point (i.e,
/// a point for which point::is_empty() returns true).
class snake
{
  point begin_, intermediate_, diagonal_start_, end_;
  bool forward_;

public:

  /// Default constructor for snake.
  snake()
    : forward_(false)
  {}

  /// Constructor from the beginning, intermediate and end points.
  ///
  /// @param b the beginning point of the snake.  That is, the
  /// starting point of the non-diagonal edge.
  ///
  /// @param i the intermediate point of the snake.  That is, the end
  /// point of the non-diagonal edge.
  ///
  /// @param e the end point of the snake.  That is the end point of
  /// the last diagonal edge.
  snake(const point& b,
	const point& i,
	const point& e)
    : begin_(b), intermediate_(i),
      end_(e), forward_(false)
  {}

  /// Constructor from the beginning, intermediate and end points.
  ///
  /// @param b the beginning point of the snake.  That is, the
  /// starting point of the non-diagonal edge.
  ///
  /// @param i the intermediate point of the snake.  That is, the end
  /// point of the non-diagonal edge.
  ///
  /// @param d the beginning of the diagonal edge.  That is the end of
  /// the first diagonal edge of the snake.
  ///
  /// @param e the end point of the snake.  That is the end point of
  /// the last diagonal edge.
  snake(const point& b,
	const point& i,
	const point& d,
	const point& e)
    : begin_(b), intermediate_(i),
      diagonal_start_(d), end_(e),
      forward_(false)
  {}

  /// Getter for the starting point of the non-diagonal edge of the
  /// snake.
  ///
  /// @return the starting point of the non-diagonal edge of the snake
  const point&
  begin() const
  {return begin_;}

  /// Getter for the starting point of the non-diagonal edge of the
  /// snake, aka begin point.
  ///
  ///@param p the new begin point.
  void
  begin(const point& p)
  {begin_ = p;}

  /// Getter for the end point of the non-diagonal edge of the snake.
  ///
  /// @return the end point of the non-diagonal edge of the snake
  const point&
  intermediate() const
  {return intermediate_;}

  /// Setter for the end point of the non-diagonal edge of the snake,
  /// aka intermediate point.
  ///
  /// @param p the new intermediate point.
  void
  intermediate(const point& p)
  {intermediate_ = p;}

  /// Getter for the end point of the first diagonal edge, aka
  /// diagonal start point.  Note that if the snake has no diagonal
  /// edge, this point is empty.
  ///
  /// @return the end point of the first diagonal edge.
  const point&
  diagonal_start() const
  {return diagonal_start_;}

  /// Setter for the end point of the first diagonal edge, aka
  /// diagonal start point.
  ///
  /// @param p the new diagonal start.d
  void
  diagonal_start(const point& p)
  {diagonal_start_ = p;}

  /// Getter for the end point of the last diagonal edge, aka snake
  /// end point.  Note that if the snake has no diagonal edge, this
  /// point is equal to the intermediate point.
  ///
  /// @return the end point of the last diagonal edge
  const point&
  end() const
  {return end_;}

  /// Setter for the end point of the last diagonal edge, aka snake
  /// end point.  Note that if the snake has no diagonal edge, this
  /// point is equal to the intermediate point.
  void
  end(const point& p)
  {end_ = p;}

  /// Setter for the begin, intermediate and end points of the snake.
  ///
  /// @param b the new snake begin point
  ///
  /// @param i the new snake intermediate point
  ///
  /// @param e the new snake end point
  void
  set(const point& b, const point&i, const point&e)
  {
    begin(b);
    intermediate(i);
    end(e);
  }

  /// Setter for the begin, intermediate, diagonal start and end points
  /// of the snake.
  ///
  /// @param b the new snake begin point
  ///
  /// @param i the new snake intermediate point
  ///
  /// @param d the new diagonal start point
  ///
  /// @param e the new snake end point
  void
  set(const point& b, const point&i, const point& d, const point&e)
  {
    begin(b);
    intermediate(i);
    diagonal_start(d);
    end(e);
  }

  /// @return true iff the snake is a forward snake.  That is, if it
  /// was built while walking the edit graph going forward (from the
  /// top left corner to the right bottom corner.
  bool
  is_forward() const
  {return forward_;}

  /// Set to true if the snake is a forward snake; that is, if it was
  /// built while walking the edit graph going forward (from the top
  /// left corner to the right bottom corner.  Set to false otherwise.
  ///
  /// @param f whether the snake is a forward snake or not.
  void
  set_forward(bool f)
  {forward_ = f;}

  /// Add an offset to the abscissas of the points of the snake, and
  /// add another offset to the ordinates of these same points.
  ///
  /// @param x_offset the offset to add to the abscissas of all the
  /// points of the snake.
  ///
  /// @param y_offset the offset to add to the ordinates of all the
  /// points of the snake.
  void
  add(int x_offset, int y_offset)
  {
    if (is_empty())
      return;

    begin_.add(x_offset, y_offset);
    intermediate_.add(x_offset, y_offset);
    if (diagonal_start_)
      diagonal_start_.add(x_offset, y_offset);
    end_.add(x_offset, y_offset);
  }

  /// @return true iff the snake has at least one diagonal edge.
  bool
  has_diagonal_edge() const
  {return !diagonal_start().is_empty();}

  /// @return true iff the non-diagonal edge is horizontal.
  bool
  has_horizontal_edge() const
  {return (begin().y() == intermediate().y());}

  /// @return true iff the non-diagonal edge is vertical.
  bool
  has_vertical_edge() const
  {return (begin().x() == intermediate().x());}

  /// @return true iff the snake is empty, that is, if all the points
  /// it contains are empty.
  bool is_empty() const
  {return begin().is_empty() && intermediate().is_empty() && end().is_empty();}
};// end class snake

/// The array containing the furthest D-path end-points, for each value
/// of K.  MAX_D is the maximum value of the D-Path.  That is, M+N if
/// M is the size of the first input string, and N is the size of the
/// second.
class d_path_vec : public std::vector<int>
{
private:

  unsigned a_size_;
  unsigned b_size_;

  /// Forbid vector size modifications
  void
  push_back(const vector<int>::value_type&);

  /// Forbid default constructor.
  d_path_vec();

  bool
  over_bounds(long long index) const
  {return  (index + offset()) >= (long long) size();}

  void
  check_index_against_bound(int index) const
  {
    if (over_bounds(index))
      {
	ostringstream o;
	o << "index '" << index
	  << "' out of range [-" << max_d() << ", " << max_d() << "]";
	throw std::out_of_range(o.str());
      }
  }

public:

  /// Constructor of the d_path_vec.
  ///
  /// For forward vectors, the underlying vector allocates 2 *
  /// [MAX_D+1].
  /// space, so that one can address elements in the index range
  /// [-MAX_D, MAX_D].  And MAX_D is the sum of the two sequence
  /// sizes. delta is the difference.
  ///
  /// For reverse vectors, note that we need to be able to address
  /// [-MAX_D - delta, MAX_D + delta], with delta being the (signed)
  /// difference between the size of the two sequences.  We consider
  /// delta being bounded by MAX_D itself; so we say we need to be
  /// able to address [-2MAX_D, 2MAX_D].
  ///
  /// @param size1 the size of the first sequence we are interested
  /// in.
  ///
  /// @param size2 the size of the second sequence we are interested
  /// in.
  d_path_vec(unsigned size1, unsigned size2)
    : vector<int>(2 * (size1 + size2 + 1 + (size1 + size2)) + 1, 0),
      a_size_(size1), b_size_(size2)
  {
  }

  std::vector<int>::const_reference
  operator[](int index) const
  {return at(index);}

  std::vector<int>::reference
  operator[](int index)
  {return at(index);}

  std::vector<int>::reference
  at(long long index)
  {
    //check_index_against_bound(index);
    long long i = index + offset();
    return vector<int>::operator[](i);
  }

  std::vector<int>::const_reference
  at(long long index) const
  {
    check_index_against_bound(index);
    long long i = offset() + index;
    return static_cast<const vector<int>* >(this)->at(i);
  }

  unsigned
  a_size() const
  {return a_size_;}

  unsigned
  b_size() const
  {return b_size_;}

  unsigned
  max_d() const
  {return a_size_ + b_size_;}

  unsigned
  offset() const
  {return max_d() + abs((long long) a_size() - (long long) b_size());}
}; // end class d_path_vec

/// The abstration of an insertion of elements of a sequence B into a
/// sequence A.  This is used to represent the edit script for
/// transforming a sequence A into a sequence B.
///
/// And insertion mainly encapsulates two components:
///
///   - An insertion point: this is the index (starting at 0) of the
///     element of the sequence A after which the insertion occurs.
///
///   - Inserted elements: this is a vector of indexes of elements of
///     sequence B (starting at 0) that got inserted into sequence A,
///     after the insertion point.
class insertion
{
  int			insertion_point_;
  vector<unsigned>	inserted_;

public:

  insertion(int insertion_point,
	    const vector<unsigned>& inserted_indexes)
    : insertion_point_(insertion_point),
      inserted_(inserted_indexes)
  {}

    insertion(int insertion_point = 0)
      : insertion_point_(insertion_point)
  {}

  int
  insertion_point_index() const
  {return insertion_point_;}

  void
  insertion_point_index(int i)
  {insertion_point_ = i;}

  const vector<unsigned>&
  inserted_indexes() const
  {return inserted_;}

  vector<unsigned>&
  inserted_indexes()
  {return inserted_;}
};// end class insertion

/// The abstraction of the deletion of one element of a sequence A.
///
/// This encapsulates the index of the element A that got deleted.
class deletion
{
  int index_;

public:

  deletion(int i)
    : index_(i)
  {}

  int
  index() const
  {return index_;}

  void
  index(int i)
  {index_ = i;}
};// end class deletion

/// The abstraction of an edit script for transforming a sequence A
/// into a sequence B.
///
/// It encapsulates the insertions and deletions for transforming A
/// into B.
class edit_script
{
  vector<insertion>	insertions_;
  vector<deletion>	deletions_;

public:

  edit_script()
  {}

  const vector<insertion>&
  insertions() const
  {return insertions_;}

  vector<insertion>&
  insertions()
  {return insertions_;}

  const vector<deletion>&
  deletions() const
  {return deletions_;}

  vector<deletion>&
  deletions()
  {return deletions_;}

  void
  append(const edit_script& es)
  {
    insertions().insert(insertions().end(),
			es.insertions().begin(),
			es.insertions().end());
    deletions().insert(deletions().end(),
		       es.deletions().begin(),
		       es.deletions().end());
  }

  void
  prepend(const edit_script& es)
  {
    insertions().insert(insertions().begin(),
			es.insertions().begin(),
			es.insertions().end());
    deletions().insert(deletions().begin(),
		       es.deletions().begin(),
		       es.deletions().end());
  }

  void
  clear()
  {
    insertions().clear();
    deletions().clear();
  }

  bool
  is_empty() const
  {return insertions().empty() && deletions().empty();}

  operator bool() const
  {return !is_empty();}

  int
  num_insertions() const
  {
    int l = 0;
    for (vector<insertion>::const_iterator i = insertions().begin();
	 i != insertions().end();
	 ++i)
      l += i->inserted_indexes().size();
    return l;
  }

  int
  num_deletions() const
  {return deletions().size();}

  int
  length() const
  {return num_insertions() + num_deletions();}
};//end class edit_script

bool
point_is_valid_in_graph(point& p,
			unsigned a_size,
			unsigned b_size);

bool
ends_of_furthest_d_paths_overlap(const point& forward_d_path_end,
				 const point& reverse_d_path_end);

/// The default equality functor used by the core diffing algorithms.
struct default_eq_functor
{
  /// This equality operator uses the default "==" to compare its
  /// arguments.
  ///
  /// @param a the first comparison argument.
  ///
  /// @param b the second comparison argument.
  ///
  /// @return true if the two arguments are equal, false otherwise.
  template<typename T>
  bool
  operator()(const T a, const T b) const
  {return a == b;}
};


/// An equality functor to deeply compare pointers.
struct deep_ptr_eq_functor
{
  /// This equality operator compares pointers by comparing the
  /// pointed-to objects.
  ///
  /// @param first the first comparison argument.
  ///
  /// @param second the second comparison argument.
  ///
  /// @return true if the objects pointed to by the pointers are
  /// equal, false otherwise.
  template<typename T>
  bool
  operator()(const T* first,
	     const T* second) const
  {
    if (!!first != !!second)
      return false;

    if (!first)
      return true;

    return *first == *second;
  }

  /// This equality operator compares pointers by comparing the
  /// pointed-to objects.
  ///
  /// @param first the first comparison argument.
  ///
  /// @param second the second comparison argument.
  ///
  /// @return true if the objects pointed to by the pointers are
  /// equal, false otherwise.
  template<typename T>
  bool
  operator()(const shared_ptr<T> first,
	     const shared_ptr<T> second) const
  {return operator()(first.get(), second.get());}

  /// This equality operator compares pointers by comparing the
  /// pointed-to objects.
  ///
  /// @param first the first comparison argument.
  ///
  /// @param second the second comparison argument.
  ///
  /// @return true if the objects pointed to by the pointers are
  /// equal, false otherwise.
  template<typename T>
  bool
  operator()(const weak_ptr<T> first,
	     const weak_ptr<T> second) const
  {return operator()(shared_ptr<T>(first), shared_ptr<T>(second));}
}; // end struct deep_ptr_eq_functor

/// Find the end of the furthest reaching d-path on diagonal k, for
/// two sequences.  In the paper This is referred to as "the basic
/// algorithm".
///
/// Unlike in the paper, the coordinates of the edit graph start at
/// (-1,-1), rather than (0,0), and they end at (M-1, N-1), rather
/// than (M,N).
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param k the number of the diagonal on which we want to find the
/// end of the furthest reaching D-path.
///
/// @param d the D in D-Path.  That's the number of insertions/deletions
/// (the number of changes, in other words) in the changeset.  That is
/// also the number of non-diagonals in the D-Path.
///
/// @param a_begin an iterator to the beginning of the first sequence
///
/// @param a_end an iterator that points right after the last element
/// of the second sequence to consider.
///
/// @param b_begin an iterator to the beginning of the second sequence.
///
/// @param b_end an iterator that points right after the last element
/// of the second sequence to consider.
///
/// @param v the vector of furthest end points of d_paths, at (d-1).
/// It contains the abscissas of the furthest end points for different
/// values of k, at (d-1).  That is, for k in [-D + 1, -D + 3, -D + 5,
/// ..., D - 1], v[k] is the abscissa of the end of the furthest
/// reaching (D-1)-path on diagonal k.
///
/// @param snak the last snake of the furthest path found.  The end
/// point of the snake is the end point of the furthest path.
///
/// @return true if the end of the furthest reaching path that was
/// found was inside the boundaries of the edit graph, false
/// otherwise.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
bool
end_of_fr_d_path_in_k(int k, int d,
		      RandomAccessOutputIterator a_begin,
		      RandomAccessOutputIterator a_end,
		      RandomAccessOutputIterator b_start,
		      RandomAccessOutputIterator b_end,
		      d_path_vec& v, snake& snak)
{
  int x = -1, y = -1;
  point begin, intermediate, diag_start, end;
  snake s;
  EqualityFunctor eq;

  // Let's pick the end point of the furthest reaching
  // (D-1)-path.  It's either v[k-1] or v[k+1]; the word
  // "furthest" means we choose the one which abscissa is the
  // greatest (that is, furthest from abscissa zero).
  if (k == -d || ((k != d) && (v[k-1] < v[k + 1])))
    // So, the abscissa of the end point of the furthest
    // reaching (D-1)-path is v[k+1].  That is a diagonal that
    // is above the current (k) diagonal, and on the right.
    // To move to the current k diagonal, one has to move
    // "down" from the diagonal k+1.  So the abscissa won't
    // change.  Only the ordinate will.  It will be given by y
    // = x - k (a bit below); as k has changed from k - 1 (it
    // has increased), y is going to be the new y that is
    // 'down' from the previous y in k - 1.
    {
      x = v[k+1];
      begin.set(x, x - (k + 1));
    }
  else
    {
      // So the abscissa of the end point of the furthest
      // (D-1)-path is v[k-1].  That is on the left of the
      // current k diagonal.  To move to the current k diagonal,
      // one has to move "right" from diagonal k - 1.  That is,
      // the y stays constant and x is incremented.
      x = v[k-1];
      begin.set(x, x - (k - 1));
      ++x;
    }

  // Now get the value of y from the equation k = x -y.
  // This is the point where we first touch K, when we move
  // from the end of the furthest reaching (D-1)-path.
  y = x - k;

  intermediate.x(x);
  intermediate.y(y);

  int last_x_index = a_end - a_begin - 1;
  int last_y_index = b_end - b_start - 1;
  // Now, follow the snake (aka, zero or more consecutive
  // diagonals).  Note that we stay on the k diagonal when we
  // do this.
  while ((x < last_x_index) && (y < last_y_index))
    if (eq(a_begin[x + 1], b_start[y + 1]))
      {
	x = x + 1;
	y = y + 1;
	if (!diag_start)
	  diag_start.set(x, y);
      }
    else
      break;

  end.x(x);
  end.y(y);

  // Note the point that we store in v here might be outside the
  // bounds of the edit graph.  But we store it at this step (for a
  // given D) anyway, because out of bound or not, we need this value
  // at this step to be able to compute the value of the point on the
  // "next" diagonal for the next D.
  v[k] = x;

  if (x >= (int) v.a_size()
      || y >= (int) v.b_size()
      || x < -1 || y < -1)
    return false;

  s.set(begin, intermediate, diag_start, end);
  s.set_forward(true);
  snak = s;

  return true;
}

/// Find the end of the furthest reaching reverse d-path on diagonal k
/// + delta.  Delta is abs(M - N), with M being the size of a and N
/// being the size of b.  This is the "basic algorithm", run backward.
/// That is, starting from the point (M,N) of the edit graph.
///
/// Unlike in the paper, the coordinates of the edit graph start at
/// (-1,-1), rather than (0,0), and they end at (M-1, N-1), rather
/// than (M,N).
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param k the number of the diagonal on which we want to find the
/// end of the furthest reaching reverse D-path.  Actually, we want to
/// find the end of the furthest reaching reverse D-path on diagonal (k
/// - delta).
///
/// @param d the D in D-path.  That's the number of insertions/deletions
/// (the number of changes, in other words) in the changeset.  That is
/// also the number of non-diagonals in the D-Path.
///
/// @param a_begin an iterator to the beginning of the first sequence
///
/// @param a_end an iterator that points right after the last element
/// of the second sequence to consider.
///
/// @param b_begin an iterator to the beginning of the second sequence.
///
/// @param b_end an iterator that points right after the last element
/// of the second sequence to consider.
///
/// @param v the vector of furthest end points of d_paths, at (d-1).
/// It contains the abscissae of the furthest end points for different
/// values of k - delta, at (d-1).  That is, for k in [-D + 1, -D + 3,
/// -D + 5, ..., D - 1], v[k - delta] is the abscissa of the end of the
/// furthest reaching (D-1)-path on diagonal k - delta.
///
/// @param snak the last snake of the furthest path found.  The end
/// point of the snake is the end point of the furthest path.
///
/// @return true iff the end of the furthest reaching path that was
/// found was inside the boundaries of the edit graph, false
/// otherwise.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
bool
end_of_frr_d_path_in_k_plus_delta (int k, int d,
				   RandomAccessOutputIterator a_begin,
				   RandomAccessOutputIterator a_end,
				   RandomAccessOutputIterator b_begin,
				   RandomAccessOutputIterator b_end,
				   d_path_vec& v, snake& snak)
{
  int a_size = a_end - a_begin;
  int b_size = b_end - b_begin;
  int delta = a_size - b_size;
  int k_plus_delta = k + delta;
  int x = -1, y = -1;
  point begin, intermediate, diag_start, end;
  snake s;
  EqualityFunctor eq;

  // Let's pick the end point of the furthest reaching (D-1)-path and
  // move from there to reach the current k_plus_delta-line.  That end
  // point of the furthest reaching (D-1)-path is either on
  // v[k_plus_delta-1] or on v[k_plus_delta+1]; the word "furthest"
  // means we choose the one which abscissa is the lowest (that is,
  // furthest from abscissa M).
  if (k_plus_delta == -d + delta
      || ((k_plus_delta != d + delta)
	  && (v[k_plus_delta + 1] <= v[k_plus_delta - 1])))
    {
      // We move left, that means ordinate won't change ...
      x = v[k_plus_delta + 1];
      y = x - (k_plus_delta + 1);
      begin.set(x, y);
      // ... and abscissa decreases.
      x = x - 1;
    }
  else
    {
      // So the furthest end point is on the k_plus_delta - 1
      // diagonal.  That is a diagonal that is 'below' the
      // k_plus_delta current diagonal.  So to join the current
      // diagonal from the k_plus_delta - 1 one, we need to move up.

      // So moving up means abscissa won't change ...
      x = v[k_plus_delta - 1];
      begin.set(x, x - (k_plus_delta - 1));
      // ... and that ordinate decreases.
      y = x - (k_plus_delta - 1) - 1;
    }

  intermediate.set(x, y);

  // Now, follow the snake.  Note that we stay on the k_plus_delta
  // diagonal when we do this.
  while (x >= 0 && y >= 0)
    if (eq(a_begin[x], b_begin[y]))
      {
	if (!diag_start)
	  diag_start.set(x, y);
	x = x - 1;
	y = y - 1;
      }
    else
      break;

  end.set(x, y);

  // Note the point that we store in v here might be outside the
  // bounds of the edit graph.  But we store it at this step (for a
  // given D) anyway, because out of bound or not, we need this value
  // at this step to be able to compute the value of the point on the
  // "next" diagonal for the next D.
  v[k_plus_delta] = x;

  if (x == -1 && y == -1)
    ;
  else if (x < -1 || y < -1)
    return false;

  s.set(begin, intermediate, diag_start, end);
  s.set_forward(false);
  snak = s;

  return true;
}

/// Tests if a given point is a match point in an edit graph.
///
/// @param a_begin the begin iterator of the first input sequence of
/// the edit graph.
///
/// @param a_end the end iterator of the first input sequence of the
/// edit graph.  This points to one element passed the end of the
/// sequence.
///
/// @param b_begin the begin iterator of the second input sequence of
/// the edit graph.
///
/// @param b_end the end iterator of the second input sequence of the
/// edit graph.  This points the one element passed the end of the
/// sequence.
///
/// @param point the point to test for being a match point.
///
/// @return true iff \a point is a match point.
template<typename RandomAccessOutputIterator>
bool
is_match_point(RandomAccessOutputIterator a_begin,
	       RandomAccessOutputIterator a_end,
	       RandomAccessOutputIterator b_begin,
	       RandomAccessOutputIterator b_end,
	       const point& point)
{
  int a_size = a_end - a_begin, b_size = b_end - b_begin;

  if (point.x() < 0
      || point.x () >= a_size
      || point.y() < 0
      || point.y() >= b_size)
    return false;

  return (a_begin[point.x()] == b_begin[point.y()]);
}

/// Returns the middle snake of two sequences A and B, as well as the
/// length of their shortest editing script.
///
///  This uses the "linear space refinement" algorithm presented in
/// section 4b in the paper.  As the paper says, "The idea for doing
/// so is to simultaneously run the basic algorithm in both the
/// forward and reverse directions until furthest reaching forward and
/// reverse paths starting at opposing corners 'overlap'."
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param a_begin an iterator pointing to the begining of sequence A.
///
/// @param a_end an iterator pointing to the end of sequence A.  Note
/// that this points right /after/ the end of vector A.
///
/// @param b_begin an iterator pointing to the begining of sequence B.
///
/// @param b_end an iterator pointing to the end of sequence B.  Note
/// that this points right /after/ the end of vector B
///
/// @param snak out parameter.  This is the snake current when the two
/// paths overlapped.  This is set iff the function returns true;
/// otherwise, this is not touched.
///
/// @return true is the snake was found, false otherwise.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
bool
compute_middle_snake(RandomAccessOutputIterator a_begin,
		     RandomAccessOutputIterator a_end,
		     RandomAccessOutputIterator b_begin,
		     RandomAccessOutputIterator b_end,
		     snake& snak, int& ses_len)
{
  int a_size = a_end - a_begin;
  int N = a_size;
  int b_size = b_end - b_begin;
  int M = b_size;
  int delta = N - M;
  d_path_vec forward_d_paths(a_size, b_size);
  d_path_vec reverse_d_paths(a_size, b_size);
  // These points below are the top leftmost point and bottom
  // right-most points of the edit graph.
  point first_point(-1, -1), last_point(a_size -1, b_size -1), point_zero(0, 0);

  // We want the initial step (D = 0, k = 0 in the paper) to find a
  // furthest reaching point on diagonal k == 0; For that, we need the
  // value of x for k == 1; So let's set that value to -1; that is for
  // k == 1 (diagonal 1), the point in the edit graph is (-1,-2).
  // That way, to get the furthest reaching point on diagonal 0 (k ==
  // 0), we go down from (-1,-2) on diagonal 1 and we hit diagonal 0
  // on (-1,-1); that is the starting value that the algorithm expects
  // for k == 0.
  forward_d_paths[1] = -1;

  // Similarly for the reverse paths, for diagonal delta + 1 (note
  // that diagonals are centered on delta, unlike for forward paths
  // where they are centered on zero), we set the initial point to
  // (a_size, b_size - 1).  That way, at step D == 0 and k == delta,
  // to reach diagonal delta from the point (a_size, b_size - 1) on
  // diagonal delta + 1, we just have to move left, and we hit
  // diagonal delta on (a_size - 1, b_size -1); that is the starting
  // point value the algorithm expects for k == 0 in the reverse case.
  reverse_d_paths[delta + 1] = a_size;

  int d_max = (M + N) / 2 + 1;
  for (int d = 0; d <= d_max; ++d)
    {
      // First build forward paths.
      for (int k = -d; k <= d; k += 2)
	{
	  snake s;
	  bool found =
	    end_of_fr_d_path_in_k<RandomAccessOutputIterator,
				  EqualityFunctor>(k, d,
						   a_begin, a_end,
						   b_begin, b_end,
						   forward_d_paths, s);
	  if (!found)
	    continue;

	  // As the paper says in 4b while explaining the middle snake
	  // algorithm:
	  //
	  // "Thus when delta is odd, check for overlap only while
	  //  extending forward paths ..."
	  if ((delta % 2)
	      && (k >= (delta - (d - 1))) && (k <= (delta + (d - 1))))
	    {
	      point reverse_end;
	      reverse_end.x(reverse_d_paths[k]);
	      reverse_end.y(reverse_end.x() - k);
	      if (ends_of_furthest_d_paths_overlap(s.end(), reverse_end))
		{
		  ses_len = 2 * d - 1;
		  snak = s;
		  return true;
		}
	    }
	}

      // Now build reverse paths.
      for (int k = -d; k <= d; k += 2)
	{
	  snake s;
	  bool found =
	    end_of_frr_d_path_in_k_plus_delta<RandomAccessOutputIterator,
					      EqualityFunctor>(k, d,
							       a_begin, a_end,
							       b_begin, b_end,
							       reverse_d_paths,
							       s);

	  if (!found)
	    continue;

	  // And the paper continues by saying:
	  //
	  // "... and when delta is even, check for overlap only while
	  // extending reverse paths."
	  int k_plus_delta = k + delta;
	  if (!(delta % 2)
	      && (k_plus_delta >= -d) && (k_plus_delta <= d))
	    {
	      point forward_end;
	      forward_end.x(forward_d_paths[k_plus_delta]);
	      forward_end.y(forward_end.x() - k_plus_delta);
	      if (ends_of_furthest_d_paths_overlap(forward_end, s.end()))
		{
		    ses_len = 2 * d;
		    snak = s;
		    return true;
		}
	    }
	}
    }
  return false;
}

bool
compute_middle_snake(const char* str1, const char* str2,
		     snake& s, int& ses_len);

/// This prints the middle snake of two strings.
///
/// @param a_begin the beginning of the first string.
///
/// @param b_begin the beginning of the second string.
///
/// @param s the snake to print.
///
/// @param out the output stream to print the snake to.
template<typename RandomAccessOutputIterator>
void
print_snake(RandomAccessOutputIterator a_begin,
	    RandomAccessOutputIterator b_begin,
	    const snake &s, ostream& out)
{
  if (s.is_empty())
    return;

   out << "snake start: ";
   out << "(" << s.begin().x() << ", " << s.end().y() << ")\n";

   out << "snake intermediate: ";
   out << "(" << s.intermediate().x() << ", " << s.intermediate().y() << ")\n";

   out << "diagonal point(s): ";
   if (s.has_diagonal_edge())
     for (int x = s.intermediate().x(), y = s.intermediate().y();
	  x <= s.end().x() && y <= s.end().y();
	  ++x, ++y)
       {
	 ABG_ASSERT(a_begin[x] == b_begin[y]);
	 out << "(" << x << "," << y << ") ";
       }
   out << "\n";

   out << "snake end: ";
   out << "(" << s.end().x() << ", " << s.end().y() << ")\n";
}

/// Compute the length of the shortest edit script for two sequences a
/// and b.  This is done using the "Greedy LCS/SES" of figure 2 in the
/// paper.  It can walk the edit graph either foward (when reverse is
/// false) or backward starting from the end (when reverse is true).
///
/// Here, note that the real content of a and b should start at index
/// 1, for this implementatikon algorithm to match the paper's
/// algorithm in a straightforward manner.  So pleast make sure that
/// at index 0, we just get some non-used value.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param a the first sequence we care about.
///
/// @param b the second sequence we care about.
///
/// @param v the vector that contains the end points of the furthest
/// reaching d-path and (d-1)-path.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
int
ses_len(RandomAccessOutputIterator a_begin,
	RandomAccessOutputIterator a_end,
	RandomAccessOutputIterator b_begin,
	RandomAccessOutputIterator b_end,
	d_path_vec& v, bool reverse)
{
  unsigned a_size = a_end - a_begin;
  unsigned b_size = b_end - b_begin;
  snake snak;

  ABG_ASSERT(v.max_d() == a_size + b_size);

  int delta = a_size - b_size;

  if (reverse)
    // Set a fictitious (M, N-1) into v[1], to find the furthest
    // reaching reverse 0-path (i.e, when we are at d == 0 and k == 0).
    v[delta + 1] = a_size - 1;
  else
    // Set a fictitious (-1,-2) point into v[1], to find the furthest
    // reaching forward 0-path (i.e, when we are at d == 0 and k == 0).
    v[1] = -1;

  for (unsigned d = 0; d <= v.max_d(); ++d)
    {
      for (int k = -d; k <= (int) d; k += 2)
	{
	  point end;
	  if (reverse)
	    {
	      bool found =
		end_of_frr_d_path_in_k_plus_delta<RandomAccessOutputIterator,
						  EqualityFunctor>(k, d,
								   a_begin, a_end,
								   b_begin, b_end,
								   v, snak);
	      // If we reached the upper left corner of the edit graph then
	      // we are done.
	      if (found && snak.end().x() == -1 && snak.end().y() == -1)
		return d;
	    }
	  else
	    {
	      end_of_fr_d_path_in_k<RandomAccessOutputIterator,
				    EqualityFunctor>(k, d,
						     a_begin, a_end,
						     b_begin, b_end,
						     v, snak);
	      // If we reached the lower right corner of the edit
	      // graph then we are done.
	      if ((snak.end().x() == (int) a_size - 1)
		  && (snak.end().y() == (int) b_size - 1))
		return d;
	    }
	}
    }
  return 0;
}

/// Compute the length of the shortest edit script for two sequences a
/// and b.  This is done using the "Greedy LCS/SES" of figure 2 in the
/// paper.  It can walk the edit graph either foward (when reverse is
/// false) or backward starting from the end (when reverse is true).
///
/// Here, note that the real content of a and b should start at index
/// 1, for this implementatikon algorithm to match the paper's
/// algorithm in a straightforward manner.  So pleast make sure that
/// at index 0, we just get some non-used value.
///
/// Note that the equality operator used to compare the elements
/// passed in argument to this function is the default "==" operator.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @param a the first sequence we care about.
///
/// @param b the second sequence we care about.
///
/// @param v the vector that contains the end points of the furthest
/// reaching d-path and (d-1)-path.
template<typename RandomAccessOutputIterator>
int
ses_len(RandomAccessOutputIterator a_begin,
	RandomAccessOutputIterator a_end,
	RandomAccessOutputIterator b_begin,
	RandomAccessOutputIterator b_end,
	d_path_vec& v, bool reverse)
{
  return ses_len<RandomAccessOutputIterator, default_eq_functor>(a_begin, a_end,
								 b_begin, b_end,
								 v, reverse);
}

int
ses_len(const char* str1,
	const char* str2,
	bool reverse = false);

bool
snake_end_points(const snake& s, point&, point&);

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// A sequence is determined by a base, a beginning offset and an end
/// offset.  The base always points to the container that contains the
/// sequence to consider.  The beginning offset is an iterator that
/// points the beginning of the sub-region of the sequence that we
/// actually want to consider.  The end offset is an iterator that
/// points to the end of the sub-region of the sequence that we
/// actually want to consider.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param a_base the iterator to the base of the first sequence.
///
/// @param a_start an iterator to the beginning of the sub-region
/// of the first sequence to actually consider.
///
/// @param a_end an iterator to the end of the sub-region of the first
/// sequence to consider.
///
///@param b_base an iterator to the base of the second sequence to
///consider.
///
/// @param b_start an iterator to the beginning of the sub-region
/// of the second sequence to actually consider.
///
/// @param b_end an iterator to the end of the sub-region of the
/// second sequence to actually consider.
///
/// @param lcs the resulting lcs.  This is set iff the function
/// returns true.
///
/// @param ses the resulting shortest editing script.
///
/// @param ses_len the length of the ses above.  Normally this can be
/// retrieved from ses.length(), but this parameter is here for sanity
/// check purposes.  The function computes the length of the ses in
/// two redundant ways and ensures that both methods lead to the same
/// result.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
void
compute_diff(RandomAccessOutputIterator a_base,
	     RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_base,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     vector<point>& lcs,
	     edit_script& ses,
	     int& ses_len)
{
  int a_size = a_end - a_begin;
  int b_size = b_end - b_begin;
  unsigned a_offset = a_begin - a_base, b_offset = b_begin - b_base;

  if (a_size == 0 || b_size == 0)
    {
      if (a_size > 0 && b_size == 0)
	// All elements of the first sequences have been deleted.  So add
	// the relevant deletions to the edit script.
	for (RandomAccessOutputIterator i = a_begin; i < a_end; ++i)
	  ses.deletions().push_back(deletion(i - a_base));

      if (b_size > 0 && a_size == 0)
	{
	  // All elements present in the second sequence are part of
	  // an insertion into the first sequence at a_end.  So add
	  // that insertion to the edit script.
	  int a_full_size = a_end - a_base;
	  int insertion_index = a_full_size ? a_full_size - 1 : -1;
	  insertion ins(insertion_index);
	  for (RandomAccessOutputIterator i = b_begin; i < b_end; ++i)
	    ins.inserted_indexes().push_back(i - b_base);

	  ses.insertions().push_back(ins);
	}

      ses_len =  a_size + b_size;
      return;
    }

  int d = 0;
  snake snak;
  vector<point> trace; // the trace of the edit graph.  Read the paper
		       // to understand what a trace is.
  bool has_snake =
    compute_middle_snake<RandomAccessOutputIterator,
			 EqualityFunctor>(a_begin, a_end,
					  b_begin, b_end,
					  snak, d);
  if (has_snake)
    {
      // So middle_{begin,end} are expressed wrt a_begin and b_begin.
      // Let's express them wrt a_base and b_base.
      snak.add(a_offset, b_offset);
      ses_len = d;
    }

  if (has_snake)
    {
      if ( snak.has_diagonal_edge())
	for (int x = snak.diagonal_start().x(), y = snak.diagonal_start().y();
	     x <= snak.end().x() && y <= snak.end().y();
	     ++x, ++y)
	  {
	    point p(x, y);
	    trace.push_back(p);
	  }
    }
  else
    {
      // So there is no middle snake.  That means there is no lcs, so
      // the two sequences are different.

      // In other words, all the elements of the first sequence have
      // been deleted ...
      for (RandomAccessOutputIterator i = a_begin; i < a_end; ++i)
	ses.deletions().push_back(deletion(i - a_base));

      // ... and all the elements of the second sequence are insertions
      // that happen at the beginning of the first sequence.
      insertion ins(a_begin - a_base);
      for (RandomAccessOutputIterator i = b_begin; i < b_end; ++i)
	ins.inserted_indexes().push_back(i - b_base);
      ses.insertions().push_back(ins);

      ses_len = a_size + b_size;
      ABG_ASSERT(ses_len == ses.length());
      return;
    }

  if (d > 1)
    {
      int tmp_ses_len0 = 0;
      edit_script tmp_ses0;
      point px, pu;
      snake_end_points(snak, px, pu);
      compute_diff<RandomAccessOutputIterator,
		   EqualityFunctor>(a_base, a_begin, a_base + (px.x() + 1),
				    b_base, b_begin, b_base + (px.y() + 1),
				    lcs, tmp_ses0, tmp_ses_len0);

      lcs.insert(lcs.end(), trace.begin(), trace.end());

      int tmp_ses_len1 = 0;
      edit_script tmp_ses1;
      compute_diff<RandomAccessOutputIterator,
		   EqualityFunctor>(a_base, a_base + (pu.x() + 1), a_end,
				    b_base, b_base + (pu.y() + 1), b_end,
				    lcs, tmp_ses1, tmp_ses_len1);
      ABG_ASSERT(tmp_ses0.length() + tmp_ses1.length() == d);
      ABG_ASSERT(tmp_ses_len0 + tmp_ses_len1 == d);
      ses.append(tmp_ses0);
      ses.append(tmp_ses1);
    }
  else if (d == 1)
    {
      if (snak.has_diagonal_edge())
	{
	  for (int x = snak.diagonal_start().x(), y = snak.diagonal_start().y();
	       x <= snak.end().x() && y <= snak.end().y();
	       ++x, ++y)
	    {
	      point p(x, y);
	      trace.push_back(p);
	    }
	}

      if (snak.has_vertical_edge())
	{
	  point p = snak.intermediate();
	  insertion ins(p.x());
	  ins.inserted_indexes().push_back(p.y());
	  ses.insertions().push_back(ins);
	}
      else if (snak.has_horizontal_edge())
	{
	  if (snak.is_forward())
	    {
	      deletion del(snak.intermediate().x());
	      ses.deletions().push_back(del);
	    }
	  else
	    {
	      deletion del(snak.begin().x());
	      ses.deletions().push_back(del);
	    }
	}
    }
  else if (d == 0)
    {
      // Obviously on the middle snake is part of the solution, as
      // there is no edit script; iow, the two sequences are
      // identical.
      lcs.insert(lcs.end(), trace.begin(), trace.end());
      ses_len = 0;
    }

  ABG_ASSERT(ses_len == ses.length());
}

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param a_start an iterator to the beginning of the first sequence
/// to consider.
///
/// @param a_end an iterator to the end of the first sequence to
/// consider.
///
/// @param b_start an iterator to the beginning of the second sequence
/// to consider.
///
/// @param b_end an iterator to the end of the second sequence to
/// consider.
///
/// @param lcs the resulting lcs.  This is set iff the function
/// returns true.
///
/// @param ses the resulting shortest editing script.
///
/// @param ses_len the length of the ses above.  Normally this can be
/// retrieved from ses.length(), but this parameter is here for sanity
/// check purposes.  The function computes the length of the ses in
/// two redundant ways and ensures that both methods lead to the same
/// result.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
void
compute_diff(RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     vector<point>& lcs,
	     edit_script& ses,
	     int& ses_len)
{
  compute_diff<RandomAccessOutputIterator,
	       EqualityFunctor>(a_begin, a_begin, a_end,
				b_begin, b_begin, b_end,
				lcs, ses, ses_len);
}

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// A sequence is determined by a base, a beginning offset and an end
/// offset.  The base always points to the container that contains the
/// sequence to consider.  The beginning offset is an iterator that
/// points the beginning of the sub-region of the sequence that we
/// actually want to consider.  The end offset is an iterator that
/// points to the end of the sub-region of the sequence that we
/// actually want to consider.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param a_base the iterator to the base of the first sequence.
///
/// @param a_start an iterator to the beginning of the sub-region
/// of the first sequence to actually consider.
///
/// @param a_end an iterator to the end of the sub-region of the first
/// sequence to consider.
///
///@param b_base an iterator to the base of the second sequence to
///consider.
///
/// @param b_start an iterator to the beginning of the sub-region
/// of the second sequence to actually consider.
///
/// @param b_end an iterator to the end of the sub-region of the
/// second sequence to actually consider.
///
/// @param lcs the resulting lcs.  This is set iff the function
/// returns true.
///
/// @param ses the resulting shortest editing script.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
void
compute_diff(RandomAccessOutputIterator a_base,
	     RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_base,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     vector<point>& lcs,
	     edit_script& ses)
{
  int ses_len = 0;

  compute_diff<RandomAccessOutputIterator,
	       EqualityFunctor>(a_base, a_begin, a_end,
				b_base, b_begin, b_end,
				lcs, ses, ses_len);
}

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param a_start an iterator to the beginning of the first sequence
/// to consider.
///
/// @param a_end an iterator to the end of the first sequence to
/// consider.
///
/// @param b_start an iterator to the beginning of the sequence to
/// actually consider.
///
/// @param b_end an iterator to the end of second sequence to
/// consider.
///
/// @param lcs the resulting lcs.  This is set iff the function
/// returns true.
///
/// @param ses the resulting shortest editing script.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
void
compute_diff(RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     vector<point>& lcs,
	     edit_script& ses)
{
  compute_diff<RandomAccessOutputIterator,
	       EqualityFunctor>(a_begin, a_begin, a_end,
				b_begin, b_begin, b_end,
				lcs, ses);
}

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @param a_start an iterator to the beginning of the first sequence
/// to consider.
///
/// @param a_end an iterator to the end of the first sequence to
/// consider.
///
/// @param b_start an iterator to the beginning of the sequence to
/// actually consider.
///
/// @param b_end an iterator to the end of second sequence to
/// consider.
///
/// @param lcs the resulting lcs.  This is set iff the function
/// returns true.
///
/// @param ses the resulting shortest editing script.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator>
void
compute_diff(RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     vector<point>& lcs,
	     edit_script& ses)
{
  compute_diff<RandomAccessOutputIterator,
	       default_eq_functor>(a_begin, a_end, b_begin, b_end, lcs, ses);
}

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// A sequence is determined by a base, a beginning offset and an end
/// offset.  The base always points to the container that contains the
/// sequence to consider.  The beginning offset is an iterator that
/// points the beginning of the sub-region of the sequence that we
/// actually want to consider.  The end offset is an iterator that
/// points to the end of the sub-region of the sequence that we
/// actually want to consider.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param a_base the iterator to the base of the first sequence.
///
/// @param a_start an iterator to the beginning of the sub-region
/// of the first sequence to actually consider.
///
/// @param a_end an iterator to the end of the sub-region of the first
/// sequence to consider.
///
/// @param b_base an iterator to the base of the second sequence to
/// consider.
///
/// @param b_start an iterator to the beginning of the sub-region
/// of the second sequence to actually consider.
///
/// @param b_end an iterator to the end of the sub-region of the
/// second sequence to actually consider.
///
/// @param ses the resulting shortest editing script.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
void
compute_diff(RandomAccessOutputIterator a_base,
	     RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_base,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     edit_script& ses)
{
  vector<point> lcs;

  compute_diff<RandomAccessOutputIterator,
	       EqualityFunctor>(a_base, a_begin, a_end,
				b_base, b_begin, b_end,
				lcs, ses);
}

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @tparm EqualityFunctor this must be a class that declares a public
/// call operator member returning a boolean and taking two arguments
/// that must be of the same type as the one pointed to by the @ref
/// RandomAccessOutputIterator template parameter. This functor is
/// used to compare the elements referred to by the iterators pased in
/// argument to this function.
///
/// @param a_start an iterator to the beginning of the first sequence
/// to consider.
///
/// @param a_end an iterator to the end of the first sequence to
/// consider.
///
/// @param b_start an iterator to the beginning of the second sequence
/// to consider.
///
/// @param b_end an iterator to the end of the second sequence to
/// consider.
///
/// @param ses the resulting shortest editing script.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator,
	 typename EqualityFunctor>
void
compute_diff(RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     edit_script& ses)
{
  compute_diff<RandomAccessOutputIterator,
	       EqualityFunctor>(a_begin, a_begin, a_end,
				b_begin, b_begin, b_end,
				ses);
}

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @tparm RandomAccessOutputIterator the type of iterators passed to
/// this function.  It must be a random access output iterator kind.
///
/// @param a_start an iterator to the beginning of the first sequence
/// to consider.
///
/// @param a_end an iterator to the end of the first sequence to
/// consider.
///
/// @param b_start an iterator to the beginning of the second sequence
/// to consider.
///
/// @param b_end an iterator to the end of the second sequence to
/// consider.
///
/// @param ses the resulting shortest editing script.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator>
void
compute_diff(RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     edit_script& ses)
{
  compute_diff<RandomAccessOutputIterator, default_eq_functor>(a_begin, a_end,
							       b_begin, b_end,
							       ses);
}

void
compute_lcs(const char* str1, const char* str2, int &ses_len, string& lcs);

void
compute_ses(const char* str1, const char* str2, edit_script& ses);

/// Display an edit script on standard output.
///
/// @param es the edit script to display
///
/// @param str1_base the first string the edit script is about.
///
/// @pram str2_base the second string the edit script is about.
template<typename RandomAccessOutputIterator>
void
display_edit_script(const edit_script& es,
		    const RandomAccessOutputIterator str1_base,
		    const RandomAccessOutputIterator str2_base,
		    ostream& out)
{
  if (es.num_deletions() == 0)
    out << "no deletion:\n";
  else if (es.num_deletions() == 1)
    {
      out << "1 deletion:\n"
	  << "\t happened at index: ";
    }
  else
    {
      out << es.num_deletions() << " deletions:\n"
	   << "\t happened at indexes: ";
    }

  for (vector<deletion>::const_iterator i = es.deletions().begin();
       i != es.deletions().end();
       ++i)
    {
      if (i != es.deletions().begin())
	out << ", ";
      out << i->index() << " (" << str1_base[i->index()] << ")";
    }
  out << "\n\n";

  if (es.num_insertions() == 0)
    out << "no insertion\n";
  else if (es.num_insertions() == 1)
    out << "1 insertion\n";
  else
      out << es.num_insertions() << " insertions:\n";
  for (vector<insertion>::const_iterator i = es.insertions().begin();
       i != es.insertions().end();
       ++i)
    {
      int idx = i->insertion_point_index();
      if (idx < 0)
	out << "\t before index of first sequence: " << idx + 1
	    << " (" << str1_base[idx + 1] << ")\n";
      else
	out << "\t after index of first sequence: " << idx
	    << " (" << str1_base[idx] << ")\n";

      if (!i->inserted_indexes().empty())
	out << "\t\t inserted indexes from second sequence: ";

      for (vector<unsigned>::const_iterator j = i->inserted_indexes().begin();
	   j != i->inserted_indexes().end();
	   ++j)
	{
	  if (j != i->inserted_indexes().begin())
	    out << ", ";
	  out << *j << " (" << str2_base[*j] << ")";
	}
      out << "\n";
    }
  out << "\n\n";
}

}//end namespace diff_utils

}//end namespace abigail
#endif // __ABG_DIFF_UTILS_H__

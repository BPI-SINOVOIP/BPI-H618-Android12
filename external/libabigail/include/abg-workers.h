// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file declares an interface for the worker threads (or thread
/// pool) design pattern.  It aims at performing a set of tasks in
/// parallel, using the multi-threading capabilities of the underlying
/// processor(s).
///

#ifndef __ABG_WORKERS_H__
#define __ABG_WORKERS_H__

#include <memory>
#include <vector>

using std::shared_ptr;

namespace abigail
{

/// The namespace of the worker threads (or thread pool)
/// implementation of libabigail.  This was modelled after the article
/// https://en.wikipedia.org/wiki/Thread_pool.
namespace workers
{

size_t get_number_of_threads();

/// This represents a task to be performed.
///
/// Each instance of this type represents a task that can be performed
/// concurrently to other instance of the same type.
///
/// An instance of @ref task is meant to be performed by a worker
/// (thread).  A set of tasks can be stored in a @ref queue.
class task
{
public:
  virtual void
  perform() = 0;

  virtual ~task(){};
}; // end class task.

typedef shared_ptr<task> task_sptr;

/// This represents a queue of tasks to be performed.
///
/// Tasks are performed by a number of worker threads.
///
/// When a task is inserted into a @ref queue, the task is said to be
/// "scheduled for execution".
///
/// This is because there are worker threads waiting for tasks to be
/// added to the queue.  When a task is added to the queue, a worker
/// thread picks it up, executes it, notifies interested listeners
/// when the @ref task's execution is completed, and waits for another
/// task to be added to the queue.
///
/// Of course, several worker threads can execute tasks concurrently.
class queue
{
public:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  /// A convenience typedef for a vector of @ref task_sptr
  typedef std::vector<task_sptr> tasks_type;

private:
  priv_sptr p_;

public:
  struct task_done_notify;
  queue();
  queue(unsigned number_of_workers);
  queue(unsigned number_of_workers,
	task_done_notify& notifier);
  size_t get_size() const;
  bool schedule_task(const task_sptr&);
  bool schedule_tasks(const tasks_type&);
  void wait_for_workers_to_complete();
  tasks_type& get_completed_tasks() const;
  ~queue();
}; // end class queue

/// This functor is to notify listeners that a given task scheduled
/// for execution has been fully executed.
struct queue::task_done_notify
{
  virtual void
  operator()(const task_sptr& task_done);
};
} // end namespace workers
} // end namespace abigail
#endif // __ABG_WORKERS_H__

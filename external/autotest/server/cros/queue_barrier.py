# Lint as: python2, python3
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from multiprocessing import Queue, queues
from six.moves import range


class QueueBarrierTimeout(Exception):
    """QueueBarrier timeout exception."""


class QueueBarrier(object):
    """This class implements a simple barrier to synchronize processes. The
    barrier relies on the fact that there a single process "main" and |n|
    different "nodes" to make the implementation simpler. Also, given this
    hierarchy, the nodes and the main can exchange a token while passing
    through the barrier.

    The so called "main" shall call main_barrier() while the "node" shall
    call the node_barrier() method.

    If the same group of |n| nodes and the same main are participating in the
    barrier, it is totally safe to reuse the barrier several times with the same
    group of processes.
    """


    def __init__(self, n):
        """Initializes the barrier with |n| node processes and a main.

        @param n: The number of node processes."""
        self.n_ = n
        self.queue_main_ = Queue()
        self.queue_node_ = Queue()


    def main_barrier(self, token=None, timeout=None):
        """Makes the main wait until all the "n" nodes have reached this
        point.

        @param token: A value passed to every node.
        @param timeout: The timeout, in seconds, to wait for the nodes.
                A None value will block forever.

        Returns the list of received tokens from the nodes.
        """
        # Wait for all the nodes.
        result = []
        try:
            for _ in range(self.n_):
                result.append(self.queue_main_.get(timeout=timeout))
        except queues.Empty:
            # Timeout expired
            raise QueueBarrierTimeout()
        # Release all the blocked nodes.
        for _ in range(self.n_):
            self.queue_node_.put(token)
        return result


    def node_barrier(self, token=None, timeout=None):
        """Makes a node wait until all the "n" nodes and the main have
        reached this point.

        @param token: A value passed to the main.
        @param timeout: The timeout, in seconds, to wait for the nodes.
                A None value will block forever.
        """
        self.queue_main_.put(token)
        try:
            return self.queue_node_.get(timeout=timeout)
        except queues.Empty:
            # Timeout expired
            raise QueueBarrierTimeout()

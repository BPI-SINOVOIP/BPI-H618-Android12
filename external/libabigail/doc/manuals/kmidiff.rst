.. _kmidiff_label:

===========
kmidiff
===========

``kmidiff`` compares the binary Kernel Module Interfaces of two Linux
Kernel trees.  The binary KMI is the interface that the Linux Kernel
exposes to its modules.  The trees we are interested in here are the
result of the build of the Linux Kernel source tree.


General approach
=================

And example of how to build your kernel if you want to compare it to
another one using kmidiff is: ::

    git clone -b v4.5 git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git linux/v4.5
    cd linux/v4.5
    make allyesconfig all

Then install the modules into a directory, for instance, the
build/modules sub-directory of the your kernel source tree: ::

    mkdir build/modules
    make modules_install INSTALL_MOD_DIR=build/modules


Then construct a list of interfaces exported by the kernel, that you
want to compare: ::

    cat > kmi-whitelist << EOF
    [kernel_4.5_kmi_whitelist]
     init_task
     schedule
     dev_queue_xmit
     __kmalloc
     printk
    EOF

Suppose you've done something similar for the v4.6 branch of the Linux
kernel, you now have these two directories: ``linux/v4.5`` and ``linux/v4.6``.
Their modules are present under the directories
``linux/v4.5/build/modules`` and ``linux/v4.6/build/modules``.

To Comparing their KMI ``kmidiff`` needs to know where to find the
vmlinux binaries and their associated modules.  Here would be what the
command line looks like: ::

    kmidiff                                     \
      --kmi-whitelist  linux/v4.6/kmi-whitelist \
      --vmlinux1       linux/v4.5/vmlinux       \
      --vmlinux2       linux/v4.6/vmlinux       \
		       linux/v4.5/build/modules \
		       linux/v4.6/build/modules

Invocation
==========

More generally, ``kmidiff`` is invoked under the form: ::

    kmidiff [options] <first-modules-dir> <second-modules-dir>

Environment
===========

By default, ``kmidiff`` compares all the interfaces (exported
functions and variables) between the Kernel and its modules.  In
practice, though, users want to compare a subset of the those
interfaces.

Users can then define a "symbol list" of the interfaces to compare.
Such a list is a just a file in the "INI" format that looks like: ::

    [kernel_version_x86_64_symbol_list]
      function1_name
      function2_name
      global_variable1_name
      ....


Note that the name of the section (the name that is between the two brackets)
of that INI file just has to end with the string "symbol_list" or "whitelist".
So you can define the name you want, for instance
``[kernel_46_x86_64_symbol_list]``.

Then each line of that symbol list file is the name of an exported
function or variable.  Only those interfaces along with the types
reachable from their signatures are going to be compared by
``kmidiff`` recursively.

Note that kmidiff compares the interfaces exported by the ``vmlinux``
binary and by the all of the compiled modules.

Options
=======

  * ``--help | -h``

    Display a short help about the command and exit.


  * ``--version | -v``

    Display the version of the program and exit.

  * ``--verbose``

    Display some verbose messages while executing.

  * ``--debug-info-dir1 | --d1`` <*di-path1*>

    For cases where the debug information for the binaries of the
    first Linux kernel is split out into separate files, tells
    ``kmidiff`` where to find those separate debug information files.

    Note that *di-path* must point to the root directory under which
    the debug information is arranged in a tree-like manner.  Under
    Red Hat based systems, that directory is usually
    ``<root>/usr/lib/debug``.

  * ``--debug-info-dir2 | --d2`` <*di-path2*>

    Like ``--debug-info-dir1``, this options tells ``kmidiff`` where
    to find the split debug information for the binaries of the second
    Linux kernel.

  * ``--vmlinux1 | --l1`` <*path-to-first-vmlinux*>

    Sets the path to the first ``vmlinux`` binary to consider.  This
    has to be the uncompressed vmlinux binary compiled with debug
    info.

  * ``--vmlinux2 | --l2`` <*path-to-first-vmlinux*>

    Sets the path to the second ``vmlinux`` binary to consider.  This
    has to be the uncompressed vmlinux binary compiled with debug
    info.

  * ``--kmi-whitelist | -w`` <*path-to-interface-whitelist*>

    Set the path to the white list of interfaces to compare while
    comparing the Kernel Module Interface of the first kernel against
    the one of the second kernel.

    If this option is not provided, *all* the exported interfaces of
    the two kernels are compared.  That takes a lot of times and is
    not necessarily meaningful because many interface are probably
    meant to see their reachable types change.

    So please, make sure you always use this option unless you really
    know what you  are doing.

  * ``--suppressions | --suppr`` <*path-to-suppressions*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppressions*.  Note that this option can
    appear multiple times on the command line.  In that case, all of
    the provided suppression specification files are taken into
    account.

    Please note that, by default, if this option is not provided, then
    the :ref:`default suppression specification files
    <abidiff_default_supprs_label>` are loaded .


  * ``--impacted-interfaces | -i``

    Tell what interfaces got impacted by each individual ABI change.

  * ``--full-impact | -f``

    Emit a change report that shows the full impact of each change on
    exported interfaces.  This is the default kind of report emitted
    by tools like ``abidiff`` or ``abipkgdiff``.

  * ``--show-bytes``

    Show sizes and offsets in bytes, not bits.  This option is
    activated by default.

  * ``--show-bits``

    Show sizes and offsets in bits, not bytes.  By default, sizes and
    offsets are shown in bytes.

  * ``--show-hex``

    Show sizes and offsets in hexadecimal base.  This option is
    activated by default.

  * ``--show-dec``

    Show sizes and offsets in decimal base.

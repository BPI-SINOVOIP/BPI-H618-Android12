
======
abidw
======

abidw reads a shared library in `ELF`_ format and emits an XML
representation of its ABI to standard output.  The emitted
representation includes all the globally defined functions and
variables, along with a complete representation of their types.  It
also includes a representation of the globally defined ELF symbols of
the file.  The input shared library must contain associated debug
information in `DWARF`_ format.

When given the ``--linux-tree`` option, this program can also handle a
Linux kernel tree.  That is, a directory tree that contains both the
vmlinux binary and Linux kernel modules.  It analyses those Linux
kernel binaries and emits an XML representation of the interface
between the kernel and its module, to standard output.  In this case,
we don't call it an ABI, but a KMI (Kernel Module Interface).  The
emitted KMI includes all the globally defined functions and variables,
along with a complete representation of their types.  The input
binaries must contain associated debug information in `DWARF`_ format.

Invocation
==========

::

  abidw [options] [<path-to-elf-file>]

Options
=======

  * ``--help | -h``

    Display a short help about the command and exit.

  * `--version | -v`

    Display the version of the program and exit.

  * ``--debug-info-dir | -d`` <*dir-path*>

    In cases where the debug info for *path-to-elf-file* is in a
    separate file that is located in a non-standard place, this tells
    ``abidw`` where to look for that debug info file.

    Note that *dir-path* must point to the root directory under which
    the debug information is arranged in a tree-like manner.  Under
    Red Hat based systems, that directory is usually
    ``<root>/usr/lib/debug``.

    This option can be provided several times with different root
    directories.  In that case, ``abidw`` will potentially look into
    all those root directories to find the split debug info for the
    elf file.

    Note that this option is not mandatory for split debug information
    installed by your system's package manager because then
    ``abidw`` knows where to find it.

  * ``--out-file`` <*file-path*>

    This option instructs ``abidw`` to emit the XML representation of
    *path-to-elf-file* into the file *file-path*, rather than emitting
    it to its standard output.

  * ``--noout``

    This option instructs ``abidw`` to not emit the XML representation
    of the ABI.  So it only reads the ELF and debug information,
    builds the internal representation of the ABI and exits.  This
    option is usually useful for debugging purposes.

  * ``--no-corpus-path``

    Do not emit the path attribute for the ABI corpus.

  * ``--suppressions | suppr`` <*path-to-suppression-specifications-file*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppression-specifications-file*.  Note that
    this option can appear multiple times on the command line.  In
    that case, all of the provided suppression specification files are
    taken into account.  ABI artifacts matched by the suppression
    specifications are suppressed from the output of this tool.


  * ``--kmi-whitelist | -kaw`` <*path-to-whitelist*>

    When analyzing a Linux kernel binary, this option points to the
    white list of names of ELF symbols of functions and variables
    which ABI must be written out.  That white list is called a "
    Kernel Module Interface white list".  This is because for the
    Kernel, we don't talk about the ABI; we rather talk about the
    interface between the Kernel and its module. Hence the term
    ``KMI`` rather than ``ABI``

    Any other function or variable which ELF symbol are not present in
    that white list will not be considered by the KMI writing process.

    If this option is not provided -- thus if no white list is
    provided -- then the entire KMI, that is, all publicly defined and
    exported functions and global variables by the Linux Kernel
    binaries is emitted.
    
  * ``--linux-tree | --lt``

    Make ``abidw`` to consider the input path as a path to a directory
    containing the vmlinux binary as several kernel modules binaries.
    In that case, this program emits the representation of the Kernel
    Module Interface (KMI) on the standard output.

    Below is an example of usage of ``abidw`` on a Linux Kernel tree.

    First, checkout a Linux kernel source tree and build it.  Then
    install the kernel modules in a directory somewhere.  Copy the
    vmlinux binary into that directory too.  And then serialize the
    KMI of that kernel to disk, using ``abidw``: ::

       $ git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
       $ cd linux && git checkout v4.5
       $ make allyesconfig all
       $ mkdir build-output
       $ make INSTALL_MOD_PATH=./build-output modules_install 
       $ cp vmlinux build-output/modules/4.5.0
       $ abidw --linux-tree build-output/modules/4.5.0 > build-output/linux-4.5.0.kmi

  * ``--headers-dir | --hd`` <headers-directory-path-1>

    Specifies where to find the public headers of the binary that the
    tool has to consider.  The tool will thus filter out types that
    are not defined in public headers.

    Note that several public header directories can be specified for
    the binary to consider.  In that case the ``--header-dir`` option
    should be present several times on the command line, like in the
    following example: ::

      $ abidw --header-dir /some/path       \
              --header-dir /some/other/path \
              binary > binary.abi

  * ``--header-file | --hf`` <header-file-path>

    Specifies where to find one of the public headers of the abi file
    that the tool has to consider.  The tool will thus filter out
    types that are not defined in public headers.

  * ``--drop-private-types``

    This option is to be used with the ``--headers-dir`` and/or
    ``header-file`` options.  With this option, types that are *NOT*
    defined in the headers are entirely dropped from the internal
    representation build by Libabigail to represent the ABI and will
    not end up in the abi XML file.

  * ``--no-elf-needed``

    Do not include the list of DT_NEEDED dependency names in the
    corpus.

  * ``--drop-undefined-syms``

    With this option functions or variables for which the (exported)
    ELF symbol is undefined are dropped from the internal
    representation build by Libabigail to represent the ABI and will
    not end up in the abi XML file.

  * ``--merge-translation-units``

    With this option translation units for the same language (and
    address size) will be merged together as if the functions,
    variables and types were all defined together.  Note that this
    also drops the compilation paths used.

  * ``--no-linux-kernel-mode``

    Without this option, if abipkgiff detects that the binaries it is
    looking at are Linux Kernel binaries (either vmlinux or modules)
    then it only considers functions and variables which ELF symbols
    are listed in the __ksymtab and __ksymtab_gpl sections.

    With this option, abipkgdiff considers the binary as a non-special
    ELF binary.  It thus considers functions and variables which are
    defined and exported in the ELF sense.

  * ``--check-alternate-debug-info`` <*elf-path*>

    If the debug info for the file *elf-path* contains a reference to
    an `alternate debug info <alt-di-label>`_ file, ``abidw`` checks
    that it can find that alternate debug info file.  In that case, it
    emits a meaningful success message mentioning the full path to the
    alternate debug info file found.  Otherwise, it emits an error
    code.

  * ``--no-show-locs``

   In the emitted ABI representation, do not show file, line or column
   where ABI artifacts are defined.

  * ``--no-parameter-names``

    In the emitted ABI representation, do not show names of function
    parameters, just the types.

  * ``--no-write-default-sizes``

    In the XML ABI representation, do not write the size-in-bits for
    pointer type definitions, reference type definitions, function
    declarations and function types when they are equal to the default
    address size of the translation unit.  Note that libabigail before
    1.8 will not set the default size and will interpret types without
    a size-in-bits attribute as zero sized.

  * ``--type-id-style`` <``sequence``|``hash``>

    This option controls how types are idenfied in the generated XML
    files.  The default ``sequence`` style just numbers (with
    ``type-id-`` as prefix) the types in the order they are
    encountered.  The ``hash`` style uses a (stable, portable) hash of
    libabigail's internal type names and is intended to make the XML
    files easier to diff.

  * ``--check-alternate-debug-info-base-name`` <*elf-path*>


    Like ``--check-alternate-debug-info``, but in the success message,
    only mention the base name of the debug info file; not its full path.

  * ``--load-all-types``

    By default, ``libabigail`` (and thus ``abidw``) only loads types
    that are reachable from functions and variables declarations that
    are publicly defined and exported by the binary.  So only those
    types are present in the output of ``abidw``.  This option however
    makes ``abidw`` load *all* the types defined in the binaries, even
    those that are not reachable from public declarations.

  *  ``--abidiff``

    Load the ABI of the ELF binary given in argument, save it in
    libabigail's XML format in a temporary file; read the ABI from the
    temporary XML file and compare the ABI that has been read back
    against the ABI of the ELF binary given in argument.  The ABIs
    should compare equal.  If they don't, the program emits a
    diagnostic and exits with a non-zero code.

    This is a debugging and sanity check option.

  *  ``--annotate``

    Annotate the ABIXML output with comments above most elements.  The
    comments are made of the pretty-printed form types, declaration or
    even ELF symbols.  The purpose is to make the ABIXML output more
    human-readable for debugging or documenting purposes.

  * ``--stats``

    Emit statistics about various internal things.

  * ``--verbose``

    Emit verbose logs about the progress of miscellaneous internal
    things.

Notes
=====

.. _alt-di-label:

Alternate debug info files
--------------------------

As of the version 4 of the DWARF specification, `Alternate debug
information <http://www.dwarfstd.org/ShowIssue.php?issue=120604.1>`_
is a `GNU`_ extension to the DWARF specification.  It has however been
proposed for inclusion into the upcoming version 5 of the DWARF
standard.  You can read more about the GNU extensions to the DWARF
standard `here
<https://fedorahosted.org/elfutils/wiki/DwarfExtensions>`_.

.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _DWARF: http://www.dwarfstd.org
.. _GNU: http://www.gnu.org


=======
abilint
=======

abilint parses the native XML representation of an ABI as emitted by
:doc:`abidw`.  Once it has parsed the XML representation of the ABI,
``abilint`` builds and in-memory model from it.  It then tries to save
it back to an XML form, to standard output.  If that read-write
operation succeeds chances are the input XML ABI representation is
meaningful.

Note that the main intent of this tool to help debugging issues in the
underlying Libabigail library.

Note also that ``abilint`` can also read an `ELF`_ input file, build the
in-memory model for its ABI, and serialize that model back into XML to
standard output.  In that case, the `ELF`_ input file must be
accompanied with its debug information in the `DWARF`_ format.

Invocation
==========

::

  abilint [options] [<abi-file1>]

Options
=======

  * ``--help``

    Display a short help message and exits.

  * `--version | -v`

    Display the version of the program and exit.

  * ``--debug-info-dir`` <*path*>

    When reading an `ELF`_ input file which debug information is split
    out into a separate file, this options tells ``abilint`` where to
    find that separate debug information file.

    Note that *path* must point to the root directory under which the
    debug information is arranged in a tree-like manner.  Under Red
    Hat based systems, that directory is usually
    ``<root>/usr/lib/debug``.

    Note also that this option is not mandatory for split debug
    information installed by your system's package manager because
    then ``abidiff`` knows where to find it.

  * ``--diff``

    For XML inputs, perform a text diff between the input and the
    memory model saved back to disk.  This can help to spot issues in
    the handling of the XML format by the underlying Libabigail library.

  * ``--noout``

    Do not display anything on standard output.  The return code of
    the command is the only way to know if the command succeeded.

  * ``--suppressions | suppr`` <*path-to-suppression-specifications-file*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppression-specifications-file*.  Note that
    this option can appear multiple times on the command line.  In
    that case, all of the provided suppression specification files are
    taken into account.  ABI artifacts matched by the suppression
    specifications are suppressed from the output of this tool.

  * ``--headers-dir | --hd`` <headers-directory-path-1>

    Specifies where to find the public headers of the first shared
    library that the tool has to consider.  The tool will thus filter
    out types that are not defined in public headers.

  * ``--header-file | --hf`` <header-file-path>

    Specifies where to find one of the public headers of the abi file
    that the tool has to consider.  The tool will thus filter out
    types that are not defined in public headers.

  * ``--stdin | --``

    Read the input content from standard input.

  * ``--tu``

    Expect the input XML to represent a single translation unit.

.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _DWARF: http://www.dwarfstd.org

####################
Overview of the Abigail framework
####################

**ABIGAIL** stands for the Application Binary Interface Generic
Analysis and Instrumentation Library.

It's a framework which aims at helping developers and software
distributors to spot some ABI-related issues like interface
incompatibility in `ELF`_ shared libraries by performing a static
analysis of the `ELF`_ binaries at hand.

The type of interface incompatibilities that **Abigail** focuses on is
related to changes on the exported ELF functions and variables
symbols, as well as layout and size changes of data types of the
functions and variables exported by shared libraries.

In other words, if the return type of a function exported by a shared
library changes in an incompatible way from one version of a given
shared library to another, we want **Abigail** to help people catch
that.

In more concrete terms, the Abigail framwork provides a shared library
named **libabigail** which exposes an API to parse a shared library in
`ELF`_ format (accompanied with its associated debug information in
`DWARF`_ format) build an internal representation of all the functions
and variables it exports, along with their types.  **Libabigail** also
builds an internal representation of the `ELF symbols`_ of these
functions and variables.  That information about these exported
functions and variables is roughly what we consider as being the ABI
of the shared library, at least, in the scope of **Libabigail**.

Aside of this internal representation, **libabigail** provides
facilities to perform deep comparisons of two ABIs.  That is, it can
compare the types of two sets of functions or variables and represents
the result in a way that allows it to emit textual reports about the
differences.

This allows us to write tools like :doc:`abidiff` that can compare the
ABI of two shared libraries and represent the result in a meaningful
enough way to help us spot ABI incompatibilities.  There are several
:doc:`other tools <libabigail-tools>` that are built using the
``Abigail`` framwork.

.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _DWARF: http://www.dwarfstd.org
.. _ELF symbols: https://blogs.oracle.com/ali/entry/inside_elf_symbol_tables

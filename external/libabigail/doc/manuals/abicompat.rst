.. _abicompat_label:

=========
abicompat
=========

abicompat checks that an application that links against a given shared
library is still ABI compatible with a subsequent version of that
library.  If the new version of the library introduces an ABI
incompatibility, then abicompat hints the user at what exactly that
incompatibility is.

.. _abicompat_invocation_label:

Invocation
==========

::

  abicompat [options] [<application> <shared-library-first-version> <shared-library-second-version>]

.. _abicompat_options_label:

Options
=======

  * ``--help``

    Display a short help about the command and exit.

  * `--version | -v`

    Display the version of the program and exit.

  * ``--list-undefined-symbols | -u``

    Display the list of undefined symbols of the application and exit.

  * ``--show-base-names | -b``

    In the resulting report emitted by the tool, this option makes the
    application and libraries be referred to by their base names only;
    not by a full absolute name.  This can be useful for use in
    scripts that wants to compare names of the application and
    libraries independently of what their directory names are.

  * ``--app-debug-info-dir | --appd`` <path-to-app-debug-info-directory>

    Set the path to the directory under which the debug information of
    the application is supposed to be laid out.  This is useful for
    application binaries for which the debug info is in a separate set
    of files.

  * ``--lib-debug-info-dir1 | --libd1`` <path-to-lib1-debug-info>

    Set the path to the directory under which the debug information of
    the first version of the shared library is supposed to be laid
    out.  This is useful for shared library binaries for which the
    debug info is in a separate set of files.

  * ``--lib-debug-info-dir2 | --libd2`` <path-to-lib1-debug-info>

    Set the path to the directory under which the debug information of
    the second version of the shared library is supposed to be laid
    out.  This is useful for shared library binaries for which the
    debug info is in a separate set of files.

  * ``--suppressions | --suppr`` <*path-to-suppressions*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppressions*.  Note that this option can
    appear multiple times on the command line; all the suppression
    specification files are then taken into account.

  * ``--no-show-locs``

   Do not show information about where in the *second shared library*
   the respective type was changed.

  * ``--weak-mode``

    This triggers the weak mode of ``abicompat``.  In this mode, only
    one version of the library is required.  That is, abicompat is
    invoked like this: ::

        abicompat --weak-mode <the-application> <the-library>

    Note that the ``--weak-mode`` option can even be omitted if only
    one version of the library is given, along with the application;
    in that case, ``abicompat`` automatically switches to operate in
    weak mode: ::

       abicompat <the-application> <the-library>

    In this weak mode, the types of functions and variables exported
    by the library and consumed by the application (as in, the symbols
    of the these functions and variables are undefined in the
    application and are defined and exported by the library) are
    compared to the version of these types as expected by the
    application.  And if these two versions of types are different,
    ``abicompat`` tells the user what the differences are.

    In other words, in this mode, ``abicompat`` checks that the types
    of the functions and variables exported by the library mean the
    same thing as what the application expects, as far as the ABI is
    concerned.

    Note that in this mode, `abicompat` doesn't detect exported
    functions or variables (symbols) that are expected by the
    application but that are removed from the library.  That is why it
    is called ``weak`` mode.

.. _abicompat_return_value_label:

Return values
=============

The exit code of the ``abicompat`` command is either 0 if the ABI of
the binaries being compared are equal, or non-zero if they differ or
if the tool encountered an error.

In the later case, the exit code is a 8-bits-wide bit field in which
each bit has a specific meaning.

The first bit, of value 1, named ``ABIDIFF_ERROR`` means there was an
error.

The second bit, of value 2, named ``ABIDIFF_USAGE_ERROR`` means there
was an error in the way the user invoked the tool.  It might be set,
for instance, if the user invoked the tool with an unknown command
line switch, with a wrong number or argument, etc.  If this bit is
set, then the ``ABIDIFF_ERROR`` bit must be set as well.

The third bit, of value 4, named ``ABIDIFF_ABI_CHANGE`` means the ABI
of the binaries being compared are different.  

The fourth bit, of value 8, named ``ABIDIFF_ABI_INCOMPATIBLE_CHANGE``
means the ABI of the binaries compared are different in an
incompatible way.  If this bit is set, then the ``ABIDIFF_ABI_CHANGE``
bit must be set as well.  If the ``ABIDIFF_ABI_CHANGE`` is set and the
``ABIDIFF_INCOMPATIBLE_CHANGE`` is *NOT* set, then it means that the
ABIs being compared might or might not be compatible.  In that case, a
human being needs to review the ABI changes to decide if they are
compatible or not.

The remaining bits are not used for the moment.

.. _abicompat_usage_example_label:

Usage examples
==============

  * Detecting a possible ABI incompatibility in a new shared library
    version: ::

	$ cat -n test0.h
	     1	struct foo
	     2	{
	     3	  int m0;
	     4
	     5	  foo()
	     6	    : m0()
	     7	  {}
	     8	};
	     9
	    10	foo*
	    11	first_func();
	    12
	    13	void
	    14	second_func(foo&);
	    15
	    16	void
	    17	third_func();
	$

	$ cat -n test-app.cc
	     1	// Compile with:
	     2	//  g++ -g -Wall -o test-app -L. -ltest-0 test-app.cc
	     3
	     4	#include "test0.h"
	     5
	     6	int
	     7	main()
	     8	{
	     9	  foo* f = first_func();
	    10	  second_func(*f);
	    11	  return 0;
	    12	}
	$

	$ cat -n test0.cc
	     1	// Compile this with:
	     2	//  g++ -g -Wall -shared -o libtest-0.so test0.cc
	     3
	     4	#include "test0.h"
	     5
	     6	foo*
	     7	first_func()
	     8	{
	     9	  foo* f = new foo();
	    10	  return f;
	    11	}
	    12
	    13	void
	    14	second_func(foo&)
	    15	{
	    16	}
	    17
	    18	void
	    19	third_func()
	    20	{
	    21	}
	$

	$ cat -n test1.h
	     1	struct foo
	     2	{
	     3	  int  m0;
	     4	  char m1; /* <-- a new member got added here! */
	     5
	     6	  foo()
	     7	  : m0(),
	     8	    m1()
	     9	  {}
	    10	};
	    11
	    12	foo*
	    13	first_func();
	    14
	    15	void
	    16	second_func(foo&);
	    17
	    18	void
	    19	third_func();
	$

	$ cat -n test1.cc
	     1	// Compile this with:
	     2	//  g++ -g -Wall -shared -o libtest-1.so test1.cc
	     3
	     4	#include "test1.h"
	     5
	     6	foo*
	     7	first_func()
	     8	{
	     9	  foo* f = new foo();
	    10	  return f;
	    11	}
	    12
	    13	void
	    14	second_func(foo&)
	    15	{
	    16	}
	    17
	    18	/* Let's comment out the definition of third_func()
	    19	   void
	    20	   third_func()
	    21	   {
	    22	   }
	    23	*/
	$


    * Compile the first and second versions of the libraries:
      ``libtest-0.so`` and ``libtest-1.so``: ::

	$ g++ -g -Wall -shared -o libtest-0.so test0.cc
	$ g++ -g -Wall -shared -o libtest-1.so test1.cc

    * Compile the application and link it against the first version of
      the library, creating the ``test-app`` binary: ::

	$ g++ -g -Wall -o test-app -L. -ltest-0.so test-app.cc

    * Now, use ``abicompat`` to see if libtest-1.so is ABI compatible
      with app, with respect to the ABI of libtest-0.so: ::

	$ abicompat test-app libtest-0.so libtest-1.so
	ELF file 'test-app' might not be ABI compatible with 'libtest-1.so' due to differences with 'libtest-0.so' below:
	Functions changes summary: 0 Removed, 2 Changed, 0 Added functions
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	2 functions with some indirect sub-type change:

	  [C]'function foo* first_func()' has some indirect sub-type changes:
	    return type changed:
	      in pointed to type 'struct foo':
		size changed from 32 to 64 bits
		1 data member insertion:
		  'char foo::m1', at offset 32 (in bits)
	  [C]'function void second_func(foo&)' has some indirect sub-type changes:
	    parameter 0 of type 'foo&' has sub-type changes:
	      referenced type 'struct foo' changed, as reported earlier

	$


    * Now use the weak mode of abicompat, that is, providing just the
      application and the new version of the library:  ::

	$ abicompat --weak-mode test-app libtest-1.so
	functions defined in library
	    'libtest-1.so'
	have sub-types that are different from what application
	    'test-app'
	expects:

	  function foo* first_func():
	    return type changed:
	      in pointed to type 'struct foo':
		size changed from 32 to 64 bits
		1 data member insertion:
		  'char foo::m1', at offset 32 (in bits)

	$

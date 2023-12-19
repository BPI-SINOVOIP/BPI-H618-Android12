.. _abipkgdiff_label:

===========
abipkgdiff
===========

``abipkgdiff`` compares the Application Binary Interfaces (ABI) of the
`ELF`_ binaries contained in two software packages.  The software
package formats currently supported are `Deb`_, `RPM`_, `tar`_
archives (either compressed or not) and plain directories that contain
binaries.

For a comprehensive ABI change report that includes changes about
function and variable sub-types, the two input packages must be
accompanied with their debug information packages that contain debug
information in `DWARF`_ format.


.. _abipkgdiff_invocation_label:

Invocation
==========

::

  abipkgdiff [option] <package1> <package2>

``package1`` and ``package2`` are the packages that contain the
binaries to be compared.


Environment
===========

.. _abipkgdiff_default_supprs_label:

abipkgdiff loads two default :ref:`suppression specifications files
<suppr_spec_label>`, merges their content and use it to filter out ABI
change reports that might be considered as false positives to users.

* Default system-wide suppression specification file

  It's located by the optional environment variable
  LIBABIGAIL_DEFAULT_SYSTEM_SUPPRESSION_FILE.  If that environment
  variable is not set, then abipkgdiff tries to load the suppression file
  $libdir/libabigail/libabigail-default.abignore.  If that file is not
  present, then no default system-wide suppression specification file
  is loaded.

* Default user suppression specification file.

  It's located by the optional environment
  LIBABIGAIL_DEFAULT_USER_SUPPRESSION_FILE.  If that environment
  variable is not set, then abipkgdiff tries to load the suppression file
  $HOME/.abignore.  If that file is not present, then no default user
  suppression specification is loaded.

In addition to those default suppression specification files,
abipkgdiff will also look inside the packages being compared and if it
sees a file that ends with the extension ``.abignore``, then it will
consider it as a suppression specification and it will combine it to the
default suppression specification that might be already loaded.

The user might as well use the ``--suppressions`` option (that is
documented further below) to provide a suppression specification.

.. _abipkgdiff_options_label:

Options
=======

  * ``--help | -h``

    Display a short help about the command and exit.

  * `--version | -v`

    Display the version of the program and exit.

  * ``--debug-info-pkg1 | --d1`` <path>

    For cases where the debug information for *package1* is split out
    into a separate file, tells ``abipkgdiff`` where to find that
    separate debug information package.

    Note that the debug info for *package1* can have been split into
    several different debug info packages.  In that case, several
    instances of this options can be provided, along with those
    several different debug info packages.

  * ``--debug-info-pkg2 | --d2`` <path>

    For cases where the debug information for *package2* is split out
    into a separate file, tells ``abipkgdiff`` where to find that
    separate debug information package.

    Note that the debug info for *package2* can have been split into
    several different debug info packages.  In that case, several
    instances of this options can be provided, along with those
    several different debug info packages.

  * ``--devel-pkg1 | --devel1`` <path>

    Specifies where to find the `Development Package`_ associated with
    the first package to be compared.  That `Development Package`_ at
    ``path`` should at least contain header files in which public
    types exposed by the libraries (of the first package to be
    compared) are defined.  When this option is provided, the tool
    filters out reports about ABI changes to types that are *NOT*
    defined in these header files.

  * ``--devel-pkg2 | --devel2`` <path>

    Specifies where to find the `Development Package`_ associated with
    the second package to be compared.  That `Development Package`_ at
    ``path`` should at least contains header files in which public
    types exposed by the libraries (of the second package to be
    compared) are defined.  When this option is provided, the tool
    filters out reports about ABI changes to types that are *NOT*
    defined in these header files.

  * ``--drop-private-types``

    This option is to be used with the ``--devel-pkg1`` and
    ``--devel-pkg2`` options.  With this option, types that are *NOT*
    defined in the headers are entirely dropped from the internal
    representation build by Libabigail to represent the ABI.  They
    thus don't have to be filtered out from the final ABI change
    report because they are not even present in Libabigail's
    representation.

    Without this option however, those private types are kept in the
    internal representation and later filtered out from the report.

    This options thus potentially makes Libabigail consume less
    memory.  It's meant to be mainly used to optimize the memory
    consumption of the tool on binaries with a lot of publicly defined
    and exported types.

  * ``--dso-only``

    Compare ELF files that are shared libraries, only.  Do not compare
    executable files, for instance.

  * ``--private-dso``

    By default, ``abipkgdiff`` does not compare DSOs that are private
    to the RPM package.  A private DSO is a DSO which SONAME is *NOT*
    advertised in the "provides" property of the RPM.

    This option instructs ``abipkgdiff`` to *also* compare DSOs that
    are *NOT* advertised in the "provides" property of the RPM.

    Please note that the fact that (by default) ``abipkgdiff`` skips
    private DSO is a feature that is available only for RPMs, at the
    moment.  We would happily accept patches adding that feature for
    other package formats.

  * ``--leaf-changes-only|-l`` only show leaf changes, so don't show
    impact analysis report.  This option implies ``--redundant``

    The typical output of ``abipkgdiff`` and ``abidiff`` when
    comparing two binaries, that we shall call ``full impact report``,
    looks like this ::

	$ abidiff libtest-v0.so libtest-v1.so
	Functions changes summary: 0 Removed, 1 Changed, 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	1 function with some indirect sub-type change:

	  [C]'function void fn(C&)' at test-v1.cc:13:1 has some indirect sub-type changes:
	    parameter 1 of type 'C&' has sub-type changes:
	      in referenced type 'struct C' at test-v1.cc:7:1:
		type size hasn't changed
		1 data member change:
		 type of 'leaf* C::m0' changed:
		   in pointed to type 'struct leaf' at test-v1.cc:1:1:
		     type size changed from 32 to 64 bits
		     1 data member insertion:
		       'char leaf::m1', at offset 32 (in bits) at test-v1.cc:4:1

	$

    So in that example the report emits information about how the data
    member insertion change of "struct leaf" is reachable from
    function "void fn(C&)".  In other words, the report not only shows
    the data member change on "struct leaf", but it also shows the
    impact of that change on the function "void fn(C&)".

    In abidiff (and abipkgdiff) parlance, the change on "struct leaf"
    is called a leaf change.  So the ``--leaf-changes-only
    --impacted-interfaces`` options show, well, only the leaf change.
    And it goes like this: ::

	$ abidiff -l libtest-v0.so libtest-v1.so
	'struct leaf' changed:
	  type size changed from 32 to 64 bits
	  1 data member insertion:
	    'char leaf::m1', at offset 32 (in bits) at test-v1.cc:4:1

	  one impacted interface:
	    function void fn(C&)
	$

    Note how the report ends up by showing the list of interfaces
    impacted by the leaf change.  That's the effect of the additional
    ``--impacted-interfaces`` option.

    Now if you don't want to see that list of impacted interfaces,
    then you can just avoid using the ``--impacted-interface`` option.
    You can learn about that option below, in any case.

    Please note that when comparing two Linux Kernel packages, it's
    this ``leaf changes report`` that is emitted, by default.  The
    normal so-called ``full impact report`` can be emitted with the
    option ``--full-impact`` which is documented later below.


  * ``--impacted-interfaces``

    When showing leaf changes, this option instructs abipkgdiff to
    show the list of impacted interfaces.  This option is thus to be
    used in addition to the ``--leaf-changes-only`` option, or, when
    comparing two Linux Kernel packages.  Otherwise, it's simply
    ignored.

  * ``--full-impact|-f``

    When comparing two Linux Kernel packages, this function instructs
    ``abipkgdiff`` to emit the so-called ``full impact report``, which
    is the default report kind emitted by the ``abidiff`` tool: ::

	$ abidiff libtest-v0.so libtest-v1.so
	Functions changes summary: 0 Removed, 1 Changed, 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	1 function with some indirect sub-type change:

	  [C]'function void fn(C&)' at test-v1.cc:13:1 has some indirect sub-type changes:
	    parameter 1 of type 'C&' has sub-type changes:
	      in referenced type 'struct C' at test-v1.cc:7:1:
		type size hasn't changed
		1 data member change:
		 type of 'leaf* C::m0' changed:
		   in pointed to type 'struct leaf' at test-v1.cc:1:1:
		     type size changed from 32 to 64 bits
		     1 data member insertion:
		       'char leaf::m1', at offset 32 (in bits) at test-v1.cc:4:1

	$


  * ``--non-reachable-types|-t``

    Analyze and emit change reports for all the types of the binary,
    including those that are not reachable from global functions and
    variables.

    This option might incur some serious performance degradation as
    the number of types analyzed can be huge.  However, if paired with
    the ``--devel-pkg{1,2}`` options, the additional non-reachable
    types analyzed are restricted to those defined in the public
    headers files carried by the referenced development packages, thus
    hopefully making the performance hit acceptable.

    Also, using this option alongside suppression specifications (by
    also using the ``--suppressions`` option) might help keep the number of
    analyzed types (and the potential performance degradation) in
    control.

    Note that without this option, only types that are reachable from
    global functions and variables are analyzed, so the tool detects
    and reports changes on these reachable types only.

  *  ``--redundant``

    In the diff reports, do display redundant changes.  A redundant
    change is a change that has been displayed elsewhere in a given
    report.

  * ``--harmless``

    In the diff report, display only the :ref:`harmless
    <harmlesschangeconcept_label>` changes.  By default, the harmless
    changes are filtered out of the diff report keep the clutter to a
    minimum and have a greater chance to spot real ABI issues.

  * ``--no-linkage-name``

    In the resulting report, do not display the linkage names of
    the added, removed, or changed functions or variables.

  * ``--no-added-syms``

    Do not show the list of functions, variables, or any symbol that
    was added.

  * ``--no-added-binaries``

    Do not show the list of binaries that got added to the second
    package.

    Please note that the presence of such added binaries is not
    considered like an ABI change by this tool; as such, it doesn't
    have any impact on the exit code of the tool.  It does only have
    an informational value.  Removed binaries are, however, considered
    as an ABI change.

  * ``--no-abignore``

    Do not search the package for the presence of suppression files.

  * ``--no-parallel``

    By default, ``abipkgdiff`` will use all the processors it has available to
    execute concurrently.  This option tells it not to extract packages or run
    comparisons in parallel.

  * ``--no-default-suppression``

    Do not load the :ref:`default suppression specification files
    <abipkgdiff_default_supprs_label>`.

  * ``--suppressions | --suppr`` <*path-to-suppressions*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppressions*.  Note that this option can
    appear multiple times on the command line.  In that case, all of
    the suppression specification files are taken into account.

    Please note that, by default, if this option is not provided, then
    the :ref:`default suppression specification files
    <abipkgdiff_default_supprs_label>` are loaded .

  * ``--linux-kernel-abi-whitelist | -w`` <*path-to-whitelist*>

    When comparing two Linux kernel RPM packages, this option points
    to the white list of names of ELF symbols of functions and
    variables that must be compared for ABI changes.  That white list
    is called a "Linux kernel ABI white list".

    Any other function or variable which ELF symbol are not present in
    that white list will not be considered by the ABI comparison
    process.

    If this option is not provided -- thus if no white list is
    provided -- then the ABI of all publicly defined and exported
    functions and global variables by the Linux Kernel binaries are
    compared.

    Please note that if a white list package is given in parameter,
    this option handles it just fine, like if the --wp option was
    used.

  * ``--wp`` <*path-to-whitelist-package*>

    When comparing two Linux kernel RPM packages, this option points
    an RPM package containining several white lists of names of ELF
    symbols of functions and variables that must be compared for ABI
    changes.  Those white lists are called "Linux kernel ABI white
    lists".

    From the content of that white list package, this program then
    chooses the appropriate Linux kernel ABI white list to consider
    when comparing the ABI of Linux kernel binaries contained in the
    Linux kernel packages provided on the command line.

    That choosen Linux kernel ABI white list contains the list of
    names of ELF symbols of functions and variables that must be
    compared for ABI changes.

    Any other function or variable which ELF symbol are not present in
    that white list will not be considered by the ABI comparison
    process.

    Note that this option can be provided twice (not mor than twice),
    specifying one white list package for each Linux Kernel package
    that is provided on the command line.

    If this option is not provided -- thus if no white list is
    provided -- then the ABI of all publicly defined and exported
    functions and global variables by the Linux Kernel binaries are
    compared.

  * ``--no-unreferenced-symbols``

    In the resulting report, do not display change information about
    function and variable symbols that are not referenced by any debug
    information.  Note that for these symbols not referenced by any
    debug information, the change information displayed is either
    added or removed symbols.
    
  * ``--no-show-locs``

   Do not show information about where in the *second shared library*
   the respective type was changed.

  * ``--show-bytes``

    Show sizes and offsets in bytes, not bits.  By default, sizes and
    offsets are shown in bits.

  * ``--show-bits``

    Show sizes and offsets in bits, not bytes.  This option is
    activated by default.

  * ``--show-hex``

    Show sizes and offsets in hexadecimal base.

  * ``--show-dec``

    Show sizes and offsets in decimal base.  This option is activated
    by default.

  *  ``--no-show-relative-offset-changes``

     Without this option, when the offset of a data member changes,
     the change report not only mentions the older and newer offset,
     but it also mentions by how many bits the data member changes.
     With this option, the latter is not shown.

  * ``--show-identical-binaries``

   Show the names of the all binaries compared, including the
   binaries whose ABI compare equal.  By default, when this option is
   not provided, only binaries with ABI changes are mentionned in the
   output.

  * ``--fail-no-dbg``

    Make the program fail and return a non-zero exit code if couldn't
    read any of the debug information that comes from the debug info
    packages that were given on the command line.  If no debug info
    package were provided on the command line then this option is not
    active.

    Note that the non-zero exit code returned by the program as a
    result of this option is the constant ``ABIDIFF_ERROR``.  To know
    the numerical value of that constant, please refer to the
    :ref:`exit code documentation <abidiff_return_value_label>`.

  * ``--keep-tmp-files``

    Do not erase the temporary directory files that are created during
    the execution of the tool.

  * ``--verbose``

    Emit verbose progress messages.


  * ``self-check``

    This is used to test the underlying Libabigail library.  When in
    used, the command expects only on input package, along with its
    associated debug info packages.  The command then compares each
    binary inside the package against its own ABIXML
    representation. The result of the comparison should yield the
    empty set if Libabigail behaves correctly.  Otherwise, it means
    there is an issue that ought to be fixed.  This option is used by
    people interested in Libabigail development for regression testing
    purposes.  Here is an example of the use of this option: ::

      $ abipkgdiff --self-check --d1 mesa-libGLU-debuginfo-9.0.1-3.fc33.x86_64.rpm  mesa-libGLU-9.0.1-3.fc33.x86_64.rpm
       ==== SELF CHECK SUCCEEDED for 'libGLU.so.1.3.1' ====
      $

.. _abipkgdiff_return_value_label:

Return value
============

The exit code of the ``abipkgdiff`` command is either 0 if the ABI of
the binaries compared are equal, or non-zero if they differ or if the
tool encountered an error.

In the later case, the value of the exit code is the same as for the
:ref:`abidiff tool <abidiff_return_value_label>`.


.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _RPM: https://en.wikipedia.org/wiki/RPM_Package_Manager
.. _Deb: https://en.wikipedia.org/wiki/Deb_%28file_format%29
.. _tar: https://en.wikipedia.org/wiki/Tar_%28computing%29
.. _DWARF: http://www.dwarfstd.org
.. _Development Package: https://fedoraproject.org/wiki/Packaging:Guidelines?rd=Packaging/Guidelines#Devel_Packages

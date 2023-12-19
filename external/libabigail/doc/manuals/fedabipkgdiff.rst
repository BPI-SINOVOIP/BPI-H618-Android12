.. _fedabipkgdiff_label:

==============
fedabipkgdiff
==============

``fedabipkgdiff`` compares the ABI of shared libraries in `Fedora`_
packages.  It's a convenient way to do so without having to manually
download packages from the `Fedora Build System`_.

``fedabipkgdiff`` knows how to talk with the `Fedora Build System`_ to
find the right packages versions, their associated debug information
and development packages, download them, compare their ABI locally,
and report about the possible ABI changes.

Note that by default, this tool reports ABI changes about types that
are defined in public header files found in the development packages
associated with the packages being compared.  It also reports ABI
changes about functions and global variables whose symbols are defined
and exported in the ELF binaries found in the packages being compared.

.. _fedabipkgdiff_invocation_label:

Invocation
==========

::

   fedabipkgdiff [option] <NVR> ...

Environment
===========

.. _fedabipkgdiff_default_supprs_label:

fedabipkgdiff loads two default :ref:`suppression specifications files
<suppr_spec_label>`, merges their content and use it to filter out ABI
change reports that might be considered as false positives to users.

* Default system-wide suppression specification file

  It's located by the optional environment variable
  LIBABIGAIL_DEFAULT_SYSTEM_SUPPRESSION_FILE.  If that environment
  variable is not set, then fedabipkgdiff tries to load the
  suppression file $libdir/libabigail/libabigail-default.abignore.  If
  that file is not present, then no default system-wide suppression
  specification file is loaded.

* Default user suppression specification file.

  It's located by the optional environment
  LIBABIGAIL_DEFAULT_USER_SUPPRESSION_FILE.  If that environment
  variable is not set, then fedabipkgdiff tries to load the
  suppression file $HOME/.abignore.  If that file is not present, then
  no default user suppression specification is loaded.


.. _fedabipkgdiff_options_label:

Options
=======

  * ``--help | -h``

    Display a short help about the command and exit.

  * ``--dry-run``

    Don't actually perform the ABI comparison.  Details about what is
    going to be done are emitted on standard output.

  * ``--debug``

    Emit debugging messages about the execution of the program.
    Details about each method invocation, including input parameters
    and returned values, are emitted.

  * ``--traceback``

    Show traceback when an exception raised. This is useful for
    developers of the tool itself to know more exceptional errors.

  * ``--server`` <URL>

    Specifies the URL of the `Koji`_ XMLRPC service the tool talks to.
    The default value of this option is http://koji.fedoraproject.org/kojihub.

  * ``--topurl`` <URL>

    Specifies the URL of the package store the tool downloads RPMs
    from.  The default value of this option is https://kojipkgs.fedoraproject.org.

  * ``--from`` <distro>

    Specifies the name of the baseline `Fedora`_ distribution in which to
    find the first `build`_ that is used for comparison. The
    ``distro`` value can be any valid value of the RPM macro
    ``%{?dist}`` for `Fedora`_, for example, ``fc4``, ``fc23``, ``fc25``.

  * ``--to`` <distro>

    Specifies the name of the `Fedora`_ distribution in which to find
    the `build`_ that is compared against the baseline specified by
    option ``--from``.  The ``distro`` value could be any valid value
    of the RPM macro ``%{?dist}`` for `Fedora`_, for example, ``fc4``,
    ``fc23``.

  * ``--all-subpackages``

    Instructs the tool to also compare the ABI of the binaries in the
    sub-packages of the packages specified.

  * ``--dso-only``

    Compares the ABI of shared libraries only.  If this option is not
    provided, the tool compares the ABI of all ELF binaries found in
    the packages.

  * ``--suppressions`` <*path-to-suppresions*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppressions*.


  * ``--no-default-suppression``

    Do not load the :ref:`default suppression specification files
    <fedabipkgdiff_default_supprs_label>`.

  * ``--no-devel-pkg``

    Do not take associated development packages into account when
    performing the ABI comparison.  This makes the tool report ABI
    changes about all types that are reachable from functions and
    global variables which symbols are defined and publicly exported
    in the binaries being compared, even if those types are not
    defined in public header files available from the packages being
    compared.

  * ``--show-identical-binaries``

   Show the names of the all binaries compared, including the
   binaries whose ABI compare equal.  By default, when this option is
   not provided, only binaries with ABI changes are mentionned in the
   output.

  * ``--abipkgdiff`` <path/to/abipkgdiff>

    Specify an alternative abipkgdiff instead of the one installed in system.

  * ``--clean-cache-before``

    Clean cache before ABI comparison.

  * ``--clean-cache-after``

    Clean cache after ABI comparison.

  * ``--clean-cache``

    If you want to clean cache both before and after ABI comparison,
    ``--clean-cache`` is the convenient way for you to save typing of two
    options at same time.

.. _build:

Note that a build is a specific version and release of an RPM package.
It's specified by its the package name, version and release. These are specified
by the `Fedora Naming Guidelines <https://fedoraproject.org/wiki/Packaging:NamingGuidelines>`_


.. _fedabipkgdiff_return_value_label:

Return value
============

The exit code of the ``abipkgdiff`` command is either 0 if the ABI of
the binaries compared are equivalent, or non-zero if they differ or if
the tool encountered an error.

In the later case, the value of the exit code is the same as for the
:ref:`abidiff tool <abidiff_return_value_label>`.

.. _fedabipkgdiff_usage_example_label:


Use cases
=========

Below are some usage examples currently supported by
``fedabipkgdiff``.

  1. Compare the ABI of binaries in a local package against the ABI of
     the latest stable package in `Fedora`_ 23.

     Suppose you have built just built the ``httpd`` package and you
     want to compare the ABI of the binaries in this locally built
     package against the ABI of the binaries in the latest ``http``
     build from `Fedora`_ 23.  The command line invocation would be: ::

       $ fedabipkgdiff --from fc23 ./httpd-2.4.18-2.fc24.x86_64.rpm

  2. Compare the ABI of binaries in two local packages.

     Suppose you have built two versions of package httpd, and you want to see
     what ABI differences between these two versions of RPM files. The
     command line invocation would be::

       $ fedabipkgdiff path/to/httpd-2.4.23-3.fc23.x86_64.rpm another/path/to/httpd-2.4.23-4.fc24.x86_64.rpm

     All what fedabipkgdiff does happens on local machine without the need of
     querying or downloading RPMs from Koji.

  3. Compare the ABI of binaries in the latest build of the ``httpd``
     package in ``Fedora 23`` against the ABI of the binaries in the
     latest build of the same package in 24.

     In this case, note that neither of the two packages are available
     locally.  The tool is going to talk with the `Fedora Build
     System`_, determine what the versions and releases of the latest
     packages are, download them and perform the comparison locally.
     The command line invocation would be: ::

       $ fedabipkgdiff --from fc23 --to fc24 httpd

  4. Compare the ABI of binaries of two builds of the ``httpd``
     package, designated their versions and releases.

     If we want to do perform the ABI comparison for all the processor
     architectures supported by `Fedora`_ the command line invocation
     would be: ::

       $ fedabipkgdiff httpd-2.8.14.fc23 httpd-2.8.14.fc24

     But if we want to perform the ABI comparison for a specific
     architecture, say, x86_64, then the command line invocation would
     be: ::

       $ fedabipkgdiff httpd-2.8.14.fc23.x86_64 httpd-2.8.14.fc24.x86_64

  5. If the use wants to also compare the sub-packages of a given
     package, she can use the --all-subpackages option.  The first
     command of the previous example would thus look like: ::

       $ fedabipkgdiff --all-subpackages httpd-2.8.14.fc23 httpd-2.8.14.fc24

.. _Fedora: https://getfedora.org
.. _Fedora Build System: http://koji.fedoraproject.org/koji
.. _Koji: https://fedoraproject.org/wiki/Koji

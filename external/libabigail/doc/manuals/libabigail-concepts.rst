#########
Concepts
#########

.. _abi_artifacts_label:

ABI artifacts
=============

An ABI artifact is a relevant part of the ABI of a shared library or
program.  Examples of ABI artifacts are exported types, variables,
functions, or `ELF`_ symbols exported by a shared library.

The set of ABI artifact for a binary is called an ABI Corpus.

.. _harmfulchangeconcept_label:

Harmful changes
===============

A change in the diff report is considered harmful if it might cause
ABI compatibility issues.  That is, it might prevent an application
dynamically linked against a given version of a library to keep
working with the changed subsequent versions of the same library.

.. _harmlesschangeconcept_label:

Harmless changes
================

A change in the diff report is considered harmless if it will not
cause any ABI compatibility issue.  That is, it will not prevent an
application dynamically linked against given version of a library to
keep working with the changed subsequent versions of the same library.

By default, ``abidiff`` filters harmless changes from the diff report.

.. _suppr_spec_label:

Suppression specifications
==========================


Definition
----------

A suppression specification file is a way for a user to instruct
:ref:`abidiff <abidiff_label>`, :ref:`abipkgdiff <abipkgdiff_label>`
or any other relevant libabigail tool to avoid emitting reports for
changes involving certain :ref:`ABI artifacts<abi_artifacts_label>`.

It contains directives (or specifications) that describe the set of
ABI artifacts to avoid emitting change reports about.

Introductory examples
---------------------

Its syntax is based on a simplified and customized form of `Ini File
Syntax`_.  For instance, to specify that change reports on a type
named FooPrivateType should be suppressed, one could write this
suppression specification: ::

   [suppress_type]
     name = FooPrivateType

If we want to ensure that only change reports about structures named
FooPrivateType should be suppressed, we could write: ::

   [suppress_type]
     type_kind = struct
     name = FooPrivateType

But we could also want to suppress change reports avoid typedefs named
FooPrivateType.  In that case we would write:  ::

   [suppress_type]
     type_kind = typedef
     name = FooPrivateType

Or, we could want to suppress change reports about all struct which
names end with the string "PrivateType": ::

   [suppress_type]
     type_kind = struct
     name_regexp = ^.*PrivateType

Let's now look at the generic syntax of suppression specification
files.

Syntax
------

Properties
^^^^^^^^^^

More generally, the format of suppression lists is organized around
the concept of `property`.  Every property has a name and a value,
delimited by the ``=`` sign.  E.g: ::

	 name = value

Leading and trailing white spaces are ignored around property names
and values.

.. _suppr_regexp_label:

Regular expressions
^^^^^^^^^^^^^^^^^^^

The value of some properties might be a regular expression.  In that
case, they must comply with the syntax of `extended POSIX regular
expressions
<http://www.gnu.org/software/findutils/manual/html_node/find_html/posix_002dextended-regular-expression-syntax.html#posix_002dextended-regular-expression-syntax>`_.
Note that Libabigail uses the regular expression engine of the `GNU C
Library`_.

Escaping a character in a regular expression
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When trying to match a string that contains a ``*`` character, like in
the pointer type ``int*``, one must be careful to notice that the
character ``*`` is a special character in the extended POSIX regular
expression syntax.  And that character must be escaped for the regular
expression engine.  Thus the regular expression that would match the
string ``int*`` in a suppression file should be ::

  int\\*

Wait; but then why the two ``\`` characters?  Well, because the ``\``
character is a special character in the `Ini File Syntax`_ used for
specifying suppressions.  So it must be escaped as well, so that the
Ini File parser leaves a ``\`` character intact in the data stream
that is handed to the regular expression engine.  Hence the ``\\``
targeted at the Ini File parser.

So, in short, to escape a character in a regular expression, always
prefix the character with the ``\\`` sequence.

Modus operandi
^^^^^^^^^^^^^^


Suppression specifications can be applied at two different points of
the processing pipeline of libabigail.

.. _late_suppression_mode_label:

In the default operating mode called "late suppression mode",
suppression specifications are applied to the result of comparing the
in-memory internal representations of two ABIs.  In this mode, if an
ABI artifact matches a suppression specification, its changes are not
mentioned in the ABI change report.  The internal representation of
the "suppressed" changed ABI artifact is still present in memory; it
is just not mentioned in the ABI change report.  The change report can
still mention statistics about the number of changed ABI artifacts
that were suppressed.

.. _early_suppression_mode_label:

There is another operating mode called the "early suppression mode"
where suppression specifications are applied during the construction
of the in-memory internal representation of a given ABI.  In that
mode, if an ABI artifact matches a suppression specification, no
in-memory internal representation is built for it.  As a result, no
change about the matched ABI artifact is going to be mentioned in the
ABI change report and no statistic about the number of suppressed ABI
changes is available.  Also, please note that because suppressed ABI
artifacts are removed from the in-memory internal representation in
this mode, the amount memory used by the internal representation is
potentially smaller than the memory consumption in the late
suppression mode.

Sections
^^^^^^^^

Properties are then grouped into arbitrarily named sections that shall
not be nested.  The name of the section is on a line by itself and is
surrounded by square brackets, i.e: ::

	 [section_name]
	 property1_name = property1_value
	 property2_name = property2_value


A section might or might not have properties.  Sections that expect to
have properties and which are found nonetheless empty are just
ignored.  Properties that are not recognized by the reader are ignored
as well.

Section names
^^^^^^^^^^^^^

Each different section can be thought of as being a directive to
suppress ABI change reports for a particular kind of ABI artifact.

``[suppress_file]``
$$$$$$$$$$$$$$$$$$$

This directive prevents a given tool from loading a file (binary or
abixml file) if its file name or other properties match certain
properties.  Thus, if the tool is meant to compare the ABIs of two
files, and if the directive prevents it from loading either one of the
files, then no comparison is performed.

Note that for the ``[suppress_file]`` directive to work, at least one
of the following properties must be provided:

    ``file_name_regexp``, ``file_name_not_regexp``, ``soname_regexp``,
    ``soname_not_regexp``.

If none of the above properties are provided, then the
``[suppress_file]`` directive is simply ignored.

The potential properties of this sections are listed below:

* ``file_name_regexp``

  Usage:

    ``file_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Prevents the system from loading the file which name matches the
  regular expression specified as value of this property.

* ``file_name_not_regexp``

  Usage:

    ``file_name_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Prevents the system from loading the file which name does not match
  the regular expression specified as value of this property.


* ``soname_regexp``

  Usage:

    ``soname_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Prevents the system from loading the file which contains a SONAME
  property that matches the regular expression of this property.  Note
  that this property also works on an abixml file if it contains a
  SONAME property.

* ``soname_not_regexp``

  Usage:

    ``soname_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Prevents the system from loading the file which contains a SONAME
  property that does *NOT* match the regular expression of this
  property.  Note that this property also works on an abixml file if
  it contains a SONAME property.

* ``label``

 Usage:

   ``label`` ``=`` <some-value>

 Define a label for the section.  A label is just an informative
 string that might be used by the tool to refer to a type suppression
 in error messages.

``[suppress_type]``
$$$$$$$$$$$$$$$$$$$

This directive suppresses report messages about a type change.

Note that for the ``[suppress_type]`` directive to work, at least one
of the following properties must be provided:

  ``file_name_regexp``, ``file_name_not_regexp``, ``soname_regexp``,
  ``soname_not_regexp``, ``name``, ``name_regexp``,
  ``name_not_regexp``, ``type_kind``, ``source_location_not_in``,
  ``source_location_not_regexp``.

If none of the above properties are provided, then the
``[suppress_type]`` directive is simply ignored.

The potential properties of this sections are listed below:

* ``file_name_regexp``

  Usage:

    ``file_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  binary file which name matches the regular expression specified as
  value of this property.

* ``file_name_not_regexp``

  Usage:

    ``file_name_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  binary file which name does not match the regular expression
  specified as value of this property.


* ``soname_regexp``

  Usage:

    ``soname_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  shared library which SONAME property matches the regular expression
  specified as value of this property.

* ``soname_not_regexp``

  Usage:

    ``soname_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  shared library which SONAME property does not match the regular
  expression specified as value of this property.

* ``name_regexp``

 Usage:

   ``name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving types whose name matches the
 regular expression specified as value of this property.


* ``name_not_regexp``

 Usage:

   ``name_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving types whose name does *NOT* match
 the regular expression specified as value of this property.  Said
 otherwise, this property specifies which types to keep, rather than
 types to suppress from reports.

* ``name``

 Usage:

   ``name`` ``=`` <a-value>

 Suppresses change reports involving types whose name equals the value
 of this property.

* ``type_kind``

 Usage:

   ``type_kind`` ``=`` ``class`` | ``struct`` | ``union`` | ``enum`` |
		       ``array`` | ``typedef`` | ``builtin``

 Suppresses change reports involving a certain kind of type.  The kind
 of type to suppress change reports for is specified by the possible
 values listed above:

   - ``class``: suppress change reports for class types.  Note that
	 even if class types don't exist for C, this value still
	 triggers the suppression of change reports for struct types,
	 in C.  In C++ however, it should do what it suggests.

   - ``struct``: suppress change reports for struct types in C or C++.
	 Note that the value ``class`` above is a super-set of this
	 one.

   - ``union``: suppress change reports for union types.

   - ``enum``: suppress change reports for enum types.

   - ``array``: suppress change reports for array types.

   - ``typedef``: suppress change reports for typedef types.

   - ``builtin``: suppress change reports for built-in (or native)
     types.  Example of built-in types are char, int, unsigned int,
     etc.

 .. _suppr_source_location_not_in_label:

* ``source_location_not_in``

 Usage:

    ``source_location_not_in`` ``=`` <``list-of-file-paths``>

 Suppresses change reports involving a type which is defined in a file
 which path is *NOT* listed in the value ``list-of-file-paths``.  Note
 that the value is a comma-separated list of file paths e.g, this
 property ::
 
   source_location_not_in = libabigail/abg-ir.h, libabigail/abg-dwarf-reader.h 

 suppresses change reports about all the types that are *NOT* defined
 in header files whose path end up with the strings
 libabigail/abg-ir.h or libabigail/abg-dwarf-reader.h.

 .. _suppr_source_location_not_regexp_label:

* ``source_location_not_regexp``

 Usage:

   ``source_location_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving a type which is defined in a file
 which path does *NOT* match the :ref:`regular expression
 <suppr_regexp_label>` provided as value of the property. E.g, this
 property ::

   source_location_not_regexp = libabigail/abg-.*\\.h

 suppresses change reports involving all the types that are *NOT*
 defined in header files whose path match the regular expression
 provided a value of the property.

 .. _suppr_has_data_member_inserted_at_label:

* ``has_data_member_inserted_at``

 Usage:

   ``has_data_member_inserted_at`` ``=`` <``offset-in-bit``>

 Suppresses change reports involving a type which has at least one
 data member inserted at an offset specified by the property value
 ``offset-in-bit``.  The value ``offset-in-bit`` is either:

	 - an integer value, expressed in bits, which denotes the
	   offset of the insertion point of the data member, starting
	   from the beginning of the relevant structure or class.

	 - the keyword ``end`` which is a named constant which value
	   equals the offset of the end of the of the structure or
	   class.

	 - the function call expression
	   ``offset_of(data-member-name)`` where `data-member-name` is
	   the name of a given data member of the relevant structure
	   or class.  The value of this function call expression is an
	   integer that represents the offset of the data member
	   denoted by ``data-member-name``.

	 - the function call expression
	   ``offset_after(data-member-name)`` where `data-member-name`
	   is the name of a given data member of the relevant
	   structure or class.  The value of this function call
	   expression is an integer that represents the offset of the
	   point that comes right after the region occupied by the
	   data member denoted by ``data-member-name``.

  .. _suppr_has_data_member_inserted_between_label:


* ``has_data_member_inserted_between``

 Usage:

   ``has_data_member_inserted_between`` ``=`` {<``range-begin``>, <``range-end``>}

 Suppresses change reports involving a type which has at least one
 data mber inserted at an offset that is comprised in the range
 between range-begin`` and ``range-end``.  Please note that each of
 the lues ``range-begin`` and ``range-end`` can be of the same form as
 the :ref:`has_data_member_inserted_at
 <suppr_has_data_member_inserted_at_label>` property above.

 Usage examples of this properties are: ::

   has_data_member_inserted_between = {8, 64}

 or: ::

   has_data_member_inserted_between = {16, end}

 or: ::

   has_data_member_inserted_between = {offset_after(member1), end}

.. _suppr_has_data_members_inserted_between_label:


* ``has_data_members_inserted_between``

 Usage:

   ``has_data_members_inserted_between`` ``=`` {<sequence-of-ranges>}

 Suppresses change reports involving a type which has multiple data
 member inserted in various offset ranges.  A usage example of this
 property is, for instance: ::

   has_data_members_inserted_between = {{8, 31}, {72, 95}}

 This usage example suppresses change reports involving a type which
 has data members inserted in bit offset ranges [8 31] and [72 95].
 The length of the sequence of ranges or this
 ``has_data_members_inserted_between`` is not bounded; it can be as
 long as the system can cope with.  The values of the boundaries of
 the ranges are of the same kind as for the
 :ref:`has_data_member_inserted_at
 <suppr_has_data_member_inserted_at_label>` property above.

 Another usage example of this property is thus: ::

   has_data_members_inserted_between =
     {
	  {offset_after(member0), offset_of(member1)},
	  {72, end}
     }

 .. _suppr_accessed_through_property_label:

* ``accessed_through``

 Usage:

   ``accessed_through`` ``=`` <some-predefined-values>

 Suppress change reports involving a type which is referred to either
 directly or through a pointer or a reference.  The potential values
 of this property are the predefined keywords below:

	 * ``direct``

	   So if the ``[suppress_type]`` contains the property
	   description: ::

	     accessed_through = direct

	   then changes about a type that is referred-to
	   directly (i.e, not through a pointer or a reference)
	   are going to be suppressed.

	 * ``pointer``

	   If the ``accessed_through`` property is set to the
	   value ``pointer`` then changes about a type that is
	   referred-to through a pointer are going to be
	   suppressed.

	 * ``reference``

	   If the ``accessed_through`` property is set to the
	   value ``reference`` then changes about a type that is
	   referred-to through a reference are going to be
	   suppressed.

	 * ``reference-or-pointer``

	   If the ``accessed_through`` property is set to the
	   value ``reference-or-pointer`` then changes about a
	   type that is referred-to through either a reference
	   or a pointer are going to be suppressed.

 For an extensive example of how to use this property, please check
 out the example below about :ref:`suppressing change reports about
 types accessed either directly or through pointers
 <example_accessed_through_label>`.

* ``drop``

 Usage:

   ``drop`` ``=`` yes | no

 If a type is matched by a suppression specification which contains
 the "drop" property set to "yes" (or to "true") then the type is not
 even going to be represented in the internal representation of the
 ABI being analyzed.  This property makes its enclosing suppression
 specification to be applied in the :ref:`early suppression
 specification mode <early_suppression_mode_label>`.  The net effect
 is that it potentially reduces the memory used to represent the ABI
 being analyzed.

 Please note that for this property to be effective, the enclosing
 suppression specification must have at least one of the following
 properties specified: ``name_regexp``, ``name``, ``name_regexp``,
 ``source_location_not_in`` or ``source_location_not_regexp``.

 .. _suppr_label_property_label:

* ``label``

 Usage:

   ``label`` ``=`` <some-value>

 Define a label for the section.  A label is just an informative
 string that might be used by a tool to refer to a type suppression in
 error messages.

.. _suppr_changed_enumerators_label:

* ``changed_enumerators``

  Usage:

    ``changed_enumerators`` ``=`` <list-of-enumerators>
    
  Suppresses change reports involving changes in the value of
  enumerators of a given enum type.  This property is applied if the
  ``type_kind`` property is set to the value ``enum``, at least.  The
  value of the ``changed_enumerators`` is a comma-separated list of
  the enumerators that the user expects to change.  For instance: ::

      changed_enumerators = LAST_ENUMERATORS0, LAST_ENUMERATOR1

``[suppress_function]``
$$$$$$$$$$$$$$$$$$$$$$$$

This directive suppresses report messages about changes on a set of
functions.

Note that for the ``[suppress_function]`` directive to work, at least
one of the following properties must be provided:

  ``label``, ``file_name_regexp``, ``file_name_not_regexp``,
  ``soname_regexp``, ``soname_not_regexp``, ``name``, ``name_regexp``,
  ``name_not_regexp``, ``parameter``, ``return_type_name``,
  ``return_type_regexp``, ``symbol_name``, ``symbol_name_regexp``,
  ``symbol_name_not_regexp``, ``symbol_version``,
  ``symbol_version_regexp``.

If none of the above properties are provided, then the
``[suppress_function]`` directive is simply ignored.

The potential properties of this sections are:

* ``label``

 Usage:

   ``label`` ``=`` <some-value>

 This property is the same as the :ref:`label property
 <suppr_label_property_label>` defined above.


* ``file_name_regexp``

  Usage:

  ``file_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  binary file which name matches the regular expression specified as
  value of this property.


* ``file_name_not_regexp``

  Usage:

    ``file_name_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  binary file which name does not match the regular expression
  specified as value of this property.

* ``soname_regexp``

  Usage:

    ``soname_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  shared library which SONAME property matches the regular expression
  specified as value of this property.

* ``soname_not_regexp``

  Usage:

    ``soname_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  shared library which SONAME property does not match the regular
  expression specified as value of this property.


* ``name``

 Usage:

   ``name`` ``=`` <some-value>

 Suppresses change reports involving functions whose name equals the
 value of this property.

* ``name_regexp``

 Usage:

   ``name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving functions whose name matches the
 regular expression specified as value of this property.

 Let's consider the case of functions that have several symbol names.
 This happens when the underlying symbol for the function has
 aliases.  Each symbol name is actually one alias name.

 In this case, if the regular expression matches the name of
 at least one of the aliases names, then it must match the names of
 all of the aliases of the function for the directive to actually
 suppress the diff reports for said function.


* ``name_not_regexp``

 Usage:

   ``name_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving functions whose names don't match
 the regular expression specified as value of this property.

 The rules for functions that have several symbol names are the same
 rules as for the ``name_regexp`` property above.

  .. _suppr_change_kind_property_label:


* ``change_kind``

 Usage:

   ``change_kind`` ``=`` <predefined-possible-values>

 Specifies the kind of changes this suppression specification should
 apply to.  The possible values of this property as well as their
 meaning are listed below:

	 - ``function-subtype-change``

	   This suppression specification applies to functions
	   that which have at least one sub-type that has
	   changed.

	 - ``added-function``

	   This suppression specification applies to functions
	   that have been added to the binary.

	 - ``deleted-function``

	   This suppression specification applies to functions
	   that have been removed from the binary.

	 - ``all``

	   This suppression specification applies to functions
	   that have all of the changes above.  Note that not
	   providing the ``change_kind`` property at all is
	   equivalent to setting it to the value ``all``.


* ``parameter``

 Usage:

   ``parameter`` ``=`` <function-parameter-specification>

 Suppresses change reports involving functions whose
 parameters match the parameter specification indicated as
 value of this property.

 The format of the function parameter specification is:

 ``'`` ``<parameter-index>`` ``<space>`` ``<type-name-or-regular-expression>``

 That is, an apostrophe followed by a number that is the
 index of the parameter, followed by one of several spaces,
 followed by either the name of the type of the parameter,
 or a regular expression describing a family of parameter
 type names.

 If the parameter type name is designated by a regular
 expression, then said regular expression must be enclosed
 between two slashes; like ``/some-regular-expression/``.

 The index of the first parameter of the function is zero.
 Note that for member functions (methods of classes), the
 this is the first parameter that comes after the implicit
 "this" pointer parameter.

 Examples of function parameter specifications are: ::

   '0 int

 Which means, the parameter at index 0, whose type name is
 ``int``. ::

   '4 unsigned char*

 Which means, the parameter at index 4, whose type name is
 ``unsigned char*``.  ::

   '2 /^foo.*&/

 Which means, the parameter at index 2, whose type name
 starts with the string "foo" and ends with an '&'.  In
 other words, this is the third parameter and it's a
 reference on a type that starts with the string "foo".

* ``return_type_name``

 Usage:

   ``return_type_name`` ``=`` <some-value>

 Suppresses change reports involving functions whose return type name
 equals the value of this property.

* ``return_type_regexp``

 Usage:

   ``return_type_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving functions whose return type name
 matches the regular expression specified as value of this property.

* ``symbol_name``

 Usage:

   ``symbol_name`` ``=`` <some-value>

 Suppresses change reports involving functions whose symbol name equals
 the value of this property.

* ``symbol_name_regexp``

 Usage:

   ``symbol_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving functions whose symbol name
 matches the regular expression specified as value of this property.

 Let's consider the case of functions that have several symbol names.
 This happens when the underlying symbol for the function has
 aliases.  Each symbol name is actually one alias name.

 In this case, the regular expression must match the names of all of
 the aliases of the function for the directive to actually suppress
 the diff reports for said function.

* ``symbol_name_not_regexp``

 Usage:

   ``symbol_name_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving functions whose symbol name does
 not match the regular expression specified as value of this property.

* ``symbol_version``

 Usage:

   ``symbol_version`` ``=`` <some-value>

 Suppresses change reports involving functions whose symbol version
 equals the value of this property.

* ``symbol_version_regexp``

 Usage:

   ``symbol_version_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving functions whose symbol version
 matches the regular expression specified as value of this property.

* ``drop``

 Usage:

   ``drop`` ``=`` yes | no

 If a function is matched by a suppression specification which
 contains the "drop" property set to "yes" (or to "true") then the
 function is not even going to be represented in the internal
 representation of the ABI being analyzed.  This property makes its
 enclosing suppression specification to be applied in the :ref:`early
 suppression specification mode <early_suppression_mode_label>`.  The
 net effect is that it potentially reduces the memory used to
 represent the ABI being analyzed.

 Please note that for this property to be effective, the enclosing
 suppression specification must have at least one of the following
 properties specified: ``name_regexp``, ``name``, ``name_regexp``,
 ``source_location_not_in`` or ``source_location_not_regexp``.

``[suppress_variable]``
$$$$$$$$$$$$$$$$$$$$$$$$

This directive suppresses report messages about changes on a set of
variables.

Note that for the ``[suppress_variable]`` directive to work, at least
one of the following properties must be provided:

  ``label``, ``file_name_regexp``, ``file_name_not_regexp``,
  ``soname_regexp``, ``soname_not_regexp``, ``name``, ``name_regexp``,
  ``name_not_regexp``, ``symbol_name``, ``symbol_name_regexp``,
  ``symbol_name_not_regexp``, ``symbol_version``,
  ``symbol_version_regexp``, ``type_name``, ``type_name_regexp``.

If none of the above properties are provided, then the
``[suppress_variable]`` directive is simply ignored.

The potential properties of this sections are:

* ``label``

 Usage:

   ``label`` ``=`` <some-value>

 This property is the same as the :ref:`label property
 <suppr_label_property_label>` defined above.


* ``file_name_regexp``

  Usage:

  ``file_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  binary file which name matches the regular expression specified as
  value of this property.

* ``file_name_not_regexp``

  Usage:

   ``file_name_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  binary file which name does not match the regular expression
  specified as value of this property.


* ``soname_regexp``

  Usage:

   ``soname_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  shared library which SONAME property matches the regular expression
  specified as value of this property.


* ``soname_not_regexp``

  Usage:

    ``soname_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

  Suppresses change reports about ABI artifacts that are defined in a
  shared library which SONAME property does not match the regular
  expression specified as value of this property.


* ``name``

 Usage:

   ``name`` ``=`` <some-value>

 Suppresses change reports involving variables whose name equals the
 value of this property.

* ``name_regexp``

 Usage:

   ``name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving variables whose name matches the
 regular expression specified as value of this property.

* ``change_kind``

 Usage:

   ``change_kind`` ``=`` <predefined-possible-values>

 Specifies the kind of changes this suppression specification should
 apply to.  The possible values of this property as well as their
 meaning are the same as when it's :ref:`used in the
 [suppress_function] section <suppr_change_kind_property_label>`.

* ``symbol_name``

 Usage:

   ``symbol_name`` ``=`` <some-value>

 Suppresses change reports involving variables whose symbol name equals
 the value of this property.

* symbol_name_regexp

 Usage:

   ``symbol_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving variables whose symbol name
 matches the regular expression specified as value of this property.


* ``symbol_name_not_regexp``

 Usage:

   ``symbol_name_not_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving variables whose symbol name does
 not match the regular expression specified as value of this property.

* ``symbol_version``

 Usage:

   ``symbol_version`` ``=`` <some-value>

 Suppresses change reports involving variables whose symbol version
 equals the value of this property.

* ``symbol_version_regexp``

 Usage:

   ``symbol_version_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving variables whose symbol version
 matches the regular expression specified as value of this property.

* ``type_name``

 Usage:

   ``type_name`` ``=`` <some-value>

 Suppresses change reports involving variables whose type name equals
 the value of this property.

* ``type_name_regexp``

 Usage:

   ``type_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

 Suppresses change reports involving variables whose type name matches
 the regular expression specified as value of this property.

Comments
^^^^^^^^

``;`` or ``#`` ASCII character at the beginning of a line
indicates a comment.  Comment lines are ignored.

Code examples
^^^^^^^^^^^^^

1. Suppressing change reports about types.

   Suppose we have a library named ``libtest1-v0.so`` which
   contains this very useful code: ::

	$ cat -n test1-v0.cc
	     1	// A forward declaration for a type considered to be opaque to
	     2	// function foo() below.
	     3	struct opaque_type;
	     4
	     5	// This function cannot touch any member of opaque_type.  Hence,
	     6	// changes to members of opaque_type should not impact foo, as far as
	     7	// ABI is concerned.
	     8	void
	     9	foo(opaque_type*)
	    10	{
	    11	}
	    12
	    13	struct opaque_type
	    14	{
	    15	  int member0;
	    16	  char member1;
	    17	};
	$

Let's change the layout of struct opaque_type by inserting a data
member around line 15, leading to a new version of the library,
that we shall name ``libtest1-v1.so``: ::

	$ cat -n test1-v1.cc
	     1	// A forward declaration for a type considered to be opaque to
	     2	// function foo() below.
	     3	struct opaque_type;
	     4
	     5	// This function cannot touch any member of opaque_type;  Hence,
	     6	// changes to members of opaque_type should not impact foo, as far as
	     7	// ABI is concerned.
	     8	void
	     9	foo(opaque_type*)
	    10	{
	    11	}
	    12
	    13	struct opaque_type
	    14	{
	    15	  char added_member; // <-- a new member got added here now.
	    16	  int member0;
	    17	  char member1;
	    18	};
	$

Let's compile both examples.  We shall not forget to compile them
with debug information generation turned on: ::

	$ g++ -shared -g -Wall -o libtest1-v0.so test1-v0.cc
	$ g++ -shared -g -Wall -o libtest1-v1.so test1-v1.cc

Let's ask :ref:`abidiff <abidiff_label>` which ABI differences it sees
between ``libtest1-v0.so`` and ``libtest1-v1.so``: ::

	$ abidiff libtest1-v0.so libtest1-v1.so
	Functions changes summary: 0 Removed, 1 Changed, 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	1 function with some indirect sub-type change:

	  [C]'function void foo(opaque_type*)' has some indirect sub-type changes:
	    parameter 0 of type 'opaque_type*' has sub-type changes:
	      in pointed to type 'struct opaque_type':
		size changed from 64 to 96 bits
		1 data member insertion:
		  'char opaque_type::added_member', at offset 0 (in bits)
		2 data member changes:
		 'int opaque_type::member0' offset changed from 0 to 32
		 'char opaque_type::member1' offset changed from 32 to 64


So ``abidiff`` reports that the opaque_type's layout has changed
in a significant way, as far as ABI implications are concerned, in
theory.  After all, a sub-type (``struct opaque_type``) of an
exported function (``foo()``) has seen its layout change.  This
might have non negligible ABI implications.  But in practice here,
the programmer of the litest1-v1.so library knows that the "soft"
contract between the function ``foo()`` and the type ``struct
opaque_type`` is to stay away from the data members of the type.
So layout changes of ``struct opaque_type`` should not impact
``foo()``.

Now to teach ``abidiff`` about this soft contract and have it
avoid emitting what amounts to false positives in this case, we
write the suppression specification file below: ::

	$ cat test1.suppr
	[suppress_type]
	  type_kind = struct
	  name = opaque_type

Translated in plain English, this suppression specification would
read: "Do not emit change reports about a struct which name is
opaque_type".

Let's now invoke ``abidiff`` on the two versions of the library
again, but this time with the suppression specification: ::

	$ abidiff --suppressions test1.suppr libtest1-v0.so libtest1-v1.so
	Functions changes summary: 0 Removed, 0 Changed (1 filtered out), 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

As you can see, ``abidiff`` does not report the change anymore; it
tells us that it was filtered out instead.

Suppressing change reports about types with data member
insertions

Suppose the first version of a library named ``libtest3-v0.so``
has this source code: ::

	/* Compile this with:
	     gcc -g -Wall -shared -o libtest3-v0.so test3-v0.c
	 */

	struct S
	{
	  char member0;
	  int member1; /* 
			  between member1 and member2, there is some padding,
			  at least on some popular platforms.  On
			  these platforms, adding a small enough data
			  member into that padding shouldn't change
			  the offset of member1.  Right?
			*/
	};

	int
	foo(struct S* s)
	{
	  return s->member0 + s->member1;
	}

Now, suppose the second version of the library named
``libtest3-v1.so`` has this source code in which a data member
has been added in the padding space of struct S and another data
member has been added at its end: ::

	/* Compile this with:
	     gcc -g -Wall -shared -o libtest3-v1.so test3-v1.c
	 */

	struct S
	{
	  char member0;
	  char inserted1; /* <---- A data member has been added here...  */
	  int member1;
	  char inserted2; /* <---- ... and another one has been added here.  */
	};

	int
	foo(struct S* s)
	{
	  return s->member0 + s->member1;
	}


In libtest3-v1.so, adding char data members ``S::inserted1`` and
``S::inserted2`` can be considered harmless (from an ABI compatibility
perspective), at least on the x86 platform, because that doesn't
change the offsets of the data members S::member0 and S::member1.  But
then running ``abidiff`` on these two versions of library yields: ::

	$ abidiff libtest3-v0.so libtest3-v1.so
	Functions changes summary: 0 Removed, 1 Changed, 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	1 function with some indirect sub-type change:

	  [C]'function int foo(S*)' has some indirect sub-type changes:
	    parameter 0 of type 'S*' has sub-type changes:
	      in pointed to type 'struct S':
		type size changed from 64 to 96 bits
		2 data member insertions:
		  'char S::inserted1', at offset 8 (in bits)
		  'char S::inserted2', at offset 64 (in bits)
	$



That is, ``abidiff`` shows us the two changes, even though we (the
developers of that very involved library) know that these changes
are harmless in this particular context.

Luckily, we can devise a suppression specification that essentially
tells abidiff to filter out change reports about adding a data
member between ``S::member0`` and ``S::member1``, and adding a data
member at the end of struct S.  We have written such a suppression
specification in a file called test3-1.suppr and it unsurprisingly
looks like: ::

	[suppress_type]
	  name = S
	  has_data_member_inserted_between = {offset_after(member0), offset_of(member1)}
	  has_data_member_inserted_at = end


Now running ``abidiff`` with this suppression specification yields: ::

	$ ../build/tools/abidiff --suppressions test3-1.suppr libtest3-v0.so libtest3-v1.so
	Functions changes summary: 0 Removed, 0 Changed (1 filtered out), 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	$ 


Hooora! \\o/ (I guess)

.. _example_accessed_through_label:

Suppressing change reports about types accessed either directly
or through pointers

Suppose we have a first version of an object file which source
code is the file widget-v0.cc below: ::

	// Compile with: g++ -g -c widget-v0.cc

	struct widget
	{
	  int x;
	  int y;

	  widget()
	    :x(), y()
	  {}
	};

	void
	fun0(widget*)
	{
	  // .. do stuff here.
	}

	void
	fun1(widget&)
	{
	  // .. do stuff here ..
	}

	void
	fun2(widget w)
	{
	  // ... do other stuff here ...
	}

Now suppose in the second version of that file, named
widget-v1.cc, we have added some data members at the end of
the type ``struct widget``; here is what the content of that file
would look like: ::

	// Compile with: g++ -g -c widget-v1.cc

	struct widget
	{
	  int x;
	  int y;
	  int w; // We have added these two new data members here ..
	  int h; // ... and here.

	  widget()
	    : x(), y(), w(), h()
	  {}
	};

	void
	fun0(widget*)
	{
	  // .. do stuff here.
	}

	void
	fun1(widget&)
	{
	  // .. do stuff here ..
	}

	void
	fun2(widget w)
	{
	  // ... do other stuff here ...
	}

When we invoke ``abidiff`` on the object files resulting from the
compilation of the two file above, here is what we get: ::

	$ abidiff widget-v0.o widget-v1.o
	Functions changes summary: 0 Removed, 2 Changed (1 filtered out), 0 Added functions
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	2 functions with some indirect sub-type change:

	  [C]'function void fun0(widget*)' has some indirect sub-type changes:
	    parameter 1 of type 'widget*' has sub-type changes:
	      in pointed to type 'struct widget':
		type size changed from 64 to 128 bits
		2 data member insertions:
		  'int widget::w', at offset 64 (in bits)
		  'int widget::h', at offset 96 (in bits)

	  [C]'function void fun2(widget)' has some indirect sub-type changes:
	    parameter 1 of type 'struct widget' has sub-type changes:
	      details were reported earlier
       $

I guess a little bit of explaining is due here.  ``abidiff``
detects that two data member got added at the end of ``struct
widget``.  it also tells us that the type change impacts the
exported function ``fun0()`` which uses the type ``struct
widget`` through a pointer, in its signature.

Careful readers will notice that the change to ``struct widget``
also impacts the exported function ``fun1()``, that uses type
``struct widget`` through a reference.  But then ``abidiff``
doesn't tell us about the impact on that function ``fun1()``
because it has evaluated that change as being **redundant** with
the change it reported on ``fun0()``.  It has thus filtered it
out, to avoid cluttering the output with noise.

Redundancy detection and filtering is fine and helpful to avoid
burying the important information in a sea of noise.  However, it
must be treated with care, by fear of mistakenly filtering out
relevant and important information.

That is why ``abidiff`` tells us about the impact that the change
to ``struct widget`` has on function ``fun2()``.  In this case,
that function uses the type ``struct widget`` **directly** (in
its signature).  It does not use it via a pointer or a reference.
In this case, the direct use of this type causes ``fun2()`` to be
exposed to a potentially harmful ABI change.  Hence, the report
about ``fun2()`` is not filtered out, even though it's about that
same change on ``struct widget``.

To go further in suppressing reports about changes that are
harmless and keeping only those that we know are harmful, we
would like to go tell abidiff to suppress reports about this
particular ``struct widget`` change when it impacts uses of
``struct widget`` through a pointer or reference.  In other
words, suppress the change reports about ``fun0()`` **and**
``fun1()``.  We would then write this suppression specification,
in file ``widget.suppr``: ::

	[suppress_type]
	  name = widget
	  type_kind = struct
	  has_data_member_inserted_at = end
	  accessed_through = reference-or-pointer

	  # So this suppression specification says to suppress reports about
	  # the type 'struct widget', if this type was added some data member
	  # at its end, and if the change impacts uses of the type through a
	  # reference or a pointer.

Invoking ``abidiff`` on ``widget-v0.o`` and ``widget-v1.o`` with
this suppression specification yields: ::

	$ abidiff --suppressions widget.suppr widget-v0.o widget-v1.o
	Functions changes summary: 0 Removed, 1 Changed (2 filtered out), 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	1 function with some indirect sub-type change:

	  [C]'function void fun2(widget)' has some indirect sub-type changes:
	    parameter 1 of type 'struct widget' has sub-type changes:
	      type size changed from 64 to 128 bits
	      2 data member insertions:
		'int widget::w', at offset 64 (in bits)
		'int widget::h', at offset 96 (in bits)
	$

As expected, I guess.

Suppressing change reports about functions.

Suppose we have a first version a library named
``libtest2-v0.so`` whose source code is: ::

	 $ cat -n test2-v0.cc

	  1	struct S1
	  2	{
	  3	  int m0;
	  4	
	  5	  S1()
	  6	    : m0()
	  7	  {}
	  8	};
	  9	
	 10	struct S2
	 11	{
	 12	  int m0;
	 13	
	 14	  S2()
	 15	    : m0()
	 16	  {}
	 17	};
	 18	
	 19	struct S3
	 20	{
	 21	  int m0;
	 22	
	 23	  S3()
	 24	    : m0()
	 25	  {}
	 26	};
	 27	
	 28	int
	 29	func(S1&)
	 30	{
	 31	  // suppose the code does something with the argument.
	 32	  return 0;
	 33	
	 34	}
	 35	
	 36	char
	 37	func(S2*)
	 38	{
	 39	  // suppose the code does something with the argument.
	 40	  return 0;
	 41	}
	 42	
	 43	unsigned
	 44	func(S3)
	 45	{
	 46	  // suppose the code does something with the argument.
	 47	  return 0;
	 48	}
	$
	
And then we come up with a second version ``libtest2-v1.so`` of
that library; the source code is modified by making the
structures ``S1``, ``S2``, ``S3`` inherit another struct: ::

	$ cat -n test2-v1.cc
	      1	struct base_type
	      2	{
	      3	  int m_inserted;
	      4	};
	      5	
	      6	struct S1 : public base_type // <--- S1 now has base_type as its base
	      7				     // type.
	      8	{
	      9	  int m0;
	     10	
	     11	  S1()
	     12	    : m0()
	     13	  {}
	     14	};
	     15	
	     16	struct S2 : public base_type // <--- S2 now has base_type as its base
	     17				     // type.
	     18	{
	     19	  int m0;
	     20	
	     21	  S2()
	     22	    : m0()
	     23	  {}
	     24	};
	     25	
	     26	struct S3 : public base_type // <--- S3 now has base_type as its base
	     27				     // type.
	     28	{
	     29	  int m0;
	     30	
	     31	  S3()
	     32	    : m0()
	     33	  {}
	     34	};
	     35	
	     36	int
	     37	func(S1&)
	     38	{
	     39	  // suppose the code does something with the argument.
	     40	  return 0;
	     41	
	     42	}
	     43	
	     44	char
	     45	func(S2*)
	     46	{
	     47	  // suppose the code does something with the argument.
	     48	  return 0;
	     49	}
	     50	
	     51	unsigned
	     52	func(S3)
	     53	{
	     54	  // suppose the code does something with the argument.
	     55	  return 0;
	     56	}
	 $ 

Now let's build the two libraries: ::

	 g++ -Wall -g -shared -o libtest2-v0.so test2-v0.cc
	 g++ -Wall -g -shared -o libtest2-v0.so test2-v0.cc

Let's look at the output of ``abidiff``: ::

	 $ abidiff libtest2-v0.so libtest2-v1.so 
	 Functions changes summary: 0 Removed, 3 Changed, 0 Added functions
	 Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	 3 functions with some indirect sub-type change:

	   [C]'function unsigned int func(S3)' has some indirect sub-type changes:
	     parameter 0 of type 'struct S3' has sub-type changes:
	       size changed from 32 to 64 bits
	       1 base class insertion:
		 struct base_type
	       1 data member change:
		'int S3::m0' offset changed from 0 to 32

	   [C]'function char func(S2*)' has some indirect sub-type changes:
	     parameter 0 of type 'S2*' has sub-type changes:
	       in pointed to type 'struct S2':
		 size changed from 32 to 64 bits
		 1 base class insertion:
		   struct base_type
		 1 data member change:
		  'int S2::m0' offset changed from 0 to 32

	   [C]'function int func(S1&)' has some indirect sub-type changes:
	     parameter 0 of type 'S1&' has sub-type changes:
	       in referenced type 'struct S1':
		 size changed from 32 to 64 bits
		 1 base class insertion:
		   struct base_type
		 1 data member change:
		  'int S1::m0' offset changed from 0 to 32
	 $

Let's tell ``abidiff`` to avoid showing us the differences on the
overloads of ``func`` that takes either a pointer or a reference.
For that, we author this simple suppression specification: ::

	 $ cat -n libtest2.suppr
	      1	[suppress_function]
	      2	  name = func
	      3	  parameter = '0 S1&
	      4	
	      5	[suppress_function]
	      6	  name = func
	      7	  parameter = '0 S2*
	 $

And then let's invoke ``abidiff`` with the suppression
specification: ::

  $ ../build/tools/abidiff --suppressions libtest2.suppr libtest2-v0.so libtest2-v1.so 
  Functions changes summary: 0 Removed, 1 Changed (2 filtered out), 0 Added function
  Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

  1 function with some indirect sub-type change:

	 [C]'function unsigned int func(S3)' has some indirect sub-type changes:
	   parameter 0 of type 'struct S3' has sub-type changes:
	     size changed from 32 to 64 bits
	     1 base class insertion:
	       struct base_type
	     1 data member change:
	      'int S3::m0' offset changed from 0 to 32


The suppression specification could be reduced using
:ref:`regular expressions <suppr_regexp_label>`: ::

  $ cat -n libtest2-1.suppr
	    1	[suppress_function]
	    2	  name = func
	    3	  parameter = '0 /^S.(&|\\*)/
  $

  $ ../build/tools/abidiff --suppressions libtest2-1.suppr libtest2-v0.so libtest2-v1.so 
  Functions changes summary: 0 Removed, 1 Changed (2 filtered out), 0 Added function
  Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

  1 function with some indirect sub-type change:

	 [C]'function unsigned int func(S3)' has some indirect sub-type changes:
	   parameter 0 of type 'struct S3' has sub-type changes:
	     size changed from 32 to 64 bits
	     1 base class insertion:
	       struct base_type
	     1 data member change:
	      'int S3::m0' offset changed from 0 to 32

  $

.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format

.. _Ini File Syntax: http://en.wikipedia.org/wiki/INI_file

.. _GNU C Library: http://www.gnu.org/software/libc

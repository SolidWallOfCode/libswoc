.. Licensed to the Apache Software Foundation (ASF) under one or more contributor license
   agreements.  See the NOTICE file distributed with this work for additional information regarding
   copyright ownership.  The ASF licenses this file to you under the Apache License, Version 2.0
   (the "License"); you may not use this file except in compliance with the License.  You may obtain
   a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software distributed under the License
   is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
   or implied.  See the License for the specific language governing permissions and limitations
   under the License.

.. include:: ../common-defs.rst

.. default-domain:: cpp
.. highlight:: cpp
.. |TV| replace:: :code:`TextView`
.. |SV| replace:: :code:`std::string_view`.

.. _string-view: https://en.cppreference.com/w/cpp/string/basic_string_view

********
TextView
********

Synopsis
********

:code:`#include "swoc/TextView.h"`

.. class:: TextView

   :libswoc:`Reference documentation <swoc::TextView>`.

This class acts as a view of memory allocated / owned elsewhere and treated as a sequence of 8 bit
characters. It is in effect a pointer and should be treated as such (e.g. care must be taken to
avoid dangling references by knowing where the memory really is). The purpose is to provide string
manipulation that is safer than raw pointers and much faster than duplicating strings.

Usage
*****

|TV| is a subclass of `std::string_view <string-view>`_ and inherits all of its methods. The
additional functionality of |TV| is for easy string manipulation, with an emphasis on fast parsing
of string data. As noted, an instance of |TV| is a pointer and needs to be handled as such. It does
not own the memory and therefore, like a pointer, care must be taken that the memory is not
deallocated while the |TV| still references it. The advantage of this is creating new views and
modifying existing ones is very cheap.

Any place that passes a :code:`char *` and a size is an excellent candidate for using a |TV|. Code
that uses functions such as :code:`strtok` or tracks pointers and offsets internally is an excellent
candidate for using |TV| instead.

Because |TV| is a subclass of :code:`std::string_view` it can be unclear which is a better choice.
In many cases it doesn't matter, since because of this relationship converting between the types is
at most as expensive as a copy of the same type, and in cases of constant reference, can be free. In
general if the string is treated as a block of data, :code:`std::string_view` is a better choice. If
the contents of the string are to be examined / parsed then |TV| is better. For example, if the
string is used simply as a key or a hash source, use :code:`std::string_view`. Contrariwise if the
string may contain substrings of interest such as key / value pairs, then use a |TV|. I do sometimes
use |TV| because of the lack of support for instance reuse in |SV| - e.g. no :code:`assign` or
:code:`clear` methods.

When passing |TV| as an argument, it is very debatable whether passing by value or passing by
reference is more efficient, therefore it's not likely to matter in production code. My personal
heuristic is whether the function will modify the value. If so, passing by value saves a copy to a
local variable therefore it should be passed by value. If the function simply passes the |TV| on to
other functions, then pass by constant reference. This distinction is irrelevant to the caller, the
same code at the call site will work in either case.

As noted, |TV| is designed as a pointer style class. Therefore it has an increment operator which is
equivalent to :code:`std::string_view::remove_prefix`. |TV| also has  a dereference operator, which
acts the same way as on a pointer. The difference is the view knows where the end of the view is.
This provides a comfortably familiar way of iterating through a view, the main difference being
checking the view itself rather than a dereference of it (like a C-style string) or a range limit.
E.g. the code to write a simple hash function [#]_ could be

.. code-block:: cpp

   void hasher(TextView v) {
      size_t hash = 0;
      while (v) {
         hash = hash * 13 + * v ++;
      }
      return hash;
   }

Because |TV| inherits from :code:`std::string_view` it can also be used as a container for range
:code:`for` loops.

.. code-block:: cpp

   void hasher(TextView const& v) {
      size_t hash = 0;
      for (char c : v) hash = hash * 13 + c;
      return hash;
   }

The standard functions :code:`strcmp`, :code:`memcmp`, and :code:`strcasecmp` are overloaded for
|TV| so that a |TV| can be used as if it were a C-style string. The size is is taken from the |TV|
and doesn't need to be passed in explicitly.

Basic Operations
================

|TV| is essentially a collection of operations which have been found to be common and useful in
manipulating contiguous blocks of text.

Construction
------------

Constructing a view means creating a view from another object which owns the memory (for creating
views from other views see `Extraction`_). This can be a :code:`char const*` pointer and size, two
pointers, a literal string, a :code:`std::string` or a :code:`std::string_view` although in the last
case there is presumably yet another object that actually owns the memory. All of these constructors
require only the equivalent of two assignment statements. The one thing to be careful of is if a
literal string is used, the |TV| will drop the terminating nul character from the view. This is almost
always the correct behavior, but if it isn't an explicit size can be used. There is no constructor
from just a C-style string because this has overloading conflicts with the literal string constructor.
In practice this has rarely been an issue, particularly with fully C++ code which should not be passing
around raw pointers to C-style strings.

A |TV| can be constructed from a null :code:`char const*` pointer or a straight :code:`nullptr`. This
will construct an empty |TV| identical to one default constructed.

Searching
---------

Because |TV| is a subclass of :code:`std::string_view` all of its search method work on a |TV|. The
only search methods provided beyond those are :libswoc:`TextView::find_if` and
:libswoc:`TextView::rfind_if` which search the view by a predicate. The predicate takes a single
:code:`char` argument and returns a :code:`bool`. The search terminates on the first character for
which the predicate returns :code:`true`.

Extraction
----------

Extraction is creating a new view from an existing view. Because views cannot in general be expanded
new views will be sub-sequences of existing views. This is the primary utility of a |TV|. As
noted in the `general description <Description>`_ |TV| supports copying or removing prefixes and
suffixes of the view. All of this is possible using the underlying :code:`std::string_view_substr`
but this is frequently much clumsier. The development of |TV| was driven to a large extent by the
desire to make such code much more compact and expressive, while being at least as safe. In particular
extraction methods on |TV| do useful and well defined things when given out of bounds arguments.
This is quite handy when extracting tokens based on separator characters.

The primary distinction is how a character in the view is selected.

*  By index, an offset in to the view. These have plain names, such as :libswoc:`TextView::prefix`.

*  By character comparison, either a single character or set of characters which is matched against a single
   character in the view. These are suffixed with "at" such as :libswoc:`TextView::prefix_at`.

*  By predicate, a function that takes a single character argument and returns a bool to indicate a match.
   These are suffixed with "if", such as :libswoc:`TextView::prefix_if`.

A secondary distinction is what is done to the view by the methods.

*  The base methods make a new view without modifying the existing view.

*  The "split..." methods remove the corresponding part of the view and return it. The selected character
   is discarded and not left in either the returned view nor the source view. If the selected character
   is not in the view, an empty view is returned and the source view is not modified.

*  The "take..." methods remove the corresponding part of the view and return it. The selected character
   is discarded and not left in either the returned view nor the source view. If the selected character
   is not in the view, the entire view is returned and the source view is cleared.

.. _`std::string_view::remove_prefix`: https://en.cppreference.com/w/cpp/string/basic_string_view/remove_prefix
.. _`std::string_view::remove_suffix`: https://en.cppreference.com/w/cpp/string/basic_string_view/remove_suffix

This is a table of the affix oriented methods, grouped by the properties of the methods. "Bounded"
indicates whether the operation requires the target character, however specified, to be within the
bounds of the view. A bounded method does nothing if the target character is not in the view. On
this note, the :code:`remove_prefix` and :code:`remove_suffix` are differently implement in |TV|
compared to :code:`std::string_view`. Rather than being undefined, the methods will clear the view
if the size specified is larger than the contents of the view.

+-----------------+--------+---------+------------------------------------------+
| Operation       | Affix  | Bounded | Method                                   |
+=================+========+=========+==========================================+
| Copy            | Prefix | No      | :libswoc:`TextView::prefix`              |
|                 +        +---------+------------------------------------------+
|                 |        | Yes     | :libswoc:`TextView::prefix_at`           |
|                 +        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::prefix_if`           |
|                 +--------+---------+------------------------------------------+
|                 | Suffix | No      | :libswoc:`TextView::suffix`              |
|                 +        +---------+------------------------------------------+
|                 |        | Yes     | :libswoc:`TextView::suffix_at`           |
|                 +        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::suffix_if`           |
+-----------------+--------+---------+------------------------------------------+
| Modify          | Prefix | No      | `std::string_view::remove_prefix`_       |
|                 |        +---------+------------------------------------------+
|                 |        | Yes     | :libswoc:`TextView::remove_prefix_at`    |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::remove_prefix_if`    |
|                 +--------+---------+------------------------------------------+
|                 | Suffix | No      | `std::string_view::remove_suffix`_       |
|                 |        +---------+------------------------------------------+
|                 |        | Yes     | :libswoc:`TextView::remove_suffix_at`    |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::remove_suffix_if`    |
+-----------------+--------+---------+------------------------------------------+
| Modify and Copy | Prefix | Yes     | :libswoc:`TextView::split_prefix`        |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::split_prefix_at`     |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::split_prefix_if`     |
|                 |        +---------+------------------------------------------+
|                 |        | No      | :libswoc:`TextView::take_prefix`         |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::take_prefix_at`      |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::take_prefix_if`      |
|                 +--------+---------+------------------------------------------+
|                 | Suffix | Yes     | :libswoc:`TextView::split_suffix`        |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::split_suffix_at`     |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::split_suffix_if`     |
|                 |        +---------+------------------------------------------+
|                 |        | No      | :libswoc:`TextView::take_suffix`         |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::take_suffix_at`      |
|                 |        +         +------------------------------------------+
|                 |        |         | :libswoc:`TextView::take_suffix_if`      |
+-----------------+--------+---------+------------------------------------------+

Other
-----

The comparison operators for |TV| are inherited from :code:`std::string_view` and therefore use the
content of the view to determine the relationship.

|TV| provides a collection of "trim" methods which remove leading or trailing characters. These have
similar suffixes with the same meaning as the affix methods. This can be done for a single
character, one of a set of characters, or a predicate. The most common use is with the predicate
:code:`isspace` which removes leading and/or trailing whitespace as needed.

Numeric conversions are provided, in signed (:libswoc:`svtoi`) and unsigned (:libswoc:`svtou`) flavors.
These functions are designed to be "complete" in the sense that any other string to integer conversion
can be mapped to one of these functions.

The standard functions :code:`strcmp`, :code:`strcasecmp`, and :code:`memcmp` are overloaded when
at least of the parameters is a |TV|. The length is taken from the view, rather than being an explicit
parameter as with :code:`strncasecmp`.

When no other useful result can be returned, |TV| methods return a reference to the instance. This
makes chaining methods easy. If a list consisted of colon separated elements, each of which was
of the form "A.B.old" and just the "A.B" part was needed, sans leading white space:

.. literalinclude:: ../../unit_tests/ex_TextView.cc
   :lines: 223-227

Parsing with TextView
=====================

Time for some examples demonstrating string parsing using |TV|. One of the goals of the design of |TV|
was to minimize the need to allocate memory to hold intermediate results. For this reason, the normal
style of use is a streaming / incremental one, where tokens are extracted from a source one by one
and placed in |TV| instances, with the orignal source |TV| being reduced by each extraction until
it is empty.

CSV Example
-----------

For example, assume :arg:`value` contains a null terminated string which is possibly several tokens
separated by commas.

.. literalinclude:: ../../unit_tests/ex_TextView.cc
   :lines: 26,40-50

.. sidebar:: Verification

   `Test code for example <https://github.com/SolidWallOfCode/libswoc/blob/1.2.4/unit_tests/ex_TextView.cc#L67>`__.

If :arg:`value` was :literal:`bob  ,dave, sam` then :arg:`token` would be successively
:literal:`bob`, :literal:`dave`, :literal:`sam`. After :literal:`sam` was extracted :arg:`value`
would be empty and the loop would exit. :arg:`token` can be empty in the case of adjacent commas, a
trailing comma, or commas separated only by whitespasce. Note no memory allocation is done because
each view is a pointer in to :arg:`value`. Because the size is contained in the view there is no
need to put nul characters in the source string as would be done by :code:`strtok` and therefore
this can be used on constant source strings.

Key / Value Example
-------------------

A similar case is parsing a list of key / value pairs in a comma separated list. Each pair is
"key=value" where white space is ignored. In this case it is also permitted to have just a keyword
for values that are boolean.

.. literalinclude:: ../../unit_tests/ex_TextView.cc
   :lines: 26,52-62

.. sidebar:: Verification

   `Test code for example <https://github.com/SolidWallOfCode/libswoc/blob/1.2.4/unit_tests/ex_TextView.cc#L74>`__.

The basic list processing is the same as the previous example, with each element being treated as
a "list" with ``=`` as the separator. Note if there is no ``=`` character then all of the list
element is moved to :arg:`key` leaving :arg:`value` empty, which is the desired result. A bit of
extra white space trimming it done in case there was space between the key and the ``=``.

Line Processing
---------------

|TV| works well when parsing lines from a file. For this example, :libswoc:`load` will
be used. This method, given a path, loads the entire content of the file into a :code:`std::string`.
This will serve as the owner of the string memory. If it is kept around with the configuration, all
of the parsed strings can be instances of |TV| that reference memory in that :code:`std::string`. If
the density of useful text is sufficiently high, this is a convenient way to handle parsing with
minimal memory allocations.

This example counts the number of code lines in the documenations ``conf.py`` file.

.. literalinclude:: ../../unit_tests/ex_TextView.cc
   :lines: 203-217

The |TV| :arg:`src` is constructed from the :code:`std::string` :arg:`content` which contains the
file contents. While that view is not empty, a line is taken each look and leading and trailing
whitespace is trimmed. If this results in an empty view or one where the first character is the
Python comment character ``#`` it is not counted. The newlines are discard by the prefix extraction.
The use of :libswoc:`TextView::take_prefix_at` forces the extraction of text even if there is no
final newline. If this were a file of key value pairs, then :arg:`line` would be subjected to one of
the other examples to extract the values. For all of this, there is only one memory allocation, that
needed for :arg:`content` to load the file contents.

Entity Tag Lists Example
------------------------

An example from actual production code is this example that parses a quoted, comma separated list of
values ("CSV"). This is used for parsing `entity tags
<https://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.11>`__ as used for HTTP fields such as
"If-Match" (`14.24 <https://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html>`__). This will be a CSV
each where each value is quoted. To make it interesting these quoted strings may contain commas,
which do not count as separators. Therefore the simple approach in previous examples will not work
in all cases. This example also does not use the callback style of the previous examples - instead
the tokens are pulled off in a streaming style with the source :code:`TextView` being passed by
reference in order to be updated by the tokenizer. Further, some callers want the quotes, and some
do not, so a flag to strip quotes from the resulting elements is needed. The final result looks like

.. literalinclude:: ../../unit_tests/ex_TextView.cc
   :start-after: "TextView Tokens"
   :lines: 2-26

.. sidebar:: Verification

   `Test code for example <https://github.com/SolidWallOfCode/libswoc/blob/1.2.4/unit_tests/ex_TextView.cc#L90>`__.

This takes a :code:`TextView&` which is the source view which will be updated as tokens are removed
(therefore the caller must do the empty view check). The other arguments are the separator character
and the "strip quotes" flag. The algorithm is to find the next "interesting" character, which is either
a separator or a quote. Quotes flip the "in quote" flag back and forth, and separators terminate
the loop if the "in quote" flag is not set. This skips quoted separators. If neither is found then
all of the view is returned as the result. Whitespace is always trimmed and then quotes are trimmed
if requested, before the view is returned. In this case keeping an offset of the amount of the source
view processed is the most convenient mechanism for tracking progress. The result is a fairly compact
piece of code that does non-trivial parsing and conversion on a source string, without a lot of
complex parsing state, and no memory allocation.

History
*******

The first attempt at this functionality was in the TSConfig library in the :code:`ts::Buffer` and
:code:`ts::ConstBuffer` classes. Originally intended just as raw memory views,
:code:`ts::ConstBuffer` in particular was repeatedly enhanced to provide better support for strings.
The header was eventually moved from :literal:`lib/tsconfig` to :literal:`lib/ts` and was used in in
various part of the Traffic Server core.

There was then a proposal to make these classes available to plugin writers as they proved handy in
the core. A suggested alternative was `Boost.StringRef
<http://www.boost.org/doc/libs/1_61_0/libs/utility/doc/html/string_ref.html>`_ which provides a
similar functionality using :code:`std::string` as the base of the pre-allocated memory. A version
of the header was ported to Traffic Server (by stripping all the Boost support and cross includes) but in use
proved to provide little of the functionality available in :code:`ts::ConstBuffer`. If extensive
reworking was required in any case, it seemed better to start from scratch and build just what was
useful in the Traffic Server context.

The next step was the :code:`TextView` class which turned out reasonably well. About this time
:code:`std::string_view` was officially adopted for C++17, which was a bit of a problem because
:code:`TextView` was extremely similar in functionality but quite different in interface. Further,
it had a number of quite useful methods that were not in :code:`std::string_view`. To simplify the
use of :code:`TextView` (which was actually called "StringView" then) it was made a subclass of
:code:`std::string_view` with user defined conversions so that two classes could be used almost
interchangeable in an efficient way. Passing a :code:`TextView` to a :code:`std::string_view
const&` is zero marginal cost because of inheritance and passing by value is also no more expensive
than just :code:`std::string_view`.

.. rubric:: Footnotes

.. [#] This is a horrible hash function, do not actually use it.

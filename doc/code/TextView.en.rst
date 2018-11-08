.. Licensed to the Apache Software Foundation (ASF) under one
   or more contributor license agreements.  See the NOTICE file
   distributed with this work for additional information
   regarding copyright ownership.  The ASF licenses this file
   to you under the Apache License, Version 2.0 (the
   "License"); you may not use this file except in compliance
   with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing,
   software distributed under the License is distributed on an
   "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
   KIND, either express or implied.  See the License for the
   specific language governing permissions and limitations
   under the License.

.. include:: ../common.defs

.. default-domain:: cpp
.. highlight:: cpp

TextView
*************

Synopsis
========

:code:`#include <swoc/TextView.h>`

.. class:: TextView

This class acts as a view in to memory allocated / owned elsewhere. It is in effect a pointer and
should be treated as such (e.g. care must be taken to avoid dangling references by knowing where the
memory really is). The purpose is to provide string manipulation that is faster and safer than
raw pointers or duplicating strings.

Description
===========

:class:`TextView` is a subclass of :code:`std::string_view` and threfore has those methods. On top
of that base it provides a number of ancillary methods to support commonly performed string
manipulations.

A :class:`TextView` should be treated as an enhanced character pointer that both a location and a
size. This is what makes it possible to pass substrings around without having to make copies or
allocate additional memory. This comes at the cost of keeping track of the actual owner of the
string memory and making sure the :class:`TextView` does not outlive the memory owner. This is
identical to memory issues with a raw pointer and it is best to treat a :class:`TextView` the same
way as a pointer. Any place that passes a :code:`char *` and a size is an excellent candidate for
using a :class:`TextView` as it is more convenient and no more risky or expensive than the existing
arguments.

In deciding between :code:`std::string_view` and :class:`TextView` remember these easily and cheaply
cross convert. In general if the string is treated as a block of data, :code:`std::string_view` is
better. If the contents of the string are to be examined / parsed then :class:`TextView` is better.
For example, if the string is used simply as a key or a hash source, use :code:`std::string_view`.
Contrariwise if the string may contain substrings of interest such as key / value pairs, then use a
:class:`TextView`.

When passing :class:`TextView` as an argument, it is very debatable whether passing by value or
passing by reference is more efficient, therefore it's not likely to matter in production code. My
personal heuristic is whether the function will modify the value. If so, passing by value saves a
copy to a local variable therefore it should be passed by value. If the function simply passes the
:class:`TextView` on to other functions, then pass by constant reference. This distinction is
irrelevant to the caller, the same code at the call site will work in either case.

:class:`TextView` provides a variety of methods for manipulating the view. This can seem complex
but they are distinguished by distinct categories of function so that for any particular use there
is a clearly best choice.

The primary distinction is how an character in the view is selected.

* Index, an offset in to the view.

* Comparison, either a single character or set of characters which is matched against a single character in the view.

* Predicate, a function that takes a single character argument and returns a bool to indicate a match.

A secondary distinction is what is done by modifying methods if the selected character is not found.

* The "split" methods do nothing - if the target character is not found, the view is not modified.

* The "take" methods always do something - if the target character is not found, the entire view is used.

Both of these cases are useful in different circumstances.

As noted, :class:`TextView` is designed as a pointer style class. Therefore it has an increment
operator which is equivalent to :func:`TextView::remove_prefix`, and a dereference operator, which
act the same way as on a pointer. The difference is the view knows where the end of the view is.
This provides a comfortably familiar way of iterating through a view, the main difference being
checking the view itself rather than a dereference of it (like a C-style string) or a range limit.

.. code-block:: cpp

   void hasher(TextView v) {
      size_t hash = 0;
      while (v) {
         hash = hash * 13 + * v ++;
      }
      return hash;
   }

:class:`TextView` can also act as a container, which means range for loops work as expected.

.. code-block:: cpp

   void hasher(TextView const& v) {
      size_t hash = 0;
      for (char c : v) hash = hash * 13 + c;
      return hash;
   }

Views are very cheap to construct therefore making a copy is a neglible expense. For this reason
if it is necessary to remember places or parts of a view, it is better to create a :class:`TextView`
instance that holds the location or substring rather than storing offsets or pointers.

Parsing with TextView
=====================

A primary use of :class:`TextView` is to do string parsing. It is easy and fast to split
strings in to tokens without modifying the original data.

CSV Example
-----------

For example, assume :arg:`value` contains a null terminated string which is possibly several tokens
separated by commas.

.. literalinclude:: ../../src/unit_tests/ex_TextView.cc
   :lines: 26,40-50

.. sidebar:: Verification

   `Test code for example <https://github.com/SolidWallOfCode/libswoc/blob/master/src/unit_tests/ex_TextView.cc#L66>`__.

If :arg:`value` was :literal:`bob  ,dave, sam` then :arg:`token` would be successively
:literal:`bob`, :literal:`dave, :literal:`sam`. After :literal:`sam` was extracted :arg:`value`
would be empty and the loop would exit. :arg:`token` can be empty in the case of adjacent commas, a
trailing comma, or commas separated only by whitespasce. Note that no memory allocation is done
because each view is a pointer in to :arg:`value`. Because the size is contained in the view there
is no need to put nul characters in the source string as would be done by :code:`strtok` and
therefore this can be used on constant source strings.

Key / Value Example
-------------------

A similar case is parsing a list of key / value pairs in a comma separated list. Each pair is
"key=value" where white space is ignored. In this case it is also permitted to have just a keyword
for values that are boolean.

.. literalinclude:: ../../src/unit_tests/ex_TextView.cc
   :lines: 26,52-62

.. sidebar:: Verification

   `Test code for example <https://github.com/SolidWallOfCode/libswoc/blob/master/src/unit_tests/ex_TextView.cc#L73>`__.

The basic list processing is the same as the previous example, with each element being treated as
a "list" with ``=`` as the separator. Note if there is no ``=`` character then all of the list
element is moved to :arg:`key` leaving :arg:`value` empty, which is the desired result. A bit of
extra white space trimming it done in case there was space between the key and the ``=``.

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

.. literalinclude:: ../../src/unit_tests/ex_TextView.cc
   :lines: 91-123

.. sidebar:: Verification

   `Test code for example <https://github.com/SolidWallOfCode/libswoc/blob/master/src/unit_tests/ex_TextView.cc#L125>`__.

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
=======

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

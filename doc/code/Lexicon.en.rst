.. Licensed to the Apache Software Foundation (ASF) under one
   or more contributor license agreements. See the NOTICE file distributed with this work for
   additional information regarding copyright ownership. The ASF licenses this file to you under the
   Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
   the License. You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software distributed under the License
   is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
   or implied. See the License for the specific language governing permissions and limitations under
   the License.

.. include:: ../common-defs.rst

.. _swoc-lexicon:
.. highlight:: cpp
.. default-domain:: cpp
.. |Lexicon| replace:: :code:`Lexicon`

****************
Lexicon
****************

|Lexicon| is a bidirectional mapping between strings and a numeric / enumeration type. It is intended
to support parsing and diagnostics for enumerations. It has some significant advantages over a simple
array of strings.

*  The integer can be looked up by string. This makes parsing much easier and more robust.
*  The integers do not have to be contiguous or zero based.
*  Multiple names can map to the same integer.
*  Defaults for missing names or integers.

Definition
**********

.. class:: template < typename E > Lexicon

   :libswoc:`Reference documentation <Lexicon>`.

Usage
*****

Lexicons can be used in a dynamic or static fashion. The basic use is as a static translation object
that converts between an enumeration and names. The constructors allow setting up the entire Lexicon.

The primary things to set up for a Lexicon are

*  The equivalence of names and values.
*  The default (if any) for a name.
*  The default (if any) for a value.

Values and names can be associated either using pairs of values and names, or a pair of a value
and a list of names, the first of which is the primary name. This must be consistent for all of
the defined values, so if one value has multiple names, all names must use the value, name list form.

In addition, defaults can be specified. Because all possible defaults have distinct signatures
there is no need to order them - the constructor can deduce what is meant. Defaults are very handy
when using a Lexicon for parsing - the default value can be an invalid value, in which case checking
an input token for being a valid name is very simple ::

   extern swoc::Lexicon<Types> lex; // Initialized elsewhere.
   auto value = lex[token];
   if (value != INVALID) { // handle successful parse }

Lexicon can also be used dynamically where the contents are built up over time or due to run time
inputs. One example is using Lexion to support enumeration or flag set columns for :ref:`ip-space`.
A configuration file can list the allowed / supported keys for the columns, which are then loaded
into a Lexicon and use to parse the data file. The key methods are

*  :libswoc:`Lexicon::define` which adds a value, name definition.
*  :libswoc:`Lexicon::set_default` which sets a default.

Each Lexicon has its own internal storage where copies of all of the strings are kept. This makes
dynamic use much easier and robust as there are no lifetime concerns with the strings.

Lexicons can be used for "normalizing" pointers to strings. Double indexing will convert the
arbitrary pointer to the string to a consistent pointer, which can then be numerically compared for
equivalence. This is only a benefit if the pointer is to be stored and compared multiple times. ::

   token = lex[lex[token]]; // Normalize string pointer.

Examples
========

For illustrative purposes, consider using :ref:`ip-space` where each address has a set of flags
representing the type of address, such as production, edge, secure, etc. This is stored in memory
as a ``std::bitset``. To load up the data a comma separated value file is provided which has the
first column as the IP address range and the subsequent values are flag names.

The starting point is an enumeration with the address types:

.. literalinclude:: ../../unit_tests/ex_Lexicon.cc
   :start-after: doc.1.begin
   :end-before: doc.1.end

To do conversions a Lexicon is created:

.. literalinclude:: ../../unit_tests/ex_Lexicon.cc
   :start-after: doc.2.begin
   :end-before: doc.2.end

The file loading and parsing is then:

.. literalinclude:: ../../unit_tests/ex_Lexicon.cc
   :start-after: doc.load.begin
   :end-before: doc.load.end

This uses the Lexicon to convert the strings in the file to the enumeration values, which are the
bitset indices. The defalt is set to ``INVALID`` so that any string that doesn't match a string
in the Lexicon is mapped to ``INVALID``.

Once the IP Space is loaded, lookup is simple, given an address:

.. literalinclude:: ../../unit_tests/ex_Lexicon.cc
   :start-after: doc.lookup.begin
   :end-before: doc.lookup.end

At this point ``flags`` has the set of flags stored for that address from the original data. Data
can be accessed like ::

   if (flags[NetType::PROD]) { ... }

Design Notes
************

Lexicon was designed to solve a common problem I had with converting between enumerations and
strings. Simple arrays were, as noted in the introduction, were not adequate, particularly for
parsing. There was also some influence from internationalization efforts where the Lexicon could be
loaded with other languages. Secondary names have proven useful for parsing, allowing easy aliases
for the enumeration (e.g., for ``true`` for a boolean the names can be a list like "yes", "1",
"enable", etc.)

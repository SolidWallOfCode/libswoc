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

.. _Scalar:

.. highlight:: cpp

.. default-domain:: cpp

Scalar
******

.. code-block:: cpp

   #include <swoc/Scalar.h>

.. class:: template < intmax_t SCALE, typename COUNTER = int, typename TAG = tag::generic > Scalar

   :tparam SCALE: Scaling factor.
   :tparam COUNTER: Storage for counter.
   :tparam TAG: Type distinguishing tag.

   :libswoc:`Reference documentation <swoc::Scalar>`.

   A quantized integral with a distinct type.

   The scaling factor :arg:`SCALE` must be an positive integer. The value of an instance will always
   be an integral multiple of :arg:`SCALE`.

   :arg:`COUNTER` must be an integral type. An instance of this type is used to hold the internal
   count. It can be omitted and will default to :expr:`int`. The size of an instance is the same size
   as :arg:`COUNTER` and an instance of that type can be replaced with a :class:`Scalar` of that size
   without changing the memory layout.

   :arg:`TAG` must be a type. It is used as a mechanism for preventing accidental cross assignments.
   Assignment of any sort from a :class:`Scalar` instance to another instance with a different
   :arg:`TAG` is a compile time error. If this isn't useful :arg:`TAG` can be omitted and will
   default to :expr:`tag::generic` which will enable all instances to interoperate.

   The type used for :arg:`TAG` can be defined in name only.::

      struct YoureIt; // no other information about YoureIt is required.
      using HectoTouch = Scalar<100, int, YoureIt>; // how many hundreds of touches.

Scalar is a header only library that provides scaled and typed numerical values. This can be used to
create data types that are restricted to being multiples of integral value, or for creating types
that act like integers but are a distinct type (thus making overloads and argument ordering simpler
and more robust). Generally using Scalar starts with defining types which have a *scale factor* and
optionally a *tag*. Values in an instance of Scalar are always multiples of the scale factor.

The tag is used to create categories of related types, the same underlying "metric" at different
scales. To enforce this Scalar does not allow assignment between instances with different tags. If
this is not important the tag can be omitted and a default generic one will be used, thereby
allowing arbitrary assignments.

.. namespace:: tag

.. struct:: generic

   A struct defined in name only that is used as the default tag for a :code:`Scalar`.

.. namespace:: NULL

Scalar is designed to be fast and efficient. When converting bewteen similar types with different
scales it will do the minimum amount of work while minimizing the risk of integer overflow. Common
factor elimination is done at compile time so that scalars which are multiples of each other do a
single multiple or divide to scale. Instances have the same memory footprint as the underlying
integer storage type. It is intended to replace lengthy and error prone hand optimizations used to
handle related values of different scales.

Usage
******

In normal use a scalar evaluates to its value rather than its count. The goal is to provide an
instance that appears to store unscaled values in a quantized way. The count is accessible if
needed.

Assignment
==========

Assigning values to, from, and between :libswoc:`Scalar` instances is usually
straightforward with a few simple rules. This is modeled on pointer arithmetic and as much as
possible follows the same rules.

*  :libswoc:`Scalar::assign` is used to directly assign a count.
*  The increment and decrement operators, and the :code:`inc` and :code:`dec` methods, operate on the count.
*  All other contexts use the value.
*  The assignment operator will preserve the value by scaling based on the scale of the source and destination.
   If this can not be done without loss, otherwise it is a compile error.
*  Untyped integer values are treated as having a :arg:`SCALE` of 1.

If the assignment of one scalar to another is not lossless (e.g. the left hand side of the
assignment has a large scale than the right hand side), direct assignment will not compile. Instead
one of the two following free functions must be used to indicate how to handle the loss.

.. function:: unspecified_type round_up(Scalar v)

   Return a wrapper that indicates :arg:`v` should be rounded up as needed.

.. function:: unspecified_type round_down(Scalar v)

   Return a wrapper that indicates :arg:`v` should be rounded down as needed.

To illustrate, suppose there were the definitions

   using deka = Scalar<10>;
   using = Scalar<100>;

An assignment of a :code:`hecto` to a :code:`deka` is implicit as the scaling is lossless.

.. code-block:: cpp

   hecto a(17);
   deka b;
   b = a; // compiles.

The opposite is not implicit because the value of a :code:`deka` can be one not representable by a
:code:`hecto`. In such a case it would have to be rounded, either up or down.

.. code-block:: cpp

   b.assign(143); // b gets the value 1430
   a = b; // compile error
   a = round_up(b); // a will be updated to have a count of 15 and value of 1500

:expr:`round_up` and :expr:`round_down` can also be used with basic integers.

.. function:: unspecified_type round_down(intmax_t)

.. function:: unspecified_type round_up(intmax_t)

Note this is very different from using :libswoc:`Scalar::assign`. The latter sets the *count* of
the scalar instance. :expr:`round_up` and :expr:`round_down` set the *value* of the scalar, dividing
the provided value by the scale to set the count to make the value match the assignment as closesly
as possible.

.. code-block:: cpp

   a = round_down(2480); // a has count 24, value 2400.
   a.assign(2480); // a has a count of 2480, value 248,000.

Arithmetic
==========

Arithmetic with scalars is based on the idea that a scalar represents its value. An instance retains
the scalar type for conversion checking but otherwise acts as the value. This makes using scalar
instances as integral arguments to other functions simple. For instance consider following the
definition.

.. code-block:: cpp

   struct SerializedData { ...};
   using Sector = Scalar<512>;

To allocate a buffer large enough for a :code:`SerializedData` that is also a multiple of a sector
would be

.. code-block:: cpp

   Sector n = round_up(sizeof(serialized_data));
   void* buffer = malloc(n);

Or more directly

.. code-block:: cpp

   void* buffer = malloc(Sector(round_up(sizeof(serialized_data))));

Scalar is designed to be easy to use but when using multiple scales simultaneously, especially in
the same expression, the computed type can be surprising. The best approach is to be explicit - a
Scalar instance is very inexpensive to create (at most 1 integer copy) therefore subexpressions can
easily be forced to a specific scale by constructing the appropriate scalar with :expr:`round_up` or
:expr:`round_down` of the subexpression. Or, define a unit scale type and convert to that as the
common type before converting the result to the desired scale.

Advanced Features
=================

Scalar has a few advanced features which are not usually needed and for which usable defaults are
provided. This is not always the case and therefore access to the machinery is provided.

I/O
---

When a scalar is printed it prints out as its value, not count. For a family of scalars it can be
desireable to have the type printed along with the value. This can be done by adding a member named
:literal:`label` to the *tag* type of the scalar. If the :literal:`label` member can be provided to
an I/O stream then it will be after the value of the scalar. Otherwise it is ignored. An example can
be found in the <Bytes>_ section of the example usage.

Examples
========

The expected common use of Scalar is to create a family of scalars representing the same
underlying unit of measure, differing only in scale. The standard example of this is computer memory
sizezs which have this property.

Bytes
-----

The initial use of :libswoc:`Scalar` will be in the cache component. This has already been tested in
some experimental work which will in time be blended back in to the main codebase. The use will be
to represent different amounts of data, in memory and on disk.

.. code-block:: cpp

   namespace tag { struct bytes { static const char * label = " bytes"; }; }
   using Bytes = Scalar<1, off_t, tag::bytes>;
   using CacheDiskBlocks = Scalar<512, off_t, tag::bytes>;
   using KB = Scalar<1024, off_t, tag::byteds>;
   using CacheStoreBlocks = Scalar<8 * KB::SCALE, off_t, tag::bytes>;
   using MB = Scalar<1024 * KB::SCALE, off_t, tag::bytes>;
   using CacheStripeBlocks = Scalar<128 * MB::SCALE, off_t, tag::bytes>;
   using GB = Scalar<1024 * MB::SCALE, off_t, tag::bytes>;
   using TB = Scalar<1024 * GB::SCALE, off_t, tag::bytes>;

This collection of types represents the data size units of interest to the cache and therefore
enables to code to be much clearer about the units and to avoid errors in converting from one to
another.

A common task is to add sizes together and round up to a multiple of some fixed size. One example is
the stripe header data, which is stored as a multiple of 8192 bytes, that is a number of
:code:`CacheStripeBlocks`. That can be done with

.. code-block:: cpp

   header->len = round_up(sizeof(CacheStripMeta) + segment_count * sizeof(SegmentFreeListHead));

vs. the original code with magic constants and the hope that the value is scaled as you think it is.

.. code-block:: cpp

   header->len = (((sizeof(CacheStripeMeta) + header->segments * sizeof(SegmentFreeListHead)) >> STORE_BLOCK_SHIFT) << STORE_BLOCK_SHIFT)

Esoteric Uses
--------------

In theory Scalar types can be useful even with only temporaries. For instance, to round to the
nearest 100.

.. code-block:: cpp

   int round_100(int y) { return Scalar<100>(round_up(y)); }

This could also be done in line. However, when I tried to use this actual practice it was a bit
cumbersome and not clearly better than the usual :code:`#define`. Therefore overloads of
:libswoc:`round_up(C value)` and :libswoc:`round_down(C value)` are provided to do this boilerplace
automatically. Both of these take a numeric template parameter, which is the scale, and a run time
argument of a value, which is rounded to a multiple of the scale, as with a Scalar type. With these
overloads, rounding is fast and simple.

.. literalinclude:: ../../unit_tests/test_Scalar.cc
   :lines: 76-82

Note these overloads return a Scalar type, which converts to its effective value when used in a
non Scalar context.

Design Notes
============

The semantics of arithmetic were the most vexing issue in building this library. The ultimate
problem is that addition to the count and to the value are both reasonable and common operations and
different users may well have different expectations of which is more "natural" when operating with
a scalar and a raw numeric value. In practice my conclusion was that even before feeling "natural"
the library should avoid surprise. Therefore the ambiguity of arithmetic with non-scalars was
avoided by not permitting those operations, even though it can mean a bit more work on the part of
the library user. The increment / decrement and compound assignment operators were judged sufficient
similar to pointer arithmetic to be unsurprising in this context. This was further influenced by the
fact that, in general, these operators are useless in the value context. E.g. if a scalar has a
scale greater than 1 (the common case) then increment and decrement of the value is always a null
operation. Once those operators are used on the count is is least surprising that the compound
operators act in the same way. The next step, to arithmetic operators, is not so clear and so those
require explicit scale indicators, such as :expr:`round_down` or explicit constructors. It was a
design goal to avoid, as much as possible, the requirement that the library user keep track of the
scale of specific variables. This has proved very useful in practice, but at the same time when
doing arithmentic is is almost always the case that either the values are both scalars (making the
arithmetic unambiguous) or the scale of the literal is known (e.g., "add 6 kilobytes").

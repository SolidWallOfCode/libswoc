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
.. highlight:: cpp
.. default-domain:: cpp
.. |MemSpan| replace:: :code:`MemSpan`

.. _swoc-mem-span:

********
MemSpan
********

Synopsis
********

:code:`#include "swoc/MemSpan.h"`

.. class:: template < typename T > MemSpan

   :libswoc:`Reference documentation <MemSpan>`.

|MemSpan| is a view on *writable* memory. This distinguishes it from classes like :code:`TextView`.
The purpose of the class is to provide a compact description of a contiguous writeable block of
memory, carrying both location and size information to (hopefully) prevent these from becoming
separated. A |MemSpan| is a view because it never owns memory. It should be treated as a smart pointer that
carries size information. As text views are intended to replace passing a character pointer and size
separately, a memory span is intended to replace passing a pointer and a size as separate arguments.

|MemSpan| has two styles. The "void" style treats the memory as purely a block of memory without
internal structure. The "typed" style treats the memory as an array of a specific type. These
can be inter-converted, in the equivalent of type casting. The rules about
conversions between span types is modeled on pointer conversions. Therefore a typed memory span
implicitly converts to a void span, but a void span requires an explicit cast or rebinding to
convert to a typed memory span.

As with pointers, a constant span can refer to writable memory and
a non-constant span can refer to writable memory. Whether the memory referenced by the
span is writable depends on whether the type of the span is a constant type. E.g. :code:`MemSpan<char>` refers
to writable memory and :code:`MemSpan<char const>` does not.

Usage
*****

A primary use of |MemSpan| is to generalize the type of a block of memory. For instance a function parameter
could be :code:`std::vector<alpha>` for some type :code:`alpha`. Calling the function, however, requires
the creation of a vector. This can require an unneeded memory allocation.

The other primary use is to work with chunks of memory embedded in larger containers. For instance in the
previous example, not only is creating another vector needed, but it also makes working with
subspans very difficult. Either copying is required or it can be worked around with various
additional parameters, or by reverting to the
most primitve and passing in a raw pointer and a count. It is much easier to have a |MemSpan| parameter type
which handles all of these cases with minimal complexity. E.g. a function like ::

   int f(MemSpan<char> source) { ... }

can be passed data from a vector, an array, a raw pointer and size, stack memory, etc., including types not
defined in the scope of the function.

Void Span
=========

A :code:`MemSpan<void>` describes an undifferentiated contiguous block of memory. It can be
constructed from either a pointer and length :libswoc:`MemSpan\< void >::MemSpan(value_arg *,
size_t)` or a half open range of two pointers :libswoc:`MemSpan\< void >::MemSpan(value_arg *, value_arg * )`.
A default constructed instance, or one constructed from :code:`nullptr` has no
memory and zero size.

The implementation differentiates between :code:`void` and :code:`void const` in order to be const correct.
A :code:`void` span implicitly converts to a :code:`void const` span but not the other way as is the case for
:code:`void*` and :code:`void const*` pointers. A :code:`void const` span is the "universal" span - all other
span types implicitly convert to this type.

The memory described by the instance can be obtained with the
data method :libswoc:`MemSpan\< void \>::data` and the size with the
size method :libswoc:`MemSpan\< void \>::size`.

Typed Span
==========

A typed |MemSpan| treats the described memory as an array of the type. E.g., :code:`MemSpan<int>`
treats its memory as an array of :code:`int`, with a :libswoc:`subscript operator <MemSpan::operator[]>`.
The construction is the same as with the :code:`void` case but the pointers must be of the
span type. As with pointers and arrays, the count is the number of instances of :code:`T` not the size in
bytes.

Rebinding
=========

A new |MemSpan| of a different type can be created from an existing instance. This is is called
"rebinding" and uses the templated :libswoc:`rebind method <MemSpan::rebind>`. This is roughly
equivalent to type casting in that the original instance is unchanged. A new instance is created
that refers to the same memory but with different type information. As an example,
:libswoc:`MemArena::alloc` returns a :code:`MemSpan<void>`. If this is to be used for string data,
then the call could look like ::

   auto span = arena.alloc(n).rebind<char>();

This would make :code:`span` of type :code:`MemSpan<char>` covering the same memory returned from
:code:`alloc`.

Rebinding requires the size of the memory block to be an integral multiple of the size of the target
type. That is, a binding that creates a partial object at the end of the array is forbidden.

Alignment
=========

Rebinding on void spans can be done along with alignment using the :libswoc:`align method
<MemSpan::align>`. This can be done with a templated method for a type, or with an explicit alignment
argument of type :code:`size_t`. It does not fail due to partial objects. Instead enough space is
discarded from the start of memory such that the returned span has the alignment required for
:code:`T` and memory space that would yield a partial object is discarded from the end. The result is
the largest subspan that is memory aligned and has space for an integral number of instances.

Conversions
===========

Memory spans will implicitly convert in a manner equivalent to raw pointers.

*  Non-const spans implicitly convert to constant spans of the same type.
*  Non-const spans implicitly convert to :code:`MemSpan<void>`.
*  Any span will implicitly convert to :code:`MemSpan<void const>`.

Construction
============

The most common style of construction is to provide a pointer and size / count. For void spans this is the
number of bytes, while typed spans take a count as with arrays. E.g. for some type :code:`Alpha` ::

   Alpha array[6];

the corresponding span would be ::

   MemSpan<Alpha> span(array, 6);

On the other hand the constructors understand arrays so this works as well ::

   MemSpan<Alpha> span(array);

There is constructor support for any contiguous memory container that supports the :code:`data`
and :code:`size` methods. A span can be contructed from :code:`std::string`, :code:`std::string_view`,
or :code:`std::vector` by passing the container. Internally this ::

   std::vector<Alpha> av;
   MemSpan<Alpha> aspan{av};

is treated as

   std::vector<Alpha> av;
   MemSpan<Alpha> aspan{av.data(), av.size};

There are template deduction guides for :code:`std::array`, :code:`std::vector`, :code:`std::string`,
and :code:`std::string_view` so that the previous could also be done as ::

   std::vector<Alpha> av;
   MemSpan aspan{av};



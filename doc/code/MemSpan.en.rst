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
separated.

|MemSpan| has two styles. The "void" style treats the memory as purely a block of memory without
internal structure. The "typed" style treats the memory as an array of a specific type. These
can be inter-converted, in the equivalent of type casting.

Usage
*****

|MemSpan| is a templated class with special handling for :code:`void`. This special case will be
described first, and then the typed cases.

Void Span
=========

A :code:`MemSpan<void>` describes an undifferentiated contiguous block of memory. It can be
constructed from either a pointer and length :libswoc:`MemSpan\< void >::MemSpan(value_type *,
size_t)` or a half open range of two pointers :libswoc:`MemSpan\< void >::MemSpan(value_type *, value_type * )`.
A default constructed instance, or one constructed from :code:`nullptr` has no
memory and zero size.

The memory described by the instance can be obtained with the
data method :libswoc:`MemSpan\< void \>::data` and the size with the
size method :libswoc:`MemSpan\< void \>::size`.

Typed Span
==========

A typed |MemSpan| treats the described memory as an array of the type. E.g., :code:`MemSpan<int>`
treats its memory as an array of :code:`int`, with a :libswoc:`subscript operator <MemSpan::operator[]>`.
The construction is the same as with the :code:`void` case but the pointers must be of the
span type.

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

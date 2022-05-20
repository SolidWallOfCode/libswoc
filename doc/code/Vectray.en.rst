.. SPDX-License-Identifier: Apache-2.0
   Copyright Apache Software Foundation 2019

.. include:: ../common-defs.rst
.. highlight:: cpp
.. default-domain:: cpp
.. |V| replace:: :code:`Vectray`

.. _swoc-vectray:

********
Vectray
********

Synopsis
********

:code:`#include "swoc/Vectray.h"`

.. class:: template < typename T, size_t N, class Allocator > Vectray

   :libswoc:`Reference documentation <Vectray>`.

|V| is a class intended to replace :code:`std::vector` in situations where performance is critical.
An instance of |V| contains a static array of size :arg:`N` which is used in preference to allocating
memory. If the number of instances is generally less than :arg:`N` then no memory allocation /
deallocation is done and the performance is as fast as a :code:`std::array`. Unlike an array, if
the required memory exceeds the static limits the internal storage is changed to a :code:`std::vector`
without data loss. Another key difference is the number of valid elements in the container can vary.

Performance gain from using this class depends upon

*  The static limit :arg:`N` being relatively small to minimize fixed costs.
*  Required storage usually fits within the static limits.

If allocation is commonly needed this will be slower than a :code:`std::vector` because of
the additional cost of copying from static to dynamic memory.

The most common use case is for arrays that are usually only 1 or 2 elements (such as options
for a parameter) and only rarely longer. |V| can then significantly reduce memory churn at small
cost.

The second common use case is for stack buffers of moderate size, such as logging buffers. Generally
these are selected to be large to enough to cover most cases, except for the occasional truncation.
In this case |V| preserves the performance of the common case while providing an escape in the
rare circumstance of exceeding the buffer size.

As always, performance tuning is an art, not a science. Do not simply assume |V| is a better choice
and use it to replace `std::vector` or `std::array` in general. Use where it looks helpful and do
measurements to verify.

Usage
*****

|V| acts as a combination of :code:`std::vector` and :code:`std::aray` and is declared like the latter.
For an instance that contains a single static element of type :code:`std::string` ::

   swoc::Vectray<std::string, 1> strings;

The static elements are not default constructed, but are constructed as needed.

Allocator
=========

Generally this should be defaulted, but is exposed so a polymorphic memory resource based
allocator can be used when needed.

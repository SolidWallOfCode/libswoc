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

.. _meta:

.. default-domain:: cpp

Meta
****

The :swoc:git:`include/swoc/swoc_meta.h` header provides some basic C++ meta-programming support that is used elsewhere in the code. Specific utilities are described below.

Meta Case
=========

The meta case utility is designed to help resolve ambiguity issues in function template instantiation. When overloading
functions, it is not generally a challenge to make them unambiguous because the types are explicit. When creating sets
of templated overloads, this can be more challenging because the templates can easily be instantiated in unanticipated
ways. Some cases can be handled with :code:`enable_if` to suppress unwanted instantiations for specific types.

Meta case supports creating an ordering of template function instantiations such that even if more than one instantiation is valid, only one is unambiguously chosen. This is done by providing a base function and a set of templated overloads. Each overload takes an extra argument which is one of the meta case types, :code:`swoc::meta::CaseTag<N>` for numeric values of :code:`N` from one to a compile time maximum (currently 9). Higher values of :code:`N` make that function overload higher priority. The base function then forwards its arguments, plus the special argument :code:`swoc::meta::CaseArg`. For instance, if the base function was::

    template < typename T > int base(T * data, size_t n);

then the lowest priority overload would be of the form::

    template < typename T > auto base(T * data, size_t n, swoc::meta::CaseTag<0>) -> decltype(/* test_expr_0 */, int());

and the next best choice would be::

    template < typename T > auto base(T * data, size_t n, swoc::meta::CaseTag<1>) -> decltype(/* test_expr_1 */, int());

and so on for each case of interest. The base function would be::

    template < typename T > int base(T * data, size_t n) { return base(data, n, swoc::meta::CaseArg); }

The purpose of the :code:`decltype` is to an expression for being compilable. That is, the particular case will be
instantiated if and only if the corresponding :code:`test_expr` compiles. Of the cases that instatiate, the one with the
highest priority tag will be called.

The test expresisons are used to check for properties of the template arguments. For example, if the goal was to initialize a :code:`sockaddr_in` structure from a :code:`uint32_t` as an IPv4 address, that might be done with::

    void init_addr(sockaddr_in * addr, uint32_t raw) {
        addr.family = AF_INET;
        addr.sin_addr = raw;
    }

This will fail on FreeBSD because there the :code:`sockaddr_in` has another member, :code:`sin_len`, which doesn't get set. On the other hand, that member doesn't exist on Linux, which creates a bit of a problem. This could be handled with a clever build / configuration system and :code:`#define`, but I consider it cleaner, easier for users, and more robust to do it in C++. Use meta case a function can be written to set :code:`sin_len` such that if the member exists, it is set, and if not it is silently ignored.

Starting with the overloads yields::

    // base case - do nothing.
    template < typename T >
    void set_sin_len(T * addr, swoc::meta::CaseTag<0>)
    { } // do nothing.
    // if member exists, set it.
    template < typename T >
    void set_sin_len(T * addr, swoc::meta::CaseTag<1>)
        -> decltype(T::sin_len, swoc::meta::VoidFunc())
    {
        addr->sin_len = sizeof(sockaddr_in);
    }

The base function becomes::

    void init_addr(sockaddr_in * addr, uint32_t raw) {
        addr.family = AF_INET;
        addr.sin_addr = raw;
        set_sin_len(addr, swoc::meta::CaseArg);
    }

If this is done in several places it would make sense to add::

    void set_sin_len(sockaddr_in * addr) { set_sin_len(addr, swoc::meta::CaseArg; }

which can be invoked with just::

    set_sin_len(addr);

The 0 level oveload is always valid because it has no test expression. The other overload has the test expression
:code:`T::sin_len` which will only compile if :code:`T` has the member :code:`sin_len`. For a Linux system, only case 0
is valid and therefore nothing is done when the function is called. For FreeBSD, both will be valid but case 1 has
higher priority therefore it will be called and the member will be set correctly. Although it looks like a lot of code
in actual use the templates will get compiled down to equivalent of a simple assignment to the member.

Example
========

To pick an actual example, consider the :class:`Scalar`. A :class:`Scalar` instantiation can have a "tag" which is a type that distinguishes otherwise identical instanations, to prevent cross 

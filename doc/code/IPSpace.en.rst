.. SPDX-License-Identifier: Apache-2.0

.. include:: ../common-defs.rst

.. default-domain:: cpp
.. highlight:: cpp

*************
IP Networking
*************

Synopsis
********

:code:`#include <swoc/swoc_ip.h>`

Usage
*****

.. class:: IPEndpoint

   :libswoc:`Reference documentation <swoc::IPEndpoint>`.

This library is for storing and manipulating IP addresses as data. It has no support for actual
network operations.

IPAddr
======

The classes :libswoc:`swoc::IPAddr`, :libswoc:`swoc::IP4Addr`, and :libswoc:`swoc::IP6Addr` are used
to hold IP addresses. :code:`IP4Addr` and :code:`IP6Addr` are family specific and hold
(respectively) IPv4 and IPv6 addresses. :code:`IPAddr` acts as a union of these two types along with
an IP family specifier that indicates the type of address contained. The type specific classes
provide performance and storage benefits when the type of the address is known or forced, while
:code:`IPAddr` provides a generic type useful for interfaces.

These classes provide support for parsing and formatting IP addresses. The constructor can take
a string and, if a valid address, will initialize the instance to that address. The downside is
there is no indication of failure other than the instance initializing to the zero or "any"
address. This can be reasonable in situations where those addresses are not valid either. However
in general the :libswoc:`swoc::IPAddr::load` method should be used, which both initializes the
instance and provides an indication of whether the input was valid.

IPRange
=======

The classes :libswoc:`swoc::IPRange`, :libswoc:`swoc::IP4Range`, and :libswoc:`swoc::IP6Range` are
used to hold ranges of IP addresses. :code:`IP4Range` and :code:`IP6Range` are family specific and
hold (respectively) IPv4 and IPv6 addresses. :code:`IPAddr` acts as a union of these two types along
with an IP family specifier that indicates the type of address contained. The type specific classes
provide performance and storage benefits when the type of the address is known or forced, while
:code:`IPRange` provides a generic type useful for interfaces. Note that an :code:`IPRange` holds a
range of addresses of a single family, it can never hold a range that is of mixed families.

These classes provide support for parsing and formatting ranges of IP adddresses. The parsing logic
accepts three forms of a range. In all cases the lower and upper limits of the range must be the
same IP address family.

Range
   Two addresses, separated by a dash ("-") character. E.g.

      172.26.13.4-172.27.12.9

Network
   An address and a CIDR based mask, separated by a slash ("/") character. E.g.

      1337:0:0:ded:BEEF::/48

Singleton
   A single IP address, which is interpreted as a range of size 1.

Such a string can be passed to the constructor, which will initialize to the corresponding range if
properly formatted, otherwise the range will be default constructed to an invalid range. There is
also the :libswoc:`swoc::IPRange::load` method which returns a :code:`bool` to indicate if the
parsing was successful.

IPSpace
=======

The :libswoc:`swoc::IPSpace` class is designed as a container for ranges of IP addresses. Lookup is
done by single addresses. Conceptually, for each possible IP address there is a *payload*. Populating
the container is done by applying a specific payload to a range of addresses. After populating an
IP address can be looked up to find the corresponding payload.

The payload is a template argument to the class, as with standard containers. There is no template
argument for the key, as that is always an IP address.

Applying payloads to the space is analogized to painting, each distinct payload considered a different
"color" for an address. There are several methods used to paint the space, depending on the desired
effect on existing payloads.

mark
   :libswoc:`swoc::IPSpace::mark` applies the :arg:`payload` to the range, replacing any existing
   payload present. This is modeled on the "painter's algorithm" where the most recent coloring
   replaces any prior colors in the region.

fill
   :libswoc:`swoc::IPSpace::fill` applies the :arg:`payload` to the range but only where there is not
   already a payload. This is modeled on "backfilling" a background. This is useful for "first match"
   logic, as which ever payload is first put in to the container will remain.

blend
   :libswoc:`swoc::IPSpace::blend` applies the :arg:`payload` to the range by combining ("blending")
   the payloads. The result of blending can be "uncolored" which results in those addresses being
   removed from the space. This is useful for applying different properties in sequence, where the
   result is a combination of the properties.

Blend
+++++

Blending is different than marking or filling, as the latter two apply the payload passed to the
method. That is, if an address is marked by either method, it is marked with precisely the payload
passed to the method. :code:`blend` is different because it can cause an address to be marked by
a payload that was not explicitly passed in to any coloring method. Instead of replacing an
existing payload, it enables computing the resulting payload from the existing payload and a value
passed to the method.

The :libswoc:`swoc::IPSpace::blend` method requires a range and a "blender", which is a functor
that blends a :arg:`color` into a :code:`PAYLOAD` instances. The signature is ::

  bool blender(PAYLOAD & payload, U const& color)

The type :code:`U` is that same as the template argument :code:`U` to the :code:`blend` method,
which must be compatible with the second argument to the :code:`blend` method. The argument passed
to :code:`blender` is the second argument to :code:`blend`.

The method is modeled on C++ `compound assignment operators
<https://en.cppreference.com/w/cpp/language/operator_assignment#Builtin_compound_assignment>`__. If
the blend operation is thought of as the "@" operator, then the blend functor performs
:code:`lhs @=rhs`. That is, :arg:`lhs` is modified to be the combination of :arg:`lhs` and :arg`rhs`.
:arg:`lhs` is always the previous payload already in the space, and :arg:`rhs` is the :arg:`color`
argument to the :code:`blend` method. The internal logic handles copying the payload instances as
needed.

The return value of the blender indicates whether the combined result in :arg:`lhs` is a valid
payload or not. If it is the method should return :code:`true`. In general most implementations will
:code:`return true;` in all cases. If the method returns :code:`false` then the address(es) for the
combined payload are removed from the container. This allows payloads to be "unblended", for one
payload to cancel out another, or to do selective erasing of ranges.

As an example, consider the case where the payload is a bitmask. It might be reasonable to keep
empty bitmasks in the container, but it would also be reasonable to decide the empty bitmask and any
address mapped to it should removed entirely from the container. In such a case, a blender that
clears bits in the payloads should return :code:`false` when the result is the empty bitmask.

Similarly, if the goal is to remove ranges that have a specific payload, then a blender that returns
:code:`false` if :arg:`lhs` matches that specific payload and :code:`true` if not, should be used.

There is a small implementation wrinkle, however, in dealing with unmapped addresses. The
:arg:`color` is not necessarily a :code:`PAYLOAD` and therefore must be converted in to one. This
is done by default constructing a :code:`PAYLOAD` instance and then calling :code:`blend` on that
and the :arg:`color`. If this returns :code:`false` then unmapped addresses will remain unmapped.

History
*******

This is based (loosely) on the :code:`IpMap` class in Apache Traffic Server, which in turn is based
on IP addresses classes developed by Network Geographics for the Infosecter product. The code in
Apach Traffic Server was a much simplified version of the original work and this is a reversion
to that richer and more complete set of classes and draws much of its structure from the Network
Geographics work directly.

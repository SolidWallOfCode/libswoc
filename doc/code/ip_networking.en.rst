.. SPDX-License-Identifier: Apache-2.0

.. include:: ../common-defs.rst

.. default-domain:: cpp
.. highlight:: cpp

*************
IP Networking
*************

Synopsis
********

:code:`#include "swoc/swoc_ip.h"`

Usage
*****

This library is for storing and manipulating IP addresses as data. It has no support for actual
network operations.

IPEndpoint
==========

:libswoc:`swoc::IPEndpoint` is a wrapper around :code:`sockaddr` and provides a number of utilities.
It enables constructing an instance from the string representation of an address, supporting IPv4
and IPv6. It will also parse and store the port if that is part of the string. Some of the internal
logic is exposed via :libswoc:`swoc::IPEndpoint::tokenize` which finds and returns the elements of
an address string, the host (address), port, and any trailing remnants. This is useful for doing
syntax checks or more specialized processing of the address string.

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

Conversions to and from :code:`sockaddr` are provided. This is handier with :code:`IPAddr` as it
will conform to the family of the address in the :code:`sockaddr`.

IPSrv
=====

A container for an address and a port. There is no really good name for this therefore I used the
DNS term for such an object. This consists of the usual triplet of classes, :swoc:`IP4Srv`, :swoc:`IP6Srv`,
and :swoc:`IPSrv`. The first two are protocol family specific and the third holds an instance of
either an :code:`IP4Srv` or an `IP6Srv`. The address and port can be manipulated separately.

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

This class has formatting support in "bwf_ip.h". In addition to all of the formatting supported for
:code:`sockaddr`, the additional extension code 'c' can be used to indicate compact range formatting.
Compact means a singleton range will be written as just the single address, and if the range is
also a network it will be printed in CIDR format.

======================= =======================
Range                   Compact
======================= =======================
10.1.0.0-10.1.0.127     10.1.0.0/25
10.2.0.1-10.2.0.127     10.2.0.1-10.2.0.127
10.3.0.0-10.3.0.126     10.3.0.0-10.3.0.126
10.4.1.1-10.4.1.1       10.4.1.1
======================= =======================

Conversions
===========

Most conversions between types should be straight forward but in some cases there is some indirection
in order to avoid bad conversions due to differences in address families.

Conversion from :libswoc:`swoc::IPEndpoint` to :libswoc:`swoc::IPAddr` is direct as the latter can
be explicitly constructed from the former. For :libswoc:`swoc::IP6Addr` and :libswoc:`swoc::IP4Addr`
the family must be checked first. The expected way to do this is ::

   if ( auto sa = ep.ip4() ; sa ) {
      IP4Addr addr(sa);
      // ....
   }

This is intended to be similar to how dynamic casts are handled when it is not guaranteed the generic
type contains an instance of the more specific type.

Conversion from address types to socket addresses can be done by constructing an :code:`IPEndpoint` or,
if the socket address structure already exists, using the :code:`copy_to` methods on the address types,
such as :libswoc:`swoc::IP4Addr::copy_to`. Note converting an address types sets only the family and
address. Converting a service type also sets the port.

.. _ip-space:

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

Examples
********

Blending Bitsets
================

.. sidebar:: Code

   Some details are omitted for brevity and because they aren't directly relevant. The full
   implementation, which is run as a unit test to verify its correctness,
   `is available here <https://github.com/SolidWallOfCode/libswoc/blob/1.4.7/unit_tests/ex_ipspace_properties.cc>`__.
   You can compile and step through the code to see how it works in more detail, or experiment
   with changing some of the example data.

As an example of blending, consider a mapping of IP addresses to a bit set, each bit representing
some independent property of the address (e.g., production, externally accessible, secure, etc.). It
might be the case that each of these was in a separate data source. In that case one approach would
be to blend each data source into the IPSpace, combining the bits in the blending functor. If
`std::bitset <http://www.cplusplus.com/reference/bitset/bitset/>`__ is used to hold the bits, the
declarations could be done as

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: "IPSpace bitset blending"
   :lines: 1-4
   :emphasize-lines: 2,4

To do the blending, a blending functor is needed.

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // Bitset blend functor
   :lines: 1-4

This always returns :code:`true` because blending any bits never yields a zero result. A lambda is
provided to do the marking of the example data for convience. This takes a list of example data
items defined as a range and a list of bits to set.

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // Example data type.
   :lines: 1

The marking logic is

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // Example marking functor.
   :lines: 1-7

Let's try it out. For the first pass this data will be used.

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // test ranges 1
   :lines: 2-8

After using this, the space contents are ::

   7 ranges
   100.0.0.0-100.0.0.255     : 10000000000000000000000000000000
   100.0.1.0-100.0.1.255     : 01000000000000000000000000000000
   100.0.2.0-100.0.2.255     : 00100000000000000000000000000000
   100.0.3.0-100.0.3.255     : 00010000000000000000000000000000
   100.0.4.0-100.0.4.255     : 00001000000000000000000000000000
   100.0.5.0-100.0.5.255     : 00000100000000000000000000000000
   100.0.6.0-100.0.6.255     : 00000010000000000000000000000000

Those are non-overlapping intervals and therefore are not really blended. Suppose the following
ranges are also blended - note these overlap the first two ranges from the previous ranges.

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // test ranges 2
   :lines: 2-4

This yields ::

   9 ranges
   100.0.0.0-100.0.0.255     : 10000000000000000000000000000001
   100.0.1.0-100.0.1.255     : 01000000000000000000000000000010
   100.0.2.0-100.0.2.127     : 00100000000000000000000000000000
   100.0.2.128-100.0.2.255   : 00100000000000000000000000000100
   100.0.3.0-100.0.3.127     : 00010000000000000000000000000100
   100.0.3.128-100.0.3.255   : 00010000000000000000000000000000
   100.0.4.0-100.0.4.255     : 00001000000000000000000000000000
   100.0.5.0-100.0.5.255     : 00000100000000000000000000000000
   100.0.6.0-100.0.6.255     : 00000010000000000000000000000000

The additional bits are now present on the other side of the bit set. Note there are now more ranges
because the last range overlapped two of the previously existing ranges. Those are split because of
the now differing payloads for the new ranges.

What happens if this range and data is blended into the space?

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // test ranges 3
   :lines: 2

The result is ::

   6 ranges
   100.0.0.0-100.0.0.255     : 10000000000000000000000000000001
   100.0.1.0-100.0.1.255     : 01000000000000000000000000000010
   100.0.2.0-100.0.3.255     : 00110000000000000000000000000100
   100.0.4.0-100.0.4.255     : 00111000000000000000000000000100
   100.0.5.0-100.0.5.255     : 00000100000000000000000000000000
   100.0.6.0-100.0.6.255     : 00000010000000000000000000000000

Note the ".2" and ".3" ranges have collapsed in to a single range with the bits 2,3,29 set. The
".4" range remains separate because it also has bit 4 set, which is distinct.

Blending allows selective erasing. Let's erase bits 2,3,29 in all ranges. First a blending functor
is needed to erase bits instead of settting them.

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // reset blend functor
   :lines: 1-5

Note this returns :code:`false` if the result of clearing the bits is an empty bitset. Just to be
thorough, let's clear those bits for all IPv4 addresses.

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // erase bits
   :lines: 1

The result is ::

   5 ranges
   100.0.0.0-100.0.0.255     : 10000000000000000000000000000001
   100.0.1.0-100.0.1.255     : 01000000000000000000000000000010
   100.0.4.0-100.0.4.255     : 00001000000000000000000000000000
   100.0.5.0-100.0.5.255     : 00000100000000000000000000000000
   100.0.6.0-100.0.6.255     : 00000010000000000000000000000000

The ".2" and ".3" ranges have disappeared, as this bit clearing cleared all the bits in those
ranges. The ".4" range remains back to its original state, the extra bits having been cleared. The
other ranges are unchanged because this operation did not change their payloads. No new ranges have
been added because the result of unsetting bits where no bits are set is also the empty bit set.
This means the blender returns :code:`false` and this prevents the range from being created.

As a final note, although the data used here was network based, that is in no way required.
This line of code being executed

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // ragged boundaries
   :lines: 1

yields the result ::

   7 ranges
   100.0.0.0-100.0.0.255     : 10000000000000000000000000000001
   100.0.1.0-100.0.1.255     : 01000000000000000000000000000010
   100.0.2.19-100.0.3.255    : 00000000000000001010100000000000
   100.0.4.0-100.0.4.255     : 00001000000000001010100000000000
   100.0.5.0-100.0.5.117     : 00000100000000001010100000000000
   100.0.5.118-100.0.5.255   : 00000100000000000000000000000000
   100.0.6.0-100.0.6.255     : 00000010000000000000000000000000

Although the examples up to now have used :code:`PAYLOAD` as the argument type to the
:code:`blend` method, this is not required in general. The type of the second argument
to :code:`blend` is determined by the second argumen to the functor, so that data other
than strictly :code:`PAYLOAD` can be blended into the space. For instance the blending
functor might directly take a list of bit indices -

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // bit list blend functor
   :lines: 1-5

In this case the call to :code:`blend` must also take a list of bit indices, not a :code:`PAYLOAD`
(e.g. a :code:`std::bitset<32>`).

.. literalinclude:: ../../unit_tests/ex_ipspace_properties.cc
   :start-after: // bit list blend functor
   :lines: 7-8

That call to :code:`blend` will blend bits 10 and 11 into all IPv4 addresses except the first and
last, yielding ::

   10 ranges
   0.0.0.1-99.255.255.255    : 00000000001100000000000000000000
   100.0.0.0-100.0.0.255     : 10000000001100000000000000000001
   100.0.1.0-100.0.1.255     : 01000000001100000000000000000010
   100.0.2.0-100.0.2.18      : 00000000001100000000000000000000
   100.0.2.19-100.0.3.255    : 00000000001100001010100000000000
   100.0.4.0-100.0.4.255     : 00001000001100001010100000000000
   100.0.5.0-100.0.5.117     : 00000100001100001010100000000000
   100.0.5.118-100.0.5.255   : 00000100001100000000000000000000
   100.0.6.0-100.0.6.255     : 00000010001100000000000000000000
   100.0.7.0-255.255.255.254 : 00000000001100000000000000000000

History
*******

This is based (loosely) on the :code:`IpMap` class in Apache Traffic Server, which in turn is based
on IP addresses classes developed by Network Geographics for the Infosecter product. The code in
Apach Traffic Server was a much simplified version of the original work and this is a reversion
to that richer and more complete set of classes and draws much of its structure from the Network
Geographics work directly.

I want to thank `Uthira Mohan <https://www.linkedin.com/in/uthiramohan>`__ for being my intial
tester and feature requestor - in particular the design of blending is a result of her feature
demands.

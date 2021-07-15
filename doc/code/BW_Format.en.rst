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
.. highlight:: cpp
.. default-domain:: cpp
.. |BW| replace:: :code:`BufferWriter`
.. |BWF| replace:: :code:`BufferWriter` formatting

.. _bw-format:

***********************
BufferWriter Formatting
***********************

Synopsis
********

:code:`#include "swoc/bwf_base.h"`

Formatted output was added to :class:`BufferWriter` for several reasons.

*  Type safe formatted output in addition to buffer safe formatted output. Rather than non-obvious
   cleverness with :code:`snprintf` and :libswoc:`BufferWriter::commit`, build the formatting
   in directly.

*  Specialized output functions for complex types, to have the class provide the formatting logic
   instead of cut and pasted code in multiple locations. This also avoids breaking modularity to get
   the data needed for good formatting. This also enables formatting wrappers which can provide
   generic and simple ways to do specific styles of output beyond formatting codes (e.g.
   :libswoc:`As_Hex`).

*  Argument naming, both for ordering, repeating, and for "global" names which can be used without
   arguments. This is also intended for use where there are context dependent names, e.g. for
   printing in the context of an HTTP header, the header field names could be made so their use
   is replaced by the value of that field.

*  The ability to pass arbitrary "extra" data to formatting functions for special, type dependent
   purposes.

The formatting style is the "prefix" or "printf" style - the format is specified first and then all
the arguments. The syntax is based on `Python formatting
<https://docs.python.org/3/library/string.html#formatstrings>`__. This contrasts to the "infix" or
"streaming" style where formatting, literals, and argument are intermixed in the order of output.
There are various arguments for both styles but conversations within the Trafffic Server community
indicated a clear preference for the prefix style. Therefore creating formatted output consists of a
:term:`format string`, containing literal text and :term:`format specifier`\ s, which are replaced
with generated text, usually based on the values of arguments to the print function.

The design is optimized for formatted output to fixed buffers. This is by far the dominant style in
the expected use cases and during the design phase I was told any performance loss compared to
:code:`snprintf` must be minimal. While work has and will be done to extend :class:`BufferWriter` to
operate on non-fixed buffers, such use is secondary to operating directly on contiguous buffers.

.. important::

   The overriding design goal is to provide the type specific formatting and flexibility of C++
   stream operators with the performance of :code:`snprintf` and :code:`memcpy`.

Usage
*****

As noted |BWF| is modeled on Python string formatting because the Traffic Server project uses quite
a bit of Python. It seemed a good model for prefix style formatting, mapping easily in to the
set of desired features. The primary divergences are

*  Names do not refer to in scope variables, but to output generators local to the print context via
   `Name Binding`_.

*  The addition of a third colon separated field to provide extension data to the formatting logic.

The primary entry point for this is :libswoc:`BufferWriter::print`.

A format string consists of literal text in which format specifiers are embedded. Each specifier
marks a place where generated output will be placed. The specifier is marked by paired braces and is
divided in to three fields, separated by colons. These fields are optional - if default output is
acceptable, a pair of braces will suffice. In a sense, ``{}`` serves the same function for output as
:code:`auto` does for programming - the compiler knows the type, it should be able to do something
reasonable without the programmer needing to be explicit. The fields are used in the less common
cases where greater control of the output is required.

Format Specifier Grammar
========================

This is the grammar for the fields inside a format specifier.

.. productionList:: spec
   specifier: "{" [name] [":" [style] [":" extension]] "}"
   name: index | ICHAR+
   index: non-negative integer
   extension: ICHAR*
   ICHAR: a printable ASCII character except for '{', '}', ':'
   style: formatting instructions.

The three fields are :token:`~spec:name`, :token:`~spec:style`, and :token:`~spec:extension`.

:token:`~spec:name`
   The :token:`~spec:name` of the argument to use. This can be a non-negative integer in which case
   it is the zero based index of the argument to the method call. E.g. ``{0}`` means the first
   argument and ``{2}`` is the third argument after the format.

      ``bw.print("{0} {1}", 'a', 'b')`` => ``a b``

      ``bw.print("{1} {0}", 'a', 'b')`` => ``b a``

   The :token:`~spec:name` can be omitted in which case it is treated as an index in parallel to the
   position in the format string relative to other argument based specifiers. Only the position in
   the format string matters, not what arguments other format specifiers may have used.

      ``bw.print("{0} {2} {}", 'a', 'b', 'c')`` => ``a c c``

      ``bw.print("{0} {2} {2}", 'a', 'b', 'c')`` => ``a c c``

   Note an argument can be printed more than once if the name is used more than once.

      ``bw.print("{0} {} {0}", 'a', 'b')`` => ``a b a``

      ``bw.print("{0} {1} {0}", 'a', 'b')`` => ``a b a``

   Alphanumeric names refer to values in a :term:`format context` table. These will be described in
   more detail someday. Such names do not count in terms of default argument indexing. These rules
   are designed to be natural, but any ambiguity can be eliminated by explicit indexing in the
   specifiers.

:token:`~spec:style`
   Basic formatting control.

   .. productionList:: fmt
      style: [[fill]align][sign]["#"]["0"][[min][.precision][,max][type]]
      fill: fill-char | URI-char
      URI-char: "%" hex-digit hex-digit
      fill-char: printable character except "{", "}", ":", "%"
      align: "<" | ">" | "=" | "^"
      sign: "+" | "-" | " "
      min: non-negative integer
      precision: positive integer
      max: non-negative integer
      type: "g" | "s" | "S" | "x" | "X" | "d" | "o" | "b" | "B" | "p" | "P"
      hex-digit: "0" .. "9" | "a" .. "f" | "A" .. "F"

   The output is placed in a field that is at least :token:`~fmt:min` wide and no more than :token:`~fmt:max`
   wide. If the output is less than :token:`~fmt:min` then

      *  The :token:`~fmt:fill` character is used for the extra space required. This can be an explicit
         character or a URI encoded one (to allow otherwise reserved characters).

      *  The output is shifted according to the :token:`!fmt:align`.

         <
            Align to the left, fill to the right.

         >
            Align to the right, fill to the left.

         ^
            Align in the middle, fill to left and right.

         =
            Numerically align, putting the fill between the sign character (left aligned) and the
            value (right aligned).

   The output is clipped by :token:`~fmt:max` width characters and by the end of the buffer.
   :token:`~fmt:precision` is used by floating point values to specify the number of places of precision.

   :token:`~fmt:type` is used to indicate type specific formatting. For integers it indicates the output
   radix and if ``#`` is present the radix is prefix is generated (one of ``0xb``, ``0``, ``0x``).
   Format types of the same letter are equivalent, varying only in the character case used for
   output. Most commonly 'x' prints values in lower cased hexadecimal (:code:`0x1337beef`) while 'X'
   prints in upper case hexadecimal (:code:`0X1337BEEF`). Note there is no upper case decimal or
   octal type because case is irrelevant for those.

      = ===============
      g generic, default.
      b binary
      B Binary
      d decimal
      o octal
      x hexadecimal
      X Hexadecimal
      p pointer (hexadecimal address)
      P Pointer (Hexadecimal address)
      s string
      S String (upper case)
      = ===============

   For several specializations the hexadecimal format is taken to indicate printing the value as if
   it were a hexidecimal value, in effect providing a hex dump of the value. This is the case for
   :code:`std::string_view` and therefore a hex dump of an object can be done by creating a
   :code:`std::string_view` covering the data and then printing it with :code:`{:x}`.

   The string type ('s' or 'S') is generally used to cause alphanumeric output for a value that
   would normally use numeric output. For instance, a :expr:`bool` is normally ``0`` or ``1``. Using
   the type 's' yields ``true`` or ``false``. The upper case form, 'S', applies only in these cases
   where the formatter generates the text, it does not apply to normally text based values unless
   specifically noted. Therefore a :code:`bool` printed with the type 'S' yields ``TRUE`` or
   ``FALSE``. This is frequently done with formatting for enumerations, printing the numeric value
   by default and printing a text equivalent for format 's' or 'S'.

:token:`~spec:extension`
   Text (excluding braces) passed to the type specific formatter function. This can be used to
   provide extensions for specific argument types (e.g., IP addresses). It is never examined by
   |BWF|, it is only effective in type specific formatting overloads.

When a format specifier is parsed, the result is placed in an instance of :libswoc:`bwf::Spec <Spec>`.

Examples
========

Some examples, comparing :code:`snprintf` and :libswoc:`BufferWriter::print`. ::

   if (len > 0) {
      auto n = snprintf(buff, len, "count %d", count);
      len -= n;
      buff += n;
   }

   bw.print("count {}", count);

   // --

   if (len > 0) {
      auto n = snprintf(buff, len, "Size %" PRId64 " bytes", sizeof(thing));
      len -= n;
      buff += n;
   }

   bw.print("Size {} bytes", sizeof(thing));

   // --

   if (len > 0) {
      auto n = snprintf(buff, len, "Number of items %ld", thing->count());
      len -= n;
      buff += n;
   }

   bw.print("Number of items {}", thing->count());

Enumerations become easier. Note in this case argument indices are used in order to print both a
name and a value for the enumeration. A key benefit here is the lack of need for a developer to know
the specific free function or method needed to do the name lookup. In this case,
:code:`HttpDebugNuames::get_server_state_name`. Rather than every developer having to memorize the
assocation between the type and the name lookup function, or grub through the code hoping for an
example, the compiler is told once and henceforth does the lookup. The implementation of the
formatter is described in `an example <bwf-http-debug-name-example>`. A sample of code previously
used to output an error message using this enumeration. ::

   if (len > 0) {
      auto n = snprintf(buff, len, "Unexpected event %d in state %s[%d] for %.*s",
         event,
         HttpDebugNames::get_server_state_name(t_state.current.state),
         t_state.current.state,
         static_cast<int>(host_len), host);
      buff += n;
      len -= n;
   }

Using |BW| ::

   bw.print("Unexpected event {0} in state {1}[{1:d}] for {2}",
      event, t_state.current.state, std::string_view{host, host_len});

Adapting to use of :code:`std::string_view` illustrates the advantage of a formatter overload
knowing how to get the size from the object and not having to deal with restrictions on the numeric
type (e.g., that :code:`%.*s` requires an :code:`int`, not a :code:`size_t`). ::

   if (len > 0) {
      len -= snprintf(buff, len, "%.*s", static_cast<int>(s.size()), s.data());
   }

vs ::

   bw.print("{}", s);

or even

   bw.write(s);

The difference is even more stark with dealing with IP addresses. There are two big advantages here.
One is not having to know the conversion function name. The other is the lack of having to declare
local variables and having to remember what the appropriate size is. Not requiring local variables
can be particularly nice in the context of a :code:`switch` statement where local variables for a
:code:`case` mean having to add extra braces, or declare the temporaries at an outer scope. ::

   char ip_buff1[INET6_ADDRPORTSTRLEN];
   char ip_buff2[INET6_ADDRPORTSTRLEN];
   ats_ip_nptop(ip_buff1, sizeof(ip_buff1), addr1);
   ats_ip_nptop(ip_buff2, sizeof(ip_buff2), add2);
   if (len > 0) {
      snprintf(buff, len, "Connecting to %s from %s", ip_buff1, ip_buff2);
   }

vs ::

   bw.print("Connecting to {} from {}", addr1, addr2);

User Defined Formatting
=======================

To get the full benefit of type safe formatting it is necessary to provide type specific formatting
functions which are called when a value of that type is formatted. This is how type specific
knowledge such as the names of enumeration values are encoded in a single location. The special
formatting for IP address data is done by providing default formatters, it is not built in to the
base formatting logic.

Most of the support for this is in the nested namespace :code:`bwf`.

The format style is stored in an instance of :libswoc:`bwf::Spec <Spec>`.

.. namespace-push:: bwf

.. class:: Spec

   Format specifier data.

   :libswoc:`Reference <Spec>`.

.. namespace-pop::

Additional type specific formatting can be provided via the :token:`~spec:extension` field. This provides
another option for tweaking formatted output vs. using wrapper classes.

To provide a formatter for a type :code:`V` the function :code:`bwformat` is overloaded. The
signature would look like this::

   swoc::BufferWriter&
   swoc::bwformat( swoc::BufferWriter& w
                 , swoc::bwf::Spec const& spec
                 , V const& v
                 )

:arg:`w` is the output and :arg:`spec` the :libswoc:`parsed format specifier <bwf::Spec>`, including
the name and extension (if any). The calling framework will handle basic alignment as per
:arg:`spec` therefore the overload normally does not need to do so. In some cases, however, the
alignment requirements are more detailed (e.g. integer alignment operations) or performance is
critical. In the latter case the formatter should make sure to use at least the :libswoc:`minimum
width <bwf::Spec::_min>` in order to disable any framework alignment operation.

It is important to note a formatter can call another formatter. For example, the formatter for
:code:`std::string` looks like

.. literalinclude:: ../../code/include/swoc/bwf_base.h
   :lines: 811-833

The code first copies the format specification and forces a leading radix. Next it does special
handling for :code:`nullptr`. If the pointer is valid, the code checks if the type ``p`` or ``P``
was used in order to select the appropriate case, then delegates the actual rendering to the
:libswoc:`integer formatter <Format_Integer>` with a type of ``x`` or ``X`` as appropriate. In turn
other formatters, if given the type ``p`` or ``P`` can cast the value to :code:`const void*` and
call :code:`bwformat` on that to output the value as a pointer. The difference between calling
:code:`bwformat` vs. :libswoc:`BufferWriter::write` is the ability to pass the format specifier
instance. If all of the formatting is handled directly, then direct |BW| methods are a good choice.
If the formatter wants to use the built in formatting then :code:`bwformat` is the right choice.
This is what is done with the pointer example above - the format specifier is copied and tweaked,
and then passed on so that any formatting provided from the original format string remains valid.

To help reduce duplication, the output stream operator :code:`operator<<` on a :code:`BufferWriter` is defined to call :code:`bwformat` with a default constructed :libswoc:`bwf::Spec` instance. This makes ::

   w << thing;

identical to ::

   bwformat(w, swoc::bwf::Spec::DEFAULT, thing);

which is also the same as ::

   w.print("{}", thing);

Enum Example
------------

.. _bwf-http-debug-name-example:

For a specific example of using |BWF| to make debug messages easier, consider the case of
:code:`HttpDebugNames` in the Traffic Server code base. This is a class that serves as a namespace
to provide various methods that convert state machine related enumerations into descriptive strings.
Currently this is undocumented (and uncommented) and is therefore used infrequently, as that
requires either blind cut and paste, or tracing through header files to understand the code. The
result is much less useful diagnostics. This can be greatly simplified by adding formatters to
:file:`proxy/http/HttpDebugNames.h` ::

   inline swoc::BufferWriter &
   bwformat(swoc::BufferWriter &w, swoc::bwf::Spec const &spec, HttpTransact::ServerState_t state)
   {
      if (spec.has_numeric_type()) {
         // allow the user to force numeric output with '{:d}' or other numeric type.
         return bwformat(w, spec, static_cast<uintmax_t>(state));
      } else {
         return bwformat(w, spec, HttpDebugNames::get_server_state_name(state));
      }
   }

With this in place, the code to print the name of the server state enumeration is ::

   bw.print("{}", t_state.current_state);

There is no need to remember names like :code:`HttpDebugNames` nor which method in it does the
conversion. The developer making the :code:`HttpDebugNames` class or equivalent can take care of
that in the same header file that provides the type. The type specific formatting is incorporated in
to the general printing mechanism and from that point on works without any local code required, or memorization by the developer.

Argument Forwarding
-------------------

It will frequently be useful for other libraries to support formatting for input strings. For such
use cases the class methods will need to take variable arguments and then forward them on to the
formatter. :class:`BufferWriter` provides :libswoc:`BufferWriter::print_v` for this purpose. Instead
of taking C style variable arguments, these overloads take a reference to a :code:`std::tuple` of
arguments. Such as tuple is easily created with `std::forward_as_tuple
<http://en.cppreference.com/w/cpp/utility/tuple/forward_as_tuple>`__. An example of this is a container of messages. The message class is

.. literalinclude:: ../../unit_tests/ex_IntrusiveDList.cc
   :lines: 37-48,62
   :emphasize-lines: 10

The container class has a :code:`debug` method to append :code:`Message` instances using |BWF|.

.. literalinclude:: ../../unit_tests/ex_IntrusiveDList.cc
   :lines: 81-82,89,98

The implementation is simple.

.. literalinclude:: ../../unit_tests/ex_IntrusiveDList.cc
   :lines: 122-131
   :emphasize-lines: 6

This gathers the argument (generally references to the arguments) in to a single tuple which is then
passed by reference, to avoid restacking the arguments for every nested function call. In essence
refernces the arguments are put on the stack (inside the tuple) once and a reference to that stack
is passed to nested functions. This replaces the C style :code:`va_list` and provides not just arguments but also complete type information.

The example code uses :libswoc:`bwprint_v` to print to a :code:`std::string`. There is corresponding
method, :libswoc:`BufferWriter::print_v`, which takes a tuple instead of an explicit list of
arguments when working with |BW| instances. Internally, of course, :libswoc:`bwprint_v` is
implemented using a local :libswoc:`FixedBufferWriter` instance and
:libswoc:`BufferWriter::print_v`.

Default Type Specific Formatting
================================

|BWF| has a number of user defined formatting overloads built in, primarily for types used inside the |BWF| implementation, to avoid circular reference problems. There is also support for formatting
`IP addresses <_ip_addr_fmt>`_ via an additional include file.

Specific types
--------------

:code:`std::string_view`
   Generally the contents of the view.

   'x' or 'X'
      A hexadecimal dump of the contents of the view in lower ('x') or upper ('X') case.

   'p' or 'P'
      The pointer and length value of the view in lower ('p') or upper ('P') case.

   's'
      The string in (forced) lower case.

   'S'
      The string in (forced) upper case.

   For printing substrings, views are sufficiently cheap to do this in the arguments. For instance,
   printing the 10th through 20th characters of the view :code:`text` means passing
   :code:`text.substr(9,11)` instead of :code:`text`.

   .. literalinclude:: ../../unit_tests/ex_bw_format.cc
      :lines: 43-44,57-58

   However, for those terminally addicted to C style formatting, this can also be done by setting
   the precision.

   .. literalinclude:: ../../unit_tests/ex_bw_format.cc
      :lines: 59-60,44,47-50

:libswoc:`TextView`
   Because this is a subclass of :code:`std::string_view`, all of the formatting for that works the same for this class.

.. _ip_addr_fmt:

:code:`sockaddr const *`
   :code:`#include "swoc/bwf_ip.h"`

   The IP address is printed. Fill is used to fill in address segments if provided, not to the
   minimum width if specified. :libswoc:`IPEndpoint` and :libswoc:`IPAddr` are supported with the
   same formatting. The formatting support in this case is extensive because of the commonality and
   importance of IP address data.

   Type overrides

      'p' or 'P'
         The pointer address is printed as hexadecimal lower ('p') or upper ('P') case.

   The extension can be used to control which parts of the address are printed. These can be in any order,
   the output is always address, port, family. The default is the equivalent of "ap". In addition, the
   character '=' ("numeric align") can be used to internally right justify the elements.

   'a'
      The address.

   'p'
      The port (host order).

   'f'
      The IP address family.

   '='
      Internally justify the numeric values. This must be the first or second character. If it is the second
      the first character is treated as the internal fill character. If omitted '0' (zero) is used.

   E.g. ::

      void func(sockaddr const* addr) {
        bw.print("To {}", addr); // -> "To 172.19.3.105:4951"
        bw.print("To {0::a} on port {0::p}", addr); // -> "To 172.19.3.105 on port 4951"
        bw.print("To {::=}", addr); // -> "To 127.019.003.105:04951"
        bw.print("Using address family {::f}", addr);
        bw.print("{::a}",addr);      // -> "172.19.3.105"
        bw.print("{::=a}",addr);     // -> "172.019.003.105"
        bw.print("{::0=a}",addr);    // -> "172.019.003.105"
        bw.print("{:: =a}",addr);    // -> "172. 19.  3.105"
        bw.print("{:>20:a}",addr);   // -> "        172.19.3.105"
        bw.print("{:>20:=a}",addr);  // -> "     172.019.003.105"
        bw.print("{:>20: =a}",addr); // -> "     172. 19.  3.105"
      }

Format Classes
--------------

Although the extension for a format can be overloaded to provide additional features, this can
become too confusing and complex to use if it is used for fundamentally different semantics on the
same based type. In that case it is better to provide a format wrapper class that holds the base
type but can be overloaded to produce different (wrapper class based) output. The classic example is
:code:`errno` which is an integral type but frequently should be formatted with additional
information such as the descriptive string for the value. To do this the format wrapper class
:code:`swoc::bwf::Errno` is provided. Using it is simple::

   w.print("File not open - {}", swoc::bwf::Errno(errno));

which will produce output that looks like

   "File not open - EACCES: Permission denied [13]"

For :code:`errno` this is handy in another way as :code:`swoc::bwf::Errno` will preserve the value
of :code:`errno` across other calls that might change it. E.g.::

   swoc::bwf::Errno last_err(errno);
   // some other code generating diagnostics that might tweak errno.
   w.print("File not open - {}", last_err);

This can also be useful for user defined data types. For instance, in the HostDB component of
Traffic Server the type of the entry is printed in multiple places and each time this code is
repeated ::

      "%s%s %s", r->round_robin ? "Round-Robin" : "",
         r->reverse_dns ? "Reverse DNS" : "", r->is_srv ? "SRV" : "DNS"

This could be wrapped in a class, :code:`HostDBFmt` such as ::

   struct HostDBFmt {
      HostDBInfo* _r { nullptr };
      HostDBFmt(r) : _r(r) {}
   };

Then define a formatter for the wrapper ::

   swoc::BufferWriter& bwformat( swoc::BufferWriter& w
                               , swoc::bwf::Spec const&
                               , HostDBFmt const& wrap
   ) {
      return w.print("{}{} {}", wrap._r->round_robin ? "Round-Robin" : "",
         r->reverse_dns ? "Reverse DNS" : "",
         r->is_srv ? "SRV" : "DNS");
   }

Now all of the cut and paste formatting code is replaced with ::

   w.print("{}", HostDBFmt(r));

These are the existing format classes in header file ``bfw_std_format.h``. All are in the
:code:`swoc::bwf` namespace.

.. namespace-push:: bwf

.. class:: Errno

   Formatting for :code:`errno`. Generically the formatted output is the short name, the
   description, and the numeric value. A format type of ``d`` will generate just the numeric value,
   while a format type of ``s`` will generate the short name and description without a number.

   For more detailed output, the extension can be used to pick just the short or long name. For
   non-numeric format codes, if the extension has the character 's' then the short name is output,
   and if it contains the character 'l' the long name is output.

   Examples:

   ========== ==============================================
   Format     Result
   ========== ==============================================
   ``:n``     [13]
   ``:s``     EACCES: Permission denied
   ``:s:sl``  EACCES: Permission denied
   ``:s:s``   EACCES
   ``:s:l``   Permission denied
   ``::s``    EACCES [13]
   ========== ==============================================

   :libswoc:`Reference <Errno>`.

.. class:: Date

   Date formatting in the :code:`strftime` style. An instance can be constructed with a :code:`strftime` compatible format, or with a :code:`time_t` and format string.

   When used the format specification can take an extention of "local" which formats the time as
   local time. Otherwise it is GMT. ``w.print("{}", Date("%H:%M"));`` will print the hour and minute
   as GMT values. ``w.print("{::local}", Date("%H:%M"));`` will print the hour and minute in the
   local time zone. ``w.print("{::gmt}"), ...);`` will output in GMT if additional explicitness is
   desired.

   :libswoc:`Reference <Date>`.

.. function:: template < typename ... Args > FirstOf(Args && ... args)

   Print the first non-empty string in an argument list. All arguments must be convertible to
   :code:`std::string_view`.

   By far the most common case is the two argument case used to print a special string if the base
   string is null or empty. For instance, something like this::

      w.print("{}", name != nullptr ? name : "<void>")

   This could also be done like::

      w.print("{}", swoc::bwf::FirstOf(name, "<void>"));

   If the first argument is a local variable that exists only to do the empty check, that variable
   can eliminated entirely.

      const char * name = thing.get_name();
      w.print("{}", name != nullptr ? name : "<void>")

   can be simplified to

      w.print("{}", swoc::bwf::FirstOf(thing.get_name(), "<void>"));

   In general avoiding ternary operators in the print argument list makes the code cleaner and
   easier to understand.

   :libswoc:`Reference <FirstOf>`.

.. class:: Optional

   A wrapper for optional output generation. This wraps a format string and a set of arguments and
   generates output conditional, either the format string with the arguments applied, or nothing.
   This is useful for output data that requires additional delimiters if present, but nothing
   if not. A common pattern for this is something like ::

      printf("Text: %d%s%s", count, data ? data : "", data ? " " : "");

   or something like ::

      printf("Text: %d");
      if (data) {
         printf(" %s", data);
      }

   In both cases, the leading space separating :arg:`data` from the previous output is printed iff
   :arg:`data` is not :code:`nullptr`. Using :code:`Optional` with |BWF| this is done with something
   like ::

      w.print("Text: {}{}", count, swoc:bwf::Optional(data != nullptr, " {}", data);

   The first argument is a conditional, which determines if output is generated, followed by a
   format string and then arguments for the format string. The number of specifiers in the format
   string and the number of arguments must agree.

   Because the case where the argument and the conditional are effective the same is so common, there
   is a specialization of :code:`Optional` which takes just a format string and an argument. This
   requires the format string to have take only one parameter, and the argument to either

   *  Have the method :code:`empty` which returns :code:`false` if there is content.

   *  Be convertible to :code:`bool` such that the argument converts to :code:`true` if there
      is content.

   This enables the example to be further reduced to ::

      w.print("Text: {}{}", count, swoc:bwf::Optional(" {}", data);

   Note this works with raw C strings, the STL string classes, and :code:`TextView`. The more general
   form can be used if this specialization doesn't suffice.

   :libswoc:`Reference <Optional>`.

Writing a Format Class
----------------------

Writing addtional format classes is designed to be easy, taking two or three steps. For example,
consider a wrapper to output a string in `rot13 <https://en.wikipedia.org/wiki/ROT13>`__.

The first step is to declare the wrapper class.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 651-652,655

This class simply stores the :code:`std::string_view` for later use.

Next the formatting for the wrapper class must be provided by overloading :code:`bwformat`.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 657-664

This uses :libswoc:`transform_view_of` to do the character rotation. The lambda to perform the per
character transform is defined separate for code cleanliness, it could just as easily have been
defined directly as an argument.

That's all that is strictly required - this code now works as expected.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 685-689

Note the universal initializer must be used because there is no constructor. That is easily fixed.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 651-655

and now this works as expected.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 691-692

Obviously other constructors can be provided for different ways to use the wrapper.

An optional third step is to use free functions, rather than constructors, to access the wrapper.
This is useful in some circumstances, one example being that it is desirable other classes can
overload the format class construction, which is not possible using only constructors. In this case,
a wrapper function could be done as

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 666-670

and used

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 694-695

Now, if there was a struct that needed Rot13 support

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 672-675

then the wrapper could be overloaded with

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 677-681

and used

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 697-699

In general, provide wrapper class constructors unless there is a specific need for using free
functions instead. Care should be used with the content of the format class to avoid expensive
copies. In this case a :code:`std::string_view` is very cheap to copy and the style of the wrapper
takes advantage of `return value optimization <https://en.wikipedia.org/wiki/Copy_elision>`__.

.. namespace-pop::

Working with standard I/O
=========================

For convenience a stream operator for :code:`std::stream` is provided to make the use more natural. ::

   std::cout << bw;
   std::cout << bw.view(); // identical effect as the previous line.

Using a :class:`BufferWriter` with :code:`printf` is straight forward by use of the sized string
format code if necessary (generally using C++ IO streams is a better choice). ::

   swoc::LocalBufferWriter<256> bw;
   bw.print("Failed to connect to {}", addr1);
   printf("%.*s\n", int(bw.size()), bw.data());

Alternatively the output can be null terminated in the formatting to avoid having to pass the size. ::

   swoc::LocalBufferWriter<256> bw;
   printf("%s\n", bw.print("Failed to connect to {}\0", addr1).data());

When using C++ stream I/O, writing to a stream can be done without any local variables at all. ::

   std::cout << swoc::LocalBufferWriter<256>().print("Failed to connect to {}", addr1)
             << std::endl;

If done repeatedly, a :code:`using` improves the look ::

   using LBW = swoc::LocalBufferWriter<256>;
   // ...
   std::cout << LBW().print("Failed to connect to {}", addr1) << std::endl;

This is handy for temporary debugging messages as it avoids having to clean up local variable
declarations later, particularly when the types involved themselves require additional local
declarations (such as in this example, an IP address which would normally require a local text
buffer for conversion before printing). As noted previously this is particularly useful inside a
:code:`case` where local variables are more annoying to set up.

Name Binding
============

The first part of each format specifier is a name. This was originally done to be more compliant
with Python formatting and is most commonly left blank, although sometimes it is used to format
arguments out of order or use them multiple times. To make this a more useful feature, |BWF|
supports :term:`name binding` which binds names to text generator functors. The generator is
expected to write output to a |BW| instance to replace the specifier, rather than a formatting
argument.

The base formatting logic is passed a functor by constant reference which provides the name binding
service. The functor is expected to have the signature ::

   unspecified_type (BufferWriter & w, bwf::Spec const& spec) const

As the format string is processed, if a format specifier has a name that is not numeric, the
formatting logic calls the functor, ignoring the return value (which can therefore be of any type,
including :code:`void`). :arg:`w` is the output buffer and :arg:`spec` is the specifier that caused
the functor to be invoked. The binding functor is expected to generate text in :arg:`w` in
accordance with the format specifier :arg:`spec`. Generally this involves looking up a functor based
on the name and calling that in turn to generate the text. The name for the binding is contained in
the :libswoc:`Spec::_name` member of :arg:`spec`.

The class :libswoc:`NameBinding` is provided as a base class for supporting name binding. It

*  Forces a virtual destructor.
*  Provides a pure virtual declaration to ensure the correct function operator is implemented.
*  Provides a standardized :libswoc:`"missing name" method <NameBinding::err_invalid_name>`.

This class is handy but not required.

|BWF| provides support for two use cases.

External Generators
-------------------

The first use case is for an "external generator" which generates text based on static or global
data. An example would be a "timestamp" generator which generates a timestamp based on the current
time. This could be associated with the name "timestamp" and used like

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 140

to generate output such as "Nov 16 12:21:05.545 Test Started".

Context Generators
------------------

The second is a "context generator" which generates text based on a context object. This use case
presumes a set of generators which access parts of a context object for text generation such that
the output of the generator depends on the state of the context object. For example, the context
object might be an HTTP request and the generators field accessors, each of which outputs the value
for a specific field of the request. Because the name is handed to the name binding object, an
implementation could subclass :libswoc:`ContextNames` and override the function operator to check the
name first against fields in the request, and only if that doesn't match, do a lookup for a
generator. :libswoc:`ContextNames` provides an implementation for storing and using name bindings.

Global Names
------------

The external name generator support is used to create a set of default global names. A global
singleton instance of an external name binding, :libswoc:`ExternalNames`, is used by default when
generating formatting output. Generators assigned to this instance are therefore available in the
default printing context. Here are a couple of examples for illustration of how this can be used.

A "timestamp" name was used as an example of a name useful to implement, so the example here will
start by doing that.

First, the generator is defined.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 71-83

This generates a time stamp with the month through seconds, dropping the leading year and clipping
everything past the seconds. It then adds milliseconds. Sample output looks like "Nov 16
11:40:20.833". This is then attached to the default global name binding in an initialization function called during process startup.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 125-132
   :emphasize-lines: 4

Because the test code is statically linked to the library, this must be done via a function called
from :code:`main` to be sure the library statics have been fully initialized. That taken care of,
using the global name is trivial.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 140

The output from a run is "Nov 16 12:21:05.545 Test Started". Note because this is a format
specifier, all of the supported format style works without additional work. That's not very useful
with a timestamp but consider printing the epoch time. Again, the generator is defined.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 85-89

The generator is then assigned to the name "now".

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 128-131
   :emphasize-lines: 2

And used with various styles.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 143

Sample output from a run is "Time is 1542393187 5bef0d63 5BEF0D63 0x5bef0d63".

Context Binding Example
-----------------------

Context name binding is useful for front ends to |BW|, not for direct use. The expected use case is
format string provided by an external agent, with format specifiers to pull data from a context
object where explicitly naming the context object isn't possible. As an example use case consider a
Traffic Server plugin that provides a cookie manipulation function. When setting a cookie value, it
is useful to access transaction specific data such as the URL, portions of the URL (e.g. the path),
HTTP field values, some other cookie item value, etc. This can be provided easily by setting up a
context binding which binds a request context, and binds the various names to the appropriate
elements in the context.

To start the example, a *very* simplified context will be used - it is
hardwired for comprehensibility, in production code the elements would be initialized for each
transaction.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 105-121

This holds the interesting information. Next up is a context name binding class that binds an
instance of :code:`Context`. This can be done with the template :libswoc:`ContextNames`. The
template class provides both a map of names to generators and the subclass of :libswoc:`NameBinding`
to pass to the formatter.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 150

For each supported name a function is defined to extract that data. For fields and cookies, the
extension will hold the field name and so the generator needs to look up the name from the extension
in the specifier. The field generators are done as local lambda functions. The other generators are
done as in place lambdas, since they simply pass a member of :code:`Context` to :code:`bwformat`. In
production code this might done with lambdas, or file scope functions, or via methods in
:code:`Context`. For writing the exmaple, lambdas were easiest and so those were used.

First the field generators, as those are more complex.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 158-165

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 167-174

:code:`NA` is a constant string used to indicate a missing field / cookie.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 67

With the field generators in place, time to hook up the generators. For the direct member ones, just define a lambda in place.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 177-187

In production code, :code:`cb` would be a process static, initialized at process start up, as the
relationship between the names and the generators doesn't change. Time to try it out.

This test gets the "YRP" field.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 189-190

This test reconstructs the URL without the query parameters.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 192-193

That's a minimalist approach, using as little additional code as possible. But it's a bit funky to
require the field names in the extension. There are various alternative approaches that could be
used. The one considered here is to do more parsing work to make it easier for the users, by making
the names more structured in the form "cookie.name" which means the value of the cookie element with
the name "name". The two implementations shown here were chosen to demonstrate features of |BWF|.

One type of implementation is to change how names are handled by the context binding (`example
<example-custom-name-dispatch>`__). Note the base formatting logic does not do name look, it only
passes the name (embedded in the specifier) to the binding. By subclassing the binding this lookup
can be intercepted and done differently, specifically by checking for names of the format "A.B" and
using A to select the table in which to lookup B. The other alternative is to change the parsing of
the format string so that a field name such as "{cookie.name}" is parsed as if it had been
"{cookie::name}" (`example <example-custom-parsing>`__). Both of these approaches require understanding
the core formatting logic and how to customize it, as explained in `Custom Formatting`.

Custom Formatting
=================

The internals of |BWF| are designed to enable using other format syntax. The one described in this
document is simply the one implemented by default. Any format which can be used to generate literal
output along with instances of :libswoc:`bwf::Spec` instances can be made to work. Along with
support for binding names, this makes it relatively easy to create custom format styles for use
in specialized applications, particularly with formatting user input, e.g. for user defined
diagnostic messages.

This starts with the :libswoc:`BufferWriter::print_nfv` method. This is the formatted output
implementation, all of the other variants serving as shims to call this method. The method has three
arguments.

:arg:`names`
   This is a container for bound names. If a specifier has a name that is not numeric, the specifier
   is passed to the name binding for output.

:arg:`ex`
   The :term:` format extractor`. This is a functor that detects end of input and extracts literals
   and specifiers. It has two required overloads and one optional.

   .. class:: Extractor

      .. function:: explicit operator bool () const

         :return: :code:`true` if there is more format string to process, otherwise :code:`false`.

      .. function:: bool operator () (std::string_view &literal, bwf::Spec &spec)

         :return: :code:`true` if a specifier was parsed and :arg:`spec` updated, otherwise :code:`false`.

         Extract the next literal and/or specifier. It may be assumed both :arg:`literal` and
         :arg;`spec` are initialized as if default constructed. If no literal is available
         :arg:`literal` should be unmodified, otherwise it should be set to the literal. If a specifier
         is found, :arg:`spec` must be updated to the parsed value of the specifier. If a specifier
         is found the method must return :code:`true` otherwise it must return :code:`false`. The
         method must always return at least one of :arg:`literal` or :arg:`spec` if the extractor
         is not empty.

      .. function:: void capture(BufferWriter & w, const bwf::Spec & spec, std::any && value)

         This is an optional method used to capture an argument. A pointer to the argument is placed
         in :arg:`value` with full type information. The method may generate output but this is
         not required. If this method is not present and the extractor returns a specifier with the
         type :libswoc:`Spec::CAPTURE_TYPE`, an exception will be thrown.

:arg:`args`
   A tuple containing the arguments to be formatted.

The formatting logic in :libswoc:`BufferWriter::print_nfv` is

.. uml::
   :align: center

   title Core Formatting

   start
   while (ex()) is (not empty)
     :ex(literal, spec);
     if (literal) then (not empty)
       :w.write(literal);
     endif
     if (spec) then (found)
       if (spec._name) then (numeric or empty)
         :format arg[spec];
       else
         :names(spec);
       endif
     endif

   endwhile (empty)

   stop

If the name in :arg:`spec` is not empty and not numeric, rather than selecting a member of :arg:`args`
the specifier is passed to the name binding, which presumably generates the appropriate output. The
name is embedded in the specifier :arg:`spec` in the :libswoc:`Spec::_name` member for use by the
name binding. Otherwise, an empty or numeric name means an argument is selected and passed to a
:code:`bwformat` overload, the specific overload selected based on the type of the argument.

For examples of this, the `Context Binding Example`_ will be redone in two different ways, each
illustrating a different approach to customizing output formatting.

.. _example-custom-parsing:

Parsing Example
---------------

For this case, the parsing of the format specifier is overridden and if the name is of the form "A.B"
it is treated as "A::B", that is "A" is put in the :arg:`_name` member and "B" is put in the :arg:`_ext`
member. Any extension is ignored. In addition, to act more like a Traffic Server plugin (and illustrate
how to use alternate specifier formats), the parser requires format specifiers to be of the form
"**%{**\ *name*\ **:**\ *style*\ **}**\ ". A double percent "%%" will mark a percent that is not
part of a format specifier.

The first step is to declare a class that will be the extractor functor.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 295-305

This will be used only as a temporary passed to :libswoc:`BufferWriter::print_nfv` and is therefore
always constructed with the format string. The format string left to parse is kept in :arg:`_fmt` which
means the empty check is really just a check on that.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 311-314

The function operator, which parses the format string to extract literals and specifiers, is a bit more
complex.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 316-352

The rough logic is

*  Search for a '%' - if not found, it's all literal, return that.

*  Make sure the '%' isn't '%%' - if it is, need to return just a literal with the leading '%' and
   skip the trailing '%', doing more parsing on the next call.

*  Check for an open brace, and if found find the close brace, then parse the internals into a
   specifier. Because the same style format as the default is used, the parser for :libswoc:`bwf::Spec`
   can be used. Otherwise if something different were needed that parsing logic would replace

   .. literalinclude:: ../../unit_tests/ex_bw_format.cc
      :lines: 336

*  If a specifier was found, check the name for a period. If found, split it and put the prefix in
   the name and the suffix in the extension.

   .. literalinclude:: ../../unit_tests/ex_bw_format.cc
      :lines: 341-346

A name binding

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 359-360

is declared and names are assigned in the usual way. In addition to assigning context related names,
external generators can also be assigned to the name binding, which can be a useful feature to
inject external names in addition to the context specific ones.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 395

After that, everything is ready to try it out.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 397-408

.. _example-custom-name-dispatch:

Name Binding Example
--------------------

Another approach is to override how name lookup is done in the binding. Because the field handling will be done in the override, methods are added to the :code:`Context` to do the generation for structured names, rather than placing that logic in the binding.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 204-225

Next a subclass of :libswoc:`ContextNames` is created which binds to a :code:`ExContext` object.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 229-232

Inside the class the function operator is overloaded to handle name look up.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 237-257

The incoming name is taken from the specifier and split on a period. If that yields a non-empty
result it is checked against the two valid structure names and the appropriate method on
:code:`ExContext` called to generate the output. Otherwise the normal name look up is done to find
the direct access generators.

An instance is constructed and the direct access names assigned

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 261-270

and it's time to try it out.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :lines: 272-288

This tests structured names, direct access names, external names ("version"), and some formatting.

C Style
-------

The formatting is sufficiently flexible to emulate C style or "printf" formatting. Given that a
major motivation for this work was the inadequacy of C style formatting, it's a bit odd to have this
example but it was done to show that even emulating :code:`printf`, it's still better. I must note
this, although this works reasonably well, it's still an example and not suitable for production
code. There are still some edge cases not handled, but as an proof of concept it's not worth fixing
every detail.

The first step is creating a format extractor, since the format string syntax is completley
different from the default. This is done by creating a class to perform the extraction and hold
state, although it will only be used as a temporary passed to :libswoc:`BufferWriter::print_nfv`.
The state is required to track "captured" arguments. These are used to emulate the '*' marker for
integers in format specifiers, which indicate their value is in an argument, not the format string.
This can be done both for maximum size and precision, so both of the must be capturable. The basic
logic is to keep a :libswoc:`bwf::Spec` in the class to hold the captured values, along with flags
indicating the capture state (it may be necessary to do two captures, if both the maximum size and
precision are variable).

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :start-at: class C_Format
   :end-before: // class C_Format

The empty indicator needs to be a bit different in that even if the format is empty, if the last
part of the format string had a capture (indicated by :arg:`_saved_p` being :code:`true`) a
non-empty state needs to be returned to get an invocation to output that last specifier.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :start-after: C_Format operator bool
   :end-before: C_Format operator bool

The capture logic takes advantage of the fact that only integers can be captured, and in fact
:code:`printf` itself requires exactly an :code:`int`. This logic is a bit more flexible, accepting
:code:`unsigned` and :code:`size_t` also, but otherwise is fairly restrictive. It should also
generate an error instead of silently returning on a bad type, but you can't have everything.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :start-after: C_Format capture
   :end-before: C_Format capture

The set up for the capture passes the capture element in the extension of the return specifier,
which this logic checks to know where to stash the captured value.

The actual parsing logic will be skipped - it's in the example file
:swoc:git:`src/unit_tests/ex_bw_format.cc` in the function operator method.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :start-after: C_Format parsing
   :end-before: {
   :lineno-match:

This handles all the basics of C style formatting including sign control, minimum and maximum
widths, precision, and leading radix support. One thing of note is that integer size indicators
(such as "l' in "%ld") are ignored - the type is known, therefore the sizing information is
redundant at best and wrong at worst, so it is parsed and discarded. If a capture is needed, state
is set the extrator instance and the specifier type is set to :libswoc:`bwf::Spec::CAPTURE_TYPE`
which will cause the formatting logic to call the extractor method :code:`capture` with the
corresponding argument. The specifier name is always empty, as strict in order processing is
mandatory.

Some example uses, along with verification of the results.

.. literalinclude:: ../../unit_tests/ex_bw_format.cc
   :start-after: C_Format tests
   :end-before: C_Format tests

Summary
-------

These example show that changing the format style and/or syntax can be done with relatively little
code. Even the C style formatting takes less than 100 lines of code to be mostly complete, even
though it can't take advantage of the parsing in :libswoc:`bwf::Spec` and handle captures. This
makes using |BWF| in existing projects with already defined syntax which is not the same as the
default a low hurdle to get over.

Design Notes
************

This is essentially my own work but I want to call out Uthira Mohan, who was there at the start of
what became |BWF|, a joint quicky project to play with variadic templates and formatting. This code
is based directly on that project, rather excessively extended, as is my wont. Alan Wang contributed
the floating point support, along with useful comments on the code and API while he was an intern.
Thanks, Uthira and Alan!

Type safe formatting has two major benefits -

*  No mismatch between the format specifier and the argument. Although some modern compilers do
   better at catching this at run time, there is still risk (especially with non-constant format
   strings) and divergence between operating systems such that there is no `universally correct
   choice <https://github.com/apache/trafficserver/pull/3476/files>`__. In addition the number of
   arguments can be verified to be correct which is often useful.

*  Formatting can be customized per type or even per partial type (e.g. :code:`T*` for generic
   :code:`T`). This enables embedding common formatting work in the format system once, rather than
   duplicating it in many places (e.g. converting enum values to names). This makes it easier for
   developers to make useful error messages. See :ref:`this example <bwf-http-debug-name-example>`
   for more detail.

As a result of these benefits there has been other work on similar projects, to replace
:code:`printf` a better mechanism. Unfortunately most of these are rather project specific and don't
suit the use case in Traffic Server. The two best options, `Boost.Format
<https://www.boost.org/doc/libs/1_64_0/libs/format/>`__ and `fmt <https://github.com/fmtlib/fmt>`__,
while good, are also not quite close enough to outweight the benefits of a version specifically
tuned for Traffic Server. ``Boost.Format`` is not acceptable because of the Boost footprint. ``fmt``
has the problem of depending on C++ stream operators and therefore not having the required level of
performance or memory characteristics. Its main benefit, of reusing stream operators, doesn't apply
to Traffic Server because of the nigh non-existence of such operators. The possibility of using C++
stream operators was investigated but changing those to use pre-existing buffers not allocated
internally was very difficult, judged worse than building a relatively simple implementation from
scratch. The actual core implementation of formatted output for :class:`BufferWriter` is not very
large - most of the overall work will be writing formatters, work which would need to be done in any
case but in contrast to current practice, only done once.

This code has under gone multiple large scale revisions, some driven by use (the most recent only
triggered by trying to write the examples in this document and finding some rough edges) and others
by a need for additional functionality (the format extractor support). I think it's close to its
final form and I am quite pleased with it. The most recent revisions to the alternate formatting
support have made it rather simple to retrofit this work in to existing / legacy applications. I do
expect to have some ongoing work on the documentation, which I consider currently basically a first
pass.

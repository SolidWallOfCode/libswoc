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

.. _swoc-errata:
.. highlight:: cpp
.. default-domain:: cpp
.. |Errata| replace:: :code:`Errata`

******
Errata
******

|Errata| is an error reporting class. The goals are

*  Very fast in the success case.
*  Easy to accumulate error messages in a failure case.

An assumption is that error handling is intrinsically expensive, in particular if errors are
reported or logged, therefore in the error case it is better to be comprehensive and easy to use.
Conversely, the success case should be optimized for performance as it is all overhead in that
case.

If errors are to be reported, they should be as detailed as possible. |Errata| supports this by
allowing error messages to accumulate. This means detailed error reports do not require passing
large amounts of context to generic functions so that errors there can be reported in context. The
lower level more generic functions can return generic errors while the calling functions can add
their higher level details to the error stack, passing these back to their callers. The end result
is both what exactly went wrong at the lowest level and the context in which the failure occurred.
E.g., for a failure to open a file, the file open logic can report the direct error (e.g.
"Permission denied") and the path, while the higher level function such as a configuration file
parser, can report it was the configuration file open that failed.

Usage
*****

The default |Errata| constructor creates an empty, successful state. An error state is created
by adding :libswoc:`Errata::Annotation`\s to the instance. This can be done in a number of ways.

The basic interface is provided by the :libswoc:`Errata::note` method. This adds a message along with
a :libswoc:`severity <Severity>` for the message. An instance is considered an error if the severity
is :libswoc:`Severity::WARN` or larger (as defined by :libswoc:`Errata::FAILURE_SEVERITY`).

For convenience the methods :libswoc:`Errata::diag`, :libswoc:`Errata::info`,
:libswoc:`Errata::warn`, :libswoc:`Errata::error` are provided which set take only a message and
supply the corresponding severity.

Formatting of message is done using BufferWriter formatting, making it straight forward to add
formatting for various types and thence making error messages simple to construct.

The template class :libswoc:`Rv` is provided to returning values and error messages together. This
class attaches an :code:`Errata` instance to an arbitrary type in a way that if the caller doesn't
use the :code:`Errata` it will still compile and run.

Each :code:`Errata` instance maintains its own memory arena for error messages so that multiple
messages are relatively inexpensive in the same instance. For this reason it is best to return
instances by `r-value reference <>`__.

See the :libswoc:`reference documentation <Errata>` for detailed class and method defintions.

In general the instance should be created at the lowest level and returned with additional messages
added as it is returned. This generall works better than passing in an instance.

Examples
========

For system calls, BufferWriter formatting has support for :code:`std::error_code` which can then be
passed on failure. Such as ::

  Errata parse_file(ts::file::path const file) {
     Errata errata;
     std::error_code ec;
     std::string content = swoc::file::load(file, ec);
     if (0 != ec) {
        return std::move(errata.error("Unable to open file - {}", ec));
     }
     // do other stuf.f
     return errata; // Empty Errata - success.
  }

Alternatively this can be done without declaring a local variable, and returning nameless instances. ::

  Errata parse_file(ts::file::path const file) {
     std::error_code ec;
     std::string content = swoc::file::load(file, ec);
     if (0 != ec) {
        return Errata().error("Unable to open file {} - {}", file, ec);
     }
     // do other stuf.f
     return {}; // Empty Errata - success.
  }

The caller can then heck the return value ::

   auto errata = parse_file(file_path);
   if (! errata.is_ok()) {
      return std::move(errata.info("While trying to open configuration file."));
   }
   // work with file.
Design Notes
************

I have carted around variants of this class for almost two decades, the evolution being driver
primarily by the evolving capabilities of C++ rather than a fundamental change in the design
philosophy. The original impetus was as noted in the introduction, to be able to generate a
very detailed and thorough error report on a failure, which as much context as possible. In some
sense, to have a stack trace without actually crashing. The original environment was in graphical
interface application where crashing was not acceptable, and error reports might come in from places
where additional debugging was not possible. Having a error stack was immensely valuable. In this
regard the original design has an error code as well as a severity so that every error could have
a unique identifier. This made it possible to track an error report to the precise source lines that
generated the problem, again a very valuable capability.

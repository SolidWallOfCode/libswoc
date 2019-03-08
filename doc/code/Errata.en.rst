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

An assumption is that error handling is always intrisincally expensive, in particular if errors are
reported or logged, therefore in the error case it is better to be comprehensive and easy to use.
Conversely, the success case should be optimized for performance as in it is all overhead in that
case.

If errors are to be reported, they should be as detailed as possible. |Errata| supports by allowing
error messages to accumulate. This means detailed error reports do not require passing large amounts
of context to generic functions so that errors there can be reported in context. Instead the calling
functions can add their local details to the error stack, passing these back to their callers. The
end result is both what exactly went wrong at the lowest level and the context in which the failure
occurred. E.g., for a failure to open a file, the file open logic can report the direct error
(e.g. "Permission denied") and the path, while the higher level function such as a configuration
file parser, can report it was the configuration file open that failed.

Definition
**********

.. class:: Errata

   :libswoc:`Reference documentation <Errata>`.

   .. function:: Errata & note(Severity level, std::string_view text)

      Add a message to this instance.

   .. class:: Annotation

      :libswoc:`Reference documentation <Errata::Annotation>`.

.. enum:: Severity

   :libswoc:`Reference documentation <Severity>`.

   This is an error severity level, based as always on the `syslog levels
   <http://man7.org/linux/man-pages/man3/syslog.3.html>`__. Currently a subset of those are
   supported.

   .. enumerator:: DIAG

      Internal diagnostics, essentially for debugging and verbose information. The lowest severity.

   .. enumerator:: INFO

      Information messages intended for end users, in contrast to :cpp:enumerator:`DIAG`
      which is for developers.

   .. enumerator:: WARN

      A warning, which indicates a failure or error that is recoverable but should be addressed.

   .. enumerator:: ERROR

      A error that prevents further action.

Usage
*****

The default |Errata| constructor creates an empty, successful state. An error state is created
by adding :class:`Errata::Annotation`\s to the instance. This can be done in a number of ways.

The basic interface is provided by the :func:`Errata::note` method. This adds a message along with
a severity for the message. An instance is considered an error

Examples
========

Design Notes
************

I have carted around variants of this class for almost two decades, the evolution being driver
primarily by the evolving capabilities of C++ rather than a fundamental change in the design
philosophy. The original impetus was as noted in the introduction, to be able to generate a
very detailed and thorough error report on a failure, which as much context as possible. In some
sense, to have a stack trace without actually crashing.

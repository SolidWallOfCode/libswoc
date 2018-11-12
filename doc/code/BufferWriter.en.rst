.. Licensed to the Apache Software Foundation (ASF) under one
   or more contributor license agreements.  See the NOTICE file
   distributed with this work for additional information
   regarding copyright ownership.  The ASF licenses this file
   to you under the Apache License, Version 2.0 (the
   "License"); you may not use this file except in compliance
   with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing,
   software distributed under the License is distributed on an
   "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
   KIND, either express or implied.  See the License for the
   specific language governing permissions and limitations
   under the License.

.. include:: ../common-defs.rst
.. highlight:: cpp
.. default-domain:: cpp
.. |BW| replace:: :code:`BufferWriter`

.. _BufferWriter:

BufferWriter
*************

Synopsis
++++++++

.. code-block:: cpp

   #include <swoc/BufferWriter.h>

Description
+++++++++++

.. class:: BufferWriter

   :libswoc:`Reference documentation <BufferWriter>`.

|BW| is intended to increase code reliability and reduce complexity in the common
circumstance of generating formatted output strings in fixed buffers. Current usage is a mixture of
:code:`snprintf` and :code:`memcpy` which provides a large scope for errors and verbose code to
check for buffer overruns. The goal is to provide a wrapper over buffer size tracking to make such
code simpler and less vulnerable to implementation error.

|BW| itself is an abstract class to describe the base interface to wrappers for
various types of output buffers. As a common example, :class:`FixedBufferWriter` is a subclass
designed to wrap a fixed size buffer. :class:`FixedBufferWriter` is constructed by passing it a
buffer and a size, which it then tracks as data is written. Writing past the end of the buffer is
clipped to prevent overruns.

Consider current code that looks like this.

.. code-block:: cpp

   char buff[1024];
   char * ptr = buff;
   size_t len = sizeof(buff);
   //...
   if (len > 0) {
     auto n = std::min(len, thing1_len);
     memcpy(ptr, thing1, n);
     len -= n;
   }
   if (len > 0) {
     auto n = std::min(len, thing2_len);
     memcpy(ptr, thing2, n);
     len -= n;
   }
   if (len > 0) {
     auto n = std::min(len, thing3_len);
     memcpy(ptr, thing3, n);
     len -= n;
   }

This is changed to

.. code-block:: cpp

   char buff[1024];
   ts::FixedBufferWriter bw(buff, sizeof(buff));
   //...
   bw.write(thing1, thing1_len);
   bw.write(thing2, thing2_len);
   bw.write(thing3, thing3_len);

The remaining length is updated every time and checked every time. A series of checks, calls to
:code:`memcpy`, and size updates become a simple series of calls to :func:`BufferWriter::write`.

For other types of interaction, :class:`FixedBufferWriter` provides access to the unused buffer via
:func:`BufferWriter::auxBuffer` and :func:`BufferWriter::remaining`. This makes it possible to easily
use :code:`snprintf`, given that :code:`snprint` returns the number of bytes written.
:func:`BufferWriter::fill` is used to indicate how much of the unused buffer was used. Therefore
something like (riffing off the previous example)::

   if (len > 0) {
      len -= snprintf(ptr, len, "format string", args...);
   }

becomes::

   bw.fill(snprintf(bw.auxBuffer(), bw.remaining(),
           "format string", args...));

By hiding the length tracking and checking, the result is a simple linear sequence of output chunks,
making the logic much eaier to follow.

Usage
+++++

The header files are divided in to two variants. :swoc:git:`include/tscore/BufferWriter.h` provides the basic
capabilities of buffer output control. :swoc:git:`include/tscore/BufferWriterFormat.h` provides the basic
:ref:`formatted output mechanisms <bw-formatting>`, primarily the implementation and ancillary
classes for :class:`BWFSpec` which is used to build formatters.

|BW| is an abstract base class, in the style of :code:`std::ostream`. There are
several subclasses for various use cases. When passing around this is the common type.

:class:`FixedBufferWriter` writes to an externally provided buffer of a fixed length. The buffer must
be provided to the constructor. This will generally be used in a function where the target buffer is
external to the function or already exists.

:class:`LocalBufferWriter` is a templated class whose template argument is the size of an internal
buffer. This is useful when the buffer is local to a function and the results will be transferred
from the buffer to other storage after the output is assembled. Rather than having code like::

   char buff[1024];
   ts::FixedBufferWriter bw(buff, sizeof(buff));

it can be written more compactly as::

   ts::LocalBufferWriter<1024> bw;

In many cases, when using :class:`LocalBufferWriter` this is the only place the size of the buffer
needs to be specified and therefore can simply be a constant without the overhead of defining a size
to maintain consistency. The choice between :class:`LocalBufferWriter` and :class:`FixedBufferWriter`
comes down to the owner of the buffer - the former has its own buffer while the latter operates on
a buffer owned by some other object. Therefore if the buffer is declared locally, use
:class:`LocalBufferWriter` and if the buffer is recevied from an external source (such as via a
function parameter) use :class:`FixedBufferWriter`.

Writing
-------

The basic mechanism for writing to a |BW| is :func:`BufferWriter::write`.
This is an overloaded method for a character (:code:`char`), a buffer (:code:`void *, size_t`)
and a string view (:code:`std::string_view`). Because there is a constructor for :code:`std::string_view`
that takes a :code:`const char*` as a C string, passing a literal string works as expected.

There are also stream operators in the style of C++ stream I/O. The basic template is

.. code-block:: cpp

   template < typename T > ts::BufferWriter& operator << (ts::BufferWriter& w, T const& t);

The stream operators are provided as a convenience, the primary mechanism for formatted output is
via overloading the :func:`bwformat` function. Except for a limited set of cases the stream operators
are implemented by calling :func:`bwformat` with the Buffer Writer, the argument, and a default
format specification.

Reading
-------

Data in the buffer can be extracted using :func:`BufferWriter::data`. This and
:func:`BufferWriter::size` return a pointer to the start of the buffer and the amount of data
written to the buffer. This is effectively the same as :func:`BufferWriter::view` which returns a
:code:`std::string_view` which covers the output data. Calling :func:`BufferWriter::error` will indicate
if more data than space available was written (i.e. the buffer would have been overrun).
:func:`BufferWriter::extent` returns the amount of data written to the |BW|. This
can be used in a two pass style with a null / size 0 buffer to determine the buffer size required
for the full output.

Advanced
--------

The :func:`BufferWriter::clip` and :func:`BufferWriter::extend` methods can be used to reserve space
in the buffer. A common use case for this is to guarantee matching delimiters in output if buffer
space is exhausted. :func:`BufferWriter::clip` can be used to temporarily reduce the buffer size by
an amount large enough to hold the terminal delimiter. After writing the contained output,
:func:`BufferWriter::extend` can be used to restore the capacity and then output the terminal
delimiter.

.. warning:: **Never** call :func:`BufferWriter::extend` without previously calling :func:`BufferWriter::clip` and always pass the same argument value.

:func:`BufferWriter::remaining` returns the amount of buffer space not yet consumed.

:func:`BufferWriter::auxBuffer` returns a pointer to the first byte of the buffer not yet used. This
is useful to do speculative output, or do bounded output in a manner similar to using
:func:`BufferWriter::clip` and :func:`BufferWriter::extend`. A new |BW| instance
can be constructed with

.. code-block:: cpp

   ts::FixedBufferWriter subw(w.auxBuffer(), w.remaining());

or as a convenience ::

   ts::FixedBuffer subw{w.auxBuffer()};

Output can be written to :arg:`subw`. If successful, then :code:`w.fill(subw.size())` will add that
output to the main buffer. Depending on the purpose, :code:`w.fill(subw.extent())` can be used -
this will track the attempted output if sizing is important. Note that space for any terminal
markers can be reserved by bumping down the size from :func:`BufferWriter::remaining`. Be careful of
underrun as the argument is an unsigned type.

If there is an error then :arg:`subw` can be ignored and some suitable error output written to
:arg:`w` instead. A common use case is to verify there is sufficient space in the buffer and create
a "not enough space" message if not. E.g. ::

   ts::FixedBufferWriter subw{w.auxWriter()};
   this->write_some_output(subw);
   if (!subw.error()) w.fill(subw.size());
   else w.write("Insufficient space"sv);

Examples
++++++++

For example, error prone code that looks like

.. code-block:: cpp

   char new_via_string[1024]; // 512-bytes for hostname+via string, 512-bytes for the debug info
   char * via_string = new_via_string;
   char * via_limit  = via_string + sizeof(new_via_string);

   // ...

   * via_string++ = ' ';
   * via_string++ = '[';

   // incoming_via can be max MAX_VIA_INDICES+1 long (i.e. around 25 or so)
   if (s->txn_conf->insert_request_via_string > 2) { // Highest verbosity
      via_string += nstrcpy(via_string, incoming_via);
   } else {
      memcpy(via_string, incoming_via + VIA_CLIENT, VIA_SERVER - VIA_CLIENT);
      via_string += VIA_SERVER - VIA_CLIENT;
   }
   *via_string++ = ']';

becomes

.. code-block:: cpp

   ts::LocalBufferWriter<1024> w; // 1K internal buffer.

   // ...

   w.write(" ["sv);
   if (s->txn_conf->insert_request_via_string > 2) { // Highest verbosity
      w.write(incoming_via);
   } else {
      w.write(std::string_view{incoming_via + VIA_CLIENT, VIA_SERVER - VIA_CLIENT});
   }
   w.write(']');

There will be no overrun on the memory buffer in :arg:`w`, in strong contrast to the original code.
This can be done better, as ::

   if (w.remaining() >= 3) {
      w.clip(1).write(" ["sv);
      if (s->txn_conf->insert_request_via_string > 2) { // Highest verbosity
         w.write(incoming_via);
      } else {
         w.write(std::string_view{incoming_via + VIA_CLIENT, VIA_SERVER - VIA_CLIENT});
      }
      w.extend(1).write(']');
   }

This has the result that the terminal bracket will always be present which is very much appreciated
by code that parses the resulting log file.

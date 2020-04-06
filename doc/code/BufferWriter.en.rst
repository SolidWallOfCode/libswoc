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

.. _BufferWriter:

************
BufferWriter
************

Synopsis
********

:code:`#include "swoc/BufferWriter.h"`

.. class:: BufferWriter

   :libswoc:`Reference documentation <BufferWriter>`.

.. class:: FixedBufferWriter

   :libswoc:`Reference documentation <FixedBufferWriter>`.

.. class:: template < uintmax_t N > LocalBufferWriter

   :libswoc:`Reference documentation <LocalBufferWriter>`.

|BW| is designed for use in the common circumstance of generating formatted output strings in fixed
buffers. The goal is to replace usage that is a mixture of :code:`snprintf`, :code:`strcpy`, and
:code:`memcpy`. |BW| automates buffer size checking and clipping for better reliability.

|BW| itself is an abstract class to describe the base interface to wrappers for various types of
output buffers. As a common example, :libswoc:`FixedBufferWriter` is a subclass that wraps a fixed
size buffer. An instance is constructed by passing it a buffer and a size, which it then tracks as
data is written. Writing past the end of the buffer is automatically clipped to prevent overruns.

|BW| tracks two sizes - the actual amount of space used in the buffer and the amount of data
written. The former is bounded by the buffer size to prevent overruns. The latter provides a
mechanism both to detect the clipping done to prevent overruns, and the amount of space needed to
avoid it.

Usage
*****

Consider a common case of code like ::

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

This is changed to::

   char buff[1024];
   swoc::FixedBufferWriter bw(buff, sizeof(buff));
   //...
   bw.write(thing1, thing1_len).
   bw.write(thing2, thing2_len);
   bw.write(thing3, thing3_len);

Or even more compactly with :libswoc:`LocalBufferWriter` ::

   swoc::LocalBufferWriter<1024> bw;
   // ...
   bw.write(thing1, thing1_len).write(thing2, thing2_len).write(thing3, thing3_len);

For every call to :libswoc:`BufferWriter::write` the remaining length is updated and checked,
discarding any overrun. This replaces the need to write the checks explictly on every :code:`memcpy`.
Usually, however, |BW| will do the needed space checks that were not done at all previously.

A similar mechanism works for :code:`snprintf`::

   if (count < sizeof(buff)) {
      count += snprintf(buff + count, sizeof(buff) - count, "blah blah", arg1, ...);
   }

vs. ::

   bw.commit(snprintf(bw.aux_data(), bw.remaining(), "blah blah", arg1, ...));

As before, the buffer limits are updated and checked, discarding as needed. Although
:code:`snprintf` can be used in this way, |BW| provides its own `print formatting <bw-format>`_
which is more flexible and powerful than C style printing.

|BW| itself is an abstract class and can't be constructed. Use is provided through concrete
subclasses.

:class:`FixedBufferWriter`
   This operates on an externally defined buffer of a fixed size. The constructor requires providing
   the start and size of the buffer. Output is limited to that buffer.

:class:`LocalBufferWriter`
   This is a template class which takes a single size argument. An internal buffer of that size is
   made part of the instance and used as the output buffer.

:class:`FixedBufferWriter` is used where the buffer is pre-existing or externally supplied. If the
buffer is only accessed by the output generation then :class:`LocalBufferWriter` is more convenient,
eliminating the need to separately declare the buffer. It also makes :class:`LocalBufferWriter`
usable in line, such as ::

   std::cout << swoc::LocalBufferWriter<1024>{}.write(...).write(...).view();

which writes output to the |BW| instance, then gets a view of the content which is written to
:code:`std::cout`.

Writing
=======

The primary method for sending output to a |BW| is :libswoc:`BufferWriter::write`. This is an
overloaded method with overloads for a character (:code:`char`), a buffer (:code:`void *, size_t`),
or a string view (:code:`std::string_view`). This covers literal strings and C-style strings because
both of those implicitly convert to :code:`std::string_view`. For :code:`snprintf` style support,
see `buffer writer formatting <bw-format>`_.

Reading
=======

Data in the buffer can be extracted using :libswoc:`BufferWriter::data`, along with
:libswoc:`BufferWriter::size`. Together these return a pointer to the start of the buffer and the
amount of data written to the buffer. The same result can be obtained with
:libswoc:`FixedBufferWriter::view` which returns a :code:`std::string_view` which covers the output
data.

Calling :libswoc:`BufferWriter::error` will indicate if more data than space available was written
(i.e. the buffer would have been overrun). :libswoc:`BufferWriter::extent` returns the amount of
data written to the |BW|. This can be used in a two pass style with a null / size 0 buffer to
determine the buffer size required for the full output.

Advanced
========

The :libswoc:`BufferWriter::restrict` and :libswoc:`BufferWriter::restore` methods can be used to
require space in the buffer. A common use case for this is to guarantee matching delimiters in
output if buffer space is exhausted. :libswoc:`BufferWriter::restrict` can be used to temporarily
reduce the buffer capacity by an amount large enough to hold the terminal delimiter. After writing
the contained output, :libswoc:`BufferWriter::restore` can be used to restore the capacity and then
output the terminal delimiter. E.g. ::

   w.restrict(1);
   w.write('[');
   /// other output to w.
   w.restore(1).write(']'); // always works, even if buffer was overrrun.

.. warning::

   :libswoc::`BufferWriter::restore` can only restore capacity that was removed by
   :libswoc:`BufferWriter::restrict`. It can **not** make the capacity larger than it was
   originally.

As something of an alternative it is easy to do "speculative" output.
:libswoc:`BufferWriter::aux_data` returns a pointer to the first byte of the buffer not yet used,
and :libswoc:`BufferWriter::remaining` returns the amount of buffer space not yet consumed. These
can be easily used to create a new :libswoc:`FixedBufferWriter` on the unused space ::

   ts::FixedBufferWriter subw(w.aux_data(), w.remaining());

Output can be written to :arg:`subw`. If successful :libswoc:`BufferWriter::commit` can be used to
add that output to the original buffer :arg:`w` ::

   w.commit(subw.size());

If there is an error :arg:`subw` can be discarded and some suitable error output written to :arg:`w`
instead. A common use case is to verify there is sufficient space in the buffer and create a "not
enough space" message if not. E.g. ::

   ts::FixedBufferWriter subw{w.aux_data(), w.remaining()};
   write_some_output(subw);
   if (!subw.error()) w.commit(subw.size());
   else w.write("[...]");

While this can be used in a manner similar to using :libswoc:`BufferWriter::restrict` and
:libswoc:`BufferWriter::restore` by subtracting from :libswoc:`BufferWriter::remaining`, this can be
a bit risky because the return value is unsigned and underflow would be problematic.

Examples
========

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

   w.write(" [");
   if (s->txn_conf->insert_request_via_string > 2) { // Highest verbosity
      w.write(incoming_via);
   } else {
      w.write(std::string_view{incoming_via + VIA_CLIENT, VIA_SERVER - VIA_CLIENT});
   }
   w.write(']');

There will be no overrun on the memory buffer in :arg:`w`, in strong contrast to the original code.
This can be done better, as ::

   if (w.remaining() >= 3) {
      w.restrict(1).write(" [");
      if (s->txn_conf->insert_request_via_string > 2) { // Highest verbosity
         w.write(incoming_via);
      } else {
         w.write(std::string_view{incoming_via + VIA_CLIENT, VIA_SERVER - VIA_CLIENT});
      }
      w.restore(1).write(']');
   }

This has the result that the terminal bracket will always be present which is very much appreciated
by code that parses the resulting log file.

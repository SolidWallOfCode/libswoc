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

.. _manual-toc:

*****************
Solid Wall of C++
*****************

The Solid Wall of C++ library is a collection of C++ classes and utilities. This code evolved out of
infrastructure used in `Apache Traffic Server <https://trafficserver.apache.org>`__. The utilities
had become useful enough there were requests to be able to use them in ATS plugins and other,
unrelated projects. Hence this library. I hope you find it as useful as I have.

.. toctree::
   :maxdepth: 1

   preface.en
   code/BufferWriter.en
   code/BW_Format.en
   code/meta.en
   code/MemSpan.en
   code/TextView.en
   code/MemArena.en
   code/IntrusiveDList.en
   code/IntrusiveHashMap.en
   code/Scalar.en
   code/Lexicon.en
   code/Errata.en
   code/IPSpace.en

Indices and tables
******************

* :ref:`genindex`
* :ref:`search`

Glossary
********

.. glossary::

   name binding
      A map of names to formatted output generators.

   format context
      A value used by a :term:`name binding` to provide context for the formatted output.

   format string
      A string that describes formatted output.

   format specifier
      An element in a :term:`format string` which is replaced by formatted output of an argument.

   scalar
      A scaled /quantized integral value.

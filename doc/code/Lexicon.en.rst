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

.. _swoc-lexicon:
.. highlight:: cpp
.. default-domain:: cpp
.. |Lexicon| replace:: :code:`Lexicon`

****************
Lexicon
****************

|Lexicon| is a bidirectional mapping between strings and a numeric / enumeration type. It is intended
to support parsing and diagnostics for enumerations. It has some significant advantages over a simple
array of strings.

*  The integer can be looked up by string. This makes parsing much easier and more robust.
*  The integers do not have to be contiguous or zero based.
*  Multiple names can map to the same integer.
*  Defaults for missing names or integers.

Definition
**********

.. class:: template < typename E > Lexicon

   :libswoc:`Reference documentation <Lexicon>`.

Usage
*****

Lexicons can be used in a dynamic or static fashion. The basic use is as a static translation object
that converts between an enumeration and names. The constructors allow setting up the entire Lexicon.

Examples
========



Design Notes
************


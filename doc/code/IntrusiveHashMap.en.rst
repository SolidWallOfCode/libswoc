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

.. _swoc-intrusive-hashmap:
.. highlight:: cpp
.. default-domain:: cpp
.. |IHM| replace:: :code:`IntrusiveHashMap`

****************
IntrusiveHashMap
****************

|IHM| is a hash map that requires links to be explicitly embedded in the contained items.

Definition
**********

.. class:: template < typename L > IntrusiveHashMap

   :libswoc:`Reference documentation <IntrusiveHashMap>`.

Usage
*****


Examples
========

Design Notes
************

The historic goal of this class is to replace the :code:`DLL` list support in Traffic Server. The
benefits of this are

*  Remove dependency on the C preprocessor.

*  Provide greater flexibility in the internal link members. Because of the use of the descriptor
   and its static methods, the links can be anywhere in the object, including in nested structures
   or super classes. The links are declared like normal members and do not require specific macros.

*  Provide STL compliant iteration. This makes the class easier to use in general and particularly
   in the case of range :code:`for` loops.

*  Track the number of items in the list.

*  Provide queue support, which is of such low marginal expense there is, IMHO, no point in
   providing a separate class for it.




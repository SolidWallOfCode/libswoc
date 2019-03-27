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

.. default-domain:: cpp
.. highlight:: cpp

********
IP Space
********

Synopsis
********

:code:`#include <swoc/swoc_ip.h>`

.. class:: IPEndpoint

   :libswoc:`Reference documentation <swoc::IPEndpoint>`.

This library is for storing and manipulating IP addresses as data. It has no support for actual
network operations.

Usage
*****

History
*******

This is based (loosely) on the :code:`IpMap` class in Apache Traffic Server, which in turn is based
on IP addresses classes developed by Network Geographics for the Infosecter product. The code in
Apach Traffic Server was a much simplified version of the original work and this is a reversion
to that richer and more complete set of classes and draws much of its structure from the Network
Geographics work directly.

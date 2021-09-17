.. Licensed to the Apache Software Foundation (ASF) under one or more contributor license
   agreements. See the NOTICE file distributed with this work for
   additional information regarding copyright ownership. The ASF licenses this file to you under the
   Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
   the License. You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software distributed under the License
   is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
   or implied. See the License for the specific language governing permissions and limitations under
   the License.

.. include:: common-defs.rst

.. _appendix:

Appendix
********

ATS Correspondence
==================

This library began as a set of classes in `Apache Traffic Server<https://trafficserver.apache.org>`__.
It has continued to be driven primarily by usefulness in writing ATS core and plugin code. For that
reason it should be incorporated back in to ATS as a replacement for existing classes.

================ =================== =======================================
libswoc          Traffic Server
================ =================== =======================================
BufferWriter     BufferWriter        Significant upgrades
Errata           Errata              Significant upgrades
IntrusiveDList   IntrusiveDList      Almost identical
IntrusiveHashMap IntrusiveHashMap    Almost identical
IpAddr           IpAddr              Significant changes
IPSpace          IpMap               Massive upgrade
MemSpan          MemSpan             Almost identical
MemArena         Arena               Complete rewrite
Scalar           Scalar              Almost identical
TextView         TextView            Significant upgrades
Lexicon          *
================ =================== =======================================

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

.. _preface:

Preface
*******

The Solid Wall of C++ library is a collection of C++ classes and utilities. This code evolved out of infrastructure used in `Apache Traffic Server <https://trafficserver.apache.org>`__. The utilities had become useful enough there were requests to be able to use them in ATS plugins and other, unrelated projects. Hence this library. I hope you find it as useful as I have.

Because this code is used inside an Apache Software Foundation project, it carries the ASF copyright. It is not, however, officially affiliated with the ASF. This is my personal project which I am pleased to share with the ASF and anyone else.

Typographic Conventions
=======================

This documentation uses the following typographic conventions:

Italic
    Used to introduce new terms on their initial appearance.

    Example:
        A :term:`Scalar` is an integral value that is always a multiple of a fixed constant.

Monospace
    Represents C/C++ language statements, commands, file paths, file content,
    and computer output.

    Example:
        The default library name is ``libswoc++``.

Bracketed Monospace
    Represents variables for which you should substitute a value in file content
    or commands.

    Example:
        Use ``test_libswoc <test>`` to run specific unit tests..

Ellipsis
    Indicates the omission of irrelevant or unimportant information.

Other Resources
===============

Websites
--------

Apache Traffic Server
    https://trafficserver.apache.org/

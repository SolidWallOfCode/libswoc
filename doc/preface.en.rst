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

.. _preface:

Preface
*******

The Solid Wall of C++ library is a collection of C++ classes and utilities that I have written find
useful. You might find them useful as well. All of these are used in the Apache Traffic Server
codebase, which is where I first developed them, along with some contributions. I have split them
out in to this side project in order to

*  Make them available to non-Traffic Server projects.

*  Have a venue where I can do more rapidly development than is possible inside Traffic Server.

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

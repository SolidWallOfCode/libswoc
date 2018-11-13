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

Because this code is used inside an Apache Software Foundation project, it carries the ASF
copyright. It is not, however, officially affiliated with the ASF. This is my personal project which
I am pleased to share with the ASF and anyone else.

The unit testing coding is divided in to two types. Files that start with "test\_..." are the core
unit tests. Files that start with "ex\_..." are unit tests that exist to provide example code for
the documentation. Therefore some of the constructs or arrangement of code in the example files will
look a bit odd as straight up unit tests. This also means that changing code in any example file
will likely require updating the documentation, which includes code from those files by line number.
This is the primary reason for the split, so that the real unit tests can be updated without concern
for breaking the documentation examples. I wanted the example code in the unit tests in order to
verify that it compiles and runs. This helps keep the documentation more up to date, particularly
if there are API changes.

Typographic Conventions
=======================

This documentation uses the following typographic conventions:

Italic
    Used to introduce new terms on their initial appearance.

    Example:
        A :term:`scalar` is an integral value that is always a multiple of a fixed constant.

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

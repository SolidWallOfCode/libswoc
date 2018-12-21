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
.. highlight:: cpp
.. default-domain:: cpp
.. |MemArena| replace:: :code:`MemArena`

.. _swoc-mem-arena:

********
MemArena
********

Synopsis
********

:code:`#include <swoc/MemArena.h>`

.. class:: template < typename T > MemArena

   :libswoc:`Reference documentation <MemArena>`.

|MemArena| provides a memory arena or pool for allocating memory. Internally |MemArena| reserves
memory in large blocks and allocates pieces of those blocks when memory is requested. Upon
destruction all of the reserved memory is released which also destroys all of the allocated memory.
This is useful when the goal is any (or all) of

*  amortizing allocation costs for many small allocations.
*  better memory locality for containers.
*  de-allocating memory in bulk.

Note that intrusive containers such as :ref:`IntrusiveDList <swoc-intrusive-list>` and
:ref:`IntrusiveHashMap <swoc-intrusive-hashmap>` work well with a |MemArena|. If the container and its elements are placed
in the |MemArena| then no specific cleanup is needed beyond destroying the |MemArena|.

A |MemArena| can also be `inverted <arena-inversion>`_. This means placing the |MemArena| instance
in its own memory pool so that the |MemArena| and associated objects can be created with a single
base library memory allocation and cleaned up with a single :code:`delete`.

Usage
*****

When a |MemArena| instance is constructed no memory is reserved. A hint can be provided so that the
first internal reservation of memory will have close to but at least that amount of free space
available to be allocated.

In normal use memory is allocated from |MemArena| using :libswoc:`MemArena::alloc` to get chunks
of memory, or :libswoc:`MemArena::make` to get constructed class instances. :libswoc:`MemArena::make`
takes an arbitrary set of arguments which it attempts to pass to a constructor for the type
:code:`T` after allocating memory (:code:`sizeof(T)` bytes) for the object. If there isn't enough
free reserved memory, a new internal block is reserved. The size of the new reserved memory will be at least
the size of the currently reserved memory, making each reservation larger than the last.

The arena can be **frozen** using :libswoc:`MemArena::freeze` which locks down the currently reserved
memory and forces the internal reservation of memory for the next allocation. By default this
internal reservation will be the size of the frozen allocated memory. If this isn't the best value a
hint can be provided to the :libswoc:`MemArena::freeze` method to specify a different value, in the
same manner as the hint to the constructor. When the arena is thawed (unfrozen) using
:libswoc:`MemArena::thaw` the frozen memory is released, which also destroys the frozen allocated
memory. Doing this can be useful after a series of allocations, which can result in the allocated
memory being in different internal blocks, along with possibly no longer in use memory. The result
is to coalesce (or garbage collect) all of the in use memory in the arena into a single bulk
internal reserved block. This improves memory efficiency and memory locality. This coalescence is
done by

#. Freezing the arena.
#. Copying all objects back in to the arena.
#. Thawing the arena.

Because the default reservation hint is large enough for all of the previously allocated memory, all
of the copied objects will be put in the same new internal block. If this for some reason this
sizing isn't correct a hint can be passed to :libswoc:`MemArena::freeze` to specify a different value
(if, for instance, there is a lot of unused memory of known size). Generally this is most useful for
data that is initialized on process start and not changed after process startup. After the process
start initilization, the data can be coalesced for better performance after all modifications have
been done. Alternatively, a container that allocates and de-allocates same sized objects (such as a
:code:`std::map`) can use a free list to re-use objects before going to the |MemArena| for more
memory and thereby avoiding collecting unused memory in the arena.

Other than a freeze / thaw cycle, there is no mechanism to release memory except for the destruction
of the |MemArena|. In such use cases either wasted memory must be small enough or temporary enough
to not be an issue, or there must be a provision for some sort of garbage collection.

Generally |MemArena| is not as useful for classes that allocate their own internal memory (such as
:code:`std::string` or :code:`std::vector`), which includes most container classes. Intrusive
container classes, such as :class:`IntrusiveDList` and :class:`IntrusiveHashMap`, are more useful
because the links are in the instance and therefore also in the arena.

Objects created in the arena must not have :code:`delete` called on them as this will corrupt
memory, usually leading to an immediate crash. The memory for the instance will be released when the
arena is destroyed. The destructor can be called if needed but in general if a destructor is needed
it is probably not a class that should be constructed in the arena. Looking at
:class:`IntrusiveDList` again for an example, if this is used to link objects in the arena, there is
no need for a destructor to clean up the links - all of the objects will be de-allocated when the
arena is destroyed. Whether this kind of situation can be arranged with reasonable effort is a good
heuristic on whether |MemArena| is an appropriate choice.

While |MemArena| will normally allocate memory in successive chunks from an internal block, if the
allocation request is large (more than a memory page) and there is not enough space in the current
internal block, a block just for that allocation will be created. This is useful if the purpose of
|MemArena| is to track blocks of memory more than reduce the number of system level allocations.

Generally the |MemArena| will be declared as a member variable of the the class that needs the
local memory. The class destructor will then clean up the memory when the containing class is
destructed. An archetypical example is :class:`Lexicon` which uses a |MemArena| to store its
data. :class:`IntrusiveHashMap` instances are used to track the data which is owned by the arena
which in turn is owned by the :code:`Lexicon` instance. Destroying the :code:`Lexicon` releases
all of the memory. There is no explicit cleanup code in :code:`Lexicon` precisely because the
|MemArena| destructor does that.

Temporary Allocation
====================

In some situations it is useful to allocate memory temporarily. While :code:`alloca` can be used
for this, that can be a bit risky because the stack is generally much smaller than the heap and
may be heavily used. |MemArena| provides the :libswoc:`MemArena::require` method as an alternative.

|MemArena| allocates memory in blocks and then hands out pieces of these blocks when it is asked for
memory. Therefore it is usually the case that some amount of not currently used memory is available
in the active block. Access to this memory can be obtained via :libswoc:`MemArena::remnant`.  This
may be large enough for use as the needed temporary memory, in which case no allocation needs to be
done. What :libswoc:`MemArena::require` does is guarantee the remnant is at least the specified
size. If it is already large enough, nothing is done, otherwise a new block is allocated such that
it has enough free space. If this is done multiple times without allocating from the arena, the same
memory space is reused. It is also guaranteed that if the next call to :libswoc:`MemArena::alloc` or
:libswoc:`MemArena::make` does not need more than the remnant, it will be allocated from the
remnant. This makes it possible to do speculative work in the arena and "commit" it (via allocation)
after the work is successful, or abandon it if not.

Examples
========

It is expected that |MemArena| will be used most commonly for string storage, where a container needs
strings and can't guarantee the lifetime of strings in arguments. A common function that is used is
a "localize" function that takes a string, allocates a copy in the arena, and returns a view of it. E.g.

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 37-43

This can then be used to create a view inside the arena.

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 187-188

That's about it for putting strings in the arena. Now, consider a class to be put in an arena.

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 147-151,155-164, 185

A key point to note is :arg:`name` is a view, not a :code:`std::string`. This means it requires
storage elsewhere, i.e. in the arena. The string :arg:`text` has already been localized so instances
can be created safely

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 190-196

All of these are in the arena and are de-allocated when the arena is cleared or destructed. Note a
literal constant can be safely used because it has a process life time. That's a bit obscure - in
actual use it's unlikely the API will know this and will localize such strings. Also note
:libswoc:`MemArena::make` can use any of the constructors in :code:`Thing`.

Another case for string storage is formatted strings. Naturally :class:`BufferWriter` will be used
because it is just so awesome.  The :libswoc:`MemArena::remnant` method will also be used to do
speculative formatted output. A problem is that at the time the formatted is provided, the final
size is not known, and so (in general) two passes are needed, one to size and one to do the actual
output. This can be optimized somewhat by using :libswoc:`MemArena::remnant`. The concept is to do
the sizing pass in to the space. If it fits, win - the string is there and no more needs to be done.
Otherwise the size needed is known and :libswoc:`MemArena::require` is used  to get the contiguous
memory needed for the string. :libswoc:`MemArena::alloc` is then used to allocate the already
initialized memory.

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 201-208

The code is a bit awkward and repetitious due to being example code. Production code would look
something like

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 132-143

Because this is a function there's no more repetition of the format string and arguments in use.

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 210-212
   :emphasize-lines: 1

.. _arena-inversion:

Inversion
---------

"Inversion" is the technique of putting the |MemArena| inside its own arena. For collections of
objects which all have the same lifetime this makes cleanup simple, as only the |MemArena| needs to
be destructed. In addition, it can minimize calls to the base allocator. If a good guess as to the
total size needed can be made, all of the objects, including the arena, can be provided for in a
single call to :code:`malloc`. This is used in :class:`Errata` for performance reasons. Here is a
simplified example.

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 65-67

Things of note:

*  A local stack variable is used to bootstrap the |MemArena|.
*  A new |MemArena| is created in the |MemArena| and the essence of the local arena moved there.
*  This works even if data has already been allocated in the arena, but it's best to do first.
*  The resulting pointer is cleaned up by calling the destructor, not by :code:`delete`. This is
   because, as noted earlier, it is not valid to call :code:`delete` on an object in the arena.
*  The destructor is carefully implemented to work correctly even if the class instance is in
   its own internal memory blocks.

In actual use, the pointer would be a class member, not another local variable, and the inversion
delayed until the memory was actually needed so that if the arena isn't needed, no allocation is done.

Although :code:`delete` cannot be used on an inverted |MemArena|, it is still possible to use
code:`std::unique_ptr` by overriding the cleanup.

Now for the actual example. This uses the :code:`localize` function from the previous example.

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 79-102
   :emphasize-lines: 1,10
   :linenos:

In this code, :arg:`ta` is the local bootstrap temporary. The :code:`std::unique_ptr` is declared to
take as a "deleter" a function taking a :code:`MemArena *`. The actual function is the lambda in
:arg:`destroyer` declared in line 1. This could have been done directly inline, such as

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 118-121

but I think separating it is better looking code.

The checking code shows that memory can be allocated in the arena first, but is not in the arena
after the inversion. It is, however, in the inverted arena at the same address as demonstrated by
line 16. All of the allocated memory involved gets cleaned up when the :code:`std::unqiue_ptr` is
destructed.

Inversion is a bit tricky to understand but that's all there is to writing code to use it, although
setting up the :code:`std::unique_ptr` can be done in a variety of ways. Although these are
functionally identical, the memory footprint varies as noted in the comments. This can make a
difference for objects that are intended to be very small if no memory is allocated.

Two styles of lambda use have been shown, but there is one more of interest.

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 125-128

This requires the separate declaration of the lambda but cuts the :code:`std::unique_ptr` size in half. Note
every lambda is a distinct type, *even if the code is character for character identical*. This means the
pointer **must** be passed :arg:`destroyer` - there is no other object that will be valid. Despite this,
it must be passed as an argument, it cannot be omitted. But, hey, it saves 8 bytes!

Instead of a lambda, one can define a function

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 53-57

and use that

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 112-114

Finally one can define a functor

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 45-51

and use that

.. literalinclude:: ../../src/unit_tests/ex_MemArena.cc
   :lines: 106-108

Note in this case the :code:`std::unqiue_ptr` is only 8 bytes (a single pointer) and doesn't
require an argument to the constructor.

Internals
*********

Allocated memory is tracked by two linked lists, one for current memory and the other for frozen
memory. The latter is used only while the arena is frozen. The list of blocks is maintained by
:ref:`IntrusiveDList <swoc-intrusive-list>` which means deallocating the memory blocks suffices to
clean up. Memory blocks are kept in an active pool so that if a new block is allocated to satisfy a
particular allocation or requirement, unused space in previous blocks is still available for use.

The uses-allocator repository
=============================

This repository contains two C++ Standards proposals and their sample
implementations. The two proposals both have to do with allocators, but are
otherwise pretty loosely coupled.  They exist in the same directory just
because it was convenient at the time.  That may change in the future.

P0591 Utility functions to implement uses-allocator construction
----------------------------------------------------------------

This paper proposes a set of utility functions to make it more convenient to
implement allocator-aware containers.  These functions can also make the
standard more readable by centralizing the logic for constructing objects
using allocators, including `std::pair`, which does not use an allocator
directly, but which may contain members that use allocators.

 o `P0591-uses-allocator.md`: This is the text of the paper, in Pandoc
   Markdown format.

 o `uses_allocator.h`: Implementation of facilities proposed in P0591
   
 o `uses_allocator.t.cpp`: Test driver for `uses_allocator.h`.

 o `Makefile`: Type `make uses_allocator` to build and run the test driver.

 o `test_assert.h`: Utility macros used in test drivers.

 o `make_from_tuple.h`: Implementation of C++17 `make_from_tuple` function and
   C++ `apply` function.

 o `make_from_tuple.t.cpp`: Test driver for `make_from_tuple`.

P0208 Copy-swap helper functions
--------------------------------

This paper proposes utility functions that more cleanly express the copy-swap
idiom used for exception safety and make the idioms work correctly with
different allocator propagation traits.

 o `P0208-copy_swap_helper.docx`: Text of the paper, in MS Word format.

 o `copy_swap_helper.h`: Implementation of the proposed facilities

 o `copy_swap_helper.t.cpp`: Test driver for `copy_swap_helper.h`.

 o `Makefile`: Type `make copy_swap_helper` to build and run the test driver.

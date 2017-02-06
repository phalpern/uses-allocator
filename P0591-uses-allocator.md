% P0591r0 | Utility functions to implement uses-allocator construction
% Pablo Halpern <phalpern@halpernwightsoftware.com>
% 2017-02-05 | Target audience: LEWG

Abstract
========
The phrase "*Uses-allocator construction* with allocator `Alloc`" is defined
in section **[allocator.uses.construction]** of the standard (20.7.7.2 of the
2014 standard or 20.10.7.2 of the 2016 CD). Although the definition is
reasonably concise, it fails to handle the case of constructing a `std::pair`
where one or both members can use `Alloc`. This omission manifests in
significant text describing the `construct` members of `polymorphic_allocator`
[memory.polymorphic.allocator.class] and `scoped_allocator_adaptor`
[allocator.adaptor]. Additionally, a `vector<pair<T,U>, A>` fails to pass the
allocator to the pair elements if `A` is a scoped or polymorphic allocator.

Though we could add the `pair` special case to the definition of
*Uses-allocator construction*, the definition would no longer be
concise. Moreover, any library implementing features that rely on
*Uses-allocator construction* would necessarily centralize the logic
into a function template. This paper, therefore, proposes a set of templates
that do exactly that, in the standard. The current uses of *Uses-allocator
construction* could then simply defer to these templates, making those
features simpler to describe and future-proof against other changes.

Because this proposal modifies wording in the standard, it is targeted at
C++20 (aka, C++Next) rather than a technical specification.

Choosing a direction
====================

Originally, I considered proposing a pair of function templates,
`make_using_allocator<T>(allocator, args...)` and
`uninitialized_construct_using_allocator(ptrToT, allocator,
args...)`. However, Implementation experience with the feature being proposed
showed that, given a type `T`, an allocator `A`, and an argument list
`Args...`, it was convenient to generate a `tuple` of the final
argument list for `T`'s constructor, then use `make_from_tuple` or `apply` to
implement the above function templates. It occurred to me that exposing this
`tuple`-building function may be desirable, as it opens the door to an entire
category of functions that use `tuple`s to manipulate argument lists in a
composable fashion.

The decision before the LEWG (assuming the basics of this proposal are
accepted) would be whether to:

 1. Standardize the function template that generates a `tuple` of arguments.
 2. Standardize the function templates that actually construct a `T` from an
    allocator and list of arguments.
 3. Both.
    
Proposed wording
================
The following wording is still rough. More detailed wording to come after LEWG
review and revision. Wording is relative to the November 2016 Committee Draft,
N5131.

Add three new function templates to `<memory>`:

    template <class T, class Alloc, class... Args>
    auto uses_allocator_construction_args(const Alloc& a, Args&&... args);

> _Returns_: A `tuple` value determined as follows:

 1. If `T` is of type `pair<T1, T2>` and
    `uses_allocator_v<T1> || uses_allocator_v<T2>` then
   a. If `Args...` is empty, returns
      `make_tuple(
        piecewise_construct,
        uses_allocator_construction_args<T1>(a),
        uses_allocator_construction_args<T2>(a))`
   b. Otherwise, if `Args...` matches `const pair<U1, U2>& arg`, returns
      `make_tuple(
        piecewise_construct,
        uses_allocator_construction_args<T1>(a, arg.first),
        uses_allocator_construction_args<T2>(a, arg.second)`
   c. Otherwise, if `Args...` matches `pair<U1, U2>&& arg`, returns
      `make_tuple(
        piecewise_construct,
        uses_allocator_construction_args<T1>(a, forward<U1>(arg.first)),
        uses_allocator_construction_args<T2>(a, forward<U2>(arg.second)))`
   d. Otherwise, if `Args...` matches `U1&& arg1, U2&& arg2`, returns
      `make_tuple(
        piecewise_construct,
        uses_allocator_construction_args<T1>(a, forward<U1>(arg1)),
        uses_allocator_construction_args<T2>(a, forward<U2>(arg2)))`
   e. Otherwise, if `Args...` matches `piecewise_construct_t, Tpl1 tpl1,
      Tpl2 tpl2`, returns
      `make_tuple(
        piecewise_construct,
        apply(uses_allocator_construction<T1>, a, tp1),
        apply(uses_allocator_construction<T2>, a, tp2)`.
   f. Otherwise the program is ill-formed.
 2. Otherwise, if `uses_

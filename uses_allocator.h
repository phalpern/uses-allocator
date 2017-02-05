/* uses_allocator.h                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_USES_ALLOCATOR_DOT_H
#define INCLUDED_USES_ALLOCATOR_DOT_H

#include <make_from_tuple.h>
#include <memory>

namespace std {

namespace experimental {
inline namespace fundamentals_v2 {

namespace pmr {

struct memory_resource
{
    bool is_equal(const memory_resource& other) const noexcept
        { return do_is_equal(other); }
protected:
    virtual bool do_is_equal(const memory_resource& other) const noexcept = 0;
};

bool operator==(const memory_resource& a, const memory_resource& b)
{
    return a.is_equal(b);
}

bool operator!=(const memory_resource& a, const memory_resource& b)
{
    return ! (a == b);
}

} // close namespace pmr
} // close namespace fundamentals_v2


inline namespace fundamentals_v3 {

////////////////////////////////////////////////////////////////////////

// Forward declaration
template <class T, class Alloc, class... Args>
auto forward_uses_allocator_args(allocator_arg_t, const Alloc& a,
                                 Args&&... args);

namespace internal {

template <bool V> using boolean_constant = integral_constant<bool, V>;

template <class T, class Alloc, class = void_t<> >
struct uses_allocator_imp : false_type { };

template <class T, class Alloc>
struct uses_allocator_imp<T, Alloc, void_t<typename T::allocator_type>>
    : is_convertible<Alloc, typename T::allocator_type> { };

// Metafunction `pair_uses_allocator<T, A>` evaluates true iff `T` is a
// specialization of `std::pair` and `uses_allocator<T::first_type, A>` and/or
// `uses_allocator<T::second_type, A>` are true.
template <class T, class A>
struct pair_uses_allocator : false_type
{
};

template <class T1, class T2, class A>
struct pair_uses_allocator<pair<T1, T2>, A>
    : boolean_constant<uses_allocator<T1, A>::value ||
                       uses_allocator<T2, A>::value>
{
};

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload is handles types for which `uses_allocator<T, Alloc>` is false.
template <class T, class Unused, class Alloc, class... Args>
auto forward_uses_allocator_imp(false_type /* pair_uses_allocator */,
                                false_type /* uses_allocator */,
                                Unused     /* uses prefix allocator arg */,
                                allocator_arg_t, const Alloc&,
                                Args&&... args)
{
    // Allocator is ignored
    return std::forward_as_tuple(std::forward<Args>(args)...);
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles types for which `uses_allocator<T, Alloc>` is
// true and constructor `T(allocator_arg_t, a, args...)` is valid.
template <class T, class Alloc, class... Args>
auto forward_uses_allocator_imp(false_type /* pair_uses_allocator */,
                                true_type  /* uses_allocator */,
                                true_type  /* uses prefix allocator arg */,
                                allocator_arg_t, const Alloc& a,
                                Args&&... args)
{
    // Allocator added to front of argument list, after `allocator_arg`.
    return std::forward_as_tuple(allocator_arg, a,
                                 std::forward<Args>(args)...);
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles types for which `uses_allocator<T, Alloc>` is
// true and constructor `T(allocator_arg_t, a, args...)` NOT valid.
// This function will produce invalid results unless `T(args..., a)` is valid.
template <class T1, class Alloc, class... Args>
auto forward_uses_allocator_imp(false_type /* pair_uses_allocator */,
                                true_type  /* uses_allocator */,
                                false_type /* prefix allocator arg */,
                                allocator_arg_t, const Alloc& a,
                                Args&&... args)
{
    // Allocator added to end of argument list
    return std::forward_as_tuple(std::forward<Args>(args)..., a);
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles specializations of `std::pair` for which
// `uses_allocator<T, Alloc>` is true for either or both of the elements and
// no other constructor arguments are passed in.
template <class T, class Alloc>
auto forward_uses_allocator_imp(true_type  /* pair_uses_allocator */,
                                false_type /* uses_allocator */,
                                false_type /* prefix allocator arg */,
                                allocator_arg_t, const Alloc& a)
{
    return std::make_tuple(
        piecewise_construct,
        forward_uses_allocator_args<typename T::first_type>(allocator_arg, a),
        forward_uses_allocator_args<typename T::second_type>(allocator_arg, a));
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles specializations of `std::pair` for which
// `uses_allocator<T, Alloc>` is true for either or both of the elements and
// a single argument of type const-lvalue-of-pair is passed in.
template <class T, class Alloc, class U1, class U2>
auto forward_uses_allocator_imp(true_type  /* pair_uses_allocator */,
                                false_type /* uses_allocator */,
                                false_type /* prefix allocator arg */,
                                allocator_arg_t, const Alloc& a,
                                const pair<U1, U2>& arg)
{
    return std::make_tuple(
        piecewise_construct,
        forward_uses_allocator_args<typename T::first_type>(allocator_arg, a,
                                                            arg.first),
        forward_uses_allocator_args<typename T::second_type>(allocator_arg, a,
                                                             arg.second));
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles specializations of `std::pair` for which
// `uses_allocator<T, Alloc>` is true for either or both of the elements and
// a single argument of type rvalue-of-pair is passed in.
template <class T, class Alloc, class U1, class U2>
auto forward_uses_allocator_imp(true_type  /* pair_uses_allocator */,
                                false_type /* uses_allocator */,
                                false_type /* prefix allocator arg */,
                                allocator_arg_t, const Alloc& a,
                                pair<U1, U2>&& arg)
{
    return std::make_tuple(
        piecewise_construct,
        forward_uses_allocator_args<typename T::first_type>(allocator_arg, a,
                                                      forward<U1>(arg.first)),
        forward_uses_allocator_args<typename T::second_type>(allocator_arg, a,
                                                      forward<U2>(arg.second)));
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles specializations of `std::pair` for which
// `uses_allocator<T, Alloc>` is true for either or both of the elements and
// two additional constructor arguments are passed in.
template <class T, class Alloc, class U1, class U2>
auto forward_uses_allocator_imp(true_type  /* pair_uses_allocator */,
                                false_type /* uses_allocator */,
                                false_type /* prefix allocator arg */,
                                allocator_arg_t, const Alloc& a,
                                U1&& arg1, U2&& arg2)
{
    return std::make_tuple(
        piecewise_construct,
        forward_uses_allocator_args<typename T::first_type>(allocator_arg, a,
                                                            forward<U1>(arg1)),
        forward_uses_allocator_args<typename T::second_type>(allocator_arg, a,
                                                            forward<U2>(arg2)));
}


} // close namespace internal

template <class T, class Alloc>
struct uses_allocator : internal::uses_allocator_imp<T, Alloc>::type { };

template <class T, class Alloc, class... Args>
auto forward_uses_allocator_args(allocator_arg_t, const Alloc& a,
                                 Args&&... args)
{
    using namespace internal;
    return forward_uses_allocator_imp<T>(pair_uses_allocator<T, Alloc>(),
                                         uses_allocator<T, Alloc>(),
                                         is_constructible<T, allocator_arg_t,
                                                          Alloc, Args...>(),
                                         allocator_arg, a,
                                         std::forward<Args>(args)...);
}

template <class T, class Alloc, class... Args>
T make_using_allocator(allocator_arg_t, const Alloc& a, Args&&... args)
{
    return make_from_tuple<T>(
        forward_uses_allocator_args<T>(allocator_arg, a,
                                       forward<Args>(args)...));

}

template <class T, class Alloc, class... Args>
T* uninitialized_construct_using_allocator(T* p,
                                           allocator_arg_t, const Alloc& a,
                                           Args&&... args)
{
    return uninitialized_construct_from_tuple(
        p,
        forward_uses_allocator_args<T>(allocator_arg, a,
                                       forward<Args>(args)...));
}

} // close namespace fundamentals_v3
} // close namespace experimental
} // close namespace std

#endif // ! defined(INCLUDED_USES_ALLOCATOR_DOT_H)

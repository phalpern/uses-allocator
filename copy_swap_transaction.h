/* copy_swap_transaction.h                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Implementation of P0208 Copy-Swap Transaction
 */

#ifndef INCLUDED_COPY_SWAP_TRANSACTION_DOT_H
#define INCLUDED_COPY_SWAP_TRANSACTION_DOT_H

// Feature-test macro
#define __cpp_lib_experimental_copy_swap_transaction 201707

#include "uses_allocator.h"
#include <utility>
#include <memory>
#include <cstdlib>

namespace std {

namespace experimental {
inline namespace fundamentals_v3 {

namespace internal {

template <typename T, typename = void_t<>>
struct has_get_allocator
    : false_type { };

template <typename T>
struct has_get_allocator<T, void_t<decltype(declval<T>().get_allocator())>>
    : true_type { };

template <typename T>
constexpr bool has_get_allocator_v = has_get_allocator<T>::value;

} // close internal namespace

template <class T, class U>
inline
enable_if_t<!internal::has_get_allocator_v<U>, remove_reference_t<T>>
copy_swap_helper(T&& other, const U&)
{
    return std::forward<T>(other);
}

template <class T, class U>
inline
enable_if_t<internal::has_get_allocator_v<U>, remove_reference_t<T>>
copy_swap_helper(T&& other, const U& alloc_donor)
{
    using TT = remove_reference_t<T>;
    return make_using_allocator<TT>(alloc_donor.get_allocator(),
                                    std::forward<T>(other));
}

template <class T>
inline
remove_reference_t<T> copy_swap_helper(T&& other)
{
    return copy_swap_helper(std::forward<T>(other), other);
}

template <class T>
inline
typename enable_if< internal::has_get_allocator_v<T>, T&>::type
swap_assign(T& lhs, decay_t<T>&& rhs)
{
    using Alloc = decltype(lhs.get_allocator());
    constexpr bool pocma =
        allocator_traits<Alloc>::propagate_on_container_move_assignment::value;
    T R = (pocma ? T(std::move(rhs)) :
           make_using_allocator<T>(lhs.get_allocator(), std::move(rhs)));
    using std::swap;
    // If pocma, assume pocs (propagate_on_container_swap)
    swap(lhs, R);
    return lhs;
}

template <class T>
inline
typename enable_if<!internal::has_get_allocator_v<T>, T&>::type
swap_assign(T& lhs, decay_t<T>&& rhs)
{
    T R(std::move(rhs));
    using std::swap;
    swap(lhs, R);
    return lhs;
}

template <class T>
inline
typename enable_if< internal::has_get_allocator_v<T>, T&>::type
swap_assign(T& lhs, decay_t<T> const& rhs)
{
    using Alloc = decltype(lhs.get_allocator());
    constexpr bool pocca =
        allocator_traits<Alloc>::propagate_on_container_copy_assignment::value;
    T R = make_using_allocator<T>((pocca ? rhs : lhs).get_allocator(), rhs);
    using std::swap;
    // If pocca, assume pocs (propagate_on_container_swap)
    swap(lhs, R);
    return lhs;
}

template <class T>
inline
typename enable_if<!internal::has_get_allocator_v<T>, T&>::type
swap_assign(T& lhs, decay_t<T> const& rhs)
{
    T R(rhs);
    using std::swap;
    swap(lhs, R);
    return lhs;
}


namespace internal {

template <class F, class... Args>
inline
void
copy_swap_transaction_imp(integral_constant<size_t, 0>, F&& f, Args&... args)
{
    // First argument is zero integral constant.
    // Terminate template recursion by actually calling `f`.
    // Note that `args` is a list of lvalue references, so it would be wrong
    // to use `std::forward<Args>`.
    std::forward<F>(f)(args...);
}

template <size_t N, class T, class... Rest>
inline
void
copy_swap_transaction_imp(integral_constant<size_t, N>, T& t, Rest&&... rest)
{
    // First argument is non-zero integral constant.
    // There is at least one argument before the invokable function.

    // Make a copy of `t` using `t`s allocator, even if `T` doesn't usually
    // propagate it's allocator on copy construction. If `T` doesn't use an
    // allocator, then `copy_swap_helper(t)` simply returns `t`.
    T tprime(copy_swap_helper(t));

    // Remove `t` from front of argument list and add rotate left by adding
    // `tprime` to the end of the list, then recurse.
    copy_swap_transaction_imp(integral_constant<size_t, N-1>(),
                              std::forward<Rest>(rest)..., tprime);

    // Transaction complete. Commit changes back to `t`.
    using std::swap;
    swap(t, tprime);
}
} // close namespace internal

template <class T, class... Rest>
inline
bool copy_swap_transaction(T& t, Rest&&... rest)
{
    integral_constant<size_t, sizeof...(Rest)> num_args_token;

    internal::copy_swap_transaction_imp(num_args_token, t,
                                        std::forward<Rest>(rest)...);
}

} // close fundamentals_v3
} // close experimental
} // close std

#endif // ! defined(INCLUDED_COPY_SWAP_TRANSACTION_DOT_H)

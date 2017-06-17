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

inline namespace Cpp17 {

enum class byte : unsigned char { };

} // close namespace Cpp17


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

// get_allocator() for types that don't use allocators.
// Returns the default allocator.
template <class T>
inline
enable_if_t<!internal::has_get_allocator_v<T>, allocator<std::byte>>
get_allocator(T&&)
{
    return allocator<byte>{};
}

// get_allocator() for types that do use allocators.
template <class T>
inline
auto get_allocator(T&& x) ->
    decltype(std::forward<T>(x).get_allocator())
{
    return std::forward<T>(x).get_allocator();
}

#if 0
// This function is no longer being proposed but, just in case, here's
// an implementation.
template <class T>
inline
remove_reference_t<T> copy_swap_helper(T&& other)
{
    using TT = remove_reference_t<T>;
    return make_using_allocator<TT>(get_allocator(other), other);
}
#endif

template <class T>
inline
T& swap_assign(T& lhs, decay_t<T>&& rhs)
{
    using Alloc = decltype(get_allocator(lhs));
    constexpr bool pocma =
        allocator_traits<Alloc>::propagate_on_container_move_assignment::value;
    T R = (pocma ? T(std::move(rhs)) :
           make_using_allocator<T>(get_allocator(lhs), std::move(rhs)));
    using std::swap;
    // If pocma, assume pocs (propagate_on_container_swap)
    swap(lhs, R);
    return lhs;
}

template <class T>
inline
T& swap_assign(T& lhs, decay_t<T> const& rhs)
{
    using Alloc = decltype(get_allocator(lhs));
    constexpr bool pocca =
        allocator_traits<Alloc>::propagate_on_container_copy_assignment::value;
    T R = make_using_allocator<T>(get_allocator(pocca ? rhs : lhs), rhs);
    using std::swap;
    // If pocca, assume pocs (propagate_on_container_swap)
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
    T tprime(make_using_allocator<T>(get_allocator(t), t));

    // Remove `t` from front of argument list and add rotate left by adding
    // `tprime` to the end of the list, then recurse.
    copy_swap_transaction_imp(integral_constant<size_t, N-1>(),
                              std::forward<Rest>(rest)..., tprime);

    // Transaction complete. Commit changes back to `t`.
    using std::swap;
    swap(t, tprime);
}
} // close namespace internal

template <class T, class... Args>
inline
void copy_swap_transaction(T& t, Args&&... args)
{
    integral_constant<size_t, sizeof...(Args)> num_args_token;

    internal::copy_swap_transaction_imp(num_args_token, t,
                                        std::forward<Args>(args)...);
}

} // close fundamentals_v3
} // close experimental
} // close std

#endif // ! defined(INCLUDED_COPY_SWAP_TRANSACTION_DOT_H)

/* copy_swap_helper.h                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Implementation of P0208 Copy-Swap Helper
 */

#ifndef INCLUDED_COPY_SWAP_HELPER_DOT_H
#define INCLUDED_COPY_SWAP_HELPER_DOT_H

// Feature-test macro
#define __cpp_lib_experimental_copy_swap_helper 201602

#include <utility>
#include <memory>
#include <cstdlib>

namespace std {

inline namespace cpp17 {

template <class...> struct void_t_imp { typedef void type; };
template <class... T> using void_t = typename void_t_imp<T...>::type;

} // close cpp17

namespace experimental {
inline namespace fundamentals_v3 {

namespace internal {

template <typename T, typename A, typename = void_t<>>
struct uses_prefix_allocator : false_type { };

template <typename T, typename A>
struct uses_prefix_allocator<T,A,void_t<decltype(T(allocator_arg, declval<A>(),
                                                   declval<T>()))>>
    : true_type { };

// template <typename T, typename A, typename V>
// struct uses_prefix_allocator<T&,A,V> : uses_prefix_allocator<T,A> { };

template <typename T, typename A>
constexpr bool uses_prefix_allocator_v = uses_prefix_allocator<T,A>::value;

template <typename T, typename A, typename = void_t<>>
struct uses_suffix_allocator : false_type { };

template <typename T, typename A>
struct uses_suffix_allocator<T,A,void_t<decltype(T(declval<T>(),
                                                   declval<A>()))>>
    : true_type { };

template <typename T, typename A>
constexpr bool uses_suffix_allocator_v = uses_suffix_allocator<T,A>::value;

template <typename T, typename = void_t<>>
struct has_get_allocator
    : false_type { };

template <typename T>
struct has_get_allocator<T, void_t<decltype(declval<T>().get_allocator())>>
    : true_type { };

template <typename T>
constexpr bool has_get_allocator_v = has_get_allocator<T>::value;

template <typename Type, typename Alloc>
inline
typename enable_if< uses_prefix_allocator_v<remove_reference_t<Type>,Alloc>,
                   remove_reference_t<Type>>::type
copy_swap_helper_imp(Type&& other, const Alloc& alloc)
{
    typedef remove_reference_t<Type> ObjType;
    return ObjType(allocator_arg, alloc, std::forward<Type>(other));
}

template <typename Type, typename Alloc>
inline
typename enable_if< uses_suffix_allocator_v<remove_reference_t<Type>,Alloc> &&
                   !uses_prefix_allocator_v<remove_reference_t<Type>,Alloc>,
                   remove_reference_t<Type>>::type
copy_swap_helper_imp(Type&& other, const Alloc& alloc)
{
    typedef remove_reference_t<Type> ObjType;
    return ObjType(std::forward<Type>(other), alloc);
}

template <typename Type, typename Alloc>
inline
typename enable_if<!uses_suffix_allocator_v<remove_reference_t<Type>,Alloc> &&
                   !uses_prefix_allocator_v<remove_reference_t<Type>,Alloc>,
                   remove_reference_t<Type>>::type
copy_swap_helper_imp(Type&& other, const Alloc&)
{
    static_assert(! uses_allocator<remove_reference_t<Type>,Alloc>::value,
                  "Type uses allocator but doesn't have appriate constructor");
    return std::forward<Type>(other);
}

} // close internal namespace

template <class T, class U>
inline
typename enable_if< internal::has_get_allocator_v<U>,
                   remove_reference_t<T>>::type
copy_swap_helper(T&& other, const U& alloc_source)
{
    using internal::copy_swap_helper_imp;
    return copy_swap_helper_imp(std::forward<T>(other),
                                alloc_source.get_allocator());
}

template <class T, class U>
inline
typename enable_if<!internal::has_get_allocator_v<U>,
                   remove_reference_t<T>>::type
copy_swap_helper(T&& other, const U& /* alloc_source */)
{
    return std::forward<T>(other);
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
    using internal::copy_swap_helper_imp;
    typedef decltype(lhs.get_allocator()) Alloc;
    constexpr bool pocma =
        allocator_traits<Alloc>::propagate_on_container_move_assignment::value;
    T R = (pocma ? T(std::move(rhs)) :
           copy_swap_helper(std::move(rhs), lhs));
    using std::swap;
    swap(lhs, R);
    return lhs;
}

template <class T>
inline
typename enable_if<!internal::has_get_allocator_v<T>,
                   T&>::type
swap_assign(T& lhs, decay_t<T>&& rhs)
{
    T R = copy_swap_helper(std::move(rhs), lhs);
    using std::swap;
    swap(lhs, R);
    return lhs;
}

template <class T>
inline
typename enable_if< internal::has_get_allocator_v<T>, T&>::type
swap_assign(T& lhs, decay_t<T> const& rhs)
{
    using internal::copy_swap_helper_imp;
    typedef decltype(lhs.get_allocator()) Alloc;
    constexpr bool pocca =
        allocator_traits<Alloc>::propagate_on_container_copy_assignment::value;
    T R = copy_swap_helper_imp(rhs, (pocca ? rhs : lhs).get_allocator());
    using std::swap;
    swap(lhs, R);
    return lhs;
}

template <class T>
inline
typename enable_if<!internal::has_get_allocator_v<T>, T&>::type
swap_assign(T& lhs, decay_t<T> const& rhs)
{
    T R = copy_swap_helper(rhs, lhs);
    using std::swap;
    swap(lhs, R);
    return lhs;
}

template <class F, class... Args>
inline
auto
copy_swap_transaction_imp(integral_constant<size_t, 0>, F&& f, Args&... args)
{
    return std::forward<F>(f)(args...);
}

template <size_t N, class T, class... Rest>
inline
auto
copy_swap_transaction_imp(integral_constant<size_t, N>, T& t, Rest&&... rest)
{
    T tprime(copy_swap_helper(t));
    auto ret = copy_swap_transaction_imp(integral_constant<size_t, N-1>(),
                                         std::forward<Rest>(rest)..., tprime);
    using std::swap;
    swap(t, tprime);
    return ret;
}

template <class T, class R1, class... Rest>
inline
auto copy_swap_transaction(T& t, R1&& r1, Rest&&... rest)
{
    return
    copy_swap_transaction_imp(integral_constant<size_t, 1 + sizeof...(Rest)>(),
                              t, std::forward<R1>(r1),
                              std::forward<Rest>(rest)...);
}

} // close fundamentals_v3
} // close experimental
} // close std

#endif // ! defined(INCLUDED_COPY_SWAP_HELPER_DOT_H)

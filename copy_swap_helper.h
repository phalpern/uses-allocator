/* copy_swap_helper.h                  -*-C++-*-
 *
 * Pablo Halpern, January 2016
 */

#ifndef INCLUDED_COPY_SWAP_HELPER_DOT_H
#define INCLUDED_COPY_SWAP_HELPER_DOT_H

#include <utility>
#include <memory>
#include <cstdlib>

namespace std {

inline namespace cpp17 {

template <class...> struct void_t_imp { typedef void type; };
template <class... T> using void_t = typename void_t_imp<T...>::type;

} // close cpp17

namespace experimental {
inline namespace fundamentals_v2 {

namespace internal {

template <typename T, typename A, typename = void_t<>>
struct uses_prefix_allocator : false_type { };

template <typename T, typename A>
struct uses_prefix_allocator<T,A,void_t<decltype(T(allocator_arg, declval<A>(),
                                                   declval<T>()))>>
    : true_type { };
          
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

template <typename T, typename = void_t<> >
struct has_get_memory_resource
    : false_type { };

template <typename T>
struct has_get_memory_resource<T, 
                       void_t<decltype(declval<T>().get_memory_resource())>>
    : true_type { };

template <typename T>
constexpr bool has_get_memory_resource_v = has_get_memory_resource<T>::value;

} // close internal namespace

template <typename Type, typename Alloc>
inline
typename enable_if< internal::uses_prefix_allocator_v<Type,Alloc>, Type>::type
copy_swap_helper(allocator_arg_t, const Alloc& alloc, const Type& other)
{
    return Type(allocator_arg, alloc, other);
}

template <typename Type, typename Alloc>
inline
typename enable_if< internal::uses_suffix_allocator_v<Type,Alloc> &&
                   !internal::uses_prefix_allocator_v<Type,Alloc>, Type>::type
copy_swap_helper(allocator_arg_t, const Alloc& alloc, const Type& other)
{
    return Type(other, alloc);
}

template <typename Type, typename Alloc>
inline
typename enable_if<!internal::uses_suffix_allocator_v<Type,Alloc> &&
                   !internal::uses_prefix_allocator_v<Type,Alloc>, Type>::type
copy_swap_helper(allocator_arg_t, const Alloc&, const Type& other)
{
    static_assert(! uses_allocator<Type,Alloc>::value,
                  "Type uses allocator but doesn't have appriate constructor");
    return other;
}

template <class Type>
inline
typename std::enable_if< internal::has_get_memory_resource_v<Type>, Type>::type
copy_swap_helper(const Type& other)
{
    return copy_swap_helper(allocator_arg, other.get_memory_resource(), other);
}

template <class Type>
inline
typename enable_if< internal::has_get_allocator_v<Type> &&
                   !internal::has_get_memory_resource_v<Type>, Type>::type
copy_swap_helper(const Type& other)
{
    return copy_swap_helper(allocator_arg, other.get_allocator(), other);
}

template <class Type>
inline
typename enable_if<!internal::has_get_allocator_v<Type> &&
                   !internal::has_get_memory_resource_v<Type>, Type>::type
copy_swap_helper(const Type& other)
{
    return other;
}

} // close fundamentals_v2
} // close experimental
} // close std

#endif // ! defined(INCLUDED_COPY_SWAP_HELPER_DOT_H)

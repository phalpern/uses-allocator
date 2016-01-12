/* using_allocator.h                  -*-C++-*-
 *
 * Pablo Halpern, 2016
 */

#ifndef INCLUDED_USING_ALLOCATOR_DOT_H
#define INCLUDED_USING_ALLOCATOR_DOT_H

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

// Types from existing TS draft
struct erased_type { };

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

template <class T, class Alloc> struct uses_allocator;

namespace internal {

template <bool V> using boolean_constant = integral_constant<bool, V>;

template <class T, class Alloc, class = void_t<> >
struct uses_allocator_imp : false_type { };

template <class T, class Alloc>
struct uses_allocator_imp<T, Alloc, void_t<typename T::allocator_type>>
    : integral_constant<
        bool,
        is_convertible<Alloc, typename T::allocator_type>::value ||
        is_same<erased_type, typename T::allocator_type>::value
      > { };

} // close namespace internal

template <class T, class Alloc>
struct uses_allocator : internal::uses_allocator_imp<T, Alloc>::type { };

} // close namespace fundamentals_v2
} // close namespace experimental
} // close namespace std


#endif // ! defined(INCLUDED_USING_ALLOCATOR_DOT_H)

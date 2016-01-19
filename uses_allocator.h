/* uses_allocator.h                  -*-C++-*-
 *
 * Pablo Halpern, 2016
 */

#ifndef INCLUDED_USES_ALLOCATOR_DOT_H
#define INCLUDED_USES_ALLOCATOR_DOT_H

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

////////////////////////////////////////////////////////////////////////
// Forward declare all types and functions
////////////////////////////////////////////////////////////////////////

namespace internal {

template <bool V> using boolean_constant = integral_constant<bool, V>;

template <class T, class Alloc, class = void_t<> >
struct uses_allocator_imp : false_type { };

template <size_t...> struct index_list { };

template <size_t N, size_t ... Seq>
struct make_index_list
{
    typedef typename make_index_list<N - 1, N - 1, Seq...>::type type;
};

template <size_t ... Seq>
struct make_index_list<0, Seq...>
{
    typedef index_list<Seq...> type;
};

template <size_t N>
using make_index_list_t = typename make_index_list<N>::type;

template <class T, class Alloc>
struct uses_allocator_imp<T, Alloc, void_t<typename T::allocator_type>>
    : boolean_constant<
        is_convertible<Alloc, typename T::allocator_type>::value ||
        is_same<erased_type, typename T::allocator_type>::value
      > { };

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload is handles types for which `uses_allocator<T, Alloc>` is false.
template <class T, class Unused, class Alloc, class... Args>
auto forward_uses_allocator_imp(false_type /* uses_allocator */,
                                Unused     /* uses prefix allocator arg */,
                                allocator_arg_t, const Alloc&,
                                Args&&... args)
{
    // Allocator is ignored
    return std::forward_as_tuple(std::forward<Args>(args)...);
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload is handles types for which `uses_allocator<T, Alloc>` is
// true and constructor `T(allocator_arg_t, a, args...)` is valid.
template <class T, class Alloc, class... Args>
auto forward_uses_allocator_imp(true_type /* uses_allocator */,
                                true_type /* uses prefix allocator arg */,
                                allocator_arg_t, const Alloc& a,
                                Args&&... args)
{
    // Allocator added to front of argument list, after `allocator_arg`.
    return std::forward_as_tuple(allocator_arg, a, 
                                 std::forward<Args>(args)...);
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload is handles types for which `uses_allocator<T, Alloc>` is
// true and constructor `T(allocator_arg_t, a, args...)` NOT valid.
// This function will produce invalid results unless `T(args..., a)` is valid.
template <class T, class Alloc, class... Args>
auto forward_uses_allocator_imp(true_type  /* uses_allocator */,
                                false_type /* prefix allocator arg */,
                                allocator_arg_t, const Alloc& a,
                                Args&&... args)
{
    // Allocator added to end of argument list
    return std::forward_as_tuple(std::forward<Args>(args)..., a);
}

template <class T, class... Args, size_t... Indexes>
T make_from_tuple_imp(const tuple<Args...>& tuple_args,
                      internal::index_list<Indexes...>)
{
    return T(get<Indexes>(tuple_args)...);
}

template <class T, class... Args, size_t... Indexes>
T* uninitialized_construct_from_tuple_imp(T* p,
                                          const tuple<Args...>& tuple_args,
                                          internal::index_list<Indexes...>)
{
    return ::new((void*) p) T(get<Indexes>(tuple_args)...);
}
    
} // close namespace internal

template <class T, class Alloc>
struct uses_allocator : internal::uses_allocator_imp<T, Alloc>::type { };

template <class T, class Alloc, class... Args>
auto forward_uses_allocator_args(allocator_arg_t, const Alloc& a,
                                 Args&&... args)
{
    using namespace internal;
    return forward_uses_allocator_imp<T>(uses_allocator<T, Alloc>(),
                                         is_constructible<T, allocator_arg_t,
                                                          Alloc, Args...>(),
                                         allocator_arg, a,
                                         std::forward<Args>(args)...);
}

template <class T, class... Args>
T make_from_tuple(const tuple<Args...>& tuple_args)
{
    using namespace internal;
    return make_from_tuple_imp<T>(tuple_args, 
                                  make_index_list_t<sizeof...(Args)>());
}

template <class T, class... Args>
T* uninitialized_construct_from_tuple(T* p, const tuple<Args...>& tuple_args)
{
    using namespace internal;
    return uninitialized_construct_from_tuple_imp<T>(p, tuple_args, 
                                       make_index_list_t<sizeof...(Args)>());
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

} // close namespace fundamentals_v2
} // close namespace experimental
} // close namespace std

#endif // ! defined(INCLUDED_USES_ALLOCATOR_DOT_H)

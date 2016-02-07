/* make_from_tuple.h                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_MAKE_FROM_TUPLE_DOT_H
#define INCLUDED_MAKE_FROM_TUPLE_DOT_H

#include <utility>
#include <cstdlib>

namespace std {

inline namespace cpp17 {

template <class...> struct void_t_imp { typedef void type; };
template <class... T> using void_t = typename void_t_imp<T...>::type;

} // close namespace cpp17

namespace experimental {
inline namespace fundamentals_v3 {

namespace internal {

template <class T, class... Args, size_t... Indexes>
T make_from_tuple_imp(const tuple<Args...>& tuple_args,
                      index_sequence<Indexes...>)
{
    return T(get<Indexes>(tuple_args)...);
}

template <class T, class... Args, size_t... Indexes>
T make_from_tuple_imp(tuple<Args...>&& tuple_args,
                      index_sequence<Indexes...>)
{
    return T(forward<Args>(get<Indexes>(tuple_args))...);
}

template <class T, class... Args, size_t... Indexes>
T* uninitialized_construct_from_tuple_imp(T* p,
                                          const tuple<Args...>& args_tuple,
                                          index_sequence<Indexes...>)
{
    return ::new((void*) p) T(get<Indexes>(args_tuple)...);
}
    
template <class T, class... Args, size_t... Indexes>
T* uninitialized_construct_from_tuple_imp(T* p,
                                          tuple<Args...>&& args_tuple,
                                          index_sequence<Indexes...>)
{
    return ::new((void*) p) T(forward<Args>(get<Indexes>(args_tuple))...);
}

} // close namespace namespace fundamentals_v3::internal

template <class T, class... Args>
T make_from_tuple(const tuple<Args...>& args_tuple)
{
    using namespace internal;
    return make_from_tuple_imp<T>(args_tuple, 
                                  make_index_sequence<sizeof...(Args)>());
}

template <class T, class... Args>
T make_from_tuple(tuple<Args...>&& args_tuple)
{
    using namespace internal;
    return make_from_tuple_imp<T>(std::move(args_tuple), 
                                  make_index_sequence<sizeof...(Args)>());
}

template <class T, class... Args>
T* uninitialized_construct_from_tuple(T* p, const tuple<Args...>& args_tuple)
{
    using namespace internal;
    return uninitialized_construct_from_tuple_imp<T>(p, args_tuple, 
                                       make_index_sequence<sizeof...(Args)>());
}

template <class T, class... Args>
T* uninitialized_construct_from_tuple(T* p, tuple<Args...>&& args_tuple)
{
    using namespace internal;
    return uninitialized_construct_from_tuple_imp<T>(p, std::move(args_tuple), 
                                       make_index_sequence<sizeof...(Args)>());
}

} // close namespace fundamentals_v3
} // close namespace experimental
} // close namespace std

#endif // ! defined(INCLUDED_MAKE_FROM_TUPLE_DOT_H)

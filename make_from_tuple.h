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

template <class T, class Tuple, size_t... Indexes>
T make_from_tuple_imp(Tuple&& t, index_sequence<Indexes...>)
{
    return T(get<Indexes>(forward<Tuple>(t))...);
}

template <class T, class Tuple, size_t... Indexes>
T* uninitialized_construct_from_tuple_imp(T* p, Tuple&& t,
                                          index_sequence<Indexes...>)
{
    return ::new((void*) p) T(get<Indexes>(std::forward<Tuple>(t))...);
}

} // close namespace namespace fundamentals_v3::internal

template <class T, class Tuple>
T make_from_tuple(Tuple&& args_tuple)
{
    using namespace internal;
    using Indices = make_index_sequence<tuple_size<decay_t<Tuple>>::value>;
    return make_from_tuple_imp<T>(forward<Tuple>(args_tuple), Indices{});
}

template <class T, class Tuple>
T* uninitialized_construct_from_tuple(T* p, Tuple&& args_tuple)
{
    using namespace internal;
    using Indices = make_index_sequence<tuple_size<decay_t<Tuple>>::value>;
    return uninitialized_construct_from_tuple_imp<T>(p,
                                                     forward<Tuple>(args_tuple),
                                                     Indices{});
}

} // close namespace fundamentals_v3
} // close namespace experimental
} // close namespace std

#endif // ! defined(INCLUDED_MAKE_FROM_TUPLE_DOT_H)

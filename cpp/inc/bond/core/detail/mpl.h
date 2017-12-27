// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*
 * This header provides a faster alternative to some of the facilities provided by boost::mpl.
 */


#pragma once

#include <bond/core/config.h>

#include <boost/static_assert.hpp>

#include <initializer_list>
#include <type_traits>
#include <utility>


namespace bond
{

namespace detail { namespace mpl
{

template <typename T> struct
identity
{
    using type = T;
};


/// Always evaluates to false, but depends on T, so can be used when
/// type-based always-false-ness is needed.
template <typename T>
struct always_false
    : std::false_type {};

/// @brief Represents a type list.
template <typename... T> struct
list;


template <typename List> struct
count;

template <typename... T> struct
count<list<T...> >
    : std::integral_constant<uint16_t, sizeof...(T)> {};


/// @brief Appends the given list of types or a single pack of list<> to the end.
template <typename List, typename... T> struct
append;

template <typename List, typename... T>
using append_t = typename append<List, T...>::type;

template <typename List, typename... T> struct
append
    : append<List, list<T...> > {};

template <typename... T, typename... U> struct
append<list<T...>, list<U...> >
    : identity<list<T..., U...> > {};


/// @brief Filters the given type list with the provided predicate.
template <typename List, template <typename> class C> struct
filter;

template <typename List, template <typename> class C>
using filter_t = typename filter<List, C>::type;

template <template <typename> class C> struct
filter<list<>, C>
    : identity<list<> > {};

template <typename T, typename... U, template <typename> class C> struct
filter<list<T, U...>, C>
    : append<typename std::conditional<C<T>::value, list<T>, list<> >::type, filter_t<list<U...>, C> > {};


template <typename List, uint16_t I> struct
at;

template <typename List, uint16_t I>
using at_t = typename at<List, I>::type;

template <typename T, typename... U> struct
at<list<T, U...>, 0> : identity<T> {};

template <typename T, typename... U, uint16_t I> struct
at<list<T, U...>, I> : at<list<U...>, I - 1> {};


template <typename List>
struct apply_functor;

template <typename... T> struct
apply_functor<list<T...> >
{
    template <typename F>
    static void invoke(F&& f)
    {
        (void)std::initializer_list<int>{ (
            (void)f(identity<T>{}),
            0)... }, f;
    }
};


template <typename List, typename F>
void apply(F&& f)
{
    apply_functor<List>::invoke(std::forward<F>(f));
}


template <typename List> struct
try_apply_functor;

template <typename T, typename... U> struct
try_apply_functor<list<T, U...> >
{
    template <typename F>
    static auto invoke(F&& f)
#ifdef BOND_NO_CXX14_RETURN_TYPE_DEDUCTION
        -> decltype(f(identity<T>{}))
#endif
    {
        if (auto&& result = f(identity<T>{}))
        {
            return result;
        }
        else
        {
            (void)std::initializer_list<int>{ (
                (void)(result || (result = f(identity<U>{}))),
                0)... };

            return result;
        }
    }
};


template <typename List, typename F>
auto try_apply(F&& f)
#ifdef BOND_NO_CXX14_RETURN_TYPE_DEDUCTION
    -> decltype(try_apply_functor<List>::invoke(std::forward<F>(f)))
#endif
{
    return try_apply_functor<List>::invoke(std::forward<F>(f));
}


#ifdef BOND_NO_CXX14_INTEGER_SEQUENCE

template <std::size_t...> struct
index_sequence;

template <std::size_t I, std::size_t... X> struct
make_index_sequence_type
    : make_index_sequence_type<I - 1, I - 1, X...> {};

template <std::size_t... X> struct
make_index_sequence_type<0, X...>
    : mpl::identity<index_sequence<X...> > {};

template <std::size_t N>
using make_index_sequence = typename make_index_sequence_type<N>::type;

#else

using std::index_sequence;
using std::make_index_sequence;

#endif

}} // namespace mpl { namespace detail

} // namespace bond

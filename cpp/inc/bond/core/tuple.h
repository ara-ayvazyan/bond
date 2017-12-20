// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "bond.h"
#include "detail/tuple_fields.h"

#include <tuple>

namespace bond
{
namespace detail
{

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


template <typename T, std::size_t I> struct
indexed_item
    : std::integral_constant<std::size_t, I>
{
    using type = T;
};


template <typename T> struct
not_ignore
    : std::integral_constant<bool,
        !std::is_same<typename std::decay<T>::type, typename std::decay<decltype(std::ignore)>::type>::value> {};

template <typename T, std::size_t I> struct
not_ignore<indexed_item<T, I> >
    : not_ignore<T> {};


template <typename List, typename Indices> struct
indexed_list_helper;

template <typename... T, std::size_t... I> struct
indexed_list_helper<mpl::list<T...>, index_sequence<I...> >
    : mpl::identity<mpl::list<indexed_item<T, I>...> > {};

template <typename List> struct
indexed_list;

template <typename... T> struct
indexed_list<mpl::list<T...> >
    : indexed_list_helper<mpl::list<T...>, make_index_sequence<sizeof...(T)> > {};


} // namespace detail


// Specialize bond::schema<T> for std::tuple<T...>
// This allows treating instances of std::tuple as Bond structs.
template <typename... T>
struct schema<std::tuple<T...> >
{
    using list = detail::mpl::list<typename std::decay<T>::type...>;

    using fields = detail::mpl::filter_t<list, detail::not_ignore>;

    using indexed_fields = detail::mpl::filter_t<typename detail::indexed_list<list>::type, detail::not_ignore>;

    template <uint16_t I>
    using indexed_field = detail::mpl::at_t<indexed_fields, I>;

    struct type
    {
        using base = no_base;

        struct var;

        using field_count = detail::mpl::count<fields>;

        template <uint16_t I>
        using field = detail::mpl::identity<detail::tuple_field<
            std::tuple<T...>,
            indexed_field<I>::value,
            typename indexed_field<I>::type,
            I> >;

        static const Metadata metadata;

        type()
        {
            // Force instantiation of template statics
            (void)metadata;
        }

        static Metadata GetMetadata()
        {
            return reflection::MetadataInit<fields>("tuple", "bond.tuple", reflection::Attributes());
        }
    };
};


template <typename... T>
const Metadata schema<std::tuple<T...> >::type::metadata
    = schema<std::tuple<T...> >::type::GetMetadata();


template <typename Protocols = BuiltInProtocols, typename Writer, typename... T>
inline void Pack(Writer& writer, T&&... args)
{
    Serialize<Protocols>(std::forward_as_tuple(args...), writer);
}


template <typename Protocols = BuiltInProtocols, typename Reader, typename... T>
inline void Unpack(Reader reader, T&... arg)
{
    auto pack = std::tie(arg...);
    Deserialize<Protocols>(reader, pack);
}

} // namepsace bond

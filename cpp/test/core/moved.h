#pragma once

#include <boost/multi_index_container.hpp>
#include <bond/core/reflection.h>


template <std::size_t I = 0, typename T>
typename boost::enable_if_c<(I == T::Schema::field_count::value), bool>::type
CheckMovedFields(const T& /*src*/)
{
    return true;
}

template <std::size_t I = 0, typename T>
typename boost::disable_if_c<(I == T::Schema::field_count::value), bool>::type
CheckMovedFields(const T& src);


template <typename T>
typename boost::enable_if<bond::has_schema<T>, bool>::type
moved(const T& src)
{
    return CheckMovedFields(src);
}


template <typename T>
typename boost::enable_if_c<bond::is_basic_type<T>::value && !bond::is_string_type<T>::value, bool>::type 
moved(const T&)
{
    return true;
}

template <typename T>
typename boost::enable_if<bond::is_string_type<T>, bool>::type 
moved(const T& src)
{
    return string_length(src) == 0;
}

template <typename T>
bool moved(const bond::bonded<T>& src)
{
    return src == bond::bonded<T>();
}

template <typename T>
typename boost::enable_if<bond::is_container<T>, bool>::type 
moved(const T& src)
{
    return container_size(src) == 0;
}

template <typename T, size_t N>
bool moved(const std::array<T, N>&)
{
    return true;
}

template <typename T, typename I>
bool moved(const boost::multi_index::multi_index_container<T, I>&)
{
    return true;
}


template <std::size_t I, typename T>
typename boost::disable_if_c<(I == T::Schema::field_count::value), bool>::type
CheckMovedFields(const T& src)
{
    using Field = bond::detail::field_info<T, I>;

    return moved(Field::GetVariable(src))
        && CheckMovedFields<I + 1>(src);
}

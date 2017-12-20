// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "omit_default.h"

namespace bond
{

namespace detail
{

template <typename Field> struct
is_optional
    : std::integral_constant<bool,
        !is_bond_type<typename Field::field_type>::value
        && std::is_same<typename Field::field_modifier, reflection::optional_field_modifier>::value> {};


template <typename Field, typename T>
inline typename boost::enable_if<is_optional<Field>, bool>::type
IsOptionalDefault(const T& var)
{
    return is_default(Field::GetVariable(var), Field::metadata);
}

template <typename Field, typename T>
inline typename boost::disable_if<is_optional<Field>, bool>::type
IsOptionalDefault(const T& /*var*/)
{
    return true;
}


template <typename T, uint16_t I = 0>
inline typename boost::enable_if_c<(I == schema<T>::type::field_count::value), bool>::type
CheckOptionalDefault(const T& /*var*/)
{
    return true;
}

template <typename T, uint16_t I = 0>
inline typename boost::disable_if_c<(I == schema<T>::type::field_count::value), bool>::type
CheckOptionalDefault(const T& var)
{
    return IsOptionalDefault<field_info<T, I> >(var) && CheckOptionalDefault<T, I + 1>(var);
}


} // namespace detail

} // namespace bond

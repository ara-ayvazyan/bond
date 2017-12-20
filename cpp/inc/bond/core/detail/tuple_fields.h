// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

namespace bond
{
namespace detail
{

template <typename Tuple, uint16_t Id, typename T, uint16_t Index>
struct tuple_field
{
    using struct_type = Tuple;
    using value_type = typename std::remove_reference<T>::type;
    using field_type = typename remove_maybe<value_type>::type;
    using field_modifier = reflection::optional_field_modifier;

    static const Metadata metadata;
    BOND_STATIC_CONSTEXPR uint16_t id = Id;
    BOND_STATIC_CONSTEXPR uint16_t index = Index;

    static BOND_CONSTEXPR const value_type& GetVariable(const struct_type& obj)
    {
        return std::get<id>(obj);
    }

    static BOND_CONSTEXPR value_type& GetVariable(struct_type& obj)
    {
        return std::get<id>(obj);
    }

    static Metadata GetMetadata()
    {
        std::string name = "item" + std::to_string(id);
        return reflection::MetadataInit(name.c_str(), Modifier::Optional, reflection::Attributes());
    }
};

template <typename Tuple, uint16_t Id, typename T, uint16_t Index>
const Metadata tuple_field<Tuple, Id, T, Index>::metadata
    = tuple_field<Tuple, Id, T, Index>::GetMetadata();


} // namespace detail

} // namespace bond

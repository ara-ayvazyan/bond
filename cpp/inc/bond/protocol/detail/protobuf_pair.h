// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "protobuf_utils.h"

#include <bond/core/bond_types.h>
#include <bond/core/reflection.h>

#include <tuple>


namespace bond
{
namespace detail
{
namespace proto
{
    template <bool IsKey, Encoding Enc>
    struct KeyValueFieldMetadata
    {
        using field_modifier = reflection::optional_field_modifier;

        static const Metadata metadata;
        BOND_STATIC_CONSTEXPR uint16_t id = IsKey ? 1 : 2;

        static Metadata GetMetadata()
        {
            Metadata m;
            m.name = IsKey ? "Key" : "Value";

            switch (Enc)
            {
            case Encoding::Fixed:
                m.attributes["ProtoEncode"] = "Fixed";
                break;
            case Encoding::ZigZag:
                m.attributes["ProtoEncode"] = "ZigZag";
                break;
            default:
                break;
            }

            return m;
        }
    };

    template <bool IsKey, Encoding Enc>
    const Metadata KeyValueFieldMetadata<IsKey, Enc>::metadata = GetMetadata();


    template <typename Pair, typename T, bool IsKey, Encoding Enc>
    struct KeyValueField : KeyValueFieldMetadata<IsKey, Enc>
    {
        using struct_type = Pair;
        using value_type = typename std::remove_reference<T>::type;
        using field_type = typename remove_maybe<value_type>::type;

        static BOND_CONSTEXPR const value_type& GetVariable(const struct_type& obj)
        {
            return std::get<IsKey ? 0 : 1>(obj);
        }

        static BOND_CONSTEXPR value_type& GetVariable(struct_type& obj)
        {
            return std::get<IsKey ? 0 : 1>(obj);
        }
    };


    template <typename Key, Encoding KeyEnc, typename Value, Encoding ValueEnc>
    struct KeyValuePair : std::tuple<Key, Value>
    {
        KeyValuePair(const std::tuple<Key, Value>& value)
            : std::tuple<Key, Value>(value)
        {}

        struct Schema
        {
            using base = no_base;

            static const Metadata metadata;

            Schema()
            {
                // Force instantiation of template statics
                (void)metadata;
            }

            using fields = boost::mpl::list<
                KeyValueField<KeyValuePair, Key, true, KeyEnc>,
                KeyValueField<KeyValuePair, Value, false, ValueEnc> >;

            static Metadata GetMetadata()
            {
                return reflection::MetadataInit(
                    "KeyValuePair", "bond.proto.KeyValuePair", reflection::Attributes());
            }
        };
    };

    template <typename Key, Encoding KeyEnc, typename Value, Encoding ValueEnc>
    const Metadata KeyValuePair<Key, KeyEnc, Value, ValueEnc>::Schema::metadata = GetMetadata();


    template <typename Protocols, Encoding KeyEnc, Encoding ValueEnc, typename Key, typename Value, typename Reader>
    inline void DeserializeKeyValuePair(Key& key, Value& value, Reader& input)
    {
        KeyValuePair<Key&, KeyEnc, Value&, ValueEnc> pair = std::tie(key, value);

        Deserialize<Protocols, Reader&>(input, pair,
            GetRuntimeSchema<KeyValuePair<Key, KeyEnc, Value, ValueEnc> >());
    }


    template <typename Protocols, Encoding KeyEnc, typename Key, typename Value, typename Reader>
    inline void DeserializeKeyValuePair(Key& key, Value& value, Encoding value_encoding, Reader& input)
    {
        switch (value_encoding)
        {
        case Encoding::Fixed:
            DeserializeKeyValuePair<Protocols, KeyEnc, Encoding::Fixed>(key, value, input);
            break;

        case Encoding::ZigZag:
            DeserializeKeyValuePair<Protocols, KeyEnc, Encoding::ZigZag>(key, value, input);
            break;

        default:
            {
                static const auto unavailable = static_cast<Encoding>(0xF);
                BOOST_ASSERT(unavailable == Unavailable<Encoding>());
                DeserializeKeyValuePair<Protocols, KeyEnc, unavailable>(key, value, input);
            }
            break;
        }
    }


    template <typename Protocols, typename Key, typename Value, typename Reader>
    inline void DeserializeKeyValuePair(
        Key& key, Encoding key_encoding, Value& value, Encoding value_encoding, Reader& input)
    {
        switch (key_encoding)
        {
        case Encoding::Fixed:
            DeserializeKeyValuePair<Protocols, Encoding::Fixed>(key, value, value_encoding, input);
            break;

        case Encoding::ZigZag:
            DeserializeKeyValuePair<Protocols, Encoding::ZigZag>(key, value, value_encoding, input);
            break;

        default:
            {
                static const auto unavailable = static_cast<Encoding>(0xF);
                BOOST_ASSERT(unavailable == Unavailable<Encoding>());
                DeserializeKeyValuePair<Protocols, unavailable>(key, value, value_encoding, input);
            }
            break;
        }
    }

} // namespace proto
} // namespace detail
} // namespace bond

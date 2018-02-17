// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>
#include <bond/core/exception.h>
#include <bond/core/bond_const_enum.h>

#include <boost/static_assert.hpp>

#include <cstring>
#include <type_traits>


namespace bond
{
namespace detail
{
namespace proto
{
    enum class WireType : uint8_t
    {
        VarInt          = 0,
        Fixed64         = 1,
        LengthDelimited = 2,
        Fixed32         = 5
    };

    enum class Encoding : uint8_t
    {
        Fixed,
        ZigZag
    };

    enum class Packing : uint8_t
    {
        False
    };

    template <typename T>
    struct Unavailable : std::integral_constant<T, static_cast<T>(0xF)>
    {
        BOOST_STATIC_ASSERT(std::is_same<T, Encoding>::value
                            || std::is_same<T, Packing>::value
                            || std::is_same<T, WireType>::value);
    };

    BOND_NORETURN inline void NotSupportedException(const char* name)
    {
        BOND_THROW(CoreException, name << " is not supported.");
    }

    inline WireType GetWireType(BondDataType type, Encoding encoding)
    {
        switch (type)
        {
        case BT_BOOL:
            return WireType::VarInt;

        case BT_FLOAT:
            return WireType::Fixed32;

        case BT_DOUBLE:
            return WireType::Fixed64;

        case BT_UINT8:
        case BT_UINT16:
        case BT_UINT32:
        case BT_UINT64:
            switch (encoding)
            {
            case Encoding::Fixed:
                return type == BT_UINT64 ? WireType::Fixed64 : WireType::Fixed32;

            case Encoding::ZigZag:
                break;

            default:
                return WireType::VarInt;
            }
            break;

        case BT_INT8:
        case BT_INT16:
        case BT_INT32:
        case BT_INT64:
            switch (encoding)
            {
            case Encoding::Fixed:
                return type == BT_INT64 ? WireType::Fixed64 : WireType::Fixed32;

            case Encoding::ZigZag:
            default:
                return WireType::VarInt;
            }
            break;

        case BT_STRUCT:
        case BT_STRING:
        case BT_WSTRING:
            return WireType::LengthDelimited;

        default:
            break;
        }

        BOOST_ASSERT(false);
        return Unavailable<WireType>::value;
    }

    template <typename Unused = void>
    struct AttributeName
    {
        static const std::string encode;
        static const std::string encode_key;
        static const std::string encode_value;
        static const std::string pack;
    };

    template <typename Unused>
    const std::string AttributeName<Unused>::encode = "ProtoEncode";

    template <typename Unused>
    const std::string AttributeName<Unused>::encode_key = "ProtoEncodeKey";

    template <typename Unused>
    const std::string AttributeName<Unused>::encode_value = "ProtoEncodeValue";

    template <typename Unused>
    const std::string AttributeName<Unused>::pack = "ProtoPack";

    inline Encoding ReadEncoding(const std::string& name, const Metadata& metadata)
    {
        auto it = metadata.attributes.find(name);
        if (it == metadata.attributes.end())
        {
            return Unavailable<Encoding>::value;
        }

        if (std::strcmp(it->second.c_str(), "Fixed") == 0)
        {
            return Encoding::Fixed;
        }

        if (std::strcmp(it->second.c_str(), "ZigZag") == 0)
        {
            return Encoding::ZigZag;
        }

        InvalidEnumValueException(it->second.c_str(), "Encoding");
    }

    inline Encoding ReadEncoding(BondDataType type, const Metadata* metadata, const std::string& name)
    {
        switch (type)
        {
        case BT_FLOAT:
        case BT_DOUBLE:
            return Encoding::Fixed;

        case BT_UINT8:
        case BT_UINT16:
        case BT_UINT32:
        case BT_UINT64:
            {
                Encoding encoding = metadata
                    ? ReadEncoding(name, *metadata)
                    : BOND_THROW(CoreException, "Ambiguous unknown field is encountered.");

                return encoding != Encoding::ZigZag
                    ? encoding
                    : BOND_THROW(CoreException, "Unsigned integers cannot have ZigZag encoding.");
            }

        case BT_INT8:
        case BT_INT16:
        case BT_INT32:
        case BT_INT64:
            return metadata
                ? ReadEncoding(name, *metadata)
                : BOND_THROW(CoreException, "Ambiguous unknown field is encountered.");

        case BT_SET:
        case BT_MAP:
            NotSupportedException("Nested set/map");

        case BT_BOOL:
        case BT_LIST:
        case BT_STRING:
        case BT_WSTRING:
        case BT_STRUCT:
        default:
            return Unavailable<Encoding>::value;
        }
    }

    inline Encoding ReadEncoding(BondDataType type, const Metadata* metadata)
    {
        return ReadEncoding(type, metadata, AttributeName<>::encode);
    }

    inline Encoding ReadKeyEncoding(BondDataType type, const Metadata* metadata)
    {
        if (type == BT_FLOAT || type == BT_DOUBLE)
        {
            NotSupportedException("Float/double map key");
        }

        return ReadEncoding(type, metadata, AttributeName<>::encode_key);
    }

    inline Encoding ReadValueEncoding(BondDataType type, const Metadata* metadata)
    {
        return ReadEncoding(type, metadata, AttributeName<>::encode_value);
    }

    inline Packing ReadPacking(const std::string& name, const Metadata& metadata)
    {
        auto it = metadata.attributes.find(name);
        if (it == metadata.attributes.end())
        {
            return Unavailable<Packing>::value;
        }

        if (std::strcmp(it->second.c_str(), "False") == 0)
        {
            return Packing::False;
        }

        InvalidEnumValueException(it->second.c_str(), "Packing");
    }

    inline Packing ReadPacking(BondDataType type, const Metadata* metadata)
    {
        switch (type)
        {
        case BT_STRING:
        case BT_WSTRING:
        case BT_STRUCT:
        case BT_LIST:
            return Packing::False;

        case BT_SET:
        case BT_MAP:
            NotSupportedException("Nested set/map");

        default:
            return metadata
                ? ReadPacking(AttributeName<>::pack, *metadata)
                : BOND_THROW(CoreException, "Ambiguous unknown field is encountered.");
        }
    }

    template <typename T, typename Enable = void> struct
    is_blob_type
        : std::false_type {};

    template <typename T> struct
    is_blob_type<T, typename boost::enable_if<is_list_container<T> >::type>
        : std::integral_constant<bool,
            (get_type_id<typename element_type<T>::type>::value == BT_INT8)> {};

} // namespace proto
} // namespace detail
} // namespace bond

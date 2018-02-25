// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "detail/protobuf_pair.h"
#include "detail/protobuf_utils.h"
#include "detail/simple_array.h"
#include "protobuf_parser.h"
#include "protobuf_field_validator.h"

#include <boost/locale.hpp>

/*
    Implements Protocol Buffers binary encoding.
    See https://developers.google.com/protocol-buffers/docs/encoding for details.
*/

namespace bond
{
    template <typename BufferT>
    class ProtobufBinaryWriter;


    template <typename BufferT>
    class ProtobufBinaryReader
    {
        using WireType = detail::proto::WireType;
        using Encoding = detail::proto::Encoding;

    public:
        using Buffer = BufferT;
        using Parser = ProtobufParser<ProtobufBinaryReader&>;
        using Writer = ProtobufBinaryWriter<Buffer>;

        BOND_STATIC_CONSTEXPR uint16_t magic = 0x5042;

        explicit ProtobufBinaryReader(const Buffer& input, bool strict_match = true)
            : _input{ input },
              _size{ 0 },
              _id{ 0 },
              _type{ detail::proto::Unavailable<WireType>::value },
              _encoding{ detail::proto::Unavailable<Encoding>::value },
              _key_encoding{ detail::proto::Unavailable<Encoding>::value },
              _strict_match{ strict_match },
              _lengths{}
        {}

        void ReadStructBegin(bool base = false)
        {
            BOOST_VERIFY(!base);

            if (_lengths.empty())
            {
                _lengths.push((std::numeric_limits<uint32_t>::max)());
            }
            else
            {
                BOOST_ASSERT(_type == WireType::LengthDelimited);

                Consume(_size);
                _lengths.push(_size);
            }
        }

        void ReadStructEnd(bool base = false)
        {
            BOOST_VERIFY(!base);
            BOOST_ASSERT(!_lengths.empty());

            uint32_t length = _lengths.pop(std::nothrow);
            BOOST_VERIFY(_lengths.empty() || length == 0);
        }

        bool ReadFieldBegin(WireType& type, uint16_t& id)
        {
            if (ReadTag())
            {
                type = _type;
                id = _id;
                return true;
            }

            return false;
        }

        void SetEncoding(Encoding encoding)
        {
            _encoding = encoding;
        }

        Encoding GetEncoding() const
        {
            return _encoding;
        }

        void SetKeyEncoding(Encoding encoding)
        {
            _key_encoding = encoding;
        }

        Encoding GetKeyEncoding() const
        {
            return _key_encoding;
        }

        const uint32_t& GetSize() const
        {
            return _size;
        }

        void ReadFieldEnd()
        {}

        void Read(bool& value)
        {
            BOOST_ASSERT(_encoding == detail::proto::Unavailable<Encoding>::value);

            switch (_type)
            {
            case WireType::VarInt:
                ReadVarInt(value);
                break;

            case WireType::LengthDelimited:
                ReadVarIntPacked(value);
                break;

            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        template <typename T>
        typename boost::enable_if_c<std::is_arithmetic<T>::value
                                    && !std::is_floating_point<T>::value>::type
        Read(T& value)
        {
            BOOST_STATIC_ASSERT(sizeof(T) <= sizeof(uint64_t));

            switch (_type)
            {
            case WireType::VarInt:
                ReadVarInt(value);
                break;

            case WireType::Fixed32:
                ReadFixed32(value);
                break;

            case WireType::Fixed64:
                ReadFixed64(value);
                break;

            case WireType::LengthDelimited:
                switch (_encoding)
                {
                case Encoding::Fixed:
                    ReadFixedPacked(value);
                    break;

                case Encoding::ZigZag:
                    BOOST_ASSERT(is_signed_int<T>::value);
                    // fall-through
                default:
                    ReadVarIntPacked(value);
                    break;
                }
                break;

            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        template <typename T>
        typename boost::enable_if<std::is_enum<T> >::type
        Read(T& value)
        {
            BOOST_STATIC_ASSERT(sizeof(value) == sizeof(int32_t));

            int32_t value32;
            Read(value32);
            value = static_cast<T>(value32);
        }

        template <typename T>
        typename boost::enable_if<std::is_floating_point<T> >::type
        Read(T& value)
        {
            BOOST_ASSERT(_encoding == Encoding::Fixed);

            switch (_type)
            {
            case WireType::Fixed32:
                ReadFixed32(value);
                break;

            case WireType::Fixed64:
                ReadFixed64(value);
                break;

            case WireType::LengthDelimited:
                ReadFixedPacked(value);
                break;

            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        template <typename T>
        typename boost::enable_if<is_string<T> >::type
        Read(T& value)
        {
            BOOST_ASSERT(_type == WireType::LengthDelimited);
            BOOST_ASSERT(_size != 0);

            detail::ReadStringData(_input, value, _size);
            Consume(_size);
            _size = 0;
        }

        template <typename T>
        typename boost::enable_if<is_wstring<T> >::type
        Read(T& value)
        {
            std::string str;
            Read(str);
            value = boost::locale::conv::utf_to_utf<typename element_type<T>::type>(str);
        }

        void Read(blob& value)
        {
            BOOST_ASSERT(_type == WireType::LengthDelimited);

            if (_size != 0)
            {
                _input.Read(value, _size);
                Consume(_size);
                _size = 0;
            }
        }

        void ReadByte(blob::value_type& value)
        {
            BOOST_ASSERT(_type == WireType::LengthDelimited);
            BOOST_ASSERT(_size != 0);

            _input.Read(value);
            Consume(sizeof(blob::value_type));
            _size -= sizeof(blob::value_type);
        }

        void Skip()
        {
            switch (_type)
            {
            case WireType::VarInt:
                {
                    uint64_t value;
                    ReadVarInt(value);
                }
                break;

            case WireType::Fixed64:
                _input.Skip(sizeof(uint64_t));
                Consume(sizeof(uint64_t));
                break;

            case WireType::LengthDelimited:
                _input.Skip(_size);
                Consume(_size);
                _size = 0;
                break;

            case WireType::Fixed32:
                _input.Skip(sizeof(uint32_t));
                Consume(sizeof(uint32_t));
                break;

            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        template <typename T>
        void Skip()
        {
            Skip();
        }

        template <typename T>
        void Skip(const bonded<T, ProtobufBinaryReader&>&)
        {
            BOOST_ASSERT(_type == WireType::LengthDelimited);
            Skip();
        }

        void Skip(BondDataType /*type*/)
        {
            Skip();
        }

        bool operator==(const ProtobufBinaryReader& rhs) const
        {
            return _input == rhs._input;
        }

    private:
        friend Parser;

        void Consume(uint32_t size)
        {
            BOOST_ASSERT(!_lengths.empty());
            uint32_t& length = _lengths.top(std::nothrow);

            if (length >= size)
            {
                length -= size;
            }
            else
            {
                BOND_THROW(CoreException, "Trying to read more than declared.");
            }
        }

        void ConsumePacked(uint32_t size)
        {
            BOOST_ASSERT(_type == WireType::LengthDelimited);
            BOOST_ASSERT(_size != 0);
            BOOST_ASSERT(_size >= size);
            _size -= size;
        }

        template <typename T, typename U>
        typename boost::enable_if<is_signed_int<T>, T>::type
        Decode(const U& value)
        {
            return _encoding == Encoding::ZigZag
                ? static_cast<T>(DecodeZigZag(value))
                : static_cast<T>(value);
        }

        template <typename T, typename U>
        typename boost::disable_if<is_signed_int<T>, T>::type
        Decode(const U& value)
        {
            return static_cast<T>(value);
        }

        template <typename T>
        typename boost::enable_if_c<std::is_unsigned<T>::value
                                    && (sizeof(T) > sizeof(uint8_t)), uint32_t>::type
        ReadVarInt(T& value)
        {
            uint32_t size = ReadVariableUnsigned(_input, value);
            Consume(size);
            return size;
        }

        template <typename T>
        typename boost::disable_if_c<std::is_unsigned<T>::value
                                    && (sizeof(T) > sizeof(uint8_t)), uint32_t>::type
        ReadVarInt(T& value)
        {
            typename std::conditional<is_signed_int<T>::value, uint64_t, uint16_t>::type value16or64;
            uint32_t size = ReadVarInt(value16or64);
            value = Decode<T>(value16or64);
            return size;
        }

        template <typename T>
        void ReadVarIntPacked(T& value)
        {
            ConsumePacked(ReadVarInt(value));
        }

        template <typename T>
        typename boost::enable_if_c<(sizeof(T) == sizeof(uint32_t))>::type
        ReadFixed32(T& value)
        {
            BOOST_STATIC_ASSERT(std::is_trivial<T>::value);
            _input.Read(value);
            Consume(sizeof(uint32_t));
        }

        template <typename T>
        typename boost::disable_if_c<(sizeof(T) == sizeof(uint32_t))>::type
        ReadFixed32(T& value)
        {
            typename std::conditional<std::is_floating_point<T>::value, float, uint32_t>::type value32;
            ReadFixed32(value32);
            value = Decode<T>(value32);
        }

        template <typename T>
        typename boost::enable_if_c<(sizeof(T) == sizeof(uint64_t))>::type
        ReadFixed64(T& value)
        {
            BOOST_STATIC_ASSERT(std::is_trivial<T>::value);
            _input.Read(value);
            Consume(sizeof(uint64_t));
        }

        template <typename T>
        typename boost::disable_if_c<(sizeof(T) == sizeof(uint64_t))>::type
        ReadFixed64(T& value)
        {
            typename std::conditional<std::is_floating_point<T>::value, double, uint64_t>::type value64;
            ReadFixed64(value64);
            value = Decode<T>(value64);
        }

        template <typename T>
        typename boost::enable_if_c<(sizeof(T) <= sizeof(uint32_t))>::type
        ReadFixedPacked(T& value)
        {
            ReadFixed32(value);
            ConsumePacked(sizeof(uint32_t));
        }

        template <typename T>
        typename boost::enable_if_c<(sizeof(T) == sizeof(uint64_t))>::type
        ReadFixedPacked(T& value)
        {
            ReadFixed64(value);
            ConsumePacked(sizeof(uint64_t));
        }

        bool ReadTag()
        {
            BOOST_ASSERT(!_lengths.empty());
            if ((_lengths.top(std::nothrow) != 0) && !_input.IsEof())
            {
                uint64_t tag;
                ReadVarInt(tag);

                const auto type = static_cast<WireType>(tag & 0x7);
                switch (type)
                {
                case WireType::VarInt:
                case WireType::Fixed32:
                case WireType::Fixed64:
                    _size = 0;
                    break;

                case WireType::LengthDelimited:
                    ReadVarInt(_size);
                    break;

                default:
                    BOND_THROW(CoreException, "Unrecognized wire type.");
                    break;
                }

                const auto id = tag >> 3;
                if (id > (std::numeric_limits<uint16_t>::max)())
                {
                    BOND_THROW(CoreException, "Field ordinal does not fit in 16 bits.");
                }

                _type = type;
                _id = static_cast<uint16_t>(id);
                _encoding = _key_encoding = detail::proto::Unavailable<Encoding>::value;

                return true;
            }

            return false;
        }


        Buffer _input;
        uint32_t _size;
        uint16_t _id;
        WireType _type;
        Encoding _encoding;
        Encoding _key_encoding;
        bool _strict_match;
        detail::SimpleArray<uint32_t> _lengths;
    };


    namespace detail
    {
        template <typename Protocols, typename T, typename X, typename Buffer>
        inline bool ApplyTransform(
            const bond::To<T, Protocols>& transform, const bonded<X, ProtobufBinaryReader<Buffer>&>& bonded)
        {
            return ApplyTransform<Protocols>(
                bond::To<T, Protocols, proto::RequiredFieldValiadator<T> >{ transform }, bonded);
        }

        template <typename Protocols, typename T, typename X, typename Buffer>
        inline bool ApplyTransform(
            const bond::To<T, bond::Protocols<ProtobufBinaryReader<Buffer> > >& transform, const bonded<X>& bonded)
        {
            BOOST_STATIC_ASSERT(std::is_same<Protocols, bond::Protocols<ProtobufBinaryReader<Buffer> > >::value);

            return ApplyTransform<Protocols>(
                bond::To<T, Protocols, proto::RequiredFieldValiadator<T> >{ transform }, bonded);
        }

    } // namesace detail


    namespace detail
    {
        template <typename Buffer>
        inline void SkipElements(BondDataType /*type*/, ProtobufBinaryReader<Buffer>& input, uint32_t size)
        {
            BOOST_VERIFY(size == 0);
            input.Skip();
        }

        template <typename T, typename Buffer>
        inline void SkipElements(
            BondDataType /*keyType*/,
            BondDataType /*elementType*/,
            ProtobufBinaryReader<Buffer>& input,
            uint32_t size)
        {
            BOOST_VERIFY(size == 0);
            input.Skip();
        }

        template <typename T, typename Buffer>
        inline void SkipElements(
            BondDataType /*keyType*/,
            const value<T, ProtobufBinaryReader<Buffer>&>& /*element*/,
            ProtobufBinaryReader<Buffer>& input,
            uint32_t size)
        {
            BOOST_VERIFY(size == 0);
            input.Skip();
        }

        template <typename Key, typename T, typename Buffer>
        inline void SkipElements(
            const value<Key, ProtobufBinaryReader<Buffer>&>& /*key*/,
            const value<T, ProtobufBinaryReader<Buffer>&>& element,
            uint32_t size)
        {
            BOOST_VERIFY(size == 0);
            element.Skip();
        }

        template <typename T, typename Buffer>
        inline void SkipElements(const value<T, ProtobufBinaryReader<Buffer>&>& element, uint32_t size)
        {
            BOOST_VERIFY(size == 0);
            element.Skip();
        }

    } // namespace detail


    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if_c<(is_list_container<X>::value || is_set_container<X>::value)
                                && is_element_matching<value<T, ProtobufBinaryReader<Buffer>&>, X>::value
                                && !detail::proto::is_blob_type<X>::value>::type
    inline DeserializeElements(X& var, const value<T, ProtobufBinaryReader<Buffer>&>& element, uint32_t size)
    {
        BOOST_VERIFY(size == 0);

        do
        {
            typename element_type<X>::type item = make_element(var);
            element.template Deserialize<Protocols>(item);
            container_append(var, item);
        }
        while (element.GetInput().GetSize() != 0);
    }


    template <typename Protocols, typename X, typename Buffer>
    typename boost::enable_if_c<is_element_matching<value<blob::value_type, ProtobufBinaryReader<Buffer>&>, X>::value
                                && detail::proto::is_blob_type<X>::value>::type
    inline DeserializeElements(
        X& var, const value<blob::value_type, ProtobufBinaryReader<Buffer>&>& element, uint32_t size)
    {
        BOOST_VERIFY(size == 0);

        resize_list(var, element.GetInput().GetSize());

        for (enumerator<X> items(var); items.more(); )
        {
            element.GetInput().ReadByte(items.next());
        }
    }


    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if<is_matching<T, X> >::type
    inline DeserializeElements(
        nullable<X>& var, const value<T, ProtobufBinaryReader<Buffer>&>& element, uint32_t size)
    {
        BOOST_VERIFY(size == 0);

        do
        {
            element.template Deserialize<Protocols>(var.set());
        }
        while (element.GetInput().GetSize() != 0);
    }


    template <typename Protocols, typename Buffer>
    inline void DeserializeElements(
        nullable<blob::value_type>& var, const value<blob::value_type, ProtobufBinaryReader<Buffer>&>& element, uint32_t size)
    {
        BOOST_VERIFY(size == 0);

        while (element.GetInput().GetSize() != 0)
        {
            element.GetInput().ReadByte(var.set());
        }
    }


    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if<is_basic_container<X> >::type
    inline DeserializeContainer(X& var, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        detail::MatchingTypeContainer<Protocols>(var, GetTypeId(element), input, 0);
    }


    template <typename Protocols, typename T, typename Buffer>
    inline void DeserializeContainer(blob& var, const T& /*element*/, ProtobufBinaryReader<Buffer>& input)
    {
        input.Read(var);
    }


    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if<is_nested_container<X> >::type
    inline DeserializeContainer(X& var, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        DeserializeElements<Protocols>(var, element, 0);
    }

    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::disable_if<is_container<X> >::type
    inline DeserializeContainer(X& /*var*/, const T& /*element*/, ProtobufBinaryReader<Buffer>& /*input*/)
    {
        BOOST_STATIC_ASSERT(detail::mpl::always_false<X>::value); // No transcoding is supported
    }


    template <typename Protocols, typename X, typename T1, typename T2, typename Buffer>
    inline void DeserializeContainer(
        X& var, const value<std::pair<T1, T2>, ProtobufBinaryReader<Buffer>&>& element, ProtobufBinaryReader<Buffer>& input)
    {
        DeserializeMap<Protocols>(var, get_type_id<T1>::value, value<T2, ProtobufBinaryReader<Buffer>&>{ input, false }, input);
    }


    template <typename Protocols, typename X, typename Key, typename T, typename Buffer>
    typename boost::enable_if<is_map_key_matching<value<Key, ProtobufBinaryReader<Buffer>&>, X> >::type
    inline DeserializeMapElements(
        X& var,
        const value<Key, ProtobufBinaryReader<Buffer>&>& /*key*/,
        const value<T, ProtobufBinaryReader<Buffer>&>& element,
        uint32_t size)
    {
        BOOST_VERIFY(size == 0);
        
        typename element_type<X>::type::first_type k = make_key(var);
        typename element_type<X>::type::second_type v = make_value(var);

        auto&& input = element.GetInput();

        detail::proto::DeserializeKeyValuePair<Protocols>(
            k, input.GetKeyEncoding(), v, input.GetEncoding(), input);
        mapped_at(var, k) = std::move(v);
    }


    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if<is_basic_container<X> >::type
    inline DeserializeMap(X& var, BondDataType keyType, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        detail::MatchingMapByElement<Protocols>(var, keyType, GetTypeId(element), input, 0);
    }


    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if<is_nested_container<X> >::type
    inline DeserializeMap(X& var, BondDataType keyType, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        detail::MapByKey<Protocols>(var, keyType, element, input, 0);
    }


    template <typename Protocols, typename Transform, typename T, typename Buffer>
    inline void DeserializeMap(
        const Transform& /*transform*/, BondDataType /*keyType*/, const T& /*element*/, ProtobufBinaryReader<Buffer>& /*input*/)
    {
        BOOST_STATIC_ASSERT(detail::mpl::always_false<T>::value); // No transcoding is supported
    }

} // namespace bond

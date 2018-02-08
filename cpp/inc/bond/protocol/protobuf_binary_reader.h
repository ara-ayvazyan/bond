// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "detail/protobuf_utils.h"
#include "detail/simple_array.h"
#include <bond/core/detail/omit_default.h>
#include <bond/core/bond_types.h>

#include <boost/locale.hpp>
#include <boost/shared_ptr.hpp>

/*
    Implements Protocol Buffers binary encoding.
    See https://developers.google.com/protocol-buffers/docs/encoding for details.
*/

namespace bond
{
    namespace detail
    {
    namespace proto
    {
        template <typename Schema>
        class FieldEnumerator;

        template <>
        class FieldEnumerator<RuntimeSchema>
        {
        public:
            explicit FieldEnumerator(const RuntimeSchema& schema)
                : _begin{ schema.GetStruct().fields.begin() },
                  _end{ schema.GetStruct().fields.end() },
                  _it{ _begin }
            {}

            const FieldDef* find(uint16_t id)
            {
                if (_it == _end
                    || (_it->id != id && (++_it == _end || _it->id != id)))
                {
                    _it = std::lower_bound(_begin, _end, id,
                        [](const FieldDef& f, uint16_t id) { return f.id < id; });
                }

                return _it != _end && _it->id == id ? &*_it : nullptr;
            }

        private:
            const std::vector<FieldDef>::const_iterator _begin;
            const std::vector<FieldDef>::const_iterator _end;
            std::vector<FieldDef>::const_iterator _it;
        };

        template <typename Schema>
        FieldEnumerator<Schema> EnumerateFields(const Schema& schema)
        {
            return FieldEnumerator<Schema>{ schema };
        }

    } // namespace proto
    } // namespace detail


    template <typename Input>
    class ProtobufParser
    {
        using WireType = detail::proto::WireType;

    public:
        ProtobufParser(Input input, bool base)
            : _input{ input }
        {
            BOOST_ASSERT(!base);
        }

        template <typename Transform, typename Schema>
        bool Apply(const Transform& transform, const Schema& schema)
        {
            detail::StructBegin(_input, false);
            bool result = Read(schema, transform);
            detail::StructEnd(_input, false);
            return result;
        }

    private:
        // use compile-time schema
        template <typename Schema, typename Transform>
        bool Read(const Schema&, const Transform& transform)
        {
            transform.Begin(Schema::metadata);
            bool done = ReadFields(typename boost::mpl::begin<typename Schema::fields>::type{}, transform);
            transform.End();
            return done;
        }

        template <typename Fields, typename Transform>
        bool ReadFields(const Fields& /*fields*/, const Transform& /*transform*/)
        {
            using Head = typename boost::mpl::deref<Fields>::type;
            // TODO:
            return false;
        }

        template <typename Transform>
        bool ReadFields(const boost::mpl::l_iter<boost::mpl::l_end>&, const Transform&)
        {
            return false;
        }

        // use runtime schema
        template <typename Transform>
        bool Read(const RuntimeSchema& schema, const Transform& transform)
        {
            transform.Begin(schema.GetStruct().metadata);
            bool done = ReadFields(schema, transform);
            transform.End();
            return done;
        }

        template <typename Transform>
        class WireTypeFieldBinder
        {
        public:
            WireTypeFieldBinder(WireType type, const Transform& transform)
                : _type{ type },
                  _transform{ transform }
            {}

            bool Field(uint16_t id, const Metadata& metadata, const value<bool, Input&>& value) const
            {
                switch (_type)
                {
                case WireType::VarInt:
                    _transform.Field(id, metadata, value);
                    return true;
                }

                return false;
            }

            template <typename T>
            typename boost::enable_if_c<std::is_arithmetic<T>::value
                                        && !std::is_floating_point<T>::value, bool>::type
            Field(uint16_t id, const Metadata& metadata, const value<T, Input&>& value) const
            {
                switch (_type)
                {
                case WireType::VarInt:
                case WireType::Fixed32:
                case WireType::Fixed64:
                    _transform.Field(id, metadata, value);
                    return true;
                }

                return false;
            }

            bool Field(uint16_t id, const Metadata& metadata, const value<float, Input&>& value) const
            {
                switch (_type)
                {
                case WireType::Fixed32:
                    _transform.Field(id, metadata, value);
                    return true;
                }

                return false;
            }

            bool Field(uint16_t id, const Metadata& metadata, const value<double, Input&>& value) const
            {
                switch (_type)
                {
                case WireType::Fixed64:
                    _transform.Field(id, metadata, value);
                    return true;
                }

                return false;
            }

            template <typename T>
            typename boost::enable_if<is_string_type<T>, bool>::type
            Field(uint16_t id, const Metadata& metadata, const value<T, Input&>& value) const
            {
                switch (_type)
                {
                case WireType::LengthDelimited:
                    _transform.Field(id, metadata, value);
                    return true;
                }

                return false;
            }

        private:
            const WireType _type;
            const Transform& _transform;
        };

        template <typename Transform>
        WireTypeFieldBinder<Transform> BindWireTypeField(WireType type, const Transform& transform)
        {
            return WireTypeFieldBinder<Transform>{ type, transform };
        }

        template <typename Transform>
        bool ReadFields(const RuntimeSchema& schema, const Transform& transform)
        {
            WireType type;
            uint16_t id;

            for (auto fields = detail::proto::EnumerateFields(schema); _input.ReadFieldBegin(type, id); _input.ReadFieldEnd())
            {
                if (const FieldDef* field = fields.find(id))
                {
                    if (field->type.id == BT_STRUCT)
                    {
                        switch (type)
                        {
                        case WireType::LengthDelimited:
                            transform.Field(id, field->metadata, bonded<void, Input>{ _input, RuntimeSchema{ schema, *field } });
                            continue;
                        }
                    }
                    else if (field->type.id == BT_LIST || field->type.id == BT_SET)
                    {
                        BOOST_ASSERT(field->type.element.hasvalue());
                        _input.SetEncoding(detail::proto::ReadEncoding(field->type.element->id, field->metadata));

                        transform.Field(id, field->metadata, value<void, Input>{ _input, RuntimeSchema{ schema, *field } });
                        continue;
                    }
                    else if (field->type.id == BT_MAP)
                    {
                        BOOST_ASSERT(field->type.key.hasvalue());
                        _input.SetKeyEncoding(detail::proto::ReadKeyEncoding(field->type.key->id, field->metadata));

                        BOOST_ASSERT(field->type.element.hasvalue());
                        _input.SetEncoding(detail::proto::ReadValueEncoding(field->type.element->id, field->metadata));

                        transform.Field(id, field->metadata, value<void, Input>{ _input, RuntimeSchema{ schema, *field } });
                        continue;
                    }
                    else
                    {
                        _input.SetEncoding(detail::proto::ReadEncoding(field->type.id, field->metadata));

                        if (detail::BasicTypeField(id, field->metadata, field->type.id, BindWireTypeField(type, transform), _input))
                        {
                            continue;
                        }
                    }
                }

                transform.UnknownField(id, value<void, Input>{ _input, BT_UNAVAILABLE });
            }

            _input.ReadFieldEnd();

            return false;
        }


        Input _input;
    };


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

        explicit ProtobufBinaryReader(const Buffer& input)
            : _input{ input },
              _type{ detail::proto::Unavailable<WireType>() },
              _id{ 0 },
              _encoding{ detail::proto::Unavailable<Encoding>() },
              _key_encoding{ detail::proto::Unavailable<Encoding>() },
              _size{ 0 }
        {}

        void ReadStructBegin(bool base = false)
        {
            if (base)
            {
                detail::proto::NotSupportedException("Inheritance");
            }

            if (_size != 0)
            {
                BOOST_ASSERT(_type == WireType::LengthDelimited);

                if (!_lengths)
                {
                    _lengths = boost::make_shared<detail::SimpleArray<uint32_t> >();
                }

                _lengths->push(_size);
            }
        }

        void ReadStructEnd(bool base = false)
        {
            BOOST_VERIFY(!base);

            if (_lengths && !_lengths->empty())
            {
                _lengths->pop(std::nothrow);
            }
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

        void SetKeyEncoding(Encoding encoding)
        {
            _key_encoding = encoding;
        }

        const uint32_t& GetSize() const
        {
            return _size;
        }

        void ReadFieldEnd()
        {}

        void Read(bool& value)
        {
            BOOST_ASSERT(_encoding == detail::proto::Unavailable<Encoding>());

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
            //BOOST_ASSERT(is_signed_int<T>::value || _encoding == detail::proto::Unavailable<Encoding>());

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
            int32_t raw;
            Read(raw);
            value = static_cast<T>(raw);
        }

        template <typename T>
        typename boost::enable_if<std::is_floating_point<T> >::type
        Read(T& value)
        {
            //BOOST_ASSERT(_encoding == detail::proto::Unavailable<Encoding>());

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
            switch (_type)
            {
            case WireType::LengthDelimited:
                BOOST_ASSERT(_size != 0);
                detail::ReadStringData(_input, value, _size);
                Consume(_size);
                _size = 0;
                break;

            default:
                BOOST_ASSERT(false);
                break;
            }
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
        void Consume(uint32_t size)
        {
            if (_lengths && !_lengths->empty())
            {
                uint32_t& length = _lengths->top(std::nothrow);

                if (length >= size)
                {
                    length -= size;
                }
                else
                {
                    BOND_THROW(CoreException, "Trying to read more than declared.");
                }
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
            if ((!_lengths || _lengths->empty() || _lengths->top(std::nothrow) != 0) && !_input.IsEof())
            {
                uint64_t tag;
                ReadVarInt(tag);

                auto raw_type = static_cast<WireType>(tag & 0x7);
                switch (raw_type)
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

                auto raw_id = tag >> 3;
                if (raw_id > (std::numeric_limits<uint16_t>::max)())
                {
                    BOND_THROW(CoreException, "Field ordinal does not fit in 16 bits.");
                }

                _type = static_cast<WireType>(raw_type);
                _id = static_cast<uint16_t>(raw_id);
                _encoding = _key_encoding = detail::proto::Unavailable<Encoding>();

                return true;
            }

            return false;
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
                BOOST_ASSERT(_size != 0);
                _input.Skip(_size);
                Consume(_size);
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


        template <typename Protocols, typename Key, typename Value>
        friend inline void DeserializeKeyValuePair(Key& key, Value& value, ProtobufBinaryReader<Buffer>& input)
        {
            detail::proto::DeserializeKeyValuePair<Protocols>(key, input._key_encoding, value, input._encoding, input);
        }


        Buffer _input;
        WireType _type;
        uint16_t _id;
        Encoding _encoding;
        Encoding _key_encoding;
        uint32_t _size;
        boost::shared_ptr<detail::SimpleArray<uint32_t> > _lengths;
    };


    template <typename T>
    typename boost::enable_if<is_list_container<T> >::type
    inline container_append(T& list, const typename element_type<T>::type& item)
    {
        list.push_back(item);
    }

    template <typename T>
    typename boost::enable_if<is_set_container<T> >::type
    inline container_append(T& set, const typename element_type<T>::type& item)
    {
        set_insert(set, item);
    }


    namespace detail
    {
        template <typename Buffer>
        inline void SkipElements(BondDataType /*type*/, ProtobufBinaryReader<Buffer>& /*input*/, uint32_t /*size*/)
        {
            BOOST_ASSERT(false);
        }

        template <typename T, typename Buffer>
        inline void SkipElements(
            BondDataType /*keyType*/,
            BondDataType /*elementType*/,
            ProtobufBinaryReader<Buffer>& /*input*/,
            uint32_t /*size*/)
        {
            BOOST_ASSERT(false);
        }

        template <typename T, typename Buffer>
        inline void SkipElements(
            BondDataType /*keyType*/,
            const value<T, ProtobufBinaryReader<Buffer>&>& /*element*/,
            ProtobufBinaryReader<Buffer>& /*input*/,
            uint32_t /*size*/)
        {
            BOOST_ASSERT(false);
        }

    } // namespace detail


    namespace detail
    {
    namespace proto
    {
        template <typename Pair, typename T, bool IsKey, Encoding Enc>
        struct MapKeyValueField
        {
            using struct_type = Pair;
            using value_type = typename std::remove_reference<T>::type;
            using field_type = typename remove_maybe<value_type>::type;
            using field_modifier = reflection::optional_field_modifier;

            static const Metadata metadata;
            BOND_STATIC_CONSTEXPR uint16_t id = IsKey ? 1 : 2;

            static BOND_CONSTEXPR const value_type& GetVariable(const struct_type& obj)
            {
                return std::get<id - 1>(obj);
            }

            static BOND_CONSTEXPR value_type& GetVariable(struct_type& obj)
            {
                return std::get<id - 1>(obj);
            }

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

        template <typename Pair, typename T, bool IsKey, Encoding Enc>
        const Metadata MapKeyValueField<Pair, T, IsKey, Enc>::metadata = GetMetadata();

        template <typename Key, Encoding KeyEnc, typename Value, Encoding ValueEnc>
        struct MapKeyValuePair : std::tuple<Key, Value>
        {
            MapKeyValuePair(const std::tuple<Key, Value>& value)
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
                    MapKeyValueField<MapKeyValuePair, Key, true, KeyEnc>,
                    MapKeyValueField<MapKeyValuePair, Value, false, ValueEnc> >;

                static Metadata GetMetadata()
                {
                    return reflection::MetadataInit(
                        "MapKeyValuePair", "bond.proto.MapKeyValuePair", reflection::Attributes());
                }
            };
        };

        template <typename Key, Encoding KeyEnc, typename Value, Encoding ValueEnc>
        const Metadata MapKeyValuePair<Key, KeyEnc, Value, ValueEnc>::Schema::metadata = GetMetadata();

        template <
            typename Protocols,
            Encoding KeyEnc,
            Encoding ValueEnc,
            typename Key,
            typename Value,
            typename Buffer>
        inline void DeserializeKeyValuePair(Key& key, Value& value, ProtobufBinaryReader<Buffer>& input)
        {
            using KeyValuePair = MapKeyValuePair<Key, KeyEnc, Value, ValueEnc>;
            using KeyValuePairRef = MapKeyValuePair<Key&, KeyEnc, Value&, ValueEnc>;

            KeyValuePairRef pair = std::tie(key, value);
            Deserialize<Protocols, ProtobufBinaryReader<Buffer>&>(input, pair, GetRuntimeSchema<KeyValuePair>());
        }

        template <typename Protocols, typename Key, typename Value, typename Buffer>
        inline void DeserializeKeyValuePair(
            Key& key,
            Encoding key_encoding,
            Value& value,
            Encoding value_encoding,
            ProtobufBinaryReader<Buffer>& input)
        {
            static const auto unavailable = static_cast<Encoding>(0xF);
            BOOST_ASSERT(unavailable == Unavailable<Encoding>());

            switch (key_encoding)
            {
            case Encoding::Fixed:
                switch (value_encoding)
                {
                case Encoding::Fixed:
                    DeserializeKeyValuePair<Protocols, Encoding::Fixed, Encoding::Fixed>(key, value, input);
                    break;

                case Encoding::ZigZag:
                    DeserializeKeyValuePair<Protocols, Encoding::Fixed, Encoding::ZigZag>(key, value, input);
                    break;

                default:
                    DeserializeKeyValuePair<Protocols, Encoding::Fixed, unavailable>(key, value, input);
                    break;
                }
                break;

            case Encoding::ZigZag:
                switch (value_encoding)
                {
                case Encoding::Fixed:
                    DeserializeKeyValuePair<Protocols, Encoding::ZigZag, Encoding::Fixed>(key, value, input);
                    break;

                case Encoding::ZigZag:
                    DeserializeKeyValuePair<Protocols, Encoding::ZigZag, Encoding::ZigZag>(key, value, input);
                    break;

                default:
                    DeserializeKeyValuePair<Protocols, Encoding::ZigZag, unavailable>(key, value, input);
                    break;
                }
                break;

            default:
                switch (value_encoding)
                {
                case Encoding::Fixed:
                    DeserializeKeyValuePair<Protocols, unavailable, Encoding::Fixed>(key, value, input);
                    break;

                case Encoding::ZigZag:
                    DeserializeKeyValuePair<Protocols, unavailable, Encoding::ZigZag>(key, value, input);
                    break;

                default:
                    DeserializeKeyValuePair<Protocols, unavailable, unavailable>(key, value, input);
                    break;
                }
                break;
            }
        }

    } // namespace proto
    } // namespace detail


    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if_c<is_container<X>::value && !is_map_container<X>::value>::type
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

    template <typename Protocols, typename Transform, typename T, typename Buffer>
    inline void DeserializeElements(
        const Transform& /*transform*/,
        const value<T, ProtobufBinaryReader<Buffer>&>& /*element*/,
        uint32_t /*size*/)
    {
        BOOST_STATIC_ASSERT("No transcoding is supported for ProtobufBinaryReader.");
    }

    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if<is_basic_container<X> >::type
    inline DeserializeContainer(X& var, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        detail::MatchingTypeContainer<Protocols>(var, GetTypeId(element), input, 0);
    }

    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::disable_if<is_basic_container<X> >::type
    inline DeserializeContainer(X& var, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        DeserializeElements<Protocols>(var, element, 0);
    }

    template <typename Protocols, typename T, typename Buffer>
    inline void DeserializeContainer(blob& var, const T& /*element*/, ProtobufBinaryReader<Buffer>& input)
    {
        input.Read(var);
    }

    inline const char* GetTypeName(const decltype(std::ignore)&, const qualified_name_tag&)
    {
        return "Ignore";
    }

    template <typename Protocols, typename X, typename Key, typename T, typename Buffer>
    inline void DeserializeMapElements(
        X& var,
        const value<Key, ProtobufBinaryReader<Buffer>&>& /*key*/,
        const value<T, ProtobufBinaryReader<Buffer>&>& element,
        uint32_t size)
    {
        BOOST_VERIFY(size == 0);

        typename element_type<X>::type::first_type k = make_key(var);
        typename element_type<X>::type::second_type v = make_value(var);

        DeserializeKeyValuePair<Protocols>(k, v, element.GetInput());
        mapped_at(var, k) = std::move(v);
    }

    template <typename Protocols, typename Transform, typename Key, typename T, typename Buffer>
    inline void DeserializeMapElements(
        const Transform& /*transform*/,
        const value<Key, ProtobufBinaryReader<Buffer>&>& /*key*/,
        const value<T, ProtobufBinaryReader<Buffer>&>& /*element*/,
        uint32_t /*size*/)
    {
        BOOST_STATIC_ASSERT("No transcoding is supported for ProtobufBinaryReader.");
    }

    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if<is_basic_container<X> >::type
    inline DeserializeMap(X& var, BondDataType keyType, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        detail::MatchingMapByElement<Protocols>(var, keyType, GetTypeId(element), input, 0);
    }

} // namespace bond

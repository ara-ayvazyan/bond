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
            const auto& fields = schema.GetStruct().fields;

            WireType type;
            uint16_t id;

            for (auto it = fields.begin(); _input.ReadFieldBegin(type, id); _input.ReadFieldEnd())
            {
                if (it == fields.end()
                    || (it->id != id && (++it == fields.end() || it->id != id)))
                {
                    it = std::lower_bound(
                        fields.begin(),
                        fields.end(),
                        id,
                        [](const FieldDef& f, uint16_t id) { return f.id < id; });
                }

                if (it != fields.end())
                {
                    const FieldDef& field = *it;

                    if (field.type.id == BT_STRUCT)
                    {
                        switch (type)
                        {
                        case WireType::LengthDelimited:
                            transform.Field(id, field.metadata, bonded<void, Input>{ _input, RuntimeSchema{ schema, field } });
                            continue;
                        }
                    }
                    else if (field.type.id == BT_LIST || field.type.id == BT_SET)
                    {
                        BOOST_ASSERT(field.type.element.hasvalue());
                        _input.SetEncoding(detail::proto::ReadEncoding(field.type.element->id, field.metadata));
                        transform.Field(id, field.metadata, value<void, Input>{ _input, RuntimeSchema{ schema, field } });
                        continue;
                    }
                    else if (field.type.id == BT_MAP)
                    {
                        BOOST_ASSERT(false); // Not implemented
                    }
                    else
                    {
                        _input.SetEncoding(detail::proto::ReadEncoding(field.type.id, field.metadata));

                        if (detail::BasicTypeField(id, field.metadata, field.type.id, BindWireTypeField(type, transform), _input))
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
                _encoding = detail::proto::Unavailable<Encoding>();

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


        Buffer _input;
        WireType _type;
        uint16_t _id;
        Encoding _encoding;
        uint32_t _size;
        boost::shared_ptr<detail::SimpleArray<uint32_t> > _lengths;
    };


    template <typename T>
    inline void list_append(T& list, const typename element_type<T>::type& item)
    {
        list.push_back(item);
    }

    namespace detail
    {
        template <typename Buffer>
        inline void SkipElements(BondDataType /*type*/, ProtobufBinaryReader<Buffer>& /*input*/, uint32_t /*size*/)
        {
            BOOST_ASSERT(false);
        }

    } // namespace detail


    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if<is_list_container<X> >::type
    inline DeserializeElements(X& var, const value<T, ProtobufBinaryReader<Buffer>&>& element, const uint32_t& size)
    {
        do
        {
            typename element_type<X>::type item = make_element(var);
            element.template Deserialize<Protocols>(item);
            list_append(var, item);
        }
        while (size != 0);
    }

    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if_c<is_list_container<X>::value
                                && is_basic_type<typename element_type<X>::type>::value>::type
    inline DeserializeContainer(X& var, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        detail::MatchingTypeContainer<Protocols>(var, GetTypeId(element), input, input.GetSize());
    }

    template <typename Protocols, typename X, typename T, typename Buffer>
    typename boost::enable_if_c<is_list_container<X>::value
                                && !is_basic_type<typename element_type<X>::type>::value>::type
    inline DeserializeContainer(X& var, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        DeserializeElements<Protocols>(var, element, input.GetSize());
    }

    template <typename Protocols, typename T, typename Buffer>
    inline void DeserializeContainer(blob& var, const T& /*element*/, ProtobufBinaryReader<Buffer>& input)
    {
        input.Read(var);
    }

} // namespace bond

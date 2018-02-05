// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "detail/protobuf_utils.h"
#include <bond/core/detail/omit_default.h>
#include <bond/core/bond_types.h>

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
        bool ReadFields(const RuntimeSchema& schema, const Transform& transform)
        {
            const auto& fields = schema.GetStruct().fields;

            WireType wire_type;
            uint16_t id;

            for (auto it = fields.begin();
                    _input.ReadFieldBegin(wire_type, id);
                    _input.ReadFieldEnd(), it != fields.end() ? ++it : it)
            {
                if (it == fields.end() || it->id != id)
                {
                    it = std::lower_bound(
                        fields.begin(),
                        fields.end(),
                        id,
                        [](const FieldDef& f, uint16_t id) { return f.id < id; });
                }

                BondDataType type;

                if (it != fields.end())
                {
                    switch (type = it->type.id)
                    {
                    case BT_BOOL:
                        switch (wire_type)
                        {
                        case WireType::VarInt:
                            transform.Field(id, it->metadata, value<bool, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_UINT8:
                        switch (wire_type)
                        {
                        case WireType::VarInt:
                        case WireType::Fixed32:
                        case WireType::Fixed64:
                            transform.Field(id, it->metadata, value<uint8_t, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_UINT16:
                        switch (wire_type)
                        {
                        case WireType::VarInt:
                        case WireType::Fixed32:
                        case WireType::Fixed64:
                            transform.Field(id, it->metadata, value<uint16_t, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_UINT32:
                        switch (wire_type)
                        {
                        case WireType::VarInt:
                        case WireType::Fixed32:
                        case WireType::Fixed64:
                            transform.Field(id, it->metadata, value<uint32_t, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_UINT64:
                        switch (wire_type)
                        {
                        case WireType::VarInt:
                        case WireType::Fixed32:
                        case WireType::Fixed64:
                            transform.Field(id, it->metadata, value<uint64_t, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_FLOAT:
                        switch (wire_type)
                        {
                        case WireType::Fixed32:
                            transform.Field(id, it->metadata, value<float, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_DOUBLE:
                        switch (wire_type)
                        {
                        case WireType::Fixed64:
                            transform.Field(id, it->metadata, value<double, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_STRING:
                        switch (wire_type)
                        {
                        case WireType::LengthDelimited:
                            transform.Field(id, it->metadata, value<std::string, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_WSTRING:
                        BOOST_ASSERT(false);
                        break;

                    case BT_INT8:
                        switch (wire_type)
                        {
                        case WireType::VarInt:
                            _input.SetEncoding(detail::proto::ReadEncoding(BT_INT8, it->metadata));
                            // fall-through
                        case WireType::Fixed32:
                        case WireType::Fixed64:
                            transform.Field(id, it->metadata, value<int8_t, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_INT16:
                        switch (wire_type)
                        {
                        case WireType::VarInt:
                            _input.SetEncoding(detail::proto::ReadEncoding(BT_INT16, it->metadata));
                            // fall-through
                        case WireType::Fixed32:
                        case WireType::Fixed64:
                            transform.Field(id, it->metadata, value<int16_t, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_INT32:
                        switch (wire_type)
                        {
                        case WireType::VarInt:
                            _input.SetEncoding(detail::proto::ReadEncoding(BT_INT32, it->metadata));
                            // fall-through
                        case WireType::Fixed32:
                        case WireType::Fixed64:
                            transform.Field(id, it->metadata, value<int32_t, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_INT64:
                        switch (wire_type)
                        {
                        case WireType::VarInt:
                            _input.SetEncoding(detail::proto::ReadEncoding(BT_INT64, it->metadata));
                            // fall-through
                        case WireType::Fixed32:
                        case WireType::Fixed64:
                            transform.Field(id, it->metadata, value<int64_t, Input&>(_input));
                            continue;
                        }
                        break;

                    case BT_STRUCT:
                        switch (wire_type)
                        {
                        case WireType::LengthDelimited:
                            transform.Field(id, it->metadata, bonded<void, Input>(_input, RuntimeSchema{ schema, *it }));
                            continue;
                        }
                        break;

                    case BT_LIST:
                    case BT_SET:
                    case BT_MAP:
                        // TODO:
                        BOOST_ASSERT(false); // Not implemented
                        //transform.Field(id, it->metadata, value<void, Input>(_input, RuntimeSchema{ schema, *it }));
                        break;

                    default:
                        BOOST_ASSERT(false);
                        break;
                    }
                }
                else
                {
                    type = BT_UNAVAILABLE;
                }

                transform.UnknownField(id, value<void, Input>(_input, type));
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
              _encoding{ detail::proto::Unavailable<Encoding>() }
        {}

        void ReadStructBegin(bool base = false)
        {
            BOOST_VERIFY(!base);

            if (_type == WireType::LengthDelimited)
            {
                uint32_t size;
                ReadVarInt(size);
            }
        }

        void ReadStructEnd(bool base = false)
        {
            BOOST_VERIFY(!base);
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

        void ReadFieldEnd()
        {}

        void ReadContainerBegin(uint32_t& /*size*/, BondDataType& /*type*/)
        {
            // TODO:
        }

        void ReadContainerBegin(uint32_t& /*size*/, std::pair<BondDataType, BondDataType>& /*type*/)
        {
            // TODO:
        }

        void ReadContainerEnd()
        {}

        void Read(bool& value)
        {
            BOOST_ASSERT(_encoding == detail::proto::Unavailable<Encoding>());

            switch (_type)
            {
            case WireType::VarInt:
                {
                    uint8_t value8;
                    ReadVarInt(value8);
                    BOOST_ASSERT(value8 == 0 || value8 == 1);
                    value = (value8 == 1);
                }
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
            BOOST_ASSERT(is_signed_int<T>::value || _encoding == detail::proto::Unavailable<Encoding>());

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

        void Read(float& value)
        {
            BOOST_ASSERT(_encoding == detail::proto::Unavailable<Encoding>());

            switch (_type)
            {
            case WireType::Fixed32:
                _input.Read(value);
                break;

            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        void Read(double& value)
        {
            BOOST_ASSERT(_encoding == detail::proto::Unavailable<Encoding>());

            switch (_type)
            {
            case WireType::Fixed64:
                _input.Read(value);
                break;

            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        template <typename T>
        typename boost::enable_if<is_string_type<T> >::type
        Read(T& value)
        {
            switch (_type)
            {
            case WireType::LengthDelimited:
                {
                    uint32_t length = 0;
                    ReadVarInt(length);
                    detail::ReadStringData(_input, value, length);
                }
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
        template <typename T>
        typename boost::enable_if<std::is_unsigned<T> >::type
        ReadVarInt(T& value)
        {
            ReadVariableUnsigned(_input, value);
        }

        template <typename T>
        typename boost::enable_if<is_signed_int<T> >::type
        ReadVarInt(T& value)
        {
            uint64_t value64;
            ReadVarInt(value64);
            value = _encoding == Encoding::ZigZag
                ? static_cast<T>(DecodeZigZag(value64))
                : static_cast<T>(value64);
        }

        void ReadVarInt(uint8_t& value)
        {
            uint16_t value16;
            ReadVarInt(value16);
            BOOST_ASSERT(value16 <= (std::numeric_limits<uint8_t>::max)());
            value = static_cast<uint8_t>(value16);
        }

        template <typename T>
        typename boost::enable_if<std::is_unsigned<T> >::type
        ReadFixed32(T& value)
        {
            uint32_t value32;
            _input.Read(value32);
            value = static_cast<T>(value32);
        }

        template <typename T>
        typename boost::enable_if<is_signed_int<T> >::type
        ReadFixed32(T& value)
        {
            uint32_t value32;
            _input.Read(value32);
            value = _encoding == Encoding::ZigZag
                ? static_cast<T>(DecodeZigZag(value32))
                : static_cast<T>(value32);
        }

        void ReadFixed32(uint32_t& value)
        {
            _input.Read(value);
        }

        template <typename T>
        typename boost::enable_if<std::is_unsigned<T> >::type
        ReadFixed64(T& value)
        {
            uint64_t value64;
            _input.Read(value64);
            value = static_cast<T>(value64);
        }

        template <typename T>
        typename boost::enable_if<is_signed_int<T> >::type
        ReadFixed64(T& value)
        {
            uint64_t value64;
            _input.Read(value64);
            value = _encoding == Encoding::ZigZag
                ? static_cast<T>(DecodeZigZag(value64))
                : static_cast<T>(value64);
        }

        void ReadFixed64(uint64_t& value)
        {
            _input.Read(value);
        }

        bool ReadTag()
        {
            if (!_input.IsEof())
            {
                uint64_t tag;
                ReadVariableUnsigned(_input, tag);

                auto raw_type = static_cast<WireType>(tag & 0x7);
                if (raw_type != WireType::VarInt && raw_type != WireType::Fixed64
                    && raw_type != WireType::LengthDelimited && raw_type != WireType::Fixed32)
                {
                    BOND_THROW(CoreException, "Unrecognized wire type.");
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
                break;

            case WireType::LengthDelimited:
                {
                    uint32_t size;
                    ReadVarInt(size);
                    _input.Skip(size);
                }
                break;

            case WireType::Fixed32:
                _input.Skip(sizeof(uint32_t));
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
    };

} // namespace bond

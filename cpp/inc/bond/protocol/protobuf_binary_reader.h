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
        bool ReadFields(const RuntimeSchema& schema, const Transform& transform)
        {
            const auto& fields = schema.GetStruct().fields;

            WireType wire_type;
            uint16_t id;

            for (auto it = fields.begin(); _input.ReadFieldBegin(wire_type, id); _input.ReadFieldEnd())
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
                        switch (wire_type)
                        {
                        case WireType::LengthDelimited:
                            transform.Field(id, it->metadata, value<std::wstring, Input&>(_input));
                            continue;
                        }
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
                        BOOST_ASSERT(it->type.element.hasvalue());
                        _input.SetEncoding(detail::proto::ReadEncoding(it->type.element->id, it->metadata));
                        transform.Field(id, it->metadata, value<void, Input>(_input, RuntimeSchema{ schema, *it }));
                        continue;

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
              _wire{ detail::proto::Unavailable<WireType>() },
              _id{ 0 },
              _encoding{ detail::proto::Unavailable<Encoding>() },
              _size{ 0 }
        {}

        void ReadStructBegin(bool base = false)
        {
            BOOST_VERIFY(!base);

            if (_size != 0)
            {
                BOOST_ASSERT(_wire == WireType::LengthDelimited);

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
                type = _wire;
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

            switch (_wire)
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
            //BOOST_ASSERT(is_signed_int<T>::value || _encoding == detail::proto::Unavailable<Encoding>());

            switch (_wire)
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
            //BOOST_ASSERT(_encoding == detail::proto::Unavailable<Encoding>());

            switch (_wire)
            {
            case WireType::Fixed32:
                ReadFixed32(value);
                break;

            default:
                BOOST_ASSERT(false);
                break;
            }
        }

        void Read(double& value)
        {
            //BOOST_ASSERT(_encoding == detail::proto::Unavailable<Encoding>());

            switch (_wire)
            {
            case WireType::Fixed64:
                ReadFixed64(value);
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
            switch (_wire)
            {
            case WireType::LengthDelimited:
                BOOST_ASSERT(_size != 0);
                detail::ReadStringData(_input, value, _size);
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

        template <typename T>
        void Skip()
        {
            Skip();
        }

        template <typename T>
        void Skip(const bonded<T, ProtobufBinaryReader&>&)
        {
            BOOST_ASSERT(_wire == WireType::LengthDelimited);
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

        template <typename T>
        typename boost::enable_if<std::is_unsigned<T> >::type
        ReadVarInt(T& value)
        {
            Consume(ReadVariableUnsigned(_input, value));
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
            Consume(sizeof(uint32_t));
            uint32_t value32;
            _input.Read(value32);
            value = static_cast<T>(value32);
        }

        template <typename T>
        typename boost::enable_if<is_signed_int<T> >::type
        ReadFixed32(T& value)
        {
            Consume(sizeof(uint32_t));
            uint32_t value32;
            _input.Read(value32);
            value = _encoding == Encoding::ZigZag
                ? static_cast<T>(DecodeZigZag(value32))
                : static_cast<T>(value32);
        }

        void ReadFixed32(uint32_t& value)
        {
            Consume(sizeof(uint32_t));
            _input.Read(value);
        }

        void ReadFixed32(float& value)
        {
            Consume(sizeof(uint32_t));
            _input.Read(value);
        }

        template <typename T>
        typename boost::enable_if<std::is_unsigned<T> >::type
        ReadFixed64(T& value)
        {
            Consume(sizeof(uint64_t));
            uint64_t value64;
            _input.Read(value64);
            value = static_cast<T>(value64);
        }

        template <typename T>
        typename boost::enable_if<is_signed_int<T> >::type
        ReadFixed64(T& value)
        {
            Consume(sizeof(uint64_t));
            uint64_t value64;
            _input.Read(value64);
            value = _encoding == Encoding::ZigZag
                ? static_cast<T>(DecodeZigZag(value64))
                : static_cast<T>(value64);
        }

        void ReadFixed64(uint64_t& value)
        {
            Consume(sizeof(uint64_t));
            _input.Read(value);
        }

        void ReadFixed64(double& value)
        {
            Consume(sizeof(uint64_t));
            _input.Read(value);
        }

        bool ReadTag()
        {
            if ((!_lengths || _lengths->empty() || _lengths->top(std::nothrow) != 0) && !_input.IsEof())
            {
                uint64_t tag;
                ReadVarInt(tag);

                auto raw_wire = static_cast<WireType>(tag & 0x7);
                switch (raw_wire)
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

                _wire = static_cast<WireType>(raw_wire);
                _id = static_cast<uint16_t>(raw_id);
                _encoding = detail::proto::Unavailable<Encoding>();

                return true;
            }

            return false;
        }

        void Skip()
        {
            switch (_wire)
            {
            case WireType::VarInt:
                {
                    uint64_t value;
                    ReadVarInt(value);
                }
                break;

            case WireType::Fixed64:
                Consume(sizeof(uint64_t));
                _input.Skip(sizeof(uint64_t));
                break;

            case WireType::LengthDelimited:
                BOOST_ASSERT(_size != 0);
                Consume(_size);
                _input.Skip(_size);
                break;

            case WireType::Fixed32:
                Consume(sizeof(uint32_t));
                _input.Skip(sizeof(uint32_t));
                break;

            default:
                BOOST_ASSERT(false);
                break;
            }
        }


        Buffer _input;
        WireType _wire;
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
                                && is_bond_type<typename element_type<X>::type>::value>::type
    inline DeserializeContainer(X& var, const T& element, ProtobufBinaryReader<Buffer>& input)
    {
        DeserializeElements<Protocols>(var, element, input.GetSize());
    }

} // namespace bond

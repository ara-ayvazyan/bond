// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "detail/protobuf_field_enumerator.h"

#include <bond/core/bond_types.h>
#include <bond/core/value.h>


namespace bond
{
    namespace detail
    {
    namespace proto
    {
        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_BOOL), bool>::type
        inline MatchWireType(WireType type, Encoding encoding, bool /*strict*/ = true)
        {
            BOOST_VERIFY(encoding == Unavailable<Encoding>::value);
            return type == WireType::VarInt;
        }

        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_UINT8 || T == BT_INT8 || T == BT_UINT16 || T == BT_INT16), bool>::type
        inline MatchWireType(WireType type, Encoding encoding, bool strict)
        {
            if (strict)
            {
                switch (encoding)
                {
                case Encoding::Fixed:
                    return type == WireType::Fixed32;

                case Encoding::ZigZag:
                    BOOST_ASSERT(T == BT_INT8 || T == BT_INT16);
                default:
                    return type == WireType::VarInt;
                }
            }
            else
            {
                return type == WireType::VarInt || type == WireType::Fixed64 || type == WireType::Fixed32;
            }
        }

        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_UINT32 || T == BT_INT32 || T == BT_UINT64 || T == BT_INT64), bool>::type
        inline MatchWireType(WireType type, Encoding encoding, bool strict)
        {
            if (strict)
            {
                switch (encoding)
                {
                case Encoding::Fixed:
                    return type == (T == BT_UINT32 || T == BT_INT32 ? WireType::Fixed32 : WireType::Fixed64);

                case Encoding::ZigZag:
                    BOOST_ASSERT(T == BT_INT32 || T == BT_INT64);
                default:
                    return type == WireType::VarInt;
                }
            }
            else
            {
                return type == WireType::VarInt || type == WireType::Fixed64 || type == WireType::Fixed32;
            }
        }

        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_FLOAT || T == BT_DOUBLE), bool>::type
        inline MatchWireType(WireType type, Encoding encoding, bool strict)
        {
            BOOST_VERIFY(encoding == Encoding::Fixed);

            if (strict)
            {
                return type == (T == BT_FLOAT ? WireType::Fixed32 : WireType::Fixed64);
            }
            else
            {
                return type == WireType::Fixed64 || type == WireType::Fixed32;
            }
        }

        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_STRING || T == BT_WSTRING || T == BT_STRUCT || T == BT_MAP), bool>::type
        inline MatchWireType(WireType type, Encoding encoding = Unavailable<Encoding>::value, bool /*strict*/ = true)
        {
            BOOST_VERIFY(encoding == Unavailable<Encoding>::value);
            return type == WireType::LengthDelimited;
        }

        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_LIST), bool>::type
        inline MatchWireType(WireType type, Encoding encoding = Unavailable<Encoding>::value, bool /*strict*/ = true)
        {
            BOOST_VERIFY(encoding == Unavailable<Encoding>::value);
            return type == WireType::VarInt;
        }

        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_SET), bool>::type
        inline MatchWireType(WireType type, Encoding encoding = Unavailable<Encoding>::value, bool /*strict*/ = true)
        {
            BOOST_VERIFY(encoding == Unavailable<Encoding>::value);
            return type == WireType::VarInt;
        }


        template <BondDataType T>
        inline bool MatchWireType(WireType type, Encoding encoding, Packing packing, bool strict)
        {
            switch (packing)
            {
            case Packing::False:
                return MatchWireType<T>(type, encoding, strict);

            default:
                return type == WireType::LengthDelimited;
            }
        }


        inline bool MatchWireType(BondDataType bond_type, WireType type, Encoding encoding, bool strict)
        {
            switch (bond_type)
            {
            case BT_BOOL:
                return MatchWireType<BT_BOOL>(type, encoding, strict);

            case BT_UINT8:
                return MatchWireType<BT_UINT8>(type, encoding, strict);

            case BT_UINT16:
                return MatchWireType<BT_UINT16>(type, encoding, strict);

            case BT_UINT32:
                return MatchWireType<BT_UINT32>(type, encoding, strict);

            case BT_UINT64:
                return MatchWireType<BT_UINT64>(type, encoding, strict);

            case BT_INT8:
                return MatchWireType<BT_INT8>(type, encoding, strict);

            case BT_INT16:
                return MatchWireType<BT_INT16>(type, encoding, strict);

            case BT_INT32:
                return MatchWireType<BT_INT32>(type, encoding, strict);

            case BT_INT64:
                return MatchWireType<BT_INT64>(type, encoding, strict);

            case BT_FLOAT:
                return MatchWireType<BT_FLOAT>(type, encoding, strict);

            case BT_DOUBLE:
                return MatchWireType<BT_DOUBLE>(type, encoding, strict);

            case BT_STRING:
                return MatchWireType<BT_STRING>(type, encoding, strict);

            case BT_WSTRING:
                return MatchWireType<BT_WSTRING>(type, encoding, strict);

            case BT_STRUCT:
                return MatchWireType<BT_STRUCT>(type, encoding, strict);

            case BT_LIST:
                return MatchWireType<BT_LIST>(type, encoding, strict);

            case BT_SET:
                return MatchWireType<BT_SET>(type, encoding, strict);

            case BT_MAP:
                return MatchWireType<BT_MAP>(type, encoding, strict);

            default:
                BOOST_ASSERT(false);
                return false;
            }
        }


        inline bool MatchWireType(BondDataType bond_type, WireType type, Encoding encoding, Packing packing, bool strict)
        {
            switch (packing)
            {
            case Packing::False:
                return MatchWireType(bond_type, type, encoding, strict);

            default:
                return type == WireType::LengthDelimited;
            }
        }


        template <typename FieldT, typename Transform, typename T>
        typename boost::enable_if<is_fast_path_field<FieldT, Transform>, bool>::type
        inline Field(const Transform& transform, const T& value)
        {
            return transform.Field(FieldT{}, value);
        }

        template <typename FieldT, typename Transform, typename T>
        typename boost::disable_if<is_fast_path_field<FieldT, Transform>, bool>::type
        inline Field(const Transform& transform, const T& value)
        {
            return transform.Field(FieldT::id, FieldT::metadata, value);
        }


        template <typename T, typename Enable = void> struct
        is_blob_type
            : std::false_type {};

        template <typename T> struct
        is_blob_type<T, typename boost::enable_if<is_list_container<T> >::type>
            : std::integral_constant<bool,
                (get_type_id<typename element_type<T>::type>::value == BT_INT8)> {};


        template <typename T, typename Enable = void> struct
        is_nested_blob_type
            : std::false_type {};

        template <typename T> struct
        is_nested_blob_type<T, typename boost::enable_if<is_list_container<T> >::type>
            : is_blob_type<typename element_type<T>::type> {};

        template <typename T> struct
        is_nested_blob_type<T, typename boost::enable_if<is_set_container<T> >::type>
            : is_blob_type<typename element_type<T>::type> {};

    } // namespace proto
    } // namespace detail


    template <typename T>
    class UnorderedRequiredFieldValiadator
    {
    protected:
        template <typename U>
        struct rebind
        {
            using type = UnorderedRequiredFieldValiadator<U>;
        };

        void Begin(const T& /*var*/) const
        {
            // TODO:
        }

        template <typename Head>
        typename boost::enable_if<std::is_same<typename Head::field_modifier,
                                               reflection::required_field_modifier> >::type
        Validate() const
        {
            // TODO:
        }

        template <typename Schema>
        typename boost::enable_if_c<next_required_field<typename Schema::fields>::value
                                    != invalid_field_id>::type
        Validate() const
        {
            // TODO:
        }

        template <typename Head>
        typename boost::disable_if<std::is_same<typename Head::field_modifier,
                                                reflection::required_field_modifier> >::type
        Validate() const
        {}

        template <typename Schema>
        typename boost::disable_if_c<next_required_field<typename Schema::fields>::value
                                    != invalid_field_id>::type
        Validate() const
        {}

    private:
    };


    template <typename Input>
    class ProtobufParser
    {
        using WireType = detail::proto::WireType;
        using Encoding = detail::proto::Encoding;
        using Packing = detail::proto::Packing;

    public:
        ProtobufParser(Input input, bool base)
            : _input{ input },
              _strict_match{ input._strict_match }
        {
            BOOST_ASSERT(!base);
        }

        template <typename Transform, typename Schema>
        bool Apply(const Transform& transform, const Schema& schema)
        {
            detail::StructBegin(_input, false);
            Read(schema, transform);
            detail::StructEnd(_input, false);

            return false;
        }

    private:
        template <typename Transform>
        class WireTypeFieldBinder
        {
        public:
            WireTypeFieldBinder(WireType type, Encoding encoding, bool strict_match, const Transform& transform)
                : _type{ type },
                  _encoding{ encoding },
                  _strict_match{ strict_match },
                  _transform{ transform }
            {}

            template <typename T>
            bool Field(uint16_t id, const Metadata& metadata, const value<T, Input&>& value) const
            {
                BOOST_STATIC_ASSERT(is_basic_type<T>::value);

                if (detail::proto::MatchWireType<get_type_id<T>::value>(_type, _encoding, _strict_match))
                {
                    _transform.Field(id, metadata, value);
                    return true;
                }

                return false;
            }

        private:
            const WireType _type;
            const Encoding _encoding;
            const bool _strict_match;
            const Transform& _transform;
        };

        template <typename Transform>
        WireTypeFieldBinder<Transform> BindWireTypeField(
            WireType type, Encoding encoding, bool strict_match, const Transform& transform)
        {
            return WireTypeFieldBinder<Transform>{ type, encoding, strict_match, transform };
        }


        // use compile-time schema
        template <typename Schema, typename Transform>
        void Read(const Schema&, const Transform& transform)
        {
            BOOST_STATIC_ASSERT(std::is_same<typename Schema::base, no_base>::value);

            transform.Begin(Schema::metadata);
            ReadFields(Schema{}, transform);
            transform.End();
        }

        template <typename Schema, typename Transform>
        void ReadFields(const Schema&, const Transform& transform)
        {
            WireType type;
            uint16_t id;

            if (_input.ReadFieldBegin(type, id))
            {
                while (ReadFields<Schema>(typename boost::mpl::begin<typename Schema::fields>::type{}, transform, type, id))
                {}
            }
        }

        template <typename Schema, typename Fields, typename Transform>
        bool ReadFields(const Fields&, const Transform& transform, WireType& type, uint16_t& id)
        {
            using Head = typename boost::mpl::deref<Fields>::type;

            do
            {
                if (Head::id == id)
                {
                    if (!Field<Head>(transform, type))
                    {
                        transform.UnknownField(Head::id, value<void, Input>{ _input, BT_UNAVAILABLE });
                    }
                }
                else if (Head::id < id)
                {
                    return ReadFields<Schema>(typename boost::mpl::next<Fields>::type{}, transform, type, id);
                }
                else // Head::id > id
                {
                    return !std::is_same<Fields, typename boost::mpl::begin<typename Schema::fields>::type>::value;
                }
            }
            while (_input.ReadFieldEnd(), _input.ReadFieldBegin(type, id));

            return false;
        }

        template <typename Schema, typename Transform>
        bool ReadFields(const boost::mpl::l_iter<boost::mpl::l_end>&, const Transform& transform, WireType& type, uint16_t& id)
        {
            transform.UnknownField(id, value<void, Input>{ _input, BT_UNAVAILABLE });
            return _input.ReadFieldEnd(), _input.ReadFieldBegin(type, id);
        }


        template <typename T, typename Transform>
        typename boost::enable_if<is_basic_type<typename T::field_type>, bool>::type
        Field(const Transform& transform, WireType type)
        {
            Encoding encoding = detail::proto::ReadEncoding(get_type_id<typename T::field_type>::value, &T::metadata);

            _input.SetEncoding(encoding);

            if (detail::proto::MatchWireType<get_type_id<typename T::field_type>::value>(type, encoding, _strict_match))
            {
                detail::proto::Field<T>(transform, value<typename T::field_type, Input&>{ _input });
                return true;
            }

            return false;
        }


        template <typename T, typename Transform>
        typename boost::enable_if<is_bond_type<typename T::field_type>, bool>::type
        Field(const Transform& transform, WireType type)
        {
            if (detail::proto::MatchWireType<BT_STRUCT>(type))
            {
                detail::proto::Field<T>(transform, bonded<typename T::field_type, Input&>{ _input });
                return true;
            }

            return false;
        }


        template <typename T, typename Transform>
        typename boost::enable_if_c<is_list_container<typename T::field_type>::value
                                    || is_set_container<typename T::field_type>::value, bool>::type
        Field(const Transform& transform, WireType type)
        {
            bool matched;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127) // C4127: conditional expression is constant
#endif
            if (detail::proto::is_blob_type<typename T::field_type>::value
                || detail::proto::is_nested_blob_type<typename T::field_type>::value)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
            {
                matched = true;
            }
            else
            {
                Packing packing = _strict_match
                    ? detail::proto::ReadPacking(get_type_id<typename element_type<typename T::field_type>::type>::value, &T::metadata)
                    : detail::proto::Unavailable<Packing>::value;

                Encoding encoding = detail::proto::ReadEncoding(get_type_id<typename element_type<typename T::field_type>::type>::value, &T::metadata);

                _input.SetEncoding(encoding);

                matched = detail::proto::MatchWireType<get_type_id<typename element_type<typename T::field_type>::type>::value>(type, encoding, packing, _strict_match);
            }

            if (matched)
            {
                detail::proto::Field<T>(transform, value<typename T::field_type, Input&>{ _input });
                return true;
            }

            return false;
        }


        template <typename T, typename Transform>
        typename boost::enable_if<is_map_container<typename T::field_type>, bool>::type
        Field(const Transform& transform, WireType type)
        {
            if (detail::proto::MatchWireType<BT_MAP>(type))
            {
                _input.SetKeyEncoding(detail::proto::ReadKeyEncoding(
                    get_type_id<typename element_type<typename T::field_type>::type::first_type>::value, &T::metadata));
                _input.SetEncoding(detail::proto::ReadValueEncoding(
                    get_type_id<typename element_type<typename T::field_type>::type::second_type>::value, &T::metadata));

                // TODO: match by element type
                detail::proto::Field<T>(transform, value<typename T::field_type, Input&>{ _input });

                return true;
            }

            return false;
        }


        // use runtime schema
        template <typename Transform>
        void Read(const RuntimeSchema& schema, const Transform& transform)
        {
            if (schema.HasBase())
            {
                detail::proto::NotSupportedException("Inheritance");
            }

            transform.Begin(schema.GetStruct().metadata);
            ReadFields(schema, transform);
            transform.End();
        }

        template <typename Transform>
        void ReadFields(const RuntimeSchema& schema, const Transform& transform)
        {
            WireType type;
            uint16_t id;

            for (auto fields = detail::proto::EnumerateFields(schema); _input.ReadFieldBegin(type, id); _input.ReadFieldEnd())
            {
                if (const FieldDef* field = fields.find(id))
                {
                    if (field->type.id == BT_STRUCT)
                    {
                        if (detail::proto::MatchWireType<BT_STRUCT>(type))
                        {
                            transform.Field(id, field->metadata, bonded<void, Input>{ _input, RuntimeSchema{ schema, *field } });
                            continue;
                        }
                    }
                    else if (field->type.id == BT_LIST || field->type.id == BT_SET)
                    {
                        BOOST_ASSERT(field->type.element.hasvalue());

                        bool matched;

                        if ((field->type.id == BT_LIST && field->type.element->id == BT_INT8)                       // blob
                            || (field->type.element->id == BT_LIST && field->type.element->element->id == BT_INT8)) // nested blobs
                        {
                            matched = true;
                        }
                        else
                        {
                            Packing packing = _strict_match
                                ? detail::proto::ReadPacking(field->type.element->id, &field->metadata)
                                : detail::proto::Unavailable<Packing>::value;

                            Encoding encoding = detail::proto::ReadEncoding(field->type.element->id, &field->metadata);

                            _input.SetEncoding(encoding);

                            matched = detail::proto::MatchWireType(field->type.element->id, type, encoding, packing, _strict_match);
                        }

                        if (matched)
                        {
                            transform.Field(id, field->metadata, value<void, Input>{ _input, RuntimeSchema{ schema, *field } });
                            continue;
                        }
                    }
                    else if (field->type.id == BT_MAP)
                    {
                        if (detail::proto::MatchWireType<BT_MAP>(type))
                        {
                            // TODO: match by element type

                            BOOST_ASSERT(field->type.key.hasvalue());
                            _input.SetKeyEncoding(detail::proto::ReadKeyEncoding(field->type.key->id, &field->metadata));

                            BOOST_ASSERT(field->type.element.hasvalue());
                            _input.SetEncoding(detail::proto::ReadValueEncoding(field->type.element->id, &field->metadata));

                            transform.Field(id, field->metadata, value<void, Input>{ _input, RuntimeSchema{ schema, *field } });
                            continue;
                        }
                    }
                    else
                    {
                        Encoding encoding = detail::proto::ReadEncoding(field->type.id, &field->metadata);

                        _input.SetEncoding(encoding);

                        if (detail::BasicTypeField(
                                id,
                                field->metadata,
                                field->type.id,
                                BindWireTypeField(type, encoding, _strict_match, transform),
                                _input))
                        {
                            continue;
                        }
                    }
                }

                transform.UnknownField(id, value<void, Input>{ _input, BT_UNAVAILABLE });
            }
        }


        Input _input;
        const bool _strict_match;
    };

} // namespace bond

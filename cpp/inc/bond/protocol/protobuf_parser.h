// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include <bond/core/detail/parser_utils.h>
#include <bond/core/bond_types.h>
#include <bond/core/value.h>


namespace bond
{
    namespace detail
    {
    namespace proto
    {
#if defined(_MSC_VER) && (_MSC_VER < 1900)
        // Using BondDataType directly in non-trivial boolean template checks fails on VC12.
        using BT = std::underlying_type<BondDataType>::type;
#else
        using BT = BondDataType;
#endif

        template <BT T>
        typename boost::enable_if_c<(T == BT_BOOL), bool>::type
        inline MatchWireType(WireType type, Encoding encoding, bool /*strict*/ = true)
        {
            BOOST_VERIFY(encoding == Unavailable<Encoding>::value);
            return type == WireType::VarInt;
        }

        template <BT T>
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
                    // fall-through
                default:
                    return type == WireType::VarInt;
                }
            }
            else
            {
                return type == WireType::VarInt || type == WireType::Fixed64 || type == WireType::Fixed32;
            }
        }

        template <BT T>
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
                    // fall-through
                default:
                    return type == WireType::VarInt;
                }
            }
            else
            {
                return type == WireType::VarInt || type == WireType::Fixed64 || type == WireType::Fixed32;
            }
        }

        template <BT T>
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

        template <BT T>
        typename boost::enable_if_c<(T == BT_STRING || T == BT_WSTRING || T == BT_STRUCT || T == BT_MAP), bool>::type
        inline MatchWireType(WireType type, Encoding encoding = Unavailable<Encoding>::value, bool /*strict*/ = true)
        {
            BOOST_VERIFY(encoding == Unavailable<Encoding>::value);
            return type == WireType::LengthDelimited;
        }

        template <BT T>
        typename boost::enable_if_c<(T == BT_LIST || T == BT_SET), bool>::type
        inline MatchWireType(WireType /*type*/, Encoding /*encoding*/, bool /*strict*/)
        {
            BOOST_ASSERT(false);
            return false;
        }

        template <BT T>
        inline bool MatchWireType(WireType type, Encoding encoding, Packing packing, bool strict)
        {
            if (strict)
            {
                switch (packing)
                {
                case Packing::False:
                    return MatchWireType<T>(type, encoding, true);

                default:
                    return type == WireType::LengthDelimited;
                }
            }
            else
            {
                return (type == WireType::LengthDelimited) || MatchWireType<T>(type, encoding, false);
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

            case BT_MAP:
                return MatchWireType<BT_MAP>(type, encoding, strict);

            case BT_LIST:
            case BT_SET:
            default:
                BOOST_ASSERT(false);
                return false;
            }
        }


        inline bool MatchWireType(BondDataType bond_type, WireType type, Encoding encoding, Packing packing, bool strict)
        {
            if (strict)
            {
                switch (packing)
                {
                case Packing::False:
                    return MatchWireType(bond_type, type, encoding, true);

                default:
                    return type == WireType::LengthDelimited;
                }
            }
            else
            {
                return (type == WireType::LengthDelimited) || MatchWireType(bond_type, type, encoding, false);
            }
        }


        template <typename T, typename Enable = void> struct
        is_nested_blob_type
            : std::false_type {};

        template <typename T> struct
        is_nested_blob_type<T, typename boost::enable_if_c<
                is_list_container<T>::value || is_set_container<T>::value>::type>
            : is_blob_type<typename element_type<T>::type> {};

    } // namespace proto
    } // namespace detail


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
        bool ReadFields(const Fields&, const Transform& transform, WireType& type, uint16_t& id, bool passed = false)
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

                    if (!passed)
                    {
                        passed = true;
                    }
                }
                else if (Head::id < id)
                {
                    return ReadFields<Schema>(typename boost::mpl::next<Fields>::type{}, transform, type, id, passed);
                }
                else // Head::id > id
                {
                    return passed || ReadFields<Schema>(typename boost::mpl::end<typename Schema::fields>::type{}, transform, type, id);
                }
            }
            while (_input.ReadFieldEnd(), _input.ReadFieldBegin(type, id));

            return false;
        }

        template <typename Schema, typename Transform>
        bool ReadFields(const boost::mpl::l_iter<boost::mpl::l_end>&, const Transform& transform, WireType& type, uint16_t& id, bool /*passed*/ = false)
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
                detail::Field(T{}, transform, value<typename T::field_type, Input&>{ _input });
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
                detail::Field(T{}, transform, bonded<typename T::field_type, Input&>{ _input });
                return true;
            }

            return false;
        }


        template <typename T, typename Transform>
        typename boost::enable_if_c<is_list_container<typename T::field_type>::value
                                    || is_set_container<typename T::field_type>::value, bool>::type
        Field(const Transform& transform, WireType type)
        {
            using is_blob = std::integral_constant<bool,
                detail::proto::is_blob_type<typename T::field_type>::value
                || detail::proto::is_nested_blob_type<typename T::field_type>::value>;

            bool matched;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127) // C4127: conditional expression is constant
#endif
            if (is_blob::value)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
            {
                matched = (type == WireType::LengthDelimited);
            }
            else
            {
                using element = typename element_type<typename T::field_type>::type;

                BOOST_STATIC_ASSERT(is_blob::value
                    || (get_type_id<element>::value != BT_LIST && get_type_id<element>::value != BT_SET && get_type_id<element>::value != BT_MAP));

                Packing packing = _strict_match
                    ? detail::proto::ReadPacking(get_type_id<element>::value, &T::metadata)
                    : detail::proto::Unavailable<Packing>::value;

                Encoding encoding = detail::proto::ReadEncoding(get_type_id<element>::value, &T::metadata);

                _input.SetEncoding(encoding);

                matched = detail::proto::MatchWireType<get_type_id<element>::value>(type, encoding, packing, _strict_match);
            }

            if (matched)
            {
                detail::Field(T{}, transform, value<typename T::field_type, Input&>{ _input });
                return true;
            }

            return false;
        }


        template <typename T, typename Transform>
        typename boost::enable_if<is_map_container<typename T::field_type>, bool>::type
        Field(const Transform& transform, WireType type)
        {
            BOOST_STATIC_ASSERT(!std::is_floating_point<typename T::field_type::key_type>::value);

            BOOST_STATIC_ASSERT(detail::proto::is_blob_type<typename T::field_type::mapped_type>::value
                                || !is_container<typename T::field_type::mapped_type>::value);

            if (detail::proto::MatchWireType<BT_MAP>(type))
            {
                _input.SetKeyEncoding(detail::proto::ReadKeyEncoding(
                    get_type_id<typename element_type<typename T::field_type>::type::first_type>::value, &T::metadata));
                _input.SetEncoding(detail::proto::ReadValueEncoding(
                    get_type_id<typename element_type<typename T::field_type>::type::second_type>::value, &T::metadata));

                detail::Field(T{}, transform, value<typename T::field_type, Input&>{ _input });

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
            const auto& fields = schema.GetStruct().fields;

            WireType type;
            uint16_t id;

            for (auto it = fields.begin(); _input.ReadFieldBegin(type, id); _input.ReadFieldEnd())
            {
                if (it == fields.end()
                    || (it->id != id && (++it == fields.end() || it->id != id)))
                {
                    it = std::lower_bound(fields.begin(), fields.end(), id,
                        [](const FieldDef& f, uint16_t id) { return f.id < id; });
                }

                if (const FieldDef* field = (it != fields.end() && it->id == id ? &*it : nullptr))
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
                            matched = (type == WireType::LengthDelimited);
                        }
                        else
                        {
                            if (field->type.element->id == BT_LIST || field->type.element->id == BT_SET || field->type.element->id == BT_MAP)
                            {
                                detail::proto::NotSupportedException("Container nesting");
                            }

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
                            if (field->type.element->id == BT_SET
                                || field->type.element->id == BT_MAP
                                || (field->type.element->id == BT_LIST && field->type.element->element->id != BT_INT8)) // nested blob
                            {
                                detail::proto::NotSupportedException("Container nesting");
                            }

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

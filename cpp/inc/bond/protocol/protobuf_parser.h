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
        inline MatchWireType(WireType type)
        {
            return type == WireType::VarInt;
        }

        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_UINT8 || T == BT_UINT16 || T == BT_UINT32 || T == BT_UINT64
                                    || T == BT_INT8 || T == BT_INT16 || T == BT_INT32 || T == BT_INT64), bool>::type
        inline MatchWireType(WireType type)
        {
            return type == WireType::VarInt || type == WireType::Fixed64 || type == WireType::Fixed32;
        }

        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_FLOAT || T == BT_DOUBLE), bool>::type
        inline MatchWireType(WireType type)
        {
            return type == WireType::Fixed64 || type == WireType::Fixed32;
        }

        template <BondDataType T>
        typename boost::enable_if_c<(T == BT_STRING || T == BT_WSTRING || T == BT_STRUCT), bool>::type
        inline MatchWireType(WireType type)
        {
            return type == WireType::LengthDelimited;
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
        template <typename Transform>
        class WireTypeFieldBinder
        {
        public:
            WireTypeFieldBinder(WireType type, const Transform& transform)
                : _type{ type },
                  _transform{ transform }
            {}

            template <typename T>
            bool Field(uint16_t id, const Metadata& metadata, const value<T, Input&>& value) const
            {
                if (detail::proto::MatchWireType<get_type_id<T>::value>(_type))
                {
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

        // use compile-time schema
        template <typename Schema, typename Transform>
        bool Read(const Schema&, const Transform& transform)
        {
            transform.Begin(Schema::metadata);
            bool done = ReadFields(Schema{}, transform);
            transform.End();
            return done;
        }

        template <typename Schema, typename Transform>
        bool ReadFields(const Schema&, const Transform& transform)
        {
            WireType type;
            uint16_t id;

            if (_input.ReadFieldBegin(type, id))
            {
                ReadFields<Schema>(typename boost::mpl::begin<typename Schema::fields>::type{}, transform, type, id);
            }

            return false;
        }

        template <typename Schema, typename Fields, typename Transform>
        bool ReadFields(const Fields&, const Transform& transform, WireType& type, uint16_t& id)
        {
            using Head = typename boost::mpl::deref<Fields>::type;

            do
            {
                if (Head::id == id)
                {
                    if (!NextField(Head{}, transform, type))
                    {
                        transform.UnknownField(id, value<void, Input>{ _input, BT_UNAVAILABLE });
                    }
                }
                else // TODO:
                {
                    return ReadFields<Schema>(typename boost::mpl::next<Fields>::type{}, transform, type, id);
                }
            }
            while (_input.ReadFieldEnd(), _input.ReadFieldBegin(type, id));

            return false;
        }

        template <typename Schema, typename Transform>
        bool ReadFields(const boost::mpl::l_iter<boost::mpl::l_end>&, const Transform&, WireType&, uint16_t&)
        {
            // TODO:
            return false;
        }


        template <typename T, typename Transform>
        typename boost::enable_if<is_basic_type<typename T::field_type>, bool>::type
        NextField(const T&, const Transform& transform, WireType type)
        {
            _input.SetEncoding(detail::proto::ReadEncoding(get_type_id<typename T::field_type>::value, &T::metadata));

            if (detail::proto::MatchWireType<get_type_id<typename T::field_type>::value>(type))
            {
                detail::proto::Field<T>(transform, value<typename T::field_type, Input&>{ _input });
                return true;
            }

            return false;
        }


        template <typename T, typename Transform>
        typename boost::enable_if<is_bond_type<typename T::field_type>, bool>::type
        NextField(const T&, const Transform& transform, WireType type)
        {
            if (detail::proto::MatchWireType<get_type_id<typename T::field_type>::value>(type))
            {
                detail::proto::Field<T>(transform, bonded<typename T::field_type, Input&>{ _input });
                return true;
            }

            return false;
        }


        template <typename T, typename Transform>
        typename boost::enable_if<is_list_container<typename T::field_type>, bool>::type
        NextField(const T&, const Transform& transform, WireType type)
        {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127) // C4127: conditional expression is constant
#endif
            if (get_type_id<typename element_type<typename T::field_type>::type>::value != BT_INT8)
            {
                _input.SetEncoding(detail::proto::ReadEncoding(get_type_id<typename element_type<typename T::field_type>::type>::value, &T::metadata));
            }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

            // TODO: match by element type
            detail::proto::Field<T>(transform, value<typename T::field_type, Input&>{ _input });

            return true;
        }


        template <typename T, typename Transform>
        typename boost::enable_if<is_set_container<typename T::field_type>, bool>::type
        NextField(const T&, const Transform& transform, WireType type)
        {
            _input.SetEncoding(detail::proto::ReadEncoding(get_type_id<typename element_type<typename T::field_type>::type>::value, &T::metadata));

            // TODO: match by element type
            detail::proto::Field<T>(transform, value<typename T::field_type, Input&>{ _input });

            return true;
        }


        template <typename T, typename Transform>
        typename boost::enable_if<is_map_container<typename T::field_type>, bool>::type
        NextField(const T&, const Transform& transform, WireType type)
        {
            if (type == WireType::LengthDelimited)
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
        bool Read(const RuntimeSchema& schema, const Transform& transform)
        {
            if (schema.HasBase())
            {
                detail::proto::NotSupportedException("Inheritance");
            }

            transform.Begin(schema.GetStruct().metadata);
            bool done = ReadFields(schema, transform);
            transform.End();
            return done;
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
                        // TODO: match by element type

                        BOOST_ASSERT(field->type.element.hasvalue());
                        if (!(field->type.id == BT_LIST && field->type.element->id == BT_INT8)) // !blob
                        {
                            _input.SetEncoding(detail::proto::ReadEncoding(field->type.element->id, &field->metadata));
                        }

                        transform.Field(id, field->metadata, value<void, Input>{ _input, RuntimeSchema{ schema, *field } });
                        continue;
                    }
                    else if (field->type.id == BT_MAP)
                    {
                        switch (type)
                        {
                        case WireType::LengthDelimited:
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
                        _input.SetEncoding(detail::proto::ReadEncoding(field->type.id, &field->metadata));

                        if (detail::BasicTypeField(id, field->metadata, field->type.id, BindWireTypeField(type, transform), _input))
                        {
                            continue;
                        }
                    }
                }

                transform.UnknownField(id, value<void, Input>{ _input, BT_UNAVAILABLE });
            }

            return false;
        }


        Input _input;
    };

} // namespace bond

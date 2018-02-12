// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "detail/protobuf_field_enumerator.h"

#include <bond/core/bond_types.h>
#include <bond/core/value.h>


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

            template <typename T>
            typename boost::enable_if<std::is_floating_point<T>, bool>::type
            Field(uint16_t id, const Metadata& metadata, const value<T, Input&>& value) const
            {
                switch (_type)
                {
                case WireType::Fixed32:
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
                        // TODO: match by element type

                        BOOST_ASSERT(field->type.element.hasvalue());
                        _input.SetEncoding(detail::proto::ReadEncoding(field->type.element->id, &field->metadata));

                        transform.Field(id, field->metadata, value<void, Input>{ _input, RuntimeSchema{ schema, *field } });
                        continue;
                    }
                    else if (field->type.id == BT_MAP)
                    {
                        switch (type)
                        {
                        case WireType::LengthDelimited:
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

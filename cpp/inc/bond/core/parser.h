// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "detail/inheritance.h"
#include "detail/omit_default.h"
#include "detail/typeid_value.h"
#include "merge.h"
#include "reflection.h"
#include "schema.h"
#include "transforms.h"
#include "value.h"

#include <bond/protocol/simple_binary_impl.h>
#include <bond/protocol/simple_json_reader_impl.h>

namespace bond
{

namespace detail
{

class ParserCommon
{
protected:
    template <typename Schema>
    void SkipFields(const Schema& /*schema*/)
    {}

    template <typename T, typename Transform>
    typename boost::disable_if<is_fast_path_field<T, Transform>, bool>::type
    OmittedField(const Transform& transform)
    {
        return transform.OmittedField(T::id, T::metadata, get_type_id<typename T::field_type>::value);
    }


    template <typename T, typename Transform>
    typename boost::enable_if<is_fast_path_field<T, Transform>, bool>::type
    OmittedField(const Transform& transform)
    {
        return transform.OmittedField(T{});
    }

    template <typename Transform>
    struct UnknownFieldBinder
        : detail::nonassignable
    {
        UnknownFieldBinder(Transform& transform)
            : transform(transform)
        {}

        template <typename T>
        bool Field(uint16_t id, const Metadata& /*metadata*/, const T& value) const
        {
            return transform.UnknownField(id, value);
        }

        Transform& transform;
    };

    template <typename Transform>
    UnknownFieldBinder<Transform> BindUnknownField(Transform& transform)
    {
        return UnknownFieldBinder<Transform>(transform);
    }
};

} // namespace detail

//
// StaticParser iterates serialized data using type schema and calls
// specified transform for each data field.
// The schema may be provided at compile-time (schema<T>::type::fields) or at runtime
// (const RuntimeSchema&). StaticParser is used with protocols which don't
// tag fields in serialized format with ids or types, e.g. Apache Avro protocol.
//
template <typename Input>
class StaticParser
    : protected detail::ParserInheritance<Input, StaticParser<Input> >,
      public detail::ParserCommon
{
public:
    StaticParser(Input input, bool base = false)
        : detail::ParserInheritance<Input, StaticParser<Input> >(input, base)
    {}


    template <typename Schema, typename Transform>
    bool
    Apply(const Transform& transform, const Schema& schema)
    {
        detail::StructBegin(_input, _base);
        bool result = this->Read(schema, transform);
        detail::StructEnd(_input, _base);
        return result;
    }

    friend class detail::ParserInheritance<Input, StaticParser<Input> >;


protected:
    using detail::ParserInheritance<Input, StaticParser<Input> >::_input;
    using detail::ParserInheritance<Input, StaticParser<Input> >::_base;

private:
    template <typename Schema>
    void SkipFields(const Schema& schema)
    {
        // Skip the structure by reading fields to Null transform
        ReadFields(schema, Null());
    }

    // use compile-time schema
    template <typename Schema, typename Transform>
    bool
    ReadFields(Schema, const Transform& transform)
    {
        return ReadFields<Schema, 0>(transform);
    }

    template <typename Schema, uint16_t I, typename Transform, typename InputT = Input>
    typename boost::enable_if_c<(I < Schema::field_count::value)
        && detail::implements_field_omitting<InputT>::value, bool>::type
    ReadFields(const Transform& transform)
    {
        using Field = detail::field_info<Schema, I>;

        if (detail::ReadFieldOmitted(_input))
            OmittedField<Field>(transform);
        else
            if (bool done = NextField<Field>(transform))
                return done;

        return ReadFields<Schema, I + 1>(transform);
    }

    template <typename Schema, uint16_t I, typename Transform, typename InputT = Input>
    typename boost::enable_if_c<(I < Schema::field_count::value)
        && !detail::implements_field_omitting<InputT>::value, bool>::type
    ReadFields(const Transform& transform)
    {
        using Field = detail::field_info<Schema, I>;

        if (bool done = NextField<Field>(transform))
            return done;

        return ReadFields<Schema, I + 1>(transform);
    }

    template <typename Schema, uint16_t I, typename Transform>
    typename boost::enable_if_c<(I == Schema::field_count::value), bool>::type
    ReadFields(const Transform& /*transform*/)
    {
        return false;
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<is_reader<Input>::value && !is_nested_field<T>::value
                             && !is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform)
    {
        return transform.Field(T::id, T::metadata, value<typename T::field_type, Input>(_input));
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<is_reader<Input>::value && !is_nested_field<T>::value
                             && is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform)
    {
        return transform.Field(T{}, value<typename T::field_type, Input>(_input));
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<is_reader<Input>::value && is_nested_field<T>::value
                             && !is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform)
    {
        return transform.Field(T::id, T::metadata, bonded<typename T::field_type, Input>(_input));
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<is_reader<Input>::value && is_nested_field<T>::value
                             && is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform)
    {
        return transform.Field(T{}, bonded<typename T::field_type, Input>(_input));
    }


    template <typename T, typename Transform>
    typename boost::disable_if<is_reader<Input, T>, bool>::type
    NextField(const Transform& transform)
    {
        return transform.Field(T::id, T::metadata, T::GetVariable(_input));
    }


    // use runtime schema
    template <typename Transform>
    bool
    ReadFields(const RuntimeSchema& schema, const Transform& transform)
    {
        bool done = false;

        for (const_enumerator<std::vector<FieldDef> > enumerator(schema.GetStruct().fields); enumerator.more() && !done;)
        {
            const FieldDef& field = enumerator.next();

            if (detail::ReadFieldOmitted(_input))
            {
                transform.OmittedField(field.id, field.metadata, field.type.id);
                continue;
            }

            if (field.type.id == bond::BT_STRUCT)
            {
                done = transform.Field(field.id, field.metadata, bonded<void, Input>(_input, RuntimeSchema(schema, field)));
            }
            else if (field.type.id == bond::BT_LIST || field.type.id == bond::BT_SET || field.type.id == bond::BT_MAP)
            {
                done = transform.Field(field.id, field.metadata, value<void, Input>(_input, RuntimeSchema(schema, field)));
            }
            else
            {
                done = detail::BasicTypeField(field.id, field.metadata, field.type.id, transform, _input);
            }
        }

        return done;
    }
};


//
// DynamicParser iterates serialized data using field tags included in the
// data by the protocol and calls specified transform for each data field.
// DynamicParser uses schema only for auxiliary metadata, such as field
// names or modifiers, and determines what fields are present from the data itself.
// The schema may be provided at compile-time (schema<T>::type::fields) or at runtime
// (const RuntimeSchema&).
// DynamicParser is used with protocols which tag fields in serialized
// format with ids and types, e.g. Mafia, Thrift or Protocol Buffers.
//
template <typename Input>
class DynamicParser
    : protected detail::ParserInheritance<Input, DynamicParser<Input> >,
      public detail::ParserCommon
{
public:
    DynamicParser(Input input, bool base)
        : detail::ParserInheritance<Input, DynamicParser<Input> >(input, base)
    {}


    template <typename Schema, typename Transform>
    bool
    Apply(const Transform& transform, const Schema& schema)
    {
        detail::StructBegin(_input, _base);
        bool result = this->Read(schema, transform);
        detail::StructEnd(_input, _base);
        return result;
    }

    friend class detail::ParserInheritance<Input, DynamicParser<Input> >;


protected:
    using detail::ParserInheritance<Input, DynamicParser<Input> >::_input;
    using detail::ParserInheritance<Input, DynamicParser<Input> >::_base;


private:      
    template <typename Schema, typename Transform>
    bool 
    ReadFields(const Schema& schema, const Transform& transform)
    {
        uint16_t     id;
        BondDataType type;

        _input.ReadFieldBegin(type, id);

        ReadFields(schema, id, type, transform);

        bool done;

        if (!_base)
        {
            // If we are not parsing a base class, and we still didn't get to
            // the end of the struct, it means that:
            //
            // 1) Actual data in the payload had deeper hierarchy than payload schema.
            //
            // or
            //
            // 2) We parsed only part of the hierarchy because that was what
            //    the transform "expected".
            //
            // In both cases we emit remaining fields as unknown

            ReadAllUnknownFields(type, id, transform);
            done = false;
        }
        else
        {
            done = (type == bond::BT_STOP);
        }

        _input.ReadFieldEnd();

        return done;
    }


    // use compile-time schema
    template <typename Schema, typename Transform>
    void
    ReadFields(Schema, uint16_t& id, BondDataType& type, const Transform& transform)
    {
        ReadFields<Schema, 0>(id, type, transform);
    }


    template <typename Schema, uint16_t I, typename Transform>
    typename boost::enable_if_c<(I < Schema::field_count::value)>::type
    ReadFields(uint16_t& id, BondDataType& type, const Transform& transform)
    {
        using Field = detail::field_info<Schema, I>;

        for (;;)
        {
            if (Field::id == id && get_type_id<typename Field::field_type>::value == type)
            {
                // Exact match
                NextField<Field>(transform);
            }
            else if (Field::id >= id && type != bond::BT_STOP && type != bond::BT_STOP_BASE)
            {
                // Unknown field or non-exact type match
                UnknownFieldOrTypeMismatch<is_basic_type<typename Field::field_type>::value>(
                    Field::id,
                    Field::metadata,
                    id,
                    type,
                    transform);
            }
            else
            {
                OmittedField<Field>(transform);
                goto NextSchemaField;
            }

            ReadSubsequentField(type, id);

            if (Field::id < id || type == bond::BT_STOP || type == bond::BT_STOP_BASE)
            {
                NextSchemaField: return ReadFields<Schema, I + 1>(id, type, transform);
            }
        }
    }

    template <typename Schema, uint16_t I, typename Transform>
    typename boost::enable_if_c<(I == Schema::field_count::value)>::type
    ReadFields(uint16_t& id, BondDataType& type, const Transform& transform)
    {
        ReadUnknownFields(type, id, transform);
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<is_nested_field<T>::value
                            && !is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform)
    {
        return transform.Field(T::id, T::metadata, bonded<typename T::field_type, Input>(_input));
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<is_nested_field<T>::value
                             && is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform)
    {
        return transform.Field(T{}, bonded<typename T::field_type, Input>(_input));
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<!is_nested_field<T>::value
                             && !is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform)
    {
        return transform.Field(T::id, T::metadata, value<typename T::field_type, Input>(_input));
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<!is_nested_field<T>::value
                             && is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform)
    {
        return transform.Field(T{}, value<typename T::field_type, Input>(_input));
    }


    // This function is called only when payload has unknown field id or type is not
    // matching exactly. This relativly rare so we don't inline the function to help
    // the compiler to optimize the common path.
    template <bool IsBasicType, typename Transform>
    BOND_NO_INLINE
    typename boost::enable_if_c<IsBasicType, bool>::type
    UnknownFieldOrTypeMismatch(uint16_t expected_id, const Metadata& metadata, uint16_t id, BondDataType type, const Transform& transform)
    {
        if (id == expected_id &&
            type != bond::BT_LIST &&
            type != bond::BT_SET &&
            type != bond::BT_MAP &&
            type != bond::BT_STRUCT)
        {
            return detail::BasicTypeField(expected_id, metadata, type, transform, _input);
        }
        else
        {
            return UnknownField(id, type, transform);
        }
    }

    template <bool IsBasicType, typename Transform>
    BOND_NO_INLINE
    typename boost::disable_if_c<IsBasicType, bool>::type
    UnknownFieldOrTypeMismatch(uint16_t /*expected_id*/, const Metadata& /*metadata*/, uint16_t id, BondDataType type, const Transform& transform)
    {
        return UnknownField(id, type, transform);
    }


    // use runtime schema
    template <typename Transform>
    void
    ReadFields(const RuntimeSchema& schema, uint16_t& id, BondDataType& type, const Transform& transform)
    {
        const auto& fields = schema.GetStruct().fields;

        for (auto it = fields.begin(), end = fields.end(); ; ReadSubsequentField(type, id))
        {
            while (it != end && (it->id < id || type == bond::BT_STOP || type == bond::BT_STOP_BASE))
            {
                const FieldDef& field = *it++;
                transform.OmittedField(field.id, field.metadata, field.type.id);
            }

            if (type == bond::BT_STOP || type == bond::BT_STOP_BASE)
            {
                break;
            }

            if (it != end && it->id == id)
            {
                const FieldDef& field = *it++;

                if (type == bond::BT_STRUCT)
                {
                    if (field.type.id == type)
                    {
                        transform.Field(id, field.metadata, bonded<void, Input>(_input, RuntimeSchema(schema, field)));
                        continue;
                    }
                }
                else if (type == bond::BT_LIST || type == bond::BT_SET || type == bond::BT_MAP)
                {
                    if (field.type.id == type)
                    {
                        transform.Field(id, field.metadata, value<void, Input>(_input, RuntimeSchema(schema, field)));
                        continue;
                    }
                }
                else
                {
                    detail::BasicTypeField(id, field.metadata, type, transform, _input);
                    continue;
                }
            }

            UnknownField(id, type, transform);
        }
    }


    template <typename Transform>
    void ReadAllUnknownFields(BondDataType& type, uint16_t& id, const Transform& transform)
    {
        for (; type != bond::BT_STOP; ReadSubsequentField(type, id))
        {
            if (type == bond::BT_STOP_BASE)
                transform.UnknownEnd();
            else
                UnknownField(id, type, transform);
        }
    }


    template <typename Transform>
    void ReadUnknownFields(BondDataType& type, uint16_t& id, const Transform& transform)
    {
        for (; type != bond::BT_STOP && type != bond::BT_STOP_BASE; ReadSubsequentField(type, id))
        {
            UnknownField(id, type, transform);
        }
    }


    template <typename T, typename Protocols, typename Validator>
    bool UnknownField(uint16_t, BondDataType type, const To<T, Protocols, Validator>&)
    {
        _input.Skip(type);
        return false;
    }


    template <typename Transform>
    bool UnknownField(uint16_t id, BondDataType type, const Transform& transform)
    {
        if (type == bond::BT_STRUCT)
        {
            return transform.UnknownField(id, bonded<void, Input>(_input, GetRuntimeSchema<Unknown>()));
        }
        else if (type == bond::BT_LIST || type == bond::BT_SET || type == bond::BT_MAP)
            return transform.UnknownField(id, value<void, Input>(_input, type));
        else
            return detail::BasicTypeField(id, schema<Unknown>::type::metadata, type, BindUnknownField(transform), _input);
    }


    void ReadSubsequentField(BondDataType& type, uint16_t& id)
    {
        _input.ReadFieldEnd();
        _input.ReadFieldBegin(type, id);
    }
};


// DOM parser works with protocol implementations using Document Object Model,
// e.g. JSON or XML. The parser assumes that fields in DOM are unordered and
// are identified by either ordinal or metadata. DOM based protocols may loosly
// map to Bond meta-schema types thus the parser delegates to the protocol for
// field type match checking.
template <typename Input>
class DOMParser
    : protected detail::ParserInheritance<Input, DOMParser<Input> >
{
    typedef typename std::remove_reference<Input>::type Reader;

public:
    DOMParser(Input input, bool base = false)
        : detail::ParserInheritance<Input, DOMParser<Input> >(input, base)
    {}


    template <typename Schema, typename Transform>
    bool Apply(const Transform& transform, const Schema& schema)
    {
        if (!_base) _input.Parse();
        return this->Read(schema, transform);
    }

    friend class detail::ParserInheritance<Input, DOMParser<Input> >;


protected:
    using detail::ParserInheritance<Input, DOMParser<Input> >::_input;
    using detail::ParserInheritance<Input, DOMParser<Input> >::_base;

private:
    template <typename Schema>
    void SkipFields(const Schema&)
    {}

    // use compile-time schema
    template <typename Schema, typename Transform>
    bool ReadFields(Schema, const Transform& transform)
    {
        return ReadFields<Schema, 0>(transform);
    }


    template <typename Schema, uint16_t I, typename Transform>
    typename boost::enable_if_c<(I < Schema::field_count::value), bool>::type
    ReadFields(const Transform& transform)
    {
        using Field = detail::field_info<Schema, I>;

        if (const typename Reader::Field* field = _input.FindField(
                Field::id,  
                Field::metadata, 
                get_type_id<typename Field::field_type>::value,
                std::is_enum<typename Field::field_type>::value))
        {
            Reader input(_input, *field);
            NextField<Field>(transform, input);
        }

        return ReadFields<Schema, I + 1>(transform);
    }

    template <typename Schema, uint16_t I, typename Transform>
    typename boost::enable_if_c<(I == Schema::field_count::value), bool>::type
    ReadFields(const Transform& /*transform*/)
    {
        return false;
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<is_nested_field<T>::value
                            && !is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform, Input input)
    {
        return transform.Field(T::id, T::metadata, bonded<typename T::field_type, Input>(input));
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<is_nested_field<T>::value
                             && is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform, Input input)
    {
        return transform.Field(T{}, bonded<typename T::field_type, Input>(input));
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<!is_nested_field<T>::value
                             && !is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform, Input input)
    {
        return transform.Field(T::id, T::metadata, value<typename T::field_type, Input>(input));
    }


    template <typename T, typename Transform>
    typename boost::enable_if_c<!is_nested_field<T>::value
                             && is_fast_path_field<T, Transform>::value, bool>::type
    NextField(const Transform& transform, Input input)
    {
        return transform.Field(T{}, value<typename T::field_type, Input>(input));
    }


    // use runtime schema
    template <typename Transform>
    bool ReadFields(const RuntimeSchema& schema, const Transform& transform)
    {
        bool done = false;

        for (const_enumerator<std::vector<FieldDef> > enumerator(schema.GetStruct().fields); enumerator.more() && !done;)
        {
            const FieldDef& fieldDef = enumerator.next();

            if (const typename Reader::Field* field = _input.FindField(fieldDef.id, fieldDef.metadata, fieldDef.type.id))
            {
                Reader input(_input, *field);

                if (fieldDef.type.id == BT_STRUCT)
                    done = transform.Field(fieldDef.id, fieldDef.metadata, bonded<void, Input>(input, RuntimeSchema(schema, fieldDef)));
                else if (fieldDef.type.id == BT_LIST || fieldDef.type.id == BT_SET || fieldDef.type.id == BT_MAP)
                    done = transform.Field(fieldDef.id, fieldDef.metadata, value<void, Input>(input, RuntimeSchema(schema, fieldDef)));
                else
                    done = detail::BasicTypeField(fieldDef.id, fieldDef.metadata, fieldDef.type.id, transform, input);
            }
        }

        return done;
    }
};


} // namespace bond


#ifdef BOND_LIB_TYPE
#if BOND_LIB_TYPE != BOND_LIB_TYPE_HEADER
#include "detail/parser_extern.h"
#endif
#else
#error BOND_LIB_TYPE is undefined
#endif

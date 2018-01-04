// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include <bond/core/bond_types.h>
#include "bond_fwd.h"
#include "detail/metadata.h"
#include "detail/mpl.h"

#include <boost/static_assert.hpp>

#include <functional>

namespace bond
{

template <typename T> struct
remove_maybe
{
    typedef T type;
};


template <typename T> struct
remove_maybe<maybe<T> >
{
    typedef typename remove_maybe<T>::type type;
};


BOND_STATIC_CONSTEXPR uint16_t invalid_field_id = 0xffff;


/** namespace bond::reflection */
namespace reflection
{
//
// Helper classes/templates
//
typedef std::map<std::string, std::string> Attributes;


// field is required
using required_field_modifier = std::integral_constant<bond::Modifier, bond::Required>;

// field is optional
using optional_field_modifier = std::integral_constant<bond::Modifier, bond::Optional>;

// field is required optional
using required_optional_field_modifier = std::integral_constant<bond::Modifier, bond::RequiredOptional>;


/// @brief Field description in compile-time schema
template <
    uint16_t field_id,
	uint16_t field_index,
    typename ModifierTag,
    typename Struct,
    typename FieldType,
    FieldType Struct::*field_ptr,
    const Metadata* metadata_ptr>
struct FieldTemplate
{
    /// @brief Type of the field's parent struct
    typedef Struct struct_type;

    /// @brief Type of the field pointer
    typedef FieldType Struct::*field_pointer;

    /// @brief Type of the field
    typedef typename remove_maybe<FieldType>::type field_type;

    /// @brief Type of the field value
    typedef FieldType value_type;

    /// @brief Modifier tag for the field
    ///
    /// Can be one of:
    /// - bond::reflection::optional_field_modifier
    /// - bond::reflection::required_field_modifier
    /// - bond::reflection::required_optional_field_modifier
    typedef ModifierTag field_modifier;

    /// @brief Static data member describing field metadata
    static const Metadata& metadata;

    /// @brief Static data member representing the field pointer
    static const field_pointer field;

    /// @brief Static data member equal to the field ordinal
    BOND_STATIC_CONSTEXPR uint16_t id = field_id;

	/// @brief Static data member equal to the field index
	BOND_STATIC_CONSTEXPR uint16_t index = field_index;

    /// @brief Static method returning const reference to the field value for a particular object
    static
    const value_type& GetVariable(const struct_type& object)
    {
        return object.*field_ptr;
    }

    /// @brief Static method returning reference to the field value for a particular object
    static
    value_type& GetVariable(struct_type& object)
    {
        return object.*field_ptr;
    }

    BOOST_STATIC_ASSERT(field_id != invalid_field_id);
};


template <
    uint16_t field_id,
	uint16_t field_index,
    typename ModifierTag,
    typename Struct,
    typename FieldType,
    FieldType Struct::*field_ptr,
    const Metadata* metadata_ptr>
const bond::Metadata&
    FieldTemplate<field_id, field_index, ModifierTag, Struct, FieldType, field_ptr, metadata_ptr>::metadata = *metadata_ptr;

template
<
    uint16_t field_id,
	uint16_t field_index,
    typename ModifierTag,
    typename Struct,
    typename FieldType,
    FieldType Struct::*field_ptr,
    const Metadata* metadata_ptr
>
const typename FieldTemplate<field_id, field_index, ModifierTag, Struct, FieldType, field_ptr, metadata_ptr>::field_pointer
    FieldTemplate<field_id, field_index, ModifierTag, Struct, FieldType, field_ptr, metadata_ptr>::field = field_ptr;


template <typename Service, typename Input, typename Result> struct
method_pointer
{
    typedef void (Service::*type)(
        const Input&,
        const std::function<void(const Result&)>&);
};


template <typename Service, typename Input> struct
method_pointer<Service, Input, void>
{
    typedef void (Service::*type)(const Input&);
};


/// @brief Method description in compile-time schema
template <
    typename Service,
    typename Input,
    typename Result,
    typename method_pointer<Service, Input, Result>::type method_ptr,
    const Metadata* metadata_ptr>
struct MethodTemplate
{
    /// @brief Type of the service
    typedef Service service_type;

    /// @brief Type of the request
    typedef Input input_type;

    /// @brief Type of the response
    typedef Result result_type;

    /// @brief Static data member describing method metadata
    static const Metadata& metadata;

    /// @brief Static data member representing the member pointer to the method
    static const typename method_pointer<service_type, input_type, result_type>::type method;
};


template <
    typename Service,
    typename Input,
    typename Result,
    typename method_pointer<Service, Input, Result>::type method_ptr,
    const bond::Metadata* metadata_ptr>
const bond::Metadata&
    MethodTemplate<Service, Input, Result, method_ptr, metadata_ptr>::metadata = *metadata_ptr;

template <
    typename Service,
    typename Input,
    typename Result,
    typename method_pointer<Service, Input, Result>::type method_ptr,
    const bond::Metadata* metadata_ptr>
const typename method_pointer<Service, Input, Result>::type
    MethodTemplate<Service, Input, Result, method_ptr, metadata_ptr>::method = method_ptr;


// Metadata initializer for fields
inline
bond::Metadata MetadataInit(const char* name)
{
    bond::Metadata metadata;

    metadata.name = name;
    return metadata;
}

inline
bond::Metadata MetadataInit(const char* name, bond::Modifier modifier, const Attributes& attributes)
{
    bond::Metadata metadata;

    metadata.name = name;
    metadata.modifier = modifier;
    metadata.attributes = attributes;

    return metadata;
}

inline
bond::Metadata MetadataInit(const char* name, const Attributes& attributes)
{
    bond::Metadata metadata;

    metadata.name = name;
    metadata.attributes = attributes;

    return metadata;
}

template <typename T>
bond::Metadata MetadataInit(const T& default_value, const char* name)
{
    bond::Metadata metadata;

    metadata.name = name;
    detail::VariantSet(metadata.default_value, default_value);
    return metadata;
}

template <typename T>
bond::Metadata MetadataInit(const T& default_value, const char* name, bond::Modifier modifier, const Attributes& attributes)
{
    bond::Metadata metadata = MetadataInit(name, modifier, attributes);

    detail::VariantSet(metadata.default_value, default_value);

    return metadata;
}

struct nothing
{};

inline
bond::Metadata MetadataInit(const nothing&, const char* name)
{
    bond::Metadata metadata;

    metadata.name = name;
    metadata.default_value.nothing = true;
    return metadata;
}

inline
bond::Metadata MetadataInit(const nothing&, const char* name, bond::Modifier modifier, const Attributes& attributes)
{
    bond::Metadata metadata = MetadataInit(name, modifier, attributes);

    metadata.default_value.nothing = true;

    return metadata;
}


// Metadata initializer for structs
inline
bond::Metadata MetadataInit(const char* name, const char* qual_name, const Attributes& attributes)
{
    bond::Metadata metadata;

    metadata.name = name;
    metadata.qualified_name = qual_name;
    metadata.attributes = attributes;

    return metadata;
}


// Metadata initializer for generic structs
template <typename Params>
bond::Metadata MetadataInit(const char* name, const char* qual_name, const Attributes& attributes)
{
    bond::Metadata metadata = MetadataInit(name, qual_name, attributes);

    std::string params;

    detail::mpl::apply<Params>(detail::TypeListBuilder(params));

    metadata.name += "<" + params + ">";
    metadata.qualified_name += "<" + params + ">";

    return metadata;
}

} // namespace reflection


namespace detail
{

template <uint16_t I>
using all_fields = std::true_type;


template <typename T, uint16_t I, typename Enable = void> struct
field_info_type
    : field_info_type<typename schema<T>::type, I> {};

template <typename T, uint16_t I> struct
field_info_type<T, I, typename boost::enable_if<is_schema<T> >::type>
    : T::template field<I> {};


template <typename T, uint16_t I>
using field_info = typename field_info_type<T, I>::type;



template <typename Schema, uint16_t I, typename Enable = void> struct
field_id
    : std::integral_constant<uint16_t, field_info<Schema, I>::id> {};

template <typename Schema, uint16_t I> struct
field_id<Schema, I, typename boost::enable_if_c<(I == Schema::field_count::value)>::type>
    : std::integral_constant<uint16_t, invalid_field_id> {};



template <template <uint16_t> class C, uint16_t I, uint16_t N, uint16_t D = (N - I)> struct
next_field_index
{
    BOOST_STATIC_ASSERT(D == N - I);

    using middle = std::integral_constant<uint16_t, ((I + N) >> 1)>;

    using left = next_field_index<C, I, middle::value>;
    using right = next_field_index<C, middle::value, N>;

    BOND_STATIC_CONSTEXPR uint16_t value = (left::value != middle::value) ? left::value : right::value;
};

template <template <uint16_t> class C, uint16_t I, uint16_t N> struct
next_field_index<C, I, N, 1>
    : std::integral_constant<uint16_t, C<I>::value ? I : N>
{
    BOOST_STATIC_ASSERT(N == I + 1);
};

template <template <uint16_t> class C, uint16_t N> struct
next_field_index<C, N, N, 0>
    : std::integral_constant<uint16_t, N> {};


template <typename Schema> struct
is_required_field
{
    template <uint16_t I> struct
    type
        : std::is_same<typename field_info<Schema, I>::field_modifier, reflection::required_field_modifier> {};
};


#if defined(_MSC_VER) && _MSC_VER < 1900

template <typename T, template <uint16_t> class C, uint16_t I, uint16_t N, typename F>
inline bool linear_search_field(uint16_t id, F&& f);

#else

template <typename T, template <uint16_t> class C, uint16_t I, uint16_t N, typename F>
inline typename boost::disable_if_c<(I == N), bool>::type
linear_search_field(uint16_t id, F&& f);

template <typename T, template <uint16_t> class C, uint16_t I, uint16_t N, typename F>
inline typename boost::enable_if_c<(I == N), bool>::type
linear_search_field(uint16_t /*id*/, F&& /*f*/);

#endif

template <typename T, template <uint16_t> class C, uint16_t I, uint16_t N, typename F>
inline typename boost::enable_if<C<I>, bool>::type
linear_search_field_helper(uint16_t id, F&& f)
{
    using Field = field_info<T, I>;

    if (Field::id == id)
    {
        std::forward<F>(f)(mpl::identity<Field>{});
        return true;
    }

    return linear_search_field<T, C, I + 1, N>(id, std::forward<F>(f));
}

template <typename T, template <uint16_t> class C, uint16_t I, uint16_t N, typename F>
inline typename boost::disable_if<C<I>, bool>::type
linear_search_field_helper(uint16_t id, F&& f)
{
    return linear_search_field<T, C, I + 1, N>(id, std::forward<F>(f));
}


#if defined(_MSC_VER) && _MSC_VER < 1900

template <uint16_t I, uint16_t N>
struct linear_search_field_functor
{
    template <typename T, template <uint16_t> class C, typename F>
    static bool invoke(uint16_t id, F&& f)
    {
        return linear_search_field_helper<T, C, I, N>(id, std::forward<F>(f));
    }
};

template <uint16_t N>
struct linear_search_field_functor<N, N>
{
    template <typename T, template <uint16_t> class C, typename F>
    static bool invoke(uint16_t /*id*/, F&& /*f*/)
    {
        return false;
    }
};

template <typename T, template <uint16_t> class C, uint16_t I, uint16_t N, typename F>
inline bool linear_search_field(uint16_t id, F&& f)
{
    return linear_search_field_functor<I, schema<T>::type::field_count::value>
        ::template invoke<T, C>(id, std::forward<F>(f));
}

#else

template <typename T, template <uint16_t> class C, uint16_t I, uint16_t N, typename F>
inline typename boost::disable_if_c<(I == N), bool>::type
linear_search_field(uint16_t id, F&& f)
{
    return linear_search_field_helper<T, C, I, N>(id, std::forward<F>(f));
}

template <typename T, template <uint16_t> class C, uint16_t I, uint16_t N, typename F>
inline typename boost::enable_if_c<(I == N), bool>::type
linear_search_field(uint16_t /*id*/, F&& /*f*/)
{
    return false;
}

#endif


template <typename T, template <uint16_t> class C = all_fields, typename F>
inline bool search_field(uint16_t id, F&& f)
{
    return linear_search_field<T, C, 0, schema<T>::type::field_count::value>(id, std::forward<F>(f));
}

} // namespace detail


const reflection::nothing nothing = {};


template <typename Schema, uint16_t I = 0> struct
next_required_field
    : detail::field_id<
        Schema,
        detail::next_field_index<
            detail::is_required_field<Schema>::template type,
            I,
            Schema::field_count::value>::value> {};


struct no_base {};


template <typename T, typename Enable = void> struct
is_writer
    : std::false_type {};


template <typename T> struct
is_writer<T, typename boost::enable_if<check_method<void (T::*)(const Metadata&, bool), &T::WriteStructBegin> >::type>
     : std::true_type {};


template <typename T>
inline typename T::base*
base_class()
{
    return NULL;
}


template <typename T> struct
remove_bonded
{
    typedef T type;
};


template <typename T> struct
remove_bonded<bonded<T> >
{
    typedef typename remove_bonded<T>::type type;
};


template <typename T> struct
is_bond_type
    : std::integral_constant<bool,
        is_bonded<typename std::remove_const<T>::type>::value
        || has_schema<typename std::remove_const<T>::type>::value> {};


struct Unknown;

template <typename Unused> struct
schema<Unknown, Unused>
{
    struct type
    {
        using base = no_base;

        struct var;

        using field_count = std::integral_constant<uint16_t, 0>;

        template <uint16_t I> struct field;

        static const Metadata metadata;

        type()
        {
            (void)metadata;
        }
    };
};

template <typename Unused>
const Metadata schema<Unknown, Unused>::type::metadata
    = reflection::MetadataInit("Unknown", "Unknown", reflection::Attributes());

template <typename T, typename Enable> struct
schema_for_passthrough
    : schema<typename remove_bonded<T>::type>
{};

template <typename T> struct
schema_for_passthrough<T, typename boost::disable_if<has_schema<typename remove_bonded<T>::type> >::type>
{
    // If type T doesn't have schema we return schema of an empty struct;
    // this allows pass-through of bonded<T> with only forward declaration for T.
    typedef typename schema<Unknown>::type type;
};

template <typename T> struct
is_container
    : std::integral_constant<bool,
        is_list_container<typename std::remove_const<T>::type>::value
        || is_set_container<typename std::remove_const<T>::type>::value
        || is_map_container<typename std::remove_const<T>::type>::value> {};


template <typename Field, typename Transform, typename Enable = void> struct
is_fast_path_field
    : std::false_type {};


template <typename Field, typename Transform> struct
is_fast_path_field<Field, Transform, typename boost::enable_if<std::is_same<typename Field::struct_type,
                                                                            typename Transform::FastPathType> >::type>
     : std::true_type {};


template <typename T> struct
is_nested_field
    : is_bond_type<typename T::field_type> {};


template <typename T> struct
is_struct_field
    : has_schema<typename T::field_type> {};


template <typename T1, typename T2, typename Enable = void> struct
is_matching_container
    : std::false_type {};


template <typename T1, typename T2> struct
is_matching_basic
    : std::integral_constant<bool,
        (is_string<T1>::value && is_string<T2>::value)
        || (is_wstring<T1>::value && is_wstring<T2>::value)
        || ((sizeof(T1) <= sizeof(T2))
            && ((std::is_unsigned<T1>::value && std::is_unsigned<T2>::value)
                || (is_signed_int_or_enum<T1>::value && is_signed_int_or_enum<T2>::value)))> {};


template <typename T> struct
is_matching_basic<typename aliased_type<T>::type, T>
    : std::true_type {};


template <typename T1, typename T2> struct
is_matching
    : std::integral_constant<bool,
        (is_bond_type<T1>::value && is_bond_type<T2>::value)
        || (is_matching_basic<T1, T2>::value)
        || (is_matching_container<T1, T2>::value)> {};


template <typename T> struct
is_matching_basic<T, T>
    : std::true_type {};


template <> struct
is_matching_basic<bool, bool>
    : std::true_type {};


template <typename T> struct
is_matching_basic<bool, T>
    : std::false_type {};


template <> struct
is_matching_basic<uint8_t, bool>
    : std::false_type {};


template <> struct
is_matching_basic<float, double>
    : std::true_type {};


template <typename T, typename Enable> struct
get_type_id;


template <typename T1, typename T2> struct
is_matching_container<T1, T2,
    typename boost::enable_if_c<is_container<T1>::value
                             && get_type_id<T1>::value == get_type_id<T2>::value>::type>
    : is_matching<typename element_type<T1>::type,
                  typename element_type<T2>::type> {};


// tuples match if the elements match
template <typename T1, typename T2, typename U1, typename U2> struct
is_matching<std::pair<T1, T2>, std::pair<U1, U2> >
    : std::integral_constant<bool,
        is_matching<T1, U1>::value
        && is_matching<T2, U2>::value> {};


template <typename T1, typename T2> struct
is_matching<std::pair<T1, T2>, std::pair<T1, T2> >
    : std::true_type {};


// value<T> matches if type matches
template <typename T1, typename Reader, typename T2> struct
is_matching<value<T1, Reader>, T2>
    : is_matching<T1, T2> {};


// value<void> matches every container
template <typename T, typename Reader> struct
is_matching<value<void, Reader>, T>
    : is_container<T> {};


template <typename T, typename X, typename Enable = void> struct
is_element_matching
    : std::false_type {};


template <typename T, typename X> struct
is_element_matching<T, X, typename boost::enable_if<is_container<X> >::type>
    : is_matching<T, typename element_type<X>::type> {};


template <typename X, typename Reader> struct
is_element_matching<value<void, Reader>, X, typename boost::enable_if<is_container<X> >::type>
    : std::integral_constant<bool,
        is_bond_type<typename element_type<X>::type>::value
        || is_container<typename element_type<X>::type>::value> {};


template <typename T, typename X, typename Enable = void> struct
is_map_element_matching
    : std::false_type {};


template <typename X, typename T> struct
is_map_element_matching<T, X, typename boost::enable_if<is_map_container<X> >::type>
    : is_matching<T, typename element_type<X>::type::second_type> {};


template <typename X, typename Reader> struct
is_map_element_matching<value<void, Reader>, X, typename boost::enable_if<is_map_container<X> >::type>
    : std::integral_constant<bool,
        is_bond_type<typename element_type<X>::type::second_type>::value
        || is_container<typename element_type<X>::type::second_type>::value> {};


template <typename T, typename X, typename Enable = void> struct
is_map_key_matching
    : std::false_type {};


template <typename T, typename X> struct
is_map_key_matching<T, X, typename boost::enable_if<is_map_container<X> >::type>
    : is_matching<T, typename element_type<X>::type::first_type> {};


template <typename T> struct
is_basic_type
    : std::integral_constant<bool,
        !(is_container<T>::value || is_bond_type<T>::value)> {};

template <> struct
is_basic_type<void>
    : std::false_type {};


// is_nested_container
template <typename T, typename Enable = void> struct
is_nested_container
    : std::false_type {};

template <typename T> struct
is_nested_container<T, typename boost::enable_if<is_map_container<T> >::type>
    : std::integral_constant<bool,
        !is_basic_type<typename element_type<T>::type::second_type>::value> {};


template <typename T> struct
is_nested_container<T, typename boost::enable_if<is_list_container<T> >::type>
    : std::integral_constant<bool,
        !is_basic_type<typename element_type<T>::type>::value> {};


template <typename T, typename Enable = void> struct
is_struct_container
    : std::false_type {};

template <typename T> struct
is_struct_container<T, typename boost::enable_if<is_map_container<T> >::type>
    : std::integral_constant<bool,
        has_schema<typename element_type<T>::type::second_type>::value
        || is_struct_container<typename element_type<T>::type::second_type>::value> {};



template <typename T> struct
is_struct_container<T, typename boost::enable_if<is_list_container<T> >::type>
    : std::integral_constant<bool,
        has_schema<typename element_type<T>::type>::value
        || is_struct_container<typename element_type<T>::type>::value> {};


// is_struct_container_field
template <typename T> struct
is_struct_container_field
    : is_struct_container<typename T::field_type> {};


// is_basic_container
template <typename T, typename Enable = void> struct
is_basic_container
    : std::false_type {};

template <typename T> struct
is_basic_container<T, typename boost::enable_if<is_map_container<T> >::type>
    : is_basic_type<typename element_type<T>::type::second_type> {};

template <typename T> struct
is_basic_container<T, typename boost::enable_if<is_list_container<T> >::type>
    : is_basic_type<typename element_type<T>::type> {};

template <typename T> struct
is_basic_container<T, typename boost::enable_if<is_set_container<T> >::type>
    : std::true_type {};


template <typename T, typename F> struct
is_matching_container_field
    : is_matching_container<T, typename F::field_type> {};


template <typename T> struct
is_container_field
    : is_container<typename T::field_type> {};


template <typename T, typename F> struct
is_matching_basic_field
    : is_matching_basic<T, typename F::field_type> {};


template <typename T, typename Reader> struct
is_basic_type<value<T, Reader> >
    : std::false_type {};


template <typename T1, typename T2> struct
is_basic_type<std::pair<T1, T2> >
    : std::false_type {};


template <typename T> struct
is_matching_field
{
    template <typename F>
    struct basic
        : is_matching_basic_field<T, F> {};

    template <typename F>
    struct container
        : is_matching_container_field<T, F> {};
};


template <typename T> struct
nested_fields
{
    template <uint16_t I> struct
    type
        : is_nested_field<detail::field_info<T, I> > {};
};


template <typename T, typename X> struct
matching_fields
{
    template <uint16_t I> struct
    type
        : std::integral_constant<bool,
            is_matching_field<X>::template basic<detail::field_info<T, I> >::value
            || is_matching_field<X>::template container<detail::field_info<T, I> >::value> {};
};


template <typename T> struct
struct_fields
{
    template <uint16_t I> struct
    type
        : is_struct_field<detail::field_info<T, I> > {};
};


template <typename T> struct
container_fields
{
    template <uint16_t I> struct
    type
        : is_container_field<detail::field_info<T, I> > {};
};


template <typename T> struct
has_base
    : has_schema<typename schema<T>::type::base> {};


template <typename T>
BondDataType
GetTypeId(const T&)
{
    return get_type_id<T>::value;
}


template <typename Reader>
BondDataType
GetTypeId(const value<void, Reader>& value)
{
    return value.GetTypeId();
}


template <typename T, typename Reader> struct
get_type_id<value<T, Reader> >
    : get_type_id<T> {};


template <typename T1, typename T2> struct
get_type_id<std::pair<T1, T2> >
{
    static const std::pair<BondDataType, BondDataType> value;
};

template <typename T1, typename T2>
const std::pair<BondDataType, BondDataType>
get_type_id<std::pair<T1, T2> >::value = std::make_pair(
    get_type_id<typename std::remove_const<T1>::type>::value,
    get_type_id<T2>::value);

template <> struct
get_type_id<bool>
    : std::integral_constant<BondDataType, BT_BOOL> {};

template <> struct
get_type_id<uint8_t>
    : std::integral_constant<BondDataType, BT_UINT8> {};

template <> struct
get_type_id<uint16_t>
    : std::integral_constant<BondDataType, BT_UINT16> {};

template <> struct
get_type_id<uint32_t>
    : std::integral_constant<BondDataType, BT_UINT32> {};

template <> struct
get_type_id<uint64_t>
    : std::integral_constant<BondDataType, BT_UINT64> {};

template <> struct
get_type_id<int8_t>
    : std::integral_constant<BondDataType, BT_INT8> {};

template <> struct
get_type_id<int16_t>
    : std::integral_constant<BondDataType, BT_INT16> {};

template <> struct
get_type_id<int32_t>
    : std::integral_constant<BondDataType, BT_INT32> {};

template <> struct
get_type_id<int64_t>
    : std::integral_constant<BondDataType, BT_INT64> {};

template <> struct
get_type_id<float>
    : std::integral_constant<BondDataType, BT_FLOAT> {};

template <> struct
get_type_id<double>
    : std::integral_constant<BondDataType, BT_DOUBLE> {};

template <> struct
get_type_id<void>
    : std::integral_constant<BondDataType, BT_UNAVAILABLE> {};

template <typename T> struct
get_type_id<T, typename boost::enable_if<std::is_enum<T>>::type>
    : get_type_id<int32_t> {};

template <typename T> struct
get_type_id<T, typename boost::enable_if<is_bond_type<T>>::type>
    : std::integral_constant<BondDataType, BT_STRUCT> {};

template <typename T> struct
get_type_id<T, typename boost::enable_if<is_set_container<typename std::remove_const<T>::type>>::type>
    : std::integral_constant<BondDataType, BT_SET> {};

template <typename T> struct
get_type_id<T, typename boost::enable_if<is_map_container<typename std::remove_const<T>::type>>::type>
    : std::integral_constant<BondDataType, BT_MAP> {};

template <typename T> struct
get_type_id<T, typename boost::enable_if<is_list_container<typename std::remove_const<T>::type>>::type>
    : std::integral_constant<BondDataType, BT_LIST> {};

template <typename T> struct
get_type_id<T, typename boost::enable_if<is_string<typename std::remove_const<T>::type>>::type>
    : std::integral_constant<BondDataType, BT_STRING> {};

template <typename T> struct
get_type_id<T, typename boost::enable_if<is_wstring<typename std::remove_const<T>::type>>::type>
    : std::integral_constant<BondDataType, BT_WSTRING> {};

template <typename T, typename Enable> struct
get_type_id
    : get_type_id<typename aliased_type<T>::type> {};


template <typename T, typename Enable = void> struct
get_list_sub_type_id
    : std::integral_constant<ListSubType, NO_SUBTYPE> {};

template <typename T> struct
get_list_sub_type_id<T, typename boost::enable_if<is_nullable<typename std::remove_const<T>::type> >::type>
    : std::integral_constant<ListSubType, NULLABLE_SUBTYPE> {};

template <typename T> struct
get_list_sub_type_id<T, typename boost::enable_if<is_blob<typename std::remove_const<T>::type> >::type>
    : std::integral_constant<ListSubType, BLOB_SUBTYPE> {};


class PrimitiveTypes
{
    struct Init
    {
        Init(uint32_t* _sizeof)
            : _sizeof(_sizeof)
        {}

        template <typename T>
        void operator()(detail::mpl::identity<T>)
        {
            _sizeof[get_type_id<T>::value] = sizeof(T);
        }

        uint32_t* _sizeof;
    };

public:
    typedef detail::mpl::list
    <
        bool, float, double,
        uint8_t, uint16_t, uint32_t, uint64_t,
        int8_t,  int16_t,  int32_t,  int64_t
    > type;

    PrimitiveTypes(uint32_t* _sizeof)
    {
        detail::mpl::apply<type>(Init(_sizeof));
    }
};


} // namespace bond


#pragma once

#include "basic_types_nsmapped_types.h"
#include <bond/core/reflection.h>

namespace nsmapped
{
    //
    // BasicTypes
    //
    struct BasicTypes::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s__bool_metadata;
        private: static const ::bond::Metadata s__str_metadata;
        private: static const ::bond::Metadata s__wstr_metadata;
        private: static const ::bond::Metadata s__uint64_metadata;
        private: static const ::bond::Metadata s__uint16_metadata;
        private: static const ::bond::Metadata s__uint32_metadata;
        private: static const ::bond::Metadata s__uint8_metadata;
        private: static const ::bond::Metadata s__int8_metadata;
        private: static const ::bond::Metadata s__int16_metadata;
        private: static const ::bond::Metadata s__int32_metadata;
        private: static const ::bond::Metadata s__int64_metadata;
        private: static const ::bond::Metadata s__double_metadata;
        private: static const ::bond::Metadata s__float_metadata;
        private: static const ::bond::Metadata s__blob_metadata;

        public: struct var
        {
            // _bool
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                bool,
                &BasicTypes::_bool,
                &s__bool_metadata
            > {} _bool;
        
            // _str
            typedef struct : ::bond::reflection::FieldTemplate<
                2,
                1,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >,
                &BasicTypes::_str,
                &s__str_metadata
            > {} _str;
        
            // _wstr
            typedef struct : ::bond::reflection::FieldTemplate<
                3,
                2,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                std::basic_string<wchar_t, std::char_traits<wchar_t>, typename std::allocator_traits<arena>::template rebind_alloc<wchar_t> >,
                &BasicTypes::_wstr,
                &s__wstr_metadata
            > {} _wstr;
        
            // _uint64
            typedef struct : ::bond::reflection::FieldTemplate<
                10,
                3,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                uint64_t,
                &BasicTypes::_uint64,
                &s__uint64_metadata
            > {} _uint64;
        
            // _uint16
            typedef struct : ::bond::reflection::FieldTemplate<
                11,
                4,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                uint16_t,
                &BasicTypes::_uint16,
                &s__uint16_metadata
            > {} _uint16;
        
            // _uint32
            typedef struct : ::bond::reflection::FieldTemplate<
                12,
                5,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                uint32_t,
                &BasicTypes::_uint32,
                &s__uint32_metadata
            > {} _uint32;
        
            // _uint8
            typedef struct : ::bond::reflection::FieldTemplate<
                13,
                6,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                uint8_t,
                &BasicTypes::_uint8,
                &s__uint8_metadata
            > {} _uint8;
        
            // _int8
            typedef struct : ::bond::reflection::FieldTemplate<
                14,
                7,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                int8_t,
                &BasicTypes::_int8,
                &s__int8_metadata
            > {} _int8;
        
            // _int16
            typedef struct : ::bond::reflection::FieldTemplate<
                15,
                8,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                int16_t,
                &BasicTypes::_int16,
                &s__int16_metadata
            > {} _int16;
        
            // _int32
            typedef struct : ::bond::reflection::FieldTemplate<
                16,
                9,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                int32_t,
                &BasicTypes::_int32,
                &s__int32_metadata
            > {} _int32;
        
            // _int64
            typedef struct : ::bond::reflection::FieldTemplate<
                17,
                10,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                int64_t,
                &BasicTypes::_int64,
                &s__int64_metadata
            > {} _int64;
        
            // _double
            typedef struct : ::bond::reflection::FieldTemplate<
                18,
                11,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                double,
                &BasicTypes::_double,
                &s__double_metadata
            > {} _double;
        
            // _float
            typedef struct : ::bond::reflection::FieldTemplate<
                20,
                12,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                float,
                &BasicTypes::_float,
                &s__float_metadata
            > {} _float;
        
            // _blob
            typedef struct : ::bond::reflection::FieldTemplate<
                21,
                13,
                ::bond::reflection::optional_field_modifier,
                BasicTypes,
                ::bond::blob,
                &BasicTypes::_blob,
                &s__blob_metadata
            > {} _blob;
        };

        using field_count = std::integral_constant<uint16_t, 14>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::_bool> {};
        template <int __bond_dummy> struct field<1, __bond_dummy> : ::bond::detail::mpl::identity<var::_str> {};
        template <int __bond_dummy> struct field<2, __bond_dummy> : ::bond::detail::mpl::identity<var::_wstr> {};
        template <int __bond_dummy> struct field<3, __bond_dummy> : ::bond::detail::mpl::identity<var::_uint64> {};
        template <int __bond_dummy> struct field<4, __bond_dummy> : ::bond::detail::mpl::identity<var::_uint16> {};
        template <int __bond_dummy> struct field<5, __bond_dummy> : ::bond::detail::mpl::identity<var::_uint32> {};
        template <int __bond_dummy> struct field<6, __bond_dummy> : ::bond::detail::mpl::identity<var::_uint8> {};
        template <int __bond_dummy> struct field<7, __bond_dummy> : ::bond::detail::mpl::identity<var::_int8> {};
        template <int __bond_dummy> struct field<8, __bond_dummy> : ::bond::detail::mpl::identity<var::_int16> {};
        template <int __bond_dummy> struct field<9, __bond_dummy> : ::bond::detail::mpl::identity<var::_int32> {};
        template <int __bond_dummy> struct field<10, __bond_dummy> : ::bond::detail::mpl::identity<var::_int64> {};
        template <int __bond_dummy> struct field<11, __bond_dummy> : ::bond::detail::mpl::identity<var::_double> {};
        template <int __bond_dummy> struct field<12, __bond_dummy> : ::bond::detail::mpl::identity<var::_float> {};
        template <int __bond_dummy> struct field<13, __bond_dummy> : ::bond::detail::mpl::identity<var::_blob> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("BasicTypes", "tests.BasicTypes",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace nsmapped

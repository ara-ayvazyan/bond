
#pragma once

#include "defaults_types.h"
#include <bond/core/reflection.h>

namespace tests
{
    //
    // Foo
    //
    struct Foo::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_m_bool_1_metadata;
        private: static const ::bond::Metadata s_m_bool_2_metadata;
        private: static const ::bond::Metadata s_m_bool_3_metadata;
        private: static const ::bond::Metadata s_m_str_1_metadata;
        private: static const ::bond::Metadata s_m_str_2_metadata;
        private: static const ::bond::Metadata s_m_int8_4_metadata;
        private: static const ::bond::Metadata s_m_int8_5_metadata;
        private: static const ::bond::Metadata s_m_int16_4_metadata;
        private: static const ::bond::Metadata s_m_int16_5_metadata;
        private: static const ::bond::Metadata s_m_int32_4_metadata;
        private: static const ::bond::Metadata s_m_int32_max_metadata;
        private: static const ::bond::Metadata s_m_int64_4_metadata;
        private: static const ::bond::Metadata s_m_int64_max_metadata;
        private: static const ::bond::Metadata s_m_uint8_2_metadata;
        private: static const ::bond::Metadata s_m_uint8_3_metadata;
        private: static const ::bond::Metadata s_m_uint16_2_metadata;
        private: static const ::bond::Metadata s_m_uint16_3_metadata;
        private: static const ::bond::Metadata s_m_uint32_3_metadata;
        private: static const ::bond::Metadata s_m_uint32_max_metadata;
        private: static const ::bond::Metadata s_m_uint64_3_metadata;
        private: static const ::bond::Metadata s_m_uint64_max_metadata;
        private: static const ::bond::Metadata s_m_double_3_metadata;
        private: static const ::bond::Metadata s_m_double_4_metadata;
        private: static const ::bond::Metadata s_m_double_5_metadata;
        private: static const ::bond::Metadata s_m_float_3_metadata;
        private: static const ::bond::Metadata s_m_float_4_metadata;
        private: static const ::bond::Metadata s_m_float_7_metadata;
        private: static const ::bond::Metadata s_m_enum1_metadata;
        private: static const ::bond::Metadata s_m_enum2_metadata;
        private: static const ::bond::Metadata s_m_enum3_metadata;
        private: static const ::bond::Metadata s_m_enum_int32min_metadata;
        private: static const ::bond::Metadata s_m_enum_int32max_metadata;
        private: static const ::bond::Metadata s_m_enum_uint32_min_metadata;
        private: static const ::bond::Metadata s_m_enum_uint32_max_metadata;
        private: static const ::bond::Metadata s_m_wstr_1_metadata;
        private: static const ::bond::Metadata s_m_wstr_2_metadata;
        private: static const ::bond::Metadata s_m_int64_neg_hex_metadata;
        private: static const ::bond::Metadata s_m_int64_neg_oct_metadata;

        public: struct var
        {
            // m_bool_1
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                Foo,
                bool,
                &Foo::m_bool_1,
                &s_m_bool_1_metadata
            > {} m_bool_1;
        
            // m_bool_2
            typedef struct : ::bond::reflection::FieldTemplate<
                1,
                1,
                ::bond::reflection::optional_field_modifier,
                Foo,
                bool,
                &Foo::m_bool_2,
                &s_m_bool_2_metadata
            > {} m_bool_2;
        
            // m_bool_3
            typedef struct : ::bond::reflection::FieldTemplate<
                2,
                2,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<bool>,
                &Foo::m_bool_3,
                &s_m_bool_3_metadata
            > {} m_bool_3;
        
            // m_str_1
            typedef struct : ::bond::reflection::FieldTemplate<
                3,
                3,
                ::bond::reflection::optional_field_modifier,
                Foo,
                std::string,
                &Foo::m_str_1,
                &s_m_str_1_metadata
            > {} m_str_1;
        
            // m_str_2
            typedef struct : ::bond::reflection::FieldTemplate<
                4,
                4,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<std::string>,
                &Foo::m_str_2,
                &s_m_str_2_metadata
            > {} m_str_2;
        
            // m_int8_4
            typedef struct : ::bond::reflection::FieldTemplate<
                5,
                5,
                ::bond::reflection::optional_field_modifier,
                Foo,
                int8_t,
                &Foo::m_int8_4,
                &s_m_int8_4_metadata
            > {} m_int8_4;
        
            // m_int8_5
            typedef struct : ::bond::reflection::FieldTemplate<
                6,
                6,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<int8_t>,
                &Foo::m_int8_5,
                &s_m_int8_5_metadata
            > {} m_int8_5;
        
            // m_int16_4
            typedef struct : ::bond::reflection::FieldTemplate<
                7,
                7,
                ::bond::reflection::optional_field_modifier,
                Foo,
                int16_t,
                &Foo::m_int16_4,
                &s_m_int16_4_metadata
            > {} m_int16_4;
        
            // m_int16_5
            typedef struct : ::bond::reflection::FieldTemplate<
                8,
                8,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<int16_t>,
                &Foo::m_int16_5,
                &s_m_int16_5_metadata
            > {} m_int16_5;
        
            // m_int32_4
            typedef struct : ::bond::reflection::FieldTemplate<
                9,
                9,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<int32_t>,
                &Foo::m_int32_4,
                &s_m_int32_4_metadata
            > {} m_int32_4;
        
            // m_int32_max
            typedef struct : ::bond::reflection::FieldTemplate<
                10,
                10,
                ::bond::reflection::optional_field_modifier,
                Foo,
                int32_t,
                &Foo::m_int32_max,
                &s_m_int32_max_metadata
            > {} m_int32_max;
        
            // m_int64_4
            typedef struct : ::bond::reflection::FieldTemplate<
                11,
                11,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<int64_t>,
                &Foo::m_int64_4,
                &s_m_int64_4_metadata
            > {} m_int64_4;
        
            // m_int64_max
            typedef struct : ::bond::reflection::FieldTemplate<
                12,
                12,
                ::bond::reflection::optional_field_modifier,
                Foo,
                int64_t,
                &Foo::m_int64_max,
                &s_m_int64_max_metadata
            > {} m_int64_max;
        
            // m_uint8_2
            typedef struct : ::bond::reflection::FieldTemplate<
                13,
                13,
                ::bond::reflection::optional_field_modifier,
                Foo,
                uint8_t,
                &Foo::m_uint8_2,
                &s_m_uint8_2_metadata
            > {} m_uint8_2;
        
            // m_uint8_3
            typedef struct : ::bond::reflection::FieldTemplate<
                14,
                14,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<uint8_t>,
                &Foo::m_uint8_3,
                &s_m_uint8_3_metadata
            > {} m_uint8_3;
        
            // m_uint16_2
            typedef struct : ::bond::reflection::FieldTemplate<
                15,
                15,
                ::bond::reflection::optional_field_modifier,
                Foo,
                uint16_t,
                &Foo::m_uint16_2,
                &s_m_uint16_2_metadata
            > {} m_uint16_2;
        
            // m_uint16_3
            typedef struct : ::bond::reflection::FieldTemplate<
                16,
                16,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<uint16_t>,
                &Foo::m_uint16_3,
                &s_m_uint16_3_metadata
            > {} m_uint16_3;
        
            // m_uint32_3
            typedef struct : ::bond::reflection::FieldTemplate<
                17,
                17,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<uint32_t>,
                &Foo::m_uint32_3,
                &s_m_uint32_3_metadata
            > {} m_uint32_3;
        
            // m_uint32_max
            typedef struct : ::bond::reflection::FieldTemplate<
                18,
                18,
                ::bond::reflection::optional_field_modifier,
                Foo,
                uint32_t,
                &Foo::m_uint32_max,
                &s_m_uint32_max_metadata
            > {} m_uint32_max;
        
            // m_uint64_3
            typedef struct : ::bond::reflection::FieldTemplate<
                19,
                19,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<uint64_t>,
                &Foo::m_uint64_3,
                &s_m_uint64_3_metadata
            > {} m_uint64_3;
        
            // m_uint64_max
            typedef struct : ::bond::reflection::FieldTemplate<
                20,
                20,
                ::bond::reflection::optional_field_modifier,
                Foo,
                uint64_t,
                &Foo::m_uint64_max,
                &s_m_uint64_max_metadata
            > {} m_uint64_max;
        
            // m_double_3
            typedef struct : ::bond::reflection::FieldTemplate<
                21,
                21,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<double>,
                &Foo::m_double_3,
                &s_m_double_3_metadata
            > {} m_double_3;
        
            // m_double_4
            typedef struct : ::bond::reflection::FieldTemplate<
                22,
                22,
                ::bond::reflection::optional_field_modifier,
                Foo,
                double,
                &Foo::m_double_4,
                &s_m_double_4_metadata
            > {} m_double_4;
        
            // m_double_5
            typedef struct : ::bond::reflection::FieldTemplate<
                23,
                23,
                ::bond::reflection::optional_field_modifier,
                Foo,
                double,
                &Foo::m_double_5,
                &s_m_double_5_metadata
            > {} m_double_5;
        
            // m_float_3
            typedef struct : ::bond::reflection::FieldTemplate<
                24,
                24,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<float>,
                &Foo::m_float_3,
                &s_m_float_3_metadata
            > {} m_float_3;
        
            // m_float_4
            typedef struct : ::bond::reflection::FieldTemplate<
                25,
                25,
                ::bond::reflection::optional_field_modifier,
                Foo,
                float,
                &Foo::m_float_4,
                &s_m_float_4_metadata
            > {} m_float_4;
        
            // m_float_7
            typedef struct : ::bond::reflection::FieldTemplate<
                26,
                26,
                ::bond::reflection::optional_field_modifier,
                Foo,
                float,
                &Foo::m_float_7,
                &s_m_float_7_metadata
            > {} m_float_7;
        
            // m_enum1
            typedef struct : ::bond::reflection::FieldTemplate<
                27,
                27,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::tests::EnumType1,
                &Foo::m_enum1,
                &s_m_enum1_metadata
            > {} m_enum1;
        
            // m_enum2
            typedef struct : ::bond::reflection::FieldTemplate<
                28,
                28,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::tests::EnumType1,
                &Foo::m_enum2,
                &s_m_enum2_metadata
            > {} m_enum2;
        
            // m_enum3
            typedef struct : ::bond::reflection::FieldTemplate<
                29,
                29,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe< ::tests::EnumType1>,
                &Foo::m_enum3,
                &s_m_enum3_metadata
            > {} m_enum3;
        
            // m_enum_int32min
            typedef struct : ::bond::reflection::FieldTemplate<
                30,
                30,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::tests::EnumType1,
                &Foo::m_enum_int32min,
                &s_m_enum_int32min_metadata
            > {} m_enum_int32min;
        
            // m_enum_int32max
            typedef struct : ::bond::reflection::FieldTemplate<
                31,
                31,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::tests::EnumType1,
                &Foo::m_enum_int32max,
                &s_m_enum_int32max_metadata
            > {} m_enum_int32max;
        
            // m_enum_uint32_min
            typedef struct : ::bond::reflection::FieldTemplate<
                32,
                32,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::tests::EnumType1,
                &Foo::m_enum_uint32_min,
                &s_m_enum_uint32_min_metadata
            > {} m_enum_uint32_min;
        
            // m_enum_uint32_max
            typedef struct : ::bond::reflection::FieldTemplate<
                33,
                33,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::tests::EnumType1,
                &Foo::m_enum_uint32_max,
                &s_m_enum_uint32_max_metadata
            > {} m_enum_uint32_max;
        
            // m_wstr_1
            typedef struct : ::bond::reflection::FieldTemplate<
                34,
                34,
                ::bond::reflection::optional_field_modifier,
                Foo,
                std::wstring,
                &Foo::m_wstr_1,
                &s_m_wstr_1_metadata
            > {} m_wstr_1;
        
            // m_wstr_2
            typedef struct : ::bond::reflection::FieldTemplate<
                35,
                35,
                ::bond::reflection::optional_field_modifier,
                Foo,
                ::bond::maybe<std::wstring>,
                &Foo::m_wstr_2,
                &s_m_wstr_2_metadata
            > {} m_wstr_2;
        
            // m_int64_neg_hex
            typedef struct : ::bond::reflection::FieldTemplate<
                36,
                36,
                ::bond::reflection::optional_field_modifier,
                Foo,
                int64_t,
                &Foo::m_int64_neg_hex,
                &s_m_int64_neg_hex_metadata
            > {} m_int64_neg_hex;
        
            // m_int64_neg_oct
            typedef struct : ::bond::reflection::FieldTemplate<
                37,
                37,
                ::bond::reflection::optional_field_modifier,
                Foo,
                int64_t,
                &Foo::m_int64_neg_oct,
                &s_m_int64_neg_oct_metadata
            > {} m_int64_neg_oct;
        };

        using field_count = std::integral_constant<uint16_t, 38>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::m_bool_1> {};
        template <int __bond_dummy> struct field<1, __bond_dummy> : ::bond::detail::mpl::identity<var::m_bool_2> {};
        template <int __bond_dummy> struct field<2, __bond_dummy> : ::bond::detail::mpl::identity<var::m_bool_3> {};
        template <int __bond_dummy> struct field<3, __bond_dummy> : ::bond::detail::mpl::identity<var::m_str_1> {};
        template <int __bond_dummy> struct field<4, __bond_dummy> : ::bond::detail::mpl::identity<var::m_str_2> {};
        template <int __bond_dummy> struct field<5, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int8_4> {};
        template <int __bond_dummy> struct field<6, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int8_5> {};
        template <int __bond_dummy> struct field<7, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int16_4> {};
        template <int __bond_dummy> struct field<8, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int16_5> {};
        template <int __bond_dummy> struct field<9, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int32_4> {};
        template <int __bond_dummy> struct field<10, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int32_max> {};
        template <int __bond_dummy> struct field<11, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int64_4> {};
        template <int __bond_dummy> struct field<12, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int64_max> {};
        template <int __bond_dummy> struct field<13, __bond_dummy> : ::bond::detail::mpl::identity<var::m_uint8_2> {};
        template <int __bond_dummy> struct field<14, __bond_dummy> : ::bond::detail::mpl::identity<var::m_uint8_3> {};
        template <int __bond_dummy> struct field<15, __bond_dummy> : ::bond::detail::mpl::identity<var::m_uint16_2> {};
        template <int __bond_dummy> struct field<16, __bond_dummy> : ::bond::detail::mpl::identity<var::m_uint16_3> {};
        template <int __bond_dummy> struct field<17, __bond_dummy> : ::bond::detail::mpl::identity<var::m_uint32_3> {};
        template <int __bond_dummy> struct field<18, __bond_dummy> : ::bond::detail::mpl::identity<var::m_uint32_max> {};
        template <int __bond_dummy> struct field<19, __bond_dummy> : ::bond::detail::mpl::identity<var::m_uint64_3> {};
        template <int __bond_dummy> struct field<20, __bond_dummy> : ::bond::detail::mpl::identity<var::m_uint64_max> {};
        template <int __bond_dummy> struct field<21, __bond_dummy> : ::bond::detail::mpl::identity<var::m_double_3> {};
        template <int __bond_dummy> struct field<22, __bond_dummy> : ::bond::detail::mpl::identity<var::m_double_4> {};
        template <int __bond_dummy> struct field<23, __bond_dummy> : ::bond::detail::mpl::identity<var::m_double_5> {};
        template <int __bond_dummy> struct field<24, __bond_dummy> : ::bond::detail::mpl::identity<var::m_float_3> {};
        template <int __bond_dummy> struct field<25, __bond_dummy> : ::bond::detail::mpl::identity<var::m_float_4> {};
        template <int __bond_dummy> struct field<26, __bond_dummy> : ::bond::detail::mpl::identity<var::m_float_7> {};
        template <int __bond_dummy> struct field<27, __bond_dummy> : ::bond::detail::mpl::identity<var::m_enum1> {};
        template <int __bond_dummy> struct field<28, __bond_dummy> : ::bond::detail::mpl::identity<var::m_enum2> {};
        template <int __bond_dummy> struct field<29, __bond_dummy> : ::bond::detail::mpl::identity<var::m_enum3> {};
        template <int __bond_dummy> struct field<30, __bond_dummy> : ::bond::detail::mpl::identity<var::m_enum_int32min> {};
        template <int __bond_dummy> struct field<31, __bond_dummy> : ::bond::detail::mpl::identity<var::m_enum_int32max> {};
        template <int __bond_dummy> struct field<32, __bond_dummy> : ::bond::detail::mpl::identity<var::m_enum_uint32_min> {};
        template <int __bond_dummy> struct field<33, __bond_dummy> : ::bond::detail::mpl::identity<var::m_enum_uint32_max> {};
        template <int __bond_dummy> struct field<34, __bond_dummy> : ::bond::detail::mpl::identity<var::m_wstr_1> {};
        template <int __bond_dummy> struct field<35, __bond_dummy> : ::bond::detail::mpl::identity<var::m_wstr_2> {};
        template <int __bond_dummy> struct field<36, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int64_neg_hex> {};
        template <int __bond_dummy> struct field<37, __bond_dummy> : ::bond::detail::mpl::identity<var::m_int64_neg_oct> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("Foo", "tests.Foo",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace tests

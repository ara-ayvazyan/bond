
#pragma once

#include "complex_types_types.h"
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
        

        public: struct var
        {};

        using field_count = std::integral_constant<uint16_t, 0>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("Foo", "tests.Foo",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    //
    // ComplexTypes
    //
    struct ComplexTypes::Schema
    {
        typedef ::bond::no_base base;

        static const ::bond::Metadata metadata;
        
        private: static const ::bond::Metadata s_li8_metadata;
        private: static const ::bond::Metadata s_sb_metadata;
        private: static const ::bond::Metadata s_vb_metadata;
        private: static const ::bond::Metadata s_nf_metadata;
        private: static const ::bond::Metadata s_msws_metadata;
        private: static const ::bond::Metadata s_bfoo_metadata;
        private: static const ::bond::Metadata s_m_metadata;

        public: struct var
        {
            // li8
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                ComplexTypes,
                std::list<int8_t, typename std::allocator_traits<arena>::template rebind_alloc<int8_t> >,
                &ComplexTypes::li8,
                &s_li8_metadata
            > {} li8;
        
            // sb
            typedef struct : ::bond::reflection::FieldTemplate<
                1,
                1,
                ::bond::reflection::optional_field_modifier,
                ComplexTypes,
                std::set<bool, std::less<bool>, typename std::allocator_traits<arena>::template rebind_alloc<bool> >,
                &ComplexTypes::sb,
                &s_sb_metadata
            > {} sb;
        
            // vb
            typedef struct : ::bond::reflection::FieldTemplate<
                2,
                2,
                ::bond::reflection::optional_field_modifier,
                ComplexTypes,
                std::vector< ::bond::blob, typename std::allocator_traits<arena>::template rebind_alloc< ::bond::blob> >,
                &ComplexTypes::vb,
                &s_vb_metadata
            > {} vb;
        
            // nf
            typedef struct : ::bond::reflection::FieldTemplate<
                3,
                3,
                ::bond::reflection::optional_field_modifier,
                ComplexTypes,
                ::bond::nullable<float>,
                &ComplexTypes::nf,
                &s_nf_metadata
            > {} nf;
        
            // msws
            typedef struct : ::bond::reflection::FieldTemplate<
                4,
                4,
                ::bond::reflection::optional_field_modifier,
                ComplexTypes,
                std::map<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, std::basic_string<wchar_t, std::char_traits<wchar_t>, typename std::allocator_traits<arena>::template rebind_alloc<wchar_t> >, std::less<std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> > >, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<arena>::template rebind_alloc<char> >, std::basic_string<wchar_t, std::char_traits<wchar_t>, typename std::allocator_traits<arena>::template rebind_alloc<wchar_t> > > > >,
                &ComplexTypes::msws,
                &s_msws_metadata
            > {} msws;
        
            // bfoo
            typedef struct : ::bond::reflection::FieldTemplate<
                5,
                5,
                ::bond::reflection::optional_field_modifier,
                ComplexTypes,
                ::bond::bonded< ::tests::Foo>,
                &ComplexTypes::bfoo,
                &s_bfoo_metadata
            > {} bfoo;
        
            // m
            typedef struct : ::bond::reflection::FieldTemplate<
                6,
                6,
                ::bond::reflection::optional_field_modifier,
                ComplexTypes,
                std::map<double, std::list<std::vector< ::bond::nullable< ::bond::bonded< ::tests::Bar> >, typename std::allocator_traits<arena>::template rebind_alloc< ::bond::nullable< ::bond::bonded< ::tests::Bar> > > >, typename std::allocator_traits<arena>::template rebind_alloc<std::vector< ::bond::nullable< ::bond::bonded< ::tests::Bar> >, typename std::allocator_traits<arena>::template rebind_alloc< ::bond::nullable< ::bond::bonded< ::tests::Bar> > > > > >, std::less<double>, typename std::allocator_traits<arena>::template rebind_alloc<std::pair<const double, std::list<std::vector< ::bond::nullable< ::bond::bonded< ::tests::Bar> >, typename std::allocator_traits<arena>::template rebind_alloc< ::bond::nullable< ::bond::bonded< ::tests::Bar> > > >, typename std::allocator_traits<arena>::template rebind_alloc<std::vector< ::bond::nullable< ::bond::bonded< ::tests::Bar> >, typename std::allocator_traits<arena>::template rebind_alloc< ::bond::nullable< ::bond::bonded< ::tests::Bar> > > > > > > > >,
                &ComplexTypes::m,
                &s_m_metadata
            > {} m;
        };

        using field_count = std::integral_constant<uint16_t, 7>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::li8> {};
        template <int __bond_dummy> struct field<1, __bond_dummy> : ::bond::detail::mpl::identity<var::sb> {};
        template <int __bond_dummy> struct field<2, __bond_dummy> : ::bond::detail::mpl::identity<var::vb> {};
        template <int __bond_dummy> struct field<3, __bond_dummy> : ::bond::detail::mpl::identity<var::nf> {};
        template <int __bond_dummy> struct field<4, __bond_dummy> : ::bond::detail::mpl::identity<var::msws> {};
        template <int __bond_dummy> struct field<5, __bond_dummy> : ::bond::detail::mpl::identity<var::bfoo> {};
        template <int __bond_dummy> struct field<6, __bond_dummy> : ::bond::detail::mpl::identity<var::m> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("ComplexTypes", "tests.ComplexTypes",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace tests
